# Upload status — local validation, 2026-10-28

Local candidate: `0.7.2-0+ubuntu26.04.classic8`. No public product-version change or GitHub publication.

## Changes

- Added a persistent bottom-right Upload indicator for the combined primary stream and Penguin multistream destinations. Rates come from per-output byte deltas and monotonic elapsed time; audio/video and container traffic are included. Recordings, unrelated applications and third-party outputs are excluded.
- Each connection has its own baseline. Startup, reconnection, counter rollback and invalid elapsed intervals reset sampling rather than causing false spikes. Values update once per second; zero during initial sampling is explained in the tooltip.
- Reused the output monitor instead of adding another timer. It now stops while no outputs are active/starting and wakes on stream events or destination start. Labels and tooltips update only when their text changes.
- Added English and German strings; other locales use the existing English fallback. Existing primary-stream diagnostics are retained separately from the aggregate indicator.

## Validation

- Production module compiled with warnings treated as errors.
- Unit test checks known byte rates, delayed ticks, zero transfer, reset, counter rollback, equal/backward timestamps and large counter values. Assertions remain enabled in release builds; the native packaging check runs the test.
- Initial integration run crashed because the upload query occurred during module post-load, before frontend output initialization. The query now waits for FINISHED_LOADING. The corrected run exited 0.
- Native Wayland test used an isolated English profile with synthetic video/audio, simultaneous local recording and a stream to a loopback-only FFmpeg receiver. No platform accounts or webcam were used.
- All 31 UI/function checks passed, including visible offline indicator, nonzero measured upload, sleeping/waking monitor, stop/reset, existing privacy protection and audio-assistant behavior.
- Local received stream averaged about 6.08 Mbit/s; the status capture showed 6.0 Mbps. The stream and recording decoded successfully; privacy recording samples passed. No frame-loss events, warnings/errors/criticals or OBS memory leaks were reported; shutdown was clean.
- Packaged module and locale files are byte-identical to tested files. DEB SHA-256: `b6c10c909d01c38a6e5e7ba0e456ab721a8e63ebe5cabfeab9dbfa97d212cdbe`.

Evidence: `/var/tmp/penguin-obs33/upload-classic8-final` and `upload-classic8-final-receiver.flv`.

## Limits / installation

This run exercised the primary stream with a real local receiver. Multiple concurrent destinations and live provider reconnection were not end-to-end exercised in this pass; independent per-output sampling and aggregation were reviewed in code. Camera and encoder implementations are unchanged. Reduced idle polling is verified; no general CPU/GPU percentage improvement is claimed.

Installation completed through the normal Ubuntu administrator confirmation. DPKG reports `0.7.2-0+ubuntu26.04.classic8`. The installed module matches the tested binary (SHA-256 `540f4d1fc773c41e40fc54f71a81398d361f807eee962276b3969fed98c21d33`). The installed-app confirmation run (`/var/tmp/penguin-obs33/upload-classic8-installed`) passed all 31 assertions. Both the received local stream and local recording fully decoded; privacy samples passed. No warning/error/critical messages or frame-loss events were reported, and shutdown exited 0 with zero OBS memory leaks. The installed status-bar capture was visually inspected.
