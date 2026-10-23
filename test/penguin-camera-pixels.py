#!/usr/bin/env python3
"""Validate GPU output from penguin-camera-gui; no image dependencies needed."""
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
report = json.loads((root / "result.json").read_text())
assert report["visible"] and report["ai_status"] == 2, report
assert report["pair_lifecycle"] and report["unrelated_filter_preserved"], report
if "--inspector" in sys.argv:
    assert report.get("inspector_used"), report
    assert report.get("background_controls_available"), report
    assert report["rvm_backend"] and report["automatic_undo_removed_matting"] and report["automatic_redo_restored_matting"], report
    assert report["controls_enabled"] and report["controls_visible"], report
    assert report["selected_rows"] == 1, report
    assert report["default_undo_shape"] == 0, report
    assert report["default_undo_radius"] == 25, report
    assert report["default_redo_shape"] == 1, report


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
assert pixel("automatic", 0, 0)[3] == 0
assert pixel("automatic-first-frame", 0, 0)[3] == 0
assert pixel("automatic-circle", 0, 0)[3] == 0
assert pixel("disabled", 0, 0)[3] == 255
saved = json.loads((root / "source.json").read_text())
filters = [f for f in saved["filters"] if f["id"] == "penguin_camera_filter"]
assert len(filters) == 1
assert filters[0]["settings"]["shape"] == 3
assert filters[0]["settings"]["background"] == 2
print("Camera shapes, green/blue keys, local inference, disabling and serialized settings passed")

assert report["blur_strength"] > 0 and report["image_applied"] and report["background_restored"], report
assert pixel("blurred", 0, 0)[3] == 0
# A 4px black/white checker must become smooth, opaque grey away from the shape edge.
for x in range(100, 120):
    r, g, b, a = pixel("blurred", x, 30)
    assert 50 < r < 205 and abs(r-g) < 5 and abs(g-b) < 5 and a == 255, (x, r, g, b, a)
assert pixel("custom-background", 0, 0)[3] == 0
assert pixel("custom-background", 100, 180) == (0, 0, 255, 255)
print("Automatic blur, custom image, rounded corners and background undo/restore passed")
