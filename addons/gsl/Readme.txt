Optional GSL support

To use an installed GNU Scientific Library, build from the fkPT2 root
with USEGSL=1 GSLINTERNAL=0. Set GSL_CONFIG to the installation's gsl-config
if it is outside PATH.

    make all USEGSL=1 GSLINTERNAL=0 GSL_CONFIG=/path/to/gsl-config

The bundled GSL source has its own README and INSTALL instructions.
See ../../docs/installation.rst for fkPT2 build settings.
