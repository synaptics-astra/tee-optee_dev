/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef VPP_FRS_SCL_H
#define VPP_FRS_SCL_H

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"

#ifdef VPP_ENABLE_DEBUG_LOG_SCL
#define VPP_SCL_DBG_LOG(...)  VPPAPI_DBG_INFO(__VA_ARGS__)
#else
#define VPP_SCL_DBG_LOG(...)
#endif //VPP_ENABLE_DEBUG_LOG_SCL


/*PIP startX Limit Validation */
#define IS_PIP_STARTX_LIMIT_CROSSED(UnitNum)  ((UnitNum == VPP_FRC_SCL_PIP) && (SRCFMT_YUV420SP == vpp_obj->plane[UnitNum].srcfmt) && (vpp_obj->chan[UnitNum].disp_win.x > 2862))
#define VPP_BASE_ADDRESS_VIPSCLTOP (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIPSCLTOP)
#define VPP_BASE_ADDRESS_VIPSCL_Y  (VPP_BASE_ADDRESS_VIPSCLTOP + RA_VIPSCLTOP_OVPSCL_Y)
#define VPP_BASE_ADDRESS_VIPSCL_UV (VPP_BASE_ADDRESS_VIPSCLTOP + RA_VIPSCLTOP_OVPSCL_UV)

/**********************************************************************
*data structure definition for FRC SCL block driver                                                  *
***********************************************************************/

/*Enum for horizontal scaler position*/
typedef enum VPP_SCL_HSCALER_POS
{
    VPP_SCL_HSCALE_ABSENT = 0,
    VPP_SCL_HSCALE_BEFORE = 2,
    VPP_SCL_HSCALE_AFTER,
    VPP_SCL_HSCALE_AUTO
}VPP_SCL_HSCALER_POS;

/*Enum for input and  output bit mode*/
typedef enum VPP_SCL_IO_BITMODE
{
    VPP_SCL_16IN_16OUT = 0,
    VPP_SCL_24IN_16OUT,
    VPP_SCL_16IN_24OUT,
    VPP_SCL_24IN_24OUT
}VPP_SCL_IO_BITMODE;

/*Enum for CSC mode in OSD formatter*/
typedef enum VPP_SCL_CSC_MODES
{
    VPP_SCL_CSC_BYPASS_MODE = 0,
    VPP_SCL_CSC_RGB_TO_YUV_709,
    VPP_SCL_CSC_RGB_TO_YUV_601,
    VPP_SCL_CSC_sRGB_TO_YUV_709,
    VPP_SCL_CSC_sRGB_TO_YUV_601,
    VPP_SCL_CSC_RGB_TO_YUV_2020,
    VPP_SCL_CSC_sRGB_TO_YUV_2020,
    VPP_SCL_CSC_MAX_MODES
} VPP_SCL_CSC_MODES;

/*Enum for input data format of OSD formatter*/
typedef enum VPP_SCL_OSD_CTRL
{
    VPP_SCL_32BIT_8888_OSD = 2,
    VPP_SCL_24BIT_888_OSD = 8,
    VPP_SCL_OSD_CTRL_MAX
}VPP_SCL_OSD_CTRL;

/*Enum for scaler to CPCB switch*/
typedef enum VPP_SCL_BE_SWITCH
{
    VPP_SCL_BE_CPCB0_PATH0 = 0,
    VPP_SCL_BE_CPCB0_PATH1,
    VPP_SCL_BE_CPCB0_PATH2,
    VPP_SCL_BE_CPCB0_PATH3,
    VPP_SCL_BE_CPCB1_PATH0,
    VPP_SCL_BE_CPCB1_PATH1,
    VPP_SCL_BE_CPCB1_PATH2,
    VPP_SCL_BE_CPCB1_PATH3,
    VPP_SCL_BE_CPCB2_PATH0,
    VPP_SCL_BE_CPCB2_PATH1,
    VPP_SCL_BE_SWITCH_MAX
}VPP_SCL_BE_SWITCH;

/*Enum for FRC SCL phase */
typedef enum VPP_FRC_SCL_PHASE_HV
{
    VPP_FRC_SCL_PHASE_HORIZONTAL = 0,
    VPP_FRC_SCL_PHASE_VERTICAL
}VPP_FRC_SCL_PHASE_HV;

