# Updates inside Penguin Studio

On Linux, **Settings → General → Check for updates** opens Penguin Studio's updater. It checks
only releases in `SigmaCodez/PenguinStudio`, compares numeric Penguin versions,
and offers newer versions. The app also checks five seconds after every normal Linux startup, including profiles that previously disabled automatic checks. Up-to-date and failed background checks do not open a dialog. A successful check earlier that day does not suppress a new startup check. Automatic notifications wait until active
outputs and modal dialogs have finished and Penguin is active. Development-preview releases can be excluded in the update dialog. Automatic startup checking is always enabled on Linux; the explicit `--disable-updater` switch remains available for isolated tests.

**Download and install update** chooses the matching x86_64 Ubuntu 26.04 or Ubuntu 26.10
DEB, Fedora 44 RPM, or Flatpak bundle. It verifies the exact asset name, release
URL, byte count and GitHub-provided SHA-256 digest before invoking a package
manager. Missing or invalid digests disable installation. Verification relies
on HTTPS and the project's GitHub release metadata, not a separate signing key.
Starting with the OBS 33-based release, no native Ubuntu 24.04 DEB is published.
The updater must find a package for the installed distribution; it will not install
a DEB built for a different Ubuntu release. Use a compatible Flatpak or Snap runtime
when a native package is unavailable.

All streaming outputs (including custom multistream outputs), recording, replay
buffers and the virtual camera must be stopped before installation.

Native packages use `pkexec` with the system `apt-get` or `dnf`, without a shell.
The application never collects an administrator password. APT is forbidden from
removing packages. Flatpak uses its existing host-service permission to invoke
`flatpak install --or-update` for the detected user/system installation. Custom
Flatpak installation roots are not automatically selected. Update packages are
staged in a temporary directory (the app cache for Flatpak), not written into
the installation directly. The update
dialog prevents ordinary window closure while the package manager is running.
After success, close and reopen Penguin Studio to load the updated application.

AppImage, Arch packages, strict Snap packages and unrecognized installation layouts show the new release
and a link to its downloads; they do not run apt/dnf or bypass sandbox rules.
Store-managed Snaps use snapd's update mechanism. Locally installed unsigned
Snaps still require a new bundle installed through the host's package tools.
An interrupted or failed package installation must be checked with the system
package manager before retrying; the UI does not claim rollback occurred.

The release workflow supplies assets and their GitHub digests. No OBS update
endpoint, downloaded installer script, credential or API token is used. The
updater does not publish releases or start a stream.
