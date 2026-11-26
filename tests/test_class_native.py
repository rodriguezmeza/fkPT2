#!/usr/bin/env python3
"""Compare the linked CLASS addon with the standalone CLASS Bash workflow."""

import argparse
import os
from pathlib import Path
import tempfile

import numpy as np

from test_fkpt_regression import PROJECT, run


TABLES = ("linear", "kfunctions", "kfunctions_nw", "rsd_multipoles")
NUMERICS = ("Nk=20", "nquadSteps=80", "numberThreads=4", "suffixModel=_class")


def compare_tables(native, bash, tables, *, exact=False):
    for table in tables:
        name = f"{table}_class.txt"
        left, right = (np.loadtxt(directory / name, ndmin=2) for directory in (native, bash))
        assert left.shape == right.shape and np.isfinite(left).all() and np.isfinite(right).all()
        if exact:
            np.testing.assert_array_equal(left, right, err_msg=name)
        else:
            np.testing.assert_allclose(left, right, rtol=2e-6, atol=1e-9, err_msg=name)


def compare_case(executable, reference_env, config, scratch, model_options, tables):
    bash = scratch / "Bash results"
    linked = scratch / "Linked results"
    run([PROJECT / "scripts/run_fkpt2_class.sh", config, *NUMERICS, *model_options,
         f"rootDir={bash}"], scratch, env=reference_env)
    assert (bash / "class_linear_class.dat").is_file()

    # The linked mode runs without a CLASS binary or any command on PATH.
    env = dict(os.environ, PATH="", CLASS_BIN=str(scratch / "absent-class"))
    run([executable, f"classIni={config}", *NUMERICS, *model_options, f"rootDir={linked}"], scratch, env=env)
    compare_tables(linked, bash, tables)
    data_files = {file.name for file in linked.rglob("*.dat")}
    assert not data_files, f"Linked run wrote intermediate input files: {data_files}"
    assert not list(scratch.glob("class_run_*")), "CLASS itself wrote an output file"
    assert (linked / "linear_class.txt").read_text().startswith(f"# CLASSIni={config.resolve()}\n")

    # The used-values file points to the CLASS configuration, so reruns need no P(k) file.
    parameters = linked / "tmp/parameters_null-fkpt2_class-usedvalues"
    repeated = scratch / "Repeated"
    run([executable, parameters, f"rootDir={repeated}"], scratch, env=env)
    compare_tables(repeated, linked, tables, exact=True)
    assert not list(repeated.rglob("*.dat"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, required=True)
    parser.add_argument("--class-bin", type=Path, required=True)
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    env = dict(os.environ, CLASS_BIN=str(args.class_bin.resolve()), FKPT2_BIN=str(executable))

    with tempfile.TemporaryDirectory(prefix="fkpt2-class-native-") as directory:
        scratch = Path(directory)
        assert "with addon class" in run([executable, "options=make-info"], scratch)

        lcdm = scratch / "zero redshift.ini"
        sample = (PROJECT / "scripts/class_lcdm.ini").read_text()
        lcdm.write_text(sample.replace("h = 0.6711", "H0 = 67.11").replace("z_pk = 0.5", "z_pk = 0.0")
                        + "\nroot = class_run\nwrite_parameters = yes\n")
        compare_case(executable, env, lcdm, scratch, (), TABLES)
        print("CLASS addon LCDM z=0: Bash tables agree and the native solve keeps the spectrum in memory")

        models = {line.split()[0] for line in run([executable, "options=print-models"], scratch).splitlines()
                  if line.strip()}
        if "HS" in models:
            case = scratch / "F5"
            case.mkdir()
            compare_case(executable, env, PROJECT / "scripts/class_lcdm.ini", case,
                         ("model=HS", "fR0=1e-5"), TABLES)
            print("CLASS addon F5: Bash tables agree with LCDM-input growth rescaling")
        if "FOLPSNu" in models:
            case = scratch / "neutrinos"
            case.mkdir()
            compare_case(executable, env, PROJECT / "scripts/class_folps_nu.ini", case,
                         (), (*TABLES, "cb_power"))
            print("CLASS addon FOLPSNu: Bash tables agree with the automatically selected cb spectrum")

        run([executable, f"classIni={lcdm}", "OmegaM=0.3"], scratch,
            expected_error="conflicts with the CLASS value")
        run([executable, f"classIni={lcdm}", "fnamePS=pkl_z05.dat"], scratch,
            expected_error="Use classIni by itself")
        run([executable, "classIni=missing.ini"], scratch, expected_error="Cannot locate CLASS input")
        multi = scratch / "multiple.ini"
        multi.write_text(sample.replace("z_pk = 0.5", "z_pk = 0.5,1.0"))
        run([executable, f"classIni={multi}"], scratch, expected_error="one finite, nonnegative CLASS redshift")
        print("CLASS addon: conflicting inputs and invalid CLASS configurations fail cleanly")


if __name__ == "__main__":
    main()
