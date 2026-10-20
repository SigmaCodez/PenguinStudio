# Historical package builds

These records retain the original versions, source revisions and checksums.
They are evidence of earlier tests, not current installation instructions.
Use [Penguin Studio releases](https://github.com/SigmaCodez/PenguinStudio/releases)
and the [current package guide](PACKAGING.md) for downloads.

## Earlier Ubuntu packages

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
Revision 6 fixes fresh-profile migration, DeckLink UI locale resources, optional-runtime diagnostics (including German system locales), and early portal registration. That revision retained the non-null Qt 6.10.2 startup focus warning; see [the historical startup report](validation/ubuntu-26.04-startup.json). Version 0.2.1 fixes startup dialog sequencing and passes all nine native Wayland startup/shutdown checks; see [the current report](validation/ubuntu-26.04-startup-focus-0.2.1.json).
These checks do not replace live streaming or hardware-specific validation.


## Earlier multi-format 0.2 downloads — 2026-10-19

The packages in this section predate the layout-version-3 redesign. Current packages for every supported format are linked in
[the package guide](PACKAGING.md#automatic-versioned-downloads).

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
At the time of this build, the local `dist/` directory contained all four package formats with SHA-256
sidecars. The [CI evidence](validation/linux-ci-0.2.json) records archive hashes;
[the native package manifest](validation/native-packages-0.2.json) records the
verified extracted DEB, RPM and Snap file hashes. The local Flatpak has its own
manifest below because it was built separately from the CI artifact.

These are development packages. Live-provider, device, hardware-encoder and
cross-desktop checks remain listed in [VERIFICATION.md](VERIFICATION.md).
Earlier 0.1 build results remain in [the baseline CI evidence](validation/linux-ci.json)
and [the baseline Flatpak manifest](validation/flatpak-build-manifest.json).

## Earlier local Flatpak bundle

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

