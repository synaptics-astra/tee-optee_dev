include m2m_common.mak

DRM_COMMON_TA_HEADER_PATH ?=
DRMSE_TA_HEADER_PATH ?= $(DRM_COMMON_TA_HEADER_PATH)
BCM_HEADER_PATH ?=

$(info DRM_COMMON_TA_HEADER_PATH: $(DRM_COMMON_TA_HEADER_PATH))
$(info DRMSE_TA_HEADER_PATH: $(DRMSE_TA_HEADER_PATH))
$(info BCM_HEADER_PATH: $(BCM_HEADER_PATH))

#TODO:decouple with external headers,will remove external headers after dependencies are merged
TA_INC_DIRS += \
	../external_headers \
	../tsp/include \
	../tsp/tsp_fw/include/allinone \
	./include  \

incdirs-y += $(TA_INC_DIRS)
# global include is for "#include <tsp_cmd.h>", the header file define
# TA_M2M_UUID. used by user_ta_header_defines.h
TA_GLOBAL_INC_DIRS := ../tsp/include
global-incdirs-y += $(TA_GLOBAL_INC_DIRS)

TA_SRC_FILES += $(wildcard ./source/*.c)
TA_SRC_FILES += ../tsp/source/tsp_mem_map.c

srcs-y += $(TA_SRC_FILES)

TA_CFLAGS+= -Wall -Wno-unused -Werror -DCONFIG_OPTEE -DTA_COMPILE -DLINUX -DSUPPORT_TRUSTED_DEVICE=1 -DTZ_3_0 -D__linux__
TA_CFLAGS += -DCONFIG_ENABLE_DRM

#TODO:will uncomment these after dependencies merged
#TA_CFLAGS += -I$(DRM_COMMON_TA_HEADER_PATH)
#TA_CFLAGS += -I$(DRMSE_TA_HEADER_PATH)
#TA_CFLAGS += -I$(BCM_HEADER_PATH)

cflags-y += $(TA_CFLAGS)