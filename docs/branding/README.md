# Penguin Studio branding

Penguin Studio is SigmaCodez's independent application based on OBS Studio. The original OBS
copyright notices, GPL license, authors, plugin interfaces, and importer names
remain intact. Penguin Studio is not affiliated with OBS Project or Meld Studio.

## Assets

- `assets/branding/penguin-studio.png`: original generated 1254 × 1254 RGBA logo.
- `assets/branding/penguin-studio.svg`: desktop scaling wrapper containing the
  unmodified PNG. This is not editable vector artwork.
- `frontend/forms/images/penguin-studio.png`: identical Qt application resource.
- `docs/branding/logo-prompt.txt`: exact generation prompt.

The logo was generated on 2026-10-19 with the built-in OpenAI image generation
tool, in one generation with transparency requested. No reference image was
used. The result was visually inspected and its transparent alpha channel was
checked. The image combines a dark penguin silhouette, white face and belly,
mint play mark, and amber beak and feet. It contains no generated lettering.
The generated result includes soft tonal variation despite the flat-color
prompt; the original image has been preserved without retouching.

The generated artwork is contributed with this project under GPL-2.0-or-later,
to the extent copyright applies. Original OBS assets retain their existing
notices and are not claimed as Penguin Studio artwork.

## Native appearance

`frontend/data/themes/Penguin.ovt` extends OBS's Yami theme rather than replacing
working controls. Charcoal surfaces, mint action buttons, generous dock titles,
and six-pixel control corners provide a distinct studio identity. The inherited
layout places scenes and sources on the left, the preview in the center, and
audio, transitions, and output controls below. Users retain movable docks and
the original appearance settings, including accessibility options.

This is an original visual interpretation of a modern streaming workspace,
not a reproduction of Meld Studio's assets or proprietary functionality.

## Identity and compatibility

- Display name: **Penguin Studio**. The product version comes from
  `packaging/version.txt`; the title and About dialog also show the OBS engine
  version without changing ABI or configuration version checks.
- Linux desktop ID: `io.github.SigmaCodez.PenguinStudio`.
- Launcher: `penguin-studio`; the internal executable remains `obs`.
- Default theme: `io.github.SigmaCodez.PenguinStudio.Dark`.
- Configuration directory: `PenguinStudio` under the normal configuration root
  (`~/.config/PenguinStudio` on a conventional Linux installation).
- Linux instance lock: `/tmp/penguin-studio.lock.<uid>`.

The configuration adapter redirects upstream frontend calls that still request
`obs-studio/...` into the Penguin Studio directory. Direct profile, scene,
theme, plugin-manager, and crash-sentinel paths are also isolated. The OBS
importer intentionally keeps its upstream path so that importing existing OBS
collections is still possible. Installation data paths and plugin ABI names
remain compatible with the inherited build layout.

New installations open in English. Every inherited frontend and plugin locale
is retained and remains available through Settings. New untranslated strings
fall back to English; Penguin Studio does not claim new human-reviewed translations.

## Validation

Run `python3 scripts/check-branding.py` for metadata, resource, locale, and
configuration invariants. If a C++17 compiler is available, compile and run
`test/penguin-config-paths.cpp` to exercise configuration path isolation.
Desktop and AppStream validators can additionally check the Linux metadata.
Full native visual inspection still requires a successful application build.
