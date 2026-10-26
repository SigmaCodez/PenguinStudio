# Incremental Rust adoption

Status: local development candidates based on 0.7.1, installed locally for review.
No product version change, release tag or package publication is part of this work.
The user reviews local changes before explicitly approving a small release bump.

## First candidate: camera tensor preprocessing

The GPU worker previously converted BGRA8 to RGB8, converted RGB8 to float,
normalized, split channels, concatenated channels, then copied into an existing
ONNX Runtime input tensor for each 320 x 192 frame. The new dependency-free Rust
static library writes normalized planar RGB directly into that tensor. It does
not allocate per frame, change the model, change mask processing, modify hand
recovery, or change CPU/GPU selection.

Enable in a native Linux source build with `-DPENGUIN_RUST_PREPROCESS=ON`.
The default remains OFF during evaluation. Rust 1.74 or newer and Cargo are
build-time requirements only. Cargo runs with `--locked --offline`; there are no
third-party Rust crates or new installed Rust runtime. Cross-compilation is
explicitly rejected for this experimental path until target/toolchain handling
is tested. Existing Linux package recipes continue using the original path.

The C ABI checks dimensions, integer overflow, stride, lengths, output alignment
and overlapping ranges before constructing slices. The caller still guarantees
valid live allocations and exclusive output access; Rust cannot validate pointer
provenance from C++. The scalar conversion uses safe slices and iterators; the
second candidate adds a bounded unsafe SIMD kernel with runtime feature checks. Panics
are contained at the ABI boundary. A rejected input uses the existing OpenCV
preprocessing path, and the existing worker failure / CPU fallback remains intact.
See the [Rust FFI guidance](https://doc.rust-lang.org/nomicon/ffi.html).

## Reproduce the focused checks

With CMake, a C++ compiler, Rust/Cargo and OpenCV core/imgproc development packages:

```sh
cmake -S plugins/penguin-backgroundremoval/rust-preprocess \
  -B build-rust -DCMAKE_BUILD_TYPE=Release
cmake --build build-rust
ctest --test-dir build-rust --output-on-failure
build-rust/penguin-rust-preprocess-test --benchmark
cargo clippy --locked --offline \
  --manifest-path plugins/penguin-backgroundremoval/rust-preprocess/Cargo.toml \
  --all-targets -- -D warnings
```

The informational benchmark alternates execution order over nine trials of
1,000 frames, uses one OpenCV thread, and reports medians. It compares the original
OpenCV pipeline, Rust, and a fused C++ control. CI checks numerical equivalence
and invalid-buffer handling; it does not enforce a noisy timing threshold.

## Local results, 2026-10-26

- Rust 1.98.0, GCC 15.2.0, Ubuntu OpenCV 4.10.0; optimized native x86_64 builds.
- Ten C++/Rust equivalence cases cover all byte values, channel order, alpha,
  padded rows, offset views, odd sizes and frames through 1920 x 1080. Maximum
  observed difference from the original preprocessing: **0** (test tolerance
  1e-7). Input storage was unchanged.
- Rust tests cover invalid dimensions, overflow, short/excess output lengths,
  short input, null, misaligned and overlapping buffers. Clippy passed.
- The C++ ABI/equivalence harness passed AddressSanitizer, UndefinedBehaviorSanitizer
  and LeakSanitizer. The linked Rust static library and system OpenCV are not
  sanitizer-instrumented. The first sandboxed LeakSanitizer run was unsupported
  under ptrace; the same test passed outside that sandbox.
- Median preprocessing: original OpenCV **167.324 us/frame**, Rust **35.572
  us/frame**, fused C++ control **17.344 us/frame**. Rust is about **4.70x** faster
  than the original path, saving about **0.132 ms/frame**. The fused C++ control
  is faster still. The gain comes from avoiding allocations/copies, not an
  intrinsic Rust speed advantage. No whole-application FPS claim is made.
- An isolated native Wayland run used the newly compiled Rust-enabled worker
  with the installed 0.7.1 application, AMD RX 9070 XT and synthetic camera
  media. GPU processing was active without CPU fallback. A 30.100-second
  H.264/AAC recording fully decoded (900 video frames, 1,405 audio frames), with
  no reported render-lag or encoder-skip events. Startup/shutdown had no
  warnings/errors and reported zero OBS memory leaks.
- That integration test used a temporary Ubuntu OpenCV build rather than the
  release's private static OpenCV build. A first attempt lacked an indirect
  OpenCV dependency in the test environment; the CPU fallback worked. After
  correcting temporary library paths the GPU run passed. No system package was
  changed. Live Facecam movement and complete Linux package builds were not
  repeated for this local candidate.

Private evidence is under `/tmp/penguin-rust-wayland-gpu` and the standalone
builds `/tmp/penguin-rust-make` and `/tmp/penguin-rust-sanitize`; temporary files
are not durable release evidence. No personal video is included in this document.

## Adoption decisions

Keep Qt UI, OBS rendering/encoding, ONNX Runtime and GPU driver code in their
existing languages. Replacing optimized libraries wholesale offers no demonstrated
benefit. Future candidates must have a measured workload, a small ownership
boundary, and correctness tests. Network parsers and worker protocol validation
may benefit from memory safety, but are not changed by this first step.

Before making this option a package default: review this local candidate with the
user, evaluate the modest absolute saving against toolchain complexity and the
faster fused C++ alternative, validate the release's static OpenCV build and all
package builders, repeat relevant camera/recording tests, and obtain explicit
release approval. This experimental module alone does not justify a broad rewrite.

## Local DEB candidate

The local review package is `0.7.1-1+ubuntu26.04.local1`; the public product
version remains 0.7.1. It is derived from the checksum-verified public DEB and
replaces only `penguin-gpu-worker`, with Rust preprocessing enabled and OpenCV
4.10.0 core/imgproc linked statically using the existing project feature flags.
No temporary-library paths or additional runtime dependencies are required.
The AMD add-on remains the installed 0.7.1 package.

This static build passed the same ten tensor comparisons (maximum difference 0).
Its benchmark measured 163.724 us/frame for the previous OpenCV preprocessing,
35.582 us/frame for Rust and 17.345 us/frame for the fused C++ control. Its isolated
Wayland GPU recording fully decoded (30.100 seconds, 900 video frames, 1,405 audio
frames), without CPU fallback or reported render/encoder losses. The extracted
package also passed all 242 workspace checks and its recording decoded fully.
Logs reported clean shutdown, no warnings/errors and zero OBS memory leaks.
The local DEB was subsequently installed through the normal Ubuntu administrator
dialog. `dpkg-query` reports `0.7.1-1+ubuntu26.04.local1`; `dpkg --verify` passed,
and the installed GPU worker SHA-256 matches the tested binary. The installed
application repeated the native Wayland GPU recording successfully (30.100
seconds, 900 decoded video frames, 1,405 audio frames), with GPU active, no CPU
fallback, no reported render/encoder losses, no warning/error entries and zero
reported OBS memory leaks on exit. No live Facecam movement test was performed.

Local DEB SHA-256:
`fe94d347b734016d2fd3692dff724547908e83d288b6b3e3c3105fc0f64cc46f`.
Installed GPU worker SHA-256:
`da683af842057b983a41016522c697f84bbf6275f80a45a3a6b258062a00e3f2`.
No permanent administrator authorization was installed and nothing was published
to GitHub.

The user's workflow is local installation after validation, then user review,
then a public release only after explicit approval. The user chose system-wide
DEB installation with normal Ubuntu administrator confirmation and declined both
per-user installation and enabling a passwordless installer. No permanent
administrator authorization is part of this local candidate.

## Second local candidate: runtime-dispatched SIMD

Local build `0.7.1-1+ubuntu26.04.local2` adds AVX2 preprocessing inside the existing
Rust module. Public product version 0.7.1 is unchanged. Supported x86-64 CPUs
unpack and normalize eight BGRA pixels per iteration. Other CPUs retain the
scalar implementation; partial vectors use scalar processing. No frame allocation
or additional dependency is introduced. Runtime feature detection guards the
isolated target-feature function, following the [Rust runtime dispatch guidance](https://doc.rust-lang.org/core/arch/index.html).
The whole binary is not compiled with `target-cpu=native` or mandatory AVX2.

The worker also retains the original preprocessing path when worker-side hand
recovery is explicitly requested, because model preparation starts companion work.
Normal Penguin Studio operation continues to run its hand companion in the parent
process, as before.

Validation on the Ryzen 7 9800X3D:

- Debug and release Rust tests, formatting and Clippy with warnings denied pass.
- 8,320 combinations of widths 1–65, row padding and input offsets compare the
  dispatched result exactly against the scalar implementation, with output guards.
- 65 C ABI cases place input and output directly before inaccessible guard pages;
  ten OpenCV comparisons have maximum absolute difference zero.
- The C++ ABI harness passes ASan/UBSan/LeakSanitizer. Rust and static OpenCV are
  not sanitizer-instrumented; guard pages provide an additional boundary check.
- Consecutive runs of the old and new static-library benchmark measured 36.235
  versus 7.271 microseconds/frame for Rust (320×192; medians of nine 1,000-frame
  trials). A repeat measured 6.882 microseconds for the new path: roughly five
  times faster preprocessing, saving about 0.029 ms/frame versus local1.
  This gain comes from explicit vectorization, not an inherent language advantage.
  The generic fused C++ control measured about 17.3 microseconds; it is not an
  equally AVX2-specialized implementation. No whole-application FPS claim is made.
- The extracted DEB passed a native Wayland GPU recording on the RX 9070 XT using
  synthetic media: GPU active, no CPU fallback, complete decode of 29.966 seconds
  (899 video frames, 1,403 audio frames), no reported render/encoder losses,
  no warning/error entries and zero reported OBS memory leaks on exit.

The extracted package also passed all 242 workspace checks under Wayland; its
short recording decoded fully (97 video and 150 audio frames), and it exited
without warning/error entries or reported OBS memory leaks.

Only the worker binary differs from the public DEB's files; dependency simulation
upgrades only Penguin Studio. The AMD add-on remains unchanged. No live Facecam
movement test or non-AVX2 hardware run is claimed. The scalar implementation is
exercised by the equivalence tests. No GitHub publication or permanent
administrator authorization is part of this candidate.

Local DEB SHA-256:
`450b22b1d0c2cd6423c5ff634340da8ec3f7a72d6ac27cc422e6b2ce9a4b2c48`.
GPU worker SHA-256:
`f58b3444525780ad173f6c45e5506277acee09969177cc23fb51d178ee5630c0`.

The local2 DEB was installed with normal Ubuntu administrator confirmation.
`dpkg-query` confirms the version above, `dpkg --verify penguin-studio` passes,
and the installed worker matches the tested SHA-256. The installed application's
first Wayland recording decoded fully but reported three render-lag frames (0.3%).
A repeat without concurrent checks decoded 30.100 seconds (900 video frames,
1,405 audio frames), with GPU active, no CPU fallback and no reported render-lag
or encoder-skip events. Both runs exited cleanly with no warning/error entries
and zero reported OBS memory leaks. The isolated outlier's cause was not established;
these measurements do not promise loss-free rendering under all system loads.

Private synthetic test evidence is retained under
`/var/tmp/penguin-obs33/rust-local2-package-gpu`,
`/var/tmp/penguin-obs33/rust-local2-package-workspace`,
`/var/tmp/penguin-obs33/rust-local2-installed-gpu` and
`/var/tmp/penguin-obs33/rust-local2-installed-gpu-repeat`.
The package and installation log are in `/var/tmp/penguin-rust-local2`.
