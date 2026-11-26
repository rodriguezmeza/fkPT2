/* Real-space CLPT following Carlson et al. (2013) and Aviles et al. (2018).
 * The Gaussian resums A_L, with loop and bias contributions expanded as in
 * Aviles et al. Eqs. 3.21-3.29. The q transforms follow Appendix C.2.
 */
#include "clpt_velocity.h"
#include "fkpt2_internal.h"
#include <limits.h>
#include <stdint.h>

enum q_column {
    Q_VALUE, X_L, Y_L, X_LOOP, Y_LOOP, V_Q, T_Q, X_10, Y_10,
    U_L, U_LOOP, U_11, U_20, Q_XI, Q_LAPLACIAN, Q_BIHARMONIC, Q_COLUMNS
};
enum tidal_column {
    TIDAL_XI2, TIDAL_XI4, TIDAL_I3, TIDAL_V10, TIDAL_I3_D, TIDAL_V10_D, TIDAL_COLUMNS
};
enum correlation_column {
    R_VALUE, XI_LINEAR, XI_ZA, XI_A, XI_W, XI10_LINEAR, XI10_LOOP,
    XI20_LINEAR, XI20_LOOP, XI01, XI02, XI11, XI_LAPLACIAN, XI_BIHARMONIC,
    XI_MATTER, XI_TRACER, CORRELATION_COLUMNS
};

static const char *const q_columns[Q_COLUMNS] = {
    "q", "X_linear", "Y_linear", "X_loop", "Y_loop", "V", "T", "X10", "Y10",
    "U_linear", "U_loop", "U11", "U20", "xi_linear", "laplacian_xi", "biharmonic_xi"
};
static const char *const correlation_columns[CORRELATION_COLUMNS] = {
    "r", "xi_linear", "xi_za", "xi_A", "xi_W", "xi10_linear", "xi10_loop",
    "xi20_linear", "xi20_loop", "xi01", "xi02", "xi11", "xi_nabla2", "xi_nabla4",
    "xi_matter", "xi_tracer"
};

enum { FOURIER_GAUSS = 8, RADIAL_GAUSS = 4, ANGULAR_GAUSS = 32, RADIAL_PANELS = 64 };
static const double tail_sigma = 12.0;

struct spline_table {
    int size;
    int columns;
    double *grid;
    double **values;
    double **curvature;
};

struct fourier_sample {
    double k, linear, loop, v, t, ri, y10, u_loop, u11, u20;
    double xi, laplacian, biharmonic, v10;
};

static double log_grid(double low, double high, int i, int size)
{
    if (i == 0)
        return low;
    if (i == size - 1)
        return high;
    return exp(log(low) + (log(high) - log(low)) * ((double)i / (size - 1)));
}

static struct spline_table new_spline_table(int size, int columns)
{
    return (struct spline_table){
        size, columns, dvector(1, size), dmatrix(0, columns - 1, 1, size),
        dmatrix(0, columns - 1, 1, size),
    };
}

static void prepare_splines(struct spline_table *table)
{
    for (int i = 0; i < table->columns; ++i)
        spline(table->grid, table->values[i], table->size, 1e30, 1e30, table->curvature[i]);
}

static double interpolate(const struct spline_table *table, int column, double x)
{
    double value;
    splint(table->grid, table->values[column], table->curvature[column], table->size, x, &value);
    return value;
}

int clpt_validate(struct cmdline_data *cmd)
{
    const char *message = NULL;
    if (cmd->Nr < 1 || cmd->rmin <= 0.0 || (cmd->Nr > 1 && cmd->rmax <= cmd->rmin))
        message = "CLPT requires Nr >= 1, rmin > 0, and rmax > rmin when Nr > 1";
    else if (cmd->clpt_kmin <= 0.0 || cmd->clpt_kmax <= cmd->clpt_kmin)
        message = "CLPT requires 0 < clpt_kmin < clpt_kmax";
    else if (cmd->clpt_pmax <= 0.0)
        message = "CLPT requires clpt_pmax > 0";
    else if (cmd->clpt_Nk < 2 || cmd->Nk_qFunctionsQuad < 2 || cmd->NqperLogDecade < 1)
        message = "CLPT requires clpt_Nk and Nk_qFunctionsQuad >= 2 and NqperLogDecade >= 1";
    if (message == NULL)
        return SUCCESS;
    snprintf(cmd->error_message, sizeof(cmd->error_message), "%s", message);
    return FAILURE;
}

