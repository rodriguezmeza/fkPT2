#!/usr/bin/env python3
"""Compare OpenMP thread counts through the CLI and Python."""

import argparse
from pathlib import Path
import tempfile

import fkpt2
import numpy as np

from test_fkpt_regression import LCDM_SPECTRUM, PROJECT, run
from test_gsm_regression import CASES, run_case


TABLE_ATTRIBUTES = {"rsd_multipoles": "multipoles"}


def check_case(executable, scratch, name, settings, k, pk):
    serial = run_case(executable, scratch / f"{name}-one", dict(settings, numberThreads=1))
    threaded = run_case(executable, scratch / f"{name}-four", dict(settings, numberThreads=4))
    serial_files = sorted(path.name for path in serial.glob("*.txt"))
    assert serial_files == sorted(path.name for path in threaded.glob("*.txt"))
    assert {"linear.txt", "kfunctions.txt", "kfunctions_nw.txt", "rsd_multipoles.txt"} <= set(serial_files)
    if settings.get("gsm", False):
        assert {"clpt.txt", "qfunctions.txt", "clpt_velocity.txt", "gsm.txt"} <= set(serial_files)

    one = fkpt2.compute(k, pk, parameters=settings, numberThreads=1)
    four = fkpt2.compute(k, pk, parameters=settings, numberThreads=4)
    for filename in serial_files:
        name_in_python = TABLE_ATTRIBUTES.get(Path(filename).stem, Path(filename).stem)
        assert (serial / filename).read_bytes() == (threaded / filename).read_bytes(), filename
        a, b = getattr(one, name_in_python), getattr(four, name_in_python)
        np.testing.assert_array_equal(a, b, err_msg=f"{name} {filename}")
        np.testing.assert_allclose(b, np.loadtxt(threaded / filename, ndmin=2), rtol=2e-6, atol=1e-12)
    print(f"OpenMP {name}: one and four threads agree in the CLI and Cython", flush=True)
    return four


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    k, pk = np.loadtxt(LCDM_SPECTRUM, unpack=True)

    with tempfile.TemporaryDirectory(prefix="fkpt2-openmp-") as directory:
        scratch = Path(directory)
        info = run([executable, "options=make-info"], scratch)
        assert "using OpenMP" in info, "Build with OPENMPMACHINE=1"

        check_case(executable, scratch, "LCDM", {"Nk": 24, "nquadSteps": 80}, k, pk)
        settings = CASES["F5 tidal and EFT"]
        f5 = check_case(executable, scratch, "F5 GSM", settings, k, pk)
        dgp = dict(CASES["DGP"], integration_method="bsstep")
        check_case(executable, scratch, "DGP GSM bsstep", dgp, k, pk)

        invalid = dict(settings, c1eft=1e6, numberThreads=4)
        message = "GSM requires a positive CLPT pair density"
        run([executable, f"fnamePS={LCDM_SPECTRUM}", *(f"{key}={value}" for key, value in invalid.items())],
            scratch, expected_error=message)
        try:
            fkpt2.compute(k, pk, parameters=invalid)
        except fkpt2.ComputationError as exc:
            assert message in str(exc)
        else:
            raise AssertionError("A failed OpenMP velocity row was accepted")
        recovered = fkpt2.compute(k, pk, parameters=settings, numberThreads=4)
        np.testing.assert_array_equal(f5.gsm, recovered.gsm)
        print("OpenMP worker errors propagate to both interfaces and leave the solver reusable")


if __name__ == "__main__":
    main()
