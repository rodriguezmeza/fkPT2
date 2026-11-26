/* Keep CLASS's headers out of the fkPT2 translation units. */
#include "class_backend.h"
#include "class.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

struct class_calculation {
    struct file_content parameters;
    struct precision precision;
    struct background background;
    struct thermodynamics thermo;
    struct perturbations perturbations;
    struct primordial primordial;
    struct fourier fourier;
    struct transfer transfer;
    struct harmonic harmonic;
    struct lensing lensing;
    struct distortions distortions;
    struct output output;
};

/* Use CLASS's line parser, with a stream owned and closed by this addon. */
static int read_class_parameters(char *path, char *contents,
                                 struct file_content *parameters, ErrorMsg error)
{
    FILE *file = contents == NULL ? fopen(path, "r") : fmemopen(contents, strlen(contents), "r");
    if (file == NULL) {
        snprintf(error, _ERRORMSGSIZE_, "Cannot open CLASS input: %s", strerror(errno));
        return _FAILURE_;
    }

    int status = parser_init(parameters, 16, path, error);
    char *line = NULL;
    size_t capacity = 0;
    int used = 0;
    while (status == _SUCCESS_ && getline(&line, &capacity, file) >= 0) {
        const char *start = line;
        while (isspace((unsigned char)*start))
            ++start;
        if (*start == '=') {
            snprintf(error, _ERRORMSGSIZE_, "Missing CLASS parameter name");
            status = _FAILURE_;
            break;
        }

        FileArg name, value;
        int data_line;
        status = parser_read_line(line, &data_line, name, value, error);
        if (status == _FAILURE_ || !data_line)
            continue;
        if (used == parameters->size) {
            if (parameters->size > INT_MAX / 2) {
                snprintf(error, _ERRORMSGSIZE_, "Too many CLASS input parameters");
                status = _FAILURE_;
                break;
            }
            status = parser_extend(parameters, parameters->size, error);
            if (status == _FAILURE_)
                break;
        }
        strcpy(parameters->name[used], name);
        strcpy(parameters->value[used], value);
        parameters->read[used++] = _FALSE_;
    }
    if (status == _SUCCESS_ && ferror(file)) {
        snprintf(error, _ERRORMSGSIZE_, "Error reading CLASS input: %s", strerror(errno));
        status = _FAILURE_;
    }
    free(line);
    fclose(file);

    if (status == _SUCCESS_) {
        /* An empty file requests the default cosmology. */
        if (used == 0) {
            strcpy(parameters->name[0], "output");
            strcpy(parameters->value[0], "mPk");
            parameters->read[0] = _FALSE_;
            used = 1;
        }
        parameters->size = used;
    }
    return status;
}

static int class_parameter_index(const struct file_content *parameters, const char *name)
{
    for (int i = 0; i < parameters->size; ++i)
        if (strcmp(parameters->name[i], name) == 0)
            return i;
    return -1;
}

static int class_set_parameter(struct file_content *parameters, const char *name, const char *value,
                               ErrorMsg error)
{
    int i = class_parameter_index(parameters, name);
    if (i < 0) {
        i = parameters->size;
        if (parser_extend(parameters, 1, error) == _FAILURE_)
            return _FAILURE_;
        strcpy(parameters->name[i], name);
    }
    strcpy(parameters->value[i], value);
    parameters->read[i] = _FALSE_;
    return _SUCCESS_;
}

static int class_get_redshift(const struct file_content *parameters, const char *name,
                              double default_value, double *value, ErrorMsg error)
{
    const int i = class_parameter_index(parameters, name);
    if (i < 0) {
        *value = default_value;
        return _SUCCESS_;
    }
    char *end;
    errno = 0;
    *value = strtod(parameters->value[i], &end);
    if (end == parameters->value[i] || *end != '\0' || errno == ERANGE ||
        !isfinite(*value) || *value < 0.0) {
        snprintf(error, _ERRORMSGSIZE_, "Supply one finite, nonnegative CLASS redshift in %s", name);
        return _FAILURE_;
    }
    return _SUCCESS_;
}

static int set_linear_parameters(struct file_content *parameters, double *z, ErrorMsg error)
{
    for (int i = 0; i < parameters->size; ++i)
        if (class_parameter_index(parameters, parameters->name[i]) != i) {
            snprintf(error, _ERRORMSGSIZE_, "Duplicate CLASS parameter '%.100s'", parameters->name[i]);
            return _FAILURE_;
        }

    double z_max;
    if (class_get_redshift(parameters, "z_pk", 0.5, z, error) == _FAILURE_ ||
        class_get_redshift(parameters, "z_max_pk", *z, &z_max, error) == _FAILURE_)
        return _FAILURE_;
    if (z_max < *z) {
        snprintf(error, _ERRORMSGSIZE_, "CLASS z_max_pk must cover the requested spectrum redshift");
        return _FAILURE_;
    }

    char redshift[32];
    snprintf(redshift, sizeof(redshift), "%.17g", *z);
    if (class_set_parameter(parameters, "output", "mPk", error) == _FAILURE_ ||
        class_set_parameter(parameters, "non_linear", "none", error) == _FAILURE_ ||
        class_set_parameter(parameters, "write_parameters", "no", error) == _FAILURE_ ||
        class_set_parameter(parameters, "z_pk", redshift, error) == _FAILURE_)
        return _FAILURE_;

    if (z_max == 0.0 && class_set_parameter(parameters, "z_max_pk", "0.001", error) == _FAILURE_)
        return _FAILURE_;
    if (class_parameter_index(parameters, "P_k_max_h/Mpc") < 0 &&
        class_parameter_index(parameters, "P_k_max_1/Mpc") < 0 &&
        class_set_parameter(parameters, "P_k_max_h/Mpc", "10", error) == _FAILURE_)
        return _FAILURE_;
    return _SUCCESS_;
}

