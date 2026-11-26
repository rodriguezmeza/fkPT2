/* Cold dark matter and baryons with massive neutrinos.
 * Growth follows the Hu-Eisenstein fit used by Noriega et al. (2022),
 * arXiv:2208.02791, in the FOLPS-nu 7a4cd56 reference.
 */
#include "fkpt2_addon.h"
#include "fkpt2_internal.h"
#include <strings.h>

struct neutrino_growth {
    double suppression;
    double a_eq;
    double k_free;
};

static int initialize_neutrinos(struct cmdline_data *cmd, struct global_data *gd)
{
    const char *problem = NULL;
    const double mnu = gd->model_options[0];
    const double omega_m = cmd->om * cmd->h * cmd->h;
    const double omega_nu = mnu / 93.14;
    if (!isfinite(omega_m) || omega_m <= 0.0 || mnu < 0.0 || omega_nu >= omega_m)
        problem = "FOLPSNu requires 0 <= mnu < 93.14 * Om * h^2 (mnu in eV)";
    else if (cmd->is_PS_input_LCDM != 0)
        problem = "FOLPSNu requires the linear cb spectrum at zout and is_PS_input_LCDM=0";
    else if (cmd->clpt || cmd->gsm)
        problem = "FOLPSNu supports power spectra. CLPT and GSM require nonlinear displacement kernels";
    if (problem != NULL) {
        snprintf(cmd->error_message, sizeof(cmd->error_message), "%s", problem);
        return FAILURE;
    }

    const double fnu = omega_nu / omega_m;
    struct neutrino_growth *growth = fkpt2_work_calloc(1, sizeof(*growth));
    gd->model_state = growth;
    if (fnu == 0.0)
        return SUCCESS;

    const double neff = 3.046;
    const double radiation_factor = 1.0 + (7.0/8.0) * pow(4.0/11.0, 4.0/3.0) * neff;
    /* Match the matter-radiation equality convention in fOverf0EH. */
    growth->a_eq = (2.469e-5 / radiation_factor) / omega_m;
    /* Rationalized form of (5 - sqrt(25 - 24 fnu)) / 4. */
    growth->suppression = 6.0 * fnu / (5.0 + sqrt(25.0 - 24.0 * fnu));
    /* FOLPS-nu uses three mass-degenerate species and y_FS = (k/k_free)^2. */
    growth->k_free = omega_m / 3.0 * sqrt(fnu / (17.2 * (1.0 + 0.488 * pow(fnu, -7.0/6.0))));
    return SUCCESS;
}

static real growth_rate_ratio(const struct cmdline_data *cmd, const struct global_data *gd,
                               real eta, real k)
{
    (void)cmd;
    const struct neutrino_growth *growth = gd->model_state;
    if (growth->suppression == 0.0)
        return 1.0;
    const double scale = k == 0.0 ? 0.0 : k / growth->k_free;
    const double rf = exp(eta) / (growth->a_eq * (1.0 + scale * scale));
    return 1.0 - growth->suppression / (1.0 + pow(rf, 0.7));
}

static int power_enabled(const struct cmdline_data *cmd)
{
    return strcasecmp(cmd->mgmodel, "FOLPSNu") == 0;
}

static int cb_power(struct cmdline_data *cmd, struct global_data *gd, fkpt2_result *result)
{
    static const char *const columns[] = {
        "k", "P_cb_linear", "f_cb_over_f0", "P_cb_1loop", "P_cb_theta_1loop", "P_theta_theta_1loop"
    };
    const size_t ncols = sizeof(columns) / sizeof(*columns);
    double *power = fkpt2_alloc_addon_table(result, "cb_power", (size_t)cmd->Nk, ncols, columns);
    const global_kFArrays *loops = &kFArrays;
    for (int i = 0; i < cmd->Nk; ++i) {
        double *row = power + (size_t)i * ncols;
        const double p = loops->pklT[i];
        const double f_ratio = loops->fkT[i] / gd->f0;
        row[0] = loops->kT[i];
        row[1] = p;
        row[2] = f_ratio;
        row[3] = p + (loops->P22ddT[i] + loops->P13ddT[i]);
        row[4] = p * f_ratio + (loops->P22duT[i] + loops->P13duT[i]);
        row[5] = p * f_ratio * f_ratio + (loops->P22uuT[i] + loops->P13uuT[i]);
    }
    return SUCCESS;
}

static const struct fkpt2_model_option neutrino_options[] = {{"mnu", 0.0}};

static const struct fkpt2_gravity_model neutrino_model = {
    .name = "FOLPSNu",
    .description = "Flat LCDM with massive-neutrino cb growth and one-loop fk kernels",
    .options = neutrino_options,
    .option_count = sizeof(neutrino_options) / sizeof(*neutrino_options),
    .initialize = initialize_neutrinos,
    .parallel_growth = true,
    .growth_rate_ratio = growth_rate_ratio,
    .eds_loop_kernels = true,
    .separate_velocity_dispersion = true,
};

static const struct fkpt2_table_producer neutrino_tables = {
    .enabled = power_enabled,
    .compute = cb_power,
};

const struct fkpt2_addon fkpt2_folps_nu_addon = {
    .name = "folps_nu",
    .model = &neutrino_model,
    .tables = &neutrino_tables,
};
