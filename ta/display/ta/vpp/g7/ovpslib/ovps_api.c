/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _OVPS_API_C_
#include "Galois_memmap.h"
#include "galois_io.h"
#include "global.h"
#include "vbuf.h"
#include "vpp_module.h"
#include "vpp_apifuncs.h"
#include "vpp_cfg.h"
#include "vpp_fe.h"
#include "vpp_scl.h"
#include "vpp_csc.h"
#include "vpp_be.h"
#include "vpp_cpcb.h"
#include "vpp_cpcb_ovl.h"
#include "vpp_be.h"
#include "vpp_be_prv.h"
#include "vpp_be_hdcp.h"
#include "vpp_be_hdmitx_prv.h"
#include "vpp_gamma.h"

#ifdef CONFIG_AMP_DCM
#include "vpp_mp.h"
#endif

#ifndef CONFIG_AMP_TEE_ENABLE
#include "OSAL_api.h"
#else
#include "tee_ta_osal.h"
#endif

#if defined(VPP_DIAGS)
#include "pllDiag.h"
#else
#include "avpll.h"
#endif //VPP_DIAGS

#include "vpp.h"
#include "vpp_isr.h"
#include "api_avio_dhub.h"
#include "vpp_clkGate.h"
#include "ovps_api.h"
#include "vpp_cmd.h"
#include "ovps_apifuncs.h"
#include "ovps_bcmbuf.h"
#include "ovps_isr.h"

#ifdef VPP_DIAGS
VPP * g_vpp;
#endif

#define OVPS_INTR_TIMEOUT (40)
#define OVPS_BCM_BUF_COUNT (2)
#define OVPS_SKIP_UPDATE_FE_FLAG (1)
#define OVPS_SUSPEND_TIMEOUT_MS 1000

extern INT OVPS_EOF_Timeout(PVOID);

#define OVPS_UPDATE_FRAME_DESCRIPTORS(client_req) ( { \
                    VBUF_INFO *pinfo, *pinfo_ovp; \
                        pinfo = (VBUF_INFO *)client_req->pinfo; \
                        pinfo_ovp = (VBUF_INFO *)pinfo->m_OvpSclFrameDesc; \
                        pinfo_ovp = (VBUF_INFO *)pinfo_ovp->m_hDesc; \
                        pinfo->m_OvpSclFrameDesc = (ARCH_PTR_TYPE)pinfo_ovp; \
                        client_req->pinfo = (VBUF_INFO *)pinfo->m_hDesc;\
})

/*********************************************************
 * FUNCTION:  OVPS Update Descriptors
 * PARAMS: OVPS_CLIENT_REQUEST *,
 *         OVPS_CLIENT_REQUEST **
 * RETURN: NONE
 *
 *********************************************************/
static void MV_OVPS_UpdateDescriptors(OVPS_CLIENT_REQUEST *client_req,
                                OVPS_CLIENT_REQUEST **pstOvpsRequest)
{

#ifndef CONFIG_AMP_TEE_ENABLE
    *pstOvpsRequest = client_req;
#else
     OVPS_UPDATE_FRAME_DESCRIPTORS(client_req);
    *pstOvpsRequest = (OVPS_CLIENT_REQUEST *)client_req->pClientDes; //Virtual descriptor
#endif

}
/*********************************************************
 * FUNCTION: Client Id is valid and inactive
 * PARAMS: clientId
 * RETURN: 1 - clientId is valid and inactive
 *         MV_OVPS_ECLIENTBUSY - clientId is active
 *         MV_OVPS_EINVALIDCLIENTID - clientId is invalid
 *********************************************************/
static OVPS_CLIENT_ERROR Get_OVPS_ClientState(int m_uchClientId)
{
    OVPS_CLIENT_ERROR ret_val = MV_OVPS_EINVALIDCLIENTID;
    OVPS_OVPOBJ_INIT();

    if ((m_uchClientId >= 0) && (m_uchClientId < MAX_VPP_OVP_OBJ)) {
        // m_uchClientId is a valid Client Id
        if (ovp_obj->client_obj[m_uchClientId].client_state == OVPS_CLIENT_INACTIVE) {
            ret_val = MV_OVPS_ECLIENTNOTACTIVE;
        } else {
            ret_val = MV_OVPS_ECLIENTBUSY;
        }
    }
    return ret_val;
}

