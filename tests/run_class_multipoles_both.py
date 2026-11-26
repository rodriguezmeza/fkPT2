#!/usr/bin/env python3
"""Run the two CLASS multipole regressions in temporary source trees."""

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile


PROJECT = Path(__file__).resolve().parents[1]
BUILD_FILES = ("Makefile", "Makefile_settings", "Makefile_machine", "MANIFEST.in",
               "LICENSE", "README.md", "pyproject.toml", "setup.py")
SOURCE_DIRS = ("addons", "general_libs", "getparam", "include", "main",
               "python", "scripts", "source", "tests")


def copy_sources(destination):
    destination.mkdir()
    for name in BUILD_FILES:
        shutil.copy2(PROJECT / name, destination / name)
    ignore = shutil.ignore_patterns(".DS_Store", "__pycache__", "*.egg-info", "*.o", "*.so", "*.pyd")
    for name in SOURCE_DIRS:
        shutil.copytree(PROJECT / name, destination / name, ignore=ignore)


def absolute_paths(paths):
    return os.pathsep.join(str(Path(path).resolve()) for path in paths.split(os.pathsep))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--class-dir", type=Path, required=True)
    parser.add_argument("--class-bin", type=Path, required=True)
    parser.add_argument("--classy-path", required=True)
    args = parser.parse_args()

    settings = ["ADDONSON=1", "MODELFOLPSNUON=1", f"CLASS_DIR={args.class_dir.resolve()}"]
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE="1")
    make = [
        *shlex.split(env.get("MAKE", "make")),
        *settings,
        f"CLASS_BIN={args.class_bin.resolve()}",
        f"CLASSY_PATH={absolute_paths(args.classy_path)}",
        f"TEST_PYTHON={shlex.quote(sys.executable)}",
        "EXEC=fkpt2",
    ]
    if env.get("PYTHONPATH"):
        make.append(f"PYTHONPATH={absolute_paths(env['PYTHONPATH'])}")
    extension_make_args = shlex.split(env.get("FKPT2_MAKE_ARGS", ""))

    failed = False
    with tempfile.TemporaryDirectory(prefix="fkpt2-class-multipoles-both-") as tmp:
        root = Path(tmp)
        linked, disabled = root / "classlib", root / "noclasslib"
        copy_sources(linked)
        shutil.copytree(linked, disabled)
        for mode, checkout in ((1, linked), (0, disabled)):
            setting = f"CLASSLIBON={mode}"
            extension_args = shlex.join([*extension_make_args, *settings, setting])
            print(f"Running multipoles regression with {setting}", flush=True)
            # Pass Make's jobserver descriptors through to the recursive build.
            result = subprocess.run(
                [*make, setting, f"WRKDIR={checkout / 'build/native'}",
                 f"CLASS_MULTIPOLES_REFERENCE_DIR={checkout / 'build/reference'}",
                 f"FKPT2_MAKE_ARGS={extension_args}", "test-class-multipoles"],
                cwd=checkout, env=env, close_fds=False,
            )
            failed = failed or result.returncode != 0
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
