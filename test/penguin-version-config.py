#!/usr/bin/env python3
"""Exercise the real OBS version helper in isolated, temporary Git repositories.

Run with Python 3, Git, and CMake on PATH. No application dependencies are needed.
The repository under test and its Git metadata are never modified.
"""

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def run(*args, cwd=None, success=True):
    result = subprocess.run(args, cwd=cwd, text=True, capture_output=True)
    if success and result.returncode:
        raise RuntimeError(f"Command failed: {args!r}\n{result.stdout}\n{result.stderr}")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", default="cmake")
    args = parser.parse_args()
    repository = Path(__file__).resolve().parents[1]
    helper = repository / "cmake/common/versionconfig.cmake"
    bootstrap = (repository / "cmake/common/bootstrap.cmake").read_text()
    default = re.search(r"^set\(_obs_default_version[^\n]+\)$", bootstrap, re.MULTILINE)
    if default is None:
        raise RuntimeError("Cannot locate the production bootstrap version default.")

    cases = [
        ("tagless-root", None, None, "33.0.0", "33.0.0", 0, 0),
        ("obs-release-tag", "33.0.0", None, "33.0.0", "33.0.0", 0, 0),
        ("penguin-release-tag", "penguin-v0.2.0", None, "33.0.0", "33.0.0", 0, 0),
        ("explicit-override", None, "33.1.4", "33.1.4", "33.1.4", 0, 0),
        ("override-wins-over-tag", "33.0.0", "33.1.4-rc2", "33.1.4-rc2", "33.1.4", 2, 0),
        ("obs-release-candidate", "33.0.0-rc3", None, "33.0.0-rc3", "33.0.0", 3, 0),
        ("obs-beta", "33.0.0-beta2", None, "33.0.0-beta2", "33.0.0", 0, 2),
        ("source-archive", None, None, "33.0.0", "33.0.0", 0, 0),
    ]

    with tempfile.TemporaryDirectory(prefix="penguin-version-test-") as temporary:
        for name, tag, override, version, canonical, rc, beta in cases:
            case = Path(temporary) / name
            source = case / "source"
            build = case / "build"
            source.mkdir(parents=True)
            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(PenguinVersionFixture LANGUAGES NONE)\n"
                f"{default.group(0)}\n"
                "set(_obs_release_candidate 0)\nset(_obs_beta 0)\n"
                f'include("{helper.as_posix()}")\n'
                'file(WRITE "${CMAKE_BINARY_DIR}/actual.txt" '
                '"${OBS_VERSION}|${OBS_VERSION_CANONICAL}|${OBS_RELEASE_CANDIDATE}|${OBS_BETA}\\n")\n'
            )
            if name != "source-archive":
                run("git", "init", "--quiet", "--initial-branch=fixture", str(source))
                run("git", "add", "CMakeLists.txt", cwd=source)
                run("git", "-c", "user.name=Penguin test", "-c", "user.email=test@example.invalid",
                    "-c", "core.hooksPath=/dev/null", "commit", "--quiet", "--no-gpg-sign",
                    "-m", "Temporary version fixture", cwd=source)
                if tag:
                    run("git", "tag", "--no-sign", tag, cwd=source)
            command = [args.cmake, "-S", str(source), "-B", str(build)]
            if override:
                command.append(f"-DOBS_VERSION_OVERRIDE={override}")
            run(*command)
            actual = (build / "actual.txt").read_text().strip()
            expected = f"{version}|{canonical}|{rc}|{beta}"
            if actual != expected:
                raise AssertionError(f"{name}: expected {expected!r}, got {actual!r}")
            print(f"PASS: {name}: {actual}")

        # A caller error must still fail instead of silently selecting a default.
        invalid = run(args.cmake, "-S", str(source), "-B", str(case / "invalid-build"),
                      "-DOBS_VERSION_OVERRIDE=not-a-version", success=False)
        if invalid.returncode == 0 or "Invalid version supplied" not in invalid.stderr:
            raise AssertionError("Invalid explicit override was not rejected.")
        print("PASS: invalid explicit override is rejected")

    print("All 9 version configuration cases passed.")


if __name__ == "__main__":
    main()
