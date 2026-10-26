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
