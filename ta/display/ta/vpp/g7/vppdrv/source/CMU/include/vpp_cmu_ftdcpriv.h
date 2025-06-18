/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRFTDCPRIV__
#define __BDRFTDCPRIV__

#include "vpp_api.h"
#include "vpp_module.h"

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */

#define VPP_CMU_MAX_FLESH_TONE_REGIONS    6
#define VPP_CMU_MAX_FLESH_TONE_COEFF      4
#define VPP_CMU_MAX_FLESH_TONE_OFFSETS    2

#define VPP_CMU_FTDC_COEFF_SIGN_BIT       0x80
#define VPP_CMU_FTDC_OFFSET_SIGN_BIT      0x10

#define VPP_CMU_FTDC_COEFF_MUL_FACTOR     64
#define VPP_CMU_FTDC_OFFSET_MUL_FACTOR    16

#define VPP_CMU_FTDC_COEFF_MAX            127
#define VPP_CMU_FTDC_COEFF_MIN            -127

#define VPP_CMU_FTDC_OFFSET_MAX           15
#define VPP_CMU_FTDC_OFFSET_MIN           -15

#define FTDC_MAIN_ENABLE_MASK             0x55
#define FTDC_PIP_ENABLE_MASK              0x6A

#define HUESAT_MAIN_ENABLE_MASK           0x4
#define HUESAT_PIP_ENABLE_MASK            0x8

#define PIXELVAL_MAIN_ENABLE_MASK         0x10
#define PIXELVAL_PIP_ENABLE_MASK          0x20

#define FTDC_TAPER_ENABLE_MASK            0x40

#define FTDC_NUM_UNITS                    2

#define FTDC_REG_ADDR(Offset,Val,RetVal) \
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,(vpp_obj->base_addr + \
                 (Offset << 2)), Val);\
    if (RetVal != MV_VPP_OK) return RetVal;\

/*-----------------------------------------------------------------------------
 * Static
 *-----------------------------------------------------------------------------
 */

static UCHAR FtdcMode=0;

static VPP_CMU_INPUT_SIGNAL_TYPE gCmuFtdcInputType[2];

/*-----------------------------------------------------------------------------
 * Get/Set Functions
 *-----------------------------------------------------------------------------
 */

/*
 * FTDC Enable for Main
 */
#define HwiCmuEnableFtdcforMain(Val,RetVal) \
{\
    FTDC_REG_ADDR(CMU_FTDC_M_EN,Val,RetVal);\
}

/*
 * FTDC Enable for Pip
 */
#define HwiCmuEnableFtdcforPip(Val,RetVal) \
{\
    FTDC_REG_ADDR(CMU_FTDC_P_EN,Val,RetVal);\
}

/*
 * Hue limits - Lower limit inner range
 */
#define HwiCmuSetFtdcInLow(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_INLOW_L,Val & 0xFF,RetVal);\
    FTDC_REG_ADDR(CMU_FTDC_INLOW_H,((Val >> 8) & 0x03),RetVal);\
}

/*
 * Hue limits - Higher limit inner range
 */
#define HwiCmuSetFtdcInHigh(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_INHIGH_L,Val & 0xFF,RetVal);\
    FTDC_REG_ADDR(CMU_FTDC_INHIGH_H,((Val >> 8) & 0x03),RetVal);\
}

/*
 * Hue limits - Lower limit outer range
 */
#define HwiCmuSetFtdcOutLow(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_OUTLOW_L,Val & 0xFF,RetVal);\
    FTDC_REG_ADDR(CMU_FTDC_OUTLOW_H,((Val >> 8) & 0x03),RetVal);\
}

/*
 * Hue limits - Higher limit outer range
 */
#define HwiCmuSetFtdcOutHigh(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_OUTHIGH_L,Val & 0xFF,RetVal);\
    FTDC_REG_ADDR(CMU_FTDC_OUTHIGH_H,((Val >> 8) & 0x03),RetVal);\
}

/*
 * Luma limits - Lower limit
 */
#define HwiCmuSetFtdcYthLow(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_YLOW,Val,RetVal); \
}

/*
 * Luma limits - Upper limit
 */
#define HwiCmuSetFtdcYthHigh(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_YHIGH,Val,RetVal);\
}

/*
 * Chroma limits :- Lower
 */
#define HwiCmuSetFtdcChroma1(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_CH1,Val,RetVal);\
}

/*
 * Chroma limits :- Lower
 */
#define HwiCmuSetFtdcChroma2(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_CH2_L,Val & 0xFF,RetVal);\
    FTDC_REG_ADDR(CMU_FTDC_CH2_H,((Val >> 8) & 0x0F),RetVal);\
}

/*
 * Chroma limits :- Lower
 */
#define HwiCmuSetFtdcChroma3(Val,RetVal) \
{ \
    FTDC_REG_ADDR(CMU_FTDC_CH3_L,Val & 0xFF,RetVal);\
    FTDC_REG_ADDR(CMU_FTDC_CH3_H,((Val >> 8) & 0x0F),RetVal);\
}

#define HwiCmuSetFtdcFleshRegionCoefficents(i , j, Val,RetVal) \
{ \
    FTDC_REG_ADDR((CMU_FTDC_1_C00 + j + i*6),Val,RetVal);\
}

/*
 * Offsets for flesh regions
 */
#define HwiCmuSetFtdcFleshRegionOffsets(i , j, Val,RetVal) \
{ \
    FTDC_REG_ADDR((CMU_FTDC_1_OFF00 + j + i*6),Val,RetVal);\
}

#endif
