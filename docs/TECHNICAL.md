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

## 输入 gate

`AsyncInputManager.isActive` 不是简单等于“模块开关开启”。它只在 gameplay replay-ready 状态返回 true。

gate 关闭时：

- 不向 native replay queue 注入 gameplay 输入；
- 恢复官方 regular keyboard input type；
- 清理 async mask；
- 避免菜单、编辑态和非消费态输入被 async 路径污染。

关卡编辑器需要区分编辑态和播放/测试态：编辑态不接管；播放/测试态允许 async replay。

暂停态也属于 gate 关闭条件。`scrController._paused` 为 true 时，native 会关闭 capture 并让 Java `dispatchTouchEvent/dispatchKeyEvent` 继续走 Unity 原始路径，避免暂停菜单和设置界面被 async gameplay 管线吞掉。

## Mask replay

当前不伪造 Unity `Input.touches`，只写官方 async mask：

- `AsyncInputManager.keyMask`
- `AsyncInputManager.keyDownMask`
- `AsyncInputManager.keyUpMask`
- `AsyncInputManager.frameDependentKeyMask`
- `AsyncInputManager.frameDependentKeyDownMask`
- `AsyncInputManager.frameDependentKeyUpMask`

多指输入使用稳定 slot，不再折叠成单个 Space。近同时 DOWN 会在已经到期且已经进入队列的范围内合并，但不会为了等待第二指而阻塞第一个 DOWN。

## 队列和生命周期

Java callback 只负责把 raw event 转交 native。native ingress thread 串行处理输入事件、reset、soft pause 和 soft resume。

进入新 gameplay capture 时会清理旧队列并重建虚拟时钟基准；soft pause/resume 只在暂停前确实处于 gameplay capture 时复用冻结时钟。

capture gate 有 stale timeout。如果 `PlayerControl_Update` 不再刷新 gate，native 会关闭 capture、清队列并恢复 regular input type，避免继续吞正常输入。

Java 层必须使用 native 返回值作为消费信号：

- native 返回 `true`：事件已进入 async gameplay 管线，Java 不应继续调用 Unity 原始输入路径。
- native 返回 `false`：事件不是 async gameplay 输入，或者当前处于暂停/UI/非 capture 状态，Java 应继续交给 `super.dispatchTouchEvent` / `super.dispatchKeyEvent`。

真实触摸在 replay 时还需要屏蔽官方 mobile `touchEnabled` 分支，否则同一个触摸可能同时被 `Input.touches` 和 async mask 计数。当前 hook 只在 mask replay 范围内让 `scrPlayer.get_touchEnabled` 返回 false，不影响菜单和普通官方输入路径。

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

- `dispatchTouchEvent` 转发 `MotionEvent`
- `dispatchKeyEvent` 转发 `KeyEvent`
- `onPause/onResume/onWindowFocusChanged` 转发生命周期边界

如果你的 Activity 包名或类名不同，需要同步修改 native JNI 导出名，或添加自己的 JNI bridge。

## 当前未解决问题

- 消费端仍在 Unity 主线程，不能保证主线程长卡顿时实时反馈。
- DLC async 兼容性尚未完整审计，当前使用 fuse 保护。
- pause barrier、snapshot history、健康指标和系统化 Baseline/Stress 压力矩阵仍是后续工作。
- `official_judgement.c` 尚未完整覆盖官方所有状态推进分支。
