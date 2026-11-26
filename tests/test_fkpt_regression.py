#!/usr/bin/env python3
"""Compare the CLI against a fresh build of the supplied fkPT reference."""

import argparse
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_REFERENCE = PROJECT.parent / "xother_files/fkpt_github/fkpt_2026-10-23"
LCDM_SPECTRUM = PROJECT / "tests/fkpt/Input/pkl_z05.dat"
F5_SPECTRUM = PROJECT / "tests/fkpt/Input/pklin_F5z045.dat"
TABLE_SHAPES = {
    "linear": (800, 5),
    "kfunctions": (120, 30),
    "kfunctions_nw": (120, 30),
    "rsd_multipoles": (120, 4),
}


def run(args, cwd, expected_error=None, env=None):
    process = subprocess.run(
        args, cwd=cwd, env=env, capture_output=True, text=True, timeout=120
    )
    if expected_error is None:
        passed = process.returncode == 0
    else:
        # A signalled process never counts as a clean validation error.
        passed = process.returncode > 0 and expected_error in process.stderr
    if not passed:
        raise AssertionError(
            f"{args} in {cwd} exited {process.returncode}\n"
            f"stdout:\n{process.stdout}\nstderr:\n{process.stderr}"
        )
    return process.stdout


def build_reference(source, scratch, compiler):
    build = scratch / "reference-build"
    build.mkdir()
    shutil.copyfile(source / "Makefile", build / "Makefile")
    for directory in ("src", "libs"):
        shutil.copytree(
            source / directory, build / directory,
            ignore=shutil.ignore_patterns("*.o", "*.d", ".DS_Store"),
        )
    # The old reference typedefs bool. Compile that unchanged source as GNU C17.
    env = os.environ.copy()
    for name in ("MAKEFLAGS", "MAKEOVERRIDES", "MFLAGS", "MAKELEVEL"):
        env.pop(name, None)
    run(["make", "CC=" + compiler, "CFLAGS=-O3 -g -std=gnu17", "all"], build, env=env)
    return build / "fkpt"


def load_table(path, shape):
    rows = []
    with path.open() as file:
        for line in file:
            line = line.split("#", 1)[0].strip()
            if line:
                rows.append(tuple(map(float, line.split())))
    if len(rows) != shape[0] or any(len(row) != shape[1] for row in rows):
        raise AssertionError(f"{path} does not have expected shape {shape}")
    if any(not math.isfinite(value) for row in rows for value in row):
        raise AssertionError(f"{path} contains nonfinite values")
    if any(a[0] >= b[0] for a, b in zip(rows, rows[1:])):
        raise AssertionError(f"{path} has a nonincreasing k grid")
    return rows


def compare_tables(candidate, reference, label):
    exact = True
    for table, shape in TABLE_SHAPES.items():
        a = load_table(candidate / f"{table}.txt", shape)
        path = reference / f"{table}.dat"
        if table != "rsd_multipoles":
            path = reference / "Output" / path.name
        b = load_table(path, shape)

        # Linear spectra are printed with 10 significant digits, and the
        # kernel and multipole tables are printed with 7. Leave room for
        # rounding and small floating-point differences between compilers.
        rtol = 2e-8 if table == "linear" else 2e-6
        for i, (actual, expected) in enumerate(zip(a, b), 1):
            for j, (x, y) in enumerate(zip(actual, expected), 1):
                exact = exact and x == y
                if not math.isclose(x, y, rel_tol=rtol, abs_tol=1e-10):
                    raise AssertionError(
                        f"{label} {table} row {i}, column {j}: fkPT2={x:.10g}, fkPT={y:.10g}"
                    )
    agreement = "identical at written precision" if exact else "within numerical tolerance"
    print(f"{label}: linear, both loop tables, and P0/P2/P4 {agreement}", flush=True)


