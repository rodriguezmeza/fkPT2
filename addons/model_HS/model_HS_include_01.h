// Use:
//#include "model_HS_include_01.h"
//
//  it is included in:
//      addons/addons_include/source/models/models_include_01.h
//

#ifndef _models_HS_include_01_h
#define _models_HS_include_01_h

// ===========================================================================
// MODELS SECTION 1
// ==========================================
// Begin: HS Model global HEADERS -> local
local void set_Model_HS(struct cmdline_data* cmd, struct global_data* gd);
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
                   Dpk, real Dpp,
                 real D2f, real D2mf);
// End: HS Model global HEADERS
// ==========================================

#endif	// ! _models_HS_include_01_h
