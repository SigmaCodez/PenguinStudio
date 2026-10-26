# QML workspace preview

Optional local navigation experiment, not a replacement for OBS rendering or
Qt Widgets dialogs. Build with `PENGUIN_QUICK_WORKSPACE_PREVIEW=ON` on Linux.
The option is off by default and must not be enabled in public packages without
local design approval and normal publication checks.

The dock offers live scene selection, keyboard-accessible panel navigation and
primary-stream/recording status. Existing controls, native plugin dialogs, the
OBS video preview and output actions retain their current implementations.
Close **Workspace preview** to return to the classic workspace; reopen it through
the normal Docks menu. It does not initiate streaming, recording or authentication.

Runtime dependencies: Qt Quick Widgets and the QtQuick, QtQuick.Layouts,
QtQml, QtQml.Models and QtQml.WorkerScript QML modules matching the application's
Qt version. A load error leaves the existing workspace available.

The plugin installs through OBS 33's system plugin directory, not its compiled
core-module list. Its QML is embedded in the binary; it loads no remote QML.
Scene names are plain text, and navigation resolves current scene UUIDs.

Qt documents an additional offscreen rendering pass for QQuickWidget:
https://doc.qt.io/qt-6/qquickwidget.html#performance-considerations
This prototype uses one small QML surface and no continuous animations. Do not
infer a performance improvement from the choice of UI technology.

Validation uses an isolated English profile and synthetic scenes. Captures of
this experiment belong with local test evidence, not current public release
previews. Test keyboard activation, scene updates, dock visibility, font scaling,
recording and clean Wayland shutdown before installing a local candidate.
