CFG_TEE_TA_LOG_LEVEL ?= 4
CFG_TA_OPTEE_CORE_API_COMPAT_1_1 = y

# CFG_SYNA_PTA=y (encryption with synaptics keys and certs)
# CFG_SYNA_PTA="" (3rd-party encryption)
CFG_SYNA_PTA=y

BINARY := 1316a183-894d-43fe-9893-bb946ae103f0

include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk
