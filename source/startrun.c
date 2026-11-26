/*==============================================================================
 MODULE: startrun.c                [fkPT2]
 Written by: Mario A. Rodriguez-Meza
 Starting date: 31 october 2025
 Purpose: routines to initialize the main code
 Language: C
 Use: 'StartRun(cmd, gd, head0, head1, head2, head3);'
 Major revisions:
 ==============================================================================*/
//        1          2          3          4        ^ 5          6          7

//
// We must check the order of memory allocation and deallocation!!!
// Here and in EndRun in fkpt2io.c
//

//
// lines where there is a "//B socket:" string are places to include module files
//  that can be found in addons/addons_include folder
//

#include "fkpt2_internal.h"
#include "fkpt2_addon.h"
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>

local int CheckParameters(struct cmdline_data *cmd, struct global_data *gd, int file_output, int from_file);
local int InputPSTable(struct cmdline_data *cmd, struct global_data *gd,
                       int file_output, const double *k, const double *pk, size_t size);
local int PSLTable(struct cmdline_data *cmd, struct global_data *gd, int file_output);
local void PSLTableNW(struct cmdline_data *cmd, struct global_data *gd);
local void GaussLegendrePoints(struct cmdline_data *cmd, struct global_data *gd);
local int print_make_info(struct cmdline_data *cmd, struct global_data *gd);
local int print_options(struct cmdline_data *cmd, struct global_data *gd);

#ifdef ADDONS
#include "startrun_include_00.h"
#endif

local int invalid_parameter(struct cmdline_data *cmd, const char *message)
{
    snprintf(cmd->error_message, sizeof(cmd->error_message), "%s", message);
    return FAILURE;
}

local const char *skip_space(const char *text)
{
    while (isspace((unsigned char)*text))
        ++text;
    return text;
}

local int read_step_size(struct cmdline_data *cmd, struct global_data *gd)
{
    char *end;
    const char *step = cmd->dxstr;
    errno = 0;
    double numerator = strtod(step, &end);
    if (end == step || errno == ERANGE || !isfinite(numerator))
        return invalid_parameter(cmd, "dtau must be a positive number or fraction");

    const char *tail = skip_space(end);
    double denominator = 1.0;
    if (*tail == '/') {
        const char *denom = tail + 1;
        errno = 0;
        denominator = strtod(denom, &end);
        if (end == denom || errno == ERANGE || !isfinite(denominator) || denominator == 0.0)
            return invalid_parameter(cmd, "dtau must have a finite, nonzero denominator");
        tail = skip_space(end);
    }
    gd->dx = numerator / denominator;
    if (*tail || !isfinite(gd->dx) || gd->dx <= 0.0)
        return invalid_parameter(cmd, "dtau must be a positive number or fraction");
    return SUCCESS;
}

/* A bare filename retains the historical Input/ lookup. Explicit paths are
 * relative to the working directory or absolute, including paths with spaces.
 */
local int spectrum_path(struct cmdline_data *cmd, struct global_data *gd)
{
    char *path = gd->fnamePSPath;
    size_t size = sizeof(gd->fnamePSPath);
    const char *name = cmd->fnamePS;
    struct stat st;

    if (strchr(name, '/') != NULL) {
        if (format_checked(path, size, "input spectrum", "%s", name) != 0)
            return invalid_parameter(cmd, "Input spectrum path is too long");
        return SUCCESS;
    }

    if (format_checked(path, size, "input spectrum", "Input/%s", name) != 0)
        return invalid_parameter(cmd, "Input spectrum path is too long");
    if (stat(path, &st) == 0 || (errno != ENOENT && errno != ENOTDIR))
        return SUCCESS;

    if (stat(name, &st) == 0 || (errno != ENOENT && errno != ENOTDIR)) {
        if (format_checked(path, size, "input spectrum", "%s", name) != 0)
            return invalid_parameter(cmd, "Input spectrum path is too long");
        return SUCCESS;
    }

#ifdef __FKPT2DIR__
    if (strcmp(name, "pkl_z05.dat") == 0) {
        const char *bundled = __FKPT2DIR__ "/tests/fkpt/Input/pkl_z05.dat";
        if (stat(bundled, &st) == 0 &&
            format_checked(path, size, "input spectrum", "%s", bundled) != 0)
            return invalid_parameter(cmd, "Bundled spectrum path is too long");
    }
#endif
    return SUCCESS;
}

int StartRun(struct cmdline_data *cmd, struct global_data *gd,
             string head0, string head1, string head2, string head3)
{
    double cpustart = CPUTIME;
    gd->headline0 = head0;
    gd->headline1 = head1;
    gd->headline2 = head2;
    gd->headline3 = head3;
    gd->startrun_cputime = TRUE;

    if (read_run_parameters(cmd) == FAILURE)
        return FAILURE;
    gd->cmd_allocated = TRUE;
    if (cmd->verbose > 0)
        printf("\n%s\n%s: %s\nVersion = %s\n", head0, head1, head2, cmd->version);

    int make_info = scanopt(cmd->options, "make-info");
    int list_options = scanopt(cmd->options, "print-options");
    int list_models = scanopt(cmd->options, "print-models");
    if (make_info || list_options || list_models) {
        if (make_info)
            print_make_info(cmd, gd);
        if (list_options)
            print_options(cmd, gd);
        if (list_models)
            for (size_t i = 0; fkpt2_model_name(i) != NULL; ++i)
                printf("%s  %s\n", fkpt2_model_name(i), fkpt2_model_description(i));
        if (!scanopt(cmd->options, "no-stop")) {
            gd->stopFlag = TRUE;
            return SUCCESS;
        }
    }

    if (fkpt2_prepare(cmd, gd, TRUE, NULL, NULL, 0) == FAILURE)
        return FAILURE;
    gd->gd_allocated = TRUE;
    gd->bytes_tot += sizeof(*gd) + sizeof(*cmd);
    gd->cputotalinout += CPUTIME - cpustart;
    return write_run_parameters(cmd, gd);
}

int fkpt2_validate_class_source(struct cmdline_data *cmd, const double *k)
{
    const bool embedded_class = cmd->class_parameters != NULL;
    if (embedded_class && *cmd->classIni)
        return invalid_parameter(cmd, "Choose a [class] section or classIni for the CLASS input");
    if (k != NULL || ((cmd->input_overrides & FKPT2_INPUT_FILE) && *cmd->fnamePS))
        return invalid_parameter(cmd, embedded_class
            ? "Use the [class] section by itself as the linear spectrum input"
            : "Use classIni by itself as the linear spectrum input");
    return SUCCESS;
}

int fkpt2_prepare(struct cmdline_data *cmd, struct global_data *gd, int file_output,
                  const double *k, const double *pk, size_t size)
{
    if (cmd->class_parameters != NULL || *cmd->classIni) {
        if (fkpt2_validate_class_source(cmd, k) == FAILURE ||
            fkpt2_addon_spectrum("class", cmd, &k, &pk, &size) == FAILURE)
            return FAILURE;
    }

    if (CheckParameters(cmd, gd, file_output, k == NULL) == FAILURE ||
        fkpt2_validate_addons(cmd) == FAILURE ||
        read_step_size(cmd, gd) == FAILURE ||
        (k == NULL && spectrum_path(cmd, gd) == FAILURE))
        return FAILURE;

    gd->xnow = gd->xout = gd->xoutinfo = cmd->x;
    gd->ol = 1.0 - cmd->om;

#ifdef ADDONS
#include "startrun_include_04.h"
#endif

    if (file_output && (setFilesDirs(cmd, gd) == FAILURE || setFilesDirs_log(cmd, gd) == FAILURE))
        return FAILURE;

    /* The existing numerical routines write diagnostics to an open stream. */
    const char *log_path = file_output && cmd->verbose_log > 0 ? gd->logfilePath : "/dev/null";
    gd->outlog = fopen(log_path, "w");
    if (gd->outlog == NULL) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "Cannot open log stream '%s': %s", log_path, strerror(errno));
        return FAILURE;
    }
    gd->outlogFlagFree = TRUE;
    fprintf(gd->outlog, "OmegaLambda=%g, Om=%g\n", gd->ol, cmd->om);

    GaussLegendrePoints(cmd, gd);
    quadraturemethod_string_to_int(cmd, gd, cmd->quadratureMethod, &gd->quadmethod_int);
    integration_method_string_to_int(cmd, gd, cmd->integration_method, &gd->method_int);
    if (set_model(cmd, gd) == FAILURE)
        return FAILURE;
    const int write_spectrum_previews = file_output && k == NULL;
    if (InputPSTable(cmd, gd, write_spectrum_previews, k, pk, size) == FAILURE ||
        PSLTable(cmd, gd, write_spectrum_previews) == FAILURE)
        return FAILURE;
    PSLTableNW(cmd, gd);

#ifdef ADDONS
#include "startrun_include_05.h"
#endif

    return SUCCESS;
}

local int CheckParameters(struct cmdline_data *cmd, struct global_data *gd, int file_output, int from_file)
{
    if (from_file && *cmd->fnamePS == '\0')
        return invalid_parameter(cmd, "fnamePS must specify a linear power spectrum");
    if (file_output && *cmd->rootDir == '\0')
        return invalid_parameter(cmd, "rootDir must be nonempty (use '.' for the current directory)");
    if (file_output && strchr(cmd->suffixModel, '/') != NULL)
        return invalid_parameter(cmd, "suffixModel must not contain '/'");
    if (cmd->kmin <= 0.0 || cmd->kmin >= cmd->kmax ||
        !isfinite(cmd->kmax / cmd->kmin))
        return invalid_parameter(cmd, "Require 0 < kmin < kmax with a finite ratio");
    if (cmd->Nk < 2)
        return invalid_parameter(cmd, "Nk must be at least 2");
    if (cmd->is_PS_input_LCDM != 0 && cmd->is_PS_input_LCDM != 1)
        return invalid_parameter(cmd, "is_PS_input_LCDM must be 0 or 1");
    if (cmd->om <= 0.0 || cmd->om > 1.0 || cmd->h <= 0.0)
        return invalid_parameter(cmd, "Require 0 < Om <= 1 and h > 0");
    if (cmd->zout <= -1.0)
        return invalid_parameter(cmd, "zout must be greater than -1");
    gd->xstop = rlog(1.0 / (1.0 + cmd->zout));
    if (cmd->x >= gd->xstop || cmd->x >= 0.0)
        return invalid_parameter(cmd, "tauini must precede both today and the output time");
    if (cmd->dxmin < 0.0 || cmd->eps <= 0.0 || cmd->eps > 1.0e-4 || cmd->maxnsteps < 1)
        return invalid_parameter(cmd, "Require dtaumin >= 0, 0 < eps <= 1e-4 and maxnsteps >= 1");
    if (strcmp(cmd->integration_method, "rkqs") != 0 &&
        strcmp(cmd->integration_method, "bsstep") != 0)
        return invalid_parameter(cmd, "integration_method must be rkqs or bsstep");
    if (strcmp(cmd->quadratureMethod, "trapezoid3") != 0)
        return invalid_parameter(cmd, "quadratureMethod must be trapezoid3");
    if (cmd->nquadSteps < 3 || cmd->ngausslegpoints < 2 ||
        cmd->epsquad <= 0.0 || cmd->epsquad >= 0.1)
        return invalid_parameter(cmd, "Require nquadSteps >= 3, ngausslegpoints >= 2 and 0 < epsquad < 0.1");
    if (cmd->numthreads < 1 || cmd->verbose < 0 || cmd->verbose_log < 0)
        return invalid_parameter(cmd, "Require numberThreads >= 1 and nonnegative verbosity");
    if (cmd->rmin < 0.0 || cmd->rmax < cmd->rmin || cmd->Nr < 0)
        return invalid_parameter(cmd, "Require 0 <= rmin <= rmax and Nr >= 0");
    return SUCCESS;
}

