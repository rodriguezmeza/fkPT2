#!/usr/bin/env python3
"""Compare combined and separate inputs to the standalone CLASS runner."""

import argparse
import os
from pathlib import Path
import tempfile

import numpy as np

from test_fkpt_regression import PROJECT, run


RUNNER = PROJECT / "scripts/run_fkpt2_class.sh"
TABLES = ("linear", "kfunctions", "kfunctions_nw", "rsd_multipoles")


def compare_tables(left, right, tables):
    for table in tables:
        name = f"{table}_shared.txt"
        expected, actual = (np.loadtxt(path / name, ndmin=2) for path in (left, right))
        assert np.isfinite(expected).all() and np.isfinite(actual).all(), name
        np.testing.assert_array_equal(actual, expected, err_msg=name)


def compare_case(scratch, executable, env, flat, model):
    scratch.mkdir()
    combined = scratch / "shared settings.ini"
    output = scratch / "combined # 50% results"
    legacy = scratch / "legacy results"
    combined.write_text(
        f"""nquadSteps 64
[ Class ] % CLASS cosmology
% unused = value
{flat.read_text(encoding="utf-8")}
[FKPT2] # Native solver settings
m {model}
Nk = 10
alpha0 = '0.15' % EFT coefficient
root "{output.name}" # output relative to the invocation directory
suffixModel = _from_file
fnamePS = ""
classIni = ""
""",
        encoding="utf-8",
    )
    overrides = ("nk=16", "nthreads=4", "suffix=_shared")
    run([RUNNER, flat, "nquadSteps=64", "alpha0=0.15", f"rootDir={legacy}", *overrides], scratch, env=env)
    run([RUNNER, combined, *overrides], scratch, env=env)

    spectrum = "class_linear_shared.dat"
    np.testing.assert_array_equal(np.loadtxt(output / spectrum), np.loadtxt(legacy / spectrum))
    tables = (*TABLES, "cb_power") if model == "FOLPSNu" else TABLES
    compare_tables(legacy, output, tables)

    used = output / "tmp" / f"{combined.name}_shared-usedvalues"
    assert "Nk = 16\n" in used.read_text(encoding="utf-8")
    repeated = scratch / "rerun"
    run([executable, used, f"rootDir={repeated}"], scratch, env=dict(env, PATH="", CLASS_BIN=""))
    compare_tables(output, repeated, tables)
    print(f"Bash CLASS {model}: combined and separate inputs agree and the used-values file reruns")
    return combined


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, required=True)
    parser.add_argument("--class-bin", type=Path, required=True)
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    env = dict(os.environ, CLASS_BIN=str(args.class_bin.resolve()), FKPT2_BIN=str(executable))

    with tempfile.TemporaryDirectory(prefix="fkpt2-class-bash-") as directory:
        scratch = Path(directory)
        combined = compare_case(scratch / "LCDM", executable, env, PROJECT / "scripts/class_lcdm.ini", "LCDM")
        models = run([executable, "options=print-models"], scratch).splitlines()
        if any(line.split()[:1] == ["FOLPSNu"] for line in models):
            compare_case(scratch / "massive", executable, env, PROJECT / "scripts/class_folps_nu.ini", "FOLPSNu")

        run([RUNNER, combined, "fnamePS=elsewhere.dat"], scratch, env=env,
            expected_error="Use the [class] section by itself")
        run([RUNNER, combined, "classIni=elsewhere.ini"], scratch, env=env,
            expected_error="Choose a [class] section or classIni")
        invalid = scratch / "invalid.ini"
        invalid.write_text("[class]\n[fkPT22]\n", encoding="utf-8")
        run([RUNNER, invalid], scratch, env=env, expected_error="unknown section")


if __name__ == "__main__":
    main()
