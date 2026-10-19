# Verification and release gates

This is a development preview based on OBS Studio 32.2.2. The new Penguin code must not be described as fully production-tested until the checks below have real recorded results.

## Qt Wayland teardown diagnostics — 2026-10-19

Installed Ubuntu package `0.2.0-5+ubuntu26.04`, built from
`ff9f3334b7d97b71c02bc03b3a0bded4113e5a5c`, passes all **nine native Wayland shutdown
scenarios**: normal exit, multiple instances, an open initial setup wizard,
connection-wizard cancellation, shutdown with a verified focused connection
text field, auto-configuration cancellation, Statistics, profile restart, and
SIGTERM. Every case exits with code 0, **zero shutdown warnings or errors**,
zero late frontend callback removals, no remaining crash sentinels, and zero
OBS memory leaks. The two previously reported Qt teardown messages are no longer emitted as warnings.

The cause is a Qt 6.10.2 diagnostic false positive for a surface proxy already
freed during normal window teardown. This is a narrowly scoped diagnostic
compatibility correction, not a change to focus handling or shutdown timing.
The original messages remain available as DEBUG output with `--verbose`;
4 such messages were retained in the installed-package suite.
All **92 classification boundary checks** pass, including genuine non-null
mismatches and critical/fatal messages that must keep their original severity.
Startup diagnostics outside this exact case remain visible and are recorded
separately; this is not a claim that all startup logs are warning-free.

[Implementation rationale and upstream references](QT_WAYLAND_DIAGNOSTICS.md),
[installed-package evidence and checksum](validation/ubuntu-26.04-qt-focus.json),
and [native reproduction instructions](../test/README-native-wayland.md).
The previous revision-4 GUI and recording results below remain historical
validation of the unchanged workspace; they were not rerun in full for this
logging-only correction.

## Modern workspace and responsive dialogs — 2026-10-19

Ubuntu package `0.2.0-4+ubuntu26.04` contains the third workspace layout:
searchable layers and numbered scenes, an integrated Inspector/Outputs/Details
sidebar, a reorganized source inspector, destination cards and wider audio
channel strips. Settings, properties, filters and the setup assistant share the
new theme. Settings navigation follows translated labels; long form rows can
stack vertically. A long stream-delay explanation now occupies two lines while
retaining the original checkbox and its complete accessible name.

The installed package passes **840 workspace assertions** across English and
German at normal and larger text sizes under native Qt 6.10.2 Wayland. Each run
exercises 1440 × 900 and 1280 × 800 windows, source transforms and undo, filtering,
scene renaming, production navigation and a real local H.264/AAC recording.
All four setup-wizard cases pass. The dialog suite covers all nine Settings pages,
source properties and filters in 35 captures with 370 input-height
checks. Navigation labels fit and all tested Settings pages have zero horizontal
overflow, including German FontScale 14 in a 1040-pixel settings window.

All six shutdown scenarios pass again with exit code 0, no shutdown errors,
no late callback removals, no remaining crash sentinels and zero OBS memory leaks.
Qt Wayland focus warnings during some dialog closures remain recorded; these
results do not claim that every log is warning-free. Layout migration preserves
the previous dock state, and a custom arrangement survives restart.

The SDK build separately passed 209 X11 and 210 Wayland workspace assertions,
16 narrow-sidebar cases, output-safety, wizard-lifecycle and mocked OAuth checks.
Live provider accounts and physical hardware were not newly tested by this
layout change. [Detailed results](validation/workspace-modern.json) record the
exact package source, build and checksums. [Reproduction instructions](../test/README-native-wayland.md)
and the [workspace guide](WORKSPACE.md) describe the controls and test probes.

Screenshots: [workspace](validation/workspace-modern-en.png),
[compact workspace](validation/workspace-modern-compact.png),
[large German text](validation/workspace-modern-de-large.png),
[video settings](validation/workspace-modern-settings-video.png), and
[advanced settings](validation/workspace-modern-settings-de-large.png).
The first two are actual X11 application captures including the preview. The
Wayland images are Qt widget renders; separate decoded recordings validate video.

## Native shutdown cleanup — 2026-10-19

