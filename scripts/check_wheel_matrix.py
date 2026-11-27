#!/usr/bin/env python3
"""Require every supported CPython/platform wheel locally or on production PyPI."""

import argparse
from email.parser import Parser
import json
from pathlib import Path
from urllib.parse import quote
from urllib.request import urlopen
import zipfile

from packaging.utils import parse_wheel_filename


PYTHONS = ("cp39", "cp310", "cp311", "cp312", "cp313", "cp314", "cp315")
PLATFORMS = ("manylinux_x86_64", "manylinux_aarch64", "musllinux_x86_64",
             "musllinux_aarch64", "macosx_x86_64", "macosx_arm64")


def coverage(filename, version):
    name, parsed_version, _, tags = parse_wheel_filename(filename)
    if name != "fkpt2" or str(parsed_version) != version:
        raise ValueError(f"Unexpected package or version: {filename}")
    found = set()
    for tag in tags:
        if tag.interpreter not in PYTHONS or tag.abi != tag.interpreter:
            raise ValueError(f"Unexpected interpreter/ABI in {filename}")
        for target in PLATFORMS:
            family, arch = target.split("_", 1)
            if tag.platform.startswith(family + "_") and tag.platform.endswith("_" + arch):
                found.add((tag.interpreter, target))
        # auditwheel can add this equivalent legacy tag alongside a manylinux tag.
        if tag.platform.startswith("manylinux2014_"):
            found.add((tag.interpreter, "manylinux_" + tag.platform.removeprefix("manylinux2014_")))
    if not found:
        raise ValueError(f"Unsupported platform: {filename}")
    return found


def validate_filenames(filenames, version):
    expected = {(python, platform) for python in PYTHONS for platform in PLATFORMS}
    found = set()
    for filename in filenames:
        found.update(coverage(filename, version))
    missing = sorted(expected - found)
    if missing:
        raise ValueError("Missing wheels: " + ", ".join(f"{python}-{platform}" for python, platform in missing))
    print(f"Complete wheel matrix: {len(expected)} CPython/platform combinations")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--directory", type=Path)
    source.add_argument("--pypi", action="store_true")
    args = parser.parse_args()
    if args.pypi:
        with urlopen(f"https://pypi.org/pypi/fkpt2/{quote(args.version, safe='')}/json", timeout=60) as response:
            release = json.load(response)
        filenames = [item["filename"] for item in release["urls"] if item["packagetype"] == "bdist_wheel" and not item["yanked"]]
    else:
        filenames = []
        for wheel in sorted(args.directory.rglob("*.whl")):
            with zipfile.ZipFile(wheel) as archive:
                paths = [name for name in archive.namelist() if name.endswith(".dist-info/METADATA")]
                if len(paths) != 1:
                    raise ValueError(f"Invalid wheel metadata: {wheel}")
                metadata = Parser().parsestr(archive.read(paths[0]).decode())
                if metadata["Name"].lower() != "fkpt2" or metadata["Version"] != args.version:
                    raise ValueError(f"Wheel metadata mismatch: {wheel}")
                for command in ("fkpt2", "fkpt2-emulate"):
                    info = archive.getinfo(f"fkpt2_cli/_runtime/bin/{command}")
                    if not (info.external_attr >> 16) & 0o111:
                        raise ValueError(f"Missing executable permissions: {wheel}: {command}")
            filenames.append(wheel.name)
    validate_filenames(filenames, args.version)


if __name__ == "__main__":
    main()
