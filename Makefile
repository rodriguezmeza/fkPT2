# Build the command line and native library from the same solver and addons.
.DEFAULT_GOAL := all
MACHINES_DIR = ./
CLASS_DIR ?= ../xother_files/github_codes_to-study/class_public_2026-11-25
include $(MACHINES_DIR)Makefile_settings

FKPT2_MISSING_ADDONS = $(filter-out $(FKPT2_ADDON_DESCRIPTORS),$(FKPT2_ADDON_REQUIRES))
ifneq ($(strip $(FKPT2_MISSING_ADDONS)),)
$(error Missing required addons $(FKPT2_MISSING_ADDONS))
endif

EXEC = fkpt2
MAIN = main.o
CORE = globals.o workspace.o api.o addons.o results.o fkpt2_io.o startrun.o \
       parameters.o models.o model_lcdm.o fkpt2_diffeqs.o \
       kfunctions.o rsd.o fkpt2.o fkpt2utils.o
ADDON_OBJECTS = $(notdir $(FKPT2_ADDON_SOURCES:.c=.o))
LIB_OBJECTS = $(CORE) $(ADDON_OBJECTS) $(EXTERNAL) $(EXTERNALCXX)
LIB_FILES = $(addprefix $(WRKDIR)/,$(LIB_OBJECTS))
FKPT2_LIBRARY = $(WRKDIR)/libfkpt2.a
FKPT2_LINK_LIBS = $(FKPT2_ADDON_LINK_LIBS) $(LDFLAG) $(MLIBS) $(FITSIOLIBS) -pthread
FKPT2_LINK_FLAGS = $(OMPFLAG) $(FKPT2_LINK_LIBS)
FKPT2_PYTHON_LINK_FLAGS = $(OMP_PYTHON_LINK) $(FKPT2_LINK_LIBS)

.PHONY: all $(EXEC) lib$(EXEC).a
all: $(EXEC) lib$(EXEC).a

# Publish complete binaries so longer calculations can run during a rebuild.
$(EXEC) lib$(EXEC).a:
	@cmp -s "$<" "$@" || { tmp="$@.$$$$.tmp"; cp "$<" "$$tmp" && mv "$$tmp" "$@"; }

$(EXEC): $(WRKDIR)/fkpt2-exe

lib$(EXEC).a: $(FKPT2_LIBRARY)

$(WRKDIR)/fkpt2-exe: $(WRKDIR)/$(MAIN) $(FKPT2_LIBRARY)
	$(CC) $(OPTFLAG) -o "$@.tmp" "$(WRKDIR)/$(MAIN)" "$(FKPT2_LIBRARY)" $(FKPT2_LINK_FLAGS)
	mv "$@.tmp" "$@"

$(FKPT2_LIBRARY): $(LIB_FILES) $(FKPT2_ADDON_LINK_DEPS)
	$(RM) "$@.tmp"
	$(AR) "$@.tmp" $(LIB_FILES)
	mv "$@.tmp" "$@"

# The snapshot makes flag and addon selection changes rebuild the solver,
# even if an addon manifest was removed or compiler flags changed.
shell_quote = '$(subst ','"'"',$(1))'
.PHONY: FORCE
$(WRKDIR)/.config: FORCE
	@mkdir -p "$(WRKDIR)"
	@printf '%s\n' $(call shell_quote,$(CC)|$(CPPFLAGS)|$(CFLAGS)|$(OPTFLAG)|$(CCFLAG)|$(INCLUDES)|$(FKPT2_LINK_FLAGS)|$(FKPT2_PYTHON_LINK_FLAGS)|$(FKPT2_ADDON_DESCRIPTORS)|$(FKPT2_ADDON_SOURCES)|$(LIB_OBJECTS)) > "$@.tmp"
	@cmp -s "$@.tmp" "$@" || mv "$@.tmp" "$@"
	@$(RM) "$@.tmp"

$(WRKDIR)/fkpt2_addons.inc: $(WRKDIR)/.config $(MAKEFILE_LIST)
	@printf '%s\n' $(foreach symbol,$(FKPT2_ADDON_DESCRIPTORS),'FKPT2_ADDON($(symbol))') > "$@"

.PHONY: fkpt2py python-build-env
fkpt2py:
	+$(PYTHON) setup.py build_ext --inplace

# Used by setup.py to build a PIC archive and obtain the native link flags.
python-build-env: $(FKPT2_LIBRARY)
	@printf '%s\n' $(call shell_quote,FKPT2_LINK_FLAGS=$(FKPT2_PYTHON_LINK_FLAGS))

# Compare all four output tables against the supplied fkPT snapshot.
FKPT_REFERENCE ?= ../xother_files/fkpt_github/fkpt_2026-10-23
FKPT_REFERENCE_CC ?= $(CC)
TEST_PYTHON ?= $(PYTHON)

.PHONY: ia-tutor test-ia-tutor
ia-tutor:
	$(PYTHON) scripts/ia_tutor.py

