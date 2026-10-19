# SPDX-License-Identifier: GPL-2.0-or-later
"""Check the maintained dependency patch without editing the real submodule."""
from pathlib import Path
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="penguin-websocket-patch-") as directory:
    root = Path(directory)
    target = root / "plugins/obs-websocket"
    for name in ("src/eventhandler/EventHandler.cpp", "src/eventhandler/EventHandler.h"):
        destination = target / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(subprocess.check_output([
            "git", "-C", str(repo / "plugins/obs-websocket"), "show", f"HEAD:{name}"
        ]))
    wrapper = root / "verify.cmake"
    wrapper.write_text(
        f'set(CMAKE_SOURCE_DIR "{repo}")\n'
        f'set(CMAKE_CURRENT_SOURCE_DIR "{root}/plugins")\n'
        f'include("{repo}/cmake/common/penguin_websocket_patch.cmake")\n'
    )
    command = ["cmake", "-P", str(wrapper)]
    subprocess.run(command, check=True, capture_output=True)
    source = target / "src/eventhandler/EventHandler.cpp"
    first = source.read_bytes()
    assert b"case OBS_FRONTEND_EVENT_EXIT:" in first
    subprocess.run(command, check=True, capture_output=True)
    assert source.read_bytes() == first
    source.write_text("incompatible upstream source\n")
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode != 0 and "Review the Penguin WebSocket" in result.stderr
    print("PASS: patch applies, is idempotent, and rejects incompatible source")
