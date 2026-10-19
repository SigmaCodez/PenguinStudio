# Qt Wayland teardown diagnostics

Qt 6.10.2 can report two warnings when a wizard's Wayland surface is destroyed
before a queued compositor leave event arrives:

- `QWaylandTextInputv3::zwp_text_input_v3_leave`: a null surface differs from the cached focus surface.
- `QWaylandTextInputv3::disableSurface`: the same null surface differs from the cached focus surface.

A null surface in this event means the client-side proxy has already been freed.
This is normal window teardown, not evidence of an invalid shutdown. Qt's
[upstream correction for QTBUG-120533](https://github.com/qt/qtbase/commit/ac8b2e9b40bf19aa889345f7b617392bef906040)
recognizes this case. An earlier
[text-input-v3 correction](https://github.com/qt/qtbase/commit/a2bc24c05189f9d39c034560f163ad1490c37996)
also removes redundant surface enable/disable calls in newer Qt.

Penguin Studio applies a diagnostic compatibility correction in its existing Qt
message handler. It classifies only these exact null-surface messages as debug
diagnostics on Qt 6.10.2 with the Wayland platform. The original messages remain
available with `penguin-studio --verbose`, including in release builds. It does
not replace the system Qt libraries, change keyboard focus, delay shutdown, or
discard other warnings. Non-null surface mismatches, other logging categories,
critical/fatal messages, and other Qt versions keep their original severity.

`test/penguin-qt-wayland-diagnostics.cpp` verifies the classification boundary,
including malformed messages and real surface mismatches. The native shutdown
runner in `scripts/packaging/verify-shutdown.sh` uses isolated profiles and verbose
logging. It checks normal exit, multiple instances, shutdown with the initial
wizard open, cancellation of the connection wizard, shutdown with an actual
connection text field focused, the auto-configuration wizard, statistics,
restart, and SIGTERM.

The runner rejects either null-surface warning anywhere in the log and rejects
all warnings and errors in the shutdown section. It retains startup diagnostics
separately and records the original debug messages when the compositor produces
the teardown sequence. Each process must exit with zero OBS memory leaks, no
late frontend callback removals, and no remaining crash sentinel. The compositor
does not produce the delayed event on every run, so a debug-message occurrence
is recorded rather than required for every scenario.

See [VERIFICATION.md](VERIFICATION.md) for the installed-package results and
[the native test guide](../test/README-native-wayland.md) for reproduction steps.

## Startup audit (Ubuntu 26.04, Qt 6.10.2)

Assigning the desktop file ID before constructing `QApplication` prevents the
host-portal duplicate-registration warning; moving it only within the application
constructor is too late for Qt platform-service construction. Nine isolated
Wayland runs verified the early-ID change without duplicate-registration warnings.

The remaining non-null `disableSurface` warning is associated with
[QTBUG-141475](https://github.com/qt/qtbase/commit/a2bc24c05189f9d39c034560f163ad1490c37996).
The upstream fix changes text-input state transitions, not just logging. It is
therefore not covered by the null-surface diagnostic classification above.

An experiment preferring the desktop's existing IBus context removed the focus
warning and passed wizard focus/cancellation checks, but introduced
`QSocketNotifier: Can only be used with threads started with QThread` during Qt
construction, reproduced in a minimal native Qt application. This workaround was
not shipped. Both direct IBus and its portal path reproduced that warning. The
application continues to honor the user's existing input-method settings.
