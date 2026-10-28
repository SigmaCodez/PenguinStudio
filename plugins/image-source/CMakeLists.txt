cmake_minimum_required(VERSION 3.28...3.30)

add_library(image-source MODULE)
add_library(OBS::image-source ALIAS image-source)

target_sources(image-source PRIVATE color-source.c image-source.c image-file-watch.h obs-slideshow.c obs-slideshow-mk2.c)

target_link_libraries(image-source PRIVATE OBS::libobs $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in image-source.rc)
  target_sources(image-source PRIVATE image-source.rc)
endif()

set_target_properties_obs(image-source PROPERTIES FOLDER plugins PREFIX "")

if(PENGUIN_BUILD_TESTS AND UNIX)
  find_package(Threads REQUIRED)
  add_executable(penguin-image-file-watch-test tests/file-watch.c)
  target_link_libraries(penguin-image-file-watch-test PRIVATE OBS::libobs Threads::Threads)
  add_test(NAME penguin-image-file-watch COMMAND penguin-image-file-watch-test)
  set_tests_properties(penguin-image-file-watch PROPERTIES TIMEOUT 15)
endif()
