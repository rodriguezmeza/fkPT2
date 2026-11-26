#include "fkpt2_addon.h"
#include "fkpt2_internal.h"
#include <stdint.h>

struct diagnostic {
    const char *name;
    double value;
};

struct fkpt2_result {
    double *data[FKPT2_TABLE_COUNT];
    size_t rows[FKPT2_TABLE_COUNT];
    fkpt2_summary summary;
    const char *model;
    struct diagnostic *diagnostics;
    size_t diagnostic_count;
    fkpt2_addon_table *addon_tables;
    size_t addon_table_count;
};

static const char *const linear_columns[FKPT2_LINEAR_COLUMNS] = {
    "k", "pklinear", "fk", "Dplus", "pklinear_nw"
};

static const char *const loop_columns[FKPT2_LOOP_COLUMNS] = {
    "k", "P22dd", "P22du", "P22uu", "P13dd", "P13du", "P13uu",
    "I1udd1A", "I2uud1A", "I2uud2A", "I3uuu2A", "I3uuu3A",
    "I2uudd1D", "I2uudd2D", "I3uuud2D", "I3uuud3D", "I4uuuu2D", "I4uuuu3D", "I4uuuu4D",
    "Pb1b2", "Pb1bs2", "Pb22", "Pb2s2", "Ps22", "Pb2theta", "Pbs2theta",
    "sigma32pkl", "pklinear", "fk", "f0"
};

static const char *const rsd_columns[FKPT2_RSD_COLUMNS] = {
    "k", "P0", "P2", "P4"
};

static const struct {
    const char *const *names;
    size_t count;
} table_columns[FKPT2_TABLE_COUNT] = {
    [FKPT2_LINEAR] = {linear_columns, FKPT2_LINEAR_COLUMNS},
    [FKPT2_LOOPS] = {loop_columns, FKPT2_LOOP_COLUMNS},
    [FKPT2_LOOPS_NOWIGGLE] = {loop_columns, FKPT2_LOOP_COLUMNS},
    [FKPT2_MULTIPOLES] = {rsd_columns, FKPT2_RSD_COLUMNS},
};

const char *fkpt2_column_name(fkpt2_table table, size_t column)
{
    if ((unsigned int)table >= FKPT2_TABLE_COUNT || column >= table_columns[table].count)
        return NULL;
    return table_columns[table].names[column];
}

const double *fkpt2_result_table(const fkpt2_result *result, fkpt2_table table,
                                 size_t *rows, size_t *columns)
{
    if ((unsigned int)table >= FKPT2_TABLE_COUNT)
        return NULL;
    if (rows != NULL)
        *rows = result->rows[table];
    if (columns != NULL)
        *columns = table_columns[table].count;
    return result->data[table];
}

fkpt2_summary fkpt2_result_summary(const fkpt2_result *result)
{
    return result->summary;
}

const char *fkpt2_result_model(const fkpt2_result *result)
{
    return result->model;
}

const char *fkpt2_result_diagnostic(const fkpt2_result *result, size_t index, double *value)
{
    if (index >= result->diagnostic_count)
        return NULL;
    if (value != NULL)
        *value = result->diagnostics[index].value;
    return result->diagnostics[index].name;
}

const fkpt2_addon_table *fkpt2_result_addon_table(const fkpt2_result *result, size_t index)
{
    return index < result->addon_table_count ? &result->addon_tables[index] : NULL;
}

double *fkpt2_alloc_addon_table(fkpt2_result *result, const char *name, size_t rows,
                               size_t columns, const char *const column_names[])
{
    if (*name == '\0' || strspn(name, "abcdefghijklmnopqrstuvwxyz0123456789_") != strlen(name) ||
        streq(name, "linear") || streq(name, "kfunctions") || streq(name, "kfunctions_nw") ||
        streq(name, "rsd_multipoles"))
        error("Invalid addon table name '%s'", name);
    for (size_t i = 0; i < result->addon_table_count; ++i)
        if (streq(name, result->addon_tables[i].name))
            error("Duplicate addon table '%s'", name);
    if (columns == 0 || rows == 0 || rows > SIZE_MAX / columns / sizeof(double))
        error("Invalid size for addon table '%s'", name);

    fkpt2_addon_table *tables = realloc(result->addon_tables,
                                        (result->addon_table_count + 1) * sizeof(*tables));
    if (tables == NULL)
        error("Out of memory adding table '%s'", name);
    result->addon_tables = tables;
    double *data = calloc(rows * columns, sizeof(*data));
    if (data == NULL)
        error("Out of memory allocating addon table '%s'", name);

    tables[result->addon_table_count++] = (fkpt2_addon_table){name, column_names, rows, columns, data};
    return data;
}

void fkpt2_result_free(fkpt2_result *result)
{
    if (result == NULL)
        return;
    for (size_t i = 0; i < FKPT2_TABLE_COUNT; ++i)
        free(result->data[i]);
    for (size_t i = 0; i < result->addon_table_count; ++i)
        free((void *)result->addon_tables[i].data);
    free(result->addon_tables);
    free(result->diagnostics);
    free(result);
}

void fkpt2_linear_row(int index, double row[FKPT2_LINEAR_COLUMNS])
{
    row[0] = kPS[index];
    row[1] = pPS[index];
    row[2] = fkT[index];
    row[3] = DplusT[index];
    row[4] = pPS_nw[index];
}

