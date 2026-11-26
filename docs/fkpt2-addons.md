# fkPT2 addons

The build discovers `addons/*/addon.mk`. The CLI and Cython extension use the
same registry. Run `./fkpt2 options=print-models` or call
`fkpt2.available_models()` to list the compiled gravity models.
`options=make-info` and `fkpt2.available_addons()` list the addons.

## Run DGP

Select `model=DGP` in either interface. The [DGP addon](../addons/model_DGP/model_dgp.c)
uses the flat LCDM background and the growth and screening terms from MGPT.
The default `eps_DGP=-1` selects the normal branch. Use `eps_DGP=1` for the
self-accelerating branch. `rc_DGP` gives the crossover scale in units of
`H0^-1` and defaults to 1. `screening` multiplies the screening source terms
and defaults to 1.

```sh
./fkpt2 model=DGP eps_DGP=1 rc_DGP=1.4 screening=0.45
```

`make test-dgp` checks growth and kernel coefficients recorded from MGPT
commit `a90ecc2` and compares the linear spectrum samples, loop tables,
and multipoles with the pre-addon fkPT2 output. Set
`MGPT_REFERENCE=/path/to/MGPT` to rebuild the reference in a temporary
directory using the [growth probe](../tests/fixtures/dgp/mgpt_probe.c).
`make test-cython` compares all four Python tables with the verified CLI
and checks the MGPT growth values.

## Add a gravity model

1. Create a directory under `addons/` with a C source file and an `addon.mk`.
   Use a unique source filename, addon name, and model name.
2. Include [fkpt2_addon.h](../include/fkpt2_addon.h). Define a
   `const struct fkpt2_gravity_model` and export a
   `const struct fkpt2_addon` whose `.model` points to it.
3. Register the source path and exported symbol in the manifest.

```make
FKPT2_ADDON_SOURCES += addons/my_gravity/my_gravity.c
FKPT2_ADDON_DESCRIPTORS += fkpt2_my_gravity_addon
```

Run `make all fkpt2py` to build both interfaces. Select the new model with
`model=MyGravity` in a CLI command, configuration file, or Python call.
Model names are case-insensitive. `ADDONSON=0` builds only LCDM.
Hu–Sawicki and DGP are enabled by default. Set `MODELHSON=0` or
`MODELDGPON=0` to disable either addon. `MODELFOLPSNUON=0` disables the
massive-neutrino addon. A manifest can make its own addon optional.

The callbacks receive the run parameters and state. They cover the background
and ODE coefficients (`OmM`, `H`, `f1`, `f2`, `A0`), the effective gravity
`mu(a,k)`, the second-order sources (`sourceA`, `sourceb`, `SD2`), and the
third-order terms (`S3I`, `S3II`, `S3FL`, `S3dI`). The callbacks receive
`eta = log(a)` and wavenumbers in `h/Mpc`. Unspecified callbacks use LCDM.
Provide the background and nonlinear sources required by your gravity theory.
The default one-loop implementation uses the existing large-scale kernel
coefficients with scale-dependent growth.

Declare private numeric settings in `model.options`. The shared parameter
`modelOptions="name=value,other=value"` overrides their defaults. The values
in `gd->model_options` follow the declared order. Use the optional
`.initialize` callback to validate settings and set up `gd->model_state`.
It can set `cmd->error_message` and return `FAILURE` to reject an invalid run.
Storage allocated via the existing `allocate()` helper lives for the
duration of the run. Use automatic or run-owned storage for mutable model data.

Set `.parallel_growth = true` on the gravity descriptor to evaluate growth
and the CLPT kernel grid with OpenMP. Callbacks on these grids run in
arbitrary order. Treat `cmd` and any memory referenced by pointers in `gd`,
including `gd->model_state`, as read-only. Each growth calculation receives a
private copy of the scalar fields in `gd`. Use automatic storage for
callback scratch data. Growth and CLPT kernel callbacks run serially for
models that leave `.parallel_growth` unset.

The FOLPSNu addon uses the optional `.growth_rate_ratio` callback to supply
`f(k, eta) / f_LCDM(eta)` for a flat ΛCDM background. The solver integrates
the associated growth amplitude and takes `f0` from scale-independent
ΛCDM. A model can select `.eds_loop_kernels` for EdS coefficients at the
computed growth rates and `.separate_velocity_dispersion` to integrate
each spectrum's velocity dispersion independently.

The [Hu–Sawicki addon](../addons/model_HS/model_hs.c) shows the full gravity
implementation. The [example addon](../tests/fixtures/gravity_addon/gravity_example.c)
shows one callback, one numeric option, and one analysis diagnostic.

## Add a result table

An addon can provide a `.tables` descriptor with enable, validation, and
compute callbacks. The compute callback receives the prepared solver data and
can allocate a named array with `fkpt2_alloc_addon_table()`. The result owns
the array and the descriptor supplies static column labels. The CLI writes
the array under its name. Python returns it in `result.addon_tables` and
exposes the labels through `result.column_names(...)`. The
[CLPT](fkpt2-clpt.md) and [GSM](fkpt2-gsm.md) addons use this interface.
An addon manifest can list required descriptor symbols in
`FKPT2_ADDON_REQUIRES` to check build dependencies.

## Add a diagnostic

Set `.analyze` on an addon descriptor to read the completed tables through
`fkpt2_result_table()` and return one finite scalar. The addon name becomes
the diagnostic key. Python exposes it in `result.diagnostics` and the CLI
prints it to standard output. Analysis-only addons can omit `.model`.
Return an error status and fill the supplied error buffer if the analysis
fails.

Run `make test-addons` to build the example in an isolated directory and
check gravity selection, model-specific settings, and analysis in both
interfaces.