static int MV_OVPS_Reset(void)
{
    UINT32 RegAddr;

    OVPS_VPPOVPSOBJ_INIT();

    ovp_obj->intr_en = 0;
#ifdef CONFIG_OVP_WITH_VPPLIB
    ovp_obj->m_vpp_sched_request = 1; /*Do OVPS init through VPP BCM*/
    ovp_obj->m_BCM_Sched = BCM_SCHED_Q0;
#else
    ovp_obj->m_BCM_Sched = BCM_SCHED_Q12;
    ovp_obj->m_vpp_sched_request = 0; /*Do OVPS init through VPP BCM*/
#endif

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP;
    //Update Shadow
    GA_REG_WORD32_READ(RegAddr + RA_OVPSCLWRAP_CTRL, &OVPS_PARAMS->ovpsclwrap_ctrl.u32);
    GA_REG_WORD32_READ(RegAddr + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_CTRL, &OVPS_PARAMS->ovp_y_scl_ctrl.u32);
    GA_REG_WORD32_READ(RegAddr + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_CTRL, &OVPS_PARAMS->ovp_uv_scl_ctrl.u32);
    GA_REG_WORD32_READ(RegAddr + RA_OVPSCLWRAP_CTRL1, &OVPS_PARAMS->ovpsclwrap_ctrl1.u32);
    GA_REG_WORD32_READ(RegAddr + RA_OVPSCLWRAP_CTRL0, &OVPS_PARAMS->ovpsclwrap_ctrl0.u32);
    GA_REG_WORD32_READ(RegAddr + RA_OVPSCLWRAP_CTRL2, &OVPS_PARAMS->ovpsclwrap_ctrl2.u32);

    GA_REG_WORD32_WRITE(RegAddr + RA_OVPSCLWRAP_tg_ctrl, 0x800);
    OVPS_PARAMS->ovpsclwrap_ctrl1.uCTRL1_ovp_flush_sel = 1;
    GA_REG_WORD32_WRITE(RegAddr + RA_OVPSCLWRAP_CTRL1, OVPS_PARAMS->ovpsclwrap_ctrl1.u32);

    OVPS_PARAMS->ovpsclwrap_ctrl1.uCTRL1_ovp_flush_sel = 0;
    GA_REG_WORD32_WRITE(RegAddr + RA_OVPSCLWRAP_CTRL1, OVPS_PARAMS->ovpsclwrap_ctrl1.u32);

    OVPS_NGPTV_EnableBypass(vpp_obj,1);
    return MV_VPP_OK;
}

