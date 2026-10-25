# OBS 33 integration — work in progress

Penguin Studio candidate: 0.7.0. No release or installation is approved by these preliminary checks.

Upstream baseline changes from `ba2f32bdf791005443988a4955e963663e16b1ed` (32.2.2) to `72e86655715d9a25d69ce446aa4dd44ba58e10d2` (33.0.0). Source changes were imported as a three-way patch, preserving Penguin history and modifications. OBS workflow files are excluded; the Penguin package workflow is maintained separately. Original author, license and source notices are retained. OBS browser was already pinned to the 33.0.0 revision; obs-websocket advances to `e28f84975984340e84fc4879cfc05741cc0afde1`.

## Integration decisions

- Preserve the Penguin window title, profile root, startup dialog and native platform modules.
- Register the custom camera filter under its actual module name in the new core-module registry. Install resources in the OBS 33 core layout and adapt private runtime lookups/RPATH.
- Preserve the ALSA reopen-thread shutdown join and WebSocket frontend-callback lifetime patch.
- Merge upstream locale updates by key, preserving Penguin additions and product identity. Keep the factual OBS 30.2 reference for unsupported Kepler hardware.
- Require FFmpeg 8 and Qt 6.10. Native DEB and AppImage move to Ubuntu 26.04; Snap to core26; Flatpak to Freedesktop 26.08. The old native Ubuntu 24.04 DEB is no longer part of the new package set. Flatpak/Snap remain the intended routes for older hosts, subject to runtime validation.

## Preliminary checks

Branding (77 locales), Penguin locale completeness, release-set selection, nine version-configuration cases and nested task queue regression checks passed. The ALSA reopen-thread shutdown regression passed after extracting SIMDe development headers into a private test directory. GitHub checks also passed the parser sanitizer, audio shutdown, configuration, OAuth and update-selection regressions. The optional AMD runtime package built successfully. Complete builds, runtime regression, real current English previews, package verification and publication remain pending. No claim of zero errors or performance improvement is made.

The new third-party plugin loader uses PenguinStudio user directories, preserving application isolation instead of scanning OBS user plugins implicitly. Shared system ABI paths retain their OBS names.


## Integration failures corrected during candidate builds

- The first Ubuntu, Fedora and Flatpak builds identified the removed legacy image-file API in Penguin camera effects. The filter now uses `gs_image_file_ex_t` and retains straight input alpha. A pixel regression also covers partially transparent replacement images; it passed against 0.6.4 as a reference under native Wayland.
- The corrected Ubuntu application compiled and its camera-effect assertions passed, but strict teardown checks caught a new frontend-tools safe-mode query after frontend callbacks had been destroyed. The module now tracks whether scripting was initialized and frees that state without querying the destroyed frontend. A compiled lifecycle test covers normal mode, safe mode and builds without scripting.
- Snapcraft initially could not resolve the hosted runner's amd64v3-only package indices. The CI-only APT override now runs after the runner's architecture configuration. The following attempt resolved packages and exposed renamed Qt/x264 runtime packages, which were updated after checking the complete stage list against Ubuntu 26.04.
- Fedora built and installed the pre-teardown-fix RPM successfully. This does not replace final-source runtime validation.

## Local reference runs (installed 0.6.4, not candidate approval)

Native Wayland: camera pixel/inspector regression including transparent backgrounds passed with clean teardown. Workspace regression passed 210 checks and produced a decodable video/audio recording. A separate 30-second synthetic GPU camera recording with the existing model cache reported active GPU processing, 901 output frames, no reported render/encoding losses and zero OBS leaks; 900 video frames decoded successfully. A cold-cache probe ended at its 110-second test deadline while the GPU worker was still compiling; the UI remained responsive and shutdown was clean. It is not recorded as a completed cold-cache GPU test.

## Native Ubuntu 26.04.1 Wayland candidate checks

Candidate source `b68bf4340b732ffd7a46a6024503fb8253559078`, Ubuntu package SHA-256
`2066215b067c04a080c1886f0d704411108cd7aaa027c9bdb0197f7ffd7965c0`, extracted with the optional AMD runtime rather than installed over the user's current version. A private mount namespace hides installed legacy core modules to avoid testing a mixture of two engines. Profiles, media and output files are isolated and synthetic.

- Workspace: 210 assertions passed, including Inspector/undo, scene operations, output controls and a real H.264/AAC recording. The 3.233-second file fully decoded (97 video frames, 150 audio frames).
- Camera pixel and Inspector checks passed: shapes, green/blue key, local inference, disabling/serialization, blur, image backgrounds, missing-image fallback, contour cleanup, transparent image alpha, hand recovery and all processing choices.
- RX 9070 XT GPU recording with a pre-existing model cache: GPU active, no CPU fallback, 30.100 seconds, 900 decoded video frames and 1,405 audio frames; 901 OBS output frames, no reported rendering/encoding losses.
- Runtime-unavailable recording: the private AMD runtime was hidden inside the test namespace, GPU selection fell back to CPU, and the 30.100-second H.264/AAC recording fully decoded with no reported rendering/encoding losses.
- All ten shutdown scenarios passed: normal, multi-instance, initial wizard open, connection-wizard cancellation, focused wizard field during shutdown, auto-configuration cancellation, statistics, updater open/focus/close, same-profile restart and SIGTERM. All reported zero startup/shutdown warnings/errors, zero late frontend unregisters and zero OBS leaks. No Qt focus diagnostic was emitted, including at debug level.

