/*==============================================================================
	DGP growth and screening sources from the MGPT implementation.
==============================================================================*/
#include "fkpt2_addon.h"

local real OmM_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta);
local  real H_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta);
local  real f1_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta);
local  real f2_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta);
local  real A0_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta);
local real mu_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta, real k);
local real sourceA_DGP(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf, real k1, real k2);
local real sourceb_DGP(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf, real k1, real k2);
local real SD2_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p);
local real S3I_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p, real
                   Dpk, real Dpp,
                  real D2f, real D2mf);
local real S3II_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k, real p, real
                    Dpk, real Dpp,
                   real D2f, real D2mf);
local real S3FL_DGP(struct cmdline_data* cmd, struct global_data* gd,
                    real eta, real x, real k, real p, real Dpk, real Dpp, real D2f,
                    real D2mf);
local real S3dI_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp,
                   real D2f, real D2mf);

local real OmM_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real OmMtmp;
    
    OmMtmp=1.0/(1.0 + ( (gd->ol)/cmd->om)*rexp(3.0*eta) );
    return (OmMtmp);
}

local  real H_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real Htmp;
    
    Htmp=rsqrt(cmd->om*rexp(-3.0*eta)+(gd->ol));
    return (Htmp);
}


local  real f1_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real f1tmp;
    
    f1tmp=3.0/(2.0*(1.0
                    + ((gd->ol)/cmd->om)*rexp(3.0*eta)
                    )
               );
    return (f1tmp);
}

local  real f2_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real f2tmp;
    
    f2tmp=2.0 - 3.0/(2.0*(1.0
                          + ((gd->ol)/cmd->om)*rexp(3.0*eta)
                          )
                     );
    return (f2tmp);
}

local real A0_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real A0tmp;
    
    A0tmp = 1.5*OmM_DGP(cmd, gd, eta)*rsqr(H_DGP(cmd, gd, eta))/rsqr(invH0);
    
    return (A0tmp);
}
// End :: mglpt_fns

// Begin: DGP Model local HEADERS
local real mass_DGP(real eta);
local real beta2_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta);
local real JFL_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p);
local real KFL_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real k, real k1, real k2);
local real KFL2_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k, real p);
local real PiF_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real k);
local real M1_DGP(real eta);
local real M2_DGP(struct cmdline_data* cmd, real eta, real k, real k1, real k2);
local real M3_DGP(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p);
local real S2a_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p);
local real S2b_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p);
local real S2FL_DGP(real eta, real x, real k, real p);
local real S2dI_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k, real p);

local real sourcea_DGP(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf);
local real sourceFL_DGP(real eta, real kf, real k1, real k2);
local real sourcedI_DGP(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real kf, real k1, real k2);

local real S3IIplus_DGP(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f);
local real S3IIminus_DGP(struct cmdline_data* cmd,
                         struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2mf);
local real S3FLplus_DGP(real eta, real x, real k, real p, real Dpk, real Dpp, real D2f);
local real S3FLminus_DGP(real eta, real x, real k, real p, real Dpk, real Dpp, real D2mf);
local real D2phiplus_DGP(real eta, real x, real k, real p,
                         real Dpk, real Dpp, real D2f);
local real D2phiminus_DGP(real eta, real x, real k, real p,
                          real Dpk, real Dpp, real D2mf);
local real K3dI_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k,  real p,
                    real Dpk, real Dpp, real D2f, real D2mf);
// End: DGP Model local HEADERS

local real mass_DGP(real eta)  // =0 in DGP
{
    real masstmp;
    
    masstmp = 0;
    
    return (masstmp);
}

// This is function \beta^2 in paper2
// Related to 3+2\omega_{BD} = 1/(2\beta^2) in paper1
local real beta2_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    real beta2tmp;
    
    //~ beta2tmp = 1.0/6.0; //This is for f(R)
    
    real  betadgp;
    // betadgp is \beta_{DGP} commonly named in the DGP literature simply as \beta
    // but it is not the same \beta as in other MG papers as paper2. 
    betadgp = 1.0 - 2.0 * cmd->eps_DGP * cmd->rc_DGP * H_DGP(cmd, gd, eta) *
    (1.0 - 1.0/(2.0*(1.0 + gd->ol/cmd->om * rexp(3.0*eta) ) ) );

    
    beta2tmp = 1.0 / (6.0*betadgp);
    
    return (beta2tmp);
}


