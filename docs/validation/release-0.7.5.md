# Penguin Studio 0.7.5 validation

The approved 0.7.4 local1/local2 builds passed combined Twitch/YouTube chat tests, simulated platform sign-in, address/undefined-behavior sanitizer checks, 237 native Wayland workspace assertions and 35 recording/local-stream assertions. Both installed builds were verified and exited normally with zero OBS memory leaks. See the linked local evidence in VERIFICATION.md.

The 0.7.5 candidate frontend was rebuilt with the new version. All 237 native Wayland workspace checks passed; shutdown reported zero OBS memory leaks. Three fresh English GUI captures from the real candidate were visually inspected and recorded in the current preview manifest. Publication, branding, locale and release-rule tests passed. Final CI package/download and installation results will follow.
