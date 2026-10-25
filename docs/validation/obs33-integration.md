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

Branding (77 locales), Penguin locale completeness, release-set selection, nine version-configuration cases and nested task queue regression checks passed. The local ALSA test could not compile because the host lacks SIMDe development headers; it must run in the configured build environment. Complete builds, runtime regression, real current English previews, package verification and publication remain pending. No claim of zero errors or performance improvement is made.