def reference_case(ref, scratch, name, settings):
    directory = scratch / f"reference-{name}"
    input_dir = directory / "Input"
    input_dir.mkdir(parents=True)
    data = LCDM_SPECTRUM.read_bytes()
    # fkPT skips a final row without a newline. Let both solvers read the
    # entire supplied table without changing the original file or reference code.
    if not data.endswith(b"\n"):
        data += b"\n"
    (input_dir / "pkl_z05.dat").write_bytes(data)
    run([ref, *settings], directory)
    return directory


def check_sigma8(stdout, linear):
    match = re.search(r"sigma8\(z=0\.5\)=([0-9eE.+-]+)", stdout)
    if not match:
        raise AssertionError("Missing sigma8 diagnostic")
    measured = float(match[1])

    # Independently integrate the tabulated spectrum in log(k) with a
    # composite trapezoid rule. Interpolate the endpoint at 1 h/Mpc.
    samples = []
    for k, power, *_ in load_table(linear, TABLE_SHAPES["linear"]):
        if k >= 1.0:
            power = prev_power + (power - prev_power) * (1.0 - prev_k) / (k - prev_k)
            k = 1.0
        x = 8.0 * k
        window = 3.0 * (math.sin(x) - x * math.cos(x)) / x**3
        samples.append((math.log(k), k**3 * power * window**2))
        if k == 1.0:
            break
        prev_k, prev_power = k, power
    integral = math.fsum(
        (b - a) * (fa + fb) / 2
        for (a, fa), (b, fb) in zip(samples, samples[1:])
    )
    expected = math.sqrt(integral / (2 * math.pi**2))
    if not math.isclose(measured, expected, rel_tol=2e-5):
        raise AssertionError(f"sigma8={measured}, independent integral={expected}")


def check_direct_spectrum(fkpt2, input_file, scratch):
    case = scratch / "direct-spectrum"
    case.mkdir()
    # A path with a space and comment character also tests explicit paths.
    spectrum = case / "F5 spectrum #1.dat"
    shutil.copyfile(input_file, spectrum)
    run([fkpt2, "model=HS", "fR0=1e-5", "zout=0.45", f"fnamePS={spectrum}",
         "is_PS_input_LCDM=0"], case)
    output = case / "Output"
    linear = load_table(output / "linear.txt", TABLE_SHAPES["linear"])
    unscaled = load_table(output / "tmp/F5 spectrum #1.dat_ext.dat", (800, 2))
    # The diagnostic table has six significant digits. Both sequences should
    # represent the same k and P(k), and the growth rate should still be HS.
    for final, before in zip(linear, unscaled):
        if any(not math.isclose(x, y, rel_tol=5e-6) for x, y in zip(final[:2], before)):
            raise AssertionError("Direct input changed the linear spectrum")
    if math.isclose(linear[0][2], linear[-1][2], rel_tol=1e-3):
        raise AssertionError("Direct HS input lost scale-dependent growth")
    if "# z=0.45," not in (output / "linear.txt").read_text():
        raise AssertionError("Output did not record the direct spectrum redshift")
    for table, shape in TABLE_SHAPES.items():
        load_table(output / f"{table}.txt", shape)


