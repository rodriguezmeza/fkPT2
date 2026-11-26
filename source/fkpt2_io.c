/*==============================================================================
 MODULE: gsm_io.c		[fkpt]
 ==============================================================================*/

#include "fkpt2_internal.h"
#include "protodefs.h"
#include <errno.h>

int StartOutput(struct cmdline_data* cmd, struct global_data* gd)
{
    fprintf(gd->outlog,"\n%8s%8s%8s", "maxnsteps", "etaini", "deta");
    fprintf(gd->outlog,"%8s\n","etaout");
    fprintf(gd->outlog,"%8d%8.2f%8.4f%8.4f\n",cmd->maxnsteps,cmd->x,gd->dx,gd->xstop);
    if (! strnull(cmd->options))
        fprintf(stdout,"\n\toptions: %s\n", cmd->options);
    
    if (cmd->verbose>=VERBOSEMININFO) {
        if (! strnull(cmd->options))
            verb_print(cmd->verbose, "\n\toptions: %s\n", cmd->options);
    }

    return SUCCESS;
}

/* Create directories directly so rootDir works with ordinary paths containing
 * spaces, and filesystem errors have an actionable message.
 */
static int ensure_directory(struct cmdline_data *cmd, const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return SUCCESS;
    if (mkdir(path, 0777) == 0)
        return SUCCESS;

    int failure = errno;
    if (failure == EEXIST) {
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode))
                return SUCCESS;
            failure = ENOTDIR;
        } else {
            failure = errno;
        }
    }

    snprintf(cmd->error_message, sizeof(cmd->error_message),
             "Cannot create output directory '%s': %s", path, strerror(failure));
    return FAILURE;
}

int setFilesDirs(struct cmdline_data *cmd, struct global_data *gd)
{
    char directory[MAXLENGTHOFFILES];
    if (format_checked(directory, sizeof(directory), "rootDir", "%s", cmd->rootDir) != 0) {
        snprintf(cmd->error_message, sizeof(cmd->error_message), "rootDir is too long");
        return FAILURE;
    }

    double cpustart = CPUTIME;
    for (char *slash = directory + 1; *slash; ++slash) {
        if (*slash != '/' || slash[-1] == '/')
            continue;
        *slash = '\0';
        if (ensure_directory(cmd, directory) == FAILURE)
            return FAILURE;
        *slash = '/';
    }
    if (ensure_directory(cmd, directory) == FAILURE)
        return FAILURE;
    gd->rootDirFlag = TRUE;

    struct output_path {
        const char *name;
        char *path;
        size_t size;
    };
#define OUTPUT_PATH(name, field) {name, gd->field, sizeof(gd->field)}
    const struct output_path outputs[] = {
        OUTPUT_PATH("kfunctions", fpfnamekfun),
        OUTPUT_PATH("kfunctions_nw", fpfnamekfunnw),
        OUTPUT_PATH("rsd_multipoles", fpfnamersd),
        OUTPUT_PATH("SPTPowerSpectrum", fpfnameSPTPowerSpectrum),
        OUTPUT_PATH("qfunctions", fpfnameqfunctions),
        OUTPUT_PATH("clpt", fpfnameclptfunctions),
        OUTPUT_PATH("v12", fpfnamev12),
        OUTPUT_PATH("sigma12_parallel", fpfnamesigma12_parallel),
        OUTPUT_PATH("sigma12_perp", fpfnamesigma12_perp),
    };
#undef OUTPUT_PATH
    for (size_t i = 0; i < sizeof(outputs) / sizeof(outputs[0]); ++i) {
        const struct output_path *out = &outputs[i];
        if (format_checked(out->path, out->size, out->name, "%s/%s%s%s",
                           cmd->rootDir, out->name, cmd->suffixModel, EXTFILES) != 0) {
            snprintf(cmd->error_message, sizeof(cmd->error_message),
                     "Output path for '%s' is too long", out->name);
            return FAILURE;
        }
    }

#ifdef ADDONS
#include "cballsio_include_09b.h"
#endif

    gd->cputotalinout += CPUTIME - cpustart;
    return SUCCESS;
}

int setFilesDirs_log(struct cmdline_data *cmd, struct global_data *gd)
{
    if (format_checked(gd->tmpDir, sizeof(gd->tmpDir), "tmpDir", "%s/tmp", cmd->rootDir) != 0 ||
        (cmd->verbose_log > 0 &&
         format_checked(gd->logfilePath, sizeof(gd->logfilePath), "logfilePath",
                        "%s/cballs%s.log", gd->tmpDir, cmd->suffixModel) != 0)) {
        snprintf(cmd->error_message, sizeof(cmd->error_message), "Temporary or log path is too long");
        return FAILURE;
    }
    return ensure_directory(cmd, gd->tmpDir);
}