int MV_OVPS_Create(void)
{
    INT32 i;
    INT32 ret = MV_VPP_OK;
    OVPS_CLIENT_OBJ *client_obj = NULL;
#ifdef VPP_DIAGS
    g_vpp =  &vpp;
#endif
    OVPS_VPPOVPSOBJ_INIT();

    if (ovp_obj->init_success)
        return MV_VPP_OK;

    for (i = 0; i < MAX_VPP_OVP_OBJ; i++) {
        client_obj = &vpp_obj->ovp_obj.client_obj[i];
        client_obj->client_state = OVPS_CLIENT_INACTIVE;
        client_obj->rqstCount = 0;
        client_obj->outputQCount = 0;
    }

    if (MV_OSAL_Mutex_Create(&(ovp_obj->client_obj_lock))) {
        return (MV_VPP_ENOMEM);
    }
    if (MV_OSAL_Mutex_Create(&(ovp_obj->ovps_isr_mutex_handle))) {
        return (MV_VPP_ENOMEM);
    }
    if (MV_OSAL_Sem_Create(&(ovp_obj->ovps_sem_handle), 0)) {
        return (MV_VPP_ENOMEM);
    }

    ovp_obj->callback = NULL;
    ovp_obj->in_progress = 0;
    for (i = 0; i < OVPS_BCM_BUF_COUNT; i++) {
        if (OVPS_BCMBUF_Create(&(ovp_obj->ovpscl_bcm_buf[i]), BCM_BUFFER_SIZE) != MV_VPP_OK) {
            ret = (MV_VPP_ENOMEM);
            goto OVPS_DESTROY;
        }

        if (OVPS_BCMBUF_Create(&(ovp_obj->ovpscl_clear_bcm_buf[i]), BCM_BUFFER_SIZE) != MV_VPP_OK) {
            ret = (MV_VPP_ENOMEM);
            goto OVPS_DESTROY;
        }

        OVPS_BCMBUF_Reset(&ovp_obj->ovpscl_bcm_buf[i]);
        OVPS_BCMBUF_Reset(&ovp_obj->ovpscl_clear_bcm_buf[i]);

        if (VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, DMA_CMD_BUFFER_SIZE, \
                    &ovp_obj->ovpscl_dma_cfgQ[i].handle) != SUCCESS) {
            ret = (MV_VPP_ENOMEM);
            goto OVPS_DESTROY;
        }
        ovp_obj->ovpscl_dma_cfgQ[i].len = 0;
        ovp_obj->ovpscl_dma_cfgQ[i].addr =
            VPP_SHM_GetVirtualAddress(ovp_obj->ovpscl_dma_cfgQ[i].handle, 0);
        ovp_obj->ovpscl_dma_cfgQ[i].phy_addr =
            VPP_SHM_GetPhysicalAddress(ovp_obj->ovpscl_dma_cfgQ[i].handle, 0);

        /* allocate VBI dhub config queue*/
        if (VPP_SHM32_MALLOC(VPP_SHM_NONSECURE,DMA_CMD_BUFFER_SIZE, \
                    &ovp_obj->ovpscl_bcm_cfgQ[i].handle) != SUCCESS) {
            ret = (MV_VPP_ENOMEM);
            goto OVPS_DESTROY;
        }
        ovp_obj->ovpscl_bcm_cfgQ[i].len = 0;
        ovp_obj->ovpscl_bcm_cfgQ[i].addr =
            VPP_SHM_GetVirtualAddress(ovp_obj->ovpscl_bcm_cfgQ[i].handle, 0);
        ovp_obj->ovpscl_bcm_cfgQ[i].phy_addr =
            VPP_SHM_GetPhysicalAddress(ovp_obj->ovpscl_bcm_cfgQ[i].handle, 0);
    }
    ovp_obj->pOvpSclBcmBuf = &ovp_obj->ovpscl_bcm_buf[0];
    ovp_obj->pOvpSclClearBcmBuf = &ovp_obj->ovpscl_clear_bcm_buf[0];
    ovp_obj->curr_ovpscl_dma_cfgQ = &(ovp_obj->ovpscl_dma_cfgQ[0]);
    ovp_obj->curr_ovpscl_bcm_cfgQ = &(ovp_obj->ovpscl_bcm_cfgQ[0]);

    if (MV_OSAL_Mutex_Create(&(ovp_obj->ovps_lock))) {
        ret = (MV_VPP_ENOMEM);
        goto OVPS_DESTROY;
    }

    if (MV_OSAL_Mutex_Create(&(ovp_obj->ovps_inputQ_lock))) {
        ret = (MV_VPP_ENOMEM);
        goto OVPS_DESTROY;
    }

    if (MV_Timer_Create(&(ovp_obj->ovps_monitor), OVPS_INTR_TIMEOUT,
                (PVOID)OVPS_EOF_Timeout, (PVOID)(g_vpp)) != S_OK)
    {
        ret = (MV_VPP_ENOMEM);
        goto OVPS_DESTROY;
    }

    /* reset planes */
    ovp_obj->plane.status = STATUS_INACTIVE;
    ovp_obj->plane.curr_frame = NULL;
    ovp_obj->plane.prev_curr_frame = NULL;
    ovp_obj->plane.curr_still_picture = NULL;
    ovp_obj->plane.still_picture = NULL;
    ovp_obj->plane.srcfmt = -1; // invalid
    ovp_obj->plane.order = -1; // invalid
    ovp_obj->plane.bits_per_pixel = -1; // invalid

    ovp_obj->plane.frm_count = 0;
    ovp_obj->plane.pts.hi_word = 0;
    ovp_obj->plane.pts.lo_word = 0;
    ovp_obj->plane.delta_pts.hi_word = 0;
    ovp_obj->plane.delta_pts.lo_word = 0;

    ovp_obj->plane.actv_win.x = 0;
    ovp_obj->plane.actv_win.y = 0;
    ovp_obj->plane.actv_win.width = 0;
    ovp_obj->plane.actv_win.height = 0;

    ovp_obj->plane.frame_type = -1; // invalid
    ovp_obj->plane.field = 0; // top field
    ovp_obj->plane.vp_buff_w_index = -1;
    ovp_obj->plane.vp_buff_r0_index = -1;
    ovp_obj->plane.vp_buff_r1_index = -1;
    ovp_obj->plane.vp_buff_num = 0;

    ovp_obj->plane.deint_mode = -1; // invalid mode
    ovp_obj->plane.DeintPreset.ViMode = VPP_DEINTMODE_SAFE;
    ovp_obj->plane.DeintPreset.ChrMotVidMode = VPP_DEINTCHROMA_MOT_MAX_OF_CHROMA_AND_LUMA_MOT;
    ovp_obj->plane.DeintPreset.ChrMotFlmMode = TRUE;
    ovp_obj->plane.vp_updated = 0;

    ovp_obj->plane.ref_win.x = 0;
    ovp_obj->plane.ref_win.y = 0;
    ovp_obj->plane.ref_win.width = 0;
    ovp_obj->plane.ref_win.height = 0;
    ovp_obj->plane.ref_win_updated = 0;

    ovp_obj->plane.crop_win_valid = 0;
    ovp_obj->plane.crop_win.x = 0;
    ovp_obj->plane.crop_win.y = 0;
    ovp_obj->plane.crop_win.width = 0;
    ovp_obj->plane.crop_win.height = 0;

    ovp_obj->plane.skip_frame = 0;

    ovp_obj->plane.mute = 0; // un-mute

    ovp_obj->plane.m_leftpadadjust_y = 0;
    ovp_obj->plane.m_leftpad_y = 0;
    ovp_obj->plane.m_rightpad_y = 0;
    ovp_obj->plane.m_toppad_y = 0;
    ovp_obj->plane.m_toppadadjust_y = 0;
    ovp_obj->plane.m_bottompad_y = 0;

    ovp_obj->plane.m_leftpad_c = 0;
    ovp_obj->plane.m_leftpadadjust_c = 0;
    ovp_obj->plane.m_rightpad_c = 0;
    ovp_obj->plane.m_toppad_c = 0;
    ovp_obj->plane.m_toppadadjust_c = 0;
    ovp_obj->plane.m_bottompad_c = 0;
    ovp_obj->plane.m_PicHeightAdjusted = 0;
    ovp_obj->plane.offline_scl_bpp = 24;
    ovp_obj->plane.offline_scl_srcfmt = SRCFMT_YUV422;

    ovp_obj->chan.status = STATUS_INACTIVE;
    ovp_obj->chan.dvID = -1; // invalid
    ovp_obj->chan.dvlayerID = -1; // invalid
    ovp_obj->chan.zorder = -1; // invalid

    ovp_obj->chan.num_of_planes = 0;

    ovp_obj->chan.scl_nl_enable = 0;
    ovp_obj->chan.scl_nl_center = 0x80;
    ovp_obj->chan.scl_nl_cratio = 0x40;

    ovp_obj->chan.disp_win.x = 0;
    ovp_obj->chan.disp_win.y = 0;
    ovp_obj->chan.disp_win.width = 0;
    ovp_obj->chan.disp_win.height = 0;
    ovp_obj->chan.disp_win_updated = 0;
    ovp_obj->chan.disp_win_attr.bgcolor = 0;
    ovp_obj->chan.disp_win_attr.alpha = 0;

    ovp_obj->plane.chanID =  CHAN_OVP_EL;
    ovp_obj->chan.dvID = CPCB_1;

    /* set plane IDs which belong to this channel */
    ovp_obj->chan.num_of_planes = 1;
    ovp_obj->chan.planeID[0] = PLANE_OVP_EL;
    ovp_obj->chan.dvlayerID = CPCB1_PLANE_5;

    OVPS_PARAMS->ovp_y_dmaWID = avioDhubChMap_vpp128b_OVPSCL_W0;
    OVPS_PARAMS->ovp_uv_dmaWID = avioDhubChMap_vpp128b_OVPSCL_W1;
    OVPS_PARAMS->ovp_y_dmaWdhubID = (int)(&VPP_dhubHandle);
    OVPS_PARAMS->ovp_uv_dmaWdhubID = (int)(&VPP_dhubHandle);
    OVPS_PARAMS->readTiledfromOVPclient = 0;

    ovp_obj->plane.dmaRdhubID = (int *)(&VPP_dhubHandle);
    vpp_obj->ovp_obj.uiAviomtrKickoffOvps = 0;

    MV_OVPS_Reset();
    ovp_obj->init_success = 1;

    ovps_printf(OVPS_HIGH, "Success\n");
    return ret;

