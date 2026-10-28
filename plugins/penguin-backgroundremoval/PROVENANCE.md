# Camera background removal provenance

Penguin Studio bundles a separately licensed adaptation of
[obs-backgroundremoval 1.4.1](https://github.com/royshil/obs-backgroundremoval/tree/1.4.1),
by Roy Shilkrot, Kaito Udagawa and the upstream contributors.

- Upstream revision: `88dd5e3d1889d4dc261b310949f103f6bf128057`.
- Source archive SHA-256: `3ebc0ef66ef7603e8cf564e7e84a412e552abc21cf548c1653074ed03175f213`.
- Plugin license: GPL-3.0-or-later; see `vendor/LICENSE` and `vendor/LICENSES`.
- The bundled Robust Video Matting model is GPL-3.0-only; its adjacent `.license`
  file identifies its authors. Model research and original implementation:
  https://github.com/PeterL1n/RobustVideoMatting.
- OpenCV 4.10.0 is built privately with only core and image processing modules;
  its Apache-2.0 license is installed with the package.
- ONNX Runtime 1.23.2 is shared privately with the legacy camera filter. Its MIT
  license and third-party notices are retained with the runtime.

## Local integration changes

The standalone plugin updater and enhancement filter are not registered. Penguin
Studio manages updates for the complete application. RVM is the model offered in the model selector. An additional MediaPipe Selfie Multiclass model is packaged for the optional moving-hand mode. The model, inference and mask
processing are otherwise the upstream implementation. A read-only `camera_status`
procedure exposes initialization, readiness and errors to the camera inspector.
The integration serializes managed filter-pair changes with rendering to prevent
partially detached filters from being rendered. Mask inference runs after capturing
the current render input, once per video tick, rather than applying the previous
frame's mask to a moving subject. This also prevents an unmasked first-frame flash.
Actual inference failures remain errors and bypass matting to preserve the input.
Input capture explicitly uses the default shader for image sources; asynchronous
cameras retain the upstream rendering path.

The camera inspector manages a tagged companion filter before the shape filter.
It uses CPU inference with up to four physical CPU cores, no threshold, no image-similarity frame
skipping and no additional temporal smoothing. The original two-thread default
was raised after real Wayland recording tests exposed render stalls; four threads
eliminated the reported stalls on the tested eight-core CPU. Existing managed
defaults migrate once; explicit non-default thread counts and independent filters
remain unchanged. The model and mask settings accepted
in a real Facecam movement test under Wayland are unchanged. RVM maintains its own recurrent
state. Existing legacy camera scenes remain readable.

This component does not change the licensing of unrelated files. Preserve all
upstream copyright notices and provide this corresponding source with binary
redistributions. Personal camera recordings and test images are not distributed.

## Optional moving-hand preservation (0.6.2)

The MediaPipe Selfie Multiclass model from the same upstream distribution is Apache-2.0 licensed; its adjacent notice retains the Google, Katsuya Hyodo, Roy Shilkrot and Kaito Udagawa credits. Its reviewed SHA-256 is `317054e916e36f47457a85afa3ef6a6cc8f5d35f3cb22e5358fd1a3e6cfb8eed`.

Penguin's optional adapter applies stable softmax to the model logits and supplements the RVM alpha with its body-skin class. Both models process the same frame concurrently, and compositing waits for both results. Each session uses at most three intra-op threads, with busy-wait spinning disabled. The option is off by default; independent filters and existing scenes remain unchanged unless explicitly enabled. Model failures remain visible errors through the existing camera status path.

### Supported recovery components (0.6.3)

Penguin's adapter limits additional skin recovery to connected components with
primary person-mask support. This reduces isolated background objects mistakenly
classified as skin. The original RVM mask is never reduced. Completely missed
hands without primary-mask support may still require better lighting or another
capture angle; this is not a guarantee of perfect segmentation. The regression
check is `test/penguin-hand-recovery.cpp`.

## Isolated GPU processing (0.6.3 development)

The camera inspector provides Automatic, GPU and CPU processing preferences.
Automatic and GPU attempt the packaged GPU worker and retain a prepared CPU
session when the worker is unavailable, still compiling, exits or misses its
response deadline. Driver initialization runs in a separate process, outside Qt.
GPU failure clears the CPU recurrent state without rebuilding the CPU session.
The transfer budget covers the complete frame request and mask response.

The GPU RVM variant is derived from the same official MobileNetV3 model with
fixed 320×192 input dimensions and a constant downsample ratio of one. It keeps
its original GPL-3.0-only notice. `scripts/packaging/prepare-camera-gpu-model.py`
records the source checksum and conversion. The resulting SHA-256 is
`41377570179d331c11486ae9d9ccf10ef806fa66cf34a6ca3caaeeba0c3a871c`.

An optional AMD runtime bundle is staged by
`scripts/packaging/prepare-amd-camera-runtime.py` using checksummed official AMD
artifacts listed in `packaging/amd-camera-runtime.json`. Supply the result through
`PENGUIN_CAMERA_GPU_RUNTIME_DIR` at build time. Staging does not install system
drivers. A build without that bundle continues on CPU. Runtime libraries and
notices are kept private to Penguin Studio. AMD RX 9070 XT testing uses MIGraphX;
other GPUs and distributable package combinations still require validation.

The optional hand companion remains on CPU in GPU mode. A same-input RX 9070 XT
comparison found materially different masks from its current MIGraphX graph;
primary RVM on GPU with CPU hand inference matched the CPU reference within one
8-bit alpha level across 120 frames. GPU mode therefore means accelerated primary
matting, not that every processing step runs on the graphics card.

Hand recovery runs in parallel in the application's existing CPU runtime while
the isolated worker processes primary RVM only. This avoids depending on the
AMD runtime's CPU hand performance. The helper has a bounded 250 ms background
transaction watchdog and CPU processing remains available if it fails.

The local asynchronous frame path runs primary inference and hand recovery off
the graphics thread. It retains one in-flight input and publishes its color and
mask together; no newer camera frame is combined with an older mask. Normal
operation adds one video-tick latency (about 33 ms at 30 FPS). If inference is
late, the last complete pair remains visible; `camera_status` exposes completed
frames and busy video ticks rather than hiding those repeats behind render-loss
counters. There is no unbounded queue. Updates and destruction drain the accepted
job before replacing or freeing model state. Model files and notices are unchanged.
Before the first complete frame/mask pair, and after a size change, output remains
transparent instead of flashing the unprocessed background. Actual inference
failures retain the visible input fallback. The camera pixel regression test
checks this first-render behavior without allowing an intervening video tick.
