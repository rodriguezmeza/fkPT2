# fkPT2 Python interface

Build from the fkPT2 source directory with a C compiler, GNU Make, and Python
3.9 or newer. Install into the active virtual environment with `python -m pip
install .`. To build in place when Cython, setuptools, and NumPy are already
installed, run `make fkpt2py PYTHON=python3` and add the source directory's
`python/` subdirectory to `PYTHONPATH`. Use `FKPT2_MAKE_ARGS` to pass native
build settings to pip, for example `FKPT2_MAKE_ARGS="MODELHSON=0 CC=clang"`.

```python
import numpy as np
from fkpt2 import FKPT2, available_models, column_names

k, pk = np.loadtxt("tests/fkpt/Input/pkl_z05.dat", usecols=(0, 1), unpack=True)
solver = FKPT2(zout=0.5, Om=0.3, Nk=120)
lcdm = solver.compute(k, pk)
f5 = solver.compute(k, pk, model="HS", fR0=1e-5)
f6 = solver.compute(k, pk, model="HS", fR0=1e-6)
print(available_models())
print(column_names("multipoles"))
print(f5.multipoles[:, 1])
```

Supply `P(k)` in `(Mpc/h)^3` at `zout` and `k` in `h/Mpc`. Arrays must contain
at least ten finite, positive samples with strictly increasing wavenumbers.
For a file input, call `solver.compute(fnamePS="path/to/pk.dat")`. For a
CLI-style configuration file, use `solver.compute(paramfile="run.ini")`.
Use the same `paramfile` argument for a file with `[class]` and `[fkpt2]`
sections. Builds with the CLASS addon use the linked library. Builds
with `CLASSLIBON=0` use the installed `classy` module to compute the
spectrum in memory. See the [CLASS guide](class_workflow.rst).

Parameters supplied directly override values in the file. The default
`is_PS_input_LCDM=1` rescales an LCDM spectrum for the selected model.
Set it to `False` for a spectrum already computed for that model.

Use `fkpt2.compute_from_class(cosmology, ...)` to generate the linear spectrum
and matching solver settings with CLASS. It passes the spectrum arrays to
the native solver in memory. See the [CLASS guide](class_workflow.rst).

For a single run, use `fkpt2.compute(...)` with the same arguments. `fkpt2.compute()` and
the `FKPT2` constructor both accept a `parameters` mapping.
Use `solver.set(...)` to update the settings used by subsequent runs.

| Result attribute | Contents |
| --- | --- |
| `linear` | Extended grid, linear spectrum, `f(k)`, `D+(k)`, and no-wiggle spectrum |
| `kfunctions` | One-loop kernels, bias and RSD terms, linear power, and growth |
| `kfunctions_nw` | The corresponding one-loop table for the no-wiggle spectrum |
| `multipoles` | `k`, `P0`, `P2`, and `P4` on the requested output grid |
| `model`, `z`, `sigma8`, `f0`, `growth` | Model, redshift, `sigma8(z)`, large-scale growth rate, and `D+(z)` normalized to the LCDM value today |
| `diagnostics` | Scalar results from compiled analysis addons |
| `addon_tables` | Named array results from the enabled computation addons |
| `cb_power` | Linear and one-loop cold dark matter plus baryon spectra for FOLPSNu |
| `clpt`, `qfunctions` | Real-space CLPT correlation functions and Lagrangian correlators when requested |
| `gsm`, `clpt_velocity` | Redshift-space correlation multipoles and the CLPT pairwise velocity moments used by GSM |

`result.column_names(...)` lists the columns for any standard or addon table. Results are
independent NumPy arrays. Python calculations run in memory and can take new
parameters and input arrays on each call. Input errors raise `ValueError` and
native solver errors raise `fkpt2.ComputationError`. Multiple Python threads may
submit work. The shared numerical workspace serializes the solves while the
wrapper releases the GIL. With an {ref}`OpenMP build <openmp-build>`,
pass `numberThreads=4` to use up to four threads within each solve.

See the [CLPT guide](fkpt2-clpt.md) and [GSM guide](fkpt2-gsm.md) for examples and settings.

Run `make test-cython` to check the Python tables against the tested CLI.
The C interface is declared in [fkpt2.h](../include/fkpt2.h) and is available in
`libfkpt2.a` after `make all`. Link with `-lm -pthread` and the libraries
required by the selected build options. For an OpenMP build, link with the
same OpenMP runtime as the native library. The caller owns each result until
`fkpt2_result_free()`.