OVPS_DESTROY:
    MV_OVPS_Destroy();
    ovps_printf(OVPS_ERR, "Failed with error %x\n", ret);
    return ret;
}

/*****************************************************************
 * FUNCTION: Open/activate the client
 * PARAMS: clientId
 * RETURN: MV_OVPS_EINVALIDCLIENTID - invalid clientId
 *         MV_OVPS_ENOFREECLIENT - All clients are active/busy
 *         MV_OVPS_ECLIENTBUSY - clientId already in use
 *         clientId - Opened/Activated clientId
 *****************************************************************/
INT MV_OVPS_Open_Client(INT m_uchClientId)
{
    OVPS_CLIENT_OBJ *client_obj = NULL;
    int ret_val;
    OVPS_VPPOVPSOBJ_INIT();

    ret_val = Get_OVPS_ClientState(m_uchClientId);

    if (ret_val == MV_OVPS_ECLIENTNOTACTIVE) {
        client_obj = &ovp_obj->client_obj[m_uchClientId];
        client_obj->client_state = OVPS_CLIENT_ACTIVE;
        ovps_printf(OVPS_HIGH, "Opening ClientId = %d is Successfull\n", m_uchClientId);
        ret_val = m_uchClientId;
    }
#ifdef OVPS_ENABLE_AUTO_OPEN /*Allow auto open*/
    //if m_uchClientId == -1, then Open/Validate one of the Free clients
    else if (m_uchClientId == OVPS_AUTO_OPEN_ID) {
        int i;
        ret_val = MV_OVPS_ENOFREECLIENT;
        for ( i = 0; i < MAX_VPP_OVP_OBJ; i++) {  //Search for a Free Client Id
            client_obj = &ovp_obj->client_obj[i];
            if (client_obj->client_state == OVPS_CLIENT_INACTIVE) {
                client_obj->client_state = OVPS_CLIENT_ACTIVE;
                ovps_printf(OVPS_HIGH, "ClientId = %d is Opened Successfully\n", i);
                ret_val = i; //return the client id, when a Free Client is found.
                break;
            }
        }
        if (ret_val == MV_OVPS_ENOFREECLIENT) {
            //Control reaches here, only if there is no Free Client found
            ovps_printf(OVPS_ERR, "No Free Client exist\n");
        }
    }
#else
    else {
        ovps_printf(OVPS_ERR, "Opening ClientId = %d is Unsuccessful reason: %d\n", \
                m_uchClientId, ret_val);
    }
#endif
    return ret_val;
}

/*****************************************************************
 * FUNCTION: Close/De-activate the client
 * PARAMS: clientId
 * RETURN: MV_OVPS_EINVALIDCLIENTID - invalid clientId
 *         MV_OVPS_ECLIENTNOTACTIVE - clientId is not busy
 0 - clientId closed successfully
 *****************************************************************/
INT MV_OVPS_Close_Client(INT m_uchClientId)
{
    int ret_val;
    OVPS_CLIENT_OBJ *client_obj = NULL;
    OVPS_VPPOVPSOBJ_INIT();

    ret_val = Get_OVPS_ClientState(m_uchClientId);

    if (ret_val != MV_OVPS_ECLIENTBUSY) {
        ovps_printf(OVPS_ERR, "ClientId = %d status: %d \n", m_uchClientId, ret_val);
    } else {
        client_obj = &ovp_obj->client_obj[m_uchClientId];
        if ((client_obj->rqstCount == 0) && (client_obj->outputQCount == 0)) {
            client_obj->client_state = OVPS_CLIENT_INACTIVE;
            ovps_printf(OVPS_HIGH, "ClientId = %d is Closed\n", m_uchClientId);
            ret_val = 0;
        } else {
            ovps_printf(OVPS_ERR, "ClientId = %d is Busy\n", m_uchClientId);
        }
    }
    return ret_val;
}

