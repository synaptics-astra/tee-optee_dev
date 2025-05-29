/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRHUESATPRIV__
#define __BDRHUESATPRIV__

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */
#define BDR_CMU_MAX_ICR_HS_AXES  14
#define CMU_HUESAT_MAIN_CHANNEL  0
#define CMU_HUESAT_PIP_CHANNEL   1

#define VPP_CMU_HUESAT_REG_NUM   54

/* Offsets */
#define CMU_HUESAT_EN            0x00
#define CMU_HUESAT_AX1_L         0x00
#define CMU_HUESAT_H1            0x1c
#define CMU_HUESAT_S1            0x2a
#define CMU_HUESAT_GL            0x39
#define CMU_HS_MAXSAT_RGBY_L     0x3b
#define CMU_HS_MAXSAT_RCR_L      0x3d
#define CMU_HS_MAXSAT_RCB_L      0x3f
#define CMU_HS_MAXSAT_GCR_L      0x41
#define CMU_HS_MAXSAT_GCB_L      0x43
#define CMU_HS_MAXSAT_BCR_L      0x45
#define CMU_HS_MAXSAT_BCB_L      0x47
#define CMU_HS_MAXSAT_ROFF_L     0x49
#define CMU_HS_MAXSAT_GOFF_L     0x4b
#define CMU_HS_MAXSAT_BOFF_L     0x4d


/* BCM Buffer write */
#define HUESAT_REG_ADDR_WRITE(UnitID,Offset,Val,RetVal) \
    if (UnitID == VPP_MAIN) { \
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + \
                     ((CMU_HS_M_EN + Offset) << 2)), Val);\
        if (RetVal != MV_VPP_OK) return RetVal;\
    } else if (UnitID == VPP_PIP) {\
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + \
                      ((CMU_HS_P_EN + Offset) << 2)), Val);\
        if (RetVal != MV_VPP_OK) return RetVal;\
    }

#define HUESAT_AXIS_REG_ADDR_WRITE(UnitID,Offset,Val,RetVal) \
    if (UnitID == VPP_MAIN) { \
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + \
                      ((CMU_HS_M_AX1_L + Offset) << 2)), Val);\
        if (RetVal != MV_VPP_OK) return RetVal;\
    } else if (UnitID == VPP_PIP) {\
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + \
                      ((CMU_HS_P_AX1_L + Offset) << 2)), Val);\
        if (RetVal != MV_VPP_OK) return RetVal;\
    }

/*-----------------------------------------------------------------------------
 * Static
 *-----------------------------------------------------------------------------
 */
static UCHAR HueSatInputSignalPip=0;

static VPP_CMU_INPUT_SIGNAL_TYPE CmuHueSatInputType[2];

/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */

// This is used to enable hue/saturation adjustment for Main and Pip channel.

struct tagCMU_HS_ENABLE_REGISTER_BITS
{
    REG_WIDTH HSEnable                : 1;
    REG_WIDTH IntelliSatEnable        : 1;
    REG_WIDTH FleshToneTaperEnable    : 1;
    REG_WIDTH Unused                  : 3;
} PACKED_STRUCT;

typedef volatile struct tagCMU_HS_ENABLE_REGISTER_BITS CMU_HS_ENABLE_REGISTER_BITS;

union tagCMU_HS_ENABLE_REGISTER
{
    CMU_HS_ENABLE_REGISTER_BITS Bits;
    REG_WIDTH Value;
}PACKED_STRUCT;

typedef union tagCMU_HS_ENABLE_REGISTER CMU_HS_ENABLE_REGISTER;
union tagCMU_HS_ENABLE_REGISTER gEnableReg;

/*-----------------------------------------------------------------------------
 * Get/Set Functions
 *-----------------------------------------------------------------------------
 */

/*
 * HUE SAT enable
 */

#define HwiCmuHSSetHSEnable(UnitID,Val,RetVal) \
{ \
    gEnableReg.Bits.HSEnable = Val; \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HUESAT_EN,gEnableReg.Value,RetVal); \
}

/*
 * Intelligent Saturation enable
 */

#define HwiCmuHSSetIntelligentSatEnable(UnitID,Val,RetVal) \
{ \
    gEnableReg.Bits.IntelliSatEnable = Val; \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HUESAT_EN,gEnableReg.Value,RetVal); \
}
/*
 *  Flesh tone taper saturation
 */