struct kernel_grid {
    struct cmdline_data *cmd;
    struct global_data *gd;
    struct spline_table *table;
    const struct clpt_radial_sample *radial;
    int size, velocities;
};

static void kernel_row(void *state, int i)
{
    const struct kernel_grid *grid = state;
    struct cmdline_data *cmd = grid->cmd;
    struct global_data private_gd = *grid->gd;
    struct global_data *gd = grid->gd->model->parallel_growth ? &private_gd : grid->gd;
    struct spline_table *table = grid->table;
    const double k = log_grid(cmd->clpt_kmin, cmd->clpt_kmax, i, table->size);
    double values[CLPT_KERNEL_WEIGHTS][CLPT_KERNEL_COUNT];
    clpt_compute_kernels(cmd, gd, k, grid->radial, grid->size, grid->velocities, values);
    table->grid[i + 1] = k;
    const int weights = grid->velocities ? CLPT_KERNEL_WEIGHTS : 1;
    for (int w = 0; w < weights; ++w) {
        for (int j = 0; j < CLPT_KERNEL_COUNT; ++j) {
            if (!isfinite(values[w][j]))
                error("CLPT produced a nonfinite loop kernel at k=%g", k);
            table->values[w * CLPT_KERNEL_COUNT + j][i + 1] = values[w][j];
        }
    }
}

static struct spline_table kernel_table(struct cmdline_data *cmd, struct global_data *gd,
                                        int velocities)
{
    const int weights = velocities ? CLPT_KERNEL_WEIGHTS : 1;
    struct spline_table table = new_spline_table(cmd->clpt_Nk, weights * CLPT_KERNEL_COUNT);
    const int size = cmd->nquadSteps;
    const double pmin = kPS[1], pmax = fmin(cmd->clpt_pmax, kPS[nPSLT]);
    struct clpt_radial_sample *radial = fkpt2_work_calloc((size_t)size, sizeof(*radial));
    for (int i = 0; i < size; ++i) {
        const double p = log_grid(pmin, pmax, i, size);
        const double p_before = log_grid(pmin, pmax, i > 0 ? i - 1 : i, size);
        const double p_after = log_grid(pmin, pmax, i < size - 1 ? i + 1 : i, size);
        radial[i] = (struct clpt_radial_sample){p,
            0.5 * (p_after - p_before) * psInterpolation_nr(p, kPS, pPS, nPSLT),
            velocities ? clpt_growth_rate(p) : 0.0};
    }
    struct kernel_grid grid = {cmd, gd, &table, radial, size, velocities};
    const int threads = gd->model->parallel_growth ? cmd->numthreads : 1;
    fkpt2_parallel_for(table.size, threads, kernel_row, &grid);
    fkpt2_work_free(radial);
    prepare_splines(&table);
    return table;
}

static struct fourier_sample fourier_sample(const struct spline_table *table, double k,
                                             double weight, struct clpt_velocity_fourier *velocity)
{
    double kernels[CLPT_KERNEL_WEIGHTS][CLPT_KERNEL_COUNT];
    const int weights = velocity != NULL ? CLPT_KERNEL_WEIGHTS : 1;
    for (int w = 0; w < weights; ++w)
        for (int j = 0; j < CLPT_KERNEL_COUNT; ++j)
            kernels[w][j] = interpolate(table, w * CLPT_KERNEL_COUNT + j, k);

    const double *base = kernels[CLPT_DENSITY];
    const double q1 = base[CLPT_Q1], q2 = base[CLPT_Q2], qi = base[CLPT_QI];
    const double q5 = base[CLPT_Q5], q8 = base[CLPT_Q8];
    const double r1 = base[CLPT_R1], r2 = base[CLPT_R2], ri = base[CLPT_RI];
    const double r1plus2 = base[CLPT_R1PLUS2];
    const double w = weight / PI2;
    const double power = psInterpolation_nr(k, kPS, pPS, nPSLT);
    if (!(power > 0.0) || !isfinite(power))
        error("CLPT interpolated linear power is invalid at k=%g", k);
    const double linear = w * power;
    const double k2 = k * k;
    const double xi = 0.5 * k2 * linear;
    if (velocity != NULL)
        *velocity = clpt_velocity_fourier(k, w, linear, kernels);

    return (struct fourier_sample){
        .k = k, .linear = linear, .loop = w * (9.0 / 98.0 * q1 + 10.0 / 21.0 * r1),
        .v = -w / k * (3.0 / 35.0) * (qi - 3.0 * q2 + 2.0 * ri - 6.0 * r2),
        .t = -w / k * (9.0 / 14.0) * (qi + 2.0 * q2 + 2.0 * ri + 4.0 * r2),
        .ri = w * (3.0 / 14.0) * ri,
        .y10 = w * (3.0 / 14.0) * (ri + 2.0 * (r2 + r1plus2 + q5)),
        .u_loop = -w * k * (5.0 / 42.0) * r1, .u11 = -w * k * (3.0 / 7.0) * r1plus2,
        .u20 = -w * k * (3.0 / 14.0) * q8, .xi = xi,
        .v10 = -w * k * (3.0 / 14.0) * base[CLPT_QS],
        /* Curvature bias uses MGPT's Gaussian windows at 1 and 2 Mpc/h. */
        .laplacian = -k2 * xi * exp(-k2), .biharmonic = k2 * k2 * xi * exp(-4.0 * k2),
    };
}

