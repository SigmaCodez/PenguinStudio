# Verification and release gates

This is a development preview based on OBS Studio 32.2.2. The new Penguin code must not be described as fully production-tested until the checks below have real recorded results.

The results below predate the [independent `main` history](HISTORY.md). Their
original source hashes, branch names and CI links remain historical evidence;
they do not assert that the new root commit was tested. Old commit links may
become unavailable after the former branches and tags are removed.

## Workspace 0.2 results — 2026-10-19

Workspace test source: [`6ed8300525b17e1fd0597389a2b139956228240d`](https://github.com/SigmaCodez/PenguinStudio/commit/6ed8300525b17e1fd0597389a2b139956228240d).
The final application source, [`8c1d5113a19fa90a5c604e79d81e6e0c60208dbc`](https://github.com/SigmaCodez/PenguinStudio/commit/8c1d5113a19fa90a5c604e79d81e6e0c60208dbc), adds the standard-button translation fix and its focused regression test. Dialog evidence records that later source explicitly.

| Check | Result | Evidence / scope |
| --- | --- | --- |
| Full application rebuild | Passed | The native workspace, inspector, themes, translations and existing plugins compiled and linked in Freedesktop SDK 25.08. Strict C++17 checks also passed for the new widgets. |
| English and German GUI | Passed | Actual application tested at 1440 × 900 and 1280 × 800. Inspector controls remain within the horizontal viewport. Screenshots use native XComposite capture, including the actual OpenGL canvas. |
| Properties, filters and Settings | Passed visual/navigation checks | 34 real dialog captures across English and German, including all nine Settings pages and lower scrollable sections. Fixed GNOME standard buttons that displayed English Cancel in German; focused mnemonic/escaping tests and actual Abbrechen button text pass. |
| Inspector and undo/redo | Passed | Position, dimensions, rotation, crop, aspect linking, visibility, locking, flips, selection changes, negative scale, bounds and grouped-child transforms after video-frame group resizing. |
| Production and canvas controls | Passed | Destinations, the no-destination setup flow, Studio Mode and recording use the actual controls. Local recordings contain 1280 × 720 H.264 video and 48 kHz AAC audio. No external broadcast was started. |
| Native Wayland workspace | Passed on the test host | The runtime suite exercises the same widgets and real recording under the Wayland Qt platform. Native X11 keyboard undo was tested separately. |
| Final Linux CI | Passed all five jobs | Branding/localization, configuration, OAuth and Qt standard-button checks plus Flatpak, DEB, RPM and Snap builds passed at final application source `8c1d511`. |
| Final Ubuntu 24.04 DEB and Fedora 44 RPM | Passed build, tests, install and CLI | Both final packages at source `8c1d511` passed the output-safety, platform-flow and wizard-lifecycle regressions, installed in their target distribution and reported Penguin Studio 0.2.0. Full GUI/device checks on clean target distributions remain open. |
| Final Snap core24 | Passed build, install and CLI | The final package at source `8c1d511` installed and its confined launcher reported Penguin Studio 0.2.0 without an undefined-symbol loader warning. Full GUI/confinement checks remain open. |
| Final Flatpak installation and native Wayland launch | Passed on the test host | The 0.2.0 bundle contains source matching final commit `8c1d511`. Installed and staged binaries match; installed launch and active-wizard shutdown passed, with zero OBS leaks and all 95 license notices verified. |
| Layout migration and restart | Passed | An isolated 0.1 test configuration migrated to workspace version 2. A customized inspector dock position survived a subsequent launch. Personal profiles were not used. |
| Shutdown with setup open | Passed | Closing the application while the first-run wizard is open exits normally. Completed GUI runs report zero OBS memory leaks. |
| Configuration, OAuth and plugin regressions | Passed | Configuration isolation, callback/PKCE checks, output lifetime checks, asynchronous wizard lifecycle and mocked platform flows rerun successfully. |
| New localization | Passed coverage checks | All 50 workspace/inspector keys are complete in English and seven translations. All 77 inherited frontend catalogues remain present. Visual checks cover English and German; native-speaker review remains open. |

Detailed results and reproduction: [workspace evidence](validation/workspace-0.2.json),
[GUI helper](../test/penguin-workspace-smoke.cpp),
[isolated runner](../scripts/packaging/verify-workspace.sh),
and the [workspace guide](WORKSPACE.md).
Additional [dialog evidence](validation/dialogs-0.2.json) includes actual
[German source properties](validation/dialogs-0.2-source-properties-de.png),
[filters](validation/dialogs-0.2-source-filters-en.png) and
[output settings](validation/dialogs-0.2-settings-output-en.png).
Screenshots: [English](validation/workspace-0.2-en.png),
[compact workspace](validation/workspace-0.2-compact.png),
[German](validation/workspace-0.2-de.png).

Final package run: [five successful jobs and artifact metadata](validation/linux-ci-0.2.json),
[verified native package file hashes](validation/native-packages-0.2.json), and
[download/install instructions](PACKAGING.md).

Final local Flatpak: [source/build/installation manifest](validation/flatpak-build-manifest-0.2.json),
[runtime metadata](validation/flatpak-0.2-metadata.ini), and
[license hashes](validation/flatpak-0.2-licenses.sha256).

The fixtures use the Penguin image asset and generated WAV files through the
shipping image/media source modules. Capture-device, portal, hardware-encoder
and real-provider limitations below still apply.

## Baseline 0.1 results — 2026-10-19

| Check | Result | Evidence / scope |
| --- | --- | --- |
| Complete application build | Passed | All 624 targets compiled and linked in Freedesktop SDK 25.08, including both Penguin plugins and CEF browser support. |
| Branding and localization | Passed | `scripts/check-branding.py` and `scripts/check-penguin-locales.py`; all 77 upstream frontend locales retained. |
| Configuration isolation | Passed | C++17 test compiled with `-Wall -Wextra -Werror`; OBS paths map to the separate Penguin configuration tree. |
| OAuth callback and encoding checks | Passed | Qt6 test covers RFC 7636 PKCE, state checks, malformed/duplicate callback fields and form encoding. No real provider account was authorized. |
| Platform controller flows | Passed with mocked providers | Real Qt events and a localhost OAuth callback exercise device polling/backoff, rotating refresh tokens, broadcast pagination, title field preservation, unauthorized-session handling and cancellation. This does not replace provider-account testing. |
| Output failure/lifetime checks | Passed | Offline tests exercise invalid endpoints, missing encoders/keys, session-key omission and safe destruction of failed, live and stopped outputs. |
| Real simultaneous landscape/portrait output | Passed | Native Wayland EGL with software H.264/AAC encoding; local FFmpeg receivers captured 1280 × 720 and 1080 × 1920 streams. Dimensions, audio and decoded picture content verified. |
| Output lifecycle and recovery | Passed | Portrait crop/fit, stop/restart, active-output destruction, unreachable receiver cleanup and automatic reconnection after receiver restart. |
| First-run wizard | Passed for inspected screens | Welcome and connection screens rendered; clipped subtitle found and fixed. Screenshots linked below. |
| Native Wayland GUI and shutdown | Passed | Fresh isolated profile, default theme, custom docks and wizard inspected. Closing the main window with the wizard open reproduced a double deletion; asynchronous dialog ownership fixes it. The exact scenario now exits successfully with zero OBS-reported memory leaks. |
| Desktop/AppStream metadata | Passed | Desktop entry and AppStream validators; launcher quoted-path smoke check. |
| Flatpak build, installation and runtime | Passed on the test host | The 0.1.0 x86_64 bundle was installed in an isolated Flatpak installation and launched on Freedesktop Platform 25.08. Native Wayland GUI/shutdown passed with fresh in-sandbox configuration. |
| Ubuntu 24.04 DEB | Passed build, tests, install and CLI | Three Penguin regression executables passed; the generated package installed and its launcher reported Penguin Studio 0.1.0. Full GUI and device checks on Ubuntu remain open. |
| Fedora 44 RPM | Passed build, tests, install and CLI | Three Penguin regression executables passed; the generated package installed with DNF and its launcher reported Penguin Studio 0.1.0. Full GUI and device checks on Fedora remain open. |
| Snap core24 | Passed build, install and CLI | The repaired package at revision `30f6279c14fa879df18b52ed219b5bef2fa6ed78` starts without the earlier SVG loader ABI warning. Full GUI/confinement tests remain open. |

Detailed output-test evidence: [output-validation.json](validation/output-validation.json).
The reproducible native test lives in [local-stream.cpp](../plugins/penguin-outputs/tests/local-stream.cpp).
Screenshots: [welcome](validation/setup-welcome.png), [connection setup](validation/setup-connection.png),
and [standalone GitHub repository](validation/standalone-repository.jpg).

Flatpak evidence: [build manifest and checksum](validation/flatpak-build-manifest.json),
[runtime metadata](validation/flatpak-metadata.ini), [license checksums](validation/flatpak-licenses.sha256),
[installed GUI test](validation/gui-validation.txt), and [themed first-run wizard](validation/first-run-wizard-wayland.png).

The streams used a synthetic source and loopback receivers only. No desktop
capture, account credentials or public broadcasts were used. The UI test surface
was Qt offscreen; OBS rendered video through native Wayland EGL. This verifies
the actual encoder/output pipeline, not all desktop portal interactions.

The following remain unverified: real Twitch/YouTube authorization and title
updates, provider ingest and RTMPS, PipeWire portal capture, physical audio/video
devices, hardware encoder sessions, full GUI/device testing on clean distributions,
package upgrades/uninstall and Snap confinement beyond CLI startup. The inherited
OBS test suite has not been run in full. Independently
editable portrait scenes and a Meld-identical editor are not implemented.

## Required release checks

- Build the full application and its two Penguin plugins with warnings enabled. Run upstream tests and Penguin-specific checks.
- Launch on GNOME Wayland, KDE Plasma Wayland, a wlroots compositor and X11; verify the first-run wizard, theme, localized UI, docking and settings isolation from OBS.
- Capture a screen/window with PipeWire, camera and microphone; verify recording, monitoring, browser sources, hardware encoders and reconnect behavior.
- Stream simultaneously to two authorized test destinations, with landscape and portrait enabled. Check audio sync, framing, reconnection and independent stop/start.
- Stop while connecting, deny an invalid key, remove a stopped destination, change profiles/scene collections and close the application while outputs are active. Check for hangs, callbacks after deletion and resource leaks.
- Test OAuth success, denial, expiry, token refresh, cancelled sessions, invalid callbacks and network outages. Verify Twitch and YouTube title changes in the provider dashboard.
- Install and launch each generated package in a clean supported distribution; check uninstall behavior, portal access and coexistence/conflict declarations.
- Inspect all source/license contents, package metadata and artifact checksums. Test on at least one AMD/Intel and one NVIDIA system before publishing compatibility claims.

Account approval, streaming service availability, GPU drivers and compositor behavior cannot be certified by static checks. No live broadcast should be started by automated repository validation.
