# Penguin Studio 0.7.7 validation

Date: 2026-11-05. The user confirmed the auto-configuration wizard was open,
reported that the Output-account fix worked, and explicitly requested publication
and a handoff update. The provider(s) exercised were not specified; this is not
recorded as exhaustive independent live-provider coverage.

The release contains the reviewed changes from
[0.7.6 local1](output-account-0.7.6-local1.md), current English previews, versioned
installation examples and release documentation. Previous releases and their
original-version evidence are preserved. Release preparation reuses the isolated
checkout `/tmp/penguin-release-076/repo` on branch `codex/release-0.7.7`, based on
main `25e9cd52423322e75a39ba2025d188610f88ce19`. Unrelated original-worktree changes
are not included.

Candidate and final package results are appended after their checks complete.
The complete Linux workflow gates publication on every required package job.

## Candidate verification

The frontend was rebuilt from source with version 0.7.7 and tested against the
verified installed local1 runtime modules. All 237 Wayland workspace and 18
Output-account assertions pass. Both sessions exited normally and reported zero
OBS memory leaks; the workspace recording fully decodes. The relevant production
source hashes match the reviewed local build evidence. All three new native
XComposite English previews were visually inspected; the current manifest records
their hashes, candidate executable hash and source provenance. No private content
was captured. Branding, locale, release/publication/portable tests (13 tests) and
the strict publication check pass.
