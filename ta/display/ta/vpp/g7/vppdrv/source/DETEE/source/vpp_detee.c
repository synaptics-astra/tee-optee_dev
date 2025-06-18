/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

/*
 * vpp_detee.c
 *
 *  Created on: Feb 26, 2019
 *      Author: jjose
 */

#include "vpp_gdetee.h"

INT VPP_DETEE_reset(VPP_OBJ *vpp_obj)
{
    INT RetVal = MV_VPP_OK;

    RetVal = DTEE_LoadDefaultVal(vpp_obj);

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: Set the register values of Guide detail enable/disable control params
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CPCB GDet values to BCM buffer
 ***************************************************************************************/
INT VPP_DETEE_SetGDetCtrl(VPP_OBJ *vpp_obj, PVPP_DETEE_GDET_CTRL pGDetCtrl)
{
    INT RetVal = MV_VPP_OK;

    if (vpp_obj->detee.detee_enabled == 0) {
        VPP_DETEE_Control(vpp_obj,TRUE);
    }
    RetVal = DETEE_SetGDetCtrl(vpp_obj,pGDetCtrl);

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: Set the register values of Guide detail parameter
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load DEEE GDet values to BCM buffer
 ***************************************************************************************/
INT VPP_DETEE_SetGDetParams(VPP_OBJ *vpp_obj, PVPP_DETEE_GDET_PARAMS pEEGDetParam)
{
    INT RetVal = MV_VPP_OK;

    if (vpp_obj->detee.detee_enabled == 0) {
        VPP_DETEE_Control(vpp_obj,TRUE);
    }

    RetVal = DETEE_SetGDetParams(vpp_obj,pEEGDetParam);

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: Set the register values of Guide detail based on mode
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CPCB GDet values to BCM buffer
 ***************************************************************************************/
INT VPP_DETEE_SetGDetParamsMode(VPP_OBJ *vpp_obj, INT mode)
{
    INT RetVal = MV_VPP_OK;

    if (vpp_obj->detee.detee_enabled == 0) {
        VPP_DETEE_Control(vpp_obj,TRUE);
    }
    RetVal = DETEE_SetGDetParamsMode(vpp_obj,mode);

    return RetVal;
}

INT VPP_DETEE_Control(VPP_OBJ *vpp_obj,int enable)
{
    INT RetVal = MV_VPP_OK;

    RetVal = DETEE_Control(vpp_obj,enable);

    return RetVal;
}

INT VPP_DETEE_ConfigTG(VPP_OBJ *vpp_obj, int width, int height)
{
    INT RetVal = MV_VPP_OK;

    RetVal = DETEE_ConfigTG(vpp_obj,width,height);

    return RetVal;
}

INT VPP_DETEE_TG_Start(VPP_OBJ *vpp_obj)
{

    DETEE_TG_Start(vpp_obj);

    return MV_VPP_OK;
}

INT VPP_DETEE_TG_Clear(VPP_OBJ *vpp_obj)
{
    DETEE_TG_Clear(vpp_obj);

    return MV_VPP_OK;
}

