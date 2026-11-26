ifeq ($(CLPTON),1)
FKPT2_ADDON_SOURCES += addons/clpt/clpt.c addons/clpt/clpt_kernels.c addons/clpt/clpt_velocity.c
FKPT2_ADDON_DESCRIPTORS += fkpt2_clpt_addon
endif
