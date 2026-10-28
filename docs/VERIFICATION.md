# Verification and release gates

> Historical test archive: dated screenshots below document the version and locale
> tested at that time, including German localization tests. They are not current
> product previews. For the current English GUI, see the [workspace guide](WORKSPACE.md)
> and [preview provenance](images/current/manifest.json).

Version 0.7.2 retains the classic workspace and adds Creator tools, asynchronous camera processing and Ubuntu 26.10 packages. See the [Creator tools validation](validation/creator-tools-2026-10-28.md) and [classic workspace record](validation/classic-workspace-0.7.2.md). The classic8 design was installed and approved locally. The final complete Linux package set is validated by the release workflow. See also the [upload and installed-app validation](validation/upload-status-2026-10-28.md).

Version 0.7.1 harmonizes the interface on the OBS 33.0.0 foundation. See the [design validation record](validation/design-consistency-0.7.1.md) for package-level layout, recording and shutdown checks.

Version 0.7.0 is based on OBS Studio 33.0.0. Its [integration record](validation/obs33-integration.md) separates new engine tests from historical results. All seven candidate package targets and the AMD add-on migration checks passed; historical results alone are not used to validate this major update.

## OBS 33 foundation 0.7.0 — 2026-10-25

Native Ubuntu 26.04.1 Wayland candidates passed 210 workspace assertions,
camera pixel/Inspector checks, 30-second GPU and CPU-fallback recordings,
all ten startup/shutdown scenarios, saved synthetic-profile migration and
local landscape/portrait output with reconnect. Offline browser output was
visually confirmed in a decoded recording. AppImage and Flatpak candidates
also passed graphical startup and camera checks; the Flatpak workspace
recording passed after correcting reserved XDG-variable handling in the
private test launcher.

The existing host RADV diagnostic also reproduces on 0.6.4. Flatpak reports
absent optional AJA devices on this machine. Those messages are documented,
not hidden. Qt 6.11.1's confirmed destroyed-surface leave diagnostic is
classified at debug level, with real focus mismatches preserved. Fresh live
provider authorization, another Facecam movement acceptance, AJA operation
and full GUI testing on every target distribution are not claimed.


## Security and camera GPU processing 0.6.3 — 2026-10-25

All eight Linux candidate artifacts passed CI. The exact Ubuntu 26.04 package
passed native Wayland camera/Inspector controls, legacy CPU selection preservation,
211 workspace checks, nine shutdown scenarios, saved Twitch/YouTube connections,
category searches, release queries and local landscape/portrait RTMP output.
The RX 9070 XT GPU package replay produced 1,498 frames with no reported losses,
full decode and clean shutdown. The live movement test was accepted by the user.
Two render-lag frames in the separate short workspace smoke test remain recorded.
See [GPU and package evidence](validation/camera-gpu-0.6.3.md) and
[security validation](validation/core-security-0.6.3.md) for scope and limitations.

## Optional hand preservation 0.6.2 — 2026-10-25

The live prototype movement test was visually accepted: both hands remained identifiable and the image appeared fluid. The full recording decoded and shutdown was clean, but one render-lag event and one encoder-skip event remain recorded. The subsequent zero-event control had no person in the inspected frames. See [test evidence](experiments/hand-preservation/results.json). The Ubuntu 26.04 candidate package passed native Wayland GUI regressions and a 40.021-second private motion replay with no reported frame-loss events, full decode and clean shutdown. See [package validation](validation/hand-preservation-0.6.2.md). The final release-package replay also decoded and shut down cleanly, but recorded one render-lag frame and one encoder-skipped frame. Lossless performance is not guaranteed.

## Native chat and camera backgrounds 0.6.0 — 2026-10-24

