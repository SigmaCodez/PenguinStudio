# Hand preservation — Penguin Studio 0.6.2

The Ubuntu 26.04 candidate package built from `fc69fe501f0fb8acc11a990aa6772d76259872ce` was tested on Ubuntu 26.04.1 under native Wayland using an isolated package overlay and a separate English profile. Its checksum and dependency preflight passed; the simulation upgrades only Penguin Studio. This candidate check did not change the system installation.

The complete camera GUI regression passed, including the new inspector checkbox, automatic model initialization, saved/ restored hand settings, shapes, chroma keys, blur, custom images, missing-image fallback, alpha cleanup and undo/redo. Startup and shutdown had no warning/error diagnostics or reported memory leaks.

A 40.021-second replay of the private Facecam movement reference used automatic custom-image replacement, rounded corners and **Preserve moving hands**. H.264 VA-API video and AAC audio decoded completely. The log recorded 1,201 output frames and 1,205 drawn frames, with no reported render-lag, encoder-skip or output-drop events. The different counters include output startup/stop timing and are not themselves a drop count. Sampled frames at 8, 20 and 30 seconds retained the person; the moving-hands sample retained both hands with soft edges. Sampling cannot establish the absence of all flicker, and a replay is not a new live-camera capture.

The earlier live prototype test was accepted by the user but logged one render-lag frame and one encoder-skipped frame in 70 seconds. That limitation remains documented; this package replay does not establish zero-loss live performance. The optional feature is off by default and adds CPU work.

All public screenshots were separately captured from this real package using synthetic media in an isolated English profile. Private camera recordings and frames remain local. See [machine-readable results](hand-preservation-0.6.2.json) and [historical prototype evidence](../experiments/hand-preservation/results.json).
