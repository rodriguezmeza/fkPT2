ifeq ($(CLASSLIBON),1)
CLASS_SOURCE_DIR := $(abspath $(CLASS_DIR))
CLASS_ARCHIVE := $(CLASS_SOURCE_DIR)/libclass.a

# The C++ runtime must match the compiler used for the CLASS archive.
CLASS_CXX ?= $(CXX)
CLASS_UNAME := $(shell uname -s)
ifeq ($(CLASS_UNAME),Darwin)
ifneq ($(findstring clang,$(shell $(CLASS_CXX) --version)),)
CLASS_CXX_LIBS ?= -lc++
endif
endif
CLASS_LIB_SUFFIX := $(if $(filter Darwin,$(CLASS_UNAME)),dylib,so)
CLASS_STDCXX := $(shell $(CLASS_CXX) -print-file-name=libstdc++.$(CLASS_LIB_SUFFIX))
CLASS_CXX_LIBS ?= $(if $(filter /%,$(CLASS_STDCXX)),"$(CLASS_STDCXX)",-lstdc++)

OPT2 += -DFKPT2_LINKED_CLASS
FKPT2_ADDON_SOURCES += addons/class_spectrum/class_backend.c addons/class_spectrum/class_spectrum.c
FKPT2_ADDON_DESCRIPTORS += fkpt2_class_addon
FKPT2_ADDON_LINK_DEPS += $(CLASS_ARCHIVE)
FKPT2_ADDON_LINK_LIBS += "$(CLASS_ARCHIVE)" $(CLASS_CXX_LIBS)

CLASS_INCLUDE_DIRS := $(addprefix $(CLASS_SOURCE_DIR)/,include external/HyRec2020 external/RecfastCLASS external/heating)
FKPT2_CPPFLAGS_class_backend += $(foreach dir,$(CLASS_INCLUDE_DIRS),-I"$(dir)")
$(WRKDIR)/class_backend.o: $(wildcard $(addsuffix /*.h,$(CLASS_INCLUDE_DIRS)))

$(CLASS_ARCHIVE):
	@printf '%s\n' 'Build the CLASS archive first with make -C "$(CLASS_SOURCE_DIR)" libclass.a.' >&2
	@exit 1
endif