/* The endpoint extrapolation fits ten samples at each end of the table. */
#define NPT 10
#define SPREAD 1.0

local int read_input_spectrum(struct cmdline_data *cmd, const char *path,
                              pointPSTable **table, int *count)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "Cannot open linear spectrum '%s': %s", path, strerror(errno));
        return FAILURE;
    }

    char *line = NULL;
    size_t line_size = 0;
    unsigned long line_number = 0;
    const char *problem = NULL;
    pointPSTable *rows = NULL;
    int size = 0;
    int capacity = 0;
    while (getline(&line, &line_size, file) != -1) {
        ++line_number;
        const char *start = skip_space(line);
        if (*start == '\0' || *start == '#' || *start == '%')
            continue;

        char *end;
        errno = 0;
        real k = strtod(start, &end);
        if (start == end || !isspace((unsigned char)*end) ||
            errno == ERANGE || !isfinite(k) || k <= 0.0 ||
            (size > 0 && rlog10(k) <= rlog10(rows[size - 1].k))) {
            problem = "expected finite, positive, strictly increasing wavenumbers";
            break;
        }
        start = end;
        errno = 0;
        real power = strtod(start, &end);
        if (start == end || (*end && *end != '#' && *end != '%' && !isspace((unsigned char)*end)) ||
            errno == ERANGE || !isfinite(power) || power <= 0.0) {
            problem = "expected finite, positive power in the second column";
            break;
        }

        if (size == capacity) {
            if (capacity > INT_MAX / 2) {
                problem = "too many samples";
                break;
            }
            capacity = capacity ? 2 * capacity : 256;
            pointPSTable *new_rows = realloc(rows, (size_t)capacity * sizeof(*rows));
            if (new_rows == NULL) {
                problem = "out of memory";
                break;
            }
            rows = new_rows;
        }
        rows[size++] = (pointPSTable){k, power};
    }

    if (problem == NULL && !feof(file))
        problem = "error reading spectrum";
    if (problem == NULL && size < NPT)
        problem = "expected at least 10 spectrum samples";
    free(line);
    fclose(file);
    if (problem != NULL) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "Linear spectrum '%s', line %lu: %s", path, line_number, problem);
        free(rows);
        return FAILURE;
    }
    *table = rows;
    *count = size;
    return SUCCESS;
}

local int write_extended_spectrum(struct cmdline_data *cmd, struct global_data *gd,
                                   const char *name)
{
    const char *routineName = "write_extended_spectrum";
    const char *base = strrchr(gd->fnamePSPath, '/');
    base = base ? base + 1 : gd->fnamePSPath;
    char path[BUFFERSIZE];

    if (format_checked(path, sizeof(path), "extended spectrum path", "%s/%s%s_%s",
                       gd->tmpDir, base, cmd->suffixModel, name) != 0)
        return invalid_parameter(cmd, "Extended spectrum output path is too long");

    FILE *file = NULL;
    OPEN_OUTPUT_OR_FAIL(file, path, "w!");
    for (int i = 0; i < nPSTable; ++i)
        fprintf(file, "%g %g\n", PSLCDMtab[i].k, PSLCDMtab[i].ps);
    CLOSE_OUTPUT_OR_FAIL(file, path);
    return SUCCESS;
}