local real mu_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta, real k)
{
    //~ real mutmp;
    //~ mutmp = 1.0 + (2.0*gd->beta2*k*k)/(k*k + rexp(2.0*eta)*rsqr(mass_DGP(eta)));
    
    //~ return (mutmp);
    
    real mutmp;
    
    
    mutmp = 1.0 + 2.0 * beta2_DGP(cmd, gd, eta);
    
    return (mutmp);
}

local real PiF_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta, real k)
{
    real PiFtmp;
    
    PiFtmp =  k*k * rexp(-2.0*eta) /6.0 / beta2_DGP(cmd, gd, eta) ;
    
    return (PiFtmp);
}


// ---------------------------------------------------------------------------
// BEGIN :: SECOND ORDER (six second order differential equations)
//

local real M2_DGP(struct cmdline_data* cmd, real eta, real kf, real k1, real k2)
{
    real M2tmp;
    
    M2tmp  = cmd->screening;
    M2tmp *= 2.0 * rsqr( cmd->rc_DGP * invH0 / cmd->h ) * rexp( -4.0 * eta)
    * ( rsqr(k1)*rsqr(k2)
       - 0.25 * rsqr( -rsqr(k1) - rsqr(k2) + rsqr(kf) ) )  ;
    
    
    return (M2tmp);
}


local real KFL_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real k, real k1, real k2)
{
    real KFLtmp;
    
    KFLtmp =  rsqr(sqr(k)-sqr(k1)-sqr(k2)) / (sqr(k1)*sqr(k2)) * (mu_DGP(cmd, gd, eta,k1) - 1.0)
        + 0.5 * (sqr(k)-sqr(k1)-sqr(k2)) / sqr(k1) * (mu_DGP(cmd, gd, eta,k1)-1.0)
        + 0.5 * (sqr(k)-sqr(k1)-sqr(k2)) / sqr(k2) * (mu_DGP(cmd, gd, eta,k2)-1.0);
    
    return (KFLtmp);
}

local real sourceA_DGP(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf, real k1, real k2)
{
    real Stmp;
    
    Stmp = sourcea_DGP(cmd, gd, eta, kf)
    //~ + sourceFL_DGP(eta, kf, k1, k2)
    - sourcedI_DGP(cmd, gd, eta, kf, k1, k2);
    
    return Stmp;
}

local real sourcea_DGP(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf)
{
    real Stmp;
    real anyk;
    
    anyk=1.0;
    
    Stmp = f1(cmd, gd, eta)*mu_DGP(cmd, gd, eta, anyk);
    
    return Stmp;
}

local real sourceb_DGP(struct cmdline_data* cmd,
                       struct global_data* gd, real eta, real kf, real k1, real k2)
{
    real Stmp;
    real anyk;
    
    anyk=1.0;
    
    Stmp = f1(cmd, gd, eta)*mu_DGP(cmd, gd, eta, anyk);
    
    return Stmp;
}

local real sourceFL_DGP(real eta, real kf, real k1, real k2) //This is zero in DGP
{
    real Stmp;
    
    Stmp = 0;
    
    return Stmp;
}

local real sourcedI_DGP(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real kf, real k1, real k2)
{
    real Stmp;
    
    Stmp = (1.0/6.0)*rsqr( OmM_DGP(cmd, gd, eta)*H_DGP(cmd, gd, eta) / (rexp(eta)*invH0) )
    * rsqr(kf)*M2_DGP(cmd, eta, kf, k1, k2)/ ( PiF_DGP(cmd, gd, eta,kf)*PiF_DGP(cmd, gd, eta,k1)*PiF_DGP(cmd, gd, eta,k2) );
    
    return Stmp;
}

//
// END :: SECOND ORDER (six second order differential equations)
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// BEGIN :: THIRD ORDER (Dsymmetric, five second order differential equations)
//

local real M1_DGP(real eta) //This is zero in DGP
{
    real M1tmp;
    
    M1tmp = 0;
    
    return (M1tmp);
}

local real M3_DGP(struct cmdline_data* cmd,
                  struct global_data* gd, real eta, real x, real k, real p)
{
    real M3tmp;
    
