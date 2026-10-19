#!/usr/bin/env bash
# Exercise native Wayland shutdown with isolated profiles and an observing helper.
set -euo pipefail
if (( $# != 2 )); then
  echo "Usage: $0 SHUTDOWN_PROBE_SO NEW_RESULTS_DIRECTORY" >&2
  exit 2
fi
probe=$(realpath "$1")
results=$(realpath -m "$2")
if [[ -e "$results" ]]; then
  echo "Use a new results directory to preserve previous evidence." >&2
  exit 2
fi
mkdir -p "$results"
for scenario in normal multi wizard stats restart signal; do
  profile=$scenario
  mode=$scenario
  if [[ "$scenario" == restart ]]; then
    profile=normal
    mode=normal
  fi
  mkdir -p "$results/$scenario" "$results/$profile/config/PenguinStudio" "$results/$profile/cache"
  if [[ "$scenario" != restart ]]; then
    printf '[General]\nLanguage=en-US\n' > "$results/$profile/config/PenguinStudio/user.ini"
  fi
  args=(--disable-updater)
  if [[ "$scenario" == multi || "$scenario" == wizard ]]; then
    args+=(--multi)
  fi
  env QT_QPA_PLATFORM=wayland XDG_CONFIG_HOME="$results/$profile/config" \
    XDG_CACHE_HOME="$results/$profile/cache" LD_PRELOAD="$probe" \
    PENGUIN_SHUTDOWN_MODE="$mode" PENGUIN_SHUTDOWN_REPORT="$results/$scenario/report.json" \
    timeout --kill-after=5s 40s penguin-studio "${args[@]}" > "$results/$scenario/application.log" 2>&1
  python3 - "$results" "$scenario" "$profile" <<'PY'
from pathlib import Path
import json
import re
import sys
root = Path(sys.argv[1])
scenario, profile = sys.argv[2:]
case = root / scenario
report = json.loads((case / 'report.json').read_text())
log = (case / 'application.log').read_text()
assert 'Platform: Wayland' in log
assert report['apiInitialized'] and report['apiDestroyed'] and report['closeRequested'], report
assert '[obs-websocket] [obs_module_load] Module loaded.' in log
assert '[obs-websocket] [obs_module_unload] Finished shutting down.' in log
assert report['unregisterCalls'] >= 4 and report['lateUnregisterCalls'] == 0, report
assert report['sentinelObserved'] == (scenario in ('normal', 'stats', 'restart', 'signal')), report
assert not list((root / profile / 'config/PenguinStudio/.sentinel').glob('run_*'))
if scenario == 'wizard':
    assert report['wizardVisibleAtClose'], report
if scenario == 'stats':
    assert report['statsOpened'], report
marker = '==== Shutting down'
assert marker in log
shutdown_log = log.split(marker, 1)[1]
errors = re.findall(r'^.*\berror:.*$', shutdown_log, re.MULTILINE | re.IGNORECASE)
assert not errors, errors
assert 'Number of memory leaks: 0' in shutdown_log
report.update(platform='wayland', exit_code=0, shutdown_errors=errors,
              shutdown_warnings=re.findall(r'^warning:.*$', shutdown_log, re.MULTILINE),
              sentinel_files_after_exit=0, obs_memory_leaks=0)
(case / 'verified.json').write_text(json.dumps(report, indent=2) + '\n')
print(f'PASS: {scenario}: zero late unregisters, zero shutdown errors, zero OBS memory leaks')
PY
done
