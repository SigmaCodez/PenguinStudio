#!/usr/bin/env python3
"""Reject stale release references and untracked current preview images."""
import argparse
import hashlib
import json
import re
from pathlib import Path


def validate(root: Path, candidate: bool = False) -> None:
    version = (root / "packaging/version.txt").read_text().strip()
    current = root / "docs/images/current"
    manifest = json.loads((current / "manifest.json").read_text())
    pending = candidate and manifest.get("pending_version") == version
    if (manifest["version"] != version and not pending) or manifest["language"] != "en-US":
        raise ValueError("Capture and inspect current-version English GUI previews before release")
    if manifest.get("pending_version") and not candidate:
        raise ValueError("Replace pending previews with reviewed candidate captures before publication")
    if pending:
        print("Candidate build only: current-version English captures are still required before publication")
    expected = {"workspace-en.png", "camera-en.png", "stream-details-en.png"}
    if {image["file"] for image in manifest["images"]} != expected:
        raise ValueError("Current preview manifest must contain all three product views")
    for image in manifest["images"]:
        data = (current / image["file"]).read_bytes()
        if not data.startswith(b"\x89PNG\r\n\x1a\n") or hashlib.sha256(data).hexdigest() != image["sha256"]:
            raise ValueError(f"Preview content differs from reviewed manifest: {image['file']}")
    pages = ["README.md", "docs/PACKAGING.md", "docs/APPIMAGE-AUR.md",
             "docs/WORKSPACE.md", "docs/CAMERA-EFFECTS.md", "docs/PLATFORMS.md"]
    for page in pages:
        text = (root / page).read_text()
        references = re.findall(r"(?:penguin-v|penguin-studio[_-]|Penguin Studio )(\d+\.\d+\.\d+)", text)
        if any(value != version for value in references):
            raise ValueError(f"Outdated current-release reference in {page}")
        for target in re.findall(r"!\[[^\]]*\]\(([^)]+)\)", text):
            if "images/current/" not in target:
                raise ValueError(f"Product preview must use reviewed current images: {page}: {target}")
    if f"current source version is **{version}**" not in (root / "docs/PACKAGING.md").read_text():
        raise ValueError("Update the current source version in the package guide")
    if f"Current release: [Penguin Studio {version}]" not in (root / "README.md").read_text():
        raise ValueError("Update the README current release")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", action="store_true", help="Allow explicitly pending previews for non-publishing candidate builds only")
    validate(Path(__file__).resolve().parents[1], parser.parse_args().candidate)
    print("Current release references and reviewed English preview manifest match")
