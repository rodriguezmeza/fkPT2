fkPT2
=====

**fkPT2** predicts one-loop redshift-space power spectrum multipoles from a
linear matter power spectrum. It supports ΛCDM, Hu–Sawicki :math:`f(R)`,
and Dvali–Gabadadze–Porrati (DGP) gravity. The FOLPSNu addon computes
power spectra for massive-neutrino cosmologies. The CLPT and Gaussian
Streaming Model addons predict real-space correlations and redshift-space
correlation multipoles.

Use the native ``fkpt2`` executable for file-based runs, the Cython module
``fkpt2`` for NumPy workflows, or the static library ``libfkpt2.a`` for C callers.

Basic usage
-----------

Install from PyPI and run a small ΛCDM calculation with the bundled spectrum.
Matching wheels for CPython 3.9–3.15 on Linux and macOS need no compiler.
See :doc:`installation` for supported platforms and the binary-only option.

.. code-block:: bash

   python3 -m venv .venv
   source .venv/bin/activate
   python -m pip install fkpt2
   fkpt2 options=print-models
   fkpt2 Nk=12 nquadSteps=64 rootDir=Output_quick

The file ``Output_quick/rsd_multipoles.txt`` contains ``k``, ``P0``, ``P2``, and
``P4``. See :doc:`quickstart` for the same run in Python and the optional CLPT
and GSM output.

Citation
--------

If you use fkPT2 in your research, please cite the fkPT paper by Mario A.
Rodriguez-Meza et al. (2024). See :doc:`references` for the citation and
papers on each observable.

.. toctree::
   :maxdepth: 2
   :caption: User guide

   overview
   installation
   quickstart
   command_line
   ia_tutor
   inputs
   class_workflow
   outputs
   models
   fkpt2-python

.. toctree::
   :maxdepth: 2
   :caption: Tutorials

   fkpt2-clpt
   fkpt2-gsm
   fkpt2-folps-nu

.. toctree::
   :maxdepth: 2
   :caption: Reference

   params
   api
   fkpt2-addons
   validation
   troubleshooting
   references
   publishing
