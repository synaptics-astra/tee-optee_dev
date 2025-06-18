/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CPCB_H_
#define _VPP_CPCB_H_

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************/
/********************** CPCB APIs ******************/
/*************************************************/

/*******************************************************************
 * FUNCTION: Reset all VPP CPCB blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_Reset(VPP_OBJ *vpp_obj);

/**********************************************************************
 * FUNCTION: initialize all VPP CPCB blocks according to configuration
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 *********************************************************************/
int VPP_CPCB_Config(VPP_OBJ *vpp_obj);

/******************************************************************
 * FUNCTION: set z-order for a CPCB
 * PARAMS: *vpp_obj - pointer to VPP obj
 *         cpcbID - CPCB ID
 *         ZOrder_1_4 - packed Z order value of  the first four layers
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetZOrder(VPP_OBJ *vpp_obj, int cpcbID, int ZOrder_1_4);

/******************************************************************
 * FUNCTION: set CPCB base plane color
 * PARAMS: *vpp_obj - pointer to VPP obj
 *         cpcbID - CPCB ID
 *         bgcolor - background color value(UYV)
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetBasePlaneColor(VPP_OBJ *vpp_obj, int cpcbID, int bgcolor);

/******************************************************************
 * FUNCTION: set CPCB plane blending factor and background color
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          dvID - CPCB ID
 *          layerID - CPCB layer ID
 *          alpha - alpha of CPCB layer
 *          bgcolor - background color of CPCB layer
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetPlaneAttribute(VPP_OBJ *vpp_obj, int dvID, int layerID, int alpha, int bgcolor, ENUM_GLOBAL_ALPHA_FLAG globalAlphaFlag);

/******************************************************************
 * FUNCTION: set CPCB plane background window
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          dvID - CPCB ID
 *          layerID - CPCB layer ID
 *          x - x of layer background window
 *          y - y of layer background window
 *          width - width of layer background window
 *          height - height of layer background window
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetPlaneBGWindow(VPP_OBJ *vpp_obj, int dvID, int layerID, int x, int y, int width, int height);

/******************************************************************
 * FUNCTION: set CPCB plane source window
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          dvID - CPCB ID
 *          layerID - CPCB layer ID
 *          x - x of layer source window
 *          y - y of layer source window
 *          width - width of layer source window
 *          height - height of layer source window
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetPlaneSourceWindow(VPP_OBJ *vpp_obj, int dvID, int layerID, int x, int y, int width, int height);

/*************************************************************************
 * FUNCTION: set CPCB plane alpha polarity
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          cpcbID - CPCB ID
 *          layerID - CPCB layer ID
 *          src_pol - source alpha polarity
 *          bg_pol - border alpha polarity
 * RETURN: MV_VPP_OK
 ************************************************************************/
int VPP_CPCB_SetPlaneAlphaPolarity(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int src_pol, int bg_pol);

/*************************************************************************
 * FUNCTION: set CPCB plane crop window
 * PARAMS: *vpp_obj - pointer to VPP obj
 *          cpcbID - CPCB ID
 *          layerID - CPCB layer ID
 *          x - x of layer crop window
 *          y - y of layer crop window
 *          width - width of layer crop window
 *          height - height of layer crop window
 * RETURN: MV_VPP_OK
 * NOTE: for CPCB source window, we do chopping in upper layer function
 *       because DataLoader and SCL also need source window chopping
 *       information.
 ************************************************************************/
int VPP_CPCB_SetPlaneCropWindow(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int x, int y, int width, int height);

/*******************************************************************
 * FUNCTION: set VPP CPCB output resolution
 * PARAMS:   *vpp_obj - pointer to VPP object
 *         dvID - CPCB ID
 *         resID - resolution ID
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_CPCB_SetOutputResolution(VPP_OBJ *vpp_obj, int dvID, int resID, int color_fmt);

int VPP_CPCB_EnablePlane(VPP_OBJ *vpp_obj, int cpcbID, int layerID, int enable);

VOID VPP_CPCB_ResetPlane(VPP_OBJ *vpp_obj, INT chanID);
#ifdef __cplusplus
}
#endif

#endif

