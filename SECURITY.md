# Penguin Studio security policy

Penguin Studio is an independent project maintained by SigmaCodez and based on
OBS Studio. This policy applies to Penguin Studio, its packages and its own
code changes. Penguin Studio is not operated or supported by the OBS Project.

## Report a vulnerability privately

Use [GitHub private vulnerability reporting](https://github.com/SigmaCodez/PenguinStudio/security/advisories/new)
to contact the Penguin Studio maintainer. Private reporting is enabled for this
repository. Do not publish exploit details, stream keys, access tokens, client
secrets or private recordings in public issues, discussions or build logs.

Include the affected version or commit, package format, operating system,
reproduction steps, expected impact and a minimal proof of concept where possible.
Use test accounts and synthetic data; redact credentials and personal information.

## Supported development versions

Penguin Studio is currently a development preview. Security fixes target the
latest source on `main` and subsequent builds. Older preview packages do not
have a separate security maintenance branch. Check the source revision recorded
for your package when reporting a problem.

## Coordinated disclosure

Please allow time to investigate and coordinate disclosure through the private
report. Response and remediation times depend on the issue and maintainer
availability; there is no guaranteed response deadline or paid bug bounty.

Test only systems and accounts you own or have permission to test. Avoid
accessing other users' data, disrupting services or making public test streams
with real credentials. This policy does not authorize testing Twitch, YouTube,
GitHub, OBS infrastructure or any other third-party service.

## Upstream components

Reports affecting Penguin Studio can be submitted here even when an upstream
component may be involved. A vulnerability reproducible in unmodified OBS Studio
should also be reported through the [OBS Project's own security policy](https://github.com/obsproject/obs-studio/security/policy).
Other dependencies have their own maintainers and disclosure processes.

OBS contact addresses and Discord are not Penguin Studio support channels.
Source attribution and licenses remain in `AUTHORS`, `COPYING` and the individual
source files. The original upstream security policy is available in the
[pinned OBS source](https://github.com/obsproject/obs-studio/blob/ba2f32bdf791005443988a4955e963663e16b1ed/SECURITY.md).

For installation failures and other non-security bugs, open a
[Penguin Studio issue](https://github.com/SigmaCodez/PenguinStudio/issues), including
sanitized logs and your operating-system and package versions.
