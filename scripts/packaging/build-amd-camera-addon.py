#!/usr/bin/env python3
"""Build the optional Ubuntu 26.04 RX 9070-series camera runtime package."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


def build(runtime, output):
    version = (Path(__file__).resolve().parents[2] / "packaging/version.txt").read_text().strip()
    metadata = json.loads((runtime / "runtime-manifest.json").read_text())
    if metadata.get("packaged_architecture") != "gfx1201":
        raise ValueError("This add-on packages only the tested gfx1201 runtime")
    for name in ("libonnxruntime.so.1", "libonnxruntime_providers_migraphx.so", "rocm/lib/libamdhip64.so"):
        if not (runtime / name).is_file():
            raise ValueError(f"Incomplete runtime: {name}")
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="penguin-amd-deb-", dir=output) as temporary:
        root = Path(temporary)
        destination = root / "usr/lib/x86_64-linux-gnu/obs-modules/penguin-camera-gpu"
        shutil.copytree(runtime, destination, symlinks=True)
        control = root / "DEBIAN"
        control.mkdir()
        (control / "control").write_text(f"""Package: penguin-studio-camera-amd-gfx1201
Version: {version}-1+ubuntu26.04
Architecture: amd64
Maintainer: Penguin Studio contributors
Section: video
Priority: optional
Depends: penguin-studio (>= {version}), libc6 (>= 2.39), libstdc++6, libgcc-s1, libnuma1, libelf1t64, libdrm2, libdrm-amdgpu1, libpciaccess0, libtinfo6, zlib1g, libzstd1
Description: Optional Penguin Studio camera GPU runtime for AMD RX 9070 series
 Private ROCm and MIGraphX libraries for primary background matting.
 Tested on RX 9070 XT under Ubuntu 26.04.1 and Wayland.
 Hand preservation remains on CPU. No system GPU driver is replaced.
""")
        artifact = output / f"penguin-studio-camera-amd-gfx1201_{version}-1+ubuntu26.04_amd64.deb"
        subprocess.run(["dpkg-deb", "--root-owner-group", "-Zxz", "-z6", "--build", str(root), str(artifact)], check=True)
        if artifact.stat().st_size >= 2_000_000_000:
            raise ValueError("Runtime package exceeds the release asset size limit")
        print(artifact)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    build(args.runtime.resolve(), args.output.resolve())
