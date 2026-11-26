#ifndef FKPT2_CLPT_KERNELS_H
#define FKPT2_CLPT_KERNELS_H

#include "fkpt2_addon.h"

enum clpt_kernel {
    CLPT_Q1, CLPT_Q2, CLPT_QI, CLPT_Q5, CLPT_Q8, CLPT_QS,
    CLPT_R1, CLPT_R2, CLPT_RI, CLPT_R1PLUS2, CLPT_KERNEL_COUNT
};

/* Velocity weights of Appendix A in arXiv:1909.05261, with d/dln(a)
 * in Hubble units. Nonlinear derivatives are divided by their order.
 * The mixed slot holds two second-order derivatives for Q1. */
enum clpt_kernel_weight {
    CLPT_DENSITY, CLPT_NONLINEAR_DERIVATIVE, CLPT_LINEAR_DERIVATIVE,
    CLPT_MIXED_DERIVATIVE, CLPT_TWO_LINEAR_DERIVATIVES, CLPT_KERNEL_WEIGHTS
};

struct clpt_radial_sample {
    double p;
    double weight; /* dp P_L(p) */
    double growth_rate;
};

double clpt_growth_rate(double k);
void clpt_compute_kernels(struct cmdline_data *cmd, struct global_data *gd, double k,
                          const struct clpt_radial_sample *radial, int size, int velocities,
                          double result[CLPT_KERNEL_WEIGHTS][CLPT_KERNEL_COUNT]);

#endif