def check_startup_failures(fkpt2, scratch):
    case = scratch / "invalid-input"
    case.mkdir()
    malformed_power = case / "malformed.dat"
    malformed_power.write_text("0.1 10\n0.1 11\n")
    invalid_file = case / "invalid.ini"
    invalid_file.write_text("fR0 = 1e-5\nfr0 = 1e-6\n")
    occupied = case / "occupied"
    occupied.write_text("this is a file, not a directory\n")
    blocked_dump = case / "blocked-dump"
    (blocked_dump / "tmp/parameters_null-fkpt2-usedvalues").mkdir(parents=True)
    for args, message in (
        (["Nk=1"], "Nk must be at least 2"),
        (["nquadSteps=801"], "nquadSteps exceeds"),
        (["kmin=nan"], "parameter 'kmin'"),
        (["zout=-1"], "zout must be greater than -1"),
        (["dtau=1/0"], "denominator"),
        (["is_PS_input_LCDM=2"], "is_PS_input_LCDM must be 0 or 1"),
        ([f"fnamePS={malformed_power}"], "strictly increasing wavenumbers"),
        ([f"fnamePS={case / 'missing.dat'}"], "Cannot open linear spectrum"),
        ([str(invalid_file)], "duplicate parameter"),
        ([f"rootDir={occupied}"], "Cannot create output directory"),
        ([f"rootDir={blocked_dump}"], "parameters_null-fkpt2-usedvalues"),
    ):
        run([fkpt2, *args], case, expected_error=message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--cc", default="gcc", help="compiler for the reference")
    args = parser.parse_args()
    fkpt2 = args.fkpt2.resolve()
    source = args.reference.resolve()
    if not fkpt2.is_file():
        parser.error(f"fkPT2 executable not found at {fkpt2}")
    for required in ("Makefile", "src/main.c"):
        if not (source / required).is_file():
            parser.error(f"{source} must be the fkPT reference source directory; missing {required}")

    with tempfile.TemporaryDirectory(prefix="fkpt2-regression-") as tmp:
        scratch = Path(tmp)
        ref = build_reference(source, scratch, args.cc)

        lcdm = scratch / "lcdm"
        lcdm.mkdir()
        run([fkpt2], lcdm)
        ref_lcdm = reference_case(ref, scratch, "lcdm", [])
        compare_tables(lcdm / "Output", ref_lcdm, "LCDM defaults")
        if list((lcdm / "Output/tmp").glob("*.log")):
            raise AssertionError("Default run unexpectedly wrote a log")

        empty = scratch / "empty-parameters"
        empty.mkdir()
        (empty / "empty.ini").touch()
        verbose = run([fkpt2, "empty.ini", "verbose=1"], empty)
        compare_tables(empty / "Output", ref_lcdm, "LCDM empty parameter file")
        check_sigma8(verbose, empty / "Output/linear.txt")

        for name, f_r0 in (("F5", "1e-5"), ("F6", "1e-6")):
            settings = ["model=HS", f"fR0={f_r0}"]
            reference = reference_case(ref, scratch, name, settings)
            candidate = scratch / name
            candidate.mkdir()
            (candidate / "Input").mkdir()
            shutil.copyfile(LCDM_SPECTRUM, candidate / "Input/pkl_z05.dat")
            run([fkpt2, *settings], candidate)
            compare_tables(candidate / "Output", reference, name)

        # Exercise typed file values and CLI alias precedence with a different
        # physical setup from the defaults. Only the file asks for logging.
        config = scratch / "file-parameters"
        config.mkdir()
        out = config / "nested/output with spaces"
        params = config / "run.ini"
        nuisance = ["b1=1.8", "b2=0.4", "bs2=-0.25", "b3nl=0.2", "alpha0=-2",
                    "alpha2=3", "alpha4=-1", "ctilde=0.1", "pshotp=3500",
                    "alpha0shot=0.8", "alpha2shot=0.2"]
        params.write_text(
            "\n".join(["model HS", "fR0=1e-6", "verbose_log=1", "dtau=0.4",
                       "quadratureMethod = trapezoid3", "quadratureMethod trapezoid3",
                       f'rootDir="{out}"  # directory for this run', *nuisance]) + "\n"
        )
        run([fkpt2, str(params), "fr0=1e-5"], config)
        ref_file = reference_case(ref, scratch, "file-parameters",
                                  ["model=HS", "fR0=1e-5", *nuisance])
        compare_tables(out, ref_file, "F5 with file settings and CLI override")
        if not (out / "tmp/cballs.log").is_file():
            raise AssertionError("verbose_log=1 in the parameter file did not produce a log")

        rerun = config / "roundtrip"
        run([fkpt2, str(out / "tmp/run.ini-usedvalues"), f"rootDir={rerun}", "verbose_log=0"], config)
        compare_tables(rerun, ref_file, "F5 used-values round trip")

        check_direct_spectrum(fkpt2, F5_SPECTRUM, scratch)
        check_startup_failures(fkpt2, scratch)
        print("Direct HS input, sigma8, and startup validation checks passed", flush=True)


if __name__ == "__main__":
    main()
