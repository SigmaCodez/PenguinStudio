# Local11 camera pipeline review

Public product version remains 0.7.1. Local candidate: 0.7.1-1+ubuntu26.04.local11.
Publication was initially excluded. On 2026-10-27 the user explicitly authorized publication after Ubuntu 26.10 support and successful final checks. The evidence below still describes the 0.7.1 local11 candidate.

## Change

Primary inference and hand recovery now run in one reusable background task,
outside the graphics thread. A completed color frame and its matching mask are
uploaded together. Pending work never grows into a queue. Model changes and
shutdown wait for accepted work before releasing its state. Resolution changes
reject incompatible completed frames. Allocation and inference failures preserve
the unfiltered input and remain reported.

This is a latency/throughput tradeoff: normal display is one video tick behind
capture (about 33 ms at 30 FPS). A slower computation repeats the previous complete
pair; new completed-frame and busy-tick counters make this observable. Render-loss
counters alone must not be used to claim that every camera input was processed.
The models, hand-mask math, GPU/CPU choice and CPU fallback remain unchanged.
No camera timeout threshold was increased and no V4L2 diagnostic was suppressed.

## Investigation

The instrumented synchronous path identified waiting for hand inference on the
graphics thread as the dominant cost. Early comparison runs were contaminated by
another project compiling Rust with near-total CPU utilization. They are retained
as investigation evidence, not a controlled performance baseline. The user then
confirmed that other builds were paused.

The first private live-test launch failed because its isolated filesystem did not
bind the private output directory as writable. It recorded no camera video. The
runner was corrected; this was a test-environment failure, not a product fix.

## Validation in progress

- Synthetic Wayland lifecycle: six phases passed, covering CPU/GPU selection,
  640x360 and odd-sized 321x193 inputs, removal/blur/image backgrounds, disabling
  and re-enabling the filter, and normal shutdown. No warnings/errors; zero
  reported memory leaks. Evidence: `/var/tmp/penguin-obs33/local11-lifecycle`.
- Final private five-minute Facecam GPU recording: 300.166 seconds, 9,003 decoded
  video frames, all 301 sampled states on GPU, no fallback, no warning/error
  entries, full decode successful and clean exit with zero reported memory leaks.
  No reported render/encoder losses. All measured graphics-thread durations were
  below 33.333 ms: median 1.259 ms, p99 1.694 ms, maximum 24.775 ms.
  Between recording start/stop events, 9,036 processing completions and five busy
  ticks were observed (events include output flush time). Busy ticks reuse a
  complete pair; they must not be represented as zero camera-frame repeats.
  User confirmed visible head/hands and fluid movement on the paired-frame path.
  Evidence: `/var/tmp/penguin-private-test-archive/local11-facecam-gpu-final`.
- CPU synthetic recording: 89.900 seconds, 2,697 decoded video frames, no reported
  render/encoder losses, full decode and clean shutdown. Graphics-thread p99
  2.421 ms, median 1.725 ms. The session maximum of 161.537 ms includes setup;
  this is not evidence that every startup tick met a video deadline.
  Evidence: `/var/tmp/penguin-obs33/local11-final-cpu`.
- The intermediate live run exercised CPU fallback after a single response
  exceeded the old 40 ms synchronous deadline. The final background transaction
  watchdog is 250 ms; it never blocks the renderer. Mock-process tests passed for
  an 80 ms delayed reply, crash, missing helper, startup cancellation and a
  bounded hung-response fallback. No V4L2 timeout was changed or suppressed.
- ASan/UBSan with leak detection passed for worker reuse, nonblocking polling,
  matching-result publication, exception propagation and draining on destruction.
- Final combined package, status-bar layout checks and installation pending.

Private webcam material stays outside the repository and must never be published.

## Status-bar follow-up

The user also requested a status-bar redesign during this task. The local build
uses spaced rounded capsules, hides inactive output clocks/network indicators,
labels active recording/live durations, and shows compact CPU/FPS values. The FPS
tooltip retains exact current and target rates. Recording pause stays explicit.
Status height follows font metrics; the first visual inspection caught a collapsed
height despite passing width-only checks, so vertical clipping checks were added.

Status-bar review at font scale 14 passed all eight checks: active recording,
English labels, pause, idle output visibility, centered/unclipped timer and metric
text, and return to idle. Actual Qt captures were visually inspected (recording,
paused and compact widths). Local preview files are in `images/local-review/local11/`,
separate from public release previews. No private camera image appears in them.