static struct fourier_sample *fourier_grid(const struct cmdline_data *cmd,
                                            const struct spline_table *kernels,
                                            double qmax, size_t *size,
                                            struct clpt_velocity_fourier **velocities)
{
    const int panels = cmd->Nk_qFunctionsQuad;
    /* Resolve the fastest Bessel oscillations as well as the logarithmic grid. */
    const double max_width = 4.0 / qmax;
    const double count = panels + ceil((cmd->clpt_kmax - cmd->clpt_kmin) / max_width);
    if (!isfinite(count) || count > INT_MAX / FOURIER_GAUSS)
        error("CLPT Fourier quadrature is too large");
    const size_t capacity = (size_t)count * FOURIER_GAUSS;
    struct fourier_sample *samples = fkpt2_work_calloc(capacity, sizeof(*samples));
    if (velocities != NULL)
        *velocities = fkpt2_work_calloc(capacity, sizeof(**velocities));
    double x[FOURIER_GAUSS + 1], w[FOURIER_GAUSS + 1];
    gauleg(-1.0, 1.0, x, w, FOURIER_GAUSS);

    double low = cmd->clpt_kmin;
    size_t n = 0;
    for (int i = 1; i <= panels; ++i) {
        const double high = log_grid(cmd->clpt_kmin, cmd->clpt_kmax, i, panels + 1);
        const int subdivisions = (int)ceil((high - low) / max_width);
        if (subdivisions == 0)
            continue;
        const double width = (high - low) / subdivisions;
        for (int j = 0; j < subdivisions; ++j) {
            const double center = low + (j + 0.5) * width;
            for (int g = 1; g <= FOURIER_GAUSS; ++g) {
                samples[n] = fourier_sample(kernels, center + 0.5 * width * x[g], 0.5 * width * w[g],
                                            velocities != NULL ? *velocities + n : NULL);
                ++n;
            }
        }
        low = high;
    }
    *size = n;
    return samples;
}

static void spherical_bessel(double x, double *j0, double *j1_over_x, double *j2, double *j3,
                   double *one_third_minus_j1_over_x)
{
    const double x2 = x * x;
    if (x < 0.5) {
        const double delta = x2 * (1.0 / 30.0 - x2 * (1.0 / 840.0 -
                             x2 * (1.0 / 45360.0 - x2 / 3991680.0)));
        *j0 = 1.0 - x2 * (1.0 / 6.0 - x2 * (1.0 / 120.0 -
                              x2 * (1.0 / 5040.0 - x2 / 362880.0)));
        *j1_over_x = 1.0 / 3.0 - delta;
        *j2 = x2 * (1.0 / 15.0 - x2 * (1.0 / 210.0 - x2 * (1.0 / 7560.0 - x2 / 498960.0)));
        *j3 = x * x2 * (1.0 / 105.0 - x2 * (1.0 / 1890.0 - x2 * (1.0 / 83160.0 - x2 / 6486480.0)));
        *one_third_minus_j1_over_x = delta;
    } else {
        *j0 = sin(x) / x;
        *j1_over_x = (*j0 - cos(x)) / x2;
        *j2 = 3.0 * *j1_over_x - *j0;
        *j3 = 5.0 * *j2 / x - x * *j1_over_x;
        *one_third_minus_j1_over_x = 1.0 / 3.0 - *j1_over_x;
    }
}

