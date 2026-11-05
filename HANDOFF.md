# Penguin Studio — maintainer handoff

Updated: 2026-11-05 (Europe/Zurich).

## Scope and evidence

This handoff was created after reading the current source, selected working-tree
diffs, build configuration and release/validation documentation. It is an
orientation record, not a full code audit or a new runtime certification.
The initial request was to inspect the code and create this file. The user then
explicitly authorized fixing the three reviewed chat bugs, testing them and
installing a local build for review, with no publication.

## Latest status — 0.7.6 published

The user explicitly approved publication on 2026-11-05 after local2 installation
and opening the app for review. Release **0.7.6** is now public as a development
prerelease: https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.7.6.
Source/tag commit: `c8842c9c1e9334076b139f54d35b88e67fbea2dc`.
All eleven jobs in workflow **41990529592** passed. The release contains nine
packages/recipe archives plus SHA256SUMS and build-manifest.json.

The exact Ubuntu release DEB passed 237 workspace, 35 recording/local-RTMP,
8 preview/WebEngine and 16 wizard assertions under Wayland. All four sessions
closed normally with zero OBS memory leaks; all generated recordings and the
received stream fully decode. The public Ubuntu download matches that tested
CI artifact. See `docs/validation/release-0.7.6.md` for final download checks.
Real provider consent or live streaming success is not inferred from approval.

System-wide installation was requested through normal `pkexec apt-get` and is
awaiting the user's Ubuntu administrator confirmation. Last verified installed
version remains `0.7.5-1+ubuntu26.04.local2`. Do not claim installed 0.7.6 validation
until that transaction completes. The verified release DEB and manifests are
archived in `dist/releases/0.7.6/`. No passwordless rules or password storage.

Release work used `/tmp/penguin-release-076/repo`, based on GitHub main
`52865ccce337e5237ba038b146ef65c12b1bcd65`. The original checkout retains its older
branch/HEAD and unrelated website work; only reviewed changes and release
publication files were synchronized back. Do not overwrite the original dirty
worktree or infer that its HEAD equals the released source. Temporary candidate
build objects were removed after successful validation to free tmpfs quota;
source, staged binaries, package archives and portable evidence remain.

## Reviewed work — wizard account setup local2

The user requested Twitch/YouTube sign-in directly on the auto-configuration
wizard's Stream Information page, keeping stream-key entry optional. Implemented
in local review build **0.7.5-1+ubuntu26.04.local2**; public version stays **0.7.5**.
The rebuilt frontend and platform module retain all local1 chat fixes.
System-wide local2 installation is complete: package verification is clean, both
installed binaries match the tested files, and all 237 installed workspace
assertions pass with clean shutdown and a fully decodable recording.

- Account-first controls open the existing native provider pane and browser login.
  **Use this account** retrieves streaming settings; **Use Stream Key** remains
  available. No output starts merely by connecting an account.
- Twitch requests stream-key read access; older grants may need **Reconnect**.
- YouTube requires an existing encoder broadcast with a bound stream configured
  in YouTube Studio. No broadcast creation/binding/transition is performed.
  Account-based YouTube setup disables bandwidth testing to avoid test video
  reaching that broadcast.
- A lifetime-bound shared request bridges the frontend and platform module.
  Service changes, cancellations and session changes discard stale results.
  Normal wizard/profile credential persistence still applies.
- New real-GUI regression module: `test/penguin-wizard-account.cpp`, CMake target
  `penguin-wizard-account`. Platform-flow fixtures cover stream-access responses.

Validation: 16 real wizard assertions, 237 workspace assertions, 35 synthetic
recording/local-RTMP assertions, 8 preview/WebEngine assertions; provider fixtures
and ASan/UBSan pass. Final GUI runs close normally with zero reported OBS leaks;
recordings and received stream fully decode. Publication/branding/locale checks
pass. Three real English local wizard captures were visually inspected, kept
separately from public previews. Real provider authorization remains user review.

The user saw a recovery dialog during testing: the first isolated Creator profile
lacked its recording directory, blocked and timed out. Its sentinel caused the
next isolated launch's warning. The corrected empty-scene test passes all 35
checks and shuts down cleanly. Only the temporary test profile was cleaned;
the user's profile was not changed.

See `docs/validation/wizard-account-0.7.5-local2.md` for provenance, limitations,
package checksum and installation status. Package retained in
`dist/local-review/0.7.5-local2/`. Publication of these reviewed changes was authorized on 2026-11-05.

