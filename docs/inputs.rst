Input power spectra
===================

Supply a whitespace-delimited text table with two columns. The first is
the wavenumber :math:`k` in h/Mpc. The second is the linear matter power
:math:`P(k)` in (Mpc/h)³ at ``zout``. Each column must contain at least ten
finite, positive samples, and the wavenumbers must increase strictly.

Use the bundled
:download:`ΛCDM spectrum at z=0.5 <../tests/fkpt/Input/pkl_z05.dat>` to try the
:doc:`quickstart`. A spectrum for a different cosmology needs the
corresponding ``Om``, ``h``, and ``zout``.

Paths and model normalization
-----------------------------

``fnamePS=./spectra/matter.dat`` supplies an explicit path. For a bare name
such as ``fnamePS=matter.dat``, the solver checks ``Input/matter.dat`` and then
the current directory. Native builds can fall back to the bundled
``pkl_z05.dat`` for that default name.

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Setting
     - Input expected
   * - ``is_PS_input_LCDM=1``
     - An LCDM spectrum at ``zout``. The solver applies the selected gravity
       model's linear growth rescaling.
   * - ``is_PS_input_LCDM=0``
     - A spectrum already computed for the selected model at ``zout``.
       The solver uses its amplitude directly.

In Python, supply the same two columns as one-dimensional arrays of equal
length, or pass ``fnamePS``. See :doc:`fkpt2-python`. To generate the linear
spectrum and its matching cosmology directly from CLASS, see
:doc:`class_workflow`. A CLASS-enabled executable accepts ``classIni``
as its in-memory spectrum source.

The solver fits the spectrum endpoints to extend the integration range and
constructs a no-wiggle spectrum for infrared resummation. The output
``linear.txt`` or ``result.linear`` records the prepared spectrum and
growth factors. For a numerical convergence check, vary the spectrum
sampling and the integration settings relevant to the observable.
