/* Test and documentation example. Constant mu exercises model dispatch. */
#include "fkpt2_addon.h"

enum { STRENGTH };
static const struct fkpt2_model_option settings[] = {{"strength", 1.0}};

static int initialize(struct cmdline_data *cmd, struct global_data *gd)
{
    if (gd->model_options[STRENGTH] > 0.0)
        return SUCCESS;
    snprintf(cmd->error_message, sizeof(cmd->error_message), "strength must be positive");
    return FAILURE;
}

static real constant_mu(struct cmdline_data *cmd, struct global_data *gd, real eta, real k)
{
    return gd->model_options[STRENGTH];
}

static int monopole_at_kmin(const fkpt2_result *result, double *value, char *error)
{
    const double *multipoles = fkpt2_result_table(result, FKPT2_MULTIPOLES, NULL, NULL);
    *value = multipoles[1];
    return FKPT2_OK;
}

static const struct fkpt2_gravity_model example_model = {
    .name = "Example",
    .description = "Constant-mu example for addon tests",
    .hooks = {.mu = constant_mu},
    .options = settings,
    .option_count = sizeof(settings) / sizeof(*settings),
    .initialize = initialize,
};

const struct fkpt2_addon fkpt2_example_addon = {
    .name = "monopole_at_kmin",
    .model = &example_model,
    .analyze = monopole_at_kmin,
};
