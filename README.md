# ADOFAI Android 异步输入

面向《冰与火之舞》3.1.2 Android IL2CPP 版本的 `arm64-v8a` 异步输入桥。

运行时在 Unity 帧循环之外采集触摸与键盘事件，将 Android 单调时钟时间映射到游戏 tick，再通过官方异步输入 mask 和 `ProcessKeyInputs(eventTick)` 回放。命中、失误、长按与多押等结果仍由游戏原有判定状态机决定。

## 当前能力

- 转发 Android `dispatchTouchEvent` 和 `dispatchKeyEvent` 事件。
- 使用独立 native ingress 线程和稳定按键槽支持多点触控。
- 将输入事件与 reset、pause、resume 控制命令按全局序列处理。
- 在暂停、菜单、编辑态、完成态、失败态和 freeroam 边界及时把输入交还 Unity。
- 在设备挂起和恢复后重新同步单调时钟与墙上时钟原点。
- 支持 DLC 场景，并为混合小关 AUTO 路径提供一次性提交事务。
- 支持通过运行时 `filesDir` 配置适配不同应用包名。
- 仅支持 `arm64-v8a`。
- 目标版本为 ADOFAI 3.1.2 Android IL2CPP；其他游戏版本需要重新验证元数据和行为。

## Native 消费端 ABI

`include/async_input_observer_abi.h` 定义了带版本的原始触摸与键盘观察者 ABI。消费端通过导出符号 `ADOFAIAsyncInput_RegisterRawObserverV1` 注册，并在读取事件前校验 `struct_size` 和 `abi_version`。

`ADOFAIAsyncInputGetIl2CppHandleV1` 导出 AsyncInput 已验证的 `libil2cpp.so` 句柄。提供者地址会发布到应用 `filesDir`，初始化完成前可能返回 `NULL`，消费端必须允许重试。

## 依赖

- Windows PowerShell
- Android NDK r25 或兼容版本
- `arm64-v8a` Dobby 静态库 `libdobby.a`
- 可选：用于运行纯 C 回归测试的 GCC

仓库包含 Dobby 公共头文件，但不包含静态库。

## 构建

```powershell
.\build.ps1 `
  -NdkRoot "C:\Android\Sdk\ndk\25.2.9519653" `
  -DobbyRoot "C:\deps\Dobby" `
  -AndroidApi 25
```

输出：

```text
out/arm64-v8a/libAsyncInput.so
```

构建脚本会检查 raw observer、IL2CPP handle provider 和 `filesDir` JNI bridge 是否存在于 `.dynsym`。

若要同时编译 Java 转发示例，增加 `-CompileJavaExample`、`-AndroidJar` 和 `-UnityClassesJar`。示例位于 `java/`。

## 接入要求

Activity 加载 `libAsyncInput.so` 后，必须把 `getFilesDir().getAbsolutePath()` 传给 `nativeConfigureAsyncInputFilesDir`。配置完成前 native patch 线程会等待，不读取固定包名路径。

Java 分发层必须使用 native 返回值决定是否消费事件：返回 `true` 时不再调用 Unity 原始输入路径，返回 `false` 时继续调用父类分发。

## 测试

```powershell
.\tests\run_tests.ps1 -GccPath "C:\toolchain\bin\gcc.exe"
```

测试覆盖 ingress 控制命令隔离、运行状态恢复、完成态与 freeroam Gate、AUTO 单次提交事务。

更多实现细节见 [docs/TECHNICAL.md](docs/TECHNICAL.md)。

## 许可证

MIT，详见 [LICENSE](LICENSE)。
