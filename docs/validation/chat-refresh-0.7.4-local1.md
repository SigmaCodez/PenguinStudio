# Local chat improvements — 0.7.4 local1

This is a local review build; no public product-version change or GitHub publication is authorized for these changes.

## Changes

- Integrated YouTube reload now invalidates and cancels previous requests, stops polling and disables send until a new chat is loaded. Broadcast switching cancels obsolete work too.
- Each view tracks only its own pending replies; disconnect no longer aborts unrelated requests belonging to a shared transport. Weak reply references tolerate reply destruction.
- One message request can be in flight at a time. Retry delay saturates at 60 seconds without an ever-growing failure counter.
- A bounded visible-message ID index replaces repeated scans of the 500-row widget history. The recent 2,048-ID deduplication cache remains, including protection against deleted messages reappearing.

## Local validation

The new reload test failed against the original release header because the old message request remained active; it passes after the fix. Tests also cover unrelated requests, failed reloads, duplicate poll suppression, bounded history, moderation replay and cleanup. Existing Twitch/YouTube moderation and session tests pass.

A synthetic run of 2,500 messages plus 500 duplicate messages took 37 ms before and 7 ms after, with the same optimized build settings. This is a single local microbenchmark, not a whole-app CPU/GPU or live-platform performance claim. The address/undefined-behavior sanitizer run passed (Qt leak detection disabled); it took 21 ms with instrumentation.

The platform-flow suite passed using in-memory HTTP and a local OAuth callback. The restricted sandbox cannot bind that callback, so this suite ran with host loopback access. No real account actions, chat sends or moderation were performed.

The package passed 237 workspace assertions under Wayland and exited normally with zero OBS memory leaks. All 35 recording/local-loopback stream assertions also passed, including audio and privacy shielding; recorded and received video fully decoded. Installation verification is pending normal administrator confirmation.
