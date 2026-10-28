# Classic workspace rollback — local review, 2026-10-27

The user requested the layout and visual style of the public 0.7.1 interface.
Publication of 0.7.2 is on hold for this changed design. No 0.7.2 release or tag
has been published by this task.

## Changes

- Restore the exact Penguin theme from the 0.7.1 source commit
  `416b2e99d8d2b23fa0069be927b1b66d71812347`.
- Exclude the optional Qt Quick navigation module from all package formats.
  The native Layers/Scenes, preview, audio mixer and Inspector/Outputs/Details
  layout, menus and settings are retained. Existing QML development sources are
  not shipped; they remain only as an optional historical experiment.
- Remove the QML-specific package dependencies and Flatpak Qt declarative/shader
  build modules. Keep camera performance, Rust, GPU usage, Settings update checks
  and Ubuntu 26.10 packaging improvements.
- Preserve the previous experimental previews separately. New English previews
  are real XComposite captures of the local DEB with synthetic media only.

## Package provenance

`0.7.2-0+ubuntu26.04.classic1` is a local review build, not a public release.
It reuses the tested native binaries from candidate CI run 40428246369, excludes
`penguin-quick-workspace.so`, restores the original theme, removes dependencies
used only by that module, and regenerates package checksums. No frontend or camera
binary was patched. Its exact package hash is in the current image manifest and
`/var/tmp/penguin-classic-review/SHA256SUMS`.

## Validation

Host: Ubuntu 26.04.1, native Wayland, RX 9070 XT. Tests use isolated English profiles
and synthetic media. The existing installed AMD camera runtime remains in use.

- 241 native workspace assertions passed, with a fully decoded short recording
  (3.233 seconds, 97 video and 150 audio frames), no reported render/encoder losses,
  no warning/error entries and clean shutdown with zero reported leaks.
- Camera pixel checks passed for shapes, rounded corners, green/blue key, local
  background removal, blur, custom/transparent backgrounds, hand mode,
  Undo/Redo and persisted settings. Clean shutdown.
- CPU recording: 30.100 seconds, 900 video / 1,405 audio frames, full decode,
  no reported render/encoder losses, no warning/error entries and clean shutdown.
- GPU recording with the existing AMD runtime: 30.100 seconds, 901 video / 1,406
  audio frames, confirmed GPU without fallback, full decode, no reported
  render/encoder losses, no warning/error entries and clean shutdown.
- An initial isolated GPU attempt omitted the optional runtime mount and correctly
  used CPU fallback. That run is not counted as a GPU acceptance result.
- Three current English previews were visually inspected; they show the classic
  layout and contain no signed-in accounts, desktop content or webcam footage.
- Package SHA-256 and APT preflight passed: one Penguin Studio update, no newly
  installed packages and no removals. Unused Qt packages are not auto-removed.

## Installed validation

Installation of `0.7.2-0+ubuntu26.04.classic1` was confirmed with dpkg and the
successful APT log. Only Penguin Studio was upgraded. The saved pre-QML dock
arrangement was restored with a private backup of user.ini; scenes, sources and
account credentials were not changed.

A second test against `/usr/bin/penguin-studio` used an isolated English profile
under native Wayland. The persisted smoke helper passed 210 UI assertions. The
3.233-second synthetic recording (97 video and 150 audio frames) decoded fully,
with no reported loss events, no warning/error entries, exit status zero and zero
reported memory leaks. This installed smoke test supplements the 241-assertion
candidate check and CPU/GPU tests above; it is not a new live Facecam test.
The installed theme matches the restored source checksum and the QML workspace
module is absent. Publication remains on hold for local review.
