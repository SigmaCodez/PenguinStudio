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
  DEPENDS Cargo.toml Cargo.lock src/lib.rs
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
  add_test(NAME penguin-rust-preprocess-unit COMMAND "${PENGUIN_CARGO}" test --locked --offline
    --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml" --target-dir "${rust_target_dir}")
endif()
