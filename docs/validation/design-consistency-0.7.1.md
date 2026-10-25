# Interface consistency validation — 0.7.1

## Package provenance

The Ubuntu 26.04 candidate was built from `24356d88d2887d98282338d9b63aeb6afc7e70b8` by [Linux package run 40236243749](https://github.com/SigmaCodez/PenguinStudio/actions/runs/40236243749). All seven package targets passed: Ubuntu DEB, Fedora RPM, Flatpak, Snap, AppImage, Arch recipe and the optional AMD camera runtime. The candidate DEB SHA-256 is `bdb5074194f57cdf9d803e16fb1df6d9fde3ee0b365a9be95c81728787ece8a6`.

## Interface checks

- The real Ubuntu candidate passed 242 workspace assertions under native Wayland, including production-button alignment, separate menu hit areas, compact layouts, Inspector containment, transforms and undo/redo.
- The Flatpak candidate passed the same 242 assertions under native Wayland with its own Qt runtime.
- Both workspace recordings fully decoded: 97 video frames and 150 audio frames over 3.233 seconds.
- Eighteen Ubuntu dialog captures at the largest supported text setting passed input-height, navigation-width and horizontal-overflow checks. A separate stress check at a larger configuration value (14) and 1280 × 800 confirmed that long camera choices no longer widen the Inspector beyond the viewport.
- Both disconnected native chat tabs were visually checked at enlarged text and closed cleanly.
- A 32.500-second synthetic GPU camera recording on the RX 9070 XT fully decoded (975 video frames and 1,522 audio frames), with no reported render-lag or encoder-skip events and clean shutdown. This is a processing regression check, not a new live movement-quality assessment.
- Camera pixel and Inspector checks passed for shapes, green/blue keying, local inference, blur, custom images, transparent image compositing, optional hand recovery, disabling, saved settings and undo/restore.
- Current product previews were captured from the unmodified Ubuntu package using XComposite under XWayland, then visually reviewed. They contain English UI and synthetic media only. Native Wayland functionality was tested separately; Qt widget renders are not presented as native video captures.

## Shutdown and focus

Nine standard startup/shutdown scenarios passed: normal, multi-instance, open wizard, cancelled connection wizard, auto-configuration, statistics, updater, saved-profile restart and signal shutdown. Logs contained no startup or shutdown warnings/errors, no late frontend unregister calls and zero OBS memory leaks.

The focused-input case initially failed its **focus precondition**: the window lost native keyboard focus before shutdown, although shutdown itself stayed clean. Those attempts are retained as unsuccessful focus tests. A separate isolated multi-instance run confirmed native keyboard focus continuously through closing the connection wizard, with clean shutdown. This does not claim that every compositor permits automated window activation.

Flatpak retains four known diagnostics for absent optional AJA devices; they are recorded rather than hidden. Live Twitch/YouTube authorization, viewer moderation and a new physical Facecam movement assessment were not repeated for this interface-only change. No account messages or stream details were modified.