    M3tmp = cmd->screening;
    M3tmp *= rsqr( cmd->rc_DGP * invH0 / cmd->h ) * rexp(-6.0*eta)
    / beta2_DGP(cmd, gd, eta) / A0_DGP(cmd, gd, eta);
    M3tmp *= (k*k * p*p*p*p * (1.0 - x*x) - k*k*k * p*p*p * x * (1.0 - x*x) ); //the last term contributes zero to R1
    
    return (M3tmp);
}


local real KFL2_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k, real p)
{
    real KFLtmp;
    
    KFLtmp = 4.0*rsqr(x)*( mu_DGP(cmd, gd, eta,k) - 1.0 )
    + (p*x/k)*(mu_DGP(cmd, gd, eta,k) - 1.0)
    + (k*x/p)*(mu_DGP(cmd, gd, eta,p) - 1.0);
    
    return (KFLtmp);
}

local real JFL_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p)
{
    real JFLtmp;
    
    JFLtmp = 9.0 / ( 2.0 * A0_DGP(cmd, gd, eta) )
    * KFL2_DGP(cmd, gd, eta, x, k, p) * PiF_DGP(cmd, gd, eta, k) * PiF_DGP(cmd, gd, eta, p);
    
    return (JFLtmp);
}


// Do not contribute in DGP
local real D2phiplus_DGP(real eta, real x, real k, real p,
                         real Dpk, real Dpp, real D2f)
{
    real D2tmp;
    D2tmp = 0;
    //~ real omBD;
    //~ real kplusp;
    
    //~ kplusp=kpp(x,k,p);
    
    //~ omBD = 1.0/(4.0 * beta2_DGP(eta)) - 1.5;
    
    //~ D2tmp = (
    //~ (1.0 + rsqr(x))
    //~ -( 2.0*A0_DGP(eta)/3.0 )
    //~ * (
    //~ ( M2_DGP(eta,kplusp,k,p) + JFL_DGP(eta,x,k,p)*(3.0+2.0*omBD) )
    //~ / (3.0*PiF_DGP(eta,k)*PiF_DGP(eta,p))
    //~ )
    //~ ) * Dpk*Dpp + D2f;
    
    return (D2tmp);
}

// Do not contribute in DGP
local real D2phiminus_DGP(real eta, real x, real k, real p,
                          real Dpk, real Dpp, real D2mf)
{
    real D2tmp;
    D2tmp=0;
       
    //~ real omBD;
    //~ real kpluspm;
    
    //~ kpluspm=kpp(-x,k,p);
        
    //~ omBD = 1.0/(4.0 * beta2_DGP(eta)) - 1.5;
    
    
    //~ D2tmp = (
    //~ (1.0 + rsqr(x))
    //~ -( 2.0*A0_DGP(eta)/3.0 )
    //~ * (
    //~ ( M2_DGP(eta,kpluspm,k,p) + JFL_DGP(eta,-x,k,p)*(3.0+2.0*omBD) )
    //~ / (3.0*PiF_DGP(eta,k)*PiF_DGP(eta,p))
    //~ )
    //~ ) * Dpk*Dpp + D2mf;
    
    return (D2tmp);
}

