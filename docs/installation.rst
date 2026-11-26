Installation
============

Build from the fkPT2 source directory. All shell commands in this guide
start from that directory unless a different location is shown.

Native CLI and library
----------------------

A default build uses a Unix C compiler, GNU Make, and an archiver such as
``ar``. The Makefile defaults to ``gcc``. Select a compiler with ``CC`` if
your installation uses a different executable.

.. code-block:: bash

   make -j4 all
   ./fkpt2 options=print-models
   ./fkpt2 options=make-info

This creates ``fkpt2`` and ``libfkpt2.a`` in the source directory. The
default native build includes the Hu–Sawicki, DGP, FOLPSNu, CLPT, and GSM addons.

Cython module
-------------

Use Python 3.9 or newer and a C compiler with access to that interpreter's
headers. Install the module into the active environment.

.. code-block:: bash

   python3 -m pip install .
   python3 -c "import fkpt2; print(fkpt2.available_models())"

For development, install the build dependencies and build the extension in
place. The extension is imported from the ``python/`` directory.

.. code-block:: bash

   python3 -m pip install setuptools wheel 'Cython>=3' 'numpy>=1.21'
   make fkpt2py PYTHON=python3
   PYTHONPATH="$PWD/python" python3 -c "import fkpt2; print(fkpt2.available_addons())"

Build options
-------------

``Makefile_settings`` selects the build features and ``Makefile_machine``
configures the compiler and linker. The addon switches come from
``addons/Makefile_addons_settings``.

.. list-table::
   :header-rows: 1
   :widths: 33 67

   * - Setting
     - Effect when set to ``0``
   * - ``ADDONSON``
     - Build the LCDM core with the addon registry empty
   * - ``CLASSLIBON``
     - Build without the linked CLASS spectrum source
   * - ``MODELHSON``
     - Omit the Hu–Sawicki gravity model
   * - ``MODELDGPON``
     - Omit the DGP gravity model
   * - ``MODELFOLPSNUON``
     - Omit the massive-neutrino model
   * - ``CLPTON``
     - Omit both CLPT and GSM
   * - ``GSMON``
     - Omit GSM while keeping CLPT available

The default build uses ``OPENMPMACHINE=0``, ``USEGSL=0``, and
``CLASSLIBON=0``. See :doc:`class_workflow` to link the upstream
CLASS library. Preserve the same
native options across the CLI and Cython builds. For example, to omit
Hu–Sawicki in both interfaces:

.. code-block:: bash

   make all MODELHSON=0
   FKPT2_MAKE_ARGS="MODELHSON=0" python3 -m pip install .

.. _openmp-build:

OpenMP
------

Enable OpenMP when building the CLI and Python extension. The compiler must
support OpenMP and its runtime library must be available when the extension
is imported.

.. code-block:: bash

   make OPENMPMACHINE=1 all fkpt2py
   ./fkpt2 numberThreads=4

On macOS with Homebrew GCC, use the installed compiler name, for example
``make OPENMPMACHINE=1 CC=gcc-15 all fkpt2py``. For a pip installation, pass
the native build settings in ``FKPT2_MAKE_ARGS``.

.. code-block:: bash

   FKPT2_MAKE_ARGS="OPENMPMACHINE=1 CC=gcc-15" python3 -m pip install .

Set ``numberThreads`` in a parameter file, on the command line, or as a
Python keyword. Use ``numberThreads=1`` for a single-thread comparison. An
OpenMP build defaults to 16 threads and the runtime can limit the size of
the thread team. OpenMP distributes the growth, one-loop, CLPT, and GSM
grid calculations within each run.

.. code-block:: python

   import fkpt2

   result = fkpt2.compute(fnamePS="tests/fkpt/Input/pkl_z05.dat", numberThreads=4)

Continue with :doc:`quickstart` and use :doc:`validation` to check the
installation. For the Sphinx build and Read the Docs settings, see
:doc:`publishing`.
