# Penguin Studio 0.7.4 release-package validation

All eleven jobs in [the release workflow](https://github.com/SigmaCodez/PenguinStudio/actions/runs/41127174159) succeeded. All nine downloaded Linux packages were checked against both SHA256SUMS and the build manifest (hashes, sizes and source commit `bf957e1a9e722239bbf408083a4355b665ab055e`).

The exact Ubuntu 26.04 DEB passed on the Ubuntu 26.04.1 Wayland desktop:

- 237 workspace checks.
- Eight native preview/WebEngine lifecycle checks, including opening and replacing a synthetic browser dock during recording; 1,144 preview draw callbacks and 751 recording frames. The recording fully decoded; sampled frames at 1, 5, 15 and 22 seconds retained the expected red source.
- 35 recording/local-loopback stream checks, including privacy shielding, audio and upload status. Both recordings and received stream fully decoded; 900 recording frames and 895 stream frames were reported.
- All three test applications exited with status zero and reported zero OBS memory leaks. No rendering or encoding frame-loss diagnostic was reported in these runs.

The user also confirmed the correction in their normal profile: the picture remained visible after opening embedded chat. That session exited normally with zero OBS memory leaks. No private scene, webcam, desktop capture, credentials or account information is included here.

The known host RADV diagnostic can appear during WebEngine startup; these results do not claim that every third-party diagnostic is absent. Other Linux formats passed their CI packaging checks; they were not all installed on physical machines. This release does not add real-account ban/timeout validation.