INT MV_OVPS_PushRequest(INT m_uchClientId, OVPS_CLIENT_REQUEST *pstOvpsRequest)
{
    int ret_val;
    VBUF_INFO *out_vbuf;
    OVPS_VPPOVPSOBJ_INIT();

    if (!pstOvpsRequest) {
        return MV_VPP_EBADPARAM;
    }

    if (vpp_obj->status == STATUS_INACTIVE || ovp_obj->init_success == 0) {
        ovps_printf(OVPS_ERR,"Invalid status\n");
        return (MV_VPP_EUNCONFIG);
    }

    if (m_uchClientId != pstOvpsRequest->m_uchClientId) {
        ovps_printf(OVPS_ERR, "ClientId = %d mismatch with the Request Client id = %d\n",
                m_uchClientId, pstOvpsRequest->m_uchClientId);
        return (MV_OVPS_ECLIENTIDMISMATCH);
    }

    MV_OSAL_Mutex_Lock(ovp_obj->ovps_isr_mutex_handle);
    out_vbuf = (VBUF_INFO*)((VBUF_INFO *)pstOvpsRequest->pinfo)->m_OvpSclFrameDesc;
    if (ovp_obj->suspend) {
        out_vbuf->m_flags |= VINFO_FLAG_MASK_DATA_ERR;
        MV_OSAL_Mutex_Lock(ovp_obj->client_obj_lock);
        if (frmq_push(&(vpp_obj->ovp_obj.plane.outputq), pstOvpsRequest)) {
            ovp_obj->client_obj[pstOvpsRequest->m_uchClientId].outputQCount++;
        } else {
            ovps_printf(OVPS_ERR, "OutputQ Full\n");
        }
        MV_OSAL_Mutex_Unlock(ovp_obj->client_obj_lock);
        MV_OSAL_Mutex_Unlock(ovp_obj->ovps_isr_mutex_handle);
        return MV_VPP_OK;
    }

    ret_val = Get_OVPS_ClientState(m_uchClientId);
    if (ret_val != MV_OVPS_ECLIENTBUSY) {
        ovps_printf(OVPS_ERR, "push request for client Id = %d failed, reason: %d\n", \
                m_uchClientId, ret_val);
        goto OVPS_PUSHREQ;
    } else {
        /* push frame into plane frame queue. */
        ret_val = frmq_push(&(ovp_obj->plane.inputq), pstOvpsRequest);
        if (!ret_val) {
            ovps_printf(OVPS_ERR,"Input frmq full\n");
            ret_val = MV_VPP_EFRAMEQFULL;
            goto OVPS_PUSHREQ;
        }
    }
    ret_val = MV_VPP_OK;

OVPS_PUSHREQ:
    MV_OSAL_Mutex_Unlock(ovp_obj->ovps_isr_mutex_handle);
    if (MV_VPP_OK == ret_val) {
        MV_OSAL_Mutex_Lock(ovp_obj->client_obj_lock);
        ovp_obj->client_obj[m_uchClientId].rqstCount++;
        MV_OSAL_Mutex_Unlock(ovp_obj->client_obj_lock);

        if (!ovp_obj->in_progress) {
            //Notify Task about new frame
            OVPS_NotifyEvent(ovp_obj, ovp_obj->eventCode, MV_OVPS_EVENT_NEW_FRAME);
        }
    } else {
        OVPS_UPDATE_FRAME_DESCRIPTORS(pstOvpsRequest);
    }
    return ret_val;
}

INT MV_OVPS_ISR_TaskHandler(UINT32 uieventSts)
{
    OVPS_VPPOVPSOBJ_INIT();

    if (vpp_obj->status != STATUS_ACTIVE)
        return 0;

    if (!ovp_obj->init_success)
        return 0;

    ovps_printf(OVPS_LOW,"Event: %d\n", uieventSts);

    MV_OSAL_Mutex_Lock(ovp_obj->ovps_isr_mutex_handle);
    /*Push frame or ISR triggered*/
    if ((uieventSts == MV_OVPS_EVENT_EOF_INTR) || (uieventSts == MV_OVPS_EVENT_NEW_FRAME)) {
        OVPS_Process(vpp_obj, !OVPS_SKIP_UPDATE_FE_FLAG);
    } else if (uieventSts == MV_OVPS_EVENT_EOF_TIMEOUT) {
        //resubmit the existing request.
        if (ovp_obj->suspend) {
            MV_OSAL_Sem_Post(ovp_obj->ovps_sem_handle);
        }
        OVPS_Process(vpp_obj, OVPS_SKIP_UPDATE_FE_FLAG);
    }
    MV_OSAL_Mutex_Unlock(ovp_obj->ovps_isr_mutex_handle);

    return 0;
}

