#!/usr/bin/env python3
"""Check complete key coverage and placeholders for Penguin Studio translations."""

import json
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
LANGUAGES = ("de-DE", "fr-FR", "es-ES", "pt-BR", "it-IT", "ja-JP", "zh-CN")


def load(path, prefix=None):
    result = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        if prefix is not None and not line.startswith(prefix):
            continue
        key, value = line.split("=", 1)
        assert key not in result, f"{path}:{number}: duplicate key {key}"
        result[key] = json.loads(value)
        assert result[key].strip(), f"{path}:{number}: empty translation"
    return result


def main():
    catalogues = [
        (module, ROOT / "plugins" / module / "data" / "locale", None)
        for module in ("penguin-outputs", "penguin-platforms")
    ]
    catalogues.append(("workspace", ROOT / "frontend" / "data" / "locale", "Penguin."))
    for module, folder, prefix in catalogues:
        english = load(folder / "en-US.ini", prefix)
        assert english, f"{module}: missing English catalogue"
        for language in LANGUAGES:
            translated = load(folder / f"{language}.ini", prefix)
            assert translated.keys() == english.keys(), (
                module, language, set(english) ^ set(translated)
            )
            for key in english:
                assert sorted(re.findall(r"%\d+", translated[key])) == sorted(
                    re.findall(r"%\d+", english[key])
                ), (module, language, key)
        print(f"{module}: {len(english)} keys complete in English and {len(LANGUAGES)} translations.")


if __name__ == "__main__":
    main()
