# QML workspace preview

Optional local QML presentation experiment. OBS rendering and the native
Qt Widgets settings logic remain authoritative. Build with `PENGUIN_QUICK_WORKSPACE_PREVIEW=ON` on Linux.
The option is off by default and must not be enabled in public packages without
local design approval and normal publication checks.

The Workspace dock puts scene selection first. **Manage** opens the native scene
manager; scene search never edits the collection. Sources and Audio mixer remain
under the preview. **Edit selected source**, **Streaming channels**, **Title &
category**, and **Live chat** select a single right-side context panel.

The top bar keeps Go live, Record, Settings and a labeled Advanced menu. Redundant
channel/details/inspector shortcuts are removed while Workspace is visible.
Replay clip saving and Studio mode remain in Advanced; output actions still route
to the original validated controls. The primary-stream action remains separate
from Go live's multi-destination behavior. Native streaming confirmations remain.

Local9 migrates the layout once, saving the previous state as BeforeSimpleLayout.
**Workspace layout → Restore previous layout** restores that saved arrangement;
**Arrange workspace** reapplies the new one. Subsequent starts keep custom layout
changes. Closing Workspace restores the native toolbar shortcuts; reopening it
reapplies the compact controls. The complete native menu remains available by
holding the Penguin Studio brand button.

Settings uses a searchable QML sidebar with native page selection, validation,
Apply and Cancel. Clicking the brand opens the searchable Command center. Arrow
keys and Enter invoke existing application actions; Escape closes the dialog.
Disabled actions and their parent menus remain protected. QML load errors retain
native settings. Original vector icons and the WorkspaceButton component use
GPL-2.0-or-later.

Runtime dependencies: Qt Quick Widgets and the QtQuick, QtQuick.Layouts,
QtQml, QtQml.Models and QtQml.WorkerScript QML modules matching the application's
Qt version. A load error leaves the existing workspace available.

The plugin installs through OBS 33's system plugin directory, not its compiled
core-module list. Its QML is embedded in the binary; it loads no remote QML.
Scene names are plain text, and navigation resolves current scene UUIDs.

Qt documents an additional offscreen rendering pass for QQuickWidget:
https://doc.qt.io/qt-6/qquickwidget.html#performance-considerations
This prototype uses small QML surfaces without continuous animations. Do not
infer a performance improvement from the choice of UI technology.

On Wayland, the module selects Qt Quick's software backend before creating its
first view. The Qt 6.10 OpenGL path recursed while creating the Wayland framebuffer
after rearranging docks during testing. Software rendering avoids that extra UI
OpenGL context; OBS video rendering, encoders and camera inference keep their GPU
paths. The Qt Quick backend selection is process-wide; third-party QML plugins
need separate compatibility testing. This does not select software video encoding.

Validation uses an isolated English profile and synthetic scenes. Captures of
this experiment belong with local test evidence, not current public release
previews. Test keyboard activation, scene updates, dock visibility, font scaling,
recording and clean Wayland shutdown before installing a local candidate.
