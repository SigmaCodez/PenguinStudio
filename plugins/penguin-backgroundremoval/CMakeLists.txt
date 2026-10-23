cmake_minimum_required(VERSION 3.28...3.30)

if(NOT TARGET penguin-camera)
  return()
endif()

include(FetchContent)
FetchContent_GetProperties(penguin_ort)

# Build only the two required OpenCV modules, privately and without optional
# downloads or device backends. This keeps all Linux package formats consistent.
function(penguin_build_camera_opencv)
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
