### MODULE SOURCE FILE ###
TA_SRC_FILES  := $(wildcard src/*.c)

srcs-y += $(TA_SRC_FILES)

LIB_HEADER_INC_DIR += ../drm_common_ta/inc
LIB_HEADER_INC_DIR += ../drm_common_ta/inc/bcm/
LIB_HEADER_INC_DIR += inc

global-incdirs-y += $(LIB_HEADER_INC_DIR)

TA_CFLAGS += -DSUPPORT_TRUSTED_DEVICE \
             -DTA_COMPILE \
             -DCONFIG_OPTEE \
             -DTZ_3_0 \
             -D_CFG_TA_STACK_PROTECTOR

cflags-y += $(TA_CFLAGS)

libnames += drm_common_ta
libdirs +=$(TA_DEV_KIT_DIR)
# For libdrm_common_ta
libdirs +=${STAGING_DIR_HOST}${libdir}

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
