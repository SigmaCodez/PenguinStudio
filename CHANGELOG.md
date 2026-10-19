# Changelog

## 0.2.0 — 2026-10-19

- Rebuilt the native workspace around scenes/layers, a central canvas, vertical audio channels and a right-hand source inspector.
- Added a production toolbar for destinations, multi-output start/stop, recording and replay clips. Existing menus, keyboard shortcuts and advanced docks remain accessible.
- Added a Studio Mode control above the canvas.
- Added direct source position, size, rotation, crop, visibility and lock editing, with undo/redo and access to properties, filters and advanced transforms.
- Fixed grouped-source transform undo after libobs resizes a parent group.
- Introduced a complete charcoal/indigo visual system, original line icons, new panel headers, channel cards, form controls and matching preview-selection colors.
- Added 50 workspace/inspector strings in English, German, French, Spanish, Brazilian Portuguese, Italian, Japanese and Simplified Chinese.
- Added versioned dock-layout migration with a backup of the previous layout. Later launches preserve user customization.
- Fixed localized Qt standard buttons when the desktop theme includes keyboard mnemonics in their labels.
- Corrected first-run cancellation so it does not reopen the destinations dock, and retained the Penguin icon throughout the application header.

See [verification](docs/VERIFICATION.md) for executed checks and remaining release gates.

## 0.1.0 — 2026-10-19

- Established the standalone Penguin Studio repository on OBS Studio 32.2.2 with original branding and isolated application settings.
- Added native multi-destination streaming, portrait crop/fit outputs, first-run destination setup and Twitch/YouTube metadata controllers.
- Added Flatpak, Snap, DEB and RPM recipes and CI builds, plus configuration, OAuth, output-lifecycle and wizard regression tests.
