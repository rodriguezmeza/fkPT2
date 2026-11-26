#ifndef FKPT2_ADDON_H
#define FKPT2_ADDON_H

#include "globaldefs.h"
#include "fkpt2.h"

struct fkpt2_model_option {
    const char *name;
    real default_value;
};

struct fkpt2_gravity_model {
    const char *name;
    const char *description;
    /* Unspecified hooks use the built-in LCDM implementation. */
    struct fkpt2_gravity_hooks hooks;
    /* Values in gd->model_options follow the order of this array. */
    const struct fkpt2_model_option *options;
    size_t option_count;
    /* Optional initialization. Set cmd->error_message and return FAILURE on error. */
    int (*initialize)(struct cmdline_data *cmd, struct global_data *gd);
    /* Opt in if growth hooks can run concurrently with private copies of gd.
     * cmd and all pointed-to run and model state must be treated as read-only. */
    bool parallel_growth;
    /* Optional fit for f(k, eta) / f_LCDM(eta) in a flat LCDM background. */
    real (*growth_rate_ratio)(const struct cmdline_data *, const struct global_data *,
                              real eta, real k);
    /* Keep the scale-dependent rates but use EdS coefficients in the loop kernels. */
    bool eds_loop_kernels;
    /* Integrate the velocity variance separately for the no-wiggle spectrum. */
    bool separate_velocity_dispersion;
};

struct fkpt2_table_producer {
    int (*enabled)(const struct cmdline_data *);
    int (*validate)(struct cmdline_data *);
    int (*compute)(struct cmdline_data *, struct global_data *, fkpt2_result *);
};

struct fkpt2_addon {
    const char *name;
    const struct fkpt2_gravity_model *model;
    /* Optional scalar diagnostic, keyed by the addon name. */
    int (*analyze)(const fkpt2_result *result, double *value, char *error);
    const struct fkpt2_table_producer *tables;
    /* Supply run-owned arrays in h units and synchronize the input cosmology. */
    int (*linear_spectrum)(struct cmdline_data *, const double **k, const double **power,
                           size_t *count);
};

/* Add an owned row-major table. Name and column labels must have static storage. */
double *fkpt2_alloc_addon_table(fkpt2_result *result, const char *name, size_t rows,
                               size_t columns, const char *const column_names[]);

#endif
