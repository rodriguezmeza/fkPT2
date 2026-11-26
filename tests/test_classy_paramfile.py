#!/usr/bin/env python3
"""Check combined CLASS and fkPT2 files in the Cython build without linked CLASS."""

from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

import fkpt2
import numpy as np

from fkpt2_class import read_class_ini
from test_cython import expect_error
from test_fkpt_regression import LCDM_SPECTRUM, PROJECT


TABLES = ("linear", "kfunctions", "kfunctions_nw", "multipoles")


def compare_case(scratch, class_ini, model):
    scratch.mkdir()
    combined = scratch / "shared settings.ini"
    class_parameters = class_ini.read_text(encoding="utf-8").replace("h = ", "'h' = ", 1)
    combined.write_text(
        '[fkPT2] % solver settings\n'
        'Nk = 20\nnquad 64\nnthreads = 2\nb1 = "1.25" # bias\nz = 0.5\n'
        f'rootDir = "{scratch / "solver results"}"\nfnamePS = ""\nclassIni = ""\n'
        '\n[CLASS] # linear spectrum\nz_pk =\n'
        + class_parameters
        + f'\nroot = {scratch / "class results"}\nwrite_parameters = yes\n',
        encoding="utf-8",
    )
    cwd = Path.cwd()
    entries = set(cwd.iterdir())
    actual = fkpt2.compute(parameters={"paramfile": combined, "b1": 1.0}, nk=12, b1=1.5)
    assert Path.cwd() == cwd and set(cwd.iterdir()) == entries
    assert list(scratch.iterdir()) == [combined], "The in-memory run wrote intermediate files"

    expected = fkpt2.compute_from_class(
        read_class_ini(class_ini), Nk=12, nquadSteps=64, numberThreads=2, b1=1.5
    )
    assert actual.model == expected.model == model and actual.z == expected.z == 0.5
    assert actual.multipoles.shape == (12, 4)
    tables = (*TABLES, "cb_power") if model == "FOLPSNu" else TABLES
    for name in tables:
        np.testing.assert_array_equal(getattr(actual, name), getattr(expected, name), err_msg=name)
    if model == "FOLPSNu":
        assert actual.cb_power[-1, 2] < 1.0
    print(f"classy {model}: the combined file and the cosmology mapping give identical results")
    return combined, actual


def main():
    assert "class" not in fkpt2.available_addons(), "Build with CLASSLIBON=0 for this regression"
    with tempfile.TemporaryDirectory(prefix="fkpt2-classy-paramfile-") as directory:
        scratch = Path(directory)
        combined, expected = compare_case(scratch / "LCDM", PROJECT / "scripts/class_lcdm.ini", "LCDM")
        if "FOLPSNu" in fkpt2.available_models():
            compare_case(scratch / "massive neutrinos", PROJECT / "scripts/class_folps_nu.ini", "FOLPSNu")

        solver = fkpt2.FKPT2(paramfile=combined, b1=1.5, nk=12)
        expect_error(ValueError, "conflicts with the CLASS value", solver.compute, zout=0.8)
        invalid = scratch / "invalid.ini"
        invalid.write_text("[class] malformed\n", encoding="utf-8")
        expect_error(ValueError, "invalid section header", solver.compute, paramfile=invalid)
        invalid.write_text("[class]\nbase_path = " + "x" * 1024 + "\n", encoding="utf-8")
        expect_error(ValueError, "CLASS input length", solver.compute, paramfile=invalid)

        k, pk = np.loadtxt(LCDM_SPECTRUM, unpack=True)
        with patch.dict(sys.modules, {"classy": None}):
            expect_error(ValueError, "Use the [class] section by itself", solver.compute, k, pk)
            expect_error(ValueError, "Use the [class] section by itself", solver.compute, fnamePS=LCDM_SPECTRUM)
            expect_error(ValueError, "Choose a [class] section or classIni", solver.compute, classIni=combined)
            expect_error(ImportError, "classy", solver.compute)
            plain = scratch / "solver only.ini"
            plain.write_text("[fkpt2]\nNk = 12\nnquadSteps = 64\n", encoding="utf-8")
            direct = fkpt2.compute(k, pk, paramfile=plain)
            assert direct.multipoles.shape == (12, 4) and np.isfinite(direct.multipoles).all()

        np.testing.assert_array_equal(solver.compute().multipoles, expected.multipoles)
        print("classy parameter file: input conflicts, optional dependency, and rerun checks passed")


if __name__ == "__main__":
    main()
