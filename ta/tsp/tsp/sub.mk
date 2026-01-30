include tsp_common.mak

BRIDGE_DRMSE_HEADER_PATH ?=
BRIDGE_DRMSE_LIB_PATH ?=
DRM_COMMON_TA_HEADER_PATH ?=
DRMSE_TA_HEADER_PATH ?= $(DRM_COMMON_TA_HEADER_PATH)
BCM_HEADER_PATH ?=

$(info BRIDGE_DRMSE_HEADER_PATH: $(BRIDGE_DRMSE_HEADER_PATH))
$(info BRIDGE_DRMSE_LIB_PATH: $(BRIDGE_DRMSE_LIB_PATH))
$(info DRM_COMMON_TA_HEADER_PATH: $(DRM_COMMON_TA_HEADER_PATH))
$(info DRMSE_TA_HEADER_PATH: $(DRMSE_TA_HEADER_PATH))
$(info BCM_HEADER_PATH: $(BCM_HEADER_PATH))

#TODO:decouple with external headers,will remove external headers after dependencies are merged
TA_INC_DIRS += \
	../external_headers \
	./include/  \
	./tsp_fw/include/allinone \

TSP_DHUB_PATH := ./dhublite
TA_INC_DIRS += $(TSP_DHUB_PATH)/include/

incdirs-y += $(TA_INC_DIRS)

# global include is for "#include <tsp_cmd.h>", the header file define
# TA_TSP_UUID. used by user_ta_header_defines.h
TA_GLOBAL_INC_DIRS := ./include/
global-incdirs-y += $(TA_GLOBAL_INC_DIRS)

TA_SRC_FILES:= $(wildcard ./source/*.c)
TA_SRC_FILES+= $(wildcard ./source/comm/*.c)
TA_SRC_FILES += $(wildcard $(TSP_DHUB_PATH)/source/*.c)

# TODO will uncomment these after dependencies merged, and for the drm feature just disable it temporarily
#libnames += bridge_drmse
#libdirs += $(BRIDGE_DRMSE_LIB_PATH)

srcs-y += $(TA_SRC_FILES)

TA_CFLAGS+= -Wall -Wno-unused -Werror -DCONFIG_OPTEE -DTA_COMPILE -DLINUX -DTZ_3_0 -DSUPPORT_TRUSTED_DEVICE=1 -D__linux__
TA_CFLAGS += -DUSE_DOCC_H

#TODO:CONFIG_ENABLE_DRM should dynamically set by the build system
#TODO:will uncomment these after dependencies merged

# TA_CFLAGS += -DCONFIG_ENABLE_DRM
# TA_CFLAGS += -I$(BRIDGE_DRMSE_HEADER_PATH)
# TA_CFLAGS += -I$(DRM_COMMON_TA_HEADER_PATH)
# TA_CFLAGS += -I$(BCM_HEADER_PATH)
# TA_CFLAGS += -I$(DRMSE_TA_HEADER_PATH)

cflags-y += $(TA_CFLAGS)