## Previous work — chat reliability local1

Public product version remains **0.7.5**. The local review package is
**0.7.5-1+ubuntu26.04.local1**. The system-wide installation is complete and verified, including 237 installed
Wayland workspace assertions, clean shutdown and zero reported OBS memory leaks.
See `docs/validation/chat-reliability-0.7.5-local1.md` for evidence.

- Twitch/YouTube request adapters now reject transport failures and malformed or
  non-object JSON on successful HTTP responses. Bodyless 204 remains supported;
  provider error statuses retain their diagnostics. An unconfirmed YouTube send
  keeps the input and warns the user to check before retrying. Mutating actions
  are never automatically retried.
- YouTube remembers a 401 while loading broadcasts and reloads after the token
  changes. Reusing the same expired token does not cause a request loop.
- The chat menu routes reload to the selected mode. Integrated Twitch reconnects;
  integrated YouTube reloads broadcasts; original website chat retains its reload.
- Added regression coverage to `test/penguin-chat-actions.cpp` and a new
  `test/penguin-chat-reload.cpp`, with CI coverage for the latter.
- `HANDOFF.md` is now explicitly included by `.gitignore`, whose root deny-list
  previously excluded the file from normal Git discovery.

The local DEB uses the checksum-verified published 0.7.5 Ubuntu 26.04 package as
its base. Only `penguin-platforms.so` was rebuilt/replaced, against the installed
release SDK and Ubuntu Qt 6.10.2. The bundled publisher desktop OAuth configuration
was preserved from the release module without printing it or reading user tokens.
All other package payload files remain byte-identical; Debian version/md5 metadata
was updated. This is a module rebuild, not a full OBS/application rebuild.

Completed checks: combined chat fixtures, Twitch history fixtures, mode routing,
ASan/UBSan (Qt leak detection disabled), simulated account/OAuth platform flow,
237 native Wayland workspace assertions, 35 recording/local RTMP assertions and
8 preview/WebEngine assertions. Final synthetic recordings and received stream
fully decoded; the three final GUI sessions exited normally and reported zero
OBS memory leaks. No real account messages, moderation or webcam capture occurred.
See the validation record for test-harness retries and limitations.

The missing development/test dependencies were installed through normal Ubuntu
administrator confirmation. No passwordless rules were created. No public
version increment, commit, push, tag, GitHub release or website deployment occurred.

## Current checkout

- Product version in `packaging/version.txt`: **0.7.6**.
- Branch: `codex/rust-camera-preprocessing`.
- Local HEAD: `b018b3a7326a8dc921da3612335118499a4b6fc7`
  (`Fix Creator tools linkage in native release tests`).
- The checkout has substantial pre-existing modified and untracked files.
  HEAD is older than the release state described by the working tree. Do not
  equate the branch name or HEAD with the current feature set.
- `docs/validation/release-0.7.5.md` records release source commit
  `41246cb547f49547ff77018bffbc3f24d3a7f918`. Remote refs, current GitHub state and
  ancestry were not verified during this handoff. Reconcile them before committing
  or publishing; do not reset, clean or overwrite the existing work.
- User-facing path: `/home/ivan/Projekte-NAS/Linux Projekte/PenguinStudio`.
  It resolves to `/mnt/Projekte/Linux Projekte/PenguinStudio` in this environment.
  The checkout is on a NAS mount. Build staging may require a local filesystem
  with reliable Unix permissions and symlinks; see `CONTRIBUTING.md`.

Existing changes span the platform/chat modules, Qt display lifecycle, workspace,
Linux updater, Creator tools, packaging/CI, tests, current previews and release
documentation. Untracked work includes `web-chat.hpp`, the preview/WebEngine
test, release 0.7.3–0.7.5 records, validation evidence, `website/`, `deploy.sh`
and website content manifests. These are existing project work, not disposable
generated files. Run `git status --short` for the exact current inventory.

## Source map

