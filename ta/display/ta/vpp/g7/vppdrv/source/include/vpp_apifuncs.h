/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_APIFUNCS_H_
#define _VPP_APIFUNCS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpp_cfg.h"


enum{
    VPP_HDMI_GET_HPD_STATUS = 0,
    VPP_HDMI_GET_RAW_EDID,
    VPP_HDMI_SET_VIDEO_FMT,
    VPP_HDMI_SET_AUDIO_FMT,
    VPP_HDMI_ENABLE_AUDIO_FMT,
    VPP_HDMI_SET_VIDEO_MUTE,
    VPP_HDMI_SET_AUDIO_MUTE,
    VPP_HDMI_SET_AV_MUTE,
    VPP_HDMI_SET_AUDIO_VUC_BITS,
    VPP_HDMI_SET_HDCP,
    VPP_HDMI_GET_HDCP_STATE,
    VPP_HDMI_SET_MODE,
    VPP_HDMI_GET_STS,
    VPP_HDMI_REG_CALLBACK,
    VPP_HDMI_UNREG_CALLBACK,
    VPP_HDCP_REG_CALLBACK,
    VPP_HDCP_UNREG_CALLBACK,
    VPP_HDMI_SEND_HOST_PKT,
    VPP_HDMI_STOP_HOST_PKT,
    VPP_HDMI_SET_ENABLE,
    VPP_HDMI_GET_HDCP_VERSION,
    VPP_HDMI_SET_HDCP_VERSION,
    VPP_HDMI_FORCE_HDMI_HOTPLUG,
    VPP_HDMI_GET_CUR_HDCP_VERSION,
    VPP_HDMI_GET_SINK_HDCP_CAP,
    VPP_HDMI_GET_POWER_STATE,
    VPP_HDMI_CMD_MAX
};

#define VPP_OCSC_BRIGHTNESS_MIN         0.0
#define VPP_OCSC_CONTRAST_MIN           0.0
#define VPP_OCSC_HUE_MIN                0.0
#define VPP_OCSC_SATURATION_MIN         0.0

// Brightness/Offset's LowThr = 256; HighThr= 3760; MaxVal = 4095;
// So calculated step size based on this, 0-500(3504.0)
#define VPP_OCSC_BRIGHTNESS_MAX         3504.0
#define VPP_OCSC_CONTRAST_MAX           2.0
#define VPP_OCSC_HUE_MAX                360.0
#define VPP_OCSC_SATURATION_MAX         2.0

/*Brightness factor is calculated by VPP_OCSC_BRIGHTNESS_MAX/500 */
#define VPP_OCSC_BRIGHTNESS_MAP_FACTOR  7.008
/*Contrast/Saturation factor is calculated by VPP_OCSC_CONTRAST_MAX/500 */
#define VPP_OCSC_CONTRAST_MAP_FACTOR    0.004
#define VPP_OCSC_SATURATION_MAP_FACTOR  VPP_OCSC_CONTRAST_MAP_FACTOR
/*Brightness factor is calculated by VPP_OCSC_HUE_MAX/500*2 */
#define VPP_OCSC_HUE_MAP_FACTOR         0.36

#define VPP_OCSC_BRIGHTNESS_INT2FLOAT(value)  (value * VPP_OCSC_BRIGHTNESS_MAP_FACTOR)
#define VPP_OCSC_CONTRAST_INT2FLOAT(value)    (value * VPP_OCSC_CONTRAST_MAP_FACTOR)
#define VPP_OCSC_SATURATION_INT2FLOAT(value)  VPP_OCSC_CONTRAST_INT2FLOAT(value)
#define VPP_OCSC_HUE_INT2FLOAT(value)         (((value * 2) >= 500) ? \
                                               (float)(value * 2) * VPP_OCSC_HUE_MAP_FACTOR - 180 : \
                                               (float)(value * 2) * VPP_OCSC_HUE_MAP_FACTOR + 180)

