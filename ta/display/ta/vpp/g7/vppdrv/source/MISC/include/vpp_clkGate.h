/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
//-----------------------------------------------------------------------------
//! \file       vpp_clkGate.h
//! \brief      The clock gate & SRAM power gate framework header file
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------


#ifndef _VPP_CLKGATE_H_
#define _VPP_CLKGATE_H_

#include "vpp_api.h"

#ifndef CONFIG_VPP_VIP_HDMIRX_CLOCKGATE_DISABLE
//Disable the HDMIRX CLOCK/POWER gating by commenting following macro, Default=Enable
#define CONFIG_VPP_VIP_HDMIRX_CLOCKGATE
#endif

//Disable VIP CLKGATING for BG4CDP-A0 for offline-SCALAR; until Dynamic gating is enabled

#define CONFIG_VPP_VIP_CLOCKGATE

//Enable plane/scl gating later - regression is seen - green screen because of not LUT programing
#define CONFIG_VPP_ENABLE_PLANE_CLKGATE
#define CONFIG_VPP_ENABLE_SCL_CLKGATE

//If VMX not required in CD platforms; then enable/disable VMX separately
//Disable the DEINT gating by commenting following macro, Default=Enable
#define VPP_FE_DEINT_POWERGATE_ENABLE

#ifdef VPP_IN_TRUST_ZONE
#define VPP_CLKGATE_PRINT(...)  VPPAPI_DBG_INFO(__VA_ARGS__)
#elif defined(VPP_DIAGS)
#define VPP_CLKGATE_PRINT(...)  printf(__VA_ARGS__)
#else
#define VPP_CLKGATE_PRINT(...)  printf(__VA_ARGS__)
#endif

//Data structure to store clock ate details
typedef struct __VPP_CLKGATE_IP_DETAILS__  {
    //Signature bit position
    UINT32 uiSignatureBitPos;
    //Name of the IP block
    char *pCIpBlockName;
    //Address of the register containing clock/RAM gating bits
    UINT32 uiRegAddress;
    //clock/RAM gating bits details - start and n bit
    UINT32 uiStartBitPos;
    UINT32 uiNBits;
    //clock/RAM gating bits details - value for byPass enabled
    UINT32 uiByPass;
    //string arrary to be used to convert the bit value to string
    UINT32 uiByPassStringNdx;
    //Pointer to shadow register
    UINT32 *pShadowRegister;
}VPP_CLKGATE_IP_DETAILS;

//Data structure to store power gate details
typedef struct __VPP_POWERGATE_DETAILS__ {
    //The unique ID to the power block
    UINT32 pwrBlkId;
    //The signature bit position - reference link to VPP_CLKGATE_IP_DETAILS structure
    UINT32 signatureBitPos;
    //Number of iteration from the 'signatureBitPos' bit position
    //If MSB set then use VPP_POWERGATE_BITMASK with single iteration
    UINT32 bitMaskIter;
}VPP_POWERGATE_DETAILS;

//The bit used to represent if VPP_POWERGATE_BITMASK to be used or not
#define VPP_POWERGATE_BITMASK_USEBIT    0x80000000

//List of feature options used internal to the VPP_CLKGATE module
#define VPP_CLKGATE_OPT_FUTURE_ENABLED  0x80000000

#define IS_VPP_CLKGATE_DELAYED_OPTION_ENABLED(var)  (var.opt & VPP_CLKGATE_OPT_FUTURE_ENABLED)
#define SET_VPP_CLKGATE_DELAYED_OPTION_ENABLE(var)  (var.opt |= VPP_CLKGATE_OPT_FUTURE_ENABLED)
#define SET_VPP_CLKGATE_DELAYED_OPTION_DISABLE(var) (var.opt &= ~VPP_CLKGATE_OPT_FUTURE_ENABLED)
#define DEC_VPP_CLKGATE_DELAYED_OPTION_VALUE(var)   (var.clkGateDelayedCount--)
#define IS_VPP_CLKGATE_DELAYED_OPTION_VALUE(var)    (var.clkGateDelayedCount)
#define SET_VPP_CLKGATE_DELAYED_OPTION_VALUE(var, value)   (var.clkGateDelayedCount = value)


