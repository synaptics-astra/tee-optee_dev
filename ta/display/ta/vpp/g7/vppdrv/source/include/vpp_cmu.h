/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_CMU_H_
#define _VPP_CMU_H_

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    VPP_CMU_COLOR_RED = 0,
    VPP_CMU_COLOR_GREEN,
    VPP_CMU_COLOR_BLUE,
    VPP_CMU_COLOR_CYAN,
    VPP_CMU_COLOR_YELLOW,
    VPP_CMU_COLOR_MAGENTA,
    VPP_CMU_COLOR_ALL,
};

/*definition of ICR mode*/
enum{
    VPP_CMU_ICR_MODE_OFF = 0,
    VPP_CMU_ICR_MODE_SKY,
    VPP_CMU_ICR_MODE_GRASS,
    VPP_CMU_ICR_MODE_SKYGRASS,
    VPP_CMU_ICR_MODE_VIVID,
};

/*************************************************/
/********************** CMU APIs ******************/
/*************************************************/

/***************************************************************************************
 * Description: reset CMU
 * PARAMS: none
 * RETURN:  1 - succeed
 *                others - failed to load CMU register default values to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_Reset(VPP_OBJ *vpp_obj);

/*******************************************/
/*one time configuration                                       */
/*******************************************/
/***************************************************************************************
 * Description: CMU unit configuration
 * PARAMS: Channel - channel selection
 * RETURN:  1 - succeed
 *                others - failed to load CMU config values to BCM buffer
 ***************************************************************************************/
//INT VPP_CMU_Config(VPP_OBJ *vpp_obj, INT Channel, INT CmuInputType);

/*****************************************/
/*Picture control, get input parameters from OSD*/
/*****************************************/

/*******************************************/
/*Advanced control, get input parameters from OSD*/
/*******************************************/

/***************************************************************************************
 * Description: set ACE mode
 * PARAMS: Channel - channel selection
 *               Mode - ACE intensity
 * RETURN:  1 - succeed
 *                others - failed to load ACE mode to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetAceLevel(VPP_OBJ *vpp_obj, INT Channel, INT AceMode, INT Range);

/***************************************************************************************
 * Description: set the level of brightness enhancement
 * PARAMS: Channel - channel selection
 *               BrLevel - brightness enhancement level
 * RETURN:  1 - succeed
 *                others - failed to load brightness enhancement level to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetBrightnessLevel(VPP_OBJ *vpp_obj, INT Channel, INT BrLevel);

/***************************************************************************************
 * Description: enable or disable taper on the flesh tone region
 * PARAMS: Channel - channel selection
 *               enable - 1: enable 0:disable
 * RETURN:  1 - succeed
 *                others - failed to load ICR mode to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_EnableFleshToneTaper(VPP_OBJ *vpp_obj, INT Channel, INT enable);

/***************************************************************************************
 * Description: set ICR mode
 * PARAMS: Channel - channel selection
 *               Mode - ICR mode
 * RETURN:  1 - succeed
 *                others - failed to load ICR mode to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetIcrMode(VPP_OBJ *vpp_obj, INT Channel, INT Mode);

/***************************************************************************************
 * Description: set FTDC mode
 * PARAMS: Channel - channel selection
 *               Mode - FTDC mode
 * RETURN:  1 - succeed
 *                others - failed to load FTDC mode to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetFtdcMode(VPP_OBJ *vpp_obj, INT Channel, INT Mode);

/***************************************************************************************
 * Description: set color level
 * PARAMS: Channel - channel selection
 *               ColorSel - color selection
 *               ColorLev - Color level (0 - 100)
 * RETURN:  1 - succeed
 *                others - failed to load color level to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetColorLevel(VPP_OBJ *vpp_obj, INT Channel, INT ColorSel, INT16 ColorLev);

/***************************************************************************************
 * Description: set color hue
 * PARAMS: Channel - channel selection
 *               ColorSel - color selection
 *               ColorHue - Color hue (0 - 100)
 * RETURN:  1 - succeed
 *                others - failed to load color level to BCM buffer
 ***************************************************************************************/
INT VPP_CMU_SetColorHue(VPP_OBJ *vpp_obj, INT Channel, INT ColorSel, INT16 ColorHue);

INT VPP_CMU_TG_Start(VPP_OBJ *vpp_obj);

INT VPP_CMU_TG_Clear(VPP_OBJ *vpp_obj);

INT VPP_CMU_TG_Config(VPP_OBJ *vpp_obj, int width, int height);

INT VPP_CMU_Enable(VPP_OBJ *vpp_obj,INT enable);

INT VPP_CMU_SetDefault(VPP_OBJ *vpp_obj);

#ifdef __cplusplus
}
#endif

#endif

