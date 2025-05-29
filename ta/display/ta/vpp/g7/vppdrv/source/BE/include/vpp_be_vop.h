/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_BE_VOP_H_
#define _VPP_BE_VOP_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"

/*Data alignment in 30-bit format after rounding.*/
typedef enum VPP_VOP_DATA_ALIGNMENT_T
{
    VPP_VOP_8_10_LSB_ALIGN = 0,
    VPP_VOP_8_10_MSB_ALIGN,
    VPP_VOP_24_30_LSB_ALIGN,
    VPP_VOP_24_30_MSB_ALIGN,
    VPP_VOP_ALIGN_MAX

}VPP_VOP_DATA_ALIGNMENT,*PVPP_VOP_DATA_ALIGNMENT;

/*Data rounding selection control*/
typedef enum tagVPP_VOP_DATA_ROUNDING
{
    VPP_VOP_12_TO_8_BITS_ROUND     = 1,
    VPP_VOP_12_TO_10_BITS_ROUND,
    VPP_VOP_DATA_BYPASS         = 4,
    VPP_VOP_DATA_ZERO           = 7,
    VPP_VOP_DATA_ROUND_MAX

}VPP_VOP_DATA_ROUNDING,*PVPP_VOP_DATA_ROUNDING;

/*DNS mode selection control*/
typedef enum tagVPP_VOP_DNS_MODE
{
    VPP_VOP_DNS_MODE_BYPASS     = 0,
    VPP_VOP_DNS_MODE_ENABLE,
    VPP_VOP_DNS_MODE_MAX

}VPP_VOP_DNS_MODE,*PVPP_VOP_DNS_MODE;

/*Enums for CSC mode.*/
typedef enum VPP_VOP_CSC_MODES
{
    VPP_VOP_CSC_BYPASS_MODE = 0,
    VPP_VOP_CSC_601_TO_709,
    VPP_VOP_CSC_709_TO_601,
    VPP_VOP_CSC_RGB_TO_YUV_709,
    VPP_VOP_CSC_YUV_709_TO_RGB,
    VPP_VOP_CSC_RGB_TO_YUV_601,
    VPP_VOP_CSC_YUV_601_TO_RGB,
    VPP_VOP_CSC_sRGB_TO_YUV_709,
    VPP_VOP_CSC_YUV_709_TO_sRGB,
    VPP_VOP_CSC_sRGB_TO_YUV_601,
    VPP_VOP_CSC_YUV_601_TO_sRGB,
    VPP_VOP_CSC_RGB_TO_YUV_2020,
    VPP_VOP_CSC_YUV_2020_TO_RGB,
    VPP_VOP_CSC_sRGB_TO_YUV_2020,
    VPP_VOP_CSC_YUV_2020_TO_sRGB,
    VPP_VOP_CSC_YUV_601_TO_2020,
    VPP_VOP_CSC_YUV_2020_TO_601,
    VPP_VOP_CSC_YUV_709_TO_2020,
    VPP_VOP_CSC_YUV_2020_TO_709,
    VPP_VOP_CSC_UV_SWAP,
    VPP_VOP_CSC_CUSTOM,
    VPP_VOP_CSC_MODES_NO_CHANGE,
    VPP_VOP_CSC_MAX_MODES = VPP_VOP_CSC_MODES_NO_CHANGE

} VPP_VOP_CSC_MODES;

/*VOP clock polarity for TTL24 and TTL30*/
typedef enum VPP_VOP_DV_CLK_POL{
    VPP_VOP_DV_CLK_POL_NORMAL = 0,
    VPP_VOP_DV_CLK_POL_INVERT

} VPP_VOP_DV_CLK_POL;

/*output data range contral for BT656/SMPTE encoders*/
typedef enum VPP_VOP_DV_SATURATE_CTRL{
    VPP_VOP_DV_SATURATE_NO_CHANGE = 0,
    VPP_VOP_DV_SATURATE_1_255,
    VPP_VOP_DV_SATURATE_16_235,

} VPP_VOP_DV_SATURATE_CTRL;

