#!/usr/bin/env python3
"""Check Gaussian streaming with independent quadrature and linear theory."""

import argparse
import json
import math
from pathlib import Path
import tempfile

from test_clpt_regression import (BIAS_EFT, bessel, natural_spline, simpson_power,
                                  simpson_rule, weak_spectrum)
from test_fkpt_regression import LCDM_SPECTRUM, PROJECT, TABLE_SHAPES, load_table, run


SETTINGS = {
    "gsm": True, "smin": 45, "smax": 115, "Ns": 3, "rmin": 45, "rmax": 115, "Nr": 3,
    "Nk": 6, "nquadSteps": 64, "clpt_Nk": 60, "clpt_kmax": 12, "NqperLogDecade": 90,
    "Nk_qFunctionsQuad": 280, "clpt_b1": 0.65, "clpt_b2": -0.35, "clpt_b01": 0.4,
    "gsm_width": 35, "gsm_sizeyT": 160, "gsm_NGL": 16, "verbose": 0,
}
MG_SETTINGS = dict(SETTINGS, clpt_Nk=24, nquadSteps=28, ngausslegpoints=8)
CASES = {
    "LCDM": SETTINGS,
    "F5": dict(MG_SETTINGS, model="HS", fR0=1e-5),
    "DGP": dict(MG_SETTINGS, model="DGP"),
}
GSM_BIAS_EFT = dict(BIAS_EFT, c2eft=2.5, s2eft=3.0)
CASES["LCDM tidal and EFT"] = dict(SETTINGS, **GSM_BIAS_EFT)
CASES["F5 tidal and EFT"] = dict(CASES["F5"], **GSM_BIAS_EFT)
GSM_COLUMNS = ("s", "xi0", "xi2", "xi4")
VELOCITY_COLUMNS = ("r", "xi_tracer", "v12", "sigma_parallel2", "sigma_perp2")


def run_case(executable, directory, settings, spectrum=LCDM_SPECTRUM):
    directory.mkdir()
    run([executable, f"fnamePS={spectrum}", *(f"{name}={value}" for name, value in settings.items())],
        directory)
    return directory / "Output"


def read_gsm(output, settings):
    suffix = settings.get("suffixModel", "")
    path = output / f"gsm{suffix}.txt"
    velocity_path = output / f"clpt_velocity{suffix}.txt"
    assert path.read_text().splitlines()[1].split()[1:] == list(GSM_COLUMNS)
    lines = velocity_path.read_text().splitlines()
    assert lines[1].split()[1:] == list(VELOCITY_COLUMNS)
    return (load_table(path, (settings["Ns"], len(GSM_COLUMNS))),
            load_table(velocity_path, (len(lines) - 2, len(VELOCITY_COLUMNS))))


def close(actual, expected, field, rtol=1e-6, atol=1e-10):
    if not math.isclose(actual, expected, rel_tol=rtol, abs_tol=atol):
        raise AssertionError(f"{field}: got {actual:.12g}, expected {expected:.12g}")


def check_streaming(gsm, velocity, width):
    """Integrate over Cartesian LOS displacement and project with Simpson's rule."""
    r, xi, mean, parallel, transverse = zip(*velocity)
    pair_density = natural_spline(r, [math.log1p(x) for x in xi])
    infall = natural_spline(r, mean)
    log_par = natural_spline(r, [math.log(x) for x in parallel])
    log_perp = natural_spline(r, [math.log(x) for x in transverse])
    los_nodes = tuple(simpson_rule(-width, width, 600))
    for s, *observed in gsm:
        projected = [0.0, 0.0, 0.0]
        for mu, angular_weight in simpson_rule(0, 1, 120):
            s_parallel, s_perp = s * mu, s * math.sqrt(1 - mu**2)
            density = 0.0
            for offset, weight in los_nodes:
                y = s_parallel + offset
                radius = math.hypot(y, s_perp)
                mu_r = y / radius
                variance = (math.exp(log_par(radius)) * mu_r**2 +
                            math.exp(log_perp(radius)) * (1.0 - mu_r**2))
                residual = s_parallel - y - mu_r * infall(radius)
                density += weight * math.exp(pair_density(radius) - residual**2 / (2 * variance)) / (
                    math.sqrt(2 * math.pi * variance))
            multipoles = (1, 5 * (3 * mu**2 - 1) / 2, 9 * (35 * mu**4 - 30 * mu**2 + 3) / 8)
            for ell, polynomial in enumerate(multipoles):
                projected[ell] += angular_weight * polynomial * (density - 1)
        for ell, actual, expected in zip((0, 2, 4), observed, projected):
            close(actual, expected, f"streaming xi{ell} at s={s}", rtol=8e-6, atol=5e-8)