void fkpt2_loop_row(const global_kFArrays *arrays, int index, double f0,
                    double row[FKPT2_LOOP_COLUMNS])
{
    row[0] = arrays->kT[index];
    row[1] = arrays->P22ddT[index];
    row[2] = arrays->P22duT[index];
    row[3] = arrays->P22uuT[index];
    row[4] = arrays->P13ddT[index];
    row[5] = arrays->P13duT[index];
    row[6] = arrays->P13uuT[index];
    row[7] = arrays->I1udd1AT[index];
    row[8] = arrays->I2uud1AT[index];
    row[9] = arrays->I2uud2AT[index];
    row[10] = arrays->I3uuu2AT[index];
    row[11] = arrays->I3uuu3AT[index];
    row[12] = arrays->I2uudd1BpCT[index];
    row[13] = arrays->I2uudd2BpCT[index];
    row[14] = arrays->I3uuud2BpCT[index];
    row[15] = arrays->I3uuud3BpCT[index];
    row[16] = arrays->I4uuuu2BpCT[index];
    row[17] = arrays->I4uuuu3BpCT[index];
    row[18] = arrays->I4uuuu4BpCT[index];
    row[19] = arrays->Pb1b2T[index];
    row[20] = arrays->Pb1bs2T[index];
    row[21] = arrays->Pb22T[index];
    row[22] = arrays->Pb2s2T[index];
    row[23] = arrays->Ps22T[index];
    row[24] = arrays->Pb2thetaT[index];
    row[25] = arrays->Pbs2thetaT[index];
    row[26] = arrays->sigma32PSLT[index];
    row[27] = arrays->pklT[index];
    row[28] = arrays->fkT[index];
    row[29] = f0;
}

static int capture_diagnostics(struct cmdline_data *cmd, fkpt2_result *result)
{
    size_t count = 0;
    const struct fkpt2_addon *addon;
    for (size_t i = 0; (addon = fkpt2_addon_at(i)) != NULL; ++i)
        count += addon->analyze != NULL;

    if (count == 0)
        return SUCCESS;

    result->diagnostics = calloc(count, sizeof(*result->diagnostics));
    if (result->diagnostics == NULL)
        error("Out of memory allocating addon diagnostics");

    for (size_t i = 0; (addon = fkpt2_addon_at(i)) != NULL; ++i) {
        if (addon->analyze == NULL)
            continue;

        double value = NAN;
        char message[FKPT2_ERROR_SIZE] = "";
        int status = addon->analyze(result, &value, message);
        if (status != FKPT2_OK || !isfinite(value)) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "Addon '%s': %s", addon->name, *message ? message : "invalid diagnostic");
            return FAILURE;
        }
        result->diagnostics[result->diagnostic_count++] = (struct diagnostic){addon->name, value};
    }
    return SUCCESS;
}

int fkpt2_capture_result(struct cmdline_data *cmd, struct global_data *gd,
                         fkpt2_result **out)
{
    fkpt2_result *result = calloc(1, sizeof(*result));
    if (result == NULL)
        error("Out of memory allocating fkPT2 result");
    *out = result;
    result->summary = (fkpt2_summary){cmd->zout, gd->sigma8, gd->f0, gd->Dplus};
    result->model = gd->model->name;
    if (!isfinite(gd->sigma8) || !isfinite(gd->f0) || !isfinite(gd->Dplus)) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "Solver produced nonfinite growth or sigma8");
        return FAILURE;
    }

    for (fkpt2_table table = FKPT2_LINEAR; table < FKPT2_TABLE_COUNT; ++table) {
        const size_t rows = table == FKPT2_LINEAR ? (size_t)nPSLT : (size_t)cmd->Nk;
        const size_t columns = table_columns[table].count;
        if (rows > SIZE_MAX / columns / sizeof(double))
            error("fkPT2 result table is too large");
        result->rows[table] = rows;
        result->data[table] = calloc(rows * columns, sizeof(double));
        if (result->data[table] == NULL)
            error("Out of memory allocating an fkPT2 result table");

        for (int i = 0; (size_t)i < rows; ++i) {
            double *row = result->data[table] + (size_t)i * columns;
            switch (table) {
            case FKPT2_LINEAR:
                fkpt2_linear_row(i + 1, row);
                break;
            case FKPT2_LOOPS:
                fkpt2_loop_row(&kFArrays, i, gd->f0, row);
                break;
            case FKPT2_LOOPS_NOWIGGLE:
                fkpt2_loop_row(&kFArrays_nw, i, gd->f0, row);
                break;
            case FKPT2_MULTIPOLES:
                fkpt2_rsd_row(cmd, gd, i, row);
                break;
            default:
                break;
            }
            for (size_t column = 0; column < columns; ++column) {
                if (!isfinite(row[column])) {
                    snprintf(cmd->error_message, sizeof(cmd->error_message),
                             "Solver produced a nonfinite result in table %d, row %d, column %s",
                             table, i + 1, fkpt2_column_name(table, column));
                    return FAILURE;
                }
            }
        }
    }
    const struct fkpt2_addon *addon;
    for (size_t i = 0; (addon = fkpt2_addon_at(i)) != NULL; ++i) {
        const struct fkpt2_table_producer *tables = addon->tables;
        if (tables != NULL && tables->enabled(cmd) && tables->compute(cmd, gd, result) == FAILURE)
            return FAILURE;
    }
    for (size_t i = 0; i < result->addon_table_count; ++i) {
        const fkpt2_addon_table *table = &result->addon_tables[i];
        for (size_t cell = 0; cell < table->rows * table->columns; ++cell)
            if (!isfinite(table->data[cell]))
                error("Addon table '%s' has a nonfinite value at row %zu, column %s",
                      table->name, cell / table->columns + 1, table->column_names[cell % table->columns]);
    }
    return capture_diagnostics(cmd, result);
}
