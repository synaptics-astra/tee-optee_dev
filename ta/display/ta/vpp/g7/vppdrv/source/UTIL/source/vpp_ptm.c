/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifdef CONFIG_VPP_PTM_REF_UNREF_ENABLE

#include "vpp_module.h"
#include "vpp_debug_util.h"
#include "vpp_ptm.h"
#include "vdec_com.h"
#include "ptm_cmd.h"
#include "tee_internal_api.h"
#include "vpp_intr_monitor.h"
#include "vpp_intr_util.h"

static TEE_Result VPP_PtmSessionOpen(VPP_PTM_OBJ * ptm_obj)
{
    TEE_Result res = TEE_SUCCESS;
    const TEE_UUID uuid = TA_PTM_UUID;
    uint32_t  cancellationTimeout = 0;
    uint32_t  core_paramTypes;
    TEE_Param core_params[4] = {{.value.a = 0, .value.b = 0}};
    uint32_t  returnOrigin;

    if(ptm_obj == NULL)
        return TEE_ERROR_GENERIC;

    /*Open Session of PTM TA*/
    if (!ptm_obj->ptmSession) {
        core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                 TEE_PARAM_TYPE_NONE,
                                 TEE_PARAM_TYPE_NONE,
                                 TEE_PARAM_TYPE_NONE);
        res = TEE_OpenTASession(&uuid,
                                cancellationTimeout,
                                core_paramTypes,
                                core_params,
                                &ptm_obj->ptmSession,
                                &returnOrigin);
        VPPAPI_DBG_INFO("open PTM session. result=0x%x, origin=0x%x, session=0x%x\n",
              res, returnOrigin, ptm_obj->ptmSession);
        if (res != TEE_SUCCESS) {
            VPPAPI_DBG_ERROR("fail to open session. result=0x%08x, origin=%d, session=%d\n",
                                   res, returnOrigin, ptm_obj->ptmSession);
            return TEE_ERROR_NOT_SUPPORTED;
        }
    }

    return res;
}

static TEE_Result VPP_PtmSessionClose(VPP_PTM_OBJ * ptm_obj)
{
    if(ptm_obj == NULL)
        return TEE_ERROR_GENERIC;

    if (ptm_obj->ptmSession) {
        //Reset AVIO to avoid any spurious interrupt during TA session closure
        VPP_AVIO_Reset();
        TEE_CloseTASession(ptm_obj->ptmSession);
        ptm_obj->ptmSession = NULL;
    }

    return TEE_SUCCESS;
}

void VPP_PtmHandleSessionIncrement(void)
{
    if(g_vpp != NULL)
    {
        g_vpp->ptm_obj.session_count++;
    }
}

void VPP_PtmHandleSessionDecrement(void)
{

    if(g_vpp == NULL)
        return;

    g_vpp->ptm_obj.session_count--;


    if(g_vpp->ptm_obj.session_count == 0)
    {
        VPP_PtmDeinit(&g_vpp->ptm_obj);
    }
}

static TEE_Result VPP_PtmTaInvokeCommand(VPP_PTM_OBJ * ptm_obj, int ptmCmd, UINT32 addr)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t        cancellationTimeout = 0;
    uint32_t        core_paramTypes;
    TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};
    uint32_t        returnOrigin;

    if (!VPP_DEBUG_UTIL_IS_FF_ENABLED_PTMTA(g_vpp->vpp_objs[0])) {
        return res;
    }

    if(ptm_obj == NULL)
        return TEE_ERROR_GENERIC;

    if (ptm_obj->ptmSession) {
        int unitNum = VPPDRV_PROF_MONITOR_UINT1_PTM_TA_PROF;
        VPP_OBJ *vpp_obj = g_vpp->vpp_objs[0];
        TEE_Result prof_res;
        VPPDRV_vppProfMonitorGen(vpp_obj, unitNum, VPPDRV_INTR_MONITOR_START, VPPDRV_INTR_MONITOR_NOSHOW);

        core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                 TEE_PARAM_TYPE_NONE,
                                 TEE_PARAM_TYPE_NONE,
                                 TEE_PARAM_TYPE_NONE);

        core_params[0].value.a = addr;
        res = TEE_InvokeTACommand(ptm_obj->ptmSession,
                                  cancellationTimeout,
                                  ptmCmd,
                                  core_paramTypes,
                                  core_params,
                                  &returnOrigin);
        if (res != TEE_SUCCESS) {
            VPPAPI_DBG_ERROR("PTM command[%d] fail result=0x%08x, session=0x%x, addr=0x%x\n",
                                   ptmCmd, res, ptm_obj->ptmSession, addr);
        }

        prof_res = VPPDRV_vppProfMonitorGen(vpp_obj, unitNum, VPPDRV_INTR_MONITOR_STOP, VPPDRV_INTR_MONITOR_NOSHOW);
        if (prof_res) {
            VPPAPI_DBG_ERROR("%s:%d:VPPDBG:TAVPP:%s unit-%d:%x\n",
                __func__, __LINE__, (ptmCmd == PTM_REF_PT) ? "R" : "U", unitNum, addr);
        }
    }

    return res;
}

