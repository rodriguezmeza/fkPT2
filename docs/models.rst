Gravity models
==============

Use ``model=LCDM``, ``model=HS``, ``model=DGP``, or ``model=FOLPSNu``.
Check the models compiled
into your build with ``./fkpt2 options=print-models`` or
``fkpt2.available_models()`` in Python.

ΛCDM
----

The default ``model=LCDM`` uses a flat ΛCDM background with scale-independent
linear growth. Its cosmological inputs are ``Om``, ``h``, and ``zout``.

Hu–Sawicki
----------

``model=HS`` selects the n=1 Hu–Sawicki model. ``fR0`` specifies the magnitude
of the present-day scalar field parameter. F5 and F6 are the names commonly
used for ``fR0=1e-5`` and ``fR0=1e-6``.

.. code-block:: bash

   ./fkpt2 model=HS fR0=1e-5 rootDir=Output_F5
   ./fkpt2 model=HS fR0=1e-6 rootDir=Output_F6

DGP
---

The DGP addon uses the flat ΛCDM background and the DGP growth and screening
sources from MGPT. ``eps_DGP=-1`` selects the normal branch and
``eps_DGP=1`` the self-accelerating branch. ``rc_DGP`` is the crossover scale
in units of :math:`H_0^{-1}`. ``screening`` multiplies the nonlinear screening
sources.

.. code-block:: bash

   ./fkpt2 model=DGP eps_DGP=-1 rc_DGP=1 rootDir=Output_DGP

Massive neutrinos
-----------------

``model=FOLPSNu`` selects the scale-dependent cb growth and one-loop power
spectra for flat ΛCDM with massive neutrinos. See :doc:`fkpt2-folps-nu`
for the cosmology parameters, input spectrum, and example.

Use the same names as keywords in Python. To use an external model-specific
linear spectrum, see :doc:`inputs`. To implement another model, see
:doc:`fkpt2-addons`.
