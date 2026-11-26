#include "fkpt2_addon.h"
#include "fkpt2_internal.h"
#include <ctype.h>
#include <errno.h>

extern const struct fkpt2_gravity_model fkpt2_lcdm_model;

#define FKPT2_ADDON(symbol) extern const struct fkpt2_addon symbol;
#include "fkpt2_addons.inc"
#undef FKPT2_ADDON

static const struct fkpt2_addon *const addons[] = {
#define FKPT2_ADDON(symbol) &symbol,
#include "fkpt2_addons.inc"
#undef FKPT2_ADDON
    NULL
};

const struct fkpt2_addon *fkpt2_addon_at(size_t index)
{
    return index < sizeof(addons) / sizeof(*addons) ? addons[index] : NULL;
}

const char *fkpt2_addon_name(size_t index)
{
    const struct fkpt2_addon *addon = fkpt2_addon_at(index);
    return addon == NULL ? NULL : addon->name;
}

int fkpt2_addon_spectrum(const char *name, struct cmdline_data *cmd,
                         const double **k, const double **power, size_t *count)
{
    for (size_t i = 0; addons[i] != NULL; ++i)
        if (addons[i]->linear_spectrum != NULL && strcmp(addons[i]->name, name) == 0)
            return addons[i]->linear_spectrum(cmd, k, power, count);
    snprintf(cmd->error_message, sizeof(cmd->error_message),
             "The CLASS addon is disabled in this build (enable CLASSLIBON=1)");
    return FAILURE;
}

int fkpt2_has_addon_results(const struct cmdline_data *cmd)
{
    for (size_t i = 0; addons[i] != NULL; ++i) {
        const struct fkpt2_addon *addon = addons[i];
        if (addon->analyze != NULL || (addon->tables != NULL && addon->tables->enabled(cmd)))
            return TRUE;
    }
    return FALSE;
}

int fkpt2_validate_addons(struct cmdline_data *cmd)
{
    int has_clpt = FALSE, has_gsm = FALSE;
    for (size_t i = 0; addons[i] != NULL; ++i) {
        const struct fkpt2_addon *addon = addons[i];
        if (addon->tables == NULL)
            continue;
        has_clpt |= strcmp(addon->name, "clpt") == 0;
        has_gsm |= strcmp(addon->name, "gsm") == 0;
        if (addon->tables->enabled(cmd) && addon->tables->validate != NULL &&
            addon->tables->validate(cmd) == FAILURE)
            return FAILURE;
    }
    if (cmd->gsm && !has_gsm) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "The GSM addon is disabled in this build (GSM requires CLPT)");
        return FAILURE;
    }
    if (cmd->clpt && !has_clpt) {
        snprintf(cmd->error_message, sizeof(cmd->error_message), "The CLPT addon is disabled in this build");
        return FAILURE;
    }
    return SUCCESS;
}

static const struct fkpt2_gravity_model *model_at(size_t index)
{
    if (index == 0)
        return &fkpt2_lcdm_model;

    --index;
    for (size_t i = 0; addons[i] != NULL; ++i)
        if (addons[i]->model != NULL && index-- == 0)
            return addons[i]->model;
    return NULL;
}

const char *fkpt2_model_name(size_t index)
{
    const struct fkpt2_gravity_model *model = model_at(index);
    return model == NULL ? NULL : model->name;
}

const char *fkpt2_model_description(size_t index)
{
    const struct fkpt2_gravity_model *model = model_at(index);
    return model == NULL ? NULL : model->description;
}

static int same_model_name(const char *left, const char *right)
{
    while (*left && tolower((unsigned char)*left) == tolower((unsigned char)*right)) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static const char *skip_space(const char *text)
{
    while (isspace((unsigned char)*text))
        ++text;
    return text;
}

static int configure_model_options(struct cmdline_data *cmd, struct global_data *gd)
{
    const struct fkpt2_gravity_model *model = gd->model;
    const size_t n = model->option_count;
    if (n != 0) {
        gd->model_options = fkpt2_work_calloc(n, sizeof(*gd->model_options));
        for (size_t i = 0; i < n; ++i)
            gd->model_options[i] = model->options[i].default_value;
    }

    const char *input = skip_space(cmd->modelOptions);
    if (*input == '\0')
        return SUCCESS;

    unsigned char *seen = n != 0 ? fkpt2_work_calloc(n, sizeof(*seen)) : NULL;
    do {
        const char *name = input;
        const char *end_name = name;
        while (*end_name && *end_name != '=' && *end_name != ',' &&
               !isspace((unsigned char)*end_name))
            ++end_name;

        input = skip_space(end_name);
        if (end_name == name || *input != '=')
            break;
        size_t len = (size_t)(end_name - name);
        size_t index = 0;
        for (; index < n; ++index) {
            const char *option = model->options[index].name;
            if (strlen(option) == len && strncmp(option, name, len) == 0)
                break;
        }
        if (index == n || seen[index]) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "%s model option '%.*s' for %s",
                     index == n ? "Unknown" : "Duplicate", (int)len, name, model->name);
            return FAILURE;
        }

        const char *value = skip_space(input + 1);
        char *end_value;
        errno = 0;
        real number = strtod(value, &end_value);
        if (end_value == value || errno == ERANGE || !isfinite(number))
            break;
        gd->model_options[index] = number;
        seen[index] = 1;
        input = skip_space(end_value);
        if (*input == '\0') {
            fkpt2_work_free(seen);
            return SUCCESS;
        }
        if (*input != ',')
            break;
        input = skip_space(input + 1);
    } while (*input);

    snprintf(cmd->error_message, sizeof(cmd->error_message),
             "Invalid modelOptions for %s; use comma-separated name=value pairs", model->name);
    return FAILURE;
}

int set_model(struct cmdline_data *cmd, struct global_data *gd)
{
    const struct fkpt2_gravity_model *model;
    for (size_t i = 0; (model = model_at(i)) != NULL; ++i)
        if (same_model_name(cmd->mgmodel, model->name))
            break;
    if (model == NULL) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "Unknown or disabled gravity model '%s' (see options=print-models)", cmd->mgmodel);
        return FAILURE;
    }

    gd->model = model;
    snprintf(gd->model_comment, sizeof(gd->model_comment), "%s Model", model->name);
    if (configure_model_options(cmd, gd) == FAILURE)
        return FAILURE;

    gd->gravity = model->hooks;
#define DEFAULT_HOOK(name) \
    if (gd->gravity.name == NULL) gd->gravity.name = fkpt2_lcdm_model.hooks.name
    DEFAULT_HOOK(OmM);
    DEFAULT_HOOK(H);
    DEFAULT_HOOK(f1);
    DEFAULT_HOOK(f2);
    DEFAULT_HOOK(A0);
    DEFAULT_HOOK(mu);
    DEFAULT_HOOK(sourceA);
    DEFAULT_HOOK(sourceb);
    DEFAULT_HOOK(SD2);
    DEFAULT_HOOK(S3I);
    DEFAULT_HOOK(S3II);
    DEFAULT_HOOK(S3FL);
    DEFAULT_HOOK(S3dI);
#undef DEFAULT_HOOK

    if (model->initialize != NULL)
        return model->initialize(cmd, gd);
    return SUCCESS;
}