static void q_row(const struct fourier_sample *samples, size_t size, double q, double row[Q_COLUMNS],
                  const struct clpt_velocity_fourier *velocities, double v[CLPT_VELOCITY_TERMS],
                  double tidal[TIDAL_COLUMNS])
{
    row[Q_VALUE] = q;
    for (size_t i = 0; i < size; ++i) {
        const struct fourier_sample *sample = &samples[i];
        const double x = sample->k * q;
        double j0, j1_over_x, j2, j3, delta;
        spherical_bessel(x, &j0, &j1_over_x, &j2, &j3, &delta);
        const double j1 = x * j1_over_x;
        row[X_L] += sample->linear * delta;
        row[Y_L] += sample->linear * j2;
        row[X_LOOP] += sample->loop * delta;
        row[Y_LOOP] += sample->loop * j2;
        row[V_Q] += sample->v * j1 - 0.2 * sample->t * j3;
        row[T_Q] += sample->t * j3;
        /* Evaluate C.43 relative to q=0 to keep A10(0)=0 with finite cuts. */
        row[X_10] += sample->ri * (j0 - 1.0) + sample->y10 * delta;
        row[Y_10] += sample->y10 * j2;
        row[U_L] -= 0.5 * sample->k * sample->linear * j1;
        row[U_LOOP] += sample->u_loop * j1;
        row[U_11] += sample->u11 * j1;
        row[U_20] += sample->u20 * j1;
        row[Q_XI] += sample->xi * j0;
        row[Q_LAPLACIAN] += sample->laplacian * j0;
        row[Q_BIHARMONIC] += sample->biharmonic * j0;
        if (velocities != NULL)
            clpt_velocity_transform(velocities + i, j0, j1, j2, j3, delta, v);
        if (tidal != NULL) {
            /* Use the series for j4 at small x to avoid cancellation. */
            const double x2 = x * x;
            const double j4 = x < 0.5
                ? x2 * x2 / 945.0 * (1.0 - x2 / 22.0 + x2 * x2 / 1144.0 -
                                     x2 * x2 * x2 / 102960.0)
                : 7.0 * j3 / x - j2;
            tidal[TIDAL_XI2] += sample->xi * j2;
            tidal[TIDAL_XI4] += sample->xi * j4;
            tidal[TIDAL_I3] += 0.5 * sample->k * sample->linear * j3;
            tidal[TIDAL_V10] += sample->v10 * j1;
            if (velocities != NULL) {
                tidal[TIDAL_I3_D] -= velocities[i].u_d * j3;
                tidal[TIDAL_V10_D] += velocities[i].v10_d * j1;
            }
        }
    }
}

/* Shear contractions from Vlah, Castorina & White (2016), Appendix D.
 * With q along z, <s_xz Psi_x> = J3 and <s_zz Psi_z> = -2 J2.
 * Wick contractions give Upsilon_perp = 4 J3^2 and Upsilon_par = 12 J2^2. */
static struct clpt_tidal_terms tidal_terms(const double shear[TIDAL_COLUMNS],
                                           double xi0, double u, const double *velocity)
{
    const double xi2 = shear[TIDAL_XI2], xi4 = shear[TIDAL_XI4];
    const double j2 = -2.0 / 15.0 * u - 0.2 * shear[TIDAL_I3];
    const double j3 = 0.2 * (u - shear[TIDAL_I3]);
    struct clpt_tidal_terms result = {
        .v10 = shear[TIDAL_V10], .v12 = 4.0 * j2 * xi2,
        .x = 4.0 * j3 * j3, .y = 12.0 * j2 * j2 - 4.0 * j3 * j3,
        .zeta = 8.0 / 315.0 * (7.0 * xi0 * xi0 + 10.0 * xi2 * xi2 + 18.0 * xi4 * xi4),
        .chi = 4.0 / 3.0 * xi2 * xi2,
    };
    if (velocity != NULL) {
        const double j2_d = -2.0 / 15.0 * velocity[U_D_L] - 0.2 * shear[TIDAL_I3_D];
        const double j3_d = 0.2 * (velocity[U_D_L] - shear[TIDAL_I3_D]);
        result.v10_d = shear[TIDAL_V10_D];
        result.v12_d = 4.0 * j2_d * xi2;
        result.x_d = 4.0 * j3 * j3_d;
        result.y_d = 12.0 * j2 * j2_d - result.x_d;
        result.x_dd = 4.0 * j3_d * j3_d;
        result.y_dd = 12.0 * j2_d * j2_d - result.x_dd;
    }
    return result;
}

