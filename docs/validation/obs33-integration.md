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
