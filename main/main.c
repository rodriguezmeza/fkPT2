/*==============================================================================
 NAME: main.c				[fkPT2]
 Written by: Mario A. Rodriguez-Meza & Alejandro Aviles
 Starting date: 15 october 2025
 Purpose: Main routine (of fkPT2 - perturbation theory code)
 Language: C
 Major revision:
 ===============================================================================
 Use: fkpt2 --help (or -h)
 Input: 	Command line parameters, Parameters file, pk
 Output: several data files...
 Units:
 History:
 Acknowledgements:
 Comments and notes: (palimsesto)... coding based on references below.
 References:    Zeno project, NEMO project, Gadget, CLASS,
                NR, GSL...
 github: https://github.com/rodriguezmeza/fkPT2.git
 Publication: cite: (ArXiv ePrint: xxxx.xxxxx)
 ==============================================================================*/
//        1          2          3          4        ^ 5          6          7

#include "fkpt2_internal.h"
#include "cmdline_defs.h"
#include "ia_tutor_cli.h"

struct cli_run {
    const char *program;
    struct cmdline_data cmd;
    struct global_data gd;
    fkpt2_result *analysis_result;
};

static int run_cli(void *state)
{
    struct cli_run *run = state;
    int status = StartRun(&run->cmd, &run->gd, (char *)run->program, HEAD1, HEAD2, HEAD3);
    if (status == SUCCESS && !run->gd.stopFlag)
        status = fkpt2_write_run(&run->cmd, &run->gd, &run->analysis_result);
    return status;
}

int main(int argc, string argv[])
{
    int tutor_status = 0;
    if (fkpt2_ia_tutor_cli(argc, argv, &tutor_status)) return tutor_status;
    struct cli_run run = {.program = argv[0]};
    run.gd.cpuinit = CPUTIME;
    run.gd.cpurealinit = rcpu_time();

    InitParam(argv, defv);
    int status = fkpt2_with_workspace(run_cli, &run, run.cmd.error_message,
                                      sizeof(run.cmd.error_message));
    fkpt2_result_free(run.analysis_result);
    if (status != SUCCESS) {
        if (*run.cmd.error_message)
            fprintf(stderr, "fkPT2: %s\n", run.cmd.error_message);
        EndRun_FreeMemory(&run.cmd, &run.gd);
        return FAILURE;
    }

    if (EndRun(&run.cmd, &run.gd) == FAILURE) {
        fprintf(stderr, "fkPT2: %s\n", run.cmd.error_message);
        return FAILURE;
    }
    return SUCCESS;
}
