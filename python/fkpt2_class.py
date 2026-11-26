"""Prepare a linear CLASS spectrum and the corresponding fkPT2 settings."""

import math
from pathlib import Path
import re


_C_KM_S = 299792.458
_DEFAULT_K_MAX = 10.0  # h/Mpc
_CLASS_ARGUMENT_SIZE = 1024  # CLASS FileArg buffer, including the terminator
_ALIASES = {
    "m": "model", "OmegaM": "Om", "z": "zout", "isinLCDM": "is_PS_input_LCDM",
    "root": "rootDir", "suffix": "suffixModel",
}
_WORKFLOW_SECTIONS = {"class", "fkpt2"}
_WORKFLOW_FIELDS = {
    "h", "Om", "zout", "model", "modelOptions", "is_PS_input_LCDM",
    "rootDir", "suffixModel", "fnamePS", "classIni",
}


def _class_ini_values(path, lines):
    settings = {}
    for number, line in lines:
        name, sep, value = line.split("#", 1)[0].partition("=")
        if not sep:
            continue
        name, value = name.strip().strip("'\""), value.strip()
        if not name or (value and name in settings):
            raise ValueError(f"{path}:{number}: invalid or duplicate CLASS parameter {name!r}")
        if value:
            settings[name] = value
    return settings


def read_class_ini(path):
    """Read the key=value entries of a CLASS input file."""
    path = Path(path)
    return _class_ini_values(path, enumerate(path.read_text(encoding="utf-8").splitlines(), 1))


def _linear_spectrum_from_class_section(contents, path, options):
    cosmology = _class_ini_values(path, enumerate(contents.splitlines(), 1))
    for name, value in cosmology.items():
        if any(len(part.encode("utf-8")) >= _CLASS_ARGUMENT_SIZE for part in (name, value)):
            raise ValueError(f"{path}: CLASS parameter {name[:80]!r} exceeds the CLASS input length")
    cosmology.pop("write parameters", None)
    cosmology["write_parameters"] = "no"
    return linear_spectrum(cosmology, parameters=options)


def read_class_workflow_ini(path):
    """Return CLASS settings and the solver file text, if it is a combined file."""
    lines = path.read_text(encoding="utf-8").splitlines()
    headers = {
        number: line.strip()[1:].partition("]")
        for number, line in enumerate(lines, 1) if line.lstrip().startswith("[")
    }
    if not any(name.strip().lower() in _WORKFLOW_SECTIONS for name, _, _ in headers.values()):
        return _class_ini_values(path, enumerate(lines, 1)), None

    # Keep the original solver line numbers for diagnostics from fkPT2.
    solver_lines = [""] * len(lines)
    class_lines = []
    in_class = has_class = False
    for number, line in enumerate(lines, 1):
        if number in headers:
            name, close, tail = headers[number]
            name, tail = name.strip().lower(), tail.lstrip()
            if not close or (tail and tail[0] not in "#%"):
                raise ValueError(f"{path}:{number}: invalid section header")
            if name not in _WORKFLOW_SECTIONS:
                raise ValueError(f"{path}:{number}: unknown section (expected [class] or [fkpt2])")
            in_class = name == "class"
            has_class |= in_class
        elif in_class:
            if not line.lstrip().startswith("%"):
                class_lines.append((number, line))
        else:
            solver_lines[number - 1] = line
    if not has_class:
        raise ValueError(f"{path}: a combined parameter file requires a [class] section")
    return _class_ini_values(path, class_lines), "\n".join(solver_lines) + "\n"


def _solver_file_value(text):
    """Decode the fkPT2 file syntax for values needed before the solver starts."""
    if not text.startswith(('"', "'")):
        return re.split(r"[#%]", text, maxsplit=1)[0].rstrip()

    quote = text[0]
    value = []
    index = 1
    while index < len(text):
        char = text[index]
        if char == quote:
            tail = text[index + 1:].lstrip()
            if not tail or tail[0] in "#%":
                return "".join(value)
            break
        if char == "\\" and index + 1 < len(text) and text[index + 1] in (quote, "\\"):
            index += 1
        value.append(text[index])
        index += 1
    raise ValueError("invalid quoted value")


