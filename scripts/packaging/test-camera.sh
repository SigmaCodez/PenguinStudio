#!/usr/bin/env bash
# Pass a compiled test/penguin-camera-gui.cpp preload helper and a NEW directory.
set -euo pipefail
probe=$(realpath "${1:?Usage: test-camera.sh HELPER_SO RESULTS_DIR}")
results=$(realpath -m "${2:?Missing results directory}")
[[ ! -e "$results" ]] || { echo 'Use a new test directory' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
mkdir -p "$results/config/PenguinStudio/basic/profiles/Test"
cat > "$results/config/PenguinStudio/user.ini" <<'PROFILE'
[General]
Language=en-US
FirstRun=true
[Basic]
Profile=Test
ProfileDir=Test
[Updates]
AutoCheck=false
PROFILE
cat > "$results/config/PenguinStudio/basic/profiles/Test/basic.ini" <<'PROFILE'
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
timeout --kill-after=5s 35s env XDG_CONFIG_HOME="$results/config" XDG_CACHE_HOME="$results/cache" \
  LD_PRELOAD="$probe" PENGUIN_CAMERA_RESULTS="$results" \
  "${PENGUIN_EXECUTABLE:-penguin-studio}" --multi --disable-updater > "$results/application.log" 2>&1
python3 "$root/test/penguin-camera-pixels.py" "$results" "${PENGUIN_CAMERA_UI_CHECK:---inspector}"
if grep -Ei 'error:|warning:|segmentation fault|assertion.*failed' "$results/application.log"; then
  echo 'Camera test emitted diagnostics requiring investigation' >&2
  exit 1
fi
grep -F 'Number of memory leaks: 0' "$results/application.log"