/*sacler coefficient mode*/
typedef enum VPP_FRC_SCL_COEFF_MODE_T
{

    FIRST_SCL_COEFF =  0,
    COEFF_MAIN_MIN = 0,
    COEFF_H_MAIN_ADP_HLUT_CS = COEFF_MAIN_MIN,
    COEFF_H_MAIN_1D_HLUT_BLK5,
    COEFF_H_MAIN_1D_HLUT_PH8,
    COEFF_H_MAIN_1D_HLUT_PH8_SOFT,
    COEFF_V_MAIN_ADP_HLUT_BS,
    COEFF_V_MAIN_1D_VLUT_DBG1,
    COEFF_V_MAIN_1D_VLUT_2TAP,
    COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL,
    COEFF_V_MAIN_1D_VLUT_BLK5,
    COEFF_V_MAIN_1D_VLUT_6TAP,
    COEFF_MAIN_MAX = COEFF_V_MAIN_1D_VLUT_6TAP,
    COEFF_GFX_MIN,
    COEFF_H_GFX_HLUT_BLK5_GFX = COEFF_GFX_MIN,
    COEFF_H_GFX_HLUT_PV8,
    COEFF_V_GFX_VLUT_CS_4TAP,
    COEFF_V_GFX_VLUT_3TAP_CUBIC,
    COEFF_V_GFX_VLUT_1TAP,
    COEFF_GFX_MAX = COEFF_V_GFX_VLUT_1TAP,
    COEFF_MAX
}VPP_FRC_SCL_COEFF_MODE, *PVPP_FRC_SCL_COEFF_MODE;


typedef union VPP_SCL_CTRL0_T
{
    UINT8 uch8;
    struct
    {
        UINT8 uchAvg4FiltSel:2;
        UINT8 uchCscEn:1;
        UINT8 uchHnlEn:1;
        UINT8 uchUpsEn:1;
        UINT8 uchDnsEn:1;
        UINT8 uchDgnlEn:1;
        UINT8 uchDemoModeEn:1;
    };
}VPP_SCL_CTRL0;

typedef union VPP_SCL_CTRL1_T
{
    UINT8 uch8;
    struct
    {
        UINT8 uchSel_1D_adp:1;
        UINT8 uchUp_down:1;
        UINT8 uchVSclInUpsEn:1;
        UINT8 uchVSclOutUpsEn:1;
        UINT8 uchVSclInUpsCoeff:1;
    };
}VPP_SCL_CTRL1;

typedef union VPP_FRC_CTRL0_0_T
{
    UINT8 uch8;
    struct
    {
        UINT8 uchFrcMode:2;
        UINT8 uchMosiacMode:1;
        UINT8 uchBitMode_16_24:1;
        UINT8 uchSpMode:4;
    };
}VPP_FRC_CTRL0_0;
typedef union VPP_FRC_CTRL0_1_T
{
    UINT8 uch8;
    struct
    {
        UINT8 uchLoadRead:1;
        UINT8 uchLoadWrite:1;
        UINT8 uchReadCtrl:1;
        UINT8 uchWriteCtrl:1;
        UINT8 uchFreeze:1;
        UINT8 uchInvFeFld:1;
        UINT8 uchInvBeFld:1;
        UINT8 uchBeFrstResetEn:1;
    };
}VPP_FRC_CTRL0_1;

typedef union VPP_FRC_CTRL1_2_T
{
    UINT8 uch8;
    struct
    {
        UINT8 uchTreaLessReadCtrl:4;
        UINT8 uchTreaLessWriteCtrl:4;
    };
}VPP_FRC_CTRL1_2;

/*FRC input resolution*/
typedef struct VPP_FRC_RES_T
{
    UINT32  HRes;
    UINT32  VRes;
} VPP_FRC_RES, *PVPP_FRC_RES;

typedef struct VPP_SCL_NL_PARAM_T
{
    VPP_SCL_RES     IORes;
    UINT8         CenterFrac;
}VPP_SCL_NL_PARAM;

typedef struct VPP_SCL_CTRL_PARAM_T
{
    VPP_SCL_RES        IORes;
    VPP_SCL_CTRL    SclCtrl;
}VPP_SCL_CTRL_PARAM;

typedef union VPP_VDO_SCL_REG_T
{
    VPP_SCL_CTRL_PARAM    SCLCtrlparam;
    VPP_SCL_NL_PARAM    SCLNLParam;
}VPP_VDO_SCL_REG;


/**********************************************************************
*Function definition for CPCB FRC SCL block driver                                                 *
***********************************************************************/