def _canonical_solver_options(options):
    settings = {}
    for key, value in options.items():
        name = _ALIASES.get(key, key)
        if name in settings and str(settings[name]) != str(value):
            raise ValueError(f"Conflicting values for fkPT2 parameter {name}")
        settings[name] = value
    return settings


def class_workflow_solver_options(path, solver_text, overrides):
    """Resolve the solver settings needed by the standalone CLASS workflow."""
    settings = {}
    for number, line in enumerate(solver_text.splitlines(), 1):
        match = re.match(r"([^\s=]+)\s*=?\s*(.*)", line.lstrip())
        if match is None:
            continue
        key, text = match.groups()
        name = _ALIASES.get(key, key)
        if name not in _WORKFLOW_FIELDS:
            continue
        try:
            value = _solver_file_value(text)
        except ValueError as exc:
            raise ValueError(f"{path}:{number}: {exc} ({key})") from exc
        if name in settings and settings[name] != value:
            raise ValueError(f"{path}:{number}: duplicate parameter ({key})")
        settings[name] = value
    settings.update(_canonical_solver_options(overrides))
    if settings.get("classIni"):
        raise ValueError("Choose a [class] section or classIni for the CLASS input")
    if settings.pop("fnamePS", ""):
        raise ValueError("Use the [class] section by itself as the linear spectrum input")
    return settings


def class_power_parameters(cosmology, redshift=None):
    """Configure linear matter power at one redshift, using CLASS parameter names."""
    settings = {
        key: ",".join(map(str, value)) if isinstance(value, (list, tuple)) else value
        for key, value in cosmology.items()
    }
    try:
        z = float(settings.get("z_pk", 0.5) if redshift is None else redshift)
    except (ValueError, TypeError) as exc:
        raise ValueError("Supply a single CLASS redshift in z_pk or redshift") from exc
    if not math.isfinite(z) or z < 0:
        raise ValueError("The CLASS power-spectrum redshift must be finite and nonnegative")

    settings.pop("non linear", None)
    settings.update(output="mPk", non_linear="none", z_pk=z)
    z_max = float(settings.get("z_max_pk", z))
    if not math.isfinite(z_max) or z_max < z:
        raise ValueError("CLASS z_max_pk must cover the requested spectrum redshift")
    if z_max == 0.0:
        # classy exposes the native k grid through a table with multiple times.
        settings["z_max_pk"] = 1e-3
    if "P_k_max_h/Mpc" not in settings and "P_k_max_1/Mpc" not in settings:
        settings["P_k_max_h/Mpc"] = _DEFAULT_K_MAX
    return settings


