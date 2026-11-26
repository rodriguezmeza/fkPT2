/* Test the original MGPT convolution using supplied q-functions. */
#define global
#include "globaldefs.h"
#include "cmdline_defs.h"
#include "protodefs.h"

int main(int argc, char **argv)
{
    gd.cpuinit = second();
    InitParam(argv, defv);
    StartRun(argv[0], HEAD1, HEAD2, HEAD3);
    CLPT_correlation_processing();
    EndRun();
    return 0;
}
