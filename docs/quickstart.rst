Quickstart
==========

The source tree includes an example linear spectrum for ΛCDM at ``zout=0.5``.
Use it to check the CLI and Python workflows before substituting your own
spectrum. See :doc:`installation` to build each interface.

Run from the command line
-------------------------

.. code-block:: bash

   ./fkpt2 fnamePS=tests/fkpt/Input/pkl_z05.dat Nk=12 nquadSteps=64 \
      rootDir=Output_quick

Inspect the multipoles with ``head Output_quick/rsd_multipoles.txt``. The
first numeric column gives ``k`` in h/Mpc and the next three give the power
multipoles in (Mpc/h)³. The small grids make this a smoke test. For a
measurement or fit, check convergence with the quadrature and grid controls
in :doc:`params`.

Run the same calculation in Python
----------------------------------

.. code-block:: python

   import fkpt2

   result = fkpt2.compute(
       fnamePS="tests/fkpt/Input/pkl_z05.dat",
       Nk=12,
       nquadSteps=64,
   )
   k, p0, p2, p4 = result.multipoles.T
   print(result.column_names("multipoles"))

Use :doc:`fkpt2-python` to supply arrays, reuse a solver with alternate
gravity settings, and inspect the available tables.

Compute correlation functions
-----------------------------

Enable Gaussian streaming to compute the redshift-space correlation
multipoles and their CLPT inputs.

.. code-block:: bash

   ./fkpt2 fnamePS=tests/fkpt/Input/pkl_z05.dat gsm=true \
      smin=25 smax=130 Ns=25 clpt_b1=0.65 clpt_bs2=-0.2 \
      rootDir=Output_gsm

The run writes ``gsm.txt``, ``clpt_velocity.txt``, ``clpt.txt``, and
``qfunctions.txt`` alongside the Fourier-space tables. Use ``clpt=true``
for standalone real-space CLPT. See :doc:`fkpt2-clpt` and :doc:`fkpt2-gsm`
for the bias and EFT settings, integration controls, and units.
