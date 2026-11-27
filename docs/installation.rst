Installation
============

Install a published release with pip, or build from the fkPT2 source directory.
Source build commands below start from that directory.

Pip installation
----------------

Install the `PyPI release <https://pypi.org/project/fkpt2/>`_ into a virtual
environment. Matching prebuilt wheels need no C compiler, GNU Make, or source
checkout. See the supported CPython versions and platforms below.

.. code-block:: bash

   python3 -m venv .venv
   source .venv/bin/activate
   python -m pip install fkpt2
   python -c "import fkpt2; print(fkpt2.available_models())"
   fkpt2 options=print-models
   fkpt2

This installs the Cython extension and Python helpers, plus the ``fkpt2``
and ``fkpt2-emulate`` commands. The wheel contains both native executables,
the default spectrum, and the tutor documentation; it does not need the
source checkout at runtime. Run the commands from any working directory.
The default run writes to that directory's ``Output/``. Native Windows
builds are not supported; use a Linux environment such as WSL.

The installed tutor uses the virtual environment's Python::

   fkpt2 options=IA-tutor --backend offline --ask "How do I compute GSM multipoles?"

The ``fkpt2 0.1.0`` release provides 42 prebuilt wheels for regular CPython
3.9 through 3.15 with this platform matrix:

.. list-table::
   :header-rows: 1

   * - System
     - Architectures
     - Binary compatibility baseline
   * - Linux (glibc)
     - x86_64 and aarch64
     - manylinux_2_28 (glibc 2.28+)
   * - Alpine Linux (musl)
     - x86_64 and aarch64
     - musllinux_1_2 (musl 1.2+)
   * - macOS
     - Intel x86_64 and Apple Silicon arm64
     - macOS 11+

To require binary distributions for fkPT2 and its dependencies, use::

   python -m pip install --only-binary=:all: fkpt2

This reports an unavailable wheel instead of falling back to compilation.
PyPy, free-threaded CPython, Windows, and other architectures are outside this
release wheel matrix. For source installations, including ``pip install .``
from this checkout, install a C compiler with Python headers, GNU Make, and
``ar``. Source build dependencies are installed automatically by pip.

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
