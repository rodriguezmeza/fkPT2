#!/usr/bin/env python3
"""Exercise the CLASS Python and standalone input paths against the same cosmology."""

import argparse
import os
from pathlib import Path
import tempfile

import numpy as np

import fkpt2
from fkpt2_class import read_class_ini
from test_fkpt_regression import PROJECT, run


SETTINGS = {"Nk": 16, "nquadSteps": 64, "numberThreads": 4}
TABLES = {
    "linear": "linear",
    "kfunctions": "kfunctions",
    "kfunctions_nw": "kfunctions_nw",
    "rsd_multipoles": "multipoles",
}


def compare_case(config, scratch, class_bin, model, redshift):
    cosmology = read_class_ini(config)
    before = set(Path.cwd().iterdir())
    result = fkpt2.compute_from_class(cosmology, parameters=SETTINGS)
    assert result.model == model and result.z == redshift
    assert set(Path.cwd().iterdir()) == before, "The Python calculation wrote an intermediate table"

    env = dict(os.environ, CLASS_BIN=str(class_bin))
    output = scratch / "native results"
    values = [*(f"{key}={value}" for key, value in SETTINGS.items()), f"rootDir={output}", "suffixModel=_check"]
    run([PROJECT / "scripts/run_fkpt2_class.sh", config, *values], scratch, env=env)
    spectrum = output / "class_linear_check.dat"
    assert spectrum.is_file()
    assert "k (h/Mpc)" in spectrum.read_text()
    used = output / "tmp/parameters_null-fkpt2_check-usedvalues"
    reread = fkpt2.compute(paramfile=used)

    tables = dict(TABLES)
    if model == "FOLPSNu":
        tables["cb_power"] = "cb_power"
    for filename, attribute in tables.items():
        from_memory = getattr(result, attribute)
        from_file = np.loadtxt(output / f"{filename}_check.txt")
        np.testing.assert_allclose(from_memory, from_file, rtol=3e-6, atol=1e-8, err_msg=filename)
        np.testing.assert_allclose(from_memory, getattr(reread, attribute), rtol=3e-8, atol=1e-9)

    k, power, settings = fkpt2.linear_spectrum_from_class(cosmology, parameters=SETTINGS)
    native_spectrum = np.loadtxt(spectrum)
    np.testing.assert_allclose(k, native_spectrum[:, 0], rtol=1e-10, atol=0)
    np.testing.assert_allclose(power, native_spectrum[:, 1], rtol=1e-9, atol=0)
    reused = fkpt2.compute(k, power, parameters=settings)
    np.testing.assert_array_equal(reused.multipoles, result.multipoles)

    if model == "FOLPSNu":
        assert settings["is_PS_input_LCDM"] == 0
        assert abs(float(settings["modelOptions"].split("=")[1]) - 0.4) < 0.003
        assert result.cb_power[-1, 2] < 1.0
    else:
        h = float(cosmology["H0"]) / 100
        matter = (float(cosmology["omega_b"]) + float(cosmology["omega_cdm"])) / h**2
        np.testing.assert_allclose((settings["h"], settings["Om"]), (h, matter), rtol=1e-11)

    print(f"CLASS {model} z={redshift}: native and in-memory spectra and solver tables agree")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--class-bin", type=Path, required=True)
    args = parser.parse_args()
    class_bin = args.class_bin.resolve()
    with tempfile.TemporaryDirectory(prefix="fkpt2-class-test-") as directory:
        root = Path(directory)
        before = Path.cwd()
        try:
            os.chdir(root)
            lcdm_dir, nu_dir = root / "massless", root / "massive"
            lcdm_dir.mkdir()
            lcdm = root / "CLASS background.ini"
            source = (PROJECT / "scripts/class_lcdm.ini").read_text()
            lcdm.write_text(source.replace("h = 0.6711", "H0 = 67.11").replace("z_pk = 0.5", "z_pk = 0.0"))
            compare_case(lcdm, lcdm_dir, class_bin, "LCDM", 0.0)
            if "FOLPSNu" in fkpt2.available_models():
                nu_dir.mkdir()
                compare_case(PROJECT / "scripts/class_folps_nu.ini", nu_dir, class_bin, "FOLPSNu", 0.5)

            lcdm_options = read_class_ini(lcdm)
            solver = fkpt2.FKPT2(parameters=SETTINGS, model="HS", fR0=1e-5)
            k = np.geomspace(1e-4, 5.0, 200)
            hs = solver.compute_from_class(lcdm_options, k=k, redshift=0.75)
            assert hs.model == "HS" and hs.z == 0.75
            run([PROJECT / "scripts/run_fkpt2_class.sh", lcdm, "zout=0.75"], root,
                env=dict(os.environ, CLASS_BIN=str(class_bin)), expected_error="conflicts with the CLASS value")
            run([PROJECT / "scripts/run_fkpt2_class.sh", lcdm], root,
                env=dict(os.environ, CLASS_BIN=str(root / "missing")), expected_error="set CLASS_BIN")
            bad = dict(lcdm_options, z_pk="0,0.5")
            try:
                fkpt2.compute_from_class(bad, parameters=SETTINGS)
            except ValueError as exc:
                assert "single CLASS redshift" in str(exc)
            else:
                raise AssertionError("The workflow accepted multiple redshifts for one solve")
        finally:
            os.chdir(before)


if __name__ == "__main__":
    main()