static void convolve_angles(const struct cmdline_data *cmd, double r, double q,
                            const double term[Q_COLUMNS], double dq, const double x[], const double w[],
                            double row[CORRELATION_COLUMNS], const double v[CLPT_VELOCITY_TERMS],
                            const struct clpt_tidal_terms *tidal, struct clpt_velocity_moments *moments)
{
    const double longitudinal = term[X_L] + term[Y_L];
    if (!(term[X_L] > 0.0 && longitudinal > 0.0))
        error("CLPT displacement covariance is not positive definite at q=%g", q);
    const double perpendicular_inverse = 1.0 / term[X_L];
    const double longitudinal_inverse = 1.0 / longitudinal;
    const double max_distance2 = tail_sigma * tail_sigma * fmax(term[X_L], longitudinal);
    const double separation = q - r;
    const double max_t = fmin(2.0, (max_distance2 - separation * separation) / (2.0 * q * r));
    if (max_t <= 0.0)
        return;

    /* t=1-cos(theta) resolves the forward peak without subtracting from 1. */
    const double norm = q * q * dq * INVSQRTDTWOPI *
                        perpendicular_inverse * sqrt(longitudinal_inverse) * (max_t / 2.0);
    for (int i = 1; i <= ANGULAR_GAUSS; ++i) {
        const double t = (1.0 + x[i]) * (0.5 * max_t);
        const double displacement = separation + r * t;
        const double perpendicular2 = r * r * t * (2.0 - t);
        const double g = longitudinal_inverse * displacement;
        const double g2 = g * g + rsqr(perpendicular_inverse) * perpendicular2;
        const double gauss = w[i] * norm * exp(-0.5 * (g * displacement +
                                                     perpendicular_inverse * perpendicular2));
        const double trace_g = 2.0 * perpendicular_inverse + longitudinal_inverse - g2;
        const double parallel_g = longitudinal_inverse - g * g;
        const double gamma_v = 3.0 * g * (2.0 * perpendicular_inverse + 3.0 * longitudinal_inverse - g2);
        const double gamma_t = g * (3.0 * longitudinal_inverse - g * g);
        const double uu_g = term[U_L] * term[U_L] * parallel_g;

        row[XI_ZA] += gauss;
        row[XI_A] -= 0.5 * gauss * (term[X_LOOP] * trace_g + term[Y_LOOP] * parallel_g);
        row[XI_W] += gauss * (term[V_Q] * gamma_v + term[T_Q] * gamma_t) / 6.0;
        row[XI10_LINEAR] -= 2.0 * gauss * term[U_L] * g;
        row[XI10_LOOP] -= gauss * (2.0 * term[U_LOOP] * g + term[X_10] * trace_g + term[Y_10] * parallel_g);
        row[XI20_LINEAR] += gauss * term[Q_XI];
        row[XI20_LOOP] -= gauss * (uu_g + term[U_11] * g);
        row[XI01] -= gauss * (uu_g + term[U_20] * g);
        row[XI02] += 0.5 * gauss * term[Q_XI] * term[Q_XI];
        row[XI11] -= 2.0 * gauss * term[Q_XI] * term[U_L] * g;
        row[XI_LAPLACIAN] += gauss * term[Q_LAPLACIAN];
        row[XI_BIHARMONIC] += gauss * term[Q_BIHARMONIC];
        if (tidal != NULL) {
            row[XI_TRACER] += gauss * cmd->clpt_bs2 * (cmd->clpt_bs2 * tidal->zeta +
                cmd->clpt_b2 * tidal->chi - 2.0 * g * (tidal->v10 + cmd->clpt_b1 * tidal->v12) -
                tidal->x * trace_g - tidal->y * parallel_g);
        }
        if (moments != NULL) {
            const double sine = sqrt(t * (2.0 - t));
            const struct clpt_velocity_angle angle = {
                1.0 - t, sine, perpendicular_inverse, longitudinal_inverse,
                g, -perpendicular_inverse * r * sine, trace_g
            };
            clpt_velocity_convolve(cmd, term[Q_XI], term[U_L], v, tidal, &angle, gauss, moments);
        }
    }
}