/***************************************************************************************
 * FUNCTION: load the default register values to FRC SCL the unit
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_LoadDefaultVal(VPP_OBJ *vpp_obj, INT UnitNum);

/***************************************************************************************
 * FUNCTION: set HV resolution to FRC unit.
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               pFrcRes - pointer to the resolution data stucture
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetFrcParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pFrcRes);

/***************************************************************************************
 * FUNCTION: set delay between DE  to line reset.
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               DeLrstDelay -delay value
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetDeLrstDelay(VPP_OBJ *vpp_obj, INT UnitNum, UINT32 DeLrstDelay);

/***************************************************************************************
 * FUNCTION: set clock puncture value, control the rate of pumping data out from SDRAM
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               ClkVal -clock value(should be less than 128)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetSclClk(VPP_OBJ *vpp_obj, INT UnitNum, UINT32 ClkVal);

/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               pIORes -pointer to I&O resolution
 *               pSclCtrl - pointer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetSclCtrlParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl);

/***************************************************************************************
 * FUNCTION: set OSD formatter data format and CSC mode
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               CscMode -CSC mode (passthrough, 601 or 709)
 *               OsdFmt - OSD input data format
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetOsdFmt(VPP_OBJ *vpp_obj, INT UnitNum, INT CscMode);

/***************************************************************************************
 * FUNCTION: set BE FIFO for loopback
 * PARAMS: pCpcb0Tot - CPCB0 output total resolution
 *               pCpcb0Res - CPCB0 active resolution
 * RETURN:  1 - succeed
 *                others - failed to load CPCB BE FIFO values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetBeFifo(VPP_OBJ *vpp_obj, PVPP_FRC_RES pCpcb0Tot, PVPP_FRC_RES pCpcb0Res);

/***************************************************************************************
 * FUNCTION: control Hscaler chopping
 * PARAMS: UnitNum - unit number(0, 1)
 *         Value - 0, 1, 2, 3
 * RETURN:  1 - succeed
 *                others - failed to load register to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_ChopCtrl(VPP_OBJ *vpp_obj, INT UnitNum, INT Value);

/***************************************************************************************
 * FUNCTION: enable/disable alpha divider of GFX0 or PG scaler
 * PARAMS: UnitNum - unit number(2, 3)
 *         Enable - 1: enable, 0: disable
 * RETURN:  1 - succeed
 *                others - failed to load register to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetAlphaDivider(VPP_OBJ *vpp_obj, INT UnitNum, INT Enable);

INT Customize_Coefficients(VPP_OBJ *vpp_obj, int coeffID, int hv, INT unitNum, BOOL bCust);

/***************************************************************************************
 * FUNCTION: set phase DDA counter ( sub-pixel shifting)
 * PARAMS: scalerId - scaler index
 *               hv - horizontal or vertical
 *               counter - DDA counter
 * RETURN:  1 - succeed
 *                others - failed to set phase DDA counter
 ***************************************************************************************/
INT FRC_SCL_SetPhaseDda(VPP_OBJ *vpp_obj, VPP_FRC_SCL_NUM scalerId, int hv, int counter );

/***************************************************************************************
 * FUNCTION: Load the required Coeff from pre calculated table
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -poINTer to I&O resolution
 *               pSclCtrl - poINTer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetSclCoeff(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl);

INT VPP_SCL_SetMainSclScanParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl);

INT VPP_SCL_SetMainSclScanParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl);
INT FRC_SCL_GenerateCoeffTables(VPP_OBJ *vpp_obj);
INT VPP_SCL_ConfigUpsCoeff(VPP_OBJ *vpp_obj, INT *piCoeff, VPP_FRC_SCL_NUM UnitNum);
INT VPP_SCL_ConfigDnsCoeff(VPP_OBJ *vpp_obj, INT *piCoeff, VPP_FRC_SCL_NUM UnitNum);
INT FRC_SCL_ReadSclCoeff(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8]);
INT VPP_ClkGate_SCL(VPP_OBJ *vpp_obj, INT UnitNum, INT enable);
INT VPP_SCL_SetGfxSclScanParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl);
INT FRC_SCL_PrintSclCoeffShadow(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8]);
INT FRC_SCL_Set_CSwap(VPP_OBJ *vpp_obj, INT UnitNum, INT iEnable);
UINT32 INTERNAL_FRC_SCL_GetRegStartAddr(VPP_OBJ *vpp_obj, VPP_FRC_SCL_NUM UnitNum);
#endif