local int InputPSTable(struct cmdline_data *cmd, struct global_data *gd,
                       int file_output, const double *k, const double *pk, size_t size)
{
    pointPSTableptr p, plog, pn;
    pointPSTableptr PSLCDMtabtmp;
    int nPSTabletmp;
    int i;
    real dk, kval, PSval, kmin, kmax;
    int mwt;
    double al,bl,chi2,q,siga,sigb,*x,*y,*sig;
    double au, bu;
    real *kPStmp;
    real *pPStmp;
    real kminext, kmaxext, dktmp, kmn, kmx;
    const int Nkext = 800;
    real kminT=1.0e-5, kmaxT=400.0;

    if (k == NULL) {
        fprintf(gd->outlog, "\nReading power spectrum from %s...\n", gd->fnamePSPath);
        if (read_input_spectrum(cmd, gd->fnamePSPath, &PSLCDMtabtmp, &nPSTabletmp) == FAILURE)
            return FAILURE;
        fkpt2_work_own(PSLCDMtabtmp);
    } else {
        if (size < NPT || size > INT_MAX - Nkext)
            return invalid_parameter(cmd, "Expected at least 10 spectrum samples and at most INT_MAX - 800");
        nPSTabletmp = (int)size;
        PSLCDMtabtmp = fkpt2_work_calloc(size, sizeof(*PSLCDMtabtmp));
        real previous_log_k = -INFINITY;
        for (size_t j = 0; j < size; ++j) {
            real kj = k[j], pj = pk[j];
            if (!isfinite(kj) || !isfinite(pj) || kj <= 0 || pj <= 0)
                return invalid_parameter(cmd, "Spectrum k and P(k) must be finite and positive");
            real log_k = rlog10(kj);
            if (log_k <= previous_log_k)
                return invalid_parameter(cmd, "Spectrum wavenumbers must be strictly increasing");
            previous_log_k = log_k;
            PSLCDMtabtmp[j] = (pointPSTable){kj, pj};
        }
    }

    fprintf(gd->outlog, "\nCreating log power spectrum...\n");
    PSLCDMLogtab = fkpt2_work_calloc((size_t)nPSTabletmp, sizeof(*PSLCDMLogtab));
    plog = PSLCDMLogtab;
    nPSLogT = 0;
    for (p = PSLCDMtabtmp; p < PSLCDMtabtmp + nPSTabletmp; ++p) {
        kPos(plog) = rlog10(kPos(p));
        PS(plog) = rlog10(PS(p));
        ++plog;
        ++nPSLogT;
    }

    fprintf(gd->outlog,"\nTotal numbers in Log PS: %d %ld\n",nPSLogT,plog-PSLCDMLogtab);
    fprintf(gd->outlog,"Total numbers in Normal PS: %d %ld\n\n",nPSTabletmp,p-PSLCDMtabtmp);

    fprintf(gd->outlog,"\n\nLinear fit (a + b x) to log-log power spectrum at minset and maxset...\n");
    fprintf(gd->outlog,"\n\nLinear fit to log-log power spectrum at minset and maxset...\n");

    x=dvector(1,NPT);
    y=dvector(1,NPT);
    sig=dvector(1,NPT);
    
// Low-ks of the PS
    fprintf(gd->outlog,"\nAt low-ks of the spectrum...\n");

    plog = PSLCDMLogtab;
    for (i=1;i<=NPT;i++) {
        x[i]=kPos(plog);
        y[i]=PS(plog);
        sig[i]=SPREAD;
        plog++;
    }
    for (mwt=0;mwt<=1;mwt++) {
        fit(x,y,NPT,sig,mwt,&al,&bl,&siga,&sigb,&chi2,&q);
        if (mwt == 0)
            fprintf(gd->outlog,"\nIgnoring standard deviations\n");
        else
            fprintf(gd->outlog,"\nIncluding standard deviations\n");
        fprintf(gd->outlog,"%12s %9.6f %18s %9.6f \n",
               "a  =  ",al,"uncertainty:",siga);
        fprintf(gd->outlog,"%12s %9.6f %18s %9.6f \n",
               "b  =  ",bl,"uncertainty:",sigb);
        fprintf(gd->outlog,"%19s %14.6f \n","chi-squared: ",chi2);
        fprintf(gd->outlog,"%23s %10.6f \n","goodness-of-fit: ",q);
    }

// High-ks of the PS
    fprintf(gd->outlog,"\nAt high-ks of the spectrum...\n");

    plog = PSLCDMLogtab+nPSLogT-1;
    for (i=1;i<=NPT;i++) {
        x[i]=kPos(plog);
        y[i]=PS(plog);
        sig[i]=SPREAD;
        plog--;
    }
    for (mwt=0;mwt<=1;mwt++) {
        fit(x,y,NPT,sig,mwt,&au,&bu,&siga,&sigb,&chi2,&q);
        if (mwt == 0)
            fprintf(gd->outlog,"\nIgnoring standard deviations\n");
        else
            fprintf(gd->outlog,"\nIncluding standard deviations\n");
        fprintf(gd->outlog,"%12s %9.6f %18s %9.6f \n",
               "a  =  ",au,"uncertainty:",siga);
        fprintf(gd->outlog,"%12s %9.6f %18s %9.6f \n",
               "b  =  ",bu,"uncertainty:",sigb);
        fprintf(gd->outlog,"%19s %14.6f \n","chi-squared: ",chi2);
        fprintf(gd->outlog,"%23s %10.6f \n","goodness-of-fit: ",q);
    }

// Extending power spectrum
    kPStmp = dvector(1,nPSLogT);
    pPStmp = dvector(1,nPSLogT);
    pPS2 = dvector(1,nPSLogT);
    pn = PSLCDMLogtab;
    i=1;
    for (pn = PSLCDMLogtab; pn<PSLCDMLogtab+nPSLogT; pn++) {
        kPStmp[i] = kPos(pn);
        pPStmp[i] = PS(pn);
        i++;
    }
//
    spline(kPStmp,pPStmp,nPSLogT,1.0e30,1.0e30,pPS2);

    kmin = kPos(PSLCDMtabtmp);
    kmax = kPos(PSLCDMtabtmp+nPSTabletmp-1);
    fprintf(gd->outlog,"\nkmin, kmax of the given power spectrum (%d values): %g, %g",nPSTabletmp, kmin, kmax);
    dktmp = (rlog10(kmax) - rlog10(kmin))/((real)(nPSTabletmp - 1));
    /* Use truncation without converting the log-grid offsets to int. */
    real NkL = trunc(rlog10(kmin/kminT) / dktmp);
    real NkU = trunc(rlog10(kmaxT/kmax) / dktmp);
    fprintf(gd->outlog, "\n\nNkL, NkU targets: %g %g\n", NkL, NkU);
    kminext = rpow(10.0, rlog10(kmin) - NkL*dktmp);
    kmaxext = rpow(10.0, rlog10(kmax) + NkU*dktmp);

    fprintf(gd->outlog,"\nkmin, kmax of the extended power spectrum (first try): %g %g",
            kminext, kmaxext);

    kmn = MIN(kminext,cmd->kmin);
    kmx = MAX(kmaxext,cmd->kmax);
    fprintf(gd->outlog,"\nkmin, kmax of the extended power spectrum (second try): %g %g\n",
            kmn, kmx);

    nPSTable = Nkext;
    fprintf(gd->outlog,"\n\nCreating new PSTable with %d values\n",nPSTable);
    PSLCDMtab = fkpt2_work_calloc((size_t)nPSTable, sizeof(*PSLCDMtab));
    dk = (rlog10(kmx) - rlog10(kmn))/((real)(nPSTable - 1));
    p = PSLCDMtab;

    /* Round trips through log10 and pow can move a tabulated endpoint
     * across the spline boundary by a few ulps. Compare in log space. */
    const double log_min = kPStmp[1];
    const double log_max = kPStmp[nPSLogT];
    const double lower = log_min - 4.0 * DBL_EPSILON * fmax(1.0, fabs(log_min));
    const double upper = log_max + 4.0 * DBL_EPSILON * fmax(1.0, fabs(log_max));
    for (i=1; i<=nPSTable; i++) {
        kval = rlog10(kmn) + dk*((real)(i - 1));
        if (kval < lower)
            PSval = al + bl*kval;
        else if (kval > upper)
            PSval = au + bu*kval;
        else
            PSval = psInterpolation_nr(fmin(fmax(kval, log_min), log_max), kPStmp, pPStmp, nPSLogT);
        kPos(p) = rpow(10.0,kval);
        PS(p) = rpow(10.0,PSval);
        p++;
    }

    free_dvector(pPS2,1,nPSLogT);
    pPS2 = NULL;
    fkpt2_work_free(PSLCDMtabtmp);
    fkpt2_work_free(PSLCDMLogtab);
    PSLCDMLogtab = NULL;
    free_dvector(pPStmp,1,nPSLogT);
    free_dvector(kPStmp,1,nPSLogT);

    free_dvector(sig,1,NPT);
    free_dvector(y,1,NPT);
    free_dvector(x,1,NPT);

    if (cmd->nquadSteps > nPSTable)
        return invalid_parameter(cmd, "nquadSteps exceeds the size of the extended spectrum");
    for (i = 0; i < nPSTable; ++i) {
        if (!isfinite(PSLCDMtab[i].k) || !isfinite(PSLCDMtab[i].ps) ||
            PSLCDMtab[i].k <= 0.0 || PSLCDMtab[i].ps <= 0.0 ||
            (i > 0 && PSLCDMtab[i].k <= PSLCDMtab[i - 1].k))
            return invalid_parameter(cmd, "Spectrum extrapolation produced an invalid table");
    }
    return file_output ? write_extended_spectrum(cmd, gd, "ext.dat") : SUCCESS;
}
#undef NPT
#undef SPREAD




