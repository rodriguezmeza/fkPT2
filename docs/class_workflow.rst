Generate linear spectra with CLASS
==================================

Use CLASS to compute the input spectrum from a cosmology, then pass it to
fkPT2 at the same redshift. The Python interface passes NumPy arrays
directly to the Cython solver. A CLASS-enabled fkPT2 executable can also
produce and use the spectrum in memory. The Bash runner uses the
standalone CLASS and fkPT2 executables and retains the input spectrum
with the results.

Set up CLASS
------------

Build fkPT2 and its Python extension as described in :doc:`installation`.
The examples use the supplied CLASS checkout, which sits beside the fkPT2
source directory in the tree below. Adjust ``class_dir`` for another checkout.

.. code-block:: bash

   class_dir="$PWD/../xother_files/github_codes_to-study/class_public_2026-11-25"
   make -C "$class_dir" -j4 class
   python3 -m pip install --target "$PWD/build/classy" "$class_dir"

Use compilers from the same C and C++ toolchain for CLASS and ``classy``.
On macOS with Homebrew GCC 15, the corresponding commands are:

.. code-block:: bash

   make -C "$class_dir" -j4 class CC=gcc-15 \
      CPP='g++-15 -std=c++11 -fpermissive -Wno-write-strings'
   CC=gcc-15 CXX=g++-15 python3 -m pip install --target "$PWD/build/classy" "$class_dir"

The Python runner automatically finds a project-local ``build/classy``
installation. For interactive Python or a notebook, start the environment
with the in-place module and local CLASS package on the import path.

.. code-block:: bash

   export PYTHONPATH="$PWD/python:$PWD/build/classy${PYTHONPATH:+:$PYTHONPATH}"

An existing working ``classy`` installation in the active environment also
works. CLASS is an optional dependency for reading power spectra supplied
as files or arrays.

Python with spectra in memory
-----------------------------

Use CLASS parameter names for the cosmology. ``z_pk`` is the redshift of
the input spectrum and the output of fkPT2. Set it in the CLASS mapping
or use the ``redshift`` keyword.

.. code-block:: python

   import fkpt2

   cosmology = {
       "h": 0.6711,
       "omega_b": 0.0220684,
       "omega_cdm": 0.120926,
       "A_s": 2.1e-9,
       "n_s": 0.965,
       "z_pk": 0.5,
       "P_k_max_h/Mpc": 10.0,
   }
   result = fkpt2.compute_from_class(cosmology, Nk=120)
   k, p0, p2, p4 = result.multipoles.T

Pass optional solver settings as keywords or as a ``parameters`` mapping.
``FKPT2.compute_from_class()`` also uses any settings stored on the solver.
The convenience function ``fkpt2.linear_spectrum_from_class()`` returns
``(k, power, settings)`` for repeated calculations with different biases.

.. code-block:: python

   k_input, pk_input, settings = fkpt2.linear_spectrum_from_class(cosmology)
   sample = fkpt2.compute(k_input, pk_input, parameters=settings, b1=1.8)

CLASS uses inverse Mpc and cubic Mpc in its Python API. The helper passes
:math:`k` in :math:`h/\mathrm{Mpc}` and :math:`P(k)` in
:math:`(\mathrm{Mpc}/h)^3` to fkPT2. It samples on CLASS's native wavenumber
grid by default. You can supply your own increasing array with the
``k`` keyword in :math:`h/\mathrm{Mpc}`.

The direct Python runner accepts a CLASS-only input file or the combined
file with ``[class]`` and ``[fkpt2]`` sections. It uses ``classy`` to
compute the spectrum in memory and prints the multipoles to standard output.
Run either command from the fkPT2 source directory.

.. code-block:: bash

   python3 scripts/run_fkpt2_class.py scripts/class_lcdm.ini Nk=120
   python3 scripts/run_fkpt2_class.py scripts/class_fkpt2.ini Nk=120

In the combined file, settings under ``[fkpt2]`` supply solver values.
Arguments such as ``Nk=120`` override those values.

Run CLASS inside the fkPT2 executable
-------------------------------------

Build the upstream CLASS static library and enable the fkPT2 addon with
``CLASSLIBON=1``. The default ``CLASS_DIR`` points to the supplied checkout.
Set it explicitly to use another checkout with the corresponding
``libclass.a``, headers, and CLASS data files.

.. code-block:: bash

   class_dir="$PWD/../xother_files/github_codes_to-study/class_public_2026-11-25"
   make -C "$class_dir" -j4 libclass.a
   make -j4 CLASSLIBON=1 CLASS_DIR="$class_dir" all

Set ``CLASS_CXX`` to the C++ compiler used for CLASS. For example, with
Homebrew GCC 15 on macOS, use:

.. code-block:: bash

   make -C "$class_dir" -j4 libclass.a CC=gcc-15 \
      CPP='g++-15 -std=c++11 -fpermissive -Wno-write-strings'
   make -j4 CLASSLIBON=1 CLASS_DIR="$class_dir" CC=gcc-15 CLASS_CXX=g++-15 all

