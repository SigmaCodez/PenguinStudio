# Facecam recording validation

Tested on Ubuntu 26.04, native Wayland, Ryzen 7 9800X3D and Elgato Facecam 4K.
Separate profiles preserved the user's scenes and sign-ins. Recordings remained
local and are not included in the repository or release assets.

The original two-thread managed RVM configuration produced approximately 14%
render/encoding stalls in a 20-second, 1280×720/30 fps recording. Profiling located
the delay in source ticking; the VAAPI encoder itself remained within budget.
Four inference threads reduced median source tick time from about 19 ms to 10 ms.
Two comparison recordings and one recording with the actual migration code had
no reported render or encoding stalls, warning/error entries or memory leaks.
All exited normally after stopping the recording.

Each successful recording used RVM matting and the rounded-rectangle shape with
radius 25, composited over a solid background. VAAPI H.264 and AAC were exercised;
a synthetic tone avoided recording microphone audio. Full-file decoding passed,
timestamps were monotonic, foreground and background were present at three
sample times, and audio/video end times differed by at most 11 ms.

The core-count policy is bounded to one through four physical cores. A regression
test covers unknown/small/large CPU counts, upgrading the old default, preserving
explicit non-default choices and not reapplying the migration after later edits.
This is hardware-specific performance evidence, not a guarantee for every scene
or machine. Package CI and installed-release validation remain separate checks.
