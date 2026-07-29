B_CHIP_CFLAGS :=-DBERLIN_VS680=0x3130 \
				-DBERLIN_VS640=0x3140 \
				-DBERLIN_VS680_A0=0x3150

###############################################################################
# VS680 VS640
###############################################################################
# CHIP_FULL_NAME := dolphin
$(info "Building for CHIP_FULL_NAME: $(CHIP_FULL_NAME)")
ifdef CHIP_FULL_NAME
	ifeq ($(CHIP_FULL_NAME), \
	$(filter $(CHIP_FULL_NAME), VS680 VS640 platypus dolphin VS640_rdk platypus_rdk dolphin_rdk VS680_rdk))

		ifeq ($(CHIP_FULL_NAME), $(filter $(CHIP_FULL_NAME), VS680 dolphin VS680_rdk dolphin_rdk))
			B_CHIP_CFLAGS +=-DBERLIN_CHIP_VERSION=BERLIN_VS680_A0
			CHIP = VS680_A0
		endif

		ifeq ($(CHIP_FULL_NAME), $(filter $(CHIP_FULL_NAME), VS640 platypus VS640_rdk platypus_rdk))
			B_CHIP_CFLAGS +=-DBERLIN_CHIP_VERSION=BERLIN_VS640
			CHIP = VS640_A0
		endif

		ifeq ($(CHIP_FULL_NAME), $(filter $(CHIP_FULL_NAME), VS640_rdk platypus_rdk dolphin_rdk VS680_rdk))
			B_CHIP_CFLAGS+=-DBERLIN_PRODUCT_FOR_RDK
		endif
		B_CHIP_CFLAGS +=-I./tsp_fw/include/Firmware_Berlin_$(CHIP)
	else
	$(error "CHIP_FULL_NAME defined error")
	endif
else
$(error "CHIP_FULL_NAME is not defined")
endif

ifdef CONFIG_GENX_ENABLE
$(info "CONFIG_GENX_ENABLE is defined")
B_CHIP_CFLAGS += -DCONFIG_GENX_ENABLE
else
$(info "CONFIG_GENX_ENABLE is not defined")
endif

TA_CFLAGS += $(B_CHIP_CFLAGS)
