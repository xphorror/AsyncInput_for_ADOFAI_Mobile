# ADOFAI Android Async Input

这是一个面向 Android IL2CPP 版 A Dance of Fire and Ice 的异步输入实验实现。

项目目标是把触摸/键盘事件的时间戳从 Unity 帧循环中分离出来，再把事件按原始时间映射为游戏可接受的 `eventTick`，通过官方 async mask 和 `ProcessKeyInputs(eventTick)` 进入官方判定状态机。

当前运行时仍以官方判定链路为唯一真源：本项目负责输入采集、时间映射、mask replay 和必要的 Android IL2CPP hook，不在默认运行路径中替换官方 hit/miss 判定。

## 当前状态

- 已支持 Android `dispatchTouchEvent` / `dispatchKeyEvent` 输入转发。
- Java 转发层必须尊重 native 返回值：返回 `true` 表示事件已经被 async gameplay 管线消费，不应再交给 Unity 原始输入路径。
- 已实现 native ingress 线程、虚拟 DateTime-like tick、`currFrameTick/offsetTick` 覆盖、六个 async mask replay。
- 主 hook 点是 `scrController.PlayerControl_Update`，不会 inline hook Android IL2CPP 中 4 字节 `ret` stub 的 `scrController.UpdateInput`。
- 多指和 multitap 使用稳定 async key slot，不再把所有物理输入折叠为单个 Space。
- 暂停菜单/非 gameplay 状态会关闭 capture 并放行原始触摸，避免 async 管线吞掉 UI 事件。
- `official_judgement.c` 保留为 trace/audit model，用于对照官方公式和状态推进，不是默认运行时判定接管方案。
- AUTO/oldAuto 路径保留为 debug/test path，用于回归测试 async 状态机链路。
- 内置异步测试宏可用于压测 `eventTick -> mask -> ProcessKeyInputs` 链路。测试宏启用且处于播放态时会屏蔽玩家 gameplay 输入，避免人工触摸污染基准样本。

## 限制

- 当前只提供 `arm64-v8a` 构建。
- 当前目标版本是 ADOFAI 3.1.2 Android IL2CPP，其他版本需要重新验证 metadata 名称、字段 offset fallback 和行为差异。
- 消费端仍由 Unity 主线程上的 `PlayerControl_Update` 驱动；输入时间戳已异步化，但主线程长时间卡顿时仍会延迟消费。
- DLC 关卡默认走 fuse 保护，恢复官方输入路径；DLC async 兼容性尚未完整审计。
- 这是研究/移植用工程，不是通用安装器。

## 依赖

- Windows PowerShell
- Android NDK r25 或相近版本
- Dobby 静态库 `libdobby.a`
- 可选：JDK、Android platform `android.jar`、Unity `classes.jar`，用于编译 Java 转发示例。

仓库已包含 Dobby 头文件：

```text
include/dobby.h
```

构建时仍需要自行提供与目标 ABI 匹配的 `libdobby.a`。

## 编译 native so

```powershell
.\build.ps1 `
  -NdkRoot "C:\Android\Sdk\ndk\25.2.9519653" `
  -DobbyRoot "C:\deps\Dobby" `
  -PackageName "com.fizzd.connectedworlds.leveleditor.debug"
```

`DobbyRoot` 目录需要包含：

```text
libdobby.a
```

输出：

```text
out/arm64-v8a/libAsyncInput.so
```

`PackageName` 会写入 native 配置文件路径：

```text
/data/data/<PackageName>/files/adofai_async_input.cfg
/data/data/<PackageName>/files/adofai_async_auto_replay.cfg
/data/data/<PackageName>/files/adofai_async_trace.cfg
/data/data/<PackageName>/files/adofai_async_test_macro.cfg
```

## 编译 Java 转发示例

默认脚本只编译 native `.so`。如果需要同时编译 Java 示例：

```powershell
.\build.ps1 `
  -NdkRoot "C:\Android\Sdk\ndk\25.2.9519653" `
  -DobbyRoot "C:\deps\Dobby" `
  -PackageName "com.fizzd.connectedworlds.leveleditor.debug" `
  -CompileJavaExample `
  -AndroidJar "C:\Android\Sdk\platforms\android-36\android.jar" `
  -UnityClassesJar "C:\Unity\Editor\Data\PlaybackEngines\AndroidPlayer\Variations\il2cpp\Release\Classes\classes.jar"
```

输出：

```text
out/java_classes/
```

Java 示例在：

```text
java/com/fizzd/connectedworlds/editorport/ExtraMenuUnityPlayerActivity.java
```

它只演示如何把 `MotionEvent`、`KeyEvent` 和 lifecycle/focus 边界转发给 native。实际工程可以使用不同 Activity 名称，但 JNI 方法签名和 package/class 名需要与 native 中的导出符号匹配。

## 技术说明

详细实现见 [docs/TECHNICAL.md](docs/TECHNICAL.md)。

## 许可证

MIT，见 [LICENSE](LICENSE)。
