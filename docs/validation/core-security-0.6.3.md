# Core security and reliability maintenance — 0.6.3

Status: candidate; all Linux package builds passed before an additional updater
focus correction. Rebuilt packages, real YouTube consent and
Facecam movement validation remain pending before publication and installation.

## Scope

Targeted review of the inherited AV1/H.264 packet helpers, AMF0/AMF3 decoding,
Linux capture teardown, task dispatch and embedded browser dependencies. This
is not a claim that every inherited component has been exhaustively audited.
No remote-code-execution claim or CVE assignment is made.

## Corrections

- Validate AV1 headers, extension lengths, variable-length sizes and payload
  bounds before access or allocation. Reject malformed extraction atomically.
  Use unsigned 64-bit shifts and check metadata allocation arithmetic.
- Bound AMF field reads and nesting, reject truncated objects and unsupported
  AMF3 references, and release partially decoded objects and class definitions.
  Preserve public decoder entry points. Avoid signed shifts and invalid debug reads.
- Avoid out-of-range pointer arithmetic and aliasing in Annex-B scanning while
  retaining the word-at-a-time search. Bound H.264 SPS exponential-Golomb reads.
- Execute queued audio and graphics callbacks outside the queue mutex, following
  [OBS commit 4315559](https://github.com/obsproject/obs-studio/commit/4315559a098308604df5cdbf3b1546787a19f050).
- Preserve the shutdown guard from [OBS commit 0820d80](https://github.com/obsproject/obs-studio/commit/0820d804bcabe7a14f0474f73584321b882c7080).
  Additionally join the ALSA reconnect worker before freeing its state and signal
  its event even when the worker has not set its running flag yet. Publish the
  capture worker running flag before thread creation, so an early stop cannot
  be overwritten by late worker startup.
- Pass an explicit canvas when creating private scenes for Studio Mode, avoiding
  the legacy fallback warning without changing diagnostic filtering.
- Prevent duplicate PipeWire portal captures in Studio Mode, following
  [OBS PR 13673](https://github.com/obsproject/obs-studio/pull/13673).
- Update OBS Browser to 2.27.4 and CEF to 7871, including upstream local-file
  scheme restrictions. Keep the existing OBS 32.2.2 ABI baseline for plugins.

Original author and license notices are retained. The upstream fixes remain
attributed above; Penguin-specific validation and additional guards are separate.

## Completed local checks

- The original one-byte malformed AV1 input reproduced an AddressSanitizer
  heap-buffer-overflow. The corrected parser rejects it.
- 100,000 deterministic malformed packet cases plus valid AV1 and AMF fixtures
  passed AddressSanitizer and UndefinedBehaviorSanitizer. A separate execution
  outside the ptrace sandbox passed LeakSanitizer as well.
- Audio and graphics task functions compiled from the actual sources passed a
  callback test that enqueues a second task while acquiring the queue mutex.
- Exhaustive short Annex-B sequences at four alignments passed the reference
  scanner comparison. Real FFmpeg AV1 and high-profile H.264 encoder outputs
  retained their headers and keyframe detection under sanitizers.
- The bounded scanner retained word-at-a-time throughput in a local 400 MiB
  synthetic scan: warmed original 0.028102 s, bounded 0.028036 s. These are
  microbenchmark observations, not an application frame-rate promise.
- The actual ALSA shutdown functions passed mocked-device lifecycle checks for
  reconnect-worker startup and capture stopped before its worker begins.
- OAuth, chat moderation, token refresh, updater error/checksum/size/output
  exclusion, hardware selection and the complete simulated provider flows passed.
- Publication candidate, branding and translation checks passed.
- Installed 0.6.2 passed the native Wayland synthetic camera-effects baseline,
  including blur, custom images, hand preservation and clean zero-leak shutdown.
  This baseline result does not validate the new package.

Reproduction commands: `bash test/security/run-packet-parsers.sh` and
`python3 test/security/task-queues.py`. The parser runner needs matching libobs
headers/libraries and SIMDe headers. `PENGUIN_SECURITY_CPPFLAGS` can supply a
non-system header location. Sanitizer results are not a substitute for codec,
recording, platform-authentication or hardware regression tests.

## Preliminary package regression checks

The Ubuntu 26.04 candidate built from `53504d2` passed the following native
Wayland checks. This candidate predates the final ALSA startup and private-scene
canvas corrections; the final package must repeat these checks before release.

- All 211 workspace checks passed. The short synthetic H.264/AAC recording
  decoded completely and included the real CEF browser source. No rendering-lag
  or encoding-skip counters were reported in this run.
- Synthetic camera checks passed for shapes, chroma key, automatic removal,
  blur, image replacement, hand preservation, undo and missing-image handling.
- All nine shutdown scenarios passed without reported startup/shutdown errors,
  Qt focus warnings or OBS allocation leaks.
- Local RTMP integration passed horizontal and vertical outputs, reconnection,
  restart, active-output destruction and failed-connection cleanup.
- Twitch restored its saved session and returned Minecraft category results.
  Real YouTube consent and persistence remain pending; simulated provider-flow
  tests alone do not establish successful real-account authorization.

The workspace exercise still emitted the baseline external RADV Vulkan notice
and the private-scene canvas warning addressed in the subsequent candidate.
The ordinary camera and shutdown scenarios did not emit those warnings.
Browser launch during an isolated account test initially failed because Brave
inherited the test mount namespace. The test now hands the official Google URL
to the normal host browser without disabling browser sandboxing.

## Final source candidate checks in progress

The Ubuntu 26.04 package from `474dc4ca1b6f27e1b908092d399115ad4f41561c`
passed its SHA-256 verification and all synthetic camera-effects checks.
All 211 workspace checks passed; the recorded browser-source composition was
visually inspected and the complete recording decoded without errors. The
private-scene canvas warning is absent. OBS reported zero allocation leaks.

This run reported two rendering-lag frames (1.5%) during the short recording;
the installed 0.6.2 baseline reported eight (5.9%). This is not a controlled
performance benchmark and does not establish a universally loss-free result.
The external RADV Vulkan notice remains visible during the workspace exercise.
A real Facecam movement recording and final real-account YouTube verification
remain required before completing the requested release validation.

All seven Linux package jobs in GitHub Actions run `40183842831` succeeded:
Ubuntu 24.04 and 26.04 DEBs, Fedora 44 RPM, Flatpak, Snap core24, AppImage and
Arch binary package validation. Publication was correctly skipped for the PR.

The final source candidate also passed all nine native Wayland startup/shutdown
scenarios and the complete local horizontal/vertical RTMP integration. Twitch
restored its saved session and returned Minecraft search results. The manual
in-app updater reached GitHub and correctly reported no newer published release
(the public version is still 0.6.2); a real installation upgrade is still pending.
All three current English previews were captured from this package, visually
reviewed, and checked with `python3 scripts/check-publication.py`.

## Additional updater-dialog regression

The real update check exposed a non-null Qt Wayland focus mismatch when closing
the updater. Releasing focus owned by the dialog before rejecting it removed the
warning in a native Wayland test of the actual corrected updater implementation.
The test closed the dialog and main window separately, retained diagnostics,
and reported zero OBS allocation leaks. Updater HTTP/checksum/size/output tests
also passed after the correction. The shutdown suite now includes an explicit
updater focus-and-close scenario; rebuilt-package verification is required.

An earlier update-only test also reached the microphone buffering limit while
using auto-detected personal audio devices. The isolated update test now disables
capture channels; that result does not establish live microphone performance.