struct linear_growth_run {
    struct cmdline_data *cmd;
    struct global_data *gd;
    real Dpkmin;
};

static void rescale_spectrum_row(void *state, int i)
{
    struct linear_growth_run *run = state;
    struct global_data private_gd = *run->gd;
    struct global_data *gd = run->gd->model->parallel_growth ? &private_gd : run->gd;
    const real Dpk = DpFunction(run->cmd, gd, PSLT[i].k);
    PSLT[i].ps *= rsqr(Dpk / run->Dpkmin);
}

static void linear_growth_row(void *state, int i)
{
    struct linear_growth_run *run = state;
    struct global_data private_gd = *run->gd;
    struct global_data *gd = run->gd->model->parallel_growth ? &private_gd : run->gd;
    const real k = PSLT[i].k;
    kPS[i + 1] = k;
    pPS[i + 1] = PSLT[i].ps;
    fkT[i + 1] = f_growth(run->cmd, gd, k);
    DplusT[i + 1] = DpFunction(run->cmd, gd, k);
}

local int PSLTable(struct cmdline_data *cmd, struct global_data *gd, int file_output)
{
    real xstop = gd->xstop;
    gd->xstop = 0.0;
    real Dp0 = DpFunction_LCDM(cmd, gd, 0.0);
    gd->xstop = xstop;
    real Dpzout = DpFunction(cmd, gd, 0.0);
    gd->Dplus = Dpzout / Dp0;
    fprintf(gd->outlog, "Dp(0)=%g, Dp(%g)=%g\n", Dp0, cmd->zout, Dpzout);

    if (file_output && write_extended_spectrum(cmd, gd, "ext2.dat") == FAILURE)
        return FAILURE;

    nPSLT = nPSTable;
    PSLT = allocate(nPSLT * sizeof(*PSLT));
    memcpy(PSLT, PSLCDMtab, nPSLT * sizeof(*PSLT));
    const int threads = gd->model->parallel_growth ? cmd->numthreads : 1;
    struct linear_growth_run run = {cmd, gd, 0.0};
    if (cmd->is_PS_input_LCDM) {
        run.Dpkmin = DpFunction(cmd, gd, PSLCDMtab[0].k);
        fprintf(gd->outlog, "Rescaling the LCDM input spectrum, Dpkmin=%g\n", run.Dpkmin);
        fkpt2_parallel_for(nPSLT, threads, rescale_spectrum_row, &run);
    }

    kPS = dvector(1, nPSLT);
    pPS = dvector(1, nPSLT);
    pPS2 = dvector(1, nPSLT);
    fkT = dvector(1, nPSLT);
    fkT2 = dvector(1, nPSLT);
    DplusT = dvector(1, nPSLT);

    if (gd->model->growth_rate_ratio != NULL)
        gd->f0 = f_growth_LCDM(cmd, gd);
    fkpt2_parallel_for(nPSLT, threads, linear_growth_row, &run);
    for (int i = 1; i <= nPSLT; ++i) {
        if (!isfinite(pPS[i]) || pPS[i] <= 0.0 ||
            !isfinite(fkT[i]) || !isfinite(DplusT[i]))
            return invalid_parameter(cmd, "Growth solver produced an invalid spectrum");
    }
    if (gd->model->growth_rate_ratio == NULL)
        gd->f0 = fkT[1];
    if (gd->f0 <= 0.0)
        return invalid_parameter(cmd, "Growth solver produced a nonpositive large-scale growth rate");

    spline(kPS, pPS, nPSLT, 1.0e30, 1.0e30, pPS2);
    spline(kPS, fkT, nPSLT, 1.0e30, 1.0e30, fkT2);
    return SUCCESS;
}