test-ia-tutor: $(EXEC)
	$(TEST_PYTHON) tests/test_ia_tutor.py --fkpt2 "$(EXEC)"

# Set MGPT_REFERENCE to recheck the DGP and CLPT fixtures with MGPT.
.PHONY: test test-default test-fkpt test-dgp test-clpt test-gsm test-folps-nu test-cython test-addons test-class-selection
test test-default: test-fkpt test-dgp test-class-selection test-ia-tutor
ifneq ($(filter fkpt2_clpt_addon,$(FKPT2_ADDON_DESCRIPTORS)),)
test test-default: test-clpt
endif
ifneq ($(filter fkpt2_gsm_addon,$(FKPT2_ADDON_DESCRIPTORS)),)
test test-default: test-gsm
endif
ifneq ($(filter fkpt2_folps_nu_addon,$(FKPT2_ADDON_DESCRIPTORS)),)
test test-default: test-folps-nu
endif

test-fkpt: $(EXEC)
	$(TEST_PYTHON) tests/test_fkpt_regression.py --fkpt2 "$(EXEC)" \
		--reference "$(FKPT_REFERENCE)" --cc "$(FKPT_REFERENCE_CC)"

test-dgp: $(EXEC)
	$(TEST_PYTHON) tests/test_dgp_regression.py --fkpt2 "$(EXEC)" \
		--cc "$(CC)" $(if $(MGPT_REFERENCE),--mgpt-reference "$(MGPT_REFERENCE)")

test-clpt: $(EXEC)
	$(TEST_PYTHON) tests/test_clpt_regression.py --fkpt2 "$(EXEC)" \
		--cc "$(CC)" $(if $(MGPT_REFERENCE),--mgpt-reference "$(MGPT_REFERENCE)")

test-gsm: $(EXEC)
	$(TEST_PYTHON) tests/test_gsm_regression.py --fkpt2 "$(EXEC)"

test-folps-nu: $(EXEC)
	$(TEST_PYTHON) tests/test_folps_nu.py --fkpt2 "$(EXEC)" \
		$(if $(FOLPS_REFERENCE),--folps-reference "$(FOLPS_REFERENCE)")

test-class-selection:
	MAKE=$(call shell_quote,$(MAKE)) $(TEST_PYTHON) tests/test_class_selection.py

.PHONY: test-python-extension
test-python-extension:
	+$(TEST_PYTHON) setup.py build_ext --inplace

test-cython: $(EXEC) test-python-extension
	PYTHONPATH="$(MDIR)/python" $(TEST_PYTHON) tests/test_cython.py --fkpt2 "$(EXEC)"
ifneq ($(filter fkpt2_clpt_addon,$(FKPT2_ADDON_DESCRIPTORS)),)
	PYTHONPATH="$(MDIR)/python" $(TEST_PYTHON) tests/test_clpt_cython.py --fkpt2 "$(EXEC)"
endif
ifneq ($(filter fkpt2_gsm_addon,$(FKPT2_ADDON_DESCRIPTORS)),)
	PYTHONPATH="$(MDIR)/python" $(TEST_PYTHON) tests/test_gsm_cython.py --fkpt2 "$(EXEC)"
endif
ifneq ($(filter fkpt2_folps_nu_addon,$(FKPT2_ADDON_DESCRIPTORS)),)
	PYTHONPATH="$(MDIR)/python" $(TEST_PYTHON) tests/test_folps_nu.py --fkpt2 "$(EXEC)" --python
endif

.PHONY: test-openmp
ifeq ($(OPENMPMACHINE),1)
test-openmp: $(EXEC) test-python-extension
	PYTHONPATH="$(MDIR)/python" $(TEST_PYTHON) tests/test_openmp.py --fkpt2 "$(EXEC)"
else
test-openmp:
	@printf '%s\n' 'Use make OPENMPMACHINE=1 test-openmp.' >&2
	@exit 1
endif

CLASS_BIN ?= $(CLASS_DIR)/class
CLASSY_PATH ?= $(MDIR)/build/classy
TEST_CLASS ?= auto
CLASS_TEST_PYTHONPATH = $(MDIR)/python:$(CLASSY_PATH)$(if $(PYTHONPATH),:$(PYTHONPATH))

# Check only when make test is requested and share the suite's import path.
ifneq ($(filter test,$(MAKECMDGOALS)),)
ifeq ($(TEST_CLASS),0)
test:
	@printf '%s\n' 'Skipping CLASS tests (TEST_CLASS=0).'
else
ifneq ($(TEST_CLASS),auto)
ifneq ($(TEST_CLASS),1)
$(error Invalid TEST_CLASS '$(TEST_CLASS)'. Use auto, 0, or 1)
endif
endif

