cmake_minimum_required(VERSION 3.28...3.30)

if(NOT TARGET penguin-camera)
  return()
endif()

include(FetchContent)
FetchContent_GetProperties(penguin_ort)

# Build only the two required OpenCV modules, privately and without optional
# downloads or device backends. This keeps all Linux package formats consistent.
function(penguin_build_camera_opencv)
  # OpenCV caches this legacy global variable. Do not redirect Penguin's test
  # executables away from the paths used by package verification scripts.
  if(DEFINED CACHE{EXECUTABLE_OUTPUT_PATH})
    set(saved_executable_output_path "$CACHE{EXECUTABLE_OUTPUT_PATH}")
    set(had_executable_output_path TRUE)
  endif()
  if(EXISTS "${CMAKE_BINARY_DIR}/CPackConfig.cmake")
    file(SHA256 "${CMAKE_BINARY_DIR}/CPackConfig.cmake" parent_package_hash)
  endif()
  # OpenCV unconditionally generates CPack files. Keep them separate from the
  # application's root CPackConfig, which owns our version and install tree.
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/opencv-packaging")
  set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/opencv-packaging/CPackConfig.cmake")
  set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/opencv-packaging/CPackSourceConfig.cmake")
  set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
  set(BUILD_LIST core,imgproc)
  set(BUILD_SHARED_LIBS OFF)
  set(BUILD_TESTS OFF)
  set(BUILD_PERF_TESTS OFF)
  set(BUILD_EXAMPLES OFF)
  set(BUILD_opencv_apps OFF)
  set(BUILD_JAVA OFF)
  set(BUILD_opencv_python2 OFF)
  set(BUILD_opencv_python3 OFF)
  set(WITH_IPP OFF)
  set(WITH_ITT OFF)
  set(WITH_OPENCL OFF)
  set(WITH_LAPACK OFF)
  set(WITH_TBB OFF)
  set(WITH_OPENMP OFF)
  set(WITH_CUDA OFF)
  set(OPENCV_GENERATE_PKGCONFIG OFF)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  FetchContent_Declare(
    penguin_cv
    URL https://github.com/opencv/opencv/archive/refs/tags/4.10.0.tar.gz
    URL_HASH SHA256=b2171af5be6b26f7a06b1229948bbb2bdaa74fcf5cd097e0af6378fce50a6eb9
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(penguin_cv)
  if(DEFINED parent_package_hash)
    file(SHA256 "${CMAKE_BINARY_DIR}/CPackConfig.cmake" current_package_hash)
    if(NOT parent_package_hash STREQUAL current_package_hash)
      message(FATAL_ERROR "OpenCV must not overwrite Penguin Studio's package configuration")
    endif()
  endif()
  if(had_executable_output_path)
    set(EXECUTABLE_OUTPUT_PATH "${saved_executable_output_path}" CACHE PATH "Output directory for applications" FORCE)
  else()
    unset(EXECUTABLE_OUTPUT_PATH CACHE)
  endif()
endfunction()
penguin_build_camera_opencv()
FetchContent_GetProperties(penguin_cv)

add_library(obs-backgroundremoval MODULE)
target_sources(
  obs-backgroundremoval PRIVATE
  vendor/src/plugin-main.c
  vendor/src/plugin-support.c
  vendor/src/background-filter-info.c
  vendor/src/background-filter.cpp
  vendor/src/ort-utils/ort-session-utils.cpp
  vendor/src/obs-utils/obs-utils.cpp
)
target_compile_features(obs-backgroundremoval PRIVATE cxx_std_20)
# Preserve upstream warnings without imposing our compiler-specific Werror policy.
set_property(TARGET obs-backgroundremoval PROPERTY COMPILE_WARNING_AS_ERROR OFF)
target_include_directories(
  obs-backgroundremoval PRIVATE
  "${penguin_ort_SOURCE_DIR}/include"
  "${penguin_cv_SOURCE_DIR}/modules/core/include"
  "${penguin_cv_SOURCE_DIR}/modules/imgproc/include"
  "${penguin_cv_BINARY_DIR}"
  "${CMAKE_BINARY_DIR}"
)
target_link_libraries(obs-backgroundremoval PRIVATE OBS::libobs opencv_core opencv_imgproc "${penguin_ort_SOURCE_DIR}/lib/libonnxruntime.so")
target_link_options(obs-backgroundremoval PRIVATE "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/vendor/src/exported_symbols_linux.map" "-Wl,-Bsymbolic")
set_target_properties_obs(obs-backgroundremoval PROPERTIES FOLDER plugins PREFIX "")
set_property(TARGET obs-backgroundremoval APPEND PROPERTY INSTALL_RPATH "$ORIGIN/penguin-camera")
install(DIRECTORY vendor/data/ DESTINATION "${OBS_DATA_DESTINATION}/obs-plugins/obs-backgroundremoval" COMPONENT Runtime)
install(FILES vendor/LICENSE PROVENANCE.md DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/penguin-studio/obs-backgroundremoval" COMPONENT Runtime)

install(FILES "${penguin_cv_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/penguin-studio/opencv" COMPONENT Runtime)

install(DIRECTORY vendor/LICENSES/ DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/penguin-studio/obs-backgroundremoval/LICENSES" COMPONENT Runtime)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/vendor/data/models/rvm_mobilenetv3_fp32.with_runtime_opt.ort" rvm_hash)
if(NOT rvm_hash STREQUAL "29ea41074104b9a76f210881226862f44f3e0a5cdadc4998f9bc6b626ee4bc41")
  message(FATAL_ERROR "The bundled RVM model does not match its reviewed checksum")
endif()

file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/vendor/data/models/selfie_multiclass_256x256.with_runtime_opt.ort" hand_model_hash)
if(NOT hand_model_hash STREQUAL "317054e916e36f47457a85afa3ef6a6cc8f5d35f3cb22e5358fd1a3e6cfb8eed")
  message(FATAL_ERROR "The bundled hand-detail model does not match its reviewed checksum")
endif()

file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/vendor/data/models/rvm_mobilenetv3_gpu.onnx" rvm_gpu_hash)
if(NOT rvm_gpu_hash STREQUAL "41377570179d331c11486ae9d9ccf10ef806fa66cf34a6ca3caaeeba0c3a871c")
  message(FATAL_ERROR "The fixed-shape RVM model does not match its reviewed checksum")
endif()

# The GPU driver lives outside the Qt/render process. A failed or slow worker
# leaves CPU matting available and is terminated when its filter is destroyed.
add_executable(penguin-gpu-worker gpu-worker.cpp vendor/src/plugin-support.c)
target_compile_features(penguin-gpu-worker PRIVATE cxx_std_20)
target_compile_definitions(penguin-gpu-worker PRIVATE ORT_API_MANUAL_INIT)
target_include_directories(penguin-gpu-worker PRIVATE
  "${penguin_ort_SOURCE_DIR}/include" "${penguin_cv_SOURCE_DIR}/modules/core/include"
  "${penguin_cv_SOURCE_DIR}/modules/imgproc/include" "${penguin_cv_BINARY_DIR}")
target_link_libraries(penguin-gpu-worker PRIVATE OBS::libobs opencv_core opencv_imgproc ${CMAKE_DL_LIBS})
set_property(TARGET penguin-gpu-worker PROPERTY COMPILE_WARNING_AS_ERROR OFF)
install(TARGETS penguin-gpu-worker RUNTIME DESTINATION "${OBS_PLUGIN_DESTINATION}/penguin-camera" COMPONENT Runtime)

set(PENGUIN_CAMERA_GPU_RUNTIME_DIR "" CACHE PATH "Optional verified private GPU runtime bundle")
if(PENGUIN_CAMERA_GPU_RUNTIME_DIR)
  if(NOT EXISTS "${PENGUIN_CAMERA_GPU_RUNTIME_DIR}/libonnxruntime.so.1")
    message(FATAL_ERROR "The optional camera GPU runtime is incomplete")
  endif()
  install(DIRECTORY "${PENGUIN_CAMERA_GPU_RUNTIME_DIR}/" DESTINATION "${OBS_PLUGIN_DESTINATION}/penguin-camera-gpu" COMPONENT Runtime)
endif()
