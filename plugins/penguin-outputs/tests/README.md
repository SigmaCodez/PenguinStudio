# Offline output safety checks

Configure the normal Linux build with `-DPENGUIN_BUILD_TESTS=ON`, then build and run:

```sh
cmake --build build --target penguin-output-safety
build/plugins/penguin-outputs/penguin-output-safety
```

The test compiles the production implementation and substitutes only the OBS boundary calls used by the checks. No account, stream key or network service is needed. It verifies:

- RTMP(S) endpoint validation rejects malformed URLs, credentials, query parameters and fragments.
- Starting without a key or an installed H.264 encoder fails preflight.
- Saved destination JSON does not contain the session stream key.
- Failed, active and already-stopped outputs release their resources in the correct order. A failed start must never force-stop an uninitialized backend, which can deadlock libobs.

These checks do not establish successful encoding, live connectivity, hardware encoder availability or Wayland capture. Those require integration testing of the built application. For a local RTMP integration test, use a loopback-only receiver, for example FFmpeg with `-listen 1 -i rtmp://127.0.0.1:19350/live/test -c copy local-test.flv`, then configure the application with server `rtmp://127.0.0.1:19350/live` and key `test`. Run a second receiver on a different port for a simultaneous portrait stream. Inspect the resulting files with `ffprobe`, including resolution, audio and duration, and verify stop/restart, connection failure, reconnect, profile changes and shutdown.

`penguin-wizard-lifecycle` is an additional offline Qt regression check. It opens
the production wizard asynchronously, destroys its parent while it remains open,
checks frontend shutdown cancellation, prevents duplicate dialogs, and verifies
that a normal first-run dismissal is remembered. It uses synthetic configuration
and no encoder, OBS session, display server or network connection.

```sh
cmake --build build --target penguin-wizard-lifecycle
QT_QPA_PLATFORM=offscreen build/plugins/penguin-outputs/penguin-wizard-lifecycle
```

The optional `penguin-local-stream` target automates an integration check against an installed build. It requires `ffmpeg`, `ffprobe` and access to a Wayland or X11 display. It renders an artificial color source, starts two loopback receivers, checks 1280 × 720 and 1080 × 1920 H.264/AAC files, exercises crop and fit, stops and restarts both streams, destroys active outputs, interrupts a receiver to check automatic reconnect, and checks an unreachable receiver. It also saves screenshots of the production setup wizard. It does not capture the desktop or use a streaming account.

```sh
cmake --build build --target penguin-local-stream
QT_QPA_PLATFORM=offscreen build/plugins/penguin-outputs/penguin-local-stream /app /tmp/penguin-stream-results
```

Replace `/app` with the installed build's prefix. The harness uses the OBS 33 `obs-modules/core` layout and accepts both `lib` and Ubuntu's `lib/x86_64-linux-gnu` library directory. It prefers native Wayland rendering and reports any fallback to X11. Passing this check does not establish portal screen-capture compatibility, platform OAuth behavior or real platform acceptance.
