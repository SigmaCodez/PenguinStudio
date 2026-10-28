# Local10 long Facecam review

Installed version: `0.7.1-1+ubuntu26.04.local10`.
Device: Elgato Facecam 4K (`/dev/video0`), MJPEG, 1280x720 at 30 FPS.
Native platform: Wayland. Separate English profiles; no streaming, no microphone,
no account changes. Automatic background removal, moving-hand recovery and rounded
corners (shape 3, radius 25) enabled. A plain synthetic color behind the camera
makes transparent areas visible. Camera recordings remain outside the repository
in a private local directory; no recordings or private previews are published.

## CPU run

- Five-minute recording: container duration 300.200 seconds, 9,003 decoded video
  frames. OBS reported 9,004 output frames; flush/end timestamps are not identical
  to decoded video-frame counts.
- 301 sampled processing statuses confirmed CPU processing, no GPU fallback and
  no bad processing status. Requested size stayed 1280x720.
- No reported render/encoder losses, capture timeouts or warning/error entries.
- Full recording decode succeeded. Normal exit code 0; zero reported memory leaks.
- V4L2 reported 9,242 captured frames over the whole session, including preview
  before and after recording. This is not a per-USB-packet loss measurement.
- User reported fluid movement and preserved head/hands during the live preview.
- Evidence: `/var/tmp/penguin-private-test-archive/local10-facecam-cpu-5min`.

## GPU run

- Five-minute recording: container duration 300.133 seconds, 9,003 decoded video
  frames. OBS reported 9,004 output frames.
- All 301 sampled processing statuses confirmed GPU processing, without CPU
  fallback or bad processing status; requested size stayed 1280x720.
- No render/encoder loss counters were reported. Full recording decode succeeded
  without decoding errors. Normal exit code 0; zero reported memory leaks.
- User reported fluid movement and preserved head/hands during the live preview.
- The run was not entirely error-free: before recording started, V4L2 reported
  one `select timed out` and a subsequent `failed to log status`. Capture recovered;
  neither message recurred during recording or shutdown. The cause is not yet
  established, and these messages must not be suppressed or described as fixed.
- The graphics-thread profiler measured a 54.090 ms maximum and 29.852 ms 99th
  percentile, with 99.8402% below the 33.333 ms frame budget across the session.
  Thus absent loss counters do not prove that every rendering iteration met its
  deadline. V4L2 counted 9,132 frames over the complete camera session; this is
  not a per-USB-packet loss measurement.
- Evidence: `/var/tmp/penguin-private-test-archive/local10-facecam-gpu-5min`.

## Outcome

Both recordings were readable in full and both applications exited normally.
The GPU camera-start timeout remains an open finding. No product code, version,
installation or GitHub publication was changed for this test. Private recordings
remain outside the repository.
