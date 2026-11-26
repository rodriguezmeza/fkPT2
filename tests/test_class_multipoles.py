#!/usr/bin/env python3
"""Compare multipoles from one combined file across the four CLASS workflows."""

import argparse
from io import StringIO
from itertools import combinations
import os
from pathlib import Path
import sys
import tempfile

import fkpt2
import numpy as np

from test_fkpt_regression import PROJECT, run


NK = 20
MULTIPOLES = Path("results/rsd_multipoles.txt")


def compare_case(scratch, reference, env, class_ini, model):
    scratch.mkdir()
    shared = scratch / "shared settings.ini"
    shared.write_text(
        "[class]\n" + class_ini.read_text(encoding="utf-8")
        + f"\n[fkpt2]\nmodel = {model}\nNk = {NK}\n"
        + "nquadSteps = 80\nnumberThreads = 2\nb1 = 1.25\nrootDir = results\n",
        encoding="utf-8",
    )

    # Keep each file-writing run in its own directory so both use the same rootDir.
    native_dir = scratch / "C binary"
    native_dir.mkdir()
    run([reference, shared], native_dir)

    result = fkpt2.compute(paramfile=shared)
    assert result.model == model

    bash_dir = scratch / "Bash runner"
    bash_dir.mkdir()
    run([PROJECT / "scripts/run_fkpt2_class.sh", shared], bash_dir, env=env)
    assert (bash_dir / "results/class_linear.dat").is_file()

    stdout = run([sys.executable, PROJECT / "scripts/run_fkpt2_class.py", shared], scratch)
    tables = {
        "linked C binary": np.loadtxt(native_dir / MULTIPOLES, ndmin=2),
        "Cython": result.multipoles,
        "Bash runner": np.loadtxt(bash_dir / MULTIPOLES, ndmin=2),
        "direct Python runner": np.loadtxt(StringIO(stdout), ndmin=2),
    }
    for name, table in tables.items():
        assert table.shape == (NK, 4) and np.isfinite(table).all(), f"{model} {name}: invalid multipole table"
    # The CLI tables round to seven significant digits. Allow for this and the
    # small interpolation differences between the CLASS executable and APIs.
    for (left_name, left), (right_name, right) in combinations(tables.items(), 2):
        np.testing.assert_allclose(left, right, rtol=2e-6, atol=1e-9,
                                   err_msg=f"{model}: {left_name} vs {right_name}")
    print(f"{model} multipoles agree across the linked C binary, Cython, Bash, and direct Python")


def has_class_addon(executable, scratch):
    info = run([executable, "options=make-info"], scratch)
    return any(line.strip() == "with addon class" for line in info.splitlines())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, required=True)
    parser.add_argument("--class-bin", type=Path, required=True)
    parser.add_argument("--linked-reference", type=Path,
                        help="linked C reference for the CLASSLIBON=0 build")
    args = parser.parse_args()

    linked = args.linked_reference is None
    if ("class" in fkpt2.available_addons()) != linked:
        parser.error(f"Build Cython with CLASSLIBON={int(linked)} for this test")
    if "FOLPSNu" not in fkpt2.available_models():
        parser.error("Build fkPT2 and Cython with MODELFOLPSNUON=1")
    executable = args.fkpt2.resolve()
    reference = (args.linked_reference or args.fkpt2).resolve()
    env = dict(os.environ, CLASS_BIN=str(args.class_bin.resolve()),
               FKPT2_BIN=str(executable), PYTHON=sys.executable)
    with tempfile.TemporaryDirectory(prefix="fkpt2-class-multipoles-") as directory:
        root = Path(directory)
        assert has_class_addon(reference, root), "The reference C binary requires linked CLASS"
        if not linked:
            assert not has_class_addon(executable, root), "Bash requires the CLASSLIBON=0 fkPT2 binary for this test"
        print(f"Comparing Cython, Bash, and direct Python with CLASSLIBON={int(linked)}")
        for model, filename in (("LCDM", "class_lcdm.ini"), ("FOLPSNu", "class_folps_nu.ini")):
            compare_case(root / model, reference, env, PROJECT / "scripts" / filename, model)


if __name__ == "__main__":
    main()
