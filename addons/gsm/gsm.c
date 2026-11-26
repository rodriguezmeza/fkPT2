/* Gaussian streaming following Valogiannis, Bean & Aviles (2020), Eq. 3.30. */
#include "clpt/clpt.h"
#include "fkpt2_internal.h"
#include <limits.h>

enum { LOS_GAUSS = 4, GSM_COLUMNS = 4 };
static const double radial_spacing = 0.5; /* Mpc/h */

struct velocity_splines {
    int size;
    double *r;
    double **values;
    double **curvature;
};

static int gsm_enabled(const struct cmdline_data *cmd)
{
    return cmd->gsm;
}

static int gsm_validate(struct cmdline_data *cmd)
{
    const char *message = NULL;
    if (cmd->Ns < 1 || cmd->smin <= 0.0 || (cmd->Ns > 1 && cmd->smax <= cmd->smin))
        message = "GSM requires Ns >= 1, smin > 0, and smax > smin when Ns > 1";
    else if (cmd->gsm_width <= 0.0)
        message = "GSM requires gsm_width > 0";
    else if (cmd->gsm_sizeyT < LOS_GAUSS || cmd->gsm_NGL < 3 || cmd->gsm_NGL == INT_MAX)
        message = "GSM requires gsm_sizeyT >= 4 and 3 <= gsm_NGL < INT_MAX";
    if (message == NULL)
        return clpt_validate(cmd);
    snprintf(cmd->error_message, sizeof(cmd->error_message), "%s", message);
    return FAILURE;
}

static struct velocity_splines velocity_splines(const struct clpt_velocity_grid *grid)
{
    struct velocity_splines table = {
        grid->size, dvector(1, grid->size),
        dmatrix(CLPT_XI, CLPT_SIGMA_PERP, 1, grid->size),
        dmatrix(CLPT_XI, CLPT_SIGMA_PERP, 1, grid->size),
    };
    for (int i = 1; i <= table.size; ++i) {
        const double *row = grid->data + (size_t)(i - 1) * CLPT_VELOCITY_COLUMNS;
        const double r = row[CLPT_R], par = row[CLPT_SIGMA_PAR], perp = row[CLPT_SIGMA_PERP];
        if (!(par > 0.0 && perp > 0.0))
            error("GSM requires positive CLPT velocity variances at r=%g "
                  "(parallel=%g, perpendicular=%g); adjust the separation range or s2eft", r, par, perp);
        table.r[i] = r;
        table.values[CLPT_XI][i] = log1p(row[CLPT_XI]);
        table.values[CLPT_V12][i] = row[CLPT_V12];
        table.values[CLPT_SIGMA_PAR][i] = log(par);
        table.values[CLPT_SIGMA_PERP][i] = log(perp);
    }
    /* Interpolation in log space preserves positive pair density and variances. */
    for (int i = CLPT_XI; i <= CLPT_SIGMA_PERP; ++i)
        spline(table.r, table.values[i], table.size, 1e30, 1e30, table.curvature[i]);
    return table;
}

static double interpolate_moment(const struct velocity_splines *table, int column, double r)
{
    double value;
    splint(table->r, table->values[column], table->curvature[column], table->size, r, &value);
    return value;
}

static double los_variance(const struct velocity_splines *table, double r, double mu2)
{
    const double parallel = exp(interpolate_moment(table, CLPT_SIGMA_PAR, r));
    const double perpendicular = exp(interpolate_moment(table, CLPT_SIGMA_PERP, r));
    return mu2 * parallel + (1.0 - mu2) * perpendicular;
}

static double stream_correlation(const struct cmdline_data *cmd, const struct velocity_splines *table,
                                 double s, double mu, const double node[], const double weight[])
{
    const double s_par = s * mu, s_perp = s * sqrt((1.0 - mu) * (1.0 + mu));
    /* The sinh map resolves narrow Gaussians while keeping the full requested window. */
    const double scale = sqrt(los_variance(table, s, mu * mu));
    const double limit = asinh(cmd->gsm_width / scale);
    const int panels = 1 + (cmd->gsm_sizeyT - 1) / LOS_GAUSS;
    const double half_step = limit / panels;
    double density = 0.0;
    for (int i = 0; i < panels; ++i) {
        const double center = -limit + (2.0 * i + 1.0) * half_step;
        for (int j = 1; j <= LOS_GAUSS; ++j) {
            const double u = center + half_step * node[j];
            const double offset = scale * sinh(u);
            const double y = s_par + offset;
            const double r = hypot(s_perp, y);
            const double mu_r = y / r;
            const double var = los_variance(table, r, mu_r * mu_r);
            const double residual = offset + mu_r * interpolate_moment(table, CLPT_V12, r);
            const double log_pairs = interpolate_moment(table, CLPT_XI, r);
            density += weight[j] * cosh(u) * exp(log_pairs - 0.5 * residual * residual / var) / sqrt(var);
        }
    }
    return density * half_step * scale * INVSQRTDTWOPI - 1.0;
}

