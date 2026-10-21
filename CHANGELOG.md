# Changelog

## 0.4.3 — 2026-10-21

- Add an x86_64 AppImage built from the Ubuntu 24.04 package, including Qt Wayland plugins and publisher-configured browser sign-in.
- Generate a checksum-pinned `penguin-studio-bin` Arch/AUR recipe and validate it with makepkg before release. AUR submission requires a maintainer account and is not yet enabled.
- Require both new artifacts in the complete Linux release set.
- Fix queued thumbnail cleanup on Qt 6.4 by passing an owned UUID through a context-bound callback.

## 0.4.2 — 2026-10-21

- Select available NVIDIA NVENC or Linux VA-API H.264 encoders automatically for new simple-output profiles, setup and multistream destinations. Preserve explicit user choices.
- Add VA-API streaming/recording settings with CBR/CQP capability checks and software fallback.
- Explicitly enable NVENC in every Linux package recipe and recommend Mesa VA drivers for native Ubuntu packages.
- Validate AMD hardware recording, landscape/portrait loopback output and clean installed-app startup/shutdown under Wayland. NVIDIA runtime testing remains pending.

## 0.4.1 — 2026-10-21

- Include publisher-configured YouTube desktop authorization in all official Linux packages, allowing browser sign-in without user-entered client credentials.
- Keep custom client configuration under advanced settings and preserve existing saved-account identities.

## 0.4.0 — 2026-10-20

- Add searchable Twitch game/category suggestions and filterable YouTube video categories.
- Restore Twitch and YouTube sign-ins from the system keyring after restart; disconnect removes the saved credentials without a plaintext fallback.
- Offer optional provider sign-in in the first-start and destination assistants.
- Fix Flatpak QtKeychain test dependencies and imported build configuration.

## 0.3.1 — 2026-10-20

- Publish a maintenance build to validate the complete in-app upgrade path from 0.3.0 on Ubuntu 26.04.1.
- Retain the 0.3.0 application behavior; verification results are recorded separately in the release validation documentation.

## 0.3.0 — 2026-10-20

- Add automatic Penguin release notifications and verified in-app Linux package updates.
- Add a native Wayland-compatible Twitch chat dock and bundled public-app login, with stream-details loading.
- Update inherited dialog and plugin product names across translations to Penguin Studio.
- Route Help support and About contribution links to the Penguin Studio repository; brand OAuth completion pages locally.
- Preserve OBS import formats, engine attribution, authors, licenses and third-party service disclosures.

## 0.2.1 — 2026-10-20

- Wait for confirmed native main-window focus before scheduling the first-run hardware wizard on Wayland, avoiding the Qt 6.10.2 text-input surface race at startup.
- Keep the connection assistant behind pending hardware setup and avoid opening it over an inactive main window.
- Add activation, one-shot and shutdown lifetime regression checks; native verification now requires warning-free startup and confirms the hardware wizard still appears.
- Publish versioned Ubuntu DEB, Fedora RPM, Flatpak and Snap downloads with checksums and source provenance.

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