local real K3dI_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k,  real p,
                    real Dpk, real Dpp, real D2f, real D2mf)
{

    
    
    real K3tmp, kplusp, kpluspm;
    real t1, t2, t3, t4, t5, t6;
    real zero;
    real omBD;
    
    real func1, func2;
    
    omBD = 1.0/ ( 4.0 * beta2_DGP(cmd, gd, eta) ) - 1.5;
    
    kplusp = kpp(x,k,p);
    kpluspm = kpp(-x,k,p);
    zero=0.000000001;
    
    func1=rsqr(OmM_DGP(cmd, gd, eta)*H_DGP(cmd, gd, eta)/invH0);
    func2=rpow(OmM_DGP(cmd, gd, eta),3.0)*rpow(H_DGP(cmd, gd, eta),4.0)/rpow(invH0,4);
    
    t1 = 2.0*func1
    *(M2_DGP(cmd, eta,k,k,0.)/(PiF_DGP(cmd, gd, eta,k)*PiF_DGP(cmd, gd, eta,zero)));
    
    t2 = (1.0/3.0)*func2
    *(
      M3_DGP(cmd, gd, eta,x,k,p) - M2_DGP(cmd, eta,k,k,0)*(M2_DGP(cmd, eta,0,p,p)+ JFL_DGP(cmd, gd, eta,-1.0,p,p)*(3.0+2.0*omBD) )
      / PiF_DGP(cmd, gd, eta,zero)
      ) / ( rsqr(PiF_DGP(cmd, gd, eta,p)) * PiF_DGP(cmd, gd, eta,k) ) ;
    
    t3 = func1
    *( M2_DGP(cmd, eta,k,p,kplusp)/( PiF_DGP(cmd, gd, eta,p)*PiF_DGP(cmd, gd, eta,kplusp) ) )
    *(
      1.0 + rsqr(x) + D2f / (Dpk*Dpp)
      );
    
    t4 = (1.0/3.0)*func2
    *(
      M3_DGP(cmd, gd, eta,x,k,p) - M2_DGP(cmd, eta,k,p,kplusp)*(M2_DGP(cmd, eta,kplusp,k,p)+ JFL_DGP(cmd, gd, eta,x,k,p)*(3.0+2.0*omBD))
      / PiF_DGP(cmd, gd, eta,kplusp)
      ) / ( rsqr(PiF_DGP(cmd, gd, eta,p)) * PiF_DGP(cmd, gd, eta,k) );
    
    t5 = func1
    *( M2_DGP(cmd, eta,k,p,kpluspm) / ( PiF_DGP(cmd, gd, eta,p)*PiF_DGP(cmd, gd, eta,kpluspm) ) )
    *(
      1.0 + rsqr(x) + D2f /(Dpk*Dpp)   // BORRAR:  Check D2f
      );
    
    t6 = (1.0/3.0)*func2
    *(
      M3_DGP(cmd, gd, eta,x,k,p) -  M2_DGP(cmd, eta,k,p,kpluspm) * ( M2_DGP(cmd, eta,kpluspm,k,p)+ JFL_DGP(cmd, gd, eta,-x,k,p)*(3.0+2.0*omBD) )
      / PiF_DGP(cmd, gd, eta,kpluspm)
      ) / ( rsqr(PiF_DGP(cmd, gd, eta,p)) * PiF_DGP(cmd, gd, eta,k) );
    
    K3tmp = t1 + t2 + t3 + t4 + t5 + t6;
    
    
    
    
    return (K3tmp);
}

local real S2a_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp;
    
    //~ kplusp = kpp(x,k,p);
    Dtmp = f1(cmd, gd, eta)*mu_DGP(cmd, gd, eta,1);
    
    return Dtmp;
}

local real S2b_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp, kplusp;
    
    kplusp = kpp(x,k,p);
    Dtmp = f1(cmd, gd, eta)*mu_DGP(cmd, gd, eta,1);
    
    return Dtmp;
}

local real S2FL_DGP(real eta, real x, real k, real p)
{
    real Dtmp, kplusp;
    
    
    Dtmp = 0;
    
    return Dtmp;
}

local real S2dI_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp, kplusp;
    
    kplusp = kpp(x,k,p);
    Dtmp = (1.0/6.0)*
    rsqr(OmM_DGP(cmd, gd, eta)*H_DGP(cmd, gd, eta)/(rexp(eta)*invH0))
    * ( (rsqr(kplusp)*M2_DGP(cmd, eta,kplusp,k,p)) / (PiF_DGP(cmd, gd, eta,kplusp)*PiF_DGP(cmd, gd, eta,k)*PiF_DGP(cmd, gd, eta,p)));
    
    return Dtmp;
}

local real SD2_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta, real x, real k, real p)
{
    real Dtmp;
    
    Dtmp = S2a_DGP(cmd, gd, eta, x, k, p) -  S2b_DGP(cmd, gd, eta, x, k, p)*rsqr(x)
    + S2FL_DGP(eta, x, k, p) - S2dI_DGP(cmd, gd, eta, x, k, p);
    
    return Dtmp;
}

local real S3I_DGP(struct cmdline_data* cmd,
                   struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp,
                   real D2f, real D2mf)
{
    real Stmp, kplusp, kpluspm;
    
    kplusp = kpp(x,k,p);
    kpluspm = kpp(-x,k,p);
    Stmp = (
            f1(cmd, gd, eta)*(mu_DGP(cmd, gd, eta,p)+mu_DGP(cmd, gd, eta,kplusp)-mu_DGP(cmd, gd, eta,k))*D2f*Dpp
            + SD2_DGP(cmd, gd, eta,x,k,p)*Dpk*Dpp*Dpp
            )*(1.0 - rsqr(x))/(1.0 + rsqr(p/k) + 2.0*(p/k)*x)
    + (
       f1(cmd, gd, eta)*(mu_DGP(cmd, gd, eta,p)+mu_DGP(cmd, gd, eta,kpluspm)-mu_DGP(cmd, gd, eta,k))*D2mf*Dpp
       + SD2_DGP(cmd, gd, eta,-x,k,p)*Dpk*Dpp*Dpp
       )*(1.0 - rsqr(x))/(1.0 + rsqr(p/k) - 2.0*(p/k)*x);
    
    return (Stmp);
}



