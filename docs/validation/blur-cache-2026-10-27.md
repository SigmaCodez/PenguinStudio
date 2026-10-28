# Background blur reuse — local performance review, 2026-10-27

No GitHub publication or public version increment. Classic workspace retained.

## Finding and change

The background filter repeated all blur passes on each render even when the
asynchronous inference worker had not published a new matching color/mask pair.
Reuse the completed blur texture for that pair. Invalidate it before new texture
uploads and on settings changes; disabling blur releases its render targets.
Failures are not cached. No model, quality threshold, resolution or inference
frequency was changed. This reduces both GPU draw submission and CPU render work
when inference runs slower than the output cadence.

## Controlled measurements

Host: Ubuntu 26.04.1, native Wayland, AMD RX 9070 XT. Isolated English profiles,
synthetic logo source, rounded corners, eight-pass blur, 1280x720 output and local
H.264/AAC recordings. These are not live Facecam or total-device-load measurements.
Control and candidate filter modules were compiled with the same compiler flags,
OpenCV 4.10.0 static libraries and Rust library. No concurrent builds ran during
these controlled comparisons. Draw counts cover the entire automated run,
including warm-up and teardown; recordings cover 30 seconds.

| Inference / output | Build | Sprite draw calls | Render-thread median | Render-thread p99 | Video frames |
| --- | --- | ---: | ---: | ---: | ---: |
| GPU / 30 FPS | Control | 15,726 | 1.659 ms | 2.321 ms | 900 |
| GPU / 30 FPS | Candidate | 15,689 | 1.613 ms | 2.208 ms | 900 |
| CPU / 60 FPS | Control | 30,174 | 0.829 ms | 4.728 ms | 1,800 |
| CPU / 60 FPS | Candidate | 20,619 | 0.625 ms | 2.170 ms | 1,800 |

At CPU/60 FPS this run used 31.7% fewer sprite draws and 24.6% less median
render-thread time. GPU/30 FPS provides no convincing performance improvement;
it is a regression comparison. These figures do not imply equivalent reductions
in total CPU/GPU utilization. Initial setup spikes still occur before recording
(maximum render-thread time about 130–330 ms in GPU runs and 191–197 ms in CPU
runs); this change does not fix those initialization stalls.

All four recordings decoded fully with no reported render/encoder loss events,
no OBS warning/error entries, exit status zero and zero reported memory leaks.
The read-only test sandbox did emit dconf initialization diagnostics before the
recordings; these are not an application crash. Final acceptance uses a private
in-memory GSettings backend and also checks CRITICAL diagnostics explicitly.
The decoded RGBA image at 15 seconds matches exactly between control and candidate
for each mode. A single static-frame comparison does not establish motion quality.

Evidence: `/var/tmp/penguin-obs33/perf-blur-control`, `perf-blur-candidate`,
`perf-blur60-cpu-control`, and `perf-blur60-cpu-candidate`.

Camera pixel checks passed for shapes, rounded corners, green/blue key, automatic
matting, blur, custom background, undo/restore, transparent backgrounds and hand
recovery. The GPU readback checks used the current `penguin-camera-gui.cpp`.

The first two additional lifecycle attempts did not load their helper because
compilation failed under `-Werror=misleading-indentation`; they were stopped and
are not acceptance results. The test helper now separates the early return from
the tick increment and identifies the main window by Qt class, as the other
helpers do. Its successful compilation was checked before retrying.

Local package: `0.7.2-0+ubuntu26.04.classic3`.
SHA-256: `f51ec19b42dbc56b0888a1305b6b04011b54a717aba98603c9cd73ee67d5231a`.
The package replaces only the background-removal module in the classic2 file
payload. It uses the same pinned OpenCV version, existing ONNX Runtime and Rust
preprocessor. APT simulation upgrades only Penguin Studio, with no added or
removed packages. The UI and public product version remain unchanged.

Final lifecycle check passed all six phases under native Wayland: CPU/GPU
switches, source resizing, background modes and filter disable/enable. It exited
successfully with no warning, error or CRITICAL diagnostics and zero reported
memory leaks. Evidence: `/var/tmp/penguin-obs33/perf-blur-lifecycle-final`.
The finished DEB file payload differs from classic2 only in the exact tested
camera module. Local installation completed after normal Ubuntu administrator confirmation.
`dpkg-query` confirms classic3 and the installed module SHA-256 exactly matches
the tested candidate (`896ac3c33c716de349b9e79b9a08e62f1873a089f439f737b287918ad0c35196`).

The installed build passed a further native Wayland CPU-mode 60-FPS blur recording:
30.050 seconds, 1,800 video and 1,406 audio frames, full decode, no reported frame
losses, no warning/error/CRITICAL diagnostics, successful exit and zero reported
memory leaks. Evidence: `/var/tmp/penguin-obs33/perf-classic3-installed-cpu`.
No GitHub changes were published.
