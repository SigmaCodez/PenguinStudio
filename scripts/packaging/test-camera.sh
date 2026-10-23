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
timeout --kill-after=5s 50s env XDG_CONFIG_HOME="$results/config" XDG_CACHE_HOME="$results/cache" XDG_DATA_HOME="$results/data" \
  LD_PRELOAD="$probe" PENGUIN_CAMERA_RESULTS="$results" \
  "${PENGUIN_EXECUTABLE:-penguin-studio}" --multi --disable-updater > "$results/application.log" 2>&1
python3 "$root/test/penguin-camera-pixels.py" "$results" "${PENGUIN_CAMERA_UI_CHECK:---inspector}"
# Headless CI has no portal settings service, DRM render node or window manager.
# Retain every diagnostic in the log, allowing only these exact environment
# messages there. Native desktop checks remain strict.
python3 - "$results/application.log" "${PENGUIN_CAMERA_HEADLESS:-0}" <<'CHECK'
import pathlib, re, sys
allowed = {
    "warning: Get on org.freedesktop.portal.Settings returned an invalid reply",
    "warning: Unable to open DRM render node.",
    "error: window manager does not support Extended Window Manager Hints (EWMH).",
} if sys.argv[2] == "1" else set()
lines = pathlib.Path(sys.argv[1]).read_text().splitlines()
failures = [line for line in lines
            if re.search(r"error:|warning:|segmentation fault|assertion.*failed", line, re.I)
            and line not in allowed]
if failures:
    raise SystemExit("Camera test emitted unexpected diagnostics:\n" + "\n".join(failures))
CHECK
grep -F 'Number of memory leaks: 0' "$results/application.log"