local real S3IIplus_DGP(struct cmdline_data* cmd,
                        struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f)
{

    real Stmp, kplusp;

    
    kplusp = kpp(x,k,p);
    
    
    //~ Stmp =
    //~ -f1_DGP(eta)*(mu_DGP(eta,p)+mu_DGP(eta,kplusp)-2.0*mu_DGP(eta,k))
    //~ * Dpp*( D2f + Dpk*Dpp*rsqr(x) )
    
    //~ -f1_DGP(eta)*(mu_DGP(eta,kplusp)-mu_DGP(eta,k))*Dpk*Dpp*Dpp
    
    //~ -(
      //~ (M1_DGP(eta)/(3.0*PiF_DGP(eta,kplusp))) * f1_DGP(eta)*KFL2_DGP(eta,x,k,p)
      //~ -rsqr(OmM_DGP(eta)*H_DGP(eta)/invH0)
      //~ * (M2_DGP(eta,kplusp,k,p)*kplusp*kplusp*rexp(-2.0*eta))
      //~ / (6.0*PiF_DGP(eta,kplusp)*PiF_DGP(eta,k)*PiF_DGP(eta,p))
      //~ )*Dpk*Dpp*Dpp;
      
      Stmp = 
      -(
      0.0 - rsqr(OmM_DGP(cmd, gd, eta)*H_DGP(cmd, gd, eta)/invH0)
      * ( M2_DGP(cmd, eta,kplusp,k,p) * kplusp * kplusp * rexp(-2.0*eta) )
      / ( 6.0 * PiF_DGP(cmd, gd, eta,kplusp) * PiF_DGP(cmd, gd, eta,k) * PiF_DGP(cmd, gd, eta,p) )
      ) *Dpk*Dpp*Dpp;
      
    return (Stmp);
}


local real S3IIminus_DGP(struct cmdline_data* cmd,
                         struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2mf)
{

    real Stmp, kpluspm;
    
    kpluspm = kpp(-x,k,p);
    
    //~ Stmp =
    //~ -f1_DGP(eta)*(mu_DGP(eta,p)+mu_DGP(eta,kpluspm)-2.0*mu_DGP(eta,k))
    //~ * Dpp*( D2mf + Dpk*Dpp*rsqr(x) )
    
    //~ -f1_DGP(eta)*(mu_DGP(eta,kpluspm)-mu_DGP(eta,k))*Dpk*Dpp*Dpp
    
    //~ -(
      //~ (M1_DGP(eta)/(3.0*PiF_DGP(eta,kpluspm))) * f1_DGP(eta)*KFL2_DGP(eta,-x,k,p)
      //~ -rsqr(OmM_DGP(eta)*H_DGP(eta)/invH0)
      //~ * (M2_DGP(eta,kpluspm,k,p)*kpluspm*kpluspm*rexp(-2.0*eta))
      //~ / (6.0*PiF_DGP(eta,kpluspm)*PiF_DGP(eta,k)*PiF_DGP(eta,p))
      //~ )*Dpk*Dpp*Dpp;
      
      Stmp=
      -(
      0.0 -rsqr(OmM_DGP(cmd, gd, eta)*H_DGP(cmd, gd, eta)/invH0)
      * ( M2_DGP(cmd, eta,kpluspm,k,p) * kpluspm * kpluspm * rexp(-2.0*eta) )
      / ( 6.0 * PiF_DGP(cmd, gd, eta,kpluspm) * PiF_DGP(cmd, gd, eta,k) * PiF_DGP(cmd, gd, eta,p) )
      ) *Dpk*Dpp*Dpp;
      
    
    return (Stmp);
}


