MOD=read_rkekid
all: $(MOD)

SOURCES = $(wildcard src/*.c) $(wildcard src/*/*.c)

INC_CFLAG = -Iinc \
            -I$(TEE_TOP_DIR)/include/client_api \
            -I$(TEE_TOP_DIR)/include/common \
            -I$(TEE_TOP_DIR)/include \
            -I$(SYSROOT_DIR)/include

$(MOD): $(SOURCES)
	$(CC) -fPIC $(E_CFLAG) $(INC_CFLAG)  $(E_LDFLAG) -Werror -lteec -pthread -O -o $(BUILD_DIR)/$@ $^

clean:
	@-rm $(BUILD_DIR)/$(MOD)

install:
	cp $(BUILD_DIR)/$(MOD) $(APP_DIR)

.PHONY: clean all install
