#include "fkpt2_internal.h"

struct compute_run {
    struct cmdline_data cmd;
    struct global_data gd;
    const double *k;
    const double *power;
    size_t n;
    fkpt2_result *result;
};

int fkpt2_abi_version(void)
{
    return FKPT2_ABI_VERSION;
}

static int compute(void *state)
{
    struct compute_run *run = state;
    if (fkpt2_prepare(&run->cmd, &run->gd, FALSE, run->k, run->power, run->n) == FAILURE)
        return FKPT2_INVALID_INPUT;
    fkpt2_solve(&run->cmd, &run->gd);
    if (fkpt2_capture_result(&run->cmd, &run->gd, &run->result) == FAILURE)
        return FKPT2_COMPUTATION_FAILED;
    return FKPT2_OK;
}

static void copy_error(char *error, const char *message)
{
    if (error != NULL)
        snprintf(error, FKPT2_ERROR_SIZE, "%s", message);
}

static int supply_class_spectrum(struct compute_run *run, fkpt2_class_provider provider,
                                 void *context)
{
    struct cmdline_data *cmd = &run->cmd;
    if (fkpt2_validate_class_source(cmd, run->k) == FAILURE)
        return FKPT2_INVALID_INPUT;

    fkpt2_class_input input = {
        .path = cmd->paramfile,
        .class_parameters = cmd->class_parameters,
        .input_overrides = cmd->input_overrides,
        .h = cmd->h, .om = cmd->om, .zout = cmd->zout,
        .is_ps_input_lcdm = cmd->is_PS_input_LCDM,
        .model = cmd->mgmodel,
        .model_options = cmd->modelOptions,
    };
    if (provider(&input, context) != 0)
        return -1;

    char *model = strdup(input.model);
    char *model_options = strdup(input.model_options);
    if (model == NULL || model_options == NULL) {
        free(model);
        free(model_options);
        snprintf(cmd->error_message, sizeof(cmd->error_message), "Out of memory setting CLASS model parameters");
        return FKPT2_COMPUTATION_FAILED;
    }
    free(cmd->mgmodel);
    free(cmd->modelOptions);
    cmd->mgmodel = model;
    cmd->modelOptions = model_options;
    cmd->h = input.h;
    cmd->om = input.om;
    cmd->zout = input.zout;
    cmd->is_PS_input_LCDM = input.is_ps_input_lcdm;
    /* The arrays have replaced the CLASS section as the spectrum source. */
    free(cmd->class_parameters);
    cmd->class_parameters = NULL;
    cmd->fnamePS[0] = '\0';
    run->k = input.k;
    run->power = input.power;
    run->n = input.count;
    return FKPT2_OK;
}

int fkpt2_compute_with_class_provider(const fkpt2_option *options, size_t option_count,
                                      const double *k, const double *power, size_t n,
                                      fkpt2_class_provider provider, void *context,
                                      fkpt2_result **result, char *error)
{
    copy_error(error, "");
    if (result == NULL) {
        copy_error(error, "A result pointer is required");
        return FKPT2_INVALID_INPUT;
    }
    *result = NULL;
    if ((option_count != 0 && options == NULL) ||
        (n == 0 ? k != NULL || power != NULL : k == NULL || power == NULL)) {
        copy_error(error, "Supply both arrays and their nonzero length, or use fnamePS");
        return FKPT2_INVALID_INPUT;
    }

    struct compute_run run = {.k = k, .power = power, .n = n};
    int status = FKPT2_INVALID_INPUT;
    if (fkpt2_read_options(&run.cmd, options, option_count) == SUCCESS) {
        /* Generate the Python spectrum before acquiring the solver workspace. */
        status = provider != NULL && run.cmd.class_parameters != NULL
            ? supply_class_spectrum(&run, provider, context) : FKPT2_OK;
        if (status == FKPT2_OK) {
            status = fkpt2_with_workspace(compute, &run, run.cmd.error_message,
                                           sizeof(run.cmd.error_message));
            if (status < 0)
                status = FKPT2_COMPUTATION_FAILED;
        }
    }
    if (run.gd.outlog != NULL)
        fclose(run.gd.outlog);
    free_run_parameters(&run.cmd);

    if (status == FKPT2_OK)
        *result = run.result;
    else {
        fkpt2_result_free(run.result);
        copy_error(error, *run.cmd.error_message ? run.cmd.error_message : "fkPT2 computation failed");
    }
    return status;
}

int fkpt2_compute(const fkpt2_option *options, size_t option_count,
                  const double *k, const double *power, size_t n,
                  fkpt2_result **result, char *error)
{
    return fkpt2_compute_with_class_provider(options, option_count, k, power, n,
                                              NULL, NULL, result, error);
}
