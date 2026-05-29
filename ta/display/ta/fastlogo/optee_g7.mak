#/* SPDX-License-Identifier: GPL-2.0 */
#/* Copyright (C) 2025 Synaptics Incorporated */

#This is optee version of ta_amp/fastlogo/fl_g7.mak
#Following are the addtions - VPPDRV_PATH1, VPP_ENABLE_MEMID_BY_VIRT_ADDR

TA_PATH := .

SoC_Subversion := A0

#TA_MODULE - already defined in Makefile.op
#TA_MODULE = libfastlogo.ta

#Initialize common variable, these can be overridden by chip specific Makefile
#Path for various important folders
ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), $(G7_CHIP_LIST)))
    CHIP_SPECIFIC_FOLDER := $(TA_PATH)/g7
    VPPTA_SPECIFIC_FOLDER := $(TA_PATH)/../vpp/g7/vppdrv
endif
TA_LIB_PATH     := $(TA_PATH)
DHUB_PATH       := $(VPPTA_SPECIFIC_FOLDER)/../../lib/dhub
OSAL_PATH       := $(VPPTA_SPECIFIC_FOLDER)/../../lib/osal
COMMON_PATH     := $(TA_PATH)/../../hal/common
VPPDRV_PATH1     := $(CHIP_SPECIFIC_FOLDER)/vppdrv
TAWRAPPER_PATH  := $(CHIP_SPECIFIC_FOLDER)/ta_wrapper
DRM_COMMON_PATH := $(TA_PATH)/../drm_common

#Initalize empty string to TA_SRC_FILES
TA_SRC_FILES :=

ver=
ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), dolphinZ1 dolphinA0))
    chip = dolphin
ifeq ($(SoC_Subversion), A0)
    ver = a0
endif
endif

ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), platypusA0))
    chip = platypus
endif

ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), $(G7_CHIP_LIST)))
    include $(TA_PATH)/g7/g7.mak
endif

###############FASTLOGO specific############################
#TA_CFLAGS += -DVPP_ENABLE_FASTLOGO_TESTAPP
TA_CFLAGS += -DVPP_ENABLE_INTERNAL_MEM_MGR
TA_CFLAGS += -DVPP_ENABLE_MEMID_BY_VIRT_ADDR
#Limit the memory chunk size allocated from internal memory manager to 5KB
#This should not defined for fastlogo, as CB feature is not available
#TA_CFLAGS += -DSHM_MAX_CHUNK_SZ=512000
#Limit the one time memory allocation size to reduce bitBlock/sizeBlock size
#SHM_MAX_CHUNK_SZ usually disabled, So 1.25MB enough for MV_VPPOBJ_Create()
TA_CFLAGS += -DBM_SHM_ONE_TIME_ALLOCATION_SIZE=1024*1024*1.25
TA_CFLAGS += -DCONFIG_VPP_FASTLOGO_SUPPORT
TA_CFLAGS += -DCONFIG_VPP_FASTLOGO_PUSHPAR_SUPPORT
#TA_CFLAGS += -DCONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
#TA_CFLAGS += -Werror
###############VPP specific############################
TA_CFLAGS += -DSoC_Galois
#VPP_GAMMA_FORMAT Currently support(RGB&YUV)
# We need to update value in CONFIG_VPP_GAMMA_FORMAT for RGB  = 0 & YUV  = 1
TA_CFLAGS += -DCONFIG_VPP_GAMMA_FORMAT=0
#CONFIG_VPP_ENABLE_HUE_BYPASS Enable=1, Disable=0
TA_CFLAGS += -DCONFIG_VPP_ENABLE_HUE_BYPASS=1
#DISABLE_CROP_FOR_ALL_PLANE Disable=1, Enable=0
TA_CFLAGS += -DDISABLE_CROP_FOR_ALL_PLANE=0

#Macro's defined to identify trustzone compilation
TA_CFLAGS += -DVPP_IN_TRUST_ZONE -DNON_OS

