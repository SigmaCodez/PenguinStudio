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
The runner checks normal exit, `--multi`, an open setup wizard, a separately opened
and closed statistics window, restart of the same normal profile, and graceful
SIGTERM handling after startup. Normal
runs must create a crash sentinel and remove it on clean exit; multi-instance
runs must never create one. Each process must exit successfully with no errors
in the shutdown section, no late frontend unregisters, and zero OBS memory leaks.
Optional hardware-plugin diagnostics during startup are outside this shutdown
check. Qt warnings are retained in each report rather than filtered out.

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
