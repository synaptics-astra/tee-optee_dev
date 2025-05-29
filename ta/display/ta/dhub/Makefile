# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2025 Synaptics Incorporated

### must be in the beginning of file ###
include $(BUILDSYS)/common.mak

### MODULE NAME ###
TA_MODULE     := libdhub.ta

### MODULE SOURCE FILE ###
TA_PATH = ./
CONFIG_PATH = ../vpp/lib/dhub

VPP_TA_COMMON_PATH = ../vpp/lib/common

TA_SRC_FILES    += $(wildcard $(TA_PATH)/source/dolphin/*.c)
TA_SRC_FILES    := $(wildcard ./source/*.c)

### MODULE CFLAGS/LDFLAGS ###
TA_LDFLAGS      :=
TA_C_INCLUDES   += ../../hal/dhub/include
TA_C_INCLUDES   += ../../common/include/dolphin

TA_CFLAGS       += -Werror

CHIP_SPECIFIC_FOLDER := dolphin

TA_C_INCLUDES   += -I$(CONFIG_PATH)/configs/$(CHIP_SPECIFIC_FOLDER)
TA_SRC_FILES    += $(wildcard $(CONFIG_PATH)/configs/$(CHIP_SPECIFIC_FOLDER)/*.c)

TA_C_INCLUDES   +=  -I$(VPP_TA_COMMON_PATH)/include

TA_CFLAGS += $(TA_C_INCLUDES)

### must be at the end of file ###
include $(BUILDSYS)/ta.mak
