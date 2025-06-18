my_path := $(abspath $(lastword $(MAKEFILE_LIST)))
cur_dir := $(dir $(my_path))

CC ?= $(CROSS_COMPILE)gcc

PLUGIN_UUID = 1316a183-894d-43fe-9893-bb946ae103f0
PLUGIN      = $(PLUGIN_UUID).plugin
PLUGIN_SRC  = $(wildcard $(cur_dir)/src/*.c)
PLUGIN_OBJ  = $(patsubst %.c, %.o, $(PLUGIN_SRC))

PLUGIN_INC = -I$(cur_dir)/inc \
             -I$(TEEC_EXPORT)/include

PLUGIN_CCFLAGS = $(CFLAGS) $(PLUGIN_INC) -Wall -fPIC -Werror
PLUGIN_LDFLAGS = $(LDFLAGS) -shared

.PHONY: all, clean
all: $(PLUGIN)

$(PLUGIN): $(PLUGIN_OBJ)
	$(CC) $(PLUGIN_LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(PLUGIN_CCFLAGS) -c $< -o $@

clean:
	$(RM) $(PLUGIN_OBJ) $(PLUGIN_UUID).plugin
