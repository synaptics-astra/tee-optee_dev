TA_INC_DIRS += \
	comm/include/  \
	inc/
global-incdirs-y +=$(TA_INC_DIRS)

TA_SRC_FILES:= $(wildcard ./src/*.c)
TA_SRC_FILES += $(wildcard ./comm/source/*.c)

srcs-y += $(TA_SRC_FILES)

TA_CFLAGS+=  -Wall -Wno-unused -Werror -DCONFIG_OPTEE -DTA_COMPILE -DLINUX -DSUPPORT_TRUSTED_DEVICE=1 -D__linux__

cflags-y += $(TA_CFLAGS)