#define HwiCmuHSSetFleshToneTaperEnable(UnitID,Val,RetVal) \
{ \
    gEnableReg.Bits.FleshToneTaperEnable = Val; \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HUESAT_EN,gEnableReg.Value,RetVal); \
}

/*
 * HUE for axes 1.. 14 for Main channel
 */

#define HwiCmuHSSetHue(UnitID,Axis,Val,RetVal) \
{ \
    HUESAT_AXIS_REG_ADDR_WRITE(UnitID, CMU_HUESAT_AX1_L + Axis*2,  Val & 0xFF,RetVal); \
    HUESAT_AXIS_REG_ADDR_WRITE(UnitID, CMU_HUESAT_AX1_L + 1 + Axis*2, (Val >> 8) & 0xFF,RetVal); \
}

/*
 * HUE change for axes 1.. 14 for Main channel
 */

#define HwiCmuHSSetHueChange(UnitID,Axis,Val,RetVal) \
{ \
    HUESAT_AXIS_REG_ADDR_WRITE(UnitID, CMU_HUESAT_H1 + Axis,(Val),RetVal); \
}
/*
 * Saturation change for axes 1.. 14 for Main channel
 */

#define HwiCmuHSSetSatChange(UnitID,Axis,Val,RetVal) \
{ \
    HUESAT_AXIS_REG_ADDR_WRITE(UnitID,CMU_HUESAT_S1 + Axis,(Val),RetVal); \
}

/*
 * Global saturation change .
 */

#define HwiCmuHSSetGlobalSatChange(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HUESAT_GL,(Val),RetVal); \
}

/*
 * Max Saturation   RGB
 */

#define HwiCmuHSSetMaxSatRGBY(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_RGBY_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_RGBY_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * Max Saturation   RCR
 */

#define HwiCmuHSSetMaxSatRCR(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_RCR_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_RCR_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * Max Saturation   RCB
 */

#define HwiCmuHSSetMaxSatRCB(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_RCB_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_RCB_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * Max Saturation   GCR
 */
#define HwiCmuHSSetMaxSatGCR(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_GCR_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_GCR_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

 /*
  * Max Saturation   GCB
  */

#define HwiCmuHSSetMaxSatGCB(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_GCB_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_GCB_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * Max Saturation   BCR
 */

#define HwiCmuHSSetMaxSatBCR(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_BCR_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_BCR_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * Max Saturation   BCB
 */

#define HwiCmuHSSetMaxSatBCB(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_BCB_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_BCB_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * Offset to calculate R for YCbCr
 */

#define HwiCmuHSSetROff(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_ROFF_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_ROFF_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * Offset to calculate G for YCbCr for Main channel
 */

#define HwiCmuHSSetGOff(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_GOFF_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_GOFF_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * Offset to calculate B for YCbCr for Main channel
 */

