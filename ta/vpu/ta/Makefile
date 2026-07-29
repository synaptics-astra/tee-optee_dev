##### TZK Makefile for vpu_ta #####
# Mandatory Envs:
# SDK_PATH :=
# PRODUCT :=

# Optional Envs:
# COMPAT_VERS           :=     # TA compatible versions,
#                              # available options: v1, v2. default: v2.
# DEBUG_FW              :=     # available options: 0, 1. default: 0.

my_path := $(abspath $(lastword $(MAKEFILE_LIST)))
# end without slash
cur_dir := $(patsubst %/,%,$(dir $(my_path)))

COMPAT_VERS ?= v1
DEBUG_FW ?= 0
CHIP_NAME ?=
TEE_VER := TZK
$(info ### AMP_BUILD_TOP: $(AMP_BUILD_TOP))

ifeq ($(AMP_BUILD_TOP), true)
    ifeq ($(AMPSDKTOP),)
        $(error "AMPSDKTOP can not be empty in amp build top!")
    endif
    BUILDSYS := $(AMPSDKTOP)/build/scripts
else
    include $(cur_dir)/build/tzk/tzk_env.mk
endif

include $(cur_dir)/build/common/chip.mk
ifeq ($(CHIP_NAME),)
    $(error "CHIP_NAME can not be empty!")
endif

### must be in the beginning of file ###
include $(BUILDSYS)/common.mak

VPU_LIB_INC := $(cur_dir)/../vpu_lib/$(CHIP_NAME)/include
VPU_LIB_PATH := $(cur_dir)/../vpu_lib/$(CHIP_NAME)/arm-eabi

TA_MODULE := libvmeta.ta


TA_SRC_FILES := $(cur_dir)/vpu_entry.c
TA_SRC_FILES += $(wildcard $(cur_dir)/common/src/*.c)

TA_CFLAGS := -Wno-unused-parameter $(CHIP_CFLAGS)
TA_CFLAGS += -I $(cur_dir) \
             -I $(cur_dir)/common/inc/ \
             -I $(VPU_LIB_INC)

TA_CFLAGS += -DTA_COMPILE -DCONFIG_TZK
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
TA_LDFLAGS += -L $(VPU_LIB_PATH) -lvcodec_fw.dbg
else
TA_LDFLAGS += -L $(VPU_LIB_PATH) -lvcodec_fw
endif

include $(BUILDSYS)/ta.mak

ifneq ($(AMP_BUILD_TOP), true)
include $(BUILDSYS)/ta_rules.mak
endif

