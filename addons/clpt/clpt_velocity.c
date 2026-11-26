/* Valogiannis, Bean & Aviles (2020), Eqs. 3.30-3.44 and Appendix A.
 * Primes are d/d ln(a), so pairwise velocities are displacements in Mpc/h. */
#include "clpt_velocity.h"

struct w_terms {
    double v1, v3, t;
};

/* The sum over the three cyclic placements determines T and the sum of V's.
 * The difference between the two distinct placements splits that V sum. */
static struct w_terms w_terms(double weight, double q_i, double q_2,
                             double r_i, double r_2, double difference, int two_derivatives)
{
    const double sum = -3.0 / 35.0 * weight * (q_i - 3.0 * q_2 + 2.0 * r_i - 6.0 * r_2);
    const double v1 = (sum - (1 + two_derivatives) * difference) / 3.0;
    return (struct w_terms){v1, v1 + difference,
        -3.0 / 14.0 * weight * (q_i + 2.0 * q_2 + 2.0 * r_i + 4.0 * r_2)};
}

struct clpt_velocity_fourier clpt_velocity_fourier(
    double k, double w, double linear,
    const double kernels[CLPT_KERNEL_WEIGHTS][CLPT_KERNEL_COUNT])
{
    const double *base = kernels[CLPT_DENSITY], *d = kernels[CLPT_NONLINEAR_DERIVATIVE];
    const double *star = kernels[CLPT_LINEAR_DERIVATIVE], *ds = kernels[CLPT_MIXED_DERIVATIVE];
    const double *ss = kernels[CLPT_TWO_LINEAR_DERIVATIVES];
    const double f = clpt_growth_rate(k);
    const double u_d = -0.5 * k * linear * f;
    const struct w_terms wd = w_terms(w / k,
        2.0 * (d[CLPT_QI] + star[CLPT_QI]), 2.0 * (d[CLPT_Q2] + star[CLPT_Q2]),
        2.0 * d[CLPT_RI] + f * base[CLPT_RI] + star[CLPT_RI],
        2.0 * d[CLPT_R2] + f * base[CLPT_R2] + star[CLPT_R2],
        -3.0 / 14.0 * w / k * (2.0 * d[CLPT_QI] - star[CLPT_QI] +
                               f * base[CLPT_RI] - 2.0 * d[CLPT_RI]), 0);
    const struct w_terms wdd = w_terms(w / k,
        4.0 * ds[CLPT_QI] + ss[CLPT_QI], 4.0 * ds[CLPT_Q2] + ss[CLPT_Q2],
        2.0 * (f * d[CLPT_RI] + ds[CLPT_RI]) + f * star[CLPT_RI],
        2.0 * (f * d[CLPT_R2] + ds[CLPT_R2]) + f * star[CLPT_R2],
        -3.0 / 14.0 * w / k * (2.0 * ds[CLPT_QI] - ss[CLPT_QI] - 4.0 * ds[CLPT_RI] +
                               f * (2.0 * d[CLPT_RI] + star[CLPT_RI])), 1);
    const double bias_d = 2.0 * d[CLPT_RI] + star[CLPT_RI];

    return (struct clpt_velocity_fourier){
        .linear_d = linear * f, .linear_dd = linear * f * f,
        .loop_d = w * (9.0 / 49.0 * d[CLPT_Q1] + 5.0 / 21.0 * f * base[CLPT_R1] +
                      5.0 / 7.0 * d[CLPT_R1]),
        .loop_dd = w * (18.0 / 49.0 * ds[CLPT_Q1] + 10.0 / 7.0 * f * d[CLPT_R1]),
        .ri_d = 3.0 / 28.0 * w * bias_d,
        .y10_d = 3.0 / 28.0 * w * (bias_d +
            4.0 * (d[CLPT_R2] + d[CLPT_R1PLUS2] + d[CLPT_Q5]) +
            2.0 * (star[CLPT_R2] + star[CLPT_R1PLUS2] + star[CLPT_Q5])),
        .ri_dd = 3.0 / 7.0 * w * ds[CLPT_RI],
        .y10_dd = 3.0 / 7.0 * w * (ds[CLPT_RI] +
                         2.0 * (ds[CLPT_R2] + ds[CLPT_R1PLUS2] + ds[CLPT_Q5])),
        .v1_d = wd.v1, .v3_d = wd.v3, .t_d = wd.t,
        .v1_dd = wdd.v1, .v3_dd = wdd.v3, .t_dd = wdd.t,
        .u_d = u_d, .u_loop_d = -5.0 / 14.0 * w * k * d[CLPT_R1],
        .u11_d = -6.0 / 7.0 * w * k * d[CLPT_R1PLUS2],
        .u20_d = -3.0 / 7.0 * w * k * d[CLPT_Q8],
        .u_curv_d = -k * k * u_d * exp(-k * k),
        .v10_d = -3.0 / 7.0 * w * k * d[CLPT_QS],
    };
}

