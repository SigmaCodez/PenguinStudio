#!/usr/bin/env bash
set -euo pipefail
package_format=${1:-DEB}
case "$package_format" in DEB|RPM) ;; *) echo 'Usage: build-native.sh DEB|RPM [build-directory]' >&2; exit 2 ;; esac
project_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir=${2:-"$project_root/build_penguin_${package_format,,}"}
cef_dir=${CEF_ROOT_DIR:-"$project_root/.deps/cef"}
if [[ ! -f "$cef_dir/include/cef_version.h" ]]; then
  python3 "$project_root/scripts/packaging/fetch-cef.py" "$cef_dir"
fi
if [[ "$package_format" == RPM ]]; then
  library_dir=lib64
else
  library_dir="lib/$(gcc -dumpmachine)"
fi
cmake -S "$project_root" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_INSTALL_LIBDIR="$library_dir" -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DOBS_VERSION_OVERRIDE=32.2.2 -DENABLE_WAYLAND=ON -DENABLE_BROWSER=ON \
  -DCEF_ROOT_DIR="$cef_dir" -DENABLE_AJA=OFF -DENABLE_VLC=ON \
  -DENABLE_WEBRTC=OFF -DENABLE_LIBFDK=OFF -DPENGUIN_BUILD_TESTS=ON \
  -DOBS_COMPILE_DEPRECATION_AS_WARNING=ON
cmake --build "$build_dir" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
"$build_dir/plugins/penguin-outputs/penguin-output-safety"
QT_QPA_PLATFORM=offscreen "$build_dir/plugins/penguin-platforms/penguin-platform-flow"
QT_QPA_PLATFORM=offscreen "$build_dir/plugins/penguin-outputs/penguin-wizard-lifecycle"
(cd "$build_dir" && cpack -G "$package_format")
