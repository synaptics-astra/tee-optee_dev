### must be in the beginning of file ###
include $(BUILDSYS)/common.mak
TA_PATH := ./

### MODULE NAME ###
TA_MODULE     := libm2m.ta

### MODULE SOURCE FILE ###
TA_SRC_FILES:= $(wildcard $(TA_PATH)/comm/source/*.c)
TA_SRC_FILES+= $(wildcard $(AMPSDKTOP)/amp/src/ddl/dmx/tsp_driver/src/ta/source/*.c)
TA_SRC_FILES+= $(TA_PATH)/../tsp/ta/source/tsp_mem_map.c

### MODULE CFLAGS/LDFLAGS ###
TA_CFLAGS+=  -Wall -Werror -DTA_COMPILE  \
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

### must be in the end of file ###
include $(BUILDSYS)/ta.mak
