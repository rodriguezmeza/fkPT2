/*==============================================================================
	MODULE: models.c			[mgpt]
==============================================================================*/
#include "fkpt2_addon.h"

local real OmM_HS(struct cmdline_data* cmd, struct global_data* gd, real eta);
local  real H_HS(struct cmdline_data* cmd, struct global_data* gd, real eta);
local  real f1_HS(struct cmdline_data* cmd, struct global_data* gd, real eta);
local  real f2_HS(struct cmdline_data* cmd, struct global_data* gd, real eta);
local  real A0_HS(struct cmdline_data* cmd, struct global_data* gd, real eta);
local real mu_HS(struct cmdline_data* cmd, struct global_data* gd, real eta, real k);
local real sourceA_HS(struct cmdline_data* cmd,
                      struct global_data* gd, real eta, real kf, real k1, real k2);
local real sourceb_HS(struct cmdline_data* cmd,
                      struct global_data* gd, real eta, real kf, real k1, real k2);
local real SD2_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p);
local real S3I_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p, real Dpk,
                  real Dpp,
                real D2f, real D2mf);
local real S3II_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p, real
                   Dpk, real Dpp,
                 real D2f, real D2mf);
local real S3FL_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p, real
                   Dpk, real Dpp, real D2f, real D2mf);
local real S3dI_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p, real
                   Dpk, real Dpp, real D2f, real D2mf);

local real OmM_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real OmMtmp;
    
    OmMtmp=1.0/(1.0 + ( (gd->ol)/cmd->om)*rexp(3.0*eta) );
    return (OmMtmp);
}

local  real H_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real Htmp;
    
    Htmp=rsqrt(cmd->om*rexp(-3.0*eta)+(gd->ol));
    return (Htmp);
}

local  real f1_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real f1tmp;
    
    f1tmp=3.0/(2.0*(1.0
                    + ((gd->ol)/cmd->om)*rexp(3.0*eta)
                    )
               );
    return (f1tmp);
}

local  real f2_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real f2tmp;
    
    f2tmp=2.0 - 3.0/(2.0*(1.0
                          + ((gd->ol)/cmd->om)*rexp(3.0*eta)
                          )
                     );
    return (f2tmp);
}

local real A0_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real A0tmp;
    
    A0tmp = 1.5*OmM_HS(cmd, gd, eta)*rsqr(H_HS(cmd, gd, eta))/rsqr(invH0);
    
    return (A0tmp);
}
// End :: mglpt_fns

// Begin: Hu-Sawicky Model local HEADERS
local real mass_HS(struct cmdline_data* cmd, struct global_data* gd, real eta);
local real JFL_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p);
local real KFL_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real k, real k1, real k2);
local real KFL2_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p);
local real PiF_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real k);
local real M1_HS(struct cmdline_data* cmd,
                 struct global_data* gd, real eta);
local real M2_HS(struct cmdline_data* cmd, struct global_data* gd, real eta);
local real M3_HS(struct cmdline_data* cmd, struct global_data* gd, real eta);
local real S2a_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p);
local real S2b_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p);
local real S2FL_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p);
local real S2dI_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p);

local real sourcea_HS(struct cmdline_data* cmd,
                      struct global_data* gd, real eta, real kf);
local real sourceFL_HS(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf, real k1, real k2);
local real sourcedI_HS(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf, real k1, real k2);

local real S3IIplus_HS(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f);
local real S3IIminus_HS(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2mf);
local real S3FLplus_HS(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real x, real k, real p, real
                       Dpk, real Dpp, real D2f);
local real S3FLminus_HS(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real x, real k, real p,
                        real Dpk, real Dpp, real D2mf);
local real D2phiplus_HS(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real x, real k, real p,
                     real Dpk, real Dpp, real D2f);
local real D2phiminus_HS(struct cmdline_data* cmd,
                         struct global_data* gd, real eta, real x, real k, real p,
                      real Dpk, real Dpp, real D2mf);
local real K3dI_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k,  real p,
                 real Dpk, real Dpp, real D2f, real D2mf);
// End: Hu-Sawicky Model local HEADERS

