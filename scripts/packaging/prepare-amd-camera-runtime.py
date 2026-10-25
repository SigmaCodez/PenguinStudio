#!/usr/bin/env python3
"""Stage a pinned, optional AMD camera runtime without changing system drivers.

Run on a local Unix filesystem with at least 15 GB free. The resulting directory
can be supplied as PENGUIN_CAMERA_GPU_RUNTIME_DIR when building Penguin Studio.
All GPU families shipped by AMD are retained. This does not install packages.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request
import zipfile


def download(item, target):
    urllib.request.urlretrieve(item["url"], target)
    with target.open("rb") as stream:
        actual = hashlib.file_digest(stream, "sha256").hexdigest()
    if actual != item["sha256"]:
        raise ValueError(f"Runtime checksum mismatch: {target.name}")


def stage(destination, architecture=None):
    if destination.exists():
        raise ValueError("Use a new output directory; existing runtime bundles are never overwritten")
    manifest = Path(__file__).resolve().parents[2] / "packaging/amd-camera-runtime.json"
    data = json.loads(manifest.read_text())
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="penguin-gpu-", dir=destination.parent) as temporary:
        root = Path(temporary)
        extracted = root / "extracted"
        extracted.mkdir()
        wheel = root / "runtime.whl"
        download(data["wheel"], wheel)
        bundle = root / "bundle"
        bundle.mkdir()
        notices = bundle / "licenses"
        notices.mkdir()
        with zipfile.ZipFile(wheel) as archive:
            for entry in archive.infolist():
                name = Path(entry.filename).name
                if entry.filename.startswith("onnxruntime/capi/libonnxruntime") and ".so" in name:
                    with archive.open(entry) as source, (bundle / name).open("wb") as target:
                        shutil.copyfileobj(source, target)
                elif "license" in entry.filename.lower() or "notice" in name.lower():
                    if not entry.is_dir():
                        (notices / name).write_bytes(archive.read(entry))
        (bundle / "libonnxruntime.so.1").symlink_to("libonnxruntime.so.1.23.2")
        for item in data["packages"]:
            package = root / (item["name"] + ".deb")
            download(item, package)
            process = subprocess.Popen(["dpkg-deb", "--fsys-tarfile", str(package)], stdout=subprocess.PIPE)
            try:
                with tarfile.open(fileobj=process.stdout, mode="r|") as archive:
                    def selected(member, target):
                        name = member.name.removeprefix("./")
                        if not (name.startswith("opt/rocm-") or name.startswith("usr/share/doc/")):
                            return None
                        if architecture and ("/rocblas/library/" in name or "/hipblaslt/library/" in name):
                            families = re.findall(r"gfx[0-9a-f]+", name)
                            if families and architecture not in families:
                                return None
                        return tarfile.data_filter(member, target)
                    archive.extractall(extracted, filter=selected)
            finally:
                process.stdout.close()
                if process.wait() != 0:
                    raise RuntimeError(f"Cannot extract {item['name']}")
            package.unlink()
        shutil.move(extracted / "opt" / ("rocm-" + data["rocm"]), bundle / "rocm")
        if (extracted / "usr/share/doc").exists():
            shutil.copytree(extracted / "usr/share/doc", notices / "amd-packages", symlinks=True)
        data["packaged_architecture"] = architecture or "all"
        (bundle / "runtime-manifest.json").write_text(json.dumps(data, indent=2) + "\n")
        bundle.rename(destination)
    print(f"Verified runtime staged at {destination}; no system packages were installed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--architecture", choices=["gfx1201"], help="Keep only the tested RX 9070 series BLAS kernels")
    args = parser.parse_args()
    stage(args.output.resolve(), args.architecture)
