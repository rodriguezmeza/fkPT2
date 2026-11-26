# Reviewed fkPT2 project discussions

Reviewed on 2026-11-26 from the accessible Codex chats **Analyze fkPT2 code**
and **Implement 3ptWL emulator in C**, and the ChatGPT discussion
**Analyze the fkPT paper**. These are historical project decisions;
use the current manuals and parameter definitions to establish actual behavior.
The private transcripts remain in `.fkpt2-tutor/chats/` on the development
machine and are excluded from Git and source distributions.

## Architecture and numerical references

The project requested a modular native C implementation with a Cython API,
an addon registry, and extensible gravity models. The numerical reference
for the original power spectra is fkPT; MGPT supplies independent gravity
and CLPT comparisons. Current details: `docs/fkpt2-addons.md`, `docs/api.rst`,
`docs/models.rst`, and `docs/validation.rst`.

The paper-analysis discussion considered approximation accuracy, kernel
normalizations, spectrum normalization, and inference validation. Findings
about older fkPT Python wrappers or modified CLASS pipelines describe those
specific versions; they do not establish bugs in current fkPT2. Compare
current regression evidence before applying a historical finding here.

## Physical observables and bias conventions

The discussions developed real-space CLPT and redshift-space Gaussian
Streaming Model multipoles, modified gravity, tidal bias, and EFT terms.
CLPT/GSM use Lagrangian bias parameters such as `clpt_b1`, while the original
power-spectrum path uses Eulerian biases such as `b1`. The linear relation
is `b1_E = 1 + b1_L`; parameters cannot be interchanged by name alone.
Velocity outputs use the convention described in `docs/fkpt2-gsm.md`.
Current scientific explanations: `docs/fkpt2-clpt.md`, `docs/fkpt2-gsm.md`,
`docs/fkpt2-folps-nu.rst`, and `docs/references.rst`.

## CLASS workflows and build selection

The project requested spectrum generation both through native CLASS and
through Python/classy, consistent cosmological parameters, and comparisons
of massive-neutrino multipoles. `CLASSLIBON` selects the native integration;
the external CLASS prerequisites remain optional. `TEST_CLASS=auto`, `0`,
and `1` provide automatic detection, explicit skipping, and required checks.
Current commands: `docs/class_workflow.rst`, `docs/installation.rst`, and
`docs/validation.rst`.

## Emulator and MCMC

The emulator request uses the dense-network approach in 3ptWL-mod and the
GSM neural-network paper to accelerate repeated likelihood evaluation with
a resident C model. Subsequent discussions requested fixed-cosmology and
varying-cosmology MCMC examples, fresh CLASS spectra in direct likelihoods,
and common AP remapping, survey-window mixing, and bin averaging.
The survey window in the demonstration is synthetic. Example speedups and
posterior recovery describe specific test runs; they are not observational
validation or general performance guarantees. New analyses need training
and validation for their own parameter ranges, redshift, grids, covariance,
and survey operator. See `docs/fkpt2-emulator.md` and
`examples/gsm_emulator_mcmc/` for the current implementation.

## Documentation and citation

The project established Sphinx/Read the Docs documentation, regression CI,
and release citation metadata. Cite the fkPT paper as requested in the
current `docs/references.rst` and `CITATION.cff`. Historical release URLs,
publication status, and measured benchmark times need current verification
before being presented as current external facts.
