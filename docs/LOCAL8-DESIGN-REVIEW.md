# Local8 design and camera scheduling review

Local candidate: `0.7.1-1+ubuntu26.04.local8` (Ubuntu 26.04, amd64).
Public version remains 0.7.1. This candidate is for local review, not publication.

## Presentation

The existing application-wide Penguin theme continues to cover native dialogs,
buttons, input fields, tabs, context menus and docks. Settings now has a QML
sidebar with page search, selection and keyboard focus. The native page model,
validation, Apply, OK and Cancel remain authoritative. Hidden and disabled pages
cannot be selected through the bridge. A QML load error retains native navigation.

Clicking the Penguin Studio brand opens the Command center: search existing
application actions, browse results with arrow keys, activate with Enter and close
with Escape. Font scaling follows the application preference. Actions retain their
native confirmations. Disabled actions and disabled parent menus cannot be invoked.
Hold the brand button for the full native menu, including dynamic submenus. This
is a QML presentation layer, not a rewrite of every settings control or plugin UI.
The Wayland Qt Quick software backend from local7 remains in place; video and
camera inference still use their existing GPU paths.

## Camera scheduling

Profiling separated staging, primary GPU inference and the CPU hand companion.
The primary GPU inference took approximately 3 ms; the companion dominated the
same-frame wait. The filter previously silently capped hand-enabled sessions to
three CPU threads even when the managed configuration requested four. The filter
now honors the configured budget within the supported 1–8 range. A reusable
companion thread replaces per-frame `std::async` thread creation; completion and
exceptions still belong to the same frame. Destruction drains accepted work before
releasing the model. No model, threshold, resolution or hand-recovery policy changed.

The staged CPU frame is now retained by reference-counted ownership instead of
being cloned again. Staging always publishes a fresh owned buffer; inference never
mutates the published snapshot. Image and alpha remain matched to the same frame.

## Local evidence

Isolated English profiles and synthetic media only. Diagnostic artifacts are under
`/var/tmp/penguin-obs33/local8-*`; they are not public release previews.

- `dialog-navigation`: 22 checks passed, including all nine native settings pages,
  invalid/disabled-page rejection, case-insensitive search, command lookup and
  routing into native Settings. Command center and Settings captures inspected at
  FontScale 14. No QML errors and clean shutdown.
- `camera-profile2`: instrumented baseline, 6 rendering stalls (0.6%). Median
  processing 23.299 ms, GPU wait 3.208 ms, hand wait 19.720 ms.
- `camera-optimized`: thread-cap and copy changes, median processing 18.805 ms;
  8 rendering stalls (0.9%). Faster median alone did not eliminate outliers.
- `camera-worker`: reusable worker, 30.1-second recording, 900 decoded video
  frames, no reported rendering/encoding losses, clean shutdown and zero
  OBS-reported leaks. These are individual measurements, not a guarantee against
  stalls under arbitrary hardware, frame rates or system load.
- Worker lifetime, exception propagation, reuse and pending-task destruction tests
  passed, including AddressSanitizer/UndefinedBehaviorSanitizer outside the sandbox.
- Six real-model repeated/resized hand-buffer comparisons and 96 exact RVM input
  comparisons/rejection cases passed. The release-mode postprocessing test passed
  twelve tensor comparisons and special-value/padded-view cases. Its initial debug
  build stopped on the existing intentional invalid-input assertion; the test's
  configured release-mode contract was then used.

- `package-workspace`: 242 assertions passed, with a decodable short recording,
  no reported losses and clean shutdown.
- `package-dialogs`: 18 views and 172 input-height checks passed at FontScale 14;
  no reported horizontal overflow. Settings and effect-dialog captures inspected.
  QWidget captures still cannot represent the native video surface reliably;
  garbled preview pixels in diagnostic grabs are not approved product previews.
- `package-gpu`: final package payload, 30.1 seconds / 900 decoded video frames,
  GPU active, no reported losses and clean shutdown.
- `package-cpu`: CPU mode remained functional, 29.966 seconds / 898 decoded video
  frames, clean shutdown. Two rendering stalls (0.2%) remain recorded as a limitation.
- `keyboard`: five-minute native Wayland session ended cleanly, but no physical
  key events were received. This is not a passed physical-keyboard acceptance test.
- Package simulation: one upgrade (Penguin Studio), no added or removed packages.

- `package-fallback-fixed`: GPU runtime hidden from the isolated test; automatic
  CPU fallback active, 29.966 seconds / 899 decoded video frames, no reported losses
  and clean shutdown. The first fallback launch failed before application startup
  because the old runner expected a removed OBS plugin directory; the runner was
  corrected for the OBS 33 layout.

- `baseline-cpu`: unchanged local7 CPU comparison, 30.1 seconds / 900 decoded
  frames, no reported losses. A single baseline and candidate run cannot establish
  a CPU regression or improvement; local8's two CPU stalls are not marked resolved.

## Installed validation and user review

System-wide installation of `0.7.1-1+ubuntu26.04.local8` completed successfully
(installer exit 0). `dpkg-query` confirmed the version and `dpkg -V penguin-studio`
reported no modified package files. No additional package was installed or removed.

`local8-installed-dialogs` passed all 22 navigation checks under native Wayland at
FontScale 14. The installed application started and closed normally, with no
warning/error or QML error entries and zero OBS-reported memory leaks.

The user subsequently confirmed visible keyboard focus and both requested page
transitions: search Settings and Enter, then find Audio, Tab and Enter. This is
user-reported acceptance; the earlier automated key-event monitor did not capture
those inputs. Escape and reverse-Tab were not explicitly confirmed by the user.
No further duplicate manual test was required for the two confirmed transitions.

The two isolated CPU-mode stalls above remain unresolved evidence. Installation
and keyboard acceptance do not convert them into a zero-loss claim. Nothing was
pushed or published to GitHub.

Package SHA-256:
`586f4f569910d12d1678443eb007334e8b4cb022fa1e4b33d24503f4643d38ad`.
