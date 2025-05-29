/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifdef ENABLE_EECMU
#ifndef _VPP_CMU_EE_PRV_H_
#define _VPP_CMU_EE_PRV_H_

#include "com_type.h"
#include "vpp_module.h"
/**********************************************************************
*macro definitions for edge enhancer                                                                     *
***********************************************************************/

/*register number in an EE unit*/
#define CMU_EE_REG_NUM (CMU_EE_CTIGAIN - CMU_EE_CTRL + 1)

/*CMU EE base address*/
#define CMU_EE_REG_BASE CMU_EE_CTRL

// Macros for DETAIL_LUT
#define FE_DET_MIN_LUT 0x00
#define FE_DET_MAX_LUT 0x0F

/*get hardware address by register name*/
#define GET_CMU_EE_ADDR(RegName, UnitNum)  (INTERNAL_EE_GetRegStartAddr(vpp_obj, UnitNum) + ((OFF_##RegName) << 2))

/**********************************************************************
*FE detail EE private configuration.                                                                        *
***********************************************************************/
///////////////////SHOULD HAVE DIFFERENT TABLES///////////////////////
/*detail EE control parameters*/
VPP_FE_DET_CTRL_PARAMS gEeDetCtrlParams[] =
{
    { // VPP_FE_DET_TH_LOW
        0x08,
        0x08,
        0x00,
        0x00,
        0x02,
        0x0C,
        0x01
    },
    { // VPP_FE_DET_TH_MEDIUM
        0x08,
        0x08,
        0x40,
        0x40,
        0x02,
        0x0C,
        0x01
    },
    { // VPP_FE_DET_TH_HIGH
        0x08,
        0x08,
        0x80,
        0x80,
        0x02,
        0x0C,
        0x01
    },
    { // EE_INTERNET_LOW_BR_SD
        0x0B,
        0x0B,
        0x14,
        0x14,
        0x02,
        0x0C,
        0x03
    },
    { // EE_INTERNET_HIGH_BR_SD
        0x08,
        0x08,
        0x34,
        0x34,
        0x02,
        0x0C,
        0x03
    },
    { // EE_INTERNET_HD
        0x08,
        0x08,
        0x50,
        0x50,
        0x02,
        0x0C,
        0x01
    }
};

/*H&V LPF sharpness*/
VPP_FE_DET_LPF_CTRL_PARAMS gEeLpfSharpness[] =
{
    {0x00, 0x00}, // VPP_FE_DET_TH_LOW
    {0x00, 0x00}, // VPP_FE_DET_TH_MEDIUM
    {0x00, 0x00} // VPP_FE_DET_TH_HIGH
};

/* LTI and CTI gains */
INT gEeLtiCtiGain[3][2] =
{
    {0x00, 0x00}, // VPP_FE_DET_TH_LOW
    {0x10, 0x10}, // VPP_FE_DET_TH_MEDIUM
    {0x20, 0x20} // VPP_FE_DET_TH_HIGH
};

/**********************************************************************
*bit field definition for CMU EE                                                                              *
***********************************************************************/
/*CMU EE control register bit fields */
typedef union VPP_CMU_EE_PRI_CTRL_T
{
    struct {
        UINT32 UpsampleEn               :1;
        UINT32 LumaGainEn               :1;
        UINT32 DetailEn                 :1;
        UINT32 ChromaGainEn             :1;
        UINT32 FtLtiEn                  :1;
        UINT32 FtDetEn                  :1;
        UINT32 NewLtiAlgoSel            :1;
        UINT32 NewCtiAlgoSel            :1;
        UINT32 LtiGain                  :8;
        UINT32 CtiGain                  :8;
        UINT32 Reserved                 :8;
    } field;

    UINT32 data;
} VPP_CMU_EE_PRI_CTRL;

/**********************************************************************
*default register value                                                                                          *
***********************************************************************/
/*CMU EE register structure*/
typedef struct VPP_CMU_EE_T{
    UINT32 VPP_CMU_EE_CTRL;
    UINT32 VPP_CMU_EE_LTIGAIN;
    UINT32 VPP_CMU_EE_CTIGAIN;
}VPP_CMU_EE, *PVPP_CMU_EE;

typedef struct tagVPP_CMU_EE_SkinToneRegion
{
    UINT16 FtdInlow;
    UINT16 FtdInhigh;
    UINT16 FtdOutlow;
    UINT16 FtdOuthigh;
    UINT8  FtdYlow;
    UINT8  FtdYhigh;
    UINT16 FtdOutchromaLow;
    UINT16 FtdInchromaLow;
    UINT16 FtdInchromaHigh;
    UINT16 FtdOutchromaHigh;
} VPP_CMU_EE_SkinToneRegion, *PVPP_CMU_EE_SkinToneRegion;
/* color upsampler Controls */
typedef struct tagVPP_CMU_EE_UpsCtrl
{
    UINT8 ups_yshift;
    UINT8 ups_cshift;
    UINT8 ups_cswap;
    UINT16 ups_yblank;
    UINT16 ups_cblank;
    UINT8 ups_y_th;
    UINT8 ups_c_th;
} VPP_CMU_EE_UpsCtrl, *PVPP_CMU_EE_UpsCtrl;

// Detail EE Control Register Bit Fields
typedef struct tagDET_CTRL_REG {
    REG_WIDTH EeEnable            : 1;
    REG_WIDTH DetailEnable        : 1;
    REG_WIDTH DetailAdd           : 1;
    REG_WIDTH Reserved            : 1;
    REG_WIDTH HfMode              : 2;
    REG_WIDTH VfMode              : 2;
}DET_CTRL_REG;

typedef union tagDET_CTRL_REG_BITS {
    DET_CTRL_REG Bits;
    REG_WIDTH Value;
}DET_CTRL_REG_BITS;


/*CMU-0 EE register default values*/
const VPP_CMU_EE gCmuEeDefValues = {
    .VPP_CMU_EE_CTRL          = 0x00000001,//default enable upsampler in EE0
    .VPP_CMU_EE_LTIGAIN       = 0x00000010,
    .VPP_CMU_EE_CTIGAIN       = 0x00000010,
};

#endif/*_VPP_CMU_EE_PRV_H_*/
#endif /* ENABLE_EECMU */
