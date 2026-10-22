#!/usr/bin/env python3
"""Validate GPU output from penguin-camera-gui; no image dependencies needed."""
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
report = json.loads((root / "result.json").read_text())
assert report["visible"] and report["ai_status"] == 2, report
if "--inspector" in sys.argv:
    assert report.get("inspector_used"), report


def pixel(name, x, y):
    width, height = map(int, report[name].split("x"))
    data = (root / (name + ".rgba")).read_bytes()
    assert len(data) == width * height * 4
    offset = (y * width + x) * 4
    return tuple(data[offset:offset + 4])


assert pixel("rectangle", 0, 0) == (0, 255, 0, 255)
for name in ("circle", "oval", "rounded", "green-key", "blue-key"):
    assert pixel(name, 0, 0)[3] == 0, name
    assert pixel(name, 320, 180) == (255, 0, 0, 255), name
# A circle has equal pixel radii; an oval fills the wide source.
assert pixel("circle", 20, 180)[3] == 0
assert pixel("oval", 20, 180)[3] == 255
assert pixel("rounded", 20, 180)[3] == 255
assert pixel("automatic-circle", 0, 0)[3] == 0
assert pixel("disabled", 0, 0)[3] == 255
saved = json.loads((root / "source.json").read_text())
filters = [f for f in saved["filters"] if f["id"] == "penguin_camera_filter"]
assert len(filters) == 1
assert filters[0]["settings"]["shape"] == 3
assert filters[0]["settings"]["background"] == 2
print("Camera shapes, green/blue keys, local inference, disabling and serialized settings passed")
