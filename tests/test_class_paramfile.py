#!/usr/bin/env python3
"""Use the same embedded CLASS and fkPT2 file from the CLI and Cython."""

import argparse
from pathlib import Path
import tempfile

import numpy as np

import fkpt2
from test_fkpt_regression import PROJECT, run


BASE_TABLES = {
    "linear": "linear",
    "kfunctions": "kfunctions",
    "kfunctions_nw": "kfunctions_nw",
    "rsd_multipoles": "multipoles",
}


def compare_case(executable, scratch, class_file, model):
    class_input = class_file.read_text()
    shared = "Nk = 20\nnquadSteps = 80\nb1 = 1.25\nnumberThreads = 2\nsuffixModel = _one\n"
    combined = scratch / "one input.ini"
    combined.write_text(
        "# Shared CLASS cosmology and fkPT2 settings\n[CLASS] # linear spectrum\n"
        + class_input
        + '\n[fkPT2] % solver settings\n' + shared + 'rootDir = "combined results"\n'
    )
    split = scratch / "solver settings.ini"
    split.write_text(shared + 'rootDir = "separate results"\n')

    # Exercise the usual precedence of CLI and keyword overrides.
    run([executable, combined, "Nk=12"], scratch)
    run([executable, split, f"classIni={class_file}", "Nk=12"], scratch)
    result = fkpt2.FKPT2(paramfile=combined).compute(Nk=12)
    expected = fkpt2.compute(paramfile=split, classIni=class_file, Nk=12)
    assert result.model == expected.model == model
    assert result.z == expected.z == 0.5
    assert result.multipoles.shape == (12, 4)
    tables = dict(BASE_TABLES)
    if model == "FOLPSNu":
        tables["cb_power"] = "cb_power"
        assert result.cb_power[-1, 2] < 1.0

    output = scratch / "combined results"
    baseline = scratch / "separate results"
    assert (output / "linear_one.txt").read_text().startswith(f"# CLASSSection={combined}\n")
    assert not list(output.rglob("*.dat")), "The linked calculation wrote an intermediate spectrum"
    for table, attribute in tables.items():
        filename = f"{table}_one.txt"
        actual_table = np.loadtxt(output / filename)
        np.testing.assert_array_equal(actual_table, np.loadtxt(baseline / filename))
        np.testing.assert_array_equal(getattr(result, attribute), getattr(expected, attribute))
        np.testing.assert_allclose(actual_table, getattr(result, attribute), rtol=2e-6, atol=1e-9)

    # The recorded configuration contains the [class] settings needed for a rerun.
    saved = output / "tmp/one input.ini_one-usedvalues"
    text = saved.read_text()
    assert "\n[fkpt2]\n" in text and "\n[class]\n" in text and "omega_b =" in text
    combined.unlink()
    run([executable, saved, "rootDir=repeat"], scratch)
    reread = fkpt2.compute(paramfile=saved)
    for table, attribute in tables.items():
        filename = f"{table}_one.txt"
        np.testing.assert_array_equal(np.loadtxt(scratch / "repeat" / filename), np.loadtxt(output / filename))
        np.testing.assert_array_equal(getattr(reread, attribute), getattr(result, attribute))
    print(f"Combined CLASS file for {model}: CLI and Cython match classIni and the self-contained rerun")


def compare_error(executable, scratch, text, message):
    path = scratch / "invalid.ini"
    path.write_text(text)
    run([executable, path], scratch, expected_error=message)
    try:
        fkpt2.compute(paramfile=path)
    except ValueError as exc:
        assert message in str(exc), str(exc)
    else:
        raise AssertionError(f"Cython accepted {message}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, required=True)
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    assert "class" in fkpt2.available_addons()

    cases = [("class_lcdm.ini", "LCDM")]
    if "FOLPSNu" in fkpt2.available_models():
        cases.append(("class_folps_nu.ini", "FOLPSNu"))

    with tempfile.TemporaryDirectory(prefix="fkpt2-class-paramfile-") as directory:
        scratch = Path(directory)
        for name, model in cases:
            case = scratch / model
            case.mkdir()
            compare_case(executable, case, PROJECT / "scripts" / name, model)

        compare_error(executable, scratch, "[clas]\nh = 0.7\n", "unknown section")
        compare_error(executable, scratch, "[class] missing comment marker\n", "invalid section header")
        compare_error(executable, scratch, "[class]\nz_pk = 0.5,1\n", "one finite, nonnegative CLASS redshift")
        compare_error(
            executable, scratch, "[class]\n[fkpt2]\nclassIni = separate.ini\n",
            "Choose a [class] section or classIni",
        )
        compare_error(
            executable, scratch, "[class]\n[fkpt2]\nfnamePS = separate.dat\n",
            "Use the [class] section by itself",
        )
    print("Combined CLASS file: both interfaces reject malformed sections and conflicting spectrum inputs")


if __name__ == "__main__":
    main()
