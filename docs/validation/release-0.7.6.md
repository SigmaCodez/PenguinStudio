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
