/* Fourier-space correlators from Aviles et al. (2018), Appendix C.1,
 * and time-weighted kernels from Valogiannis et al. (2020), Appendix A. */
#include "clpt_kernels.h"
#include "fkpt2_internal.h"

extern const struct fkpt2_gravity_model fkpt2_lcdm_model;

double clpt_growth_rate(double k)
{
    double value;
    splint(kPS, fkT, fkT2, nPSLT, k, &value);
    return value;
}

struct second_kernel {
    double density;
    double velocity; /* one half the derivative of the displacement */
};

static struct second_kernel q_kernel(struct cmdline_data *cmd, struct global_data *gd, int lcdm,
                                     int velocities, double k, double p, double k2, double sine2)
{
    double a = KA_LCDM, b = KA_LCDM;
    double ap = KA_LCDM * gd->f0 + 0.5 * KAp_LCDM, bp = ap;
    if (!lcdm) {
        global_D2v2_ptr second = DsSecondOrder_func(cmd, gd, k, p, k2);
        const double norm = (3.0 / 7.0) * Dpk1D2(second) * Dpk2D2(second);
        a = DA2D2(second) / norm;
        b = DB2D2(second) / norm;
        if (velocities) {
            ap = 0.5 * DA2primeD2(second) / norm;
            bp = 0.5 * DB2primeD2(second) / norm;
        }
        fkpt2_work_free(second);
    }
    return (struct second_kernel){(a - b) + b * sine2, (ap - bp) + bp * sine2};
}

static void integrate_q(struct cmdline_data *cmd, struct global_data *gd, int lcdm, int velocities,
                        double k, const struct clpt_radial_sample *sample, double pmax,
                        double result[CLPT_KERNEL_WEIGHTS][CLPT_KERNEL_COUNT])
{
    const double ratio = sample->p / k;
    const double ratio2 = ratio * ratio;
    const double small = kPS[1] / k;
    const double large = pmax / k;
    const double low = fmax(-1.0, (1.0 + ratio2 - large * large) / (2.0 * ratio));
    const double high = fmin(fmin(1.0, 0.5 / ratio),
                             (1.0 + ratio2 - small * small) / (2.0 * ratio));
    if (high <= low)
        return;

    const int tidal = cmd->clpt_bs2 != 0.0;
    /* Use |k-p| >= p and symmetrize the integrands before doubling. */
    const double center = (high + low) / 2.0;
    const double half_width = (high - low) / 2.0;
    for (int j = 1; j <= nGL(pGL); ++j) {
        const double x = center + half_width * xGL(pGL)[j];
        const double y = rsqr(1.0 - ratio) + 2.0 * ratio * (1.0 - x);
        const double k2 = k * sqrt(y);
        const double sine2 = (1.0 - x * x) / y;
        const struct second_kernel kernel = q_kernel(cmd, gd, lcdm, velocities, k, sample->p, k2, sine2);
        const double weight = 2.0 * half_width * wGL(pGL)[j] * sample->weight *
                              psInterpolation_nr(k2, kPS, pPS, nPSLT);
        const double rx = ratio * x;
        const double q2 = rx * (1.0 - rx) / y;
        const double qi = ratio2 * sine2;
        const double q5 = 0.5 * (rx + ratio2 * (1.0 - 2.0 * x * x)) / y;
        double *density = result[CLPT_DENSITY];
        density[CLPT_Q1] += weight * ratio2 * kernel.density * kernel.density;
        density[CLPT_Q2] += weight * q2 * kernel.density;
        density[CLPT_QI] += weight * qi * kernel.density;
        density[CLPT_Q5] += weight * q5 * kernel.density;
        density[CLPT_Q8] += weight * ratio2 * kernel.density;
        if (tidal) {
            /* S2 = (p_hat . (k-p)_hat)^2 - 1/3. */
            const double shear = ratio2 * (2.0 / 3.0 - sine2);
            density[CLPT_QS] += weight * shear * kernel.density;
            if (velocities)
                result[CLPT_NONLINEAR_DERIVATIVE][CLPT_QS] += weight * shear * kernel.velocity;
        }
        if (!velocities)
            continue;

        const double fp = sample->growth_rate, f2 = lcdm ? gd->f0 : clpt_growth_rate(k2);
        const double f_mean = 0.5 * (fp + f2);
        /* Q5's differentiated linear leg follows the exchanged momentum. */
        const double q5_star = f2 * q5 + 0.5 * rx * (fp - f2);
        double *d = result[CLPT_NONLINEAR_DERIVATIVE], *star = result[CLPT_LINEAR_DERIVATIVE];
        double *ds = result[CLPT_MIXED_DERIVATIVE], *ss = result[CLPT_TWO_LINEAR_DERIVATIVES];
        const double v = weight * kernel.velocity, p = weight * kernel.density;
        d[CLPT_Q1] += v * ratio2 * kernel.density;
        d[CLPT_Q2] += v * q2;
        d[CLPT_QI] += v * qi;
        d[CLPT_Q5] += v * q5;
        d[CLPT_Q8] += v * ratio2;
        star[CLPT_Q2] += p * q2 * f_mean;
        star[CLPT_QI] += p * qi * f_mean;
        star[CLPT_Q5] += p * q5_star;
        ds[CLPT_Q1] += v * ratio2 * kernel.velocity;
        ds[CLPT_Q2] += v * q2 * f_mean;
        ds[CLPT_QI] += v * qi * f_mean;
        ds[CLPT_Q5] += v * q5_star;
        ss[CLPT_Q2] += p * q2 * fp * f2;
        ss[CLPT_QI] += p * qi * fp * f2;
    }
}

