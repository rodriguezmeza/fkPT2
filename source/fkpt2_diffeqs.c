/*==============================================================================
 MODULE: gsm_diffeqs.c			[gsm]     
 ==============================================================================*/

#include "fkpt2_addon.h"

extern const struct fkpt2_gravity_model fkpt2_lcdm_model;
#include "protodefs.h"

local void derivsFirstOrder(struct cmdline_data* cmd, struct global_data* gd,double x,double y[],double dydx[]);
local void derivsFirstOrder_LCDM(struct cmdline_data* cmd, struct global_data* gd,double x,double y[],double dydx[]);
local void derivsSecondOrder(struct cmdline_data* cmd, struct global_data* gd,double x,double y[],double dydx[]);
local void derivsThirdOrder(struct cmdline_data* cmd, struct global_data* gd,double eta,double y[],double dydx[]);

global void derivsFirstOrder(struct cmdline_data* cmd, struct global_data* gd,double x,double y[],double dydx[])
{

    dydx[1] = y[2];
    dydx[2]=f1(cmd, gd,  x)*mu(cmd, gd,  x,gd->kf)*y[1]-f2(cmd, gd,  x)*y[2];
}

local void derivsFirstOrder_LCDM(struct cmdline_data* cmd, struct global_data* gd,double x,double y[],double dydx[])
{
    
    dydx[1] = y[2];
    const struct fkpt2_gravity_hooks *lcdm = &fkpt2_lcdm_model.hooks;
    dydx[2] = lcdm->f1(cmd, gd, x)*y[1] - lcdm->f2(cmd, gd, x)*y[2];
}

local void derivsSecondOrder(struct cmdline_data* cmd, struct global_data* gd,double x,double y[],double dydx[])
{
    
    dydx[1] = y[2];
    dydx[2]=f1(cmd, gd,  x)*mu(cmd, gd,  x,gd->k1)*y[1]-f2(cmd, gd,  x)*y[2];
//
    dydx[3] = y[4];
    dydx[4]=f1(cmd, gd,  x)*mu(cmd, gd,  x,gd->k2)*y[3]-f2(cmd, gd,  x)*y[4];
//
    dydx[5] = y[6];
    dydx[6]=f1(cmd, gd,  x)*mu(cmd, gd,  x,gd->kf)*y[5]-f2(cmd, gd,  x)*y[6]
    + sourceA(cmd, gd,  x,gd->kf,gd->k1,gd->k2)*y[1]*y[3];
//
    dydx[7] = y[8];
    dydx[8]=f1(cmd, gd,  x)*mu(cmd, gd,  x,gd->kf)*y[7]-f2(cmd, gd,  x)*y[8]
    + sourceb(cmd, gd,  x,gd->kf,gd->k1,gd->k2)*y[1]*y[3];
}

local void derivsThirdOrder(struct cmdline_data* cmd, struct global_data* gd,double eta,double y[],double dydx[])
{
    real Dpk, Dpp, D2f, D2mf, kplusp, kpluspm;

    kplusp = kpp(gd->x,gd->k,gd->p);
    kpluspm = kpp(-gd->x,gd->k,gd->p);
    Dpk  = y[1];
    Dpp  = y[3];
    D2f  = y[5];
    D2mf = y[7];

    dydx[1] = y[2];
    dydx[2] = f1(cmd, gd,  eta)*mu(cmd, gd,  eta,gd->k)*y[1]-f2(cmd, gd,  eta)*y[2];
//
    dydx[3] = y[4];
    dydx[4] = f1(cmd, gd,  eta)*mu(cmd, gd,  eta,gd->p)*y[3]-f2(cmd, gd,  eta)*y[4];
//
    dydx[5] = y[6];
    dydx[6] = f1(cmd, gd,  eta)*mu(cmd, gd,  eta,kplusp)*y[5]-f2(cmd, gd,  eta)*y[6] //;
    + SD2(cmd, gd,  eta,gd->x,gd->k,gd->p)*y[1]*y[3];
//
    dydx[7] = y[8];
    dydx[8] = f1(cmd, gd,  eta)*mu(cmd, gd,  eta,kpluspm)*y[7]-f2(cmd, gd,  eta)*y[8] //;
    + SD2(cmd, gd,  eta,-gd->x,gd->k,gd->p)*y[1]*y[3];
//
    dydx[9] = y[10];
    dydx[10] = f1(cmd, gd,  eta)*mu(cmd, gd,  eta,gd->k)*y[9]-f2(cmd, gd,  eta)*y[10]
    + S3I(cmd, gd,  eta,gd->x,gd->k,gd->p,Dpk,Dpp,D2f,D2mf)
    + S3II(cmd, gd,  eta,gd->x,gd->k,gd->p,Dpk,Dpp,D2f,D2mf)
    + S3FL(cmd, gd,  eta,gd->x,gd->k,gd->p,Dpk,Dpp,D2f,D2mf)
    + S3dI(cmd, gd,  eta,gd->x,gd->k,gd->p,Dpk,Dpp,D2f,D2mf);
}

