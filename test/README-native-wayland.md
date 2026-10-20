# Native Ubuntu Wayland GUI regression checks

Use a disposable configuration directory. Never preload these helpers into a
personal profile: the workspace helper changes scenes, sources and output
settings. The wizard helper opens and closes the real auto-configuration wizard.
Neither helper is shipped in the application.

On a developer machine, install the development packages matching the running
application's Qt version (`g++`, `pkg-config`, `qt6-base-dev`, `libsimde-dev`,
`libx11-dev`, `libxcomposite-dev`). The Penguin DEB provides the libobs headers and
libraries. Do not install a conflicting distribution `libobs-dev` package.

For the compact wizard regression, compile the helper:

```sh
c++ -std=c++17 -shared -fPIC -Wall -Wextra -Werror \
  test/penguin-wizard-layout.cpp -o /tmp/penguin-wizard-layout.so \
  $(pkg-config --cflags --libs Qt6Widgets)
```

Create a new, isolated directory and launch the installed application from the
logged-in Wayland session:

```sh
qa_run=$(mktemp -d /tmp/penguin-wizard.XXXXXX)
mkdir -p "$qa_run/config/PenguinStudio" "$qa_run/cache"
printf '[General]\nLanguage=en-US\n' > "$qa_run/config/PenguinStudio/user.ini"
env QT_QPA_PLATFORM=wayland \
  XDG_CONFIG_HOME="$qa_run/config" XDG_CACHE_HOME="$qa_run/cache" \
  PENGUIN_WIZARD_RESULTS="$qa_run" LD_PRELOAD=/tmp/penguin-wizard-layout.so \
  timeout --kill-after=5s 30s penguin-studio --multi --disable-updater \
  > "$qa_run/application.log" 2>&1
python3 - "$qa_run" <<'PY'
import json
from pathlib import Path
import sys
root = Path(sys.argv[1])
report = json.loads((root / 'wizard.json').read_text())
assert report['platform'] == 'wayland'
assert report['captured']
assert {field['name'] for field in report['fields']} == {'canvasRes', 'fps'}
assert all(field['passed'] for field in report['fields']), report
assert 'Number of memory leaks: 0' in (root / 'application.log').read_text()
print('PASS: compact wizard text fits, fields do not overlap, clean shutdown')
PY
```

The probe requests a 514 × 304 logical-pixel window, matching the reported
failure. The application must enforce its own minimum size. The helper never
repairs the window: its JSON verifies both text height and field intersections.
Run again with `Language=de-DE` and, separately, an `[Appearance]` section with
`FontScale=14` to cover longer translations and larger text.

`wizard.png` is a Qt widget render from the running native Wayland application,
not a desktop screenshot. This method is suitable for form controls; it does not
validate independently rendered OpenGL/CEF child surfaces. Validate video output
with a real recording and decoded frames instead.

The broader workspace helper is `penguin-workspace-smoke.cpp`; the Flatpak runner
and fixture setup are in `scripts/packaging/verify-workspace.sh`. It exercises
scenes, inspector transforms, undo/redo, studio mode, connection wizard routing,
and local H.264/AAC recording. Native runs must compile against the installed
Qt/libobs libraries and use the same isolated fixture setup.

Local simultaneous landscape/portrait output checks are documented in
`plugins/penguin-outputs/tests/README.md`. These loopback checks and mock-provider
tests do not establish real Twitch/YouTube authorization or broadcast acceptance.
Wayland portal capture additionally requires the user's screen/window selection.

For an offline browser-source recording check, set `PENGUIN_SMOKE_BROWSER` to the
absolute path of `test/fixtures/penguin-browser.html` in the native workspace run.
The helper adds the HTML source behind the image fixture. Decode a recorded frame
and confirm the green bar and white fixture text appear; a source-created check
alone does not prove CEF rendered its content. The fixture makes no web requests.

## Shutdown lifetime regression

Build the observer against the installed native Qt/OBS development headers:

