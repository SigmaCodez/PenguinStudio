# OBS CMake Linux CPack configuration module

include_guard(GLOBAL)

include(cpackconfig_common)

# Add GPLv2 license file to CPack
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/frontend/data/license/gplv2.txt")
set(CPACK_PACKAGE_EXECUTABLES "penguin-studio" "Penguin Studio")
set(CPACK_PACKAGE_NAME "penguin-studio")
set(CPACK_PACKAGE_VENDOR "SigmaCodez")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/SigmaCodez/PenguinStudio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Penguin Studio streaming and recording studio")
if(DEFINED PENGUIN_PACKAGE_DISTRO)
  string(APPEND CPACK_PACKAGE_DESCRIPTION_SUMMARY " for ${PENGUIN_PACKAGE_DISTRO}")
endif()

if(ENABLE_RELEASE_BUILD)
  set(CPACK_PACKAGE_VERSION "${OBS_VERSION_CANONICAL}")
else()
  set(CPACK_PACKAGE_VERSION "${OBS_VERSION}")
endif()

file(STRINGS "${CMAKE_SOURCE_DIR}/packaging/version.txt" CPACK_PACKAGE_VERSION LIMIT_COUNT 1)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "penguin-studio-${CPACK_PACKAGE_VERSION}-sources")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/.git" "${CMAKE_BINARY_DIR}" "/.ccache" "/.deps")
set(CPACK_ARCHIVE_THREADS 0)

if(OS_LINUX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS TRUE)
  set(CPACK_SET_DESTDIR TRUE)
  set(CPACK_DEBIAN_DEBUGINFO_PACKAGE TRUE)
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SigmaCodez")
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_PACKAGE_SECTION "video")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-wayland, qt6-svg-plugins, pkexec")
  set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "pipewire, xdg-desktop-portal, mesa-va-drivers")
  # Penguin Studio retains libobs ABI and install paths. A native OBS install owns these files.
  set(CPACK_DEBIAN_PACKAGE_CONFLICTS "obs-studio, obs-studio-common, obs-plugins, libobs0, libobs0t64, libobs-dev")
  set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
  set(CPACK_RPM_PACKAGE_LICENSE "GPL-2.0-or-later")
  set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
  set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtwayland, polkit")
  set(CPACK_RPM_PACKAGE_SUGGESTS "pipewire, xdg-desktop-portal")
  set(
    CPACK_RPM_PACKAGE_CONFLICTS
    "obs-studio, obs-studio-libs, obs-studio-devel, obs-studio-plugin-browser, obs-studio-plugin-vlc"
  )
  set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
elseif(OS_FREEBSD)
  set(CPACK_GENERATOR "FREEBSD")

  set(CPACK_FREEBSD_PACKAGE_MAINTAINER "${OBS_COMPANY_NAME}")
  set(CPACK_FREEBSD_PACKAGE_LICENSE "GPLv2")

  set(
    CPACK_FREEBSD_PACKAGE_DEPS
    "audio/alsa-lib"
    "audio/fdk-aac"
    "audio/jack"
    "audio/pulseaudio"
    "audio/sndio"
    "devel/jansson"
    "devel/libpci"
    "devel/libsysinfo"
    "devel/nlohmann-json"
    "devel/qt6-base"
    "devel/qt6-svg"
    "devel/swig"
    "devel/websocketpp"
    "ftp/curl"
    "graphics/mesa-libs"
    "graphics/qr-code-generator"
    "lang/luajit"
    "lang/python39"
    "misc/e2fsprogs-libuuid"
    "multimedia/ffmpeg"
    "multimedia/librist"
    "multimedia/pipewire"
    "multimedia/v4l_compat"
    "multimedia/vlc"
    "net/asio"
    "www/libdatachannel"
    "www/srt"
  )
endif()

include(CPack)
