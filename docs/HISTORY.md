# Source provenance and Git history

Penguin Studio's `main` branch starts from an independent source snapshot after
the history reset on October 19, 2026. The former OBS-derived branches and tags
are no longer part of the published branch history. The reset preserves the
Penguin implementation, the inherited OBS source, author attribution, license
notices and pinned submodules. It changes the repository's history, not the
origin or licensing of the source.

The upstream baseline is OBS Studio 32.2.2, commit
[`ba2f32bdf791005443988a4955e963663e16b1ed`](https://github.com/obsproject/obs-studio/commit/ba2f32bdf791005443988a4955e963663e16b1ed).
Keep [AUTHORS](../AUTHORS), [COPYING](../COPYING), the original
[OBS README](../README.rst), source copyright headers and third-party notices
when redistributing or updating the application. `.gitmodules` and its pinned
Git entries retain the upstream submodules and their separate histories.

## Existing verification and packages

The 0.1 and 0.2 records in `docs/validation/` describe work performed before the
reset. Their original commit hashes, branch names, run IDs and package checksums
remain unchanged. In particular, the 0.2 workspace tests used
`6ed8300525b17e1fd0597389a2b139956228240d`; the final 0.2 application packages
used `8c1d5113a19fa90a5c604e79d81e6e0c60208dbc`. The repository-state record
describes the former `penguin/main` default branch and retained ancestry at
the time it was captured.

Those commits may no longer be reachable from the current public branches or
tags. Use the retained maintainer backup when the exact pre-reset Git history
is needed. A history reset does not rerun tests, rebuild an existing package or
change its recorded source revision. New builds must record their own source
revision and verification results.

## Future upstream changes

Clone Penguin Studio recursively from `main`. Add OBS as a separate read-only
source of updates when needed:

```sh
git remote add upstream https://github.com/obsproject/obs-studio.git
git fetch --no-tags upstream
```

The new history has no shared merge base with OBS or with older Penguin
checkouts. Review changes against the pinned upstream baseline, import selected
patches deliberately, retain their attribution and licenses, and run the
relevant build and runtime checks. Fetching with `--no-tags` avoids importing
OBS release tags into the local Penguin tag namespace. A fresh recursive clone
is the simplest way to start working with the new history; preserve any local
work before replacing an older checkout.

Package recipes explicitly set `OBS_VERSION_OVERRIDE=32.2.2` for the engine.
Penguin Studio's application version is maintained separately in
`packaging/version.txt`; deleting or creating Git tags does not release or
rename an existing package.

Direct CMake configuration also falls back to the pinned engine version
`32.2.2` when Git returns a commit hash or a Penguin release tag. The reset
includes that compatibility fix. Run `python3 test/penguin-version-config.py`
to verify the nine isolated cases covering tagless checkouts, OBS and Penguin
tags, explicit overrides, prereleases and source archives. All nine passed
when preparing this snapshot; this is a version-configuration check, not a
new full application or package test.