INT MV_OVPS_PopRequest(OVPS_CLIENT_REQUEST **pstOvpsRequest)
{
    void *frame = NULL;
    OVPS_CLIENT_REQUEST * out_request = NULL;
    int ret_val = 0;
    OVPS_VPPOBJ_INIT();

    if (pstOvpsRequest == NULL) {
        ovps_printf(OVPS_ERR, "Invalid Client req\n");
        return MV_VPP_EBADPARAM;
    }

    /* pop plane's frame queue from output queue */
    ret_val = frmq_pop(&(vpp_obj->ovp_obj.plane.outputq), (void **)&frame);
    if (ret_val == 1) {
        ret_val = 0;
        out_request = (OVPS_CLIENT_REQUEST *)frame;
        frmq_pop_commit(&(vpp_obj->ovp_obj.plane.outputq));

        MV_OSAL_Mutex_Lock(vpp_obj->ovp_obj.client_obj_lock);
        if (vpp_obj->ovp_obj.client_obj[out_request->m_uchClientId].outputQCount)
            vpp_obj->ovp_obj.client_obj[out_request->m_uchClientId].outputQCount--;
        else
            ovps_printf(OVPS_ERR, "Mismatch in outputQCount..\n");
        MV_OSAL_Mutex_Unlock(vpp_obj->ovp_obj.client_obj_lock);
    }

    if (out_request != NULL) {
        MV_OVPS_UpdateDescriptors(out_request, pstOvpsRequest);
    } else {
        ovps_printf(OVPS_MID, "Scaled frame not ready\n");
        ret_val = MV_OVPS_EQUEUEEMPTY;
    }

    return ret_val;
}

/*****************************************************************
 * FUNCTION: Get the number of frames waiting from a client
 * PARAMS: clientId
 * RETURN: MV_VPP_EUNCONFIG - invalid VPP_MAGIC / VPP_OBJ is not initialized
 *         MV_VPP_ENODEV - invalid VPP_OBJ / NULL passed
 *         MV_OVPS_EINVALIDCLIENTID - clientId is invalid
 *         MV_OVPS_ECLIENTNOTACTIVE - clientId is not busy
 *         clientId - Closed/De-Activated clientId
 *****************************************************************/
INT MV_OVPS_GetNoOfRequestsWaiting(INT m_uchClientId, UINT32 *puiRequestsWaiting)
{
    OVPS_CLIENT_OBJ *client_obj = NULL;
    int ret_val;
    OVPS_VPPOBJ_INIT();

    if (!puiRequestsWaiting)
        return MV_VPP_EBADPARAM;

    ret_val = Get_OVPS_ClientState(m_uchClientId);
    if (ret_val != MV_OVPS_ECLIENTBUSY) {
        ovps_printf(OVPS_ERR,"Request waiting for client Id = %d failed (%d)\n",\
                m_uchClientId, ret_val, MV_OVPS_ECLIENTNOTACTIVE);
    } else {
        client_obj = &vpp_obj->ovp_obj.client_obj[m_uchClientId];
        *puiRequestsWaiting = client_obj->rqstCount;
        ret_val = 0;
    }

    if (!ret_val) {
        ovps_printf(OVPS_INFO, "Waiting request count: %d\r\n", *puiRequestsWaiting);
    }
    return ret_val;
}

INT MV_OVPS_ReleaseRequest(INT m_uchClientId,OVPS_CLIENT_REQUEST **pstOvpsRequest)
{
    void *frame = NULL;
    OVPS_CLIENT_REQUEST *client_req = NULL;
    int ret_val, rc;
    OVPS_VPPOBJ_INIT();

    if (!pstOvpsRequest) {
        ovps_printf(OVPS_ERR, "Invalid Client req\n");
        return MV_VPP_EBADPARAM;
    }

    ret_val = Get_OVPS_ClientState(m_uchClientId);
    if (ret_val != MV_OVPS_ECLIENTBUSY) {
        ovps_printf(OVPS_ERR,"Release request for client = %d failed (%d)\n", m_uchClientId, ret_val);
        return ret_val;
    } else {
        /* pop plane's frame queue from i/p queue */
        MV_OSAL_Mutex_Lock(vpp_obj->ovp_obj.ovps_inputQ_lock);
        rc = frmq_pop(&(vpp_obj->ovp_obj.plane.inputq), (void **)&frame);
        if (!rc) {
            MV_OSAL_Mutex_Unlock(vpp_obj->ovp_obj.ovps_inputQ_lock);
            ovps_printf(OVPS_ERR,"Input Queue is Empty\n");
            return MV_OVPS_EQUEUEEMPTY;
        } else {
            frmq_pop_commit(&(vpp_obj->ovp_obj.plane.inputq));
            MV_OSAL_Mutex_Unlock(vpp_obj->ovp_obj.ovps_inputQ_lock);

            client_req = (OVPS_CLIENT_REQUEST *)frame;
            if (client_req->m_uchClientId != m_uchClientId) {
                ovps_printf(OVPS_ERR,"ClientID mismatch (%d %d)\n",
                        client_req->m_uchClientId, m_uchClientId);
                return MV_OVPS_ECLIENTIDMISMATCH;
            } else {
                MV_OSAL_Mutex_Lock(vpp_obj->ovp_obj.client_obj_lock);
                if (vpp_obj->ovp_obj.client_obj[m_uchClientId].rqstCount)
                    vpp_obj->ovp_obj.client_obj[m_uchClientId].rqstCount--;
                else
                    ovps_printf(OVPS_ERR, "Mismatch in rqstCount..\n");
                MV_OSAL_Mutex_Unlock(vpp_obj->ovp_obj.client_obj_lock);
                MV_OVPS_UpdateDescriptors(client_req, pstOvpsRequest);
            }
        }
    }
    return MV_VPP_OK;
}

