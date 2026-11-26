#!/usr/bin/env python3
"""Check that Cython exposes the GSM tables and uses the native CLI settings."""

import argparse
from pathlib import Path
import tempfile

import fkpt2
import numpy as np

from test_clpt_regression import addon_tables
from test_fkpt_regression import LCDM_SPECTRUM, PROJECT
from test_gsm_regression import CASES, GSM_COLUMNS, SETTINGS, VELOCITY_COLUMNS, read_gsm, run_case


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    assert {"clpt", "gsm"} <= set(fkpt2.available_addons())
    k, pk = np.loadtxt(LCDM_SPECTRUM, unpack=True)

    with tempfile.TemporaryDirectory(prefix="fkpt2-gsm-python-") as tmp:
        scratch = Path(tmp)
        root = scratch / "in memory"
        solver = fkpt2.FKPT2(rootDir=root)
        for name, settings in CASES.items():
            output = run_case(executable, scratch / name, settings)
            expected, velocity = read_gsm(output, settings)
            result = solver.compute(k, pk, **settings)
            assert result.column_names("gsm") == GSM_COLUMNS
            assert result.column_names("clpt_velocity") == VELOCITY_COLUMNS
            assert result.gsm is result.addon_tables["gsm"]
            assert result.clpt_velocity is result.addon_tables["clpt_velocity"]
            expected_clpt, expected_q = addon_tables(output, settings)
            np.testing.assert_allclose(result.clpt, expected_clpt, rtol=2e-10, atol=1e-12)
            np.testing.assert_allclose(result.qfunctions, expected_q, rtol=2e-10, atol=1e-12)
            np.testing.assert_allclose(result.gsm, expected, rtol=2e-10, atol=1e-12)
            np.testing.assert_allclose(result.clpt_velocity, velocity, rtol=2e-10, atol=1e-12)
            if name == "LCDM":
                previous, copy = result, result.gsm.copy()
            print(f"Cython GSM {name}: multipoles and velocity moments match the CLI", flush=True)

        assert not root.exists()
        np.testing.assert_array_equal(previous.gsm, copy)
        one = solver.compute(k, pk, **dict(SETTINGS, Ns=1))
        assert one.gsm.shape == (1, 4) and one.gsm[0, 0] == SETTINGS["smin"]
        default = solver.compute(fnamePS=LCDM_SPECTRUM, Nk=6, nquadSteps=64)
        assert default.gsm is None and default.clpt_velocity is None and not default.addon_tables

        try:
            solver.compute(k, pk, **dict(SETTINGS, smin=0))
        except ValueError as exc:
            assert "GSM requires Ns" in str(exc)
        else:
            raise AssertionError("Invalid GSM settings were accepted")

        try:
            solver.compute(k, pk, **dict(SETTINGS, s2eft=-100))
        except fkpt2.ComputationError as exc:
            assert "positive CLPT velocity variances" in str(exc)
        else:
            raise AssertionError("Nonpositive GSM dispersions were accepted")

        # A failed convolution must leave subsequent calls independent.
        recovered = solver.compute(k, pk, **SETTINGS)
        np.testing.assert_array_equal(recovered.gsm, previous.gsm)


if __name__ == "__main__":
    main()
