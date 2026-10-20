cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets)
add_library(penguin-platforms MODULE)
target_sources(penguin-platforms PRIVATE penguin-platforms.cpp oauth-helpers.hpp twitch-chat.hpp)
target_link_libraries(penguin-platforms PRIVATE OBS::libobs OBS::frontend-api Qt::Widgets Qt::Network Qt::WebSockets)
set_target_properties_obs(penguin-platforms PROPERTIES FOLDER plugins PREFIX "")

if(PENGUIN_BUILD_TESTS AND OS_LINUX)
  add_executable(penguin-platform-flow tests/platform-flow.cpp)
  set_target_properties(penguin-platform-flow PROPERTIES AUTOMOC ON)
  target_include_directories(
    penguin-platform-flow
    PRIVATE "${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/config" "${CMAKE_SOURCE_DIR}/frontend/api"
  )
  target_link_libraries(penguin-platform-flow PRIVATE Qt::Widgets Qt::Network Qt::WebSockets)
  target_compile_options(penguin-platform-flow PRIVATE -fPIC -ffunction-sections -fdata-sections)
  target_link_options(penguin-platform-flow PRIVATE -Wl,--gc-sections)
endif()