int MV_OVPS_Destroy(void)
{
    INT32 i, num_of_request, num_of_out_frms;
    OVPS_VPPOVPSOBJ_INIT();

    if (ovp_obj->init_success == 0)
        return (MV_VPP_EUNCONFIG);

    num_of_request = frmq_size(&(vpp_obj->ovp_obj.plane.inputq));
    num_of_out_frms = frmq_size(&(vpp_obj->ovp_obj.plane.outputq));

    if (num_of_request || num_of_out_frms || ovp_obj->in_progress) {
        ovps_printf(OVPS_ERR,"num_of_request  = %d,num_of_out_frms = %d,in_progress = %d\n",
                num_of_request,num_of_out_frms,ovp_obj->in_progress);
        return (MV_VPP_EHARDWAREBUSY);
    }

    for (i = 0; i < OVPS_BCM_BUF_COUNT; i++) {
        if (&ovp_obj->ovpscl_bcm_buf[i])
            OVPS_BCMBUF_Destroy(&(ovp_obj->ovpscl_bcm_buf[i]));
        if (&ovp_obj->ovpscl_clear_bcm_buf[i])
            OVPS_BCMBUF_Destroy(&(ovp_obj->ovpscl_clear_bcm_buf[i]));

        if (ovp_obj->ovpscl_dma_cfgQ[i].handle) {
            VPP_SHM_Free(ovp_obj->ovpscl_dma_cfgQ[i].handle);
            ovp_obj->ovpscl_dma_cfgQ[i].handle = 0;
        }
        if (ovp_obj->ovpscl_bcm_cfgQ[i].handle) {
            VPP_SHM_Free(ovp_obj->ovpscl_bcm_cfgQ[i].handle);
            ovp_obj->ovpscl_bcm_cfgQ[i].handle = 0;
        }
    }
    ovp_obj->pOvpSclBcmBuf = NULL;
    ovp_obj->pOvpSclClearBcmBuf = NULL;
    ovp_obj->pOvpSclStartBcmBuf = NULL;

    if (ovp_obj->ovps_lock) {
        MV_OSAL_Mutex_Destroy(&(ovp_obj->ovps_lock));
        ovp_obj->ovps_lock = NULL;
    }

    if (ovp_obj->client_obj_lock) {
        MV_OSAL_Mutex_Destroy(&(ovp_obj->client_obj_lock));
        ovp_obj->client_obj_lock = NULL;
    }

    if (ovp_obj->ovps_inputQ_lock) {
        MV_OSAL_Mutex_Destroy(&(ovp_obj->ovps_inputQ_lock));
        ovp_obj->ovps_inputQ_lock = NULL;
    }

    if (ovp_obj->ovps_monitor != NULL) {
        MV_Timer_Stop(&ovp_obj->ovps_monitor);
        MV_Timer_Destroy(&ovp_obj->ovps_monitor);
        ovp_obj->ovps_monitor = NULL;
    }

    ovp_obj->init_success = 0;
    ovp_obj->intr_en = 0;
    ovps_printf(OVPS_HIGH, "Success..\n");
    return (MV_VPP_OK);
}

static HRESULT OVPS_Callback(void *ovp_ob, UINT32 uiEventCode, VOID *pEventInfo, VOID *pContext)
{
    TEE_Result result;
    uint32_t cb_cmdID = VPP_CB_OVPS;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);

    TEE_Param cb_params[4];
    VPP_OVP_OBJ *ovp_obj;
    ovp_obj = (VPP_OVP_OBJ *)ovp_ob;

    ovps_printf(OVPS_MID, "obj: %p evt: %d evtinfo: %p ctx: %p\r\n", ovp_obj, uiEventCode, *(UINT32 *)pEventInfo, pContext);
    cb_params[0].value.a = uiEventCode;
    cb_params[0].value.b = *(INT32 *)pEventInfo;
    cb_params[1].value.a = ovp_obj->cb.CallbackFuncAddr;
    cb_params[2].value.a = ovp_obj->cb.UserDataAddr;

    cb_params[3].value.a = 0xdeadbeef;
    result = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return result;
}

int OVPS_RegEventCallback(int handle, UINT32 eventCode,
        UINT64 funcAddr, UINT64 contextAddr)
{
    OVPS_VPPOVPSOBJ_INIT();

    ovp_obj->cb.CallbackFuncAddr = funcAddr;
    ovp_obj->cb.UserDataAddr = contextAddr;
    ovp_obj->callback = OVPS_Callback;
    ovp_obj->eventCode = eventCode;

    ovps_printf(OVPS_MID, "fn: %p ctx: %p cb: %p ev: %d\r\n", funcAddr, contextAddr, (UINT8 *)OVPS_Callback, eventCode);
    return (MV_VPP_OK);
}

/*********************************************************
 * FUNCTION: To Suspend the OVPS
 * PARAMS:
 * RETURN: Failure = -1
 *         Success =  0
 *********************************************************/
