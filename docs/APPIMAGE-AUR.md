# AppImage and Arch Linux packages

Starting with the 0.4.3 packaging configuration, the Linux workflow builds an
x86_64 AppImage and a checksum-pinned `penguin-studio-bin` Arch packaging recipe.
Use only downloads actually present on the selected [release page](https://github.com/SigmaCodez/PenguinStudio/releases).
A recipe archive on GitHub does not mean the package has been submitted to AUR.
The project does not yet have an AUR maintainer account configured.

## AppImage

The AppImage bundles the tested Ubuntu 24.04 application, Qt and application
libraries. It requires an x86_64 Linux host with glibc 2.39 or newer, compatible
GPU drivers and a working desktop session. It does not bundle a kernel, graphics
drivers or a desktop portal. Older distributions should use Flatpak instead.

Download the AppImage and compare its SHA-256 with the release checksum file:

```sh
chmod +x penguin-studio-0.4.3-x86_64.AppImage
./penguin-studio-0.4.3-x86_64.AppImage
```

If FUSE is unavailable, use:

```sh
./penguin-studio-0.4.3-x86_64.AppImage --appimage-extract-and-run
```

Wayland screen capture needs PipeWire and the desktop's portal backend. Account
persistence needs a working Secret Service or KWallet keyring. Official packages
include the Twitch and YouTube application configuration. User accounts, stream
keys and passwords are never included in the download. Native VLC sources and
Python scripting may require additional host runtimes.

For updates, stop outputs, close Penguin Studio and replace the AppImage with the
new verified download. In-app notifications link to releases; this package does
not invoke APT/DNF or replace a running AppImage. Existing user configuration is
retained. It uses the same PenguinStudio profile directory as native packages;
do not run two copies against the same profile concurrently.

## Arch Linux / AUR recipe

Download `penguin-studio-0.4.3-aur.tar.gz`, extract it, review `PKGBUILD`, then:

```sh
tar -xzf penguin-studio-0.4.3-aur.tar.gz
cd penguin-studio-bin
makepkg -si
```

Run makepkg as an ordinary user. It downloads the exact release AppImage and
verifies a SHA-256 hash, then installs its extracted contents under
`/opt/penguin-studio-bin`. No FUSE mount is needed. The package includes a desktop
entry, icon and original license notices; pacman tracks installed files. It
provides and conflicts with `penguin-studio`, but does not claim to provide OBS
libraries to other Arch packages. Use pacman or an AUR helper for updates once an
AUR entry exists. Until then, repeat these steps with the next release's recipe.

The release workflow generates `.SRCINFO` using makepkg, builds the package as an
unprivileged user in Arch Linux, installs it and checks the launcher before
uploading the recipe archive. This checks packaging, not full GUI or device
compatibility on every Arch installation.

## Maintainer: publish to AUR

1. Create an account at [AUR](https://aur.archlinux.org/register), choose a
   maintainer username and add a dedicated public SSH key to the account.
2. Follow the [AUR submission guidelines](https://wiki.archlinux.org/title/AUR_submission_guidelines),
   including checking whether the package name already exists. Review the
   generated PKGBUILD and `.SRCINFO` before submission.
3. Clone `ssh://aur@aur.archlinux.org/penguin-studio-bin.git`, copy the two files
   from the verified release recipe, commit and push using the maintainer's key.
4. For each version, use the new recipe produced by CI. Do not reuse the previous
   AppImage checksum, use `SKIP`, or publish a recipe before its download exists.

Automatic generation is enabled in GitHub Actions. Uploads to AUR are not enabled
without a registered account and dedicated publishing credentials. Never commit
private SSH keys or account passwords to this repository.

## Build provenance

`scripts/packaging/build-appimage.sh` uses the Ubuntu 24.04 DEB from the same CI
run, discovers dependencies of OBS plugins as well as the main executable, and
bundles Qt's Wayland platform plugins. AppImage tooling is pinned by SHA-256 in
`packaging/appimage/tools.json`; changed upstream downloads fail closed until
reviewed. All original license and author notices remain in the bundle.
The implementation follows the [linuxdeploy packaging guide](https://docs.appimage.org/packaging-guide/from-source/linuxdeploy-user-guide.html).
