/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file vpp_be_ta.c
//!
//! \brief Implementation of VPP commands for TrustZone.
//!
//! Purpose: This is used for controlling access to VPP API in TrustZone.
//!
//!     Version    Date                     Author
//!     V 1.00,    Dec 23 2013,             Vijay Kumar Raju P
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////


#include "tee_internal_api.h"
#include "tee_ta_vpp_be.h"
#include "tee_ta_osal.h"
#include "vpp_cmd.h"
#include "vpp_api.h"
#include "vdec_com.h"
#include "vpp_module.h"
#include "vpp_be_prv.h"
#include "vpp_be_hdmitx.h"
#include "vpp_be.h"
#include "vpp_be_hdcp.h"
#include "vpp_be_hdcp_2x.h"
#include "vpp_be_cec_msg.h"
#include "vpp_be_cec_srv.h"
#include "vpp_be_cec_srv_prv.h"

#define VPP_TA_LOG printk

#if defined (HDCP2X_REPEATER)
static const TEE_UUID TAHdcp2xRx_UUID = HDCP2X_UUID;
static TEE_TASessionHandle hdcp2xSessionHandle = 0x00;
#endif

int VPP_HDMICmdHandler(int handle){
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    //actual call happens here
    HdmiTxHdcpTask((void*)(vpp_obj));

    return (MV_VPP_OK);
}

int VPP_CECHandler(int handle){
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    //actual call happens here
    CEC_Task((void*)(vpp_obj));

    return (MV_VPP_OK);
}

static HRESULT hdcp_callback(void *vpp_ob, UINT32 uiEventCode, VOID *pEventInfo, VOID *pContext){

    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_HDCP;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);

    TEE_Param cb_params[4];
    VPP_OBJ *vpp_obj;
    vpp_obj=(VPP_OBJ *)vpp_ob;
    cb_params[0].value.a = uiEventCode;
    cb_params[0].value.b = *(INT32 *)pEventInfo;
    cb_params[1].value.a = vpp_obj->global.hdcp_cb_params.CallbackFuncAddr;
    cb_params[2].value.a = vpp_obj->global.hdcp_cb_params.UserDataAddr;

    cb_params[3].value.a = 0xdeadbeef;
    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: hack

    return retVal;
}

static HRESULT hdmi_callback(void *vpp_ob,UINT32 uiEventCode, VOID *pEventInfo, VOID *pContext){

    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_HDMI;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);

    TEE_Param cb_params[4];
    VPP_OBJ *vpp_obj;
    vpp_obj=(VPP_OBJ *)vpp_ob;
    cb_params[0].value.a = uiEventCode;
    cb_params[0].value.b = *(INT32 *)pEventInfo;
    cb_params[1].value.a = vpp_obj->global.hdmi_cb_params.CallbackFuncAddr;
    cb_params[2].value.a = vpp_obj->global.hdmi_cb_params.UserDataAddr;

    cb_params[3].value.a = 0xdeadbeef;
    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: hack


	/*
    printk("tzdebug: hdmi callback triggered %d %d %X %X", uiEventCode, *(INT32 *)pEventInfo,
            hdmi_cb_params.CallbackFuncAddr,
            hdmi_cb_params.UserDataAddr);
	*/


    return retVal;
}


int VPP_RegHdcpCallback(int handle, UINT32 eventCode,
        UINT64 funcAddr, UINT64 contextAddr){
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    vpp_obj->global.hdcp_cb_params.CallbackFuncAddr = funcAddr;
    vpp_obj->global.hdcp_cb_params.UserDataAddr = contextAddr;
    MV_VPPOBJ_RegisterHdcpCallback(handle, eventCode, hdcp_callback, NULL);

    return (MV_VPP_OK);
}

int VPP_UnRegHdcpCallback(int handle){

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);


    //clean the buffer???
    MV_VPPOBJ_UnRegisterHdcpCallback(handle);

    return (MV_VPP_OK);
}


int VPP_RegHdmiCallback(int handle, UINT32 eventCode,
        UINT64 funcAddr, UINT64 contextAddr){
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    vpp_obj->global.hdmi_cb_params.CallbackFuncAddr = funcAddr;
    vpp_obj->global.hdmi_cb_params.UserDataAddr = contextAddr;
    MV_VPPOBJ_RegisterHdmiCallback(handle, eventCode, hdmi_callback, NULL);

    return (MV_VPP_OK);
}

