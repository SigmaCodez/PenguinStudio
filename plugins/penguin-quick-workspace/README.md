# QML workspace preview

Optional local QML presentation experiment. OBS rendering and the native
Qt Widgets settings logic remain authoritative. Build with `PENGUIN_QUICK_WORKSPACE_PREVIEW=ON` on Linux.
The option is off by default and must not be enabled in public packages without
local design approval and normal publication checks.

The dock offers live scene selection, case-insensitive scene search,
keyboard-accessible panel navigation and primary-stream/recording status.
Sources, audio, inspector, destinations, stream details and chat remain accessible
through their native docks. **Edit** beside Scenes opens the complete native scene manager.
The first local7 start arranges navigation left, sources/mixer below the preview
and inspector right. **Workspace layout** can restore the previous saved dock
state or apply the new arrangement again. Later starts retain the user's layout.
Original vector icons and the shared WorkspaceButton component use GPL-2.0-or-later.

Existing controls, native plugin dialogs, the
OBS video preview and output actions retain their current implementations.
Close **Workspace preview** to hide navigation; reopen it through the Docks menu.
Navigation does not initiate streaming, recording or authentication.

Local8 also gives Settings a searchable QML sidebar while preserving the native
page selection, validation, Apply and Cancel behavior. Clicking **Penguin Studio**
opens a searchable Command center. Arrow keys and Enter select existing application
actions; their normal confirmations still apply. Hold the brand button to access
the complete native menus, including dynamically populated submenus. Disabled
actions and disabled parent menus remain protected. QML load errors retain native
settings; the original menu remains available.

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
