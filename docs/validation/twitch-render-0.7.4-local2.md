# Local Twitch chat improvements — 0.7.4 local2

Local-only review build; no public version increment or publication.

The integrated Twitch view now reconciles retained message IDs instead of destroying and rebuilding every row. New messages append in order; moderation removes only matching rows; selection and the top visible reading position survive history trimming. An unchanged history skips layout work. The 500-message bound is unchanged.

Pending API replies are scoped to the Twitch view and held through weak references. Disconnect cancels only those requests, leaving unrelated users of a shared transport intact. A delayed send-button cooldown cannot modify a later account session.

## Validation

The regression fixture fails against the original implementation because a persistent model index for an unchanged row becomes invalid. It passes after the change. Tests cover append order, removal, user-wide moderation, clear events, bounded history, selection and reading-position retention, and cancellation ownership. A reading-anchor issue caused by the list's spacing was found and corrected during testing.

100 updates of 500 retained rows took 358 ms with the original implementation and 5 ms with the new implementation in local optimized fixtures. This measures this specific rendering task, not total application CPU/GPU usage or real stream throughput.

The combined Twitch/YouTube chat fixture passes with the native Wayland backend. Address/undefined-behavior sanitizer tests pass (Qt leak detection disabled), as does the simulated platform-flow suite with its local OAuth callback. No real messages or moderation actions were sent.

The local package passed all 237 workspace checks and 35 recording/local-stream checks under Wayland. Recording and received stream fully decoded; test applications exited normally with zero OBS memory leaks. The system-wide local2 installation completed with normal administrator confirmation. Package integrity and the installed module match were verified. All 237 workspace checks passed again on the installed app under Wayland; it exited with status zero and reported zero OBS memory leaks. Layout and public version remain unchanged.
