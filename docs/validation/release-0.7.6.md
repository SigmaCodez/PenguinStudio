# Penguin Studio 0.7.6 validation

Publication was explicitly approved on 2026-11-05 after the reviewed local2 build
was installed and opened for manual account testing. Public version advances
from 0.7.5 to 0.7.6; the previous release remains unchanged.

The approved changes and local evidence are recorded in
[wizard account setup](wizard-account-0.7.5-local2.md) and
[chat reliability](chat-reliability-0.7.5-local1.md). The release checkout starts
from main `52865ccce337e5237ba038b146ef65c12b1bcd65` and contains only those changes,
version/publication documentation and refreshed real English captures.
Unrelated local website changes are not part of this release.

## Candidate and final package checks

Candidate and final package results are appended after completion. The complete
Linux workflow gates publication on every required build and release check.
No live-account test success is inferred from release approval or simulated
provider tests. Other distributions receive automated package validation;
physical-machine coverage of every desktop and GPU is not claimed.

The frontend was rebuilt with version 0.7.6 and tested with the verified installed
local2 runtime modules. All 237 Wayland workspace and 16 real-wizard assertions
pass, both sessions exit normally with zero reported OBS memory leaks, and the
workspace recording fully decodes. The three fresh XComposite English previews
were visually inspected; their provenance and executable hash are in the current
manifest. No private content was captured. Release/publication/portable Python
suites (13 tests), branding/locales and strict publication checks pass. Chat action
and reload-routing fixtures also pass under the CI warning/error flags.

The release candidate build initially encountered short writes while archiving
an OpenCV dependency in tmpfs. Removing redundant extracted package copies made
space; the rebuild completed. Archived DEBs and historical test evidence were
retained. This was a local build-environment issue, not a source change.

## Exact Ubuntu release artifact

The Ubuntu 26.04 CI DEB from workflow
[41990529592](https://github.com/SigmaCodez/PenguinStudio/actions/runs/41990529592)
passed its artifact SHA256 check:
`a32ccf47a7b1aa18498b99d3f853bc2f903d91bd7873f8e2ab816d1e76ebd96e`.
It reports `0.7.6-1+ubuntu26.04` and Penguin Studio 0.7.6 / OBS 33.0.0.

The exact package, with its own OBS libraries, data and modules bound into an
isolated environment, passed all **237 workspace**, **35 recording/local RTMP**,
**8 preview/WebEngine** and **16 wizard account** assertions under Wayland.
All four processes exited with status zero and reported zero OBS memory leaks.
Workspace, Creator and preview recordings, plus the received stream, fully decode.
Only synthetic media and simulated account completion were used. Expected host
RADV and WebEngine OOM-adjustment diagnostics do not imply warning-free operation.

APT's `--simulate --no-remove` preflight proposes only the Penguin Studio upgrade,
with no additional or removed packages. No optional AMD runtime is installed on
this host, so no new optional runtime is added for this release verification.

## Published release verification

All eleven jobs in the release workflow passed. The release is public as a
development prerelease at
[0.7.6](https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v0.7.6).
All **nine public downloads** were downloaded and verified against both
SHA256SUMS and build-manifest.json, including file sizes and exact source commit
`c8842c9c1e9334076b139f54d35b88e67fbea2dc`. The public Ubuntu 26.04 DEB is
byte-identical to the locally tested CI artifact. Portable results, manifest and
workflow metadata are retained alongside this document. The first download pass
hit the local tmpfs user quota; removing regenerable candidate build objects
allowed verification to resume and finish. No package mismatch was found.

The verified Ubuntu package and manifests are archived in `dist/releases/0.7.6/`.
The system-wide `pkexec apt-get --no-remove` transaction currently awaits normal
Ubuntu administrator confirmation; the last verified installed version is still
`0.7.5-1+ubuntu26.04.local2`. Installed-0.7.6 verification is therefore pending.
There are no passwordless authorization changes, and no new optional AMD runtime
is installed. Historical release descriptions and evidence remain unchanged.
