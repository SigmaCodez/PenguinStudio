cmake_minimum_required(VERSION 3.28...3.30)

# Official Linux packages currently target x86_64. Other platforms keep the
# existing source filters without downloading an incompatible inference runtime.
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
  return()
endif()
include(FetchContent)
FetchContent_Declare(
  penguin_ort
  URL https://github.com/microsoft/onnxruntime/releases/download/v1.23.2/onnxruntime-linux-x64-1.23.2.tgz
  URL_HASH SHA256=1fa4dcaef22f6f7d5cd81b28c2800414350c10116f5fdd46a2160082551c5f9b
)
FetchContent_MakeAvailable(penguin_ort)
add_library(penguin-camera MODULE penguin-camera.cpp)
target_include_directories(penguin-camera PRIVATE "${penguin_ort_SOURCE_DIR}/include")
target_link_libraries(penguin-camera PRIVATE OBS::libobs "${penguin_ort_SOURCE_DIR}/lib/libonnxruntime.so")
set_target_properties_obs(penguin-camera PROPERTIES FOLDER plugins PREFIX "")
# Keep this runtime private to our plugin, including inside portable packages.
set_property(TARGET penguin-camera APPEND PROPERTY INSTALL_RPATH "$ORIGIN/../penguin-camera")
install(DIRECTORY "${penguin_ort_SOURCE_DIR}/lib/" DESTINATION "${OBS_PLUGIN_DESTINATION}/penguin-camera"
  COMPONENT Runtime FILES_MATCHING PATTERN "libonnxruntime.so*")

file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/data/selfie_segmentation.onnx" camera_model_hash)
if(NOT camera_model_hash STREQUAL "1511ab5564cb0ae179cba9bcad6ad931003ccc71656f11fd270967023678c6a5")
  message(FATAL_ERROR "The bundled camera model does not match its reviewed checksum")
endif()
if(PENGUIN_BUILD_TESTS)
  add_executable(penguin-camera-model "${CMAKE_SOURCE_DIR}/test/penguin-camera-model.cpp")
  target_include_directories(penguin-camera-model PRIVATE "${CMAKE_SOURCE_DIR}" "${penguin_ort_SOURCE_DIR}/include")
  target_link_libraries(penguin-camera-model PRIVATE "${penguin_ort_SOURCE_DIR}/lib/libonnxruntime.so")
endif()

install(FILES "${penguin_ort_SOURCE_DIR}/LICENSE" "${penguin_ort_SOURCE_DIR}/ThirdPartyNotices.txt"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/penguin-studio/onnxruntime" COMPONENT Runtime)
