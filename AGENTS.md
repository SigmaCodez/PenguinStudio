# Penguin Studio publication maintenance

- Keep public product text, release notes, captions and current GUI previews in English.
- Every version change must update the README, current installation examples and current GUI previews. Use the real UI of that version, not recolored old images or mockups.
- Update `docs/images/current/manifest.json` from verified captures and run `python3 scripts/check-publication.py` before publishing. Visually inspect every new preview; metadata cannot establish its language or accuracy by itself.
- Use an isolated English demo profile and synthetic media. Never publish private webcam recordings, desktop contents, account identities or credentials.
- Preserve historical test evidence with its original version/locale, clearly separated from current product previews. Preserve author and license notices.
- Update the current GitHub release description after final package tests. Do not rewrite older releases as if they were the current version.

## Local review before release

- Implement and validate improvements locally first, then install the latest validated local build so the user can review it. Keep local build identifiers distinct from public release versions.
- Local installation is authorized after successful checks; GitHub publication still requires explicit release approval. Never request or store the user's administrator password.
- Do not create or publish a GitHub release, push a release tag, or increment the product version until the user explicitly approves that release after local review.
- After approval, use a small version increment appropriate to the approved change and complete the publication checks above.

- Keep system-wide DEB installation for local builds. The user declined a per-user installation. Any passwordless helper must be explicitly approved, narrowly scoped and must not store passwords. Never expand its approved payload scope silently.
