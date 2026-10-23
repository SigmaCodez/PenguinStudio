# RVM camera integration checks

Environment: Ubuntu 26.04.1, native Wayland, Elgato Facecam 4K at
1280 × 720 / 30 fps using NV12. Tests use a separate configuration and do not
broadcast or change the user's normal scenes.

## Quality decision

The previous MediaPipe and MODNet experiments did not pass the user's movement
assessment. The unchanged official obs-backgroundremoval 1.4.1 plugin with RVM,
CPU inference, two threads, threshold disabled, image similarity disabled and
additional temporal smoothing disabled was accepted by the user as noticeably
better and usable. This is a practical observation for that camera and scene,
not a guarantee for every background or movement.

## Integration checks

The bundled source was built locally with private ONNX Runtime 1.23.2 and static
OpenCV 4.10.0 core/imgproc. A known portrait image retained the person while
removing its background. The test exercised all four camera shapes, disabling
matting, restoring settings and keeping an unrelated disabled filter untouched.

An initial rapid remove/restore test exposed a render-time crash: a detached
filter could reach `obs_source_skip_video_filter` after its parent was cleared.
Managed pair changes now hold the graphics context lock so rendering cannot
observe a partially detached pair. The same test then completed normally with
zero reported memory leaks.

The integrated Facecam test completed normally, followed by a fresh process
loading its saved scene. Both filters, their order and ready state were restored.
Both runs ended with exit status 0, no warning/error log entries and zero reported
memory leaks. The two-second first-frame grace removed the camera's startup-only
timeouts; normal timeout handling resumes after the first frame or grace period.
Boundary cases are covered by `test/penguin-v4l2-timeout.c`.

`test/penguin-camera-gui.cpp` additionally checks the Inspector's automatic-mode
Undo/Redo, rapid filter-pair lifecycle and preservation of unrelated filters in
Linux package CI. Package CI and installation checks are separate from these
local component tests. No personal camera images are stored in this repository.

A package regression test also caught reordered companion filters when an
unrelated filter was added after the Undo snapshot. Restoration now constructs
the complete filter order before moving entries, preserving unrelated filters'
relative order. The reproduced scenario passed locally under Wayland with five
rapid restore cycles, no diagnostics and zero reported memory leaks.

The full pixel comparisons were then run locally with the managed filter API:
circle/oval geometry, rounded corners, green/blue keying, automatic matting,
disabling and serialized settings all passed. Image-source capture now supplies
the default shader when required, avoiding invalid-shader diagnostics. This run
exited normally with no warning/error entries and zero reported memory leaks.

Ubuntu package CI exposed a duplicate UUID during rapid restoration: a removed
filter may still exist on the deferred destruction queue. Recreated managed
filters now receive a fresh UUID, while existing filters retain their identity.
The lifecycle test holds the removed instance alive deliberately and verifies
that its replacement has a different UUID. The expanded pixel/lifecycle test
passed under Wayland with no diagnostics and zero reported memory leaks.
