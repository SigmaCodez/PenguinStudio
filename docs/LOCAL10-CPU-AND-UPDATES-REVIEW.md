# Local10 render timing and startup-update review

Candidate only: public version remains 0.7.1. No publication is authorized.
Installed baseline is 0.7.1-1+ubuntu26.04.local9.

## Root cause and correction

The image source called os_stat synchronously from video_tick every second.
With the synthetic image on the project filesystem, tick_sources reached 992.7 ms
in one diagnostic recording. The inference/render stage in that run had a 28.43 ms
99th percentile. This separated filesystem blocking from model execution.

The source now queries timestamps on a private background worker. Only one query
may be pending. Results are accepted only for the current path. The worker owns a
copied path, never an OBS source or graphics object. File changes still trigger the
existing reload behavior; decoding/upload on an actual change is not redesigned.
Hidden sources do not start polling. A worker starts lazily on the first periodic
check, and destruction joins any outstanding query before module unload. An OS
filesystem operation that never returns can still delay destruction; no unsafe
thread cancellation is used.

CPU/GPU inference models, resolutions, masks, scheduling and OpenCV policy remain
unchanged. The rejected scheduling experiments are retained only in temporary
evidence and are excluded from the candidate repository/package.

## Startup updates

The Linux startup check is scheduled after five seconds and is no longer blocked
by a successful check in the past 24 hours. The explicit automatic-update opt-out
and --disable-updater still apply. Active outputs/modal dialogs defer notifications.
Release selection still compares public Penguin versions and refuses same/older
versions, so public 0.7.1 cannot replace a local 0.7.1 test build automatically.

## Validation in progress

- Initial scheduling experiment: three CPU rendering stalls in 90 seconds.
- GPU scheduling experiment: 22 stalls; unchanged installed baseline: 38 stalls
  under the contemporaneous system load. These are not zero-loss passes.
- Compact-label experiment: 165 stalls, with tick_sources reaching 992.7 ms. It
  exposed blocking filesystem checks and was not accepted as a performance fix.
- `local10-file-watch-cpu`: 90-second recording, no reported rendering/encoding
  losses, full decode and clean exit. tick_sources max 6.417 ms, p99 0.011 ms;
  zero reported memory leaks. This run predates lazy worker creation, which still
  needs final regression coverage.
- File-watch unit test: delayed query does not block polling, only one query can
  be pending, missing and stale paths are handled, and destruction joins work.
- AddressSanitizer/UndefinedBehaviorSanitizer with leak detection passed the worker
  lifetime test on the host. The sandboxed leak checker could not run under process
  tracing; that environment failure was not counted as a pass.
- `local10-final-cpu`: about 93 seconds, no reported render/encoder losses, full
  decode and clean shutdown. tick_sources max 9.992 ms, p99 0.011 ms.
- `local10-final-gpu`: 90 seconds, 2,700 decoded video frames, no reported losses,
  full decode and clean shutdown. tick_sources max 6.785 ms, p99 0.010 ms.
- `local10-final-fallback`: CPU fallback active, 30 seconds, 900 decoded frames,
  no reported losses and clean shutdown.
- `local10-image-refresh`: eight checks passed, including real file replacement
  and changing the source path; clean shutdown with zero reported memory leaks.
- `local10-final-workspace`: 232 checks passed at font scale 14, recording decoded,
  no reported losses and clean shutdown. Widget-layout capture visually inspected;
  native-video capture artifacts make it unsuitable as a public product preview.
- Updater mock-transport tests passed, including recent-check bypass, in-flight
  deduplication, explicit opt-out, checksum/size rejection and active-output guards.
- First real startup probe received HTTP 200 despite a fresh LastCheck. Its helper
  queried the frontend API too early and injected two log errors; the helper was
  corrected to discover the Qt window without calling an uninitialized API.
- `local10-startup-final`: automatic updater created despite a fresh LastCheck;
  actual GitHub release request completed with HTTP 200. No warning/error entries,
  exit code 0 and zero reported memory leaks.
- Installation completed successfully through the normal pkexec/APT path. Version
  verified as `0.7.1-1+ubuntu26.04.local10`; `dpkg -V penguin-studio` reported no
  deviations. No passwordless authorization was created or password collected.
- `local10-installed-startup`: the installed executable performed the automatic
  GitHub check despite a fresh LastCheck (HTTP 200), ran natively under Wayland,
  and exited with code 0, no warning/error entries and zero reported memory leaks.
  The user later reported missing the administrator dialog; installation had
  already completed, so no redundant authorization request was made.

Tests use synthetic media at 1280x720/30 FPS in isolated English Wayland profiles.
These are measured results, not a guarantee against stalls under arbitrary future
system load or a new real-webcam movement test.

## Local package

`0.7.1-1+ubuntu26.04.local10`, SHA-256
`b28e682d783419a8f650f9092f7568883412c754b096a98c82ceab89656f2c0e`.
Only the main executable and image-source module differ from local9. The camera
module is byte-identical. Dependency simulation upgrades only Penguin Studio and
adds/removes no packages. No GitHub publication or public version change.
