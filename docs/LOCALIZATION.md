# Languages

Penguin Studio opens in English on a new installation. Select another language
in **Settings → General → Language**. All 77 inherited OBS frontend locale files
and the upstream plugin locale catalogues are retained.

The new workspace, source inspector and native modules **Broadcast destinations**
and **Stream details** ship complete strings for these languages:

| Locale | Language |
| --- | --- |
| `en-US` | English (default and fallback) |
| `de-DE` | German |
| `fr-FR` | French |
| `es-ES` | Spanish |
| `pt-BR` | Brazilian Portuguese |
| `it-IT` | Italian |
| `ja-JP` | Japanese |
| `zh-CN` | Simplified Chinese |

For other OBS languages, new Penguin text falls back to English. Existing OBS
translations continue to apply to inherited features. The project does not
claim complete translation of new features into every upstream language.

Translations were added with the initial Penguin Studio implementation. Native
speaker review and visual checks for clipping remain release validation work.
Keep the product name, platform names, protocol names, and format placeholders
such as `%1` intact when translating. Preserve the distinction between a
portrait crop or fit of the main program and an independently editable scene.

Run `python3 scripts/check-penguin-locales.py` to verify key coverage, valid quoted
values, duplicate keys, non-empty translations, and matching placeholders.
Contribute changes through the Penguin Studio repository; upstream OBS Crowdin
does not manage the new Penguin modules.

## Product names in inherited dialogs

Visible application references in startup, restart, crash, remux, settings and
plugin dialogs use Penguin Studio across the inherited locale catalogues.
Locale keys, format placeholders, protocol identifiers and API names remain
unchanged. Browser and WebSocket translations are applied as reviewed patches
at CMake configure time, preserving the pinned submodule revisions. Patch checks
fail if an upstream update requires review, and repeated configuration is safe.

OBS remains visible where it identifies an import source, the underlying engine,
upstream supporters, authors, licenses or the actual third-party log-upload
service. These are not Penguin Studio service claims. The Help menu points to
Penguin Studio issues; its support label is translated in the eight Penguin
languages, with English fallback for other locales. The About contribution link
uses this repository's `main` branch. OAuth completion pages use Penguin Studio
text without loading the OBS logo from a remote server.

`scripts/check-branding.py` rejects stale application names in frontend locale
values while allowing the explicit attribution, service and importer entries.
