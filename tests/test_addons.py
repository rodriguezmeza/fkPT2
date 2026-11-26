#!/usr/bin/env python3
"""Build and exercise a gravity and analysis addon without editing core source."""

import argparse
import math
import os
from pathlib import Path
import shlex
import sys
import tempfile

from test_fkpt_regression import LCDM_SPECTRUM, PROJECT, run


ADDON_MAKEFILES = (
    "addons/clpt/addon.mk addons/gsm/addon.mk addons/model_DGP/addon.mk addons/model_HS/addon.mk "
    "tests/fixtures/gravity_addon/addon.mk"
)


def check_python():
    import fkpt2
    import numpy as np

    assert {"DGP", "Example"} <= fkpt2.available_models().keys()
    assert {"clpt", "dgp", "gsm", "monopole_at_kmin"} <= set(fkpt2.available_addons())
    solver = fkpt2.FKPT2(fnamePS=LCDM_SPECTRUM, Nk=4, nquad=36)
    lcdm = solver.compute()
    example = solver.compute(model="Example")
    modified = solver.compute(model="Example", modelOptions="strength=1.05")
    np.testing.assert_array_equal(lcdm.multipoles, example.multipoles)
    assert not np.array_equal(example.multipoles, modified.multipoles)
    for result in (lcdm, example, modified):
        assert result.diagnostics["monopole_at_kmin"] == result.multipoles[0, 1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--check-python", action="store_true")
    args = parser.parse_args()
    if args.check_python:
        check_python()
        return

    env = os.environ.copy()
    for name in ("MAKEFLAGS", "MAKEOVERRIDES", "MFLAGS", "MAKELEVEL"):
        env.pop(name, None)

    with tempfile.TemporaryDirectory(prefix="fkpt2-addons-") as tmp:
        scratch = Path(tmp)
        build = scratch / "native"
        executable = build / "fkpt2-exe"
        make = ["make", "-s", "-j4", f"WRKDIR={build}", f"CC={args.cc}", executable]
        run([*make, "MODELHSON=0", "MODELFOLPSNUON=0", "CLPTON=0"], PROJECT, env=env)
        models = run([executable, "verbose=0", "options=print-models"], scratch)
        assert "LCDM" in models and "DGP" in models and "HS" not in models
        assert "FOLPSNu" not in models
        run([executable, "model=HS"], scratch, expected_error="Unknown or disabled gravity model")
        run([executable, "model=FOLPSNu"], scratch, expected_error="Unknown or disabled gravity model")
        run([executable, "clpt=true"], scratch, expected_error="CLPT addon is disabled")

        run([executable, "gsm=true"], scratch, expected_error="GSM addon is disabled")

        run([*make, "MODELDGPON=0", "GSMON=0"], PROJECT, env=env)
        models = run([executable, "verbose=0", "options=print-models"], scratch)
        assert "LCDM" in models and "HS" in models and "DGP" not in models
        run([executable, "model=DGP"], scratch, expected_error="Unknown or disabled gravity model")
        run([executable, "gsm=true"], scratch, expected_error="GSM addon is disabled")

        settings = ["MODELHSON=1", "MODELDGPON=1", "CLPTON=1", f"FKPT2_ADDON_MAKEFILES={ADDON_MAKEFILES}"]
        run([*make, *settings], PROJECT, env=env)
        models = run([executable, "verbose=0", "options=print-models"], scratch)
        assert {"LCDM", "DGP", "HS", "Example"} <= {line.split()[0] for line in models.splitlines()}

        command = [executable, f"fnamePS={LCDM_SPECTRUM}", "Nk=4", "nquad=36", "verbose=0"]
        baseline = run([*command, "model=LCDM", "rootDir=LCDM"], scratch)
        example = run([*command, "model=example", "rootDir=Example"], scratch)
        modified = run([*command, "model=example", "modelOptions= strength = 1.05 ",
                        "rootDir=Modified"], scratch)
        assert example == baseline and example != modified
        for name, stdout in (("LCDM", baseline), ("Example", example), ("Modified", modified)):
            value = float(stdout.split("=")[1])
            row = (scratch / name / "rsd_multipoles.txt").read_text().splitlines()[2]
            assert math.isclose(value, float(row.split()[1]), rel_tol=1e-6)
        for option, error in (
            ("strength=-1", "strength must be positive"),
            ("strength=nan", "Invalid modelOptions"),
            ("unknown=1", "Unknown model option"),
            ("strength=1,strength=2", "Duplicate model option"),
        ):
            run([*command, "model=example", f"modelOptions={option}"], scratch, expected_error=error)

        env["FKPT2_MAKE_ARGS"] = shlex.join(["-j4", f"CC={args.cc}", *settings])
        env["PYTHONPATH"] = str(scratch / "module")
        run([sys.executable, "setup.py", "build_ext", "--build-temp", str(scratch / "module-build"),
             "--build-lib", env["PYTHONPATH"]], PROJECT, env=env)
        run([sys.executable, Path(__file__).resolve(), "--check-python"], scratch, env=env)

        run([*make, "ADDONSON=0"], PROJECT, env=env)
        models = run([executable, "verbose=0", "options=print-models"], scratch)
        assert [line.split()[0] for line in models.splitlines()] == ["LCDM"]

    print("Addon enable/disable, model-specific settings, and CLI/Python diagnostics passed")


if __name__ == "__main__":
    main()
