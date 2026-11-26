#ifndef FKPT2_H
#define FKPT2_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FKPT2_ABI_VERSION 2
#define FKPT2_ERROR_SIZE 512

typedef struct fkpt2_result fkpt2_result;

typedef struct {
    const char *name;
    const char *value;
} fkpt2_option;

typedef enum {
    FKPT2_LINEAR,
    FKPT2_LOOPS,
    FKPT2_LOOPS_NOWIGGLE,
    FKPT2_MULTIPOLES,
    FKPT2_TABLE_COUNT
} fkpt2_table;

typedef struct {
    double z;
    double sigma8;
    double f0;
    double growth;
} fkpt2_summary;

typedef struct {
    const char *name;
    const char *const *column_names;
    size_t rows;
    size_t columns;
    const double *data;
} fkpt2_addon_table;

enum {
    FKPT2_OK = 0,
    FKPT2_INVALID_INPUT,
    FKPT2_COMPUTATION_FAILED
};

int fkpt2_abi_version(void);

/*
 * Solve synchronously using the CLI defaults and supplied options. k and power
 * are contiguous arrays of n samples. Pass NULL, NULL, 0 to read fnamePS.
 * Input storage is borrowed for this call only. Runs are serialized internally.
 * Returns an owned result on success and NULL plus a diagnostic on failure.
 * error may be NULL; otherwise it must have room for FKPT2_ERROR_SIZE bytes.
 */
int fkpt2_compute(const fkpt2_option *options, size_t option_count,
                  const double *k, const double *power, size_t n,
                  fkpt2_result **result, char *error);

void fkpt2_result_free(fkpt2_result *result);
/* Row-major, immutable storage valid until fkpt2_result_free(). */
const double *fkpt2_result_table(const fkpt2_result *result, fkpt2_table table,
                                 size_t *rows, size_t *columns);
/* Iterate over enabled addon tables until NULL. Storage belongs to result. */
const fkpt2_addon_table *fkpt2_result_addon_table(const fkpt2_result *result, size_t index);
const char *fkpt2_column_name(fkpt2_table table, size_t column);
fkpt2_summary fkpt2_result_summary(const fkpt2_result *result);
const char *fkpt2_result_model(const fkpt2_result *result);
/* Result diagnostics provided by analysis addons. Names are static strings. */
const char *fkpt2_result_diagnostic(const fkpt2_result *result, size_t index,
                                    double *value);

/* Build capabilities. Iteration ends when the name is NULL. */
const char *fkpt2_model_name(size_t index);
const char *fkpt2_model_description(size_t index);
const char *fkpt2_addon_name(size_t index);

#ifdef __cplusplus
}
#endif
#endif
