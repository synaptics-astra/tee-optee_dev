ifeq ($(TEE_CATEGORY), TZK)
    include $(CURDIR)/build/tzk/vmeta_tzk.mk
else ifeq ($(TEE_CATEGORY), OPTEE)
    $(shell cp $(CURDIR)/build/optee/sub.mk $(CURDIR))
    include $(CURDIR)/build/optee/vmeta_optee.mk
else
    $(error "TEE_CATEGORY must be spcified with a right value(TZK, OPTEE)!")
endif