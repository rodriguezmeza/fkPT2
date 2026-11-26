#ifndef FKPT2_DIFFEQ_H
#define FKPT2_DIFFEQ_H

struct cmdline_data;
struct global_data;

typedef void (*fkpt2_derivs)(struct cmdline_data *, struct global_data *,
                             double, double [], double []);

enum fkpt2_ode_method { FKPT2_BSSTEP, FKPT2_RKQS };

/* Integrate in place and return the time of the last accepted step. */
double fkpt2_integrate(struct cmdline_data *cmd, struct global_data *gd,
                       double y[], int n, fkpt2_derivs derivs);

#endif
