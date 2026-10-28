# Creator recording helpers — local validation, 2026-10-28

Local candidate: `0.7.2-0+ubuntu26.04.classic7`. No product-version increment or GitHub publication.

## Scope

- Added recording state, approximate encoded video duration, written-data counters, primary stream state and privacy state to Performance help. Sampling uses existing counters every two seconds while the dialog is visible; there is no extra image processing or timer when hidden.
- Added an explicit Copy diagnostics action with an allowlisted snapshot of states, counters and classified advice. Raw errors, account data, names and paths are not copied. No automatic transmission.
- Added recording-container resilience advice, with separate standard MP4/MOV, recoverable-container and custom-output explanations. No configuration is changed.
- README and Creator tools documentation describe the new local candidate. The existing classic layout is retained.

## Checks

- Module compiled with `-Wall -Wextra -Werror`.
- Unit checks cover duration conversion including fractional frame rates and invalid input, format advice, unknown-format non-disclosure, audio silence boundaries and failure classification. Assertions were enabled.
- First Wayland run found a stale privacy label when entering Performance help between timer ticks. Refreshing immediately on tab entry fixed it.
- Final synthetic Wayland integration: 24 assertions passed, including active/stopped recording summaries, metrics, existing privacy/audio functions and button text fit.
- The 30-second recording fully decoded (899 video frames, 1403 audio packets counted by the verifier); privacy samples were opaque and silent and resumed samples contained picture/audio. No loss events, warnings, errors or critical messages were reported. Shutdown returned 0 with zero OBS memory leaks.
- English Performance help capture visually inspected; provenance is in `docs/images/local-review/creator-extension-classic7/manifest.json`.
- Packaged module is byte-identical to the tested module. APT simulation: one upgrade, no additions or removals.
- DEB SHA-256: `f3e2a69b8bbc5d3073fdbc5823398263d183321b625d99edaffd5a42c98e9ae6`.

Evidence: `/var/tmp/penguin-obs33/creator-extension-classic7-final`.

## Limits / installation

Tests use synthetic media, not a live account or webcam. Camera processing and CPU/GPU worker binaries are unchanged. Clipboard interaction with external applications and physical keyboard use have not been manually verified in this pass. Duration is approximate and counters can reset between outputs; no disk-space forecast is promised.

Normal Ubuntu administrator confirmation completed. DPKG reports `0.7.2-0+ubuntu26.04.classic7`; the installed module matches the tested module (SHA-256 `095ef33f17c729be559a8a148d50752cdf4657bc2d3b932e8b45f52ead7eda63`). The final installed-app test (`/var/tmp/penguin-obs33/creator-extension-classic7-installed`) passed all 24 assertions. Its 29.966-second synthetic recording fully decoded (898 video frames, 1401 audio packets counted by the verifier); privacy samples passed, no loss events or warning/error/critical messages were reported, and shutdown returned 0 with zero OBS memory leaks.
