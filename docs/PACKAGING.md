# Linux builds and packages

Penguin Studio's current source version is **0.2.0**, based on OBS Studio
**32.2.2**. The application ID is `io.github.SigmaCodez.PenguinStudio` and the
launcher is `penguin-studio`. Internal OBS libraries and plugin paths retain
their names for compatibility. Application settings are stored separately
from OBS Studio.

## Supported build targets

| Package | Build environment | Architecture | Distribution scope |
| --- | --- | --- | --- |
| Flatpak | Freedesktop SDK 25.08, pinned OBS dependencies | x86_64 | Linux distributions with Flatpak and working graphics/desktop portals |
| DEB | Ubuntu 24.04 | amd64 | Ubuntu 24.04 and compatible derivatives; rebuild for other library ABIs |
| RPM | Fedora 44 with RPM Fusion free build dependencies | x86_64 | Fedora 44; rebuild for other RPM distributions |
| Snap | core24 / Ubuntu 24.04 | amd64 | Hosts with snapd; development grade until confinement tests pass |

These are build targets, not a claim that every distribution and GPU has
passed functional testing. A DEB or RPM file is not portable across arbitrary
distribution releases. Flatpak is the preferred route for Arch, Debian,
Fedora, openSUSE, Ubuntu, Mint, and other distributions. Its bundled runtime
avoids depending on the host's Qt and FFmpeg versions. ARM builds require
separate dependency manifests and validation; the current Flatpak and Snap
recipes deliberately target x86_64 only.

## Current 0.2 development downloads — 2026-10-19

All five jobs in the final [Linux build run](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39264364809)
passed at application source revision
`8c1d5113a19fa90a5c604e79d81e6e0c60208dbc`. This source includes the redesigned
workspace and the standard-button translation fix. Later documentation and
GUI-test evidence commits do not change the application code.

These packages and their recorded source revisions predate the independent
`main` history. The [history reset](HISTORY.md) preserves their provenance;
it does not rebuild the packages or rerun their checks. The recorded commits
may no longer be reachable from the current repository branches.

| Package | Artifact | Completed checks |
| --- | --- | --- |
| Flatpak x86_64 | [Download ZIP](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39264364809/artifacts/12192348657) | Full application build, export and bundle. The separately built local bundle below also passed installation and native Wayland launch/shutdown checks. |
| Ubuntu 24.04 DEB | [Download ZIP](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39264364809/artifacts/12193416845) | Build, three regression tests, package installation and CLI startup. |
| Fedora 44 RPM | [Download ZIP](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39264364809/artifacts/12193337597) | Build, three regression tests, package installation and CLI startup. |
| Snap core24 | [Download ZIP](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39264364809/artifacts/12193486708) | Build, package installation and confined CLI startup. Full GUI/confinement tests remain open. |

Extract the package from its artifact ZIP before installing. CI artifacts have
an expiry date; keep a local copy or rebuild from the recorded source revision.
The final local `dist/` directory contains all four package formats with SHA-256
sidecars. The [CI evidence](validation/linux-ci-0.2.json) records archive hashes;
[the native package manifest](validation/native-packages-0.2.json) records the
verified extracted DEB, RPM and Snap file hashes. The local Flatpak has its own
manifest below because it was built separately from the CI artifact.

These are development packages. Live-provider, device, hardware-encoder and
cross-desktop checks remain listed in [VERIFICATION.md](VERIFICATION.md).
Earlier 0.1 build results remain in [the baseline CI evidence](validation/linux-ci.json)
and [the baseline Flatpak manifest](validation/flatpak-build-manifest.json).

## Flatpak

The current local development bundle is `dist/penguin-studio-0.2.0-x86_64.flatpak`
(142,757,376 bytes). Its SHA-256 is
`bcfaa6fecf3f3e9e3b1135ccbbf1d8277989ddabff10994137bc3a1cc261c968`.
It contains application source matching commit
`8c1d5113a19fa90a5c604e79d81e6e0c60208dbc`, including the redesigned workspace
and standard-button translation repair. The bundle was installed separately,
its installed launcher reported 0.2.0, and its native Wayland GUI passed the
active-wizard shutdown check. See the
[exact build, source comparison and validation manifest](validation/flatpak-build-manifest-0.2.json).
The earlier 0.1.0 bundle and its evidence remain preserved.
Binaries belong in build artifacts rather than Git source history; the local
`dist/` directory is intentionally ignored by Git.

To install the local versioned bundle, the runtime is sufficient. For the CI
download, replace the bundle path below with the extracted
`penguin-studio-x86_64.flatpak` file:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform//25.08
flatpak install --user ./dist/penguin-studio-0.2.0-x86_64.flatpak
flatpak run io.github.SigmaCodez.PenguinStudio
```

Install Flatpak and flatpak-builder through your distribution, then run:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform//25.08 org.freedesktop.Sdk//25.08
flatpak-builder --user --install-deps-from=flathub --force-clean \
  --repo=build_penguin_flatpak_repo build_penguin_flatpak \
  build-aux/io.github.SigmaCodez.PenguinStudio.json
flatpak build-bundle build_penguin_flatpak_repo penguin-studio-x86_64.flatpak \
  io.github.SigmaCodez.PenguinStudio
flatpak install --user ./penguin-studio-x86_64.flatpak
flatpak run io.github.SigmaCodez.PenguinStudio
```