local real mass_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real masstmp;
    
    masstmp = (1.0/invH0)*rsqrt(1.0/(2.0*rabs(cmd->fR0)))
                * rpow(cmd->om*rexp(-3.0*eta) + 4.0*(gd->ol),(2.0+cmd->nHS)/2.0)
                / rpow(cmd->om+4.0*(gd->ol),(1.0+cmd->nHS)/2.0);

    return (masstmp);
}

local real mu_HS(struct cmdline_data* cmd, struct global_data* gd, real eta, real k)
{
    real mutmp;
    mutmp = 1.0 + (2.0*gd->beta2*k*k)/(k*k + rexp(2.0*eta)*rsqr(mass_HS(cmd, gd, eta)));

    return (mutmp);
}

local real PiF_HS(struct cmdline_data* cmd, struct global_data* gd, real eta, real k)
{
    real PiFtmp;
    
    PiFtmp = k*k/rexp(2.0*eta) + rsqr(mass_HS(cmd, gd, eta));
    
    return (PiFtmp);
}


// ---------------------------------------------------------------------------
// BEGIN :: SECOND ORDER (six second order differential equations)
//

local real M2_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real M2tmp;

    M2tmp = cmd->screening;
    M2tmp *= (9.0/(4.0*rsqr(invH0)))*rsqr(1.0/rabs(cmd->fR0))
    * rpow(cmd->om*rexp(-3.0*eta)+4.0*(gd->ol),5.0)
    / rpow(cmd->om+4*(gd->ol),4.0);
    
    return (M2tmp);
}


local real KFL_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real k, real k1, real k2)
{
    real KFLtmp;

    KFLtmp = 0.5*(rsqr(sqr(k)-sqr(k1)-sqr(k2))/(sqr(k1)*sqr(k2)))*(mu_HS(cmd, gd, eta,k1)+mu_HS(cmd, gd, eta,k2)-2.0)
        + 0.5*((sqr(k)-sqr(k1)-sqr(k2))/sqr(k1))*(mu_HS(cmd, gd, eta,k1)-1.0)
        + 0.5*((sqr(k)-sqr(k1)-sqr(k2))/sqr(k2))*(mu_HS(cmd, gd, eta,k2)-1.0);

    return (KFLtmp);
}

local real sourceA_HS(struct cmdline_data* cmd,
                      struct global_data* gd, real eta, real kf, real k1, real k2)
{
    real Stmp;

    Stmp = sourcea_HS(cmd, gd, eta, kf)
            + sourceFL_HS(cmd, gd, eta, kf, k1, k2)
            - sourcedI_HS(cmd, gd, eta, kf, k1, k2);

    return Stmp;
}

local real sourcea_HS(struct cmdline_data* cmd,
                      struct global_data* gd, real eta, real kf)
{
    real Stmp;
    
    Stmp = f1(cmd, gd, eta)*mu_HS(cmd, gd, eta, kf);

    return Stmp;
}

local real sourceb_HS(struct cmdline_data* cmd,
                      struct global_data* gd, real eta, real kf, real k1, real k2)
{
    real Stmp;
    
    Stmp = f1(cmd, gd, eta)*( mu_HS(cmd, gd, eta, k1) + mu_HS(cmd, gd, eta, k2) - mu_HS(cmd, gd, eta, kf));

    return Stmp;
}

local real sourceFL_HS(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf, real k1, real k2)
{
    real Stmp;
    
    Stmp = f1(cmd, gd, eta)*(rsqr(mass_HS(cmd, gd, eta))/PiF_HS(cmd, gd, eta,kf))*KFL_HS(cmd, gd, eta, kf, k1, k2);

    return Stmp;
}

local real sourcedI_HS(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf, real k1, real k2)
{
    real Stmp;
    
    Stmp = (1.0/6.0)*rsqr(OmM_HS(cmd, gd, eta)*H_HS(cmd, gd, eta)/(rexp(eta)*invH0))
            * rsqr(kf)*M2_HS(cmd, gd, eta)/ ( PiF_HS(cmd, gd, eta,kf)*PiF_HS(cmd, gd, eta,k1)*PiF_HS(cmd, gd, eta,k2) );
    
    return Stmp;
}

