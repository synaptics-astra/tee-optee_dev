# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2026 Synaptics Incorporated

TA_PATH := .

SoC_Subversion := A0

#TA_MODULE - already defined in Makefile.op
#TA_MODULE = libovp.ta

COMMON_PATH	:= $(TA_PATH)/../../common
COMMON_HAL_PATH	:= $(TA_PATH)/../../hal/common/include

#Initalize empty string to TA_SRC_FILES
TA_SRC_FILES :=

ver=
ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), dolphinZ1 dolphinA0))
    chip = dolphin
ifeq ($(SoC_Subversion), A0)
    ver = a0
endif
endif

ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), platypusA0))
    chip = platypus
endif

# Enable below define to dump OVP regs for debugging
#TA_CFLAGS += -DDEBUG_DUMP_OVP_REGS

TA_CFLAGS += -DNON_OS
TA_CFLAGS += -DCONFIG_AMP_TEE_ENABLE
TA_CFLAGS += -DBERLIN -DBERLIN_SINGLE_CPU

TA_C_INCLUDES  += $(TA_PATH)
########common header file#####
TA_C_INCLUDES  += $(COMMON_HAL_PATH)/
TA_C_INCLUDES  += $(COMMON_PATH)/ta_include/
TA_C_INCLUDES  += $(COMMON_PATH)/include/
TA_C_INCLUDES  += $(COMMON_PATH)/include/$(chip)
TA_C_INCLUDES  += $(COMMON_PATH)/ddl/vcodec/vpu_lib/$(chip)/include
########OVP head file#####
TA_C_INCLUDES  += $(TA_PATH)/include
TA_C_INCLUDES  += $(TA_PATH)/source/include
TA_C_INCLUDES  += $(TA_PATH)/ta_wrapper/include
########DEINT head file#####
TA_C_INCLUDES  += $(TA_PATH)/source/DEINT/include

##########OVP source code##############
TA_SRC_FILES += $(wildcard $(TA_PATH)/source/ovp_api.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/source/ovp_apifuncs_prv.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/source/ovp_debug.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/source/ovp_logger.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/source/ovp_isr.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/source/ovp_optee_missing.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/source/DEINT/source/ovp_deint_api.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/source/DEINT/source/ovp_deint_cfg.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/ta_wrapper/source/tee_ta_ovp.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/ta_wrapper/source/tee_ta_mmap_ovp.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/ta_wrapper/source/tee_ta_buffer.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/ta_wrapper/source/tee_ta_buffer_check.c)
TA_SRC_FILES += $(wildcard $(TA_PATH)/ta_wrapper/source/tee_ta_ovp_shm_cmd.c)
##########common helper code##############
TA_SRC_FILES += $(wildcard $(COMMON_HAL_PATH)/mmap/tee_ta_mmap_helper.c)
