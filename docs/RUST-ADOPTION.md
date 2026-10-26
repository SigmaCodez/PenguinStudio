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

## Third local candidate: hand-mask processing and avoided copies

The next local review build keeps public version 0.7.1. Work was split into
independent performance and portability checks, then integrated locally.

The dependency-free Rust library now also converts six-class hand-model logits
into the skin-alpha mask. It caches the skin-class exponential already computed
for the denominator instead of computing it twice, writes directly into the
existing output mask and validates dimensions, strides, alignment, address
arithmetic and nonoverlap at its C ABI boundary. Cropped rows and output padding
are supported without allocation. The original C++ implementation remains the
fallback if input validation fails or the experimental build option is disabled.
The main camera plugin and the isolated worker link the same Rust static library.

Two complementary optimizations stay in C++ where a rewrite offers no advantage:
single-channel CHW/HWC output skips a redundant temporary allocation and two
copies, and supported hand-component eligibility is calculated once per component
rather than once per pixel. Model behavior, thresholds, Gaussian blur and the
rule that hand recovery must never reduce the primary person mask are retained.
A direct Rust port of the component loops was measured and rejected because it
was slower; an optimized lookup port offered no consistent advantage over C++.

The Rust dispatch suite now runs the real C ABI under emulated Nehalem (no AVX2)
and Haswell (AVX2), in addition to native execution. The declared Rust 1.74 minimum
is checked separately. These are local checks and prepared CI changes; no GitHub
workflow has been published or run for this development candidate.

Focused results on the Ryzen 7 9800X3D:

- Six-class mask conversion at the production 256×154 crop: medians over nine
  alternating 100-frame trials were 619.806 µs for the previous C++ loop,
  329.685 µs for Rust and 559.809 µs for a C++ control caching the exponential.
  This saves about 0.29 ms per hand-model frame versus the previous loop; it is
  not a claim about total inference time or application FPS.
- Thirty strided layouts produced exactly equal finite mask values, with zero
  maximum difference. Thirty-six extreme/nonfinite variants retained NaN behavior
  (NaN payload equality is not promised). Sixty-five guard-page boundary cases
  passed. Rust tests also reject partial overlap and address arithmetic overflow.
- Single-channel output avoids about 3.2 µs of copying per 320×192 frame. Twelve
  single/multichannel comparisons and padded/special-value cases pass unchanged.
- Thirty-two hand-component merge comparisons preserve the previous result,
  leave the skin input unchanged and never reduce the primary matte. The focused
  loop prototype showed a workload-dependent gain from component lookup; this is
  kept in C++ and is not attributed to Rust.

Independent review caught an uninitialized-output contract issue before local
installation: OpenCV allocates the destination without initializing its floats.
The final Rust implementation therefore writes through `MaybeUninit<f32>` rather
than constructing initialized float references. This adds no zero-fill work and
also allows untouched output padding to remain uninitialized. The corrected
source passes seven Rust tests on 1.74, Clippy with warnings denied and all seven
CMake/CTest groups, including native/emulated dispatch and hand-softmax guards.

The local3 package is derived from the verified local2 package and changes only
the background-removal plugin and GPU worker. The experimental option remains
OFF by default for public package recipes. No additional runtime dependency or
AMD add-on update is required. Installation and package evidence follows below.

Local3 package validation:

- Corrected C ABI harness passes AddressSanitizer, UndefinedBehaviorSanitizer and
  LeakSanitizer; the Rust static library itself is not sanitizer-instrumented.
- Native Wayland GPU and CPU runs each decoded fully: 30.100 seconds, 900 video
  frames and 1,405 audio frames. GPU run had GPU active without CPU fallback;
  CPU run used the explicitly selected CPU path. Neither reported loss events,
  warning/error entries or OBS memory leaks on exit.
- The package passed all 242 workspace checks and decoded the short UI-test
  recording (97 video frames, 150 audio frames), with clean shutdown.
- Package comparison against local2 confirms only `obs-backgroundremoval.so`
  and `penguin-gpu-worker` changed. Dependency simulation upgrades only Penguin
  Studio, adding/removing no packages. Author/license files remain byte-identical.

These recording tests use synthetic media and isolated English profiles; they are
not a live Facecam motion evaluation. Evidence is stored privately under
`/var/tmp/penguin-obs33/rust-local3-package-{gpu,cpu,workspace}`. No GitHub release,
release tag or product version bump was created.

Local3 DEB SHA-256:
`21af541f07d27e22e89348e958d8cd72cb3a5826b509b32dda2d796b05c06b06`.

The corrected `0.7.1-1+ubuntu26.04.local3` DEB was installed through normal Ubuntu
administrator authorization. Package verification passes; the AMD add-on remains
0.7.1. The installed application's final GPU recording decoded fully (29.966
seconds, 899 video frames, 1,403 audio frames), with GPU active, no CPU fallback,
no reported loss events, no warning/error entries and zero reported OBS memory
leaks at shutdown. Evidence: `/var/tmp/penguin-obs33/rust-local3-installed-gpu`.

Installed plugin SHA-256:
`396172dbbb660fe5cfc5f87da793db9501d9f495dc67a4307f84c60d8d02fb84`.
Installed worker SHA-256:
`c315e06622b030a4f1fe9cde1cfdee3c2714c583a23fa245ba8f3deff40282a8`.
Both match the tested build. The local package and installation log are retained
in `/var/tmp/penguin-rust-local3`. User settings and accounts were not used by tests.