//#ifdef EN_VPP_SRAM_CLKGATE
typedef enum __ENUM_VPP_SRAM_PWRGATE_BLK__ {
    //01 to 10
    VPP_SRAM_PWRGATE_BLK_GFX1_LB,
    VPP_SRAM_PWRGATE_BLK_GFX1_LUT,
    VPP_SRAM_PWRGATE_BLK_GFX0_LB,
    VPP_SRAM_PWRGATE_BLK_GFX0_LUT,
    VPP_SRAM_PWRGATE_BLK_HDRX_PIPE_SCL1D_LB,
    VPP_SRAM_PWRGATE_BLK_HDRX_PIPE_SCL1D_LUT,
    VPP_SRAM_PWRGATE_BLK_MAINSCL_LB,
    VPP_SRAM_PWRGATE_BLK_MAINSCL_LUT,
    VPP_SRAM_PWRGATE_BLK_MAINPL_UPS_420_422,
    VPP_SRAM_PWRGATE_BLK_HDRX_PIPE_VIP_UPS420,

    //11 to 20
    VPP_SRAM_PWRGATE_BLK_VMXIP,
    VPP_SRAM_PWRGATE_BLK_MAINPL_DIF_Y0,
    VPP_SRAM_PWRGATE_BLK_MAINPL_DIF_Y1,
    VPP_SRAM_PWRGATE_BLK_MAINPL_DIF_C0,
    VPP_SRAM_PWRGATE_BLK_MAINPL_DIF_C1,
    VPP_SRAM_PWRGATE_BLK_HDMIRX,
    VPP_SRAM_PWRGATE_BLK_HDMITX,
    VPP_SRAM_PWRGATE_BLK_AIO64_DHUB,
    VPP_SRAM_PWRGATE_BLK_VPP128_DHUB,
    VPP_SRAM_PWRGATE_BLK_GFX0_UPS420_SP,

    //21 to 30
    VPP_SRAM_PWRGATE_BLK_HDMI_VOP_DNS422_420,
    VPP_SRAM_PWRGATE_BLK_MAIN_DINT,
    VPP_SRAM_PWRGATE_BLK_MAIN_DINT_DMX,
    VPP_SRAM_PWRGATE_BLK_TGWRAP0,
    VPP_SRAM_PWRGATE_BLK_DVCOMP,
    VPP_SRAM_PWRGATE_BLK_DBVIP_ICSC,
    VPP_SRAM_PWRGATE_BLK_DBVIP_CVM_MAIN,
    VPP_SRAM_PWRGATE_BLK_DBVIP_CVM_GFX0,
    VPP_SRAM_PWRGATE_BLK_DBVIP_CVM_GFX1,
    VPP_SRAM_PWRGATE_BLK_EL_UPS_420_422_SP,
    VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_C0,
    VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_C1,
    VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_Y0,
    VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_Y1,
    VPP_SRAM_PWRGATE_BLK_OVPSCL_LUMA_LB,
    VPP_SRAM_PWRGATE_BLK_OVPSCL_LUMA_LUT,
    VPP_SRAM_PWRGATE_BLK_OVPSCL_CHROMA_LB,
    VPP_SRAM_PWRGATE_BLK_OVPSCL_CHROMA_LUT,
}ENUM_VPP_SRAM_PWRGATE_BLK;

typedef enum __ENUM_VPP_SRAM_PWRGATE__ {
    VPP_SRAM_PWRGATE_DISABLE = 0,
    VPP_SRAM_PWRGATE_ENABLE,
}ENUM_VPP_SRAM_PWRGATE;

typedef enum __ENUM_VPP_CLK_GATE__ {
    VPP_CLK_GATE_DISABLE = 0,
    VPP_CLK_GATE_ENABLE,
    VPP_CLK_GATE_IGNORE,
}ENUM_VPP_CLK_GATE;

