/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRGAMMAPRIV__
#define __BDRGAMMAPRIV__

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */

#define VPP_MAIN_GAMMA_TABLE_START          0
#define VPP_MAIN_8BIT_GAMMA_TABLE_END       256

#define VPP_PIP_GAMMA_8BIT_TABLE_START      256
#define VPP_PIP_GAMMA_8BIT_TABLE_END        512


#define VPP_PIP_GAMMA_TABLE_START           1024
#define VPP_PIP_GAMMA_TABLE_END             2048

#define VPP_MAIN_AND_PIP_GAMMA_TABLE_START  0
#define VPP_MAIN_AND_PIP_GAMMA_TABLE_END    512

#define GAMMA_MAIN_ENABLE_MASK              0x1D
#define GAMMA_PIP_ENABLE_MASK               0x1E

#define GAMMA_NUM_UNITS    2

#define GAMMA_HCTRL_OFFSET 0

/*
 * Local Gamma BCM Buffer Write
 */
#define GAMMA_BCMBUF_WRITE(Offset,Val)\
    *(volatile unsigned int *)GmBcmBufPtr = Val;\
    GmBcmBufPtr++;\
    *(volatile unsigned int *)GmBcmBufPtr = Offset;\
    GmBcmBufPtr++;

/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */

struct tag_GAMMA_COLOR_REGISTER_BITS
{
    REG_WIDTH GammaRedLSB              : 2;
    REG_WIDTH GammaGreenLSB            : 2;
    REG_WIDTH GammaBlueLSB             : 2;
    REG_WIDTH                          : 2;
} PACKED_STRUCT;

typedef volatile struct tag_GAMMA_COLOR_REGISTER_BITS
    GAMMA_COLOR_REGISTER_BITS;


union tag_GAMMA_COLOR_REGISTER
{
    GAMMA_COLOR_REGISTER_BITS Bits;
    REG_WIDTH Val;
} PACKED_STRUCT;

typedef volatile union tag_GAMMA_COLOR_REGISTER GAMMA_COLOR_REGISTER;


/////////////////////////////////////////////////////////////////////////////////////////////

struct tag_GAMMA_CONTROL_REGISTER_BITS
{
    REG_WIDTH GammaMainEnable            : 1;
    REG_WIDTH GammaPipEnable             : 1;
    REG_WIDTH BCbGammaEnable             : 1;
    REG_WIDTH GYGammaEnable              : 1;
    REG_WIDTH RCrGammaEnable             : 1;
    REG_WIDTH Reserved                   : 3;
} PACKED_STRUCT;

typedef volatile struct tag_GAMMA_CONTROL_REGISTER_BITS
    GAMMA_CONTROL_REGISTER_BITS;


union tag_GAMMA_CONTROL_REGISTER
{
    GAMMA_CONTROL_REGISTER_BITS Bits;
    UCHAR Value;
} PACKED_STRUCT;

typedef volatile union tag_GAMMA_CONTROL_REGISTER GAMMA_CONTROL_REGISTER;

/////////////////////////////////////////////////////////////////////////////////////////////


/*-----------------------------------------------------------------------------
 * Get/Set Functions
 *-----------------------------------------------------------------------------
 */

/*
 * Enable / Disable GammaProgamming Enable
 */

#define HwiSetGammaMode(Mode,RetVal) \
{ \
    GAMMA_REG_WRITE(GAMMA_HCTRL,Mode,RetVal); \
\
}

/*
 * Enable / Disable Gamma Lookup for Main
 */

#define HwiEnableGammaLookupforMain(Val,RetVal) \
{ \
    GammaCtrlReg.Bits.GammaMainEnable = Val; \
    GAMMA_REG_WRITE(GAMMA_CTRL,GammaCtrlReg.Value,RetVal); \
\
}

/*
 * Enable / Disable Gamma Lookup for Pip
 */

#define HwiEnableGammaLookupforPip(Val,RetVal) \
{ \
    GammaCtrlReg.Bits.GammaPipEnable = Val; \
    GAMMA_REG_WRITE(GAMMA_CTRL,GammaCtrlReg.Value,RetVal); \
\
}

/*
 * Enable / Disable Gamma 8bit data Write / Read
 */


#define HwiGammaSetLoadDataSize(Val,RetVal) \
{ \
    GammaCtrlReg.Bits.LoadDataSize = Val; \
    GAMMA_REG_WRITE(GAMMA_CTRL,GammaCtrlReg.Value,RetVal); \
\
}
/*
 * Enable / Disable BCb Gamma Correction
 */

#define HwiEnableBCbGammaCorrection(Val,RetVal) \
{ \
    GammaCtrlReg.Bits.BCbGammaEnable = Val; \
    GAMMA_REG_WRITE(GAMMA_CTRL,GammaCtrlReg.Value,RetVal); \
\
}

/*
 * Enable / Disable GYGamma Correction
 */

#define HwiEnableGYGammaCorrection(Val,RetVal) \
{ \
    GammaCtrlReg.Bits.GYGammaEnable = Val; \
    GAMMA_REG_WRITE(GAMMA_CTRL,GammaCtrlReg.Value,RetVal); \
\
}

/*
 * Enable / Disable RCr Gamma Correction
 */

#define HwiEnableRCrGammaCorrection(Val,RetVal) \
{ \
    GammaCtrlReg.Bits.RCrGammaEnable = Val; \
    GAMMA_REG_WRITE(GAMMA_CTRL,GammaCtrlReg.Value,RetVal); \
\
}

/////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Programm Gamma Address
 */

#define HwiSetGammaAddress(Val,RetVal) \
{ \
    GAMMA_REG_WRITE(GAMMA_HADDR_L,(Val & 0x0FF),RetVal); \
    GAMMA_REG_WRITE(GAMMA_HADDR_H,((Val) >> 8 & 0x0F),RetVal); \
\
}

/*
 * Programm Gamma Color MSB - Red
 */

#define HwiSetGammaRedMSB(Val,RetVal) \
{ \
    GAMMA_REG_WRITE(GAMMA_HDATA_H_R,Val,RetVal); \
\
}

/*
 * Programm Gamma Color - Green
 */

#define HwiSetGammaGreenMSB(Val,RetVal) \
{ \
    GAMMA_REG_WRITE(GAMMA_HDATA_H_G,Val,RetVal); \
\
}

/*
 * Programm Gamma Color - Blue
 */

#define HwiSetGammaBlueMSB(Val,RetVal) \
{ \
    GAMMA_REG_WRITE(GAMMA_HDATA_H_B,Val,RetVal); \
\
}
/////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Programm Gamma Color LSB - RGB
 */

#define HwiSetGammaRGBLSB(Val,RetVal) \
{ \
    GAMMA_REG_WRITE(GAMMA_LDATA,Val,RetVal); \
\
}

/*
 * Programm Gamma Color High LSB - RGB
 */

#define HwiSetGammaRGBHighLSB(Val,RetVal) \
{ \
    GAMMA_REG_WRITE(GAMMA_HDATA_L,Val,RetVal); \
\
}

/////////////////////////////////////////////////////////////////////////////////////////////


#endif

