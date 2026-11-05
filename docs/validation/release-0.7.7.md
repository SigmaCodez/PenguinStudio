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

Candidate and final package results are recorded below.
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

## Exact Ubuntu release artifact

The Ubuntu 26.04 DEB from workflow
[42024096136](https://github.com/SigmaCodez/PenguinStudio/actions/runs/42024096136)
passed its artifact SHA256 check: `6e94cf62d1db0e8d214bc5174594f36dfff9c704d38db5bb085410558842b1d6`.
It reports `0.7.7-1+ubuntu26.04` and Penguin Studio 0.7.7 / OBS 33.0.0.
The release source is `1a3d226d19f92cb8a37d2878ba58a678078db1bc`, merged through
[PR #20](https://github.com/SigmaCodez/PenguinStudio/pull/20) under the existing
branch rules. The redundant PR workflow was cancelled; the main release workflow
completed successfully with its complete package gates.

The exact package, including its own OBS libraries, modules and data, passed all
**237 workspace**, **35 recording/local RTMP**, **8 preview/WebEngine** and
**18 Output-account** assertions under Wayland in isolated profiles. All four
sessions exited with status zero and reported zero OBS memory leaks. All four
recording/received-stream files fully decode. Synthetic media and simulated
account replies were used; no private profile or real broadcast was involved.
Expected RADV and WebEngine OOM-adjustment diagnostics do not establish
warning-free operation. Portable assertion results and a decoding/exit summary
are retained alongside this document.

APT's `--simulate --no-remove` preflight proposes only the Penguin Studio upgrade,
with no new or removed packages. Publication does not replace the reviewed local
installation, verified as `0.7.6-1+ubuntu26.04.local1` during release validation.
No administrator-policy change or password storage is involved.

## Published release verification

All eleven jobs in workflow 42024096136 passed. Version
[0.7.7](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.7.7)
is public as a development prerelease. All **nine public downloads** were
downloaded and verified against both SHA256SUMS and build-manifest.json,
including file sizes and the exact source commit
`1a3d226d19f92cb8a37d2878ba58a678078db1bc`. No package mismatch was found.
The public Ubuntu 26.04 DEB is byte-identical to the locally tested CI artifact.
The verified Ubuntu DEB and manifests are archived in `dist/releases/0.7.7/`.
The current release description was updated with these final package results;
older release descriptions were left unchanged.

The final documentation update is separate from the immutable package source.
The original worktree retains its older HEAD and unrelated changes; only the
approved fix and release/documentation files are synchronized back. No release
package installation is claimed: the reviewed local1 installation remains.
