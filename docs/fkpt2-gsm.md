# Gaussian Streaming Model

Enable the addon for a run with `gsm=true`. GSM also computes CLPT using the
selected gravity model and the same `clpt_b1`, `clpt_b2`, `clpt_bs2`, and
`clpt_b01` biases and density counterterm `c1eft`.

```sh
./fkpt2 fnamePS=tests/fkpt/Input/pkl_z05.dat gsm=true smin=25 smax=130 Ns=100 clpt_b1=0.65 clpt_bs2=-0.2
```

The CLI writes `gsm.txt` and `clpt_velocity.txt` in `rootDir` and applies
`suffixModel` to each file. The run also returns the existing `clpt` and
`qfunctions` tables. Both new tables are available in Cython and through
the native addon table interface.

```python
import fkpt2

result = fkpt2.compute(
    fnamePS="tests/fkpt/Input/pkl_z05.dat",
    gsm=True, smin=25, smax=130, Ns=100, clpt_b1=0.65, clpt_bs2=-0.2,
    c1eft=4.0, c2eft=2.5, s2eft=3.0,
)
s, xi0, xi2, xi4 = result.gsm.T
r, xi_tracer, v12, sigma_parallel2, sigma_perp2 = result.clpt_velocity.T
```

| Table | Columns |
| --- | --- |
| `gsm` | `s` in Mpc/h followed by the monopole `xi0`, quadrupole `xi2` and hexadecapole `xi4` |
| `clpt_velocity` | `r` in Mpc/h, `xi_tracer`, radial mean `v12` in Mpc/h, and central variances along and transverse to the pair in (Mpc/h)^2 |

The velocity convention is peculiar velocity divided by `a H(a)`, with
negative `v12` for infall. The transverse variance is for one component.
GSM projects the mean and both central variances onto the real-space
line of sight and integrates the Gaussian conditional on pair separation.
The multipoles use Legendre projections in the redshift-space angle.
The one-loop velocity calculation uses time derivatives of the CLPT
displacements and scale-dependent linear growth from the selected model.
The tidal term enters the real-space correlation, mean pairwise velocity,
and both dispersions. The velocity calculation differentiates the displacement
legs and uses the selected model's growth rates and second-order kernel.
See [Valogiannis, Bean and Aviles (2020)](https://arxiv.org/abs/1909.05261).

The three EFT coefficients default to zero. The density correction is
included in the pair-count normalization. The velocity correction uses the
Zel'dovich convolution of the radial gradient of the smoothed, growth-weighted
linear correlation. Its coefficient in the raw mean combines with the
curvature term as `c2eft - 2*clpt_b01` for that gradient.
The dispersion correction is added isotropically in the central covariance.
See [Vlah, Castorina and White (2016)](https://arxiv.org/abs/1609.02908).

| Parameters | Role |
| --- | --- |
| `smin`, `smax`, `Ns` | Linear multipole separation grid. `Ns=1` evaluates at `smin`. |
| `gsm_width` | Half-width in Mpc/h of the line-of-sight integration window. Default 100. |
| `gsm_sizeyT` | Line-of-sight quadrature size, rounded up to a multiple of four. Default 160. |
| `gsm_NGL` | Number of Gauss–Legendre nodes for the angular projection. Default 16. |
| `c1eft` | CLPT density counterterm in (Mpc/h)^2. See the [CLPT guide](fkpt2-clpt.md). |
| `c2eft` | Counterterm for the mean pairwise velocity in (Mpc/h)^2. |
| `s2eft` or `sigma2eft` | Isotropic variance counterterm in (Mpc/h)^2, weighted by `(1 + xi_za) / (1 + xi_tracer)`. Default zero. |

The velocity grid covers the real-space separations sampled by the streaming
integral, with spacing no larger than 0.5 Mpc/h. The CLPT settings
`rmin`, `rmax`, and `Nr` control the separate real-space correlation table.
Choose a line-of-sight window large enough to contain the Gaussian tails.
At small separations where the perturbative calculation gives a nonpositive
pair density or variance, GSM reports the offending radius.

GSM is compiled by default and is off for individual runs unless requested.
Build with `GSMON=0` to omit GSM, or `CLPTON=0` to omit both addons.
`make test-gsm` checks LCDM, F5, and DGP against numerical regression values,
validates the projection with a separate quadrature, and checks the linear
Kaiser limit, tidal dispersion, and the EFT responses. The LCDM and F5 cases
also exercise the tidal and all three EFT coefficients together.
`make test-cython` compares the results with the CLI.