static void add_r_triplet(double kernel[], double w, double ratio2, double x2,
                          double rx, double minus, double plus)
{
    kernel[CLPT_R2] += w * rx * ((1.0 - rx) * minus - (1.0 + rx) * plus);
    kernel[CLPT_RI] += w * ratio2 * x2 * (minus + plus);
    kernel[CLPT_R1PLUS2] += w * ratio2 * ((1.0 - rx) * minus + (1.0 + rx) * plus);
}

static void integrate_r(struct cmdline_data *cmd, struct global_data *gd, int lcdm, int velocities,
                        double k, const struct clpt_radial_sample *sample,
                        double result[CLPT_KERNEL_WEIGHTS][CLPT_KERNEL_COUNT])
{
    const double ratio = sample->p / k;
    const double ratio2 = ratio * ratio;
    const int nodes = nGL(pGL);
    for (int j = 1; j <= (nodes + 1) / 2; ++j) {
        const double x = fabs(xGL(pGL)[j]);
        const double w = sample->weight * wGL(pGL)[j] * (2 * j == nodes + 1 ? 0.5 : 1.0);
        const double one_minus_x2 = 1.0 - x * x;
        const double y_minus = rsqr(1.0 - ratio) + 2.0 * ratio * (1.0 - x);
        const double y_plus = rsqr(1.0 - ratio) + 2.0 * ratio * (1.0 + x);
        double minus = KA_LCDM * one_minus_x2;
        double plus = minus;
        double dminus = (KA_LCDM * gd->f0 + 0.5 * KAp_LCDM) * one_minus_x2;
        double dplus = dminus;
        /* For LCDM the time dependence separates from the angular shape.
         * KR1_LCDM is 21/5 times D3/D^3 evaluated at p/k -> 0, x -> 0. */
        const double geometry = 0.25 * rsqr(one_minus_x2) * (1.0 / y_minus + 1.0 / y_plus);
        double third_kernel = KR1_LCDM * geometry;
        double dthird = KR1p_LCDM * gd->f0 * geometry;
        if (!lcdm) {
            global_D3v2_ptr third = DsThirdOrder_func(cmd, gd, x, k, sample->p);
            const double growth = DpkD3(third) * DppD3(third);
            const double norm = (3.0 / 7.0) * growth;
            minus = D2mfD3(third) / norm;
            plus = D2fD3(third) / norm;
            third_kernel = (21.0 / 10.0) * D3symmD3(third) / (growth * DppD3(third));
            if (velocities) {
                dminus = 0.5 * D2mfprimeD3(third) / norm;
                dplus = 0.5 * D2fprimeD3(third) / norm;
                dthird = (7.0 / 10.0) * D3symmprimeD3(third) / (growth * DppD3(third));
            }
            fkpt2_work_free(third);
        }

        const double rx = ratio * x;
        const double a = minus / y_minus, b = plus / y_plus;
        double *density = result[CLPT_DENSITY];
        density[CLPT_R1] += 2.0 * w * ratio2 * third_kernel;
        add_r_triplet(density, w, ratio2, one_minus_x2, rx, a, b);
        if (!velocities)
            continue;

        const double ap = dminus / y_minus, bp = dplus / y_plus;
        const double wf = w * sample->growth_rate;
        result[CLPT_NONLINEAR_DERIVATIVE][CLPT_R1] += 2.0 * w * ratio2 * dthird;
        add_r_triplet(result[CLPT_NONLINEAR_DERIVATIVE], w, ratio2, one_minus_x2, rx, ap, bp);
        add_r_triplet(result[CLPT_LINEAR_DERIVATIVE], wf, ratio2, one_minus_x2, rx, a, b);
        add_r_triplet(result[CLPT_MIXED_DERIVATIVE], wf, ratio2, one_minus_x2, rx, ap, bp);
    }
}

void clpt_compute_kernels(struct cmdline_data *cmd, struct global_data *gd, double k,
                          const struct clpt_radial_sample *radial, int size, int velocities,
                          double result[CLPT_KERNEL_WEIGHTS][CLPT_KERNEL_COUNT])
{
    const int lcdm = gd->model == &fkpt2_lcdm_model;
    const int weights = velocities ? CLPT_KERNEL_WEIGHTS : 1;
    memset(result, 0, (size_t)weights * sizeof(*result));
    for (int i = 0; i < size; ++i) {
        integrate_q(cmd, gd, lcdm, velocities, k, &radial[i], radial[size - 1].p, result);
        integrate_r(cmd, gd, lcdm, velocities, k, &radial[i], result);
    }

    const double measure = k * k / FOURPI2;
    const double linear = psInterpolation_nr(k, kPS, pPS, nPSLT);
    for (int w = 0; w < weights; ++w)
        for (int i = 0; i < CLPT_KERNEL_COUNT; ++i)
            result[w][i] *= i < CLPT_R1 ? measure : measure * linear;
}