local real S3II_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k, real p, real
                    Dpk, real Dpp, real D2f, real D2mf)
{
    real Stmp;
    
    Stmp =  S3IIplus_DGP(cmd, gd, eta, x, k, p, Dpk, Dpp, D2f)
    + S3IIminus_DGP(cmd, gd, eta, x, k, p, Dpk, Dpp, D2mf);
    //~ Stmp =  0;
    
    return (Stmp);
}


//The next is zero for DGP
local real S3FLplus_DGP(real eta, real x, real k, real p, real Dpk, real Dpp, real D2f)
{
    real Stmp, kplusp;
    Stmp=0;
    
    //~ kplusp = kpp(x,k,p);
    
    //~ Stmp = f1(eta)*(M1_DGP(eta)/(3.0*PiF_DGP(eta,k)))
    //~ *(
      //~ (2.0*rsqr(p+k*x)/rsqr(kplusp) - 1.0 - (k*x)/p )
      //~ *( mu_DGP(eta,p)-1.0 )* D2f * Dpp
      
      //~ + ( (rsqr(p) + 3.0*k*p*x + 2.0*k*k * x*x)/rsqr(kplusp) )
      //~ *( mu_DGP(eta,kplusp) - 1.0) * D2phiplus_DGP(eta,x,k,p,Dpk,Dpp,D2f) * Dpp
      
      //~ + 3.0*rsqr(x)*( mu_DGP(eta,k) + mu_DGP(eta,p) - 2.0 ) * Dpk * Dpp * Dpp
      //~ );
    
    return (Stmp);
}


//The next is zero for DGP
local real S3FLminus_DGP(real eta, real x, real k, real p, real Dpk, real Dpp, real D2mf)
{
    real Stmp, kpluspm;
    Stmp=0;
    
    //~ kpluspm = kpp(-x,k,p);
    
    //~ Stmp = f1(eta)*(M1_DGP(eta)/(3.0*PiF_DGP(eta,k)))
    //~ *(
      //~ (2.0*rsqr(p-k*x)/rsqr(kpluspm) - 1.0 + (k*x)/p )
      //~ *( mu_DGP(eta,p)-1.0 )* D2mf * Dpp
      
      //~ + ( (rsqr(p) - 3.0*k*p*x + 2.0*k*k * x*x)/rsqr(kpluspm) )
      //~ *( mu_DGP(eta,kpluspm) - 1.0) * D2phiminus_DGP(eta,x,k,p,Dpk,Dpp,D2mf) * Dpp
      
      //~ + 3.0*rsqr(x)*( mu_DGP(eta,k) + mu_DGP(eta,p) - 2.0 ) * Dpk * Dpp * Dpp
      //~ );
    
    return (Stmp);
}


//The next is zero for DGP
local real S3FL_DGP(struct cmdline_data* cmd, struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f, real D2mf)
{
    real Stmp;
    
    //uncomment for general models
    //~ Stmp = S3FLplus_DGP(eta, x, k, p, Dpk, Dpp, D2f)
    //~ + S3FLminus_DGP(eta, x, k, p, Dpk, Dpp, D2mf);
    Stmp =0;
    
    return (Stmp);
}

local real S3dI_DGP(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp,
                    real D2f, real D2mf)
{
    real Stmp;
    
    Stmp = - rsqr(k) * rexp(-2.0*eta) / (6.0 * PiF_DGP(cmd, gd, eta,k))
    * K3dI_DGP(cmd, gd, eta,x,k,p,Dpk,Dpp,D2f,D2mf) * Dpk * Dpp * Dpp;
    
    
    return (Stmp);
}




//
// END :: THIRD ORDER (Dsymmetric, five second order differential equations)

static const struct fkpt2_gravity_model fkpt2_dgp_model = {
    .name = "DGP",
    .description = "Dvali–Gabadadze–Porrati",
    .hooks = {
        .OmM = OmM_DGP,
        .H = H_DGP,
        .f1 = f1_DGP,
        .f2 = f2_DGP,
        .A0 = A0_DGP,
        .mu = mu_DGP,
        .sourceA = sourceA_DGP,
        .sourceb = sourceb_DGP,
        .SD2 = SD2_DGP,
        .S3I = S3I_DGP,
        .S3II = S3II_DGP,
        .S3FL = S3FL_DGP,
        .S3dI = S3dI_DGP,
    },
    .parallel_growth = true,
};

const struct fkpt2_addon fkpt2_dgp_addon = {
    .name = "dgp",
    .model = &fkpt2_dgp_model,
};