| Area | Main entry points | Responsibilities |
| --- | --- | --- |
| Application | `frontend/obs-main.cpp`, `frontend/widgets/PenguinWorkspace.cpp` | Startup, classic Qt Widgets production workspace, controls and dock layout |
| Inspector | `frontend/widgets/PenguinInspector.cpp` | Scene-item transforms, crop, visibility, locking and undo |
| Native preview | `frontend/widgets/OBSQTDisplay.cpp`, `frontend/utility/SurfaceEventFilter.hpp` | libobs display lifecycle across native Qt surface changes |
| Platform accounts | `plugins/penguin-platforms/penguin-platforms.cpp`, `oauth-helpers.hpp`, `credential-store.hpp` in that directory | Twitch/YouTube authorization, metadata, broadcast selection and keychain storage |
| Chat | `plugins/penguin-platforms/{twitch-chat,youtube-chat,chat-view,web-chat}.hpp` | Integrated API chat, moderation and optional original website views |
| Outputs | `plugins/penguin-outputs/` | Multiple RTMP(S) destinations, portrait output and independent encoders |
| Creator tools | `plugins/penguin-outputs/creator-tools.cpp` | Recording checks, chapters, highlight notes, privacy and audio assistance |
| Camera | `plugins/penguin-camera/`, `plugins/penguin-backgroundremoval/` | Camera appearance, local matting, GPU helper, CPU fallback and Rust preprocessing |
| Updates | `frontend/utility/PenguinLinuxUpdater.cpp`, `frontend/widgets/OBSBasic_Updater.cpp` | Linux update checks and installation flow |
| Build/release | `.github/workflows/penguin-linux.yml`, `scripts/packaging/`, `packaging/version.txt` | Checks, Linux package generation and publication |
| Website | `website/`, root `deploy.sh`, `docs/website-content.json` | Promotional site, translations and release-content synchronization |

OBS Studio 33.0.0 is the documented engine baseline. Keep upstream ABI names,
author attribution and individual license notices. `docs/ARCHITECTURE.md` gives
the broader design, but some statements are stale: the platform module now
optionally links Qt WebEngine (`plugins/penguin-platforms/CMakeLists.txt`). Do not
treat its older “no embedded browser dependency” statement as an absolute.
The classic workspace remains the current product UI; a Quick module also exists.

## Important behavior confirmed in source

### Chat and accounts

- Twitch rendering retains unchanged message rows, selection and the visible
  message anchor instead of clearing the list on every update. Retained message
  IDs identify rows; history trimming restores the anchor's viewport position.
- Twitch tracks its own pending network replies. Disconnect aborts those replies,
  not all requests on a shared network manager. Delayed send-button callbacks
  check the session generation before changing controls.
- YouTube reload/selection cancels old requests and increments a generation.
  A `reading` guard prevents overlapping message polls. `seen` and `visibleIds`
  avoid repeated linear scans while preserving deletion replay protection and
  the 500-row visible-history bound.
- The YouTube chat view can request the exact broadcast selected in Stream
  details. Do not reintroduce an active-only query for an explicit selection.
- Moderation validates action/target data, prevents overlapping actions and
  distinguishes provider failures. YouTube unban requires a known ban ID.
- Website chat uses separate persistent WebEngine profiles when available.
  Google browser/API authorization does not create an embedded website session.
  Keep cookies and API credentials separate; no sign-in-policy bypass.
  Without WebEngine, integrated API chat remains available.

### Preview, recording and camera

- WebEngine initialization can replace the native Qt window/surface. The preview
  watches `WinIdChange`, rebinds the window/filter after Qt processes the change,
  and restores display creation on surface/expose events. Preserve this behavior
  when changing native widgets or embedded browser docks.
- Creator tools call the real recording chapter API. The output must support
  chapters (for example Hybrid MP4); a successful request is not a finalized file.
- Highlight notes are session-only, limited to 1,000 entries and explicitly
  exported to text. Timestamps are approximate; split recordings may reset them.
- Camera processing pairs a completed color frame with its matching mask.
  The reusable worker drains accepted work on destruction; caller-side frame
  admission is responsible for consuming completion before submitting more work.
  GPU helper and CPU fallback are separate paths. Repeated completed frames are
  not equivalent to processing every camera input; preserve honest counters.
- Normal Linux startup performs a background update check. A newer release is
  announced without interrupting active outputs or a modal startup flow.

## Latest documented release and test boundaries

Read `docs/VERIFICATION.md`, `docs/releases/0.7.5.md` and
`docs/validation/release-0.7.5.md` first. They record:

- All eleven jobs in the corrected 0.7.5 workflow passed; nine downloads matched
  the checksums/build manifest.
- The exact Ubuntu release DEB passed 237 workspace assertions, 35
  recording/local-stream assertions and eight Wayland preview/WebEngine checks.
  Videos fully decoded and the test applications exited normally with zero
  reported OBS memory leaks.