//
// END :: SECOND ORDER (six second order differential equations)
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// BEGIN :: THIRD ORDER (Dsymmetric, five second order differential equations)
//

local real M1_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real M1tmp;
    
    M1tmp = 3.0*rsqr(mass_HS(cmd, gd, eta));
    
    return (M1tmp);
}

local real M3_HS(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real M3tmp;

    M3tmp = cmd->screening;
    M3tmp *= (45.0/(8.0*rsqr(invH0)))*rpow(1.0/rabs(cmd->fR0),3.0)
    * rpow(cmd->om*rexp(-3.0*eta)+4.0*(gd->ol),7.0)
    / rpow(cmd->om+4*(gd->ol),6.0);
    
    return (M3tmp);
}

local real KFL2_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p)
{
    real KFLtmp;
    
    KFLtmp = 2.0*rsqr(x)*( mu_HS(cmd, gd, eta,k) + mu_HS(cmd, gd, eta,p)-2.0 )
            + (p*x/k)*(mu_HS(cmd, gd, eta,k) - 1.0)
            + (k*x/p)*(mu_HS(cmd, gd, eta,p) - 1.0);

    return (KFLtmp);
}

local real JFL_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p)
{
    real JFLtmp;
    
    JFLtmp = (9.0/(2.0*A0(cmd, gd, eta)))
            * KFL2_HS(cmd, gd, eta, x, k, p) * PiF_HS(cmd, gd, eta, k) * PiF_HS(cmd, gd, eta, p);

    return (JFLtmp);
}

local real D2phiplus_HS(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real x, real k, real p,
                     real Dpk, real Dpp, real D2f)
{
    real D2tmp;
    
    D2tmp = (
             (1.0 + rsqr(x))
             -( 2.0*A0(cmd, gd, eta)/3.0 )
             * (
                ( M2_HS(cmd, gd, eta) + JFL_HS(cmd, gd, eta,x,k,p)*(3.0+2.0*cmd->omegaBD) )
                / (3.0*PiF_HS(cmd, gd, eta,k)*PiF_HS(cmd, gd, eta,p))
                )
             ) * Dpk*Dpp + D2f;
    
    return (D2tmp);
}

local real D2phiminus_HS(struct cmdline_data* cmd,
                         struct global_data* gd, real eta, real x, real k, real p,
                     real Dpk, real Dpp, real D2mf)
{
    real D2tmp;
    
    D2tmp = (
             (1.0 + rsqr(x))
             -( 2.0*A0(cmd, gd, eta)/3.0 )
             * (
                ( M2_HS(cmd, gd, eta) + JFL_HS(cmd, gd, eta,-x,k,p)*(3.0+2.0*cmd->omegaBD) )
                / (3.0*PiF_HS(cmd, gd, eta,k)*PiF_HS(cmd, gd, eta,p))
                )
             ) * Dpk*Dpp + D2mf;

    return (D2tmp);
}

