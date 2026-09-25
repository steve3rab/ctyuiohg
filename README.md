# Creo JavaFX integration

## Runtime baseline

- Java / JavaFX: **17**
- Native integration: C++17 + JNI
- JNI runtime contract: `JNI_VERSION_10`
- JavaFX UI work runs on the JavaFX Application Thread.
- Heavy Creo / Pro/TOOLKIT work must not run on the JavaFX Application Thread.

## User experience

The integration shows a lightweight loading view immediately when a window is requested. Accepted actions switch to a processing view while the native Creo-side operation runs. The modal Creo host remains blocked until processing completes successfully or fails.

The JavaFX launcher is intentionally small. Reusable loading/processing/error view construction is kept in `ArchiJavaFxViews.java`; lifecycle and JNI responsibilities remain in `ArchiCreoJniLauncher.java`.

## Processing contract

After receiving an accepted `JavaFxResult`, the Creo-side handler performs its operation and must call:

```cpp
ArchiJavaFxService::instance().completeProcessing(
    result.requestId,
    true);
```

On failure:

```cpp
ArchiJavaFxService::instance().completeProcessing(
    result.requestId,
    false,
    "User-facing error message");
```

This keeps the JavaFX window responsive and gives the user explicit feedback instead of making the window appear frozen.