real Interpolation_local(real k, double kPS[], double pPS[], int nPS, double pPS2[]);



local void PSLTableNW(struct cmdline_data* cmd, struct global_data* gd)
{
    char namebuf[256];
    stream outstr;
    double ksmin;
    double ksmax;
    double dk;
    int Nks;
    int cutmin;
    int cutmax;
    int i;
    double *kPSNW;
    double *pPSNW;
    double *pPSNW2;
    
    double *kPSNWtmp;
    double *pPSNWtmp;
    double *pPSNWtmp2;

    double *logkpkT;
    double *logkpkTeven;
    double *logkpkTodd;

    double *mlogkpkTevencutted;
    double *logkpkTevencutted;
    double *logkpkTevencutted2;

    double *mlogkpkToddcutted;
    double *logkpkToddcutted;
    double *logkpkToddcutted2;
    
    double *preT;

    double *logkpkTsave;
    double *logkpkTsave2;
    double *logkpkTOut;
    double *logkpkTIm;
    double *logkpkTImOut;
    double PSL;
    int FSTtype;
    int FSTsizestep;
//
    int j;
    int Nksevencutted;
    int Nksoddcutted;

    double outkmin, outkmax, doutk;
    int outsize;
    double kval;
    double ki;
    double kPSNW8;
    double PNWval;
    double PNW;
    double PSLkval;
    double DeltaAppf;
    double fac1;
    real aTime;
    aTime = second();

    outkmin = 0.00001;
    outkmax = 10;
    outsize = nPSLT; //800;

    ksmin = 7.0e-5 / cmd->h;
    ksmax = 7.0 / cmd->h;
    Nks = 65536;            // 2^16 
    cutmin = 120;
    cutmax = 240;

    if (Nks==1)
        dk = 0.;
    else
        dk = (ksmax - ksmin)/((double)(Nks - 1));

    fprintf(gd->outlog," -> PSLTableNW: dk value: %g\n",dk);

    logkpkT = dvector(1,Nks);

    logkpkTsave = dvector(1,Nks);
    logkpkTsave2 = dvector(1,Nks);
    logkpkTeven = dvector(1,Nks/2);
    logkpkTodd = dvector(1,Nks/2);
    
    Nksevencutted = Nks/2 - (cutmax-cutmin)-4;
    Nksoddcutted = Nks/2 - (cutmax-cutmin)-1;

    fprintf(gd->outlog," -> PSTableNW : numbers discarded %d %d\n", Nksevencutted, Nksoddcutted);
    
    mlogkpkTevencutted = dvector(1,Nksevencutted);
    logkpkTevencutted = dvector(1,Nksevencutted);
    logkpkTevencutted2 = dvector(1,Nksevencutted);

    mlogkpkToddcutted = dvector(1,Nksoddcutted);
    logkpkToddcutted = dvector(1,Nksoddcutted);
    logkpkToddcutted2 = dvector(1,Nksoddcutted);
    
    preT = dvector(1,Nks);

    kPSNWtmp = dvector(1,Nks);
    pPSNWtmp = dvector(1,Nks);
    pPSNWtmp2 = dvector(1,Nks);

    kPSNW = dvector(1,outsize);
    pPSNW = dvector(1,outsize);
    pPSNW2 = dvector(1,outsize);
        

    for (i=1; i<=Nks; i++) {
        kval = ksmin + (double)(i-1)*dk;
        PSL = Interpolation_local(kval, kPS, pPS, nPSLT, pPS2);
        logkpkT[i] = rlog10(kval * PSL);
        logkpkTsave[i] = logkpkT[i];
    }

    dsinft(logkpkT, Nks);
        for (j=1; j<=Nks/2; j++) {
            logkpkTeven[j] = logkpkT[2*j];
            logkpkTodd[j] = logkpkT[2*j-1];
        }

    i=1;
    for (j=2; j<=cutmin-2; j++) {
        mlogkpkTevencutted[i] = (double)j;
        logkpkTevencutted[i] = logkpkTeven[j];
        ++i;
    }
    for (j=cutmax+2; j<=Nks/2; j++) {
        mlogkpkTevencutted[i] = (double)j;
        logkpkTevencutted[i] = logkpkTeven[j];
        ++i;
    }
    fprintf(gd->outlog," -> PSTableNW : even cutted %d\n", i-1);

// El primero es cero:
    i=1;
    for (j=1; j<=cutmin-1; j++) {
        mlogkpkToddcutted[j] = (double)j;
        logkpkToddcutted[j] = logkpkTodd[j];
        ++i;
    }
    for (j=cutmax+1; j<=Nks/2; j++) {
        mlogkpkToddcutted[i] = (double)j;
        logkpkToddcutted[i] = logkpkTodd[j];
        ++i;
    }
    fprintf(gd->outlog," -> PSTableNW : odd cutted %d\n", i-1);


    spline(mlogkpkTevencutted,logkpkTevencutted,Nksevencutted,1.0e30,1.0e30,logkpkTevencutted2);
    spline(mlogkpkToddcutted,logkpkToddcutted,Nksoddcutted,1.0e30,1.0e30,logkpkToddcutted2);

    double yy, dyy;
    unsigned long jj;

    for (i=1; i<=Nks/2; i++) {
        if(cutmin < i && i < cutmax) {
            locate(mlogkpkTevencutted, Nksevencutted, (double)(i+1), &jj);
            polint(&mlogkpkTevencutted[jj-2],&logkpkTevencutted[jj-2],5,(double)(i+1),&yy,&dyy);
            preT[2*i] = yy;
            locate(mlogkpkToddcutted, Nksoddcutted, (double)(i), &jj);
            polint(&mlogkpkToddcutted[jj-2],&logkpkToddcutted[jj-2],5,(double)(i),&yy,&dyy);
            preT[2*i-1] = yy;
        } else{
            preT[2*i] = logkpkT[2*i];
            preT[2*i-1] = logkpkT[2*i-1];
        }
    }

    dsinft(preT, Nks);

    fac1 = (2.0/(double) Nks);
    for (i=1; i<=Nks; i++) {
        preT[i] *= fac1;
    }

    for (i=1; i<=Nks; i++) {
        kPSNWtmp[i] = ksmin + (double)(i-1)*dk;
        pPSNWtmp[i] = rpow(10.0,preT[i])/kPSNWtmp[i];
    }
    spline(kPSNWtmp,pPSNWtmp,Nks,1.0e30,1.0e30,pPSNWtmp2);

    doutk = (rlog10(outkmax) - rlog10(outkmin))/((double)(outsize - 1));

    kPSNW8 = kPSNWtmp[8];
    
    pPS_nw = dvector(1,nPSLT);   //nuevo
    pPS2_nw = dvector(1,nPSLT); //nuevo
    
    
    for (i=1; i<=outsize; i++) {
		
        ki= kPS[i];

		if (ki <= 0.001) {	
			PSL       = Interpolation_local(kPSNW8, kPS, pPS, nPSLT, pPS2);
			PNWval    = Interpolation_local(kPSNW8, kPSNWtmp, pPSNWtmp, Nks, pPSNWtmp2);
			PSLkval   = Interpolation_local(ki, kPS, pPS, nPSLT, pPS2);
            DeltaAppf = (kPS[i]*(PSL - PNWval) / PNWval) /kPSNW8;
            pPS_nw[i] = PSLkval/(DeltaAppf + 1);
		} else if (ki > 0.001 && ki < ksmax){
                pPS_nw[i] = Interpolation_local(ki, kPSNWtmp, pPSNWtmp, Nks, pPSNWtmp2);
        } else {     
                pPS_nw[i] = Interpolation_local(ki, kPS, pPS, nPSLT, pPS2);
        }


    }

    spline(kPS, pPS_nw, nPSLT, 1.0e30, 1.0e30, pPS2_nw);

    free_dvector(pPSNW2,1,outsize);
    free_dvector(pPSNW ,1,outsize);
    free_dvector(kPSNW ,1,outsize);

    free_dvector(pPSNWtmp2,1,Nks);
    free_dvector(pPSNWtmp,1,Nks);
    free_dvector(kPSNWtmp,1,Nks);

    free_dvector(preT,1,Nks);

    free_dvector(logkpkToddcutted2,1,Nksoddcutted);
    free_dvector(logkpkToddcutted,1,Nksoddcutted);
    free_dvector(mlogkpkToddcutted,1,Nksoddcutted);

    free_dvector(logkpkTevencutted2,1,Nksevencutted);
    free_dvector(logkpkTevencutted,1,Nksevencutted);
    free_dvector(mlogkpkTevencutted,1,Nksevencutted);

    free_dvector(logkpkTodd,1,Nks/2);
    free_dvector(logkpkTeven,1,Nks/2);
    free_dvector(logkpkTsave2,1,Nks);
    free_dvector(logkpkTsave,1,Nks);

    free_dvector(logkpkT,1,Nks);

    fprintf(gd->outlog," -> PSLTableNW CPU time %g...\n",second()-aTime);
}

