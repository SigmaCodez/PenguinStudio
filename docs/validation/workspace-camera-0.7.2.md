# Workspace and camera validation — 0.7.2

## Local candidate, 2026-10-27

The tested desktop is Ubuntu 26.04.1 under Wayland, with an AMD RX 9070 XT.
Tests use isolated English profiles and synthetic media unless explicitly noted.
Private Facecam footage is stored outside the repository and is not distributed.

- Native Wayland workspace: 232 assertions passed, including navigation, source
  controls, settings and recording. The short recording fully decoded and closed
  cleanly with no reported render/encoder losses or memory leaks.
- Separate 90-second GPU, CPU and forced CPU-fallback recordings fully decoded:
  2,700, 2,702 and 2,706 video frames respectively. No reported render/encoder
  losses; all three stopped and exited cleanly. One late inference tick in each
  CPU/GPU run reused the previous complete color/mask pair; none occurred during
  the fallback interval. These counters differ from OBS render/encoder losses.
- The private five-minute Facecam test produced 9,003 decoded frames. All sampled
  states used GPU processing without fallback. Five busy inference ticks reused
  the previous complete pair. There were no reported render/encoder losses or
  warning/error entries, and shutdown reported zero memory leaks. The user
  accepted head/hand visibility and fluid movement.
- Six camera lifecycle phases covered CPU/GPU selection, odd dimensions, removal,
  blur, custom images, disable/re-enable and destruction. Worker sanitizer tests
  and bounded failure/fallback tests passed.
- Status-bar assertions and visual review verified centered, unclipped labels,
  recording/pause/idle behavior and utilization of the selected RX 9070 XT.
  GPU utilization describes the whole rendering device, not this app alone.
- Settings update checks opened above the modal settings dialog and returned
  correctly. Updater tests covered HTTP failures, checksums, download size,
  active outputs and exact Ubuntu-release asset selection.
- Qt Quick software navigation avoids competing with the video graphics context
  on X11 and Wayland. Camera inference, video rendering and hardware encoding
  retain their GPU paths. XWayland captures and Wayland workspace tests passed
  after fixing the competing-context startup failure.
- Automated Qt Quick navigation/filtering/layout checks passed. Programmatic
  keyboard focus was not granted by this Wayland session; those focus assertions
  did not pass. The user previously confirmed Tab, Enter and Escape on the same
  navigation controls. No new manual keyboard acceptance is claimed here.

## Ubuntu 26.10 userspace

A native build against verified Ubuntu 26.10 base libraries passed encoder,
updater, output-safety, platform-flow and wizard tests. A synthetic Wayland GPU
recording decoded 2,700 frames in 90.033 seconds, with no reported render/encoder
losses, no additional busy inference ticks and clean shutdown. Ten status-bar
checks also passed. This uses the host's kernel and Wayland compositor with
26.10 userspace; it is not a separately booted 26.10 desktop certification.
The local user-namespace installation could not complete system-account ownership
changes; clean package installation remains a CI release gate.

## Public previews

The three English product images were captured from the extracted
`0.7.2-0+ubuntu26.04.candidate1` DEB, not redrawn or recolored. Their exact package
hash, source commit and image hashes are recorded in
[the current manifest](../images/current/manifest.json). All three were visually
inspected. They contain synthetic artwork/audio and disconnected account panels.

## Remaining release gates

Complete all Linux package builds and their installation/runtime checks, test the
final Ubuntu packages, publish only the complete verified set, and install the
final Ubuntu 26.04 packages using the normal administrator confirmation. This
record must be updated with the final CI run and installation outcome.

### First-frame regression caught before publication

The exact camera pixel test caught a raw-background flash before the asynchronous
worker's first result. Startup and resize now draw transparent output until a
matching color/mask pair is ready; actual inference failures still preserve input.
All camera pixel checks then passed, including the first-frame assertion, shapes,
keys, blur/image composition, alpha edges, hand mode, Undo/Redo and persistence.
The corrected test exited without warning/error entries or reported leaks.
