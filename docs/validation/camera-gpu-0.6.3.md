# Camera GPU processing — development validation

Date: 2026-10-25. This records an unreleased working tree, not a published package.
Host: Ubuntu 26.04.1, native Wayland, AMD Radeon RX 9070 XT.
Installed Penguin Studio remains 0.6.2. Candidate C supplies the application
binary while the modified camera plugin and GPU worker are overlaid privately.
Consequently these checks do not validate the new inspector widgets or final
Linux packages.

## Completed checks

- Fixed-shape RVM versus the original CPU model on 120 private reference frames:
  maximum alpha difference 0.0000067652, mean difference 0.000000018312.
- Native MIGraphX profiling confirmed GPU execution for both matting and hand
  preservation models on the RX 9070 XT.
- The initial in-process GPU experiment blocked the UI and was rejected.
  Initialization now runs in a child process while CPU processing continues.
- An actual Facecam GPU recording completed with 1,513 output frames, no reported
  rendering/encoding losses, full decode and a clean shutdown with zero reported
  memory leaks. The user was away: this is not a live movement-quality pass.
- A GPU recording of the existing private movement reference completed with
  1,495 output frames, no reported rendering/encoding losses, full decode and
  clean shutdown. Visual inspection retained head and raised hands, with some
  soft edges; perfect segmentation is not claimed.
- Automatic mode using relocatable private AMD runtime libraries completed with
  1,492 output frames, no reported rendering/encoding losses, full decode and
  clean shutdown with zero reported memory leaks. No system ROCm installation
  or driver change was made.
- The fake-worker regression test passed outside the restricted execution
  sandbox: readiness, mask transfer, cancellation during preparation, child
  exit and response timeout. The sandbox run failed the child readiness check;
  that failed execution is not counted as a pass.
- Deliberately terminating the isolated GPU worker restored CPU processing
  without closing the application. The subsequent 1,504-frame recording and
  shutdown completed, but 11 rendering-lag frames (0.7%) were reported. This is
  a functional fallback pass, not a zero-loss performance pass.
- A user-confirmed live Facecam movement recording completed with 1,501 output
  frames, no reported rendering/encoding losses and clean shutdown with zero
  reported memory leaks. The user reported problematic contours/hands. Inspection
  confirms thin background remnants and soft contours; live visual quality is
  **not accepted**. Do not publish this implementation as finished.
- Locale completeness passed for English and seven translations.

## Mask parity investigation

A 120-frame same-input worker comparison measured mean 8-bit alpha error 1.632,
maximum 255 and 1.96% of pixels differing by more than 10 levels with both models
on GPU. With the hand companion disabled, mean error was 0.00000719, maximum 1,
and no pixels differed by more than 10 levels. The problematic difference is
therefore in the hand path. The working implementation now keeps hand inference
on CPU and accelerates primary RVM; the repeated 120-frame comparison then measured mean error 0.00000638,
maximum 1 and no pixels differing by more than 10 levels. Restoring affinity for all runtime threads after preparation and extending the
bounded frame transaction deadline from 20 to 40 ms avoided premature fallback.
A 50-second reference recording then completed with 1,489 output frames, GPU
active throughout, no reported rendering/encoding losses, clean shutdown and
zero reported memory leaks. Live quality still requires renewed acceptance.

## Outstanding

Validate CPU fallback in the real application, cold preparation and shutdown,
final CPU/GPU comparison, actual inspector widgets, distributable runtime
packaging and a new user-confirmed live movement test. No release is approved by
this document. Private videos, images, account data and desktop captures remain
outside the repository and publication artifacts.


## Live startup follow-up

The first corrected hybrid live preview exceeded the 40 ms worker deadline and
fell back before recording. A diagnostic repeat confirmed a frame-response
timeout after GPU activation. The hand companion has therefore been moved to
the application's existing Microsoft ONNX Runtime CPU session, running in
parallel with the isolated primary-GPU request. The worker receives primary-only
requests. This latest implementation is undergoing live verification; results
above refer to the explicitly described earlier configurations.


The latest primary-only GPU worker plus parallel application-side CPU hand
recovery completed the known-person reference recording with 1,512 output
frames, GPU active throughout, no reported rendering/encoding losses, full
FFmpeg decode, clean shutdown and zero reported memory leaks. Visual inspection
shows the head and raised hands retained, with soft edges around moving fingers.
A 1,513-frame live Facecam recording also passed those technical checks, but its
inspected frames show only the replacement background. User presence and live
visual acceptance remain unconfirmed; this live run is not a movement-quality
pass. No new release or system installation has been performed.


## Confirmed live movement test — 2026-10-25

The repeated Elgato Facecam test used native Wayland, the RX 9070 XT primary GPU
model, parallel application-side CPU hand recovery, a synthetic replacement
background and rounded corners. The user confirmed that hands and head remained
visible and that the picture was fluid and usable.

The approximately 50-second recording completed with 1,504 output frames. The
application log reported no rendering-lag or encoding-skipped frames, warnings
or errors; GPU processing remained active without CPU fallback. The full file
decoded successfully with FFmpeg. Normal application shutdown exited with status
zero and reported zero memory leaks. Six inspected frames show the visible
subject and moving hands; some soft motion edges remain, so this does not claim
perfect segmentation. The recording and review images remain private under
`/tmp/penguin-gpu/gpu-parent-live-repeat` and `/tmp/penguin-gpu` respectively.

This supersedes the missing user-presence/visual-acceptance limitation for the
live test only. Final inspector UI testing, distributable runtime packaging and
release/install validation remain outstanding. No release or system update was
performed during this test.
