# Chat and camera background validation — 0.6.0

## Local checks completed

- Native Twitch event tests: plain-text rendering, duplicate suppression, bounded
  history, deleted/cleared messages and safe EventSub reconnect endpoints.
- In-memory HTTP fixtures: Twitch timeout/delete, permission denial without losing
  the account, participant pagination, retained session-ban controls; YouTube
  broadcast selection, server polling interval, token rotation, timeout/unban IDs,
  expired-token recovery and cancellation of old-account requests.
- Existing platform-flow regression: device grant backoff, saved account rotation,
  update exclusion, Google PKCE/loopback callbacks, pagination, preservation of
  existing video metadata, revoked sessions and callback shutdown.
- Publication guard: pending image refresh is allowed only for explicit candidate
  checks; publishing rejects it. Image hashes, locale and stale text remain checked.
- The camera module's actual GPU output under native Wayland passed shape/keying,
  automatic matting, blur of a high-frequency checkerboard, custom-image
  composition, rounded corners, undo/restore and missing-image transparency tests.
- Isolated native Wayland chat windows rendered local demo messages in both tabs
  and closed without warning/error diagnostics or reported memory leaks.

These local component runs used the installed engine with newly compiled modules
and isolated synthetic scenes. They are not a substitute for validating the fully
rebuilt application and its new Inspector controls. Package validation and final
English product captures are recorded below when complete.

No real messages were sent, viewers banned, stream titles changed or private
camera footage published. Live-platform moderation is not claimed as tested.

## Synthetic recording

A native Wayland recording switched from automatic blur to a custom image while
keeping rounded corners. The 20.1-second H.264/AAC file (1280 × 720, 30 fps) fully
decoded, contained the synthetic test tone, and reported zero dropped frames.
Frames from both effect phases were visually inspected. The run had no warning/
error diagnostics, no reported render/encoder stalls, and zero reported memory
leaks on normal shutdown. No webcam or microphone was captured.

## Full-package validation

The Ubuntu 26.04 candidate built from `197550e4` passed the full native Wayland
Inspector test: all six background choices were available, blur and image paths
were applied through the real controls, and shape/keying, rounded corners,
undo/redo, settings restoration and missing-image fallback passed. It closed
with no warning/error diagnostics and zero reported memory leaks.

The three English product previews were captured from that actual package under
XWayland, using synthetic media and explicitly labelled local chat messages.
Each was visually reviewed; capture provenance is in the current-image manifest.
Later chat fixes are covered by the current request/event regression fixtures.

## Final Ubuntu package and installation

The package from release source `73bd5d5b` passed the same strict native Wayland
Inspector tests and both chat-tab GUI checks. Its checksum and dependency
simulation passed; installing `0.6.0-1+ubuntu26.04` on Ubuntu 26.04.1 upgraded only
Penguin Studio, with no added or removed dependencies. Installed-file verification
passed.

The installed application then passed the Inspector/pixel suite and both chat-tab
checks again. A 20.033-second 1280 × 720, 30 fps H.264/AAC recording switched blur
to a custom image with rounded corners. Full decoding, synthetic audio and visual
frame checks passed with zero reported dropped frames. All three installed runs
closed normally without warnings/errors or reported memory leaks. No personal
profile, webcam, microphone or live moderation action was used.

See [machine-readable results](chat-camera-0.6.0.json) for the package hash and
recording/control evidence. The complete download set is gated by the
[release workflow](https://github.com/SigmaCodez/PenguinStudio/actions/runs/40035647084).