Tooling note: preparing the isolated minimum-version check also updated the local
rustup manager to 1.29.2. The active rustc/cargo toolchain remains 1.98.0; the
prepared CI installation explicitly disables rustup self-updates.

## Fourth local candidate: exact shortcuts and GPU resource reuse

Local4 keeps the public 0.7.1 version. It reduces work in the existing Rust
hand-mask converter and in the existing OBS graphics path, without changing the
models, shaders, mask thresholds, frame rate or inference resolution.

The Rust converter uses conservative finite-logit bounds to return the same
clamped alpha of exactly zero or one without evaluating six exponentials.
A non-skin maximum at least 2 above the skin logit implies a skin probability
below 0.136, safely below the existing 0.2 threshold. Skin at least 3 above every
other logit implies probability above 0.8, safely above the existing 0.65 upper
threshold. Nonfinite values retain the original path. Fixed 64-pixel blocks choose
an implementation using samples; every pixel still receives its own exact result.
Uncertain blocks retain the previous branchless loop. No new unsafe code, C ABI,
allocation or persistent classifier state is introduced.

A direct per-pixel shortcut prototype was rejected for making ambiguous contours
about 44% slower. The final block dispatch measured approximately 281 µs for mixed
logits, 333 µs for ambiguous logits, 43 µs for confident background and 65 µs for
confident skin, versus about 331 µs for local3. These are workload-dependent
320×192-camera hand-mask microbenchmarks (256×154 crop), not whole-app CPU savings.
Eight Rust tests pass on current Rust and 1.74. The comparisons include 100,000
finite bit patterns, 15,600 independent C++ boundary cases, strided/nonfinite
inputs, guard pages and mixed-block cases. An additional RGB SIMD conversion
was rejected: about 3.7 µs saving did not justify another unsafe SIMD kernel.

The graphics changes stay in C++ because OBS owns the graphics context/resources:

- Reuse the alpha texture with checked map/copy/unmap, correct row pitches and
  resizing. A failed upload preserves the source instead of showing a stale mask.
- Alternate two cached blur render targets and sample each preceding pass directly.
  This removes the initial full-frame copy and one copy after each blur iteration.
- Release resources in the graphics context; disabling blur releases its targets.
  Multi-pass blur can retain one additional BGRA scratch frame compared with the
  previous path. This trades bounded VRAM for fewer allocations and image copies.

Native RX 9070 XT/Wayland comparisons passed 100 alpha-upload cases and 72 blur
cases, including odd/padded dimensions, resizing, standard/focal blur and 1–8
passes. Maximum output-byte difference was zero. Reproducible native graphics
checks and instructions are under `plugins/penguin-backgroundremoval/tests/`.
These results establish eliminated GPU copies, not a measured system-wide GPU
utilization or power reduction.

Package A/B comparison against installed local3 used the same synthetic fixture,
RX 9070 XT, native Wayland session and isolated English profiles. Each image and
blur recording decoded 30.100 seconds, 900 video frames and 1,405 audio frames,
without reported loss events, warning/error entries or OBS leaks at shutdown.
The decoded RGBA frame at 20 seconds was byte-identical in both modes.

Plugin-call instrumentation over each entire application run (about 1,203 render
calls, including startup/shutdown) measured:

| Work | local3 | local4 |
| --- | ---: | ---: |
| Alpha texture creates | 1,203 | 1 |
| Extra full-frame copies, eight-pass blur | 10,827 | 0 |

All tracked resources were released. The new blur uses persistent render targets;
zero direct BGRA texture-create calls does not mean zero GPU storage allocation.
Alpha-hash sets matched, but their sequence/frequency varied slightly, so full
inference-sequence identity is not claimed. Direct graphics checks had zero byte
difference across all 172 cases. Comparison reports are privately retained under
`/var/tmp/penguin-obs33/rust-local4-candidate-{image,blur}/comparison.json`.

The final package lifecycle check switched image → blur → image → blur and
resized the synthetic input from 1254×1254 to 640×360 and then 960×540, retaining
the matting instance. All nine phase/dimension checks passed. The subsequent
30.100-second recording decoded 900 video and 1,405 audio frames, without
reported losses, warnings/errors or OBS leaks. All five render targets and the
alpha texture were released; no additional texture-copy calls occurred.

The CPU package recording also decoded 900 video and 1,405 audio frames without
reported losses or shutdown errors. All 242 native Wayland workspace checks
passed. These are isolated synthetic-media regressions, not a new live Facecam
movement assessment.

The local4 DEB changes only the background-removal module and GPU worker payloads
relative to local3. Other files, including author/license notices, are identical.
APT's simulation upgrades only Penguin Studio, adding/removing no packages.
Package SHA-256:
`8991ae5794c6fb3c6d6c97236ae7c167046d90d75c4c2d9efa4136d42f6cd841`.

System-wide installation of `0.7.1-1+ubuntu26.04.local4` completed through the
normal Ubuntu administrator confirmation. `dpkg --verify` passed and both
installed module/worker SHA-256 hashes matched the tested build. The installed
RX 9070 XT/native Wayland GPU-blur test decoded 30.100 seconds (900 video and
1,405 audio frames), with GPU processing active and no CPU fallback, reported
loss events, warning/error entries or OBS leaks. Instrumentation confirmed one
alpha texture, zero extra texture-copy calls and all three render targets freed.
Evidence: `/var/tmp/penguin-obs33/rust-local4-installed-blur/`.
No GitHub release, release tag or public version change was made.
