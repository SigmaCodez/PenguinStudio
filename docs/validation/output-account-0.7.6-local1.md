# Shared account streaming access — 0.7.6 local1

Date: 2026-11-05. Public version remains 0.7.6. Local review only; no release,
public version increment, tag, push or website deployment.

## Correction

The earlier wizard integration configured only the primary OBS stream. The
Broadcast destinations dock still required an independent session key. This
local build connects the platform account state to those destinations too.

- Validated Twitch sign-in, restored sessions and selected YouTube broadcasts
  resolve streaming access and publish a session-only snapshot shared by the
  platform module and Output dock. A newer request or disconnect invalidates
  older replies. Provider errors remain visible instead of claiming readiness.
- Supported legacy destinations default to account mode. Existing and newly
  added Twitch/YouTube destinations reuse the matching signed-in account; their
  editor hides manual-key entry while account mode is selected.
- The output editor opens the real streaming-account dialog and accepts the
  resolved endpoint/key. Manual setup remains an explicit alternative; toggling
  it clears the hidden account key. Manually configured destinations are not
  overwritten by subsequent account updates.
- Disconnect clears inactive account destinations. Running outputs keep their
  existing configuration; the new snapshot applies once they stop. Signing in,
  restoring a session or editing a destination never starts an output.
- Destination JSON saves only the account/manual preference and allowlisted
  non-secret settings. It never persists streaming keys. Account destinations
  restore through the platform's existing keychain-backed session on next launch.
- YouTube still requires an existing configured encoder broadcast and bound
  stream. No broadcasts are created, bound or transitioned. Older Twitch grants
  may require reconnecting to grant stream-key read access.

## Validation

- 18 real Wayland GUI assertions pass, including a legacy target, immediate
  account propagation, provider separation, manual fallback, changed accounts,
  disconnect, no automatic outputs and credential-free persistence.
- Output safety tests cover the shared state, observer lifetime, invalidation,
  endpoint adoption and manual-key preservation, as well as existing validation
  and output cleanup. ASan/UBSan passes with leak detection disabled for Qt.
- Existing provider/OAuth fixtures pass ASan/UBSan, including request cancellation
  and stream-access responses. Real provider login was not performed by automation.
- Wizard lifecycle tests pass: asynchronous dialogs, parent destruction, shutdown
  cancellation and duplicate prevention.
- 237 workspace, 35 recording/local-RTMP and eight preview/WebEngine assertions
  pass. All four final GUI sessions exit normally and report zero OBS memory
  leaks. All recordings and the received stream fully decode.
- Publication and locale checks pass. Current public previews remain unchanged.
  The new isolated English Output dock capture was visually inspected and is
  retained in `docs/images/local-review/output-account-0.7.6-local1/`.

## Build and installation

The verified published 0.7.6 Ubuntu DEB is the base. Only the platform and output
modules were rebuilt against the installed release SDK; the eight output locale
files and Debian local version/md5 metadata were updated. The frontend and other
runtime components remain the published versions, and package symlinks are
preserved. This is not a full application rebuild. The bundled public publisher
OAuth configuration was preserved privately from the installed module without
reading user credentials; the temporary build include was removed afterward.

Package: `0.7.6-1+ubuntu26.04.local1`, archived with SHA256SUMS in
`dist/local-review/0.7.6-local1/`. Source/package hashes and portable test results
are beside this document. Installation through normal `pkexec apt-get --no-remove`
completed successfully; only Penguin Studio was upgraded. `dpkg --verify` is
clean and both installed modules match the tested files. No passwordless rules
or administrator password storage were introduced.

The app is restarted for manual review with a one-shot helper that only invokes
the requested auto-configuration wizard; it does not alter accounts, scenes or
stream settings, and removes its preload variable before child processes start.