def check_linear_limit(executable, scratch):
    weak = weak_spectrum(scratch)
    settings = dict(SETTINGS, clpt_b01=0, gsm_sizeyT=320)
    output = run_case(executable, scratch / "weak", settings, weak)
    gsm, velocity = read_gsm(output, settings)
    c2eft = 2.5
    eft = run_case(executable, scratch / "weak-velocity-eft", dict(settings, c2eft=c2eft), weak)
    tidal = run_case(executable, scratch / "weak-tidal", dict(settings, clpt_bs2=1.0), weak)
    _, velocity_eft = read_gsm(eft, settings)
    _, velocity_tidal = read_gsm(tidal, settings)
    linear = load_table(output / "linear.txt", TABLE_SHAPES["linear"])
    power = simpson_power(linear, 1e-4, settings["clpt_kmax"])
    f, b = linear[0][2], 1 + settings["clpt_b1"]
    for s, *observed in gsm:
        samples = [(w * j0, w * j2, w * j4) for k, w in power
                   for j0, _, j2, _, j4, _ in (bessel(k * s),)]
        h0, h2, h4 = map(math.fsum, zip(*samples))
        # The Hankel transform contributes i^ell, including the sign of xi2.
        kaiser = ((b * b + 2 * b * f / 3 + f * f / 5) * h0,
                  -(4 * b * f / 3 + 4 * f * f / 7) * h2, 8 * f * f / 35 * h4)
        for ell, actual, expected in zip((0, 2, 4), observed, kaiser):
            close(actual, expected, f"Kaiser xi{ell} at s={s}", rtol=2e-3, atol=8e-11)

        index = min(range(len(velocity)), key=lambda i: abs(velocity[i][0] - s))
        r, xi, v12, parallel, perpendicular = velocity[index]
        samples = [(w * j0, w * j1 / k, w * (delta + j2) / k**2, w * delta / k**2)
                   for k, w in power for j0, j1, j2, _, _, delta in (bessel(k * r),)]
        density, mean, dispersion_par, dispersion_perp = map(math.fsum, zip(*samples))
        target = (b * b * density, -2 * b * f * mean, 2 * f * f * dispersion_par,
                  2 * f * f * dispersion_perp)
        for name, actual, expected in zip(VELOCITY_COLUMNS[1:], (xi, v12, parallel, perpendicular), target):
            close(actual, expected, f"linear {name} at r={r}", rtol=2e-3, atol=8e-11)

        # The mean EFT template is the gradient of the growth-weighted, smoothed xi.
        target_mean_shift = -c2eft * f * math.fsum(
            w * k * math.exp(-k*k) * bessel(k*r)[1] for k, w in power)
        _, xi_eft, v_eft, par_eft, perp_eft = velocity_eft[index]
        assert xi_eft == xi
        close((1 + xi) * (v_eft - v12), target_mean_shift, f"mean EFT at r={r}", rtol=2e-3, atol=8e-11)
        close(par_eft + v_eft**2, parallel + v12**2, "mean EFT raw parallel variance")
        close(perp_eft, perpendicular, "mean EFT transverse variance")

        # The shear response of each raw second moment is twice Upsilon_dd.
        radial = [(w * j1 / k, w * j3 / k) for k, w in power
                  for _, j1, _, j3, _, _ in (bessel(k*r),)]
        i1, i3 = map(math.fsum, zip(*radial))
        j2_d = f * (2 * i1 / 15 - i3 / 5)
        j3_d = -f * (i1 + i3) / 5
        _, xi_tidal, v_tidal, par_tidal, perp_tidal = velocity_tidal[index]
        par_shift = (1 + xi_tidal) * (par_tidal + v_tidal**2) - (1 + xi) * (parallel + v12**2)
        perp_shift = (1 + xi_tidal) * perp_tidal - (1 + xi) * perpendicular
        close(par_shift, 24 * j2_d**2, f"tidal raw parallel variance at r={r}", rtol=4e-3, atol=2e-11)
        close(perp_shift, 8 * j3_d**2, f"tidal raw transverse variance at r={r}", rtol=4e-3, atol=2e-11)


