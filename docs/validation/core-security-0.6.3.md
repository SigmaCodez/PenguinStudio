# Core security and reliability maintenance — 0.6.3

Status: candidate; package and native Wayland validation pending.

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
