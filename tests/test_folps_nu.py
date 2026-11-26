#!/usr/bin/env python3
"""Compare the massive-neutrino addon with the FOLPS-nu 7a4cd56 notebook cases."""

import argparse
import importlib.util
from pathlib import Path
import sys
import tempfile

import numpy as np

from test_fkpt_regression import PROJECT, run


FIXTURES = PROJECT / "tests/fixtures/folps_nu"
CASES = {
    "m00": (0.120926, 0.0, 0.7629031052972103),
    "m04": (0.116631, 0.00429461, 0.7629024494170353),
}
H, Z, OMEGA_B = 0.6711, 0.5, 0.0220684
BIAS = 1.645
NUISANCE = {
    "b1": BIAS, "b2": -0.46, "bs2": -4/7*(BIAS - 1), "b3nl": 32/315*(BIAS - 1),
    "alpha0": 3.0, "alpha2": -28.9, "alpha4": 5.0, "ctilde": 0.15,
    "alpha0shot": 0.08, "alpha2shot": -8.1, "pshotp": 1/0.0002118763,
}
COLUMNS = (
    "k", "P_cb_linear", "f_cb_over_f0", "P_cb_1loop", "P_cb_theta_1loop", "P_theta_theta_1loop",
)
TOLERANCES = {
    "P_cb_linear": 9e-4, "f_cb_over_f0": 2e-7,
    "P_cb_1loop": 1.5e-3, "P_cb_theta_1loop": 2.5e-3, "P_theta_theta_1loop": 4e-3,
    "P0": 3.5e-3, "P2": 1.1e-2, "P4": 1.5e-2,
}


def spectrum_path(case):
    return FIXTURES / f"pkl_cb_{case}_z05.dat"


def settings(case):
    omega_c, omega_nu, _ = CASES[case]
    return dict(NUISANCE, model="FOLPSNu", modelOptions=f"mnu={93.14*omega_nu}",
                Om=(OMEGA_B + omega_c + omega_nu) / H**2, h=H, zout=Z,
                is_PS_input_LCDM=0, numberThreads=4)


def live_reference(directory):
    from scipy import integrate

    # The frozen Python reference calls simps, which recent SciPy removed.
    if not hasattr(integrate, "simps"):
        integrate.simps = integrate.simpson

    spec = importlib.util.spec_from_file_location("folps_reference", directory / "FOLPSnu.py")
    module = importlib.util.module_from_spec(spec)
    old = sys.dont_write_bytecode
    try:
        sys.dont_write_bytecode = True
        spec.loader.exec_module(module)
    finally:
        sys.dont_write_bytecode = old

    module.Matrices(Nfftlog=256)
    result = {}
    for case, (omega_c, omega_nu, _) in CASES.items():
        spectrum = np.loadtxt(spectrum_path(case), unpack=True)
        table, _ = module.NonLinear(spectrum, (Z, OMEGA_B, omega_c, omega_nu, H))
        k, linear, ratio, dd, dt, tt = table[:6]
        _, p0, p2, p4 = module.RSDmultipoles(k, list(NUISANCE.values()))
        result[case] = np.column_stack(
            (k, linear, ratio, linear + dd, linear * ratio + dt, linear * ratio**2 + tt, p0, p2, p4)
        )
    return result


def command(executable, case, **options):
    values = {**settings(case), 'fnamePS': spectrum_path(case), **options}
    return [executable, *(f"{key}={value}" for key, value in values.items())]


def compare_to_reference(case, power, multipoles, reference):
    assert power.shape == (120, len(COLUMNS)) and multipoles.shape == (120, 4)
    assert np.isfinite(power).all() and np.isfinite(multipoles).all()
    np.testing.assert_allclose(power[:, 0], reference[:, 0], rtol=1e-9, atol=0)
    np.testing.assert_allclose(multipoles[:, 0], power[:, 0], rtol=6e-7, atol=0)
    # The perturbative comparison uses k up to 0.25 h/Mpc. Verify shape and
    # finiteness above this range, where multipoles can cross zero.
    keep = reference[:, 0] <= 0.25
    native = np.column_stack((power, multipoles[:, 1:]))
    for column, (name, rtol) in enumerate(TOLERANCES.items(), 1):
        np.testing.assert_allclose(
            native[keep, column], reference[keep, column], rtol=rtol, atol=0, err_msg=f"{case} {name}"
        )
    if case == "m00":
        np.testing.assert_allclose(power[:, 2], 1.0, rtol=0, atol=2e-15)
    else:
        assert power[0, 2] > 0.999 and power[-1, 2] < 0.985
        assert np.all(np.diff(power[:, 2]) < 0)


