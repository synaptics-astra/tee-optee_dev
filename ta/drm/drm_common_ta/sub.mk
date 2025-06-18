### MODULE SOURCE FILE ###
TA_SRC_FILES  := $(wildcard src/bcm/*.c) \
                 $(wildcard src/keytable/*.c) \
                 $(wildcard src/log/*.c) \
                 $(wildcard src/memory/*.c)

TA_CFLAGS := -Wno-unused-parameter

TA_CFLAGS += -DSUPPORT_TRUSTED_DEVICE \
             -DTA_COMPILE \
             -DCONFIG_OPTEE

global-incdirs-y += inc
global-incdirs-y += inc/bcm
srcs-y += $(TA_SRC_FILES)
cflags-y += $(TA_CFLAGS)

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
