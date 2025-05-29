#/* SPDX-License-Identifier: GPL-2.0 */
#/* Copyright (C) 2025 Synaptics Incorporated */

#needs to be moved to top level make file later
HDMI_VENDOR := SYNOPSYS

ifeq ($(HDMI_VENDOR), SYNOPSYS)
HDMI_SYNOPSYS_DIR := $(VPPDRV_PATH)/source/BE/hdmi/vendor/synopsys/
$(warning snps dir : $(HDMI_SYNOPSYS_DIR))
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/bsp/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/scdc/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/api/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/phy/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/edid/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/core/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/identification/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/hdcp/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/util/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_SYNOPSYS_DIR)/api/src/cec/*.c)

TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/api/include
TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/api/include/api
TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/api/include/bsp
TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/api/include/core
TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/api/include/identification
TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/api/include/phy
TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/api/include/util
TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/api/src
TA_C_INCLUDES += $(HDMI_SYNOPSYS_DIR)/

#ESM files

TA_CFLAGS += -DTROOT_APPID_MAX=6
TA_CFLAGS += -DELLIPTIC_DISABLE_SYSTEM_MUTEX -DELLIPTIC_DISABLE_SYSTEM_SEMAPHORE

#vendor_interface
HDMI_VENDOR_INTERFACE_DIR := $(VPPDRV_PATH)/source/BE/hdmi/vendor_interface/synopsys/
TA_SRC_FILES += $(wildcard $(HDMI_VENDOR_INTERFACE_DIR)/*.c)

ifeq ($(HDCP), 1)
ifeq ($(CONFIG_AMP_HDMI_HDCP_2X), y)
#hdcp2x_interface
HDMI_HDCP2X_INTERFACE_DIR := $(VPPDRV_PATH)/source/BE/hdmi/hdcp2x_interface/
TA_SRC_FILES += $(wildcard $(HDMI_HDCP2X_INTERFACE_DIR)/*.c)
TA_C_INCLUDES += $(AMPSDKTOP)/amp/src/hal/hdcp2x/include
TA_C_INCLUDES += $(VPPDRV_PATH)/source/BE/hdmi/hdcp2x_interface
endif
endif

ifeq ($(CONFIG_AMP_HDMI_AUDIO_RETURN), y)
ifeq ($(SoC_Ver)$(SoC_Subversion), dolphinZ1)
TA_CFLAGS += -DSNPS_ARC_ENABLE
else ifeq ($(SoC_Ver)$(SoC_Subversion), dolphinA0)
TA_CFLAGS += -DEARC_RX_ENABLED
include $(VPPDRV_PATH)/source/BE/hdmi/earc/earc.mak
endif
endif

TA_C_INCLUDES += $(HDMI_VENDOR_INTERFACE_DIR)

#vendor_hal
HDMI_VENDOR_HAL_DIR := $(VPPDRV_PATH)/source/BE/hdmi/vendor_hal/synopsys/
TA_SRC_FILES += $(wildcard $(HDMI_VENDOR_HAL_DIR)/*.c)
TA_SRC_FILES += $(wildcard $(HDMI_VENDOR_HAL_DIR)/esm/*.c)

TA_C_INCLUDES += $(HDMI_VENDOR_HAL_DIR)

endif
#common
HDMI_COMMON_DIR := $(VPPDRV_PATH)/source/BE/hdmi/common/
TA_SRC_FILES += $(wildcard $(HDMI_COMMON_DIR)/*.c)

