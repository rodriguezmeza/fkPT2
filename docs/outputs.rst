Outputs and file formats
========================

The CLI writes whitespace-separated numeric tables to ``rootDir``, which
defaults to ``Output``. Header lines begin with ``#``. The Python module
returns NumPy arrays in memory. Use ``result.column_names(table)`` to
retrieve their column labels.

.. list-table::
   :header-rows: 1
   :widths: 29 23 48

   * - CLI file
     - Python attribute
     - Contents
   * - ``linear.txt``
     - ``linear``
     - ``k``, prepared linear power, ``fk``, ``Dplus``, and no-wiggle linear power
   * - ``kfunctions.txt``
     - ``kfunctions``
     - One-loop kernels, bias and redshift-space terms, and linear growth on the output k grid
   * - ``kfunctions_nw.txt``
     - ``kfunctions_nw``
     - The corresponding no-wiggle kernel table
   * - ``rsd_multipoles.txt``
     - ``multipoles``
     - ``k``, ``P0``, ``P2``, and ``P4``
   * - ``cb_power.txt``
     - ``cb_power``
     - Linear and one-loop cold dark matter plus baryon spectra for :doc:`fkpt2-folps-nu`
   * - ``clpt.txt``
     - ``clpt``
     - Real-space correlation and bias terms, including ``xi_matter`` and ``xi_tracer``
   * - ``qfunctions.txt``
     - ``qfunctions``
     - Lagrangian correlators used by CLPT
   * - ``clpt_velocity.txt``
     - ``clpt_velocity``
     - ``r``, ``xi_tracer``, ``v12``, ``sigma_parallel2``, and ``sigma_perp2``
   * - ``gsm.txt``
     - ``gsm``
     - ``s``, ``xi0``, ``xi2``, and ``xi4``

Wavenumbers are in h/Mpc. Power spectrum multipoles are in (Mpc/h)³.
Separations are in Mpc/h, and the correlations are dimensionless.
The GSM mean velocity is in Mpc/h after dividing peculiar velocity by
:math:`aH(a)`, and the central variances are in (Mpc/h)². See
:doc:`fkpt2-clpt` and :doc:`fkpt2-gsm` for the relevant grids and options.

Run records and filenames
-------------------------

Set ``suffixModel=_f5`` to append ``_f5`` before the extension, for example
``rsd_multipoles_f5.txt``. Use separate output directories or suffixes
to retain multiple runs.

The ``tmp/`` subdirectory of ``rootDir`` contains the effective settings.
With ``run.ini`` and suffix ``_f5``, the settings file is
``tmp/run.ini_f5-usedvalues``. When a run has no parameter file, the base
name is ``parameters_null-fkpt2``. Set ``verbose_log=1`` to write a
native run log in the same subdirectory.
