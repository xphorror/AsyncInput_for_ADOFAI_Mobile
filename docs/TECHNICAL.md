# 技术解析

## 设计目标

Android 版 Unity/IL2CPP 在高负载或 FPS 波动时，普通输入路径会把输入消费绑定到帧循环。这个项目的目标是保留官方判定状态机，同时把输入事件的采集时间、排序和 replay tick 从 Unity 帧率中分离。

当前稳定路线不是重写判定系统，而是：

```text
Android MotionEvent/KeyEvent
        ↓
native ingress thread
        ↓
raw_ns -> DateTime-like eventTick
        ↓
AsyncInputManager six masks
        ↓
scrController.ProcessKeyInputs(eventTick)
        ↓
official Simulated_PlayerControl_Update(targetTick)
```

## 为什么仍走官方判定

官方状态机包含 floor advance、hold、multipress、fail、conditional effect、AUTO/oldAuto 等大量副作用。直接在 native 线程替换 hit/miss 判定会带来较高一致性风险。

因此当前版本只把输入时间戳和 replay 入口异步化，判定仍交给官方 `ProcessKeyInputs(eventTick)`。

`official_judgement.c` 是 trace/audit model：它用于对照 `GetHitMargin` 公式和部分状态推进，不是默认运行时判定真源。

## Hook 点

当前主要 hook：

- `AsyncInputManager.get_isActive`
- `AsyncInputUtils.UpdateOffsetTime(long)`
- `scrController.PlayerControl_Update`
- `scrController.Fail2Action`
- 若干用于 trace/audit、AUTO debug path 和 Android 行为修正的辅助 hook

禁止 hook：

- `scrController.UpdateInput`

原因是目标 Android IL2CPP 产物中该函数是 4 字节 `ret` stub。inline hook 需要覆盖超过一条 AArch64 指令，可能污染紧邻函数的 prologue。当前实现改用 `PlayerControl_Update` 作为稳定必达驱动点。

## 时间域

官方 async 路径使用 DateTime-like tick：

```text
targetSongTick = eventTick - AsyncInputManager.offsetTick
offsetTick = asyncNowTick - dspTime * 10000000
```

因此传给 `ProcessKeyInputs(eventTick)` 的值不能是裸 DSP tick，也不能是 Android uptime tick。native 输入线程将 Android raw event time 映射到 DateTime-like tick，并在主线程覆盖：

- `AsyncInputManager.currFrameTick`
- `AsyncInputManager.prevFrameTick`
- `AsyncInputManager.offsetTick`
- `AsyncInputManager.offsetTickUpdated`

当前实现按 PC-like 方式维护 `offsetTick`：`fixDivider <= 1` 或 session hard reset 时直接对齐，其余帧按 `delta / fixDivider` 平滑更新。这样避免每帧硬覆盖导致的时间桥抖动，也避免 gameplay 判定 tick 被 lifecycle soft pause 的虚拟时钟基准影响。

### 换算原点必须持续对齐

事件 tick 和帧 tick 来自两个不同的系统时钟，靠一个缓存原点桥接：

| 量 | 时钟源 |
|---|---|
| 帧 tick（`currFrameTick` / `offsetTick`） | `CLOCK_REALTIME` |
| 事件 tick（Android 输入事件时间戳） | `CLOCK_MONOTONIC`，经 `wall - uptime` 原点换算 |

Android 上 `CLOCK_MONOTONIC` 在系统 suspend 期间停走，而 `CLOCK_REALTIME` 继续走。因此**每次设备挂起都会让这个原点失效，失效量恰好等于挂起时长**。原点若只在启动时算一次，锁屏再解锁后 `eventTick` 会整体落后，`eventTick - offsetTick` 进入错误 song time，`AdjustAngle` 把行星角度投影回挂起前，判定严重错乱。

保持两者同域不是一次性初始化，而是持续义务。实现为两级：