CLASS_TEST_MISSING := $(shell \
	{ test -f "$(CLASS_DIR)/libclass.a" && test -r "$(CLASS_DIR)/libclass.a"; } || \
		printf '[%s]\n' $(call shell_quote,Missing readable CLASS archive $(CLASS_DIR)/libclass.a); \
	{ test -f "$(CLASS_BIN)" && test -x "$(CLASS_BIN)"; } || \
		printf '[%s]\n' $(call shell_quote,Missing CLASS executable at CLASS_BIN=$(CLASS_BIN)); \
	PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=$(call shell_quote,$(CLASS_TEST_PYTHONPATH)) \
		$(TEST_PYTHON) -c 'from classy import Class' >/dev/null 2>&1 || \
		printf '[%s]\n' $(call shell_quote,Unable to import classy using TEST_PYTHON=$(TEST_PYTHON) and CLASSY_PATH=$(CLASSY_PATH)))
ifeq ($(CLASS_TEST_MISSING),)
test: test-class
else
ifeq ($(TEST_CLASS),1)
$(error TEST_CLASS=1 requires the CLASS dependencies. $(CLASS_TEST_MISSING))
else
test:
	@printf '%s\n' 'Skipping CLASS tests (need libclass.a, an executable CLASS_BIN, and importable classy).'
endif
endif
endif
endif

.PHONY: test-class test-class-bash test-class-python-runner
test-class: $(EXEC) test-python-extension test-class-bash test-class-python-runner \
            test-class-multipoles-both
	PYTHONPATH=$(call shell_quote,$(CLASS_TEST_PYTHONPATH)) \
		$(TEST_PYTHON) tests/test_class_workflow.py --class-bin "$(CLASS_BIN)"

test-class-bash: $(EXEC)
	$(TEST_PYTHON) tests/test_class_bash.py --fkpt2 "$(EXEC)" --class-bin "$(CLASS_BIN)"

test-class-python-runner: test-python-extension
	PYTHONPATH=$(call shell_quote,$(CLASS_TEST_PYTHONPATH)) \
		$(TEST_PYTHON) tests/test_class_python_runner.py

.PHONY: test-class-native test-class-paramfile test-class-multipoles test-class-multipoles-both test-classy-paramfile
test-class-multipoles-both:
	+MAKE=$(call shell_quote,$(MAKE)) $(TEST_PYTHON) tests/run_class_multipoles_both.py \
		--class-dir "$(CLASS_DIR)" --class-bin "$(CLASS_BIN)" --classy-path "$(CLASSY_PATH)"

test-class-multipoles: $(EXEC) test-python-extension
	PYTHONPATH=$(call shell_quote,$(CLASS_TEST_PYTHONPATH)) \
		$(TEST_PYTHON) tests/test_class_multipoles.py --fkpt2 "$(EXEC)" --class-bin "$(CLASS_BIN)" \
		$(CLASS_MULTIPOLES_REFERENCE_ARG)

ifneq ($(filter fkpt2_class_addon,$(FKPT2_ADDON_DESCRIPTORS)),)
test-class-native: $(EXEC)
	$(TEST_PYTHON) tests/test_class_native.py --fkpt2 "$(EXEC)" --class-bin "$(CLASS_BIN)"

test-cython: test-class-paramfile
test-class-paramfile: $(EXEC) test-python-extension
	PYTHONPATH="$(MDIR)/python" $(TEST_PYTHON) tests/test_class_paramfile.py --fkpt2 "$(EXEC)"

test-classy-paramfile:
	@printf '%s\n' 'Use make CLASSLIBON=0 $@.' >&2
	@exit 1
else
# Build the linked reference separately so Bash can use the active build.
CLASS_MULTIPOLES_REFERENCE_DIR = $(WRKDIR)/class-multipoles-reference
CLASS_MULTIPOLES_REFERENCE_ARG = --linked-reference "$(CLASS_MULTIPOLES_REFERENCE_DIR)/fkpt2-exe"
.PHONY: test-class-multipoles-reference
test-class-multipoles: test-class-multipoles-reference
test-class-multipoles-reference:
	+$(MAKE) CLASSLIBON=1 WRKDIR="$(CLASS_MULTIPOLES_REFERENCE_DIR)" \
		"$(CLASS_MULTIPOLES_REFERENCE_DIR)/fkpt2-exe"

test-class: test-classy-paramfile
test-classy-paramfile: test-python-extension
	PYTHONPATH=$(call shell_quote,$(CLASS_TEST_PYTHONPATH)) \
		$(TEST_PYTHON) tests/test_classy_paramfile.py

test-class-native test-class-paramfile:
	@printf '%s\n' 'Use make CLASSLIBON=1 $@.' >&2
	@exit 1
endif

test-addons:
	$(TEST_PYTHON) tests/test_addons.py --cc "$(CC)"

.PHONY: clean
clean:
	rm -rf build dist python/fkpt2.egg-info
	$(RM) "$(EXEC)" "lib$(EXEC).a" python/fkpt2*.so python/fkpt2*.pyd
