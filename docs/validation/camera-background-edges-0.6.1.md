# Custom camera background edges — 0.6.1

## Real-camera candidate check

An Elgato Facecam 4K at 1280 × 720, 30 fps was tested with an isolated profile
under native Wayland on Ubuntu 26.04.1. The installed 0.6.0 engine was used with
the candidate shader mounted temporarily at the normal camera-plugin data path.
This was not yet a test of a complete 0.6.1 package.

The user accepted background blur during head and hand movement, but reported
background residue/flicker with a custom replacement image. Local frame inspection
also showed weak edge spill around hair and shoulders. Suppressing weak alpha
before replacement compositing was accepted as significantly better and usable.
No perfect segmentation claim is made; difficult lighting, motion blur and fine
hair can still affect quality. No private frames are included in the repository.

Both live runs closed without warning/error diagnostics or reported memory leaks.
A restart restored the custom image, rounded corners and active matting, then
closed cleanly. The GPU regression fails on the previous shader's weak-alpha
composite and passes after correction. It also covers transparent and opaque
pixels, a soft intermediate transition, unchanged output without replacement,
all existing camera modes and undo/restore.

See [redacted test evidence](camera-background-edges-0.6.1.json).

## Packaged validation

The Ubuntu 26.04 candidate from `0c136b20` passed the complete Inspector/GPU
suite under native Wayland, including the new alpha-spill regression, all camera
modes and undo/restore. Its shader exactly matches the reviewed source. Restarting
the saved real Facecam scene restored the custom image and rounded corners, with
an active 1280 × 720 input and working matting. Both runs closed without warning/
error diagnostics or reported leaks.

Final release installation is recorded after the complete Linux build succeeds.
