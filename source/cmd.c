
local int StartRun_Common(struct cmdline_data* cmd, struct global_data* gd)
{
    string routineName = "StartRun_Common";

    real dx1, dx2;
    char *ep;

    gd->cmd_allocated = TRUE;
    gd->gd_allocated = FALSE;

    gd->outlog = NULL;
    gd->outlogFlagFree = FALSE;
    
    if (strlen(cmd->rootDir)==0 || strnull(cmd->rootDir))
        gd->rootDirFlag = FALSE;
    else
        gd->rootDirFlag = TRUE;

    if (scanopt(cmd->options, "make-info")) {
        print_make_info(cmd, gd);
        if (!scanopt(cmd->options, "no-stop")) {        // make a better way out
            gd->stopFlag = TRUE;
            return FAILURE;
        }
    }

    if (scanopt(cmd->options, "print-options")) {
        print_options(cmd, gd);
        if (!scanopt(cmd->options, "no-stop")) {        // make a better way out
            gd->stopFlag = TRUE;
            return FAILURE;
        }
    }

//B socket:
#ifdef ADDONS
#include "startrun_include_04.h"
#endif
//E

    class_call_cballs(setFilesDirs(cmd, gd), errmsg, errmsg);
    class_call_cballs(setFilesDirs_log(cmd, gd), errmsg, errmsg);

    strcpy(gd->mode,"w");
    if (cmd->verbose_log > 0) {
        if (!(gd->outlog = fopen(gd->logfilePath, gd->mode)))
            cBALLS_FAIL(cmd, "\n%s: error opening file '%s' \n",
                        routineName, gd->logfilePath);

        gd->outlogFlagFree = TRUE;
    }

    debug_tracking_s("00", routineName);

    //B mv to gd structure...
    cmd->c1eft = 0.0;
    cmd->c2eft = 0.0;
    cmd->s2eft = 0.0;
    // output array params:
    cmd->smin = 1;
    cmd->smax = 130;
    cmd->Ns = 100;
 
    //  q functions:
    cmd->NqperLogDecade = 100;
    cmd->Nk_qFunctionsQuad = 1200;
    //B  For GSM integration:
    cmd->gsm_width = 100.;  //Width of Gaussian in GSM integration
    cmd->gsm_sizeyT = 160; //Number of points in 1-d GSM integration
    cmd->gsm_NGL = 16;                              // Number of GL points in
                                                    //  angular integration
                                                    //  for rsd multipoles

    cmd->screening =1.0;  ";set to =0 if you want no screenings", ":sc",
    // DGP:
    cmd->eps_DGP = -1.0;
    cmd->rc_DGP = 1.0;

    // CLPT correlation functions table:
    cmd->rmin = 1.;
    cmd->rmax = 210.;
    cmd->Nr = 210;

    cmd->olstr ="1 - Om";  // This is for DGP only

    //
    // Differential equations evolution parameters:
    cmd->x =-4.0;    //";Initial conformal time value :: Log[1/(1 + zini)]",
    cmd->dxstr = "2/5";  //";Conformal time integration step",
    cmd->dxmin = 0.;  // ";Min conformal time integration step size",
    cmd->eps = 0.0001;  //";Differential equations solver tolerance error parameter",":epssolver",

    cmd->maxnsteps = 10000;  //";Maximum number of integration steps", ":maxn",
    cmd->integration_method = "rkqs";  //";Integration method to use", ":solver",
//
// Quadrature parameters:
//    cmd->quadratureMethod = "trapezoid3"; //  Quadrature method to use", ":quadm
    

    cmd->ngausslegpoints = 16;  //NOT USED   ;Number of Gauss-Legendre of integration points", ":nglpts
    cmd->epsquad = 1.0e-6;    //NOT USED  Quadrature tolerance error parameter (open Romberg method: romberg)
// Post processing parameters:
    cmd->postprocessing = FALSE;
//    cmd->options = "";

    if(cmd->verbose>=1){
        printf("\n \t\t fkpt - compute power spectrum in modified gravity models \n\n");
        
        fprintf(stdout,"Reading pkl from file %s in the form column1: k[h/Mpc], 2: pkl[Mpc/h]^3 \n", cmd->fnamePS);
        fprintf(stdout,"redshift: z=%g\n",    cmd->xstop);
        fprintf(stdout,"OmegaM=%g\n",         cmd->om);
        fprintf(stdout,"h=%g\n",              cmd->h);
        fprintf(stdout,"b1=%g\n",             cmd->b1);
        fprintf(stdout,"b2=%g\n",             cmd->b2);
        fprintf(stdout,"bs2=%g\n",            cmd->bs2);
        fprintf(stdout,"b3nl=%g\n",           cmd->b3nl);
        fprintf(stdout,"alpha0=%g\n",         cmd->alpha0);
        fprintf(stdout,"alpha2=%g\n",         cmd->alpha2);
        fprintf(stdout,"alpha4=%g\n",         cmd->alpha4);
        fprintf(stdout,"ctilde=%g\n",         cmd->ctilde);
        fprintf(stdout,"Pshotp=%g\n",         cmd->PshotP);
        fprintf(stdout,"alpha0shot=%g\n",     cmd->alpha0shot);
        fprintf(stdout,"alpha2shot=%g\n",     cmd->alpha2shot);
    }
    //E mv to gd structure...

    debug_tracking_s("01", routineName);

    // Background cosmology:
    ep = strchr(cmd->olstr, '-');
    if (ep == NULL) {
        gd->ol = GetdParam("OL");
        fprintf(gd->outlog,"\nOL string input without '-' :: %s\n",cmd->olstr);
        fprintf(gd->outlog,"\nOLambda, Om and sum  : %g %g %g\n",gd->ol, cmd->om, gd->ol+cmd->om);
    } else {
        ep = strchr(cmd->olstr, '1');
        if (ep == NULL)
            error("\nstart_Common: OL not in the format '1 - Om' \n");
        else {
            ep = strchr(cmd->olstr, 'O');
            if (ep == NULL)
                error("\nstart_Common: OL not in the format '1 - Om' \n");
            else
                if (*(ep+1) == 'm') {
                    fprintf(gd->outlog,"\nFound Om\n");
                    gd->ol = 1. - cmd->om;
                    fprintf(gd->outlog,"\nOLambda and Om : %g %g\n",gd->ol, cmd->om);
                } else {
                    fprintf(gd->outlog,"\nNot found Om\n");
                    error("\nstart_Common: OL not in the format (1 - Om) \n");
                }
        }
        
    }

    debug_tracking_s("01b", routineName);

    gd->dx = (sscanf(cmd->dxstr, "%lf/%lf", &dx1, &dx2) == 2 ?
                dx1/dx2 : atof(cmd->dxstr));
    if ( dx2 == 0. )
        error("\n\nstartrun_Common: dx : dx2 must be finite\n");

    CheckParameters(cmd, gd);
    GaussLegendrePoints(cmd, gd);
    quadraturemethod_string_to_int(cmd, gd,  cmd->quadratureMethod, &gd->quadmethod_int);
    integration_method_string_to_int(cmd, gd,  cmd->integration_method, &gd->method_int);
    gd->xnow = cmd->x;
    gd->xout = gd->xnow;
    gd->xoutinfo = gd->xnow;

    gd->xstop = rlog(1.0/(1.0+cmd->xstop));

    set_model(cmd, gd);

    if (!strnull(cmd->fnamePS)) {
        sprintf(gd->fnamePSPath,"Input/%s",cmd->fnamePS);
        fprintf(gd->outlog,"PS file Path and file name: %s\n",gd->fnamePSPath);
        fflush(gd->outlog);
        InputPSTable(cmd, gd);
        PSLTable(cmd, gd);
        PSLTableNW(cmd, gd);
    }
    if (cmd->nquadSteps > nPSLT)
        error("CheckParameters: nquadSteps > nPSLT\n");

    debug_tracking_s("02", routineName);

//B socket:
#ifdef ADDONS
#include "startrun_include_05.h"
#endif
//E

    gd->gd_allocated = TRUE;

    return SUCCESS;
}

