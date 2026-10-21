# Hardware encoding

Penguin Studio includes NVIDIA NVENC and Linux VA-API encoding. New simple-output
profiles prefer NVIDIA H.264 when its encoder is registered, then AMD AMF where
available, then VA-API H.264. On Linux, AMD GPUs use VA-API through the installed
Mesa driver. VA-API can also support Intel GPUs.

The setup assistant and new multistream destinations also prefer available
hardware encoders. Existing explicit encoder selections are preserved. To change
an existing profile, choose the hardware encoder in **Settings → Output** or run
the auto-configuration wizard. Advanced-output settings remain user-controlled.

VA-API is offered in simple mode only when the default device and H.264 profile
advertise both CBR (streaming) and CQP (quality-based recording). The driver selects
the compatible render device; no `/dev/dri` node is hard-coded. If this capability
check fails, new profiles keep software x264. A saved simple-mode VA-API selection
falls back to x264 when the capability is no longer available. In advanced output,
other driver-supported rate controls and codecs remain accessible.

NVIDIA requires a compatible GPU and installed NVIDIA driver with NVENC support.
The packages include the encoder plugin, not a replacement system GPU driver.
AMD requires a Mesa VA-API driver that supports encoding the selected codec.
Debian/Ubuntu packages recommend `mesa-va-drivers`. Flatpak and Snap also require
working graphics access and a compatible graphics runtime/host driver. Hardware
session limits still apply when several outputs run simultaneously.

H.264 is the automatic default for streaming compatibility. HEVC and AV1 remain
explicit selections where supported by the GPU, driver and streaming service.

## Validation

`penguin-hardware-encoder` exercises missing capabilities, CBR-only and CQP-only
devices, disabled choices and property callbacks. Native DEB/RPM builds run it.

On 2026-10-21, a local libobs test using the installed encoder plugin successfully
encoded two synthetic 1280×720, 30 FPS clips on a Radeon RX 9070 XT under native
Wayland/EGL: CBR at 2500 kbit/s and CQP at QP 23. Both outputs stopped normally.
This validates the VA-API capability query and encoder settings, not all GPU models
or a complete application GUI session. NVIDIA runtime testing requires NVIDIA
hardware and has not been performed on this AMD machine.

The production multistream controller was also tested with VA-API under Wayland:
simultaneous 1280×720 landscape and 1080×1920 portrait H.264/AAC outputs to local
receivers, crop/fit, stop/restart, reconnect after a receiver interruption, active
output destruction and unreachable-server cleanup all passed. Test media was
synthetic; no streaming account or desktop capture was used.