typedef enum __VPP_POWERGATE_BITMASK_BIT__ {
    VPP_POWERGATE_BITMASK_BIT_PWDN     = 0x01,
    VPP_POWERGATE_BITMASK_BIT_PDLVMC   = 0x02,
    VPP_POWERGATE_BITMASK_BIT_PDFVSSM  = 0x04,
}VPP_POWERGATE_BITMASK_BIT;

#define VPP_POWERGATE_BITMASK  (VPP_POWERGATE_BITMASK_BIT_PWDN|VPP_POWERGATE_BITMASK_BIT_PDLVMC)

typedef enum __VPP_CLKGATE_OPTION_BIT__ {
    VPP_CLKGATE_OPTION_SEPARATE_BCM     = 0x01,
    VPP_CLKGATE_OPTION_SAME_BCM_WRITE   = 0x02,
    VPP_CLKGATE_OPTION_NO_SHADOW_REG    = 0x80,
}VPP_CLKGATE_OPTION_BIT;


/* Clock Gate related functions --------- start here */
void VPP_ClkGate_Initialize(VPP_OBJ *vpp_obj);

void VPP_ClkGate_Enable(VPP_OBJ *vpp_obj, int option);

void VPP_ClkGate_EnableGating(VPP_OBJ *vpp_obj, int clkGateNdx, ENUM_VPP_CLK_GATE isPowerDown,int option);

void VPP_ClkGate_triggerDelayOperation(VPP_OBJ *vpp_obj, ENUM_VPP_CLKGATE_OPT options);

void VPP_ClkGate_performDelayOperation(VPP_OBJ *vpp_obj);

void VPP_ClkGate_GetSignature(VPP_OBJ *vpp_obj, UINT32 *pSignatureCount,
        VPP_CLK_GATE_SIGNATURE *pSignature, VPP_CLKGATE_OPT options);

void VPP_ClkGate_SetSignature(VPP_OBJ *vpp_obj, VPP_CLK_GATE_SIGNATURE *pSignatureValue,
        VPP_CLK_GATE_SIGNATURE *pSignatureMask, VPP_CLKGATE_OPT options);

void VPP_ClkGate_FE_DEINT(VPP_OBJ *vpp_obj, ENUM_VPP_CLK_GATE enableClkGate);

void VPP_ClkGate_HDMIRX(VPP_OBJ *vpp_obj, UINT32 isClkGateEnabled);

int VPP_ClkGate_Plane(VPP_OBJ *vpp_obj, int planeID, int enable);
int VPP_ClkGate_SCL(VPP_OBJ *vpp_obj, int UnitNum, int enable);
/* Power Gate related functions --------- start here */
void VPP_SRAM_PowerGate(VPP_OBJ *vpp_obj, ENUM_VPP_SRAM_PWRGATE_BLK pwrGateBlk, ENUM_VPP_SRAM_PWRGATE isPowerDown);

void VPP_SRAM_PowerGate_HDMIRX(VPP_OBJ *vpp_obj, ENUM_VPP_SRAM_PWRGATE powerGateFlag);

void VPP_SRAM_PowerGate_SCL(VPP_OBJ *vpp_obj, VPP_FRC_SCL_NUM UnitNum, ENUM_VPP_SRAM_PWRGATE powerGateFlag);

void VPP_SRAM_PowerGate_Plane(VPP_OBJ *vpp_obj, ENUM_PLANE_ID planeID, ENUM_VPP_SRAM_PWRGATE powerGateFlag);

void VPP_SRAM_PowerGate_DEINT(VPP_OBJ *vpp_obj, ENUM_VPP_SRAM_PWRGATE powerGateFlag);

void VPP_SRAM_PowerGate_DIF(VPP_OBJ *vpp_obj, ENUM_PLANE_ID planeID, ENUM_VPP_SRAM_PWRGATE powerGateFlag);

void MV_VPP_EnableClkGate(void);
void MV_VPP_DisableClkGate(void);
void MV_RA_Gbl_ClkD2_Ctrl(UINT32 u32ClkRegAddr, BOOL bClkState);

#endif //_VPP_CLKGATE_H_



