# must be at the beginning of file
include ../../build/common.mak

### MODULE NAME ###
TA_MODULE     := libfactory.ta

### MODULE SOURCE FILE ###
TA_SRC_FILES  := $(wildcard src/*.c)

### MODULE CFLAGS/LDFLAGS ###
TA_CFLAGS+= -I$(LIB_HEADER_INC_DIR) \
            -Iinc

ifeq ($(CHIP_FULL_NAME), bg5ctA0)
	TA_CFLAGS += -DBG5CT_A0_DEFINED
endif

TA_CFLAGS += -Werror -DTZ_3_0

TA_LDFLAGS += -L$(LIB_DEPT_DIR) -ldrm_common_ta

### must be in the end of file ###
include ../../build/ta.mak
include ../../build/ta_rules.mak
