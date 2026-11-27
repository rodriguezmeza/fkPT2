# fkPT2

[![Read the Docs build status](https://app.readthedocs.org/projects/fkpt2/badge/?version=stable)](https://fkpt2.readthedocs.io/en/stable/)
[![GitHub release v0.2.0](https://img.shields.io/badge/release-v0.2.0-blue?logo=github)](https://github.com/rodriguezmeza/fkPT2/releases/tag/v0.2.0)
[![Zenodo archive v0.2.0](https://zenodo.org/badge/DOI/10.5281/zenodo.24573150.svg)](https://doi.org/10.5281/zenodo.24573150)
[![GitHub Actions tests](https://github.com/rodriguezmeza/fkPT2/actions/workflows/tests.yml/badge.svg?branch=main&event=push)](https://github.com/rodriguezmeza/fkPT2/actions/workflows/tests.yml)

fkPT2 computes one-loop redshift-space power spectrum multipoles from a
linear spectrum. It supports ΛCDM, Hu–Sawicki f(R) gravity, DGP gravity, and
massive-neutrino cosmologies. Its CLPT and Gaussian Streaming Model addons
compute real-space correlations and redshift-space correlation multipoles.

- **Author** Mario A. Rodriguez-Meza
- **Major contributor** Alejandro Aviles

## Install with pip

Prebuilt wheels are available for **CPython 3.9–3.15** on Linux and macOS,
for Intel and ARM. A matching wheel installs without a C compiler or GNU Make.
Install the [PyPI release](https://pypi.org/project/fkpt2/) into a virtual environment:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install fkpt2
python -c "import fkpt2; print(fkpt2.available_models())"
fkpt2 options=print-models
```

To require prebuilt wheels for fkPT2 and its dependencies:

```sh
python -m pip install --only-binary=:all: fkpt2
```

This reports an unavailable wheel if the environment is outside the supported
matrix. Source installations require a C compiler, GNU Make, and `ar`.

The installation provides the Python interface, the `fkpt2` and
`fkpt2-emulate` commands, the default spectrum, and the tutor documentation.
The commands work from any directory; calculation outputs go to the current
directory's `Output/` by default. For an editable development installation,
use `python -m pip install -e .`. See the
[installation guide](docs/installation.rst)
for platform compatibility, source installations, and build options.

## Build and run

Use GNU Make and a C compiler. The [installation guide](docs/installation.rst)
covers compiler selection, the native static library, and optional build settings.

```sh
git clone https://github.com/rodriguezmeza/fkPT2.git
cd fkPT2
make all
./fkpt2
./fkpt2 model=HS fR0=1e-5
./fkpt2 model=DGP
```

The default ΛCDM run uses a bundled spectrum at redshift 0.5. Supply
`fnamePS` as an explicit path or as a filename in `Input/`. Its first
two columns give `k` in h/Mpc and `P(k)` in (Mpc/h)³ at `zout`.
The default `is_PS_input_LCDM=1` applies the fkPT growth rescaling.
Use `is_PS_input_LCDM=0` for a spectrum computed for the selected model.

Parameter files accept `name = value` or `name value` and allow `#` or
`%` comments. Command-line values override the file, as in
`./fkpt2 run.ini fR0=1e-6`. The results go to `rootDir` (default
`Output`), which contains `rsd_multipoles.txt` and the resolved settings
under `tmp/`. Run `./fkpt2 --help` for settings and see the
[output guide](docs/outputs.rst) for table columns.

## Python and addons

Install the Cython extension with `python -m pip install .`, or build it in
place with `make fkpt2py` and set `PYTHONPATH=python` for a local run.

```python
import fkpt2

result = fkpt2.compute(fnamePS="tests/fkpt/Input/pkl_z05.dat", model="HS", fR0=1e-5)
k, p0, p2, p4 = result.multipoles.T
```

The [Python guide](docs/fkpt2-python.md) covers array input, parameter files,
and results. The [addon guide](docs/fkpt2-addons.md) shows how to register
gravity models and diagnostics.

## CLASS and massive neutrinos

Use `fkpt2.compute_from_class(cosmology, ...)` to generate a linear spectrum
with `classy` and pass it to fkPT2 in memory. Build with `CLASSLIBON=1` to
enable the same in-memory workflow in the C executable. The CLI and Python
can share the settings in [a combined parameter file](scripts/class_fkpt2.ini).

```sh
./fkpt2 scripts/class_fkpt2.ini
bash scripts/run_fkpt2_class.sh scripts/class_fkpt2.ini
```

The second command uses the standalone CLASS executable. See the
[CLASS guide](docs/class_workflow.rst) for setup, the direct Python runner,
and CLASS-only input files.

Use `model=FOLPSNu` with a linear cold dark matter plus baryon spectrum at
`zout`, the total matter fraction `Om`, and `modelOptions=mnu=0.4` for a
total neutrino mass of 0.4 eV. Set `is_PS_input_LCDM=0` to use the input
spectrum directly. The addon writes `cb_power.txt` and the standard
multipoles. See the [massive-neutrino guide](docs/fkpt2-folps-nu.rst) for
CLASS examples and the FOLPS-nu validation.

## Correlation functions and OpenMP

- Use `clpt=true` for real-space matter and biased-tracer correlations and
  Lagrangian displacement correlators. [CLPT guide](docs/fkpt2-clpt.md)
- Use `gsm=true` for the redshift-space correlation multipoles
  `xi0`, `xi2`, and `xi4` from the CLPT velocity moments.
  [Gaussian Streaming Model guide](docs/fkpt2-gsm.md)
- Build with `make OPENMPMACHINE=1 all fkpt2py` and set `numberThreads=4`
  in a CLI or Python run to use OpenMP. [OpenMP setup](docs/installation.rst#openmp)

## Regression tests

The base suite compares the ΛCDM, F5, and F6 results with fkPT and checks
DGP, CLPT, GSM, and the FOLPSNu addon. Install NumPy and fetch the same
reference revision used in CI.

```sh
python3 -m pip install numpy
git clone https://github.com/alejandroaviles/fkpt.git ../fkpt-reference
git -C ../fkpt-reference checkout 6354d80a05724ce2a8c47b88720b9377c47bbdfe
make test TEST_CLASS=0 FKPT_REFERENCE=../fkpt-reference
```

The default `TEST_CLASS=auto` includes the CLASS suite when the CLASS
executable, `libclass.a`, and `classy` are available. Use `TEST_CLASS=1`
to require those dependencies. GitHub Actions exercises both `TEST_CLASS=0`
and `TEST_CLASS=1`. See the [validation guide](docs/validation.rst) for
the input requirements and individual test targets.

## Documentation

Ask the local tutor about the documentation, parameters, APIs, and imported
project discussions:

```sh
./fkpt2 options=IA-tutor
./fkpt2 options=IA-tutor --ask "How do I compute GSM multipoles in Python?"
./fkpt2 options=IA-tutor --backend offline
```

The default uses local Ollama with `qwen3.5:4b` after `ollama pull qwen3.5:4b`
and starting its service. Use `--backend offline` to search documentation
without a model, or `/backend offline` during a conversation. See the
[tutor guide](docs/ia_tutor.rst) for PDF support, private chat
imports, configuration, and source citations.

The [published fkPT2 documentation](https://fkpt2.readthedocs.io/en/stable/) covers installation,
the CLI and Python API, gravity models, and how to add models and observables.
To build the [Sphinx manual](docs/index.rst) locally, use Python 3.11 or newer.

```sh
python3 -m pip install -r docs/requirements.txt
make -C docs html
```

Open `docs/_build/html/index.html`. The [publishing guide](docs/publishing.rst)
covers Read the Docs setup. For a terminal reference, run
`man ./docs/man/fkpt2.1`.

## Citation

If you use fkPT2 in your research, please cite the fkPT paper by Mario A.
Rodriguez-Meza et al. (2024). [JCAP 03 (2024) 049](https://doi.org/10.1088/1475-7516/2024/03/049) ·
[arXiv:2312.10510](https://arxiv.org/abs/2312.10510)

See the [scientific references](docs/references.rst) for papers on each
observable. fkPT2 is distributed under the [MIT License](LICENSE).
