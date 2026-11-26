#ifndef FKPT2_INTERNAL_H
#define FKPT2_INTERNAL_H

#include "globaldefs.h"
#include "fkpt2.h"

#define FKPT2_LINEAR_COLUMNS 5
#define FKPT2_LOOP_COLUMNS 30
#define FKPT2_RSD_COLUMNS 4

int fkpt2_with_workspace(int (*run)(void *), void *state, char *error, size_t size);
void fkpt2_parallel_for(int count, int threads, void (*work)(void *, int), void *state);
void *fkpt2_work_calloc(size_t count, size_t size);
void *fkpt2_work_own(void *memory);
void fkpt2_work_free(void *memory);
void fkpt2_work_raise(const char *message);

int fkpt2_read_options(struct cmdline_data *cmd, const fkpt2_option *options, size_t count);
int fkpt2_validate_class_source(struct cmdline_data *cmd, const double *k);
int fkpt2_prepare(struct cmdline_data *cmd, struct global_data *gd, int file_output,
                  const double *k, const double *pk, size_t size);
void fkpt2_solve(struct cmdline_data *cmd, struct global_data *gd);
int fkpt2_write_run(struct cmdline_data *cmd, struct global_data *gd,
                    fkpt2_result **analysis_result);

void fkpt2_linear_row(int index, double row[FKPT2_LINEAR_COLUMNS]);
void fkpt2_loop_row(const global_kFArrays *arrays, int index, double f0,
                    double row[FKPT2_LOOP_COLUMNS]);
void fkpt2_rsd_row(struct cmdline_data *cmd, struct global_data *gd, int index,
                   double row[FKPT2_RSD_COLUMNS]);

const struct fkpt2_addon *fkpt2_addon_at(size_t index);
int fkpt2_validate_addons(struct cmdline_data *cmd);
int fkpt2_addon_spectrum(const char *name, struct cmdline_data *cmd,
                         const double **k, const double **power, size_t *count);
void fkpt2_write_spectrum_source(FILE *out, const struct cmdline_data *cmd);
int fkpt2_has_addon_results(const struct cmdline_data *cmd);
int fkpt2_write_addon_tables(struct cmdline_data *cmd, const fkpt2_result *result);
int fkpt2_capture_result(struct cmdline_data *cmd, struct global_data *gd,
                         fkpt2_result **out);

#endif
