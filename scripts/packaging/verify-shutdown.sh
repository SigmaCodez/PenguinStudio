#!/usr/bin/env bash
# Exercise native Wayland shutdown with isolated profiles and an observing helper.
set -euo pipefail
if (( $# != 2 )); then
  echo "Usage: $0 SHUTDOWN_PROBE_SO NEW_RESULTS_DIRECTORY" >&2
  exit 2
fi
executable=${PENGUIN_EXECUTABLE:-penguin-studio}
probe=$(realpath "$1")
results=$(realpath -m "$2")
if [[ -e "$results" ]]; then
  echo "Use a new results directory to preserve previous evidence." >&2
  exit 2
fi
mkdir -p "$results"
for scenario in normal multi wizard wizard-cancel wizard-input auto-config stats updater restart signal; do
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
  # Retain classified Qt teardown diagnostics at DEBUG level for verification.
  args=(--disable-updater --verbose)
  if [[ "$scenario" == updater ]]; then
    args=(--verbose)
  fi
  if [[ "$scenario" == multi || "$scenario" == wizard ]]; then
    args+=(--multi)
  fi
  env QT_QPA_PLATFORM=wayland XDG_CONFIG_HOME="$results/$profile/config" \
    XDG_CACHE_HOME="$results/$profile/cache" LD_PRELOAD="$probe" \
    PENGUIN_SHUTDOWN_MODE="$mode" PENGUIN_SHUTDOWN_REPORT="$results/$scenario/report.json" \
    timeout --kill-after=5s 40s "$executable" "${args[@]}" > "$results/$scenario/application.log" 2>&1
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
assert report['sentinelObserved'] == (scenario not in ('multi', 'wizard')), report
assert report['startupAutoConfigObserved'] == (scenario != 'restart'), report
assert not list((root / profile / 'config/PenguinStudio/.sentinel').glob('run_*'))
if scenario == 'wizard':
    assert report['wizardVisibleAtClose'], report
if scenario in ('wizard-cancel', 'wizard-input'):
    assert report['connectionWizardOpened'] and report['wizardInputFocused'], report
if scenario == 'wizard-input':
    assert report['wizardVisibleAtClose'] and report['wizardInputFocusedAtClose'], report
if scenario in ('wizard-cancel', 'auto-config'):
    assert report['wizardCancelRequested'] and report['wizardClosedBeforeShutdown'], report
    assert not report['wizardVisibleAtClose'], report
if scenario == 'auto-config':
    assert report['autoConfigOpened'], report
if scenario == 'updater':
    assert report['updaterOpened'] and report['updaterFocused'] and report['updaterClosed'], report
if scenario == 'stats':
    assert report['statsOpened'], report
marker = '==== Shutting down'
assert marker in log
shutdown_log = log.split(marker, 1)[1]
startup_log = log.split(marker, 1)[0]
errors = re.findall(r'^.*\berror:.*$', shutdown_log, re.MULTILINE | re.IGNORECASE)
assert not errors, errors
# Cancellation occurs before the main shutdown marker. Inspect the entire run so
# focus warnings cannot be hidden by moving a dialog's destruction earlier.
focus_message = r'[^\n]*QWaylandTextInput\w*::(?:zwp_text_input_v\d+_leave|disableSurface)\b[^\n]*'
null_surface = r'\bsurface (?:0x0+|\(nil\)) with focusing surface\b'
focus_warnings = re.findall(r'^warning:' + focus_message + '$', log, re.MULTILINE)
teardown_focus_warnings = [line for line in focus_warnings if re.search(null_surface, line)]
assert not teardown_focus_warnings, teardown_focus_warnings
shutdown_warnings = re.findall(r'^warning:.*$', shutdown_log, re.MULTILINE)
assert not shutdown_warnings, shutdown_warnings
focus_debug = re.findall(r'^debug:' + focus_message + '$', log, re.MULTILINE)
assert 'Number of memory leaks: 0' in shutdown_log
startup_warnings = re.findall(r'^warning:.*$', startup_log, re.MULTILINE)
startup_errors = re.findall(r'^error:.*$', startup_log, re.MULTILINE)
report.update(startup_warnings=startup_warnings, startup_errors=startup_errors,
              startup_clean=not startup_warnings and not startup_errors, platform='wayland', exit_code=0, shutdown_errors=errors,
              shutdown_warnings=shutdown_warnings,
              qt_focus_warnings=focus_warnings,
              qt_teardown_focus_warnings=teardown_focus_warnings,
              startup_qt_focus_warnings=re.findall(r'^warning:' + focus_message + '$', startup_log, re.MULTILINE),
              qt_teardown_focus_debug_messages=[line for line in focus_debug if re.search(null_surface, line)],
              sentinel_files_after_exit=0, obs_memory_leaks=0)
(case / 'verified.json').write_text(json.dumps(report, indent=2) + '\n')
assert not startup_errors, startup_errors
assert not startup_warnings, startup_warnings
print(f'STARTUP: {scenario}: no warnings or errors')
print(f'PASS: {scenario}: zero late unregisters, zero shutdown warnings/errors, zero Qt teardown focus warnings, zero OBS memory leaks')
PY
done
