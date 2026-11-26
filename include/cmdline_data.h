/*==============================================================================
 HEADER: cmdline_data.h		[fkPT2]
 Written by: Mario A. Rodriguez-Meza
 Starting date: october 2025
 Purpose: Definitions of global variables and parameters
 Language: C
 Use: '#include "cmdline_data.h"
 Major revisions:
 ==============================================================================*/
//        1          2          3          4        ^ 5          6          7

//
// lines where there is a "//B socket:" string are places to include module files
//  that can be found in addons/addons_include folder
//

#ifndef _cmdline_data_h
#define _cmdline_data_h

#include "vectdefs.h"
#include "common_defs.h"
#include "fkpt2_class_provider.h"

struct cmdline_data{
    // Every item in cmdline_defs.h must have an item here::

    // Background cosmology:
        real om;
        string olstr;
        real h;
    // bias parameters:
        real b1;
        real b2;
    //    real bs;
        real bs2;
        real c1eft;
        real c2eft;
        real s2eft;
    //
        real b3nl;
        real alpha0;
        real alpha2;
        real alpha4;
        real ctilde;
        real PshotP;
        real alpha0shot;
        real alpha2shot;
    // Differential equations evolution parameters:
        real x;
        string dxstr;
        real xstop;
        int maxnsteps;
        string integration_method;
        real dxmin;
        real eps;

        real zout;

    // k table
        string fnamePS;
        string classIni;
        char *class_parameters;     /* CLASS input from the [class] parameter file section. */
        unsigned int input_overrides;
        int is_PS_input_LCDM;
        real kmin;
        real kmax;
        int Nk;
    //
    // GSM correlation multipoles:
        bool gsm;
        real smin;
        real smax;
        int Ns;
        real gsm_width;
        int gsm_sizeyT;
        int gsm_NGL;
    // q functions:
        int NqperLogDecade;
        int Nk_qFunctionsQuad;
    // CLPT correlation functions:
        bool clpt;
        int clpt_Nk;
        real clpt_kmin;
        real clpt_kmax;
        real clpt_pmax;
        real clpt_b1;
        real clpt_b2;
        real clpt_bs2;
        real clpt_b01;
        real rmin;
        real rmax;
        int Nr;
    // Post processing parameters:
        bool postprocessing;
    //
        string paramfile;
    // Modified gravity model parameters:
        string mgmodel;
        string modelOptions;
        string suffixModel;
        string model_paramfile;
        int nHS;
        real fR0;
        real omegaBD;
        real screening;
    // DGP:
        real eps_DGP;
        real rc_DGP;
    // Quadrature parameters:
        string quadratureMethod;
        int nquadSteps;
        int ngausslegpoints;
        real epsquad;

    //B Parameters about the I/O file(s)
    // Output parameters
    string rootDir;
    //E

    //B Miscellaneous parameters
    string preScript;
    string posScript;
    int verbose;
    int verbose_log;
// OPENMPCODE
    int numthreads;
//E
    string options;
    //E

    string version;

    //B socket:
    #ifdef ADDONS
    #include "cmdline_data_include.h"
    #endif
    //E
    
    ErrorMsg error_message;

};

#endif // ! _cmdline_data_h

