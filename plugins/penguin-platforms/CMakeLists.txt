cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets)
find_package(Qt6 QUIET COMPONENTS WebEngineWidgets)
find_package(Qt6Keychain REQUIRED)
add_library(penguin-platforms MODULE)
target_sources(penguin-platforms PRIVATE penguin-platforms.cpp oauth-helpers.hpp twitch-chat.hpp youtube-chat.hpp chat-view.hpp web-chat.hpp category-picker.hpp credential-store.hpp)
target_link_libraries(penguin-platforms PRIVATE OBS::libobs OBS::frontend-api Qt::Widgets Qt::Network Qt::WebSockets qt6keychain)
set_target_properties_obs(penguin-platforms PROPERTIES FOLDER plugins PREFIX "")

if(PENGUIN_BUILD_TESTS AND OS_LINUX)
  add_executable(penguin-platform-flow tests/platform-flow.cpp)
  set_target_properties(penguin-platform-flow PROPERTIES AUTOMOC ON)
  target_include_directories(
    penguin-platform-flow
    PRIVATE "${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/config" "${CMAKE_SOURCE_DIR}/frontend/api"
  )
  target_link_libraries(penguin-platform-flow PRIVATE Qt::Widgets Qt::Network Qt::WebSockets qt6keychain)
  target_compile_options(penguin-platform-flow PRIVATE -fPIC -ffunction-sections -fdata-sections)
  target_link_options(penguin-platform-flow PRIVATE -Wl,--gc-sections)
endif()

if(TARGET Qt6::WebEngineWidgets)
  target_link_libraries(penguin-platforms PRIVATE Qt6::WebEngineWidgets)
  if(TARGET penguin-platform-flow)
    target_link_libraries(penguin-platform-flow PRIVATE Qt6::WebEngineWidgets)
  endif()
else()
  message(STATUS "Original platform web chat unavailable: keeping integrated API chat")
endif()