These are synthetic regression checks, not a new live Facecam movement acceptance or an account-provider authorization test. The first private fallback harness mistakenly hid the runtime before mounting the root filesystem, leaving the GPU runtime visible; that cold-initialization run was discarded. Its interruption also revealed held references in the private recording helper, which now releases its fixture at the pre-cleanup frontend shutdown event. Neither failed harness run is counted as successful candidate validation.

- Offline browser-source run: all 211 workspace assertions passed, the recording fully decoded and a decoded frame visibly contained the HTML fixture. The only additional diagnostic was `WARNING: radv is not a conformant Vulkan implementation, testing use only.` The same warning reproduced with the installed 0.6.4 browser-source run on this host; it is retained as a graphics-driver limitation, not hidden or counted as a warning-free run.
- Local output integration used the OBS 33 core-module layout and native Wayland EGL. Simultaneous 1280×720/1080×1920 H.264/AAC, portrait crop/fit, stop/restart, reconnect, active-output destruction and unreachable-server cleanup passed. All five output files fully decoded. No account or public stream was used.
- Current English previews were captured from package source `011eb6811ebcec260a1975a96615e30c70a7da8b`, SHA-256 `fa81df0186944c74000ecba1dd4779b7498a8c362875afae22e380057e868a9a`, and all three images were visually reviewed. The package differs from the tested b68 candidate only in subsequent documentation, the Arch minimum-glibc declaration and Plugin Manager help URL.

- AppImage packaging detected another Qt 6.10 migration: the old `libqwayland-egl.so` and `libqwayland-generic.so` platform files have become `libqwayland.so`. The packaging list now targets the installed Qt 6.10 platform plugin; full AppImage/Arch validation is being rerun.

- The final pre-packaging Ubuntu source (`011eb6811`) repeated the 30-second GPU recording successfully: 900 decoded video frames, 1,405 audio frames, GPU active, no CPU fallback, no reported frame-loss events and clean teardown. Interrupting cold GPU preparation with the corrected observing helper also exited normally without diagnostics or OBS leaks.
- An isolated saved 0.6.4 profile opened and closed cleanly in 0.7.0; all four saved scene/source identities and the scene order were retained.
- The locally rebuilt AppImage with the Qt 6.10 plugin correction passed both its X11 graphical smoke test and an isolated native Wayland launch/normal exit. The latter emitted no warning/error diagnostics and reported zero OBS leaks. The local packaging run used privately extracted packaging tools and an already compiled smoke observer; GitHub independently rebuilds the package and observer for release.

## Flatpak runtime checks

The 0.7.0 Flatpak at source `011eb6811` was installed into a separate Flatpak installation with Freedesktop 26.08 and Qt 6.11.1. Native Wayland startup/exit, all 210 workspace assertions and a fully decoded local recording passed after correcting the test launcher: `flatpak run` replaces reserved XDG variables, so the launcher now sets its disposable profile paths inside the sandbox command. The initial runs using external `--env=XDG_CONFIG_HOME` are discarded and are not evidence of profile isolation or recording correctness. The native installation and native profile were not modified.

AJA modules remain enabled as before. This host has no AJA hardware, so those two optional modules report absence and failed initialization; AJA hardware operation is not claimed. Camera pixel/Inspector checks passed in the actual Flatpak runtime, including image alpha and hand-recovery initialization.

Qt 6.11.1 reproduced the null-surface text-input leave diagnostic on dialog teardown. Its tagged upstream source still contains the warning, while [Qt's fix](https://github.com/qt/qtbase/commit/ac8b2e9b40bf19aa889345f7b617392bef906040) explains the destroyed-proxy case as normal teardown. The existing exact diagnostic classifier now additionally recognizes only this confirmed leave message on Qt 6.11.1; the older disable-surface exception remains restricted to Qt 6.10.2. Other versions, categories, severities and non-null surface mismatches retain their original treatment. All 99 classifier checks passed. The message remains available at debug level. Final patched Flatpak runtime validation is pending.

## Main-package-only update compatibility

The in-app updater installs the main package, so an existing 0.6.x AMD runtime add-on may remain installed in its previous private directory. The new module first prefers the OBS 33 layout and then accepts that legacy private runtime location; its worker executable still comes from the new main package. The currently shipped old/new runtimes both use ONNX Runtime 1.23.2. This preserves GPU processing across a main-package-only upgrade; runtime initialization failures still fall back to CPU. Packaged validation of this migration is pending.
