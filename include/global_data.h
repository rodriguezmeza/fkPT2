/*==============================================================================
 HEADER: global_data.h		[fkPT2]
 Written by: Mario A. Rodriguez-Meza
 Starting date: 15 october 2025
 Purpose: Definitions of global variables and parameters
 Language: C
 Use: '#include "global_data.h"
 Major revisions:
 ==============================================================================*/
//        1          2          3          4        ^ 5          6          7

//
// lines where there is a "//B socket:" string are places to include module files
//  that can be found in addons/addons_include folder
//

#ifndef _global_data_h
#define _global_data_h

#include "vectdefs.h"
#include "common_defs.h"
#include "gravity_hooks.h"

struct fkpt2_gravity_model;

struct global_data{

    real cpuinit;
    long cpurealinit;                               // get time of the day
    real cputotalinout;
    real cputotal;

    INTEGER bytes_tot;

    //B from StartRun_Common
    /*
    real c1eft;
    real c2eft;
    real s2eft;
    real smin;
    real smax;
    int Ns;
    int NqperLogDecade;
    int Nk_qFunctionsQuad;
    real gsm_width;
    int gsm_sizeyT;
    int gsm_NGL;
//    real screening;
//    real eps_DGP;
//    real rc_DGP;
    real rmin;
    real rmax;
    int Nr;
    string olstr;
    real xx;
    string dxstr;
    real dxmin;
    real eps;
    int maxnsteps;
    string integration_method;
    int ngausslegpoints;
    real epsquad;
    bool postprocessing;
*/
//    real xstop;
    //E
    
    
    real dx;
    int method_int;
    int quadmethod_int;

// Modified gravity model parameters:
    real beta2;
//
    char integration_method_comment[100];
    char quadraturemethod_comment[100];

    string headline0;
    string headline1;
    string headline2;
    string headline3;

    const struct fkpt2_gravity_model *model;
    struct fkpt2_gravity_hooks gravity;
    real *model_options;
    void *model_state;
    char model_comment[100];

    FILE *outlog;
    
    real ol;

    real xnow;
    real xout;
    real xoutinfo;
    real xstop;

    char mode[2];

    char fnamePSPath[BUFFERSIZE];
    char logfilePath[BUFFERSIZE];
    char tmpDir[BUFFERSIZE];
    char fpfnamekfun[BUFFERSIZE];
    char fpfnamekfunnw[BUFFERSIZE];
    char fpfnameSPTPowerSpectrum[BUFFERSIZE];
    char fpfnameqfunctions[BUFFERSIZE];
    char fpfnameclptfunctions[BUFFERSIZE];
    char fpfnamersd[BUFFERSIZE];
    char fpfnamev12[BUFFERSIZE];
    char fpfnamesigma12_parallel[BUFFERSIZE];
    char fpfnamesigma12_perp[BUFFERSIZE];
    char fpfnameParams[100];

    real kf;
    real k1;
    real k2;

    real x;
    real k;
    real p;
    
    real f0;
    real Dplus;
    real particles_meanPath;
    real sigma2L;
    real sigma8;
    real sigma2v, sigma2v_nw;
    
    real Sigma2;
    real deltaSigma2;

    //B to control memory allocation/deallocation
    bool startrun_cputime;
    bool cmd_allocated;
    bool gd_allocated;
    bool rootDirFlag;
    bool outlogFlagFree;
    bool stopFlag;
    //E

//B socket:
#ifdef ADDONS
#include "global_data_include.h"
#endif
//E

};

#endif // ! _global_data_h

