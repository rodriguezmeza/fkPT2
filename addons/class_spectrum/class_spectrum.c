#include "class_backend.h"
#include "fkpt2_addon.h"
#include "fkpt2_internal.h"
#include <strings.h>

static int same_value(double supplied, double derived, double relative)
{
    return fabs(supplied - derived) <= fmax(1e-10, relative * fmax(fabs(supplied), fabs(derived)));
}

static int match_cosmology(struct cmdline_data *cmd, const struct class_spectrum *spectrum)
{
    const struct {
        unsigned int bit;
        const char *name;
        double supplied, derived;
    } parameters[] = {
        {FKPT2_INPUT_H, "h", cmd->h, spectrum->h},
        {FKPT2_INPUT_OM, "Om", cmd->om, spectrum->omega_m},
        {FKPT2_INPUT_Z, "zout", cmd->zout, spectrum->redshift},
    };
    for (size_t i = 0; i < sizeof(parameters) / sizeof(*parameters); ++i)
        if ((cmd->input_overrides & parameters[i].bit) &&
            !same_value(parameters[i].supplied, parameters[i].derived, 1e-7)) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "fkPT2 %s=%.12g conflicts with the CLASS value %.12g",
                     parameters[i].name, parameters[i].supplied, parameters[i].derived);
            return FAILURE;
        }

    cmd->h = spectrum->h;
    cmd->om = spectrum->omega_m;
    cmd->zout = spectrum->redshift;
    return SUCCESS;
}

static int replace_model_setting(char **setting, const char *value, struct cmdline_data *cmd)
{
    char *copy = strdup(value);
    if (copy == NULL) {
        snprintf(cmd->error_message, sizeof(cmd->error_message), "Out of memory setting CLASS model parameters");
        return FAILURE;
    }
    free(*setting);
    *setting = copy;
    return SUCCESS;
}

static int set_class_model(struct cmdline_data *cmd, const struct class_spectrum *spectrum)
{
    if (spectrum->omega_nu > 0.0 && !(cmd->input_overrides & FKPT2_INPUT_MODEL) &&
        replace_model_setting(&cmd->mgmodel, "FOLPSNu", cmd) == FAILURE)
        return FAILURE;

    if (strcasecmp(cmd->mgmodel, "FOLPSNu") != 0) {
        if (spectrum->omega_nu == 0.0)
            return SUCCESS;
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "Select model=FOLPSNu for a CLASS spectrum with massive neutrinos");
        return FAILURE;
    }

    const double mass = 93.14 * spectrum->omega_nu * (spectrum->h * spectrum->h);
    if (cmd->modelOptions[strspn(cmd->modelOptions, " \t")] != '\0') {
        double supplied;
        int length = 0;
        if (sscanf(cmd->modelOptions, " mnu = %lf %n", &supplied, &length) != 1 ||
            cmd->modelOptions[length] != '\0' || !isfinite(supplied) ||
            !same_value(supplied, mass, 1e-6)) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "CLASS fixes the FOLPSNu mass parameter at mnu=%.12g eV", mass);
            return FAILURE;
        }
    }

    if ((cmd->input_overrides & FKPT2_INPUT_RESCALE) && cmd->is_PS_input_LCDM != 0) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "FOLPSNu uses CLASS's linear cb spectrum with is_PS_input_LCDM=0");
        return FAILURE;
    }
    cmd->is_PS_input_LCDM = 0;

    char option[40];
    snprintf(option, sizeof(option), "mnu=%.17g", mass);
    return replace_model_setting(&cmd->modelOptions, option, cmd);
}

static int class_linear_spectrum(struct cmdline_data *cmd, const double **k, const double **power,
                                 size_t *count)
{
    char *path = cmd->paramfile;
    if (cmd->class_parameters == NULL) {
        path = realpath(cmd->classIni, NULL);
        if (path == NULL) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "Cannot locate CLASS input '%.500s': %s", cmd->classIni, strerror(errno));
            return FAILURE;
        }
        free(cmd->classIni);
        cmd->classIni = path;
    }

    struct class_spectrum spectrum;
    if (fkpt2_class_backend(path, cmd->class_parameters, &spectrum,
                            cmd->error_message, sizeof(cmd->error_message)) != 0)
        return FAILURE;

    const double *data = fkpt2_work_own(spectrum.data);
    if (match_cosmology(cmd, &spectrum) == FAILURE || set_class_model(cmd, &spectrum) == FAILURE)
        return FAILURE;
    cmd->fnamePS[0] = '\0';
    *count = spectrum.count;
    *k = data;
    *power = data + *count;
    return SUCCESS;
}

const struct fkpt2_addon fkpt2_class_addon = {
    .name = "class",
    .linear_spectrum = class_linear_spectrum,
};
