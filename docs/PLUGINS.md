# Plugins

Penguin Studio uses the OBS 33 plugin loader. Third-party plugins must support the engine ABI and the libraries available in your package format. Keep a copy of your scene collections before changing a plugin used by those scenes.

## Native Linux locations

For a per-user plugin named `example`, the current layout is:

```text
${XDG_DATA_HOME:-$HOME/.local/share}/PenguinStudio/plugins/example/example.so
${XDG_DATA_HOME:-$HOME/.local/share}/PenguinStudio/plugins/example/data/
```

Use the plugin publisher's package or installation instructions when available. System packages use the distribution's library directory followed by `obs-modules/plugins/`, with resources under `share/obs/obs-modules/plugins/`. Penguin's bundled core modules use `obs-modules/core/`; do not overwrite them with third-party files.

## Legacy plugin locations

The OBS 33-based release still recognizes the older Penguin per-user layout:

```text
${XDG_CONFIG_HOME:-$HOME/.config}/PenguinStudio/plugins/example/bin/64bit/example.so
${XDG_CONFIG_HOME:-$HOME/.config}/PenguinStudio/plugins/example/data/
```

Older system `obs-plugins/` locations are also recognized for compatibility. The Plugin Manager marks these installations as legacy. Ask the plugin maintainer for a package using the current layout before support for legacy locations is removed in a future engine update. Do not move a plugin's binary alone: it may require matching resources, libraries and an ABI update. Remove duplicate installations only after verifying the replacement.

Penguin Studio keeps its per-user plugin directory separate from OBS Studio's per-user configuration. Shared system plugin paths retain their OBS ABI names.

## Flatpak and Snap

Flatpak uses its own sandbox data/configuration directories and compatible extension points. Native host plugins may depend on libraries unavailable inside that sandbox. Snap likewise uses confined application paths. Use plugins packaged for the relevant runtime; copying a native `.so` into a sandbox does not establish compatibility.

## Troubleshooting

Use the Plugin Manager to inspect missing or failed plugins. Safe Mode disables third-party plugins and scripting for diagnosis. Preserve the plugin's author and license notices when redistributing it.
