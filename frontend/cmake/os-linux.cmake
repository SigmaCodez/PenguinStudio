target_sources(
  obs-studio
  PRIVATE
    utility/PenguinLinuxUpdater.cpp
    utility/PenguinLinuxUpdater.hpp
    utility/PenguinUpdatePolicy.hpp
    utility/CrashHandler_Linux.cpp
    utility/NativeEventFilter.cpp
    utility/platform-x11.cpp
    utility/system-info-posix.cpp
)
target_compile_definitions(
  obs-studio
  PRIVATE OBS_INSTALL_PREFIX="${OBS_INSTALL_PREFIX}" $<$<BOOL:${ENABLE_PORTABLE_CONFIG}>:ENABLE_PORTABLE_CONFIG>
)
target_link_libraries(obs-studio PRIVATE Qt::DBus)

if(Qt6_VERSION AND Qt6_VERSION VERSION_LESS "6.9.0")
  target_link_libraries(obs-studio PRIVATE Qt::GuiPrivate)
endif()

find_package(Libpci REQUIRED)
target_link_libraries(obs-studio PRIVATE Libpci::pci)

if(TARGET OBS::python)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  target_link_libraries(obs-studio PRIVATE Python::Python)
  target_link_options(obs-studio PRIVATE LINKER:-no-as-needed)
endif()

if(NOT DEFINED APPDATA_RELEASE_DATE)
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git log --tags -1 --pretty=%cd --date=short
      OUTPUT_VARIABLE APPDATA_RELEASE_DATE
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  elseif(EXISTS "${CMAKE_SOURCE_DIR}/cmake/.CMakeBuildNumber")
    file(TIMESTAMP "${CMAKE_SOURCE_DIR}/cmake/.CMakeBuildNumber" APPDATA_RELEASE_DATE "%Y-%m-%d")
  else()
    file(TIMESTAMP "${CMAKE_SOURCE_DIR}/CMakeLists.txt" APPDATA_RELEASE_DATE "%Y-%m-%d")
  endif()
endif()

if(NOT DEFINED GIT_HASH)
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git rev-parse HEAD
      OUTPUT_VARIABLE GIT_HASH
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  else()
    set(GIT_HASH "master")
  endif()
endif()

configure_file(cmake/linux/io.github.SigmaCodez.PenguinStudio.metainfo.xml.in io.github.SigmaCodez.PenguinStudio.metainfo.xml)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/io.github.SigmaCodez.PenguinStudio.metainfo.xml"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
)

install(FILES cmake/linux/io.github.SigmaCodez.PenguinStudio.desktop DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
install(PROGRAMS cmake/linux/penguin-studio DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(
  FILES "${CMAKE_SOURCE_DIR}/assets/branding/penguin-studio.svg"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps"
  RENAME io.github.SigmaCodez.PenguinStudio.svg
)

# Keep attribution available in every binary package, including sandboxed builds.
install(
  FILES "${CMAKE_SOURCE_DIR}/COPYING" "${CMAKE_SOURCE_DIR}/AUTHORS"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/penguin-studio"
)
if(ENABLE_BROWSER AND EXISTS "${CEF_ROOT_DIR}/LICENSE.txt")
  install(
    FILES "${CEF_ROOT_DIR}/LICENSE.txt"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/penguin-studio/cef"
  )
endif()

if(PENGUIN_BUILD_TESTS)
  add_executable(penguin-hardware-encoder "${CMAKE_SOURCE_DIR}/test/penguin-hardware-encoder.cpp")
  target_include_directories(penguin-hardware-encoder PRIVATE "${CMAKE_SOURCE_DIR}")
  target_link_libraries(penguin-hardware-encoder PRIVATE OBS::libobs)
  add_executable(penguin-update-flow "${CMAKE_SOURCE_DIR}/test/penguin-update-flow.cpp")
  target_include_directories(penguin-update-flow PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/frontend/api")
  target_link_libraries(penguin-update-flow PRIVATE OBS::libobs Qt::Widgets Qt::Network)
endif()
