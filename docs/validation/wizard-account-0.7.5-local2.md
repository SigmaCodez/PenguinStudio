# Wizard account setup — 0.7.5 local2

Date: 2026-11-04. Public product version remains **0.7.5**. Local review only;
no publication, release tag, push or public version increment.

## Behavior

The real Auto-Configuration Wizard Stream Information page defaults to account
sign-in for Twitch and YouTube. **Use Stream Key** remains an optional alternative.
The existing platform account controls open from this page. **Use this account**
resolves streaming access after browser authorization; the user need not copy a key.
Service changes and manual fallback clear the wizard's previous account selection.
Request lifetime and session generation prevent stale/cancelled replies from
applying credentials. Keys are not stored in global action properties or shown
on the connected-account page. Standard wizard/profile storage still applies.

Twitch requests `channel:read:stream_key`; older grants may need **Reconnect**.
YouTube uses the selected existing encoder broadcast and its bound stream's
RTMPS ingestion settings. If no bound stream exists, setup explains that the
user must configure one in YouTube Studio or use manual stream-key setup.
No broadcasts are created, bound or transitioned. YouTube account setup disables
the bandwidth test to avoid transmitting test video to the selected broadcast.
Neither provider starts an output merely by connecting an account.

API references: [Twitch stream key](https://dev.twitch.tv/docs/api/reference#get-stream-key),
[YouTube broadcasts](https://developers.google.com/youtube/data/live/reference/rest/v3/liveBroadcasts),
[YouTube streams](https://developers.google.com/youtube/data/live/reference/rest/v3/liveStreams).

## Build and package

Package: `penguin-studio_0.7.5-1+ubuntu26.04.local2_amd64.deb`, retained with
SHA256SUMS in `dist/local-review/0.7.5-local2/` (ignored build output).
SHA-256: `529ed5dd9d2753678fbc8a7ce1253af98b02f41931941c6fedbb20716048cf56`.

CMake/Ninja built `obs-studio` and `penguin-platforms` from this checkout in a
staged source tree, with browser/CEF, Wayland and Rust processing enabled,
RelWithDebInfo, OBS version override 33.0.0 and local package release metadata.
The GUI test module also builds through the new `penguin-wizard-account` target
with `PENGUIN_BUILD_TESTS=ON`. Build logs/runtime artifacts are under
`/tmp/penguin-wizard-local2/`. Required development packages were installed through
normal Ubuntu administrator confirmation.

The DEB inherits the verified published 0.7.5 package via local1, replacing
`usr/bin/obs` and `penguin-platforms.so` with stripped staged binaries and updating
Debian version/md5 metadata. Other runtime components remain those of the release;
this is not a rebuild of every packaged component. The publisher's bundled public
desktop OAuth configuration was preserved privately from the installed module;
no user tokens were read. The temporary include was removed after compilation.

## Validation

- Platform fixtures pass, including OAuth/session behavior and new stream-access
  cases: Twitch success/403/reconnect/cancellation; YouTube unbound broadcast,
  rejected insecure ingestion URL, valid bound RTMPS stream and read-only requests.
- The same fixture passes ASan/UBSan with Qt leak detection disabled.
- Real Wayland wizard with synthetic account bridge: **16 assertions pass**,
  covering account-first controls, disabled Next without credentials, manual
  fallback, provider selection, clearing stale keys, account completion and
  disabled YouTube bandwidth testing. No outputs start during setup.
- Wayland workspace: **237 assertions pass**.
- Synthetic recording and isolated local RTMP: **35 assertions pass**.
  Both recording and received stream fully decode, as does workspace recording.
- Native preview/WebEngine: **8 assertions pass**; its recording fully decodes.
- All four final GUI runs exit normally and report zero OBS memory leaks.
- Publication, branding and locale checks pass. Public previews/version remain
  unchanged. All three new local wizard images were visually inspected and show
  the real English UI with synthetic data, separately under
  `docs/images/local-review/wizard-account-0.7.5-local2/`.

Fixture logs, source hashes and portable GUI result JSON are retained alongside
this document. The GUI bridge test substitutes provider completion; provider API
behavior is separately covered by fixtures. Real Twitch/Google consent and live
provider streaming remain user review tasks, not claimed automated successes.

### Test-harness recovery

The first Creator test profile lacked its configured recording directory. The
resulting dialog blocked the test until timeout. Its unclean test shutdown left
a sentinel and the next test launch showed the recovery warning reported by the
user. Only the isolated `/tmp/penguin-wizard-local2/creator-loopback` profile was
cleaned. The corrected run starts with empty scenes and a valid recording path,
passes all 35 checks and exits normally with zero reported OBS memory leaks.
This warning was not evidence of an account-setup crash. The user's profile was
not changed. `coredumpctl` is unavailable on this host.

All test sessions use isolated English profiles, synthetic media, no camera
access, no user keychain and an isolated network namespace. Live service calls
are mocked. The local receiver runs only inside that namespace.

## Installation verification

Installed system-wide with `pkexec apt-get --no-remove` through normal Ubuntu
administrator authorization. `dpkg-query` reports `0.7.5-1+ubuntu26.04.local2`;
`dpkg --verify penguin-studio` is clean. Installed frontend and platform module
are byte-identical to the staged, tested files. The installed application passed
all **237 workspace assertions**, exited normally, reported zero OBS memory
leaks and produced a fully decodable synthetic recording. The installation
simulation and actual transaction updated only Penguin Studio.

No public release/version/tag/push or website deployment occurred. Next step:
user reviews real Twitch/YouTube sign-in in the installed build. Publication
requires separate explicit approval after that review.