/*HS/HDE select on DV0/1_HS pad for TTL24 and TTL30*/
typedef enum VPP_VOP_DV_HS_PAD_SEL{
    VPP_VOP_DV_HS_PAD_HDE = 0,
    VPP_VOP_DV_HS_PAD_HS,

} VPP_VOP_DV_HS_PAD_SEL;

/*VS/VDE select on DV0/1_VS pad for TTL24 and TTL30*/
typedef enum VPP_VOP_DV_VS_PAD_SEL{
    VPP_VOP_DV_VS_PAD_VDE = 0,
    VPP_VOP_DV_VS_PAD_VS,

} VPP_VOP_DV_VS_PAD_SEL;

/*DV output selection for TTL24 and TTL30*/
typedef enum VPP_VOP_DV_OUTPUT_SEL{
    VPP_VOP_DV_OUTPUT_BT656 = 0,
    VPP_VOP_DV_OUTPUT_SMPTE_DDR,
    VPP_VOP_DV_OUTPUT_INVALID,
    VPP_VOP_DV_OUTPUT_SMPTE_SDR//the output of TTL24 is ingored, if TTL30 is in this mode

} VPP_VOP_DV_OUTPUT_SEL;

/*DV Luma and Chroma selection in SMPTE DDR mode for TTL24 and TTL30*/
typedef enum VPP_VOP_DV_YC_POS_SEL{
    VPP_VOP_DV_YC_POS_C_HIGH = 0,
    VPP_VOP_DV_YC_POS_Y_HIGH,

} VPP_VOP_DV_YC_POS_SEL;

/*DV Luma and Chroma sequence selection in SMPTE SDR mode for TTL30*/
typedef enum VPP_VOP_DV_YC_SEQ_SEL{
    VPP_VOP_DV_YC_SEQ_Y_15_8 = 0,
    VPP_VOP_DV_YC_SEQ_Y_7_0,

} VPP_VOP_DV_YC_SEQ_SEL;

/*Enum for VOP clock select*/
typedef enum VPP_VOP_CLK_SEL
{
    VPP_VOP_CLK_DISABLE = -1,
    VPP_VOP_CLK_D1,
    VPP_VOP_CLK_D2,
    VPP_VOP_CLK_D4,
    VPP_VOP_CLK_D6,
    VPP_VOP_CLK_D8,
    VPP_VOP_CLK_D12,
    VPP_VOP_CLK_D3,
    VPP_VOP_CLK_MAX

}VPP_VOP_CLK_SEL;

/*Type of output*/
typedef enum VPP_VOP_OUTPUT
{
    VPP_VOP_AHD = 0,
    VPP_VOP_ASD,
    VPP_VOP_HDMI,
    VPP_VOP_TTL24,
    VPP_VOP_TTL30,
    VPP_VOP_OUTPUT_MAX

}VPP_VOP_OUTPUT,*PVPP_VOP_OUTPUT;

/*HDMI control data structure*/
typedef struct VPP_VOP_HDMI_CTRL_T
{
    UINT32 CBlank;
    UINT8 InputSel;
    UINT8 OutputAlign;
}VPP_VOP_HDMI_CTRL,*PVPP_VOP_HDMI_CTRL;

/*AHD and ASD control data structure*/
typedef struct VPP_VOP_AHD_ASD_CTRL_T
{
    UINT8 InputSel;
    UINT8 OutputAlign;
}VPP_VOP_AHD_ASD_CTRL,*PVPP_VOP_AHD_ASD_CTRL;

/*TTL30 control data structure*/
typedef struct VPP_VOP_TTL30_CTRL_T
{
    UINT32 CBlank;
    UINT8 InputSel;
    UINT8 CscMode;
    UINT8 ClkPol;
    UINT8 Saturate;
    UINT8 HsHdeSel;
    UINT8 VsVdeSel;
    UINT8 OutputSel;
    UINT8 YCPosSel;
}VPP_VOP_TTL30_CTRL,*PVPP_VOP_TTL30_CTRL;