Pass a CLASS input file with ``classIni``. This uses the linked CLASS
library and writes the standard fkPT2 results. The run records the
CLASS input file for reproducibility and uses the linear spectrum
directly from memory.

.. code-block:: bash

   ./fkpt2 classIni=scripts/class_lcdm.ini Nk=120 rootDir=Output_CLASS_native
   ./fkpt2 classIni=scripts/class_folps_nu.ini Nk=120 rootDir=Output_NU_native

Use ``classIni`` as the spectrum input in place of ``fnamePS``. You can
set it in a fkPT2 parameter file or on the command line. With
``CLASSLIBON=1`` on the Cython build, ``fkpt2.compute(classIni=...)``
uses the same linked addon.

Compare the linked calculation with the standalone Bash pipeline using
``make CLASSLIBON=1 test-class-native``. Build the CLASS ``class`` target
for this comparison and use the same reference checkout.

Use one parameter file
----------------------

Put the CLASS settings under ``[class]`` and the fkPT2 settings under
``[fkpt2]``. With ``CLASSLIBON=1``, the CLI and Cython use the linked
CLASS addon. With ``CLASSLIBON=0``, Cython uses the installed ``classy``
module and passes the spectrum to fkPT2 in memory.

.. code-block:: ini

   [class]
   h = 0.6711
   omega_b = 0.0220684
   omega_cdm = 0.120926
   A_s = 2.1e-9
   n_s = 0.965
   z_pk = 0.5

   [fkpt2]
   model = LCDM
   Nk = 120
   rootDir = Output_CLASS_combined

Run the bundled example from the fkPT2 source directory. The direct CLI
command uses a build with the linked CLASS addon.

.. code-block:: bash

   ./fkpt2 scripts/class_fkpt2.ini

.. code-block:: python

   import fkpt2

   result = fkpt2.compute(paramfile="scripts/class_fkpt2.ini")
   k, p0, p2, p4 = result.multipoles.T

Supply CLI overrides or Python keyword arguments for settings in
``[fkpt2]`` as usual. Use one spectrum source per run. The
``[class]`` section supplies the CLASS cosmology, ``classIni`` selects a
separate CLASS input file for the linked addon, and ``fnamePS`` selects a
spectrum file. The linked CLI includes the ``[class]`` block in its
used-values file so that the record can be used as a new input.

With ``classy`` installed, run ``make CLASSLIBON=0 test-classy-paramfile``
to compare the Cython parameter-file and cosmology-mapping paths.

Run the two executables from Bash
---------------------------------

The Bash runner also accepts the combined file. Build fkPT2 with
``CLASSLIBON=0`` and use the standalone CLASS executable.

.. code-block:: bash

   make CLASSLIBON=0
   bash scripts/run_fkpt2_class.sh scripts/class_fkpt2.ini

Pass ``key=value`` arguments to override entries in ``[fkpt2]``. You can
also give the runner a CLASS-only input file and pass fkPT2 settings as
arguments.

.. code-block:: bash

   bash scripts/run_fkpt2_class.sh scripts/class_lcdm.ini Nk=120 rootDir=Output_CLASS
   bash scripts/run_fkpt2_class.sh scripts/class_folps_nu.ini Nk=120 rootDir=Output_CLASS_NU

The script looks for CLASS in the supplied checkout and fkPT2 in this
source directory. Set ``CLASS_BIN`` or ``FKPT2_BIN`` to select another
executable. The runner uses Python's standard library to prepare the input
and read CLASS's background output.

The script writes ``class_linear.dat`` alongside the standard fkPT2 output
tables. ``suffixModel`` applies to the saved spectrum and the other tables.
The file in ``rootDir/tmp/`` records the resolved fkPT2 settings and the
saved spectrum path. Pass that file directly to ``fkpt2`` to repeat the run.

Run ``make CLASSLIBON=0 test-class-bash`` to compare the combined and
CLASS-only input paths and check that the saved settings can repeat the run.

Cosmology shared by the two interfaces
--------------------------------------

All three workflows obtain ``h`` and total ``Om`` from the present-day CLASS
background. Specify a single ``z_pk`` in the CLASS file. The default is
``0.5``. CLASS is configured for linear power and uses a default upper
wavenumber of 10 h/Mpc when the input omits it. Increase the upper
wavenumber if your calculation needs more small-scale input.

The workflows support flat ΛCDM and the :doc:`fkpt2-folps-nu` approximation
for massive neutrinos. The default model is ``LCDM`` in the massless case.
Select ``model=HS`` or ``model=DGP`` to use the generated ΛCDM spectrum as
the input for fkPT2's growth rescaling. With massive neutrinos the runners
select ``FOLPSNu``, pass linear cold-dark-matter-plus-baryon power, and
derive the addon mass parameter from the CLASS non-cold density using
:math:`m_\nu=93.14\,h^2\Omega_\nu\,\mathrm{eV}`. The example uses three
mass-degenerate neutrinos.

Run ``make test-class`` to compare the CLASS workflows and check the
multipoles in both linked and CLASS-disabled builds. The suite uses the
CLASS archive, the standalone executable, and ``classy``. Set ``CLASS_DIR``,
``CLASS_BIN``, and ``CLASSY_PATH`` to use custom installations.
