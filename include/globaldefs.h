/*==============================================================================
 HEADER: globaldefs.h        [fkPT2]
 Written by: Mario A. Rodriguez-Meza
 Starting date: 31 october 2025
 Purpose: Definitions of global variables and parameters
 Language: C
 Use: '#include "globaldefs.h"
 Major revisions:
 ==============================================================================*/
//        1          2          3          4        ^ 5          6          7

#ifndef _globaldefs_h
#define _globaldefs_h


#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
//

#include "stdinc.h"
#include "numrec.h"
#include "diffeqs.h"
#include "quads.h"
#include "mathfns.h"
#include "mathutil.h"
#include "inout.h"
#include "vectmath.h"
#include "getparam.h"
#include "machines.h"
#include "strings.h"
#include "common_defs.h"

//B OpenMP section
#ifdef OPENMPCODE
#include "omp.h"
#endif
//E

#if !defined(global)
#  define global extern
#endif

#ifdef CLASSLIB
#include "common.h"
#endif
global ErrorMsg errmsg;

/*
#define IPName(param,paramtext)    \
{strcpy(tag[nt],paramtext);    \
addr[nt]=&(param);    \
id[nt++]=INT;}

#define RPName(param,paramtext)    \
{strcpy(tag[nt],paramtext);    \
addr[nt]=&param;    \
id[nt++]=DOUBLE;}

#define BPName(param,paramtext)    \
{strcpy(tag[nt],paramtext);    \
addr[nt]=&param;    \
id[nt++]=BOOLEAN;}

#define SPName(param,paramtext,n)    \
{strcpy(tag[nt],paramtext);    \
param=(string) malloc(n);    \
addr[nt]=param;    \
id[nt++]=STRING;}
//
*/

#include "models.h"


#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define invH0     2997.92458   //This is H_0^{-1} in Mpc/h units
#define FOURPI2   39.4784176043574    //but see 0903.5321
#define PI2     9.8696044010893586188
#define TWOPI2     19.739208802178716
#define SIXPI2  59.21762640653615
#define INVSQRTDTWOPI 0.39894228040143267794

#include "cmdline_data.h"
#include "global_data.h"

global real *yout;
#define NEQS3Order    10
#define NEQS2Order    8
#define NEQS1Order      2

typedef struct _pointPSTable {
    real k;
    real ps;
} pointPSTable, *pointPSTableptr;

global int nPSTable;
global pointPSTableptr PSLCDMtab;
global int nPSLogT;
global pointPSTableptr PSLCDMLogtab;

global int nPSLT;
global pointPSTableptr PSLT;
global int nPSLTLog;
global pointPSTableptr PSLTLog;

global real *kPS;
global real *pPS;
global real *pPS2;

global real *pPS_nw;
global real *pPS2_nw;

#define kPos(x)    (((pointPSTableptr) (x))->k)
#define PS(x)    (((pointPSTableptr) (x))->ps)

global real *fkT;
global real *fkT2;
global real *DplusT;
global real *DplusT2;

typedef struct {
    real eta;
    real y1;
    real y2;
    real y3;
    real y4;
    real y5;
    real y6;
    real y7;
    real y8;
} global_D2v2, *global_D2v2_ptr;

#define etaD2(x)    (((global_D2v2_ptr) (x))->eta)
#define Dpk1D2(x)    (((global_D2v2_ptr) (x))->y1)
#define Dpk2D2(x)    (((global_D2v2_ptr) (x))->y3)
#define DA2D2(x)    (((global_D2v2_ptr) (x))->y5)
#define DA2primeD2(x)    (((global_D2v2_ptr) (x))->y6)   //Modificacion-LCDMfk
#define DB2D2(x)    (((global_D2v2_ptr) (x))->y7)
#define DB2primeD2(x) (((global_D2v2_ptr) (x))->y8)
//

typedef struct {
    real eta;
    real y1;
    real y2;
    real y3;
    real y4;
    real y5;
    real y6;
    real y7;
    real y8;
    real y9;
    real y10;
} global_D3v2, *global_D3v2_ptr;

#define etaD3(x)    (((global_D3v2_ptr) (x))->eta)
#define DpkD3(x)    (((global_D3v2_ptr) (x))->y1)
#define DppD3(x)    (((global_D3v2_ptr) (x))->y3)
#define D2fD3(x)    (((global_D3v2_ptr) (x))->y5)
#define D2mfD3(x)    (((global_D3v2_ptr) (x))->y7)
#define D2fprimeD3(x)  (((global_D3v2_ptr) (x))->y6)
#define D2mfprimeD3(x) (((global_D3v2_ptr) (x))->y8)
#define D3symmD3(x)    (((global_D3v2_ptr) (x))->y9)
#define D3symmprimeD3(x)    (((global_D3v2_ptr) (x))->y10) //Modificacion-LCDMfk
//

// GL structure
typedef struct {
    int npts;
    real x1;
    real x2;
    real *xgl;
    real *wgl;
} global_GL, *global_GL_ptr;

// STATIC problem: gcc version 11
//global_GL_ptr pGL;
global global_GL_ptr pGL;
//~ global_GL_ptr pGL;

#define nGL(x)    (((global_GL_ptr) (x))->npts)
#define x1GL(x)    (((global_GL_ptr) (x))->x1)
#define x2GL(x)    (((global_GL_ptr) (x))->x2)
#define xGL(x)    (((global_GL_ptr) (x))->xgl)
#define wGL(x)    (((global_GL_ptr) (x))->wgl)


