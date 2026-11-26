Build and publish the documentation
===================================

Local HTML build
----------------

Use Python 3.11 or newer and install the pinned documentation dependencies
in an environment for the docs.

.. code-block:: bash

   python3 -m pip install -r docs/requirements.txt
   python3 -m sphinx -E -a -n -W --keep-going -b html docs docs/_build/html

With the requirements installed, ``make -C docs html`` also builds the site.
Open ``docs/_build/html/index.html`` to preview it. The build reads the
reStructuredText pages, the existing Markdown guides, the package version,
and the canonical parameter definitions from the source tree. It needs
only the documentation dependencies.

Read the Docs
-------------

The repository-root ``.readthedocs.yaml`` selects the Python version,
documentation requirements, and ``docs/conf.py``. Read the Docs runs the
HTML builder and treats warnings as build errors.

#. Put the fkPT2 source and the root configuration file in the repository
   that will back the documentation project.
#. In Read the Docs, choose **Add project** and select that repository.
#. Enable the branch or tag to publish and check the first build log.
   Open the published **latest** version to verify navigation and search.

Read the Docs builds the site from the source files in the selected branch.
See the official
`project setup guide <https://docs.readthedocs.com/platform/stable/intro/add-project.html>`_
and
`configuration file reference <https://docs.readthedocs.com/platform/stable/config-file/v2.html>`_.
