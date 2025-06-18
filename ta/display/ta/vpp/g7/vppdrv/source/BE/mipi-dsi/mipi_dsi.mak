#/* SPDX-License-Identifier: GPL-2.0 */
#/* Copyright (C) 2025 Synaptics Incorporated */

#needs to be moved to top level make file later
MIPI_DSI_VENDOR := SYNOPSYS

ifeq ($(MIPI_DSI_VENDOR), SYNOPSYS)
MIPI_SYNOPSYS_DIR := $(VPPDRV_PATH)/source/BE/mipi-dsi/vendor/synopsys/
$(warning snps dir : $(MIPI_SYNOPSYS_DIR))
TA_SRC_FILES += $(wildcard $(MIPI_SYNOPSYS_DIR)/source/*.c)
TA_SRC_FILES += $(wildcard $(MIPI_SYNOPSYS_DIR)/hal/*.c)
TA_SRC_FILES += $(wildcard $(MIPI_SYNOPSYS_DIR)/panel/*.c)
TA_SRC_FILES += $(wildcard $(VPPDRV_PATH)/source/BE/mipi-dsi/*.c)

TA_C_INCLUDES += $(MIPI_SYNOPSYS_DIR)/include
TA_C_INCLUDES += $(MIPI_SYNOPSYS_DIR)/source
TA_C_INCLUDES += $(MIPI_SYNOPSYS_DIR)/hal
TA_C_INCLUDES += $(MIPI_SYNOPSYS_DIR)/panel

TA_CFLAGS += -DGEN_3 -DDISPLAYS
endif
#common