TEE_Result VPP_PtmRef(VPP_PTM_OBJ * ptm_obj, int planeID, VDEC_VID_BUF_DESC *frame)
{
    TEE_Result res = TEE_SUCCESS;
    VBUF_INFO *pinfo=NULL;

    if(frame == NULL)
    {
        return res;
    }

    if(ptm_obj == NULL)
    {
        return TEE_ERROR_GENERIC;
    }

#ifndef CONFIG_AMP_TEE_ENABLE
    pinfo = (VBUF_INFO*) frame->user_data;
#else
    pinfo = (VBUF_INFO *)frame;
#endif

    if(!IS_MMU_ENABLED(pinfo->m_is_compressed)){
        return res;
    }

    res = VPP_PtmTaInvokeCommand(ptm_obj, PTM_REF_PT, (UINT32)pinfo->m_pbuf_start);
    if (res != TEE_SUCCESS) {
        VPPAPI_DBG_ERROR("ta_ref FAILED. result=0x%08x, session=0x%x, addr=0x%x\n",
                               res, ptm_obj->ptmSession, (UINT32)pinfo->m_pbuf_start);
    } else {
        ptm_obj->referencedAddr[planeID] = (UINT32)pinfo->m_pbuf_start;
        //VPPAPI_DBG_INFO("ta_ref success addr=0x%x\n",(UINT32)pinfo->m_pbuf_start);
    }

    return res;
}

TEE_Result VPP_PtmUnref(VPP_PTM_OBJ * ptm_obj, int planeID, VDEC_VID_BUF_DESC *frame)
{
    TEE_Result res = TEE_SUCCESS;
    VBUF_INFO *pinfo=NULL;

    if(frame == NULL)
    {
        return res;
    }

    if(ptm_obj == NULL)
    {
        return TEE_ERROR_GENERIC;
    }

#ifndef CONFIG_AMP_TEE_ENABLE
    pinfo = (VBUF_INFO*) frame->user_data;
#else
    pinfo = (VBUF_INFO *)frame;
#endif

    if(!IS_MMU_ENABLED(pinfo->m_is_compressed)){
        return res;
    }

    res = VPP_PtmTaInvokeCommand(ptm_obj, PTM_UNREF_PT, (UINT32)pinfo->m_pbuf_start);
    if (res != TEE_SUCCESS) {
        VPPAPI_DBG_ERROR("ta_unref FAILED. result=0x%08x, session=0x%x, addr=0x%x\n",
                               res, ptm_obj->ptmSession, (UINT32)pinfo->m_pbuf_start);
    } else {
        ptm_obj->referencedAddr[planeID] = 0;
        //VPPAPI_DBG_INFO("ta_unref success addr=0x%x\n",(UINT32)pinfo->m_pbuf_start);
    }

    return res;
}

TEE_Result VPP_PtmUpdateRef(VPP_PTM_OBJ * ptm_obj, int PlaneID, VDEC_VID_BUF_DESC *frame_new, VDEC_VID_BUF_DESC *frame_prev)
{
    TEE_Result res = TEE_SUCCESS;

    res = VPP_PtmUnref(ptm_obj, PlaneID, frame_prev);
    res |= VPP_PtmRef(ptm_obj, PlaneID, frame_new);

    return res;
}

static TEE_Result VPP_PtmUnrefAll(VPP_PTM_OBJ * ptm_obj)
{
    int i = 0;

    if(ptm_obj == NULL)
    {
        return TEE_ERROR_GENERIC;
    }

    for(i=0; i<MAX_NUM_PLANES; i++)
    {
        //Release any previous frame to be release in next VBI
        VBUF_INFO *pinfo;
        PLANE *plane = &g_vpp->vpp_objs[0]->plane[i];

        if ((plane->status == STATUS_DISP_VIDEO) && plane->prev_curr_frame) {
            pinfo = (VBUF_INFO *)plane->prev_curr_frame;
        } else if ((plane->status == STATUS_DISP_LOGO) && plane->prev_still_picture) {
            pinfo = (VBUF_INFO *)plane->prev_still_picture;
        } else {
            pinfo = NULL;
        }

        if (pinfo) {
            VPP_PtmTaInvokeCommand(ptm_obj, PTM_UNREF_PT, (UINT32)pinfo->m_pbuf_start);
        }

        if(ptm_obj->referencedAddr[i] != 0)
        {
            VPP_PtmTaInvokeCommand(ptm_obj, PTM_UNREF_PT, ptm_obj->referencedAddr[i]);
            ptm_obj->referencedAddr[i] = 0;
        }
    }

    return TEE_SUCCESS;
}

TEE_Result VPP_PtmInit(VPP_PTM_OBJ * ptm_obj)
{
    TEE_Result res = TEE_SUCCESS;
    int i = 0;

    if(ptm_obj == NULL)
        return TEE_ERROR_GENERIC;

    for(i=0; i<MAX_NUM_PLANES; i++)
    {
        ptm_obj->referencedAddr[i] = 0;
    }
    ptm_obj->session_count = 0;
    ptm_obj->ptmSession=NULL;

    res = VPP_PtmSessionOpen(ptm_obj);

    return res;
}

void VPP_PtmDeinit(VPP_PTM_OBJ * ptm_obj)
{
    if(!ptm_obj)
        return;

    VPP_PtmUnrefAll(ptm_obj);
    VPP_PtmSessionClose(ptm_obj);
}
#endif
