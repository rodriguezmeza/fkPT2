#!/usr/bin/env python3
"""Test the CLPT Fourier transforms and compare the convolution with MGPT."""

import argparse
from bisect import bisect_right
import json
import math
from pathlib import Path
import tempfile

from test_dgp_regression import build_mgpt_probe, check_value
from test_fkpt_regression import LCDM_SPECTRUM, PROJECT, TABLE_SHAPES, load_table, run


FIXTURES = PROJECT / "tests/fixtures/clpt"
BASELINE = json.loads((FIXTURES / "expected.json").read_text())
SETTINGS = {
    "clpt": True, "Nk": 6, "nquadSteps": 64, "clpt_Nk": 60, "clpt_kmax": 12,
    "rmin": 65, "rmax": 115, "Nr": 3, "NqperLogDecade": 60, "Nk_qFunctionsQuad": 280,
    "clpt_b1": 0.65, "clpt_b2": -0.35, "clpt_b01": 0.4,
}
F5_SETTINGS = dict(SETTINGS, model="HS", fR0=1e-5, clpt_Nk=24, nquadSteps=28, ngausslegpoints=8)
BIAS_EFT = {"clpt_bs2": 0.7, "c1eft": 4.0}
BASE_TABLES = {name: (800 if name == "linear" else SETTINGS["Nk"], cols)
               for name, (_, cols) in TABLE_SHAPES.items()}


def run_cli_case(executable, directory, settings, spectrum=LCDM_SPECTRUM):
    directory.mkdir()
    options = [f"{name}={value}" for name, value in settings.items()]
    run([executable, f"fnamePS={spectrum}", *options], directory)
    return directory / "Output"


def addon_tables(output, settings):
    rows = sum(not line.startswith("#") for line in (output / "qfunctions.txt").read_text().splitlines())
    return load_table(output / "clpt.txt", (settings["Nr"], 16)), load_table(output / "qfunctions.txt", (rows, 16))


def check_bias(correlations, settings):
    b1, b2, b01 = (settings[name] for name in ("clpt_b1", "clpt_b2", "clpt_b01"))
    for row in correlations:
        matter = math.fsum(row[2:5]) + settings.get("c1eft", 0.0) * row[12]
        tracer = (matter + b1 * (row[5] + row[6]) + b1**2 * (row[7] + row[8])
                  + b2 * row[9] + b2**2 * row[10] + b1 * b2 * row[11]
                  + 2 * (1 + b1) * b01 * row[12] + b01**2 * row[13])
        check_value(matter, row[14], "CLPT matter sum", 1e-12)
        check_value(tracer, row[15], "CLPT Lagrangian bias sum", 1e-12)


def natural_spline(grid, values):
    """Evaluate the natural cubic by solving its tridiagonal system."""
    size = len(grid)
    upper = [0.0] * size
    curvature = [0.0] * size
    for i in range(1, size - 1):
        left, right = grid[i] - grid[i - 1], grid[i + 1] - grid[i]
        pivot = 2 * (left + right) - left * upper[i - 1]
        slope_change = (values[i + 1] - values[i]) / right - (values[i] - values[i - 1]) / left
        upper[i] = right / pivot
        curvature[i] = (6 * slope_change - left * curvature[i - 1]) / pivot
    for i in range(size - 2, 0, -1):
        curvature[i] -= upper[i] * curvature[i + 1]

    def evaluate(x):
        j = min(size - 2, max(0, bisect_right(grid, x) - 1))
        width = grid[j + 1] - grid[j]
        u = (grid[j + 1] - x) / width
        v = 1 - u
        return u * values[j] + v * values[j + 1] + width**2 / 6 * (
            (u**3 - u) * curvature[j] + (v**3 - v) * curvature[j + 1])
    return evaluate


def simpson_rule(low, high, panels):
    step = (high - low) / panels
    for i in range(panels + 1):
        coefficient = 1 if i in (0, panels) else (4 if i % 2 else 2)
        yield low + step * i, coefficient * step / 3


def simpson_power(linear, kmin, kmax):
    """Integrate the prepared spline with an independent composite Simpson rule."""
    grid, power = tuple(zip(*(row[:2] for row in linear)))
    spectrum = natural_spline(grid, power)
    return [(k, weight * spectrum(k) * k**2 / (2 * math.pi**2))
            for k, weight in simpson_rule(kmin, kmax, 24000)]


def bessel(x):
    """j_0, j_1, j_2, j_3, j_4, and the finite-separation displacement factor."""
    j0 = math.sin(x) / x
    if x < 0.1:
        x2 = x * x
        j1 = x * (1 / 3 - x2 / 30 + x2**2 / 840)
        j2 = x2 / 15 * (1 - x2 / 14 + x2**2 / 504)
        j3 = x * x2 / 105 * (1 - x2 / 18 + x2**2 / 792)
        j4 = x2**2 / 945 * (1 - x2 / 22)
        delta = x2 * (1 / 30 - x2 / 840)
    else:
        j1 = (j0 - math.cos(x)) / x
        j2 = 3 * j1 / x - j0
        j3 = 5 * j2 / x - j1
        j4 = 7 * j3 / x - j2
        delta = 1 / 3 - j1 / x
    return j0, j1, j2, j3, j4, delta


