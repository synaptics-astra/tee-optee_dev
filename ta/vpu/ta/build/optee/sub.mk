my_path := $(abspath $(lastword $(MAKEFILE_LIST)))
cur_dir := $(dir $(my_path))

include $(CURDIR)/build/common/chip.mk

CHIP_NAME = $(firstword $(subst _, , $(CHIP_FULL_NAME)))
VPU_PLUGIN_INC := $(CURDIR)/../plugin/inc
VPU_LIB_INC := $(CURDIR)/../vpu_lib/$(CHIP_NAME)/include
VPU_LIB_PATH := $(CURDIR)/../vpu_lib/$(CHIP_NAME)/arm-aarch64

TA_SRC_FILES := $(CURDIR)/vmeta_entry.c

TA_CFLAGS := -Wno-unused-parameter
TA_CFLAGS += -I $(CURDIR)/common/inc/ \
             -I $(VPU_LIB_INC) \
             -I $(VPU_PLUGIN_INC)
TA_CFLAGS += -DSUPPORT_TRUSTED_DEVICE \
             -DTA_COMPILE \
             -DCONFIG_OPTEE

COMPAT_VERS ?= v2
DEBUG_FW ?= 0

$(info ### COMPAT_VERS: $(COMPAT_VERS))
$(info ### DEBUG_FW: $(DEBUG_FW))

ifeq ($(COMPAT_VERS),)
    $(error "COMPAT_VERS can not be empty!")
endif

ifneq ($(findstring v1, $(COMPAT_VERS)),)
    include $(CURDIR)/v1/v1.mk
    TA_COMMON_CFLAGS += -DVMETA_V1_EN
endif

ifneq ($(findstring v2, $(COMPAT_VERS)),)
    include $(CURDIR)/v2/v2.mk
    TA_COMMON_CFLAGS += -DVMETA_V2_EN
endif

ifeq ($(DEBUG_FW), 1)
libnames += vcodec_fw.dbg
else
libnames += vcodec_fw
endif

global-incdirs-y += build/optee/
libdirs += $(VPU_LIB_PATH)
srcs-y += $(TA_SRC_FILES)
cflags-y += $(TA_CFLAGS) $(TA_COMMON_CFLAGS)