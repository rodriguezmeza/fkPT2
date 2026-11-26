Overview
========

fkPT2 starts from a linear matter power spectrum at the requested output
redshift. It prepares the linear and no-wiggle spectra, solves for the
selected model's growth, and computes one-loop kernels. The Fourier-space
prediction includes bias, effective field theory (EFT), and shot-noise terms
with infrared resummation of the baryon acoustic oscillation signal. The
solver projects the anisotropic power onto Legendre multipoles.

.. math::

   P_\ell(k) = \frac{2\ell+1}{2}\int_{-1}^{1}d\mu\,
                P(k,\mu)\,\mathcal{L}_\ell(\mu),
   \qquad \ell=0,2,4.

The cosine :math:`\mu` is measured between the wavevector and the line of
sight. See :doc:`references` for the fkPT treatment of scale-dependent growth.

Choose an observable
--------------------

.. list-table::
   :header-rows: 1
   :widths: 37 22 41

   * - Observable
     - Run setting
     - Result
   * - Redshift-space power spectrum
     - Standard run
     - ``P0``, ``P2``, ``P4`` as functions of ``k``
   * - Real-space correlation
     - ``clpt=true``
     - Matter and biased-tracer correlations as functions of ``r``
   * - Redshift-space correlation
     - ``gsm=true``
     - ``xi0``, ``xi2``, ``xi4`` as functions of ``s``

The CLPT addon convolves the displacement and bias terms with the linear
displacement kernel. GSM uses the CLPT tracer correlation, mean pairwise
velocity, and parallel and transverse dispersions to predict the
redshift-space correlation. A GSM run includes the CLPT tables.

The Fourier-space prediction and the CLPT and GSM calculations have separate
bias parameter sets. The Fourier-space prediction uses ``b1``, ``b2``,
``bs2``, and ``b3nl``. The configuration-space addons use the Lagrangian
coefficients ``clpt_b1``, ``clpt_b2``, ``clpt_bs2``, and ``clpt_b01``.

Choose an interface
-------------------

The :doc:`command_line` interface writes text tables and a resolved parameter
file. The :doc:`fkpt2-python` interface uses the same solver and returns
independent NumPy arrays. C and C++ applications can link the static library
through the :doc:`api`. The :doc:`models` page describes each built-in gravity
choice.