//B write.c
#define FMTQFUNCTIONSDAT    \
"%e %e %e %e %e %e %e \
 %e %e %e %e %e %e %e \
 %e %e %e %e %e %e %e\n"

local int print_kfunctions(struct cmdline_data *cmd, struct global_data *gd,
                            const global_kFArrays *arrays, char *path);
local int print_linear(struct cmdline_data* cmd, struct global_data* gd);

int write_fkpt(struct cmdline_data* cmd, struct global_data* gd)
{
    if(cmd->verbose==1){
        fprintf(stdout,"\nz = %g\n",cmd->zout);
        fprintf(stdout,"D_+ = %g\n",gd->Dplus);
        fprintf(stdout,"f0 = %g, ",gd->f0);
        fprintf(stdout,"f_LCDM = %g   \n",f_growth_LCDM(cmd, gd));
        fprintf(stdout,"sigma8(z=%g)=%g \n",cmd->zout,gd->sigma8);
    };
    
    if (print_linear(cmd, gd) == FAILURE || print_kfunctions(cmd, gd, &kFArrays, gd->fpfnamekfun) == FAILURE)
        return FAILURE;
    return print_kfunctions(cmd, gd, &kFArrays_nw, gd->fpfnamekfunnw);
}
int fkpt2_write_addon_tables(struct cmdline_data *cmd, const fkpt2_result *result)
{
    if (result == NULL)
        return SUCCESS;

    const fkpt2_addon_table *table;
    const char *routineName = "fkpt2_write_addon_tables";
    for (size_t index = 0; (table = fkpt2_result_addon_table(result, index)) != NULL; ++index) {
        char path[BUFFERSIZE];
        if (format_checked(path, sizeof(path), "addon output path", "%s/%s%s%s",
                           cmd->rootDir, table->name, cmd->suffixModel, EXTFILES) != 0) {
            snprintf(cmd->error_message, sizeof(cmd->error_message), "Addon output path is too long");
            return FAILURE;
        }

        FILE *file = NULL;
        OPEN_OUTPUT_OR_FAIL(file, path, "w!");
        fprintf(file, "# model=%s z=%g\n#", fkpt2_result_model(result), cmd->zout);
        for (size_t j = 0; j < table->columns; ++j)
            fprintf(file, " %s", table->column_names[j]);
        fputc('\n', file);
        for (size_t i = 0; i < table->rows; ++i)
            for (size_t j = 0; j < table->columns; ++j)
                fprintf(file, "%.15e%c", table->data[i * table->columns + j],
                        j + 1 == table->columns ? '\n' : ' ');
        CLOSE_OUTPUT_OR_FAIL(file, path);
    }
    return SUCCESS;
}

#undef FMTQFUNCTIONSDAT

void fkpt2_write_spectrum_source(FILE *out, const struct cmdline_data *cmd)
{
    if (cmd->class_parameters != NULL)
        fprintf(out, "# CLASSSection=%s", cmd->paramfile);
    else if (*cmd->classIni)
        fprintf(out, "# CLASSIni=%s", cmd->classIni);
    else
        fprintf(out, "# InputPklFile=%s", cmd->fnamePS);
}

local int print_kfunctions(struct cmdline_data *cmd, struct global_data *gd,
                            const global_kFArrays *arrays, char *path)
{
    const char *routineName = "print_kfunctions";
    stream outstr = NULL;
    OPEN_OUTPUT_OR_FAIL(outstr, path, "w!");

    fkpt2_write_spectrum_source(outstr, cmd);
    fprintf(outstr, ", redshift z=%g\n", cmd->zout);
    fprintf(outstr,"# Precision: k-functions: nquadStetps=%d\n",
                cmd->nquadSteps);
    fprintf(outstr,"%1s%8s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s \
        %15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s%15s",
            "#","1.k","2.P22dd","3.P22du","4.Pdduu",
            "5.P13dd","6.P13du","7.P13uu",
            "8.I1udd1A", "9.I2uud1A", "10.I2uud2A", "11.I3uuu2A", "12.I3uuu3A",
            "13.I2uudd1D", "14.I2uudd2D", "15.I3uuud2D", "16.I3uuud3D",
            "17.I4uuuu2", "18.I4uuuu3D", "19.I4uuuu4D",
            "20.Pb1b2", "21.Pb1bs2", "22.Pb22", "23.Pb2s2",
            "24.Ps22", "25.Pb2theta", "26.Pbs2theta",
            "27.sigma32pkl","28.pklinear", "29.fk", "30.f0\n");

    for (int i = 0; i < golists.Nk; ++i) {
        double row[FKPT2_LOOP_COLUMNS];
        fkpt2_loop_row(arrays, i, gd->f0, row);
        for (size_t j = 0; j < FKPT2_LOOP_COLUMNS; ++j)
            fprintf(outstr, "%e%c", row[j], j + 1 == FKPT2_LOOP_COLUMNS ? '\n' : ' ');
    }
    CLOSE_OUTPUT_OR_FAIL(outstr, path);
    return SUCCESS;
}

