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
Studio manages updates for the complete application. Only the RVM model is
packaged and offered in the model selector. The model, inference and mask
processing are otherwise the upstream implementation. A read-only `camera_status`
procedure exposes initialization, readiness and errors to the camera inspector.
The integration serializes managed filter-pair changes with rendering to prevent
partially detached filters from being rendered. The expected wait for the first asynchronous mask is not logged as a warning;
actual inference failures remain errors and bypass matting to preserve the input.

The camera inspector manages a tagged companion filter before the shape filter.
It uses CPU inference with two threads, no threshold, no image-similarity frame
skipping and no additional temporal smoothing. These are the settings accepted
in a real Facecam movement test under Wayland. RVM maintains its own recurrent
state. Existing legacy camera scenes remain readable.

This component does not change the licensing of unrelated files. Preserve all
upstream copyright notices and provide this corresponding source with binary
redistributions. Personal camera recordings and test images are not distributed.
