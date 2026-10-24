# Linux builds and packages

Penguin Studio's current source version is **0.6.1**, based on OBS Studio
**32.2.2**. The application ID is `io.github.SigmaCodez.PenguinStudio` and the
launcher is `penguin-studio`. Internal OBS libraries and plugin paths retain
their names for compatibility. Application settings are stored separately
from OBS Studio.

See [AppImage and Arch/AUR instructions](APPIMAGE-AUR.md) for the new package formats.

## Supported build targets

| Package | Build environment | Architecture | Distribution scope |
| --- | --- | --- | --- |
| AppImage | Ubuntu 24.04 with bundled Qt | x86_64 | glibc 2.39+ hosts; graphics drivers and portals remain host-provided |
| Arch/AUR recipe | Arch Linux makepkg; packages the AppImage | x86_64 | Arch Linux; GitHub recipe download, AUR submission pending |
| Flatpak | Freedesktop SDK 25.08, pinned OBS dependencies | x86_64 | Linux distributions with Flatpak and working graphics/desktop portals |
| DEB | Ubuntu 24.04 and Ubuntu 26.04, built separately | amd64 | Install only the package matching the host Ubuntu release |
| RPM | Fedora 44 with RPM Fusion free build dependencies | x86_64 | Fedora 44; rebuild for other RPM distributions |
| Snap | core24 / Ubuntu 24.04 | amd64 | Hosts with snapd; development grade until confinement tests pass |

These are build targets, not a claim that every distribution and GPU has
passed functional testing. A DEB or RPM file is not portable across arbitrary
distribution releases. Flatpak is the preferred route for Arch, Debian,
Fedora, openSUSE, Ubuntu, Mint, and other distributions. Its bundled runtime
avoids depending on the host's Qt and FFmpeg versions. ARM builds require
separate dependency manifests and validation; the current Flatpak and Snap
recipes deliberately target x86_64 only.

## Automatic versioned downloads

**[Download Penguin Studio 0.6.1](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.6.1)**
Publication requires every package build and release check to pass. Refer to
the release build manifest for the final source commit and checksums.

[GitHub Releases](https://github.com/SigmaCodez/PenguinStudio/releases) provides
permanent downloads of Ubuntu 24.04 and 26.04 DEBs, Fedora 44 RPM, Flatpak,
Snap, AppImage and an Arch recipe archive, plus `SHA256SUMS` and
`build-manifest.json`. The Arch recipe is not yet listed on aur.archlinux.org.
Releases are marked as prereleases while Penguin Studio remains a development preview.

To publish a new version, increase **`packaging/version.txt`** (for example from
`0.6.1` to `0.6.2`) and push it to `main`. This single file supplies the application,
native package and Snap versions; the Flatpak filename uses the same value.
Refresh the English product text and real candidate-version screenshots together
with the version change; the [publication check](PUBLICATION.md) rejects stale previews.
The Linux workflow detects the version change, runs the checks, builds all six binary packages and validates the Arch recipe, creates `penguin-v<version>` at the exact build commit, uploads the
complete download set (six binaries and one recipe archive) to a draft, and only then publishes it. A failed build or
upload cannot publish a partial release. Existing public releases are preserved;
use a new version for changed binaries. Re-run failed jobs on the original run to
retry a failed release for the same commit.

For the initial release or a deliberate retry, open **Actions → Penguin Studio
Linux packages → Run workflow**, select `main`, `packages: all`, and enable
`publish`. Manual builds default to artifacts only. Matching `penguin-v<version>`
tag pushes also publish all formats. Pull requests and ordinary pushes without
a version change only produce CI artifacts. Do not use `[skip ci]` on a version
bump. Snap Store and Flathub submissions are separate from these direct downloads.

## Ubuntu 26.04 / 26.04.1 installation

Download the Ubuntu 26.04 DEB from [Penguin Studio releases](https://github.com/SigmaCodez/PenguinStudio/releases).
Ubuntu 26.04.1 uses the same package. Do not install the Ubuntu 24.04 DEB on
26.04 or mix distribution repositories to satisfy its dependencies.

The installed Ubuntu 26.04 package passed the full Wayland camera and alpha-spill
tests, plus a real Facecam restart with the saved custom image and clean shutdown. See
[0.6.1 validation](validation/camera-background-edges-0.6.1.md) for scope.
Earlier installed results remain in the [verification history](VERIFICATION.md).

The download is `penguin-studio_0.6.1-1+ubuntu26.04_amd64.deb`.
Verify it against `SHA256SUMS` from the same release before installing.

After downloading the matching DEB, the repository's installer checks the
release, architecture and APT dependency resolution before installation:

```sh
bash scripts/packaging/install-deb.sh --check /path/to/penguin-studio_0.6.1-1+ubuntu26.04_amd64.deb
bash scripts/packaging/install-deb.sh /path/to/penguin-studio_0.6.1-1+ubuntu26.04_amd64.deb
```

The first command makes no system changes. The second uses the normal Ubuntu
administrator prompt and refuses package removals. Alternatively, open the
matching DEB in the App Center. Flatpak remains the option that does not depend
on the host's Qt, Python or FFmpeg library versions.

## Flatpak

Download `penguin-studio-0.6.1-x86_64.flatpak` from the Penguin Studio release.
Only the runtime is needed to install or update the bundle:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform//25.08
flatpak install --user --or-update ./penguin-studio-0.6.1-x86_64.flatpak
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
installation. Download the package directly from Penguin Studio releases and install only
on its supported target distribution:

```sh
# Ubuntu 26.04 / 26.04.1, after downloading the matching build:
sudo apt install --no-remove ./penguin-studio_0.6.1-1+ubuntu26.04_amd64.deb
# Use a package built for your Ubuntu release.
# Fedora 44:
sudo dnf install ./penguin-studio-0.6.1-1.x86_64.rpm
```

To build from source, use the target distribution and start from a recursive checkout:

```sh
git clone --recurse-submodules --branch main https://github.com/SigmaCodez/PenguinStudio.git
cd PenguinStudio
sudo bash scripts/packaging/install-build-deps.sh
bash scripts/packaging/build-native.sh DEB  # Build on the intended Ubuntu 24.04 or 26.04 host
# Or, inside Fedora 44:
bash scripts/packaging/build-native.sh RPM
```

The scripts download CEF from the URL and SHA-256 pinned in `CMakePresets.json`,
build Penguin Studio with Wayland and browser support, and invoke CPack. DEB runtime
dependencies are derived with `dpkg-shlibdeps`; RPM dependency generation is
enabled. DEB revisions and descriptions identify the build distribution release. Output is under `build_penguin_deb/` or `build_penguin_rpm/`.

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
sudo snap install --dangerous ./penguin-studio_0.6.1_amd64.snap
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
uploads artifacts when successful. Version changes also publish the complete
release download set as described above. Automated builds do not establish that
unexecuted live-platform tests have passed.

The commands in this guide are the Penguin Studio build instructions. Optional
[OBS integration guidance](HISTORY.md#future-upstream-changes) is for maintainers.
Tool references:
[Flatpak builder documentation](https://docs.flatpak.org/en/latest/flatpak-builder.html),
and [Snapcraft CMake plugin](https://documentation.ubuntu.com/snapcraft/stable/common/craft-parts/reference/plugins/cmake_plugin/).