real Interpolation_local(real k, double kPS[], double pPS[], int nPS, double pPS2[])
{
    real psftmp;
    
//    if ( k < kPS[1] || k > kPS[nPS] )
//        fprintf(stdout,"\n\nInterpolation_nr_local: warning! :: k is out of range... %g %g %g\n",k, kPS[1], kPS[nPS]);

    splint(kPS,pPS,pPS2,nPS,k,&psftmp);

    return (psftmp);
}

local void GaussLegendrePoints(struct cmdline_data* cmd,
                               struct global_data* gd)
{
    double dx,ss,xval;
    real x1=-1.0, x2=1.0;
    
    int i;
    double xx=0.0;
    
    pGL = (global_GL_ptr) allocate(sizeof(global_GL));

    nGL(pGL) = cmd->ngausslegpoints;
    xGL(pGL)=dvector(1,cmd->ngausslegpoints);
    wGL(pGL)=dvector(1,cmd->ngausslegpoints);
    x1GL(pGL) = -1.0;
    x2GL(pGL) = 1.0;

    fprintf(gd->outlog,"\nComputation of GLs...\n");
    gauleg(x1GL(pGL),x2GL(pGL),xGL(pGL),wGL(pGL),nGL(pGL));

    fprintf(gd->outlog,"\nEnd of Gauss-Legendre computing (StartRun)\n");

}

global  real psInterpolation_nr(real k, double kPS[], double pPS[], int nPS)
{
    real psftmp;

    splint(kPS,pPS,pPS2,nPS,k,&psftmp);
    
    return (psftmp);
}


