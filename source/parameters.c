/* Configuration shared by the fkPT2 command line and parameter files. */
#include "fkpt2_internal.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <strings.h>

enum parameter_kind { PARAM_REAL, PARAM_INT, PARAM_BOOL, PARAM_TEXT };

struct run_parameter {
    const char *name;
    const char *alias;
    const char *default_value;
    size_t offset;
    enum parameter_kind kind;
};

/* Derive the type from the actual member so it cannot disagree with storage. */
#define FIELD_KIND(member) _Generic(((struct cmdline_data *)0)->member, \
    real: PARAM_REAL, int: PARAM_INT, bool: PARAM_BOOL, string: PARAM_TEXT)
#define PARAMETER(member, name, value, help) \
    {#name, NULL, value, offsetof(struct cmdline_data, member), FIELD_KIND(member)},
#define PARAMETER_ALIAS(member, name, value, help, alias) \
    {#name, #alias, value, offsetof(struct cmdline_data, member), FIELD_KIND(member)},

static const struct run_parameter run_parameters[] = {
#include "run_parameters.def"
};
#undef PARAMETER_ALIAS
#undef PARAMETER
#undef FIELD_KIND

#define PARAMETER_COUNT (sizeof(run_parameters) / sizeof(run_parameters[0]))

static void *parameter_field(struct cmdline_data *cmd, const struct run_parameter *param)
{
    return (char *)cmd + param->offset;
}

static char *trim(char *text)
{
    while (isspace((unsigned char)*text))
        ++text;

    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
        --end;
    *end = '\0';
    return text;
}

/* Strip comments and unpack an optional quoted value in place. */
static char *parameter_value(char *text)
{
    text = trim(text);
    if (*text != '"' && *text != '\'') {
        char *comment = strpbrk(text, "#%");
        if (comment != NULL)
            *comment = '\0';
        return trim(text);
    }

    char quote = *text++;
    char *end = text;
    char *out = text;

    while (*end && *end != quote) {
        if (*end == '\\' && (end[1] == quote || end[1] == '\\'))
            ++end;
        *out++ = *end++;
    }
    if (*end != quote)
        return NULL;
    end = trim(end + 1);
    if (*end && *end != '#' && *end != '%')
        return NULL;
    *out = '\0';
    return text;
}

static size_t parameter_index(const char *name)
{
    for (size_t i = 0; i < PARAMETER_COUNT; ++i) {
        const struct run_parameter *param = &run_parameters[i];
        if (strcmp(name, param->name) == 0 ||
            (param->alias != NULL && strcmp(name, param->alias) == 0))
            return i;
    }
    return PARAMETER_COUNT;
}

static int append_class_line(char **input, size_t *used, size_t *capacity, const char *line)
{
    const size_t length = strlen(line);
    if (*used > SIZE_MAX - length - 2)
        return FAILURE;
    const size_t required = *used + length + 2;
    if (required > *capacity) {
        const size_t doubled = *capacity <= SIZE_MAX / 2 ? 2 * *capacity : *capacity;
        const size_t new_capacity = required > doubled ? required : doubled;
        char *space = realloc(*input, new_capacity);
        if (space == NULL)
            return FAILURE;
        *input = space;
        *capacity = new_capacity;
    }
    memcpy(*input + *used, line, length);
    *used += length;
    (*input)[(*used)++] = '\n';
    (*input)[*used] = '\0';
    return SUCCESS;
}

static int load_parameter_file(struct cmdline_data *cmd, char **values)
{
    FILE *file = fopen(cmd->paramfile, "r");
    if (file == NULL) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "Cannot read parameter file '%s': %s", cmd->paramfile, strerror(errno));
        return FAILURE;
    }

    char buffer[BUFFERSIZE];
    unsigned long line = 0;
    int status = SUCCESS;
    bool in_class = false;
    size_t class_length = 0, class_capacity = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        ++line;
        const char *problem = NULL;
        bool incomplete = strlen(buffer) == sizeof(buffer) - 1 &&
                          strchr(buffer, '\n') == NULL && !feof(file);
        char *name = trim(buffer);
        if (incomplete) {
            problem = "line too long";
        } else if (*name == '\0' || *name == '#' || *name == '%') {
            continue;
        } else if (*name == '[') {
            char *end = strchr(name + 1, ']');
            const char *tail = end == NULL ? "" : trim(end + 1);
            if (end == NULL || (*tail && *tail != '#' && *tail != '%')) {
                problem = "invalid section header";
            } else {
                *end = '\0';
                const char *section = trim(name + 1);
                if (strcasecmp(section, "class") == 0) {
                    in_class = true;
                    /* A declared, empty section requests the CLASS defaults. */
                    if (cmd->class_parameters == NULL &&
                        append_class_line(&cmd->class_parameters, &class_length, &class_capacity, "") == FAILURE)
                        problem = "out of memory";
                } else if (strcasecmp(section, "fkpt2") == 0) {
                    in_class = false;
                } else {
                    problem = "unknown section (expected [class] or [fkpt2])";
                }
            }
        } else if (in_class) {
            if (append_class_line(&cmd->class_parameters, &class_length, &class_capacity, name) == FAILURE)
                problem = "out of memory";
        } else {
            char *separator = name + strcspn(name, "= \t\r\n\f\v");
            char *text = separator;
            if (*separator == '=') {
                ++text;
            } else if (*separator != '\0') {
                text = trim(separator + 1);
                if (*text == '=')
                    ++text;
            }
            *separator = '\0';

            size_t i = parameter_index(name);
            char *value = parameter_value(text);
            if (i == PARAMETER_COUNT)
                problem = "unknown parameter";
            else if (value == NULL)
                problem = "invalid quoted value";
            else if (values[i] != NULL) {
                /* Older used-values files can repeat the same setting. */
                if (strcmp(values[i], value) != 0)
                    problem = "duplicate parameter";
            } else if ((values[i] = strdup(value)) == NULL) {
                problem = "out of memory";
            }
        }
        if (problem != NULL) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "%s:%lu: %s (%.80s)", cmd->paramfile, line, problem, name);
            status = FAILURE;
            break;
        }
    }
    if (status == SUCCESS && ferror(file)) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "Error reading parameter file '%s'", cmd->paramfile);
        status = FAILURE;
    }
    fclose(file);
    return status;
}

