/* ==============================================================================
 HEADER: cmdline_defs.h        [fkPT2]
 Written by: Mario A. Rodriguez-Meza
 Starting date: 15 october 2025
 Purpose: Definitions for importing arguments from the command line
 Language: C
 Use: '#include "...."
 Major revisions:
 ==============================================================================*/
//        1          2          3          4          5          6          7

#ifndef _cmdline_defs_h
#define _cmdline_defs_h

#define HEAD1 "CosmoININ"
#define HEAD2 "fkPT2 code for computing RSD power spectrum multipoles"
#define HEAD3 "Usage: fkpt2 [parameter-file] [name=value ...]"

#define PARAMETER(member, name, value, help) #name "=" value, ";" help,
#define PARAMETER_ALIAS(member, name, value, help, alias) PARAMETER(member, name, value, help) ":" #alias,

string defv[] = {
    ";" HEAD1 ": " HEAD2 "\n\t" HEAD3,
    "paramfile=", ";Parameter file. Command-line values take precedence.",
#include "run_parameters.def"
#ifdef ADDONS
#include "cmdline_defs_include.h"
#endif
    NULL,
};

#undef PARAMETER_ALIAS
#undef PARAMETER
#endif
