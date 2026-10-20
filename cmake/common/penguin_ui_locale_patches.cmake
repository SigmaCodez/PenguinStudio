# Keep submodule API names and authors intact while adapting visible product text.
function(penguin_apply_ui_locale_patch component)
  find_package(Git REQUIRED)
  set(patch "${CMAKE_SOURCE_DIR}/patches/${component}-penguin-locale.patch")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${patch}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${component}"
    RESULT_VARIABLE applied OUTPUT_QUIET ERROR_QUIET
  )
  if(applied EQUAL 0)
    return()
  endif()
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --check "${patch}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${component}"
    RESULT_VARIABLE checked ERROR_VARIABLE error
  )
  if(NOT checked EQUAL 0)
    message(FATAL_ERROR "Review Penguin Studio ${component} UI translations: ${error}")
  endif()
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply "${patch}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${component}"
    RESULT_VARIABLE result
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Could not apply Penguin Studio ${component} UI translations")
  endif()
endfunction()
