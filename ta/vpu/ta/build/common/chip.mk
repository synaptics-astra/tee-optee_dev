
CHIP_CFLAGS := -DBERLIN_DOLPHIN=0x3130 \
               -DBERLIN_PLATYPUS=0x3140 \
               -DBERLIN_DOLPHIN_A0=0x3150

CHIP_CFLAGS += -DBERLIN_VS680=0x3130 \
               -DBERLIN_VS640=0x3140 \
               -DBERLIN_VS680_A0=0x3150

ifneq ($(CHIP_FULL_NAME), )
    CHIP_NAME := $(firstword $(subst _, , $(CHIP_FULL_NAME)))
else ifeq ($(SoC_Ver), $(filter $(SoC_Ver), VS680 VS640))
    CHIP_NAME := $(shell echo $(SoC_Ver)|tr A-Z a-z)
else
    $(error "CHIP_FULL_NAME or SoC_Ver must be specified!")
endif

_SET_CHIP_VER :=
ifneq ($(AMP_BUILD_TOP), true)
    _SET_CHIP_VER := 1
endif
ifeq ($(TEE_VER), OPTEE)
    _SET_CHIP_VER := 1
endif

ifneq ($(_SET_CHIP_VER),)
ifeq ($(CHIP_NAME), $(filter $(CHIP_NAME), platypus vs640))
    CHIP_CFLAGS += -DBERLIN_CHIP_VERSION=BERLIN_PLATYPUS
endif

ifeq ($(CHIP_NAME), $(filter $(CHIP_NAME), dolphin vs680))
    CHIP_CFLAGS += -DBERLIN_CHIP_VERSION=BERLIN_DOLPHIN_A0
endif
endif

$(info ### CHIP_NAME: $(CHIP_NAME))
#$(info ### CHIP_CFLAGS: $(CHIP_CFLAGS))