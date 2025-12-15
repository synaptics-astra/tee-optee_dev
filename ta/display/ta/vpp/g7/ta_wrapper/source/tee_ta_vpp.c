/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_vpp.c
//!
//! \brief Implementation of VPP commands for TrustZone.
//!
//! Purpose: This is used for controlling access to VPP API in TrustZone.
//!
//!     Version    Date                     Author
//!     V 1.00,    Nov 12 2013,             Jiankang Yu
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "tee_ta_vpp.h"
#include "tee_ta_vpp_be.h"
#include "vpp_env_ur.h"
#include "ovps_api.h"
#include "vpp_ptm.h"
#include "vpp_clk.h"
#include "vpp_debug_util.h"
#include "outctrl_cmd.h"
#include "vpp_obj.h"

#include "vpp_be_hdr.h"
#include "vpp_dv_api.h"
#include "vpp_ngptv_api.h"
#include "vpp_vmx_ta_wrap.h"
#include "vpp_isr.h"
#include "tee_ta_cmd_check.h"

/* ***************************************************
 * Create a new instance for the trusted application *
 * ***************************************************/
TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    TEE_Result result =TEE_SUCCESS;
#ifdef TZ_3_0
    result = VPP_TA_AddMemMap();
#endif
    VPP_TA_IncCreateInstanceCount();
    return result;
}

/* *************************************************
 * Destory the instance of the trusted application *
 * *************************************************/
void TA_EXPORT TA_DestroyEntryPoint(void)
{
    return;
}

/* ********************************************************************
 * open a session with the trusted application when a client requests *
 * ********************************************************************/
TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(uint32_t paramTypes,
                                        TEE_Param params[4],
                                        void **sessionContext)
{
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
    MV_VPPOBJ_FetchOtpConfig();
#endif
    return TEE_SUCCESS;
}

/* *****************************************************
 * Close a client session with the trusted application *
 * *****************************************************/
void TA_EXPORT TA_CloseSessionEntryPoint(void *sessionContext)
{
    VPP_PtmHandleSessionDecrement();

    return;
}


