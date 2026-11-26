#!/usr/bin/env python3
"""Verify native Cython results against the CLI covered by the fkPT regression."""

import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import shutil
import tempfile

import fkpt2
import numpy as np

from test_dgp_regression import (
    DGP_CASES, DGP_TABLE_SHAPES, check_growth, check_summary, dgp_options, run_cli_case,
)
from test_fkpt_regression import LCDM_SPECTRUM, F5_SPECTRUM, PROJECT, TABLE_SHAPES, run


TABLES = {
    "linear": "linear",
    "kfunctions": "kfunctions",
    "kfunctions_nw": "kfunctions_nw",
    "multipoles": "rsd_multipoles",
}


def compare(result, output, shapes):
    for field, filename in TABLES.items():
        table = getattr(result, field)
        assert table.shape == shapes[filename]
        assert len(fkpt2.column_names(field)) == table.shape[1]
        assert table.dtype == np.float64 and table.flags.c_contiguous
        expected = np.loadtxt(output / f"{filename}.txt")
        tolerance = 2e-9 if field == "linear" else 1e-6
        np.testing.assert_allclose(table, expected, rtol=tolerance, atol=1e-10,
                                   err_msg=field)


def expect_error(error, message, call, *args, **kwargs):
    try:
        call(*args, **kwargs)
    except error as exc:
        assert message in str(exc), str(exc)
    else:
        raise AssertionError(f"Expected {error.__name__} containing {message!r}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    args = parser.parse_args()
    executable = args.fkpt2.resolve()

    assert set(fkpt2.available_models()) >= {"LCDM", "HS", "DGP"}
    assert {"hu_sawicki", "dgp"} <= set(fkpt2.available_addons())
    assert fkpt2.column_names("multipoles") == ("k", "P0", "P2", "P4")

    k, pk = np.loadtxt(LCDM_SPECTRUM, unpack=True)
    solver = fkpt2.FKPT2()
    with tempfile.TemporaryDirectory(prefix="fkpt2-python-") as tmp:
        scratch = Path(tmp)
        for label, model, f_r0 in (("LCDM", "LCDM", 1e-10), ("F5", "HS", 1e-5), ("F6", "HS", 1e-6)):
            cwd = scratch / label
            cwd.mkdir()
            run([executable, f"fnamePS={LCDM_SPECTRUM}", f"model={model}", f"fR0={f_r0}"], cwd)
            result = solver.compute(k, pk, model=model.lower(), fR0=f_r0,
                                    rootDir=scratch / "no Python outputs")
            compare(result, cwd / "Output", TABLE_SHAPES)
            assert result.model == model and result.z == 0.5
            assert result.sigma8 > 0 and result.growth > 0 and result.f0 > 0
            np.testing.assert_array_equal(result.kfunctions[:, -1], result.f0)
            if label == "LCDM":
                lcdm = result
                previous = result.multipoles.copy()
            print(f"Cython {label}: all four tables match the CLI", flush=True)

        for name, case in DGP_CASES.items():
            output = run_cli_case(executable, scratch / f"dgp-{name}", name, case)
            settings = dgp_options(case)
            settings["model"] = "dgp"
            result = solver.compute(k, pk, **settings)
            assert result.model == "DGP" and result.z == settings["zout"]
            compare(result, output, DGP_TABLE_SHAPES)
            check_growth(result.linear, case["mgpt"], f"Cython DGP {name}")
            check_summary(result.growth, result.sigma8, case, f"Cython DGP {name}", 2e-6)
            print(f"Cython DGP {name}: all four tables match the CLI and growth matches MGPT",
                  flush=True)

        assert not (scratch / "no Python outputs").exists()
        np.testing.assert_array_equal(lcdm.multipoles, previous)

        direct = scratch / "direct mode"
        direct.mkdir()
        spectrum = direct / "spectrum with spaces.dat"
        shutil.copyfile(F5_SPECTRUM, spectrum)
        params = direct / "run.ini"
        params.write_text(f'fnamePS = "{spectrum}"\nmodel = HS\nfr0 = 1e-6\n'
                          "isinLCDM = 0\nz = 0.45\nnk = 4\nnquad = 36\nb1 = 1.7\n")
        run([executable, params, "fr0=1e-5"], direct)
        with_file = solver.compute(paramfile=params, fR0=1e-5)
        shapes = {name: (800 if name == "linear" else 4, shape[1])
                  for name, shape in TABLE_SHAPES.items()}
        compare(with_file, direct / "Output", shapes)
        assert with_file.z == 0.45

        expect_error(ValueError, "Supply both", fkpt2.compute, k)
        expect_error(ValueError, "equal length", fkpt2.compute, k, pk[:-1])
        expect_error(ValueError, "strictly increasing", fkpt2.compute, k[::-1], pk)
        expect_error(ValueError, "finite and positive", fkpt2.compute, k, pk * np.nan)
        expect_error(ValueError, "Unknown parameter", fkpt2.compute, k, pk, missing=3)
        expect_error(ValueError, "Conflicting values", fkpt2.compute, k, pk, Nk=3, nk=4)
        expect_error(ValueError, "Unknown or disabled", fkpt2.compute, k, pk, model="bad")
        expect_error(fkpt2.ComputationError, "Too many steps", fkpt2.compute, k, pk, maxnsteps=1)

        solver.set(Nk=4, nquadSteps=36)
        bsstep = solver.compute(k, pk, integration_method="bsstep")
        assert np.isfinite(bsstep.multipoles).all()

        def run_model(model, f_r0):
            return solver.compute(k, pk, model=model, fR0=f_r0).multipoles

        models = [("LCDM", 1e-10), ("HS", 1e-5), ("HS", 1e-6)]
        serial = [run_model(*model) for model in models]
        with ThreadPoolExecutor(max_workers=3) as threads:
            parallel = list(threads.map(lambda model: run_model(*model), models))
        for table, expected in zip(parallel, serial):
            np.testing.assert_array_equal(table, expected)
        np.testing.assert_array_equal(lcdm.multipoles, previous)

    print("Cython file/array inputs, native error recovery, result ownership, and concurrent calls passed")


if __name__ == "__main__":
    main()