static void correlation_row(const struct cmdline_data *cmd, const struct spline_table *q_table,
                            const struct fourier_sample *samples, size_t count, double r,
                            double width, const double mu[], const double mu_w[],
                            double row[CORRELATION_COLUMNS], struct clpt_velocity_moments *moments)
{
    row[R_VALUE] = r;
    double linear[Q_COLUMNS] = {0};
    if (moments == NULL) {
        q_row(samples, count, r, linear, NULL, NULL, NULL);
    } else {
        linear[X_L] = interpolate(q_table, X_L - 1, r);
        linear[Y_L] = interpolate(q_table, Y_L - 1, r);
        linear[Q_XI] = interpolate(q_table, Q_XI - 1, r);
    }
    row[XI_LINEAR] = linear[Q_XI];
    row[XI_ZA] = -1.0;

    /* Center the radial quadrature on the local displacement scale. */
    const double sigma = sqrt(linear[X_L] + linear[Y_L]);
    if (!(sigma > 0.0) || !isfinite(sigma))
        error("CLPT displacement width is invalid at r=%g", r);
    const double low = asinh(fmax(q_table->grid[1] - r, -width) / sigma);
    const double high = asinh(width / sigma);
    const double step = (high - low) / RADIAL_PANELS;
    double x[RADIAL_GAUSS + 1], w[RADIAL_GAUSS + 1];
    gauleg(-1.0, 1.0, x, w, RADIAL_GAUSS);
    const int tidal_count = cmd->clpt_bs2 != 0.0 ? TIDAL_COLUMNS : 0;
    const int velocity_offset = Q_COLUMNS - 1 + tidal_count;

    for (int i = 0; i < RADIAL_PANELS; ++i) {
        const double center = low + (i + 0.5) * step;
        for (int j = 1; j <= RADIAL_GAUSS; ++j) {
            const double u = center + 0.5 * step * x[j];
            const double q = r + sigma * sinh(u);
            double term[Q_COLUMNS];
            for (int col = 1; col < Q_COLUMNS; ++col)
                term[col] = interpolate(q_table, col - 1, q);
            double v[CLPT_VELOCITY_TERMS];
            if (moments != NULL)
                for (int col = 0; col < CLPT_VELOCITY_TERMS; ++col)
                    v[col] = interpolate(q_table, velocity_offset + col, q);
            struct clpt_tidal_terms tidal;
            if (tidal_count) {
                double shear[TIDAL_COLUMNS];
                for (int col = 0; col < TIDAL_COLUMNS; ++col)
                    shear[col] = interpolate(q_table, Q_COLUMNS - 1 + col, q);
                tidal = tidal_terms(shear, term[Q_XI], term[U_L], moments != NULL ? v : NULL);
            }
            convolve_angles(cmd, r, q, term, 0.5 * step * w[j] * sigma * cosh(u), mu, mu_w,
                            row, v, tidal_count ? &tidal : NULL, moments);
        }
    }

    const double b1 = cmd->clpt_b1, b2 = cmd->clpt_b2, b01 = cmd->clpt_b01;
    row[XI_MATTER] = row[XI_ZA] + row[XI_A] + row[XI_W] + cmd->c1eft * row[XI_LAPLACIAN];
    row[XI_TRACER] += row[XI_MATTER] + b1 * (row[XI10_LINEAR] + row[XI10_LOOP]) +
        b1 * b1 * (row[XI20_LINEAR] + row[XI20_LOOP]) + b2 * row[XI01] +
        b2 * b2 * row[XI02] + b1 * b2 * row[XI11] +
        2.0 * (1.0 + b1) * b01 * row[XI_LAPLACIAN] + b01 * b01 * row[XI_BIHARMONIC];
}

