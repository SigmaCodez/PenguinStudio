#!/usr/bin/env python3
"""Check Penguin Studio branding, metadata, and configuration boundaries."""

from pathlib import Path
import configparser
import hashlib
import re
import struct
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
APP_ID = "io.github.SigmaCodez.PenguinStudio"


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def main():
    logo = (ROOT / "assets/branding/penguin-studio.png").read_bytes()
    assert logo[:8] == b"\x89PNG\r\n\x1a\n", "Logo must be a PNG"
    width, height, depth, color_type = struct.unpack(">IIBB", logo[16:26])
    assert width == height and width >= 512
    assert depth == 8 and color_type == 6, "Logo must retain its RGBA channel"
    resource = (ROOT / "frontend/forms/images/penguin-studio.png").read_bytes()
    assert logo == resource, "Qt resource differs from the original generated logo"

    for form in ("frontend/forms/obs.qrc", "frontend/forms/OBSAbout.ui"):
        ET.fromstring(read(form))
    assert "images/penguin-studio.png" in read("frontend/forms/obs.qrc")

    desktop = configparser.ConfigParser(interpolation=None, strict=True)
    desktop.optionxform = str
    desktop.read(ROOT / f"frontend/cmake/linux/{APP_ID}.desktop", encoding="utf-8")
    entry = desktop["Desktop Entry"]
    assert entry["Name"] == "Penguin Studio"
    assert entry["Exec"] == "penguin-studio" and entry["Icon"] == APP_ID
    metadata = ET.fromstring(read(f"frontend/cmake/linux/{APP_ID}.metainfo.xml.in"))
    assert metadata.findtext("id") == APP_ID
    assert metadata.findtext("launchable") == APP_ID + ".desktop"

    theme = read("frontend/data/themes/Penguin.ovt")
    assert f"id: '{APP_ID}.Dark'" in theme
    assert "extends: 'com.obsproject.Yami'" in theme
    assert f'"{APP_ID}.Dark"' in read("frontend/cmake/templates/ui-config.h.in")

    # Direct filesystem concatenations must not bypass the isolated config root.
    isolated = (
        "frontend/OBSApp.cpp",
        "frontend/OBSApp_Themes.cpp",
        "frontend/widgets/OBSBasic_Profiles.cpp",
        "frontend/widgets/OBSBasic_SceneCollections.cpp",
        "frontend/plugin-manager/PluginManager.cpp",
        "frontend/utility/CrashHandler.cpp",
    )
    for path in isolated:
        assert not re.search(r'"/?obs-studio(?:/|")', read(path)), path
    assert '"obs-studio/basic/scenes/"' in read("frontend/importer/OBSImporter.cpp")
    assert '/tmp/penguin-studio.lock.' in read("frontend/utility/platform-x11.cpp")
    assert "GetPreferredLocales()" not in read("frontend/OBSApp.cpp")

    keys = []
    for line in read("frontend/data/locale/en-US.ini").splitlines():
        if line and not line.startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            assert value.startswith('"') and value.endswith('"'), key
            keys.append(key)
    assert len(keys) == len(set(keys)), "Duplicate English locale keys"
    assert "PenguinStudio.About" in keys
    locales = list((ROOT / "frontend/data/locale").glob("*.ini"))
    assert len(locales) >= 50, "Inherited locale catalogue is incomplete"
    print(f"Branding checks passed: {width}x{height} RGBA logo, {len(locales)} locales.")
    print("Logo SHA-256:", hashlib.sha256(logo).hexdigest())


if __name__ == "__main__":
    main()
