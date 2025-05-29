/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CPCB_OVL_H_
#define _VPP_CPCB_OVL_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"

/**********************************************************************
 *macro for CPCB video overlay and OSD overlay                         *
 ***********************************************************************/

/*CPCB VOL unit number validation*/
#define CHECK_OVL_NUM(UnitNum)\
    do {\
        if ((UnitNum < VPP_CPCB_OVL0) ||(UnitNum >= VPP_CPCB_OVL_MAX))\
            return MV_VPP_EBADPARAM;\
    } while (0)

/**********************************************************************
 *Type definition for CPCB OVL block driver                            *
 ***********************************************************************/

/*Enum for OVL unit number*/
typedef enum VPP_CPCB_OVL_UNIT_NUM_T
{
    VPP_CPCB_OVL0    = 0,
    VPP_CPCB_OVL1,
    VPP_CPCB_OVL2,
    VPP_CPCB_OVL_MAX
} VPP_CPCB_OVL_UNIT_NUM, *PVPP_CPCB_OVL_UNIT_NUM;

typedef enum VPP_CPCB_OVL_CHANNEL_T
{
    VPP_CPCB_OVL_CHANNEL_MAIN = 0,
    VPP_CPCB_OVL_CHANNEL_PIP,
    VPP_CPCB_OVL_CHANNEL_OSD0,
    VPP_CPCB_OVL_CHANNEL_MAX
} VPP_CPCB_OVL_ZORDER;

typedef enum VPP_OVL_PLANE_UNIT_NUM_T
{
    Vpp_OVL_PLANE0 = 0,
    Vpp_OVL_PLANE1,
    Vpp_OVL_PLANE2,
    Vpp_OVL_PLANE3,
} VPP_OVL_PLANE_UNIT_NUM;

typedef enum VPP_OVL_OSD_MODE
{
    VPP_OVL_OSD_MODE_NORMAL = 0 ,
    VPP_OVL_OSD_MODE_MATTE
} VPP_OVL_OSD_MODE;

typedef enum VPP_PERPIXEL_ALPHA_STATUS
{
    VPP_PERPIXEL_ALPHA_DISABLE = 0,
    VPP_PERPIXEL_ALPHA_ENABLE =1,
} VPP_PERPIXEL_ALPHA_STATUS;

typedef enum VPP_OVL_OSD_DOMAIN
{
    VPP_OVL_OSD_YUV_DMN = 0,
    VPP_OVL_OSD_RGB_DMN,
} VPP_OVL_OSD_DOMAIN;

typedef enum VPP_OVL_PLANE_ALPHA_FLAG
{
    VPP_OVL_PLANE_ALPHA_FLAG_ENABLE = 0,
    VPP_OVL_PLANE_ALPHA_FLAG_DISABLE,
} VPP_OVL_PLANE_ALPHA_FLAG;
typedef enum VPP_OVL_PLANE_ALPHA
{
    VPP_OVL_PLANE_ALPHA_OPAQUE = 0xFFF,
    VPP_OVL_PLANE_ALPHA_HALF_TRANSPERENT = 0x7FF,
    VPP_OVL_PLANE_ALPHA_TRANSPERENT = 0x00,
} VPP_OVL_PLANE_ALPHA;
typedef struct VPP_OVL_UPS_CONFIG_T
{
    UINT8 uchUpsEnable;
    UINT8 uchMsbSwap;
    UINT8 uchCswap;
    UINT8 uchYshift;
    UINT8 uchCshift;
    UINT32 uiYblank;
    UINT32 uiCblank;
} VPP_OVL_UPS_CONFIG, *PVPP_OVL_UPS_CONFIG;

typedef struct VPP_OVL_OSD_CTRL
{
    INT    OSdDomain;
    INT    OsdMode;
} VPP_OVL_OSD_CTRL, *PVPP_OVL_OSD_CTRL;

typedef union VPP_OVL_AL_POL_T {
    struct {
        UINT32 p1:  1;
        UINT32 p2:  1;
        UINT32 p3:  1;
    } field;
    UINT32 data;
} VPP_OVL_AL_POL, *PVPP_OVL_AL_POL;

/**********************************************************************
 *Function definition for CPCB OVL block driver                                                        *
 ***********************************************************************/

/***************************************************************************************
 * FUNCTION: load the default register values of CPCB OVL block
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL default values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_LoadDefaultVal(VPP_OBJ *vpp_obj, INT OvlUnit);

/***************************************************************************************
 * FUNCTION: set the OSD Overlay mode and domain
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               pZOrder - *pZOrder for layer1 channel selection, *(pZOrder+1) for layer2 channel selection.....
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetChannelOrder(VPP_OBJ *vpp_obj, INT OvlUnit, INT *pZOrder);

/***************************************************************************************
 * FUNCTION: set the OSD Overlay mode and domain
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               OsdCtrl - structure of mode and domain
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetCtrl(VPP_OBJ *vpp_obj, INT OvlUnit, PVPP_OVL_OSD_CTRL pOsdCtrl);

/***************************************************************************************
 * FUNCTION: set the OSD Overlay mode and domain
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               plane - plane1 or plane2
 *               PlaneAlpha - alpha value
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetPlaneAlpha(VPP_OBJ *vpp_obj, INT OvlUnit, INT plane, UINT32 PlaneAlpha, VPP_PERPIXEL_ALPHA_STATUS enablePerPixelAlpha);

/***************************************************************************************
 * FUNCTION: set the fixed color associated with each backend plane
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *         plane - plane number
 *         Color - plane fix color value
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetPlaneColor(VPP_OBJ *vpp_obj, INT OvlUnit, INT Plane, UINT32 Color);

/***************************************************************************************
 * FUNCTION: set the polarity of alpha associated with each backend plane
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *               plane - plane number
 *               src_pol - source plane alpha polarity, 0: normal, 1: inverse
 *               bg_pol - border plane alpha polarity, 0: normal, 1: inverse
 * RETURN:  1 - succeed
 *                others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetPlaneAlphaPolarity(VPP_OBJ *vpp_obj, INT OvlUnit, INT Plane, UINT32 src_pol, UINT32 bg_pol);

/***************************************************************************************
 * FUNCTION: set plane's border alpha
 * PARAMS: OvlUnit - CPCB OVL unit number(OVL0, OVL1, OVL2)
 *         plane - plane number
 *         alpha - alpha value
 * RETURN:  1 - succeed
 *          others - failed to load CPCB OVL values to BCM buffer
 ***************************************************************************************/
INT VPP_OVL_SetBorderAlpha(VPP_OBJ *vpp_obj, INT OvlUnit, INT plane, INT alpha);

INT VPP_OVL_EnablePreMultipliedAlpha(VPP_OBJ *vpp_obj, INT OvlUnit, INT Plane, INT Enable);

INT VPP_OVL_SetARGB32PreAlpha(VPP_OBJ *vpp_obj, UINT32 planeEn);

INT VPP_OVL_GetBpp(VPP_OBJ *vpp_obj, INT32 planeID, INT32 iBitsPerPixel, INT32 iSrcFmt);
INT VPP_OVL_getDataAlignBytes_X(INT clientMode, INT maxBufferWidth);
INT VPP_OVL_getDataAlignBytes_Y(INT clientMode, INT maxBufferHeight);
INT VPP_OVL_getTileDetails(INT clientMode, INT resWidth, INT resHeight, PVPP_TILE pVppTile);

#endif/*_VPP_CPCB_OVL_H_*/