#define VPP_OCSC_BRIGHTNESS_FLOAT2INT(value)  (value / VPP_OCSC_BRIGHTNESS_MAP_FACTOR)
#define VPP_OCSC_CONTRAST_FLOAT2INT(value)    (value / VPP_OCSC_CONTRAST_MAP_FACTOR)
#define VPP_OCSC_SATURATION_FLOAT2INT(value)  VPP_OCSC_CONTRAST_FLOAT2INT(value)
#define VPP_OCSC_HUE_FLOAT2INT(value)         ((value >= 180) ? \
                                               ((value - 180)/VPP_OCSC_HUE_MAP_FACTOR)/2 : \
                                               ((value + 180)/VPP_OCSC_HUE_MAP_FACTOR)/2)

#define IS_VPP_VALID_OCSC_BRIGHTNESS(CtrlVal) \
                    ((CtrlVal < VPP_OCSC_BRIGHTNESS_MIN) || (CtrlVal > VPP_OCSC_BRIGHTNESS_MAX))
#define IS_VPP_VALID_OCSC_CONTRAST(CtrlVal) \
                    ((CtrlVal < VPP_OCSC_CONTRAST_MIN) || (CtrlVal > VPP_OCSC_CONTRAST_MAX))
#define IS_VPP_VALID_OCSC_HUE(CtrlVal) \
                    ((CtrlVal < VPP_OCSC_HUE_MIN) || (CtrlVal > VPP_OCSC_HUE_MAX))
#define IS_VPP_VALID_OCSC_SATURATION(CtrlVal) \
                    ((CtrlVal < VPP_OCSC_SATURATION_MIN) || (CtrlVal > VPP_OCSC_SATURATION_MAX))

#define IS_VPP_VALID_OCSC_PICTURE_CTRL_VAL(CtrlSel, CtrlVal) \
            (((CtrlSel == PICTURE_CTRL_BRIGHTNESS) && IS_VPP_VALID_OCSC_BRIGHTNESS(CtrlVal)) || \
            ((CtrlSel == PICTURE_CTRL_CONTRAST) && IS_VPP_VALID_OCSC_CONTRAST(CtrlVal))|| \
            ((CtrlSel == PICTURE_CTRL_HUE) && IS_VPP_VALID_OCSC_HUE(CtrlVal)) || \
            ((CtrlSel == PICTURE_CTRL_SATURATION) && IS_VPP_VALID_OCSC_SATURATION(CtrlVal)))

enum{
    VPP_CMU_SET_FIRST =0,
    VPP_CMU_SET_BR = 0,
    VPP_CMU_SET_CNTRST = 1,
    VPP_CMU_SET_HUE = 2,
    VPP_CMU_SET_SAT = 3,
    VPP_CMU_SET_CLRT = 4,
    VPP_CMU_SET_ACE = 5,
    VPP_CMU_SET_BREN = 6,
    VPP_CMU_SET_ICR = 7,
    VPP_CMU_SET_FTDC = 8,
    VPP_CMU_SET_CLRSAT = 9,
    VPP_CMU_SET_CLRHUE =10,
    VPP_CMU_SET_FT_TAPER = 11,
    VPP_CMU_SET_MAX
};



/*******************************************************************
 * FUNCTION: set CPCB background color
 * INPUT: cpcbID - id of the CPCB
 *           bgcolor - CPCB background color value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 * Note: this function has to be called before enabling a plane
 *       which belongs to that DV.
 *******************************************************************/
int VPP_SetCPCBBgColor(VPP_OBJ *vpp_obj, int *in_params);

/*******************************************************************
 * FUNCTION: set CPCB(for Berlin) or DV(for Galois) output resolution
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        resID - id of output resolution
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 * Note: this function has to be called before enabling a plane
 *       which belongs to that CPCB or DV.
 *******************************************************************/
int VPP_SetCPCBOutputResolution(VPP_OBJ *vpp_obj, int *in_params);

