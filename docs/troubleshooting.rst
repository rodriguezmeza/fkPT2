Troubleshooting
===============

The solver cannot read the spectrum
-----------------------------------

Check the working directory and the path in ``fnamePS``. Quote a shell
argument that contains spaces, for example
``'fnamePS=./spectra/linear matter.dat'``. The input requires at least ten
rows with finite, positive power and strictly increasing, positive
wavenumbers. See :doc:`inputs`.

The model or addon is unavailable
---------------------------------

Check ``./fkpt2 options=print-models`` or
``./fkpt2 options=make-info``. Rebuild with the relevant switch in
:doc:`installation`. GSM requires a build with both ``CLPTON=1`` and
``GSMON=1``. In Python, inspect ``fkpt2.available_models()`` and
``fkpt2.available_addons()`` in the environment used for the calculation.

Python cannot import fkpt2
--------------------------

Install with ``python3 -m pip install .`` in the environment used to run
the calculation. After an in-place build, include the checkout's
``python/`` directory on ``PYTHONPATH``. Use the same interpreter for
building and importing the extension. Restart a notebook kernel after
replacing its extension.

CLPT reports a Fourier range error
----------------------------------

The requested ``clpt_kmin`` and ``clpt_kmax`` must lie within the prepared
spectrum. Check the ``k`` range in ``linear.txt`` from a standard run and set
the requested CLPT Fourier limits accordingly. See :doc:`fkpt2-clpt`.

GSM reports a nonpositive density or variance
---------------------------------------------

The error identifies a real-space radius sampled by the streaming integral.
Review the requested separation window and the tracer bias and EFT settings.
The one-loop model needs a positive pair density and positive dispersions
over the integration domain. The :doc:`fkpt2-gsm` guide describes the
isotropic counterterm and how the real-space domain is chosen.
