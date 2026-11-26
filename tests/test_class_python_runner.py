#!/usr/bin/env python3
"""Compare CLASS-only and combined parameter files in the direct Python runner."""

from io import StringIO
from pathlib import Path
import sys
import tempfile

import fkpt2
import numpy as np

from test_fkpt_regression import PROJECT, run


RUNNER = PROJECT / "scripts/run_fkpt2_class.py"


def compare_case(scratch, class_ini, model):
    scratch.mkdir()
    input_dir = scratch / "input files"
    input_dir.mkdir()
    flat = input_dir / "CLASS only.ini"
    combined = input_dir / "shared settings.ini"
    cosmology = class_ini.read_text(encoding="utf-8")
    flat.write_text(cosmology, encoding="utf-8")
    combined.write_text(
        "[fkPT2] % native solver settings\n"
        'Nk = 20\nnquad 64\nnthreads = 2\nb1 = 1.0\nb2 = "0.3" # bias\n'
        'rootDir = "solver results"\n'
        "\n[CLASS] # CLASS cosmology\n"
        + cosmology
        + "\nroot = class_output\nwrite_parameters = yes\n",
        encoding="utf-8",
    )

    before = set(scratch.rglob("*"))
    command = (sys.executable, RUNNER)
    overrides = ("nk=12", "b1=1.5")
    expected = run([*command, flat, "nquadSteps=64", "numberThreads=2", "b2=0.3", *overrides], scratch)
    actual = run([*command, combined.relative_to(scratch), *overrides], scratch)
    assert actual == expected, model
    assert actual.startswith("# k P0 P2 P4\n")
    multipoles = np.loadtxt(StringIO(actual))
    assert multipoles.shape == (12, 4) and np.isfinite(multipoles).all()
    assert set(scratch.rglob("*")) == before, "The Python runner wrote intermediate files"
    print(f"Direct Python runner {model} gives identical multipoles for combined and CLASS-only inputs")
    return combined


def main():
    with tempfile.TemporaryDirectory(prefix="fkpt2-class-python-runner-") as directory:
        scratch = Path(directory)
        combined = compare_case(scratch / "LCDM", PROJECT / "scripts/class_lcdm.ini", "LCDM")
        if "FOLPSNu" in fkpt2.available_models():
            compare_case(scratch / "massive neutrinos", PROJECT / "scripts/class_folps_nu.ini", "FOLPSNu")

        # The direct runner uses classy even in a build with linked CLASS.
        without_classy = ('import runpy, sys; sys.modules["classy"] = None; '
                          'runpy.run_path(sys.argv.pop(1), run_name="__main__")')
        run([sys.executable, "-c", without_classy, RUNNER, combined], scratch,
            expected_error="A working CLASS Python module (classy) is required")
        run([sys.executable, RUNNER, combined, "fnamePS=spectrum.dat"], scratch,
            expected_error="Use the [class] section by itself")


if __name__ == "__main__":
    main()
