# Incremental Rust adoption

Status: local development candidate based on 0.7.1. No product version change,
release tag, package publication or system installation is part of this change.
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
provenance from C++. The conversion itself uses safe slices and iterators. Panics
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
Installation is a separate step and must be verified after it completes.

The user's workflow is local installation after validation, then user review,
then a public release only after explicit approval. See
[restricted local updates](../scripts/local-update/README.md) for the prepared
password-free installer scope; it is not a general unattended package manager.
