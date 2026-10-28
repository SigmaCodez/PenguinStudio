#!/usr/bin/env bash
# Run inside the selected distro's clean build environment as root.
set -euo pipefail
source /etc/os-release
case "$ID" in
  ubuntu|debian)
    apt-get -o Acquire::Retries=3 -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30 update
    apt-get -o Acquire::Retries=3 -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30 install -y --no-install-recommends \
      build-essential cmake ninja-build pkg-config git curl ca-certificates python3 cargo rustc \
      extra-cmake-modules dpkg-dev file \
      libavcodec-dev libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev \
      libswresample-dev libswscale-dev libx264-dev libcurl4-openssl-dev \
      libmbedtls-dev libjansson-dev libluajit-5.1-dev python3-dev \
      libx11-dev libxcb-randr0-dev libxcb-shm0-dev libxcb-xinerama0-dev \
      libxcb-composite0-dev libxcomposite-dev libxinerama-dev libxcb1-dev \
      libx11-xcb-dev libxcb-xfixes0-dev libxkbcommon-dev libxdamage-dev \
      libnss3 libnspr4 libxrandr2 libatk1.0-dev libatk-bridge2.0-dev swig libcmocka-dev libxss-dev \
      libglvnd-dev libgles2-mesa-dev libwayland-dev librist-dev libsrt-openssl-dev \
      libpci-dev libpipewire-0.3-dev libqrcodegencpp-dev uthash-dev libsimde-dev \
      qtkeychain-qt6-dev qt6-websockets-dev qt6-base-dev qt6-base-private-dev qt6-svg-dev qt6-wayland \
      libasound2-dev libfontconfig-dev libfreetype-dev libjack-jackd2-dev \
      libpulse-dev libsndio-dev libspeexdsp-dev libudev-dev libv4l-dev \
      libva-dev libvlc-dev libvpl-dev libdrm-dev \
      nlohmann-json3-dev libwebsocketpp-dev libasio-dev libffmpeg-nvenc-dev
    ;;
  fedora)
    dnf install -y "https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm"
    dnf install -y gcc gcc-c++ cmake ninja-build git python3 curl cargo rust \
      extra-cmake-modules rpm-build pkgconf-pkg-config file libuuid-devel glib2-devel zlib-ng-compat-devel \
      ffmpeg-free-devel libavcodec-free-devel libavdevice-free-devel \
      x264-devel libcurl-devel mbedtls-devel jansson-devel luajit-devel python3-devel \
      libX11-devel libXcomposite-devel libXinerama-devel libXrandr-devel \
      libXdamage-devel libXScrnSaver-devel libxcb-devel libxkbcommon-devel \
      at-spi2-atk-devel swig libcmocka-devel libglvnd-devel wayland-devel \
      librist-devel srt-devel pciutils-devel pipewire-devel qrencode-devel \
      libqrcodegencpp-devel uthash-devel simde-devel \
      qtkeychain-qt6-devel qt6-qtwebsockets-devel qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtsvg-devel qt6-qtwayland-devel \
      alsa-lib-devel fontconfig-devel freetype-devel jack-audio-connection-kit-devel \
      pulseaudio-libs-devel speexdsp-devel systemd-devel libv4l-devel \
      libva-devel vlc-devel libvpl-devel libdrm-devel json-devel websocketpp-devel \
      asio-devel nv-codec-headers
    ;;
  *) printf 'Unsupported dependency bootstrap distro: %s\n' "$ID" >&2; exit 2 ;;
esac