#define HwiCmuHSSetBOff(UnitID,Val,RetVal) \
{ \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_BOFF_L,(Val & 0xFF),RetVal); \
    HUESAT_REG_ADDR_WRITE(UnitID,CMU_HS_MAXSAT_BOFF_L + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * HUE for axes 1.. 14 for Main channel
 */

#define GlobalCmuHSSetHue(Axis,Val) \
{ \
    switch(Axis) { \
        case VPP_CMU_HS_AXIS1: \
            stHsCfg0.uCFG0_AX1 = Val; \
            break; \
        case VPP_CMU_HS_AXIS2: \
            stHsCfg0.uCFG0_AX2 = Val; \
            break; \
        case VPP_CMU_HS_AXIS3: \
            stHsCfg0.uCFG0_AX3 = Val; \
            break; \
        case VPP_CMU_HS_AXIS4: \
            stHsCfg1.uCFG1_AX4 = Val; \
            break; \
        case VPP_CMU_HS_AXIS5: \
            stHsCfg1.uCFG1_AX5 = Val; \
            break; \
        case VPP_CMU_HS_AXIS6: \
            stHsCfg1.uCFG1_AX6 = Val; \
            break; \
        case VPP_CMU_HS_AXIS7: \
            stHsCfg2.uCFG2_AX7 = Val; \
            break; \
        case VPP_CMU_HS_AXIS8: \
            stHsCfg2.uCFG2_AX8 = Val; \
            break; \
        case VPP_CMU_HS_AXIS9: \
            stHsCfg2.uCFG2_AX9 = Val; \
            break; \
        case VPP_CMU_HS_AXIS10: \
            stHsCfg3.uCFG3_AX10 = Val; \
            break; \
        case VPP_CMU_HS_AXIS11: \
            stHsCfg3.uCFG3_AX11 = Val; \
            break; \
        case VPP_CMU_HS_AXIS12: \
            stHsCfg3.uCFG3_AX12 = Val; \
            break; \
        case VPP_CMU_HS_AXIS13: \
            stHsCfg4.uCFG4_AX13 = Val; \
            break; \
        case VPP_CMU_HS_AXIS14: \
            stHsCfg4.uCFG4_AX14 = Val; \
            break; \
    } \
}

/*
 * HUE change for axes 1.. 14 for Main channel
 */

#define GlobalCmuHSSetHueChange(Axis,Val) \
{ \
    switch(Axis) { \
        case VPP_CMU_HS_AXIS1: \
            stHsCfg4.uCFG4_H1 = Val; \
            break; \
        case VPP_CMU_HS_AXIS2: \
            stHsCfg4.uCFG4_H2 = Val; \
            break; \
        case VPP_CMU_HS_AXIS3: \
            stHsCfg5.uCFG5_H3 = Val; \
            break; \
        case VPP_CMU_HS_AXIS4: \
            stHsCfg5.uCFG5_H4 = Val; \
            break; \
        case VPP_CMU_HS_AXIS5: \
            stHsCfg5.uCFG5_H5 = Val; \
            break; \
        case VPP_CMU_HS_AXIS6: \
            stHsCfg5.uCFG5_H6 = Val; \
            break; \
        case VPP_CMU_HS_AXIS7: \
            stHsCfg5.uCFG5_H7 = Val; \
            break; \
        case VPP_CMU_HS_AXIS8: \
            stHsCfg5.uCFG5_H8 = Val; \
            break; \
        case VPP_CMU_HS_AXIS9: \
            stHsCfg6.uCFG6_H9 = Val; \
            break; \
        case VPP_CMU_HS_AXIS10: \
            stHsCfg6.uCFG6_H10 = Val; \
            break; \
        case VPP_CMU_HS_AXIS11: \
            stHsCfg6.uCFG6_H11 = Val; \
            break; \
        case VPP_CMU_HS_AXIS12: \
            stHsCfg6.uCFG6_H12 = Val; \
            break; \
        case VPP_CMU_HS_AXIS13: \
            stHsCfg6.uCFG6_H13 = Val; \
            break; \
        case VPP_CMU_HS_AXIS14: \
            stHsCfg6.uCFG6_H14 = Val; \
            break; \
    } \
}

/*
 * Saturation change for axes 1.. 14 for Main channel
 */

#define GlobalCmuHSSetSatChange(Axis,Val) \
{ \
    switch(Axis) { \
        case VPP_CMU_HS_AXIS1: \
            stHsCfg7.uCFG7_S1 = Val; \
            break; \
        case VPP_CMU_HS_AXIS2: \
            stHsCfg7.uCFG7_S2 = Val; \
            break; \
        case VPP_CMU_HS_AXIS3: \
            stHsCfg7.uCFG7_S3 = Val; \
            break; \
        case VPP_CMU_HS_AXIS4: \
            stHsCfg7.uCFG7_S4 = Val; \
            break; \
        case VPP_CMU_HS_AXIS5: \
            stHsCfg8.uCFG8_S5 = Val; \
            break; \
        case VPP_CMU_HS_AXIS6: \
            stHsCfg8.uCFG8_S6 = Val; \
            break; \
        case VPP_CMU_HS_AXIS7: \
            stHsCfg8.uCFG8_S7 = Val; \
            break; \
        case VPP_CMU_HS_AXIS8: \
            stHsCfg8.uCFG8_S8 = Val; \
            break; \
        case VPP_CMU_HS_AXIS9: \
            stHsCfg9.uCFG9_S9 = Val; \
            break; \
        case VPP_CMU_HS_AXIS10: \
            stHsCfg9.uCFG9_S10 = Val; \
            break; \
        case VPP_CMU_HS_AXIS11: \
            stHsCfg9.uCFG9_S11 = Val; \
            break; \
        case VPP_CMU_HS_AXIS12: \
            stHsCfg9.uCFG9_S12 = Val; \
            break; \
        case VPP_CMU_HS_AXIS13: \
            stHsCfg10.uCFG10_S13 = Val; \
            break; \
        case VPP_CMU_HS_AXIS14: \
            stHsCfg10.uCFG10_S14 = Val; \
            break; \
    } \
}

/*
 * HUE CFG Reg Address for axes 1.. 14 for Main channel
 */

#define CmuHSHueCfgRegAddr(Axis, reg, val) \
{ \
    switch(Axis) { \
        case VPP_CMU_HS_AXIS1: \
        case VPP_CMU_HS_AXIS2: \
        case VPP_CMU_HS_AXIS3: \
            reg = RA_HS_CFG0; \
            val = stHsCfg0.u32; \
            break; \
        case VPP_CMU_HS_AXIS4: \
        case VPP_CMU_HS_AXIS5: \
        case VPP_CMU_HS_AXIS6: \
            reg = RA_HS_CFG1; \
            val = stHsCfg1.u32; \
            break; \
        case VPP_CMU_HS_AXIS7: \
        case VPP_CMU_HS_AXIS8: \
        case VPP_CMU_HS_AXIS9: \
            reg = RA_HS_CFG2; \
            val = stHsCfg2.u32; \
            break; \
        case VPP_CMU_HS_AXIS10: \
        case VPP_CMU_HS_AXIS11: \
        case VPP_CMU_HS_AXIS12: \
            reg = RA_HS_CFG3; \
            val = stHsCfg3.u32; \
            break; \
        case VPP_CMU_HS_AXIS13: \
        case VPP_CMU_HS_AXIS14: \
            reg = RA_HS_CFG4; \
            val = stHsCfg4.u32; \
            break; \
    } \
}

/*
 * HUE change CFG Reg Address for axes 1.. 14 for Main channel
 */

#define CmuHSHueChangeCfgRegAddr(Axis, reg, val) \
{ \
    switch(Axis) { \
        case VPP_CMU_HS_AXIS1: \
        case VPP_CMU_HS_AXIS2: \
            reg = RA_HS_CFG4; \
            val = stHsCfg4.u32; \
            break; \
        case VPP_CMU_HS_AXIS3: \
        case VPP_CMU_HS_AXIS4: \
        case VPP_CMU_HS_AXIS5: \
        case VPP_CMU_HS_AXIS6: \
        case VPP_CMU_HS_AXIS7: \
        case VPP_CMU_HS_AXIS8: \
            reg = RA_HS_CFG5; \
            val = stHsCfg5.u32; \
            break; \
        case VPP_CMU_HS_AXIS9: \
        case VPP_CMU_HS_AXIS10: \
        case VPP_CMU_HS_AXIS11: \
        case VPP_CMU_HS_AXIS12: \
        case VPP_CMU_HS_AXIS13: \
        case VPP_CMU_HS_AXIS14: \
            reg = RA_HS_CFG6; \
            val = stHsCfg6.u32; \
            break; \
    } \
}

/*
 * Saturation change CFG Reg Address for axes 1.. 14 for Main channel
 */

#define CmuHSSatChangeCfgRegAddr(Axis, reg, val) \
{ \
    switch(Axis) { \
        case VPP_CMU_HS_AXIS1: \
        case VPP_CMU_HS_AXIS2: \
        case VPP_CMU_HS_AXIS3: \
        case VPP_CMU_HS_AXIS4: \
            reg = RA_HS_CFG7; \
            val = stHsCfg7.u32; \
            break; \
        case VPP_CMU_HS_AXIS5: \
        case VPP_CMU_HS_AXIS6: \
        case VPP_CMU_HS_AXIS7: \
        case VPP_CMU_HS_AXIS8: \
            reg = RA_HS_CFG8; \
            val = stHsCfg8.u32; \
            break; \
        case VPP_CMU_HS_AXIS9: \
        case VPP_CMU_HS_AXIS10: \
        case VPP_CMU_HS_AXIS11: \
        case VPP_CMU_HS_AXIS12: \
            reg = RA_HS_CFG9; \
            val = stHsCfg9.u32; \
            break; \
        case VPP_CMU_HS_AXIS13: \
        case VPP_CMU_HS_AXIS14: \
            reg = RA_HS_CFG10; \
            val = stHsCfg10.u32; \
            break; \
    } \
}

#endif