def check_frozen(label, gsm, velocity, expected):
    actual_rows = gsm + [min(velocity, key=lambda row: abs(row[0] - ref[0]))
                         for ref in expected["velocity"]]
    for actual_row, reference in zip(actual_rows, expected["gsm"] + expected["velocity"]):
        for actual, value in zip(actual_row, reference):
            close(actual, value, f"{label} at {reference[0]}", rtol=3e-6, atol=2e-9)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    expected = json.loads((PROJECT / "tests/fixtures/gsm/expected.json").read_text())

    with tempfile.TemporaryDirectory(prefix="fkpt2-gsm-") as tmp:
        scratch = Path(tmp)
        for name, settings in CASES.items():
            output = run_case(executable, scratch / name, settings)
            gsm, velocity = read_gsm(output, settings)
            assert [row[0] for row in gsm] == [45.0, 80.0, 115.0]
            check_frozen(name, gsm, velocity, expected[name])
            assert all(xi > -1 and par > 0 and perp > 0 for _, xi, _, par, perp in velocity)
            if name == "LCDM":
                lcdm_gsm, lcdm_velocity = gsm, velocity
                clpt = load_table(output / "clpt.txt", (settings["Nr"], 16))
                assert (output / "qfunctions.txt").is_file()
            elif name == "LCDM tidal and EFT":
                check_streaming(gsm, velocity, settings["gsm_width"])

        check_streaming(lcdm_gsm, lcdm_velocity, SETTINGS["gsm_width"])
        output = run_case(executable, scratch / "eft",
                          dict(SETTINGS, clpt=True, sigma2eft=3.0, suffixModel="_eft"))
        shifted_gsm, shifted = read_gsm(output, SETTINGS | {"suffixModel": "_eft"})
        assert shifted_gsm != lcdm_gsm
        shifts = []
        for before, after in zip(lcdm_velocity, shifted):
            assert before[:3] == after[:3]
            increase = after[3] - before[3]
            assert increase > 0
            close(increase, after[4] - before[4], "isotropic EFT increment")
            shifts.append(increase)
        radii, xi, *_ = zip(*lcdm_velocity)
        pair_correlation = natural_spline(radii, xi)
        eft_increment = natural_spline(radii, shifts)
        for row in clpt:
            r, xi_za, xi_tracer = row[0], row[2], row[-1]
            close(pair_correlation(r), xi_tracer, f"shared CLPT correlation at r={r}", atol=2e-7)
            close(eft_increment(r), 3 * (1 + xi_za) / (1 + xi_tracer), f"EFT at r={r}", atol=2e-7)

        check_linear_limit(executable, scratch)
        off = run_case(executable, scratch / "off", dict(SETTINGS, gsm=False))
        assert not (off / "gsm.txt").exists() and not (off / "clpt_velocity.txt").exists()
        for table in TABLE_SHAPES:
            assert (off / f"{table}.txt").read_bytes() == (
                scratch / "LCDM" / "Output" / f"{table}.txt").read_bytes()

        for option, message in (("Ns=0", "GSM requires Ns"), ("gsm_width=0", "gsm_width > 0"),
                                ("gsm_NGL=2", "GSM requires gsm_sizeyT"),
                                ("Nr=0", "CLPT requires Nr")):
            run([executable, "gsm=true", option], scratch, expected_error=message)
        print("GSM CLI: LCDM, F5, DGP, tidal bias, EFT, and independent streaming and linear limits passed")


if __name__ == "__main__":
    main()
