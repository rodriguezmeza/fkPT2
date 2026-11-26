"""Compute the fkPT2 one-loop redshift-space power spectrum in memory."""
import os

import numpy as np

from libc.stdlib cimport free, malloc
from libc.string cimport memcpy

cimport cfkpt2 as native


if native.fkpt2_abi_version() != native.FKPT2_ABI_VERSION:
    raise ImportError("The fkPT2 native library has an incompatible ABI")


class ComputationError(RuntimeError):
    """The numerical solver or a computation addon failed."""


cdef class Result:
    """NumPy copies of the native output tables and the run summary."""

    cdef readonly object linear, kfunctions, kfunctions_nw, multipoles
    cdef readonly object model, diagnostics
    cdef readonly object addon_tables, addon_columns
    cdef readonly double z, sigma8, f0, growth

    @property
    def cb_power(self):
        """Real-space one-loop cb power spectra for the FOLPSNu model."""
        return self.addon_tables.get("cb_power")

    @property
    def clpt(self):
        """Real-space correlation functions when CLPT was requested."""
        return self.addon_tables.get("clpt")

    @property
    def qfunctions(self):
        """Lagrangian correlators when CLPT was requested."""
        return self.addon_tables.get("qfunctions")

    @property
    def gsm(self):
        """Redshift-space correlation multipoles from Gaussian streaming."""
        return self.addon_tables.get("gsm")

    @property
    def clpt_velocity(self):
        """Pairwise velocity moments used by the Gaussian streaming model."""
        return self.addon_tables.get("clpt_velocity")

    def column_names(self, table):
        """Return column labels for a standard or addon result table."""
        if table in self.addon_columns:
            return self.addon_columns[table]
        return column_names(table)

    def __init__(self):
        raise TypeError("Obtain a Result from fkpt2.compute()")


def available_models():
    """Return the compiled model names and descriptions."""
    cdef size_t i = 0
    cdef const char *name = native.fkpt2_model_name(0)
    models = {}
    while name != NULL:
        models[name.decode("utf-8")] = native.fkpt2_model_description(i).decode("utf-8")
        i += 1
        name = native.fkpt2_model_name(i)
    return models


def available_addons():
    """Return the names of the compiled addons."""
    cdef size_t i = 0
    cdef const char *name = native.fkpt2_addon_name(0)
    names = []
    while name != NULL:
        names.append(name.decode("utf-8"))
        i += 1
        name = native.fkpt2_addon_name(i)
    return tuple(names)


def column_names(table):
    """Return the column names for a Result table."""
    table_ids = {
        "linear": native.FKPT2_LINEAR,
        "kfunctions": native.FKPT2_LOOPS,
        "kfunctions_nw": native.FKPT2_LOOPS_NOWIGGLE,
        "multipoles": native.FKPT2_MULTIPOLES,
    }
    cdef native.fkpt2_table table_id = <native.fkpt2_table><int>table_ids[table]
    cdef size_t i = 0
    cdef const char *name = native.fkpt2_column_name(table_id, 0)
    names = []
    while name != NULL:
        names.append(name.decode("utf-8"))
        i += 1
        name = native.fkpt2_column_name(table_id, i)
    return tuple(names)


cdef object copy_array(const double *data, size_t rows, size_t columns):
    array = np.empty((rows, columns), dtype=np.float64)
    cdef double[:, ::1] view = array
    memcpy(&view[0, 0], data, rows * columns * sizeof(double))
    return array


cdef object copy_table(const native.fkpt2_result *result, native.fkpt2_table table):
    cdef size_t rows, columns
    cdef const double *data = native.fkpt2_result_table(result, table, &rows, &columns)
    return copy_array(data, rows, columns)


