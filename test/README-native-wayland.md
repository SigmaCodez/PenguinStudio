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
