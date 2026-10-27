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
changes; the subsequent clean CI package installation and camera pixel tests passed.

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

After this correction, fresh native Wayland recordings passed full decode and
clean shutdown: CPU 90.966 seconds / 2,727 video frames, GPU 90.033 seconds /
2,699 video frames. Neither reported render/encoder losses. The CPU interval
contained three busy inference ticks and the GPU interval eight; these reused
the last matched pair. The GPU run stayed on GPU without fallback.

The first post-correction fallback run also contained a Firefox launch failure
inside the read-only test namespace. Its strict clean-log gate failed; it is
retained separately and is not counted as a clean run. A controlled repeat was
performed below without changing browser permissions or hiding warnings.

The controlled fallback repeat passed: 94.100 seconds, 2,823 decoded video frames,
confirmed CPU fallback, five busy inference ticks, no reported render/encoder
losses, no warning/error entries and zero reported memory leaks on shutdown.

## Native Ubuntu 26.10 CI package

The exact `0.7.2-1+ubuntu26.10` DEB from candidate run
[40428246369](https://github.com/SigmaCodez/PenguinStudio/actions/runs/40428246369)
passed clean installation, launcher and camera pixel checks in CI. Its SHA-256
is `eb7593a4945c0cbc9098d799c50a4c34bd86648324bb31e2229158835446a566`.
Together with the matching CI AMD add-on, the extracted package then recorded
90.033 seconds / 2,700 decoded video frames under native Wayland with 26.10
userspace. GPU stayed active without fallback; one inference tick reused a pair.
There were no reported render/encoder losses, warning/error entries or leaks,
and shutdown was clean. The host kernel/compositor limitation above still applies.

## Native Ubuntu 26.04 CI package

The `0.7.2-1+ubuntu26.04` DEB from the same candidate run has SHA-256
`5d84a42434a2037f1d5f6901a7e58c72717d44810e722df897ec409156a85994`.
It passed clean installation and camera pixel tests in CI. The actual extracted
package passed 232 local Wayland workspace checks, a fully decoded short
recording and clean shutdown. With its matching CI AMD add-on, a 90.033-second
GPU recording decoded 2,700 video frames with no fallback, no additional busy
inference ticks, no reported render/encoder losses and clean shutdown.
The host APT preflight upgrades only Penguin Studio and its existing AMD add-on;
it installs no additional packages and removes none.

The same Ubuntu 26.04 CI package passed a CPU recording: 90.033 seconds /
2,700 fully decoded video frames, one busy inference tick, no reported
render/encoder losses and clean shutdown. Settings update checks found the
button, displayed the updater above settings, received the current-version
response and restored the settings dialog, with no warning/error entries.

The CI AppImage passed 232 Wayland workspace assertions, a fully decoded
3.233-second recording and clean shutdown, with no reported render/encoder
losses. Its bundled Qt Quick/QML import directories were present.

AppImage camera pixel checks also passed under Wayland: first-frame masking,
shapes, green/blue key, local inference, blur, custom/transparent images,
alpha-edge cleanup, hand mode, Undo/Redo and persisted settings. The process
exited cleanly without warning/error entries or reported memory leaks.

## Flatpak candidate regression

Candidate run 40428246369 completed all Linux builds successfully. The Flatpak
bundle (SHA-256 `befe88b534e07cbf5338358edf3234105a856fe7fae5d85020670e87fb0f0d0d`)
passed 241 legacy workspace assertions, a fully decoded short recording and camera
pixel tests, but visual inspection exposed a missing Qt Quick workspace module:
it was installed outside Flatpak's scanned bundled-module directory. These
checks therefore do not constitute acceptance of the new Flatpak interface.
The module now installs in the bundled core directory for Flatpak only; other
formats retain their tested location. CI additionally checks that exact installed
file. A rebuilt Flatpak and a fresh visual/runtime check are required.

The first launch was also blocked by the host document portal's missing FUSE
mount. Restarting the normal user portal service restored it without changing
application permissions. Four known absent-AJA-device diagnostics were present
in the successful Flatpak runs; no other warning/error entries or memory leaks
were reported.

A controlled path probe loaded the unchanged built Qt Quick module through the
isolated profile's supported plugin location. The modern workspace then passed
232 assertions and a fully decoded 3.233-second recording (97 video / 150 audio
frames), with clean shutdown and only the known absent-AJA diagnostics. Visual
inspection confirmed the intended simplified navigation. This isolates discovery
as the cause; the rebuilt bundled package still needs its own acceptance run.

The first rebuilt bundle (`7fada3315963984a08dfc89b97d756f0dbced2428e590f3fbc939215de73d014`)
contained the module in core but still did not load it: bundled core modules also
require explicit registration before libobs generates its module list. The
acceptance check rejected this bundle. Flatpak now registers the workspace before
that generation; CI checks both the installed module and its name in libobs.
Other platforms retain plugin discovery and are not added to the core list.