1. **边界强制重同步** — 输入线程处理 lifecycle reset 与 soft resume 时重新采样原点。此刻队列已清、capture gate 已关，换原点不会误换算在飞事件。
2. **每帧漂移自愈** — 主线程每帧比对原点，漂移超 20 ms 才触发。覆盖不经生命周期回调的 doze、歌曲中途的 `CLOCK_REALTIME` 阶跃，以及 soft resume 命令不等 ack 时主线程抢先跑一帧的窗口。

原点采样用 `uptime -> wall -> uptime` 三明治，两次 uptime 间隔过大判为被抢占并丢弃该次采样，避免调度延迟被误判成真实漂移。

原点一旦移动即视为时钟不连续，会清空事件队列并强制 `offsetTick` hard reset —— 队列里每个事件携带的是入队时冻结的旧 `offsetTick` 快照，跨越不连续后不再可用。日志关键字为 `CLOCK_ORIGIN_RESYNC` 和 `CLOCK_ORIGIN_DRIFT`。

官方 `scrPlanet.SwitchChosen()` 的命中判定读取 `cachedAngle`。mask replay 在调用官方判定前会把 `angle` 和 `cachedAngle` 同时投影到当前 `eventTick`，保证 `GetHitMargin(cachedAngle, targetExitAngle, ...)` 看到的是同一个目标时间点。

## 输入 gate

`AsyncInputManager.isActive` 不是简单等于“模块开关开启”。它只在 gameplay replay-ready 状态返回 true。

主开关区分用户请求态与运行态。Hook 事务完成前，开启请求会被保留但运行态保持关闭；安装完成后再统一恢复持久化设置，避免启动竞态让输入提前进入未完整的 Hook 链。

gate 关闭时：

- 不向 native replay queue 注入 gameplay 输入；
- 恢复官方 regular keyboard input type；
- 清理 async mask；
- 避免菜单、编辑态和非消费态输入被 async 路径污染。

关卡编辑器需要区分编辑态和播放/测试态：编辑态不接管；播放/测试态允许 async replay。

暂停态也属于 gate 关闭条件。`scrController._paused` 为 true 时，native 会关闭 capture 并让 Java `dispatchTouchEvent/dispatchKeyEvent` 继续走 Unity 原始路径，避免暂停菜单和设置界面被 async gameplay 管线吞掉。

controller gate 同时读取实时 `gameworld`、当前状态和 MonsterLove 状态机的 destination state。`gameworld=false`、当前状态离开 `PlayerControl`，或目标状态开始切向 Won/Fail/其他状态时立即关闭 capture。目标状态不可读时仅回退到当前状态判断。DLC 标志保留为诊断信息，不参与 gate。

进入 `Fail2Action` 前会清空异步队列、mask 和 capture，使死亡后的下一次输入回到官方 `Fail2` 更新路径。这样编辑器测试关卡仍执行官方重试，内置关卡和分段小关也能执行各自的官方完成/切换语义。

## Mask replay

当前不伪造 Unity `Input.touches`，只写官方 async mask：

- `AsyncInputManager.keyMask`
- `AsyncInputManager.keyDownMask`
- `AsyncInputManager.keyUpMask`
- `AsyncInputManager.frameDependentKeyMask`
- `AsyncInputManager.frameDependentKeyDownMask`
- `AsyncInputManager.frameDependentKeyUpMask`

多指输入使用稳定 slot，不再折叠成单个 Space。近同时 DOWN 会在已经到期且已经进入队列的范围内合并，但不会为了等待第二指而阻塞第一个 DOWN。

### AUTO 一次性提交

自动砖仍通过官方 `Hit(isAuto)` 路径提交。兼容层为一次 replay 建立局部事务，嵌套的第一个 `Hit(isAuto)` 获得唯一提交权并调用 original，随后立即清除本次 synthetic down/held，防止同一输入在下一个小关再次触发。外层返回 original 的真实结果；若官方状态机没有产生提交，则记录警告并走原始回退路径。

## 队列和生命周期

Java callback 只负责把 raw event 转交 native。native ingress thread 串行处理输入事件、reset、soft pause 和 soft resume。普通事件和控制命令使用独立容量的队列，再按全局 `seq` 合并消费；输入洪峰只会淘汰最旧普通事件，不会覆盖生命周期命令。

