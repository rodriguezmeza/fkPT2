#include "globaldefs.h"

#include "stdinc.h"
#include "mathfns.h"
#include "diffeqs.h"
#include "numrec.h"

void rk4(struct cmdline_data* cmd, struct global_data* gd,
         double y[], double dydx[], int n, double x, double h, double yout[],
               void (*derivs)(struct cmdline_data*, struct global_data*,
                              double, double [], double []))
{
    int i;
    double xh,hh,h6,*dym,*dyt,*yt;
    
    dym=dvector(1,n);
    dyt=dvector(1,n);
    yt=dvector(1,n);
    hh=h*0.5;
    h6=h/6.0;
    xh=x+hh;
    for (i=1;i<=n;i++) yt[i]=y[i]+hh*dydx[i];
    (*derivs)(cmd, gd, xh,yt,dyt);
    for (i=1;i<=n;i++) yt[i]=y[i]+hh*dyt[i];
    (*derivs)(cmd, gd, xh,yt,dym);
    for (i=1;i<=n;i++) {
        yt[i]=y[i]+h*dym[i];
        dym[i] += dyt[i];
    }
    (*derivs)(cmd, gd, x+h,yt,dyt);
    for (i=1;i<=n;i++)
        yout[i]=y[i]+h6*(dydx[i]+dyt[i]+2.0*dym[i]);
    free_dvector(yt,1,n);
    free_dvector(dyt,1,n);
    free_dvector(dym,1,n);
}

/* Keep the precision of the legacy FMIN and FMAX error estimates. */
static inline float min_float(float a, float b) { return a < b ? a : b; }
static inline float max_float(float a, float b) { return a > b ? a : b; }

// rkck
static void rkck(struct cmdline_data* cmd, struct global_data* gd, double y[], double dydx[], int n, double x, double h, double yout[],
          double yerr[], void (*derivs)(struct cmdline_data*, struct global_data*, double, double [], double []))
{
    int i;
    static const double a2=0.2,a3=0.3,a4=0.6,a5=1.0,a6=0.875,b21=0.2,
    b31=3.0/40.0,b32=9.0/40.0,b41=0.3,b42 = -0.9,b43=1.2,
    b51 = -11.0/54.0, b52=2.5,b53 = -70.0/27.0,b54=35.0/27.0,
    b61=1631.0/55296.0,b62=175.0/512.0,b63=575.0/13824.0,
    b64=44275.0/110592.0,b65=253.0/4096.0,c1=37.0/378.0,
    c3=250.0/621.0,c4=125.0/594.0,c6=512.0/1771.0,
    dc5 = -277.00/14336.0;
    double dc1=c1-2825.0/27648.0,dc3=c3-18575.0/48384.0,
    dc4=c4-13525.0/55296.0,dc6=c6-0.25;
    double ak2[n + 1], ak3[n + 1], ak4[n + 1], ak5[n + 1], ak6[n + 1], ytemp[n + 1];
    for (i=1;i<=n;i++)
        ytemp[i]=y[i]+b21*h*dydx[i];
    (*derivs)(cmd, gd, x+a2*h,ytemp,ak2);
    for (i=1;i<=n;i++)
        ytemp[i]=y[i]+h*(b31*dydx[i]+b32*ak2[i]);
    (*derivs)(cmd, gd, x+a3*h,ytemp,ak3);
    for (i=1;i<=n;i++)
        ytemp[i]=y[i]+h*(b41*dydx[i]+b42*ak2[i]+b43*ak3[i]);
    (*derivs)(cmd, gd, x+a4*h,ytemp,ak4);
    for (i=1;i<=n;i++)
        ytemp[i]=y[i]+h*(b51*dydx[i]+b52*ak2[i]+b53*ak3[i]+b54*ak4[i]);
    (*derivs)(cmd, gd, x+a5*h,ytemp,ak5);
    for (i=1;i<=n;i++)
        ytemp[i]=y[i]+h*(b61*dydx[i]+b62*ak2[i]+b63*ak3[i]+b64*ak4[i]+b65*ak5[i]);
    (*derivs)(cmd, gd, x+a6*h,ytemp,ak6);
    for (i=1;i<=n;i++)
        yout[i]=y[i]+h*(c1*dydx[i]+c3*ak3[i]+c4*ak4[i]+c6*ak6[i]);
    for (i=1;i<=n;i++)
        yerr[i]=h*(dc1*dydx[i]+dc3*ak3[i]+dc4*ak4[i]+dc5*ak5[i]+dc6*ak6[i]);
}

//
// rkqs
#define SAFETY 0.9
#define PGROW -0.2
#define PSHRNK -0.25
#define ERRCON 1.89e-4

