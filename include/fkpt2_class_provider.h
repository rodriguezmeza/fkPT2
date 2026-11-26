#ifndef FKPT2_CLASS_PROVIDER_H
#define FKPT2_CLASS_PROVIDER_H

#include "fkpt2.h"

/* Parameters whose presence matters when a spectrum source supplies a cosmology. */
enum fkpt2_input_override {
    FKPT2_INPUT_FILE = 1u << 0,
    FKPT2_INPUT_MODEL = 1u << 1,
    FKPT2_INPUT_RESCALE = 1u << 2,
    FKPT2_INPUT_OM = 1u << 3,
    FKPT2_INPUT_H = 1u << 4,
    FKPT2_INPUT_Z = 1u << 5,
};

/* Internal Cython bridge. The provider fills the arrays and updates the
 * cosmology and model settings. Returned storage is borrowed for the solve. */
typedef struct {
    const char *path;
    const char *class_parameters;
    unsigned int input_overrides;
    double h, om, zout;
    int is_ps_input_lcdm;
    const char *model;
    const char *model_options;
    const double *k;
    const double *power;
    size_t count;
} fkpt2_class_input;

typedef int (*fkpt2_class_provider)(fkpt2_class_input *input, void *context);

/* A provider failure returns -1 to propagate the existing Python exception. */
int fkpt2_compute_with_class_provider(const fkpt2_option *options, size_t option_count,
                                      const double *k, const double *power, size_t n,
                                      fkpt2_class_provider provider, void *context,
                                      fkpt2_result **result, char *error);

#endif
