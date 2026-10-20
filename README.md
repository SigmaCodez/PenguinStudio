<p align="center"><img src="assets/branding/penguin-studio.png" width="180" alt="Penguin Studio penguin logo"></p>

# Penguin Studio

**Your studio, every screen.** An independent Linux broadcasting application by SigmaCodez, built on OBS Studio.

Penguin Studio brings a redesigned production workspace, native multi-destination output controls, portrait output and platform connections to the OBS production engine. See the [architecture documentation](docs/ARCHITECTURE.md) for the engine baseline and technical details.

![Penguin Studio modern workspace with searchable layers and scenes, audio channel strips and an integrated inspector](docs/validation/workspace-modern-en.png)

Actual Linux application capture with test media. See the [workspace guide](docs/WORKSPACE.md) for the controls and layout.

> Development preview. See [verification status](docs/VERIFICATION.md) for completed checks and remaining release tests, and [packaging](docs/PACKAGING.md) for development downloads and supported build targets. Validate your devices and streaming accounts before relying on this build for a live show.

## What is included

- A native production workspace with searchable layers and numbered scenes on the left, canvas in the center, a channel-strip audio mixer below, and tabbed inspector/output/details tools on the right. The top bar brings together destinations, live output, recording and replay clips.
- An editable inspector for source position, size, rotation, crop, visibility and locking, with undo/redo, source properties and filters. See the [workspace guide](docs/WORKSPACE.md).
- Original Penguin Studio logo, a complete dark control theme, English defaults and a separate `PenguinStudio` configuration directory.
- The upstream OBS capture, scene, recording, mixer, filter, plugin and translation infrastructure, including all 77 frontend locale files.
- A first-launch destination assistant, platform dashboard links and up to eight independently controlled RTMP(S) outputs.
- Landscape program output and a separate 1080 × 1920 portrait canvas with center crop or fit. Each destination has its own H.264/AAC encoding pipeline.
- Native Twitch device authorization and YouTube desktop authorization with PKCE. The **Stream details** dock can update titles through their APIs without an embedded browser, including in a Wayland session.
- Build definitions for Flatpak, Snap, DEB and RPM, with a Linux CI workflow. See [packaging](docs/PACKAGING.md).

The workspace takes inspiration from modern broadcasting tools such as Meld Studio. It uses original branding and OBS's native Qt interface; it is not an identical Meld implementation.

## Get started

**[Download Linux packages](https://github.com/SigmaCodez/PenguinStudio/releases)** — Ubuntu DEB, Fedora RPM, Flatpak and Snap, with SHA-256 checksums.

**Ubuntu 26.04 / 26.04.1:** use a DEB built specifically for Ubuntu 26.04 or the
Flatpak bundle. Choose the package matching your distribution release. See [installation and troubleshooting](docs/PACKAGING.md#ubuntu-2604--26041-installation).

1. Install the Penguin Studio package for your system following the [installation guide](docs/PACKAGING.md).
2. Launch `penguin-studio`. Finish the OBS hardware configuration assistant and Penguin's destination assistant.
3. Choose a platform, copy its RTMP(S) server and stream key from its creator dashboard, and choose landscape or portrait output.
4. Use **Destinations** to add further destinations. **Go live** starts all configured Penguin destinations; **End stream** stops them. The live button's menu also exposes OBS's separate primary stream action.
5. Open the arrow menu beside **Go live**, then choose **Stream details** to connect Twitch or YouTube for title updates. Current `main` builds bundle the Penguin Twitch application ID and add **Docks → Twitch chat**. YouTube still requires your own registered application; see [platform setup](docs/PLATFORMS.md).
6. Make a private test stream before a public broadcast. Check picture framing, sound, reconnects, CPU/GPU load, dropped frames and total upload bandwidth.

In the new **Broadcast destinations** and **Stream details** docks, stream keys, OAuth access/refresh tokens and client secrets remain in memory for the current session. They must be entered again after restart. Destination names, nonsecret server URLs, encoder choices and public application IDs are saved locally. Never put a stream key into a saved server URL. The inherited OBS **Settings → Stream** workflow has separate storage behavior and can persist stream credentials in the local profile.

## Penguin Studio updates

New Penguin Studio versions are published on our [GitHub Releases page](https://github.com/SigmaCodez/PenguinStudio/releases), with release notes, Linux packages and SHA-256 checksums. To update, close Penguin Studio and install the newer package for the same distribution release and package format. See the [package guide](docs/PACKAGING.md) for format-specific instructions. Direct downloads do not require a source checkout.

Penguin Studio has its own version numbers and release schedule. Our GitHub Actions workflow builds and publishes the Linux packages when a new project version is released. These downloads are development previews; check the release notes and [verification status](docs/VERIFICATION.md) before updating a production setup.

## Scope and current limits

- Portrait currently crops or fits the program scene. Independently editable portrait scenes and a dual-preview editor remain future work.
- Encoders are selected per destination; hardware availability and simultaneous encoding limits depend on the GPU and drivers. Encoder sharing is not implemented.
- Twitch and YouTube metadata connections require their respective developer registrations and enabled API access. No OBS Project application credentials are bundled.
- Kick, Facebook, TikTok and other RTMP(S) platforms use server/key setup. Native account login and title editing are not implemented for every provider; platform eligibility restrictions still apply.
- PipeWire capture and a working desktop portal are needed for Wayland screen sharing. Browser sources still depend on upstream CEF support and must be tested on the target compositor.
- Flatpak is the primary cross-distribution target. Native DEB/RPM builds target the distribution used to build them. Retained libobs paths mean native packages conflict with an existing native OBS installation.
- All existing OBS languages remain selectable. New Penguin dialogs have their own translations and English fallback; see [localization](docs/LOCALIZATION.md).

## Updates

Version 0.3.0 adds **Help → Check for updates**, automatic release notifications
and verified package installation for supported Ubuntu, Fedora and Flatpak
installations. See [in-app updates](docs/UPDATES.md) for package
support and the Snap limitation. Downloads always come from
[Penguin Studio releases](https://github.com/SigmaCodez/PenguinStudio/releases).

## Contributing

For source checkout, builds and maintenance, see the [developer guide](CONTRIBUTING.md).

## Attribution and license

Penguin Studio is an independent application based on OBS Studio, not an official OBS Project or Meld Studio product. OBS Studio and the derived source are distributed under **GPL-2.0-or-later**; see [COPYING](COPYING), [AUTHORS](AUTHORS), and the original [OBS README](README.rst). Third-party components retain their individual licenses. Original Penguin contributions use the same GPL license. Redistributed binaries must be accompanied by the corresponding source and required notices.