static int class_initialize(struct class_calculation *run, ErrorMsg error)
{
    struct precision *pr = &run->precision;
    struct background *ba = &run->background;
    struct thermodynamics *th = &run->thermo;
    struct perturbations *pt = &run->perturbations;
    struct primordial *pm = &run->primordial;
    struct fourier *fo = &run->fourier;

    if (input_read_from_file(&run->parameters, pr, ba, th, pt, &run->transfer,
                            pm, &run->harmonic, fo, &run->lensing, &run->distortions,
                            &run->output, error) == _FAILURE_)
        return _FAILURE_;

    class_call(background_init(pr, ba), ba->error_message, error);
    class_test(fabs(ba->Omega0_k) > 1e-9, error,
               "The CLASS workflow requires a spatially flat background for fkPT2");
    class_test(fabs(ba->Omega0_fld) > 1e-9 || fabs(ba->Omega0_scf) > 1e-9 ||
               fabs(ba->Omega0_idm) > 1e-9 || fabs(ba->Omega0_dcdm) > 1e-9, error,
               "This CLASS background requires a different fkPT2 growth model");

    class_call(thermodynamics_init(pr, ba, th), th->error_message, error);
    class_call(perturbations_init(pr, ba, th, pt), pt->error_message, error);
    class_call(primordial_init(pr, pt, pm), pm->error_message, error);
    class_call(fourier_init(pr, ba, th, pt, pm, fo), fo->error_message, error);
    return _SUCCESS_;
}

static int class_extract_spectrum(struct class_calculation *run, double z,
                                  struct class_spectrum *spectrum, ErrorMsg error)
{
    struct background *ba = &run->background;
    struct fourier *fo = &run->fourier;
    class_test(fo->k_size < 10 || (size_t)fo->k_size > SIZE_MAX / (2 * sizeof(double)),
               error, "Invalid size of the CLASS wavenumber grid");
    const size_t count = (size_t)fo->k_size;
    double *data = malloc(2 * count * sizeof(*data));
    class_test(data == NULL, error, "Out of memory copying the linear CLASS spectrum");
    double *power = data + count;

    /* The standalone CLASS output uses these same samples and units. */
    if (fourier_pk_at_z(ba, fo, logarithmic, pk_linear, z, fo->index_pk_cluster,
                       power, NULL) == _FAILURE_) {
        free(data);
        snprintf(error, _ERRORMSGSIZE_, "%s", fo->error_message);
        return _FAILURE_;
    }

    const double volume = pow(ba->h, 3);
    for (size_t i = 0; i < count; ++i) {
        data[i] = exp(fo->ln_k[i]) / ba->h;
        power[i] = exp(power[i]) * volume;
    }
    *spectrum = (struct class_spectrum){
        .h = ba->h, .omega_m = ba->Omega0_m, .omega_nu = ba->Omega0_ncdm_tot,
        .redshift = z, .count = count, .data = data,
    };
    return _SUCCESS_;
}

int fkpt2_class_backend(char *path, char *contents, struct class_spectrum *spectrum,
                        char *message, size_t size)
{
    struct class_calculation *run = calloc(1, sizeof(*run));
    if (run == NULL) {
        snprintf(message, size, "Out of memory initializing CLASS");
        return _FAILURE_;
    }
    ErrorMsg error;
    double z;
    int status = read_class_parameters(path, contents, &run->parameters, error);
    if (status == _SUCCESS_)
        status = set_linear_parameters(&run->parameters, &z, error);
    if (status == _SUCCESS_)
        status = class_initialize(run, error);
    if (status == _SUCCESS_)
        status = class_extract_spectrum(run, z, spectrum, error);
    if (status == _FAILURE_)
        snprintf(message, size, "CLASS: %s", error);

    if (run->fourier.is_allocated)
        fourier_free(&run->fourier);
    if (run->primordial.is_allocated)
        primordial_free(&run->primordial);
    if (run->perturbations.is_allocated)
        perturbations_free(&run->perturbations);
    if (run->thermo.is_allocated)
        thermodynamics_free(&run->thermo);
    if (run->background.is_allocated)
        background_free(&run->background);
    parser_free(&run->parameters);
    free(run);
    return status;
}
