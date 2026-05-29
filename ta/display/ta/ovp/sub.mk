#/* SPDX-License-Identifier: GPL-2.0 */
#/* Copyright (C) 2026 Synaptics Incorporated */

#The main makefile for this TA

TA_PATH := .

#Actual makefile for this TA
include optee.mak

TA_INC_DIRS += \
	$(TA_C_INCLUDES)

global-incdirs-y +=$(TA_INC_DIRS)

srcs-y += $(TA_SRC_FILES)

TA_CFLAGS +=  -Wno-unused -DTA_COMPILE -DLINUX -DSUPPORT_TRUSTED_DEVICE=1 -D__linux__ -DCONFIG_OPTEE

cflags-y += $(TA_CFLAGS)