cdef Result copy_result(const native.fkpt2_result *result):
    cdef Result output = Result.__new__(Result)
    output.linear = copy_table(result, native.FKPT2_LINEAR)
    output.kfunctions = copy_table(result, native.FKPT2_LOOPS)
    output.kfunctions_nw = copy_table(result, native.FKPT2_LOOPS_NOWIGGLE)
    output.multipoles = copy_table(result, native.FKPT2_MULTIPOLES)
    cdef native.fkpt2_summary summary = native.fkpt2_result_summary(result)
    output.z = summary.z
    output.sigma8 = summary.sigma8
    output.f0 = summary.f0
    output.growth = summary.growth
    output.model = native.fkpt2_result_model(result).decode("utf-8")

    cdef size_t i = 0
    cdef double value
    cdef const char *name = native.fkpt2_result_diagnostic(result, i, &value)
    diagnostics = {}
    while name != NULL:
        diagnostics[name.decode("utf-8")] = value
        i += 1
        name = native.fkpt2_result_diagnostic(result, i, &value)
    output.diagnostics = diagnostics

    output.addon_tables = {}
    output.addon_columns = {}
    i = 0
    cdef size_t j
    cdef const native.fkpt2_addon_table *table = native.fkpt2_result_addon_table(result, i)
    while table != NULL:
        key = table.name.decode("utf-8")
        output.addon_tables[key] = copy_array(table.data, table.rows, table.columns)
        labels = []
        for j in range(table.columns):
            labels.append(table.column_names[j].decode("utf-8"))
        output.addon_columns[key] = tuple(labels)
        i += 1
        table = native.fkpt2_result_addon_table(result, i)
    return output


cdef bytes encode_value(object value):
    if isinstance(value, (bool, np.bool_)):
        return b"1" if value else b"0"
    if isinstance(value, (bytes, os.PathLike)):
        return os.fsencode(value)
    return str(value).encode("utf-8")


cdef int spectrum_from_class_section(native.fkpt2_class_input *source, void *context) except -1 with gil:
    from fkpt2_class import _linear_spectrum_from_class_section

    options = {"modelOptions": source.model_options.decode("utf-8")}
    if source.input_overrides & native.FKPT2_INPUT_MODEL:
        options["model"] = source.model.decode("utf-8")
    if source.input_overrides & native.FKPT2_INPUT_RESCALE:
        options["is_PS_input_LCDM"] = source.is_ps_input_lcdm
    if source.input_overrides & native.FKPT2_INPUT_OM:
        options["Om"] = source.om
    if source.input_overrides & native.FKPT2_INPUT_H:
        options["h"] = source.h
    if source.input_overrides & native.FKPT2_INPUT_Z:
        options["zout"] = source.zout

    waves, power, settings = _linear_spectrum_from_class_section(
        source.class_parameters.decode("utf-8"), os.fsdecode(source.path), options
    )
    cdef const double[::1] k_view = waves
    cdef const double[::1] pk_view = power
    cdef bytes model = encode_value(settings["model"])
    cdef bytes model_options = encode_value(settings["modelOptions"])
    # Keep the borrowed arrays and strings alive until the synchronous solve ends.
    (<list>context).extend((k_view, pk_view, model, model_options))
    source.k, source.power = &k_view[0], &pk_view[0]
    source.count = k_view.shape[0]
    source.h, source.om, source.zout = settings["h"], settings["Om"], settings["zout"]
    source.model, source.model_options = model, model_options
    source.is_ps_input_lcdm = settings.get("is_PS_input_LCDM", source.is_ps_input_lcdm)
    return 0


def compute(k=None, pk=None, *, parameters=None, **overrides):
    """Compute from arrays, a spectrum file, or a CLASS calculation.

    Use the CLI parameter names in a mapping or as keywords. Keywords take
    precedence over the mapping. Supply paramfile to read solver settings
    and an optional [class] section from the same file. The section uses
    classy when the native CLASS addon is disabled.
    """
    options = dict(parameters) if parameters is not None else {}
    options.update(overrides)
    return _compute(k, pk, options)


def _compute_classy_paramfile(paramfile, parameters):
    """Run a combined configuration through classy for the Python runner."""
    if "paramfile" in parameters:
        raise ValueError("Pass the combined parameter file as the first argument")
    return _compute(None, None, dict(parameters, paramfile=paramfile), use_classy=True)


