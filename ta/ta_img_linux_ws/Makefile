### must be in the beginning of file ###
include ../build/common.mak

TA_PATH := ./

### MODULE NAME ###
TA_MODULE     := libgfx_img_linux.ta

### MODULE SOURCE FILE ###
TA_LDFLAGS	:=
TA_C_INCLUDES	:= $(wildcard $(TA_PATH)/src)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/include)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/include/rogue)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/include/public)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/hwdefs/rogue/km/configs)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/hwdefs/rogue/km/cores)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/hwdefs/rogue/km/)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/hwdefs/rogue/)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/services/include)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/services/server/include)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/services/shared/include)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/services/server/devices)
TA_C_INCLUDES	+= $(wildcard $(TA_PATH)/services/server/devices/rogue/)


TA_SRC_FILES	:= src/tee_ta_pvr.c
TA_SRC_FILES	+= services/server/devices/rgxfwimageutils.c
TA_SRC_FILES	+= services/server/devices/rogue/rgxlayer_impl.c
TA_SRC_FILES	+= services/server/devices/rogue/rgxstartstop.c

TA_CFLAGS := -DLINUX -DSUPPORT_TRUSTED_DEVICE=1 -D__linux__
TA_CFLAGS += -DRGX_FW_HEAP_SHIFT=25 -DRGX_FW_HEAP_USES_FIRMWARE_OSID=0
TA_CFLAGS += -DRGX_NUM_DRIVERS_SUPPORTED=1 -DTEE_DDK

### MODULE CFLAGS/LDFLAGS ###
ifeq ($(CHIP_FULL_NAME),dolphin)
TA_CFLAGS += -DDOLPHIN
endif
ifeq ($(CHIP_FULL_NAME),dolphinZ1)
TA_CFLAGS += -DDOLPHIN
endif

ifeq ($(CHIP_FULL_NAME),platypus)
TA_CFLAGS += -DPLATYPUS
endif

### must be at the end of file ###
include ../build/ta.mak
include ../build/ta_rules.mak