struct gsm_projection {
    const struct cmdline_data *cmd;
    const struct velocity_splines *table;
    const double *mu, *mu_w, *node, *weight;
    double *out;
    double smax, ds;
};

static void project_multipoles(void *state, int i)
{
    const struct gsm_projection *grid = state;
    const struct cmdline_data *cmd = grid->cmd;
    double *row = grid->out + (size_t)i * GSM_COLUMNS;
    const double s = i == cmd->Ns - 1 ? grid->smax : cmd->smin + i * grid->ds;
    row[0] = s;
    for (int j = 1; j <= cmd->gsm_NGL; ++j) {
        const double mu = grid->mu[j];
        const double xi = stream_correlation(cmd, grid->table, s, mu, grid->node, grid->weight);
        if (!isfinite(xi))
            error("GSM produced a nonfinite correlation at s=%g, mu=%g", s, mu);
        const double mu2 = mu * mu;
        const double xi_w = grid->mu_w[j] * xi;
        row[1] += xi_w;
        row[2] += 5.0 * xi_w * (3.0 * mu2 - 1.0) / 2.0;
        row[3] += 9.0 * xi_w * ((35.0 * mu2 - 30.0) * mu2 + 3.0) / 8.0;
    }
}

static int gsm_compute(struct cmdline_data *cmd, struct global_data *gd, fkpt2_result *result)
{
    const int angles = cmd->gsm_NGL;
    double *mu = dvector(1, angles), *mu_w = dvector(1, angles);
    gauleg(0.0, 1.0, mu, mu_w, angles);
    const double smax = cmd->Ns == 1 ? cmd->smin : cmd->smax;
    /* The closest point in the LOS window is at the largest angle cosine. */
    const double max_mu = mu[angles];
    const double min_perp = cmd->smin * sqrt((1.0 - max_mu) * (1.0 + max_mu));
    const double min_parallel = fmax(0.0, cmd->smin * max_mu - cmd->gsm_width);
    const double rmin = hypot(min_perp, min_parallel);
    const double rmax = smax + cmd->gsm_width;
    const double size = 1.0 + ceil((rmax - rmin) / radial_spacing);
    if (!(rmin > 0.0 && rmax > rmin && size < INT_MAX))
        error("GSM velocity grid is invalid");

    struct clpt_velocity_grid grid = {rmin, rmax, (int)size, NULL};
    if (clpt_compute(cmd, gd, result, &grid) == FAILURE)
        return FAILURE;
    const struct velocity_splines table = velocity_splines(&grid);

    static const char *const columns[GSM_COLUMNS] = {"s", "xi0", "xi2", "xi4"};
    double *out = fkpt2_alloc_addon_table(result, "gsm", (size_t)cmd->Ns, GSM_COLUMNS, columns);
    double node[LOS_GAUSS + 1], weight[LOS_GAUSS + 1];
    gauleg(-1.0, 1.0, node, weight, LOS_GAUSS);
    const double ds = cmd->Ns == 1 ? 0.0 : (smax - cmd->smin) / (cmd->Ns - 1);
    struct gsm_projection projection = {cmd, &table, mu, mu_w, node, weight, out, smax, ds};
    fkpt2_parallel_for(cmd->Ns, cmd->numthreads, project_multipoles, &projection);
    return SUCCESS;
}

static const struct fkpt2_table_producer gsm_tables = {
    .enabled = gsm_enabled,
    .validate = gsm_validate,
    .compute = gsm_compute,
};

const struct fkpt2_addon fkpt2_gsm_addon = {
    .name = "gsm",
    .tables = &gsm_tables,
};
