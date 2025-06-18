/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __BDRICSC__
#define __BDRICSC__

#include "vpp_csc_common.h"
#include "vpp_api.h"
#include "vpp_module.h"

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */

#define VPP_CSC_CAP_VALUE(VAL, MIN, MAX) ((VAL)<(MIN) ? MIN : ((VAL)>(MAX) ? MAX : VAL))

/*-----------------------------------------------------------------------------
 * Standard Function Prototype
 *-----------------------------------------------------------------------------
 */

INT VPP_CSC_ICSC_SetDefaults(VPP_OBJ* vpp_obj,VPP_CSC_TYPE Channel);

/* Loads Input CSC Matrices */
INT VPP_CSC_ICSC_SetColorSpaceConversion(VPP_OBJ* vpp_obj,
                        VPP_CSC_TYPE Channel,
                        VPP_CSC_MODE CscMode);

/***************************************************************************************
 * Description: set brightness
 * PARAMS: Channel - channel selection
 *         Brightness - brightness value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load brightness value to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_ICSC_SetBrightness(VPP_OBJ *vpp_obj, VPP_CSC_TYPE Channel, INT16 Brightness);

/***************************************************************************************
 * Description: set contrast
 * PARAMS: Channel - channel selection
 *         Contrast - contrast value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load contrast value to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_ICSC_SetContrast(VPP_OBJ *vpp_obj, INT Channel, INT16 Contrast);

/***************************************************************************************
 * Description: set hue
 * PARAMS: Channel - channel selection
 *         Hue - hue value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load hue value to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_ICSC_SetHue(VPP_OBJ *vpp_obj, INT Channel, INT16 Hue);

/***************************************************************************************
 * Description: set saturation
 * PARAMS: Channel - channel selection
 *         Saturation - saturation value
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to load saturation value to BCM buffer
 ***************************************************************************************/
INT VPP_CSC_ICSC_SetSaturation(VPP_OBJ *vpp_obj, INT Channel, INT16 Sat);

/***************************************************************************************
 * Description: get user configured BCHS values
 * PARAMS: Plane ID and mode
 * RETURN:  MV_VPP_OK - succeed
 *          others - invalid params
 ***************************************************************************************/
INT VPP_CSC_GetBCHS_setvalues(VPP_OBJ *vpp_obj, UINT32 PlaneId, UINT32 mode, float *value);

#endif