/*Dither Algorithm selection.*/
typedef enum VPP_VOP_TTL24_DITHER_ALG
{
    VPP_VOP_DIT_BYPASS_MODE = 0,
    VPP_VOP_PRBS_HPF_ALGORITHM,
    VPP_VOP_ORDERED_DIT_ALGORITHM,
    VPP_VOP_ROUND_AND_TRUNC,
    VPP_VOP_DIT_END

}VPP_VOP_TTL24_DITHER_ALG,*PVPP_VOP_TTL24_DITHER_ALG;

/*TTL24 output mode selection when DNS enable*/
typedef enum VPP_VOP_TTL24_18_24_DNS_ENB
{
    VPP_VOP_YC_16BIT_SEL_TTL24_OP = 0,  // YC 16 bit is selected
    VPP_VOP_YC_20BIT_SEL_TTL24_OP,        // YC 20-bit is selected
    VPP_VOP_YC_24BIT_SEL_TTL24_OP,           // YC 24-bit is selected
    VPP_VOP_YC_TTL24_MAX

}VPP_VOP_TTL24_18_24_DNS_ENB, *PVPP_VOP_TTL24_18_24_DNS_ENB;

/*TTL24 output mode selection when DNS disable*/
typedef enum VPP_VOP_TTL24_18_24_DNS_DISB
{
    VPP_VOP_RGB_YCBCR_24BIT_SEL   = 0,
    VPP_VOP_RGB_YCBCR_18BIT_SEL   = 1,
    VPP_VOP_RGB_BIT_DISB_MAX

}VPP_VOP_TTL24_18_24_DNS_DISB, *PVPP_VOP_TTL24_18_24_DNS_DISB;

/*Dither selection*/
typedef struct VPP_VOP_TTL24_DITHER_SEL_T
{
    UINT32 Dither;        //Dither algorithm selection
    UINT32 DitMirror;
    UINT32 DitInpRes;     // Input data width
    UINT32 DitOutRes;     // Output data width

}VPP_VOP_TTL24_DITHER_SEL,*PVPP_VOP_TTL24_DITHER_SEL;

/*TTL 24 control data structure*/
typedef struct VPP_VOP_TTL24_CTRL_T
{
    UINT32 CBlank;
    UINT8 InputSel;
    UINT8 CscMode;
    UINT8 ClkPol;
    UINT8 Saturate;
    UINT8 HsHdeSel;
    UINT8 VsVdeSel;
    UINT8 OutputSel;
    UINT8 YCPosSel;
}VPP_VOP_TTL24_CTRL,*PVPP_VOP_TTL24_CTRL;

/***************************************************************************************
 * FUNCTION: load all the default values of VOP block to BCM buffer
 * PARAMS: void
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP default values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_LoadDefaultVal(VPP_OBJ *vpp_obj);

/***************************************************************************************
 * FUNCTION: set HDMI DNS mode
 * PARAMS: DnsMode - HDMI DNS mode
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP HDMI DNS mode values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiDnsMode(VPP_OBJ *vpp_obj, INT DnsMode);

/***************************************************************************************
 * FUNCTION: set HDMI parameters
 * PARAMS: pHdmiCtrl - pointer to HDMI control parameters
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP HDMI control values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiParams(VPP_OBJ *vpp_obj, PVPP_VOP_HDMI_CTRL pHdmiCtrl);

/***************************************************************************************
 * FUNCTION: set AHD parameters
 * PARAMS: pAhdCtrl - pointer to AHD control parameters
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP AHD control values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetAhdParams(VPP_OBJ *vpp_obj, PVPP_VOP_AHD_ASD_CTRL pAhdCtrl);

/***************************************************************************************
 * FUNCTION: set ASD parameters
 * PARAMS: pAsdCtrl - pointer to ASD control parameters
 * RETURN:  1 - succeed
 *                others - failed to load BE VOP ASD control values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetAsdParams(VPP_OBJ *vpp_obj, PVPP_VOP_AHD_ASD_CTRL pAsdCtrl);

/***************************************************************************************
 * FUNCTION: sets output round mode
 * PARAMS: OutputSel - indicates which VOP unit to be loaded(HDMI, AHD, ASD). TTL24 and TTL30 have no round
 *               RndMode - round mode
 * RETURN:  1 - succeed
 *                others - failed to load round register value to BCM buffer or invalid parameters
 ***************************************************************************************/