DOWN/UP 发布后，在输入线程健康时最多等待 2 ms 确认该序列已完成 ingress 消费。超时不会删除或重发事件，记录仍由 worker 稍后处理；超时日志每秒最多输出一次。

进入新 gameplay capture 时会清理旧队列并重建虚拟时钟基准；soft pause/resume 只在暂停前确实处于 gameplay capture 时复用冻结时钟。注意 `onPause` 与 `onWindowFocusChanged(false)` 通常会连发两次 soft pause，"暂停前是否在 capture"的标记因此只置位、不被第二次覆写，清零交给 soft resume 和 reset。

soft resume 还会强制重同步时间换算原点，详见「时间域 / 换算原点必须持续对齐」。

capture 不再根据“主线程超过固定时间没有刷新”推断会话失效。Unity 主线程短暂停顿期间保留输入状态，capture 只在可证明的场景、状态、暂停、失焦、重置或显式关闭边界结束。

Java 层必须使用 native 返回值作为消费信号：

- native 返回 `true`：事件已进入 async gameplay 管线，Java 不应继续调用 Unity 原始输入路径。
- native 返回 `false`：事件不是 async gameplay 输入，或者当前处于暂停/UI/非 capture 状态，Java 应继续交给 `super.dispatchTouchEvent` / `super.dispatchKeyEvent`。

真实触摸在 replay 时还需要屏蔽官方 mobile `touchEnabled` 分支，否则同一个触摸可能同时被 `Input.touches` 和 async mask 计数。当前 hook 只在 mask replay 范围内让 `scrPlayer.get_touchEnabled` 返回 false，不影响菜单和普通官方输入路径。

## 测试宏

测试宏通过导出函数控制：

- `ADOFAIAsyncInput_SetTestMacroEnabled(int enabled)`
- `ADOFAIAsyncInput_IsTestMacroEnabled()`

它用于验证内部异步链路：

```text
target floor -> synthetic raw_ns -> eventTick -> queue -> mask -> ProcessKeyInputs
```

测试宏不是完整真实触摸基准。它不会覆盖 Android 触摸采样、Java/Native 转发、系统输入调度或多指硬件行为；这些仍需要真实触摸测试。宏稳定只能说明内部 replay/判定链路稳定。

为避免污染基准，测试宏只在当前队列空、没有 held source、测试宏源未按住时投递下一击。测试宏启用且 gameplay gate 打开时，玩家触摸/键盘 gameplay 输入会被消费掉，不进入 async 队列，也不继续转发给 Unity。移动端 UI 区域触摸仍会透传给菜单/暂停界面。

## Trace / Audit

高频 trace 默认关闭，可通过配置或外部菜单调用导出函数开启：

- `ADOFAIAsyncInput_SetTraceEnabled(int enabled)`
- `ADOFAIAsyncInput_IsTraceEnabled()`

trace 包括：

- replay event tick
- mask edge
- official hit margin 出口
- shadow/audit model 对照
- AUTO/oldAuto debug path

这些 trace 只用于诊断和回归，不改变默认判定真源。

## Java 转发示例

`java/com/fizzd/connectedworlds/editorport/ExtraMenuUnityPlayerActivity.java` 演示了最小转发方式：

- `onCreate` 将 `getFilesDir().getAbsolutePath()` 配置给 native；
- `dispatchTouchEvent` 转发 `MotionEvent`
- `dispatchKeyEvent` 转发 `KeyEvent`
- `onPause/onResume/onWindowFocusChanged` 转发生命周期边界

如果你的 Activity 包名或类名不同，需要同步修改 native JNI 导出名，或添加自己的 JNI bridge。

## 当前未解决问题

- 消费端仍在 Unity 主线程，不能保证主线程长卡顿时实时反馈。
- DLC 与 HOLD 的组合仍需要按目标设备和关卡做专项行为验证。
- pause barrier、snapshot history、健康指标和系统化 Baseline/Stress 压力矩阵仍是后续工作。
- `official_judgement.c` 尚未完整覆盖官方所有状态推进分支。
