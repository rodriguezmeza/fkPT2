Command-line usage
==================

Run the executable with a parameter file, individual overrides, or both.

.. code-block:: text

   ./fkpt2 [parameter-file] [name=value ...]

Use the compiled executable to inspect the available settings.

.. code-block:: bash

   ./fkpt2 --help
   ./fkpt2 options=print-models
   ./fkpt2 options=make-info
   ./fkpt2 options=print-options
   ./fkpt2 options=IA-tutor

The :doc:`ia_tutor` is a separate interactive information mode. It accepts tutor
arguments such as ``--ask``, ``--backend offline``, and ``--model qwen3.5:4b``.
It uses local Ollama with ``qwen3.5:4b`` by default.
Launch it directly from one terminal; it does not initialize a calculation.

Standalone discovery commands report the settings for that build. Parameter
names are case-sensitive. Model names, such as ``LCDM`` and ``HS``, are
case-insensitive. The :doc:`params` page lists defaults and aliases.

Parameter files
---------------

Use one setting per line. Both ``name = value`` and ``name value`` are accepted.
The characters ``#`` and ``%`` introduce comments outside quoted values.
Use single or double quotes around a text value that includes a comment
character.

Save the following as ``run.ini`` in the source directory.

.. code-block:: ini

   # F5 using an LCDM linear spectrum at the same redshift
   fnamePS = tests/fkpt/Input/pkl_z05.dat
   model = HS
   fR0 = 1e-5
   zout = 0.5
   Om = 0.3
   h = 0.67
   is_PS_input_LCDM = 1
   Nk = 24
   nquadSteps = 128
   rootDir = "Output F5"
   suffixModel = _f5

Explicit ``name=value`` arguments take precedence over the file. To use the
same settings for F6 and write a separate set of results, run:

.. code-block:: bash

   ./fkpt2 run.ini fR0=1e-6 'rootDir=Output F6' suffixModel=_f6

The file can also be supplied as ``paramfile=run.ini``. Relative input
and output paths are interpreted from the process working directory.
See :doc:`inputs` for the spectrum format and :doc:`outputs` for result names.
