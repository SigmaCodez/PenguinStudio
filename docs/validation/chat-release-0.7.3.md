# Penguin Studio 0.7.3 release validation

Validated on 2026-10-29. Release source: `f39e1093d164e72bb6ab6598816a8153be9f5f18`.

[All Linux package jobs](https://github.com/SigmaCodez/PenguinStudio/actions/runs/40789653943) passed. The nine published packages were downloaded and matched both SHA256SUMS and the [build manifest](https://github.com/SigmaCodez/PenguinStudio/releases/download/penguin-v0.7.3/build-manifest.json): Ubuntu 26.04 and 26.10, Fedora 44, Flatpak, Snap, AppImage, Arch recipe, and both optional AMD runtime DEBs. Snap WebEngine paths and AppImage positioning-plugin dependencies were corrected before publication. The Arch recipe is not an AUR listing.

## Ubuntu 26.04 / Wayland

The final Ubuntu DEB passed 237 workspace assertions and 31 recording/loopback-stream checks. Recording and received-stream media decoded completely. Privacy audio/video behavior and measured upload reporting passed. The runs exited normally with zero reported OBS memory leaks.

System-wide installation upgraded only Penguin Studio and the existing AMD camera add-on to `0.7.3-1+ubuntu26.04`. Package file verification passed. A separate test of the installed application passed all 237 workspace assertions, exited with code 0, and reported zero OBS memory leaks without warning/error lines in that workspace test.

## Chat boundaries

Six isolated Wayland checks covered both original chat tabs, lazy browser creation, expected platform URLs, page freezing and resuming. The chat and output modules in the final package matched the preceding tested build byte for byte. The locally bundled AppImage also passed these six checks and clean shutdown. Signed-out platform pages can emit their own JavaScript/WebGPU diagnostics; these are not hidden or described as a warning-free chat session.

During local review, the user confirmed integrated YouTube sending and deletion of a designated test message in both the app and browser. No separate test accounts were available for real timeout, ban or unban tests; those paths use simulated platform responses in the regression suite. Google account eligibility and embedded website login are not guaranteed. Flatpak currently provides integrated API chat. See [chat behavior](../CHAT.md) and the [local review history](../local-chat-review.md).

Current English previews use an isolated demo profile and synthetic media. Private account details, webcam media and website-chat captures are not published. See the [preview manifest](../images/current/manifest.json).
