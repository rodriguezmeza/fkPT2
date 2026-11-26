#ifndef FKPT2_CLASS_BACKEND_H
#define FKPT2_CLASS_BACKEND_H

#include <stddef.h>

struct class_spectrum {
    double h, omega_m, omega_nu, redshift;
    size_t count;
    /* One owned allocation contains k followed by P(k), both in h units. */
    double *data;
};

int fkpt2_class_backend(char *path, char *contents, struct class_spectrum *spectrum,
                        char *error, size_t error_size);

#endif
