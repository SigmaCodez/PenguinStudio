# Classic workspace maintenance — 2026-10-27

Local review only; no public version increment or GitHub publication.

## Findings and fix

The normal German-language startup logged a missing `de-DE` translation for
`obs-backgroundremoval.so`. Add the complete 44-key German locale. Existing
translations, upstream attribution and licenses are preserved. English remains
the default and the language of public documentation.

The previous normal session reported a graphics-thread median of 0.141 ms,
99th percentile of 0.534 ms and maximum of 6.884 ms at a 16.667 ms frame budget.
This is an idle-session observation, not a camera benchmark or evidence of a
performance gain. No speculative render-pipeline change was made.

## Checks

- German locale keys exactly match the 44 English source keys.
- Eight Rust preprocessing tests pass, including invalid buffer layouts,
  vector boundaries, padding and nonfinite hand-model values.
- GPU monitor tests pass for bounds, invalid counters, adapter changes and
  destruction with a pending asynchronous sample.
- Native Wayland startup/shutdown with an isolated German profile and the new
  locale exits successfully, without warning/error entries or reported leaks.
- The local classic2 DEB changes only the German locale in its file payload;
  executable binaries remain identical to the previously tested classic1 build.
- APT simulation upgrades only Penguin Studio, with no additions or removals.

Package: `0.7.2-0+ubuntu26.04.classic2`.
SHA-256: `7fd00a46f47fe21ed8c06c1a0dfda908c25d97213397e3f4eab688f49eaa3152`.

Installation completed after the normal Ubuntu administrator confirmation.
`dpkg-query` confirms classic2; the installed German locale matches the source.
A second isolated native Wayland German startup/shutdown test against the installed
binary exited with status zero, without warning/error entries or reported memory
leaks. The shutdown probe confirmed context destruction and no late unregister
calls. Evidence: `/var/tmp/penguin-obs33/classic2-installed-german-shutdown`.

This pass did
not repeat live Facecam or account-authentication tests, and does not claim a CPU
or GPU performance improvement. Current English previews remain visually
unchanged because this correction only supplies a German filter translation.
