ifeq ($(GSMON)$(CLPTON),11)
FKPT2_ADDON_SOURCES += addons/gsm/gsm.c
FKPT2_ADDON_DESCRIPTORS += fkpt2_gsm_addon
FKPT2_ADDON_REQUIRES += fkpt2_clpt_addon
endif
