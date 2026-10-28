# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.28)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(PenguinRustPreprocess LANGUAGES CXX)
  enable_testing()
  set(PENGUIN_RUST_PREPROCESS_TESTS ON)
endif()
if(CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "Rust preprocessing currently requires a native build")
endif()
find_program(PENGUIN_CARGO cargo HINTS "$ENV{HOME}/.cargo/bin" REQUIRED)
set(rust_target_dir "${CMAKE_CURRENT_BINARY_DIR}/cargo")
set(rust_library "${rust_target_dir}/release/libpenguin_camera_preprocess.a")
add_custom_command(
  OUTPUT "${rust_library}"
  COMMAND "${PENGUIN_CARGO}" build --release --locked --offline
    --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml" --target-dir "${rust_target_dir}"
  DEPENDS Cargo.toml Cargo.lock src/lib.rs src/hand_softmax.rs
  VERBATIM
)
add_custom_target(penguin-rust-preprocess-build DEPENDS "${rust_library}")
add_library(penguin-rust-preprocess STATIC IMPORTED GLOBAL)
set_target_properties(penguin-rust-preprocess PROPERTIES
  IMPORTED_LOCATION "${rust_library}"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
add_dependencies(penguin-rust-preprocess penguin-rust-preprocess-build)
# Rust std's platform dependencies (no additional installed Rust runtime).
find_package(Threads REQUIRED)
target_link_libraries(penguin-rust-preprocess INTERFACE Threads::Threads ${CMAKE_DL_LIBS} m)
if(PENGUIN_RUST_PREPROCESS_TESTS)
  if(NOT TARGET opencv_core)
    find_path(OPENCV_INCLUDE_DIR opencv2/core.hpp PATH_SUFFIXES opencv4 REQUIRED)
    find_library(OPENCV_CORE opencv_core REQUIRED)
    find_library(OPENCV_IMGPROC opencv_imgproc REQUIRED)
  else()
    set(OPENCV_CORE opencv_core)
    set(OPENCV_IMGPROC opencv_imgproc)
  endif()
  add_executable(penguin-rust-preprocess-test test.cpp)
  target_compile_features(penguin-rust-preprocess-test PRIVATE cxx_std_17)
  target_include_directories(penguin-rust-preprocess-test PRIVATE ${OPENCV_INCLUDE_DIR})
  target_link_libraries(penguin-rust-preprocess-test PRIVATE penguin-rust-preprocess ${OPENCV_CORE} ${OPENCV_IMGPROC})
  add_test(NAME penguin-rust-preprocess-equivalence COMMAND penguin-rust-preprocess-test)
  # This small C ABI test has no OpenCV dependency and can also run under a
  # CPU emulator to exercise real runtime feature detection, not a forced branch.
  add_executable(penguin-rust-dispatch-test dispatch-test.cpp)
  target_compile_features(penguin-rust-dispatch-test PRIVATE cxx_std_17)
  target_link_libraries(penguin-rust-dispatch-test PRIVATE penguin-rust-preprocess)
  add_test(NAME penguin-rust-dispatch-native COMMAND penguin-rust-dispatch-test)
  add_executable(penguin-rust-hand-softmax-test hand-softmax-test.cpp)
  target_compile_features(penguin-rust-hand-softmax-test PRIVATE cxx_std_17)
  target_link_libraries(penguin-rust-hand-softmax-test PRIVATE penguin-rust-preprocess)
  add_test(NAME penguin-rust-hand-softmax-equivalence COMMAND penguin-rust-hand-softmax-test)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    find_program(PENGUIN_QEMU_X86_64 qemu-x86_64)
    if(PENGUIN_QEMU_X86_64)
      add_test(NAME penguin-rust-dispatch-scalar
        COMMAND "${PENGUIN_QEMU_X86_64}" -cpu Nehalem $<TARGET_FILE:penguin-rust-dispatch-test> --require-no-avx2)
      add_test(NAME penguin-rust-dispatch-avx2
        COMMAND "${PENGUIN_QEMU_X86_64}" -cpu Haswell,-hle,-rtm $<TARGET_FILE:penguin-rust-dispatch-test> --require-avx2)
      set_tests_properties(penguin-rust-dispatch-scalar penguin-rust-dispatch-avx2 PROPERTIES TIMEOUT 60)
    endif()
  endif()
  add_test(NAME penguin-rust-preprocess-unit COMMAND "${PENGUIN_CARGO}" test --locked --offline
    --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml" --target-dir "${rust_target_dir}")
  option(PENGUIN_CAMERA_POSTPROCESS_TESTS "Test camera model postprocessing (requires OBS and ORT headers)" OFF)
  if(PENGUIN_CAMERA_POSTPROCESS_TESTS)
    find_path(PENGUIN_POSTPROCESS_OBS_INCLUDE_DIR obs-module.h PATH_SUFFIXES obs REQUIRED)
    find_path(PENGUIN_POSTPROCESS_ORT_INCLUDE_DIR onnxruntime_cxx_api.h
      HINTS "${penguin_ort_SOURCE_DIR}/include" PATH_SUFFIXES onnxruntime REQUIRED)
    add_executable(penguin-camera-postprocess-test ../tests/postprocess.cpp)
    target_compile_features(penguin-camera-postprocess-test PRIVATE cxx_std_17)
    target_compile_definitions(penguin-camera-postprocess-test PRIVATE ORT_API_MANUAL_INIT NDEBUG)
    target_include_directories(penguin-camera-postprocess-test PRIVATE ${OPENCV_INCLUDE_DIR}
      "${PENGUIN_POSTPROCESS_OBS_INCLUDE_DIR}" "${PENGUIN_POSTPROCESS_ORT_INCLUDE_DIR}")
    target_link_libraries(penguin-camera-postprocess-test PRIVATE ${OPENCV_CORE} ${OPENCV_IMGPROC})
    add_test(NAME penguin-camera-postprocess-equivalence COMMAND penguin-camera-postprocess-test)
    add_executable(penguin-rvm-preprocess-test ../tests/rvm-preprocess.cpp)
    target_compile_features(penguin-rvm-preprocess-test PRIVATE cxx_std_20)
    target_compile_definitions(penguin-rvm-preprocess-test PRIVATE ORT_API_MANUAL_INIT NDEBUG PENGUIN_RUST_PREPROCESS=1)
    target_include_directories(penguin-rvm-preprocess-test PRIVATE ${OPENCV_INCLUDE_DIR}
      "${PENGUIN_POSTPROCESS_OBS_INCLUDE_DIR}" "${PENGUIN_POSTPROCESS_ORT_INCLUDE_DIR}")
    if(TARGET OBS::libobs)
      set(PENGUIN_TEST_OBS OBS::libobs)
    else()
      find_library(PENGUIN_TEST_OBS obs REQUIRED)
    endif()
    target_link_libraries(penguin-rvm-preprocess-test PRIVATE penguin-rust-preprocess
      ${OPENCV_CORE} ${OPENCV_IMGPROC} ${PENGUIN_TEST_OBS})
    add_test(NAME penguin-rvm-preprocess-equivalence COMMAND penguin-rvm-preprocess-test)
    find_library(PENGUIN_TEST_ORT onnxruntime HINTS "${PENGUIN_POSTPROCESS_ORT_INCLUDE_DIR}/../lib" REQUIRED)
    add_executable(penguin-hand-buffers-test ../tests/hand-buffers.cpp)
    target_compile_features(penguin-hand-buffers-test PRIVATE cxx_std_20)
    target_compile_definitions(penguin-hand-buffers-test PRIVATE ORT_API_MANUAL_INIT NDEBUG PENGUIN_RUST_PREPROCESS=1)
    target_include_directories(penguin-hand-buffers-test PRIVATE ${OPENCV_INCLUDE_DIR}
      "${PENGUIN_POSTPROCESS_OBS_INCLUDE_DIR}" "${PENGUIN_POSTPROCESS_ORT_INCLUDE_DIR}")
    target_link_libraries(penguin-hand-buffers-test PRIVATE penguin-rust-preprocess
      ${OPENCV_CORE} ${OPENCV_IMGPROC} ${PENGUIN_TEST_OBS} ${PENGUIN_TEST_ORT})
    add_test(NAME penguin-hand-buffers-equivalence COMMAND penguin-hand-buffers-test
      "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/data/models/selfie_multiclass_256x256.with_runtime_opt.ort")


  endif()
endif()
