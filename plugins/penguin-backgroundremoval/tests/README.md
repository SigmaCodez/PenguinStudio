# Native texture reuse regression test

`texture-reuse.cpp` compares the previous and optimized texture paths using the
real OBS OpenGL backend and the checked-in Kawase shader. It uses synthetic pixels
only and does not open a webcam, read a desktop image or alter application settings.

Requirements: a C++17 compiler, `pkg-config`, OBS development headers/libraries
(including `libobs-opengl`), Wayland client development files, and any header
requirements of the installed OBS development package (such as SIMDe).
Run from the repository root in an active Wayland session, with no recording or
other GPU benchmark running concurrently:

```sh
c++ -O2 -std=c++17 \
  -I"$(pkg-config --variable=includedir libobs)/obs" \
  $(pkg-config --cflags-only-I libobs wayland-client) \
  plugins/penguin-backgroundremoval/tests/texture-reuse.cpp \
  $(pkg-config --libs libobs wayland-client) \
  -o /tmp/penguin-texture-reuse-test

/tmp/penguin-texture-reuse-test \
  libobs-opengl.so \
  plugins/penguin-backgroundremoval/vendor/data/effects/kawase_blur.effect
```

If the OpenGL module is outside the loader's search path, pass its full path as
the first argument. Both the module and shader must come from the build being
reviewed. The test does not require an installed Rust runtime or OpenCV.

Expected results: **100 alpha-upload comparisons** and **72 blur comparisons**
with no pixel differences. Coverage includes changing patterns, odd widths,
padded input rows, target resizing, normal/focal blur, and 1/2/3/8 blur passes.
The OpenGL R8 readback explicitly accounts for four-byte row alignment; some OBS
versions report only the logical unaligned row width from the staging API.

Exit codes: `0` passed, `1` failed, `2` invalid arguments, `77` skipped because no
Wayland session was advertised. A skip is not evidence of a successful GPU test.
This checks pixel equivalence, not overall CPU/GPU utilization, allocation-failure
recovery, or complete filter lifecycle behavior. Application-level recording,
mode-switching, resize and shutdown tests remain necessary before installation.


## CPU RVM and hand-buffer regression checks

Enable `PENGUIN_CAMERA_POSTPROCESS_TESTS` in the Rust preprocessing test project
with OBS and ONNX Runtime headers/libraries available. CTest then includes:

- `penguin-rvm-preprocess-equivalence`: 96 bitwise comparisons of the complete
  CPU tensor preprocessing path and hand-model RGB input, including padded views,
  odd sizes, 1080p, degenerate dimensions and rejected tensor layouts. Informational
  alternating-run benchmarks report preprocessing time, not whole-app CPU usage.
- `penguin-hand-buffers-equivalence`: real CPU inference with the bundled hand
  model, comparing persistent buffers and cached names against independent
  per-frame buffers across repeated and changing dimensions. The fixture is
  synthetic, with no camera or account access.

These tests require the private OpenCV configuration used by the camera module.
The RVM optimization keeps non-RVM and Rust-disabled preprocessing available.