cdef _compute(k, pk, dict options, bint use_classy=False):
    if (k is None) != (pk is None):
        raise ValueError("Supply both k and pk")
    if k is None and not any(options.get(name) for name in ("fnamePS", "paramfile", "classIni")):
        raise ValueError("Supply k and pk, fnamePS, classIni, or a paramfile")

    cdef const double[::1] k_view, pk_view
    cdef const double *k_data = NULL
    cdef const double *pk_data = NULL
    cdef size_t n = 0

    if k is not None:
        # Native execution can release the GIL. Use private contiguous copies.
        k_array = np.array(k, dtype=np.float64, order="C", copy=True)
        pk_array = np.array(pk, dtype=np.float64, order="C", copy=True)
        if k_array.ndim != 1 or pk_array.ndim != 1 or k_array.shape != pk_array.shape:
            raise ValueError("k and pk must be one-dimensional arrays of equal length")
        if k_array.size < 10:
            raise ValueError("Expected at least 10 spectrum samples")
        k_view, pk_view = k_array, pk_array
        n = k_view.shape[0]
        k_data, pk_data = &k_view[0], &pk_view[0]

    encoded = []
    cdef bytes name, value
    for key, item in options.items():
        if not isinstance(key, str):
            raise TypeError("Parameter names must be strings")
        name, value = key.encode("utf-8"), encode_value(item)
        if b"\0" in name or b"\0" in value:
            raise ValueError("Parameter names and values cannot contain NUL")
        encoded.append((name, value))

    cdef size_t count = len(encoded)
    cdef native.fkpt2_option *native_options = NULL
    cdef native.fkpt2_result *result = NULL
    cdef char error[native.FKPT2_ERROR_SIZE]
    cdef int status
    cdef native.fkpt2_class_provider provider = NULL
    cdef list class_storage
    cdef void *context = NULL
    if options.get("paramfile") and (use_classy or "class" not in available_addons()):
        class_storage = []
        context = <void *>class_storage
        provider = spectrum_from_class_section

    if count != 0:
        native_options = <native.fkpt2_option *>malloc(count * sizeof(native.fkpt2_option))
        if native_options == NULL:
            raise MemoryError()
    try:
        for i in range(count):
            name, value = encoded[i]
            native_options[i].name = name
            native_options[i].value = value

        with nogil:
            status = native.fkpt2_compute_with_class_provider(
                native_options, count, k_data, pk_data, n, provider, context, &result, error
            )
        if status == native.FKPT2_INVALID_INPUT:
            raise ValueError(error.decode("utf-8", "replace").strip())
        if status != native.FKPT2_OK:
            raise ComputationError(error.decode("utf-8", "replace").strip())
        return copy_result(result)
    finally:
        native.fkpt2_result_free(result)
        free(native_options)


def linear_spectrum_from_class(cosmology, *, redshift=None, k=None, parameters=None, **overrides):
    """Generate CLASS input arrays and the synchronized fkPT2 settings."""
    from fkpt2_class import linear_spectrum

    return linear_spectrum(cosmology, redshift=redshift, k=k, parameters=parameters, **overrides)


def compute_from_class(cosmology, *, redshift=None, k=None, parameters=None, **overrides):
    """Generate a CLASS linear spectrum and pass it to the native solver in memory."""
    waves, power, options = linear_spectrum_from_class(
        cosmology, redshift=redshift, k=k, parameters=parameters, **overrides
    )
    return compute(waves, power, parameters=options)


class FKPT2:
    """Reusable set of parameters for independent fkPT2 computations."""

    def __init__(self, parameters=None, **overrides):
        self._parameters = {}
        self.set(parameters, **overrides)

    def set(self, parameters=None, **overrides):
        """Update the stored parameters used by subsequent runs."""
        if parameters is not None:
            self._parameters.update(parameters)
        self._parameters.update(overrides)

    def compute(self, k=None, pk=None, **overrides):
        """Run with the stored parameters and optional per-call overrides."""
        return compute(k, pk, parameters=self._parameters, **overrides)

    def compute_from_class(self, cosmology, *, redshift=None, k=None, **overrides):
        """Run CLASS and fkPT2 with the stored settings and optional overrides."""
        return compute_from_class(
            cosmology, redshift=redshift, k=k, parameters=self._parameters, **overrides
        )
