# Qt Quick workspace development

The locally reviewed Qt Quick presentation layer is included in the 0.7.2 Linux
packages. It extends the existing Qt Widgets application; it is not a replacement
for the renderer or native settings controllers.

## Scope

- Scene selection uses the current OBS scene UUIDs.
- Inspector, Destinations, Stream details and Live chat open existing docks.
- Status reflects the primary stream and recording state.
- Text follows the configured interface font scale. The navigation scrolls when
  its content exceeds the available height.
- Keyboard focus has a visible outline; Return, Enter and Space activate items.
- Closing **Workspace preview** hides the navigation dock. The normal Docks
  menu can reopen it. Visibility is retained across restart.

The OBS renderer, preview, encoders, authentication, camera processing and native
plugin dialogs keep their existing implementations. Navigation does not initiate
streaming, recording or account authorization. The prototype currently uses English
labels. Production localization is a prerequisite for a wider UI migration.

## Build and packaging

The optional `PENGUIN_QUICK_WORKSPACE_PREVIEW` CMake option is off by default.
Official Linux package definitions enable it after local design review. Qt Widgets, QuickWidgets and Qml
development packages are required. `PENGUIN_BUILD_TESTS` also builds the navigation
smoke helper; it is not installed into the production package.

Install the module in the OBS 33 system plugin directory:
`lib/<architecture>/obs-modules/plugins`, using the configured CMake library path.
It must not be added to the compiled core-module directory without updating the
core allowlist. QML resources are embedded; no remote QML is loaded.

The Ubuntu 26.04 local6 package adds these runtime dependencies:
`libqt6quickwidgets6`, `qml6-module-qtquick`, `qml6-module-qtquick-layouts`,
`qml6-module-qtqml`, `qml6-module-qtqml-models`, and
`qml6-module-qtqml-workerscript`. The package payload comparison against local5
found exactly two additions: the module and its README. Existing payload files
were unchanged and none were removed.

Candidate: `0.7.1-1+ubuntu26.04.local6`, amd64.
SHA-256: `c640e25a9f194c012b55e8d1c9f21eb6b76c8f0835f3de916402ea6007af8065`.

## Validation and limits

Validation uses isolated English profiles and synthetic media on native Wayland.
The navigation smoke helper exercises scene selection, panel routing, keyboard
activation, dock hiding/restoration and QML rendering. Keyboard dispatch explicitly
focuses the embedded Qt Quick content and item before sending the key: compositor
activation of an automated test window is not guaranteed under Wayland. This is
an event-handling test, not proof of every physical keyboard/compositor interaction.
Automated runs were intermittent when the window did not receive active focus,
including a post-install run. Keep these failures in the evidence; do not count
them as successful keyboard checks. On 2026-10-26, the user confirmed physical
keyboard operation on the installed local6 build under Wayland: after clicking
Inspector, Tab navigation showed the focus indicator and Enter opened Live chat.
This closes the manual keyboard acceptance check for that navigation path. It is
user-observed evidence, separate from the intermittent automated focus results.

Both normal and enlarged text (FontScale 10 and 14) were inspected using real QML
framebuffer captures. The existing workspace suite passed all 242 checks, including
its synthetic recording. Recording validation checks decoding, OBS loss statistics
and clean shutdown. The final GPU test produced a decodable 30.1-second file with
900 video frames, no reported loss events, no warning/error log entries and zero
OBS-reported memory leaks at exit. This task does not claim a new live Facecam movement test or
new Twitch/YouTube authorization tests.

Local test evidence is under `/var/tmp/penguin-obs33/`, including
`qml-local6-workspace`, `qml-local6-focus-details`, `qml-local6-large-keys`,
`qml-local6-saved`, and `qml-local6-gpu-final`. These are local experimental captures,
not the current public release previews. Whole-window QWidget grabs do not reliably
capture the native OBS video surface; they must not be used as approved product
screenshots.

Qt Quick is not inherently faster than Qt Widgets. QQuickWidget adds an offscreen
render pass and disables Qt Quick's threaded render loop. The prototype has one
small surface, no continuous animation and no polling timer in production. A full
frontend migration needs measured CPU/GPU/frame-time comparisons and compatibility
work for existing plugin dialogs, docking, accessibility and translations.

Reference: https://doc.qt.io/qt-6/qquickwidget.html#performance-considerations

No release, tag, public version change or GitHub publication is authorized by this
local experiment. Keep the system-wide DEB installation and normal Ubuntu
administrator confirmation for local review.

## Local8 follow-up

See [the local8 review](LOCAL8-DESIGN-REVIEW.md) for the searchable QML Settings
sidebar, Command center, native-action protections and camera scheduling changes.
The native settings page model remains authoritative. Closing the navigation dock
only hides that dock; it does not disable the Settings or Command center surfaces.
No public package flag or product version has changed.

## Local9 simplified presentation

See [the local9 review](LOCAL9-CPU-AND-WORKSPACE-REVIEW.md) for the scene-first
Workspace, removal of redundant toolbar shortcuts, single context panel and
Advanced tools. Native actions and the complete menu remain available. This is
still a local-review build; public previews and product version are unchanged.
