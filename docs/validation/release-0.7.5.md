# Penguin Studio 0.7.5 validation

The approved 0.7.4 local1/local2 builds passed combined Twitch/YouTube chat tests, simulated platform sign-in, address/undefined-behavior sanitizer checks, 237 native Wayland workspace assertions and 35 recording/local-stream assertions. Both installed builds were verified and exited normally with zero OBS memory leaks. See the linked local evidence in VERIFICATION.md.

The 0.7.5 candidate frontend was rebuilt with the new version. All 237 native Wayland workspace checks passed; shutdown reported zero OBS memory leaks. Three fresh English GUI captures from the real candidate were visually inspected and recorded in the current preview manifest. Publication, branding, locale and release-rule tests passed. Final package/download results are recorded below; installation verification is complete.

The first CI run stopped on an older Twitch scroll test that asserted a fixed scrollbar value after trimming a row. The assertion now checks the persistent message identity and its viewport position instead, matching the approved reading-anchor behavior. The updated test passes locally; all CI gates are rerun before publication. No production code was changed for this test correction.

## Final Linux packages

All eleven jobs in [the corrected release workflow](https://github.com/SigmaCodez/PenguinStudio/actions/runs/41249398071) passed. All nine published packages match SHA256SUMS and the build manifest, including file sizes and source commit `41246cb547f49547ff77018bffbc3f24d3a7f918`.

The exact Ubuntu 26.04 release DEB passed 237 workspace assertions, 35 recording/local-stream assertions and eight native preview/WebEngine lifecycle assertions under Wayland. Recorded and received video fully decoded. The preview test recorded 1,140 draw callbacks and retained the expected red source at 1, 5, 15 and 22 seconds while a synthetic browser dock was opened and replaced. All three applications exited with status zero and reported zero OBS memory leaks.

The downloaded release DEB is byte-identical to the tested CI artifact. Installation preflight confirms that only Penguin Studio and the existing AMD camera runtime are upgraded, with no added or removed dependencies.

The tests use isolated profiles and synthetic media, without real chat sends or moderation. Other package formats passed CI packaging checks, not physical-machine coverage of every Linux desktop. Host RADV diagnostics may still appear during WebEngine initialization; this is not a claim that every third-party warning is absent.

## Installed release

The system-wide Ubuntu installation of Penguin Studio and the AMD camera runtime, both `0.7.5-1+ubuntu26.04`, completed with normal administrator confirmation. Package integrity and the installed executable were verified. The installed app passed all 237 workspace checks under Wayland, exited with status zero and reported zero OBS memory leaks.
