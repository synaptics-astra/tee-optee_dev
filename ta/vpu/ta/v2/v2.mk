v2_path := $(abspath $(lastword $(MAKEFILE_LIST)))
# end without slash
v2_dir := $(patsubst %/,%,$(dir $(v2_path)))

TA_SRC_FILES += $(wildcard $(v2_dir)/src/*.c)

TA_CFLAGS += -I $(v2_dir)/inc