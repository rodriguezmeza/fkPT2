/* Sample the original MGPT solvers with the same initial conditions as fkPT2.
 * Build this in place of main.c in a temporary copy of the MGPT sources.
 */
#define global
#include "globaldefs.h"
#include "cmdline_defs.h"
#include "protodefs.h"

int main(int argc, char **argv)
{
    gd.cpuinit = second();
    InitParam(argv, defv);
    StartRun(argv[0], HEAD1, HEAD2, HEAD3);

    const double eta = gd.xstop;
    const double step = 1e-4;
    const double kmin = 1e-20;
    const double dplus = DpFunction(0.0);

    gd.xstop = eta + step;
    const double log_dplus_hi = log(DpFunction(0.0));
    gd.xstop = eta - step;
    const double log_dplus_lo = log(DpFunction(0.0));
    const double f0 = (log_dplus_hi - log_dplus_lo) / (2 * step);

    gd.xstop = 0.0;
    const double growth = dplus / DpFunction_LCDM(0.0);
    gd.xstop = eta;

    global_D2v2_ptr growth2 = DsSecondOrder_func(kmin, kmin, kmin);
    global_D3v2_ptr third = DsThirdOrder_func(1e-7, kmin, kmin);
    const double a = DA2D2(growth2) / ((3.0 / 7.0) * Dpk1D2(growth2) * Dpk2D2(growth2));
    const double r1 = (21.0 / 5.0) * D3symmD3(third) /
                      (DpkD3(third) * DppD3(third) * DppD3(third));
    free(growth2);
    free(third);

    printf("DGP regression %.17g %.17g %.17g %.17g %.17g\n", dplus, f0, growth, a, r1);
    EndRun();
    return 0;
}