- System-wide installation of app and AMD camera runtime
  `0.7.5-1+ubuntu26.04` was verified, followed by installed workspace checks.

These are historical release records. The newer local1 package and installed
GUI checks are documented above; they do not revalidate every original release
artifact, hardware/runtime combination or the live website deployment.

Known boundaries:

- Live timeout/ban/unban testing was not performed because separate test accounts
  were unavailable. In-memory fixtures are not proof of live provider permissions.
- `docs/local-chat-review.md` records user-confirmed YouTube test-message sending
  and deletion in an older local build. Its local version labels are historical.
- Other package formats passed packaging/CI checks, not physical-machine coverage
  of every Linux desktop. Flatpak currently provides integrated chat without
  Qt WebEngine according to the release notes.
- Host RADV/WebEngine diagnostics and older V4L2 startup observations must not be
  silently relabelled as fixed. Preserve their original scope and evidence.
- Portrait is a crop/fit of program output, not an independent editable scene;
  encoders are per destination and hardware session limits still apply.

## Initial handoff-only checks (before the fixes)

On 2026-11-04, all three commands completed successfully against this checkout:

```sh
python3 scripts/check-publication.py
python3 scripts/check-branding.py
python3 scripts/check-penguin-locales.py
```

At the initial handoff-only stage, no compile, GUI run, camera capture, account
action, package installation or website build was performed. The later local1
validation above supersedes that stage for the modified chat module. Existing current previews were not visually
re-reviewed. Passing the publication script alone does not certify their visual
accuracy or English language.

For future changes, use the exact build/test commands and dependency setup in
`.github/workflows/penguin-linux.yml`, `docs/PACKAGING.md` and relevant test
READMEs. Do not assume `ctest` runs every Penguin check: CI also compiles and runs
standalone fixtures. Relevant tests include `test/penguin-twitch-chat.cpp`,
`test/penguin-chat-actions.cpp`, `plugins/penguin-platforms/tests/platform-flow.cpp`,
`test/penguin-preview-webengine.cpp`, `test/penguin-workspace-smoke.cpp`,
`test/penguin-creator-tools.cpp` and `test/penguin-update-flow.cpp`.
The workspace verification script requires explicit isolated runtime/app/results
paths; inspect its usage before running it against a new environment.

## Website maintenance

`website/README.md` is the operational reference. The site uses Next.js/React,
exports static content, and documents Node 24 / pnpm 11.25.0 for development.
English, German, French, Spanish, Italian and European Portuguese are supported.
Its documented server timer checks release content every 15 minutes; browser
pages recheck every minute and on activation. Live state was not inspected here.

The synchronizer selects published `penguin-v*` releases, validates tagged
README/preview data and switches content snapshots atomically. Translation
manifests must match the release tag and English-content digest; otherwise current
English content is used. Keep README source headings stable or update the parser.
Website code/layout changes require deployment; content refresh does not deploy
new code. Do not run `deploy.sh` merely to refresh this handoff.

Documented website checks (inside `website/`): `pnpm test`, `pnpm build`,
`node scripts/security-headers.mjs`, and
`python3 -m unittest discover -s tests -p 'test_*.py'` after dependency setup.

## Rules and next-session checklist

`AGENTS.md` is authoritative for this repository:

1. Implement and validate locally first. Install the latest validated local build
   for user review; keep local identifiers distinct from public versions.
2. Use system-wide DEB installation with normal Ubuntu administrator confirmation.
   Never request/store an administrator password or configure passwordless
   sudo/polkit. Per-user installation was declined.
3. Do not bump the public version, publish a release or push a release tag until
   explicit approval for that release after local review. Historical approvals
   in old validation documents do not authorize a new release.
4. Public product text and current GUI previews stay English. A version change
   requires README, installation examples, actual UI captures and
   `docs/images/current/manifest.json` updates. Visually inspect every new capture
   and run the publication check before publication.
5. Use isolated English demo profiles and synthetic media. Never publish private
   camera/desktop captures, account identities or credentials. Preserve old test
   evidence separately, with its original version and locale.
6. After final package tests, update the current release description; do not
   rewrite older releases as current.

At the next session: read this file and `AGENTS.md`, inspect the dirty worktree,
confirm the requested next task, and verify the local build/runtime prerequisites.
Before release work, reconcile local HEAD with the recorded release state and
current remote refs without discarding the working tree. The three chat fixes and local installation are authorized; public release is
now authorized for the reviewed changes as recorded above.
