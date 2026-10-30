# Highlight notes — local review

Build: 0.7.3-1+ubuntu26.04.local2. No GitHub publication or public version change.

## Implementation

A separate Highlight notes tab adds optional editing notes, approximate frame-based recording timestamps and UTF-8 text export. Notes from different recording starts remain separate, including when the window is hidden. Pauses do not advance their video timestamp. Recording format support is independent of embedded chapter support.

Notes are limited to 1,000 entries and 240 input characters per entry. They live only in memory for the current app session; the interface explicitly asks the user to export before quitting. They are not included in diagnostics, embedded in media or uploaded. Export uses QSaveFile; cancelling or failing an export does not clear the list. Existing toolbar layout is unchanged.

## Evidence

- Warning-as-error module compilation and existing creator unit checks passed.
- Native Wayland with an isolated English profile, synthetic video and tones; no real camera, accounts or public streams used.
- 34 note/chapter checks passed: Unicode input, paused timestamp stability, a new recording while the tools window was hidden, successful text-file selection/export, retention of notes after export, and existing chapter/audio/privacy checks.
- Both Hybrid MP4 recordings fully decoded. The first export test saved the default suggested filename because of test-dialog input; the test driver was corrected, then explicit filename selection passed.
- 35 loopback-stream/recording checks passed. Privacy shielding verified in decoded video/audio; local RTMP capture and recording fully decoded.
- Test applications exited normally with zero OBS memory leaks. The note export run logged one Glycin sandbox warning from the file-dialog environment; this is recorded rather than claiming that run had no warnings. No security settings were disabled to suppress it.
- Real English Highlight notes preview visually inspected and retained under docs/images/local-review/highlight-notes-073-local2.
- Package payload comparison against public 0.7.3: only penguin-outputs.so differs. No generated Python bytecode packaged.
- All 237 workspace checks passed; exit 0, no warning/error entries, zero OBS memory leaks. APT simulation confirms one upgrade, no additions or removals. Normal Ubuntu installation completed after administrator confirmation. Installed version: 0.7.3-1+ubuntu26.04.local2. Package verification reported no discrepancies; the installed output module matches the tested binary. The installed-package Wayland run passed all 237 workspace checks, exited with status 0, and reported no warning/error entries and zero OBS memory leaks.

Package SHA-256: `62cad6f8879afe14581e4ef2a03c6fb3f0409cd5458845969b8d039c2aa0d5be`.

Temporary raw evidence: /tmp/penguin-notes073. Sanitized check reports accompany this document.
