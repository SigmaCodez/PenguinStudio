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

## Installed Ubuntu package

The final Ubuntu 26.04 package from release build `40043295576` (source
`a515b1ac`) was checksum-verified and installed as `0.6.1-1+ubuntu26.04`.
Only Penguin Studio was upgraded; no dependencies were added or removed.
Installed package-file verification passed.

The installed build passed the full native Wayland camera/Inspector suite,
including blur, custom images, rounded corners, alpha cleanup and undo/restore.
A real Facecam restart restored the saved image and rounded corners with active
matting at 1280 × 720. Both runs exited normally with no warning/error diagnostics
or reported memory leaks. The live motion feedback above remains the user's
assessment of the identical shader correction; the post-install restart was a
functional check, not a new subjective motion assessment.
