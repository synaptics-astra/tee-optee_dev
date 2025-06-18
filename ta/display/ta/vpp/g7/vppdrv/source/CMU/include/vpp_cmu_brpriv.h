/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRBRPRIV__
#define __BDRBRPRIV__

/*-----------------------------------------------------------------------------
 * Static
 *-----------------------------------------------------------------------------
 */
#define BR_NUM_UNITS 2

static VPP_CMU_INPUT_SIGNAL_TYPE CmuBrInputType[2];

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */

#define CMU_BR_NUM_UNITS 2

// Total no. of registers
#define CMU_BR_REG_NUM  6

// Offset macros
#define BR_EN_OFFSET  0
#define BR_TH1_OFFSET 2
#define BR_TH2_OFFSET 4

#define BR_REG_WRITE(vpp_obj,UnitID,Offset,Val,RetVal) \
    if (UnitID == VPP_MAIN) {\
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, \
                    (vpp_obj->base_addr) + ((CMU_BR_M_EN + Offset) << 2),Val); \
        if (RetVal != MV_VPP_OK) return RetVal;\
    }\
    if (UnitID == VPP_PIP) {\
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf , \
                    (vpp_obj->base_addr) + ((CMU_BR_P_EN + Offset) << 2),Val); \
        if (RetVal != MV_VPP_OK) return RetVal;\
    }\

/*-----------------------------------------------------------------------------
 * Enums and Structures
 *-----------------------------------------------------------------------------
 */
struct tagCMU_BR_EN_REGISTER_BITS
{
   REG_WIDTH en                       : 4;
   REG_WIDTH taper                    : 1;
   REG_WIDTH taper_size               : 3;
} PACKED_STRUCT;

typedef volatile struct tagCMU_BR_EN_REGISTER_BITS CMU_BR_EN_REGISTER_BITS;

union tagCMU_BR_REG
{
   CMU_BR_EN_REGISTER_BITS Bits;
   REG_WIDTH Value;
}PACKED_STRUCT;

typedef volatile union tagCMU_BR_REG CmuBrenreg;
static CmuBrenreg CmuBrEnReg;

/*-----------------------------------------------------------------------------
 * Get/Set Functions
 *-----------------------------------------------------------------------------
 */
#define HwiCmuBrSetMode(vpp_obj,UnitID,Val,RetVal) \
{ \
   CmuBrEnReg.Bits.en = Val; \
   BR_REG_WRITE(vpp_obj,UnitID,BR_EN_OFFSET,CmuBrEnReg.Value,RetVal); \
}

/*
 * Two side taper/One side taper
 */
#define HwiCmuBrSetTaper(vpp_obj,UnitID,Val,RetVal) \
{ \
   CmuBrEnReg.Bits.taper = Val; \
   BR_REG_WRITE(vpp_obj,UnitID,BR_EN_OFFSET,CmuBrEnReg.Value,RetVal); \
}

/*
 * Brightness Taper size region.
 */
#define HwiCmuBrSetTaperSize(vpp_obj,UnitID,Val,RetVal) \
{\
   CmuBrEnReg.Bits.taper_size = Val; \
   BR_REG_WRITE(vpp_obj,UnitID,BR_EN_OFFSET,CmuBrEnReg.Value,RetVal); \
}

/*
 * Brightness control threshold
 */
#define HwiCmuBrSetThreshold1(vpp_obj,UnitID,Val,RetVal) \
{ \
   BR_REG_WRITE(vpp_obj,UnitID,BR_TH1_OFFSET,(Val & 0xFF),RetVal); \
   BR_REG_WRITE(vpp_obj,UnitID,BR_TH1_OFFSET + 1,(Val >> 8 & 0x03),RetVal); \
}

#define HwiCmuBrSetThreshold2(vpp_obj,UnitID,Val,RetVal) \
{ \
   BR_REG_WRITE(vpp_obj,UnitID,BR_TH2_OFFSET,(Val & 0xFF),RetVal); \
   BR_REG_WRITE(vpp_obj,UnitID,BR_TH2_OFFSET + 1,(Val >> 8 & 0x03),RetVal); \
}

#endif

