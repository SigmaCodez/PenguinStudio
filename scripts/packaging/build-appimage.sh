#!/usr/bin/env bash
# Package the tested Ubuntu 26.04 binaries; run on that distribution after build-native.sh.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
deb=$(realpath "${1:?Usage: build-appimage.sh ubuntu26.04.deb [output-directory]}")
out=$(realpath -m "${2:-$root/build_penguin_appimage}")
version=$(cat "$root/packaging/version.txt")
source /etc/os-release
[[ "$ID" == ubuntu && "$VERSION_ID" == 26.04 && $(uname -m) == x86_64 ]]
[[ $(dpkg-deb -f "$deb" Package) == penguin-studio ]]
[[ $(dpkg-deb -f "$deb" Version) == "$version"-*+ubuntu26.04 ]]
mkdir -p "$out/tools"
appdir="$out/AppDir"
[[ ! -e "$appdir" ]] || { echo 'Use a fresh AppImage output directory' >&2; exit 1; }
dpkg-deb -x "$deb" "$appdir"
python3 - "$root/packaging/appimage/tools.json" "$out/tools" <<'PY'
import hashlib, json, pathlib, sys, urllib.request
for tool in json.loads(pathlib.Path(sys.argv[1]).read_text()):
    path = pathlib.Path(sys.argv[2]) / tool['name']
    urllib.request.urlretrieve(tool['url'], path)
    if hashlib.sha256(path.read_bytes()).hexdigest() != tool['sha256']:
        raise SystemExit(f"Checksum mismatch: {tool['name']}; review the upstream release before updating the pin")
    path.chmod(0o755)
PY
# OBS initializes Python at startup. Shipping libpython without its matching
# standard library would abort on hosts with a different Python version.
python_stdlib=$(python3 -c 'import sysconfig; print(sysconfig.get_path("stdlib"))')
cp -aL "$python_stdlib" "$appdir/usr/lib/"
install -m755 "$root/packaging/appimage/penguin-studio" "$appdir/usr/bin/penguin-studio"
# linuxdeploy cannot discover dlopen-loaded OBS modules from the executable alone.
libraries=()
while IFS= read -r -d '' lib; do libraries+=(--library "$lib"); done \
  < <(find "$appdir/usr/lib" -type f -name '*.so*' -print0)
export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE=/usr/bin/qmake6
qt_plugins=$($QMAKE -query QT_INSTALL_PLUGINS)
for plugin in platforms/libqwayland.so iconengines/libqsvgicon.so imageformats/libqsvg.so; do
  [[ -f "$qt_plugins/$plugin" ]] || { echo "Missing Qt runtime plugin: $plugin" >&2; exit 1; }
done
# Qt 6.10 consolidates the former Wayland EGL/generic platform plugins.
export EXTRA_PLATFORM_PLUGINS='libqwayland.so'
export EXTRA_QT_MODULES='svg;waylandclient'
export LD_LIBRARY_PATH="$appdir/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export NO_STRIP=1
"$out/tools/linuxdeploy-x86_64.AppImage" --appdir "$appdir" --exclude-library 'libva*.so*' \
  --executable "$appdir/usr/bin/obs" --executable "$appdir/usr/bin/obs-ffmpeg-mux" \
  "${libraries[@]}" --plugin qt \
  --desktop-file "$appdir/usr/share/applications/io.github.SigmaCodez.PenguinStudio.desktop" \
  --icon-file "$appdir/usr/share/icons/hicolor/scalable/apps/io.github.SigmaCodez.PenguinStudio.svg"
# Qt's platform plugins also need the Wayland shell/graphics integrations.
# The Qt deploy plugin does not currently copy these Qt 6 subdirectories.
for integration in wayland-decoration-client wayland-graphics-integration-client wayland-shell-integration; do
  cp -a "$qt_plugins/$integration" "$appdir/usr/plugins/"
done
test -d "$appdir/usr/plugins/wayland-shell-integration"
test -d "$appdir/usr/plugins/wayland-graphics-integration-client"
# Retain dependency copyright notices as well as Penguin/OBS/CEF licenses from the DEB.
mkdir -p "$appdir/usr/share/licenses/penguin-studio/bundled-dependencies"
for copyright in /usr/share/doc/*/copyright; do
  [[ -f "$copyright" ]] || continue
  package=$(basename "$(dirname "$copyright")")
  cp -L "$copyright" "$appdir/usr/share/licenses/penguin-studio/bundled-dependencies/$package.txt"
done
export OUTPUT="$out/penguin-studio-$version-x86_64.AppImage"
export ARCH=x86_64
export LDAI_RUNTIME_FILE="$out/tools/runtime-x86_64"
"$out/tools/linuxdeploy-x86_64.AppImage" --appdir "$appdir" --exclude-library 'libva*.so*' --output appimage
# --appimage-extract-and-run works without FUSE, including CI containers.
"$OUTPUT" --appimage-extract-and-run --version | tee "$out/launcher-version.txt"
grep -F "Penguin Studio $version" "$out/launcher-version.txt"
(cd "$out" && sha256sum "$(basename "$OUTPUT")" > "$(basename "$OUTPUT").sha256")

# A CLI version check alone does not initialize Qt plugins, Python or OBS modules.
c++ -std=c++17 -fPIC -shared "$root/test/penguin-portable-smoke.cpp" \
  $(pkg-config --cflags --libs Qt6Widgets) -o "$out/gui-probe.so"
mkdir -p "$out/gui-config/PenguinStudio/basic/profiles/Test"
cat > "$out/gui-config/PenguinStudio/user.ini" <<'PROFILE'
[General]
Language=en-US
FirstRun=true
[Basic]
Profile=Test
ProfileDir=Test
[Updates]
AutoCheck=false
PROFILE
cat > "$out/gui-config/PenguinStudio/basic/profiles/Test/basic.ini" <<'PROFILE'
[General]
Name=Test
[Video]
BaseCX=1280
BaseCY=720
OutputCX=1280
OutputCY=720
FPSType=0
FPSCommon=30
PROFILE
xvfb-run -a timeout --kill-after=5s 45s env -u LD_LIBRARY_PATH \
  LD_PRELOAD="$out/gui-probe.so" XDG_CONFIG_HOME="$out/gui-config" \
  XDG_CACHE_HOME="$out/gui-cache" QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 \
  PENGUIN_TEST_RESULT="$out/gui-result.json" \
  "$OUTPUT" --appimage-extract-and-run --multi > "$out/gui.log" 2>&1
python3 - "$out/gui-result.json" <<'PYTEST'
import json, pathlib, sys
result = json.loads(pathlib.Path(sys.argv[1]).read_text())
assert result['main_window_visible'], result
assert result['platform'] == 'xcb', result
print('AppImage graphical startup and normal shutdown passed')
PYTEST

if grep -E 'QMetaMethod::invoke:.*unregistered|Fatal Python error' "$out/gui.log"; then
  echo 'Portable GUI runtime regression detected' >&2
  exit 1
fi