/******************************************************************************
 * FUNCTION: open a display window of a video/graphics plane for display.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of the plane
 *        bgcolor - background color of window
 *        alpha - alpha of window
 *        x - start x-coordination of window in pixel in end display resolution
 *        y - start y-coordination of window in pixel in end display resolution
 *        width - width of window in pixel in end display resolution
 *        height - height of window in pixel in end display resolution
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_OpenDispWindow(VPP_OBJ *vpp_obj, int *in_params);

/******************************************************************************
 * FUNCTION: change a display window of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of the plane
 *        bgcolor - background color of window
 *        alpha - alpha of window
 *        x - start x-coordination of window in pixel in end display resolution
 *        y - start y-coordination of window in pixel in end display resolution
 *        width - width of window in pixel in end display resolution
 *        height - height of window in pixel in end display resolution
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_ChangeDispWindow(VPP_OBJ *vpp_obj, int *in_params);

/******************************************************************************
 * FUNCTION: close display window of a video/graphics plane in displaying
 * INPUT: planeID - id of the plane
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_CloseDispWindow(VPP_OBJ *vpp_obj, int *in_params);

/*******************************************************************************
 * FUNCTION: set crop window for a plane
 * INPUT: planeID - id of the plane
 *        x - start x of crop window in pixel
 *        y - start y of crop window in pixel
 *        width - width of crop window in pixel
 *        height - height of crop window in pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 *****************************************************************************/
int VPP_SetCropWindow(VPP_OBJ *vpp_obj, int *in_params);

/*******************************************************************************
 * FUNCTION: set reference window for a plane
 * INPUT: planeID - id of the plane
 *        x - start x-coordination of window in pixel in end display resolution
 *        y - start y-coordination of window in pixel in end display resolution
 *        width - width of window in pixel in end display resolution
 *        height - height of window in pixel in end display resolution
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 *****************************************************************************/
int VPP_SetRefWindow(VPP_OBJ *vpp_obj, int *in_params);
int VPP_SetOvpWindow(VPP_OBJ *vpp_obj, int *in_params);
int VPP_CloseOvpWindow(VPP_OBJ *vpp_obj, int *in_params);
int VPP_Enable_OVPClient(VPP_OBJ *vpp_obj, int *in_params);

/*******************************************************************
 * FUNCTION: enable/disable linear horizontal scaling
 * INPUT: planeID - id of the plane
 *        enable - 1: enable; 0: disable
 *        center - center value [0, 255]
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int VPP_SetScalerNonLinearControl(VPP_OBJ *vpp_obj, int *in_params);

/***************************************************************************************
 * FUNCTION: set phase DDA counter ( sub-pixel shifting)
 * PARAMS: scalerId - scaler index
 *               hv - horizontal or vertical
 *               counter - DDA counter
 * RETURN:  1 - succeed
 *                others - failed to set phase DDA counter
 ***************************************************************************************/
int VPP_SetScalerPhaseDda(VPP_OBJ *vpp_obj, int *in_params );

/*******************************************************************
 * FUNCTION: display the display mode for a plane
 * INPUT: planeID - id of the plane
 *        mode - 0: display logo, 1: display video
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int VPP_SetDisplayMode(VPP_OBJ *vpp_obj, int *in_params);

/***************************************************
 * FUNCTION: set plane border alpha
 * PARAMS:  planeID - id of the plane
 *          alpha - border alpha value
 * RETURN: MV_VPP_OK - succeed
 ***********************************************/
int VPP_SetPlaneBorderAlpha(VPP_OBJ *vpp_obj, int *in_params);

/***************************************************
 * FUNCTION: set alpha polarity of a plane
 * PARAMS:  planeID - plane ID
 *          src_pol - source alpha polarity
 *          bg_pol - border alpha polarity
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int VPP_SetPlaneAlphaPolarity(VPP_OBJ *vpp_obj, int *in_params);

int VPP_SetPlaneMuteState(VPP_OBJ *vpp_obj, int *in_params);

/***************************************************
 * FUNCTION: mute/un-mute a plane
 * PARAMS:  planeID - plane to mute/un-mute
 *          mute - 1: mute, 0: un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int VPP_SetPlaneMute(VPP_OBJ *vpp_obj, int *in_params);

/***************************************************
 * FUNCTION: set hue/saturation/brightness/contrast
 * INPUT:  planeID - plane to adjust pq
 *           mode - the item to be handled
 *           value - the value to be set
 *           OvpOrMain - to set on main or in global way
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ***********************************************/
int VPP_SetPQICSC(VPP_OBJ *vpp_obj, int *in_params);

