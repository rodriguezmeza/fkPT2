#ifndef FKPT2_GRAVITY_HOOKS_H
#define FKPT2_GRAVITY_HOOKS_H

#include "stdinc.h"

struct cmdline_data;
struct global_data;

/* Source-level interface for addons built with the solver. eta is log(a). */
typedef real (*fkpt2_background_fn)(struct cmdline_data *, struct global_data *, real eta);
typedef real (*fkpt2_mu_fn)(struct cmdline_data *, struct global_data *, real eta, real k);
typedef real (*fkpt2_second_fn)(struct cmdline_data *, struct global_data *,
                                real eta, real kf, real k1, real k2);
typedef real (*fkpt2_symmetric_second_fn)(struct cmdline_data *, struct global_data *,
                                          real eta, real x, real k, real p);
typedef real (*fkpt2_third_fn)(struct cmdline_data *, struct global_data *,
                               real eta, real x, real k, real p, real Dpk, real Dpp,
                               real D2f, real D2mf);

struct fkpt2_gravity_hooks {
    fkpt2_background_fn OmM, H, f1, f2, A0;
    fkpt2_mu_fn mu;
    fkpt2_second_fn sourceA, sourceb;
    fkpt2_symmetric_second_fn SD2;
    fkpt2_third_fn S3I, S3II, S3FL, S3dI;
};

#endif
