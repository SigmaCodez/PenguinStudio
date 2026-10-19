# Keep the upstream submodule revision while carrying the frontend lifetime fix.
# Both checks fail after an incompatible upstream change, requiring patch review.
find_package(Git REQUIRED)
set(_penguin_websocket_patch "${CMAKE_SOURCE_DIR}/patches/obs-websocket-shutdown.patch")
execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${_penguin_websocket_patch}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/obs-websocket"
  RESULT_VARIABLE _penguin_patch_applied
  OUTPUT_QUIET ERROR_QUIET
)
if(NOT _penguin_patch_applied EQUAL 0)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --check "${_penguin_websocket_patch}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/obs-websocket"
    RESULT_VARIABLE _penguin_patch_check
    ERROR_VARIABLE _penguin_patch_error
  )
  if(NOT _penguin_patch_check EQUAL 0)
    message(FATAL_ERROR "Review the Penguin WebSocket shutdown patch: ${_penguin_patch_error}")
  endif()
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply "${_penguin_websocket_patch}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/obs-websocket"
    RESULT_VARIABLE _penguin_patch_result
  )
  if(NOT _penguin_patch_result EQUAL 0)
    message(FATAL_ERROR "Could not apply the Penguin WebSocket shutdown patch")
  endif()
endif()