typedef struct {
    real k;
    real P0;
    real P2;
    real P4;
} global_rsdmultipoles, *global_rsdmultipoles_ptr;

#define k_rsdmultipoles(x)    (((global_rsdmultipoles_ptr) (x))->k)
#define P0_rsdmultipoles(x)    (((global_rsdmultipoles_ptr) (x))->P0)
#define P2_rsdmultipoles(x)    (((global_rsdmultipoles_ptr) (x))->P2)
#define P4_rsdmultipoles(x)    (((global_rsdmultipoles_ptr) (x))->P4)


// END :: CLPT correlation auxiliary functions and structures

// NEW for v2

typedef struct {
real kmin;
real kmax;
int Nk;
real qmin;
real qmax;
int Nq;		
real rmin;
real rmax;
int Nr;
real smin;
real smax;
int Ns;
} global_output_lists, *global_output_lists_ptr;

global global_output_lists golists;


//B To use in inout and cballsio
global real *inout_xval;
global real *inout_yval;
global real *inout_zval;
global real *inout_uval;
global real *inout_vval;
global real *inout_wval;
//E

global long idum;                // seed for random generators


typedef struct {
int eta;
real k;
//
real P22dd;
real P22du;
real P22uu;
// A 
real I1udd1A;
real I2uud1A;
real I2uud2A;
real I3uuu2A;
real I3uuu3A;
//  B plus C   
real I2uudd1BpC;
real I2uudd2BpC;
real I3uuud2BpC;
real I3uuud3BpC;
real I4uuuu2BpC;
real I4uuuu3BpC;
real I4uuuu4BpC;
//  Bias
real Pb1b2;
real Pb1bs2;
real Pb22;
real Pb2s2;
real Ps22;
real Pb2theta;
real Pbs2theta;
//
real P13dd;
real P13du;
real P13uu;
real sigma32PSL;
} global_kFs, *global_kFs_ptr;

#define kFs(x)    (((global_kFs_ptr) (x))->k)
#define P22dd(x)    (((global_kFs_ptr) (x))->P22dd)
#define P22du(x)    (((global_kFs_ptr) (x))->P22du)
#define P22uu(x)    (((global_kFs_ptr) (x))->P22uu)
// A 
#define I1udd1A(x)    (((global_kFs_ptr) (x))->I1udd1A)
#define I2uud1A(x)    (((global_kFs_ptr) (x))->I2uud1A)
#define I2uud2A(x)    (((global_kFs_ptr) (x))->I2uud2A)
#define I3uuu2A(x)    (((global_kFs_ptr) (x))->I3uuu2A)
#define I3uuu3A(x)    (((global_kFs_ptr) (x))->I3uuu3A)
//  B plus C   
#define I2uudd1BpC(x)    (((global_kFs_ptr) (x))->I2uudd1BpC)
#define I2uudd2BpC(x)    (((global_kFs_ptr) (x))->I2uudd2BpC)
#define I3uuud2BpC(x)    (((global_kFs_ptr) (x))->I3uuud2BpC)
#define I3uuud3BpC(x)    (((global_kFs_ptr) (x))->I3uuud3BpC)
#define I4uuuu2BpC(x)    (((global_kFs_ptr) (x))->I4uuuu2BpC)
#define I4uuuu3BpC(x)    (((global_kFs_ptr) (x))->I4uuuu3BpC)
#define I4uuuu4BpC(x)    (((global_kFs_ptr) (x))->I4uuuu4BpC)
//  Bias
#define Pb1b2(x)    (((global_kFs_ptr) (x))->Pb1b2)
#define Pb1bs2(x)    (((global_kFs_ptr) (x))->Pb1bs2)
#define Pb22(x)    (((global_kFs_ptr) (x))->Pb22)
#define Pb2s2(x)    (((global_kFs_ptr) (x))->Pb2s2)
#define Ps22(x)    (((global_kFs_ptr) (x))->Ps22)
#define Pb2theta(x)    (((global_kFs_ptr) (x))->Pb2theta)
#define Pbs2theta(x)    (((global_kFs_ptr) (x))->Pbs2theta)
//
#define P13dd(x)    (((global_kFs_ptr) (x))->P13dd)
#define P13du(x)    (((global_kFs_ptr) (x))->P13du)
#define P13uu(x)    (((global_kFs_ptr) (x))->P13uu)
#define sigma32PSL(x)    (((global_kFs_ptr) (x))->sigma32PSL)

typedef struct {
int eta;
real *kT;
//
real *P22ddT;
real *P22duT;
real *P22uuT;
// A 
real *I1udd1AT;
real *I2uud1AT;
real *I2uud2AT;
real *I3uuu2AT;
real *I3uuu3AT;
//  B plus C   
real *I2uudd1BpCT;
real *I2uudd2BpCT;
real *I3uuud2BpCT;
real *I3uuud3BpCT;
real *I4uuuu2BpCT;
real *I4uuuu3BpCT;
real *I4uuuu4BpCT;
//  Bias
real *Pb1b2T;
real *Pb1bs2T;
real *Pb22T;
real *Pb2s2T;
real *Ps22T;
real *Pb2thetaT;
real *Pbs2thetaT;
//
real *P13ddT;
real *P13duT;
real *P13uuT;
real *sigma32PSLT;
real *pklT;
real *fkT;
} global_kFArrays, *global_kFArrays_ptr;


global global_kFArrays kFArrays;

global global_kFArrays kFArrays_nw;

#include "protodefs.h"

#endif // ! _globaldefs_h

