### must be in the beginning of file ###
include $(BUILDSYS)/common.mak
TA_PATH := ./

### MODULE NAME ###
TA_MODULE     := libm2m.ta

### MODULE SOURCE FILE ###
TA_SRC_FILES += $(wildcard ./source/*.c)
TA_SRC_FILES += ../tsp/source/tsp_mem_map.c

### MODULE CFLAGS/LDFLAGS ###
TA_CFLAGS+=  -Wall -Werror -DTA_COMPILE  \
    -I../external_headers \
    -I../tsp/include \
    -I../tsp/tsp_fw/include/allinone \
    -I./include  \
    -I$(AMPSDKTOP)/ta_amp/drmse_ta/inc/ \
    -I$(AMPSDKTOP)/ta_amp/drm_common/inc/ \
    -I$(AMPSDKTOP)/ta_amp/drm_common/inc/bcm \
    -I$(AMPSDKTOP)/ta_amp/bridge/drmse/include \
    $(TEE_EXT_INC) \
    -I$(TA_PATH)/comm/include/  \
    -I$(AMPSDKTOP)/amp/src/ddl/dmx/tsp_driver/src/ta/include/  \
    -I$(AMPSDKTOP)/amp/src/ddl/dmx/tsp_driver/inc/ \
    -I$(AMPSDKTOP)/amp/src/ddl/dmx/comm/include/ \
    -I$(TA_PATH)../tsp/ta/include \
    -I$(TA_PATH)../drmse_ta/inc/ \
    -I$(TA_PATH)../drm_common/inc/ \
    -I$(TA_PATH)../drm_common/inc/bcm

ifeq ($(SoC_Ver), $(filter $(SoC_Ver), VS550 BG5 BG4))
TA_CFLAGS += -I$(AMPSDKTOP)/amp/src/ddl/dmx/tsp_driver/inc/ucode/legacy
else
#VS680 VS640 and future chips
TA_CFLAGS += -I$(AMPSDKTOP)/amp/src/ddl/dmx/tsp_driver/inc/ucode/allinone
endif

TA_CFLAGS += -DCONFIG_ENABLE_DRM

ifdef CONFIG_GENX_ENABLE
$(info "CONFIG_GENX_ENABLE is defined")
TA_CFLAGS += -DCONFIG_GENX_ENABLE
else
# for syna release build GENX_ENBALE is not set
ifeq ($(SoC_Ver), VS640)
$(info "CONFIG_GENX_ENABLE is defined by SoC_Ver")
TA_CFLAGS += -DCONFIG_GENX_ENABLE
else
$(info "CONFIG_GENX_ENABLE is not defined")
endif
endif

### must be in the end of file ###
include $(BUILDSYS)/ta.mak