int VPP_UnRegHdmiCallback(int handle){

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    //clean the buffer???
    MV_VPPOBJ_UnRegisterHdmiCallback(handle);

    return (MV_VPP_OK);
}

#ifdef EARC_RX_ENABLED
static HRESULT earc_callback(void *vpp_ob,UINT32 uiEventCode, VOID *pEventInfo, VOID *pContext){

    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_EARC;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);

    TEE_Param cb_params[4];
    VPP_OBJ *vpp_obj;
    vpp_obj=(VPP_OBJ *)vpp_ob;
    cb_params[0].value.a = uiEventCode;
    cb_params[0].value.b = (UINT32) pEventInfo;
    cb_params[1].value.a = vpp_obj->global.earc_cb_params.CallbackFuncAddr;
    cb_params[2].value.a = vpp_obj->global.earc_cb_params.UserDataAddr;

    memcpy(&(vpp_obj->global.gEarcEvtInfo), pEventInfo,
            sizeof(EARC_EVT_INFO));
    cb_params[3].value.a = 0xdeadbeef;
    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);

    return retVal;
}

int VPP_RegEarcCallback(int handle, UINT32 eventCode,
        UINT64 funcAddr, UINT64 contextAddr){
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    vpp_obj->global.earc_cb_params.CallbackFuncAddr = funcAddr;
    vpp_obj->global.earc_cb_params.UserDataAddr = contextAddr;
    MV_VPPOBJ_eARC_RegisterCallback(handle, eventCode, earc_callback, NULL);

    return (MV_VPP_OK);
}

int VPP_UnRegEarcCallback(int handle){
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    MV_VPPOBJ_eARC_UnRegisterCallback(handle);

    return (MV_VPP_OK);
}

int VPP_GetEarcEvtInfo(int handle, void *pbuf)
{
    EARC_EVT_INFO *pshmbuf = (EARC_EVT_INFO *)pbuf;
    VPP_OBJ *vpp_obj;
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if(!pshmbuf)
        return MV_VPP_EBADPARAM;
    memcpy(pshmbuf, &(vpp_obj->global.gEarcEvtInfo),
            sizeof(EARC_EVT_INFO));

    return (MV_VPP_OK);
}

#endif /* EARC_RX_ENABLED */

int VPP_SetHdmi5vState(int state)
{
    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_HDMI_5V;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = state;
    cb_params[1].value.a = 0xdeadbeef;

    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    if (retVal || (cb_params[1].value.a == 0xdeadbeef)) {
        return MV_VPP_EIOFAIL;
    }
    return retVal;
}

static HRESULT cec_callback(void *vpp_ob,UINT32 uiEventCode, VOID *pEventInfo, VOID *pContext){

    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_CEC;
    VPP_OBJ *vpp_obj;
    vpp_obj = (VPP_OBJ *)vpp_ob;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);

    TEE_Param cb_params[4];
    cb_params[0].value.a = uiEventCode;
    cb_params[0].value.b = (uint32_t)pEventInfo; //send address
    cb_params[1].value.a = vpp_obj->global.cec_cb_params.CallbackFuncAddr;
    cb_params[2].value.a = vpp_obj->global.cec_cb_params.UserDataAddr;

    memcpy(&(vpp_obj->global.gCecEvtInfo), pEventInfo, sizeof(VPP_HDMI_CEC_EVT_INFO));
    cb_params[3].value.a = 0xdeadbeef;
    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: hack

    return retVal;
}

int VPP_GetCECEvtInfo(int handle, void *pbuf)
{
    VPP_HDMI_CEC_EVT_INFO *pshmbuf = (VPP_HDMI_CEC_EVT_INFO *)pbuf;
    VPP_OBJ *vpp_obj;
    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }
    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    if(!pshmbuf)
        return MV_VPP_EBADPARAM;
    memcpy(pshmbuf, &(vpp_obj->global.gCecEvtInfo),sizeof(VPP_HDMI_CEC_EVT_INFO));
    return (MV_VPP_OK);
}