The final Ubuntu 26.04 package was installed on Ubuntu 26.04.1, upgrading only
Penguin Studio. Installed-file verification, all six camera background controls,
GPU pixel tests, undo/restore and both native chat tabs passed under Wayland.
A 20.033-second synthetic H.264/AAC recording switched between blur and a custom
image while retaining rounded corners; it fully decoded with zero reported
frame drops. Start and normal shutdown emitted no warning/error diagnostics or
reported memory leaks.

Moderation and participant requests were verified using in-memory API fixtures;
no real viewers were moderated or messages sent. Viewer counts and chat-participant
lists are distinct. See [detailed results](validation/chat-camera-0.6.0.md) and
[machine-readable evidence](validation/chat-camera-0.6.0.json).

## Current-frame camera matting 0.5.3

Real Facecam head and hand movements on Ubuntu 26.04.1 under native Wayland
reproduced bright background strips around moving contours in 0.5.2. A comparison
recording without matting also showed substantial camera motion blur. Replaying
the same local input with current-frame inference reduced the exposed background
strips, without changing the model, threshold, smoothing or camera controls.

The corrected component passed the Inspector, shape, chroma-key, filter-pair
lifecycle and shutdown tests. The new first-render alpha assertion fails on the
installed 0.5.2 component and passes with the correction. It checks a freshly
created filter before a video tick can prepare a mask.

A corrected live Facecam recording and a replay comparison each produced a fully
decodable, approximately 20-second 720p/30 H.264/AAC file with rounded corners.
Each reported one render/encoding stall (about 0.2%); the stricter zero-stall
recording check therefore did **not** pass. Both closed with exit code 0, no
warnings/errors and zero reported memory leaks. Fast, blurred hands can still
have incomplete edges. These local component results do not certify a release
package or guarantee perfect matting. Personal footage is not in this repository.

