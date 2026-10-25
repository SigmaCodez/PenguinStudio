# Penguin Studio workspace

Penguin Studio organizes the native Qt application around a large canvas, searchable layers and scenes, audio channel strips, and an integrated production sidebar.

![Penguin Studio 0.7.1 workspace in English](images/current/workspace-en.png)

These English previews were captured from the validated 0.7.1 Ubuntu package using an isolated demo profile and synthetic media.

## Compose and inspect

Layers sit above Scenes on the left. Numbered scene cards make scene order clear. Each list has a case-insensitive search field; filtering only changes the displayed rows, never source visibility or scene contents. Clear the field to restore the complete list. Drag reordering is disabled while a filter is active, to prevent ambiguous ordering. The **+** buttons run the existing scene and source creation actions; each panel's **More** menu retains remove, reorder and other inherited actions. Source dragging, visibility, locking and scene renaming remain available.

The center shows the production canvas, the selected scene name and the configured base resolution. **Studio Mode** switches between the normal editor and preview/program operation. Media controls remain below the preview. Source properties and filters are provided by the Inspector; the legacy source toolbar can still be enabled from the View menu.

The right sidebar uses **Inspector**, **Outputs**, and **Details** tabs, so production tools remain beside the canvas instead of opening as floating windows. Select one layer to edit it in the **Inspector**. Position, size, rotation, crop, visibility and locking update the source itself. Transform changes participate in the application's undo/redo. Properties and effects/filters are available at the top of the Inspector. A compact arrange strip provides fit, center and flip actions through the existing canvas operations. Crop settings expand on demand; reset and the advanced transform dialog remain accessible. Multiple selection displays a prompt to select one layer for numeric inspection.

The bottom mixer presents native volume controls, meters and monitoring settings as wider vertical channel strips, ordered from source identity through meter/fader and level to mute/monitor controls. The mixer menu can switch back to horizontal controls. Docks can still be moved and resized.

## Production controls

- **Penguin Studio** opens the full application menu: File, Edit, View, Docks, Profile, Scene Collection, Tools and Help. On macOS the native system menu is retained as well.
- **Destinations** selects the integrated multi-output sidebar. Destinations appear as stacked cards with name, layout and real output status; editing, removal, individual start/stop and start/stop all remain available.
- **Stream details** selects the scrollable Twitch/YouTube account and stream-management forms. **Settings** has its own toolbar shortcut.
- **Go live** runs the destination panel's validated Start All action. With no destinations configured, it opens the setup assistant. While destinations are running, **End stream** runs Stop All.
- The live button's menu exposes **Stream details** and the separate primary stream toggle inherited from OBS. The primary stream's setup and credentials are separate from Penguin destinations.
- **Record** uses the original recording controller. Its menu offers pause/resume when the recording mode supports it.
- **Clip** saves the active replay buffer. Enable the replay buffer in Settings → Output and start it through **More** before saving clips.
- **More** contains replay-buffer and virtual-camera controls, Settings, the original Controls and Scene Transitions docks, and Reset workspace.

The top FPS readout comes from the running video engine. No simulated audio levels, output states or canvas previews are shown.

Settings, source properties, filters, and the setup assistant use the same control theme. Settings navigation measures translated labels, and its initial window width follows the selected text size within the available screen. Long forms retain scrolling, with minimum input heights that keep their text readable.

## Existing configurations

On the first launch with layout version 3, Penguin applies the new layout and vertical mixer defaults. It saves the current dock arrangement in `BasicWindow/PenguinPreV3DockState`, retains any earlier `PenguinPreviousDockState` backup, and records `PenguinWorkspaceVersion=3` in the separate Penguin configuration. Scene collections, profiles and sources are preserved. Later launches restore the user's saved arrangement instead of resetting it again.

**More → Reset workspace** reapplies the production layout. The complete Docks menu remains available from the Penguin Studio application button.

## Scope

Portrait destinations currently crop or fit the program scene. The workspace does not imply an independently editable second portrait scene. Twitch/YouTube authorization, live-provider verification, capture devices and cross-distribution release checks retain the limits documented in [VERIFICATION.md](VERIFICATION.md).
