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

**[Download Penguin Studio 0.2.0](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.2.0)**
(published 2026-10-20). All five package jobs and the release job passed in
[run 39394971754](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39394971754),
built from `3a229e46eac6b48a50b829e676d361107d9671df`. The uploaded package sizes
and GitHub SHA-256 digests match the downloaded build manifest and checksum file;
see [release verification](validation/linux-release-0.2.0.json).

[GitHub Releases](https://github.com/SigmaCodez/PenguinStudio/releases) provides
permanent downloads of Ubuntu 24.04 and 26.04 DEBs, Fedora 44 RPM, Flatpak and
Snap, plus `SHA256SUMS` and `build-manifest.json`. Releases are marked as
prereleases while Penguin Studio remains a development preview.

To publish a new version, increase **`packaging/version.txt`** (for example from
`0.2.0` to `0.2.1`) and push it to `main`. This single file supplies the application,
native package and Snap versions; the Flatpak filename uses the same value.
The Linux workflow detects the version change, runs the checks, builds all five
packages, creates `penguin-v<version>` at the exact build commit, uploads the
complete download set to a draft, and only then publishes it. A failed build or
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

The original `penguin-studio_0.2.0_amd64.deb` was built for **Ubuntu 24.04**.
It cannot be installed on Ubuntu 26.04: it requires FFmpeg 6, Python 3.12,
Mbed TLS 2 and Qt library package names unavailable in the newer distribution.
The Ubuntu App Center may leave an installation spinner visible even after
PackageKit reports `dependencyResolutionFailed`. Waiting or retrying that DEB
will not resolve the incompatibility.

New native builds include the distribution release in the Debian revision,
for example `penguin-studio_0.2.0-6+ubuntu26.04_amd64.deb`. Ubuntu 26.04.1 uses
the Ubuntu 26.04 package. Do not mix 24.04 repositories into 26.04, remove
package dependencies, or create compatibility library symlinks to force an
older binary to load.

The earlier locally verified [Ubuntu 26.04 DEB artifact](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39333338354/artifacts/12225468993)
was built from `6ea4a2994fff010d3cdef93a6007a9435023b674`. The
[verification run](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39333338354)
passed compilation, output-safety/platform-flow/wizard tests, package installation
and `penguin-studio --version` on Ubuntu 26.04. The downloaded archive and DEB
checksums were verified, and APT dependency resolution passed on Ubuntu 26.04.1
without requiring package removals. The original installation is recorded in [the installation report](validation/ubuntu-26.04-installation.json);
revision 2 fixed compact wizard layout, as recorded in [the native Wayland report](validation/ubuntu-26.04-wayland.json).
Revision 3 additionally fixes crash-sentinel and frontend callback cleanup; all six
installed Wayland shutdown cases pass in [the shutdown report](validation/ubuntu-26.04-shutdown.json).
Revision 4 adds the modern workspace, responsive dialogs and their installed Wayland GUI, recording and shutdown checks in [the redesign report](validation/workspace-modern.json).
Revision 5 corrects the severity of two known Qt 6.10.2 Wayland teardown diagnostics and passes all nine installed shutdown scenarios without shutdown warnings or errors; see [the focused report](validation/ubuntu-26.04-qt-focus.json).
Revision 6 fixes fresh-profile migration, DeckLink UI locale resources, optional-runtime diagnostics (including German system locales), and early portal registration. The remaining non-null Qt 6.10.2 focus warning is still open; see [the startup report](validation/ubuntu-26.04-startup.json).
These checks do not replace live streaming or hardware-specific validation.

The local package is `dist/penguin-studio_0.2.0-6+ubuntu26.04_amd64.deb`.
Its SHA-256 is `bb95491596cdc0bf27bc85edbe0e4aa230bd7df1dc7e1e01bc26970b29bfbdd3`.

After downloading the matching DEB, the repository's installer checks the
release, architecture and APT dependency resolution before installation:

```sh
bash scripts/packaging/install-deb.sh --check /path/to/penguin-studio_0.2.0-6+ubuntu26.04_amd64.deb
bash scripts/packaging/install-deb.sh /path/to/penguin-studio_0.2.0-6+ubuntu26.04_amd64.deb
```

The first command makes no system changes. The second uses the normal Ubuntu
administrator prompt and refuses package removals. Alternatively, open the
matching DEB in the App Center. Flatpak remains the option that does not depend
on the host's Qt, Python or FFmpeg library versions.

## Earlier multi-format 0.2 downloads — 2026-10-19

The packages in this section predate the layout-version-3 redesign. The latest
Ubuntu 26.04 package is linked above; rebuild other formats from current source
to include the new layout.

All five jobs in the earlier [Linux build run](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39264364809)
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

The earlier local development bundle is `dist/penguin-studio-0.2.0-x86_64.flatpak`
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
# Ubuntu 26.04 / 26.04.1, after downloading the matching build:
sudo apt install --no-remove ./penguin-studio_0.2.0-6+ubuntu26.04_amd64.deb
# Use a package built for your Ubuntu release.
# Fedora 44:
sudo dnf install ./penguin-studio-0.2.0-1.x86_64.rpm
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
uploads artifacts when successful. Version changes also publish the complete
release download set as described above. Automated builds do not establish that
unexecuted live-platform tests have passed.

References: [OBS Linux build instructions](https://github.com/obsproject/obs-studio/wiki/Build-Instructions-For-Linux),
[Flatpak builder documentation](https://docs.flatpak.org/en/latest/flatpak-builder.html),
and [Snapcraft CMake plugin](https://documentation.ubuntu.com/snapcraft/stable/common/craft-parts/reference/plugins/cmake_plugin/).
