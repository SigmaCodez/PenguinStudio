# Chat reliability — 0.7.5 local1

Date: 2026-11-04. Public version remains **0.7.5**. The user authorized these
three fixes, local tests and system-wide installation for review, explicitly
excluding publication.

## Changes

- Both chat request adapters validate successful responses for transport failure
  and valid JSON objects. Bodyless HTTP 204 succeeds; provider error codes and
  their diagnostic payloads remain available. A truncated YouTube send response
  keeps the user's text and reports uncertainty. No automatic retry of sends or
  moderation actions is added.
- A YouTube broadcast-list 401 is remembered. A changed access token retries the
  load; repeated delivery of the same expired token does not start a retry loop.
  Disconnect clears the recovery state.
- The chat options menu reloads the active mode: original website, integrated
  Twitch reconnect, or integrated YouTube broadcast reload. Weak references in
  module wiring avoid callbacks to destroyed chat widgets.

## Build provenance

Local package: `penguin-studio_0.7.5-1+ubuntu26.04.local1_amd64.deb`.
It is retained under `dist/local-review/0.7.5-local1/` (ignored build output).
Package SHA-256:
`07279658ac5c8ced8a2540ec0770b0e09ebee503849b28d220558981dd69a83a`.

The published Ubuntu 26.04 0.7.5 DEB was downloaded and checked against its
published SHA256SUMS. Only `usr/lib/x86_64-linux-gnu/obs-modules/core/penguin-platforms.so`
was rebuilt and replaced. All other payload files were checked against the base
package's MD5 manifest and remain unchanged. Debian version and MD5 metadata were
updated; product version and public previews were not changed.

This is a targeted module build, **not a full application/engine rebuild**. It
uses the release SDK supplied by the installed Penguin package, Qt 6.10.2 and
Qt Keychain from Ubuntu 26.04. The release module's bundled public desktop OAuth
application configuration was preserved in a temporary, mode-0600 include file,
without printing its values or accessing user tokens. That include was removed
after compilation. The module was built with C++17, `-O2 -g -shared -fPIC`, the
installed OBS headers/libraries, Qt Widgets/Network/WebSockets/WebEngineWidgets,
and `qt6keychain`. Source and module hashes are in the summary JSON.

The missing development and video-test dependencies were installed with normal
Ubuntu administrator confirmation. No passwordless configuration was added.
The distribution's conflicting `libobs-dev` package was not installed; the
Penguin release already supplies the required SDK.

## Automated and package checks

- Combined Twitch/YouTube fixtures pass, including invalid JSON, non-object JSON,
  connection loss after 200/204 headers, valid bodyless 204, token recovery,
  unchanged-token exclusion, preservation of an unconfirmed send, reload,
  request ownership, moderation, bounded history and stale-session cancellation.
- Existing Twitch history/reading-anchor suite passes.
- New `test/penguin-chat-reload.cpp` exercises the real menu while switching
  between original and integrated modes, without loading a website. It is also
  wired into the Linux CI workflow; CI itself was not dispatched.
- Address/undefined-behavior sanitizer execution passes. Qt leak detection was
  disabled (`ASAN_OPTIONS=detect_leaks=0`); this is not an ASan leak-test claim.
- Platform flow passes with in-memory provider replies and a real loopback-only
  OAuth callback. The first sandboxed invocation could not bind the callback;
  the host-loopback rerun passed. No real accounts or provider actions were used.
- Native Wayland workspace: **237 assertions passed**.
- Synthetic recording/local RTMP: **35 assertions passed**, including privacy,
  audio assistance and upload metrics. Both the recording and received stream
  fully decode with ffmpeg.
- Native preview/WebEngine: **8 assertions passed**, with **1,143 draw callbacks**.
  The recording fully decodes; the 15-second frame was visually checked and shows
  the expected synthetic red source after browser creation.
- The final three GUI test processes exited with status 0 and reported zero OBS
  memory leaks. The workspace recording fully decodes too.
- Publication, branding and locale checks pass. Release/publication/portable
  Python suites: 13 tests pass. Website Python suite: 12 tests pass.
- All staged package MD5 entries pass. `apt-get --simulate --no-remove` proposes
  only the Penguin package upgrade, with no dependencies added or removed.

### Test harness corrections and limits

The first copied profiles retained absolute locations pointing at the workspace
test directory. Those paths were corrected. A subsequent Creator run still
contained prior synthetic scenes; duplicate source names caused three audio
assertions to fail. Its report is retained as `test-harness-stale-scenes.json`.
The final 35-assertion run starts with an empty scene collection and independent
paths and passes. These were test-profile issues, not hidden product fixes.

All profiles were isolated and English, with synthetic images/tones and no
camera device access. The local RTMP receiver ran in the test's isolated network
namespace. Internet access and account/keychain access were not used for GUI
tests. Runtime logs/artifacts are in `/tmp/penguin-chat-local1/`; portable result
JSON and fixture logs are retained beside this document.

Qt widget-render captures under Wayland do not correctly capture the native video
surface (the workspace capture shows artifacts in that region). They are not
public previews or proof of native video correctness; decoded video and the
dedicated draw-callback checks provide that evidence. Expected offscreen Qt
warnings, blocked network-update diagnostics, host RADV and WebEngine sandbox
OOM-adjustment diagnostics appeared. No claim of warning-free execution is made.

Real provider sends, bans/timeouts, all Linux package formats and a full clean
application rebuild were not tested in this task. Website JavaScript/build tests
were not run (Node is unavailable; website code was not changed).

## Installation

The system-wide local DEB was installed through `pkexec apt-get --no-remove` with
normal administrator confirmation. `dpkg-query` reports
`0.7.5-1+ubuntu26.04.local1`; `dpkg --verify penguin-studio` is clean and the installed
platform module is byte-identical to the tested module. The installed application
passed all 237 Wayland workspace assertions, exited with status 0 and reported
zero OBS memory leaks. Its synthetic recording fully decoded.

No commit, push, release tag, GitHub release, website deployment or public-version
increment was performed. The next step after installation verification is user
review of the local build. Publication still requires explicit approval.
