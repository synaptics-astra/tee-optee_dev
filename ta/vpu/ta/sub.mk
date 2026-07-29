_sub_mk_path := $(abspath $(lastword $(MAKEFILE_LIST)))
# end without slash
cur_dir := $(patsubst %/,%,$(dir $(_sub_mk_path)))

COMPAT_VERS ?= v2
DEBUG_FW ?= 0
CHIP_NAME ?=
TEE_VER := OPTEE
$(info ### AMP_BUILD_TOP: $(AMP_BUILD_TOP))

include $(cur_dir)/build/common/chip.mk
ifeq ($(CHIP_NAME),)
    $(error "CHIP_NAME can not be empty!")
endif

VPU_PLUGIN_INC := $(cur_dir)/../plugin/inc
VPU_LIB_INC := $(cur_dir)/../vpu_lib/$(CHIP_NAME)/include
VPU_LIB_PATH := $(cur_dir)/../vpu_lib/$(CHIP_NAME)/arm-aarch64

TA_SRC_FILES := $(cur_dir)/vpu_entry.c
TA_SRC_FILES += $(wildcard $(cur_dir)/common/src/*.c)

TA_CFLAGS := -Wno-unused-parameter $(CHIP_CFLAGS)
TA_CFLAGS += -I $(cur_dir) \
             -I $(cur_dir)/common/inc/ \
             -I $(VPU_LIB_INC) \
             -I $(VPU_PLUGIN_INC)

TA_CFLAGS += -DSUPPORT_TRUSTED_DEVICE \
             -DTA_COMPILE \
             -DCONFIG_OPTEE

TA_CFLAGS += -Wall -Werror

$(info ### COMPAT_VERS: $(COMPAT_VERS))
$(info ### DEBUG_FW: $(DEBUG_FW))

ifeq ($(COMPAT_VERS),)
    $(error "COMPAT_VERS can not be empty!")
endif

ifneq ($(findstring v1, $(COMPAT_VERS)),)
    include $(cur_dir)/v1/v1.mk
    TA_COMMON_CFLAGS += -DVMETA_V1_EN
    TA_CFLAGS += -I $(cur_dir)/v1/inc/
endif

ifneq ($(findstring v2, $(COMPAT_VERS)),)
    include $(cur_dir)/v2/v2.mk
    TA_COMMON_CFLAGS += -DVMETA_V2_EN
    TA_CFLAGS += -I $(cur_dir)/v2/inc/
endif

ifeq ($(DEBUG_FW), 1)
TA_CFLAGS += -DDEBUG_FW
libnames += vcodec_fw.dbg
else
libnames += vcodec_fw
endif

#global-incdirs-y += build/optee/
libdirs += $(VPU_LIB_PATH)
srcs-y += $(TA_SRC_FILES)
cflags-y += $(TA_CFLAGS) $(TA_COMMON_CFLAGS)