# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2025 Synaptics Incorporated

CHIP_SPECIFIC_FOLDER := dolphin
SoC_Ver := VS680
SoC_Subversion := A0
TA_PATH := .
HAL_PATH := ../../hal
VPP_PATH := ../vpp
COMMON_PATH := ../../common
FASTLOGO_PATH := ../fastlogo

TA_INC_DIRS = \
                $(HAL_PATH)/dhub/include \
                $(HAL_PATH)/fastlogo/include \
                $(VPP_PATH)/lib/osal/include \
                $(VPP_PATH)/lib/dhub/configs/$(CHIP_SPECIFIC_FOLDER) \
                $(HAL_PATH)/common/include \
                $(VPP_PATH)/lib/dhub/include \
                $(VPP_PATH)/g7/ta_wrapper/common \
                $(VPP_PATH)/g7/ta_wrapper/include \
                $(VPP_PATH)/g7/vppdrv/source/include \
                $(VPP_PATH)/g7/vppdrv/source/MISC/include \
                $(COMMON_PATH)/include \
                $(COMMON_PATH)/ddl/vcodec/vpu_lib/dolphin/include \
                $(COMMON_PATH)/include/dolphin \
                $(COMMON_PATH)/ta_include \
                $(FASTLOGO_PATH)/optee \
                ./ta_wrapper/include \


TA_SRC_FILES    += $(wildcard ./ta_wrapper/source/tee_ta_mmap_dhub.c)
TA_SRC_FILES    += $(HAL_PATH)/common/include/mmap/tee_ta_mmap_helper.c
TA_SRC_FILES    += $(wildcard ./ta_wrapper/source/tee_ta_dhub_memmap.c)
TA_SRC_FILES    += $(wildcard ./ta_wrapper/source/tee_ta_dhub.c)
TA_SRC_FILES    += $(wildcard $(VPP_PATH)/lib/dhub/source/*.c)
TA_SRC_FILES    += $(wildcard $(VPP_PATH)/lib/dhub/configs/$(CHIP_SPECIFIC_FOLDER)/*.c)
TA_SRC_FILES    += $(wildcard $(VPP_PATH)/lib/osal/alcore/Tee/OSALMutex.c)

TA_CFLAGS+= -DCONFIG_OPTEE -DTZ_3_0 -DTA_COMPILE
TA_CFLAGS+= -DVPP_IN_TRUST_ZONE -DNON_OS -DBERLIN_SINGLE_CPU
TA_CFLAGS+= -DSUPPORT_TRUSTED_DEVICE=1 -DCONFIG_AMP_DHUB_2ND_DMA -DCONFIG_AMP_TEE_ENABLE

global-incdirs-y += $(TA_INC_DIRS)
srcs-y += $(TA_SRC_FILES)
cflags-y += $(TA_CFLAGS)