def solver_parameters(background, redshift, options):
    """Use the present-day CLASS background to synchronize both interfaces."""
    if background["z"] != 0.0:
        raise ValueError("The CLASS background must include its present-day row")
    critical = background["(.)rho_crit"]
    if not math.isclose(background["(.)rho_tot"] / critical, 1.0, rel_tol=0.0, abs_tol=1e-9):
        raise ValueError("The CLASS workflow requires a spatially flat background for fkPT2")
    for label in ("(.)rho_fld", "(.)rho_scf", "(.)rho_idm", "(.)rho_dcdm"):
        if abs(background.get(label, 0.0) / critical) > 1e-9:
            raise ValueError(f"The CLASS background component {label} requires a different fkPT2 growth model")

    h = background["H [1/Mpc]"] * _C_KM_S / 100.0
    om = background["Omega_m(z)"]
    omega_nu = sum(value for label, value in background.items()
                   if label.startswith("(.)rho_ncdm[")) / critical

    settings = _canonical_solver_options(options)
    if "fnamePS" in settings or "paramfile" in settings:
        raise ValueError("Pass fkPT2 solver settings directly when generating the spectrum with CLASS")
    for name, value in (("h", h), ("Om", om), ("zout", redshift)):
        if name in settings and not math.isclose(float(settings[name]), value, rel_tol=1e-7, abs_tol=1e-10):
            raise ValueError(f"fkPT2 {name}={settings[name]} conflicts with the CLASS value {value:.12g}")
        settings[name] = value

    model = str(settings.setdefault("model", "FOLPSNu" if omega_nu > 0.0 else "LCDM"))
    use_cb = model.casefold() == "folpsnu"
    if not use_cb and omega_nu > 0.0:
        raise ValueError("Select model=FOLPSNu for a CLASS spectrum with massive neutrinos")

    if use_cb:
        mnu = 93.14 * omega_nu * h**2
        supplied = str(settings.get("modelOptions", "")).strip()
        if supplied:
            name, _, mass = supplied.partition("=")
            try:
                matches = name.strip() == "mnu" and math.isclose(float(mass), mnu, rel_tol=1e-6, abs_tol=1e-10)
            except ValueError:
                matches = False
            if not matches:
                raise ValueError(f"CLASS fixes the FOLPSNu mass parameter at mnu={mnu:.12g} eV")
        if str(settings.get("is_PS_input_LCDM", 0)).lower() not in ("0", "false"):
            raise ValueError("FOLPSNu uses CLASS's linear cb spectrum with is_PS_input_LCDM=0")
        settings.update(modelOptions=f"mnu={mnu:.17g}", is_PS_input_LCDM=0)
    return settings, use_cb, omega_nu


def linear_spectrum(cosmology, *, redshift=None, k=None, parameters=None, **overrides):
    """Return (k in h/Mpc, linear P in (Mpc/h)^3, synchronized fkPT2 settings)."""
    import numpy as np

    try:
        from classy import Class, CosmoComputationError, CosmoSevereError
    except ImportError as exc:
        raise ImportError("A working CLASS Python module (classy) is required to generate the linear spectrum") from exc

    settings = class_power_parameters(cosmology, redshift)
    z = settings["z_pk"]
    options = dict(parameters or {})
    options.update(overrides)

    cosmo = Class()
    try:
        cosmo.set(settings)
        cosmo.compute(["fourier"])
        background = {name: values[-1] for name, values in cosmo.get_background().items()}
        options, use_cb, omega_nu = solver_parameters(background, z, options)
        h = options["h"]
        _, native_k, _ = cosmo.get_pk_and_k_and_z(nonlinear=False)
        if k is None:
            k = native_k / h
        else:
            k = np.array(k, dtype=np.float64, copy=True)
            if (k.ndim != 1 or k.size < 10 or not np.isfinite(k).all()
                    or k[0] <= 0.0 or np.any(np.diff(k) <= 0.0)):
                raise ValueError("Supply at least ten positive, increasing k samples in h/Mpc")
            if k[-1] * h > native_k[-1] * (1.0 + 1e-12):
                raise ValueError("Increase CLASS P_k_max to cover the requested k samples")

        # Evaluate the tabulated endpoints on the interpolation side of
        # CLASS's exp(log(k)) bounds, without moving the returned wavenumbers.
        physical_k = k * h
        if math.isclose(physical_k[0], native_k[0], rel_tol=1e-12):
            physical_k[0] = native_k[0] * (1.0 + 5e-15)
        physical_k[-1] = min(physical_k[-1], native_k[-1] * (1.0 - 5e-15))
        # Before conversion, classy takes 1/Mpc and returns Mpc^3.
        pk_at = cosmo.pk_cb_lin if use_cb and omega_nu > 0.0 else cosmo.pk_lin
        power = np.fromiter((pk_at(ki, z) for ki in physical_k), dtype=np.float64, count=k.size) * h**3
        if not np.isfinite(power).all() or np.any(power <= 0.0):
            raise ValueError("CLASS returned nonpositive or nonfinite linear power")
    except (CosmoComputationError, CosmoSevereError) as exc:
        raise RuntimeError(f"CLASS could not compute the linear spectrum: {exc}") from exc
    finally:
        cosmo.struct_cleanup()

    return k, power, options