/***************************************************
 * FUNCTION: Read SCL Coeff LUTs
 * PARAMS:  planeID - plane to mute/un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int VPP_ReadSclCoeff(VPP_OBJ *vpp_obj, int *in_params);

/***************************************************
 * FUNCTION: Write SCL Coeff LUTs
 * PARAMS:  planeID - plane to mute/un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int VPP_WriteSclCoeff(VPP_OBJ *vpp_obj, int *in_params);

/******************************************************************************
 * FUNCTION: Write the val to addr on interrupt
 * INPUT: addr - address to which needs to be write
 *        val  - value to be written
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_RegWrite(VPP_OBJ *vpp_obj, int *in_params);

int VPP_DumpRegs(VPP_OBJ *vpp_obj,int *in_params);

/******************************************************************************
 * FUNCTION: Enable/disable dynamic clock gating if IPs not used or softbypass mode
 * INPUT: enable - enable/disable value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_enClkGate(VPP_OBJ *vpp_obj, int *in_params);

/******************************************************************************
 * FUNCTION: Enable/disable clock gating in one plane path
 * INPUT: enable - enable/disable value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_ClkGatePlane(VPP_OBJ *vpp_obj, int *in_params);

/*******************************************************************
 * FUNCTION: change z-order for all CPCBs
 * INPUT: NULL
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int VPP_ChangeZOrder(VPP_OBJ *vpp_obj, int *in_params);

/*****************************************************************************
 * FUNCTION: set csc mode for gfx plane (mainly for AYUV32 input)
 * INPUT: planeID - id of the GFX plane
 *        csc_mode - csc mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *****************************************************************************/
int VPP_SetGfxCsc(VPP_OBJ *vpp_obj, int *in_params);

/*******************************************************************
 * FUNCTION: set the source of video output
 * INPUT: voutID - id of the Vout
 *            cpcbID - id of the CPCB
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int VPP_SetVoutSource(VPP_OBJ *vpp_obj, int *in_params);

/********************************************************************************
 * FUNCTION: Set Deint Param
 * INPUT: in_params - contain cmd and DS contain parameter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetDeintEvalCommand(VPP_OBJ *vpp_obj, int *in_params);

/****************************************************************
 * FUNCTION: set De-interlacer and VNR working mode
 * INPUT: deint_mode - de-interlacer mode
 *        vnr_mode - VNR mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *****************************************************************/
int VPP_SetDeintMode(VPP_OBJ *vpp_obj, int *in_params);


/********************************************************
 * FUNCTION: set pattern size for patgen in main channel
 * PARAMS:  width - width of pattern
 *          height - height of pattern
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *********************************************************/
int VPP_SetPatternSize(VPP_OBJ *vpp_obj, int *in_params);

/********************************************************
 * FUNCTION: set pattern frame type
 * PARAMS:  type - progressive or interlaced
 *          cadence - normal, 3-2 cad or 2-2 cad
 *          rff - repeat field flag
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int VPP_SetPatternFrameType(VPP_OBJ *vpp_obj, int *in_params);

/********************************************************
 * FUNCTION: set pattern data
 * PARAMS:  mode - mode
 *          hPith - horizontal pitch
 *          vPith - vertical pitch
 *          hColPith - horizontal color pitch
 *          vColPith - vertical color pitch
 *          colSeed1 - color seed for frame 1
 *          colSeed2 - color seed for frame 2
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int VPP_SetPatternData(VPP_OBJ *vpp_obj, int *in_params);

/*******************************************************************
 * FUNCTION: set HDMI command
 * INPUT:  param0 - Hdmi command to be executed
 *      :  param(1..n) - parameters specific to the command
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_VPP_EBADPARAM - invalid parameters
 *******************************************************************/
