my_path := $(abspath $(lastword $(MAKEFILE_LIST)))
cur_dir := $(dir $(my_path))

TA_SRC_FILES += $(wildcard $(cur_dir)/src/*.c) \
                $(cur_dir)/../common/src/vmeta_log.c \
                $(cur_dir)/../common/src/vpu_hal_mem.c \
                $(cur_dir)/../common/src/vpu_obj_mgr.c

TA_CFLAGS += -I $(cur_dir)/inc