#!/usr/bin/env python3
"""Download and verify the Linux CEF bundle pinned by OBS's build presets."""

import hashlib
import json
import platform
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path


def main():
    root = Path(__file__).resolve().parents[2]
    destination = Path(sys.argv[1] if len(sys.argv) > 1 else root / ".deps/cef").resolve()
    machine = platform.machine()
    target = {"x86_64": "ubuntu-x86_64", "aarch64": "ubuntu-aarch64"}.get(machine)
    if not target:
        raise SystemExit(f"No upstream CEF bundle is pinned for {machine}")
    presets = json.loads((root / "CMakePresets.json").read_text())
    dependency = next(item for item in presets["configurePresets"] if item["name"] == "dependencies")
    cef = dependency["vendor"]["obsproject.com/obs-studio"]["dependencies"]["cef"]
    revision = cef.get("revision", {}).get(target)
    suffix = f"_v{revision}" if revision else ""
    filename = f"cef_binary_{cef['version']}_linux_{machine}{suffix}.tar.xz"
    archive = destination.parent / filename
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        temporary = archive.with_suffix(".download")
        with urllib.request.urlopen(f"{cef['baseUrl']}/{filename}", timeout=120) as response:
            with temporary.open("wb") as output:
                shutil.copyfileobj(response, output)
        temporary.replace(archive)
    digest = hashlib.sha256()
    with archive.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    if digest.hexdigest() != cef["hashes"][target]:
        raise SystemExit(f"Checksum mismatch: remove {archive} and retry")
    destination.mkdir(parents=True, exist_ok=True)
    subprocess.run(["tar", "-xJf", str(archive), "--strip-components=1", "-C", str(destination)], check=True)
    print(destination)


if __name__ == "__main__":
    main()
