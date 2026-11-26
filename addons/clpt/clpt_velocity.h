#ifndef FKPT2_CLPT_VELOCITY_H
#define FKPT2_CLPT_VELOCITY_H

#include "clpt.h"
#include "clpt_kernels.h"

enum clpt_velocity_term {
    X_D_L, Y_D_L, X_D_LOOP, Y_D_LOOP, X_DD_L, Y_DD_L, X_DD_LOOP, Y_DD_LOOP,
    X10_D, Y10_D, X10_DD, Y10_DD, V1_D, V3_D, T_D, V1_DD, V3_DD, T_DD,
    U_D_L, U_D_LOOP, U11_D, U20_D, U_CURV_D, CLPT_VELOCITY_TERMS
};

struct clpt_velocity_fourier {
    double linear_d, loop_d, linear_dd, loop_dd;
    double ri_d, y10_d, ri_dd, y10_dd;
    double v1_d, v3_d, t_d, v1_dd, v3_dd, t_dd;
    double u_d, u_loop_d, u11_d, u20_d, u_curv_d, v10_d;
};

struct clpt_velocity_angle {
    double cosine, sine;
    double inv_perp, inv_par;
    double g_q, g_t, trace_g;
};

/* Upsilon = X I + Y q_hat q_hat. The d and dd slots replace one
 * and two displacements by their velocities, respectively. */
struct clpt_tidal_terms {
    double v10, v12, x, y, zeta, chi;
    double v10_d, v12_d, x_d, y_d, x_dd, y_dd;
};

struct clpt_velocity_moments {
    double mean, parallel, perpendicular;
};

struct clpt_velocity_fourier clpt_velocity_fourier(
    double k, double w, double linear,
    const double kernels[CLPT_KERNEL_WEIGHTS][CLPT_KERNEL_COUNT]);

void clpt_velocity_transform(const struct clpt_velocity_fourier *sample,
                             double j0, double j1, double j2, double j3, double delta,
                             double term[CLPT_VELOCITY_TERMS]);

void clpt_velocity_convolve(const struct cmdline_data *cmd, double xi, double u,
                            const double v[CLPT_VELOCITY_TERMS],
                            const struct clpt_tidal_terms *tidal,
                            const struct clpt_velocity_angle *angle, double gauss,
                            struct clpt_velocity_moments *moments);

#endif