struct transform_grid {
    struct spline_table *table;
    const struct fourier_sample *samples;
    const struct clpt_velocity_fourier *velocities;
    double *data;
    size_t count;
    double qmin, qmax;
    int tidal_count;
};

static void transform_row(void *state, int i)
{
    const struct transform_grid *grid = state;
    struct spline_table *table = grid->table;
    const double q = log_grid(grid->qmin, grid->qmax, i, table->size);
    double *row = grid->data + (size_t)i * Q_COLUMNS;
    double v[CLPT_VELOCITY_TERMS] = {0};
    double tidal[TIDAL_COLUMNS] = {0};
    q_row(grid->samples, grid->count, q, row, grid->velocities, v,
          grid->tidal_count ? tidal : NULL);
    table->grid[i + 1] = q;
    for (int j = 1; j < Q_COLUMNS; ++j)
        table->values[j - 1][i + 1] = row[j];
    for (int j = 0; j < grid->tidal_count; ++j)
        table->values[Q_COLUMNS - 1 + j][i + 1] = tidal[j];
    if (grid->velocities != NULL)
        for (int j = 0; j < CLPT_VELOCITY_TERMS; ++j)
            table->values[Q_COLUMNS - 1 + grid->tidal_count + j][i + 1] = v[j];
}

struct correlation_grid {
    const struct cmdline_data *cmd;
    const struct spline_table *q_table;
    const struct fourier_sample *samples;
    const double *mu, *mu_w;
    double *data;
    size_t count;
    double width, min, max, step;
    int size;
};

static double separation_at(const struct correlation_grid *grid, int i)
{
    return i == grid->size - 1 ? grid->max : grid->min + i * grid->step;
}

static void correlation_at(void *state, int i)
{
    const struct correlation_grid *grid = state;
    correlation_row(grid->cmd, grid->q_table, grid->samples, grid->count,
                    separation_at(grid, i), grid->width, grid->mu, grid->mu_w,
                    grid->data + (size_t)i * CORRELATION_COLUMNS, NULL);
}

static void velocity_at(void *state, int i)
{
    const struct correlation_grid *grid = state;
    const double r = separation_at(grid, i);
    double density[CORRELATION_COLUMNS] = {0};
    struct clpt_velocity_moments moments = {0};
    correlation_row(grid->cmd, grid->q_table, NULL, 0, r, grid->width,
                    grid->mu, grid->mu_w, density, &moments);
    const double pair_density = 1.0 + density[XI_TRACER];
    if (!(pair_density > 0.0) || !isfinite(pair_density))
        error("GSM requires a positive CLPT pair density at r=%g", r);
    const double mean = moments.mean / pair_density;
    /* Add the isotropic dispersion counterterm in the ZA pair density. */
    const double eft = grid->cmd->s2eft * (1.0 + density[XI_ZA]) / pair_density;
    double *row = grid->data + (size_t)i * CLPT_VELOCITY_COLUMNS;
    row[CLPT_R] = r;
    row[CLPT_XI] = density[XI_TRACER];
    row[CLPT_V12] = mean;
    row[CLPT_SIGMA_PAR] = moments.parallel / pair_density - mean * mean + eft;
    row[CLPT_SIGMA_PERP] = moments.perpendicular / pair_density + eft;
    if (!isfinite(mean) || !isfinite(row[CLPT_SIGMA_PAR]) || !isfinite(row[CLPT_SIGMA_PERP]))
        error("CLPT produced nonfinite velocity moments at r=%g", r);
}

static int clpt_enabled(const struct cmdline_data *cmd)
{
    return cmd->clpt && !cmd->gsm;
}