static void rkqs(struct cmdline_data* cmd, struct global_data* gd, double y[], double dydx[], int n, double *x, double htry, double eps,
          double yscal[], double *hnext,
          void (*derivs)(struct cmdline_data*, struct global_data*, double, double [], double []))
{
    int i;
    double errmax, h, htemp, xnew;
    double yerr[n + 1], ytemp[n + 1];
    h=htry;
    for (;;) {
        rkck(cmd, gd, y,dydx,n,*x,h,ytemp,yerr,derivs);
        errmax=0.0;
        for (i=1;i<=n;i++) {
            const double scaled_error = fabs(yerr[i]/yscal[i]);
            errmax = errmax > scaled_error ? errmax : scaled_error;
        }
        errmax /= eps;
        if (errmax <= 1.0) break;
        htemp=SAFETY*h*rpow(errmax,PSHRNK);
        h = h >= 0.0 ? (htemp > 0.1*h ? htemp : 0.1*h) : min_float(htemp, 0.1*h);
        xnew=(*x)+h;
        if (xnew == *x) nrerror("stepsize underflow in rkqs");
    }
    if (errmax > ERRCON) *hnext=SAFETY*h*rpow(errmax,PGROW);
    else *hnext=5.0*h;
    *x += h;
    for (i=1;i<=n;i++) y[i]=ytemp[i];
}
#undef SAFETY
#undef PGROW
#undef PSHRNK
#undef ERRCON


#define KMAXX 8
#define IMAXX (KMAXX+1)
#define SAFE1 0.25
#define SAFE2 0.7
#define REDMAX 1.0e-5
#define REDMIN 0.7
#define TINY 1.0e-30
#define SCALMX 0.1

/* Keep the adaptive order and extrapolation table local to one solve. */
struct bsstep_state {
    int first, kmax, kopt;
    double xnew, a[IMAXX+1], alf[KMAXX+1][KMAXX+1];
};

static const int nseq[IMAXX+1] = {0,2,4,6,8,10,12,14,16,18};

static void initialize_bsstep(struct bsstep_state *step, double eps)
{
    const double eps1 = SAFE1 * eps;
    step->first = 1;
    step->xnew = -1.0e29;
    step->a[1] = nseq[1] + 1;
    for (int k = 1; k <= KMAXX; ++k)
        step->a[k + 1] = step->a[k] + nseq[k + 1];
    for (int iq = 2; iq <= KMAXX; ++iq)
        for (int k = 1; k < iq; ++k)
            step->alf[k][iq] = rpow(eps1, (step->a[k + 1] - step->a[iq + 1]) /
                                              ((step->a[iq + 1] - step->a[1] + 1.0) * (2*k + 1)));
    int order;
    for (order = 2; order < KMAXX; ++order)
        if (step->a[order + 1] > step->a[order] * step->alf[order - 1][order])
            break;
    step->kopt = step->kmax = order;
}

static void mmid(struct cmdline_data *, struct global_data *, double [], double [], int,
                 double, double, int, double [], fkpt2_derivs);
static void pzextr(int iest, double xest, double yest[], double yz[], double dy[],
                   int nv, double d[][KMAXX+1], double x[]);