local real K3dI_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k,  real p,
                 real Dpk, real Dpp, real D2f, real D2mf)
{
    real K3tmp, kplusp, kpluspm;
    real t1, t2, t3, t4, t5, t6;

    kplusp = kpp(x,k,p);
    kpluspm = kpp(-x,k,p);

    t1 = 2.0*rsqr(OmM_HS(cmd, gd, eta)*H_HS(cmd, gd, eta)/invH0)
            *(M2_HS(cmd, gd, eta)/(PiF_HS(cmd, gd, eta,k)*PiF_HS(cmd, gd, eta,0)));

    t2 = (1.0/3.0)*(rpow(OmM_HS(cmd, gd, eta),3.0)*rpow(H_HS(cmd, gd, eta),4.0)/rpow(invH0,4) )
        *(
            M3_HS(cmd, gd, eta) - M2_HS(cmd, gd, eta)*(M2_HS(cmd, gd, eta) + JFL_HS(cmd, gd, eta,-1.0,p,p)*(3.0+2.0*cmd->omegaBD))
                        /(PiF_HS(cmd, gd, eta,0))
          ) / ( rsqr(PiF_HS(cmd, gd, eta,p)) * PiF_HS(cmd, gd, eta,k) );

    t3 = rsqr(OmM_HS(cmd, gd, eta)*H_HS(cmd, gd, eta)/invH0)
        *(M2_HS(cmd, gd, eta)/(PiF_HS(cmd, gd, eta,p)*PiF_HS(cmd, gd, eta,kplusp)))
        *(
            1.0 + rsqr(x) + (D2f)/(Dpk*Dpp)
          );
    
    t4 = (1.0/3.0)*(rpow(OmM_HS(cmd, gd, eta),3.0)*rpow(H_HS(cmd, gd, eta),4.0)/rpow(invH0,4) )
        *(
            M3_HS(cmd, gd, eta) - M2_HS(cmd, gd, eta)*(M2_HS(cmd, gd, eta) + JFL_HS(cmd, gd, eta,x,k,p)*(3.0+2.0*cmd->omegaBD))
                        /(PiF_HS(cmd, gd, eta,kplusp))
          ) / ( rsqr(PiF_HS(cmd, gd, eta,p)) * PiF_HS(cmd, gd, eta,k) );
    
    t5 = rsqr(OmM_HS(cmd, gd, eta)*H_HS(cmd, gd, eta)/invH0)
        *(M2_HS(cmd, gd, eta)/(PiF_HS(cmd, gd, eta,p)*PiF_HS(cmd, gd, eta,kpluspm)))
        *(
            1.0 + rsqr(x) + (D2mf)/(Dpk*Dpp)
          );
    
    t6 = (1.0/3.0)*(rpow(OmM_HS(cmd, gd, eta),3.0)*rpow(H_HS(cmd, gd, eta),4.0)/rpow(invH0,4) )
        *(
          M3_HS(cmd, gd, eta) - M2_HS(cmd, gd, eta)*(M2_HS(cmd, gd, eta) + JFL_HS(cmd, gd, eta,-x,k,p)*(3.0+2.0*cmd->omegaBD))
                /(PiF_HS(cmd, gd, eta,kpluspm))
          ) / ( rsqr(PiF_HS(cmd, gd, eta,p)) * PiF_HS(cmd, gd, eta,k) );

    K3tmp = t1 + t2 + t3 + t4 + t5 + t6;

    return (K3tmp);
}

local real S2a_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp, kplusp;

    kplusp = kpp(x,k,p);
    Dtmp = f1(cmd, gd, eta)*mu_HS(cmd, gd,eta,kplusp);

    return Dtmp;
}

local real S2b_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp, kplusp;
    
    kplusp = kpp(x,k,p);
    Dtmp = f1(cmd, gd, eta)*(mu_HS(cmd, gd, eta,k)+mu_HS(cmd, gd, eta,p)-mu_HS(cmd, gd, eta,kplusp));
    
    return Dtmp;
}

local real S2FL_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp, kplusp;
    
    kplusp = kpp(x,k,p);
    Dtmp = f1(cmd, gd, eta)*(
                    M1_HS(cmd, gd, eta)/(3.0*PiF_HS(cmd, gd, eta,kplusp))
                    *KFL2_HS(cmd, gd, eta,x,k,p)
                    );
    return Dtmp;
}

local real S2dI_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp, kplusp;
    
    kplusp = kpp(x,k,p);
    Dtmp = (1.0/6.0)*
            rsqr(OmM_HS(cmd, gd, eta)*H_HS(cmd, gd, eta)/(rexp(eta)*invH0))
        * ( (rsqr(kplusp)*M2_HS(cmd, gd, eta)) / (PiF_HS(cmd, gd, eta,kplusp)*PiF_HS(cmd, gd, eta,k)*PiF_HS(cmd, gd, eta,p)));

    return Dtmp;
}

local real SD2_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp;

    Dtmp = S2a_HS(cmd, gd, eta, x, k, p) -  S2b_HS(cmd, gd, eta, x, k, p)*rsqr(x)
        + S2FL_HS(cmd, gd, eta, x, k, p) - S2dI_HS(cmd, gd, eta, x, k, p);

    return Dtmp;
}

