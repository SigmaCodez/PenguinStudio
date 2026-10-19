# Penguin Studio workspace

The workspace follows a production-oriented arrangement inspired by the
[official Meld Studio interface](https://meldstudio.co/): scenes and layers on
the left, a large canvas in the center, audio channels below, and a source
inspector on the right. Penguin Studio uses its own logo, original icon
artwork, and native Qt implementation. It is not affiliated with Meld Studio.

## Visual system

- **Neutral charcoal surfaces** distinguish the application frame (`#14161B`),
  panels (`#1B1D23`), and preview surround (`#0B0C10`). Subtle separators and
  compact panel headings keep the canvas prominent.
- **Indigo selection** identifies the active scene, selected layer, focus, and
  editing states. The primary indigo is `#384B8B`; focus accents use `#899BFF`.
- **Mint production action** (`#64DBB0`) highlights Go live. Active streaming
  uses a darker green surface with a light label. Recording has a separate
  red active state. Text and checked states accompany color changes.
- **Typography and spacing** follow the inherited OBS font scale. Section
  headings use smaller bold text, source names provide a stronger hierarchy,
  and controls share consistent borders, corner radii, focus states, and
  padding. Mixer channels use compact cards with the native faders and meters.

`frontend/data/themes/Penguin.ovt` defines the theme. The reusable icon API in
`frontend/utility/PenguinIcons.hpp` provides 17 original line symbols through
`PenguinStudio::StudioIcon(Icon, QColor, int)`: Camera, Layers, Scene, Plus,
Settings, Live, Record, Clip, Inspect, More, Chevron, Monitor, Portrait, Audio,
Folder, Reset, and Link. Icons render at both standard and double density;
no external desktop icon theme is required.

## Native interaction and resizing

The toolbar, dock headers, scene and source lists, canvas, mixer, and inspector
are real Qt widgets connected to the application and OBS backend. The
application menu retains the native menu tree and keyboard shortcuts.
Destinations and stream details remain accessible from the production controls.
The inspector edits the selected source and retains the existing properties,
filters, transform, and undo operations.

The toolbar adapts to window width in Qt logical pixels:

| Window width | Adaptation |
| --- | --- |
| Below 1280 | Hide the secondary Workspace label. |
| Below 1180 | Hide the toolbar FPS badge and show Destinations as an icon with a tooltip. |

Docks can be resized, and the inspector scrolls vertically. **Reset workspace**
restores the scene/layer/canvas/mixer/inspector arrangement. These adaptations
do not establish a guarantee for every display scale, window size, or translated
label; actual GUI validation is recorded separately under `docs/validation/`.
