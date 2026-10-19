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