/* Integrate ln(D_cb / D_LCDM) along with the LCDM growing mode. */
static void derivsFittedGrowth(struct cmdline_data *cmd, struct global_data *gd,
                               double eta, double y[], double dydx[])
{
    derivsFirstOrder_LCDM(cmd, gd, eta, y, dydx);
    const real ratio = gd->model->growth_rate_ratio(cmd, gd, eta, gd->kf);
    dydx[3] = (ratio - 1.0) * y[2] / y[1];
}

static void linear_solution(struct cmdline_data *cmd, struct global_data *gd, real k,
                            fkpt2_derivs derivs, double y[NEQS1Order + 1])
{
    gd->kf = k;
    y[1] = rexp(gd->xnow);
    y[2] = y[1];
    fkpt2_integrate(cmd, gd, y, NEQS1Order, derivs);
}

global real DpFunction(struct cmdline_data *cmd, struct global_data *gd, real k)
{
    if (gd->model->growth_rate_ratio != NULL) {
        const double a_ini = exp(gd->xnow);
        double y[] = {0.0, a_ini, a_ini, 0.0};
        gd->kf = k;
        fkpt2_integrate(cmd, gd, y, 3, derivsFittedGrowth);
        return y[1] * exp(y[3]);
    }
    double y[NEQS1Order + 1];
    linear_solution(cmd, gd, k, derivsFirstOrder, y);
    return y[1];
}

global real f_growth(struct cmdline_data *cmd, struct global_data *gd, real k)
{
    if (gd->model->growth_rate_ratio != NULL)
        return gd->f0 * gd->model->growth_rate_ratio(cmd, gd, gd->xstop, k);
    double y[NEQS1Order + 1];
    linear_solution(cmd, gd, k, derivsFirstOrder, y);
    return y[2] / y[1];
}

global real DpFunction_LCDM(struct cmdline_data *cmd, struct global_data *gd, real k)
{
    double y[NEQS1Order + 1];
    linear_solution(cmd, gd, k, derivsFirstOrder_LCDM, y);
    return y[1];
}

global real f_growth_LCDM(struct cmdline_data *cmd, struct global_data *gd)
{
    double y[NEQS1Order + 1];
    linear_solution(cmd, gd, 0.0001, derivsFirstOrder_LCDM, y);
    return y[2] / y[1];
}

