#/* SPDX-License-Identifier: GPL-2.0 */
#/* Copyright (C) 2025 Synaptics Incorporated */

#The main makefile for this TA

#Top level Macros needed by this TA
#g7 chip family
G7_CHIP_LIST := dolphinZ1 platypusA0 dolphinA0

TA_PATH := .

#Top level common makefile for any TA
include optee_common.mak

#Actual makefile for this TA
include optee_g7.mak

TA_INC_DIRS += \
	$(TA_C_INCLUDES)

global-incdirs-y +=$(TA_INC_DIRS)

srcs-y += $(TA_SRC_FILES)

TA_CFLAGS +=  -Wno-unused -DTA_COMPILE -DLINUX -DSUPPORT_TRUSTED_DEVICE=1 -D__linux__ -DCONFIG_OPTEE

cflags-y += $(TA_CFLAGS)
