# Contributing to Penguin Studio

Penguin Studio is an independent project maintained by SigmaCodez. Open changes
against the `main` branch of this repository. Questions or changes specific
to Penguin Studio belong here; the OBS Project does not support this application.

## Source checkout and builds

The repository includes the application source and pinned submodules. Clone the
`main` branch recursively:

```sh
git clone --recurse-submodules --branch main https://github.com/SigmaCodez/PenguinStudio.git
cd PenguinStudio
```

Follow the [build and package guide](docs/PACKAGING.md). Build on a filesystem
supporting Unix permissions and symbolic links; staging outside an exFAT/NTFS
checkout may be required. See [architecture](docs/ARCHITECTURE.md),
[platform setup](docs/PLATFORMS.md), [release checks](docs/VERIFICATION.md) and
[brand assets](docs/branding/README.md) for implementation details.

Importing OBS changes is an optional maintainer task. The
[upstream update instructions](docs/HISTORY.md#future-upstream-changes) explain
how to add the source remote, review patches against the pinned baseline and
preserve attribution. Users installing or updating Penguin Studio packages do
not need an OBS remote or a source checkout.

## Before changing code

- Use American English for code, comments, documentation and default UI text.
- Follow [CODESTYLE.md](CODESTYLE.md) and the checked-in formatter configuration.
- Keep OBS API/ABI names, license headers and author attribution intact.
- Keep product code in the Penguin modules where practical, making upstream
  OBS updates easier to review and import.
- Never commit account credentials, stream keys, access/refresh tokens or private
  recordings. Use synthetic sources and local receivers for automated tests.
- Add user-facing strings to the English locale and maintain the Penguin locale
  catalogues described in [LOCALIZATION.md](docs/LOCALIZATION.md).

## Review and validation

Explain the problem, resulting behavior and verification in each pull request.
Use focused changes and disclose untested environments or external account flows.
All contributors are responsible for understanding and reviewing submitted code,
including any work created with automated tools. Generated code is not a substitute
for compilation, runtime testing or license review.

Run the relevant checks from [VERIFICATION.md](docs/VERIFICATION.md). In particular:

```sh
python3 scripts/check-branding.py
python3 scripts/check-penguin-locales.py
```

For output or platform changes, run the C++ safety/OAuth tests and the local stream
integration test when a suitable graphics session is available. Package changes
must build on their declared target distribution; do not describe untested distro
or hardware combinations as supported.

Follow the [Penguin Studio Code of Conduct](COC.rst) and use a respectful,
constructive tone in discussions. Security-sensitive reports
must not expose another person's credentials or personal data in public issues.

## Upstream and licensing

The repository starts from an independent source snapshot. Read
[source provenance and history](docs/HISTORY.md) before importing upstream
changes; the new Git history has no shared merge base with OBS.

Changes intended for OBS itself must follow the OBS Project's own contribution
policies and be submitted separately. Its original guidelines remain available in
[the pinned upstream source](https://github.com/obsproject/obs-studio/blob/ba2f32bdf791005443988a4955e963663e16b1ed/CONTRIBUTING.md).

Original Penguin contributions are licensed under GPL-2.0-or-later, matching the
derived application. See [COPYING](COPYING). Preserve the licenses of third-party
components and provide corresponding source with distributed binaries.
