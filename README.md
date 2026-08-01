# ADOFAI Android Async Input

Android `arm64-v8a` input bridge for the IL2CPP build of A Dance of Fire and Ice 3.1.2.

The runtime timestamps touch and keyboard events outside Unity's frame loop, maps them to game ticks, and replays them through the game's async input masks and `ProcessKeyInputs(eventTick)` path. The official judgement state machine remains the source of truth for hit and miss decisions.

## Current behavior

- Forwards Android `dispatchTouchEvent` and `dispatchKeyEvent` events to native code.
- Uses one native ingress thread and stable async key slots for multi-touch input.
- Releases input to Unity while gameplay capture is inactive, including pause and menu states.
- Resynchronizes the monotonic-to-wall-clock origin after device suspend and resume.
- Supports `arm64-v8a` only.
- Targets ADOFAI 3.1.2 Android IL2CPP; other game versions require separate metadata and behavior validation.

## Native consumer ABI

`include/async_input_observer_abi.h` defines the versioned raw touch and keyboard observer ABI. Consumers register with the exported `ADOFAIAsyncInput_RegisterRawObserverV1` symbol and must validate `struct_size` and `abi_version` before reading event data.

The exported `ADOFAIAsyncInputGetIl2CppHandleV1` function exposes AsyncInput's existing validated `libil2cpp.so` handle to approved in-process consumers. Its pointer is published in the app files directory for discovery. The provider can return `NULL` until AsyncInput has initialized IL2CPP, so consumers must retry instead of assuming immediate availability.

## Dependencies

- Windows PowerShell
- Android NDK r25 or a compatible version
- An `arm64-v8a` Dobby static library named `libdobby.a`

The repository includes the Dobby public header under `include/`, but not the static library.

## Build

```powershell
.\build.ps1 `
  -NdkRoot "C:\Android\Sdk\ndk\25.2.9519653" `
  -DobbyRoot "C:\deps\Dobby" `
  -PackageName "com.fizzd.connectedworlds.leveleditor.debug"
```

Output:

```text
out/arm64-v8a/libAsyncInput.so
```

The build verifies that both native consumer ABI symbols are present in `.dynsym`.

To compile the optional Java forwarding example, also pass `-CompileJavaExample`, `-AndroidJar`, and `-UnityClassesJar`. The example source is under `java/`.

See [docs/TECHNICAL.md](docs/TECHNICAL.md) for implementation details.

## License

MIT. See [LICENSE](LICENSE).
