#/* SPDX-License-Identifier: GPL-2.0 */
#/* Copyright (C) 2025 Synaptics Incorporated */

#Subset of the ampsdk/build/scripts/common.mak

SoC_Subversion := A0
###############################################################################
ifeq ($(SoC_Ver), $(filter $(SoC_Ver), platypus dolphin))
B_CHIP_CFLAGS += -I$(TA_PATH)/../../common/include/$(SoC_Ver)/
endif

TA_CFLAGS += $(B_CHIP_CFLAGS)
TA_CFLAGS += -DCONFIG_AMP_TEE_ENABLE
#TA_CFLAGS += -DCONFIG_TEE
TA_CFLAGS += -DTZ_3_0
TA_CFLAGS += -DVPP_IN_TRUST_ZONE
TA_CFLAGS += -D_XOPEN_SOURCE=500
TA_CFLAGS += -DCONFIG_AMP_DHUB_2ND_DMA=y
TA_CFLAGS += -DCONFIG_ENABLE_VPP_TZ=y
#TA_CFLAGS += -DCONFIG_AMP_LIB_SEPARATE_VPPDRV=y
#TA_CFLAGS += -DCONFIG_AMP_DCM=y
#TA_CFLAGS += -DCONFIG_AMP_VPP_HDR=y
#TA_CFLAGS += -DCONFIG_DOLBY_EN_AOI=y
TA_CFLAGS += -DCONFIG_AMP_VPP_INVERSE_SCAN=y
TA_CFLAGS += -DCONFIG_AMP_VPP_ROTATION=y
