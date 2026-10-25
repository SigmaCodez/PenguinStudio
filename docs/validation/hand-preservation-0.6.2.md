# Hand preservation — Penguin Studio 0.6.2

The Ubuntu 26.04 candidate package built from `fc69fe501f0fb8acc11a990aa6772d76259872ce` was tested on Ubuntu 26.04.1 under native Wayland using an isolated package overlay and a separate English profile. Its checksum and dependency preflight passed; the simulation upgrades only Penguin Studio. This candidate check did not change the system installation.

The complete camera GUI regression passed, including the new inspector checkbox, automatic model initialization, saved/ restored hand settings, shapes, chroma keys, blur, custom images, missing-image fallback, alpha cleanup and undo/redo. Startup and shutdown had no warning/error diagnostics or reported memory leaks.

A 40.021-second replay of the private Facecam movement reference used automatic custom-image replacement, rounded corners and **Preserve moving hands**. H.264 VA-API video and AAC audio decoded completely. The log recorded 1,201 output frames and 1,205 drawn frames, with no reported render-lag, encoder-skip or output-drop events. The different counters include output startup/stop timing and are not themselves a drop count. Sampled frames at 8, 20 and 30 seconds retained the person; the moving-hands sample retained both hands with soft edges. Sampling cannot establish the absence of all flicker, and a replay is not a new live-camera capture.

The earlier live prototype test was accepted by the user but logged one render-lag frame and one encoder-skipped frame in 70 seconds. That limitation remains documented; this package replay does not establish zero-loss live performance. The optional feature is off by default and adds CPU work.

All public screenshots were separately captured from this real package using synthetic media in an isolated English profile. Private camera recordings and frames remain local. See [machine-readable results](hand-preservation-0.6.2.json) and [historical prototype evidence](../experiments/hand-preservation/results.json).

## Final release package validation — 2026-10-25

The final Ubuntu 26.04 package from release commit `7ef39b2befb4fe4205c0df135d79ce4426089c3d` passed checksum, dependency preflight and the complete native Wayland camera GUI regression. Its SHA-256 is `882a821b5f55a0db2f1fdfa8dd646f1bd21307e2ce7c1c723514f411484cf1f9`.

A second 40.021-second motion-reference replay with hand preservation, a custom background and rounded corners decoded completely and closed without warning/error diagnostics or reported memory leaks. It logged **one render-lag frame and one encoder-skipped frame**, with zero output-drop events. Both moving hands remained visible in the inspected frame, with soft edges. This final result supersedes any inference that the candidate's zero-event run guarantees lossless performance. The feature remains optional and off by default. The package was tested in an isolated overlay, not installed system-wide as part of this check.
