#!/usr/bin/env python3
"""Compare native CLPT results obtained through the CLI and Cython."""

import argparse
from pathlib import Path
import tempfile

import fkpt2
import numpy as np

from test_clpt_regression import BIAS_EFT, F5_SETTINGS, SETTINGS, addon_tables, check_bias, run_cli_case
from test_fkpt_regression import LCDM_SPECTRUM, PROJECT


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    assert "clpt" in fkpt2.available_addons()
    k, pk = np.loadtxt(LCDM_SPECTRUM, unpack=True)

    with tempfile.TemporaryDirectory(prefix="fkpt2-clpt-python-") as temp:
        scratch = Path(temp)
        cases = {
            "LCDM": SETTINGS, "F5": F5_SETTINGS,
            "LCDM tidal and EFT": dict(SETTINGS, **BIAS_EFT),
        }
        for name, settings in cases.items():
            output = run_cli_case(executable, scratch / name, settings)
            expected, expected_q = addon_tables(output, settings)
            result = fkpt2.compute(k, pk, parameters=settings, rootDir=scratch / "no output")
            assert result.column_names("clpt")[-2:] == ("xi_matter", "xi_tracer")
            assert result.column_names("qfunctions")[:3] == ("q", "X_linear", "Y_linear")
            assert result.clpt is result.addon_tables["clpt"]
            assert result.qfunctions is result.addon_tables["qfunctions"]
            np.testing.assert_allclose(result.clpt, expected, rtol=2e-10, atol=1e-12)
            np.testing.assert_allclose(result.qfunctions, expected_q, rtol=2e-10, atol=1e-12)
            if "clpt_bs2" in settings:
                density_shift = settings["c1eft"] * previous.clpt[:, 12]
                np.testing.assert_allclose(result.clpt[:, -2] - previous.clpt[:, -2],
                                           density_shift, rtol=1e-8, atol=1e-12)
                tidal_shift = result.clpt[:, -1] - previous.clpt[:, -1] - density_shift
                assert np.max(np.abs(tidal_shift)) > 1e-6
            else:
                check_bias(result.clpt, settings)
            if name == "LCDM":
                previous = result
                copy = result.clpt.copy()
            print(f"Cython CLPT {name}: correlation and q-function tables match the CLI", flush=True)

        assert not (scratch / "no output").exists()
        np.testing.assert_array_equal(previous.clpt, copy)
        default = fkpt2.compute(fnamePS=LCDM_SPECTRUM, Nk=6, nquadSteps=64)
        assert default.clpt is None and default.qfunctions is None and not default.addon_tables
        try:
            fkpt2.compute(k, pk, clpt=True, Nr=0)
        except ValueError as exc:
            assert "CLPT requires Nr" in str(exc)
        else:
            raise AssertionError("Invalid CLPT settings were accepted")


if __name__ == "__main__":
    main()
