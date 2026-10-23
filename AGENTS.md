# Penguin Studio publication maintenance

- Keep public product text, release notes, captions and current GUI previews in English.
- Every version change must update the README, current installation examples and current GUI previews. Use the real UI of that version, not recolored old images or mockups.
- Update `docs/images/current/manifest.json` from verified captures and run `python3 scripts/check-publication.py` before publishing. Visually inspect every new preview; metadata cannot establish its language or accuracy by itself.
- Use an isolated English demo profile and synthetic media. Never publish private webcam recordings, desktop contents, account identities or credentials.
- Preserve historical test evidence with its original version/locale, clearly separated from current product previews. Preserve author and license notices.
- Update the current GitHub release description after final package tests. Do not rewrite older releases as if they were the current version.
