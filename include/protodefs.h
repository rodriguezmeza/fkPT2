/*==============================================================================
 HEADER: protodefs.h                [fkPT2]
 Written by: Mario A. Rodriguez-Meza
 Starting date: 31 october 2025
 Purpose: Definitions of global prototypes
 Language: C
 Use: '#include "protodefs.h"
 Major revisions:
 ==============================================================================*/
//        1          2          3          4        ^ 5          6          7

//
// lines where there is a "//B socket:" string are places to include module files
//  that can be found in addons/addons_include folder
//

#ifndef _protodefs_h
#define _protodefs_h

void integration_method_string_to_int(struct cmdline_data* cmd, struct global_data* gd,string,int *);
void quadraturemethod_string_to_int(struct cmdline_data* cmd, struct global_data* gd,string,int *);

int StartRun(struct cmdline_data* cmd,
              struct global_data* gd, string, string, string, string);
int StartOutput(struct cmdline_data* cmd, struct global_data* gd);
int read_run_parameters(struct cmdline_data *cmd);
int write_run_parameters(struct cmdline_data *cmd, struct global_data *gd);
void free_run_parameters(struct cmdline_data *cmd);
int EndRun(struct cmdline_data* cmd, struct global_data* gd);
global int EndRun_FreeMemory(struct cmdline_data* cmd,
                             struct  global_data* gd);
global int EndRun_FreeMemory_cmd(struct cmdline_data* cmd,
                             struct  global_data* gd);
global int EndRun_FreeMemory_gd(struct cmdline_data* cmd,
                             struct  global_data* gd);

global void global_variables(struct cmdline_data* cmd, struct global_data* gd);
global void compute_kfunctions(struct cmdline_data* cmd, struct global_data* gd);
int compute_rsdmultipoles(struct cmdline_data* cmd, struct global_data* gd);
global void compute_qfunctions(struct cmdline_data* cmd, struct global_data* gd);
global void compute_clpt(struct cmdline_data* cmd, struct global_data* gd);
int write_fkpt(struct cmdline_data* cmd, struct global_data* gd);

// MGLPT DIFFEQS and QUAD
global real DpFunction(struct cmdline_data* cmd, struct global_data* gd,real k);
global real DpFunction_LCDM(struct cmdline_data* cmd, struct global_data* gd,real k);
global real f_growth(struct cmdline_data* cmd, struct global_data* gd,real k);
global real f_growth_LCDM(struct cmdline_data* cmd, struct global_data* gd);
global global_D2v2_ptr DsSecondOrder_func(struct cmdline_data* cmd, struct global_data* gd,real kf, real k1, real k2);
global global_D3v2_ptr DsThirdOrder_func(struct cmdline_data* cmd, struct global_data* gd,real x, real k, real p);

// MGLPT QUADS
global_kFs ki_functions_driver(struct cmdline_data* cmd, struct global_data* gd,real ki, double kPKL[], double pPKL[], int nPKLT, double pPKL2[]);

// I/O directories:
global int setFilesDirs_log(struct cmdline_data* cmd, struct global_data* gd);
global int setFilesDirs(struct cmdline_data* cmd, struct global_data* gd);

global  real psInterpolation_nr(real k, double kPS[], double pPS[], int nPS);
//

//B models.h
int set_model(struct cmdline_data* cmd, struct global_data* gd);
// functions (mgpt_fns)
global real OmM(struct cmdline_data* cmd, struct global_data* gd, real eta);
global real H(struct cmdline_data* cmd, struct global_data* gd, real eta);
global real f1(struct cmdline_data* cmd, struct global_data* gd, real eta);
global real f2(struct cmdline_data* cmd, struct global_data* gd, real eta);
global real A0(struct cmdline_data* cmd, struct global_data* gd, real eta);
//
// Model global HEADERS
global real mu(struct cmdline_data* cmd, struct global_data* gd, real eta, real k);
global real sourceA(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real kf, real k1, real k2);
global real sourceb(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real kf, real k1, real k2);
global real kpp(real x, real k, real p);
global real SD2(struct cmdline_data* cmd,
                struct global_data* gd, real eta, real x, real k, real p);
