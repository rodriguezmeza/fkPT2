Massive-neutrino power spectra
==============================

Select ``model=FOLPSNu`` to compute the one-loop cold dark matter plus
baryon (cb) power spectra and the infrared-resummed redshift-space galaxy
multipoles in a flat ΛCDM cosmology with massive neutrinos. This model uses
the Hu–Eisenstein growth fit implemented in FOLPS-nu and fkPT2's native
radial and angular loop quadrature.

Input spectrum and cosmology
-----------------------------

Supply a two-column linear **cb** power spectrum at ``zout``. Use
:math:`k` in :math:`h/\mathrm{Mpc}` and :math:`P_{\rm cb}^{\rm lin}`
in :math:`(\mathrm{Mpc}/h)^3`. Set ``is_PS_input_LCDM=0`` to use that
spectrum directly. Set ``Om`` to the **total** matter fraction today,
including the massive neutrinos. Set the sum of neutrino masses in eV
using ``modelOptions=mnu=...``.

.. math::

   \Omega_m h^2 = \Omega_b h^2 + \Omega_c h^2 + \Omega_\nu h^2 ,
   \qquad
   \Omega_\nu h^2 = \frac{\sum_i m_{\nu,i}}{93.14\ \mathrm{eV}}.

The fit uses three mass-degenerate neutrinos with :math:`N_{\rm eff}=3.046`.
The default ``mnu=0`` gives scale-independent linear growth. Supply a
spectrum generated with the same mass, cosmology, and redshift used for the
fkPT2 run.

Run the supplied FOLPS-nu example at :math:`z=0.5` with
:math:`\sum_i m_{\nu,i}=0.4\ \mathrm{eV}`.

.. code-block:: bash

   make all
   ./fkpt2 model=FOLPSNu modelOptions=mnu=0.4 is_PS_input_LCDM=0 \
      fnamePS=tests/fixtures/folps_nu/pkl_cb_m04_z05.dat \
      zout=0.5 Om=0.3174997353873 h=0.6711 rootDir=Output_FOLPSNu

For Python, install or build the Cython module as described in
:doc:`installation`.

.. code-block:: python

   import numpy as np
   import fkpt2

   k, pk_cb = np.loadtxt("tests/fixtures/folps_nu/pkl_cb_m04_z05.dat", unpack=True)
   result = fkpt2.compute(
       k, pk_cb, model="FOLPSNu", modelOptions="mnu=0.4",
       zout=0.5, Om=0.3174997353873, h=0.6711, is_PS_input_LCDM=0,
   )
   print(result.column_names("cb_power"))
   print(result.cb_power)
   print(result.multipoles)

Use ``fnamePS=...`` to pass a file directly in Python. The Fourier-space
bias, EFT, and stochastic settings are shared with the other models.
OpenMP builds accept ``numberThreads`` in both interfaces.

Output
------

The CLI writes ``cb_power.txt`` and the standard ``rsd_multipoles.txt``.
The corresponding Python arrays are ``result.cb_power`` and
``result.multipoles``.

.. list-table::
   :header-rows: 1
   :widths: 32 68

   * - Column in ``cb_power``
     - Contents
   * - ``k``
     - Wavenumber in :math:`h/\mathrm{Mpc}`
   * - ``P_cb_linear``
     - Linear cb density power
   * - ``f_cb_over_f0``
     - Scale-dependent growth rate divided by its large-scale normalization
   * - ``P_cb_1loop``
     - Real-space cb density power, linear plus one loop
   * - ``P_cb_theta_1loop``
     - Real-space cb density–velocity divergence cross power, linear plus one loop
   * - ``P_theta_theta_1loop``
     - Real-space cb velocity divergence power, linear plus one loop

All power columns are in :math:`(\mathrm{Mpc}/h)^3`. The velocity field is
normalized as :math:`\theta_{\rm cb} = -\nabla\cdot\mathbf v_{\rm cb}/(aH f_0)`.
The reported ``sigma8`` is derived from the supplied cb spectrum. The
multipoles use the true cosmology's wavenumbers and include galaxy bias, EFT
counterterms, and shot noise.

Method and validation
---------------------

The solver obtains :math:`f_0` from scale-independent flat ΛCDM growth
with the same total matter fraction. The Hu–Eisenstein fit gives
:math:`f_{\rm cb}(k)/f_0`. The one-loop calculation retains these
scale-dependent growth rates in the velocity kernels while taking the
higher-order growth coefficients at their Einstein–de Sitter values,
as in the FOLPS-nu FFTLog expressions. The no-wiggle and full spectra
each contribute their own velocity dispersion to the redshift-space model.

``FOLPSNu`` is a Fourier-space model of the cb fluid. Calculating CLPT or GSM
for this cosmology would also require nonlinear displacement and velocity
kernels. The configuration-space addons are therefore unavailable with
``FOLPSNu``.

Run ``make test-folps-nu`` to compare the CLI with recorded FOLPS-nu output,
or pass ``FOLPS_REFERENCE=/path/to/FOLPS-nu`` with SciPy installed to generate
the reference output at test time. The input spectra are the two notebook
tables from the supplied FOLPS-nu snapshot (commit ``7a4cd56``). The
comparison uses :math:`\sum_i m_{\nu,i}=0` and :math:`0.4\ \mathrm{eV}` at
:math:`z=0.5` through :math:`k=0.25\,h/\mathrm{Mpc}`. The reference uses
256 FFTLog samples, and the fixture includes nonzero bias, EFT, and stochastic
parameters. The tests in ``make test-cython`` also compare the CLI and Python
outputs. See :doc:`references` for the scientific citation.

Build with ``MODELFOLPSNUON=0`` to omit the addon.
