from libc.stddef cimport size_t

cdef extern from "fkpt2.h":
    enum:
        FKPT2_ABI_VERSION
        FKPT2_ERROR_SIZE
        FKPT2_OK
        FKPT2_INVALID_INPUT

    ctypedef struct fkpt2_option:
        const char *name
        const char *value

    ctypedef struct fkpt2_result:
        pass

    ctypedef enum fkpt2_table:
        FKPT2_LINEAR
        FKPT2_LOOPS
        FKPT2_LOOPS_NOWIGGLE
        FKPT2_MULTIPOLES

    ctypedef struct fkpt2_summary:
        double z
        double sigma8
        double f0
        double growth

    ctypedef struct fkpt2_addon_table:
        const char *name
        const char *const *column_names
        size_t rows
        size_t columns
        const double *data

    const fkpt2_addon_table *fkpt2_result_addon_table(const fkpt2_result *result, size_t index)
    int fkpt2_abi_version()
    int fkpt2_compute(const fkpt2_option *options, size_t option_count,
                      const double *k, const double *power, size_t n,
                      fkpt2_result **result, char *error) noexcept nogil
    void fkpt2_result_free(fkpt2_result *result) noexcept
    const double *fkpt2_result_table(const fkpt2_result *result, fkpt2_table table,
                                     size_t *rows, size_t *columns)
    const char *fkpt2_column_name(fkpt2_table table, size_t column)
    fkpt2_summary fkpt2_result_summary(const fkpt2_result *result)
    const char *fkpt2_result_model(const fkpt2_result *result)
    const char *fkpt2_result_diagnostic(const fkpt2_result *result, size_t index,
                                        double *value)
    const char *fkpt2_model_name(size_t index)
    const char *fkpt2_model_description(size_t index)
    const char *fkpt2_addon_name(size_t index)

cdef extern from "fkpt2_class_provider.h":
    enum:
        FKPT2_INPUT_MODEL
        FKPT2_INPUT_RESCALE
        FKPT2_INPUT_OM
        FKPT2_INPUT_H
        FKPT2_INPUT_Z

    ctypedef struct fkpt2_class_input:
        const char *path
        const char *class_parameters
        unsigned int input_overrides
        double h, om, zout
        int is_ps_input_lcdm
        const char *model
        const char *model_options
        const double *k
        const double *power
        size_t count

    ctypedef int (*fkpt2_class_provider)(fkpt2_class_input *source, void *context) except -1 nogil
    int fkpt2_compute_with_class_provider(const fkpt2_option *options, size_t option_count,
                                          const double *k, const double *power, size_t n,
                                          fkpt2_class_provider provider, void *context,
                                          fkpt2_result **result, char *error) except -1 nogil
