# Offline platform controller checks

Build with `-DPENGUIN_BUILD_TESTS=ON`, then run:

```sh
QT_QPA_PLATFORM=offscreen build/plugins/penguin-platforms/penguin-platform-flow
```

The executable includes the production Qt controllers and replaces only HTTP
transport, browser launch and the OBS configuration/translation boundary. No
requests reach Twitch or Google. A Google callback is exercised through a real
ephemeral IPv4 loopback connection with synthetic credentials.

Checks cover Twitch authorization-pending and slow-down polling, token validation,
one-use refresh-token rotation, concurrent refresh/metadata operations, disconnect
and destruction cancellation, Google PKCE/code exchange, paginated broadcasts and
pagination cycles, selection/edit preservation, snippet field preservation, cached
title updates, revoked authorization, and public-ID-only persistence.

These tests do not verify provider consent, application registration, real account
permissions, TLS, quotas or successful live metadata changes. Those require
registered OAuth applications and explicitly authorized test accounts.