static void bsstep(struct cmdline_data *cmd, struct global_data *gd, struct bsstep_state *step,
                   double y[], double dydx[], int nv, double *xx, double htry, double eps,
                   double yscal[], double *hnext, fkpt2_derivs derivs)
{
    int i, k, kk, km;
    double errmax, fact, h, red, scale, work, wrkmin;
    double d[nv+1][KMAXX+1], x[KMAXX+1], err[KMAXX+1];
    double yerr[nv+1], ysav[nv+1], yseq[nv+1];
    int reduct, exitflag = 0;
    h=htry;
    for (i=1;i<=nv;i++) ysav[i]=y[i];
    if (*xx != step->xnew || h != (*hnext)) {
        step->first=1;
        step->kopt=step->kmax;
    }
    reduct=0;
    for (;;) {
        for (k=1;k<=step->kmax;k++) {
            step->xnew=(*xx)+h;
            if (step->xnew == (*xx)) nrerror("step size underflow in bsstep");
            mmid(cmd, gd, ysav,dydx,nv,*xx,h,nseq[k],yseq,derivs);
            const double substep = h/nseq[k];
            pzextr(k,substep*substep,yseq,y,yerr,nv,d,x);
            if (k != 1) {
                errmax=TINY;
                for (i=1;i<=nv;i++) errmax=max_float(errmax,fabs(yerr[i]/yscal[i]));
                errmax /= eps;
                km=k-1;
                err[km]=rpow(errmax/SAFE1,1.0/(2*km+1));
            }
            if (k != 1 && (k >= step->kopt-1 || step->first)) {
                if (errmax < 1.0) {
                    exitflag=1;
                    break;
                }
                if (k == step->kmax || k == step->kopt+1) {
                    red=SAFE2/err[km];
                    break;
                }
                else if (k == step->kopt && step->alf[step->kopt-1][step->kopt] < err[km]) {
                    red=1.0/err[km];
                    break;
                }
                else if (step->kopt == step->kmax && step->alf[km][step->kmax-1] < err[km]) {
                    red=step->alf[km][step->kmax-1]*SAFE2/err[km];
                    break;
                }
                else if (step->alf[km][step->kopt] < err[km]) {
                    red=step->alf[km][step->kopt-1]/err[km];
                    break;
                }
            }
        }
        if (exitflag) break;
        red=min_float(red,REDMIN);
        red=max_float(red,REDMAX);
        h *= red;
        reduct=1;
    }
    *xx=step->xnew;
    step->first=0;
    wrkmin=1.0e35;
    for (kk=1;kk<=km;kk++) {
        fact=max_float(err[kk],SCALMX);
        work=fact*step->a[kk+1];
        if (work < wrkmin) {
            scale=fact;
            wrkmin=work;
            step->kopt=kk+1;
        }
    }
    *hnext=h/scale;
    if (step->kopt >= k && step->kopt != step->kmax && !reduct) {
        fact=max_float(scale/step->alf[step->kopt-1][step->kopt],SCALMX);
        if (step->a[step->kopt+1]*fact <= wrkmin) {
            *hnext=h/fact;
            step->kopt++;
        }
    }
}
static void mmid(struct cmdline_data* cmd, struct global_data* gd,
          double y[], double dydx[], int nvar, double xs, double htot, int nstep,
          double yout[], void (*derivs)(struct cmdline_data*, struct global_data*, double, double[],
                                        double[]))
{
    int n,i;
    double x, swap, h2, h;
    double ym[nvar + 1], yn[nvar + 1];
    h=htot/nstep;
    for (i=1;i<=nvar;i++) {
        ym[i]=y[i];
        yn[i]=y[i]+h*dydx[i];
    }
    x=xs+h;
    (*derivs)(cmd, gd, x,yn,yout);
    h2=2.0*h;
    for (n=2;n<=nstep;n++) {
        for (i=1;i<=nvar;i++) {
            swap=ym[i]+h2*yout[i];
            ym[i]=yn[i];
            yn[i]=swap;
        }
        x += h;
        (*derivs)(cmd, gd, x,yn,yout);
    }
    for (i=1;i<=nvar;i++)
        yout[i]=0.5*(ym[i]+yn[i]+h*yout[i]);
}


static void pzextr(int iest, double xest, double yest[], double yz[], double dy[],
                   int nv, double d[][KMAXX+1], double x[])
{
    int k1,j;
    double q, f2, f1, delta;
    double c[nv + 1];
    x[iest]=xest;
    for (j=1;j<=nv;j++) dy[j]=yz[j]=yest[j];
    if (iest == 1) {
        for (j=1;j<=nv;j++) d[j][1]=yest[j];
    } else {
        for (j=1;j<=nv;j++) c[j]=yest[j];
        for (k1=1;k1<iest;k1++) {
            delta=1.0/(x[iest-k1]-xest);
            f1=xest*delta;
            f2=x[iest-k1]*delta;
            for (j=1;j<=nv;j++) {
                q=d[j][k1];
                d[j][k1]=dy[j];
                delta=c[j]-q;
                dy[j]=f1*delta;
                c[j]=f2*delta;
                yz[j] += dy[j];
            }
        }
        for (j=1;j<=nv;j++) d[j][iest]=dy[j];
    }
}


double fkpt2_integrate(struct cmdline_data *cmd, struct global_data *gd,
                       double y[], int n, fkpt2_derivs derivs)
{
    const double start = gd->xnow, end = gd->xstop;
    double x = start, h = SIGN(gd->dx, end - start), hnext = 0.0;
    double dydx[n + 1], scale[n + 1];
    struct bsstep_state bs;
    const int use_bs = gd->method_int == FKPT2_BSSTEP;
    if (use_bs)
        initialize_bsstep(&bs, cmd->eps);
    for (int step = 0; step < cmd->maxnsteps; ++step) {
        derivs(cmd, gd, x, y, dydx);
        for (int i = 1; i <= n; ++i)
            scale[i] = fabs(y[i]) + fabs(dydx[i] * h) + TINY;
        if ((x+h-end)*(x+h-start) > 0.0)
            h = end - x;
        if (use_bs)
            bsstep(cmd, gd, &bs, y, dydx, n, &x, h, cmd->eps, scale, &hnext, derivs);
        else
            rkqs(cmd, gd, y, dydx, n, &x, h, cmd->eps, scale, &hnext, derivs);
        if ((x-end)*(end-start) >= 0.0)
            return x;
        if (fabs(hnext) <= cmd->dxmin)
            nrerror("Step size too small in odeint");
        h = hnext;
    }
    nrerror("Too many steps in routine odeint");
    return x;
}