def check_python(case, cli_power, cli_multipoles, reference):
    import fkpt2

    k, pk = np.loadtxt(spectrum_path(case), unpack=True)
    options = settings(case)
    result = fkpt2.compute(k, pk, parameters=options)
    assert result.model == "FOLPSNu" and result.column_names("cb_power") == COLUMNS
    assert result.cb_power is result.addon_tables["cb_power"]
    np.testing.assert_allclose(result.cb_power, cli_power, rtol=1e-12, atol=0)
    np.testing.assert_allclose(result.multipoles, cli_multipoles, rtol=8e-7, atol=0)
    np.testing.assert_allclose(result.f0, CASES[case][2], rtol=5e-5, atol=0)
    compare_to_reference(case, result.cb_power, result.multipoles, reference)

    serial = fkpt2.compute(k, pk, parameters=dict(options, numberThreads=1))
    for name in ("linear", "cb_power", "multipoles"):
        np.testing.assert_array_equal(getattr(serial, name), getattr(result, name))


def check_growth_amplitude():
    import fkpt2

    k, pk = np.loadtxt(spectrum_path("m04"), unpack=True)
    options = dict(settings("m04"), Nk=8, nquadSteps=64, eps=1e-8)
    # The growth derivative depends on the cosmology and redshift alone.
    # Reuse the spectrum and differentiate with respect to ln(a).
    eta = -np.log1p(Z)
    step = 0.002
    plus = fkpt2.compute(k, pk, parameters=dict(options, zout=np.expm1(-eta - step)))
    minus = fkpt2.compute(k, pk, parameters=dict(options, zout=np.expm1(-eta + step)))
    central = fkpt2.compute(k, pk, parameters=options)
    rate = (np.log(plus.linear[:, 3]) - np.log(minus.linear[:, 3])) / (2*step)
    np.testing.assert_allclose(rate, central.linear[:, 2], rtol=5e-5, atol=0)
    print("FOLPSNu: derivative of the growth amplitude agrees with the fitted rate")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    parser.add_argument("--folps-reference", type=Path)
    parser.add_argument("--python", action="store_true", help="also check the built Cython module")
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    reference = (live_reference(args.folps_reference.resolve()) if args.folps_reference else
                 {case: np.loadtxt(FIXTURES / f"reference_{case}.dat") for case in CASES})

    with tempfile.TemporaryDirectory(prefix="fkpt2-folps-nu-") as directory:
        scratch = Path(directory)
        models = run([executable, "options=print-models"], scratch)
        assert "FOLPSNu" in models
        if args.python:
            import fkpt2

            assert "FOLPSNu" in fkpt2.available_models() and "folps_nu" in fkpt2.available_addons()

        for case in CASES:
            run(command(executable, case, rootDir=case), scratch)
            power = np.loadtxt(scratch / case / "cb_power.txt")
            multipoles = np.loadtxt(scratch / case / "rsd_multipoles.txt")
            compare_to_reference(case, power, multipoles, reference[case])
            if args.python:
                check_python(case, power, multipoles, reference[case])
            interface = "CLI and Cython" if args.python else "CLI"
            reference_kind = "live" if args.folps_reference else "recorded"
            print(f"FOLPSNu {case}: {interface} comparison with {reference_kind} FOLPS-nu passed")

        for option, value, message in (
            ("modelOptions", "mnu=-0.1", "FOLPSNu requires 0 <= mnu"),
            ("modelOptions", "mnu=20", "FOLPSNu requires 0 <= mnu"),
            ("is_PS_input_LCDM", 1, "requires the linear cb spectrum"),
        ):
            invalid = dict(settings("m04"), **{option: value})
            run([executable, f"fnamePS={spectrum_path('m04')}",
                 *(f"{key}={value}" for key, value in invalid.items())], scratch, expected_error=message)
            if args.python:
                try:
                    fkpt2.compute(fnamePS=spectrum_path("m04"), parameters=invalid)
                except ValueError as exc:
                    assert message in str(exc)
                else:
                    raise AssertionError(f"FOLPSNu accepted invalid {option}")

    if args.python:
        check_growth_amplitude()


if __name__ == "__main__":
    main()