```sh
c++ -std=c++17 -shared -fPIC -Wall -Wextra -Werror -I/usr/include/obs \
  test/penguin-shutdown-probe.cpp -o /tmp/penguin-shutdown-probe.so \
  $(pkg-config --cflags --libs Qt6Widgets) -ldl
bash scripts/packaging/verify-shutdown.sh /tmp/penguin-shutdown-probe.so /tmp/penguin-shutdown-new
```

The observer calls the real frontend API unchanged. It counts callback removal
after frontend destruction and writes the report at process teardown, after the
WebSocket module has unloaded. It never hides or changes application log messages.
The runner checks nine scenarios: normal exit, `--multi`, an open initial setup
wizard, connection-wizard cancellation, shutdown with a connection text field
verified to have native keyboard focus, auto-configuration cancellation, a
separately opened and closed statistics window, restart of the same normal
profile, and graceful SIGTERM handling after startup. Normal runs must create a
crash sentinel and remove it on clean exit; multi-instance runs must never create
one. Each process must exit successfully with no warnings or errors in the
shutdown section, no late frontend unregisters, and zero OBS memory leaks.
The runner rejects the known null-surface Qt focus warnings anywhere in the log,
including before the shutdown marker. Verbose logging preserves the original
debug diagnostics from the narrowly scoped [Qt compatibility correction](../docs/QT_WAYLAND_DIAGNOSTICS.md).
Startup hardware diagnostics and non-null Qt focus warnings are retained in the
reports separately; they are not reclassified by this correction.

The WebSocket submodule remains pinned to its upstream revision. Its maintained
fix lives in `patches/obs-websocket-shutdown.patch` and is applied by CMake before
building the plugin. Reconfiguration is idempotent; incompatible upstream changes
fail configuration and require patch review. This intentionally modifies the
submodule working tree during configuration, without changing its pinned commit.
`python3 test/penguin-websocket-patch.py` checks application, repeated application,
and rejection of an incompatible source copy without editing the real submodule.

## Modern workspace layout regression

The workspace probe exercises the actual dock layout and the source/scene models.
Use a fresh profile for each of the following cases, with the application running
under native Wayland:

| Case | `user.ini` settings | Window sizes exercised |
| --- | --- | --- |
| English | `[General]` / `Language=en-US` | 1440 × 900 and 1280 × 800 |
| German | `[General]` / `Language=de-DE` | 1440 × 900 and 1280 × 800 |
| Larger text | English plus `[Appearance]` / `FontScale=14` | 1440 × 900 and 1280 × 800 |
| German with larger text | German plus `[Appearance]` / `FontScale=14` | 1440 × 900 and 1280 × 800 |

Compile `penguin-workspace-smoke.cpp` against the same Qt and libobs versions as
the application. Set `PENGUIN_SMOKE_EXPECT_REDESIGN=1`, `PENGUIN_SMOKE_DIR` to the
fixture/capture directory, and `PENGUIN_SMOKE_IMAGE` to the absolute path of
`assets/branding/penguin-studio.png`. Reuse the WAV and isolated recording-profile
fixture setup in `scripts/packaging/verify-workspace.sh`. Native runs preload the
helper directly into `penguin-studio`; Flatpak runs must compile and load the helper
inside the SDK to avoid crossing Qt ABI versions.

The checks require Layers above Scenes, the mixer below the preview, the inspector
on the right, readable search fields, an unclipped toolbar, and useful preview space
at both sizes. The two real search fields are exercised with matching text in a
different case, a query with no results, and clearing the query. Filtering must retain
all source/scene items and their selection, and must not change source visibility
in the actual OBS scene. Destinations and stream details must be reachable through
integrated right-sidebar tabs, with labels that fit the available tab width. The
scene delegate must still open its real rename editor, cancel with Escape, commit
to the OBS source, and restore the original name. The inspector's Crop section must
expand through its visible control before editing. The existing transform, undo/redo, scene switching,
studio mode, setup wizard and local video/audio recording checks remain active.

