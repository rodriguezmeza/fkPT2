API reference
=============

Python
------

.. py:module:: fkpt2

.. py:function:: compute(k=None, pk=None, *, parameters=None, **overrides)

   Run the solver and return a :class:`Result`. Supply ``k`` and ``pk``
   together as one-dimensional arrays, or supply ``fnamePS`` or
   ``paramfile`` for file input. The ``parameters`` mapping and overrides use
   the :doc:`params` names. Explicit keyword arguments override keys in
   the mapping. Effective options from either source override the parameter file.

.. py:class:: FKPT2(parameters=None, **overrides)

   Store default settings for repeated computations.

   .. py:method:: set(parameters=None, **overrides)

      Update the stored settings. Keyword arguments take precedence over
      the mapping.

   .. py:method:: compute(k=None, pk=None, **overrides)

      Compute with the stored settings and the given per-call overrides.
      The overrides apply to this call.

.. py:class:: Result

   A completed run, including NumPy arrays, the cosmology summary, and any
   enabled addon results. Obtain a result from :func:`compute` or
   :meth:`FKPT2.compute`. See :doc:`fkpt2-python` for all attributes.

   .. py:method:: column_names(table)

      Return the tuple of column labels for a standard or available addon table.

.. py:function:: available_models()

   Return a dictionary mapping compiled model names to descriptions.

.. py:function:: available_addons()

   Return a tuple of the compiled addon names.

.. py:function:: column_names(table)

   Return the tuple of labels for a standard table. The keys are ``linear``,
   ``kfunctions``, ``kfunctions_nw``, and ``multipoles``. Use the result method
   for addon table keys.

.. py:exception:: ComputationError

   A native solver or computation addon failure. Input validation failures
   raise ``ValueError``.

Native library
--------------

Include :download:`fkpt2.h <../include/fkpt2.h>` and link ``libfkpt2.a``.
For the default build, save the example as ``example.c`` and compile it
from the source directory with
``cc -Iinclude example.c ./libfkpt2.a -lm -pthread -o example``.
If you enabled optional native libraries, link those libraries as well.

.. code-block:: c

   #include "fkpt2.h"
   #include <stdio.h>

   int main(void)
   {
       const fkpt2_option options[] = {
           {"fnamePS", "tests/fkpt/Input/pkl_z05.dat"},
           {"Nk", "12"},
           {"nquadSteps", "64"},
       };
       fkpt2_result *result = NULL;
       char error[FKPT2_ERROR_SIZE];
       int status = fkpt2_compute(options, sizeof(options) / sizeof(options[0]),
                                  NULL, NULL, 0, &result, error);
       if (status != FKPT2_OK) {
           fprintf(stderr, "%s\n", error);
           return 1;
       }

       size_t rows, columns;
       const double *data = fkpt2_result_table(result, FKPT2_MULTIPOLES, &rows, &columns);
       printf("%zu rows, %zu columns, first P0 = %g\n", rows, columns, data[1]);
       fkpt2_result_free(result);
       return 0;
   }

For array input pass contiguous ``double`` arrays and their length in place
of ``NULL, NULL, 0``. Input arrays are borrowed during the call. Result tables
are row-major and remain valid until ``fkpt2_result_free()``. Iterate
``fkpt2_result_addon_table()`` with indices starting at zero until it returns
``NULL``. Each table carries its name, shape, data, and column labels.
``fkpt2_result_summary()`` returns the run summary. Native solves share a
workspace and are serialized.