INT BE_VOP_SetRoundMode(VPP_OBJ *vpp_obj, INT OutputSel, INT RndMode);

/***************************************************************************************
 * FUNCTION: load CSC coeffs and offsets to VOP
 * PARAMS: OutputSel - indicates which VOP unit to be loaded(HDMI, AHD, ASD, TTL24, TTL30)
 *              ColorFmt - Color Format
 * RETURN:  1 - succeed
 *                others - failed to load CSC coeffs and offsets to BCM buffer
 ***************************************************************************************/
INT BE_VOP_LoadCscCoeff(VPP_OBJ *vpp_obj, INT OutputSel, INT ColorFmt);

/**************************************************************************************
 * FUNCTION: set HDMI/HD/SD encoder clock dividor
 * PARAMS: OutputSel - VOP output ID(VPP_VOP_HDMI, VPP_VOP_AHD, VPP_VOP_ASD)
 *         ClkSel - clock selector
 * RETURN:  1 - succeed
 *          others - failed
 ***************************************************************************************/
INT BE_VOP_ClkSelect(VPP_OBJ *vpp_obj, INT OutputSel, INT ClkSel);

/***************************************************************************************
 * FUNCTION: Enable or disable HDMI formatter
 * PARAMS: Enable - 1: Enable  0: Disable
 * RETURN:  1 - succeed
 *                others - failed to load register values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiFrmtEnable(VPP_OBJ *vpp_obj, UINT8 Enable);

/***************************************************************************************
 * FUNCTION: Set HDMI formatter to one time L/R configuration mode, and indicate veiw ID for next frame during VblackL+VactiveL time.
 * PARAMS: ViewID - 0: Left view  1: Right view
 * RETURN:  1 - succeed
 *                others - failed to load register values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiFrmtViewIDOneTime(VPP_OBJ *vpp_obj, UINT8 ViewID);

/***************************************************************************************
 * FUNCTION: Set HDMI formatter to continuous L/R configuration mode, and indicate veiw ID for next frame during VblackL+VactiveL time.
 * PARAMS: ViewID - 0: Left view  1: Right view
 * RETURN:  1 - succeed
 *                others - failed to load register values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiFrmtViewIDContinuous(VPP_OBJ *vpp_obj, UINT8 ViewID);

/***************************************************************************************
 * FUNCTION: Set HDMI formatter packing mode.
 * PARAMS: Mode - 0: Progressive Frame Packing
 *                2: Field Alternative Packing
 *                3: Field Frame Packing
 *         Enable - 0: disable Active Space insert
 *                  1: enable Active Space insert
 * RETURN:  1 - succeed
 *                others - failed to load register values to BCM buffer
 ***************************************************************************************/
INT BE_VOP_SetHdmiFrmtPackingMode(VPP_OBJ *vpp_obj, UINT8 Mode, UINT8 Enable);

/***************************************************************************************
 * FUNCTION: Reconfigure output CSC based on input stream
 * PARAMS: none
 *
 * RETURN: none
 *
 ***************************************************************************************/
VOID BE_VOP_ReConfigVOPCsc(VPP_OBJ *vpp_obj);

INT BE_VOP_SetOutputFmt_420(VPP_OBJ *vpp_obj, BOOL Enable, UINT32 bitdepth);

INT BE_VOP_SetPixTot(VPP_OBJ *vpp_obj, INT OutputSel, INT Width, INT Height);

int BE_VOP_SetDSIParams(VPP_OBJ *vpp_obj, int cpcbout);

INT BE_VOP_SetDsiDitherMode(VPP_OBJ *vpp_obj, UINT32 uiMode, UINT32 uiBitDepth);

void BE_VOP_ConfigDither(T32DITHER_CFG0 *stDitherCfg0, UINT32 uiMode, UINT32 uiBitDepth);
#endif