static int store_parameter(struct cmdline_data *cmd, const struct run_parameter *param,
                           const char *value)
{
    void *field = parameter_field(cmd, param);
    char *end;
    errno = 0;

    switch (param->kind) {
    case PARAM_REAL: {
        real number = strtod(value, &end);
        if (end != value && *end == '\0' && errno != ERANGE && isfinite(number)) {
            *(real *)field = number;
            return SUCCESS;
        }
        break;
    }
    case PARAM_INT: {
        long number = strtol(value, &end, 10);
        if (end != value && *end == '\0' && errno != ERANGE &&
            number >= INT_MIN && number <= INT_MAX) {
            *(int *)field = (int)number;
            return SUCCESS;
        }
        break;
    }
    case PARAM_BOOL: {
        char lower[6];
        size_t length = strlen(value);
        if (length >= sizeof(lower))
            break;
        for (size_t i = 0; i <= length; ++i)
            lower[i] = (char)tolower((unsigned char)value[i]);

        if (strcmp(lower, "true") == 0 || strcmp(lower, "yes") == 0 ||
            strcmp(lower, "t") == 0 || strcmp(lower, "y") == 0 || strcmp(lower, "1") == 0)
            *(bool *)field = TRUE;
        else if (strcmp(lower, "false") == 0 || strcmp(lower, "no") == 0 ||
                 strcmp(lower, "f") == 0 || strcmp(lower, "n") == 0 || strcmp(lower, "0") == 0)
            *(bool *)field = FALSE;
        else
            break;
        return SUCCESS;
    }
    case PARAM_TEXT:
        if (strlen(value) >= MAXLENGTHOFSTRSCMD || strpbrk(value, "\r\n") != NULL)
            break;
        *(char **)field = strdup(value);
        if (*(char **)field == NULL) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "Out of memory reading '%s'", param->name);
            return FAILURE;
        }
        return SUCCESS;
    }

    snprintf(cmd->error_message, sizeof(cmd->error_message),
             "Invalid value '%.120s' for parameter '%s'", value, param->name);
    return FAILURE;
}

static unsigned int input_override_bit(const struct run_parameter *param)
{
    switch (param->offset) {
    case offsetof(struct cmdline_data, fnamePS): return FKPT2_INPUT_FILE;
    case offsetof(struct cmdline_data, mgmodel): return FKPT2_INPUT_MODEL;
    case offsetof(struct cmdline_data, is_PS_input_LCDM): return FKPT2_INPUT_RESCALE;
    case offsetof(struct cmdline_data, om): return FKPT2_INPUT_OM;
    case offsetof(struct cmdline_data, h): return FKPT2_INPUT_H;
    case offsetof(struct cmdline_data, zout): return FKPT2_INPUT_Z;
    default: return 0;
    }
}

int read_run_parameters(struct cmdline_data *cmd)
{
    char *values[PARAMETER_COUNT] = {NULL};
    cmd->paramfile = GetParam("paramfile");
    int status = SUCCESS;

    if (*cmd->paramfile)
        status = load_parameter_file(cmd, values);

    if (status == SUCCESS) {
        for (size_t i = 0; i < PARAMETER_COUNT; ++i) {
            const struct run_parameter *param = &run_parameters[i];
            const char *value = values[i];
            int on_cli = GetParamStat((char *)param->name) & ARGPARAM;
            if (value != NULL || on_cli)
                cmd->input_overrides |= input_override_bit(param);
            if (value == NULL || on_cli)
                value = GetParam((char *)param->name);
            if (store_parameter(cmd, param, value) == FAILURE) {
                status = FAILURE;
                break;
            }
        }
    }

    for (size_t i = 0; i < PARAMETER_COUNT; ++i)
        free(values[i]);
    if (status == FAILURE)
        free_run_parameters(cmd);
    return status;
}

