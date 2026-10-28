# Further performance candidates

Source review: 2026-10-26, following local5 (`86c7365ae`). These are implementation
candidates, not measured speedups. This review does not change application behavior,
package versions or releases. Existing author/license notices remain unchanged.

## 1. Reduce camera GPU readback size

`plugins/penguin-backgroundremoval/vendor/src/obs-utils/obs-utils.cpp`,
`getRGBAFromStageSurface`, renders, stages and maps at source resolution, then
clones that data into `inputBGRA`. `background-filter.cpp` clones it again for
processing; RVM subsequently needs only 320×192 input.

A 1920×1080 BGRA frame contains 8,294,400 logical bytes; 320×192 contains 245,760.
At 30 frames/s those payloads are approximately 248.8 MB/s versus 7.4 MB/s,
excluding alignment, copies and synchronization. This is a payload calculation,
not a predicted utilization reduction or achieved optimization.

Candidate: retain the original texture for compositing/blur but downscale into a
separate inference target before staging. Keep the existing path for other models
and any full-resolution consumers, including image-similarity behavior.
OBS effects/shader code plus existing C++ graphics ownership is the natural first
implementation. Moving this code to Rust alone would not remove the readback.

Acceptance: compare interpolation, color/channel order and mask quality against
OpenCV; test fast hands, odd sizes, resizing, blur, image backgrounds and CPU
fallback. Measure staging/map time, transferred bytes, graphics-frame time and
recording loss counters on the RX 9070 XT under Wayland. GPU scaling need not be
bit-identical to OpenCV; visual acceptance must not be inferred from metadata.

## 2. Share compatible multistream encoding

`plugins/penguin-outputs/penguin-outputs.cpp`, `Initialize`, creates a video encoder
and an AAC encoder for each destination, and a private canvas for each portrait
output. Identical destinations may therefore duplicate rendering/encoding work.

Candidate: an explicitly owned pool for compatible canvas/encoder configurations,
initially implemented against the current OBS C/C++ API. Share only after checking
layout, video source, codec, bitrate, rate control, frame rate, dimensions,
keyframe settings, color format and audio configuration. Landscape and portrait,
or different service requirements, must not be combined indiscriminately.

Acceptance: confirm encoder-sharing lifecycle semantics against the bundled OBS
implementation before changing ownership; local RTMP sinks must cover independent
start/stop/reconnect, a slow sink, timestamp continuity and unequal configurations.
Measure active encoder sessions, frame time and CPU/GPU load. Do not broadcast to
real services for these tests. Savings remain conditional on compatible settings.

## 3. Incremental chat rendering

`plugins/penguin-platforms/twitch-chat.hpp`, `Render`, clears and recreates the
whole list. Bursts are already coalesced over 50 ms and history is capped at 500
messages, but each render still rebuilds that history. `chat-view.hpp` constructs
text layouts both for size hints and painting.

Candidate: insert/update/remove affected rows in Qt, with bounded layout caching
keyed by message, width, font and display scaling. Keep selection, moderation IDs,
scroll position, accessibility and the history cap intact. Prefer C++/Qt here;
a Rust parser would not eliminate Qt item creation or layout work.

Acceptance: synthetic high-volume chat, deletion/timeout/clear events, Unicode,
resizing, font scaling and scroll-follow behavior. Measure GUI stalls and layout
counts. No live chat messages or moderation actions are needed.

## 4. Persistent hand-inference worker

`plugins/penguin-backgroundremoval/vendor/src/models/ModelRVM.hpp`,
`StartHandRecovery`, uses `std::async(std::launch::async, ...)` with a cloned frame
for each job. The previous task is joined before reuse, so an unbounded queue is
not the current issue; repeated task/thread and allocation setup is the candidate.

Candidate: a long-lived worker with at most one pending job and explicit buffer
ownership. Rust is plausible for the bounded worker/state machine; a persistent
C++ worker is the necessary comparison. Speed would come from reuse and scheduling,
not Rust syntax. Keep same-frame hand masks and exception/error propagation;
never let an obsolete result apply after resizing or filter destruction.

Acceptance: count thread creation, allocations and scheduling time; stress mode
changes, inference failure, shutdown and camera reconnect. Benchmark both designs
before accepting an FFI boundary or replacing existing lifecycle logic.

## Lower priority / retain existing backends

- GPU-worker IPC transfers a 320×192 BGRA frame and one-byte mask synchronously:
  307,200 logical bytes per inference. Profile waits first. Shared memory/Rust IPC
  adds lifetime and failure-recovery complexity; bandwidth alone does not justify it.
- The updater already streams bounded chunks to disk while incrementally hashing.
  A language replacement has no demonstrated performance benefit here.
- YouTube polling already honors the API-provided interval with a 10-second floor.
  Faster polling or a different language is not an optimization target.
- Keep codec implementations, ONNX execution and Qt rendering backends. No evidence
  from this review justifies replacements in Rust, Go, Python or JavaScript.
- The source review is not a whole-application profile. Prioritize camera readback,
  compatible multistream sharing, chat updates and persistent-worker benchmarking;
  accept changes only after measured gains and regression checks.
