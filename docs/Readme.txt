Building the fkPT2 manual

From the repository root, use Python 3.11 or newer and run

    python3 -m pip install -r docs/requirements.txt
    make -C docs html

Open docs/_build/html/index.html. The publishing guide in publishing.rst
describes the Read the Docs configuration.

For the terminal manual, run man ./docs/man/fkpt2.1 from the repository
root. After editing docs/man/fkpt2.m, regenerate its man and HTML versions
with sh docs/man/makehtml.sh. This uses mandoc.
