# Real-space CLPT

Build fkPT2 normally and request CLPT for a run with `clpt=true`. The addon
uses the selected gravity model and prepared linear spectrum at `zout`.

```sh
./fkpt2 clpt=true rmin=50 rmax=150 Nr=101 clpt_b1=0.65 clpt_b2=-0.35 clpt_bs2=-0.2 c1eft=4
```

The CLI writes `clpt.txt` and `qfunctions.txt` in `rootDir` and applies
`suffixModel` to both names. The first column is the Eulerian separation
`r` or the Lagrangian separation `q` in Mpc/h. Each file lists the column
names in its header.

```python
import fkpt2

result = fkpt2.compute(
    fnamePS="tests/fkpt/Input/pkl_z05.dat",
    clpt=True, rmin=50, rmax=150, Nr=101, clpt_b1=0.65, clpt_b2=-0.35,
    clpt_bs2=-0.2, c1eft=4.0,
)
r = result.clpt[:, 0]
xi = result.clpt[:, result.column_names("clpt").index("xi_tracer")]
qfunctions = result.qfunctions
```

The arrays live in memory and also appear in `result.addon_tables`.
`result.clpt` and `result.qfunctions` are `None` when the run omits CLPT.
C callers can iterate over the addon tables with `fkpt2_result_addon_table()`.
The build includes CLPT by default. Use `CLPTON=0` to disable the addon.
The [GSM addon](fkpt2-gsm.md) uses CLPT correlations and pairwise velocities
to compute redshift-space correlation multipoles.

## Correlations and integration settings

The correlation table gives the linear correlation, the Zel'dovich term
`xi_za`, the displacement-loop corrections `xi_A` and `xi_W`, the bias
components, and the assembled `xi_matter` and `xi_tracer`. The matter
correlation is `xi_za + xi_A + xi_W + c1eft * xi_nabla2`. The tracer result
adds the local Lagrangian biases `clpt_b1` and `clpt_b2`, the quadratic
tidal bias `clpt_bs2`, and the curvature bias `clpt_b01`. These parameters
default to zero, and both tables retain their existing columns.

`clpt_bs2` multiplies the mean-subtracted square of the traceless
Lagrangian shear tensor. It contributes terms proportional to `clpt_bs2` and its square
and the cross terms with `clpt_b1` and `clpt_b2` to `xi_tracer`.
The Fourier-space bias parameter `bs2` belongs to the power-spectrum model.

`c1eft` is the leading density counterterm in (Mpc/h)^2. It uses the
MGPT convention of convolving the smoothed linear Laplacian with the
Zel'dovich kernel. The same Laplacian is the leading curvature template,
with combined tracer coefficient `c1eft + 2*(1 + clpt_b1)*clpt_b01`.
The curvature terms use MGPT's Gaussian windows exp(-k²) and exp(-4k²),
with k in h/Mpc. See [Vlah, White and Aviles (2015)](https://arxiv.org/abs/1506.05264)
and [Vlah, Castorina and White (2016)](https://arxiv.org/abs/1609.02908).

| Parameters | Role |
| --- | --- |
| `rmin`, `rmax`, `Nr` | Linear separation grid. `Nr=1` evaluates at `rmin`. |
| `clpt_kmin`, `clpt_kmax` | Fourier integration range in h/Mpc. Defaults to 0.0001 through 20. |
| `clpt_pmax` | Internal loop momentum cutoff in h/Mpc. Defaults to 30 and is capped by the prepared spectrum. |
| `clpt_Nk` | Logarithmic wavenumber samples for the CLPT loop functions. Defaults to 160. |
| `nquadSteps`, `ngausslegpoints` | Radial and angular quadrature resolution for the loop functions. |
| `NqperLogDecade`, `Nk_qFunctionsQuad` | Lagrangian separation resolution and minimum Fourier integration panels. |

CLPT resums the linear displacement covariance and expands the loop
and bias corrections as in Aviles et al. The Fourier kernels use the
published Appendix C expressions and the selected gravity model's growth
solver. The mixed displacement term `X10` is evaluated relative to zero
separation to enforce its zero-lag identity at finite Fourier cutoffs.
For modified gravity, the loop stage evaluates scale-dependent ODE kernels
and takes longer than the LCDM calculation.

## Regression tests

`make test-clpt` checks the linear Hankel transform with independent
quadrature, the small-separation limits, the q-function values, the bias
sum, the F5 calculation, the tidal Wick contractions at weak clustering,
and the density EFT response. Its correlation reference comes from the
MGPT convolution evaluated on the same q-functions. Set
`MGPT_REFERENCE=/path/to/MGPT` to rerun that comparison with a fresh
temporary MGPT build. `make test-cython` also checks the two CLPT tables
through Python.

The implementation follows Carlson, Reid and White, arXiv:1209.0780,
and Aviles et al., arXiv:1809.07713, with the real-space conventions
in MGPT commit `a90ecc2`.
