"""Sphinx configuration for the fkPT2 source documentation."""

from pathlib import Path
import sys
import tomllib

DOCS = Path(__file__).resolve().parent
sys.path.insert(0, str(DOCS / "_ext"))

project = "fkPT2"
author = "Mario A. Rodriguez-Meza and contributors"
copyright = "2024, Mario A. Rodriguez-Meza"

with (DOCS.parent / "pyproject.toml").open("rb") as metadata:
    release = tomllib.load(metadata)["project"]["version"]
version = release

root_doc = "index"
language = "en"
extensions = ["myst_parser", "fkpt2_parameters"]
nitpicky = True
exclude_patterns = [
    "_build",
    "_ext",
    "man",
    "**/.DS_Store",
    "**/Thumbs.db",
    "README.rst",
]

html_theme = "sphinx_rtd_theme"
html_title = "fkPT2 documentation"
html_show_sourcelink = True
