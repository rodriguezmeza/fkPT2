#ifndef FKPT2_CLPT_H
#define FKPT2_CLPT_H

#include "fkpt2_addon.h"

enum clpt_velocity_column {
    CLPT_R, CLPT_XI, CLPT_V12, CLPT_SIGMA_PAR, CLPT_SIGMA_PERP,
    CLPT_VELOCITY_COLUMNS
};

struct clpt_velocity_grid {
    double min, max;
    int size;
    double *data;
};

int clpt_validate(struct cmdline_data *cmd);
int clpt_compute(struct cmdline_data *cmd, struct global_data *gd,
                 fkpt2_result *result, struct clpt_velocity_grid *grid);

#endif
