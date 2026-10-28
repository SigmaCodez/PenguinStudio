# Creator basics — local validation, 2026-10-28

Status: implemented, installed and validated locally. The installed classic5 package passed the 15-assertion synthetic Wayland recording/assistant/privacy test again before release preparation. Release approval was received on 2026-10-28; publication is pending the complete Linux build set.

- Product version: 0.7.2 (unchanged candidate).
- Local DEB: `0.7.2-0+ubuntu26.04.classic5`.
- Base: installed `classic4`; classic workspace and camera optimizations retained.
- Package SHA-256: `ced3e0d99397592f47e9905fa0b3029c991bc900152791771982677004cdb434`.
- Payload comparison with classic4: only `usr/lib/x86_64-linux-gnu/obs-modules/core/penguin-outputs.so` changed. DEB metadata and package checksums were regenerated.
- APT simulation: one upgrade, no additional packages, no removals.

## Included behavior

The [Creator tools guide](../CREATOR-TOOLS.md) describes the advisory stream check, performance/output error explanations, level-only audio assistant, and privacy shield. The assistant uses the existing Qt Widgets style. A persistent privacy control is available in the classic top bar.

The audio assistant preserves existing filter types, offers optional local noise suppression, and retains a single undo transaction. A later manual volume adjustment is not overwritten by Undo. The error helper classifies errors without echoing raw URLs, credentials or paths.

Privacy masks the program video and silences normal source audio without altering mixer mute/fader states. The shield remains effective across the tested scene/transition changes and newly created regular audio sources. It is not a camera hardware switch or a promise about third-party outputs that bypass the program/source pipeline.

## Evidence

All GUI runs used isolated English profiles and synthetic color sources, images and audio tones. No real webcam, microphone or streaming account was used. All files below are local test artifacts, not published downloads.

| Check | Result |
| --- | --- |
| Build with `-Wall -Wextra -Werror` | Passed |
| AddressSanitizer / UndefinedBehaviorSanitizer / LeakSanitizer | Passed outside the sandbox tracer |
| Audio guard | All audio planes silenced; inactive samples unchanged; block canary and timestamp preserved; null plane/zero-frame case passed |
| Error classification | Authorization/storage/encoder cases passed; private URL/key not echoed |
| Existing workspace regression | 210 assertions passed under Wayland |
| Creator tools package integration | 15 assertions passed under Wayland |
| CPU camera recording | 30.633 s, 918 video frames; CPU confirmed; no reported frame loss |
| GPU camera recording | 38.566 s, 1,157 video frames; GPU confirmed, no CPU fallback; no reported frame loss |
| Saved-scene restart | Four sources loaded, zero stale privacy filters |
| Shutdown | No warnings/errors/criticals in final package and restart runs; zero reported OBS memory leaks |
| Package dependency simulation | Only Penguin Studio upgraded |

Creator integration covered opening the dialog, persistent toolbar control, privacy on/off, a new audio source during privacy, scene/transition change, voice measurement, existing compressor preservation, no duplicate compressor, optional noise suppression, limiter creation, undo, manual volume preservation, recording stop, and shutdown while privacy remained active.

The final recording was fully decoded. Privacy sample intervals (4.5–11.5 s and 28.5–29.5 s) contained 211 and 31 decoded video frames respectively, checked at 16 × 9 resolution: all were uniformly dark (maximum RGB component below 12). All 672,000 and 96,000 decoded interleaved audio samples in those intervals were exactly zero. Before and after privacy, the synthetic colored picture and nonzero test tone returned.

The final payload also passed a full recording/assistant/undo/shutdown cycle after adding output error classification. CPU/GPU regression runs preceded that final diagnostic-only addition; camera-processing binaries were unchanged throughout.

Final test directories:

- `/var/tmp/penguin-obs33/creator-package-final`
- `/var/tmp/penguin-obs33/creator-restart`
- `/var/tmp/penguin-obs33/creator-regression-workspace`
- `/var/tmp/penguin-obs33/creator-regression-cpu`
- `/var/tmp/penguin-obs33/creator-regression-gpu`
- `/var/tmp/penguin-creator-classic5` (saved local helpers)

The two dialog previews in `docs/images/local-review/creator-basics-classic5` were visually inspected: English UI, synthetic source names, no account identity or personal media. They are kept separate from public current-version previews. `scripts/check-publication.py --candidate` passed. No public version increment or release approval is implied.

## Limits

- Preflight is advisory and does not validate provider authorization or upload capacity.
- Real microphone tuning and the keyboard shortcut with physical Wayland input still need user review; the recordings and audio measurements here were synthetic.
- Privacy cannot recall already buffered/transmitted frames and does not cover independent third-party pipelines.
- A quiet performance sample is not a guarantee of zero frame losses in every workload.