INT MV_OVPS_Suspend(BOOL optimize)
{
    UINT32 RegAddr;

    OVPS_VPPOVPSOBJ_INIT();

    if (ovp_obj->init_success == 0)
        return (-1);

#ifndef CONFIG_OVP_WITH_VPPLIB
    MV_OVPS_PreSuspend(optimize);
#endif
    MV_OSAL_Mutex_Lock(ovp_obj->ovps_isr_mutex_handle);
    // Disable Interrupt
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP;
    GA_REG_WORD32_WRITE((RegAddr + RA_OVPSCLWRAP_INTR_EN), 0x0);
    MV_OSAL_Mutex_Unlock(ovp_obj->ovps_isr_mutex_handle);

    return MV_VPP_OK;
}
/*********************************************************
 * FUNCTION: To Clock gate OVPS clocks
 * PARAMS:
 * RETURN: Failure = -1
 *         Success =  0
 *********************************************************/
INT MV_OVPS_EnableClkGate(void)
{
    OVPS_VPPOVPSOBJ_INIT();

    if (ovp_obj->init_success == 0)
        return (-1);

    MV_OSAL_Mutex_Lock(ovp_obj->ovps_isr_mutex_handle);
    // Disable OVPS Clock
    MV_RA_Gbl_ClkD2_Ctrl(RA_Gbl_avioOvpClk, clkD2_ctrl_ClkEn_disable);
    MV_OSAL_Mutex_Unlock(ovp_obj->ovps_isr_mutex_handle);

    return MV_VPP_OK;
}
/*********************************************************
 * FUNCTION: To Resume the OVPS
 * PARAMS:
 * RETURN: Failure = -1
 *         Success =  0
 *********************************************************/
INT MV_OVPS_Resume(BOOL optimize)
{
    OVPS_VPPOVPSOBJ_INIT();

    if (ovp_obj->init_success == 0)
        return -1;

    MV_OSAL_Mutex_Lock(ovp_obj->ovps_isr_mutex_handle);
    // Enable OVPS Clock
    if (!optimize) {
        MV_RA_Gbl_ClkD2_Ctrl(RA_Gbl_avioOvpClk, clkD2_ctrl_ClkEn_enable);
    }
    MV_OVPS_Reset();

    //  Interrupt will be enabled by OVPSCL_ISR_service
    // Trigger any request that came from user during resume
    OVPS_Process(vpp_obj, OVPS_SKIP_UPDATE_FE_FLAG);

    ovp_obj->suspend = FALSE;
    MV_OSAL_Mutex_Unlock(ovp_obj->ovps_isr_mutex_handle);

    return MV_VPP_OK;
}
/*********************************************************
 * FUNCTION: To Stop the OVPS monitor Timer
 * PARAMS:
 * RETURN:
 *
 *********************************************************/
INT MV_OVPS_TimerStop(void)
{
    OVPS_VPPOVPSOBJ_INIT();

    if (ovp_obj->ovps_monitor != NULL) {
        MV_Timer_Stop(&ovp_obj->ovps_monitor);
    }

    return MV_VPP_OK;
}

/*********************************************************
 * FUNCTION: MV_OVPS_dhub_sem_wait
 * PARAMS:
 * RETURN:
 *
 *********************************************************/
static int MV_OVPS_dhub_sem_wait(VPP_OBJ *vpp_obj, int timeout)
{
    // OVPS DHUB channel interrupt
    int channel = avioDhubSemMap_vpp128b_vpp_inr12;

    while(!(semaphore_chk_full(vpp_obj->pSemHandle, channel)))
    {
        timeout--;
        MV_OSAL_Task_Sleep(1);

        if (timeout <= 0)
        {
            ovps_printf(OVPS_ERR, "OVPS optimize optimize DHUB Timeout\n");
            return MV_VPP_ETIMEOUT;
        }
    }
    //pop sema
    semaphore_pop(vpp_obj->pSemHandle, channel, 1);
    semaphore_clr_full(vpp_obj->pSemHandle, channel);
    if (semaphore_chk_full(vpp_obj->pSemHandle, channel))
    {
        ovps_printf(OVPS_ERR, " OVPS optimize  Error: dHub channel %d interrupt is NOT cleared\n", channel);
        return MV_VPP_EHWFAIL;
    }
    return MV_VPP_OK;
}

/*********************************************************
 * FUNCTION: OVPS Pre-Suspend
 * PARAMS:
 * RETURN:
 *
 *********************************************************/
INT MV_OVPS_PreSuspend (BOOL optimize)
{
    int timeout = OVPS_SUSPEND_TIMEOUT_MS;
    MV_VPP_ERROR err = MV_VPP_OK;
    OVPS_VPPOVPSOBJ_INIT();

    MV_OSAL_Mutex_Lock(ovp_obj->ovps_isr_mutex_handle);
    ovp_obj->suspend = TRUE;
    bool ovps_wait_flag = ovp_obj->in_progress;
    MV_OSAL_Mutex_Unlock(ovp_obj->ovps_isr_mutex_handle);
    if (ovps_wait_flag) {
        if (optimize) {
            MV_OSAL_Sem_Wait(ovp_obj->ovps_sem_handle);
        } else {
            //ISR disabled in kernel, So poll for dhub interrupt with timeout
            err = MV_OVPS_dhub_sem_wait(vpp_obj, timeout);
        }
    }

    return err;
}
