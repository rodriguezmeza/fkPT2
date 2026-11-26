Validation
==========

The tests compare the command-line and Python paths and check selected
quantities against the upstream fkPT and MGPT references.

Fourier-space regression
------------------------

Use the fkPT reference revision pinned by the GitHub Actions workflow.
The bundled example spectra in ``tests/fkpt/Input/`` come from that revision.

.. code-block:: bash

   git clone https://github.com/alejandroaviles/fkpt.git ../fkpt-reference
   git -C ../fkpt-reference checkout 6354d80a05724ce2a8c47b88720b9377c47bbdfe
   make test-fkpt FKPT_REFERENCE=../fkpt-reference

The test builds the reference in a temporary directory and compares the
linear spectrum, both kernel tables, and power multipoles for ΛCDM, F5,
and F6. It also checks parameter-file overrides, model-specific spectrum
input, and the linear ``sigma8`` normalization. The reference source
remains in its own checkout.

DGP and configuration-space regression
--------------------------------------

.. code-block:: bash

   make test-dgp test-clpt test-gsm

The DGP tests check growth on both branches against recorded MGPT values
and compare the four standard output tables with fkPT2 baselines. The CLPT
tests check Fourier transforms, small-separation limits, the MGPT
convolution, tidal bias, and the density counterterm. The GSM tests check
the velocity moments and multipoles, independent streaming quadrature,
the linear Kaiser limit, and the tidal and EFT responses.

Pass ``MGPT_REFERENCE=/path/to/MGPT`` to rebuild the MGPT probes used by
``test-dgp`` and ``test-clpt``. ``make test-default`` runs the Fourier-space,
DGP, and enabled CLPT, GSM, and FOLPSNu tests.

Massive-neutrino regression
---------------------------

.. code-block:: bash

   make test-folps-nu
   make test-folps-nu FOLPS_REFERENCE=/path/to/FOLPS-nu

The first command uses recorded reference tables. The second uses SciPy
and the supplied FOLPS-nu code to compute them at test time. Both check cb
growth, real-space power, and redshift-space multipoles. The Cython test
suite compares those results with the CLI and checks the growth derivative
and the one- and four-thread runs. See :doc:`fkpt2-folps-nu` for details.

Interface and addon regression
------------------------------

With Cython, setuptools, and NumPy installed, run:

.. code-block:: bash

   make test-cython test-addons

The Cython tests compare arrays with the CLI. They also exercise the
input paths, result lifetime, and solver error recovery. The addon tests
build an example model and diagnostic in isolation and check addon
selection and configuration. Use ``TEST_PYTHON=python3`` or specify the
interpreter from the environment you intend to test.

CLASS workflow regression
-------------------------

``TEST_CLASS`` controls the CLASS suite in ``make test``.

- ``TEST_CLASS=auto`` is the default. It includes the CLASS suite when
  its dependencies are available.
- ``TEST_CLASS=0`` runs the base suite only.
- ``TEST_CLASS=1`` requires the CLASS suite and reports every unavailable
  dependency before running tests.

The check looks for a readable ``libclass.a`` in ``CLASS_DIR``, an
executable at ``CLASS_BIN``, and ``classy`` importable with
``TEST_PYTHON`` and ``CLASSY_PATH``.

Run ``make test-class-selection`` to verify the mode selection and required
dependency diagnostics with temporary fixtures. Both ``make test`` and
``make test-default`` include this regression.

With the standalone CLASS executable, run ``make CLASSLIBON=0 test-class-bash``
to compare the saved spectra and fkPT2 tables from combined and CLASS-only
inputs. The test also checks direct fkPT2 reruns from the used-values file.

With ``classy`` installed, run ``make CLASSLIBON=0 test-classy-paramfile``
to check Cython combined files against the in-memory cosmology-mapping
workflow, including keyword overrides and conflicting spectrum inputs.

Run ``make test-class-python-runner`` with ``classy`` installed to compare
the direct Python runner's multipoles from combined and CLASS-only inputs.

Run ``make CLASSLIBON=1 test-class-multipoles`` to test with linked CLASS
or ``make CLASSLIBON=0 test-class-multipoles`` to test Cython, the Bash
runner, and the direct Python runner with the CLASS addon disabled.
The second command builds a separate linked C reference using the same
compiler and OpenMP settings. Both commands require the FOLPSNu addon,
the CLASS archive, the standalone CLASS executable, and ``classy``.

Run ``make test-class-multipoles-both`` to check both build modes in one
command. It runs each mode in a temporary source tree with its own
executable and Cython extension. The temporary builds enable FOLPSNu
and preserve the current build.

For each of ΛCDM and massive neutrinos, the test passes one combined
``[class]`` and ``[fkpt2]`` file to the linked C binary, Cython, the Bash
runner, and the direct Python runner. It compares ``k``, ``P0``, ``P2``,
and ``P4`` between every pair of outputs with a relative tolerance of
``2e-6`` and an absolute tolerance of ``1e-9``.

With the CLASS archive, standalone executable, and ``classy`` set up as
described in :doc:`class_workflow`, ``make test-class`` runs the isolated
multipoles regression with both ``CLASSLIBON=1`` and ``CLASSLIBON=0``.
The suite also exercises the Bash and direct Python runners, compares
CLASS spectra and fkPT2 tables for ΛCDM and massive neutrinos, and checks
that the Python solve runs in memory.

With ``CLASSLIBON=1``, run ``make CLASSLIBON=1 test-class-native``
to compare the linked addon against the Bash workflow. This test checks
ΛCDM, F5, and massive neutrinos and verifies that a linked run can use
its recorded settings again without a saved input spectrum.

Run ``make CLASSLIBON=1 test-class-paramfile`` to compare the combined
parameter file with ``classIni`` in the CLI and Cython. This test also
checks overrides, reruns from the used-values file, and input validation.
A CLASS-enabled ``make test-cython`` includes the test.

OpenMP regression
-----------------

With the Cython build dependencies installed, run:

.. code-block:: bash

   make OPENMPMACHINE=1 test-openmp

Use the compiler from your :ref:`OpenMP build <openmp-build>`. The test
compares one- and four-thread results in the CLI and Cython for ΛCDM,
F5 GSM with tidal bias and EFT, and DGP GSM with the Bulirsch–Stoer ODE
integrator. It also checks that both interfaces report a calculation
error from a worker and that the solver accepts a subsequent valid run.