local int print_linear(struct cmdline_data* cmd, struct global_data* gd)
{
    const char *routineName = "print_linear";
    stream outstr = NULL;
    char namebuf[BUFFERSIZE];

    if (format_checked(namebuf, sizeof(namebuf), "linear spectrum path",
                       "%s/linear%s%s", cmd->rootDir, cmd->suffixModel, EXTFILES) != 0) {
        snprintf(cmd->error_message, sizeof(cmd->error_message), "Linear spectrum output path is too long");
        return FAILURE;
    }
    OPEN_OUTPUT_OR_FAIL(outstr, namebuf, "w!");

    fkpt2_write_spectrum_source(outstr, cmd);
    fputc('\n', outstr);
    fprintf(outstr,"# z=%g, OmegaM=%g, h=%g\n",
        cmd->zout, cmd->om, cmd->h);
    fprintf(outstr,"%1s%11s%17s%20s%22s%22s",
            "#"," 1.k[h/Mpc]","2.pkl","3.f(k)","4.D+(k)","5.pkl_nw\n");
   
    for (int i=1; i<=nPSLT; i++) {
        double row[FKPT2_LINEAR_COLUMNS];
        fkpt2_linear_row(i, row);
        fprintf(outstr, "%-19.9e %-19.9e %-19.9e %-19.9e %-19.9e\n",
                row[0], row[1], row[2], row[3], row[4]);
    }

    CLOSE_OUTPUT_OR_FAIL(outstr, namebuf);
    return SUCCESS;
}

//E write.c


//B
local int EndRun_CloseLog(struct cmdline_data *cmd, struct global_data *gd)
{
    gd->outlogFlagFree = FALSE;
    const char *path = cmd->verbose_log > 0 ? gd->logfilePath : "/dev/null";
    return cballs_stream_close_checked(cmd, "EndRun", &gd->outlog, path);
}
//E

int EndRun(struct cmdline_data* cmd, struct global_data* gd)
{
    string routineName = "EndRun";
    stream outstr;

    if (cmd->verbose >= VERBOSENORMALINFO) {
#ifdef OPENMPCODE
//        printf("cpusearch \t= %lf %s\n",
//               gd->cpusearch, PRNUNITOFTIMEUSED);
#else
//        printf("cpusearch \t= %lf %s\n",
//               gd->cpusearch, PRNUNITOFTIMEUSED);
#endif
//        printf("cputotalinout \t= %lf %s\n",
//               gd->cputotalinout, PRNUNITOFTIMEUSED);
    }

    if (cmd->verbose > VERBOSENOINFO) {
        real cpuTotal = CPUTIME - gd->cpuinit;
        printf("\nFinal CPU time : %lf %s\n",
               cpuTotal, PRNUNITOFTIMEUSED);

        /*
        if (scanopt(cmd->options, "measure-cputime")) {
            OPEN_OUTPUT_OR_FAIL(outstr, gd->fpfnameCPUFileName, "a");
            WRITE_OUTPUT_OR_FAIL(outstr, gd->fpfnameCPUFileName,
                                "%g %g %g\n",
                                (double) cmd->nbody, cpuTotal, gd->cpusearch);
            CLOSE_OUTPUT_OR_FAIL(outstr, gd->fpfnameCPUFileName);
        }
        */
        printf("Final real time: %ld",
               (rcpu_time()-gd->cpurealinit));
        printf(" %s\n\n", PRNUNITOFTIMEUSED);       // Only work this way
    }

    return EndRun_FreeMemory(cmd, gd);
}

//
// We must check the order of memory allocation and deallocation!!!
//
global int EndRun_FreeMemory(struct cmdline_data* cmd,
                             struct  global_data* gd)
{
    int status = EndRun_CloseLog(cmd, gd);
    if (gd->cmd_allocated) {
        free_run_parameters(cmd);
        gd->cmd_allocated = FALSE;
    }
    return status;
}

global int EndRun_FreeMemory_gd(struct cmdline_data* cmd,
                             struct  global_data* gd)
{
    string routineName = "EndRun_FreeMemory_gd";
    int ifile;

    //B Set gsl uniform random :: If not needed globally
    //      this line have to go to testdata
    #ifdef USEGSL
        gsl_rng_free (gd->r);           // allocated by random_init
                                        //  routine in startrun.c
    #endif
    //E

    gd->gd_allocated = FALSE;

    return SUCCESS;
}