int clpt_compute(struct cmdline_data *cmd, struct global_data *gd,
                 fkpt2_result *result, struct clpt_velocity_grid *grid)
{
    if (cmd->clpt_kmin < kPS[1] || cmd->clpt_kmax > kPS[nPSLT]) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "CLPT Fourier range [%g, %g] lies outside the prepared spectrum [%g, %g]",
                 cmd->clpt_kmin, cmd->clpt_kmax, kPS[1], kPS[nPSLT]);
        return FAILURE;
    }

    if (cmd->clpt_pmax <= kPS[1]) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "CLPT loop momentum cutoff must exceed the minimum available k=%g", kPS[1]);
        return FAILURE;
    }

    /* Use the full-spectrum displacement variance to bound the truncated
     * transform's covariance by 4 sigma_psi^2. */
    const double width = 2.0 * tail_sigma * sqrt(gd->sigma2L);
    const double rmin = grid != NULL ? fmin(cmd->rmin, grid->min) : cmd->rmin;
    const double rmax = grid != NULL ? fmax(cmd->rmax, grid->max) : cmd->rmax;
    const double qmin = fmin(1e-4, rmin / 16.0);
    const double qmax = fmax(250.0, rmax + width);
    const double q_count = 1.0 + ceil(cmd->NqperLogDecade * (log10(qmax) - log10(qmin)));
    if (!(width > 0.0 && qmin > 0.0) || !isfinite(width) || !isfinite(q_count) || q_count >= INT_MAX)
        error("CLPT displacement range or q grid is invalid");

    const int nq = (int)q_count;
    struct spline_table kernels = kernel_table(cmd, gd, grid != NULL);
    size_t count = 0;
    struct clpt_velocity_fourier *velocities = NULL;
    const struct fourier_sample *samples = fourier_grid(cmd, &kernels, qmax, &count,
                                                       grid != NULL ? &velocities : NULL);
    const int tidal_count = cmd->clpt_bs2 != 0.0 ? TIDAL_COLUMNS : 0;
    const int velocity_offset = Q_COLUMNS - 1 + tidal_count;
    struct spline_table q_table = new_spline_table(nq, velocity_offset +
                                                   (grid != NULL ? CLPT_VELOCITY_TERMS : 0));
    double *q_data = fkpt2_alloc_addon_table(result, "qfunctions", (size_t)nq, Q_COLUMNS, q_columns);
    struct transform_grid transform = {&q_table, samples, velocities, q_data, count,
                                       qmin, qmax, tidal_count};
    fkpt2_parallel_for(nq, cmd->numthreads, transform_row, &transform);
    fkpt2_work_free(velocities);
    prepare_splines(&q_table);

    double *out = fkpt2_alloc_addon_table(result, "clpt", (size_t)cmd->Nr, CORRELATION_COLUMNS,
                                          correlation_columns);
    double mu[ANGULAR_GAUSS + 1], mu_w[ANGULAR_GAUSS + 1];
    gauleg(-1.0, 1.0, mu, mu_w, ANGULAR_GAUSS);
    const double dr = cmd->Nr > 1 ? (cmd->rmax - cmd->rmin) / (cmd->Nr - 1) : 0.0;
    struct correlation_grid correlations = {
        .cmd = cmd, .q_table = &q_table, .samples = samples, .mu = mu, .mu_w = mu_w,
        .data = out, .count = count, .width = width, .min = cmd->rmin,
        .max = cmd->Nr == 1 ? cmd->rmin : cmd->rmax, .step = dr, .size = cmd->Nr,
    };
    fkpt2_parallel_for(cmd->Nr, cmd->numthreads, correlation_at, &correlations);

    if (grid == NULL)
        return SUCCESS;

    static const char *const velocity_columns[CLPT_VELOCITY_COLUMNS] = {
        "r", "xi_tracer", "v12", "sigma_parallel2", "sigma_perp2"
    };
    grid->data = fkpt2_alloc_addon_table(result, "clpt_velocity", (size_t)grid->size,
                                         CLPT_VELOCITY_COLUMNS, velocity_columns);
    const double dv = (grid->max - grid->min) / (grid->size - 1);
    struct correlation_grid moments = {
        .cmd = cmd, .q_table = &q_table, .mu = mu, .mu_w = mu_w, .data = grid->data,
        .width = width, .min = grid->min, .max = grid->max, .step = dv, .size = grid->size,
    };
    fkpt2_parallel_for(grid->size, cmd->numthreads, velocity_at, &moments);
    return SUCCESS;
}

static int compute_clpt_tables(struct cmdline_data *cmd, struct global_data *gd, fkpt2_result *result)
{
    return clpt_compute(cmd, gd, result, NULL);
}

static const struct fkpt2_table_producer clpt_tables = {
    .enabled = clpt_enabled,
    .validate = clpt_validate,
    .compute = compute_clpt_tables,
};

const struct fkpt2_addon fkpt2_clpt_addon = {
    .name = "clpt",
    .tables = &clpt_tables,
};