TA_CFLAGS += \
    -DPE_3D_FMT_CONVERT \
    -DPE_3D_BD

TA_CFLAGS += \
    -DBERLIN \
    -DBERLIN_SINGLE_CPU \
    -DGPIO_I2C

TA_CFLAGS += -DCONFIG_VPP_AUTOPUSH_SUPPORT
# TA_CFLAGS += -I$(AMPSDKTOP)/drm/common/inc
TA_CFLAGS += -DVPP_OTP_GET_PROPERTY_FROM_DRMSE_TA
# TA_CFLAGS += -I$(AMPSDKTOP)/ta_amp/bridge/otp/inc
# TA_CFLAGS += -I$(AMPSDKTOP)/ta_amp/drm_common/inc
#TA_CFLAGS += -DCONFIG_MV_AMP_HDMI_SPD_SUPPORT

#TA_CFLAGS += -DVPP_SHM_ALLOC_UNCACHEABLE
#TA_CFLAGS += -DDISABLE_BCM_ENGINE
#TA_CFLAGS += -DVPP_ISR_POLLING

#Enable this flag to do REFWIN/DISPWIN unit testing with VPP_Test_APP
#TA_CFLAGS += -DVPP_TEST_APP

#TA_CFLAGS += -Werror -DSoC_Galois -D__CODE_LINK__=0 -Wno-error=overflow
TA_CFLAGS += -DVPP_SUS_RES
TA_CFLAGS += -DVPP_SUSPEND_RESUME_CLKGATE_ENABLE
TA_CFLAGS += -DVPP_ENABLE_AVIO_RESET_DURING_SUSPEND_RESUME

TA_C_INCLUDES  += $(TA_PATH)
TA_C_INCLUDES  += $(TA_PATH)/optee
########chip head file#####
TA_C_INCLUDES  += $(COMMON_PATH)/include/
TA_C_INCLUDES  += $(TA_PATH)/../../common/ta_include/
#TA_C_INCLUDES  += $(TA_PATH)/chip/$(chip)/include
TA_C_INCLUDES  += $(TA_PATH)/../../common/include/
# TA_C_INCLUDES  += $(TA_PATH)/../../../../../common
#TA_C_INCLUDES  += $(TA_PATH)/../../common/inc
#TA_C_INCLUDES  += $(AMPSDKTOP)/amp/inc
########OSAL head file#####
TA_C_INCLUDES  += $(OSAL_PATH)/include
########DHUB head file#####
TA_C_INCLUDES  += $(DHUB_PATH)/include
TA_C_INCLUDES  += $(DHUB_PATH)/configs/$(chip)
########VPP CA head file#####
TA_C_INCLUDES  += $(TA_PATH)/../../common/ddl/vcodec/vpu_lib/$(chip)/include
TA_C_INCLUDES  += $(TA_PATH)/../../hal/fastlogo/include

########DRM_COMMON head file#####
########VPP source code head file####
TA_C_INCLUDES  += $(TAWRAPPER_PATH)/include

#TA_C_INCLUDES  += $(VPPDRV_PATH1)/source/include
TA_C_INCLUDES  += $(VPPDRV_PATH1)/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/BE/CEC/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/MISC/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/UTIL/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/BE/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/FE/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/FE/include/dcm
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/SCL/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/CPCB/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/CSC/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/DETEE/include
TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/CMU/include
# ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), $(G7_CHIP_LIST)))
# TA_C_INCLUDES  += $(VPPTA_SPECIFIC_FOLDER)/source/NGPTV/include
# endif
ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), dolphinZ1))
    TA_C_INCLUDES   += $(VPPTA_SPECIFIC_FOLDER)/source/MISC/source/dolphinZ1
else
    TA_C_INCLUDES   += $(VPPTA_SPECIFIC_FOLDER)/source/MISC/source/common
endif

