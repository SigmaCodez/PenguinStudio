# Penguin Studio 0.7.2 release validation

The user approved publication after installed classic8 validation. Current English previews were recaptured from that installed application with synthetic media and visually inspected. The original NAS checkout remains preserved; publication work uses a local checkout because SMB Git operations stalled.

Initial CI run 40747505461 passed branding/translations, OAuth and configuration tests, Rust checks, documentation checks, AMD runtime builds and Snap build/install checks. Native packages exposed missing Creator-tools symbols in the test executables, not in the application module.

The local streaming test now links the production Creator-tools translation unit. The isolated wizard fixture explicitly supplies its absent frontend/Creator lifecycle doubles and asserts the close call; actual Creator-tools teardown remains covered by the separate installed-app Wayland tests. No production behavior was changed by this build correction.

Both executables linked locally. The wizard lifecycle test passed. The real loopback-only streaming test passed under native Wayland: concurrent landscape/portrait H.264/AAC outputs, crop/fit, stop/restart, reconnect, active shutdown and unreachable-server cleanup. All resulting FLV files fully decoded. Deliberate disconnect/unreachable-server cases produce expected diagnostics; this is not a warning-free live-provider test.

Evidence: `/var/tmp/penguin-release072-publish/local-stream-validation` and `local-stream-validation.log`.

Final complete Linux package CI and release installation are still pending.