int VPP_SetHdmiCmd(VPP_OBJ *vpp_obj, int *in_params);

#ifdef XVYCC_SUPPORT
/*******************************************************************
 * FUNCTION: enable\disable xvYCC
 * INPUT:  param0 - 1: enable; 0:disable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_VPP_EBADPARAM - invalid parameters
 *******************************************************************/
int VPP_xvYCCEnable(VPP_OBJ *vpp_obj, int *in_params);
#endif

/*******************************************************************
 * FUNCTION: Check if given hdmi output mode supported
 * INPUT:  mode: hdmi output modes
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_VPP_EUNSUPPORT - not supported
 *******************************************************************/
int VPP_HDMIOutputModeSupport(VPP_OBJ *vpp_obj, int mode);

/*******************************************************************
 * FUNCTION: set Luma Key
 * INPUT:  enable - enable control
 *         low_key - lower key
 *         up_key - upper key
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int VPP_SetLumaKey(VPP_OBJ *vpp_obj, int *in_params);

/*******************************************************************
 * FUNCTION: set CMU control
 * INPUT: chanID - id of the channel
 *        CtrlSel - CMU control selection
 *        CtrlVal - CMU control value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_VPP_EBADCALL - channel is not active
 *         MV_VPP_EBADPARAM - invalid parameters
 *******************************************************************/
int VPP_SetCmuControl(VPP_OBJ *vpp_obj, int *in_params);


/********************************************************************************
 * FUNCTION: Set ACE Param
 * INPUT: in_params - contain cmd and DS contain parameter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetAceEvalCommand(VPP_OBJ *vpp_obj, int *in_params);

/********************************************************************************
 * FUNCTION: Set FTDC Param
 * INPUT: in_params - contain cmd and DS contain parameter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetFTDCEvalCommand(VPP_OBJ *vpp_obj, int *in_params);
/********************************************************************************
 * function: set Guide Detail parameters for different modes
 * input: mode
 *
 * return: mv_vpp_ok - succeed
 *         mv_ebadparam - invalid parameters
 *         mv_eunconfig - vpp not configured
 *         mv_vpp_enodev - no device
 *         mv_eunsupport - channel not connected in configuration
 *         mv_vpp_ebadcall - channel not connected to dv1
 *         mv_ecmdqfull - command queue is full
 ********************************************************************************/
int VPP_SetGDetParams(VPP_OBJ *vpp_obj, int *in_params);

/********************************************************************************
 * function: set EE parameters for different modes
 * input: mode
 *
 * return: mv_vpp_ok - succeed
 *         mv_ebadparam - invalid parameters
 *         mv_eunconfig - vpp not configured
 *         mv_vpp_enodev - no device
 *         mv_eunsupport - channel not connected in configuration
 *         mv_vpp_ebadcall - channel not connected to dv1
 *         mv_ecmdqfull - command queue is full
 ********************************************************************************/
int VPP_SetEEParams(VPP_OBJ *vpp_obj, int *in_params);

/****************************************************************
 * FUNCTION: set EE0 parameters(main channel only)
 * INPUT:  ee_param - edge enhancer parameter profile
 * RETURN: MV_VPP_OK - SUCCEED
 *****************************************************************/
int VPP_SetEEParam(VPP_OBJ *vpp_obj, int *in_params);

/****************************************************************
 * FUNCTION: set EE0 gain value(main channel only)
 * INPUT:  ee_mode - edge enhancer mode
 * RETURN: MV_VPP_OK - SUCCEED
 *****************************************************************/
int VPP_SetEEGain(VPP_OBJ *vpp_obj, int *in_params);
int VPP_SetPQOCSC(VPP_OBJ *vpp_obj, int *in_params);