TA_SRC_FILES +=  $(wildcard $(TA_PATH)/optee/*.c)

NGPTV_TA_DUMMY_PATH := source/DUMMY/NGPTV
NGPTV_TA_PATH := $(NGPTV_TA_DUMMY_PATH)

NGPTV_TA_SRC_PATH := $(NGPTV_TA_PATH)/source
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/$(NGPTV_TA_SRC_PATH)/*.c)

NGPTV_TA_INC_PATH := $(NGPTV_TA_PATH)/include
NGPTV_INC := $(VPPTA_SPECIFIC_FOLDER)/$(NGPTV_TA_INC_PATH)
TA_C_INCLUDES  += $(NGPTV_INC)
ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), dolphinA0))
    #TA_C_INCLUDES  += $(NGPTV_INC)/vs6xx/dolphina0
    TA_C_INCLUDES  += $(NGPTV_INC)/regdump/dolphinxx
else
    ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), dolphinZ1))
        TA_C_INCLUDES  += $(NGPTV_INC)/regdump/dolphinxx
    else
        TA_C_INCLUDES  += $(NGPTV_INC)/regdump/platypus
    endif
    #TA_C_INCLUDES  += $(NGPTV_INC)/vs6xx
endif

#VMX header files
VMX_TA_DUMMY_PATH := source/DUMMY/VMX
VMX_TA_PATH := $(VMX_TA_DUMMY_PATH)

VPPVMX_PATH     := $(VPPTA_SPECIFIC_FOLDER)/$(VMX_TA_PATH)/vpp_vmx
#TA_C_INCLUDES   += $(VPPVMX_PATH)/include

VPPVMX_PATH1    := $(VPPTA_SPECIFIC_FOLDER)/$(VMX_TA_PATH)
TA_C_INCLUDES   += $(VPPVMX_PATH1)/include
TA_C_INCLUDES   += $(VPPVMX_PATH1)/include/regdump
#TA_C_INCLUDES   += $(VPPVMX_PATH1)/include/VS6xx
TA_SRC_FILES    += $(wildcard $(VPPVMX_PATH1)/source/*.c)
TA_SRC_FILES    += $(wildcard $(VPPVMX_PATH)/source/*.c)

##########Common source code##############
TA_SRC_FILES += $(wildcard $(DHUB_PATH)/source/hal_dhub.c)
TA_SRC_FILES += $(wildcard $(DHUB_PATH)/source/avio_dhub_drv.c)
TA_SRC_FILES += $(wildcard $(DHUB_PATH)/configs/$(chip)/avio_dhub_cfg.c)
##########VPP driver source code##########
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/FE/source/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/SCL/source/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/CPCB/source/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/CSC/source/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/BE/source/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/UTIL/source/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/MISC/source/*.c)
#TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/MISC/source/$(chip)$(ver)/*.c)


TA_SRC_FILES += $(wildcard $(VPPDRV_PATH1)/source/*.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_obj.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_cpcb.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_csc.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_cfg.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_fe.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_scl.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_offlineScl.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_clk.c)

ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), $(G7_CHIP_LIST)))
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_avioMtr.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_dv_wrap.c)
TA_SRC_FILES += $(wildcard $(VPPTA_SPECIFIC_FOLDER)/source/vpp_be_hdr.c)
endif

##########VPP TA interface source#########
TA_SRC_FILES += $(wildcard $(TAWRAPPER_PATH)/source/*.c)

##########OSAL source code##############
TA_SRC_FILES += $(wildcard $(OSAL_PATH)/alcore/Tee/OSALMutex.c)
TA_SRC_FILES += $(wildcard $(OSAL_PATH)/alcore/Tee/OSALSem.c)

ifeq ($(SoC_Ver)$(SoC_Subversion), $(filter $(SoC_Ver)$(SoC_Subversion), $(G7_CHIP_LIST)))
VPPDRV_PATH := $(VPPTA_SPECIFIC_FOLDER)
#HDMI Makefile
include $(VPPDRV_PATH)/source/BE/hdmi/hdmi.mak
#MIPI DSI Makefile
include $(VPPDRV_PATH)/source/BE/mipi-dsi/mipi_dsi.mak
endif
