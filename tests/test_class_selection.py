#!/usr/bin/env python3
"""Exercise TEST_CLASS in the real Makefile with temporary dependencies."""

from itertools import product
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile


PROJECT = Path(__file__).resolve().parents[1]
BASE_RECIPES = ("tests/test_fkpt_regression.py", "tests/test_dgp_regression.py")
CLASS_RECIPE = "tests/test_class_workflow.py"
MISSING_MESSAGES = {
    "CLASS_DIR": "Missing readable CLASS archive",
    "CLASS_BIN": "Missing CLASS executable",
    "CLASSY_PATH": "Unable to import classy",
}


def dependency_fixtures(scratch):
    class_dir = scratch / "CLASS files"
    class_dir.mkdir()
    (class_dir / "libclass.a").write_bytes(b"!<arch>\n")
    executable = class_dir / "class"
    executable.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
    executable.chmod(0o755)
    nonexecutable = class_dir / "class-without-execute-permission"
    nonexecutable.touch()
    nonexecutable.chmod(0o644)

    usable_classy = scratch / "usable-classy"
    broken_classy = scratch / "broken-classy"
    marker = "from pathlib import Path\nPath(__file__).with_suffix('.imported').touch()\n"
    for directory, body in (
        (usable_classy, "class Class: pass\n"),
        (broken_classy, "raise ImportError('classy import unavailable in this fixture')\n"),
    ):
        directory.mkdir()
        (directory / "classy.py").write_text(marker + body, encoding="utf-8")

    # Each pair gives the unavailable and available paths for a dependency.
    return {
        "CLASS_DIR": (scratch / "missing-archive", class_dir),
        "CLASS_BIN": (nonexecutable, executable),
        "CLASSY_PATH": (broken_classy, usable_classy),
    }


def check_selection(make, scratch, env, mode, settings, missing):
    arguments = [f"{name}={path}" for name, path in settings.items()]
    if mode is not None:
        arguments.append(f"TEST_CLASS={mode}")
    probe_marker = settings["CLASSY_PATH"] / "classy.imported"
    probe_marker.unlink(missing_ok=True)
    result = subprocess.run([*make, *arguments, "test"], cwd=scratch, env=env,
                            capture_output=True, text=True, timeout=30)
    details = (
        f"TEST_CLASS={mode or 'unset'}, unavailable {sorted(missing)}, "
        f"exit status {result.returncode}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    should_fail = mode == "1" and bool(missing)
    if should_fail:
        assert result.returncode > 0 and "TEST_CLASS=1 requires" in result.stderr, details
        reported = {name for name, message in MISSING_MESSAGES.items() if message in result.stderr}
        assert reported == missing, details
        assert all(str(settings[name]) in result.stderr for name in missing), details
    else:
        assert result.returncode == 0, details

    should_run_class = mode != "0" and not missing
    assert result.stdout.count(CLASS_RECIPE) == int(should_run_class), details
    for recipe in BASE_RECIPES:
        assert result.stdout.count(recipe) == int(not should_fail), details
    if mode == "0":
        assert not probe_marker.exists(), details


def main():
    env = os.environ.copy()
    for name in ("MAKEFLAGS", "MAKEOVERRIDES", "MFLAGS", "MAKELEVEL", "GNUMAKEFLAGS",
                 "MAKEFILES", "TEST_CLASS", "PYTHONPATH"):
        env.pop(name, None)

    # Omit the build hooks and this regression from the nested dry run.
    make = [
        *shlex.split(os.environ.get("MAKE", "make")), "-n", "-s",
        "-o", "fkpt2", "-o", "test-python-extension", "-o", "test-class-multipoles-both",
        "-o", "test-class-selection",
        "ADDONSON=0", "CLASSLIBON=0", "OPENMPMACHINE=0", "USEGSL=0", "CFITSIOON=0",
        f"TEST_PYTHON={shlex.quote(sys.executable)} -S",
    ]
    with tempfile.TemporaryDirectory(prefix="fkpt2-class-selection-") as directory:
        scratch = Path(directory)
        for name in ("Makefile", "Makefile_settings", "Makefile_machine"):
            shutil.copyfile(PROJECT / name, scratch / name)
        fixtures = dependency_fixtures(scratch)
        for availability in product((False, True), repeat=len(fixtures)):
            settings = {
                name: paths[present]
                for (name, paths), present in zip(fixtures.items(), availability)
            }
            missing = {name for name, present in zip(fixtures, availability) if not present}
            for mode in (None, "auto", "0", "1"):
                check_selection(make, scratch, env, mode, settings, missing)

    print("TEST_CLASS modes and required dependency diagnostics passed in all eight availability combinations")


if __name__ == "__main__":
    main()
