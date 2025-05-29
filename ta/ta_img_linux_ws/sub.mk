TA_PATH		:= .
global-incdirs-y += include

global-incdirs-y	+= $(TA_PATH)/src
global-incdirs-y	+= $(TA_PATH)/include
global-incdirs-y	+= $(TA_PATH)/include/rogue
global-incdirs-y	+= $(TA_PATH)/include/public
global-incdirs-y	+= $(TA_PATH)/hwdefs/rogue/km/configs
global-incdirs-y	+= $(TA_PATH)/hwdefs/rogue/km/cores
global-incdirs-y	+= $(TA_PATH)/hwdefs/rogue/km/
global-incdirs-y	+= $(TA_PATH)/hwdefs/rogue/
global-incdirs-y	+= $(TA_PATH)/services/include
global-incdirs-y	+= $(TA_PATH)/services/server/include
global-incdirs-y	+= $(TA_PATH)/services/shared/include
global-incdirs-y	+= $(TA_PATH)/services/server/devices
global-incdirs-y	+= $(TA_PATH)/services/server/devices/rogue/


TA_SRC_FILES	:= src/tee_ta_pvr.c
TA_SRC_FILES	+= services/server/devices/rgxfwimageutils.c
TA_SRC_FILES	+= services/server/devices/rogue/rgxlayer_impl.c
TA_SRC_FILES	+= services/server/devices/rogue/rgxstartstop.c

srcs-y += $(TA_SRC_FILES)
TA_CFLAGS := -DLINUX -DSUPPORT_TRUSTED_DEVICE=1 -D__linux__ -DCONFIG_OPTEE
TA_CFLAGS += -DRGX_FW_HEAP_SHIFT=25 -DRGX_FW_HEAP_USES_FIRMWARE_OSID=0
TA_CFLAGS += -DRGX_NUM_DRIVERS_SUPPORTED=1 -DTEE_DDK

### MODULE CFLAGS/LDFLAGS ###
ifeq ($(CHIP_FULL_NAME),dolphin)
TA_CFLAGS += -DDOLPHIN
endif

ifeq ($(CHIP_FULL_NAME),platypus)
TA_CFLAGS += -DPLATYPUS
endif
cflags-y += $(TA_CFLAGS)

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
