#!/usr/bin/env bash
# Launch only the isolated test build. Never run this against a personal profile.
set -euo pipefail

if (( $# < 3 || $# > 6 )); then
  echo "Usage: $0 FLATPAK_USER_DIR DEV_APP RESULTS_DIR [xcb|wayland] [SEED_TEST_PROFILE] [workspace|wizard-close|layout-save|layout-reopen]" >&2
  exit 2
fi

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
sdk_dir=$(realpath "$1")
app_dir=$(realpath "$2")
results_dir=$(realpath -m "$3")
platform=${4:-xcb}
seed_profile=${5:-}
test_mode=${6:-workspace}
if [[ "$test_mode" != workspace && "$test_mode" != wizard-close && "$test_mode" != layout-save && "$test_mode" != layout-reopen ]]; then
  echo "Unknown workspace test mode." >&2
  exit 2
fi
if [[ "$platform" != xcb && "$platform" != wayland ]]; then
  echo "The test platform must be xcb or wayland." >&2
  exit 2
fi
if [[ -e "$results_dir" ]]; then
  echo "Use a new results directory so previous validation evidence is preserved." >&2
  exit 2
fi
mkdir -p "$results_dir/config" "$results_dir/cache" "$results_dir/captures"
if [[ -n "$seed_profile" ]]; then
  python3 - "$seed_profile" "$results_dir/config" <<'PY'
from pathlib import Path
import shutil
import sys
source = Path(sys.argv[1]).resolve()
target = Path(sys.argv[2]).resolve()
shutil.copytree(source, target, dirs_exist_ok=True)
configuration = target / "PenguinStudio/global.ini"
if configuration.exists():
    configuration.write_text(configuration.read_text().replace(str(source), str(target)))
PY
fi

python3 - "$results_dir/config/PenguinStudio/user.ini" "$results_dir/captures" <<'PY'
import configparser
import os
from pathlib import Path
import sys
import math
import struct
import wave
path = Path(sys.argv[1])
settings = configparser.ConfigParser(interpolation=None)
settings.optionxform = str
if path.exists():
    settings.read(path)
if not settings.has_section("General"):
    settings.add_section("General")
settings.set("General", "Language", os.environ.get("PENGUIN_SMOKE_LOCALE", "en-US"))
if not settings.has_section("Basic"):
    settings.add_section("Basic")
settings.set("Basic", "Profile", "Untitled")
settings.set("Basic", "ProfileDir", "Untitled")
path.parent.mkdir(parents=True, exist_ok=True)
with path.open("w") as target:
    settings.write(target, space_around_delimiters=False)
captures = Path(sys.argv[2])
recordings = captures / "recordings"
recordings.mkdir()
profile = path.parent / "basic/profiles/Untitled/basic.ini"
profile_settings = configparser.ConfigParser(interpolation=None)
profile_settings.optionxform = str
if profile.exists():
    profile_settings.read(profile)
for section in ("General", "Output", "SimpleOutput"):
    if not profile_settings.has_section(section):
        profile_settings.add_section(section)
profile_settings.set("General", "Name", "Untitled")
profile_settings.set("Output", "Mode", "Simple")
for key, value in {"FilePath": str(recordings), "RecFormat2": "mkv", "RecQuality": "Small",
                   "StreamEncoder": "x264", "RecEncoder": "x264"}.items():
    profile_settings.set("SimpleOutput", key, value)
profile.parent.mkdir(parents=True, exist_ok=True)
with profile.open("w") as target:
    profile_settings.write(target, space_around_delimiters=False)
for name, frequency in (("Microphone test tone", 220), ("Desktop test tone", 440)):
    with wave.open(str(captures / f"{name}.wav"), "wb") as audio:
        audio.setparams((2, 2, 48000, 0, "NONE", "not compressed"))
        audio.writeframes(b"".join(struct.pack("<hh", int(4900*math.sin(2*math.pi*frequency*i/48000)),
                                             int(3900*math.sin(2*math.pi*frequency*i/48000))) for i in range(144000)))
PY

export FLATPAK_USER_DIR="$sdk_dir"
flatpak build --filesystem="$results_dir" --filesystem="$repo_dir:ro" "$app_dir" sh -c '
  c++ -std=c++17 -shared -fPIC -Wall -Wextra -Werror -I/app/include/obs \
    "$1/test/penguin-workspace-smoke.cpp" -o "$2/workspace-smoke.so" \
    $(pkg-config --cflags --libs Qt6Widgets x11 xcomposite) \
    -L/app/lib -lobs -lobs-frontend-api -pthread
' sh "$repo_dir" "$results_dir"

timeout 60s flatpak build --unshare=network --socket=x11 --socket=wayland --device=dri \
  --filesystem="$results_dir" --filesystem="$repo_dir:ro" "$app_dir" sh -c '
  export QT_QPA_PLATFORM="$3" XDG_CONFIG_HOME="$2/config" XDG_CACHE_HOME="$2/cache"
  export LD_PRELOAD="$2/workspace-smoke.so" PENGUIN_SMOKE_DIR="$2/captures"
  export PENGUIN_SMOKE_IMAGE="$1/assets/branding/penguin-studio.png"
  export PENGUIN_SMOKE_EXPECT_REDESIGN=1 PENGUIN_SMOKE_MODE="$4"
  exec /app/bin/penguin-studio --disable-updater
' sh "$repo_dir" "$results_dir" "$platform" "$test_mode" >"$results_dir/application.log" 2>&1

python3 - "$results_dir/captures/workspace-checks.json" "$app_dir" "$results_dir" "$test_mode" <<'PY'
from pathlib import Path
import json
import re
import subprocess
import sys
report = json.loads(Path(sys.argv[1]).read_text())
checks = report["checks"]
for check in checks:
    print(f'{"PASS" if check["passed"] else "FAIL"}: {check["name"]}')
if not checks or not all(check["passed"] for check in checks):
    raise SystemExit(1)
result_dir = Path(sys.argv[3])
log = (result_dir / "application.log").read_text()
leaks = re.findall(r"Number of memory leaks: (\d+)", log)
if not leaks or int(leaks[-1]) != 0:
    raise SystemExit("The application did not report a clean OBS shutdown.")
if sys.argv[4] == "workspace":
    recordings = list((result_dir / "captures/recordings").glob("*.mkv"))
    if len(recordings) != 1:
        raise SystemExit("Expected one local fixture recording.")
    probe = subprocess.run(["flatpak", "build", f"--filesystem={result_dir}", sys.argv[2], "ffprobe",
                            "-v", "error", "-show_streams", "-show_format", "-of", "json", str(recordings[0])],
                           text=True, capture_output=True, check=True)
    media = json.loads(probe.stdout)
    (result_dir / "recording-probe.json").write_text(json.dumps(media, indent=2) + "\n")
    types = {stream["codec_type"] for stream in media["streams"]}
    if not {"audio", "video"}.issubset(types) or float(media["format"]["duration"]) < 1:
        raise SystemExit("The recording must contain real video and audio.")
    print("PASS: local recording contains video and audio")
print(f'Workspace checks passed on {report["platform"]}.')
PY