global global_D2v2_ptr DsSecondOrder_func(struct cmdline_data* cmd, struct global_data* gd,real kf, real k1, real k2)
{
    double ystart[NEQS2Order + 1];
    global_D2v2_ptr ptmp;
//
    gd->kf = kf;
    gd->k1 = k1;
    gd->k2 = k2;
//
    
    ystart[1]=rexp(gd->xnow);
    ystart[2]=rexp(gd->xnow);
    ystart[3]=rexp(gd->xnow);
    ystart[4]=rexp(gd->xnow);
    ystart[5]=3.0*rexp(2.0*gd->xnow)/7.0;
    ystart[6]=6.0*rexp(2.0*gd->xnow)/7.0;
    ystart[7]=3.0*rexp(2.0*gd->xnow)/7.0;
    ystart[8]=6.0*rexp(2.0*gd->xnow)/7.0;
    
    const double eta = fkpt2_integrate(cmd, gd, ystart, NEQS2Order, derivsSecondOrder);
    
    ptmp = (global_D2v2_ptr) allocate(1 * sizeof(global_D2v2));

    etaD2(ptmp) = eta;
    Dpk1D2(ptmp) = ystart[1];
    Dpk2D2(ptmp) = ystart[3];
    DA2D2(ptmp) = ystart[5];
    DA2primeD2(ptmp) = ystart[6];  //Modificacion-LCDMfk
    DB2D2(ptmp) = ystart[7];
    DB2primeD2(ptmp) = ystart[8];
//
    
    return ptmp;
}

global global_D3v2_ptr DsThirdOrder_func(struct cmdline_data* cmd,
                                         struct global_data* gd,real x, real k, real p)
{
    double ystart[NEQS3Order + 1];
    global_D3v2_ptr ptmp;

    gd->x = x;
    gd->k = k;
    gd->p = p;
//
//
    ystart[1]=rexp(gd->xnow);
    ystart[2]=rexp(gd->xnow);
    ystart[3]=rexp(gd->xnow);
    ystart[4]=rexp(gd->xnow);
//
    ystart[5]=(3.0/7.0)*rexp(2.0*gd->xnow)*(1.0-rsqr(x));
    ystart[6]=(6.0/7.0)*rexp(2.0*gd->xnow)*(1.0-rsqr(x));
    ystart[7]=(3.0/7.0)*rexp(2.0*gd->xnow)*(1.0-rsqr(x));
    ystart[8]=(6.0/7.0)*rexp(2.0*gd->xnow)*(1.0-rsqr(x));
//
    ystart[9]=(5.0/(7.0*9.0))*rexp(3.0*gd->xnow)*rsqr(1.0-rsqr(x))
        *(
            1.0/(1.0 + rsqr(p/k) + 2.0 * (p/k) * x)
          + 1.0/(1.0 + rsqr(p/k) - 2.0 * (p/k) * x)
        );
//
    ystart[10]=(15.0/(7.0*9.0))*rexp(3.0*gd->xnow)*rsqr(1.0-rsqr(x))
        *(
            1.0/(1.0 + rsqr(p/k) + 2.0 * (p/k) * x)
          + 1.0/(1.0 + rsqr(p/k) - 2.0 * (p/k) * x)
          );
//
    const double eta = fkpt2_integrate(cmd, gd, ystart, NEQS3Order, derivsThirdOrder);

    ptmp = (global_D3v2_ptr) allocate(1 * sizeof(global_D3v2));
//
    etaD3(ptmp) = eta;
    DpkD3(ptmp) = ystart[1];
    DppD3(ptmp) = ystart[3];
    D2fD3(ptmp) = ystart[5];
    D2mfD3(ptmp) = ystart[7];
    D2fprimeD3(ptmp) = ystart[6];
    D2mfprimeD3(ptmp) = ystart[8];
    D3symmD3(ptmp) = ystart[9];
    D3symmprimeD3(ptmp) = ystart[10];  //Modificacion-LCDMfk
//
    
    return ptmp;
}

void integration_method_string_to_int(struct cmdline_data *cmd, struct global_data *gd,
                                      string method_str, int *method_int)
{
    if (strcmp(method_str, "rkqs") == 0) {
        *method_int = FKPT2_RKQS;
        strcpy(gd->integration_method_comment, "rkqs integration method");
    } else {
        *method_int = FKPT2_BSSTEP;
        strcpy(gd->integration_method_comment, "bsstep integration method");
    }
}
