# Unified classic interface — local review, 2026-10-28

Release preparation was held for the user's subsequent design feedback. The new layout remains a local review build; no GitHub publication has occurred.

## Scope

Local build: `0.7.2-0+ubuntu26.04.classic6`.

- The privacy control shares the production toolbar's inner layout, spacing, input height, icon size, hover/pressed/disabled states and focus treatment. It uses an original shield icon from the existing icon renderer, retained when the action changes state.
- Below 1,200 pixels the privacy action uses the same square icon-only treatment as compact toolbar controls. Its action/shortcut/tooltip remain available.
- Creator tools uses the existing charcoal/violet theme with a shared heading hierarchy, consistent cards, labelled status badges and compact action rows. It no longer stacks full-width action buttons.
- Stream checks adapt row heights to wrapped text. Audio selection and measurement share one row; apply/undo actions remain grouped. Privacy is integrated into a compact footer card with an explicit resume label.
- The classic editor, menus, settings, camera processing and output pipelines are preserved.

## Checks

- Module compilation passed with `-Wall -Wextra -Werror`.
- 19 Creator tools assertions passed under Wayland, including matching privacy/record button heights, shared parent layout, retained active-state icon and visible button text fitting its content rectangle.
- Existing workspace regression: 210 assertions passed under Wayland.
- Synthetic recordings fully decoded; privacy intervals remained dark and silent; no render/encoder/network loss events or warning/error/critical messages were reported in the final test runs. Shutdown reported zero OBS memory leaks.
- The DEB payload differs from classic5 only in `penguin-outputs.so` and `themes/Penguin.ovt`. Both packaged files are byte-identical to the tested candidate files.
- APT simulation upgrades only Penguin Studio, with no additions or removals.
- Package SHA-256: `12f59068db2913c7172b4b14d21ae56cd062041cb8b5f20e5634a1859a2b6f2d`.
- All three current English product previews were captured from the real application under XWayland using an isolated synthetic profile and visually inspected. The prior classic1 captures and provenance were archived separately. Native Wayland was used for the functional tests.

Evidence: `/var/tmp/penguin-obs33/creator-design-final`, `unified-design-workspace`, and `unified-design-previews`. Camera CPU/GPU performance evidence is retained in the preceding Creator tools validation; this design change does not alter those processing binaries.

Installation completed through the normal Ubuntu administrator confirmation. DPKG reports classic6 and both changed installed files match the tested package SHA-256 hashes. The first installed test produced a complete passing 19-check report, fully decoded recording and clean application log, but its outer launcher reported signal 143; a separate installed repeat (`/var/tmp/penguin-obs33/unified-design-installed-confirm`) completed with exit status 0. That repeat passed all 19 assertions, full recording decoding and privacy sample checks, with no reported frame-loss events or warning/error/critical messages and zero OBS memory leaks. The initial launcher signal did not recur; its cause was not established. Physical keyboard use and the user's visual acceptance remain local-review tasks. No absolute guarantee about all workloads or every display configuration is implied.
