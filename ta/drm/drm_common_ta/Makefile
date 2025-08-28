### must be in the beginning of file ###
include ../../build/common.mak

### MODULE NAME ###
TA_LIB_NAME = libdrm_common_ta

### MODULE SOURCE FILE ###
TA_SRC_FILES  := $(wildcard src/bcm/*.c) \
                 $(wildcard src/keytable/*.c) \
                 $(wildcard src/log/*.c) \
                 $(wildcard src/memory/*.c)

### MODULE CFLAGS/LDFLAGS ###
TA_CFLAGS+= -Wall -Werror \
            -I./inc \
            -I./inc/bcm

######################################################
# Module Public header files
PUBLIC_HEADER_FILES :=  $(wildcard inc/*.h)  \
                        $(wildcard inc/bcm/*.h)

### must be at the end of file ###
include ../../build/ta_static_lib.mak
include ../../build/ta_rules.mak