local real S3I_HS(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp,
                real D2f, real D2mf)
{
    real Stmp, kplusp, kpluspm;

    kplusp = kpp(x,k,p);
    kpluspm = kpp(-x,k,p);
    Stmp = (
            f1(cmd, gd, eta)*(mu_HS(cmd, gd, eta,p)+mu_HS(cmd, gd, eta,kplusp)-mu_HS(cmd, gd, eta,k))*D2f*Dpp
                + SD2_HS(cmd, gd, eta,x,k,p)*Dpk*Dpp*Dpp
            )*(1.0 - rsqr(x))/(1.0 + rsqr(p/k) + 2.0*(p/k)*x)
        + (
           f1(cmd, gd, eta)*(mu_HS(cmd, gd, eta,p)+mu_HS(cmd, gd, eta,kpluspm)-mu_HS(cmd, gd, eta,k))*D2mf*Dpp
            + SD2_HS(cmd, gd, eta,-x,k,p)*Dpk*Dpp*Dpp
           )*(1.0 - rsqr(x))/(1.0 + rsqr(p/k) - 2.0*(p/k)*x);

    return (Stmp);
}

local real S3IIplus_HS(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f)
{
    real Stmp, kplusp;
    
    kplusp = kpp(x,k,p);
    
    Stmp =
    -f1(cmd, gd, eta)*(mu_HS(cmd, gd, eta,p)+mu_HS(cmd, gd, eta,kplusp)-2.0*mu_HS(cmd, gd, eta,k))
    * Dpp*( D2f + Dpk*Dpp*rsqr(x) )
    
    -f1(cmd, gd, eta)*(mu_HS(cmd, gd, eta,kplusp)-mu_HS(cmd, gd, eta,k))*Dpk*Dpp*Dpp
    
    -(
      (M1_HS(cmd, gd, eta)/(3.0*PiF_HS(cmd, gd, eta,kplusp))) * f1(cmd, gd, eta)*KFL2_HS(cmd, gd, eta,x,k,p)
      -rsqr(OmM_HS(cmd, gd, eta)*H_HS(cmd, gd, eta)/invH0)
      * (M2_HS(cmd, gd, eta)*kplusp*kplusp*rexp(-2.0*eta))
      / (6.0*PiF_HS(cmd, gd, eta,kplusp)*PiF_HS(cmd, gd, eta,k)*PiF_HS(cmd, gd, eta,p))
      )*Dpk*Dpp*Dpp;
    
    return (Stmp);
}

local real S3IIminus_HS(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2mf)
{
    real Stmp, kpluspm;

    kpluspm = kpp(-x,k,p);

    Stmp =
    -f1(cmd, gd, eta)*(mu_HS(cmd, gd, eta,p)+mu_HS(cmd, gd, eta,kpluspm)-2.0*mu_HS(cmd, gd, eta,k))
    * Dpp*( D2mf + Dpk*Dpp*rsqr(x) )
    
    -f1(cmd, gd, eta)*(mu_HS(cmd, gd, eta,kpluspm)-mu_HS(cmd, gd, eta,k))*Dpk*Dpp*Dpp
    
    -(
      (M1_HS(cmd, gd, eta)/(3.0*PiF_HS(cmd, gd, eta,kpluspm))) * f1(cmd, gd, eta)*KFL2_HS(cmd, gd, eta,-x,k,p)
      -rsqr(OmM_HS(cmd, gd, eta)*H_HS(cmd, gd, eta)/invH0)
      * (M2_HS(cmd, gd, eta)*kpluspm*kpluspm*rexp(-2.0*eta))
      / (6.0*PiF_HS(cmd, gd, eta,kpluspm)*PiF_HS(cmd, gd, eta,k)*PiF_HS(cmd, gd, eta,p))
      )*Dpk*Dpp*Dpp;
    
    return (Stmp);
}

local real S3II_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f, real D2mf)
{
    real Stmp;
    
    Stmp =  S3IIplus_HS(cmd, gd, eta, x, k, p, Dpk, Dpp, D2f)
         + S3IIminus_HS(cmd, gd, eta, x, k, p, Dpk, Dpp, D2mf);

    return (Stmp);
}