void clpt_velocity_transform(const struct clpt_velocity_fourier *p,
                             double j0, double j1, double j2, double j3, double delta,
                             double v[CLPT_VELOCITY_TERMS])
{
    v[X_D_L] += p->linear_d * delta;
    v[Y_D_L] += p->linear_d * j2;
    v[X_D_LOOP] += p->loop_d * delta;
    v[Y_D_LOOP] += p->loop_d * j2;
    v[X_DD_L] += p->linear_dd * delta;
    v[Y_DD_L] += p->linear_dd * j2;
    v[X_DD_LOOP] += p->loop_dd * delta;
    v[Y_DD_LOOP] += p->loop_dd * j2;
    v[X10_D] += p->ri_d * (j0 - 1.0) + p->y10_d * delta;
    v[Y10_D] += p->y10_d * j2;
    v[X10_DD] += p->ri_dd * (j0 - 1.0) + p->y10_dd * delta;
    v[Y10_DD] += p->y10_dd * j2;
    v[V1_D] += p->v1_d * j1 - 0.2 * p->t_d * j3;
    v[V3_D] += p->v3_d * j1 - 0.2 * p->t_d * j3;
    v[T_D] += p->t_d * j3;
    v[V1_DD] += p->v1_dd * j1 - 0.2 * p->t_dd * j3;
    v[V3_DD] += p->v3_dd * j1 - 0.2 * p->t_dd * j3;
    v[T_DD] += p->t_dd * j3;
    v[U_D_L] += p->u_d * j1;
    v[U_D_LOOP] += p->u_loop_d * j1;
    v[U11_D] += p->u11_d * j1;
    v[U20_D] += p->u20_d * j1;
    v[U_CURV_D] += p->u_curv_d * j1;
}

struct dispersion_projection {
    double qq, gg, qg; /* Components along the projection, averaged for the transverse plane. */
};

static double projected_dispersion(const struct cmdline_data *cmd, double xi, double u,
                                    const double v[CLPT_VELOCITY_TERMS],
                                    const struct clpt_velocity_angle *angle,
                                    struct dispersion_projection p)
{
    const double b1 = cmd->clpt_b1, b2 = cmd->clpt_b2;
    const double g_q = angle->g_q;
    const double qGq = angle->inv_par - g_q * g_q;
    const double Gee = angle->inv_perp + (angle->inv_par - angle->inv_perp) * p.qq - p.gg;
    const double qGqe = angle->inv_par * p.qq - g_q * p.qg;
    const double ad = v[X_D_L], bd = v[Y_D_L];
    const double linear = v[X_DD_L] + v[Y_DD_L] * p.qq;
    return linear * (1.0 + b1 * b1 * xi - 2.0 * b1 * u * g_q) +
           v[X_DD_LOOP] + v[Y_DD_LOOP] * p.qq +
           2.0 * b1 * (v[X10_DD] + v[Y10_DD] * p.qq) +
           2.0 * (b1 * b1 + b2) * v[U_D_L] * v[U_D_L] * p.qq -
           4.0 * b1 * v[U_D_L] * (ad * p.qg + bd * g_q * p.qq) -
           ad * ad * Gee - 2.0 * ad * bd * qGqe - bd * bd * qGq * p.qq -
           v[V1_DD] * g_q - 2.0 * v[V3_DD] * p.qg - v[T_DD] * g_q * p.qq;
}

void clpt_velocity_convolve(const struct cmdline_data *cmd, double xi, double u,
                            const double v[CLPT_VELOCITY_TERMS],
                            const struct clpt_tidal_terms *tidal,
                            const struct clpt_velocity_angle *angle, double gauss,
                            struct clpt_velocity_moments *moments)
{
    const double c = angle->cosine, s = angle->sine;
    const double g_q = angle->g_q, g_r = c * g_q + s * angle->g_t;
    const double qGq = angle->inv_par - g_q * g_q;
    const double qGr = angle->inv_par * c - g_q * g_r;
    const double b1 = cmd->clpt_b1, b2 = cmd->clpt_b2;
    const double linear = v[X_D_L] * g_r + v[Y_D_L] * c * g_q;
    /* U_CURV_D is minus the gradient of the smoothed xi weighted by f(k). */
    const double curvature = 2.0 * cmd->clpt_b01 - cmd->c2eft;
    const double mean = -linear * (1.0 + b1 * b1 * xi) -
        v[X_D_LOOP] * g_r - v[Y_D_LOOP] * c * g_q -
        0.5 * (2.0 * v[V1_D] * qGr + v[V3_D] * c * angle->trace_g + v[T_D] * c * qGq) +
        2.0 * b1 * ((v[U_D_L] + v[U_D_LOOP]) * c - v[X10_D] * g_r - v[Y10_D] * c * g_q -
                    u * (v[X_D_L] * qGr + v[Y_D_L] * c * qGq)) +
        (b2 * v[U20_D] + b1 * b1 * v[U11_D] + 2.0 * b1 * b2 * xi * v[U_D_L]) * c -
        2.0 * (b2 + b1 * b1) * u * v[U_D_L] * g_q * c + curvature * v[U_CURV_D] * c;

    const double g_perp = s * g_q - c * angle->g_t;
    const struct dispersion_projection parallel = {c * c, g_r * g_r, c * g_r};
    const struct dispersion_projection perpendicular = {
        0.5 * s * s, 0.5 * g_perp * g_perp, 0.5 * s * g_perp
    };
    moments->mean += gauss * mean;
    moments->parallel += gauss * projected_dispersion(cmd, xi, u, v, angle, parallel);
    moments->perpendicular += gauss * projected_dispersion(cmd, xi, u, v, angle, perpendicular);

    if (tidal != NULL) {
        /* The shear operator can be at either tracer in each raw moment. */
        const double weight = 2.0 * cmd->clpt_bs2 * gauss;
        moments->mean += weight * ((tidal->v10_d + b1 * tidal->v12_d) * c -
                                  tidal->x_d * g_r - tidal->y_d * g_q * c);
        moments->parallel += weight * (tidal->x_dd + tidal->y_dd * parallel.qq);
        moments->perpendicular += weight * (tidal->x_dd + tidal->y_dd * perpendicular.qq);
    }
}