/****************************************************************
 * FUNCTION: set EE0 LTI and CTI gain value(main channel only)
 * INPUT:  ee_mode - edge enhancer mode
 * RETURN: MV_VPP_OK - SUCCEED
 *****************************************************************/
int VPP_SetEeLtiCtiGain(VPP_OBJ *vpp_obj, int *in_params);

#if defined(PE_3D_BD)
int VPP_SetSSType(VPP_OBJ *vpp_obj, int *in_params);

int VPP_SetSSDefaultOffset(VPP_OBJ *vpp_obj, int *in_params);

int VPP_SetSSOffsetSeqIDRef(VPP_OBJ *vpp_obj, int *in_params);

int VPP_SetSSOffsetMode(VPP_OBJ *vpp_obj, int *in_params);
#endif

/****************************************************************************
 * FUNCTION: set GFX0/PG plane input mode
 * INPUT: planeID - id of the plane
 *        mode - GFX_INPUT_MODE_NORMAL, GFX_INPUT_MODE_MATTE
 * RETURN: MV_VPP_OK - SUCCEED
 ***************************************************************************/
int VPP_SetGfxInputMode(VPP_OBJ *vpp_obj, int *in_params);

/****************************************************************
 * FUNCTION: write register group
 *****************************************************************/
int VPP_WriteRegGroup(VPP_OBJ *vpp_obj, int *in_params);

int VPP_SetSclCoeffs(VPP_OBJ *vpp_obj, int *in_params);

int VPP_Receive_CECData(VPP_CEC_RX_MSG_BUF *rx_buf);

/***************************************************************************************
 * FUNCTION: Configure inverse scan
 * PARAMS:   None
 *
 * RETURN:  None
 ***************************************************************************************/
int VPP_ConfigInverseScan(VPP_OBJ *vpp_obj, int *in_params);

/***************************************************************************************
 * FUNCTION: Configure rotation Angle.
 * PARAMS:   None
 *
 * RETURN:  None
 ***************************************************************************************/
int VPP_SetRotationAngle(VPP_OBJ *vpp_obj, int *in_params);
int VPP_EnableDvDump(VPP_OBJ *vpp_obj, int *in_params);
int VPP_SetDvDataDump(VPP_OBJ *vpp_obj, int *in_params);
int VPP_ClearDhubCommand(VPP_OBJ *vpp_obj, int *in_params);
int cropDispWindow(VPP_WIN *disp_win, RESOLUTION_INFO *resinfo,
                        VPP_WIN *crop_disp_win, CHAN *chan);
void VPP_Enable_INT(UINT32 DhubSemMap, UINT32 Enable);
void CEC_Enable_INT(UINT32 Enable);
int VPP_SetDcmPQ(VPP_OBJ *vpp_obj, int *in_params);
int VPP_SetDcmAoiWin(VPP_OBJ *vpp_obj, int *in_params);
UINT32 VPP_GetGcsc(VPP_OBJ *vpp_obj, int *in_params);
void VPP_SetColorTransformGain(VPP_OBJ *vpp_obj, int *in_params);
int VPP_SetOfflineParams(VPP_OBJ *vpp_obj, int *in_params);
int VPP_PrintGamma(VPP_OBJ *vpp_obj, int *in_params);
int VPP_SetGamma(VPP_OBJ *vpp_obj, int *in_params);
int VPP_SetGammaEnable(VPP_OBJ *vpp_obj, int *in_params);
int VPP_SetHVSwap_Enable(VPP_OBJ *vpp_obj, int *in_params);
int VPP_AxiLatencyMeterParams(VPP_OBJ *vpp_obj, int *in_params);
int GetVCOFreq(int resID, int bit_depth);
void VPP_SetClk(VPP_OBJ *vpp_obj, int cpcbID, int resID);
int VPP_ReConfigPath(VPP_OBJ *vpp_obj, unsigned int uiDisplayMode);
void VPP_ResetPlane(VPP_OBJ *vpp_obj, unsigned int mode);
VBUF_INFO *VPP_GetFrame(PLANE *plane);
#ifdef __cplusplus__
}
#endif

#endif

