# Facecam recording motion check — 0.6.1

## Result: visual quality issue remains open

A 70-second local recording on Ubuntu 26.04.1 under native Wayland used an
Elgato Facecam 4K at 1280 × 720 / 30 fps, a custom image background, rounded
corners (18%), hardware H.264 and synthetic AAC audio. Microphones were disabled.
The user reported disappearing hands during fast motion and later clarified that
the head remained intact. Local decoded frames confirm partial hand transparency
and missing finger regions. This check does **not** pass visual motion quality.
The earlier accepted live edge-cleanup result remains historical evidence, not a
guarantee that all hand movements are handled correctly.

The installed build's file decodes completely without errors and lasts 70.015 s.
The output counter reports zero dropped frames, but that does not cover rendering
or encoding delay: the log reports one lagged render frame and one skipped encode
frame. Therefore this run is not described as having zero frame losses.
The application stopped recording and exited normally without warning/error
diagnostics or reported memory leaks.

## Isolated diagnostic experiments

A local unprocessed camera reference was recorded with user participation. The
user judged moving hands mostly clear in that view. It was replayed through the
same pipeline for controlled comparisons; individual output samples can differ
slightly in timing and must not be treated as pixel-aligned ground truth.

- Increasing model input from 320 × 192 to 512 × 288 did not reliably preserve
  hands, although the live test had no reported render/encode frame loss.
- Combining that resolution with weaker alpha cleanup still showed hand holes.
  This isolated test initially omitted the plugin locale directory and logged a
  missing-English-text warning. The test mount was corrected; this was not an
  installed-package defect.
- Full 1280 × 720 input with internal downsampling at 0.25, following RVM's
  two-stage approach, did not reliably improve the result and reported 39 lagged
  render frames (3.2%) and 54 skipped encode frames (4.5%).
- Weaker alpha cleanup alone did not remove the hand holes and reported ten
  lagged render frames and ten skipped encode frames in that replay.

All seven files decode without errors and all application sessions close normally
with zero reported memory leaks. None of these experiments meets both the visual
quality and performance requirements, so none is promoted into production. The
installed version remains the original, verified 0.6.1 package.

See [redacted diagnostic results](facecam-motion-0.6.1.json). Private recordings,
frames, account data and camera serial numbers are not included. A subsequent
fix needs a reproducible improvement on the moving-hand reference plus a fresh
live motion check; parameter changes alone are not sufficient evidence.

Model reference: [RVM inference documentation](https://github.com/PeterL1n/RobustVideoMatting/blob/master/documentation/inference.md).