def weak_spectrum(scratch):
    # Suppress nonlinear corrections and damp the sharp Fourier endpoint.
    weak = scratch / "weak.dat"
    with weak.open("w") as file:
        for line in LCDM_SPECTRUM.read_text().splitlines():
            k, p = map(float, line.split())
            file.write(f"{k:.17e} {5e-4 * p / (1 + k**8):.17e}\n")
    return weak


def check_linear_transform(correlations, qfunctions, linear):
    power = simpson_power(linear, 1e-4, SETTINGS["clpt_kmax"])
    for r, xi, *_ in correlations:
        expected = math.fsum(weight * math.sin(k * r) / (k * r) for k, weight in power)
        if not math.isclose(xi, expected, rel_tol=5e-6, abs_tol=1e-9):
            raise AssertionError(f"At r={r} the linear Hankel transform is {xi}, Simpson gives {expected}")
    q, x, y = qfunctions[0][:3]
    # j_1(kq)/(kq) = 1/3 - (kq)^2/30 and j_2(kq) = (kq)^2/15.
    x_limit = q * q / 15 * math.fsum(weight for _, weight in power)
    if not math.isclose(x, x_limit, rel_tol=2e-7):
        raise AssertionError(f"Small-q covariance should be {x_limit}, got {x}")
    if not math.isclose(y, 2 * x, rel_tol=2e-7):
        raise AssertionError(f"Small-q Y should approach twice X, got {y} vs {2 * x}")
    next_q, next_x10 = qfunctions[1][0], qfunctions[1][7]
    if not math.isclose(qfunctions[0][7] / q**2, next_x10 / next_q**2, rel_tol=2e-6):
        raise AssertionError("Small-q X10 should vanish quadratically")


def tidal_hankel(power, r):
    samples = [(w * j0, w * j2, w * j4, w * j1 / k, w * j3 / k) for k, w in power
               for j0, j1, j2, j3, j4, _ in (bessel(k*r),)]
    xi0, xi2, xi4, i1, i3 = map(math.fsum, zip(*samples))
    x = 4 * ((i1 + i3) / 5)**2
    y = 12 * ((2*i1 - 3*i3) / 15)**2 - x
    return xi0, xi2, xi4, x, y


def check_nuisance_terms(executable, scratch, correlations):
    # EFT uses the same Laplacian template as the curvature contribution.
    settings = dict(SETTINGS, c1eft=BIAS_EFT["c1eft"])
    output = run_cli_case(executable, scratch / "density-eft", settings)
    eft, _ = addon_tables(output, settings)
    check_bias(eft, settings)
    for before, after in zip(correlations, eft):
        assert before[:14] == after[:14]
        for index in (14, 15):
            change = settings["c1eft"] * before[12]
            assert change != 0
            check_value(after[index] - before[index], change, "CLPT density EFT increment", 1e-8)

    # At weak clustering the ZA convolution tends to evaluation at q=r.
    # Taking even and mixed responses isolates <s1^2 s2^2> and <delta1^2 s2^2>.
    weak = weak_spectrum(scratch)
    basis = dict(SETTINGS, rmin=15, rmax=33, clpt_b1=0, clpt_b2=0, clpt_b01=0, NqperLogDecade=90)
    rows = []
    for b2, bs2 in ((0, 0), (0, 1), (0, -1), (1, 0), (1, 1)):
        settings = dict(basis, clpt_b2=b2, clpt_bs2=bs2)
        output = run_cli_case(executable, scratch / f"shear-{b2}-{bs2}", settings, weak)
        table, _ = addon_tables(output, settings)
        rows.append(table)
    linear = load_table(output / "linear.txt", BASE_TABLES["linear"])
    power = simpson_power(linear, 1e-4, basis["clpt_kmax"])
    for base, plus, minus, density, mixed in zip(*rows):
        r = base[0]
        assert all(row[14] == base[14] for row in (plus, minus, density, mixed))
        zeta = (plus[15] + minus[15]) / 2 - base[15]
        chi = (mixed[15] - density[15]) - (plus[15] - base[15])
        xi0, xi2, xi4, x, y = tidal_hankel(power, r)
        # Eigenvalues of the shear covariance on traceless symmetric tensors.
        lam0 = 2*xi0/15 - 4*xi2/21 + 12*xi4/35
        lam1 = 2*xi0/15 - 2*xi2/21 - 8*xi4/35
        lam2 = 2*xi0/15 + 4*xi2/21 + 2*xi4/35
        expected = (2 * (lam0**2 + 2*lam1**2 + 2*lam2**2), 4*xi2**2/3)
        for label, value, target in zip(("tidal auto", "density-tidal cross"), (zeta, chi), expected):
            if not math.isclose(value, target, rel_tol=4e-3, abs_tol=3e-14):
                raise AssertionError(f"{label} at r={r}: CLPT {value:.12g}, linear Wick contraction {target:.12g}")

        # In the EdS kernel limit, div(V10) is a sum of products of Hankel
        # transforms. Use that identity in place of the loop momentum integral.
        v10 = (6.0 / 7.0) * (-4*xi0**2/45 + 20*xi2**2/63 - 8*xi4**2/35)
        h = 0.045
        *_, x_lower, y_lower = tidal_hankel(power, r - h)
        *_, x_upper, y_upper = tidal_hankel(power, r + h)
        div_div = ((x_upper + x_lower - 2*x + y_upper + y_lower - 2*y) / h**2 +
                   (x_upper - x_lower + 2 * (y_upper - y_lower)) / (h*r) + 2*y/r**2)
        odd = (plus[15] - minus[15]) / 2
        # Allow for the LCDM time factor, the finite cuts, and the weak ZA smoothing.
        if abs(odd - v10 - div_div) > 0.04 * (abs(v10) + abs(div_div)):
            raise AssertionError(f"tidal response at r={r}: CLPT {odd:.12g}, EdS {v10 + div_div:.12g}")