//B glogal.c
void global_variables(struct cmdline_data* cmd, struct global_data* gd)
{
    golists.rmin=1.0;
    golists.rmax= cmd->smax + 40.;
    golists.Nr =(int)( golists.rmax - golists.rmin + 1.0);
    
    golists.qmin = 0.001;
    golists.qmax = golists.rmax + 50.;
    golists.Nq = (int)floor(log10(golists.qmax/golists.qmin)
                            *(real)cmd->NqperLogDecade);
    
    golists.kmin = cmd->kmin;
    golists.kmax = cmd->kmax;
    golists.Nk = cmd->Nk;
    
    golists.smin = cmd->smin;
    golists.smax = cmd->smax;
    golists.Ns = cmd->Ns;

    kFArrays.kT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.P22ddT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.P22duT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.P22uuT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
        // A
    kFArrays.I1udd1AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I2uud1AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I2uud2AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I3uuu2AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I3uuu3AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    //  B plus C
    kFArrays.I2uudd1BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I2uudd2BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I3uuud2BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I3uuud3BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I4uuuu2BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I4uuuu3BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.I4uuuu4BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    //  Bias
    kFArrays.Pb1b2T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.Pb1bs2T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.Pb22T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.Pb2s2T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.Ps22T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.Pb2thetaT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.Pbs2thetaT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    //
    kFArrays.P13ddT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.P13duT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.P13uuT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.sigma32PSLT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.pklT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays.fkT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));

    // A
    //  B plus C
    //  Bias
    //

    kFArrays_nw.kT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.P22ddT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.P22duT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.P22uuT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    // A
    kFArrays_nw.I1udd1AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I2uud1AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I2uud2AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I3uuu2AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I3uuu3AT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    //  B plus C
    kFArrays_nw.I2uudd1BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I2uudd2BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I3uuud2BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I3uuud3BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I4uuuu2BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I4uuuu3BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.I4uuuu4BpCT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    //  Bias
    kFArrays_nw.Pb1b2T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.Pb1bs2T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.Pb22T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.Pb2s2T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.Ps22T = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.Pb2thetaT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.Pbs2thetaT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    //
    kFArrays_nw.P13ddT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.P13duT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.P13uuT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.sigma32PSLT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.pklT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    kFArrays_nw.fkT = fkpt2_work_calloc((size_t)golists.Nk, sizeof(real));
    
    // A
    //  B plus C
    //  Bias
    //
}

//E glogal.c


//B
global int startrun_memoryAllocation(struct  cmdline_data *cmd,
                                     struct  global_data* gd)
{
    string routineName = "startrun_memoryAllocation";
    // Free allocated memory in reverse order as were allocated
    //  First is allocated above gsl structure gd->r

    INTEGER bytes_tot_local=0;
    //B PXD functions
#ifdef PXD
    /*
    gd->vecPXD = dvector(1,cmd->sizeHistN);
    //B offset at 0 in order to work with Cython
    gd->matPXD = dmatrix(0,cmd->sizeHistN-1,0,cmd->sizeHistN-1);
    //E
    bytes_tot_local += cmd->sizeHistN*sizeof(real);
    bytes_tot_local += cmd->sizeHistN*cmd->sizeHistN*sizeof(real);
    gd->rBins = dvector(1,cmd->sizeHistN);
    gd->histZetaMFlatten = dvector(1,cmd->sizeHistN*cmd->sizeHistN);
    */
#endif
    //E PXD functions

//    gd->histNN = dvector(1,cmd->sizeHistN);
//    gd->histCF = dvector(1,cmd->sizeHistN);
//    gd->histNNSub = dvector(1,cmd->sizeHistN);

//    bytes_tot_local += 8*cmd->sizeHistN*sizeof(real);
//    bytes_tot_local += cmd->sizeHistN*cmd->sizeHistN*sizeof(real);


//B socket:
#ifdef ADDONS
    // this is empty and can be remove these 3 lines
#include "startrun_include_10.h"                    // should be sync with
                                                //  "cballsio_include_10.h"
#endif
//E

    gd->bytes_tot += bytes_tot_local;
    verb_print_normal_info(cmd->verbose, cmd->verbose_log, gd->outlog,
                           "\n%s: Allocated %g MByte for histograms storage.\n",
                           routineName, bytes_tot_local*INMB);

//    gd->histograms_allocated = TRUE;

    return SUCCESS;
}


local int print_make_info(struct cmdline_data* cmd,
                                  struct  global_data* gd)
{
    verb_print_zero(cmd->verbose,
               "\nprint_make_info:\n\n");

#ifdef ADDONS
    verb_print_zero(cmd->verbose, "with ADDONS\n");
#endif

#ifdef OPENMPCODE
    verb_print_zero(cmd->verbose, "using OpenMP\n");
#endif

#ifdef SINGLEP
    verb_print_zero(cmd->verbose, "SINGLEP\n");
#endif

#ifdef LONGINT
    verb_print_zero(cmd->verbose, "LONGINT\n");
#endif

    for (size_t i = 0; fkpt2_addon_name(i) != NULL; ++i)
        verb_print_zero(cmd->verbose, "with addon %s\n", fkpt2_addon_name(i));

//B I/O modules
//E

//B libraries
#ifdef GETPARAM
    verb_print_zero(cmd->verbose, "using GETPARAM\n");
#endif

#ifdef USEGSL
#ifdef GSLINTERNAL
    verb_print_zero(cmd->verbose, "using internal GSL\n");
#else
    verb_print_zero(cmd->verbose, "using GSL\n");
#endif
#endif
//E

#ifdef DEBUG
    verb_print_zero(cmd->verbose, "DEBUG\n");
#endif

    verb_print_zero(cmd->verbose,
               "\n");


    return SUCCESS;
}

local int print_options(struct cmdline_data* cmd,
                                  struct  global_data* gd)
{
    verb_print_zero(cmd->verbose,
                    "\nprint_options_info:\n");

    verb_print_zero(cmd->verbose, "\n- make-info: %s\n",
                    "use to print on screen Makefile settings");
    
    verb_print_zero(cmd->verbose, "\n- print-models: list compiled gravity models\n");
    verb_print_zero(cmd->verbose, "\n- IA-tutor: standalone documentation tutor (fkpt2 options=IA-tutor --help)\n");
    verb_print_zero(cmd->verbose, "\n- no-stop: %s\n\t%s\n",
                    "use in combination with 'make-info'",
                    "useful to continue the computation.");

    verb_print_zero(cmd->verbose, "\n- print-options: %s\n",
        "use to print on screen all possible options in 'options' parameter");

    
    verb_print_zero(cmd->verbose,
                    "\n\n");

    return SUCCESS;
}


// pre-processing
// stop
//
//
//