Keep the builder state and build directory on the same local filesystem.
Reserve at least 20 GB of free disk space for SDKs, downloaded dependencies,
debug symbols, and compiler output. Building on a network share can be slow
and can lose executable/symlink metadata; a local checkout is recommended.

The manifest keeps upstream's checksum-pinned dependencies and CEF browser
support. It grants network, audio, graphics/device, Wayland and PipeWire access,
plus broad host filesystem access (`--filesystem=host`) for local sources and
recordings. This is broader than access to media folders alone. The plugin extension
namespace is separate from OBS Studio, so extensions must be packaged for
Penguin Studio explicitly.

## Native DEB and RPM

Native packages conflict with OBS Studio packages because both provide the
same libobs ABI and library/plugin paths. Use Flatpak or Snap for parallel
installation. After extracting the matching artifact ZIP, install only on its
supported target distribution:

```sh
# Ubuntu 24.04:
sudo apt install ./penguin-studio_0.2.0_amd64.deb
# Fedora 44:
sudo dnf install ./penguin-studio-0.2.0-1.x86_64.rpm
```

To build from source, use the target distribution and start from a recursive checkout:

```sh
git clone --recurse-submodules --branch main https://github.com/SigmaCodez/PenguinStudio.git
cd PenguinStudio
sudo bash scripts/packaging/install-build-deps.sh
bash scripts/packaging/build-native.sh DEB  # Ubuntu 24.04
# Or, inside Fedora 44:
bash scripts/packaging/build-native.sh RPM
```

The scripts download CEF from the URL and SHA-256 pinned in `CMakePresets.json`,
build Penguin Studio with Wayland and browser support, and invoke CPack. DEB runtime
dependencies are derived with `dpkg-shlibdeps`; RPM dependency generation is
enabled. Output is under `build_penguin_deb/` or `build_penguin_rpm/`.

The native, Flatpak and Snap recipes explicitly pass
`-DOBS_VERSION_OVERRIDE=32.2.2` to CMake. Include the same option when configuring
the engine directly; its version is separate from Penguin Studio's application
version in `packaging/version.txt` and must not be inferred from Penguin Git tags.

The native recipe disables AJA hardware support and WebRTC in
line with the upstream Ubuntu preset; vendor hardware and WebRTC builds need
their additional dependencies and tests.

## Snap

The Snap recipe compiles this source tree and includes the pinned CEF bundle;
it does not repackage an upstream OBS binary. To build locally, run `snapcraft`
on a host with Snapcraft installed. To install a downloaded or locally built
Snap on a host with snapd:

```sh
sudo snap install --dangerous ./penguin-studio_0.2.0_amd64.snap
sudo snap connect penguin-studio:audio-record
sudo snap connect penguin-studio:camera
# Only if media on removable drives is needed:
sudo snap connect penguin-studio:removable-media
```

The recipe is `grade: devel`. Store name registration, publishing, signing,
automatic interface connections, and release promotion are separate
maintainer actions. The CI job uses `snapcraft --destructive-mode` only in an
ephemeral Ubuntu 24.04 runner; use Snapcraft's normal isolated build locally.

## Wayland and release checks

Screen capture requires PipeWire and an appropriate `xdg-desktop-portal`
backend for the desktop (GNOME, KDE, or the compositor's recommended backend).
Portal consent is controlled by the desktop. The launcher allows Qt to use
Wayland; it does not force an X11 session. Platform stream-title updates use
network APIs and do not require screen scraping or X11 window automation.

Before promoting a package, verify at least:

- First-run onboarding, separate OBS/Penguin settings, and language switching.
- GNOME and KDE Wayland screen/window capture, including portal cancellation.
- Microphone, desktop audio, camera, recording, browser source, and saved scenes.
- Horizontal and vertical outputs with two local RTMP receivers, reconnects,
  stop/start cycles, and one destination failing while another continues.
- Twitch/YouTube account authorization, title updates, token expiry and revocation
  using maintainer-owned OAuth client registrations and test accounts.
- Hardware encoding on the available NVIDIA/AMD/Intel drivers and software fallback.
- Package install, upgrade, uninstall, and Snap confinement on clean machines.

The workflow `.github/workflows/penguin-linux.yml` builds each target and
uploads artifacts when successful. It does not publish a release or claim
unexecuted live-platform tests have passed.

References: [OBS Linux build instructions](https://github.com/obsproject/obs-studio/wiki/Build-Instructions-For-Linux),
[Flatpak builder documentation](https://docs.flatpak.org/en/latest/flatpak-builder.html),
and [Snapcraft CMake plugin](https://documentation.ubuntu.com/snapcraft/stable/common/craft-parts/reference/plugins/cmake_plugin/).
