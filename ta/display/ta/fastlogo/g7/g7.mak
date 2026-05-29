#/* SPDX-License-Identifier: GPL-2.0 */
#/* Copyright (C) 2025 Synaptics Incorporated */

#BG7 specific extension Makefile

ver=
ifeq ($(SoC_Ver), $(filter $(SoC_Ver), dolphin))
    chip = dolphin
ifeq ($(SoC_Subversion), A0)
    ver = a0
endif
endif
ifeq ($(SoC_Ver), $(filter $(SoC_Ver), platypus))
    chip = platypus
endif

COMMON_PATH     := $(TA_PATH)/../../hal/common

########chip head file#####
ifeq ($(CONFIG_AMP_THDR), y)
    TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/FE/include/thdr
    TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/THDR/include
    TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/THDR/include/LUT
endif

########VPP source code####
ifeq ($(DV), 1)
    TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/DV/dolby/include
    TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/DV/dolby/source/tcgen
    TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/DV/pipe/include
    TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/DV/pipe/include/VS6xx
    TA_C_INCLUDES  += $(AMPSDKTOP)/amp/src/ddl/comp_dvs/dolby_vision/vdr_rpu/include
    ifeq ($(SoC_Ver), $(filter $(SoC_Ver), platypus))
        TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/DV/pipe/include/regdump/platypus
    else
        TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/DV/pipe/include/regdump/dolphinxx
    endif
else
    TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/DUMMY/DV/include
endif

##########VPP driver source code##########
ifeq ($(DV), 1)
    TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/DV/dolby/source/tcgen/*.c)
    TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/DV/dolby/source/*.c)
    TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/DV/pipe/source/*.c)
else
    TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/DUMMY/DV/source/*.c)
endif
ifeq ($(CONFIG_AMP_THDR), y)
    TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/FE/source/thdr/*.c)
    TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/FE/source/thdr/vitm_luts/*.c)
    TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/THDR/source/*.c)
endif
ifeq ($(CONFIG_AMP_DCM), y)
    TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/FE/source/dcm/*.c)
endif

TA_C_INCLUDES += $(VPPTA_SPECIFIC_FOLDER)/../ta_wrapper/common
TA_SRC_FILES += $(wildcard $(COMMON_PATH)/include/mmap/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/../ta_wrapper/common/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/../ta_wrapper/source/$(chip)$(ver)/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/MISC/source/$(chip)$(ver)/*.c)

TA_CFLAGS += -DHDCP_DEBUG_DISABLE
TA_CFLAGS += -DVPP_UNDERFLOW_FIX
TA_CFLAGS += -DVPP_SW_BRINGUP
TA_CFLAGS += -DHDCP_ENABLED=0
TA_CFLAGS += -DHDCP2X_ENABLED=0

ifeq ($(SoC_Ver), $(filter $(SoC_Ver), dolphin))
    TA_CFLAGS += -DENABLE_GFX2_VOP2_PATH
    TA_CFLAGS += -DENABLE_PIP_GFX0_PATH
endif
