#!/usr/bin/env python3
"""Compare DGP with MGPT growth and the pre-move fkPT2 spectrum tables."""

import argparse
import json
import math
import os
from pathlib import Path
import re
import shutil
import tempfile

from test_fkpt_regression import LCDM_SPECTRUM, PROJECT, TABLE_SHAPES, load_table, run


FIXTURE_DIR = PROJECT / "tests/fixtures/dgp"
FIXTURE = json.loads((FIXTURE_DIR / "expected.json").read_text())
DGP_CASES = FIXTURE["cases"]
DGP_TABLE_SHAPES = {
    name: (rows if name == "linear" else FIXTURE["settings"]["Nk"], columns)
    for name, (rows, columns) in TABLE_SHAPES.items()
}


def dgp_options(case):
    return {**FIXTURE["settings"], **case["overrides"]}


def check_value(actual, expected, field, relative_tolerance):
    if not math.isclose(actual, expected, rel_tol=relative_tolerance, abs_tol=1e-10):
        raise AssertionError(f"{field}: got {actual:.12g}, expected {expected:.12g}")


def check_growth(linear, reference, label):
    # DGP's linear growth is scale-independent on this background.
    for k, _, f, dplus, _ in linear:
        check_value(f, reference["f0"], f"{label} f(k) at k={k}", 2e-7)
        check_value(dplus, reference["Dplus"], f"{label} D+(k) at k={k}", 2e-7)


def check_summary(growth, sigma8, case, label, tolerance):
    check_value(growth, case["mgpt"]["growth"], f"{label} normalized D+(z)", tolerance)
    check_value(sigma8, case["fkpt2"]["sigma8"], f"{label} sigma8(z)", tolerance)


def stdout_value(stdout, pattern):
    match = re.search(pattern + r"\s*=\s*([0-9.eE+-]+)", stdout)
    if not match:
        raise AssertionError(f"Missing fkPT2 diagnostic {pattern!r}")
    return float(match.group(1))


def run_cli_case(executable, directory, name, case):
    directory.mkdir()
    options = [f"{key}={value}" for key, value in dgp_options(case).items()]
    stdout = run([executable, *options, f"fnamePS={LCDM_SPECTRUM}", "verbose=1"], directory)
    output = directory / "Output"
    tables = {
        table: load_table(output / f"{table}.txt", shape)
        for table, shape in DGP_TABLE_SHAPES.items()
    }
    label = f"DGP {name}"
    check_growth(tables["linear"], case["mgpt"], label)
    # The CLI's scalar diagnostics are written with six significant digits.
    check_summary(stdout_value(stdout, r"D_\+"), stdout_value(stdout, r"sigma8\(z=[^)]*\)"),
                  case, label, 6e-6)
    for quantity in ("A", "KR1"):
        check_value(stdout_value(stdout, quantity + "_LCDM"), case["mgpt"][quantity],
                    f"{label} {quantity}", 6e-6)

    expected = case["fkpt2"]
    for index, row in expected["linear_samples"].items():
        for column, (actual, value) in enumerate(zip(tables["linear"][int(index)], row), 1):
            check_value(actual, value, f"{label} linear row {index}, column {column}", 2e-8)
    for table in ("kfunctions", "kfunctions_nw", "rsd_multipoles"):
        assert len(expected[table]) == len(tables[table])
        for index, (actual_row, expected_row) in enumerate(zip(tables[table], expected[table]), 1):
            assert len(actual_row) == len(expected_row)
            for column, (actual, value) in enumerate(zip(actual_row, expected_row), 1):
                check_value(actual, value, f"{label} {table} row {index}, column {column}", 2e-6)
    return output


def build_mgpt_probe(source, scratch, compiler, probe=FIXTURE_DIR / "mgpt_probe.c"):
    build = scratch / "mgpt-build"
    shutil.copytree(source / "src", build / "src",
                    ignore=shutil.ignore_patterns("*.o", "*.d", "mgpt", "*.dSYM", ".DS_Store"))
    shutil.copyfile(probe, build / "src/main.c")
    env = os.environ.copy()
    for name in ("MAKEFLAGS", "MAKEOVERRIDES", "MFLAGS", "MAKELEVEL"):
        env.pop(name, None)
    run(["make", f"CC={compiler}", "CFLAGS=-O3 -g -std=gnu17 -include stdbool.h", "all"],
        build / "src", env=env)
    return build / "mgpt"


def check_live_mgpt(probe, directory, case):
    input_dir = directory / "Input"
    input_dir.mkdir(parents=True)
    # MGPT's reader requires a final newline to include the last input row.
    (input_dir / "spectrum.dat").write_bytes(LCDM_SPECTRUM.read_bytes().rstrip(b"\n") + b"\n")
    settings = dgp_options(case)
    stdout = run([
        probe, "mgm=DGP", "fnamePS=spectrum.dat", "solverMethod=rkqs",
        f'Om={settings["Om"]}', f'h={settings["h"]}', f'zout={settings["zout"]}',
        f'etaini={settings["tauini"]}', f'epsSolver={settings["eps"]}',
        f'epsDGP={settings.get("eps_DGP", -1)}', f'rcDGP={settings.get("rc_DGP", 1)}',
        f'screening={settings.get("screening", 1)}',
    ], directory)
    match = re.search(r"^DGP regression (.+)$", stdout, re.M)
    if not match:
        raise AssertionError(f"MGPT did not produce the growth and kernel diagnostics:\n{stdout}")
    values = list(map(float, match[1].split()))
    assert len(values) == 5
    for name, actual in zip(("Dplus", "f0", "growth", "A", "KR1"), values):
        check_value(actual, case["mgpt"][name], f"MGPT {name}", 2e-7)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fkpt2", type=Path, default=PROJECT / "fkpt2")
    parser.add_argument("--mgpt-reference", type=Path)
    parser.add_argument("--cc", default="gcc", help="compiler for the optional MGPT build")
    args = parser.parse_args()
    executable = args.fkpt2.resolve()
    if not executable.is_file():
        parser.error(f"fkPT2 executable not found at {executable}")
    if args.mgpt_reference is not None and not (args.mgpt_reference / "src/Makefile").is_file():
        parser.error(f"Missing src/Makefile under {args.mgpt_reference}")

    with tempfile.TemporaryDirectory(prefix="fkpt2-dgp-") as temp:
        scratch = Path(temp)
        probe = (build_mgpt_probe(args.mgpt_reference, scratch, args.cc)
                 if args.mgpt_reference is not None else None)
        for name, case in DGP_CASES.items():
            if probe is not None:
                check_live_mgpt(probe, scratch / f"mgpt-{name}", case)
            run_cli_case(executable, scratch / f"cli-{name}", name, case)
            reference = "fresh MGPT build" if probe is not None else "MGPT fixture"
            print(f"DGP {name}: growth and kernel coefficients match the {reference}; "
                  "linear samples, both loop tables, and multipoles match the fkPT2 baseline",
                  flush=True)


if __name__ == "__main__":
    main()
