cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(penguin-outputs MODULE)
add_library(OBS::penguin-outputs ALIAS penguin-outputs)

target_sources(penguin-outputs PRIVATE penguin-outputs.cpp)
target_link_libraries(penguin-outputs PRIVATE OBS::libobs OBS::frontend-api Qt::Widgets)

set_target_properties_obs(penguin-outputs PROPERTIES FOLDER plugins/penguin-outputs PREFIX "")

option(PENGUIN_BUILD_TESTS "Build offline Penguin output safety checks" OFF)
if(PENGUIN_BUILD_TESTS AND OS_LINUX)
  add_executable(penguin-output-safety tests/output-safety.cpp)
  target_include_directories(
    penguin-output-safety
    PRIVATE "${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/config" "${CMAKE_SOURCE_DIR}/frontend/api"
  )
  target_link_libraries(penguin-output-safety PRIVATE Qt::Widgets)
  target_compile_options(penguin-output-safety PRIVATE -fPIC -ffunction-sections -fdata-sections)
  target_link_options(penguin-output-safety PRIVATE -Wl,--gc-sections)

  add_executable(penguin-wizard-lifecycle tests/wizard-lifecycle.cpp)
  target_link_libraries(penguin-wizard-lifecycle PRIVATE OBS::libobs OBS::frontend-api Qt::Widgets)

  find_package(X11 REQUIRED)
  add_executable(penguin-local-stream tests/local-stream.cpp)
  target_link_libraries(penguin-local-stream PRIVATE OBS::libobs OBS::frontend-api Qt::Widgets X11::X11)
  if(ENABLE_WAYLAND)
    target_link_libraries(penguin-local-stream PRIVATE Wayland::Client)
  endif()
endif()