global real S3I(struct cmdline_data* cmd,
                struct global_data* gd, real eta, real x, real k, real p, real Dpk,
                real Dpp,
                real D2f, real D2mf);
global real S3II(struct cmdline_data* cmd,
                 struct global_data* gd, real eta, real x, real k, real p, real Dpk,
                 real Dpp,
                 real D2f, real D2mf);
global real S3FL(struct cmdline_data* cmd,
                 struct global_data* gd, real eta, real x, real k, real p, real Dpk,
                 real Dpp, real D2f, real D2mf);
global real S3dI(struct cmdline_data* cmd,
                 struct global_data* gd, real eta, real x, real k, real p, real Dpk,
                 real Dpp,
                 real D2f, real D2mf);
//E

void rk4(struct cmdline_data *cmd, struct global_data *gd,
         double y[], double dydx[], int n, double x, double h, double yout[],
         fkpt2_derivs derivs);


//B quads.h
double qromo(struct cmdline_data* cmd, struct global_data* gd,double (*func)(struct cmdline_data*, struct global_data*, double), double a, double b,
             double (*choose)(struct cmdline_data*, struct global_data*, double (*)(struct cmdline_data*, struct global_data*, double), double, double, int),
             double epsq, int KK);

double midpnt(struct cmdline_data* cmd, struct global_data* gd,double (*func)(struct cmdline_data*, struct global_data*, double), double a, double b, int n);
//E quads.h


//B rsd.h
global_rsdmultipoles pk_rsd_multipoles(struct cmdline_data* cmd,
                                       struct global_data* gd, int ii, real b1, real b2,
                                       real bs2, real b3nl, real alpha0, real alpha2,
                                       real alpha4, real ctilde, real PshotP,
                                       real alpha0shot, real alpha2shot);

local real pk_IR(int ii, real mu, real b1, real b2, real bs2, real b3nl,
            real alpha0, real alpha2, real alpha4, real ctilde,
            real PshotP, real alpha0shot, real alpha2shot);

local real pk_loop(struct cmdline_data* cmd, struct global_data* gd,
                   int ii,real mu, real b1, real b2,
                   real bs2, real b3nl, real alpha0, real alpha2, real alpha4,
                   real ctilde, real PshotP, real alpha0shot, real alpha2shot);

local real pk_loop_nw(struct cmdline_data* cmd, struct global_data* gd,
                      int ii,real mu, real b1, real b2,
                      real bs2, real b3nl, real alpha0, real alpha2, real alpha4,
                      real ctilde, real PshotP, real alpha0shot, real alpha2shot);

local real A_TNS(real f0, real mu, real I1udd1A, real I2uud1A,
                 real I2uud2A, real I3uuu2A, real I3uuu3A);

local real D_function(real f0, real mu, real I2uudd1D, real I2uudd2D,
                      real I3uuud2D, real I3uuud3D, real I4uuuu2D,
                      real I4uuuu3D, real I4uuuu4D);
            
local real pk_EFT_IR(struct cmdline_data* cmd, struct global_data* gd,
                     int i, real mu, real b1, real b2, real bs2, real b3nl,
                     real alpha0, real alpha2, real alpha4, real ctilde,
                     real PshotP, real alpha0shot, real alpha2shot);

local real pk_ME(struct cmdline_data* cmd, struct global_data* gd,
                 int i, real mu, real b1, real b2, real bs2, real b3nl);

//E rsd.h

#include "models.h"

global int startrun_memoryAllocation(struct  cmdline_data *cmd,
                                     struct  global_data* gd);


global int ThreadCount(struct  cmdline_data* cmd, struct  global_data* gd,
                       INTEGER nbody, int cat1);

int cballs_stream_close_checked(struct cmdline_data *cmd, const char *routineName,
                                 stream *stream_ptr, const char *filename);

global int cballs_system_checked(struct cmdline_data *cmd,
                                 string routineName,
                                 string command);


#endif // ! _protodefs_h