Installed Ubuntu package `0.2.0-3+ubuntu26.04` passes six native Wayland exit
scenarios: normal close, multi-instance mode, an open setup wizard, opening and
closing Statistics, restart with the same profile, and graceful SIGTERM.
Each exits with code 0, zero shutdown errors, zero late frontend callback
removals, zero remaining crash sentinels, and zero OBS-reported memory leaks.
Closing with the setup wizard open still emits two Qt 6.10.2 Wayland text-input
focus warnings; they are retained in the report. The other five shutdown logs
contain no warnings. These warnings do not prevent a successful exit.
The observer forwards all API calls unchanged; the previous package fails the
same regression test with two late callback removals.

Inactive crash handlers now skip sentinel cleanup. AudioMixer and the WebSocket
plugin remove callbacks before frontend destruction and avoid removing them
again in their destructors. The pinned upstream WebSocket revision is retained
with a checked, idempotent build patch. See [the evidence](validation/ubuntu-26.04-shutdown.json)
and [reproduction instructions](../test/README-native-wayland.md).
These checks cover shutdown; optional hardware-plugin startup diagnostics and
live-provider validation remain outside this test.

## Native Ubuntu 26.04.1 Wayland follow-up — 2026-10-19

The native DEB was exercised on the user's Wayland desktop with isolated test
profiles. [The detailed report](validation/ubuntu-26.04-wayland.json) records
117 workspace checks, a real H.264/AAC recording with decoded image and audible
test tones, portal screen capture at 2560 × 1440, and simultaneous local
1280 × 720 / 1080 × 1920 streams including reconnect and shutdown.
An additional browser-source run passed 118 checks; its offline HTML fixture was
verified in a [decoded recording frame](validation/ubuntu-26.04-browser-frame.png).
All nine settings pages, source properties and filters opened under native Qt
6.10.2; [video settings](validation/ubuntu-26.04-settings-video.png) show the form
rendering. These are Qt widget renders, not compositor screenshots.

A compact auto-configuration wizard reproduced clipped text (a 2-pixel text area
for an 18-pixel font). The fix gives inputs a minimum content height and prevents
the wizard from shrinking below its natural page size. The
[regression probe and instructions](../test/README-native-wayland.md) cover text
height and overlapping fields. The installed `0.2.0-2+ubuntu26.04` package also passes all 118 workspace/browser
checks, produces a verified H.264/AAC recording, and passes the wizard probe in English, German
and at font scale 14. Compare [before](validation/ubuntu-26.04-wizard-before.png)
with [after](validation/ubuntu-26.04-wizard-en.png),
[German](validation/ubuntu-26.04-wizard-de.png) and
[larger text](validation/ubuntu-26.04-wizard-large.png).

Streaming-account authorization and title updates were checked with mocked
providers only. No public broadcast, physical camera/microphone recording or
hardware-encoder benchmark was performed. Optional NVIDIA/DeckLink drivers,
VLC runtime and the v4l2loopback virtual-camera module are absent on this host.

## Historical results

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

## Startup diagnostics — revision 6 work

The startup audit of the installed Ubuntu 26.04 revision 5 package found a
nonexistent legacy scene file being migrated on fresh profiles, missing DeckLink
UI locale resources, and optional Linux runtimes being reported as load errors.
Revision 6 checks that the legacy file exists, includes the small DeckLink UI
catalogues (English fallback where untranslated), and distinguishes an absent
optional runtime from an installed but broken library. NVENC unload now checks
whether its encoder resources were initialized. An unavailable v4l2loopback
capability is informational; camera runtime failures retain their severity.
Application identity is assigned before icon and portal-related initialization.

`test-optional-library.sh` exercises absence, successful loading, invalid ELF and
missing dependent libraries. The tests also cover German Germany and German Switzerland locales. Only absence
of the exact requested SONAME becomes informational, using the localized ENOENT
suffix rather than an English-only loader message. Other loader failures remain errors. The existing Qt diagnostic
boundary remains unchanged: QTBUG-141475 affects real text-input focus handling,
so its non-null mismatch warning must not be hidden as a shutdown false positive.

The native shutdown report now includes every startup warning and error, plus a
`startup_clean` flag. Passing shutdown checks alone does not prove a clean start.
Installed revision 6 results are pending; no warning-free startup claim is made.
