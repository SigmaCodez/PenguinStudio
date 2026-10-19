# Penguin Studio workspace

The 0.2 workspace changes the application's native Qt layout and controls. It draws inspiration from Meld's scene/layer, canvas, mixer and inspector arrangement while keeping Penguin's own branding and the OBS production engine.

## Compose and inspect

Scenes and Layers sit on the left. The **+** buttons run the existing scene and source creation actions; each panel's **More** menu retains remove, reorder and other inherited actions. Source dragging, visibility, locking and scene renaming remain available.

The center shows the real OBS canvas, the selected scene name and the configured base resolution. **Studio Mode** switches between the normal editor and preview/program operation. Existing source and media toolbars remain below the preview.

Select one layer to edit it in the right-hand **Inspector**. Position, size, rotation, crop, visibility and locking update the source itself. Transform changes participate in OBS undo/redo. Fit, center, flips and reset use the existing canvas operations. Source properties, effects/filters and the advanced transform dialog remain directly accessible. Multiple selection displays a prompt to select one layer for numeric inspection.

The bottom mixer uses actual OBS volume controls, meters and monitoring settings in a vertical channel layout. The mixer menu can switch back to horizontal controls. Docks can still be moved and resized.

## Production controls

- **Penguin Studio** opens the full application menu: File, Edit, View, Docks, Profile, Scene Collection, Tools and Help. On macOS the native system menu is retained as well.
- **Destinations** opens the Penguin multi-output panel.
- **Go live** runs the destination panel's validated Start All action. With no destinations configured, it opens the setup assistant. While destinations are running, **End stream** runs Stop All.
- The live button's menu exposes **Stream details** and OBS's separate primary stream toggle. The primary stream's setup and credentials are separate from Penguin destinations.
- **Record** uses the original recording controller. Its menu offers pause/resume when the recording mode supports it.
- **Clip** saves the active replay buffer. Enable the replay buffer in Settings → Output and start it through **More** before saving clips.
- **More** contains replay-buffer and virtual-camera controls, Settings, the original Controls and Scene Transitions docks, and Reset workspace.

The top FPS readout comes from the running video engine. No simulated audio levels, output states or canvas previews are shown.

## Existing configurations

On the first 0.2 launch, Penguin installs the new dock layout and vertical mixer defaults. It stores the prior dock state as `BasicWindow/PenguinPreviousDockState` and records `PenguinWorkspaceVersion=2` in the isolated Penguin configuration. Scene collections, profiles and sources are preserved. Subsequent launches restore the user's saved dock arrangement instead of resetting it again.

**More → Reset workspace** reapplies the production layout. The complete Docks menu remains available from the Penguin Studio application button.

## Scope

Portrait destinations currently crop or fit the program scene. The workspace does not imply an independently editable second portrait scene. Twitch/YouTube authorization, live-provider verification, capture devices and cross-distribution release checks retain the limits documented in [VERIFICATION.md](VERIFICATION.md).