global int EndRun_FreeMemory_cmd(struct cmdline_data* cmd,
                             struct  global_data* gd)
{
    string routineName = "EndRun_FreeMemory_cmd";
/*
#ifdef SAVERESTORE
if (gd->restorefileFlag == TRUE && cmd->restorefile != NULL) {
    free(cmd->restorefile);
    cmd->restorefile = NULL;
    gd->restorefileFlag = FALSE;
}

if (gd->statefileFlag == TRUE && cmd->statefile != NULL) {
    free(cmd->statefile);
    cmd->statefile = NULL;
    gd->statefileFlag = FALSE;
}
#endif
*/
/*
#ifdef IOLIB
    if (gd->columnsFlag == TRUE && cmd->columns != NULL) {
        free(cmd->columns);
        cmd->columns = NULL;
        gd->columnsFlag = FALSE;
    }
#endif
*/

    //B
/*
    if (gd->outfileFlag == TRUE && cmd->outfile != NULL) {
        free(cmd->outfile);
        cmd->outfile = NULL;
        gd->outfileFlag = FALSE;
    }

    if (gd->outfilefmtFlag == TRUE && cmd->outfilefmt != NULL) {
        free(cmd->outfilefmt);
        cmd->outfilefmt = NULL;
        gd->outfilefmtFlag = FALSE;
    }
*/

/*
    if (gd->histNNFileNameFlag == TRUE && cmd->histNNFileName != NULL) {
        free(cmd->histNNFileName);
        cmd->histNNFileName = NULL;
        gd->histNNFileNameFlag = FALSE;
    }

    if (gd->histXi2pcfFileNameFlag == TRUE && cmd->histXi2pcfFileName != NULL) {
        free(cmd->histXi2pcfFileName);
        cmd->histXi2pcfFileName = NULL;
        gd->histXi2pcfFileNameFlag = FALSE;
    }

    if (gd->histZetaFileNameFlag == TRUE && cmd->histZetaFileName != NULL) {
        free(cmd->histZetaFileName);
        cmd->histZetaFileName = NULL;
        gd->histZetaFileNameFlag = FALSE;
    }
*/

/*
    if (gd->suffixOutFilesFlag == TRUE && cmd->suffixOutFiles != NULL) {
        free(cmd->suffixOutFiles);
        cmd->suffixOutFiles = NULL;
        gd->suffixOutFilesFlag = FALSE;
    }

    if (gd->testmodelFlag == TRUE && cmd->testmodel != NULL) {
        free(cmd->testmodel);
        cmd->testmodel = NULL;
        gd->testmodelFlag = FALSE;
    }
*/

/*
    if (gd->preScriptFlag == TRUE && cmd->preScript != NULL) {
        free(cmd->preScript);
        cmd->preScript = NULL;
        gd->preScriptFlag = FALSE;
    }
    
    if (gd->posScriptFlag == TRUE && cmd->posScript != NULL) {
        free(cmd->posScript);
        cmd->posScript = NULL;
        gd->posScriptFlag = FALSE;
    }
*/
    //E

/*
    if (gd->optionsFlag == TRUE && cmd->options != NULL) {
        free(cmd->options);
        cmd->options = NULL;
        gd->optionsFlag = FALSE;
    }
*/

/*
    if (gd->rootDirFlagFree == TRUE && cmd->rootDir != NULL) {
        free(cmd->rootDir);
        cmd->rootDir = NULL;
        gd->rootDirFlagFree = FALSE;
    }
*/

/*
    if (gd->iCatalogsFlag == TRUE && cmd->iCatalogs != NULL) {
        free(cmd->iCatalogs);
        cmd->iCatalogs = NULL;
        gd->iCatalogsFlag = FALSE;
    }
*/

/*
    if (gd->infilefmtFlag == TRUE && cmd->infilefmt != NULL) {
        free(cmd->infilefmt);
        cmd->infilefmt = NULL;
        gd->infilefmtFlag = FALSE;
    }
    
    if (gd->infileFlag == TRUE && cmd->infile != NULL) {
        free(cmd->infile);
        cmd->infile = NULL;
        gd->infileFlag = FALSE;
    }
*/

/*
    if (gd->rsmoothFlagFree == TRUE && cmd->rsmooth != NULL) {
        free(cmd->rsmooth);
        cmd->rsmooth = NULL;
        gd->rsmoothFlagFree = FALSE;
    }
    
    if (gd->searchMethodFlag == TRUE && cmd->searchMethod != NULL) {
        free(cmd->searchMethod);
        cmd->searchMethod = NULL;
        gd->searchMethodFlag = FALSE;
    }
*/
    // last one must be paramfile, check!!

    gd->cmd_allocated = FALSE;

    return SUCCESS;
}

