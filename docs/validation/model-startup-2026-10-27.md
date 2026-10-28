# Nonblocking camera model startup — local review, 2026-10-27

No public version increment or GitHub publication. Classic workspace and previous
blur reuse optimization are retained.

## Causes and changes

- ONNX session/model construction occurred inside filter update while the caller
  held the graphics lock. It now runs in the existing bounded inference worker
  before its first inference, including initialization of the CPU fallback.
- Updates arriving during initialization/inference retain only the latest settings
  instead of joining an unfinished job on the UI/render thread. The next render
  applies them after completion. Owned settings references are released on
  replacement, application and destruction. Shutdown still drains accepted work.
- Shader programs are retained across settings updates.
- A saved background image was decoded/uploaded even in blur/removal modes that
  do not use it. Image loading now occurs only in custom-image mode. Its saved
  setting is preserved, so selecting image mode still loads the chosen image.
- Until matching color/mask output is ready, the camera filter draws transparent
  output. It does not expose the unprocessed background during preparation.

## CPU comparison

Ubuntu 26.04.1, native Wayland, RX 9070 XT; isolated English profiles, synthetic
logo, rounded corners, eight-pass blur, 720p/60-FPS H.264/AAC recording. No private
webcam footage or accounts. These figures describe this host, not universal limits.

| Build | Effect activation | First ready status | Max render work | Recorded video frames |
| --- | ---: | ---: | ---: | ---: |
| Installed classic3 | 73.38 ms | 135 ms | 72.03 ms | See baseline evidence |
| Async model only (intermediate) | 63.40 ms | 192 ms | 61.20 ms | 1,799 |
| Async model + skip unused image | 3.70 ms | 85 ms | 9.70 ms | 1,800 |

The intermediate result exposed image loading as the dominant remaining cost;
it was not accepted as the final fix. The final CPU recording is 30.050 seconds,
fully decoded (1,800 video / 1,406 audio frames), with no reported render/encoder
losses and zero reported memory leaks on clean shutdown. Its render work stayed
below 16.667 ms throughout. Scheduling intervals can still vary; the maximum
observed interval was 18.915 ms. This is not a guarantee against OS scheduling
jitter, cold GPU-driver compilation or slow custom-image decoding.

Evidence under `/var/tmp/penguin-obs33`: `startup-classic3-cpu`,
`startup-classic4-cpu` (intermediate), `startup-classic4-cpu-v2` (final).

## GPU and rapid updates

The final AMD GPU run reported 2.95 ms activation, GPU ready at 1,297 ms, and
maximum render work of 8.59 ms. The preparation wait remains real but no longer
holds the graphics lock. This run used an existing GPU compilation cache; it is
not a cold-driver compilation benchmark. It recorded 30.050 seconds / 1,800 video
and 1,406 audio frames, decoded fully, with GPU active and no CPU fallback.
No loss events or warning/error/CRITICAL diagnostics were reported; shutdown was
clean with zero reported memory leaks.

All six lifecycle phases passed: rapid changes during initialization, final
settings winning, CPU/GPU changes, resizing, blur/image modes and disable/enable.
The longest rapid update call was 0.0943 ms. Evidence: `startup-classic4-gpu` and
`startup-classic4-lifecycle` under the same local evidence directory.

Pixel checks passed for transparent first-frame output, shapes, rounded corners,
green/blue key, automatic matting, blur, custom images, missing-image fallback,
alpha handling, hand recovery and persisted/undo-restored settings. The reusable
worker lifetime test also passed. Evidence: `startup-classic4-pixels`.

Automatic CPU fallback was verified with the GPU runtime hidden only inside the
test sandbox: fallback true, GPU false, 30.050-second recording with 1,800 video
and 1,406 audio frames, full decode, no reported loss events or warning/error/
CRITICAL diagnostics, and clean shutdown with zero reported memory leaks.
Evidence: `startup-classic4-fallback`.

Package: `0.7.2-0+ubuntu26.04.classic4`.
SHA-256: `75eb53547735ec67f1c3ce3b46befa1424ff58b3b41295c47d03ef378d2a503e`.
The finished package changes only the two exact tested camera module binaries
relative to classic3. APT simulation upgrades only Penguin Studio, without new
packages or removals. Installation completed after normal Ubuntu administrator approval. dpkg confirms
classic4 and both installed camera-module hashes match the tested binaries.
The installed build passed all six lifecycle phases under native Wayland in an
isolated profile, including CPU/GPU changes and the latest rapid update winning.
The maximum rapid update call was 5.145 ms. It exited successfully with no warning,
error or CRITICAL diagnostics and zero reported memory leaks.
Evidence: `/var/tmp/penguin-obs33/startup-classic4-installed`.
Nothing was published to GitHub.