/* *****************************************
 * Invoke a command within a given session *
 * *****************************************/
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(void *sessionContext,
                                          uint32_t commandID,
                                          uint32_t paramTypes,
                                          TEE_Param params[4])
{
    UINT32 VPPBase = 0xF7f60000;
    INT handle;
    TEE_Result res = TEE_SUCCESS;//TEE_ERROR_NOT_SUPPORTED;
    TEE_Identity identity;
    VPP_WIN win;
    VPP_WIN *pWin;
    VPP_INIT_PARM vpp_init_parm;
    VBUF_INFO *vbuf_info;
    VPP_SHM_BUFFER_DESC bufDesc;
#ifdef TZ_3_0
    int instanceID=0;
    instanceID=GETINST_ID(commandID);
    commandID = GETCMD_ID(commandID);
#endif

    VPP_CLK_GATE_SIGNATURE signature;
    VPP_CLKGATE_OPT options;
    VPP_COLORTEMP ClrTempGainParam;
    VPP_CMU_EE_CTRL_PARAM CmuEeCtrlParam;

    unsigned int Size;


    VPP_STS_COUNTER sts_counter;


#ifdef VPP_TA_LOG_CMD_ENTRY
    VPPAPI_DBG_INFO("cmd:%ld, para0a=%lx, para0b=%lx, para1a=%lx, para1b=%lx,\n",
        commandID, params[0].value.a, params[0].value.b,
        params[1].value.a, params[1].value.b);
#endif

    ///Check for any TA voilation happened in any session before proceeding in this session
#ifdef TZ_3_0
    if(!((commandID==VPP_INIT) || (commandID==VPP_INITIATE_VPPS)))
#else //TZ_3_0
    if(!((commandID==VPP_INIT)))
#endif //TZ_3_0
    {
        VPP_VMXAPI_CheckTaAccessVoilation();
    }

    //Perform origin check
    res = VPP_ValidateOrigin(commandID);
    if(res != TEE_SUCCESS) {
        return res;
    }

    switch (commandID) {
        case VPP_INIT:
        memset(&vpp_init_parm, 0, sizeof(VPP_INIT_PARM));
        /*set default value to vpp_init_parm*/
        vpp_init_parm.iHDMIEnable = 1;
        vpp_init_parm.iVdacEnable = 1;
        /*get vpp_init_parm*/
        vpp_init_parm.iHDMIEnable = ((params[0].value.a == 1)? 1 : 0);
        vpp_init_parm.iVdacEnable = ((params[0].value.b == 1)? 1 : 0);
#if defined (TZ_3_0) && defined (VPP_ENABLE_INTERNAL_MEM_MGR)
       vpp_init_parm.phShm = params[2].value.a;
       vpp_init_parm.uiShmSize=params[2].value.b;
#endif
       params[1].value.a = MV_VPP_Init(&vpp_init_parm);
#ifdef TZ_3_0
       params[1].value.b = vpp_init_parm.g_vpp;
#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
       params[0].value.b = vpp_init_parm.gMemhandle;
#endif
#endif
        VPP_VMXAPI_InitTaAccessVoilation();
        MV_VPP_Clk_Ctrl();
        break;

#ifdef TZ_3_0
        case VPP_INITIATE_VPPS:
        res = MV_VPP_INITVPPS(instanceID, params[0].value.a,params[0].value.b);
        if (res == MV_VPP_OK) {
           VPP_VMXAPI_InitTaAccessVoilation();
           VPP_PtmHandleSessionIncrement();
        } else if (res == MV_VPP_ESWSTATEWRONG) {
           VPPAPI_DBG_ERROR("Double init found\n");
           res = MV_VPP_OK;
        }
        params[1].value.a = res;
        break;
#endif
        case VPP_AVIORESET:
        params[0].value.a = MV_VPPOBJ_AVIO_Reset();
        break;

        case VPP_RESET:
        params[0].value.a = MV_VPPOBJ_Reset(0);
        break;

        case VPP_CREATE:
        params[0].value.a = MV_VPPOBJ_Create(VPPBase,
                                             &handle);
        break;
        case VPP_DESTROY:
        params[0].value.a = MV_VPPOBJ_Destroy(0);
        break;

        case VPP_PASSSHM:
        params[3].value.b = MV_VPPOBJ_BindSHM(params);
        break;

        case NXG_COMMAND_HANDLER:
        params[2].value.b = MV_VPPOBJ_NxgTaCommandHandler(params);
        break;

        case VPP_SETRES:
        params[2].value.a = MV_VPPOBJ_SetCPCBOutputResolution(0,
                                params[0].value.a, params[0].value.b,
                                params[1].value.a);
        break;

        case VPP_SETREFWIN:
        win.x = params[0].value.b;
        win.y = params[1].value.a;
        win.width = params[1].value.b;
        win.height = params[2].value.a;
        params[3].value.a = MV_VPPOBJ_SetRefWindow(0, params[0].value.a, &win);
        break;

        case VPP_SETREFWINFROMISR:
        win.x = params[0].value.b;
        win.y = params[1].value.a;
        win.width = params[1].value.b;
        win.height = params[2].value.a;
        params[3].value.a = MV_VPPOBJ_SetRefWindowFromISR(0, params[0].value.a, &win);
        break;

        case VPP_OPENDISPWIN:
        {
            VPP_WIN_ATTR attr;
            VPP_WIN_ATTR *pAttr;

            win.x = params[0].value.b;
            win.y = params[1].value.a;
            win.width = params[1].value.b;
            win.height = params[2].value.a;
            attr.bgcolor = params[2].value.b;
            attr.alpha= params[3].value.a;
            attr.globalAlphaFlag = VPP_ATTR_PARAM_GET_GLOBALPHA(params[0].value.a);
            pAttr = ((attr.bgcolor == -1) && (attr.alpha == -1))? NULL : &attr;
            params[3].value.b = MV_VPPOBJ_OpenDispWindow(0, VPP_ATTR_PARAM_GET_PLANEID(params[0].value.a),
                                                     &win, pAttr);
        }
        break;

        case VPP_CHANGEDISPWIN:
        {
            VPP_WIN_ATTR attr;
            VPP_WIN_ATTR *pAttr;

            win.x = params[0].value.b;
            win.y = params[1].value.a;
            win.width = params[1].value.b;
            win.height = params[2].value.a;
            attr.bgcolor = params[2].value.b;
            attr.alpha= params[3].value.a;
            attr.globalAlphaFlag = VPP_ATTR_PARAM_GET_GLOBALPHA(params[0].value.a);
            pWin = ((win.x == -1)&&(win.y == -1)&&(win.width == -1)&&(win.height == -1))? NULL : &win;
            pAttr = ((attr.bgcolor == -1) && (attr.alpha == -1))? NULL : &attr;
            params[3].value.b = MV_VPPOBJ_ChangeDispWindow(0, VPP_ATTR_PARAM_GET_PLANEID(params[0].value.a),
                                                     pWin, pAttr);
        }
        break;

        case VPP_CHANGEDISPWINFROMISR:
        {
            VPP_WIN_ATTR attr;
            VPP_WIN_ATTR *pAttr;

            win.x = params[0].value.b;
            win.y = params[1].value.a;
            win.width = params[1].value.b;
            win.height = params[2].value.a;
            attr.bgcolor = params[2].value.b;
            attr.alpha= params[3].value.a;
            attr.globalAlphaFlag = VPP_ATTR_PARAM_GET_GLOBALPHA(params[0].value.a);
            pWin = ((win.x == -1)&&(win.y == -1)&&(win.width == -1)&&(win.height == -1))? NULL : &win;
            pAttr = ((attr.bgcolor == -1) && (attr.alpha == -1))? NULL : &attr;
            params[3].value.b = MV_VPPOBJ_ChangeDispWindowFromISR(0, VPP_ATTR_PARAM_GET_PLANEID(params[0].value.a),
                                                     pWin, pAttr);
        }
        break;

        case VPP_GETDISPWINDOW:
        {
            VPP_WIN_ATTR attr;

            win.x = win.y = win.width = win.height = 0;
            attr.alpha = attr.bgcolor = 0;
            params[0].value.b = MV_VPPOBJ_GetDispWindow(0, params[0].value.a, &win, &attr);
            params[1].value.a = win.x;
            params[1].value.b = win.y;
            params[2].value.a = win.width;
            params[2].value.b = win.height;
            params[3].value.a = attr.bgcolor;
            params[3].value.b = attr.alpha;
            params[0].value.a = attr.globalAlphaFlag;
        }
        break;

        case VPP_GETREFWINDOW:
        win.x = win.y = win.width = win.height = 0;
        params[0].value.b = MV_VPPOBJ_GetRefWindow(0, params[0].value.a, &win);
        params[1].value.a = win.x;
        params[1].value.b = win.y;
        params[2].value.a = win.width;
        params[2].value.b = win.height;
        break;

        case VPP_GETCONTENTWINDOW:
        win.x = win.y = win.width = win.height = 0;
        params[0].value.b = MV_VPPOBJ_GetContentWindow(0, params[0].value.a, &win);
        params[1].value.a = win.x;
        params[1].value.b = win.y;
        params[2].value.a = win.width;
        params[2].value.b = win.height;
        break;

        case VPP_HDMISETVIDFMT:
        params[2].value.a = MV_VPPOBJ_SetHdmiVideoFmt(0, params[0].value.a,
                                params[0].value.b, params[1].value.a);
        break;

        case VPP_HDMISETTMDSMUXSEL:
        params[0].value.b = MV_VPPOBJ_SetHdmiTmdsMuxSel(0, params[0].value.a);
        break;


        case VPP_HANDLEINT:
        VPP_ISR_Handler(params[0].value.a,
                        params[0].value.b);
        params[1].value.a = 0;
        break;

        case CEC_HANDLEINT:
        CEC_ISR_Handler(params[0].value.a,
                        params[0].value.b);
        params[1].value.a = 0;
        break;

        case VPP_RECYCLEFRAME:
        vbuf_info = (VBUF_INFO *)MV_VPPOBJ_RecycleFrames(0, params[0].value.a);
        if (vbuf_info) {
            params[1].value.a = vbuf_info->m_hDesc;
        } else {
            params[1].value.a = 0;
        }
        break;

        case VPP_SETDISPMODE:
        params[1].value.a = MV_VPPOBJ_SetDisplayMode(0, params[0].value.a,
                                                     params[0].value.b);
        break;

        case VPP_SETDISPMODEFROMISR:
        params[1].value.a = MV_VPPOBJ_SetDisplayModeFromISR(0, params[0].value.a,
                                                     params[0].value.b);
        break;

        case VPP_GETFIELDID:
        params[1].value.b = MV_VPPOBJ_GetFieldID(0, params[0].value.a, (INT *)&params[1].value.a);
        break;

        case VPP_CLOSEDISPWINDOW:
        params[1].value.a = MV_VPPOBJ_CloseDispWindow(0, params[0].value.a);
        break;

        case VPP_GETPLANESTATUS:
        params[1].value.b = MV_VPPOBJ_GetPlaneStatus(0, params[0].value.a, (INT *)&params[1].value.a);
        break;

        case VPP_SETCPCBBGCOLOR:
        {
            VPP_BG_COLOR BgColor;
            BgColor.Color = params[0].value.b;
            BgColor.ColorFmt = params[1].value.a;
            params[2].value.a = MV_VPPOBJ_SetCPCBBgColor(0, params[0].value.a, &BgColor);
        }
        break;

        case VPP_GETCPCBOUTRES:
        params[1].value.b = MV_VPPOBJ_GetCPCBOutputResolution(0, params[0].value.a, (INT *)&params[1].value.a);
        break;

        case VPP_GETCPCBVCOFREQ:
        params[1].value.b = MV_VPPOBJ_GetCPCBVCOFreq(0, params[0].value.a, (INT *)&params[1].value.a);
        break;

        case VPP_GETCPCBINPUTCLOCK:
        params[1].value.b = MV_VPPOBJ_GetCPCBInputClock(0, params[0].value.a, (unsigned int *)&params[1].value.a);
        break;

        case VPP_GETCPCBOUTPIXELCLOCK:
        params[1].value.b = MV_VPPOBJ_GetCPCBOutputPixelClock(0, params[0].value.a, (INT *)&params[1].value.a);
        break;

        case VPP_GETCPCBOUTFRAMERATE:
        params[1].value.b = MV_VPPOBJ_GetCPCBOutputFrameRate(0, params[0].value.a, (INT *)&params[1].value.a);
        break;

        case VPP_SETPLANEMUTE:
        params[1].value.a = MV_VPPOBJ_SetPlaneMute(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_GETPLANEMUTE:
        params[1].value.b = MV_VPPOBJ_GetPlaneMute(0, params[0].value.a, (unsigned int *)&params[1].value.a);
        break;

        case VPP_SETROTANGLE:
        params[1].value.a = MV_VPPOBJ_SetRotationAngle(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_GETROTANGLE:
        params[1].value.a = MV_VPPOBJ_GetRotationAngle(0, params[0].value.a, (INT *)&params[0].value.b);
        break;

        case VPP_CFGINVSCAN:
        params[1].value.a = MV_VPPOBJ_ConfigInverseScan(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_SETCROPWIN:
        win.x = params[0].value.b;
        win.y = params[1].value.a;
        win.width = params[1].value.b;
        win.height = params[2].value.a;
        params[3].value.a = MV_VPPOBJ_SetCropWindow(0, params[0].value.a, &win);
        break;

        case VPP_SETDEINTMODE:
        params[1].value.a = MV_VPPOBJ_SetDeintMode(0, params[0].value.a);
        break;

        case VPP_ENCLKGATE:
        params[1].value.a = MV_VPPOBJ_enClkGate(0, params[0].value.a);
        break;

        case VPP_GETCLKSIGNATURE:
        options.opt=params[0].value.a;
        params[0].value.b = MV_VPPOBJ_GetClkGateSignature(0, (UINT32 *)&(params[1].value.a),&signature,options);
        params[1].value.b = signature.signature0;
        params[2].value.a = signature.signature1;
        params[2].value.b = signature.signature2;
        params[3].value.a = signature.signature3;
        params[3].value.b = signature.signature4;
        params[0].value.a = signature.signature5;
        break;

#ifndef REMOVE_BG4_NON_SUPPORTED_INTERFACE
        case VPP_CAPTUREAUXFRAME:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_CaptureAUXFrame(0, params[0].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_REGCAPTUREAUXFRAME:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_RegisterCaptureAUXCallback(0, params[0].value.a, params[0].value.b, NULL);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;
#endif //REMOVE_BG4_NON_SUPPORTED_INTERFACE

        case VPP_GETRUNTIMESTATUS:
        params[2].value.a = MV_VPPOBJ_GetViewID(0, params[0].value.a, (INT *)&params[1].value.a);

#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[2].value.b = MV_VPPOBJ_GetUHDStatus(0, params[0].value.a, (INT *)&params[1].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

#ifdef VPP_ENABLE_FRAME_CAPTURE
        case VPP_UPDATE_CAPTURE_FRAME_BUFFER:
        {
            VPP_OBJ *vpp_obj = g_vpp->vpp_objs[0];

            /* Use the Buffer provided from caller. If not,
             * memory will be allocated from internal memory and
             * may fail due to memory unavailability.
             */
            if(!vpp_obj->vpp_FrameBuff_addr) {
                vpp_obj->vpp_FrameBuff_addr = (void*)params[0].value.a;
                params[1].value.a = TEE_SUCCESS;
            } else {
                VPPAPI_DBG_ERROR("VPP_UPDATE_CAPTURE_FRAME_BUFFER: Already Updated");
                params[1].value.a = TEE_ERROR_NOT_SUPPORTED;
            }
        }
        break;

        case VPP_REGCAPTUREVIDFRAME:
        params[1].value.a = MV_VPPOBJ_RegisterCaptureVideoCallback(0, (VPP_CAPTURE_CALLBACK)params[0].value.a);
        break;

        case VPP_FRAMECAPTUREHANDLER:
        params[0].value.a = MV_VPPOBJ_FrameCaptureTask( 0 );
        break;
#endif

        /*3D command*/
        case VPP_SET3DCONVERTMODE:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_Set3DConvertMode(0, params[0].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SET3DINVERTMODE:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_Set3DInvertMode(0, params[0].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_GET3DCONVERTMODE:
#ifdef PE_3D_FMT_CONVERT
        params[1].value.a = MV_VPPOBJ_Get3DConvertMode(0, (INT *)&params[0].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_GETVIEWID:
        params[1].value.b = MV_VPPOBJ_GetViewID(0, params[0].value.a, (INT *)&params[1].value.a);
        break;

        case VPP_SETSSTYPE:
        params[1].value.a = MV_VPPOBJ_SetSSType(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_SETSSDEFAULTOFFSET:
        params[1].value.a = MV_VPPOBJ_SetSSDefaultOffset(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_SETSSOFFSETMODE:
        params[1].value.a = MV_VPPOBJ_SetSSOffsetMode(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_SETSSOFFSETSEQIDREF:
        params[1].value.a = MV_VPPOBJ_SetSSOffsetSeqIDRef(0, params[0].value.a, params[0].value.b);
        break;

        /*LVDS command*/
#ifndef REMOVE_BG4_NON_SUPPORTED_INTERFACE
        case VPP_SETLVDSPOWER:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_SetLvdsPower(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_LVTXINIT:
#ifdef VPP_CONFIG_LVTX
        panelConfig.u32[0] = params[0].value.a;
        panelConfig.u32[1] = params[0].value.b;
        params[1].value.a = MV_VPPOBJ_LVTX_Init(0, &panelConfig);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_LVTXSETSCRAMBLE:
#ifdef VPP_CONFIG_LVTX
        params[1].value.b = MV_VPPOBJ_VBO_SetScramblerEn(0, params[0].value.a,  params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_LVTXSETENCODER:
#ifdef VPP_CONFIG_LVTX
        params[1].value.b = MV_VPPOBJ_VBO_SetEncoderEn(0, params[0].value.a,  params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_LVTXSETEMPHASIS:
#ifdef VPP_CONFIG_LVTX
        params[1].value.b = MV_VPPOBJ_VBO_SetEmphasis(0, params[0].value.a,  params[0].value.b,
                                                                    params[1].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_LVTXGETRECEIVERSTATUS:
#ifdef VPP_CONFIG_LVTX
        params[1].value.a = MV_VPPOBJ_VBO_GetRecieverStatus(0, &params[0].value.a, &params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SETDEFAULTLVDSMAPPING:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_SetDefaultLvdsMapping(0, params[0].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SETLVDSICSC:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_SetLvdsIcsc(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_GETLVDSICSC:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.b = MV_VPPOBJ_GetLvdsIcsc(0, params[0].value.a, &params[1].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_ENABLELVDSGAMMA:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_EnableLvdsGamma(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SETLVDSWBGAIN:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_SetLvdsWBGain(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_GETLVDSWBGAIN:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.b = MV_VPPOBJ_GetLvdsWBGain(0, params[0].value.a, &params[1].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SETLVDSWBOFFSET:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_SetLvdsWBOffset(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_GETLVDSWBOFFSET:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.b = MV_VPPOBJ_GetLvdsWBOffset(0, params[0].value.a, &params[1].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SETLVDSSSC:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[2].value.a = MV_VPPOBJ_SetLvdsSSC(0, params[0].value.a, params[0].value.b,
                                                    params[1].value.a, params[1].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_BYPASSLVDSGAMMA_DEGAMMA:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_BypassLvdsDeGamma_Gamma(0, params[0].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_WRITELVDSGAMMALUT:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_WriteLvdsGammaLut(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_CONFIGINVERSESCAN:
#if defined(ENABLE_BG4_NON_SUPPORTED_INTERFACE)
        params[1].value.a = MV_VPPOBJ_ConfigInverseScan(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SETVBYONEMODE:
#if defined(ENABLE_BG4_NON_SUPPORTED_INTERFACE)
        params[1].value.a = MV_VPPOBJ_SetVbyOnePanelMode(0, params[0].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;
#endif //REMOVE_BG4_NON_SUPPORTED_INTERFACE

        /*PQ command*/
        case VPP_SETALPHAPOLARITY:
        params[2].value.a = MV_VPPOBJ_SetAlphaPolarity(0, params[0].value.a, params[0].value.b,
                                                          params[1].value.a);
        break;

        case VPP_SETSCALARCOEFFMODE:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[2].value.a = MV_VPPOBJ_SetScalerCOEFFMode(0, params[0].value.a, params[0].value.b,
                                                            params[1].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_LOADSCLCUSTCOEFFS:
        params[2].value.a = MV_VPPOBJ_LoadSCLCustCoeffs(0, params[0].value.a, params[0].value.b,
                                                           params[1].value.a);
        break;

        case VPP_RELOADSCLDEFCOEFFS:
        params[2].value.a = MV_VPPOBJ_ReloadSCLDefCoeffs(0, params[0].value.a, params[0].value.b,
                                                            params[1].value.a);
        break;

        case VPP_GETSCLCOEFFMODES:
        params[2].value.a = MV_VPPOBJ_GetSCLCoeffModes(0, params[0].value.a, (INT *)&params[1].value.a,
                                                            (INT *)&params[1].value.b);
        break;

        case VPP_SETPICTURECONTROL:
        params[2].value.a = MV_VPPOBJ_SetPictureControl(0, params[0].value.a, params[0].value.b,
                                                           params[1].value.a);
        break;

        case VPP_SETBRLEVEL:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_SetBrLevel(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SETSCALARNONLINERCONTROL:
        params[2].value.a = MV_VPPOBJ_SetScalerNonLinearControl(0, params[0].value.a, params[0].value.b,
                                                                    params[1].value.a, params[1].value.b);
        break;

        case VPP_SETDEINTVNRMODE:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[1].value.a = MV_VPPOBJ_SetDeintVNRMode(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case VPP_SETSCALARPHASEDDA:
        params[2].value.a = MV_VPPOBJ_SetScalerPhaseDda(0, params[0].value.a, params[0].value.b,
                                                            params[1].value.a);
        break;

        case VPP_SETPLANEBORDERALPHA:
        params[1].value.a = MV_VPPOBJ_SetPlaneBorderAlpha(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_SETVOUTSOURCE:
        params[2].value.a = MV_VPPOBJ_SetVoutSource(0, params[0].value.a, params[0].value.b,
                                                       params[1].value.a);
        break;

        case VPP_SETVOUTMUTE:
        params[1].value.a = MV_VPPOBJ_SetVoutMute(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_SETVOUTMUTE_EXT:
        params[2].value.a = MV_VPPOBJ_SetVoutMute_Ext(0, params[0].value.a, params[0].value.b,
                                                         params[1].value.a, params[1].value.b);
        break;


        case VPP_SETGFXINPUTMODE:
        params[1].value.a = MV_VPPOBJ_SetGfxInputMode(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_SETGFXCSC:
        {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            int cscMode = params[0].value.b;

            if (cscMode == 0)// gfx bypass
                params[1].value.a = MV_VPPOBJ_GfxBypass(0, 1);
            else
                params[1].value.a = MV_VPPOBJ_GfxBypass(0, 0);
            if (params[1].value.a != MV_VPP_OK){
                res = TEE_ERROR_BAD_STATE;
                break;
            }
            params[1].value.a = MV_VPPOBJ_SetGfxCsc(0, params[0].value.a, params[0].value.b);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;
        case VPP_HDMISETPHYPOWER:
        params[0].value.b = MV_VPPOBJ_SetHdmiPhyPower(0, params[0].value.a);
        break;

        case VPP_HDMIENABLEAUDIOFMT:
        params[0].value.b = MV_VPPOBJ_EnableHdmiAudioFmt(0, params[0].value.a);
        break;

        case VPP_HDMISET3DSTRUCT:
        params[1].value.a = MV_VPPOBJ_SetHdmi3DStructure(0, params[0].value.a,params[0].value.b);
        break;

        case VPP_HDMIAUDIOFIFORESET:
        params[0].value.b = MV_VPPOBJ_SetHdmiAudioFifoReset(0, params[0].value.a);
        break;

        case VPP_HDMISETAUDIOMUTE:
        params[0].value.b = MV_VPPOBJ_SetHdmiAudioMute(0 ,params[0].value.a);
        break;

        case VPP_HDMISETHDMIMODE:
        params[0].value.b = MV_VPPOBJ_SetHdmiMode(0 ,params[0].value.a);
        break;

        case VPP_HDMISETVIDEOINFO:
        params[1].value.a = MV_VPPOBJ_SetHdmiVideoInfo(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_HDMIENABLE:
        params[1].value.a = MV_VPPOBJ_SetHDMIEnable(0, params[0].value.a, params[0].value.b);
        break;

        case VPP_HDMISTOPAUXPKT:
        params[0].value.b = MV_VPPOBJ_StopHdmiAuxPkt(0 ,(VPP_HDMI_PKT_ID)params[0].value.a);
        break;

        case CEC_CFG_FEATURE_SUPPORT:
        params[0].value.b = MV_VPPOBJ_CecConfigFeatureSupport(0, params[0].value.a);
        break;

        case CEC_GET_FEATURE_SUPPORT:
        params[0].value.b = MV_VPPOBJ_CecGetFeatureSupportConfig(0, (INT *)&params[0].value.a);
        break;

        case CEC_ENABLE:
        params[0].value.b = MV_VPPOBJ_CecEnable(0, params[0].value.a);
        break;

        case CEC_SET_RETRANSIMIT_COUNT:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        params[0].value.b = MV_VPPOBJ_CecSetReTransmitCount(0, params[0].value.a);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

        case CEC_SET_PHY_ADDR:
        params[0].value.b = MV_VPPOBJ_CecSetPhysicalAddr(0, params[0].value.a);
        break;

        case CEC_ALLOC_LOG_ADDR:
        params[1].value.b = MV_VPPOBJ_CecAllocLogicalAddr(0, (BOOL)params[0].value.a, (VPP_CEC_DEVICE_TYPE)params[0].value.b, params[1].value.a);
        break;

        case VPP_SETXVYCCMODE:
#ifdef XVYCC_SUPPORT
        params[0].value.b = MV_VPPOBJ_SetxvYccMode(0, params[0].value.a);
#endif
        break;
       case VPP_SETHDRENABLE:
        params[0].value.b = MV_VPPOBJ_HDREnable(0, params[0].value.a);
        break;

        case VPP_ENABLEHDCP:
        if (VPP_IsAccessFromOutCtrl(commandID)) {
            params[0].value.b = MV_VPPOBJ_EnableHDCP(0, params[0].value.a);
        }
        break;


        case VPP_GETHDCPSTATE:
        params[0].value.b = MV_VPPOBJ_GetHDCPState(0, (INT *)&params[0].value.a);
        break;

        case VPP_GETHDMIPOWERSTATUS:
        params[0].value.b = MV_VPPOBJ_GetHdmiPowerStatus(0, (INT *)&params[0].value.a );
        break;

        case VPP_GETSTATUS:
        params[0].value.b = MV_VPPOBJ_GetVppStatus(0, (INT *)&params[0].value.a );
        break;

        case VPP_GETHDCPVERSION:
        params[0].value.b = MV_VPPOBJ_GetHDCPVersion(0, (INT *)&params[0].value.a );
        break;

        case VPP_SETHDCPVERSION:
        if (VPP_IsAccessFromOutCtrl(commandID)) {
            params[0].value.b = MV_VPPOBJ_SetHDCPVersion(0, params[0].value.a);
        }
        break;

        case VPP_RPTR_DS_SETHDCPVERSION:
        params[0].value.b = MV_VPPOBJ_SetRptrDownstreamHDCPVersion(0, params[0].value.a);
        break;

        case VPP_GETCURHDCPVERSION:
        params[0].value.b = MV_VPPOBJ_GetCurrentHDCPVersion(0, (VPP_HDMI_HDCP_VERSION *)&params[0].value.a );
        break;

        case VPP_GETSINKHDCPCAPABILITY:
        params[0].value.b = MV_VPPOBJ_GetSinkHDCPCapability(0, (VPP_HDMI_HDCP_VERSION *)&params[0].value.a );
        break;

        case VPP_SETHDCPREPEATERMODE:
        params[0].value.b = MV_VPPOBJ_SetHDCPRepeaterMode(0, params[0].value.a);
        break;

        case VPP_GETHDR10PLAYBACKSTATUS:
        params[0].value.b = MV_VPPOBJ_GetHDR10PlaybackStatus(0, (BOOL *)&params[0].value.a );
        break;

        case VPP_GETHPDSTATUS:
        params[0].value.b = MV_VPPOBJ_GetHPDStatus(0, (INT *)&params[0].value.a );
        break;

        case VPP_SETMACROVISION:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

        case VPP_SETVDACPOWER:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

        case CMD_OSAL_HANDLER:
        res = MV_OSAL_Handler(sessionContext, commandID, paramTypes, params);
        break;
        case VPP_HDMICMDHANDLER:
        params[0].value.a = VPP_HDMICmdHandler( 0 ); //FIXME: need to pass handle
        break;
        case VPP_CECHANDLER:
        params[0].value.a = VPP_CECHandler( 0 ); //FIXME: need to pass handle
        break;

        case VPP_REGHDMICALLBACK:
        params[3].value.a = VPP_RegHdmiCallback(0, params[0].value.a,params[1].value.a,params[2].value.a);
        break;
        case VPP_UNREGHDMICALLBACK:
        params[0].value.a = VPP_UnRegHdmiCallback(0 );
        break;

#ifdef EARC_RX_ENABLED
        case VPP_EARC_SETMODE:
        params[0].value.b = MV_VPPOBJ_eARC_SetMode(0 ,params[0].value.a);
        break;

        case VPP_EARC_RESETMODE:
        params[0].value.b = MV_VPPOBJ_eARC_ResetMode(0);
        break;

        case VPP_EARC_AUDIOLATENCY:
        params[1].value.a = MV_VPPOBJ_eARC_AudioLatency(0 ,params[0].value.a);
        break;

        case VPP_EARC_REGCALLBACK:
        params[3].value.a = VPP_RegEarcCallback(0, params[0].value.a,params[1].value.a,params[2].value.a);
        break;

        case VPP_EARC_UNREGCALLBACK:
        params[0].value.a = VPP_UnRegEarcCallback(0);
        break;
#endif /* EARC_RX_ENABLED */

        case VPP_REGHDCPCALLBACK:
        params[3].value.a = VPP_RegHdcpCallback(0, params[0].value.a,params[1].value.a,params[2].value.a);
        break;
        case VPP_UNREGHDCPCALLBACK:
        params[0].value.a = VPP_UnRegHdcpCallback(0 );
        break;
        case VPP_REGHDMICPCB0CALLBACK:
        params[1].value.a = VPP_RegHdmiCpcb0Callback(0, params[0].value.a);
        break;
        case VPP_UNREGHDMICPCB0CALLBACK:
        params[0].value.a = VPP_UnRegHdmiCpcb0Callback(0 );
        break;
        case VPP_REGCECCALLBACK:
        params[3].value.a = VPP_RegCecCallback(0, params[0].value.a,params[1].value.a,params[2].value.a);
        break;
        case VPP_UNREGCECCALLBACK:
        params[0].value.a = VPP_UnRegCecCallback(0 );
        break;

        case VPP_SUSPEND:
        params[0].value.a = MV_VPPOBJ_Suspend(0, params[0].value.b);
        break;
        case VPP_RESUME:
        params[0].value.a = MV_VPPOBJ_Resume(0, params[0].value.b);
        break;

#ifndef REMOVE_BG4_NON_SUPPORTED_INTERFACE
        case VPP_SET4KRES:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
        case VPP_SET4KBYPASSMODE:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
        case VPP_SET4KOVLMODEFROMISR:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

        case VPP_SET4KOVLMODE:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

        case VPP_DDDSETCTRL:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

        case VPP_DDDGETPARAMS:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

        case VPP_DDDSETPARAMS:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

        case VPP_DDDISENABLED:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
#endif

        case VPP_GETSTSCOUNTER:
        params[1].value.a = MV_VPPOBJ_GetSTSCounter(0, &sts_counter);
        params[0].value.a = sts_counter.value_low;
        params[0].value.b = sts_counter.value_high;
        break;
        case VPP_GETSTSRUNNINGCOUNTER:
        params[1].value.a = MV_VPPOBJ_GetSTSRunningCounter(0, &sts_counter);
        params[0].value.a = sts_counter.value_low;
        params[0].value.b = sts_counter.value_high;
        break;

#ifndef REMOVE_BG4_NON_SUPPORTED_INTERFACE
        case VPP_SETCMUPOSITION:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
#endif //REMOVE_BG4_NON_SUPPORTED_INTERFACE

#if defined (HDCP2X_REPEATER)
        case VPP_HDCP2XNOTIFYDEVACTIVE:
        params[0].value.b = VPP_HDCP2XUpstreamActive(params[0].value.a);
        break;

        case VPP_HDCP2XGETDEVSTAT:
        params[3].value.a = VPP_HDCP2XGetStatus(params);
        break;

        case VPP_HDCP2XSTARTAUTH:
        params[0].value.b = VPP_HDCP2XStartAuth(params[0].value.a);
        break;

        case VPP_HDCP2XGETCONNINFO:
        params[1].value.b = VPP_HDCP2XGetConnInfo(params);
        break;

        case VPP_HDCP2XGETDEVID:
        params[0].value.a = VPP_HDCP2XGetRxId(params);
        break;

#endif //HDCP2X_REPEATER_SUPPORT

        case VPP_HDCP2XSENDCSM:
        params[1].value.a = MV_VPPOBJ_HDCPRepUpstreamCSMUpdate(0);
        break;

        case VPP_HDMIHPDTASKHANDLER:
        params[0].value.a = VPP_HDMI_HPDTaskHandler( 0 ); //FIXME: need to pass handle
        break;

        case VPP_BYPASS_ENABLE:
        params[0].value.b = MV_VPPOBJ_EnableBypassMode(0, params[0].value.a);
        break;

        case VPP_DEBUG_CMD:
        if (VPP_IsAccessFromOutCtrl(commandID)) {
            if (params[0].value.a == VPP_DBG_CMD_HDMIDUMPREG) {
                void *shmAddr = (void *)(params[1].value.a);
                Size = sizeof(VPP_HDMITX_REG_DUMP);
                VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
                res = VPP_ValidateInput(&bufDesc, 0);
                if(res != TEE_SUCCESS) {
                    break;
                }
            }
            params[2].value.b  = MV_VPPOBJ_Debug(params[0].value.a, params[0].value.b,
                     (void *)params[1].value.a, (void *)params[1].value.b);
        } else {
                res = TEE_ERROR_NOT_SUPPORTED;
        }
        break;

        case VPP_WATERMARK:
        params[2].value.a = VPP_VMXAPI_VmxTaCommandHandler(params);
        break;

#ifdef CONFIG_AMP_DOLBY_DUMP
        case VPP_DVFRAMEDUMPENABLE:
            if (VPP_IsAccessFromOutCtrl(commandID)) {
                params[0].value.b = MV_VPPOBJ_DV_EnableDolbyDump(0, params[0].value.a);
            }
        break;
#endif //CONFIG_AMP_DOLBY_DUMP

        case VPP_LOWLATENCY_ENABLE:
            params[0].value.b = MV_VPPOBJ_EnableLowLatency(0, params[0].value.a);
        break;

        case VPP_SET_PRIORITY_MODE:
            params[0].value.b = MV_VPPOBJ_SetPriorityMode(0, params[0].value.a);
        break;

        case VPP_SET_DOVI2HDR_MAP:
            params[0].value.b = MV_VPPOBJ_SetDovi2HdrMap(0, params[0].value.a);
        break;
        case VPP_SET_HDR_FEATURE:
            params[0].value.b = MV_VPPOBJ_SetHdrFeature(0, params[0].value.a);
        break;

       case VPP_SETGFXINFO:
        params[1].value.b = MV_VPPOBJ_SetGfxInfo(0, params[0].value.a, \
                params[0].value.b, params[1].value.a);
        break;

        case VPP_SET_HDMI_OUTPUT_MODE:
            params[0].value.b = MV_VPPOBJ_SetHdmiOutputMode(0, params[0].value.a);
        break;

        case VPP_SET_EE_MODE:
            if (VPP_CMU_EE_NEW_LTI_CTI_WITH_CUSTOM_GAIN_CONTROL == params[0].value.a) {
                CmuEeCtrlParam.stVppCmuLtiCtiCustomCtrlParam.u8Ltigain = params[0].value.b;
                CmuEeCtrlParam.stVppCmuLtiCtiCustomCtrlParam.u8Ctigain = params[1].value.a;
            }
            params[1].value.b = MV_VPPOBJ_SetEEMode(0, params[0].value.a, &CmuEeCtrlParam);
        break;

        case VPP_SET_COLOR_TRANSFORMGAIN:
            ClrTempGainParam.Rgain = (float)VPP_OCSC_GAIN_INT2FLOAT(params[0].value.a);
            ClrTempGainParam.Ggain = (float)VPP_OCSC_GAIN_INT2FLOAT(params[0].value.b);
            ClrTempGainParam.Bgain = (float)VPP_OCSC_GAIN_INT2FLOAT(params[1].value.a);
            params[1].value.b = MV_VPPOBJ_SetColorTransformGain(0, &ClrTempGainParam);
        break;

        case VPP_SET_GAMMA_MODE:
            params[1].value.b = MV_VPPOBJ_SetGmMode(0, params[0].value.a, params[0].value.b, \
                                                                            params[1].value.a);
        break;

        case VPP_SETSRM:
        {
            VPP_HDCP_SRM_DATA *pSrmData;
            VPP_DEV_SIGN_DATA *pDevSignData;
            VPP_HDCP_SRM_INFO *pSrmInfo;
            UINT32 offset = 0;
            UINT8 *shmAddr = NULL;
            shmAddr = (UINT8*)params[1].memref.buffer;
            if(shmAddr == NULL || params[1].memref.size == 0)
            {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

            Size = params[1].memref.size;
            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res != TEE_SUCCESS)
                break;

            if(Size <=  (sizeof(VPP_DEV_SIGN_DATA ) + sizeof(VPP_HDCP_SRM_DATA)
                         + sizeof(VPP_HDCP_SRM_INFO)))
            {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

            pSrmInfo = ((VPP_HDCP_SRM_INFO*)shmAddr);
            offset+=sizeof(VPP_HDCP_SRM_INFO);

            pDevSignData =  ((VPP_DEV_SIGN_DATA*)(shmAddr+offset));
            offset+=sizeof(VPP_DEV_SIGN_DATA);

            pSrmData = ((VPP_HDCP_SRM_DATA*)(shmAddr+offset));
            offset+=sizeof(VPP_HDCP_SRM_DATA);
            pSrmData->pBuf = ((UINT8*)(shmAddr+offset));

            if(Size !=  (sizeof(VPP_DEV_SIGN_DATA ) + sizeof(VPP_HDCP_SRM_DATA)
                         + sizeof(VPP_HDCP_SRM_INFO)  + pSrmData->bufLen))
            {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

            params[0].value.a = MV_VPPOBJ_SetSRM (0, pSrmData, pDevSignData, pSrmInfo);
        }
        break;

        case VPP_GETSRMVERSION:
        {
            params[1].value.a = MV_VPPOBJ_GetSRMVersion(0, (UINT16*)&params[0].value.a, (UINT16*)&params[0].value.b);
        }
        break;

        case VPP_ENABLESRMCHECK:
        {
            res = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
            if (res != SUCCESS) {
                VPPAPI_DBG_ERROR("Currently client UUID verification fail 0x%x!\n", res);
                res = TEE_ERROR_NOT_SUPPORTED;
                break;
            } else {
                params[0].value.b = MV_VPPOBJ_EnableSRMCheck(0, params[0].value.a);
            }
        }
        break;

#ifdef CONFIG_AMP_THDR
        case VPP_THDRVITM_PARAMS:
        {
            params[1].value.a = MV_VPPOBJ_SetThdrVitmParams(0, params[0].value.a, params[0].value.b);
        }
        break;
#endif
#ifdef CONFIG_AMP_DCM
        case VPP_SETDCMPQ:
        {
             params[2].value.a = MV_VPPOBJ_SetDcmPictCtrl(0,  params[0].value.a, params[0].value.b,  params[1].value.a, params[1].value.b);
        }
        break;
#endif
        case VPP_GETGCSC:
        {
            params[1].value.b = MV_VPPOBJ_GetGcsc(0, params[0].value.a, params[0].value.b, (UINT32 *)&params[1].value.a);
        }
        break;
        case VPP_GETMEMID:
        {
            params[1].value.b = MV_VPPOBJ_GetMemId(0, params[0].value.a, params[0].value.b, (UINT32 *)&params[1].value.a);
        }
        break;
        case VPP_PUTMEMID:
        {
            params[1].value.b = MV_VPPOBJ_PutMemId(0, params[0].value.a);
        }
        break;

        case VPP_GET_BLOCK_STATUS:
        {
            params[1].value.b = MV_VPPOBJ_GetBlockStatus(0, params[0].value.b, params[0].value.a, (INT *)&params[1].value.a);
        }
        break;

#ifdef ENABLE_OVPSCL
        case OVPS_HANDLEINT:
        params[0].value.b = MV_OVPS_ISR_TaskHandler(params[0].value.a);
        break;
        case OVPS_CREATE:
        params[0].value.b = MV_OVPS_Create();
        break;
        case OVPS_DESTROY:
        params[0].value.b = MV_OVPS_Destroy();
        break;
        case OVPS_OPEN_CLIENT:
        params[0].value.b = MV_OVPS_Open_Client(params[0].value.a);
        break;
        case OVPS_CLOSE_CLIENT:
        params[0].value.b = MV_OVPS_Close_Client(params[0].value.a);
        break;
        case OVPS_REGEVENTCALLBACK:
        params[3].value.a = OVPS_RegEventCallback(0, params[0].value.a,params[1].value.a,params[2].value.a);
        break;
        case OVPS_GET_NUM_OF_REQ:
        {
            params[1].value.b = MV_OVPS_GetNoOfRequestsWaiting(params[0].value.a,(UINT32 *)&params[1].value.a);
        }
        break;
        case OVPS_POP_REQUEST:
        {
            OVPS_CLIENT_REQUEST *pstOvpsRequest = NULL;

            params[0].value.b = MV_OVPS_PopRequest(&pstOvpsRequest);
            params[0].value.a = (UINT32)pstOvpsRequest;
        }
        break;
        case OVPS_REL_REQUEST:
        {
            OVPS_CLIENT_REQUEST *pstOvpsRequest = NULL;

            params[1].value.b = MV_OVPS_ReleaseRequest(params[0].value.a,&pstOvpsRequest);
            params[1].value.a = (UINT32)pstOvpsRequest;
        }
        break;
        case OVPS_PUSH_FRM:
        {
            OVPS_CLIENT_REQUEST *pstOvpsRequest;
            unsigned int Size;
            UINT8 *shmAddr;

            shmAddr = (UINT8 *)params[1].memref.buffer;
            Size = params[1].memref.size;
            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res != TEE_SUCCESS)
                break;

            pstOvpsRequest = ( OVPS_CLIENT_REQUEST *)shmAddr;

            shmAddr = (UINT8 *)params[2].memref.buffer;
            Size = params[2].memref.size;
            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res != TEE_SUCCESS)
                break;

            if(pstOvpsRequest)
            {
                VBUF_INFO *pVBufInfo;
                pstOvpsRequest->pinfo = (void*)shmAddr;
                pVBufInfo = (void*)shmAddr;

                shmAddr = (UINT8 *)params[3].memref.buffer;
                Size = params[3].memref.size;
                VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
                res = VPP_ValidateInput(&bufDesc, 0);
                if(res != TEE_SUCCESS)
                    break;

                pVBufInfo->m_OvpSclFrameDesc = (ARCH_PTR_TYPE)shmAddr;
                params[0].value.b = MV_OVPS_PushRequest(params[0].value.a,pstOvpsRequest);
            }
        }
        break;
#endif
        default:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
    }
    return res;
}

TA_DEFINE_BEGIN
    .uuid              = TAVPP_UUID,
    .name              = "VPP",
    .singleInstance    = false,
    .multiSession      = false,
    .instanceKeepAlive = false,
TA_DEFINE_END

