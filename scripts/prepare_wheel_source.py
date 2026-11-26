#!/usr/bin/env python3
"""Fetch and verify the immutable PyPI source archive used to backfill wheels."""

import argparse
from email.parser import Parser
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tarfile
from urllib.parse import quote, urlsplit
from urllib.request import urlopen


def prepare(version, directory):
    if not re.fullmatch(r"[0-9][A-Za-z0-9.!+_-]*", version):
        raise ValueError("Invalid release version")
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    with urlopen(f"https://pypi.org/pypi/fkpt2/{quote(version, safe='')}/json", timeout=60) as response:
        release = json.load(response)
    if release["info"]["name"].lower() != "fkpt2" or release["info"]["version"] != version:
        raise ValueError("PyPI returned a different project or version")
    archives = [item for item in release["urls"] if item["packagetype"] == "sdist"]
    if len(archives) != 1:
        raise ValueError("Expected exactly one published source archive")
    archive = archives[0]
    url = urlsplit(archive["url"])
    if url.scheme != "https" or url.hostname != "files.pythonhosted.org":
        raise ValueError("Expected a source archive hosted by PyPI")
    filename = archive["filename"]
    if Path(filename).name != filename or not filename.endswith(".tar.gz"):
        raise ValueError("Invalid source archive filename")
    destination = directory / filename
    with urlopen(archive["url"], timeout=120) as response:
        contents = response.read()
    digest = hashlib.sha256(contents).hexdigest()
    if digest != archive["digests"]["sha256"]:
        raise ValueError("Downloaded source archive does not match PyPI's SHA256")
    destination.write_bytes(contents)
    with tarfile.open(destination) as bundle:
        members = bundle.getmembers()
        roots = set()
        for member in members:
            path = PurePosixPath(member.name)
            if path.is_absolute() or ".." in path.parts or not (member.isfile() or member.isdir()):
                raise ValueError("Unsafe source archive member")
            roots.add(path.parts[0])
        if len(roots) != 1:
            raise ValueError("Expected one source archive root")
        source = directory / roots.pop()
        if source.exists():
            raise ValueError(f"Source directory already exists: {source}")
        bundle.extractall(directory, **({"filter": "data"} if hasattr(tarfile, "data_filter") else {}))
    metadata = Parser().parsestr((source / "PKG-INFO").read_text())
    if metadata["Name"].lower() != "fkpt2" or metadata["Version"] != version:
        raise ValueError("Source metadata does not match the requested release")
    provenance = {"name": "fkpt2", "version": version, "filename": filename,
                  "url": archive["url"], "sha256": digest}
    (directory / "source-provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
    print(f"Verified published source: {filename} (sha256 {digest})", flush=True)
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version")
    parser.add_argument("--directory", type=Path, default=Path("wheel-source"))
    args = parser.parse_args()
    print(prepare(args.version, args.directory))


if __name__ == "__main__":
    main()