int fkpt2_read_options(struct cmdline_data *cmd, const fkpt2_option *options, size_t count)
{
    const char *overrides[PARAMETER_COUNT] = {NULL};
    const char *paramfile = NULL;

    for (size_t i = 0; i < count; ++i) {
        const char *name = options[i].name;
        const char *value = options[i].value;
        if (name == NULL || value == NULL) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "Option names and values must be non-null strings");
            return FAILURE;
        }

        const char **entry;
        if (strcmp(name, "paramfile") == 0)
            entry = &paramfile;
        else {
            size_t index = parameter_index(name);
            if (index == PARAMETER_COUNT) {
                snprintf(cmd->error_message, sizeof(cmd->error_message),
                         "Unknown parameter '%s'", name);
                return FAILURE;
            }
            entry = &overrides[index];
        }
        if (*entry != NULL && strcmp(*entry, value) != 0) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "Conflicting values for parameter '%s'", name);
            return FAILURE;
        }
        *entry = value;
    }

    char *file_values[PARAMETER_COUNT] = {NULL};
    cmd->paramfile = (char *)paramfile;
    int status = paramfile != NULL && *paramfile ? load_parameter_file(cmd, file_values) : SUCCESS;
    if (status == SUCCESS) {
        for (size_t i = 0; i < PARAMETER_COUNT; ++i) {
            const struct run_parameter *param = &run_parameters[i];
            const char *value = overrides[i] != NULL ? overrides[i] : file_values[i];
            if (value != NULL)
                cmd->input_overrides |= input_override_bit(param);
            if (value == NULL)
                value = strcmp(param->name, "verbose") == 0 ? "0" : param->default_value;
            if (store_parameter(cmd, param, value) == FAILURE) {
                status = FAILURE;
                break;
            }
        }
    }
    for (size_t i = 0; i < PARAMETER_COUNT; ++i)
        free(file_values[i]);
    return status;
}

void free_run_parameters(struct cmdline_data *cmd)
{
    free(cmd->class_parameters);
    cmd->class_parameters = NULL;
    for (size_t i = 0; i < PARAMETER_COUNT; ++i) {
        const struct run_parameter *param = &run_parameters[i];
        if (param->kind == PARAM_TEXT) {
            char **value = parameter_field(cmd, param);
            free(*value);
            *value = NULL;
        }
    }
}

static void write_quoted_value(FILE *file, const char *value)
{
    fputc('"', file);
    for (; *value; ++value) {
        if (*value == '"' || *value == '\\')
            fputc('\\', file);
        fputc(*value, file);
    }
    fputc('"', file);
}

int write_run_parameters(struct cmdline_data *cmd, struct global_data *gd)
{
    const char *routineName = "write_run_parameters";
    const char *base = strrchr(cmd->paramfile, '/');
    base = base != NULL ? base + 1 : cmd->paramfile;
    if (*base == '\0')
        base = "parameters_null-fkpt2";

    char path[BUFFERSIZE];
    if (format_checked(path, sizeof(path), "used-values path", "%s/%s%s-usedvalues",
                       gd->tmpDir, base, cmd->suffixModel) != 0) {
        snprintf(cmd->error_message, sizeof(cmd->error_message), "Used-values path is too long");
        return FAILURE;
    }

    FILE *file = NULL;
    OPEN_OUTPUT_OR_FAIL(file, path, "w!");
    fprintf(file, "# Effective fkPT2 parameters\n");
    if (cmd->class_parameters != NULL)
        fputs("[fkpt2]\n", file);
    for (size_t i = 0; i < PARAMETER_COUNT; ++i) {
        const struct run_parameter *param = &run_parameters[i];
        const void *field = parameter_field(cmd, param);

        fprintf(file, "%s = ", param->name);
        switch (param->kind) {
        case PARAM_REAL:
            fprintf(file, "%.17g", (double)*(const real *)field);
            break;
        case PARAM_INT:
            fprintf(file, "%d", *(const int *)field);
            break;
        case PARAM_BOOL:
            fputs(*(const bool *)field ? "true" : "false", file);
            break;
        case PARAM_TEXT:
            write_quoted_value(file, *(char *const *)field);
            break;
        }
        fputc('\n', file);
    }
    if (cmd->class_parameters != NULL) {
        fputs("\n[class]\n", file);
        fputs(cmd->class_parameters, file);
    }
    CLOSE_OUTPUT_OR_FAIL(file, path);
    return SUCCESS;
}