def check_q_samples(qfunctions):
    for expected in BASELINE["q_samples"]:
        row = min(qfunctions, key=lambda current: abs(current[0] - expected[0]))
        for column, (actual, value) in enumerate(zip(row, expected), 1):
            check_value(actual, value, f"q={row[0]} column {column}", 5e-7)


def check_mgpt_convolution(correlations, reference):
    for row, expected in zip(correlations, reference):
        # MGPT interpolates xi_L from the q grid. It also groups U_loop
        # under the xi10_linear heading. Compare the combined xi10 instead.
        for index in (0, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14):
            value, target = row[index], expected[index]
            if index == 5:
                value += row[6]
                target += expected[6]
            if not math.isclose(value, target, rel_tol=2e-4, abs_tol=5e-8):
                raise AssertionError(f"r={row[0]} CLPT column {index + 1}: {value}, MGPT {target}")


def convolve_with_mgpt(source, scratch, compiler, output):
    probe = build_mgpt_probe(source, scratch, compiler, FIXTURES / "mgpt_clpt_probe.c")
    case = scratch / "reference"
    (case / "Input").mkdir(parents=True)
    (case / "Output").mkdir()
    (case / "Input/spectrum.dat").write_bytes(LCDM_SPECTRUM.read_bytes().rstrip(b"\n") + b"\n")
    (case / "Output/qfunctions.dat").write_bytes((output / "qfunctions.txt").read_bytes())
    run([probe, "fnamePS=spectrum.dat", "mgm=LCDM", "Om=0.3", "h=0.67", "zout=0.5",
         *(f"{name}={SETTINGS[name]}" for name in ("rmin", "rmax", "Nr"))], case)
    return load_table(case / "CorrelationFunction.dat", (SETTINGS["Nr"], 15))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    parser.add_argument("--mgpt-reference", type=Path)
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    if args.mgpt_reference is not None and not (args.mgpt_reference / "src/Makefile").is_file():
        parser.error(f"Missing src/Makefile under {args.mgpt_reference}")

    with tempfile.TemporaryDirectory(prefix="fkpt2-clpt-") as temp:
        scratch = Path(temp)
        output = run_cli_case(executable, scratch / "lcdm", SETTINGS)
        correlations, qfunctions = addon_tables(output, SETTINGS)
        assert [row[0] for row in correlations] == [65.0, 90.0, 115.0]
        assert all(row[1] > 0 and row[1] + row[2] > 0 for row in qfunctions)
        check_bias(correlations, SETTINGS)
        check_q_samples(qfunctions)
        check_linear_transform(correlations, qfunctions, load_table(output / "linear.txt", BASE_TABLES["linear"]))
        check_mgpt_convolution(correlations, BASELINE["mgpt_convolution"])
        if args.mgpt_reference is not None:
            ref = convolve_with_mgpt(args.mgpt_reference, scratch, args.cc, output)
            check_mgpt_convolution(correlations, ref)

        check_nuisance_terms(executable, scratch, correlations)

        off = run_cli_case(executable, scratch / "off", dict(SETTINGS, clpt=False))
        assert not (off / "clpt.txt").exists() and not (off / "qfunctions.txt").exists()
        for name in BASE_TABLES:
            assert (off / f"{name}.txt").read_bytes() == (output / f"{name}.txt").read_bytes()

        f5, _ = addon_tables(run_cli_case(executable, scratch / "f5", F5_SETTINGS), F5_SETTINGS)
        check_bias(f5, F5_SETTINGS)
        assert abs(f5[0][14] - correlations[0][14]) > 1e-5
        for key, value, message in (("Nr", 0, "CLPT requires Nr"),
                                    ("clpt_kmax", 0.00001, "clpt_kmin < clpt_kmax")):
            run([executable, "clpt=true", f"{key}={value}"], scratch, expected_error=message)

    print("CLPT CLI: Fourier and small-q limits, MGPT convolution, tidal and local bias, density EFT, and F5 passed")


if __name__ == "__main__":
    main()
