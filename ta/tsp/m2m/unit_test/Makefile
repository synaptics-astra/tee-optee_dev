my_path := $(abspath $(lastword $(MAKEFILE_LIST)))
cur_dir := $(dir $(my_path))

MODULE = m2m_test

CROSS_COMPILE ?=
OUT_DIR ?= $(CURDIR)/out

$(info CROSS_COMPILE: $(CROSS_COMPILE))

ifeq ($(V),)
	Q = @
else
	Q =
endif

CPP_SOURCES = $(wildcard $(CURDIR)/*.cpp)


TA_CLIENT_KIT ?=
TA_CLIENT_LIB_PATH ?=
TSP_TA_HEADER_PATH ?= $(CURDIR)/../../tsp/include
M2M_TA_HEADER_PATH ?= $(CURDIR)/../include
GTEST_HEADER_PATH ?=
GTEST_SRC_PATH ?=
GTEST_LIB_PATH ?=

$(info TA_CLIENT_LIB_PATH: $(TA_CLIENT_LIB_PATH))
$(info TA_CLIENT_KIT: $(TA_CLIENT_KIT))
$(info GTEST_HEADER_PATH: $(GTEST_HEADER_PATH))
$(info GTEST_LIB_PATH: $(GTEST_LIB_PATH))

ifneq ($(GTEST_SRC_PATH),)
CC_SOURCES = $(GTEST_SRC_PATH)/gtest-all.cc
else
LDFLAGS += -lgtest
LDFLAGS += -L$(GTEST_LIB_PATH)
endif

OBJS = $(filter %.o,$(patsubst %.cpp,%.o,$(CPP_SOURCES)) \
    $(patsubst %.cc,%.o,$(CC_SOURCES)))

$(info OBJS: $(OBJS))

OBJ = $(addprefix $(OUT_DIR)/, $(OBJS))

$(info OBJ: $(OBJ))

CXXFLAGS = -I$(CURDIR)/include \
		   -I$(TSP_TA_HEADER_PATH) \
		   -I$(M2M_TA_HEADER_PATH) \
		   -I$(GTEST_HEADER_PATH) \
		   -I$(GTEST_HEADER_PATH)/../ \
		   -I$(TA_CLIENT_KIT)/libteec/include \

CXXFLAGS += -Wunused -Wunused-local-typedefs -Wunused-parameter \
			-Wunused-value  -Wunused-variable \
			-D__LITTLE_ENDIAN__ -DCONFIG_OPTEE -DTRUSTZONE

CXXFLAGS += -std=c++17

LDFLAGS += -L$(TA_CLIENT_LIB_PATH)
LDFLAGS += -pthread -lteec

%.o : %.cc %.cpp

.PHONY : all clean

all: $(OUT_DIR)/$(MODULE)

-include $(OBJ:.o=.d)

$(OUT_DIR)/%.d : %.cpp
		$(Q)mkdir -p $(dir $@)
			$(Q)$(CROSS_COMPILE)-$(CXX) $(CXXFLAGS) -MM $< > $@

$(OUT_DIR)/%.d : %.cc
		$(Q)mkdir -p $(dir $@)
			$(Q)$(CROSS_COMPILE)-$(CXX) $(CXXFLAGS) -MM $< > $@

$(OUT_DIR)/%.o : %.cpp $(OUT_DIR)/%.d
		$(Q)$(CROSS_COMPILE)-$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/%.o : %.cc $(OUT_DIR)/%.d
		$(Q)$(CROSS_COMPILE)-$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/$(MODULE) : $(OBJ)
		$(Q)$(CROSS_COMPILE)-$(CXX) $^ $(LDFLAGS) -o $@

clean:
		$(Q)rm -rf $(OUT_DIR)