The final 0.5.3 Ubuntu 26.04 package subsequently passed the same recording checks
without reported rendering/encoding stalls. The installed-package GUI and replay
checks passed under Wayland with normal shutdown and zero reported memory leaks.
The tester accepted the corrected live result as significantly better and usable.
All seven downloads passed [release build 40023377072](https://github.com/SigmaCodez/PenguinStudio/actions/runs/40023377072).

## Camera appearance 0.5.0 — 2026-10-23

The new camera module passed native Wayland rendering tests using generated
color fixtures and a public test portrait in an isolated profile. Pixel checks
verified circles versus ovals, rounded corners, green and blue keying, local AI
masking, and restored unfiltered output. Source settings survived a restart.
Closing with automatic removal active produced no warnings/errors or reported
memory leaks. A deliberately missing model produced one actionable error while
preserving the original video and shape effect; shutdown remained clean.

The exact 0.5.0 Ubuntu 26.04 DEB also passed in a temporary filesystem namespace
at its normal install paths, without replacing the installed application. Tests
covered the Inspector, existing-filter default Undo/Redo, new-filter creation
and removal, corner-radius Undo/Redo, and synthetic asynchronous camera frames.
All native runs closed with zero warnings/errors and zero reported memory leaks.
Both Ubuntu CI jobs passed the installed-package camera tests. Their headless
Xvfb environment explicitly permits only the recorded missing portal-settings,
DRM render-node and EWMH diagnostics; these exceptions do not apply on the desktop.
The Linux package jobs passed for both DEBs, RPM, Flatpak, Snap, AppImage and the
Arch recipe.

The released Ubuntu 26.04 package was then installed on Ubuntu 26.04.1, upgrading
only Penguin Studio from 0.4.4 to 0.5.0. Package-file verification passed. Fresh
native Wayland tests of the installed application passed the Inspector and pixel
checks for shapes, local AI removal, green/blue keying, disabling, Undo/Redo and
synthetic asynchronous camera frames. A separate restart restored the saved
settings and closed normally with AI removal active. All three runs exited with
code 0, no warnings/errors and zero reported memory leaks. Personal profiles and
accounts were not used.
A physical webcam, moving hair, multiple simultaneous cameras and other desktop
environments have not yet been validated for this feature.
See [camera-effects-0.5.0.json](validation/camera-effects-0.5.0.json) and the
[camera guide](CAMERA-EFFECTS.md).

## AAC encoder shutdown correction — 2026-10-22

The FFmpeg audio encoder now enters draining mode and retrieves delayed packets
before releasing its codec context. OBS has detached the output callbacks before
this destructor runs, so these cleanup packets are released locally. The change
does not extend output stop timestamps, route packets into stopped outputs or
filter the FFmpeg log.

`test/penguin-audio-drain.c` exercises real AAC contexts with no frames, a single
frame, 300 frames and pending packets, plus repeated cleanup and PCM encoding.
The local test passed with FFmpeg 8.0.1, including AddressSanitizer,
UndefinedBehaviorSanitizer and LeakSanitizer. GitHub Actions also runs this test
against its Ubuntu 24.04 FFmpeg libraries.

The installed 0.4.3 baseline reproduced the warning on all three successive
native Wayland recordings with synthetic audio. The files decoded successfully;
their audio/video end times differed by at most 8 ms. This is a resource-lifecycle
correction, not a claim that previously recorded files were unusable.

The corrected Ubuntu 26.04 package from
[run 39862855871](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39862855871)
was installed and tested on Ubuntu 26.04.1 on 2026-10-23. Three successive native
Wayland recordings used AMD VA-API H.264 and FFmpeg AAC with a synthetic 440 Hz
tone. All three stopped normally, contained audible audio and fully decoded
without errors. Audio/video end times differed by at most 12 ms. The old AAC
warning appeared three times in the 0.4.3 baseline and zero times in 0.4.4.
Application shutdown and a separate restart/close test exited with code 0, no
warnings/errors and zero reported memory leaks. Package-file verification passed;
installation upgraded only Penguin Studio. No personal profile, account, external
stream or desktop capture was used.

The installed package is from the additional Ubuntu-only CI run of the same
source commit; the complete release workflow is separate. Exact package hash and
results: [aac-shutdown-0.4.4.json](validation/aac-shutdown-0.4.4.json).

## AppImage and Arch packaging 0.4.3 — 2026-10-22

All six binary package builds and the Arch recipe build/install checks passed in
[run 39684701505](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39684701505).
The complete release uses source `6529d27909072ac136240a811eebbc7991f36bcf`.
AppImage GUI startup/close is checked in CI using Xvfb; Arch checks include shared
library availability, launcher execution and desktop-entry validation.

The released Ubuntu 26.04 DEB was hash-verified and installed on Ubuntu 26.04.1,
upgrading only Penguin Studio from 0.4.2 to 0.4.3. Package-file verification passed.
Native Wayland startup and normal shutdown passed with exit code 0, no warnings
or errors and zero reported memory leaks. A separate synthetic recording selected
AMD VA-API (`ffmpeg_vaapi_tex`), produced 221 frames and stopped normally.

The hash-verified release AppImage was extracted and tested under native Wayland
with host OBS resources/libraries hidden. Its visible main window, hardware
recording (221 frames), normal recording stop and shutdown passed, with exit code
0 and zero reported leaks. This validates the shipped layout, not a locally
patched replacement. Earlier missing Wayland integration plugins, incompatible
bundled VA-API libraries and the Qt 6.4 queued-thumbnail metatype warning were
corrected before the final release build.

Both synthetic recording tests still emitted FFmpeg's AAC diagnostic:
`2 frames left in the queue on closing`. Therefore recording shutdown is not
claimed to be completely diagnostic-free. The no-recording native lifecycle test
was clean. No account authorization, external streaming or desktop capture was
performed in these packaging checks. NVIDIA hardware and full GUI/device coverage
on Arch and other distributions remain untested. The Arch recipe is downloadable
on GitHub; an actual AUR listing requires a maintainer account.

Machine-readable results and package hashes:
[portable-packages-0.4.3.json](validation/portable-packages-0.4.3.json).

## Hardware encoding 0.4.2 — 2026-10-21

The source at `83c1639912f09298754ed8c01513ddaa4921c7ed` adds Linux VA-API to
simple-output defaults, the auto-configuration wizard, encoder menus and new
multistream destinations. Registered NVIDIA NVENC encoders retain priority.
Explicit user selections are preserved. VA-API capability checks require CBR
and CQP support on the default device/profile; unsupported simple-mode selections
fall back to software x264. All Linux package recipes explicitly enable NVENC.

A checksum-verified Ubuntu 26.04 CI package was tested under native Wayland/EGL
on a Radeon RX 9070 XT with an isolated profile. New profile defaults selected
VA-API for streaming and recording, both simple-output menus contained and selected
it, the auto-configuration wizard preferred hardware, and a new multistream
destination selected `ffmpeg_vaapi_tex`. A short synthetic recording used that
encoder and stopped normally. The application exited with code 0, no logged
warnings/errors and zero reported memory leaks.

Separate CBR/CQP hardware encoding and simultaneous landscape/portrait loopback
streaming passed, including crop/fit, restart, reconnect and active shutdown.
No external stream, desktop capture or live account was used. Regression tests
cover rate-control availability, disabled entries and property callbacks; output
safety and wizard lifecycle tests also passed. These results do not establish
NVIDIA runtime compatibility or support for every GPU/driver combination.

The same Ubuntu package was then installed successfully, upgrading only Penguin
Studio from 0.4.1 to 0.4.2 with no additions or removals. Package-file and dependency
checks passed. A repeat test of the installed application passed every hardware
selection and recording check and exited cleanly, again with no logged warnings
or errors and zero reported leaks. A subsequent restart of the same test profile also exited without warnings or
errors and reported zero leaks. An earlier missing-source warning was traced to
a private synthetic test fixture; making that fixture serializable resolved it
without changing application code. Personal profile settings were unchanged.

[Release 0.4.2](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.4.2)
contains Ubuntu 24.04 and 26.04 DEBs, Fedora RPM, Flatpak and Snap packages. All
five CI builds and publication passed. The installed Ubuntu package matches the
published SHA-256 checksum.

See [hardware configuration](HARDWARE-ENCODING.md) and
[test evidence](validation/hardware-encoding-0.4.2.json).

## Released 0.4.1 — 2026-10-21

[Release 0.4.1](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.4.1)
provides all five Linux packages with publisher-configured YouTube browser sign-in.
The normal flow offers **Connect with YouTube**, with application credentials hidden
under advanced settings. Existing saved accounts retain their client identity.
Each package job verified the publisher configuration in its compiled platform
module. The initial Flatpak check used the cache directory instead of the exported
application directory; the corrected complete run passed at commit
`43de468de1193f7c7887543e7b025d6812dcc037`.

The downloaded Ubuntu 26.04 DEB matches the published checksum and source manifest.
Its bundled Google client matches the previously live-tested local account. APT
preflight upgrades Penguin Studio only, without adding or removing packages.
The local installation was not changed in this release task. Automated tests cover
the browser button, hidden/advanced settings and existing OAuth flows; the installed
0.4.1 GUI has not yet received a new live-provider test. Google testing/production
and verification restrictions still apply to the shared app.

Evidence: [release and package checks](validation/linux-release-0.4.1.json).

## Released and installed 0.4.0 — 2026-10-20

[Release 0.4.0](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.4.0)
contains Ubuntu 24.04 and 26.04 DEBs, Fedora RPM, Flatpak and Snap, plus SHA-256
checksums and the exact source manifest. All five package jobs and the publication
job passed at commit `bb3ff5120e9d75cd66f0d090f728fc7accdbcb21`.
Two Flatpak configuration failures were fixed: disable QtKeychain's optional
upstream tests (the packaged Qt has no Qt6Test), and build the library explicitly
as Release so Penguin Studio can resolve its imported CMake target.

The published Ubuntu 26.04 package was checked against the release manifest and
installed on Ubuntu 26.04.1, upgrading 0.3.1 to 0.4.0. Only `libqt6keychain1` was
added; no packages were removed. Package-file verification passed.

The installed application, without development-plugin overlays, was tested on
GNOME Wayland with an isolated profile. Real Twitch browser authorization and
chat connection succeeded. Sign-in was saved in the system keyring and restored
on two subsequent starts without another authorization. Game search returned
20 suggestions including Minecraft. A synthetic popup attempt lacked a Wayland
input serial and logged a grabbing-popup warning; the test was repeated using
real user input. The user confirmed the visible suggestions, and the popup was
visually inspected. The initial login and real-input runs exited with code 0,
no logged warnings/errors, and zero OBS-reported memory leaks. The synthetic
attempt also exited normally; no application warning filter was introduced.

No chat messages or stream metadata changes were sent. The isolated account was
disconnected and its keyring entry removed after testing. Normal user settings
were unchanged. A subsequent user-operated YouTube test confirmed browser authorization, categories,
secure saving and automatic reconnection after restart. Both application logs showed
Wayland, normal shutdown and zero reported leaks; native UI results were user-confirmed.
See [YouTube test evidence](validation/youtube-wayland-0.4.0.json).

Evidence: [package build results and manifest](validation/linux-release-0.4.0.json),
[installed Wayland checks](validation/accounts-installed-0.4.0.json), and
[visible game suggestions](validation/accounts-search-0.4.0.png).

## Searchable categories and saved accounts — 2026-10-20 (pre-release checks)

On Ubuntu 26.04.1 / GNOME Wayland, development builds of both Penguin plugins
were overlaid onto the installed 0.3.1 application using an isolated configuration.
The system installation and the user's normal account configuration were not changed.

- Real Twitch device authorization succeeded after user approval. Twitch chat connected,
  and the refresh credential was saved in the OS keyring.
- After closing and restarting the application, Twitch and chat reconnected without
  opening another authorization page. Typing `Minecraft` returned 20 category suggestions,
  including named games with IDs. No chat message or metadata update was sent.
- Both application runs exited with status 0, no logged warnings/errors and zero
  OBS-reported memory leaks. The isolated test credential was removed afterward.
- The optional wizard account dialog was inspected under Wayland. A hidden reparented
  page was fixed; fields now render and both provider tabs return when the dialog closes.
- Automated Qt tests cover stale search responses, selection validation, refresh-token
  rotation and restoration, disconnect versus pending reads, keyring failure reporting,
  YouTube assignable categories and metadata preservation. Wizard tests cover optional
  sign-in on the welcome page, cancellation, parent destruction and shutdown.
- Real Secret Service write/read/delete passed with plaintext fallback disabled.
  Native chat and update-policy regressions passed as well.

YouTube authorization was tested with simulated provider responses, not a real Google
account. YouTube's API exposes video categories, not individual Gaming title search.
New strings are localized in English and German; the other six Penguin translations
currently use English fallback text for these additions. Sandboxed keyring access and
new package dependencies still need clean-package runtime coverage.

## Real in-app upgrade 0.3.0 → 0.3.1 — 2026-10-20

The installed Ubuntu 26.04.1 application completed a genuine in-app upgrade.
Its Help updater offered published version 0.3.1, downloaded the matching Ubuntu
26.04 DEB, passed its production size and SHA-256 validation, and invoked
`pkexec /usr/bin/apt-get install -y --no-remove` itself. The observer clicked the
real installation button; no separate installer command or replacement updater
implementation was used. The package manager exited normally with code 0 and
the updater displayed its installation-success message.

The system package log confirms `0.3.0-1+ubuntu26.04` → `0.3.1-1+ubuntu26.04`.
Installed-file verification passed. After closing the old process and reopening
the app with the same isolated test profile, the launcher and application log
reported 0.3.1. Both native docks were present, and a new update lookup reported
the current version with installation disabled. Both application runs exited
with code 0, no logged warnings/errors, and zero reported OBS memory leaks.
The success and post-restart dialogs were visually inspected. No updater defect
was found in this run. The restart was performed explicitly after the app's
instruction; automatic restart is not implemented.

The maintenance release retains 0.3.0 functionality and supplies all five Linux
packages. This test establishes the Ubuntu upgrade path, not live upgrades on
other package formats. Twitch authentication was not repeated here; see the
installed 0.3.0 test below. Personal profiles were unchanged.

Evidence: [upgrade results](validation/in-app-upgrade-0.3.1.json),
[success dialog](validation/in-app-upgrade-installed.png),
[release package manifest](validation/linux-release-0.3.1.json).

## Installed 0.3.0 validation — 2026-10-20

[Version 0.3.0](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.3.0)
is published as a development preview with all five Linux packages. All package
jobs and release publication passed. Uploaded sizes and SHA-256 digests match
the release manifest. The Ubuntu 26.04 DEB was downloaded, hashed and installed
on Ubuntu 26.04.1 after system administrator confirmation. APT upgraded only
Penguin Studio and added `libqt6websockets6`, removed nothing, and exited with
code 0. Installed-file verification passed; the launcher reports version 0.3.0.

The installed application passed a native GNOME Wayland test with an isolated
profile and a temporary Qt observer/controller. The user completed real Twitch
authorization in the system browser. Account and native chat connections
succeeded, both docks were present, and loading stream details populated the
current title. The actual Help update action reported the installed version as
current and disabled installation appropriately. No chat message was sent and
no stream metadata was changed.

Closing the connected application exited with code 0, no Penguin warnings or
errors, and zero reported OBS memory leaks. Firefox/Snap emitted separate browser
and mount-namespace diagnostics to inherited stderr. No application implementation
overlays were used. This verifies installation via the system package manager
and update lookup; a subsequent in-app version upgrade was not exercised.
Other distributions/compositors still require live GUI validation. The test
instance was closed; Twitch authorization remains session-only.

Evidence: [release assets](validation/linux-release-0.3.0.json),
[installed Wayland results](validation/installed-0.3.0-wayland.json).

## Native Twitch chat and Linux updates — unreleased, 2026-10-20

The development module connected to a real Twitch account after the user
completed device authorization in Firefox. Both account validation and all four
native chat EventSub subscriptions succeeded under GNOME Wayland. No chat message
was sent and no live stream title/category was changed. Closing this connected
test instance exited successfully, without logged warnings/errors or reported
OBS memory leaks. The test profile and tokens were isolated from personal app
configuration; tokens remained in memory only.

The actual updater dialog was exercised against GitHub releases using a simulated
older compiled version (`0.2.0`) to offer the real `0.2.1` Ubuntu 26.04 asset.
English and German runs displayed the update and enabled installation; no package
manager was executed. A real-data regression fixed GitHub's `%2B` encoding of the
plus sign in Ubuntu package names. A visual check also found and fixed clipping
of asynchronously loaded, wrapped update text. Both corrected language runs
closed with no logged warnings/errors and zero reported OBS memory leaks.

Offline tests exercise the production updater's failed downloads, size limits,
checksum rejection and active-output exclusion. Additional tests cover Twitch
polling/refresh/revocation, loading metadata without overwriting user edits,
chat moderation events, history bounds and reconnect URL validation. Native
GUI checks used development overlays over the verified 0.2.1 package; they are
not an installation test or a new release. Other package formats need their own
GUI and installation validation. Strict Snap in-app installation remains limited
by its package manager; see [update support](UPDATES.md).

The final implementation (`31e1ae7898cbed51aef1b822e38d5e6efaf30791`) passed
[all five package jobs](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39463554436):
Ubuntu 24.04/26.04 DEB, Fedora 44 RPM, Flatpak and Snap. Native and Snap jobs also
passed their CI installation and launcher checks. Release publication was skipped
because the version remains unchanged; these are development artifacts.

The verified Ubuntu 26.04 CI package then passed a native Wayland GUI run without
application implementation overlays. Both native docks were present and the actual
Help update action opened the updater, correctly reporting the current version
with installation disabled. Startup and shutdown produced no logged warnings/errors,
exit code was 0, and reported OBS memory leaks were 0. This used an extracted
package, its additional QtWebSockets dependency, a Qt observer and an isolated
profile; it did not modify the installed application or test a package upgrade.

Evidence: [sanitized test report](validation/updates-twitch-wayland.json),
[German update dialog](validation/update-dialog-de.png),
[native chat dock before sign-in](validation/twitch-chat-native.png).

## UI branding audit — unreleased source, 2026-10-20

Source `d05aaf0f9863bbf477d8129a131ce9ddee5d0c0f` passed the Ubuntu 26.04
build, regression, package installation and CLI checks in
[the test build](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39454064392).
Publication was disabled. This is an unreleased source test, not a replacement
for the published or system-installed 0.2.1 package.

The verified test DEB passed two native GNOME Wayland runs on Ubuntu 26.04.1,
using English and German isolated profiles. About, Authors, License, Remux and
the Safe Mode restart confirmation were exercised. Project/support labels and
the contribution link point to Penguin Studio; the full displayed author and
license texts match the installed baseline exactly. Dialog renders were also
visually inspected. Both runs exited with code 0, no logged warnings or errors,
and zero reported OBS memory leaks.

The extracted package ran at its normal paths in a private filesystem view,
with a temporary Qt observer/controller and no focus shim or warning filter.
The system installation and personal profiles were unchanged. Static checks
covered all 77 frontend locales and plugin locale patches, including preserved
keys and substitution placeholders. Actual OBS import formats, upstream credits
and external service identities retain their correct names. Streaming and other
package formats were not retested for this text-only update.

Evidence: [UI branding results and package digests](validation/ui-branding.json).

## Installed 0.2.1 validation — 2026-10-20

The published Ubuntu 26.04 package `0.2.1-1+ubuntu26.04` is now installed on
Ubuntu 26.04.1. Its SHA-256 matches the release manifest. APT upgraded only
Penguin Studio, removed no packages, and exited successfully.

The system-installed executable passed all **ten native GNOME Wayland runs**:
normal exit, multiple instances, open initial wizard, connection-wizard
cancellation, focused connection input at shutdown, auto-configuration
cancellation, Statistics, profile restart, SIGTERM, and an additional focused
wizard with German application text under `de_CH.UTF-8`. All runs had zero
startup warnings/errors, zero shutdown warnings/errors, zero late frontend
callback removals, no remaining crash sentinels, and zero reported OBS memory
leaks. Initial setup visibility and native input focus were verified. Tests used
isolated profiles and an observer-only helper, with no filesystem overlays or
focus shim. Personal configuration was not modified.

Evidence: [installed-package report](validation/ubuntu-26.04-installed-0.2.1.json).
The pre-publication validation below remains as historical evidence.

## Wayland startup focus correction — 0.2.1, 2026-10-20

[Penguin Studio 0.2.1](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.2.1) fixes the first-run hardware
wizard opening before main-window activation completes. The connection assistant
also waits for pending hardware setup and an active main window. No new warning
filter or input-method override is used.

The unmodified Ubuntu 26.04 executable and libraries extracted from the verified
release DEB, mounted at their normal paths in a private filesystem view,
passed **nine native GNOME Wayland scenarios** on Ubuntu 26.04.1,
Qt 6.10.2 and AMD Radeon RX 9070 XT. Every run exited with code 0, zero startup
warnings/errors, zero shutdown warnings/errors, zero late frontend callback
removals, no remaining crash sentinel, and zero reported OBS memory leaks.
An additional focused-wizard run with German application text and
`de_CH.UTF-8` also passed, bringing the total to **ten clean runs**.
The test verifies the initial hardware wizard appears, connection text fields
receive native keyboard focus, cancellation works, and the same profile reopens
cleanly. Tests used isolated profiles and an observer-only helper; this package
was not installed system-wide by this validation.

All five package jobs passed in [the build run](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39402886896).
The complete set was published using the repository release script after native
GUI validation. All seven release assets were verified against their sizes and
SHA-256 digests. The offscreen lifecycle fixture was updated to show and activate
its window, and now also verifies hidden/pending windows defer first-run setup.

Evidence: [native package results](validation/ubuntu-26.04-startup-focus-0.2.1.json)
and [release asset verification](validation/linux-release-0.2.1.json).
Other desktops, GPUs, live provider sessions, and local Snap/Flatpak GUI behavior
were not retested as part of this focus correction. Earlier results below are
historical and retain their original findings.

## Automated Linux release — 2026-10-20

[Penguin Studio 0.2.0](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.2.0)
is published as a development prerelease with five packages: Ubuntu 24.04 and
26.04 DEB, Fedora 44 RPM, Flatpak x86_64 and Snap core24 amd64. All package jobs
and the publication job passed in [the release run](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39394971754).
Native packages passed their regression executables, installation and CLI checks;
Snap passed installation and confined CLI startup; Flatpak passed build, export
and bundle generation. This release run did not repeat GUI or live-stream tests.

All seven uploaded assets (five packages, `SHA256SUMS`, `build-manifest.json`)
were verified after publication. Package sizes and GitHub-computed SHA-256
values match the manifest and checksum list. The two metadata files were also
downloaded and hashed. See [release verification](validation/linux-release-0.2.0.json).

The workflow was checked with actionlint. Release regression tests cover version
changes, ordinary pushes, pull requests, matching/mismatched tags, restricted
manual publication, version rollback, incomplete/duplicate/wrong-version/empty
package sets, and upload failure leaving a release unpublished. Future version
increases in `packaging/version.txt` on `main` automatically run the same pipeline.

## Startup diagnostics — 2026-10-20

Installed Ubuntu package `0.2.0-6+ubuntu26.04`, built from
`6ea4a2994fff010d3cdef93a6007a9435023b674`, was tested on GNOME Wayland,
Qt 6.10.2 and an AMD Radeon RX 9070 XT. All **eleven runs** exited with code 0,
zero startup errors, zero shutdown warnings/errors, zero late frontend callback
removals, no remaining crash sentinels, and zero reported OBS memory leaks.

The nine regular scenarios cover normal exit, multiple instances, initial setup,
connection-wizard cancellation and focused input, auto-configuration cancellation,
Statistics, reopening the same profile, and SIGTERM. Two additional runs verify
legacy scene migration with the collection name preserved, and German application
text under `de_CH.UTF-8`. The initial migration fixture had stale absolute paths
to an older isolated test profile; correcting those paths resolved its assertion.
No personal profile or streaming account was used.

**Startup is not warning-free:** nine runs each retained one non-null Qt Wayland
text-input focus warning (QTBUG-141475). Reopening the profile and the migrated
legacy profile had none. The report therefore explicitly records
`startup_clean: false` and `shutdown_clean: true`. No additional warning filter or
input-method override was shipped. NVIDIA and DeckLink hardware were unavailable
on this host, so their hardware paths remain unverified.

Evidence: [installed startup and shutdown report](validation/ubuntu-26.04-startup.json)
and [successful native build](https://github.com/SigmaCodez/PenguinStudio/actions/runs/39333338354).

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
The installed results below retain this distinction; no warning-free startup claim is made.

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
