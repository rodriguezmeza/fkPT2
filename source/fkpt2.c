/*==============================================================================
 NAME: fkpt2.c				[fkPT2]
 Written by: Mario A. Rodriguez-Meza & Alejandro Aviles
 Starting date: 31 october 2025
 Purpose: Main routine
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

void fkpt2_solve(struct cmdline_data *cmd, struct global_data *gd)
{
    global_variables(cmd, gd);
    compute_kfunctions(cmd, gd);
}

int fkpt2_write_run(struct cmdline_data *cmd, struct global_data *gd,
                    fkpt2_result **analysis_result)
{
    fkpt2_solve(cmd, gd);
    if (fkpt2_has_addon_results(cmd)) {
        if (fkpt2_capture_result(cmd, gd, analysis_result) == FAILURE)
            return FAILURE;
        const char *name;
        double value;
        for (size_t i = 0; (name = fkpt2_result_diagnostic(*analysis_result, i, &value)) != NULL; ++i)
            printf("%s = %.17g\n", name, value);
    }
    if (compute_rsdmultipoles(cmd, gd) == FAILURE || write_fkpt(cmd, gd) == FAILURE)
        return FAILURE;
    return fkpt2_write_addon_tables(cmd, *analysis_result);
}