On Wayland, `workspace-1440-widgets.png` and `workspace-1280-widgets.png` are Qt
widget renders from the running application. Inspect these for hierarchy, spacing,
contrast and text clipping. They do not capture independently rendered preview or
browser surfaces and must not be presented as compositor screenshots. Inspect a
decoded recording frame for real video output. The `xcb` path retains native X11
window captures, including the composited preview.

Every run must have only passing entries in `workspace-checks.json`, a recording
with video and audio streams, and zero OBS leaks on shutdown. Also run the compact
wizard probe in all four language/font cases and the separate shutdown regression
against the final installed package. Retain warnings in the evidence even when
they are unrelated to a tested layout assertion.

## Settings and source dialogs

`penguin-dialog-smoke.cpp` opens the real Source properties and Effects and filters
buttons in the inspector, then the top-bar Settings button. It captures all nine
settings pages and scrolls long pages to the bottom. It uses a newly created image
source with a color-correction filter in a disposable profile and closes dialogs
without saving settings. The helper is test-only and must never be preloaded into
a personal configuration.

Compile against the installed application's matching Qt and OBS headers/libraries:

```sh
c++ -std=c++17 -shared -fPIC -Wall -Wextra -Werror -I/usr/include/obs \
  test/penguin-dialog-smoke.cpp -o /tmp/penguin-dialog-smoke.so \
  $(pkg-config --cflags --libs Qt6Widgets) -lobs -lobs-frontend-api
qa_repo=$(pwd)
qa_dialogs=$(mktemp -d /tmp/penguin-dialogs.XXXXXX)
mkdir -p "$qa_dialogs/config/PenguinStudio" "$qa_dialogs/cache" "$qa_dialogs/captures"
printf '[General]\nLanguage=de-DE\n\n[Appearance]\nFontScale=14\n' \
  > "$qa_dialogs/config/PenguinStudio/user.ini"
env QT_QPA_PLATFORM=wayland XDG_CONFIG_HOME="$qa_dialogs/config" \
  XDG_CACHE_HOME="$qa_dialogs/cache" LD_PRELOAD=/tmp/penguin-dialog-smoke.so \
  PENGUIN_DIALOG_DIR="$qa_dialogs/captures" \
  PENGUIN_DIALOG_IMAGE="$qa_repo/assets/branding/penguin-studio.png" \
  timeout --kill-after=5s 70s penguin-studio --multi --disable-updater \
  > "$qa_dialogs/application.log" 2>&1
```

Repeat with a fresh English profile at the default font size. The 14-point case is
a deliberate stress test above the current Appearance slider's 12-point maximum.
The JSON report records input content heights versus font heights, full visible
widget bounds and minimum sizes, actual screen geometry, horizontal scroll ranges
and settings navigation label/viewport widths. Require every dialog to be found
and captured, every input height check to pass, and every `navigationFits` value to be true. On the validation desktop,
require every settings page's `horizontalOverflow` array to be empty and inspect
the PNGs for form clipping. Vertical scrolling remains appropriate for long pages;
navigation labels must always fit without horizontal scrolling. Require all nine settings
page names, source properties and source filters in the report, and a clean OBS
shutdown. The Advanced page's `streamDelayPreserve.passed` must also be true: the
complete multiline caption fits, its accessible name retains the original text,
and enabling, toggling and restoring the checkbox still work. Inspect the separate
`stream-delay-checkbox.png` capture for its full caption. These images are Qt widget
renders, so independently rendered video previews must be verified with the workspace recording checks.

As of 0.2.1, the native runner also rejects **all startup warnings and errors**
and confirms the hardware wizard was visible on each fresh-profile run. Set
`PENGUIN_EXECUTABLE` to test an extracted package's launcher; use its matching
libraries and resources. The observer must be rebuilt from the same checkout.
`penguin-startup-dialog.cpp` separately checks activation, one-shot dispatch and
parent destruction with Qt's offscreen platform.