int VPP_RegCecCallback(int handle, UINT32 eventCode,
        UINT64 funcAddr, UINT64 contextAddr){
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

    vpp_obj->global.cec_cb_params.CallbackFuncAddr = funcAddr;
    vpp_obj->global.cec_cb_params.UserDataAddr = contextAddr;
    MV_VPPOBJ_CecRegisterCallback(handle, eventCode, cec_callback, NULL);

    return (MV_VPP_OK);
}

int VPP_UnRegCecCallback(int handle){

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    //clean the buffer???
    MV_VPPOBJ_CecUnregisterCallback(handle);

    return (MV_VPP_OK);
}


// hdmi cpcb0 callback
static void hdmicpcb0_callback(int id, void *ptr1, void *ptr2){

    VPP_OBJ *vpp_obj;
    uint32_t cb_cmdID = VPP_CB_HDMICPCB0;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);

    TEE_Param cb_params[4];
    vpp_obj = (VPP_OBJ *)ptr1;

    cb_params[0].value.a = vpp_obj->global.hdmicpcb0_cb_params.CallbackFuncAddr;// ((Vpp_Callback_Params *)pContext)->CallbackFuncAddr;

    cb_params[1].value.a = 0xdeadbeef;
    VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: hack

}

int VPP_RegHdmiCpcb0Callback(int handle, UINT64 funcAddr){
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    vpp_obj->global.hdmicpcb0_cb_params.CallbackFuncAddr = funcAddr;
    vpp_obj->global.hdmicpcb0_cb_params.UserDataAddr = 0L;
    MV_VPPOBJ_RegisterHdmiCpcb0Callback(handle, hdmicpcb0_callback);

//    printk("tzdebug: hdmicpcb0 registered %X", funcAddr);

    return (MV_VPP_OK);
}

int VPP_UnRegHdmiCpcb0Callback(int handle){

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    //clean the buffer???
    MV_VPPOBJ_UnRegisterHdmiCpcb0Callback(handle);

    return (MV_VPP_OK);
}


// Following are call back functions
HRESULT VPP_CreateHdmiTask(void){
    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_CREATE_HDMITASK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = 0xdeadbeef;

    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: hack

    return retVal;
}

HRESULT VPP_CreateCECTask(void){
    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_CREATE_CECTASK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = 0xdeadbeef;
    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: no hack

    return retVal;
}

HRESULT VPP_DeleteHdmiTask(void){
    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_DELETE_HDMITASK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = 0xdeadbeef;

    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: hack

    return retVal;
}

HRESULT VPP_DeleteCECTask(void){
    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_DELETE_CECTASK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = 0xdeadbeef;
    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: no hack

    return retVal;
}

int VPP_HDMI_HPDTaskHandler(int handle)
{
    VPP_OBJ *vpp_obj;

    if (g_vpp->magic != VPP_MAGIC){
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];

#ifdef HDMI_HPD_TASK
    HdmiTxHpdTask((void*)(vpp_obj));
#else
    MV_OSAL_Task_Sleep(100);
#endif

    return (MV_VPP_OK);
}

HRESULT VPP_CreateHdmiHpdTask(void)
{
    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_CREATE_HDMI_HPDTASK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = 0xdeadbeef;

    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return retVal;
}

HRESULT VPP_DeleteHdmiHpdTask(void){
    HRESULT retVal = MV_VPP_OK;

    uint32_t cb_cmdID = VPP_CB_DELETE_HDMI_HPDTASK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = 0xdeadbeef;

    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    //retVal = cb_params[0].value.a; //FIXME: hack

    return retVal;
}

int VPP_UpdateDynamicList(UINT8* mnfrId, UINT8* prodId)
{
    uint32_t cb_cmdID = VPP_CB_HDMI_SINKBL_UPDATE;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = mnfrId[0] | (mnfrId[1] << 8) | (mnfrId[2] << 16);
    cb_params[0].value.b = prodId[0] | (prodId[1] << 8) | (prodId[2] << 16) | (prodId[3] << 24);
    cb_params[1].value.a = 0xdeadbeef;

    if ((VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL)) || (cb_params[1].value.a == 0xdeadbeef)) {
        return MV_VPP_EIOFAIL;
    }
    return S_OK;
}
