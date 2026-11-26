/*==============================================================================
	MODULE: models.c			[mgpt]
==============================================================================*/
#include "fkpt2_addon.h"

real kpp(real x, real k, real p)
{
    return rsqrt(rsqr(k) + rsqr(p) + 2.0*k*p*x);
}

real OmM(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    return gd->gravity.OmM(cmd, gd, eta);
}

real H(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    return gd->gravity.H(cmd, gd, eta);
}

real f1(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    return gd->gravity.f1(cmd, gd, eta);
}

real f2(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    return gd->gravity.f2(cmd, gd, eta);
}

real A0(struct cmdline_data* cmd, struct global_data* gd, real eta)
{
    return gd->gravity.A0(cmd, gd, eta);
}

real mu(struct cmdline_data* cmd,
               struct global_data* gd, real eta, real k)
{
    return gd->gravity.mu(cmd, gd, eta, k);
}

real sourceA(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real kf, real k1, real k2)
{
    return gd->gravity.sourceA(cmd, gd, eta, kf, k1, k2);
}

real sourceb(struct cmdline_data* cmd,
                    struct global_data* gd, real eta, real kf, real k1, real k2)
{
    return gd->gravity.sourceb(cmd, gd, eta, kf, k1, k2);
}

real SD2(struct cmdline_data* cmd,
                struct global_data* gd, real eta, real x, real k, real p)
{
    return gd->gravity.SD2(cmd, gd, eta, x, k, p);
}

real S3I(struct cmdline_data* cmd,
                struct global_data* gd, real eta, real x, real k, real p, real Dpk,
                real Dpp, real D2f, real D2mf)
{
    return gd->gravity.S3I(cmd, gd, eta, x, k, p, Dpk, Dpp, D2f, D2mf);
}

real S3II(struct cmdline_data* cmd,
                 struct global_data* gd, real eta, real x, real k, real p, real Dpk,
                 real Dpp, real D2f, real D2mf)
{
    return gd->gravity.S3II(cmd, gd, eta, x, k, p, Dpk, Dpp, D2f, D2mf);
}

real S3FL(struct cmdline_data* cmd,
                 struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f, real D2mf)
{
    return gd->gravity.S3FL(cmd, gd, eta, x, k, p, Dpk, Dpp, D2f, D2mf);
}

real S3dI(struct cmdline_data* cmd,
                 struct global_data* gd, real eta, real x, real k, real p, real Dpk, real Dpp, real D2f, real D2mf)
{
    return gd->gravity.S3dI(cmd, gd, eta, x, k, p, Dpk, Dpp, D2f, D2mf);
}
