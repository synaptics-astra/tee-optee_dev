TA_COMMON_CFLAGS += -DBERLIN_PLATYPUS=0x3140 \
                    -DBERLIN_DOLPHIN_A0=0x3150

ifeq ($(CHIP_FULL_NAME), dolphin_a0)
# Register address and size are defined in Firmware_Berlin_XXX_A0.
TA_COMMON_CFLAGS += -DBERLIN_CHIP_VERSION=BERLIN_DOLPHIN_A0 \
                    -DMEMMAP_CHIP_CTRL_REG_BASE=0xF7EA0000 \
                    -DMEMMAP_CHIP_CTRL_REG_SIZE=0x10000 \
                    -DRA_Gbl_decoderClk=0x074C \
                    -DRA_Gbl_encoderClk=0x0754 \
                    -DMEMMAP_V4G_REG_BASE=0xF7600000 \
                    -DMEMMAP_V4G_REG_SIZE=0x200000 \
                    -DMEMMAP_H1_REG_BASE=0xF7000000 \
                    -DMEMMAP_H1_REG_SIZE=0x400000 \
                    -DMEMMAP_G1_REG_BASE=0xF7910000 \
                    -DMEMMAP_G1_REG_SIZE=0x10000
else ifeq ($(CHIP_FULL_NAME), platypus)
# Register address and size are defined in Firmware_Berlin_XXX_A0.
TA_COMMON_CFLAGS += -DBERLIN_CHIP_VERSION=BERLIN_PLATYPUS \
                    -DMEMMAP_CHIP_CTRL_REG_BASE=0xF7EA0000 \
                    -DMEMMAP_CHIP_CTRL_REG_SIZE=0x10000 \
                    -DRA_Gbl_decoderClk=0x074C \
                    -DRA_Gbl_encoderClk=0x0754 \
                    -DMEMMAP_V4G_REG_BASE=0xF7600000 \
                    -DMEMMAP_V4G_REG_SIZE=0x100000 \
                    -DMEMMAP_H1_REG_BASE=0xF7000000 \
                    -DMEMMAP_H1_REG_SIZE=0x1000 \
                    -DMEMMAP_G1_REG_BASE=0xF7910000 \
                    -DMEMMAP_G1_REG_SIZE=0x10000
else # CHIP_FULL_NAME
$(error "Unknown CHIP_FULL_NAME: $(CHIP_FULL_NAME)")
endif # CHIP_FULL_NAME end.