local real S3FLplus_HS(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f)
{
    real Stmp, kplusp;
    
    kplusp = kpp(x,k,p);
    
    Stmp = f1(cmd, gd, eta)*(M1_HS(cmd, gd, eta)/(3.0*PiF_HS(cmd, gd, eta,k)))
    *(
        (2.0*rsqr(p+k*x)/rsqr(kplusp) - 1.0 - (k*x)/p )
        *( mu_HS(cmd, gd, eta,p)-1.0 )* D2f * Dpp
      
        + ( (rsqr(p) + 3.0*k*p*x + 2.0*k*k * x*x)/rsqr(kplusp) )
        *( mu_HS(cmd, gd, eta,kplusp) - 1.0) * D2phiplus_HS(cmd, gd, eta,x,k,p,Dpk,Dpp,D2f) * Dpp
      
      + 3.0*rsqr(x)*( mu_HS(cmd, gd, eta,k) + mu_HS(cmd, gd, eta,p) - 2.0 ) * Dpk * Dpp * Dpp
    );
    
    return (Stmp);
}

local real S3FLminus_HS(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2mf)
{
    real Stmp, kpluspm;
    
    kpluspm = kpp(-x,k,p);
    
    Stmp = f1(cmd, gd, eta)*(M1_HS(cmd, gd, eta)/(3.0*PiF_HS(cmd, gd, eta,k)))
    *(
      (2.0*rsqr(p-k*x)/rsqr(kpluspm) - 1.0 + (k*x)/p )
      *( mu_HS(cmd, gd, eta,p)-1.0 )* D2mf * Dpp
      
      + ( (rsqr(p) - 3.0*k*p*x + 2.0*k*k * x*x)/rsqr(kpluspm) )
      *( mu_HS(cmd, gd, eta,kpluspm) - 1.0) * D2phiminus_HS(cmd, gd, eta,x,k,p,Dpk,Dpp,D2mf) * Dpp
      
      + 3.0*rsqr(x)*( mu_HS(cmd, gd, eta,k) + mu_HS(cmd, gd, eta,p) - 2.0 ) * Dpk * Dpp * Dpp
      );
    
    return (Stmp);
}

local real S3FL_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p, real
                   Dpk, real Dpp, real D2f, real D2mf)
{
    real Stmp;

    Stmp = S3FLplus_HS(cmd, gd, eta, x, k, p, Dpk, Dpp, D2f)
        + S3FLminus_HS(cmd, gd, eta, x, k, p, Dpk, Dpp, D2mf);

    return (Stmp);
}

local real S3dI_HS(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp,
                 real D2f, real D2mf)
{
    real Stmp;
    
    Stmp = -(rsqr(k)/rexp(2.0*eta))
        *(1.0/(6.0*PiF_HS(cmd, gd, eta,k)))
        *K3dI_HS(cmd, gd, eta,x,k,p,Dpk,Dpp,D2f,D2mf)*Dpk*Dpp*Dpp;

    return (Stmp);
}

//
// END :: THIRD ORDER (Dsymmetric, five second order differential equations)
// ---------------------------------------------------------------------------

static int initialize_model(struct cmdline_data *cmd, struct global_data *gd)
{
    if (cmd->fR0 == 0.0) {
        snprintf(cmd->error_message, sizeof(cmd->error_message),
                 "fR0 must be nonzero for the %s kernel solver", cmd->mgmodel);
        return FAILURE;
    }

    gd->beta2 = 1.0 / 6.0;
    cmd->omegaBD = 0.0;
    cmd->nHS = 1;
    return SUCCESS;
}

static const struct fkpt2_gravity_model fkpt2_hs_model = {
    .name = "HS",
    .description = "Hu–Sawicki f(R), n=1",
    .hooks = {
        .OmM = OmM_HS,
        .H = H_HS,
        .f1 = f1_HS,
        .f2 = f2_HS,
        .A0 = A0_HS,
        .mu = mu_HS,
        .sourceA = sourceA_HS,
        .sourceb = sourceb_HS,
        .SD2 = SD2_HS,
        .S3I = S3I_HS,
        .S3II = S3II_HS,
        .S3FL = S3FL_HS,
        .S3dI = S3dI_HS,
    },
    .initialize = initialize_model,
    .parallel_growth = true,
};

const struct fkpt2_addon fkpt2_hu_sawicki_addon = {
    .name = "hu_sawicki",
    .model = &fkpt2_hs_model,
};
