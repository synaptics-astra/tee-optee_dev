/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_ISR_C_

#ifdef CONFIG_AMP_TEE_ENABLE
#include "com_type.h"
#endif
#include "vpp_module.h"
#include "vpp_fe.h"
#include "vpp_scl.h"
#include "vpp_cpcb.h"
#include "vpp_apifuncs.h"
#include "vpp_be.h"
#include "vpp_be_prv.h"
#include "vpp_be_hdmitx.h"
#include "vpp_be_hdr.h"
#include "vpp_cfg.h"
#include "vpp_offlineScl.h"
#include "vpp_isr.h"
#include "vpp.h"
#include "vpp_fe_deint.h"
#include "vpp_fe_dlr.h"
#include "vpp_frc_scl.h"
#include "vpp_be_vop.h"
#include "vpp_bcmbuf.h"
#include "vpp_bcmbuf_util.h"
#include "vpp_intr_util.h"
#include "api_avio_dhub.h"
#include "avioDhub.h"
#include "cinclude.h"
#include "hdmiRxPipe.h"
#include "vpp_cpcb_ovl.h"
#include "vpp_dv_frame_dump.h"
#include "vpp_cpcb_disptg.h"
#include "vpp_ptm.h"
#include "vpp_vmx_apifuncs.h"
#include "vpp_dv_api.h"

#include "vpp_intr_monitor.h"

#include "vpp_debug_util.h"
#include "vpp_debug_dump.h"
#include "ctypes.h"
#ifndef CONFIG_AMP_TEE_ENABLE
#include <sys/ioctl.h>
#endif
#include "vpp_env_ur.h"
#include "vpp_avioMtr.h"
#include "ovps_api.h"

#ifdef CONFIG_AMP_DCM
#include "vpp_fe_dcm_common.h"
#endif

#include "vpp_ngptv_api.h"

#include "vpp_cmu.h"
#ifdef ENABLE_DETEE
#include "vpp_gdetee.h"
#endif
//Disable DHUB clear upon underflow
#ifdef ENABLE_DHUB_CLEAR_UPON_UF
#undef ENABLE_DHUB_CLEAR_UPON_UF
#endif
#ifdef ENABLE_UF_DETECTION_LOGGING
#undef ENABLE_UF_DETECTION_LOGGING
#endif
#define MAX_DELAY_COUNT_DVCOMP_CLR_SEQ  10

/*Enabled below macro to restrict reference window(up/down-scaling) for GFX0/GFX1 to 1920x1080 as per HW limitation in >=BG4 series*/
#define VPP_APPLY_GFX_SCL_REFWIN_LIMITATION
//Enable line dropping for down scaling > 2X
#define ENABLE_SCALAR_LINEDROPPING
//Enable the new method to find the number of line to be dropped using formula method
#define ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA
#define VPP_LINEDROP_STRIDE_MAX (0x7FFFFF) /* BG4CDpA0 2ND DMA uses 24Bit singned values */

/*FIXME: VPP_CT changes - need correct mapping*/
#define avioDhubSemMap_vpp_vppCPCB0_intr avioDhubSemMap_vpp128b_vpp_inr0
#define vpp_vop2_DTG_frst_intr avioDhubSemMap_vpp128b_vpp_inr14
#define avioDhubSemMap_vpp_vppOUT4_intr avioDhubSemMap_vpp128b_vpp_inr6
#define avioDhubSemMap_vpp_CH10_intr avioDhubSemMap_vpp128b_CH8_intr
#define avioDhubSemMap_vpp_vppOUT0_intr avioDhubSemMap_vpp128b_vpp_inr2
#define avioDhubSemMap_vpp_vppOUT3_intr avioDhubSemMap_vpp128b_vpp_inr5
#define VBItoCPCB(x, mode)  (bTST(x, avioDhubSemMap_vpp128b_vpp_inr0)? CPCB_1:(bTST(x,avioDhubSemMap_vpp128b_vpp_inr14) && (mode <= VOUT_DISP_SINGLE_MODE_SEC))?CPCB_1:CPCB_2)

#define SWAP_W_H(x,y)   \
{\
    int tmp;    \
    tmp = x;    \
    x = y;      \
    y = tmp;    \
};

#ifdef CONFIG_OVP_WITH_VPPLIB
#define VPP_SCHED_BCMBUF_To_CFGQ(bcmbuf, cfgQ) \
{\
     if(vpp_obj->ovp_obj.m_vpp_sched_request) { \
         rc = VPP_BCMBUF_To_CFGQ(bcmbuf, cfgQ); \
     } \
};
#define VPP_SCHED_CFGQ_To_CFGQ(cfg0, cfg1) \
{\
    if(vpp_obj->ovp_obj.m_vpp_sched_request) { \
        VPP_CFGQ_To_CFGQ(cfg0, cfg1); \
        vpp_obj->ovp_obj.m_vpp_sched_request = 0; \
    } \
};
#else
#define VPP_SCHED_BCMBUF_To_CFGQ(bcmbuf, cfgQ)
#define VPP_SCHED_CFGQ_To_CFGQ(cfg0, cfg1)
#endif

#include "vpp_cpcb_tg.h"

//Bellow definition are for OTP. They are copied from DRM cypto
#define DRM_BASE_ADDR      0xF7CC0000
#define OTP_OFFSET         0x000006a0
#define BINDINFO_OFFSET_BYTE_32    0x00000138
#define BINDINFO_OFFSET_BYTE_24    0x00000130
#define BINDINFO_OFFSET_BYTE_28    0x00000134
#define BI_TYPE_SENSIO       3
//end of definition for OTP

#define CPU_ID 0

extern int berlin_pe_dev;
extern int berlin_cec_dev;

#define PULLUP_ENABLE

//#define REPEAT_FRAME_ENHANCE
//#define VPP_DEBUG

#ifdef VPP_DEBUG
volatile int main_dhub_fail = 0;
volatile int deint_wdhub_fail = 0;
volatile int deint_r0dhub_fail = 0;
volatile int gfx0_dhub_fail = 0;
volatile int gfx1_dhub_fail = 0;
volatile int gfx2_dhub_fail = 0;
volatile int pg_dhub_fail = 0;
#endif

#ifdef VPP_TIME_PROFILE
volatile int gVBI_ISR_Latency[MAX_NUM_CPCBS] = {0};
volatile int gVBI_ISR_VPP_Latency[MAX_NUM_CPCBS] = {0};
#endif

#include "vpp_clkGate.h"

//#define VPP_ISR_PROFILE
#define VPP_ISR_SCHED_TIMEOUT 2

inline static void disp_callback_dummy(VPP_INTR_INFO *dummy)
{
    MV_ASSERT(0);   //should not be here
}

VPP_INTR_CALLBACK_T disp_isr_start_service = disp_callback_dummy;
VPP_INTR_CALLBACK_T clk_isr_service = disp_callback_dummy;
VPP_INTR_CALLBACK_T disp_dsr_service = disp_callback_dummy;
VPP_INTR_CALLBACK_T disp_isr_end_service = disp_callback_dummy;

//#ifdef ENABLE_DEC
extern int g_flow_control[2];
//#endif

// HPD Debounce Logic
#define HPD_STABLE_CHECK_TIME (50) /* In MSec */
static UINT32 hpd_debounce_time;
static UINT32 GetCurTimeInMSec (void)
{
    UINT32 timeInMS = 0;
#ifndef CONFIG_AMP_TEE_ENABLE
    timeInMS = MV_Time_GetOSTimeMS();
#else
    TEE_Time curr_time;
    TEE_GetREETime(&curr_time);
    timeInMS = (curr_time.seconds*1000 + curr_time.millis);
#endif
    return timeInMS;
}

#ifdef ENABLE_SCALAR_LINEDROPPING
#ifndef ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA
/* Know issue expected to be fixed in silicon -
 * Jaggy image for following Dispwin parameter (0 0 1920 541) to (0 0 1920 549) */

//Get the starting line to be selected while applying line dropping.
static int getLineDroppingStartY(PLANE *plane) {
    int startY = 0;
    //TBD: LineDroppingStyle other than 'LINEDROPPING_SEL_LINE_FIRST' is not working of TILED mode
    //so override user selection, until the feature is avaialable/fixed
    if(0 == plane->sclLineDropEn)
        return MV_VPP_EUNCONFIG;
    plane->lineDroppingStyle = LINEDROPPING_SEL_LINE_FIRST;
    switch(plane->lineDroppingStyle) {
        case LINEDROPPING_SEL_LINE_MID:
            if(plane->lineDroppingStrides) {
                startY = ((plane->lineDroppingStrides+1)/2)-1;
            }
        break;
        case LINEDROPPING_SEL_LINE_LAST:
            if(plane->lineDroppingStrides) {
                startY = plane->lineDroppingStrides-1;
            }
        break;
        case LINEDROPPING_SEL_LINE_FIRST:
        default:
            startY = 0;
        break;
    }
    return startY;
}
static int is4KInputLineDroppingRequired(VPP_OBJ *vpp_obj, int chanID) {
    PLANE *plane;
    CHAN *chan;
    int planeID;
    int cpcbID;
    int lineDrop4KSel = 0;

    chan = &vpp_obj->chan[chanID];
    planeID = PLANE_OF_CHAN(vpp_obj, chanID);
    cpcbID = CPCB_OF_CHAN(vpp_obj, chanID);
    plane = &vpp_obj->plane[planeID];

    if(0 == plane->sclLineDropEn)
        return MV_VPP_EUNCONFIG;

    if(planeID == PLANE_MAIN) {
        if((RES_4Kx2K60 == vpp_obj->uiResID[cpcbID]) && ( chan->disp_win_final.height >= 1080) &&
        ((((plane->ref_win.height - chan->disp_win_final.height)* 100)/ (plane->ref_win.height)) > 13)) {
            lineDrop4KSel = 1;
        }
    } else {
        //PIP or GFX plane - Any 4K resolution/canvas
        if( (vpp_obj->uiResID[cpcbID] >= RES_4Kx2K2398 &&
                    vpp_obj->uiResID[cpcbID] <= RES_4Kx2K60_SMPTE) &&
                ( chan->disp_win_final.height >= 1080) &&
                (!(RES_720P_4Kx1K120_3D == vpp_obj->uiResID[cpcbID] ||
                   RES_4Kx2K30_HDMI == vpp_obj->uiResID[cpcbID])) ) {
            lineDrop4KSel = 1;
        }
    }
    return lineDrop4KSel;
}
#endif
#endif //ENABLE_SCALAR_LINEDROPPING

void VPP_RegisterInterruptService(VPP_INTR_CALLBACK_T f1, VPP_INTR_CALLBACK_T f2,
        VPP_INTR_CALLBACK_T f3, VPP_INTR_CALLBACK_T f4)
{
    if (NULL != f1) {
        disp_isr_start_service = f1;
    }
    if (NULL != f2) {
        clk_isr_service = f2;
    }
    if (NULL != f3) {
        disp_dsr_service = f3;
    }
    if (NULL != f4) {
        disp_isr_end_service = f4;
    }
}

/////////////////////////////////////////////////////
//                                                 //
//  Interrupt Service Routines for Berlin          //
//                                                 //
/////////////////////////////////////////////////////
/********************************************************************************
 * FUNCTION: update current frames of all active plane of a VPP FE channel
 * PARAMS: *vpp_obj - pointer to VPP object
 *         chanID - channel  * RETURN: MV_VPP_OK - succeed
 *******************************************************************************/
static void updateFEChannelCurrentFrame(VPP_OBJ *vpp_obj, int chanID)
{
    int i, planeID, cpcb_resID, cpcbID;
    unsigned int pts;
    PLANE *plane;
    CHAN *chan;
    VDEC_VID_BUF_DESC *frm_descr = NULL;
    VBUF_INFO *pinfo = NULL;

    cpcbID = CPCB_OF_CHAN(vpp_obj, chanID);
    chan = &vpp_obj->chan[chanID];

    /* update current frame for active planes in DISP_VIDEO status of this channel */
    for (i = 0; i < chan->num_of_planes; i++) {
        planeID = chan->planeID[i];
        plane = &vpp_obj->plane[planeID];

        VPP_DEBUG_UTIL_monitorFrameDrop(vpp_obj, VPP_DEBUG_FRAMEDROP_STATE_START, planeID, frm_descr);

        /* go through all active planes in */
        if (plane->status == STATUS_DISP_VIDEO){
            /* recycle only when new frame is popped */
            if (plane->prev_curr_frame != plane->curr_frame && plane->prev_curr_frame){
                VPP_PtmUnref(&g_vpp->ptm_obj, planeID, plane->prev_curr_frame);
                frmq_push(&plane->outputq, plane->prev_curr_frame);
            }
            plane->prev_curr_frame = plane->curr_frame;
            plane->curr_frame = NULL; /*reset the curr_frame and try to pop a new frame*/
            /* try to get a new frame descriptor from frame queue */
            while (frmq_pop(&plane->inputq, (void **)&frm_descr)) {

#ifndef CONFIG_AMP_TEE_ENABLE
                /* new frame descriptor found */
                pinfo = (VBUF_INFO *)frm_descr->user_data;
#else
                pinfo = (VBUF_INFO *)frm_descr;
#endif
                VPP_DEBUG_UTIL_monitorFrameDrop(vpp_obj, VPP_DEBUG_FRAMEDROP_STATE_PROCESS,
                        planeID, frm_descr);

#ifdef BERLIN_DTV_MERGE
                // to avoid data abort exception in DTG MHEG test 6.8.2B
                if (pinfo == NULL) {
                    frmq_pop_commit(&plane->inputq);
                    frmq_push(&plane->outputq, frm_descr);
                    VPP_Free(frm_descr);
                    continue;
                }
#endif
                /* if new frame has error or phy_addr as zero, discard new frame */
                if (pinfo->m_flags & 0x02) {
                    /* frame has error, discard new frame */
                    frmq_pop_commit(&plane->inputq);
                    frmq_push(&plane->outputq, frm_descr);
                    continue;
                }

                /*if required, Enable early HDMI signaling and pop inputq frames only after signaling*/
                if((vpp_obj->m_HdrMode == VPP_HDR_MODE_AUTO) && (planeID == PLANE_MAIN) && (cpcbID == CPCB_1)) {
                    if(VPP_BE_HDR_SetHDMIEarlySignaling(vpp_obj, pinfo))
                        break;
                }

                frmq_pop_commit(&plane->inputq);
                /* recycle current frame descriptor */
                if (plane->curr_frame) {
                    //Mark m_FrameDesc_RefStat flag that this frame is not in use for recycling it.
                    pinfo->m_FrameDesc_RefStat &= (~0 ^ VPP_FRAME_IN_USE);
                    frmq_push(&plane->outputq, plane->curr_frame);
                }
                /* update current frame descriptor with new frame descriptor */
                plane->curr_frame = frm_descr;
                //printfVBInfo(pinfo);
                /* if new frame's PTS is valid, update plane's PTS and reset frame count */
                if (pinfo->m_flags & 0x01) {
                    /* new frame with valid PTS */
                    /* reset plane's last valid and current PTS */
#ifndef CONFIG_AMP_TEE_ENABLE
                    plane->pts.hi_word = ((VBUF_INFO *)(frm_descr->user_data))->m_pts_hi;
                    plane->pts.lo_word = ((VBUF_INFO *)(frm_descr->user_data))->m_pts_lo;
#else
                    plane->pts.hi_word = ((VBUF_INFO *)frm_descr)->m_pts_hi;
                    plane->pts.lo_word = ((VBUF_INFO *)frm_descr)->m_pts_lo;
#endif
                    plane->delta_pts.hi_word = 0;
                    plane->delta_pts.lo_word = 0;
                    /* reset frame count */
                    plane->frm_count = -1; /* count back 1 to make frm_count 0*/
                } else {
                    /* what to do ? just invalid the frame pts */
                }
                if(IS_DV_CertTest() && (planeID == PLANE_MAIN) && (vpp_obj->m_uchDvEnableDump)) {
                    //avoid frame skip in dolby IDK dump cases
                    break;
                }
                //Record the extra scl requirement for built-in frame into channel object
                chan->built_in_frame_scl_required = pinfo->builtinFrame & 0x02;
                pinfo->builtinFrame &= ~0x02;
            } // <- while loop, pop a new frame from frame queue

            /*repeat the old frame if no one is not popped*/
            if (!plane->curr_frame) {
                plane->curr_frame = plane->prev_curr_frame;
                pinfo = (VBUF_INFO *)plane->prev_curr_frame;
            } else {
                VPP_PtmRef(&g_vpp->ptm_obj, planeID, plane->curr_frame);
                if(cpcbID == CPCB_1) {
                    VPP_DV_Updated_Metadata_Addr(pinfo);
                }
#ifdef CONFIG_AMP_DOLBY_DUMP
                if((planeID == PLANE_MAIN) && (cpcbID == CPCB_1)) {
                    //Get the new buffer for capture the debug dump frame
                    //Please don't get new frame if it is built in frame;
                    //use offline scalar buffer for built-in frame
                    if (pinfo && !pinfo->builtinFrame) {
                        VPP_DV_FrameDump_GetNewFrame(vpp_obj);
                        VPP_DV_SetDelayedPriorityMode(vpp_obj, vpp_obj->vpp_frame_count);
                    } else {
                       vpp_obj->vpp_frame_count = 0;
                    }
                }
#endif //CONFIG_AMP_DOLBY_DUMP
            }
            /* update frame interrupt count for this plane */
            plane->frm_count ++;
            /* update current PTS */
            if (plane->frm_count){
                RESOLUTION_INFO  *pResinfo;
                cpcb_resID = VPP_GetResId(chan->dvID);
                pResinfo = VPP_GetResInfo(cpcb_resID);
                pts = plane->delta_pts.lo_word;
                plane->delta_pts.lo_word += pResinfo->pts_per_cnt_4;
                if (plane->delta_pts.lo_word < pts)
                    plane->delta_pts.hi_word ++; /* wrap around */
            }

#ifdef VPP_ENABLE_FRAME_CAPTURE
            if(plane->CaptureVidFrm.gCapStartFlag == true){
                MV_OSAL_Sem_Post(vpp_obj->vpp_FrameCapture_sem);
            }
#endif
            VPP_DEBUG_UTIL_monitorFrameRepeat(vpp_obj, VPP_DEBUG_FRAMEDROP_STATE_PROCESS,
                    planeID, plane->curr_frame);

        } // <- DISP_VIDEO
        else if(plane->status == STATUS_DISP_LOGO) {
            /* Expecting minimum three buffers for ref & unref
             * If only two buffer, already ref buffer may get ref and unref in next ISR
             */
            if(plane->prev_still_picture && plane->prev_still_picture != plane->curr_still_picture) {
                VPP_PtmUnref(&g_vpp->ptm_obj, planeID, plane->prev_still_picture);
            }
            if (plane->still_picture && plane->still_picture != plane->curr_still_picture) {
                VPP_PtmRef(&g_vpp->ptm_obj, planeID, plane->still_picture);
            }
            plane->prev_still_picture = plane->curr_still_picture;
            plane->curr_still_picture = plane->still_picture;
            VPP_DEBUG_UTIL_monitorFrameDrop(vpp_obj, VPP_DEBUG_FRAMEDROP_STATE_PROCESS, \
                    planeID, plane->curr_still_picture);
            VPP_DEBUG_UTIL_monitorFrameRepeat(vpp_obj, VPP_DEBUG_FRAMEDROP_STATE_PROCESS, \
                    planeID, plane->curr_still_picture);
        }
    } // <- for loop
}

void VPP_Early_Update_CurrentFrame(VPP_OBJ *vpp_obj, int chanID, void *frame)
{
    VBUF_INFO *pinfo;
    VDEC_VID_BUF_DESC *inline_frame;
    inline_frame = frame;
    PLANE *plane;
    int crop_actv_offset = 0, crop_actv_offset_UV = 0;
    plane = &vpp_obj->plane[PLANE_MAIN];
#if !defined(VPP_IN_TRUST_ZONE)
    /* new frame descriptor found */
    pinfo = (VBUF_INFO *)inline_frame->user_data;
#else
    pinfo = (VBUF_INFO *)inline_frame;
#endif
    /* need to confirm once */
    if(plane->vpp_last_frame_addrY != NULL)
    {
        crop_actv_offset = (int)pinfo->m_pbuf_start+ pinfo->m_disp_offset +
            pinfo->m_content_offset+plane->crop_actv_offset;
        *(plane->vpp_last_frame_addrY) =  crop_actv_offset;
#if !defined(VPP_IN_TRUST_ZONE)
        VPP_SHM_CleanCache (plane->vbi_cfgq_shm_handle,0,32);
#else
        TEE_FlushCache(plane->vpp_last_frame_addrY, 8);
#endif
    }
    if(plane->vpp_last_frame_addrUV != NULL)
    {
        crop_actv_offset_UV = (int)pinfo->m_buf_pbuf_start_UV + pinfo->m_disp_offset +
            pinfo->m_content_offset+plane->crop_actv_offset_UV;
        *(plane->vpp_last_frame_addrUV) =  crop_actv_offset_UV;
#if !defined(VPP_IN_TRUST_ZONE)
        VPP_SHM_CleanCache (plane->vbi_cfgq_shm_handle,0,32);
#else
        TEE_FlushCache(plane->vpp_last_frame_addrUV, 8);
#endif
    }
}
/******************************************************************************************
 * FUNCTION: crop display content
 * PARAMS: *crop_actv_win - (O)pointer to cropped display content size in memory
 *         *crop_actv_offset - (O)pointer to cropped display content offset
 *         *crop_scl_actv_win - (O)pointer to cropped display content size in display window
 *         *pinfo - (I)pointer to display content info in memory
 *         *ref_win - (I)pointer to reference window
 *         *disp_win - (I)pointer to display window
 *         *cpcb_resinfo - (I)pointer to end display resolution info
 * RETURN: none
 *****************************************************************************************/
void cropDispContent(VPP_WIN *crop_actv_win, int *crop_actv_offset,
        VPP_WIN *crop_scl_actv_win, VBUF_INFO *pinfo, VPP_WIN *ref_win,
        VPP_WIN *disp_win, RESOLUTION_INFO *cpcb_resinfo, int dataAlign_x,
        int dataAlign_y, int bpp, PLANE *plane)
{
    int tmp;
    float bytesPerPixel = bpp/8.0;
    VPP_TILE tile;
    int ref_win_backup_width = 0, ref_win_backup_height = 0, ref_win_backup_x = 0, ref_win_backup_y = 0;
    int BL_Width = 0, BL_height = 0, Bl_TileStrideFrac = 0, BL_LeftPad = 0, BL_TopPad = 0;
    int mtr_pan_scan_x_offset = 0, mtr_pan_scan_y_offset = 0;
    int mtr_pan_scan_x_crop = 0, mtr_pan_scan_y_crop = 0;
    UINT32 uiInteraceFactor = cpcb_resinfo->scan ? 2 : 1;

    ref_win_backup_width = ref_win->width;
    ref_win_backup_height = ref_win->height;
    ref_win_backup_x = ref_win->x;
    ref_win_backup_y = ref_win->y;

    if((plane->m_SeparateElcropRequired)&&(plane->m_El_crop_recalculation_required))
    {
        ref_win->width = ref_win->width/2;
        ref_win->height = ref_win->height/2;
        ref_win->x = ref_win->x/2;
        ref_win->y = ref_win->y/2;

#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
        pinfo->m_active_left = -ref_win->x;
        pinfo->m_active_top = -ref_win->y;
#endif

        if((plane->m_mtrR__en_el)&&(plane->m_SampleCropEnable_el))
        {
            BL_Width = plane->m_Aligned_Luma_Width;
            BL_height = plane->m_Aligned_Luma_Height;
            BL_LeftPad = plane->m_leftpad_y;
            BL_TopPad = plane->m_toppad_y;
            plane->m_leftpad_y = pinfo->m_luma_left_ofst;
            plane->m_toppad_y = pinfo->m_luma_top_ofst;
        }

        Bl_TileStrideFrac = plane->tile.tileStrideFrac;
    }

    /* crop display content in reference window */

    /* initially set crop display content same with the original */
    crop_actv_win->x = pinfo->m_active_left;
    if(SRCFMT_YUV420SP == plane->srcfmt)
    {
        /*For 420SP two Y line share one UV line. So aligning to even */
        crop_actv_win->y = (pinfo->m_active_top + pinfo->m_active_top%2);
    }
    else
        crop_actv_win->y = pinfo->m_active_top;
    crop_actv_win->width = pinfo->m_active_width;
    crop_actv_win->height = pinfo->m_active_height;
    *crop_actv_offset = 0;
    plane->m_leftpadadjust_y = 0;
    plane->m_toppadadjust_y = 0;
    VPP_OVL_getTileDetails(plane->uchClientMode, ref_win->width, ref_win->height, &tile);

    if(plane->m_SeparateElcropRequired &&
            ((SRCFMT_YUV420SP_TILED_MODE0 == pinfo->m_srcfmt) &&
             (10 == pinfo->m_bits_per_pixel)))
    {
        plane->tileStrideFrac_EL = plane->tile.tileStrideFrac;
    }
    else
    {
        plane->tileStrideFrac_EL = 0;
    }
    VDBG_IF_DISP_WIN(ref_win);
    VDBG_IF_DISP_WIN(disp_win);
    VDBG_IF_DISP_WIN(crop_actv_win);
    VDBG_IF_DISP_WIN(crop_scl_actv_win);
    VDBG_IF_DISP("crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",
            *crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8),
            (int)((*crop_actv_offset)/(bytesPerPixel)));

    /* horizontal cropping */
    if (crop_actv_win->x < 0)
    {
        /* crop left side */
        tmp = -crop_actv_win->x;
        tmp -= tmp%dataAlign_x;

        if((pinfo->m_srcfmt == SRCFMT_ARGB32)||
                (pinfo->m_srcfmt == SRCFMT_RGB888)||
                (pinfo->m_srcfmt == SRCFMT_ARGB32_PM)||
                (pinfo->m_srcfmt == SRCFMT_ARBG2101010)||
                (pinfo->m_srcfmt == SRCFMT_YUV422)||
                (pinfo->m_srcfmt == SRCFMT_ARGB4444)||
                (pinfo->m_srcfmt == SRCFMT_RGB565)||
                (pinfo->m_srcfmt == SRCFMT_ARGB1555)||
                (pinfo->m_srcfmt == SRCFMT_ARGB8332)||
                (pinfo->m_srcfmt == SRCFMT_YUV420SP) ||
                (pinfo->m_srcfmt == SRCFMT_YUV444_DWA)||
                (pinfo->m_srcfmt == SRCFMT_YUV444))
        {
            *crop_actv_offset -= ((-tmp) * bytesPerPixel);
        }
        else if(pinfo->m_srcfmt == SRCFMT_YUV420SP_DWA ||
                pinfo->m_srcfmt == SRCFMT_YUV422SP_DWA)
        {
            *crop_actv_offset -= (((-tmp) * 10 * 32)/(30 * 8));
        }
        else //TILED MODE
        {
            *crop_actv_offset -= ((-tmp/tile.width) * tile.bytesPerTile);
        }

        VDBG_IF_DISP("x:crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",
                *crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8),
                (int)((*crop_actv_offset)/(bytesPerPixel)));

        crop_actv_win->width += crop_actv_win->x;
        crop_actv_win->x = 0;
    }

    if ((crop_actv_win->x + crop_actv_win->width) > ref_win->width) {
        /* crop right side */
        crop_actv_win->width -= (crop_actv_win->x + crop_actv_win->width - ref_win->width);
        plane->m_ref_win_update_pan_scan = 1;
    }

    if (crop_actv_win->width <= 0)
        return;

    crop_scl_actv_win->x = crop_actv_win->x * disp_win->width / ref_win->width + disp_win->x;
    crop_scl_actv_win->width = crop_actv_win->width * disp_win->width / ref_win->width;

    /* horizontal cropping of display content */
    if (crop_scl_actv_win->x < 0) {
        /* crop left side */
        tmp = crop_scl_actv_win->x * ref_win->width / disp_win->width;
        crop_actv_win->width += tmp;
#ifdef CONFIG_AMP_VPP_ROTATION
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
        {   if(plane->m_SampleCropEnable || plane->m_SampleCropEnable_el)
            {
                mtr_pan_scan_x_offset = ((-tmp)+plane->m_leftpad_y)/dataAlign_x;
                mtr_pan_scan_x_crop = ((-tmp))%dataAlign_x;
                plane->m_ref_win_update_pan_scan = 1;
            }
            else
            {
                *crop_actv_offset -= ((tmp/tile.width) * tile.bytesPerTile);
            }
        }
        else
        {
            *crop_actv_offset -= (tmp * bytesPerPixel);
        }
        VDBG_IF_DISP("cx:crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",
                *crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8),
                (int)((*crop_actv_offset)/(bytesPerPixel)));
        crop_scl_actv_win->width += crop_scl_actv_win->x;
        crop_scl_actv_win->x = 0;
    }
    if ((crop_scl_actv_win->x + crop_scl_actv_win->width) > cpcb_resinfo->active_width) {
        /* crop right side */
        tmp = crop_scl_actv_win->x + crop_scl_actv_win->width - cpcb_resinfo->active_width;
        crop_actv_win->width -= tmp * ref_win->width / disp_win->width;
        crop_scl_actv_win->width -= tmp;
        plane->m_ref_win_update_pan_scan = 1;
        //crop_scl_actv_win->x = 0;
    }

    if((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt) &&
            (plane->m_SampleCropEnable || plane->m_SampleCropEnable_el))
    {
        int xalign = 0;
        int temp = -(pinfo->m_active_left);
        int AlignedWidth = 0;
        if(temp < 0)
            temp = -temp;
        xalign = dataAlign_x;

        if(!plane->m_SeparateElcropRequired)
        {
            plane->uDhubCropOffset_y.u32_Tile_Xoffset = mtr_pan_scan_x_offset +
                ((temp + plane->m_leftpad_y)/xalign);
        }
        else
        {
            plane->el_uDhubCropOffset_y.u32_Tile_Xoffset = mtr_pan_scan_x_offset +
                ((temp + plane->m_leftpad_y)/xalign);
        }
        plane->m_leftpadadjust_y = ((mtr_pan_scan_x_crop + temp + plane->m_leftpad_y)%xalign);
        if(pinfo->m_bits_per_pixel == 8)
        {
            {
                AlignedWidth = ((crop_actv_win->width  + plane->m_leftpadadjust_y + 63)/64)*64;
                plane->m_rightpad_y = AlignedWidth - (crop_actv_win->width + plane->m_leftpadadjust_y);
                plane->m_Aligned_Luma_Width = AlignedWidth;
            }
        }
        else if(pinfo->m_bits_per_pixel == 10)
        {
            {
                AlignedWidth = ((crop_actv_win->width + plane->m_leftpadadjust_y + 47)/48)*48;
                plane->m_rightpad_y = AlignedWidth - (crop_actv_win->width + plane->m_leftpadadjust_y);
                plane->m_Aligned_Luma_Width = AlignedWidth;
            }
        }
    }

    /* vertical cropping */
    if (crop_actv_win->y < 0) {
        /* crop top side */
        tmp = -crop_actv_win->y;
        tmp -= tmp%dataAlign_y;
#ifdef CONFIG_AMP_VPP_ROTATION
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
            *crop_actv_offset -= (-tmp * ((int)(pinfo->m_buf_stride/VPP_TILED_MODE_STRIDE_FACTOR)));
        else
            *crop_actv_offset -= (-tmp * ((int)pinfo->m_buf_stride));
        VDBG_IF_DISP("y:crop_actv_offset : 0x%x, y:%d, bits_per_pixels= %d, pixel = %d\n",
                *crop_actv_offset, crop_actv_win->y, (int)(bytesPerPixel*8),
                (int)((*crop_actv_offset)/(bytesPerPixel)));
        crop_actv_win->height += crop_actv_win->y;
        crop_actv_win->y = 0;
    }

    if ((crop_actv_win->y + crop_actv_win->height) > ref_win->height) {
        /* crop bottom side */
        crop_actv_win->height -= (crop_actv_win->y + crop_actv_win->height - ref_win->height);
        plane->m_ref_win_update_pan_scan = 1;
    }

    if (crop_actv_win->height <= 0)
        return;

    crop_scl_actv_win->y = crop_actv_win->y * disp_win->height / ref_win->height + disp_win->y;
    crop_scl_actv_win->height = crop_actv_win->height * disp_win->height / ref_win->height;

    /* vertical cropping of display content */
    if (crop_scl_actv_win->y < 0) {
        /* crop top side */
        tmp = crop_scl_actv_win->y * ref_win->height / disp_win->height;
        crop_actv_win->height += tmp;
#ifdef CONFIG_AMP_VPP_ROTATION
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
        {
            if(plane->m_SampleCropEnable || plane->m_SampleCropEnable_el)
            {
                mtr_pan_scan_y_offset = ((-tmp)+plane->m_toppad_y)/dataAlign_y;
                mtr_pan_scan_y_crop = ((-tmp)+plane->m_toppad_y)%dataAlign_y;
                plane->m_ref_win_update_pan_scan = 1;
            }
            else
            {
                *crop_actv_offset -= tmp * ((int)(pinfo->m_buf_stride/VPP_TILED_MODE_STRIDE_FACTOR));
            }
        }
        else
        {
            *crop_actv_offset -= tmp * ((int)pinfo->m_buf_stride);
        }
        VDBG_IF_DISP("cy:crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",
                *crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8),
                (int)((*crop_actv_offset)/(bytesPerPixel)));
        crop_scl_actv_win->height += crop_scl_actv_win->y;
        crop_scl_actv_win->y = 0;
    }
    if ((crop_scl_actv_win->y + crop_scl_actv_win->height) >
            (cpcb_resinfo->active_height / uiInteraceFactor)) {
        /* crop bottom side */
        tmp = crop_scl_actv_win->y + crop_scl_actv_win->height -
            (cpcb_resinfo->active_height / uiInteraceFactor);
        crop_actv_win->height -= tmp * ref_win->height / disp_win->height;
        plane->m_ref_win_update_pan_scan = 1;
        crop_scl_actv_win->height -= tmp;
    }

    if((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt) &&
            (plane->m_SampleCropEnable || plane->m_SampleCropEnable_el))
    {
        int yalign = 0;
        int temp = -(pinfo->m_active_top);
        int AlignedHeight = 0;
        if(temp < 0)
            temp = -temp;
        yalign = 8;//dataAlign_y;
        if(!plane->m_SeparateElcropRequired)
        {
            plane->uDhubCropOffset_y.u32_Tile_Yoffset = mtr_pan_scan_y_offset +
                (((temp/yalign)*2) + (plane->m_toppad_y/4));
        }
        else
        {
            plane->el_uDhubCropOffset_y.u32_Tile_Yoffset = mtr_pan_scan_y_offset +
                (((temp/yalign)*2) + (plane->m_toppad_y/4));
        }
        plane->m_toppadadjust_y = ((temp+mtr_pan_scan_y_crop)%yalign);
        AlignedHeight = ((crop_actv_win->height + plane->m_toppadadjust_y + 7)/8)*8;
        plane->m_bottompad_y = AlignedHeight - (crop_actv_win->height + plane->m_toppadadjust_y);
        plane->m_Aligned_Luma_Height = AlignedHeight;

    }

    if(plane->m_SeparateElcropRequired)
    {
        if(plane->m_mtrR__en_el && plane->m_SampleCropEnable_el)
        {
            plane->m_El_Aligned_Luma_Width = plane->m_Aligned_Luma_Width;
            plane->m_El_Aligned_Luma_Height = plane->m_Aligned_Luma_Height;
            plane->m_Aligned_Luma_Width = BL_Width;
            plane->m_Aligned_Luma_Height = BL_height;
            plane->m_leftpad_y = BL_LeftPad;
            plane->m_toppad_y = BL_TopPad;
        }
        plane->tile.tileStrideFrac = Bl_TileStrideFrac;
    }

    ref_win->width = ref_win_backup_width;
    ref_win->height = ref_win_backup_height;
    ref_win->x = ref_win_backup_x;
    ref_win->y = ref_win_backup_y;

    VDBG_IF_DISP_WIN(crop_actv_win);
    VDBG_IF_DISP_WIN(crop_scl_actv_win);
    VDBG_IF_DISP("crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",
            *crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8),
            (int)((*crop_actv_offset)/(bytesPerPixel)));
    return;
}

void cropDispContent_UV(VPP_WIN *crop_actv_win, int *crop_actv_offset,
        VPP_WIN *crop_scl_actv_win, VBUF_INFO *pinfo, VPP_WIN *ref_win,
        VPP_WIN *disp_win, RESOLUTION_INFO *cpcb_resinfo,
        int dataAlign_x, int dataAlign_y, int bpp, PLANE *plane)
{
    int tmp;
    float bytesPerPixel = bpp/8.0;
    VPP_TILE tile;
    int ref_win_backup_width = 0, ref_win_backup_height = 0, ref_win_backup_x = 0, ref_win_backup_y = 0;
    int BL_Width = 0, BL_Height = 0, BL_LeftPad = 0, BL_TopPad = 0;
    int mtr_pan_scan_x_offset = 0, mtr_pan_scan_y_offset = 0;
    int mtr_pan_scan_x_crop = 0, mtr_pan_scan_y_crop = 0;
    UINT32 uiInteraceFactor = cpcb_resinfo->scan ? 2 : 1;

    ref_win_backup_width = ref_win->width;
    ref_win_backup_height = ref_win->height;
    ref_win_backup_x = ref_win->x;
    ref_win_backup_y = ref_win->y;

    if((plane->m_SeparateElcropRequired)&&(plane->m_El_crop_recalculation_required))
    {
        ref_win->width = ref_win->width/2;
        ref_win->height = ref_win->height/2;
        ref_win->x = ref_win->x/2;
        ref_win->y = ref_win->y/2;

#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
        pinfo->m_active_left = -ref_win->x;
        pinfo->m_active_top = -ref_win->y;
#endif
        if((plane->m_mtrR__en_el)&&(plane->m_SampleCropEnable_el))
        {
            BL_Width = plane->m_Aligned_Chroma_Width;
            BL_Height = plane->m_Aligned_Chroma_Height;
            BL_LeftPad = plane->m_leftpad_c;
            BL_TopPad = plane->m_toppad_c;
            plane->m_leftpad_c = pinfo->m_chroma_left_ofst;
            plane->m_toppad_c = pinfo->m_chroma_top_ofst;
        }
    }

    /* crop display content in reference window */

    /* initially set crop display content same with the original */
    crop_actv_win->x = pinfo->m_active_left;
    if(SRCFMT_YUV420SP == plane->srcfmt)
    {
        /*For 420SP two Y line share one UV line. So aligning to even */
        crop_actv_win->y = (pinfo->m_active_top+pinfo->m_active_top%2)/2;
    }
    else
        crop_actv_win->y = pinfo->m_active_top/2;
    //since y is already divide by 2, align should also be be divide by 2
    dataAlign_y = dataAlign_y>4 ? dataAlign_y/2 : dataAlign_y;
    crop_actv_win->width = pinfo->m_active_width;
    crop_actv_win->height = pinfo->m_active_height/2;
    if(pinfo->m_srcfmt == SRCFMT_YUV422SP_DWA)
    {
        crop_actv_win->y = pinfo->m_active_top;
        crop_actv_win->height = pinfo->m_active_height;
    }
    *crop_actv_offset = 0;
    plane->m_leftpadadjust_c = 0;
    plane->m_toppadadjust_c = 0;

    VPP_OVL_getTileDetails(plane->uchClientMode, ref_win->width, ref_win->height/2, &tile);

    /* horizontal cropping */
    if (crop_actv_win->x < 0)
    {
        /* crop left side */
        tmp = -crop_actv_win->x;
        tmp -= tmp%dataAlign_x;

        if(pinfo->m_srcfmt == SRCFMT_YUV420SP)
        {
            *crop_actv_offset -= ((-tmp) * bytesPerPixel);
        }
        else if(pinfo->m_srcfmt == SRCFMT_YUV420SP_DWA ||
                pinfo->m_srcfmt == SRCFMT_YUV422SP_DWA)
        {
            *crop_actv_offset -= (((-tmp) * 10 * 32)/(30 * 8));
        }
        else //TILED MODE
        {
            *crop_actv_offset -= ((-tmp/tile.width) * tile.bytesPerTile);
        }

        crop_actv_win->width += crop_actv_win->x;
        crop_actv_win->x = 0;
    }

    if ((crop_actv_win->x + crop_actv_win->width) > ref_win->width) {
        /* crop right side */
        crop_actv_win->width -= (crop_actv_win->x + crop_actv_win->width - ref_win->width);
    }

    if (crop_actv_win->width <= 0)
        return;

    crop_scl_actv_win->x = crop_actv_win->x * disp_win->width / ref_win->width + disp_win->x;
    crop_scl_actv_win->width = crop_actv_win->width * disp_win->width / ref_win->width;

    /* horizontal cropping of display content */
    if (crop_scl_actv_win->x < 0) {
        /* crop left side */

        tmp = crop_scl_actv_win->x * ref_win->width / disp_win->width;
        crop_actv_win->width += tmp;
#ifdef CONFIG_AMP_VPP_ROTATION
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
        {
            if(plane->m_SampleCropEnable || plane->m_SampleCropEnable_el)
            {
                mtr_pan_scan_x_offset  = (-tmp)/dataAlign_x;
                mtr_pan_scan_x_crop = ((-tmp))%dataAlign_x;
            }
            else
            {
                *crop_actv_offset -= ((tmp/tile.width) * tile.bytesPerTile);
            }
        }
        else
        {
            *crop_actv_offset -= tmp * pinfo->m_bytes_per_pixel;
        }
        crop_scl_actv_win->width += crop_scl_actv_win->x;
        crop_scl_actv_win->x = 0;
    }
    if ((crop_scl_actv_win->x + crop_scl_actv_win->width) > cpcb_resinfo->active_width) {
        /* crop right side */
        tmp = crop_scl_actv_win->x + crop_scl_actv_win->width - cpcb_resinfo->active_width;
        crop_actv_win->width -= tmp * ref_win->width / disp_win->width;
        crop_scl_actv_win->width -= tmp;
    }

    if((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt) &&
            (plane->m_SampleCropEnable || plane->m_SampleCropEnable_el))
    {
        int xalign = 0;
        int temp = -(pinfo->m_active_left);
        int AlignedWidth = 0;
        if(temp < 0)
            temp = -temp;
        xalign = dataAlign_x;
        if(!plane->m_SeparateElcropRequired)
        {
            plane->uDhubCropOffset_c.u32_Tile_Xoffset = mtr_pan_scan_x_offset +
                ((temp + plane->m_leftpad_c)/xalign);
        }
        else
        {
            plane->el_uDhubCropOffset_c.u32_Tile_Xoffset = mtr_pan_scan_x_offset +
                ((temp + plane->m_leftpad_c)/xalign);
        }
        plane->m_leftpadadjust_c = ((temp + mtr_pan_scan_x_crop + plane->m_leftpad_c)%xalign);
        if(pinfo->m_bits_per_pixel == 8)
        {
            {
                AlignedWidth = ((crop_actv_win->width + plane->m_leftpadadjust_c + 63)/64)*64;
                plane->m_rightpad_c = AlignedWidth - (crop_actv_win->width + plane->m_leftpadadjust_c);
                plane->m_Aligned_Chroma_Width = AlignedWidth;
            }
        }
        else if(pinfo->m_bits_per_pixel == 10)
        {
            {
                AlignedWidth = ((crop_actv_win->width + plane->m_leftpadadjust_c + 47)/48)*48;
                plane->m_rightpad_c = AlignedWidth - (crop_actv_win->width + plane->m_leftpadadjust_c);
                plane->m_Aligned_Chroma_Width = AlignedWidth;
            }
        }
    }

    /* vertical cropping */
    if (crop_actv_win->y < 0) {
        /* crop top side */
        tmp = -crop_actv_win->y;
        tmp -= tmp%dataAlign_y;
#ifdef CONFIG_AMP_VPP_ROTATION
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
            *crop_actv_offset -=
                (-tmp * ((int)(pinfo->m_buf_stride_UV/VPP_TILED_MODE_STRIDE_FACTOR)));
        else
            *crop_actv_offset -= (-tmp * ((int)pinfo->m_buf_stride_UV));
        VDBG_IF_DISP("y:crop_actv_offset_uv : 0x%x, y:%d, bits_per_pixels= %d, pixel = %d\n",
                *crop_actv_offset, crop_actv_win->y, (int)(bytesPerPixel*8),
                (int)((*crop_actv_offset)/(bytesPerPixel)));
        crop_actv_win->height += crop_actv_win->y;
        crop_actv_win->y = 0;
    }

    if ((crop_actv_win->y + crop_actv_win->height) > (ref_win->height/2)) {
        /* crop bottom side */
        crop_actv_win->height -= (crop_actv_win->y + crop_actv_win->height - (ref_win->height/2));
    }

    if (crop_actv_win->height <= 0)
        return;

    crop_scl_actv_win->y = crop_actv_win->y * disp_win->height / ref_win->height + disp_win->y;
    crop_scl_actv_win->height = crop_actv_win->height * disp_win->height / ref_win->height;

    /* vertical cropping of display content */
    if (crop_scl_actv_win->y < 0) {
        /* crop top side */
        tmp = crop_scl_actv_win->y * ref_win->height / disp_win->height;
        crop_actv_win->height += tmp;
#ifdef CONFIG_AMP_VPP_ROTATION
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if(pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
        {
            if(plane->m_SampleCropEnable || plane->m_SampleCropEnable_el)
            {
                mtr_pan_scan_y_offset  = (-tmp)/dataAlign_y;
                mtr_pan_scan_y_crop = (-tmp)%dataAlign_y;
            }
            else
            {
                *crop_actv_offset -= tmp * ((int)(pinfo->m_buf_stride_UV/VPP_TILED_MODE_STRIDE_FACTOR));
            }
        }
        else
        {
            *crop_actv_offset -= tmp * ((int)pinfo->m_buf_stride_UV);
        }
        VDBG_IF_DISP("y:crop_actv_offset_uv : 0x%x, y:%d, bits_per_pixels= %d, pixel = %d\n",
                *crop_actv_offset, crop_actv_win->y, (int)(bytesPerPixel*8),
                (int)((*crop_actv_offset)/(bytesPerPixel)));
        crop_scl_actv_win->height += crop_scl_actv_win->y;
        crop_scl_actv_win->y = 0;
    }
    if ((crop_scl_actv_win->y + crop_scl_actv_win->height) >
            (cpcb_resinfo->active_height / uiInteraceFactor)) {
        /* crop bottom side */
        tmp = crop_scl_actv_win->y + crop_scl_actv_win->height -
            (cpcb_resinfo->active_height / uiInteraceFactor);
        crop_actv_win->height -= tmp * ref_win->height / disp_win->height;
        crop_scl_actv_win->height -= tmp;
    }

    if((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt) &&
            (plane->m_SampleCropEnable || plane->m_SampleCropEnable_el))
    {
        int yalign = 0;
        int temp = -(pinfo->m_active_top/2);
        int AlignedHeight = 0;
        if(temp < 0)
            temp = -temp;
        yalign = dataAlign_y;

        if(!plane->m_SeparateElcropRequired)
        {
            plane->uDhubCropOffset_c.u32_Tile_Yoffset =
                mtr_pan_scan_y_offset + ((temp + plane->m_toppad_c)/yalign);
        }
        else
        {
            plane->el_uDhubCropOffset_c.u32_Tile_Yoffset =
                mtr_pan_scan_y_offset + ((temp + plane->m_toppad_c)/yalign);
        }
        plane->m_toppadadjust_c = ((temp+mtr_pan_scan_y_crop)%yalign);
        AlignedHeight = (((crop_actv_win->height) + plane->m_toppadadjust_c + 3)/4)*4;
        plane->m_bottompad_c = AlignedHeight - ((crop_actv_win->height) + plane->m_toppadadjust_c);
        plane->m_Aligned_Chroma_Height = AlignedHeight;
    }

    if((plane->m_SeparateElcropRequired)&&(plane->m_SampleCropEnable_el)&&(plane->m_mtrR__en_el))
    {
        plane->m_El_Aligned_Chroma_Width = plane->m_Aligned_Chroma_Width;
        plane->m_El_Aligned_Chroma_Height = plane->m_Aligned_Chroma_Height;
        plane->m_Aligned_Chroma_Width = BL_Width;
        plane->m_Aligned_Chroma_Height = BL_Height;
        plane->m_leftpad_c = BL_LeftPad;
        plane->m_toppad_c = BL_TopPad;
    }

    ref_win->width = ref_win_backup_width;
    ref_win->height = ref_win_backup_height;
    ref_win->x = ref_win_backup_x;
    ref_win->y = ref_win_backup_y;

    VDBG_IF_DISP_WIN(crop_actv_win);
    VDBG_IF_DISP_WIN(crop_scl_actv_win);
    VDBG_IF_DISP("crop_actv_offset_uv : 0x%x, y:%d, bits_per_pixels= %d, pixel = %d\n",
            *crop_actv_offset, crop_actv_win->y, (int)(bytesPerPixel*8),
            (int)((*crop_actv_offset)/(bytesPerPixel)));

    return;
}

/* Calclute the virtual stride(MTR stride) from DMA stride */
UINT32 vppMtrR_ConfigStride(UINT8 *semId, UINT32 stride)
{
    int mtrRStride = 0;
    int val;

    /* Min MTR stride is 2^12. So discarding the same,
       which is not required to set for MTR stride */
    val = (stride - 1 ) >> 12;
    while(val > 0)
    {
        mtrRStride++;
        val = val >> 1;
    }

    /* MTR stide is from 2-4 bit position */
    *semId &= (~0x1C);
    if(mtrRStride < 5)
        *semId |= (mtrRStride << 2);
    else
        return MV_VPP_EBADPARAM;
    return MV_VPP_OK;
}

/* To set mtu size and inverse mode  for each a given channel */
static int AioSetDhubChannelCfg(VPP_OBJ *vpp_obj, INT dmaId, UINT8 uchInverseScanMode, SIGN32 mtuByte)
{
    HDL_dhub *dhub = &AG_dhubHandle.dhub;
    UINT32 RegAddr;
    T32dHubChannel_CFG  cfg;
    UINT32 i = 0;

    switch(mtuByte)
    {
        case dHubChannel_CFG_MTU_8byte  : dhub->MTUb[dmaId] = 3; break;
        case dHubChannel_CFG_MTU_32byte : dhub->MTUb[dmaId] = 5; break;
        case dHubChannel_CFG_MTU_128byte: dhub->MTUb[dmaId] = 7; break;
        case dHubChannel_CFG_MTU_256byte: dhub->MTUb[dmaId] = 8; break;
        case dHubChannel_CFG_MTU_1024byte : dhub->MTUb[dmaId] = 10; break;
        case dHubChannel_CFG_MTU_2048byte : dhub->MTUb[dmaId] = 11; break;
        case dHubChannel_CFG_MTU_4096byte : dhub->MTUb[dmaId] = 12; break;
    }
    for(i=0; i<VPP_NUM_OF_CHANNELS; i++)
    {
        if(VPP_config[i].chanId == dmaId)
            break;
    }
    if(VPP_NUM_OF_CHANNELS == i)
    {
        VPPAPI_DBG_ERROR("%s %d  >> ERROR :: Invalid dmaId \n", __func__, __LINE__);
        return MV_VPP_EBADPARAM;
    }
    cfg.u32 = 0;
    cfg.uCFG_MTU = mtuByte;
    cfg.uCFG_QoS = AG_config[i].chanQos;
    cfg.uCFG_selfLoop = AG_config[i].chanSelfLoop;
    switch(uchInverseScanMode)
    {
        case 0:
            //Normal Scanning
            cfg.uCFG_hScan = 0; cfg.uCFG_vScan = 0;
            break;
        case 1:
            //Only H inverse scan
            cfg.uCFG_hScan = 1;
            cfg.uCFG_vScan = 0;
            break;
        case 2:
            //Only V inverse Scan
            cfg.uCFG_hScan = 0;
            cfg.uCFG_vScan = 1;
            break;
        case 3:
            //Both HV inverse
            cfg.uCFG_hScan = 1; cfg.uCFG_vScan = 1;
            break;
        default :
            return MV_VPP_EBADPARAM;
    }
    RegAddr = dhub->ra + RA_dHubReg_ARR + dmaId*sizeof(SIE_dHubChannel);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, cfg.u32);
    AG_config[i].chanMtuSize = mtuByte;

    return MV_VPP_OK;
}

/* To set mtu size and inverse mode  for each a given channel */
int vppSetDhubChannelCfg(VPP_OBJ *vpp_obj, INT dmaId, UINT8 uchInverseScanMode, SIGN32 mtuByte)
{
    HDL_dhub *dhub = &VPP_dhubHandle.dhub;
    UINT32 RegAddr;
    T32dHubChannel_CFG  cfg;
    UINT32 i = 0;

    switch(mtuByte)
    {
        case dHubChannel_CFG_MTU_8byte  : dhub->MTUb[dmaId] = 3; break;
        case dHubChannel_CFG_MTU_32byte : dhub->MTUb[dmaId] = 5; break;
        case dHubChannel_CFG_MTU_128byte: dhub->MTUb[dmaId] = 7; break;
        case dHubChannel_CFG_MTU_256byte: dhub->MTUb[dmaId] = 8; break;
        case dHubChannel_CFG_MTU_1024byte : dhub->MTUb[dmaId] = 10; break;
        case dHubChannel_CFG_MTU_2048byte : dhub->MTUb[dmaId] = 11; break;
        case dHubChannel_CFG_MTU_4096byte : dhub->MTUb[dmaId] = 12; break;
    }
    for(i=0; i<VPP_NUM_OF_CHANNELS; i++)
    {
        if(VPP_config[i].chanId == dmaId)
            break;
    }
    if(VPP_NUM_OF_CHANNELS == i)
    {
        VPPAPI_DBG_ERROR("%s %d  >> ERROR :: Invalid dmaId \n", __func__, __LINE__);
        return MV_VPP_EBADPARAM;
    }
    cfg.u32 = 0;
    cfg.uCFG_MTU = mtuByte;
    cfg.uCFG_QoS = VPP_config[i].chanQos;
    cfg.uCFG_selfLoop = VPP_config[i].chanSelfLoop;
    switch(uchInverseScanMode)
    {
        case 0:
            //Normal Scanning
            cfg.uCFG_hScan = 0; cfg.uCFG_vScan = 0;
            break;
        case 1:
            //Only H inverse scan
            cfg.uCFG_hScan = 1;
            cfg.uCFG_vScan = 0;
            break;
        case 2:
            //Only V inverse Scan
            cfg.uCFG_hScan = 0;
            cfg.uCFG_vScan = 1;
            break;
        case 3:
            //Both HV inverse
            cfg.uCFG_hScan = 1; cfg.uCFG_vScan = 1;
            break;
        default :
            return MV_VPP_EBADPARAM;
    }
    RegAddr = dhub->ra + RA_dHubReg_ARR + dmaId*sizeof(SIE_dHubChannel);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, cfg.u32);
    VPP_config[i].chanMtuSize = mtuByte;

    return MV_VPP_OK;
}


/* calculate 2NDDMA params to support VPP_DHUB_BURST_OPT */
UNSG32 vppDhubCfg2NDDMA(VPP_OBJ *vpp_obj, VPP_DHUB_CFG_2NDDMA_PARAM *pstCfg2NDDMAParam)
{
    SIGN32 burst, step1, size1, step2, size2;
    SIGN32 mtu;
    UNSG32 RetVal;
    UNSG32 addr;

    if ((0 == pstCfg2NDDMAParam->plane->uchInverseScanMode   || \
          2 == pstCfg2NDDMAParam->plane->uchInverseScanMode) && \
          VPP_IS_MULTIPLE_OF_BURST(pstCfg2NDDMAParam->width) && \
             (!(IS_MMU_ONLY_ENABLED(pstCfg2NDDMAParam->mtrREn)) || \
                (IS_MMU_ONLY_ENABLED(pstCfg2NDDMAParam->mtrREn) && \
                VPP_IS_STRIDE_ALIGN_TO_BURST(pstCfg2NDDMAParam->stride))))
    {
        /* Use VPP_IS_STRIDE_ALIGN_TO_BURST  to check stride multiple of
         * 1024byte in MMU only case.
         * if width is multiple of VPP_DHUB_BURST_OPT. This is a must for compressed input
         * H flip and HV flip can't support 256B burst */
        burst = (AVIO_DHUB_BURST_BW_OPT > 0) ? AVIO_DHUB_BURST_BW_OPT: pstCfg2NDDMAParam->width;
        mtu   = AVIO_DHUB_MTU_OPT;
    }
    else if (VPP_IS_MULTIPLE_OF_MMU_BURST(pstCfg2NDDMAParam->width))
    {
        /* MMU only enabled case(eg. 420SP on BG5CTpZ1),
           width should be multiple of VPP_DHUB_BURST_MMU_ONLY */
        /* For non-MTR, MMU enabled cases burst size need to be VPP_DHUB_BURST_MMU_ONLY. Otherwise
         * horizontal lines are observed in dolphin */
        burst = VPP_DHUB_BURST_MMU_ONLY;
        mtu   = AVIO_DHUB_MTU_MMU_ONLY;
    }
    else
    {
      /* Overlayed horizontal line on top of video is seen when MTU > 128 and
      * burst is not multiple of MTU */
        burst = pstCfg2NDDMAParam->width;
        mtu   = dHubChannel_CFG_MTU_128byte;
    }

    if(pstCfg2NDDMAParam->mtrREn)
    {

        if((pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_MV_R0)
#ifdef ENABLE_PIP_GFX0_PATH
            || (pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_GFX_PIP_R0)
#endif
        )
        {
            // Luma addr calulation.
            addr  = VPP_AVIOMTR_DHUB_ADDR +
                ( (pstCfg2NDDMAParam->plane->m_toppad_y /
                   pstCfg2NDDMAParam->plane->tile.height) * VPP_AVIOMTR_DHUB_STRIDE );
        }
        else if((pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_MV_R1)
#ifdef ENABLE_PIP_GFX0_PATH
            || (pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_PIP_R1)
#endif
        )
        {
            // Chroma addr calulation.
            addr  = VPP_AVIOMTR_DHUB_ADDR +
                ((pstCfg2NDDMAParam->plane->m_toppad_c /pstCfg2NDDMAParam->plane->tile.height) *
                 VPP_AVIOMTR_DHUB_STRIDE );
        }
        else
        {
            addr  = VPP_AVIOMTR_DHUB_ADDR;
        }

        step2 = VPP_AVIOMTR_DHUB_STRIDE * pstCfg2NDDMAParam->plane->lineDroppingStrides;
    }
    else
    {
        addr  = pstCfg2NDDMAParam->addr;
        step2 = pstCfg2NDDMAParam->stride;
    }
    step1 = burst;
    size1 = pstCfg2NDDMAParam->width / burst;
    size2 = pstCfg2NDDMAParam->height;

    if(pstCfg2NDDMAParam->plane->chanID == PLANE_GFX2)
    {
        AioSetDhubChannelCfg(vpp_obj, pstCfg2NDDMAParam->dmaId,
                pstCfg2NDDMAParam->plane->uchInverseScanMode, mtu);
    }
    else
        vppSetDhubChannelCfg(vpp_obj, pstCfg2NDDMAParam->dmaId,
                pstCfg2NDDMAParam->plane->uchInverseScanMode, mtu);


    if(0 == pstCfg2NDDMAParam->mtrREn)
    {
        RetVal =  START_2NDDMA(pstCfg2NDDMAParam->dhubId, pstCfg2NDDMAParam->dmaId,
                addr, burst, step1, size1, step2, size2, (T64b*)pstCfg2NDDMAParam->cfgQ);
    }
    else
    {
        RetVal =  START_2NDDMA_MTRR(pstCfg2NDDMAParam->dhubId,
                pstCfg2NDDMAParam->dmaId, addr, burst, step1, size1, step2, size2,
                pstCfg2NDDMAParam->chkSemId, pstCfg2NDDMAParam->updSemId,
                (T64b*)pstCfg2NDDMAParam->cfgQ);
    }
    return RetVal;
}


#ifdef CONFIG_DEINT_ENABLE
static void updateVPSize(VPP_OBJ *vpp_obj, PLANE *plane, VPP_FE_DLR_INPUT_RES *feRes)
{
    int input_type;
    VPP_VP_CLIENT_SIZE vp_size;
    UINT32 TotalInpPix = 0;
    UINT32 RegAddr = 0;
    if(ROTATION_0D == plane->uchRotationAngle)
    {
        vp_size.ihRes = plane->crop_actv_win.width +
            vpp_obj->scl_ctrl[PLANE_MAIN].leftCrop + vpp_obj->scl_ctrl[PLANE_MAIN].rightCrop;

        if (plane->frame_type) { // progressive frame
            vp_size.ivRes = plane->crop_actv_win.height;
            vp_size.IsProgressive = 1;
        } else {
            vp_size.ivRes = plane->crop_actv_win.height / 2;
            vp_size.IsProgressive = 0;
        }
    }
    else
    {
        vp_size.ihRes = feRes->HRes;

        if (plane->frame_type) { // progressive frame
            vp_size.ivRes = feRes->VRes;
            vp_size.IsProgressive = 1;
        } else {
            vp_size.ivRes = feRes->VRes/2;
            vp_size.IsProgressive = 0;
        }
    }
    if (!plane->frame_type) { // interlaced input
        FE_DEINT_SetVpDeintClientSize(vpp_obj, &vp_size, 0/* no training required */);
    }
    else
    {
        TotalInpPix = vp_size.ivRes * vp_size.ihRes;
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, TotalInpPix);
    }
    /* update De-Interlacer input parameters */
    if (plane->crop_actv_win.width <= 750) {
        if (plane->crop_actv_win.height <= 500)
            input_type = VPP_DEINT_INPUT_SD_NTSC; /* SD NTSC input */
        else
            input_type = VPP_DEINT_INPUT_SD_PAL; /* SD PAL input */
    } else {
        input_type = VPP_DEINT_INPUT_HD; /* HD input */
    }
    FE_DEINT_SetInputParams(vpp_obj, input_type, NULL);
}
#else
static void updateVPSize(VPP_OBJ *vpp_obj, PLANE *plane, VPP_FE_DLR_INPUT_RES *feRes)
{
    VPP_VP_CLIENT_SIZE vp_size;
    UINT32 TotalInpPix = 0;
    UINT32 RegAddr = 0;

    if(ROTATION_0D == plane->uchRotationAngle)
    {
        vp_size.ihRes = plane->crop_actv_win.width +
            vpp_obj->scl_ctrl[PLANE_MAIN].leftCrop + vpp_obj->scl_ctrl[PLANE_MAIN].rightCrop;

        if (plane->frame_type) { // progressive frame
            vp_size.ivRes = plane->crop_actv_win.height;
            vp_size.IsProgressive = 1;
        } else {
            vp_size.ivRes = plane->crop_actv_win.height / 2;
            vp_size.IsProgressive = 0;
        }
    }
    else
    {
        vp_size.ihRes = feRes->HRes;
        if (plane->frame_type) { // progressive frame
            vp_size.ivRes = feRes->VRes;
            vp_size.IsProgressive = 1;
        } else {
            vp_size.ivRes = feRes->VRes/2;
            vp_size.IsProgressive = 0;
        }
    }
    TotalInpPix = vp_size.ivRes * vp_size.ihRes;
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, TotalInpPix);
}
#endif

static void updateVPSetting(VPP_OBJ *vpp_obj, PLANE *plane)
{
#ifdef CONFIG_DEINT_ENABLE
    int vp_mode;
    VPP_VP_CLIENT_CLK vp_clk;
    VPP_DINT_PRESET_MODE_PARAMS md_params;
    GaloisMemClear(&md_params, sizeof(VPP_DINT_PRESET_MODE_PARAMS));

    //    VPPAPI_DBG_INFO("VP setting frame type: %d\n", plane->frame_type);
    if (plane->frame_type) { // progressive frame
        vp_clk.field.ClkEnable = 0; // for progressive input
        vp_clk.field.WriteEnable = 0;
        vp_clk.field.Read0Enable = 0;
        vp_clk.field.Read1Enable = 0;
        plane->vp_buff_num = 0; /* no field buffer needed */
        if (plane->status != STATUS_DISP_LOGO)
            vp_mode = VPP_DEINTPRESET_BYPASS_AND_FLD_BUFF_ENABLED;
        else
            vp_mode = VPP_DEINTPRESET_BYPASS_AND_FLD_BUFF_DISABLED;
        FE_DEINT_SetPresetMode(vpp_obj, vp_mode, &md_params);
    } else { // interlaced frame

        /* update VP clock, size & buffer mode setting */
        if (plane->deint_mode == DEINT_3D) {
            vp_clk.field.ClkEnable = 1; // for interlaced input
            vp_clk.field.WriteEnable = 1;
            vp_clk.field.Read0Enable = 1;
            vp_clk.field.Read1Enable = 1;
            plane->vp_buff_num = 3; /* 3 field buffers needed */
            plane->vp_buffs[0] = (void *)plane->vp_buff_base;
            plane->vp_buffs[1] = (void *)(plane->vp_buff_base + DEINT_BUFFER_SIZE/6);
            plane->vp_buffs[2] = (void *)(plane->vp_buff_base + DEINT_BUFFER_SIZE*2/6);
            plane->vp_buffs_right[0] = (void *)(plane->vp_buff_base + DEINT_BUFFER_SIZE*3/6);
            plane->vp_buffs_right[1] = (void *)(plane->vp_buff_base + DEINT_BUFFER_SIZE*4/6);
            plane->vp_buffs_right[2] = (void *)(plane->vp_buff_base + DEINT_BUFFER_SIZE*5/6);
            // if (plane->vp_buff_r1_index == -1)
            {
                plane->vp_buff_r0_index = 0; /* current field */
                plane->vp_buff_w_index = 1; /* next field */
                plane->vp_buff_r1_index = 2; /* previous field */
            }

            VPPAPI_DBG_INFO("Deint buff addresses are %p. %p. %p\n", plane->vp_buffs[0], plane->vp_buffs[1], plane->vp_buffs[2]);
        } else {
            vp_clk.field.ClkEnable = 1;
            vp_clk.field.WriteEnable = 0;
            vp_clk.field.Read0Enable = 0;
            vp_clk.field.Read1Enable = 0;
            plane->vp_buff_num = 0; /* no field buffer needed */
        }

        md_params.ChrMotVidMode = plane->DeintPreset.ChrMotVidMode;
        //VPP_DEINTCHROMA_MOT_MAX_OF_CHROMA_AND_LUMA_MOT;
        md_params.ViMode = plane->DeintPreset.ViMode;//VPP_DEINTMODE_SAFE;
        md_params.ChrMotFlmMode = plane->DeintPreset.ChrMotFlmMode; //TRUE
        /* update de-interlacer setting */
        if (plane->deint_mode == DEINT_3D) { // 3D De-Interlacer
            //    md_params.ChrMotFlmMode = TRUE;
            vp_mode = VPP_DEINTPRESET_3D_MA_VECTOR;
        } else { // 2D De-Interlacer
            //md_params.ChrMotVidMode = VPP_DEINTCHROMA_MOT_MAX_OF_CHROMA_AND_LUMA_MOT;
            md_params.ChrMotFlmMode = FALSE;
            //md_params.ViMode = VPP_DEINTMODE_SAFE;
            vp_mode = VPP_DEINTPRESET_2D_VECTOR;
        }

        md_params.FieldPolarity = 0;
        FE_DEINT_SetPresetMode(vpp_obj, vp_mode, &md_params);
    }

    /* updata VP/De-Interlacer setting */
    FE_DEINT_SetVpDeintClientClk(vpp_obj, &vp_clk);
#endif
}

static void handleVPBuffer(VPP_OBJ *vpp_obj, PLANE *plane)
{
#ifdef CONFIG_DEINT_ENABLE
    UNSG32 size, buf_w_addr, buf_r0_addr, buf_r1_addr;
#if defined(VPP_DEBUG)
    int dhub_status;
#endif
    int cpcbID = CPCB_OF_CHAN(vpp_obj, plane->chanID);
    int *cfgQ;
    int *cfgQ_shadow = vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ->addr;
    int *cfgQ_len = &(vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ->len);
    VPP_DHUB_CFG_2NDDMA_PARAM stCfg2NDDMAParam;
    unsigned int uiWidth = 0, uiStride = 0, uiHeight = 0, uiSclInHres = 0;
    RESOLUTION_INFO  *pResinfo;

    pResinfo = VPP_GetResInfo(VPP_GetResId(cpcbID));
    if (plane->vp_buff_num == 2) { /* 2 buffer system */

    } else { /* 3 buffer system */

#if defined(REPEAT_FRAME_ENHANCE)
        if (!vpp_obj->main_frame_repeat) {
#endif
#ifdef PE_3D_FMT_CONVERT
            if ((!pResinfo->flag_3d) || (vpp_obj->dv[cpcbID].viewID==0)) {
#endif
                /* update VP read/write buffer pointers */
                plane->vp_buff_w_index = (plane->vp_buff_w_index + 1) % plane->vp_buff_num;
                plane->vp_buff_r0_index = (plane->vp_buff_r0_index + 1) % plane->vp_buff_num;
                plane->vp_buff_r1_index = (plane->vp_buff_r1_index + 1) % plane->vp_buff_num;
#ifdef PE_3D_FMT_CONVERT
            }
#endif
#if defined(REPEAT_FRAME_ENHANCE)
        }
        if(vpp_obj->main_frame_repeat ==2) vpp_obj->main_frame_repeat = 0;
#endif

#ifdef PE_3D_FMT_CONVERT
        if ((pResinfo->flag_3d) && (vpp_obj->dv[cpcbID].viewID==1)) {
            buf_w_addr = (UNSG32)plane->vp_buffs_right[plane->vp_buff_w_index];
            buf_r0_addr = (UNSG32)plane->vp_buffs_right[plane->vp_buff_r0_index];
            buf_r1_addr = (UNSG32)plane->vp_buffs_right[plane->vp_buff_r1_index];
        } else

#endif
        {
            buf_w_addr = (UNSG32)plane->vp_buffs[plane->vp_buff_w_index];
            buf_r0_addr = (UNSG32)plane->vp_buffs[plane->vp_buff_r0_index];
            buf_r1_addr = (UNSG32)plane->vp_buffs[plane->vp_buff_r1_index];
        }
        /* start VP read/write buffer DMA */
        size = plane->crop_actv_win.width * (plane->crop_actv_win.height / 2 + 1);
        size  = (size * (((VP_FB_CTRL_BITS *)
                        (&vpp_obj->fe.VpFbCtrl))->Bits.DeintClientMode ? 26 : 20)+ 7) / 8;

        FE_DEINT_VpClient_Clear(vpp_obj, VPP_VP_CLIENT_WRITE);
        FE_DEINT_VpClient_Clear(vpp_obj, VPP_VP_CLIENT_READ0);
        FE_DEINT_VpClient_Clear(vpp_obj, VPP_VP_CLIENT_READ1);

        uiSclInHres = plane->crop_actv_win.width +
            vpp_obj->scl_ctrl[PLANE_MAIN].leftCrop + vpp_obj->scl_ctrl[PLANE_MAIN].rightCrop;
        uiWidth = (uiSclInHres * (((VP_FB_CTRL_BITS *)
                        (&vpp_obj->fe.VpFbCtrl))->Bits.DeintClientMode ? 26 : 20) + 7) / 8;
        uiStride = (((uiSclInHres * 20) / 8 + 127) / 128) * 128;
        uiHeight = plane->crop_actv_win.height / 2 + 1;

        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
        *cfgQ_len += START_2DDMA(plane->deint_dmaWdhubID, plane->deint_dmaWID,
                buf_w_addr, uiStride, uiWidth, uiHeight, (T64b*)cfgQ);

        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
        stCfg2NDDMAParam.dhubId = (INT)plane->deint_dmaR0dhubID;
        stCfg2NDDMAParam.dmaId  = plane->deint_dmaR0ID;
        stCfg2NDDMAParam.addr   = buf_r0_addr;
        stCfg2NDDMAParam.width  = uiWidth;
        stCfg2NDDMAParam.stride = uiStride;
        stCfg2NDDMAParam.height = uiHeight;
        stCfg2NDDMAParam.cfgQ   = cfgQ;
        stCfg2NDDMAParam.mtrREn = 0;
        stCfg2NDDMAParam.plane  = plane;
        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
        stCfg2NDDMAParam.dhubId = (INT)plane->deint_dmaR1dhubID;
        stCfg2NDDMAParam.dmaId  = plane->deint_dmaR1ID;
        stCfg2NDDMAParam.addr   = buf_r1_addr;
        stCfg2NDDMAParam.width  = uiWidth;
        stCfg2NDDMAParam.stride = uiStride;
        stCfg2NDDMAParam.height = uiHeight;
        stCfg2NDDMAParam.cfgQ   = cfgQ;
        stCfg2NDDMAParam.mtrREn = 0;
        stCfg2NDDMAParam.plane  = plane;
        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

        FE_DEINT_VpClient_Enable(vpp_obj, VPP_VP_CLIENT_WRITE, 1);
        FE_DEINT_VpClient_Enable(vpp_obj, VPP_VP_CLIENT_READ0, 1);
        FE_DEINT_VpClient_Enable(vpp_obj, VPP_VP_CLIENT_READ1, 1);


#ifdef FIXME_IN_BG4CT
        /* start VP read/write clients */
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (vpp_obj->base_addr + RA_Vpp_DIW_CLIENT), 1);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (vpp_obj->base_addr + RA_Vpp_DIR0_CLIENT), 1);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (vpp_obj->base_addr + RA_Vpp_DIR1_CLIENT), 1);
#endif
    }
#endif
}

int VPP_VIP_OTG_SetTGParams(VPP_OBJ *vpp_obj, UINT32 uiWidth, UINT32 uiHeight)
{
    T32TG_SIZE tg_size;
    T32TG_HB tg_hb;
    T32TG_HB_CR tg_hb_cr;
    T32TG_HB_CR2 tg_hb_cr2;
    T32TG_VB0 tg_vb0;
    T32TG_VB0_CR tg_vb0_cr;
    T32TG_VB0_CR2 tg_vb0_cr2;

    tg_size.uSIZE_Y = uiHeight + 6;
    tg_size.uSIZE_X = uiWidth + 40;
    tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = uiWidth + 8;
    tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
    tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = uiHeight + 1;
    tg_vb0.uVB0_BE = tg_vb0_cr.uVB0_CR_BE = 0;
    tg_vb0_cr2.uVB0_CR2_BE = 2;

    //Config VIP OTG
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_OTG +
            RA_TG_SIZE, tg_size.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_OTG +
            RA_TG_HB, tg_hb.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_OTG +
            RA_TG_HB_CR, tg_hb_cr.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_OTG +
            RA_TG_HB_CR2, tg_hb_cr2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_OTG +
            RA_TG_VB0, tg_vb0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_OTG +
            RA_TG_VB0_CR, tg_vb0_cr.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_OTG +
            RA_TG_VB0_CR2, tg_vb0_cr2.u32);
    return (MV_VPP_OK);
}

/* set FETG parameters */
int VPP_VIP_TG_SetTGParams(VPP_OBJ *vpp_obj, UINT32 uiWidth, UINT32 uiHeight)
{
    T32TG_SIZE tg_size;
    T32TG_HB tg_hb;
    T32TG_HB_CR tg_hb_cr;
    T32TG_HB_CR2 tg_hb_cr2;
    T32TG_VB0 tg_vb0;
    T32TG_VB0_CR tg_vb0_cr;
    T32TG_VB0_CR2 tg_vb0_cr2;
    PLANE *plane = NULL;

    plane = &vpp_obj->plane[PLANE_MAIN]; /* FIXME: Remove hardcoded plane ID to MAIN*/
    if(VPP_SCL_MODE_INLINE != plane->scl_mode)
    {
        uiHeight = VPP_OFFLINESCL_MAX(vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IVRes,
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OVRes);
        uiWidth = VPP_OFFLINESCL_MAX(vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IHRes,
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OHRes);
        tg_size.uSIZE_Y = uiHeight + 6;
        tg_size.uSIZE_X = uiWidth + 60;
    }
    else
    {
        tg_size.uSIZE_Y = uiHeight + 6;
        tg_size.uSIZE_X = uiWidth + 50; /*  Changed from 60 to 50 to fix 4K60 DV dump issue  */
    }
    tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = uiWidth + 8;
    tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
    tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = uiHeight + 3;
    tg_vb0.uVB0_BE = tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 2;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_TG +
            RA_TG_SIZE, tg_size.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_TG +
            RA_TG_HB, tg_hb.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_TG +
            RA_TG_HB_CR, tg_hb_cr.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_TG +
            RA_TG_HB_CR2, tg_hb_cr2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_TG +
            RA_TG_VB0, tg_vb0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_TG +
            RA_TG_VB0_CR, tg_vb0_cr.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, MEMMAP_AVIO_REG_BASE +
            AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_VIP_TG +
            RA_TG_VB0_CR2, tg_vb0_cr2.u32);

    /* Config VIP OTG */
    if(VPP_SCL_MODE_INLINE != plane->scl_mode)
    {
        VPP_VIP_OTG_SetTGParams(vpp_obj, vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OHRes,
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OVRes);
    }
    else
    {
        VPP_VIP_OTG_SetTGParams(vpp_obj, uiWidth, uiHeight);
    }

    return (MV_VPP_OK);


}

int startVppWriteBackClients(VPP_OBJ *vpp_obj, UINT32 uiMode, UINT32 uiWidth, UINT32 uiHeight)
{
    UINT32 uiRegAddr = 0, uiUvClientValid = 0, uiBuffAddr_Y = 0, uiBuffAddr_UV = 0;
    INT32 iStrideInBytes = 0, iWidthInBytes = 0, iDhubId = 0, iChanId_Y = 0, iChanId_UV = 0;
    PLANE *plane;
#ifndef VPP_OFFLINE_NONSTDRESWR_DISABLE
    T32VIP_WriteClient_NonStdRes stVipWriteClientNonStdRes;
    stVipWriteClientNonStdRes.u32 =0;
    OFFLINE_BUFF_DESCR *pOfflineBuffDescr = NULL;
#endif
    T32VIPSCLTOP_CTRL0 stVipSclTopCtrl0;
    VPP_DHUB_CFG_2NDDMA_PARAM stCfg2NDDMAParam;
#ifdef CONFIG_AMP_DOLBY_DUMP
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    UINT32 uiHdmiVidFmt = pHdmiTx->videoParams.VideoFmt;
    UINT32 uiColorDepth = pHdmiTx->videoParams.ColorDepth;
#endif //CONFIG_AMP_DOLBY_DUMP
    plane = &(vpp_obj->plane[PLANE_MAIN]);

    int *cfgQ;
    int *cfgQ_shadow = vpp_obj->dv[CPCB_1].curr_cpcb_vbi_dma_cfgQ->addr;
    int *cfgQ_len = &(vpp_obj->dv[CPCB_1].curr_cpcb_vbi_dma_cfgQ->len);

    iDhubId = (INT32)vpp_obj->plane[PLANE_MAIN].dmaRdhubID;
    iChanId_Y = avioDhubChMap_vpp128b_1DSCL_W0;
    iChanId_UV = avioDhubChMap_vpp128b_1DSCL_W1;

#ifdef CONFIG_AMP_DOLBY_DUMP
    //Get the base address for the current frame
    VDEC_VID_BUF_DESC *frm_descr = plane->curr_frame;
#ifndef CONFIG_AMP_TEE_ENABLE
    VBUF_INFO *pinfo = (VBUF_INFO *)frm_descr->user_data;
#else
    VBUF_INFO *pinfo = (VBUF_INFO *)frm_descr;
#endif

    //Please don't get new frame if it is built in frame; use offline scalar buffer for built-in frame
    if(!pinfo->builtinFrame) {
        uiBuffAddr_Y = VPP_DV_FrameDump_GetCurrFrame(vpp_obj);
    } else {
        uiBuffAddr_Y = (UINT32)plane->offline_buff_descrs[0].addr;
    }
#endif //CONFIG_AMP_DOLBY_DUMP

    switch(uiMode)
    {
#if defined (CONFIG_AMP_DOLBY_DUMP)
        case VPP_WRITEBACK_MODE_DISABLE:
            {
            }
            break;
        case VPP_WRITEBACK_MODE_DV_COMP:
            {
                iWidthInBytes = uiWidth * 2;
                iStrideInBytes = iWidthInBytes;
                uiBuffAddr_UV =  uiBuffAddr_Y + iStrideInBytes * uiHeight;
                plane->wb_buff_index = 0;

                uiUvClientValid = 1;
            }
            break;
        case VPP_WRITEBACK_MODE_EDR:
            {
                if(uiHdmiVidFmt == OUTPUT_COLOR_FMT_YCBCR420)
                {
                    VPPAPI_DBG_ERROR("OUTPUT_COLOR_FMT_YCBCR420 capture not supported..!!!\n");
                    return 0;
                }
                plane->wb_buff_index = 0;
                iWidthInBytes = uiWidth * 3; //12-bit YUV422
                iStrideInBytes = iWidthInBytes;
                plane->wb_buff_index = (plane->wb_buff_index + 1) & 0x01;
                uiUvClientValid = 0;
            }
            break;
        case VPP_WRITEBACK_MODE_DITHER:
            {
                plane->wb_buff_index = 0;
                if(uiHdmiVidFmt == OUTPUT_COLOR_FMT_YCBCR422)
                {
                    if(uiColorDepth == OUTPUT_BIT_DEPTH_12BIT)
                        iWidthInBytes = (uiWidth * 3);
                    else if(uiColorDepth == OUTPUT_BIT_DEPTH_10BIT)
                        iWidthInBytes = (uiWidth * 20)/8; //10-bit YUV422
                    else
                        iWidthInBytes = (uiWidth * 16)/8; //8-bit YUV422
                }
                else if((uiHdmiVidFmt == OUTPUT_COLOR_FMT_YCBCR444) ||
                        (uiHdmiVidFmt == OUTPUT_COLOR_FMT_RGB888))
                {
                    //YUV444
                    if(uiColorDepth == OUTPUT_BIT_DEPTH_12BIT)
                        iWidthInBytes = (uiWidth * 36)/8; //12-bit YUV444
                    else if(uiColorDepth == OUTPUT_BIT_DEPTH_10BIT)
                        iWidthInBytes = (uiWidth * 30)/8; //10-bit YUV444
                    else
                        iWidthInBytes = (uiWidth * 24)/8; //8-bit YUV444
                }
                else
                {
                   VPPAPI_DBG_ERROR("OUTPUT_COLOR_FMT_YCBCR420 capture not supported..!!!\n");
                   return 0;
                }
                VPP_DV_Dither_SWpatSel(vpp_obj);
                iStrideInBytes = ((iWidthInBytes+15)/16)*16;
                plane->wb_buff_index = (plane->wb_buff_index + 1) & 0x01;
                uiUvClientValid = 0;
            }
            break;
#endif
        case VPP_WRITEBACK_MODE_OFFLINE_SCL:
            {
                pOfflineBuffDescr = &(plane->offline_buff_descrs[plane->wb_buff_index]);
                iWidthInBytes = uiWidth * pOfflineBuffDescr->uiBitsPerPixel /
                    VPP_OFFLINESCL_BITS_PER_BYTES;
                iStrideInBytes = pOfflineBuffDescr->stride;
                uiBuffAddr_Y = (UINT32)pOfflineBuffDescr->addr;

#ifndef VPP_OFFLINE_NONSTDRESWR_DISABLE
                if(iWidthInBytes & (VPP_OFFLINE_DHUB_SIZE_MULTI - 1))
                {/* Width is non multiple of 16; so use nonStdRes read/write */

                    stVipWriteClientNonStdRes.uNonStdRes_enable = 1;
                    stVipWriteClientNonStdRes.uNonStdRes_pixlineTot = uiWidth;
                }
                else
                {
                    stVipWriteClientNonStdRes.uNonStdRes_enable = 0;
                    stVipWriteClientNonStdRes.uNonStdRes_pixlineTot = 0;
                }

                uiRegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE;
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_WR +
                        RA_VIP_WriteClient_NonStdRes, stVipWriteClientNonStdRes.u32);
#endif
                uiUvClientValid = 0;
            }
            break;
        default:
            break;
    }

    cfgQ = cfgQ_shadow + *cfgQ_len * 2;
    //Programm VIP DHUB and write clients.
    stCfg2NDDMAParam.dhubId = iDhubId;
    stCfg2NDDMAParam.dmaId  = iChanId_Y;
    stCfg2NDDMAParam.addr   = uiBuffAddr_Y;
    stCfg2NDDMAParam.width  = iWidthInBytes;
    stCfg2NDDMAParam.stride = iStrideInBytes;
    stCfg2NDDMAParam.height = uiHeight;
    stCfg2NDDMAParam.cfgQ   = cfgQ;
    stCfg2NDDMAParam.mtrREn = 0;
    stCfg2NDDMAParam.plane  = plane;
    *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

    uiRegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE;

    //Start vip clients
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_WR +
            RA_VIP_WriteClient_Wr, 2);
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_WR +
            RA_VIP_WriteClient_Wr, 0);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_WR +
            RA_VIP_WriteClient_Wr, 1);

    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_CTRL, 2);
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_CTRL, 0);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_CTRL, 1);

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCLTOP;

    stVipSclTopCtrl0.u32 =0;
    stVipSclTopCtrl0.uCTRL0_clear           = 1;
    stVipSclTopCtrl0.uCTRL0_Y_otg_clear     = 1;
    stVipSclTopCtrl0.uCTRL0_UV_otg_clear    = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_VIPSCLTOP_CTRL0, stVipSclTopCtrl0.u32);
    stVipSclTopCtrl0.u32 =0;
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_VIPSCLTOP_CTRL0, stVipSclTopCtrl0.u32);
    stVipSclTopCtrl0.uCTRL0_Y_otg_start  = 1;
    stVipSclTopCtrl0.uCTRL0_UV_otg_start = 1;
    stVipSclTopCtrl0.uCTRL0_start_scl = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_VIPSCLTOP_CTRL0, stVipSclTopCtrl0.u32);

    uiRegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE;
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_tg_ctrl, 2);
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_tg_ctrl, 0);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_tg_ctrl, 1);


    if(uiUvClientValid)
    {
        //Programm VIP DHUB and write clients
        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
        stCfg2NDDMAParam.dhubId = iDhubId;
        stCfg2NDDMAParam.dmaId  = iChanId_UV;
        stCfg2NDDMAParam.addr   = uiBuffAddr_UV;
        stCfg2NDDMAParam.width  = iWidthInBytes;
        stCfg2NDDMAParam.stride = iStrideInBytes;
        stCfg2NDDMAParam.height = uiHeight/2;
        stCfg2NDDMAParam.cfgQ   = cfgQ;
        stCfg2NDDMAParam.mtrREn = 0;
        stCfg2NDDMAParam.plane  = plane;
        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_tg_ctrl_420, 2);
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_tg_ctrl_420, 0);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_tg_ctrl_420, 1);

        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr +
                RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_VIP_WriteClient_Wr, 2);
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, uiRegAddr +
                RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_VIP_WriteClient_Wr, 0);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr +
                RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_VIP_WriteClient_Wr, 1);
    }

    return 0;
}

int configVppWriteBackMode(VPP_OBJ *vpp_obj, UINT32 uiMode, UINT32 uiWidth, UINT32 uiHeight)
{
    UINT32 uiRegAddr = 0;
    UINT32 uiPixTot = 0, uiUvClientValid = 0, uiPackSel_Y = 0, uiPackSel_UV = 0;
    T32DITHER_CFG0 stDitherCfg0;
    UINT32 uiInWidth = 0, uiInHeight = 0, uiInPixTot = 0;
    T32VIP_WriteClient_NonStdRes stVipWriteClientNonStdRes;
    OFFLINE_BUFF_DESCR *pOfflineBuffDescr = NULL;
    PLANE *plane = NULL;
#ifdef CONFIG_AMP_DOLBY_DUMP
    RESOLUTION_INFO  *pResinfo;
    UINT32 widthBytes;
    UINT32 bdp = 12;
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    UINT32 uiHdmiVidFmt = pHdmiTx->videoParams.VideoFmt;
    UINT32 uiColorDepth = pHdmiTx->videoParams.ColorDepth;
    pResinfo = VPP_GetResInfo(VPP_GetResId(CPCB_1));
#endif //CONFIG_AMP_DOLBY_DUMP

    stVipWriteClientNonStdRes.u32 =0;
    if(uiMode > VPP_WRITEBACK_MODE_MAX)
    {
        return 0;
    }

    //Diable VIP interrupts
    HDL_semaphore *pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
    semaphore_intr_enable(pSemHandle, avioDhubSemMap_vpp128b_vpp_inr10, 0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);
    semaphore_intr_enable(pSemHandle, avioDhubSemMap_vpp128b_vpp_inr9, 0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);

    switch(uiMode)
    {
#ifdef CONFIG_AMP_DOLBY_DUMP
        case VPP_WRITEBACK_MODE_DV_COMP:
            {
#ifdef BA_HDMIRX_PIPE_CTRL0_read_sel
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_read_sel = 2;
#else
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_read_sel = 2;
#endif
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_hdmi420sp_wrbk = 1;
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_fifo_sts_ctrl = 1;

                vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_scl1d_bypass = 1;
                vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_ups420_bypass = 1;
                vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_csc_bypass = 1;
                vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_dns444_422_bypass = 1;
                vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_dither_bypass = 1;

                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg01.uCFG0_sp_wrbk_8b = 0;
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_fifo_sts_ctrl = 1;
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 1;

                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_dvtest_offsclr_Fifoctrl = 1;
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_dvtest_ups420sp_datasel = 1;

                uiPackSel_Y = 0;
                uiPackSel_UV = 3;

                uiUvClientValid = 1;
                uiInWidth = uiWidth;
                uiInHeight = uiHeight;
            }
            break;
        case VPP_WRITEBACK_MODE_EDR:
        {
            //For DM writeback & EDR over HDMI
            if(VPP_FE_DV_COMP_MODE_TEST_CVM == VPP_DV_GetDvCompMode(vpp_obj)){
                uiWidth = pResinfo->active_width;
                uiHeight = pResinfo->active_height;
            }
            /*path configuration*/
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_ups420_bypass = 1;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_csc_bypass = 1;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_dns444_422_bypass = 1;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_dither_bypass = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_dvtest_dither_en = 0; //added for dither
#ifdef BA_HDMIRX_PIPE_CTRL0_read_sel
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_read_sel = 3;
#else
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_read_sel = 3;
#endif
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_dlbyedr_datasel = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_fifo_sts_ctrl = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_ofifo_sts_ctrl = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 2;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_scl1d_bypass = 1;

            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_rdy0_sts_ctrl = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_yuv422_444_sel = 0;
            uiPackSel_Y = 5;
            uiInWidth = uiWidth;
            uiInHeight = uiHeight;
        }
            break;
        case VPP_WRITEBACK_MODE_DITHER:
            //For Dither writeback
            /*path configuration*/

            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_ups420_bypass = 1;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_csc_bypass = 1;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_dns444_422_bypass = 1;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_dither_bypass = 1;

            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_dvtest_dither_en = 1;
#ifdef BA_HDMIRX_PIPE_CTRL0_read_sel
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_read_sel = 3;
#else
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_read_sel = 3;
#endif
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_dlbyedr_datasel = 1;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_scl1d_bypass = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_scl_InFifo_Yctrl = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_scl_InFifo_UVctrl = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_rdy0_sts_ctrl = 1;
            uiPackSel_Y = 5;

            if(uiHdmiVidFmt == OUTPUT_COLOR_FMT_YCBCR422)
            {
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_yuv422_444_sel = 0;
                if(uiColorDepth == OUTPUT_BIT_DEPTH_12BIT){
                    uiPackSel_Y = 5; //12-bit YUV422
                    bdp = 12;
                }else if(uiColorDepth == OUTPUT_BIT_DEPTH_10BIT){
                    bdp = 10;
                    uiPackSel_Y = 1;
                }else if(uiColorDepth == OUTPUT_BIT_DEPTH_8BIT){
                    bdp = 8;
                    uiPackSel_Y = 0;
                }
                widthBytes = (uiWidth * bdp * 2)/8;
            }
            else
            {
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_yuv422_444_sel = 1;
                if(uiColorDepth == OUTPUT_BIT_DEPTH_12BIT){
                    uiPackSel_Y = 8;
                    bdp = 12;
                }else if(uiColorDepth == OUTPUT_BIT_DEPTH_10BIT){
                    uiPackSel_Y = 6;
                    bdp = 10;
                }else if(uiColorDepth == OUTPUT_BIT_DEPTH_8BIT){
                    uiPackSel_Y = 5;
                    bdp = 8;
                }
                widthBytes = (uiWidth * bdp * 3)/8;
            }
            if((widthBytes % 16) != 0)
            {
                stVipWriteClientNonStdRes.uNonStdRes_enable = 1;
                stVipWriteClientNonStdRes.uNonStdRes_pixlineTot = uiWidth;
            }
            if(uiColorDepth == OUTPUT_BIT_DEPTH_8BIT)
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 0;
            else if(uiColorDepth == OUTPUT_BIT_DEPTH_10BIT)
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 1;
            else
                vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 2;

            uiInWidth = uiWidth;
            uiInHeight = uiHeight;
            break;
#endif
        case VPP_WRITEBACK_MODE_DISABLE:
            {
                semaphore_intr_enable(pSemHandle, avioDhubSemMap_vpp128b_vpp_inr10, 0/*empty*/,
                        1/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);
                semaphore_intr_enable(pSemHandle, avioDhubSemMap_vpp128b_vpp_inr9, 0/*empty*/,
                        1/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);
            }
            break;

        case VPP_WRITEBACK_MODE_OFFLINE_SCL:
            plane = &(vpp_obj->plane[PLANE_MAIN]);
            pOfflineBuffDescr = &(plane->offline_buff_descrs[plane->wb_buff_index]);
#ifdef BA_HDMIRX_PIPE_CTRL0_read_sel
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_read_sel = 2;
#else
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_read_sel = 2;
#endif
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_fifo_sts_ctrl = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_ofifo_sts_ctrl = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_uvPack_sel = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_scl_InFifo_Yctrl = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_scl_InFifo_UVctrl = 1;

            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_rdy0_sts_ctrl = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_rdy1_sts_ctrl = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.uCTRL0_scl_OFifo_fullCtrl = 0;

            uiRegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE;

            /* Enable DNS444_422 */
            /*T32DNS444_422_CFG5 stDns422To422Cfg5;*/
            /*NOTE : Since default values are good enough for DNS444_422, not programming now */
            /* Set dither mode and control */
            stDitherCfg0.uCFG0_mode = 1;
            stDitherCfg0.uCFG0_ctrl = 1;
            switch(pOfflineBuffDescr->uiBitsPerPixel)
            {
                case 16:
                    uiPackSel_Y = 0;
                    stDitherCfg0.uCFG0_ctrl = 1;
                    vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 0;
                    break;
                case 20:
                    uiPackSel_Y = 1;
                    stDitherCfg0.uCFG0_ctrl = 0;
                    vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 1;
                    break;
                case 24:
                    uiPackSel_Y = 5;
                    stDitherCfg0.uCFG0_ctrl = 2;
                    vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 2;
                    break;
                default:
                    uiPackSel_Y = 0;
                    stDitherCfg0.uCFG0_ctrl = 1;
                    vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.uCFG0_write_sel_8b = 0;
                    break;
            }

            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                    uiRegAddr + RA_HDMIRX_PIPE_VIP_DITHER + RA_DITHER_CFG0, stDitherCfg0.u32);

            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg01.uCFG0_sp_wrbk_8b = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_dvtest_offsclr_Fifoctrl = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_yuv422_444_sel = 0;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_scl_OFifoY_aEmpCtrl = 1;
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.uCFG4_scl_OFifoUV_aEmpCtrl = 1;

            uiUvClientValid = 0;
#ifdef VPP_OFFLINESCL_USING_VIP
            uiInWidth = vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IHRes;
            uiInHeight = vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IVRes;
#else
            uiInWidth = uiWidth;
            uiInHeight = uiHeight;
#endif
            break;

        default:
            break;
    }

    uiPixTot = uiWidth * uiHeight;
    uiInPixTot = uiInWidth * uiInHeight;

    uiRegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_CTRL,
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_CTRL0,
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_BYPASS_CTRL,
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_CFG0,
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_CFG4,
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_scl1d_Inpix, uiInPixTot);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_scl1d_Outpix, uiPixTot);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_hdmirxpipe_Inpix, uiInPixTot);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_WR +
            RA_WriteClient_pix, uiPixTot);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_WR +
            RA_WriteClient_NonStdRes, stVipWriteClientNonStdRes.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_VIP_WR +
            RA_VIP_WriteClient_pack, uiPackSel_Y);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_sclOFIFO_YInpix, uiPixTot);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_sclOFIFO_UVInpix, uiPixTot/2);


    if(uiUvClientValid)
    {
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                uiRegAddr + RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_WriteClient_pix, uiPixTot / 2);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                uiRegAddr + RA_HDMIRX_PIPE_VIP_CRCH_WR + RA_VIP_WriteClient_pack, uiPackSel_UV);
    }

    return 0;
}

#ifdef XVYCC_SUPPORT
static void xvYCC_process(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo)
{
    VPP_HDMI_PKT auxPkt;

    if ((vpp_obj->xvYCC_enable != FALSE) && (vpp_obj->xvYCC_capable != FALSE) &&
            (pinfo->m_is_xvYCC_valid != 0)){
        if ((GaloisMemComp((void*)(&vpp_obj->xvYCC_data),
                        (void*)(&pinfo->m_xvYCC_data), sizeof(xvYCC_DATA)) != 0)){
            /* record xvYCC data */
            GaloisMemcpy((void*)(&vpp_obj->xvYCC_data),
                    (void*)(&pinfo->m_xvYCC_data), sizeof(xvYCC_DATA));
            /* send gamut meta data packets */
            auxPkt.gmdPkt.nextField = TRUE;
            auxPkt.gmdPkt.noCurGBD = FALSE;
            auxPkt.gmdPkt.gdbProfile = VPP_HDMI_GDB_PROFILE_P0;
            auxPkt.gmdPkt.seqInfo = VPP_HDMI_PKT_GMD_ONLY_IN_SEQ;
            auxPkt.gmdPkt.affectedGamutSeqNum = 0;
            auxPkt.gmdPkt.curGamutSeqNum = 0;
            if (!pinfo->m_xvYCC_data.m_format) {
                /* vertice meta data */
                auxPkt.gmdPkt.gbdData[0] = ((pinfo->m_xvYCC_data.m_precision&0x03)<<3) |
                    (pinfo->m_xvYCC_data.m_space&0x07); // Format
                auxPkt.gmdPkt.gbdData[1] = 0x00; // HVertices
                auxPkt.gmdPkt.gbdData[2] = 0x04; // VVertices

                if (pinfo->m_xvYCC_data.m_precision == 0) { /* 8-bit */
                    auxPkt.gmdPkt.gbdData[3] = pinfo->m_xvYCC_data.m_d0; // Black (Y)
                    auxPkt.gmdPkt.gbdData[4] = pinfo->m_xvYCC_data.m_d1; // Black (Cb)
                    auxPkt.gmdPkt.gbdData[5] = pinfo->m_xvYCC_data.m_d2; // Black (Cr)
                    auxPkt.gmdPkt.gbdData[6] = pinfo->m_xvYCC_data.m_d3; // Red (Y)
                    auxPkt.gmdPkt.gbdData[7] = pinfo->m_xvYCC_data.m_d4; // Red (Cb)
                    auxPkt.gmdPkt.gbdData[8] = pinfo->m_xvYCC_data.m_d5; // Red (Cr)
                    auxPkt.gmdPkt.gbdData[9] = pinfo->m_xvYCC_data.m_d6; // Green (Y)
                    auxPkt.gmdPkt.gbdData[10] = pinfo->m_xvYCC_data.m_d7; // Green (Cb)
                    auxPkt.gmdPkt.gbdData[11] = pinfo->m_xvYCC_data.m_d8; // Green (Cr)
                    auxPkt.gmdPkt.gbdData[12] = pinfo->m_xvYCC_data.m_d9; // Blue (Y)
                    auxPkt.gmdPkt.gbdData[13] = pinfo->m_xvYCC_data.m_d10; // Blue (Cb)
                    auxPkt.gmdPkt.gbdData[14] = pinfo->m_xvYCC_data.m_d11; // Blue (Cr)
                } else if (pinfo->m_xvYCC_data.m_precision == 1) { /* 10-bit */
                    auxPkt.gmdPkt.gbdData[3] = pinfo->m_xvYCC_data.m_d0>>2;
                    auxPkt.gmdPkt.gbdData[4] =
                        ((pinfo->m_xvYCC_data.m_d0<<6) | (pinfo->m_xvYCC_data.m_d1>>4)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[5] =
                        ((pinfo->m_xvYCC_data.m_d1<<4) | (pinfo->m_xvYCC_data.m_d2>>6)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[6] =
                        ((pinfo->m_xvYCC_data.m_d2<<2) | (pinfo->m_xvYCC_data.m_d3>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[7] = pinfo->m_xvYCC_data.m_d3 & 0x0ff;
                    auxPkt.gmdPkt.gbdData[8] = pinfo->m_xvYCC_data.m_d4>>2;
                    auxPkt.gmdPkt.gbdData[9] =
                        ((pinfo->m_xvYCC_data.m_d4<<6) | (pinfo->m_xvYCC_data.m_d5>>4)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[10] =
                        ((pinfo->m_xvYCC_data.m_d5<<4) | (pinfo->m_xvYCC_data.m_d6>>6)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[11] =
                        ((pinfo->m_xvYCC_data.m_d6<<2) | (pinfo->m_xvYCC_data.m_d7>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[12] = pinfo->m_xvYCC_data.m_d7 & 0x0ff;
                    auxPkt.gmdPkt.gbdData[13] = pinfo->m_xvYCC_data.m_d8>>2;
                    auxPkt.gmdPkt.gbdData[14] =
                        ((pinfo->m_xvYCC_data.m_d8<<6) | (pinfo->m_xvYCC_data.m_d9>>4)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[15] =
                        ((pinfo->m_xvYCC_data.m_d9<<4) | (pinfo->m_xvYCC_data.m_d10>>6)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[16] =
                        ((pinfo->m_xvYCC_data.m_d10<<2) | (pinfo->m_xvYCC_data.m_d11>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[17] = pinfo->m_xvYCC_data.m_d11 & 0x0ff;
                } else if (pinfo->m_xvYCC_data.m_precision == 2) { /* 12-bit */
                    auxPkt.gmdPkt.gbdData[3] = pinfo->m_xvYCC_data.m_d0>>4;
                    auxPkt.gmdPkt.gbdData[4] =
                        ((pinfo->m_xvYCC_data.m_d0<<4) | (pinfo->m_xvYCC_data.m_d1>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[5] = pinfo->m_xvYCC_data.m_d1 & 0x0ff;
                    auxPkt.gmdPkt.gbdData[6] = pinfo->m_xvYCC_data.m_d2>>4;
                    auxPkt.gmdPkt.gbdData[7] =
                        ((pinfo->m_xvYCC_data.m_d2<<4) | (pinfo->m_xvYCC_data.m_d3>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[8] = pinfo->m_xvYCC_data.m_d3 & 0x0ff;
                    auxPkt.gmdPkt.gbdData[9] = pinfo->m_xvYCC_data.m_d4>>4;
                    auxPkt.gmdPkt.gbdData[10] =
                        ((pinfo->m_xvYCC_data.m_d4<<4) | (pinfo->m_xvYCC_data.m_d5>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[11] = pinfo->m_xvYCC_data.m_d5 & 0x0ff;
                    auxPkt.gmdPkt.gbdData[12] = pinfo->m_xvYCC_data.m_d6>>4;
                    auxPkt.gmdPkt.gbdData[13] =
                        ((pinfo->m_xvYCC_data.m_d6<<4) | (pinfo->m_xvYCC_data.m_d7>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[14] = pinfo->m_xvYCC_data.m_d7 & 0x0ff;
                    auxPkt.gmdPkt.gbdData[15] = pinfo->m_xvYCC_data.m_d8>>4;
                    auxPkt.gmdPkt.gbdData[16] =
                        ((pinfo->m_xvYCC_data.m_d8<<4) | (pinfo->m_xvYCC_data.m_d9>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[17] = pinfo->m_xvYCC_data.m_d9 & 0x0ff;
                    auxPkt.gmdPkt.gbdData[18] = pinfo->m_xvYCC_data.m_d10>>4;
                    auxPkt.gmdPkt.gbdData[19] =
                        ((pinfo->m_xvYCC_data.m_d10<<4) | (pinfo->m_xvYCC_data.m_d11>>8)) & 0x0ff;
                    auxPkt.gmdPkt.gbdData[20] = pinfo->m_xvYCC_data.m_d11 & 0x0ff;
                }
            } else {
                /* r/g/b range meta data */
                auxPkt.gmdPkt.gbdData[0] = 0x80 | ((pinfo->m_xvYCC_data.m_precision&0x03)<<3) |
                    (pinfo->m_xvYCC_data.m_space&0x07); // Format
                if (pinfo->m_xvYCC_data.m_precision == 0) { /* 8-bit */
                    auxPkt.gmdPkt.gbdData[1] = pinfo->m_xvYCC_data.m_d0; // min Red
                    auxPkt.gmdPkt.gbdData[2] = pinfo->m_xvYCC_data.m_d1; // max Red
                    auxPkt.gmdPkt.gbdData[3] = pinfo->m_xvYCC_data.m_d2; // min Green
                    auxPkt.gmdPkt.gbdData[4] = pinfo->m_xvYCC_data.m_d3; // max Green
                    auxPkt.gmdPkt.gbdData[5] = pinfo->m_xvYCC_data.m_d4; // min Blue
                    auxPkt.gmdPkt.gbdData[6] = pinfo->m_xvYCC_data.m_d5; // max Blue
                } else if (pinfo->m_xvYCC_data.m_precision == 1) { /* 10-bit */
                    auxPkt.gmdPkt.gbdData[1] = pinfo->m_xvYCC_data.m_d0>>2; // min Red
                    auxPkt.gmdPkt.gbdData[2] =
                        ((pinfo->m_xvYCC_data.m_d0<<6) | (pinfo->m_xvYCC_data.m_d1>>4)) & 0x0ff;
                    // max Red
                    auxPkt.gmdPkt.gbdData[3] =
                        ((pinfo->m_xvYCC_data.m_d1<<4) | (pinfo->m_xvYCC_data.m_d2>>6)) & 0x0ff;
                    // min Green
                    auxPkt.gmdPkt.gbdData[4] =
                        ((pinfo->m_xvYCC_data.m_d2<<2) | (pinfo->m_xvYCC_data.m_d3>>8)) & 0x0ff;
                    // max Green
                    auxPkt.gmdPkt.gbdData[5] = pinfo->m_xvYCC_data.m_d3 & 0x0ff; // min Blue
                    auxPkt.gmdPkt.gbdData[6] = pinfo->m_xvYCC_data.m_d4>>2; // max Blue
                    auxPkt.gmdPkt.gbdData[7] =
                        ((pinfo->m_xvYCC_data.m_d4<<6) | (pinfo->m_xvYCC_data.m_d5>>4)) & 0x0ff;
                    // max Blue
                    auxPkt.gmdPkt.gbdData[8] = (pinfo->m_xvYCC_data.m_d5<<4) & 0x0ff; // max Blue
                } else if (pinfo->m_xvYCC_data.m_precision == 2) { /* 12-bit */
                    auxPkt.gmdPkt.gbdData[1] = pinfo->m_xvYCC_data.m_d0>>4; // min Red
                    auxPkt.gmdPkt.gbdData[2] =
                        ((pinfo->m_xvYCC_data.m_d0<<4) | (pinfo->m_xvYCC_data.m_d1>>8)) & 0x0ff;
                    // max Red
                    auxPkt.gmdPkt.gbdData[3] = pinfo->m_xvYCC_data.m_d1 & 0x0ff; // min Green
                    auxPkt.gmdPkt.gbdData[4] = pinfo->m_xvYCC_data.m_d2>>4; // min Red
                    auxPkt.gmdPkt.gbdData[5] =
                        ((pinfo->m_xvYCC_data.m_d2<<4) | (pinfo->m_xvYCC_data.m_d3>>8)) & 0x0ff;
                    // max Red
                    auxPkt.gmdPkt.gbdData[6] = pinfo->m_xvYCC_data.m_d3 & 0x0ff; // min Green
                    auxPkt.gmdPkt.gbdData[7] = pinfo->m_xvYCC_data.m_d4>>4; // min Red
                    auxPkt.gmdPkt.gbdData[8] =
                        ((pinfo->m_xvYCC_data.m_d4<<4) | (pinfo->m_xvYCC_data.m_d5>>8)) & 0x0ff;
                    // max Red
                    auxPkt.gmdPkt.gbdData[9] = pinfo->m_xvYCC_data.m_d5 & 0x0ff; // min Green
                }
            }
            VPP_BE_HdmiFormatAuxPkt (vpp_obj, TRUE, VPP_HDMI_PKT_ID_GAMUT_METADATA, &auxPkt);
            VPP_BE_HdmiSendAuxPkt (vpp_obj, VPP_HDMI_PKT_ID_GAMUT_METADATA, TRUE);
        }
    }else{
        /* clear local copy of xvYCC data */
        GaloisMemClear((void *)(&vpp_obj->xvYCC_data), sizeof(xvYCC_DATA));
        if(vpp_obj->hdmi_bt2020en != TRUE)
        {
            /* stop gamut metadata sending if it's enabled */
            VPP_BE_HdmiFormatAuxPkt (vpp_obj, FALSE, VPP_HDMI_PKT_ID_GAMUT_METADATA, NULL);
            VPP_BE_HdmiStopAuxPkt(vpp_obj, VPP_HDMI_PKT_ID_GAMUT_METADATA);
        }
    }
}
#endif

#ifdef CONFIG_AMP_VPP_ROTATION
typedef struct VPP_DHUB_PARAM_ROTATION_s
{
    //Input Parameters
    UINT32 m_uiSrcFtm;
    UINT32 m_uiSrcOrder;
    UINT32 m_uiBitDepth;
    UINT32 m_uiRotationAngle;   //Rotation Angle
    UINT32 m_uiPicW;    //Active Picture width
    UINT32 m_uiPicH;    //Active Picture Height
    UINT32 m_uiStride;  //Stride
    UINT32 m_uiBurst_Y;
    UINT32 m_uiBurst_UV;
    UINT32 m_uiTileW_Y;
    UINT32 m_uiTileW_UV;
    UINT32 m_uiTileH;
    //Output Paramters
    UINT32 m_uiSize_0_Y;  //Size 0
    UINT32 m_uiSize_0_UV;  //Size 0
    UINT32 m_uiStep_0;  //Step 0
    UINT32 m_uiSize_1_Y;
    UINT32 m_uiSize_1_UV;
    UINT32 m_uiStep_1_Y;
    UINT32 m_uiStep_1_UV;
    UINT32 m_uiSize_2_Y;
    UINT32 m_uiSize_2_UV;
    UINT32 m_uiStep_2_Y;
    UINT32 m_uiStep_2_UV;
    UINT32 m_uiBaseAddr_Y;  //Base Address
    UINT32 m_uiBaseAddr_UV;  //Base Address
}VPP_DHUB_PARAM_ROTATION;


static int GetDhubParam(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, VPP_DHUB_PARAM_ROTATION *pstDhubParam)
{

    if(NULL == pstDhubParam)
    {
        return -1;
    }
    if((SRCFMT_YUV420SP_TILED_V4H8 != pstDhubParam->m_uiSrcFtm) &&
            (SRCFMT_YUV420SP_TILED_MODE0 != pstDhubParam->m_uiSrcFtm))
    {
        return 0;
    }
    switch(pstDhubParam->m_uiSrcFtm)
    {
        case SRCFMT_YUV420SP_TILED_V4H8:
            {
                switch(pstDhubParam->m_uiRotationAngle)
                {
                    case ROTATION_0D:
                        {
                            //Working values --> burst 7680, step1 7680, size1 1, step2 7680, size2 270
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);

                        }
                        break;
                    case ROTATION_90D:
                        {

                            pstDhubParam->m_uiBurst_Y = 16;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 4;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            if(pstDhubParam->m_uiPicW > 2160)
                            {
                                pstDhubParam->m_uiPicW = pstDhubParam->m_uiPicW / 2;
                                pstDhubParam->m_uiStride = pstDhubParam->m_uiStride * 2;
                            }
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_1_Y =  (~(pstDhubParam->m_uiStride)) + 1;
                            //2's complement of stride
                            pstDhubParam->m_uiStep_1_UV =  (~(pstDhubParam->m_uiStride)) + 1;
                            //2's complement of stride
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_1_Y - 1) * pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                ( pstDhubParam->m_uiSize_1_UV - 1) * pstDhubParam->m_uiStride;
                            //Do not change it.
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileH, pstDhubParam->m_uiTileW_Y);
                        }
                        break;
                    case ROTATION_180D:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1;
                            //2's complement
                            pstDhubParam->m_uiStep_1_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1;
                            //2's complement
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiStride +
                                ( pstDhubParam->m_uiSize_1_Y - 1) *  pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                (pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiStride +
                                ( pstDhubParam->m_uiSize_1_UV - 1) *  pstDhubParam->m_uiBurst_UV;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    case ROTATION_270D:
                        {
                            pstDhubParam->m_uiBurst_Y = 16;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 4;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            if(pstDhubParam->m_uiPicW > 2160)
                            {
                                pstDhubParam->m_uiPicW = pstDhubParam->m_uiPicW / 2;
                                pstDhubParam->m_uiStride = pstDhubParam->m_uiStride * 2;
                            }
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_2_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1;
                            //2's complement
                            pstDhubParam->m_uiStep_2_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1;
                            //2's complement
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                ( pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiBurst_UV;
                            //Do not change it.
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileH, pstDhubParam->m_uiTileW_Y);
                        }
                        break;
                    case ROTATION_H_FLIP:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1;
                            //2's complement
                            pstDhubParam->m_uiStep_1_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1;
                            //2's complement
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_1_Y - 1) * pstDhubParam->m_uiSize_0_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                ( pstDhubParam->m_uiSize_1_UV - 1) * pstDhubParam->m_uiSize_0_UV ;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    case ROTATION_V_FLIP:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                ( pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);

                        }
                        break;
                    default:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                }

            }
            break;
        case SRCFMT_YUV420SP_TILED_MODE0:
            {
                switch(pstDhubParam->m_uiRotationAngle)
                {
                    case ROTATION_0D:
                        {
                            //Working values --> burst 7680, step1 7680, size1 1, step2 7680, size2 270
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 6;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);

                        }
                        break;
                    case ROTATION_90D:
                        {
                            pstDhubParam->m_uiBurst_Y = 16;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 3;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            if(pstDhubParam->m_uiPicW > 2160)
                            {
                                pstDhubParam->m_uiPicW = pstDhubParam->m_uiPicW / 2;
                                pstDhubParam->m_uiStride = pstDhubParam->m_uiStride * 2;
                            }
                            pstDhubParam->m_uiPicH = (pstDhubParam->m_uiPicH / 32) * 32;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_1_Y =  (~(pstDhubParam->m_uiStride)) + 1;
                            //2's complement of stride
                            pstDhubParam->m_uiStep_1_UV =  (~(pstDhubParam->m_uiStride)) + 1;
                            //2's complement of stride
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_1_Y - 1) * pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                ( pstDhubParam->m_uiSize_1_UV - 1) * pstDhubParam->m_uiStride;
                            //Do not change it.
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileH, pstDhubParam->m_uiTileW_Y);
                        }
                        break;
                    case ROTATION_180D:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 6;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1;
                            //2's complement
                            pstDhubParam->m_uiStep_1_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1;
                            //2's complement
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiStride +
                                ( pstDhubParam->m_uiSize_1_Y - 1) *  pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                (pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiStride +
                                ( pstDhubParam->m_uiSize_1_UV - 1) *  pstDhubParam->m_uiBurst_UV;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    case ROTATION_270D:
                        {
                            pstDhubParam->m_uiBurst_Y = 16;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 3;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiPicH = (pstDhubParam->m_uiPicH / 32) * 32;
                            if(pstDhubParam->m_uiPicW > 2160)
                            {
                                pstDhubParam->m_uiPicW = pstDhubParam->m_uiPicW / 2;
                                pstDhubParam->m_uiStride = pstDhubParam->m_uiStride * 2;
                            }
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_2_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1;
                            //2's complement
                            pstDhubParam->m_uiStep_2_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1;
                            //2's complement
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                ( pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiBurst_UV;
                            //Do not change it.
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileH, pstDhubParam->m_uiTileW_Y);
                        }
                        break;
                    case ROTATION_H_FLIP:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 6;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1;
                            //2's complement
                            pstDhubParam->m_uiStep_1_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1;
                            //2's complement
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_1_Y - 1) * pstDhubParam->m_uiSize_0_Y ;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                ( pstDhubParam->m_uiSize_1_UV - 1) * pstDhubParam->m_uiSize_0_UV ;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    case ROTATION_V_FLIP:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 6;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y +
                                ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV +
                                ( pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    default:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW +
                                    pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 +
                                    pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV =
                                pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId,
                                    pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                }
            }
        default:
            break;
    }
    return 0;
}

static int GetRotationAngle(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, UINT32 *puiRotationAngle)
{
    UINT32 uiRet = MV_VPP_OK;
    PLANE *plane = NULL;
    UINT32 uiRotationAngle = 0;

    if(NULL == puiRotationAngle)
    {
        return MV_VPP_EBADPARAM;
    }
    plane = &(vpp_obj->plane[uiPlaneId]);

    uiRotationAngle = plane->uchRotationAngle;

    if(ROTATION_0D == plane->uchRotationAngle)
    {
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
        switch(plane->uchInverseScanMode)
        {
            case SCAN_INVERSE_NORMAL:
                uiRotationAngle = ROTATION_0D;
                break;
            case SCAN_INVERSE_H:
                uiRotationAngle = ROTATION_H_FLIP;
                break;
            case SCAN_INVERSE_V:
                uiRotationAngle = ROTATION_V_FLIP;
                break;
            case SCAN_INVERSE_HV:
                uiRotationAngle = ROTATION_180D;
                break;
            default:
                uiRotationAngle = ROTATION_0D;
                break;
        }
#endif
    }
    else
    {
        uiRotationAngle = plane->uchRotationAngle;
    }

    *puiRotationAngle = uiRotationAngle;

    return uiRet;
}
#endif

/* This funcion is used to configure Decompression based on input frame */
static int vppConfigDecompression(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo, INT chanID)
{
    PLANE *plane = NULL;
    INT planeID = -1;

    planeID = PLANE_OF_CHAN(vpp_obj, chanID);
    plane = &vpp_obj->plane[planeID];
    if(pinfo->m_is_compressed != plane->m_mtrR__en)
    {
        plane->m_mtrR__en = pinfo->m_is_compressed;
        if(plane->m_mtrR__en)
        {
            plane->m_mtrR_updSemId_Y  |= 0x1; /* Compression mode */
            plane->m_mtrR_updSemId_UV |= 0x1; /* Compression mode */
        }
        else
        {
            plane->m_mtrR_updSemId_Y  &= (~0x1); /* Bypass mode */
            plane->m_mtrR_updSemId_UV &= (~0x1); /* Bypass mode */
        }
    }
    VPP_AVIOMTR_UpdateDesc(vpp_obj, VPP_AVIOMTR_CONFIG_VPP, pinfo, planeID);
    return MV_VPP_OK;
}

static BOOL isVPPExtraLineDropRequired(RESOLUTION_INFO  *pResinfo, CHAN *chan, PLANE *plane)
{
    BOOL extraLDen = FALSE;
    int planeID;

    planeID = PLANE_OF_CHAN(NULL, plane->chanID);

    if ((plane->ref_win.height / plane->lineDroppingStrides) >
                             chan->disp_win_final.height) {
        /* Is Big downscaling and output window is very small */
        if (VPP_OFFLINESCL_IS_LINEDROP_ONLY(plane->ref_win.height, chan->disp_win_final.height))
            extraLDen = TRUE;

        /* Is Portrait video playback in interlaced display resolution(YouTube) */
        if (PLANE_MAIN == planeID &&
                VPP_IS_PORTRAIT_FRAME_RESTRICT(plane->ref_win.width, plane->ref_win.height,
                    chan->disp_win_final.width, chan->disp_win_final.height, pResinfo->scan))
            extraLDen = TRUE;
    }

    return extraLDen;
}

extern void AxiLatencyMeterHandler(VPP_OBJ *vpp_obj);

/********************************************************************************
 * FUNCTION: start all active plane data loader of a VPP FE channel
 * PARAMS: *vpp_obj - pointer to VPP object
 *         chanID - channel
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EBADPARAM - invalid parameter
 *         MV_VPP_EUNCONFIG - plane not activated yet
 *******************************************************************************/
static unsigned int uiIsrLocalResID[MAX_NUM_CPCBS] = {0};
static int startChannelDataLoader(VPP_OBJ *vpp_obj, int chanID)
{
    int planeID, cpcbID;
    int crop_actv_offset = 0, crop_actv_offset_UV = 0;
    int CropWpl, PixelPerBeat;
    int crop_recalculation_required;
    int data_loader_update_required;
    int data_loader_crop_required;
    int cpcb_source_window_update_required;
    int cpcb_bg_window_update_required;
    int scaler_update_required;
    int scaler_crop_update_required;
    int scalarCroppingRequired = 0;
    int orgRefWidth = 0;
    CHAN *chan;
    PLANE *plane;
    VDEC_VID_BUF_DESC *inline_frame;
    VBUF_INFO *pinfo;
    VPP_WIN *actv_win = NULL;
    VPP_WIN crop_actv_win, crop_scl_actv_win;
    VPP_FE_DLR_INPUT_RES res;
    VPP_FE_DLR_INPUT_RES client_res;
    VPP_FE_DLR_INPUT_RES feRes;
    VPP_FE_DLR_INPUT_RES cropRes;
    VPP_FE_DLR_INPUT_RES SclOutRes;
    VPP_FE_DLR_INPUT_RES sDBVIPSize;
    OFFLINE_BUFF_DESCR *pOfflineBuffDescr = NULL;
    VPP_WIN tmp_crop_scl_actv_win;
    VPP_WIN tmp_crop_win_final;
    VPP_WIN tmp_crop_disp_win_final;
#ifdef ENABLE_OVP
    UNSG32 uiBuff0Valid = 0, uiBuff1Valid = 0;
#endif
    VPP_DHUB_CFG_2NDDMA_PARAM stCfg2NDDMAParam;
    VPP_FRC_SCL_NUM eVppFrcSclNum = -1;
    UNSG8 iSclCoeffUpdate = 0;
    INT is_OfflineConfigured = 0;
    VPP_SCL_RES stVppSclRes;
#ifdef RA_VPP_MP_PIP_dvCompYUV2RGB
    VPP_FE_DLR_INPUT_RES stDVinpRes;
#endif
#ifdef ENABLE_OVPSCL_PIP
    UINT32 uiInterlacedFactor;
#endif
    UINT8 m_uchDvCompMode;
#ifdef VPP_DEBUG
    int dhub_status;
#endif
#if defined(PE_3D_BD)
    int i;
    char ss_offset = 0;
    int ss_seq_id;
    int pinfo_active_left_record = 0, pinfo_active_top_record = 0;
    void* pinfo_pbuf_start = 0;
#endif
    int *cfgQ, *cfgQ_shadow;
    int *cfgQ_len;
#ifdef PE_3D_FMT_CONVERT
    int src_3D_fmt = 0;
    unsigned int  main_ref_width_record = 0, main_ref_height_record = 0;
#endif
    /*If stride is already multipled by a factor then no need to multiple stride otherwise apply factor
     *Note: Whenever linedropping is enabled in tiled mode, Y Alignment should be 4 (hgt of a tile)
     *      then ensure Ensure tiledModeStrideFactor & tiledModeWidHgtFactor are 4 or multiple of 4*/
    int tiledModeStrideFactor=1;
    int tiledModeWidHgtFactor=1;
#ifndef ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA
    int tileYAlign = 1;
#endif
    unsigned int uiCromaAllignedLinedropFactor = 1;
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    static unsigned char uchChangeVnr = 0;
    RESOLUTION_INFO  *pResinfo, *pDVResinfo;


    cpcbID = CPCB_OF_CHAN(vpp_obj, chanID);
    chan = &vpp_obj->chan[chanID];
    pDVResinfo = VPP_GetResInfo(VPP_GetResId(cpcbID));
    pResinfo   = VPP_GetResInfo(vpp_obj->uiResID[cpcbID]);

    cfgQ_shadow = vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ->addr;
    cfgQ_len = &(vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ->len);

    /******************************************************/
    /* 1. get frame descriptor and crop display content   */
    /******************************************************/
    //    VPPAPI_DBG_INFO("Start channel data loader\n");

    /* get the right inline frame to start DMA */
    if (chanID != CHAN_AUX)
    { /* none AUX channel */
        planeID = PLANE_OF_CHAN(vpp_obj, chanID);
        plane = &vpp_obj->plane[planeID];

        if (plane->status == STATUS_DISP_VIDEO)
            inline_frame = plane->curr_frame;
        else if (plane->status == STATUS_DISP_LOGO)
            inline_frame = plane->curr_still_picture;
        else if (plane->status == STATUS_DISP_PATGEN)
            inline_frame = plane->dummy_frame;
        else
            inline_frame = NULL;

        if (!inline_frame) {
            VPP_CPCB_ResetPlane(vpp_obj, chanID);
            return (MV_VPP_OK);
        }

        if (planeID == PLANE_VMX) {
            VPP_VMXAPI_StartDataChannelLoader(vpp_obj, chanID);
            return (MV_VPP_OK);
        }

        crop_recalculation_required = 0;
        data_loader_update_required = 0;
        data_loader_crop_required = 0;
        cpcb_source_window_update_required = 0;
        cpcb_bg_window_update_required = 0;
        scaler_update_required = 0;
        scaler_crop_update_required = 0;
        plane->m_SeparateElcropRequired = 0;
        plane->sclModeChanged = 0;
        plane->m_dolby_ref_win_updated = 0;
        plane->dolbyLineDropEn = 0;

        /* parse inline frame and start inline DMA for it */
#ifndef CONFIG_AMP_TEE_ENABLE
        pinfo = (VBUF_INFO *)inline_frame->user_data;
#else
        pinfo = (VBUF_INFO *)inline_frame;
#endif
        VPP_DEBUG_UTIL_storeVbufInfo(vpp_obj, planeID, pinfo);
        VPP_DEBUG_UTIL_getVbufInfo(vpp_obj, planeID, &pinfo);

        if (plane->frame_win.x != pinfo->m_active_left
                || plane->frame_win.y != pinfo->m_active_top
                || plane->frame_win.width != pinfo->m_active_width
                || plane->frame_win.height !=pinfo->m_active_height) {
            /* update the plane content win */
            plane->frame_win.x = pinfo->m_active_left;
            plane->frame_win.y = pinfo->m_active_top;
            plane->frame_win.width = pinfo->m_active_width;
            plane->frame_win.height = pinfo->m_active_height;
            crop_recalculation_required = 1;
            data_loader_update_required = 1;
        }

        //Process/Apply the resolution cap based on SA entry
        VPP_ENV_SA_ProcessResolutionCap(vpp_obj, planeID);

        if(SRCFMT_YUV420SP_TILED_MODE0 == pinfo->m_srcfmt ||
                SRCFMT_YUV420SP_TILED_MODE1 == pinfo->m_srcfmt
#ifdef CONFIG_AMP_VPP_ROTATION
                || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_V4H8
#endif
         )
        {
            tiledModeStrideFactor=VPP_TILED_MODE_STRIDE_FACTOR==1?4:1;
            tiledModeWidHgtFactor=1*4;
        }

#ifdef CONFIG_AMP_DCM
        if(cpcbID == CPCB_1)
            dcmHandler(vpp_obj, planeID, pinfo);
#endif

        /* update color primary */
        if((pinfo->m_colorprimaries  == VPP_COLOR_PRIM_BT709)
                || (pinfo->m_colorprimaries  == VPP_COLOR_PRIM_BT470BG)
                || (pinfo->m_colorprimaries  == VPP_COLOR_PRIM_BT170M)
                || (pinfo->m_colorprimaries  == VPP_COLOR_PRIM_BT240M)
                || (pinfo->m_colorprimaries  == VPP_COLOR_PRIM_BT2020))
        {
            vpp_obj->CscInp_colorprim = pinfo->m_colorprimaries ;
        }
        else
        {
            if((pinfo->m_active_width<=720)
                    &&(pinfo->m_active_height<=480)){
                vpp_obj->CscInp_colorprim = VPP_COLOR_PRIM_BT170M;
            }
            else if((pinfo->m_active_width<=720)
                    &&(pinfo->m_active_height<=576)){
                vpp_obj->CscInp_colorprim = VPP_COLOR_PRIM_BT470BG;
            }
            else{
                vpp_obj->CscInp_colorprim = VPP_COLOR_PRIM_BT709;
            }
        }
        //AxiLatencyMeterHandler(vpp_obj);//FIXME:VPP_SW_BRINGUP:Not defined. Check this

#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
        //FIXME:graphics plane need not calculate the display offset;
        //instead send x,y through pinfo->m_active_left, pinfo->m_active_top
        //FIXME:Note application can set its own left per frame
        //in pinfo->m_active_left, pinfo->m_active_top and they are also expected to be -ve number
        pinfo->m_active_left = -plane->ref_win.x;
        pinfo->m_active_top  = -plane->ref_win.y;
#else
        if(planeID == PLANE_GFX1)
        {
            pinfo->m_active_left = -plane->ref_win.x;
            pinfo->m_active_top  = -plane->ref_win.y;
        }
#endif //ENABLE_REFWIN_FOR_FPGA_TESTING

        if(planeID == PLANE_MAIN)
        {
            if(uchChangeVnr)
            {
                if (uchChangeVnr == 1)
                {
                    // Change back to the stored VNR mode
                    plane->vp_updated = 2;
                }
                uchChangeVnr--;
            }
        }

        if(plane->force_scl_mode_count) {
            plane->force_scl_mode_count--;
            if(!plane->force_scl_mode_count) {
                //Re-trigger Scaler calculation & select inline/offline SCL
                crop_recalculation_required = 1;
                VPPAPI_DBG_INFO("SCL Force line-dropping : Plane:%d, frame-delay:%d\n",
                        plane->chanID, plane->force_scl_mode_count);
            }
        }

#if defined(PE_3D_BD)
        if (pDVResinfo->flag_3d)
        {
            /* parse the right eye view info */
            if(planeID != PLANE_PIP)
            {
                /* plane is in BD mode */
                if (plane->ss_type == 1)
                {
                    /* right view is valid*/
                    if ( pinfo->m_is_right_eye_view_valid)
                    {
                        /* current CPCB interrupt is for right view */
                        if (pinfo->m_3D_SwitchLeftRightView == 0)
                        {
                            if (vpp_obj->dv[cpcbID].viewID == 1)
                            {
                                if (pinfo->m_right_eye_view_descriptor != NULL)
                                {
#ifdef VPP_TZ_TBD_CHECK
                                    /* right view may have a different vbuf_info */
                                    pinfo = (VBUF_INFO *)((VDEC_VID_BUF_DESC *)
                                            (pinfo->m_right_eye_view_descriptor))->user_data;
#else
                                    /*m_right_eye_view_descriptor will always be NULL*/
#endif
                                }
                                else
                                {
                                    /* right view have the same vbuf_info with left view,
                                       just update the start address*/
                                    pinfo_pbuf_start = (void*)pinfo->m_pbuf_start;
                                    pinfo->m_pbuf_start = (ARCH_PTR_TYPE)pinfo->m_right_eye_view_buf_start;
                                }
                            }
                        }
                        else
                        {
                            //Switch right view and left view
                            if (vpp_obj->dv[cpcbID].viewID == 0)
                            {
                                if (pinfo->m_right_eye_view_descriptor != NULL)
                                {
#ifdef VPP_TZ_TBD_CHECK
                                    /* right view may have a different vbuf_info */
                                    pinfo = (VBUF_INFO *)((VDEC_VID_BUF_DESC *)
                                            (pinfo->m_right_eye_view_descriptor))->user_data;
#else
                                    /*m_right_eye_view_descriptor will always be NULL*/
#endif
                                }
                                else
                                {
                                    /* right view have the same vbuf_info with left view,
                                       just update the start address*/
                                    pinfo_pbuf_start = (void*)pinfo->m_pbuf_start;
                                    pinfo->m_pbuf_start = pinfo->m_right_eye_view_buf_start;
                                }
                            }
                        }

                        /* record offset meta data */
                        if(planeID == PLANE_MAIN)
                        {
                            for (i = 0; i < pinfo->m_number_of_offset_sequences; i++)
                                vpp_obj->ss_offset_meta_data[i] = pinfo->m_offset_meta_data[i];
                            for (i = pinfo->m_number_of_offset_sequences; i < 32; i++)
                                vpp_obj->ss_offset_meta_data[i] = 0;
                        }
                    }
                }
            }
        }
#endif

#ifdef XVYCC_SUPPORT
        //HDMI_IN mode
        if (((chanID == CHAN_MAIN) || (chanID == CHAN_PIP)) &&
                (vpp_obj->xvYCC_mode != 1) && (cpcbID == CPCB_1))
            xvYCC_process(vpp_obj, pinfo);
#endif

#ifdef CONFIG_AMP_VPP_ROTATION
        if(PLANE_MAIN == planeID || PLANE_PIP == planeID)
        {
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
            //If Inverse scan is supported, change rotation angle based on inverse scan supported.
            if(plane->uchInverseScanModeUpdated || plane->uchRotationAngleUpdate)
#else
            if(plane->uchRotationAngleUpdate)
#endif
            {
#ifdef VPP_FORCE_DISABLE_ROTATION_FOR_COMPRESSED_FRAME
                //Allow/Consider user specified rotation for non-compressed
                //frame otherwise disable rotation
                plane->uchRotationAngle =
                pinfo->m_is_compressed ? ROTATION_0D : plane->uchUserRotationAngle;
#endif //VPP_FORCE_DISABLE_ROTATION_FOR_COMPRESSED_FRAME

                if(SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt ||
                        ((SRCFMT_YUV420SP_TILED_MODE0 == pinfo->m_srcfmt) &&
                         (10 == pinfo->m_bits_per_pixel)))
                {
                    UINT32 uiRotationAngle = 0;
                    GetRotationAngle(vpp_obj, planeID, &uiRotationAngle);
                    //Configure LUT
                    if (PLANE_MAIN == planeID)
                        FE_DLR_ConfigDifBlock(vpp_obj, VPP_FE_DLR_MAIN, pinfo->m_srcfmt,
                                pinfo->m_order, pinfo->m_bits_per_pixel, uiRotationAngle);
                    else if(PLANE_PIP == planeID)
                        FE_DLR_ConfigDifBlock(vpp_obj, VPP_FE_DLR_PIP, pinfo->m_srcfmt,
                                pinfo->m_order, pinfo->m_bits_per_pixel, uiRotationAngle);

                    plane->ref_win_updated = 1;
                }
                plane->uchRotationAngleUpdate = 0;
            }
        }
#endif
        if(cpcbID == CPCB_1) {
            //Give a pulse to the delayed GetClkSignature
            VPP_ClkGate_performDelayOperation(vpp_obj);
            if(PLANE_MAIN == planeID) {
                //Get the latest DV TA property value
                VPP_DV_FE_Handler(vpp_obj, plane, pinfo);
            }
        }

        /* Initialize the El sample crop to zero for non dolby content*/
        if(!pinfo->m_dv_present_mode)
        {
            plane->m_SampleCropEnable_el = 0;
            plane->m_El_Aligned_Luma_Width = 0;
            plane->m_El_Aligned_Luma_Height = 0;
            plane->m_El_Aligned_Chroma_Width = 0;
            plane->m_El_Aligned_Chroma_Height = 0;
            plane->m_SeparateElcropRequired = 0;
            plane->m_El_crop_recalculation_required = 0;
            plane->m_Bl_Crop_Out_Width = 0;
            plane->m_Bl_Crop_Out_Height = 0;
        }

        /* no VPP settting update in interlaced bottom field input */
#if defined(PULLUP_ENABLE)
        if ((plane->frame_type != 0) ||
                (!pDVResinfo->flag_3d &&
                 (plane->field == 0)) || pDVResinfo->flag_3d)
#else
        if ((plane->frame_type != 0) ||
                (!pDVResinfo->flag_3d &&
                 (plane->field != 1)) || pDVResinfo->flag_3d)
#endif
        {
            UINT8 enableOfflineAfterHdmiRxFlag = 1;
#ifdef VPP_DISABLE_OFFLINE_AFTER_HDMIRX
            /* VPP will not use VIP path for offline down scaling
               once after it get a VBUF_SRC_HDMIRX */
            enableOfflineAfterHdmiRxFlag = VBUF_SRC_HDMIRX != plane->uiBufSrc;
#endif
            /* update plane data format setting according to new frame */
            if ((pinfo->m_srcfmt != plane->srcfmt) ||
                    (pinfo->m_order != plane->order) ||
                    (pinfo->m_bits_per_pixel != plane->bits_per_pixel) ||
                    (pinfo->m_luma_left_ofst != plane->m_leftpad_y) ||
                    (pinfo->m_luma_top_ofst != plane->m_toppad_y) ||
                    (pinfo->m_chroma_left_ofst != plane->m_leftpad_c) ||
                    (pinfo->m_chroma_top_ofst != plane->m_toppad_c) ||
                    (pinfo->m_is_compressed != plane->m_mtrR__en) ||
                    (pinfo->m_buf_src != plane->uiBufSrc && enableOfflineAfterHdmiRxFlag) ||
                    (pinfo->m_dv_present_mode != plane->uiDolbyPresentMode) ||
                    (!(IS_GFX_SRC_FORMAT(pinfo->m_srcfmt)) &&
                     (vpp_obj->CscInp_colorprim  < VPP_COLOR_PRIM_MAX) &&
                     (vpp_obj->CscInp_colorprim  != plane->color_primaries)))
            {
                //VPP_DEBUG_UTIL_printVbufInfo(vpp_obj, chanID, pinfo);
                plane->color_primaries = vpp_obj->CscInp_colorprim;
                plane->uiDolbyPresentMode = pinfo->m_dv_present_mode;

                /*Hold on gfx for the time being */
                if(chanID == CHAN_MAIN){
                    if(isHDR(pinfo) || isHLG(pinfo))
                        vpp_obj->isHDRContent = TRUE;
                    else
                        vpp_obj->isHDRContent = FALSE;
                }
                data_loader_update_required = 1; // bg4ct - update datafmt and size are in FE Setsize

#ifdef VPP_FORCE_DISABLE_ROTATION_FOR_COMPRESSED_FRAME
                //Allow/Consider user specified rotation for
                //non-compressed frame otherwise disable rotation
                plane->uchRotationAngle =
                    pinfo->m_is_compressed ? ROTATION_0D : plane->uchUserRotationAngle;
#endif //VPP_FORCE_DISABLE_ROTATION_FOR_COMPRESSED_FRAME

                /* For compressed inputs check the padding and update data loader */
                if((pinfo->m_luma_left_ofst != plane->m_leftpad_y)||
                        (pinfo->m_luma_top_ofst != plane->m_toppad_y)||
                        (pinfo->m_chroma_left_ofst != plane->m_leftpad_c)||
                        (pinfo->m_chroma_top_ofst != plane->m_toppad_c))
                {
                    data_loader_update_required = 1;
                    crop_recalculation_required = 1;
                    if(((SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt) ||
                                ((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0) &&
                                 (10 == pinfo->m_bits_per_pixel)))
                            &&(ROTATION_0D == plane->uchRotationAngle))
                    {
                        if(IS_MTR_ENABLED(pinfo->m_is_compressed))
                        {
                            if(pinfo->m_luma_left_ofst != 0 || pinfo->m_chroma_left_ofst != 0)
                            {
                                plane->m_SampleCropEnable = 1;
                            }
                        }
                        else
                        {
                            plane->m_SampleCropEnable = 0;
                            plane->m_rightpad_y = 0;
                            plane->m_bottompad_y = 0;
                            plane->m_rightpad_c = 0;
                            plane->m_bottompad_c = 0;
                        }
                    }
                    else
                    {
                        plane->m_SampleCropEnable = 0;
                        plane->m_leftpad_y = 0;
                        plane->m_leftpadadjust_y = 0;
                        plane->m_toppad_y = 0;
                        plane->m_toppadadjust_y = 0;
                        plane->m_leftpad_c = 0;
                        plane->m_leftpadadjust_c = 0;
                        plane->m_toppad_c = 0;
                        plane->m_toppadadjust_c = 0;
                        plane->m_rightpad_y = 0;
                        plane->m_bottompad_y = 0;
                        plane->m_rightpad_c = 0;
                        plane->m_bottompad_c = 0;
                        plane->m_Aligned_Luma_Width = 0;
                        plane->m_Aligned_Luma_Height = 0;
                        plane->m_Aligned_Chroma_Width = 0;
                        plane->m_Aligned_Chroma_Height = 0;

                    }
                }
                /* update current data format in vpp structure */
                plane->srcfmt = pinfo->m_srcfmt;
                plane->order = pinfo->m_order;
                plane->bits_per_pixel = pinfo->m_bits_per_pixel;
                plane->m_leftpad_y = pinfo->m_luma_left_ofst;
                plane->m_toppad_y = pinfo->m_luma_top_ofst;
                plane->m_leftpad_c = pinfo->m_chroma_left_ofst;
                plane->m_toppad_c = pinfo->m_chroma_top_ofst;
                //invoke getBPP to get alignment details for cropping
                plane->bpp = VPP_OVL_GetBpp(vpp_obj, planeID, plane->bits_per_pixel, plane->srcfmt);
                plane->tile.tileStartFrac = 0;
                if(enableOfflineAfterHdmiRxFlag)
                    plane->uiBufSrc = pinfo->m_buf_src;

                //Try to trigger the delayed GetClkSignature for format change
                VPP_ClkGate_triggerDelayOperation(vpp_obj, VPP_CLKGATE_DLY_OPT_SRCFMT);

                if (planeID == PLANE_MAIN)
                {
                    FE_DLR_Config(vpp_obj, VPP_FE_DLR_MAIN, plane->srcfmt,
                            plane->order, plane->bpp, pinfo->m_is_frame_seq);
#ifdef CONFIG_AMP_VPP_ROTATION
                    if(SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt ||
                            ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                             (10 == pinfo->m_bits_per_pixel)))
                    {
                        VPP_SRAM_PowerGate_DIF(vpp_obj, planeID, VPP_SRAM_PWRGATE_DISABLE);
                        //Configure LUT
                        FE_DLR_ConfigDifBlock(vpp_obj, planeID, plane->srcfmt,
                                plane->order, plane->bpp, plane->uchRotationAngle);
                    }
                    else
                    {
                        //Powergate DIF LUT
                        VPP_SRAM_PowerGate_DIF(vpp_obj, planeID, VPP_SRAM_PWRGATE_ENABLE);
                    }
#endif
                    FE_DLR_SetPlanePackSel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_0,
                            plane->srcfmt, plane->bpp);
#ifdef ENABLE_OVP
                    if (!pinfo->m_is_frame_seq)
                    {
                        FE_DLR_SetPlanePackSel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_2,
                                plane->srcfmt, plane->bits_per_pixel);
                        FE_DLR_SetPlanePackSel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_3,
                                plane->srcfmt, plane->bits_per_pixel);
                    }
#endif
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA
#ifdef CONFIG_AMP_VPP_ROTATION
                            || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt
#endif
                      )
                    {
                        FE_DLR_SetPlanePackSel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_1,
                                plane->srcfmt, plane->bpp);
                    }
                }
#ifdef ENABLE_PIP_GFX0_PATH
                else if (planeID == PLANE_PIP)
                {
                    FE_DLR_Config(vpp_obj, VPP_FE_DLR_PIP, plane->srcfmt,
                            plane->order, plane->bpp, pinfo->m_is_frame_seq);
                    FE_DLR_EnableDeAlpha(vpp_obj, planeID, VPP_IS_DEALPHA_SRC_FMT(plane->srcfmt, cpcbID));
#ifdef CONFIG_AMP_VPP_ROTATION
                    if(SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt ||
                            ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                             (10 == pinfo->m_bits_per_pixel)))
                    {
                        VPP_SRAM_PowerGate_DIF(vpp_obj, planeID, VPP_SRAM_PWRGATE_DISABLE);
                        //Configure LUT
                        FE_DLR_ConfigDifBlock(vpp_obj, VPP_FE_DLR_PIP, plane->srcfmt,
                                plane->order, plane->bpp, plane->uchRotationAngle);
                    }
                    else
                    {
                        //Powergate DIF LUT
                        VPP_SRAM_PowerGate_DIF(vpp_obj, planeID, VPP_SRAM_PWRGATE_ENABLE);
                    }
#endif
                    FE_DLR_SetPlanePackSel(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_0, plane->srcfmt, plane->bpp);
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_V4H8 ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA)
                    {
                        FE_DLR_SetPlanePackSel(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_1,
                                plane->srcfmt, plane->bpp);
                    }
                    if(plane->srcfmt == SRCFMT_XRGB32)
                    {
                        //enableperpixelalpha value as 0 so that it will take per plane alpha
                        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE2,
                                VPP_OVL_PLANE_ALPHA_OPAQUE,VPP_PERPIXEL_ALPHA_DISABLE);
                    }
                    else if(plane->srcfmt == SRCFMT_ARGB32 || plane->srcfmt == SRCFMT_ARGB32_PM ||
                            plane->srcfmt == SRCFMT_IPT444)
                    {
                        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE2,
                                chan->disp_win_attr.alpha, VPP_PERPIXEL_ALPHA_ENABLE);
                    }
                    else if(plane->srcfmt== SRCFMT_RGB565)
                    {
                        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE2,
                                VPP_OVL_PLANE_ALPHA_OPAQUE, VPP_PERPIXEL_ALPHA_DISABLE);
                    }
                    else //video format
                    {
                        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE2,
                                chan->disp_win_attr.alpha, VPP_PERPIXEL_ALPHA_DISABLE);
                    }
                }
#endif //ENABLE_PIP_GFX0_PATH
                else if (planeID == PLANE_GFX1) {
                    FE_DLR_EnableDeAlpha(vpp_obj, planeID, VPP_IS_DEALPHA_SRC_FMT(plane->srcfmt, cpcbID));
                    FE_DLR_Config(vpp_obj, VPP_FE_DLR_GFX1, plane->srcfmt, plane->order,
                            plane->bpp, pinfo->m_is_frame_seq);
                    FE_DLR_SetPlanePackSel(vpp_obj, VPP_FE_DLR_CHANNEL_GFX1_0,
                            plane->srcfmt, plane->bpp);
                    if(plane->srcfmt == SRCFMT_XRGB32)
                    {
                        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE3,
                                VPP_OVL_PLANE_ALPHA_OPAQUE, VPP_PERPIXEL_ALPHA_DISABLE);
                    }
                    else if(plane->srcfmt== SRCFMT_RGB565)
                    {
                        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE3,
                                VPP_OVL_PLANE_ALPHA_OPAQUE, VPP_PERPIXEL_ALPHA_DISABLE);
                    }
                    else
                    {
                        VPP_OVL_SetPlaneAlpha(vpp_obj, cpcbID, Vpp_OVL_PLANE3,
                                chan->disp_win_attr.alpha, VPP_PERPIXEL_ALPHA_ENABLE);
                    }
                }
                else if (planeID == PLANE_GFX2) {
                    FE_DLR_Config(vpp_obj, VPP_FE_DLR_GFX2, plane->srcfmt, plane->order,
                            plane->bpp, pinfo->m_is_frame_seq);
                    /*Disable DeAlpha for no overlay cases*/
                    FE_DLR_EnableDeAlpha(vpp_obj, planeID, VPP_IS_DEALPHA_SRC_FMT(plane->srcfmt, cpcbID));
                    FE_DLR_SetPlanePackSel(vpp_obj, VPP_FE_DLR_CHANNEL_GFX2_0,
                            plane->srcfmt, plane->bpp);
                }
                /* display content format changed, recalculate cropping */
                crop_recalculation_required = 1;
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                plane->uchInverseScanModeUpdated = 1;
#endif
            }
            /* Configure the DHUB for compressed inputs */
            /*TODO Only buf address change needs to be hadled in every ISR.
              Other frame descriptors needs to be changed only when there is a change */
            vppConfigDecompression(vpp_obj, pinfo, chanID);

            if(((plane->crop_actv_win.width == 0 && plane->crop_actv_win.height == 0) &&
                        (pinfo->m_active_width != 0 && pinfo->m_active_height != 0))) {
                //cropping required for the first valid frame
                crop_recalculation_required = 1;
            }
            if(planeID == PLANE_PIP) {
                /* FIXME: PIP builtin frame downscaling is causing underflow */
                if((pinfo->builtinFrame == TRUE) && \
                        (plane->ref_win_user.width != VPP_PIP_BUILTIN_FRAME_WIDTH)) {
                    crop_recalculation_required = 1;
                    data_loader_update_required = 1;
                    plane->ref_win_user_init = 1;
                    plane->ref_win_user.width = VPP_PIP_BUILTIN_FRAME_WIDTH;
                    plane->ref_win_user.height = VPP_PIP_BUILTIN_FRAME_HEIGHT;
                    VPPAPI_DBG_INFO("VPPDBG: PIP builtInFrame set to (%d, %d)  vbi_num=%d cpcbID=%d\n",
                                                plane->ref_win_user.width, plane->ref_win_user.height, vpp_obj->dv[cpcbID].vbi_num, cpcbID);
                } else if (((pinfo->builtinFrame != TRUE)) &&
                    (VPP_PIP_BUILTIN_FRAME_WIDTH == plane->ref_win_user.width)) {
                    /* Reverting to actual ref window upon receiving a new nonBuiltInFrame*/
                    plane->ref_win_user.width = pinfo->m_active_width;
                    plane->ref_win_user.height = pinfo->m_active_height;
                    plane->ref_win_user_init = 1;
                    VPPAPI_DBG_INFO("VPPDBG: PIP nonbuiltInFrame set to (%d, %d)  crop_recalculation_required =%d data_loader_update_required=%d, vbi_num=%d cpcbID=%d\n",
                    plane->ref_win_user.width, plane->ref_win_user.height, crop_recalculation_required, data_loader_update_required, vpp_obj->dv[cpcbID].vbi_num, cpcbID);
                }
            }
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
            if((pinfo->builtinFrame == TRUE) && (chan->built_in_frame_scl_required))
            {
                if(!chan->built_in_frame_pushed)
                {
                    VDBG_DISP_BUILTIN_FRAME("1. active:wid/hgt : %d/%d, rewin:wid/hgt : %d/%d, \
                            ref_win_bkup:%d/%d, ref_win_init(%d):%d/%d, disp_win_updated=%d, \
                            disp_win_user=%dx%d:%dx%d\n",
                            pinfo->m_active_width, pinfo->m_active_height,
                            plane->ref_win.width,  plane->ref_win.height,
                            chan->ref_win_user.width,chan->ref_win_user.height,
                            plane->ref_win_user_init, plane->ref_win_user.width,
                            plane->ref_win_user.height, chan->disp_win_user_updated,
                            chan->disp_win_user.x, chan->disp_win_user.y,
                            chan->disp_win_user.width, chan->disp_win_user.height);
                    //take backup of the current settings - dispwin
                    if(!chan->disp_win_user_updated) {
                        //take backup only if there is no change done in current ISR itself
                        chan->disp_win_user = chan->disp_win;
                    }

                    //take backup of the current settings - refwin
                    //Ideally this is not required, as user configuration is temporary stored
                    //in plane->ref_win_user
                    chan->ref_win_user = plane->ref_win;

                    //Make 1:1 scaling configuration - refwin
                    plane->ref_win.x =  plane->ref_win.y = 0;
                    plane->ref_win.width = pinfo->m_active_width;
                    plane->ref_win.height = pinfo->m_active_height;
                    plane->ref_win_updated = 1;

                    //Make 1:1 scaling configuration - dispwin
                    chan->disp_win = plane->ref_win;
                    chan->disp_win_updated = 1;

                    //Initialize the built-in frame related variables
                    chan->built_in_frame_pushed = 1;
                }
                //Allow refwin to be updated by user during built-in frame scenario,
                //but don't take action
                if(plane->ref_win_user_init == 1) {
                    VDBG_DISP_BUILTIN_FRAME("1.1 active:wid/hgt : %d/%d, rewin:wid/hgt : %d/%d, \
                            ref_win_bkup:%d/%d, ref_win_init(%d):%d/%d\n",
                            pinfo->m_active_width, pinfo->m_active_height,
                            plane->ref_win.width,  plane->ref_win.height,
                            chan->ref_win_user.width,chan->ref_win_user.height,
                            plane->ref_win_user_init, plane->ref_win_user.width,
                            plane->ref_win_user.height);
                    //Store the user defined ref_win in backup variable
                    chan->ref_win_user = plane->ref_win_user;
                    plane->ref_win_user_init = 0;
                }
            }
            else if((chan->built_in_frame_pushed == 1) || (chan->disp_win_user_updated == 1) ||
                    (plane->ref_win_user_init == 1))
            {
                if(((chan->built_in_frame_pushed == 1) ||
                            (vpp_obj->resIdUpdated[cpcbID])||
                            (chan->disp_win.x != chan->disp_win_user.x)||
                            (chan->disp_win.y != chan->disp_win_user.y)||
                            (chan->disp_win.width != chan->disp_win_user.width)||
                            (chan->disp_win.height != chan->disp_win_user.height)) &&
                        (!((chan->disp_win_user.width<=0) || (chan->disp_win_user.height<=0))))
                {
                    VDBG_DISP_BUILTIN_FRAME("2. active:wid/hgt : %d/%d, rewin:wid/hgt : %d/%d, \
                            ref_win_bkup:%d/%d, ref_win_init(%d):%d/%d\n",
                            pinfo->m_active_width, pinfo->m_active_height,
                            plane->ref_win.width,  plane->ref_win.height,
                            chan->ref_win_user.width,chan->ref_win_user.height,
                            plane->ref_win_user_init, plane->ref_win_user.width,
                            plane->ref_win_user.height);

                    //Apply user setting or restore the previous non built-in frame setting - dispwin
                    chan->disp_win = chan->disp_win_user;
                    chan->disp_win_updated = 1;
                    chan->disp_win_user_updated = 0;

                    //Apply user setting or restore the previous non built-in frame setting - refwin
                    if(chan->built_in_frame_pushed == 1) {
                        if(((chan->ref_win_user.width <= 0) && (chan->ref_win_user.height <= 0))) {
                            //Actually this is not required
                            chan->ref_win_user.width = pinfo->m_active_width;
                            chan->ref_win_user.height = pinfo->m_active_height;
                        }
                        //Restore the ref_win from backup variable
                        plane->ref_win = chan->ref_win_user;
                        plane->ref_win_updated = 1;
                        VDBG_DISP_BUILTIN_FRAME("Changed to normal frame : disp_win_updated:%d, \
                                ref_win_updated:%d\n",
                                chan->disp_win_updated,plane->ref_win_updated);
                        chan->built_in_frame_pushed = 0;
                    }
                }
                //Allow ref_win changes from user
                if(plane->ref_win_user_init == 1) {
                    VDBG_DISP_BUILTIN_FRAME("3. active:wid/hgt : %d/%d, rewin:wid/hgt : %d/%d, \
                            ref_win_bkup:%d/%d, ref_win_init(%d):%d/%d\n",
                            pinfo->m_active_width, pinfo->m_active_height,
                            plane->ref_win.width, plane->ref_win.height,
                            chan->ref_win_user.width,chan->ref_win_user.height,
                            plane->ref_win_user_init, plane->ref_win_user.width,
                            plane->ref_win_user.height);
                    //Override backup variable with the latest ref_win specified by user during
                    //built-in to non-built-in transition
                    //or apply re_win during non-built-in scenario
                    plane->ref_win = plane->ref_win_user;
                    plane->ref_win_updated = 1;
                    plane->ref_win_user_init = 0;
#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
                    //FIXME:graphics plane need not calculate the display offset;
                    //instead send x,y through pinfo->m_active_left, pinfo->m_active_top
                    //FIXME:Note application can set its own left per frame
                    //in pinfo->m_active_left, pinfo->m_active_top and they are also
                    //expected to be -ve number
                    pinfo->m_active_left = -plane->ref_win.x;
                    pinfo->m_active_top  = -plane->ref_win.y;
#else
                    if(planeID == PLANE_GFX1)
                    {
                        pinfo->m_active_left = -plane->ref_win.x;
                        pinfo->m_active_top  = -plane->ref_win.y;
                    }
#endif //ENABLE_REFWIN_FOR_FPGA_TESTING
                }
            }
            if(chan->disp_win_updated && ((VPP_GetResId(cpcbID)!=RES_INVALID) &&
                        (VPP_GetResId(cpcbID)<RES_RESET))) {
                VDBG_DISP_BUILTIN_FRAME("#%s-Built-in Frame:Change Dispwin(planeID:%d: \
                    disp_win:<%d,%d,%d,%d>, disp_win_user:<%d,%d,%d,%d>,"
                        " ref_win:<%d,%d,%d,%d>, ref_win_user:<%d,%d,%d,%d>, res:<%d>\n",
                        chan->built_in_frame_pushed?"":"Non", planeID,
                        chan->disp_win.x,chan->disp_win.y,
                        chan->disp_win.width,chan->disp_win.height,
                        chan->disp_win_user.x,chan->disp_win_user.y,
                        chan->disp_win_user.width,chan->disp_win_user.height,
                        plane->ref_win.x,plane->ref_win.y,
                        plane->ref_win.width,plane->ref_win.height,
                        chan->ref_win_user.x,chan->ref_win_user.y,
                        chan->ref_win_user.width,chan->ref_win_user.height,
                        VPP_GetResId(cpcbID));

                /* crop display window */
                cropDispWindow(&(chan->disp_win),
                        pDVResinfo,
                        &(chan->crop_disp_win), chan);
            }
#endif //CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1

#if defined(PE_3D_BD)
            if (pDVResinfo->flag_3d)
            {
                /* get ss offset */
                if (plane->ss_offset_mode == SS_OFFSET_NONE)
                {
                    ss_offset = 0;
                }
                else if (plane->ss_offset_mode == SS_OFFSET_DEFAULT)
                {
                    ss_offset = plane->ss_offset_default;
                }
                else
                {
                    if (vpp_obj->plane[PLANE_MAIN].ss_type == 1)
                    {
                        if ((ss_seq_id = plane->ss_offset_seq_id) == 0xFF)
                            //ss_offset = plane->ss_offset_default;
                            //if the id is oxff which means invalid, use 0 as the offset
                            ss_offset = 0;
                        else
                            ss_offset = (char)vpp_obj->ss_offset_meta_data[ss_seq_id];
                    }
                    else
                    {
                        ss_offset = plane->ss_offset_default;
                    }
                }

                /* record active window */
                pinfo_active_left_record = pinfo->m_active_left;

                /* adjust active window, content offset will be adjusted later */
                if ((vpp_obj->dv[cpcbID].viewID == 0) && (ss_offset > 0))
                {
                    /* left view shifts closer*/
                    pinfo->m_active_left += ss_offset; //shift right
                }
                else if ((vpp_obj->dv[cpcbID].viewID == 1) && (ss_offset < 0))
                {
                    /* right view shifts far away */
                    pinfo->m_active_left += (UINT32)(ss_offset & 0x7F); //shift right
                }
                else if ((vpp_obj->dv[cpcbID].viewID == 0) && (ss_offset < 0))
                {
                    /* left view shifts far away */
                    pinfo->m_active_left -= (UINT32)(ss_offset & 0x7F); //shift left
                }
                else
                {
                    /* right view shifts closer */
                    pinfo->m_active_left -= ss_offset; //shift left
                }

                /* cover the case that offset change from non zero to zero */
                if (ss_offset != plane->ss_offset_current)
                {
                    crop_recalculation_required = 1;
                    plane->ss_offset_current = ss_offset;
                }
            }
#endif

#ifdef PE_3D_FMT_CONVERT
            /* convert main video from SbS or TaB to FP */
            if (planeID == PLANE_MAIN)
            {
                /* get the source 3D format */
                if (vpp_obj->src_3D_cvt_mode == SRC_3D_FMT_AUTO)
                {
                    if (pinfo->m_3D_source_fmt == VDEC_BUF_3D_FMT_SBS)
                    {
                        src_3D_fmt = SRC_3D_FMT_SBS;
                    }
                    else if (pinfo->m_3D_source_fmt == VDEC_BUF_3D_FMT_TAB)
                    {
                        src_3D_fmt = SRC_3D_FMT_TAB;
                    }
                    else
                    {
                        src_3D_fmt = SRC_3D_FMT_FP;
                    }
                }
                else
                {
                    src_3D_fmt = vpp_obj->src_3D_cvt_mode;
                    if (src_3D_fmt == SRC_3D_FMT_SBS)
                    {
                        pinfo->m_3D_source_fmt = VDEC_BUF_3D_FMT_SBS;
                        pinfo->m_3D_SBS_sampling_mode = SBS_HORIZONTAL_MODE0;
                        pinfo->m_3D_interpretation_type = 0 ;
                    }
                    else if (src_3D_fmt == SRC_3D_FMT_SBS_SENSIO)
                    {
                        pinfo->m_3D_source_fmt = VDEC_BUF_3D_FMT_SBS;
                        pinfo->m_3D_SBS_sampling_mode = SBS_QUINCUNX_MODE1;
                        pinfo->m_3D_interpretation_type = 0 ;
                    }
                }

                if((pinfo->m_3D_source_fmt == VDEC_BUF_3D_FMT_SBS) &&
                        (pinfo->m_3D_SBS_sampling_mode == SBS_QUINCUNX_MODE1))
                {
                    if((pinfo->m_active_width <= 640) ||
                            (pDVResinfo->frame_rate<=FRAME_RATE_60))
                    {
                        // input is 720 or output frame rate <= 60
                    }
                    else
                    {
                        // input is 1080 and output frame rate > 60
                    }
                }

                main_ref_width_record = plane->ref_win.width;
                main_ref_height_record = plane->ref_win.height;
                pinfo_active_left_record = pinfo->m_active_left;
                pinfo_active_top_record = pinfo->m_active_top;

                /* convert side-by-side to frame-packing */
                if ((src_3D_fmt == SRC_3D_FMT_SBS)||(src_3D_fmt == SRC_3D_FMT_SBS_SENSIO))
                {
                    plane->ref_win.width = plane->ref_win.width / 2;
                    if (((vpp_obj->dv[cpcbID].viewID == 1) &&
                                (pinfo->m_3D_interpretation_type == 0)) ||
                            ((vpp_obj->dv[cpcbID].viewID == 0) &&
                             (pinfo->m_3D_interpretation_type == 1)))
                    {
                        pinfo->m_active_left -= (INT32)(pinfo->m_active_width / 2);
                    }
                }
                /* convert top-and-bottom to frame-packing */
                if (src_3D_fmt == SRC_3D_FMT_TAB)
                {
                    plane->ref_win.height = plane->ref_win.height / 2;
                    if (((vpp_obj->dv[cpcbID].viewID == 1) &&
                                (pinfo->m_3D_interpretation_type == 0)) ||
                            ((vpp_obj->dv[cpcbID].viewID == 0) &&
                             (pinfo->m_3D_interpretation_type == 1)))
                    {
                        pinfo->m_active_top -=  (INT32)(pinfo->m_active_height / 2);
                    }
                }
            }
#endif
            actv_win = &(plane->actv_win);
            if ((pinfo->m_disp_offset + pinfo->m_content_offset) != plane->disp_offset)
            {
                plane->disp_offset = (pinfo->m_disp_offset + pinfo->m_content_offset);
                /* display content location & size changed, recalculate cropping */
                crop_recalculation_required = 1;
            }

#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
            if(plane->uchInverseScanModeUpdated)
            {
                //Update Bit-Map if inverse scan is updated.
                FE_DLR_SetBitmap(vpp_obj, planeID);
                plane->uchInverseScanModeUpdated = 0;
            }
#endif
            /* check whether reference window is changed */
            if (plane->ref_win_updated)
            {
                VDBG_IF_DISP_ENABLE(1);
                //Try to trigger the delayed GetClkSignature for ref/disp win
                VPP_ClkGate_triggerDelayOperation(vpp_obj, VPP_CLKGATE_DLY_OPT_REF_DISP_WIN);
                plane->ref_win_updated = 0;

#ifdef CONFIG_FORCE_LINEDROPPING_FOR_GFX1
                if(PLANE_MAIN == planeID)
                {
                    //If MAIN plane refwin is changed, then force GFX1
                    //refwin to enable/disable line dropping
                    vpp_obj->plane[PLANE_GFX1].ref_win_updated = 1;
                }
#endif //CONFIG_FORCE_LINEDROPPING_FOR_GFX1

                if((cpcbID == CPCB_1) && (planeID == PLANE_MAIN)) {
                    VPP_DV_DataUpdate(vpp_obj, plane, pinfo, DV_DATA_REFWIN);
                }
                if((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable == 0))
                {
                    if(pinfo->m_chroma_left_ofst != 0 || pinfo->m_chroma_left_ofst != 0)
                    {
                        plane->m_SampleCropEnable = 1;
                    }
                }
                /* reference window changed, recalculate cropping */
                crop_recalculation_required = 1;
                /*Let us update loder each time  refwin is changed */
                data_loader_update_required = 1;
            }

            /* check whether display window is changed */
            if (chan->disp_win_updated)
            {
                VDBG_IF_DISP_ENABLE(1);
                //Try to trigger the delayed GetClkSignature for ref/disp win
                VPP_ClkGate_triggerDelayOperation(vpp_obj, VPP_CLKGATE_DLY_OPT_REF_DISP_WIN);
                chan->disp_win_updated = 0;
                /* display window changed, recalculate cropping */
                crop_recalculation_required = 1;
                cpcb_bg_window_update_required = 1;
                /*Any change in crop calculation may require to change CROP control register in FE/SCL*/
                data_loader_update_required = 1;
                if((cpcbID == CPCB_1) && (planeID == PLANE_MAIN)) {
                    VPP_DV_DataUpdate(vpp_obj, plane, pinfo, DV_DATA_DISPWIN);
                }
                chan->disp_win_final = chan->disp_win;
                chan->crop_disp_win_final = chan->crop_disp_win;

#ifdef VPP_ENABLE_INTERLACED_OUTPUT_SCALING_CROPING
                    if(SCAN_INTERLACED == pResinfo->scan)
                    {
                        /* The display & crop should be divided by 2 for interlaced output */
                        chan->disp_win_final.y = (int)(chan->disp_win.y/2);
                        chan->disp_win_final.height = (chan->disp_win.height+1)/2;
                        chan->crop_disp_win_final.y = (int)(chan->crop_disp_win.y/2);
                        chan->crop_disp_win_final.height = (chan->crop_disp_win.height+1)/2;
                        VDBG_DISP("Interlaced Output display window : \n\tdisp_win(starty,height) : org(%d,%d) -> new(%d,%d)"
                                "\n\tcrop_disp_win(starty,height) : org(%d,%d) -> new(%d,%d)\n",
                                chan->disp_win.x, chan->disp_win.height, chan->disp_win_final.x, chan->disp_win_final.height,
                                chan->crop_disp_win.y,chan->crop_disp_win.height, chan->crop_disp_win_final.y, chan->crop_disp_win_final.height);
                    }
#endif
                    VDBG_DISP("CROP CALCULATION Done , data_loader_update_required : %d \n",data_loader_update_required);
                }

#ifdef CONFIG_DEINT_ENABLE
            //Clock/Power gate DEINT for progressive frame
            if(planeID == PLANE_MAIN)
                VPP_ClkGate_FE_DEINT(vpp_obj,
                        pinfo->m_is_frame_seq? VPP_CLK_GATE_ENABLE:VPP_CLK_GATE_DISABLE);
#endif

            if (crop_recalculation_required)
            {
                int dataAlign_x = VPP_OVL_getDataAlignBytes_X(plane->uchClientMode,
                        pinfo->m_content_width);
                int dataAlign_y = VPP_OVL_getDataAlignBytes_Y(plane->uchClientMode,
                        pinfo->m_content_height);

#ifdef CONFIG_AMP_VPP_ROTATION
                if (ROTATION_90D == plane->uchRotationAngle ||
                        ROTATION_270D ==  plane->uchRotationAngle)
                {
                    SWAP_W_H(dataAlign_x, dataAlign_y);
                    if((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                            (10 == pinfo->m_bits_per_pixel))
                    {
                        dataAlign_x = 24;
                        dataAlign_y = 96;
                    }
                }
                if ((ROTATION_0D == plane->uchRotationAngle) &&
                        (plane->m_SampleCropEnable)&&(IS_MTR_ENABLED(plane->m_mtrR__en)))
                {
                    if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt)
                    {
                        /* Multiple of compressed tile width*/
                        dataAlign_x = 64;
                    }
                    else if((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                            (10 == pinfo->m_bits_per_pixel))
                    {
                        /* Multiple of compressed tile width*/
                        dataAlign_x = 48;
                    }
                    dataAlign_y = 4;
                }
#endif

#ifdef ENABLE_SCALAR_LINEDROPPING
                int org_ref_win_height = plane->ref_win.height;
                int org_ref_win_width = plane->ref_win.width;
#ifdef CONFIG_AMP_VPP_ROTATION
                if (ROTATION_90D == plane->uchRotationAngle ||
                        ROTATION_270D ==  plane->uchRotationAngle)
                {
                    SWAP_W_H(plane->ref_win.width, plane->ref_win.height);
                }
#endif
#ifdef ENABLE_OVPSCL_PIP
                    uiInterlacedFactor = (SCAN_INTERLACED == pResinfo->scan)? 2 : 1;
                    if((planeID == PLANE_PIP) && (!IS_GFX_SRC_FORMAT(pinfo->m_srcfmt)) &&
                            (plane->ref_win.height/uiInterlacedFactor > chan->disp_win_final.height || plane->ref_win.width > chan->disp_win_final.width))
                    {
                        plane->ref_win.height = pinfo->m_active_height;
                        plane->ref_win.width = pinfo->m_active_width;
                        chan->disp_win_final.height = plane->ref_win.height;
                        chan->disp_win_final.width = plane->ref_win.width;
                        VPPAPI_DBG_INFO("PIP down Scaling not supported. Fallback to 1:1\n");
                    }
#endif
                stVppSclRes.IVRes = plane->ref_win.height;
                stVppSclRes.IHRes = plane->ref_win.width;
                stVppSclRes.OVRes = chan->disp_win_final.height;
                stVppSclRes.OHRes = chan->disp_win_final.width;
                stVppSclRes.x     = 0;
                stVppSclRes.y     = 0;
                VPP_OfflineScl_ConfigLineDrop(vpp_obj, &stVppSclRes, chanID);

                if(1 == plane->sclLineDropEn)
                {

#ifdef ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA
                        UNSG32 vpp_sys_clk = 600000;
                        int mult_fact_vratio = 100;
                        int req_vscale_ratio = 1;
                        int max_vscale_ratio = 1;
                        int org_lineDrop;

                        VPP_GetVppSysClk_KHz(vpp_obj, &vpp_sys_clk,chanID);

                        max_vscale_ratio = mult_fact_vratio;
                        max_vscale_ratio *= vpp_sys_clk;
                        max_vscale_ratio /= pResinfo->freq;
                        max_vscale_ratio *= pResinfo->active_width; //active width or htotal???
                        max_vscale_ratio /= plane->ref_win.width;
                        //max_vscale_ratio = (mult_fact_vratio*m_resinfo_table[vpp_obj->uiResID[cpcbID]].width*vpp_sys_clk*plane->lineDroppingStrides)/(m_resinfo_table[vpp_obj->uiResID[uiResID]].freq*plane->ref_win.width);
                        plane->lineDroppingStrides = 0;
                        do
                        {
                            plane->lineDroppingStrides++;
                            req_vscale_ratio = mult_fact_vratio;
                            req_vscale_ratio *= plane->ref_win.height;
                            req_vscale_ratio /= plane->lineDroppingStrides;
                            req_vscale_ratio /= chan->disp_win_final.height;

                            //printk("ratio: %d %d %d\n", req_vscale_ratio, max_vscale_ratio, plane->lineDroppingStrides);
                        }while(req_vscale_ratio > max_vscale_ratio);


#ifdef CONFIG_FORCE_LINEDROPPING_FOR_GFX1
                    if(planeID == PLANE_GFX1) {
                        /*If not downscaling and MAIN plane has frame > 2K,
                          then force line dropping on GFX1
                         *TBD:Need to check minimum input limit to allow line dropping, e.g. 720 */
                        if((plane->lineDroppingStrides == 1) &&
                                ((vpp_obj->plane[PLANE_MAIN].curr_frame) &&
                                 (vpp_obj->scl_res[VPP_FRC_SCL_MAIN].IVRes > 1080))) {
                            plane->lineDroppingStrides = 2;
                            VDBG_IF_DISP("GFX1 lineDroppingStrides : %d, srcfmt : %d, bpp : %d\n",
                                    plane->lineDroppingStrides,  plane->srcfmt, plane->bpp);
                        }
                    }
#endif //CONFIG_FORCE_LINEDROPPING_FOR_GFX1

                    if((planeID == PLANE_PIP) || (planeID == PLANE_GFX1))
                    {
                        // if the the downscale is more than 20% increase Line dropping stride
                        if((stVppSclRes.OVRes < stVppSclRes.IVRes &&
                                    SCAN_PROGRESSIVE == pResinfo->scan))
                        {
                            if(((((stVppSclRes.IVRes - stVppSclRes.OVRes) * 100 ) /
                                            stVppSclRes.IVRes) >= 20 ) &&
                                    (stVppSclRes.OVRes < (stVppSclRes.IVRes/plane->lineDroppingStrides)))
                                plane->lineDroppingStrides = plane->lineDroppingStrides + 1;
                        }
                        if (IS_GFX_SRC_FORMAT(pinfo->m_srcfmt) &&
                                plane->ref_win.width > 1920 &&
                                SCAN_INTERLACED == pResinfo->scan) {
                            plane->lineDroppingStrides *= 2;
                        }
                    }

                    if (isVPPExtraLineDropRequired(pResinfo, chan, plane))
                        plane->lineDroppingStrides++;

                    if(VPP_IS_LIMIT_DOWNSCL_AFTER_RES_CHANGE(planeID, plane->ref_win.width, chan->disp_win_final.width, pResinfo, vpp_obj->dv[cpcbID].vbi_num, plane->m_mtrR__en)) {
                        orgRefWidth = plane->ref_win.width;
                        plane->ref_win.width = (((int)((chan->disp_win_final.width) / VPP_DHUB_SIZE))*VPP_DHUB_SIZE);
                        plane->ref_win_updated = 1;
                    }

                    org_lineDrop = plane->lineDroppingStrides;
                    VDBG_IF_DISP("lineDroppingStrides : %d, srcfmt : %d, bpp : %d\n",
                            plane->lineDroppingStrides,  plane->srcfmt, plane->bpp);
                    //Limit linedropping when stride size(in bytes) is bigger
                    //then allowed limit VPP_LINEDROP_STRIDE_MAX(16bits-0xFFFF)
                    //e.g : 10bit tiled mode : whenever hgt < 541,
                    //we see garbage, i.e /4, so avoid going above /3 factor
                    if(plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1
#ifdef CONFIG_AMP_VPP_ROTATION
                            || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
#endif
                      )
                    {
                        if((pinfo->m_buf_stride*plane->lineDroppingStrides)*tiledModeStrideFactor >
                                VPP_LINEDROP_STRIDE_MAX)
                            plane->lineDroppingStrides = VPP_LINEDROP_STRIDE_MAX /
                                (pinfo->m_buf_stride*tiledModeStrideFactor);
                    }
                    else
                    {
                        if((pinfo->m_buf_stride*plane->lineDroppingStrides) > VPP_LINEDROP_STRIDE_MAX)
                            plane->lineDroppingStrides = VPP_LINEDROP_STRIDE_MAX/pinfo->m_buf_stride;
                    }
                    if(org_lineDrop != plane->lineDroppingStrides)
                    {
                        printf("**** %d %s >>  Stride Limitaion ******* \n", __LINE__, __func__);
                        VDBG_IF_DISP("Changing lineDroppingStrides from : %d to %d\n",
                                org_lineDrop, plane->lineDroppingStrides);
                    }
                    plane->ref_win.height /= plane->lineDroppingStrides;
#else //ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA

                    int max_downscale_hratio = 1;
                    int mult_fact_hratio = 1000;
                    int lineDrop4KSel = is4KInputLineDroppingRequired(vpp_obj, chanID);

                    if(SCAN_PROGRESSIVE == pResinfo->scan)
                        max_downscale_hratio = 2*mult_fact_hratio;
                    else
                        max_downscale_hratio = 4*mult_fact_hratio;

                    //check if hscale ratio is > 2 to enable line dropping
                    if(plane->ref_win.height > chan->disp_win_final.height &&
                            (((plane->ref_win.height*mult_fact_hratio/
                               (chan->disp_win_final.height*1)) > max_downscale_hratio) ||
                             ( 1 == lineDrop4KSel)))
                    {
                        int lineDropSel;    //Line selected for processing in line drop case
                        int tempStride = 0;
                        if(plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1
#ifdef CONFIG_AMP_VPP_ROTATION
                                || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
#endif
                          )
                        {
                            tileYAlign = 4;
                            tiledModeStrideFactor=VPP_TILED_MODE_STRIDE_FACTOR==1?4:1;
                            tiledModeWidHgtFactor=1*4;
                        }
#ifdef ENABLE_CROMA_ALLIGNED_LINEDROP
                        if((SRCFMT_YUV420SP == plane->srcfmt) ||
                                (SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) ||
                                (SRCFMT_YUV420SP_TILED_MODE1== plane->srcfmt) ||
                                (SRCFMT_YUV420SP_DWA == plane->srcfmt)
#ifdef CONFIG_AMP_VPP_ROTATION
                                || (SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt)
#endif
                          )
                        {
                            uiCromaAllignedLinedropFactor = 2;
                        }
#endif
                        //enable line dropping
                        if(1 == lineDrop4KSel)
                        {
                            plane->lineDroppingStrides = 2;
                            plane->ref_win.height/=plane->lineDroppingStrides;
                        }
                        else
                        {
                            plane->lineDroppingStrides = plane->ref_win.height/
                                chan->disp_win_final.height;
                            VDBG_IF_DISP("lineDroppingStrides : %d, srcfmt : %d, bpp : %d\n",
                                    plane->lineDroppingStrides,  plane->srcfmt, plane->bpp);
                            int org_lineDrop = plane->lineDroppingStrides;
                            //Limit linedropping when stride size(in bytes) is bigger
                            //then allowed limit VPP_LINEDROP_STRIDE_MAX(16bits-0xFFFF)
                            //e.g : 10bit tiled mode : whenever hgt < 541, we see garbage,
                            //i.e /4, so avoid going above /3 factor
                            tempStride = (pinfo->m_buf_stride*plane->lineDroppingStrides)*
                                tiledModeStrideFactor*uiCromaAllignedLinedropFactor;
                            if(tempStride > VPP_LINEDROP_STRIDE_MAX)
                                plane->lineDroppingStrides = VPP_LINEDROP_STRIDE_MAX/
                                    (pinfo->m_buf_stride*tiledModeStrideFactor*
                                     uiCromaAllignedLinedropFactor);

                            if(org_lineDrop != plane->lineDroppingStrides)
                            {
                                printf("**** %d %s >>  Stride Limitaion ******\n", __LINE__, __func__);
                                VDBG_IF_DISP("Changing lineDroppingStrides from : %d to %d\n",
                                        org_lineDrop, plane->lineDroppingStrides);
                            }

                            //reduce the ref_win height by same amount
                            plane->ref_win.height/=plane->lineDroppingStrides;
                        }
                        lineDropSel = getLineDroppingStartY(plane);
                        if (!pinfo->m_is_frame_seq && (planeID == PLANE_MAIN))
                        {
                            //Ensure StartY is at even for interlaced i/p, specifically SP formats
                            lineDropSel &= (~0x01);
                        }
                        /*pinfo->m_active_top store the starty as -ve number
                          so add lineDropSel as -ve number to move the startY
                          by required number of lines */
                        VDBG_IF_DISP("pinfo->m_active_top : %d\n", pinfo->m_active_top);
                        pinfo->m_active_top  = pinfo->m_active_top + (-lineDropSel*tileYAlign);
                        VDBG_IF_DISP("lineDroppingStrides        : %d\n",
                                plane->lineDroppingStrides);
                        VDBG_IF_DISP("lineDroppingStyle->LineSel : %d->%d\n",
                                plane->lineDroppingStyle,lineDropSel);
                        VDBG_IF_DISP("temp ref_win.height : %d\n", plane->ref_win.height);
                        VDBG_IF_DISP("pinfo->m_active_top : %d\n", pinfo->m_active_top);
                    }
                    else
                    {
                        plane->lineDroppingStrides = 0;
                    }
#endif //!ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA
                }
#ifdef CONFIG_AMP_VPP_ROTATION
                if (ROTATION_90D == plane->uchRotationAngle ||
                        ROTATION_270D ==  plane->uchRotationAngle)
                {
                    SWAP_W_H(plane->ref_win.width, plane->ref_win.height);
                }
#endif
#endif //ENABLE_SCALAR_LINEDROPPING
#ifdef VPP_APPLY_GFX_SCL_REFWIN_LIMITATION
                /* GFX0/GFX1 GFX src format frame should not have
                 * ref_win.width > 1920 expect for 1:1 or
                 * horizontal only(w/ or w/o linedrop) scaling */
                if(IS_GFX_SRC_FORMAT(pinfo->m_srcfmt) &&
                        (plane->ref_win.height != chan->disp_win_final.height)) {
                    if(plane->ref_win.width > 1920) {
                        plane->ref_win.width = 1920;
                    }
                }
#endif //VPP_APPLY_GFX_SCL_REFWIN_LIMITATION
                /* crop display content */
                if(SRCFMT_YUV420SP == pinfo->m_srcfmt ||
                        plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                        plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                        SRCFMT_YUV420SP_DWA == pinfo->m_srcfmt ||
                        SRCFMT_YUV422SP_DWA == pinfo->m_srcfmt
#ifdef CONFIG_AMP_VPP_ROTATION
                        || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt
#endif
                  )
                {
                    cropDispContent_UV(&crop_actv_win, &crop_actv_offset_UV, &crop_scl_actv_win, pinfo, &plane->ref_win, &chan->disp_win_final, pDVResinfo, dataAlign_x, dataAlign_y, plane->bpp, plane);
                }

                cropDispContent(&crop_actv_win, &crop_actv_offset, &crop_scl_actv_win, pinfo, &plane->ref_win, &chan->disp_win_final, pDVResinfo, dataAlign_x, dataAlign_y, plane->bpp, plane);

                if(IS_MTR_ENABLED(plane->m_mtrR__en))
                {
                    if(plane->m_leftpad_y != plane->m_leftpad_c)
                    {
                        // Crop-2 is required for the case when Luma and Chroma
                        //left-offsets are not same.
                        vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en = 1;
                        vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en = 1;
                        plane->m_SampleCropEnable = 1;
                    }
                    else
                    {
                        vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en = 0;
                        vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en = 0;
                    }

                    if(plane->m_Aligned_Luma_Width > plane->m_Aligned_Chroma_Width)
                    {
                        if(plane->uDhubCropOffset_c.u32_Tile_Xoffset != 0)
                        {
                            plane->m_leftpadadjust_c +=
                                (plane->m_Aligned_Luma_Width - plane->m_Aligned_Chroma_Width);
                        }
                        else
                        {
                            plane->m_rightpad_c +=
                                (plane->m_Aligned_Luma_Width - plane->m_Aligned_Chroma_Width);
                        }
                        plane->m_Aligned_Chroma_Width = plane->m_Aligned_Luma_Width;
                        plane->uDhubCropOffset_c.u32_Tile_Xoffset =
                            plane->uDhubCropOffset_y.u32_Tile_Xoffset;
                    }
                    else if(plane->m_Aligned_Luma_Width < plane->m_Aligned_Chroma_Width)
                    {
                        if(plane->uDhubCropOffset_y.u32_Tile_Xoffset != 0)
                        {
                            plane->m_leftpadadjust_y +=
                                (plane->m_Aligned_Chroma_Width - plane->m_Aligned_Luma_Width);
                        }
                        else
                        {
                            plane->m_rightpad_y +=
                                (plane->m_Aligned_Chroma_Width - plane->m_Aligned_Luma_Width);
                        }
                        plane->m_Aligned_Luma_Width = plane->m_Aligned_Chroma_Width;
                        plane->uDhubCropOffset_y.u32_Tile_Xoffset =
                            plane->uDhubCropOffset_c.u32_Tile_Xoffset;
                    }
                }
#ifdef ENABLE_SCALAR_LINEDROPPING
                if((1 == plane->sclLineDropEn) && plane->lineDroppingStrides)
                {
#ifdef CONFIG_AMP_VPP_ROTATION
                    if (ROTATION_90D == plane->uchRotationAngle ||
                            ROTATION_270D ==  plane->uchRotationAngle)
                    {
                        plane->ref_win.width = org_ref_win_width;
                    }
                    else
#endif
                    {
                        //restore the ref_win.height to the user specified one
                        plane->ref_win.height = org_ref_win_height;
                    }
                }
                else
#endif //ENABLE_SCALAR_LINEDROPPING
                {
                    //Let us keep lineDroppingStrides = 1 for default case ; i.e no dropping
                    plane->lineDroppingStrides = 1;
                }
                VDBG_IF_DISP("lineDroppingStrides : %d\n", plane->lineDroppingStrides);

                if(planeID != PLANE_OVP_EL)
                {
                    if ((crop_actv_win.width <= 0) || (crop_actv_win.height <= 0) ||
                            (crop_scl_actv_win.width <= 0) || (crop_scl_actv_win.height <= 0))
                    {
                        /* crop all */
                        plane->crop_actv_win.width = 0;
                        plane->crop_actv_win.height = 0;
                        plane->crop_scl_actv_win.width = 0;
                        plane->crop_scl_actv_win.height = 0;
                        if(cpcbID == CPCB_1)
                        {
                            /* close CPCB video source/background plane if opened previously */
                            VPP_CPCB_SetPlaneCropWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
                            VPP_CPCB_SetPlaneSourceWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
                            VPP_CPCB_EnablePlane(vpp_obj, cpcbID, chan->dvlayerID, 0);
                            VPP_CPCB_SetPlaneBGWindow(vpp_obj, cpcbID, chan->dvlayerID,
                                    chan->crop_disp_win_final.x, chan->crop_disp_win_final.y,
                                    chan->crop_disp_win_final.width, chan->crop_disp_win_final.height);
                        }

#if defined(PE_3D_BD)
                        if (pDVResinfo->flag_3d)
                        {
                            /* recover left view start address, if was updated to right view */
                            if ((plane->ss_type == 1) && (pinfo_pbuf_start != 0))
                                pinfo->m_pbuf_start = (ARCH_PTR_TYPE)pinfo_pbuf_start;
                        }
#endif
                        return (MV_VPP_OK);
                    }
                }

                /* crop display content height & width must be a even number for interlaced input */
                if (!pinfo->m_is_frame_seq && (planeID == PLANE_MAIN))
                {
#ifdef ENABLE_SCALAR_LINEDROPPING
                    //Make the height multiple of 4 so that we end up with even number for odd/even field
                    if(1 == plane->sclLineDropEn)
                        crop_actv_win.height &= (~0x03);
                    else
                        crop_actv_win.height &= (~0x01);
#else
                    crop_actv_win.height &= (~0x01);
#endif
                }

                if ((crop_actv_win.width != plane->crop_actv_win.width) ||
                        (crop_actv_win.height != plane->crop_actv_win.height))
                {
                    /* update plane->crop_actv_win */
                    plane->crop_actv_win.width = crop_actv_win.width;
                    plane->crop_actv_win.height = crop_actv_win.height;
                    data_loader_update_required = 1;
                }

                if ((crop_scl_actv_win.x != plane->crop_scl_actv_win.x)
                        || (crop_scl_actv_win.y != plane->crop_scl_actv_win.y)
                        || (crop_scl_actv_win.width != plane->crop_scl_actv_win.width)
                        || (crop_scl_actv_win.height != plane->crop_scl_actv_win.height))
                {
                    /* update plane->crop_scl_actv_win */
                    plane->crop_scl_actv_win.x = crop_scl_actv_win.x;
                    plane->crop_scl_actv_win.y = crop_scl_actv_win.y;
                    plane->crop_scl_actv_win.width = crop_scl_actv_win.width;
                    plane->crop_scl_actv_win.height = crop_scl_actv_win.height;
                    cpcb_source_window_update_required = 1;
                    data_loader_update_required = 1;
                }

#if defined(PE_3D_BD)
                if (pDVResinfo->flag_3d)
                {
                    /* adjust content offset */
                    if ((vpp_obj->dv[cpcbID].viewID == 0) && (ss_offset < 0))
                    {
                        /* left view shifts far away */
                        if ((pinfo->m_active_left >= 0) &&
                                ((pinfo->m_active_left - (ss_offset & 0x7F)) < plane->ref_win.x)){
                            //remove the part cut by ref window
                            crop_actv_offset += pinfo->m_bits_per_pixel/8 *
                                (plane->ref_win.x -
                                 (pinfo->m_active_left - (ss_offset & 0x7F)));
                        } else if (pinfo->m_active_left < 0) {
                            crop_actv_offset += pinfo->m_bits_per_pixel/8 * (ss_offset & 0x7F);
                        }
                    }
                    else if ((vpp_obj->dv[cpcbID].viewID == 1) && (ss_offset > 0))
                    {
                        /* right view shifts closer */
                        if ((pinfo->m_active_left >= 0) &&
                                ((pinfo->m_active_left - ss_offset) < plane->ref_win.x)){
                            //remove the part cut by ref window
                            crop_actv_offset += pinfo->m_bits_per_pixel/8 *
                                (plane->ref_win.x - (pinfo->m_active_left - ss_offset));
                        } else if (pinfo->m_active_left < 0) {
                            crop_actv_offset += pinfo->m_bits_per_pixel/8 * ss_offset;
                        }
                    }

                    /* recover active window */
                    pinfo->m_active_left = pinfo_active_left_record;
                    actv_win->x = pinfo->m_active_left;
                    actv_win->width = pinfo->m_active_width;
                }
#endif

#ifdef PE_3D_FMT_CONVERT
                /* recover main plane reference win */
                if ((planeID == PLANE_MAIN) &&
                        ((src_3D_fmt==SRC_3D_FMT_SBS) ||
                         (src_3D_fmt==SRC_3D_FMT_SBS_SENSIO) ||
                         (src_3D_fmt==SRC_3D_FMT_TAB)))
                {
                    plane->ref_win.width = main_ref_width_record;
                    plane->ref_win.height = main_ref_height_record;
                    pinfo->m_active_left = pinfo_active_left_record;
                    pinfo->m_active_top = pinfo_active_top_record;
                }
#endif

#ifndef CONFIG_AMP_VPP_ROTATION
                //Fix Me: Satya
                //Adjust the base address with the disp_offset & content_offset
                crop_actv_offset -= plane->cropDispContentOffset;
                crop_actv_offset_UV -= plane->cropDispContentOffset;
#endif
                /* update plane->crop_actv_offset */
                plane->crop_actv_offset = crop_actv_offset;
                plane->crop_actv_offset_UV = crop_actv_offset_UV;

            } // <- crop calculation

            if (PLANE_MAIN == planeID) {
                VPP_VMXAPI_Notify(vpp_obj, MV_VPP_VMX_CALLBACK_MAIN_INFO, pinfo);
            }

            if ((plane->crop_actv_win.width <= 0) || (plane->crop_actv_win.height <= 0)){

#if defined(PE_3D_BD)
                if (pDVResinfo->flag_3d)
                {
                    /* recover left view start address, if was updated to right view */
                    if ((plane->ss_type == 1) && (pinfo_pbuf_start != NULL))
                        pinfo->m_pbuf_start = (ARCH_PTR_TYPE)pinfo_pbuf_start;
                }
#endif
                return (MV_VPP_OK);
            }

            /* update data loader if frame type changes */
            if (planeID == PLANE_MAIN)
            {
                if (plane->frame_type != pinfo->m_is_frame_seq)
                {
                    /* crop display content height must be a even number for interlaced input */
                    if (!pinfo->m_is_frame_seq)
                    {
#ifdef ENABLE_SCALAR_LINEDROPPING
                        //Make the height multiple of 4 so that we end up with
                        //even number for odd/even field
                        if(1 == plane->sclLineDropEn)
                            plane->crop_actv_win.height &= (~0x03);
                        else
                            plane->crop_actv_win.height &= (~0x01);
#else
                        plane->crop_actv_win.height &= (~0x01);
#endif
                    }

                    data_loader_update_required = 1;
                    // update current frame type
                    plane->frame_type = pinfo->m_is_frame_seq;
                } // <- update frame type if frame type changes
            }
            else
            {
                if (plane->frame_type == -1)
                {
                    data_loader_update_required = 1;
                    // update current frame type
                    plane->frame_type = 1; // force to progressive
                }
            }

#ifdef ENABLE_OVP
            if(0 == pinfo->m_OrgInputInterlaced)
            {
                if(!(IS_MTR_ENABLED(plane->m_mtrR__en)))
                {
                    crop_actv_offset = VPP_CROP_GetActiveLumaOffset(pinfo, plane);
                    crop_actv_offset_UV = VPP_CROP_GetActiveChromaOffset(pinfo, plane);
                }
            }
            else if(1 == pinfo->m_OrgInputInterlaced)
            {
                //Always buff1 is valid first.
                if(0 == plane->uchFrameBuffInUse)
                {
                    uiBuff1Valid = 1;
                }
                else
                {
                    if(pinfo->m_Out_buf0_valid)
                    {
                        uiBuff0Valid = 1;
                    }
                    else
                    {
                        uiBuff1Valid = 1;
                    }
                }

                if(uiBuff1Valid)
                {
                    crop_actv_offset = (int)pinfo->m_pbuf_Out_start_1 +
                        pinfo->m_disp_offset + pinfo->m_content_offset+plane->crop_actv_offset;
                    crop_actv_offset_UV = (int)pinfo->m_pbuf_Out_start_UV_1 +
                        pinfo->m_disp_offset + pinfo->m_content_offset+plane->crop_actv_offset_UV;
                    //Mark m_FrameDesc_RefStat flag that VPP is using this frame.
                    pinfo->m_FrameDesc_RefStat |= VPP_FRAME_IN_USE;
                    //Buff1 is in use.
                    plane->uchFrameBuffInUse = 1;
                }
                if(uiBuff0Valid)
                {
                    crop_actv_offset = (int)pinfo->m_pbuf_Out_start_0 +
                        pinfo->m_disp_offset + pinfo->m_content_offset+plane->crop_actv_offset;
                    crop_actv_offset_UV = (int)pinfo->m_pbuf_Out_start_UV_0 +
                        pinfo->m_disp_offset + pinfo->m_content_offset+plane->crop_actv_offset_UV;
                    plane->uchFrameBuffInUse = 0;
                    //Check whether previous frame and current frame are same or not.
                    if(vpp_obj->main_last_frame  == inline_frame)
                    {
                        plane->uchFrameBuffInUse = 1;
                    }
                    vpp_obj->main_last_frame = inline_frame;
                    //Mark m_FrameDesc_RefStat flag that VPP is using this frame.
                    pinfo->m_FrameDesc_RefStat |= VPP_FRAME_IN_USE;
                }

            }
#else
            if(!(IS_MTR_ENABLED(plane->m_mtrR__en)))
            {
                crop_actv_offset = (int)pinfo->m_pbuf_start + plane->crop_actv_offset;
                crop_actv_offset_UV = (int)pinfo->m_buf_pbuf_start_UV + plane->crop_actv_offset_UV;
            }

#endif
            //Please add the offset calculated for TILED moved in previous iteration,
            //since the final cropped disp offset is stored in plane->crop_disp_offset
            if ((plane->crop_disp_offset != (crop_actv_offset)) ||
                    (plane->crop_disp_offset_UV != (crop_actv_offset_UV))){
                VDBG_DISP("Adjusted crop_actv_offset:%x,%x, crop_actv_offset_UV:%x,%x\n",
                        crop_actv_offset,plane->crop_disp_offset,crop_actv_offset_UV,
                        plane->crop_disp_offset_UV);
                plane->crop_disp_offset = crop_actv_offset;
                plane->crop_disp_offset_UV = crop_actv_offset_UV;
                //data_loader_crop_required = 1;
            }

            /* update plane data loader resolution */
            if (data_loader_update_required || data_loader_crop_required)
            {
                VPP_FE_DLR_INPUT_CROP feCropRes;
                int actualHStart,actualVStart;
                int topCrop, bottomCrop;
                int leftCrop, rightCrop;
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                int leftAdjust=0, rightAdjust=0, enableAdditionalScalarCropping=0;
#endif
                int dataAlign_x = 0, dataAlign_y = 0;

                VDBG_IF_DISP("data_loader_update_required : %d , data_loader_crop_required : %d\n",
                        data_loader_update_required, data_loader_crop_required);
                //input to FETG
                res.HRes = plane->crop_actv_win.width;
                if (!pinfo->m_is_frame_seq && (planeID == PLANE_MAIN))
                {
#ifdef ENABLE_OVP
                    res.VRes = plane->crop_actv_win.height;
#else
                    res.VRes = plane->crop_actv_win.height/2;
#endif
                }
                else
                {
                    res.VRes = plane->crop_actv_win.height;
                }

                actualHStart = pinfo->m_active_left<0? -pinfo->m_active_left: pinfo->m_active_left;
                actualVStart = pinfo->m_active_top<0? -pinfo->m_active_top: pinfo->m_active_top;
                if(SRCFMT_YUV420SP == plane->srcfmt)
                {
                    /*For 420SP two Y line share one UV line. So aligning to even */
                    actualVStart += actualVStart%2;
                }

                //Adjust the startx with the disp_offset and content_offset
                actualHStart += plane->leftCropAdjustment;

                dataAlign_x = VPP_OVL_getDataAlignBytes_X(plane->uchClientMode, pinfo->m_content_width);
                dataAlign_y = VPP_OVL_getDataAlignBytes_Y(plane->uchClientMode, pinfo->m_content_height);
#ifdef CONFIG_AMP_VPP_ROTATION
                if (ROTATION_90D == plane->uchRotationAngle ||
                        ROTATION_270D ==  plane->uchRotationAngle)
                {
                    SWAP_W_H(dataAlign_x, dataAlign_y);
                    if((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                            (10 == pinfo->m_bits_per_pixel))
                    {
                        dataAlign_x = 24;
                        dataAlign_y = 96;
                    }
                }
#endif
                /* For compressed inputs  Horizontal alignment is 64 &
                   vertical alignment is 4 (64x4 compressed tile)*/
                if ((ROTATION_0D == plane->uchRotationAngle) &&
                        IS_MTR_ENABLED(plane->m_mtrR__en))
                {
                    if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt)
                    {
                        /* Multiple of compressed tile width*/
                        dataAlign_x = 64;
                    }
                    else if((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                            (10 == pinfo->m_bits_per_pixel))
                    {
                        /* Multiple of compressed tile width*/
                        dataAlign_x = 48;
                    }
                    dataAlign_y = 8;
                }

                //aligned input to FETG-Crop & scalar
                //Initially crop and actual are same, If required cropRes will become > final resolution
                cropRes = res;
                leftCrop = actualHStart%dataAlign_x;
                rightCrop = res.HRes%dataAlign_x;

                if(leftCrop)
                {
                    actualHStart -= leftCrop;
                }
                //Hres should be multiple of pixel granularity : res.HRes > cropRes.HRes
                if(rightCrop || leftCrop)
                {
                    res.HRes = ((res.HRes + leftCrop + (dataAlign_x-1)) / dataAlign_x ) * dataAlign_x;

#ifdef ENABLE_FE_FULLCROP
#ifdef CONFIG_AMP_VPP_ROTATION
                    if(pinfo->m_srcfmt == SRCFMT_ARGB32 || plane->srcfmt == SRCFMT_ARGB32 ||
                            pinfo->m_srcfmt == SRCFMT_RGB888 || plane->srcfmt == SRCFMT_RGB888 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB32_PM || plane->srcfmt == SRCFMT_ARGB32_PM ||
                            pinfo->m_srcfmt == SRCFMT_ARGB4444 || plane->srcfmt == SRCFMT_ARGB4444 ||
                            pinfo->m_srcfmt == SRCFMT_ARBG2101010 ||
                            plane->srcfmt == SRCFMT_ARBG2101010 ||
                            pinfo->m_srcfmt == SRCFMT_RGB565|| plane->srcfmt == SRCFMT_RGB565 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB1555|| plane->srcfmt == SRCFMT_ARGB1555 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB8332 || plane->srcfmt == SRCFMT_ARGB8332 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            SRCFMT_YUV420SP == plane->srcfmt ||
                            SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt)
#else
                    if(pinfo->m_srcfmt == SRCFMT_ARGB32 || plane->srcfmt == SRCFMT_ARGB32 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB32_PM || plane->srcfmt == SRCFMT_ARGB32_PM ||
                            pinfo->m_srcfmt == SRCFMT_RGB888 || plane->srcfmt == SRCFMT_RGB888 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB4444 || plane->srcfmt == SRCFMT_ARGB4444 ||
                            pinfo->m_srcfmt == SRCFMT_ARBG2101010 ||
                            plane->srcfmt == SRCFMT_ARBG2101010 ||
                            pinfo->m_srcfmt == SRCFMT_RGB565|| plane->srcfmt == SRCFMT_RGB565 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB1555|| plane->srcfmt == SRCFMT_ARGB1555 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB8332 || plane->srcfmt == SRCFMT_ARGB8332 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            SRCFMT_YUV420SP == plane->srcfmt)
#endif
                        //For tiled format, we will have always read complete stride
#else
                    if(IS_GFX_SRC_FORMAT(pinfo->m_srcfmt) || IS_GFX_SRC_FORMAT(plane->srcfmt))
#endif
                    {
                        //chop according to final HRes to be configured into readclient
                        //exclude leftcrop from final res to get right chop
                        rightCrop = res.HRes - plane->crop_actv_win.width - leftCrop;
                    }
                    else
                    {
                        //This is not required for GFX, but required for less reading in 422,420SP cases
                        rightCrop = (res.HRes - cropRes.HRes) - leftCrop;
                    }
                }
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                if(!(IS_GFX_SRC_FORMAT(pinfo->m_srcfmt) || IS_GFX_SRC_FORMAT(plane->srcfmt)))
                {
                    //let us do the odd start/width cropping at scalar - fetg
                    //will do only cropping at even pixel boundary
                    leftAdjust = leftCrop & 0x01;   //make startx position always even
                    rightAdjust = rightCrop & 0x01; //max width always even
                    if(leftAdjust || rightAdjust) {
                        enableAdditionalScalarCropping = 1;
                    }
                    leftCrop-=leftAdjust;
                    rightCrop-=rightAdjust;
                }
#endif

#ifdef CONFIG_AMP_VPP_ROTATION
                if(ROTATION_180D == plane->uchRotationAngle ||
                        ROTATION_H_FLIP == plane->uchRotationAngle)
                {
                    SWAP_W_H(leftCrop, rightCrop);
                    SWAP_W_H(leftAdjust, rightAdjust);
                }
#endif
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                if(SCAN_INVERSE_H == plane->uchInverseScanMode ||
                        SCAN_INVERSE_HV == plane->uchInverseScanMode)
                {
                    if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt ||
                            ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                             (10 == pinfo->m_bits_per_pixel)))
                    {
                        //Swap only if no rotation is set. If rotation is set, then do not swap them
                        if(ROTATION_0D == plane->uchRotationAngle)
                        {
                            SWAP_W_H(leftCrop, rightCrop);
                            SWAP_W_H(leftAdjust, rightAdjust);
                        }
                    }
                    else
                    {
                        SWAP_W_H(leftCrop, rightCrop);
                        SWAP_W_H(leftAdjust, rightAdjust);
                    }
                }
#endif
                feCropRes.leftCrop = leftCrop;
                feCropRes.rightCrop = rightCrop;
                vpp_obj->scl_ctrl[planeID].leftCrop = leftCrop;
                vpp_obj->scl_ctrl[planeID].rightCrop = rightCrop;

                topCrop = actualVStart%dataAlign_y;
                bottomCrop = res.VRes%dataAlign_y;
                if(topCrop)
                {
                    actualVStart -= topCrop;
                }
                //Vres should be multiple of pixel row granularity : res.VRes > cropRes.VRes
                if(topCrop || bottomCrop)
                {
                    res.VRes = ((res.VRes + topCrop + (dataAlign_y-1)) / dataAlign_y ) * dataAlign_y;
                    bottomCrop = (res.VRes - cropRes.VRes) - topCrop;
                }
                feCropRes.topCrop = topCrop;
                feCropRes.bottomCrop = bottomCrop;

#ifdef CONFIG_AMP_VPP_ROTATION
                if (ROTATION_90D == plane->uchRotationAngle)
                {
                    int tmp_left_crop = 0, tmp_left_adjust = 0, tmp_right_crop = 0, tmp_right_adjust = 0;
                    SWAP_W_H(res.HRes, res.VRes);
                    SWAP_W_H(cropRes.HRes, cropRes.VRes);

                    tmp_left_crop = feCropRes.leftCrop;
                    tmp_right_crop = feCropRes.rightCrop;
                    tmp_left_adjust = leftAdjust;
                    tmp_right_adjust = rightAdjust;

                    feCropRes.leftCrop = feCropRes.bottomCrop & (~0x01);
                    leftAdjust = feCropRes.bottomCrop & 0x01;
                    feCropRes.rightCrop = feCropRes.topCrop &  (~0x01);
                    rightAdjust = feCropRes.topCrop & 0x01;

                    feCropRes.topCrop = tmp_left_crop + tmp_left_adjust;
                    feCropRes.bottomCrop = tmp_right_crop + tmp_right_adjust;
                    if(leftAdjust || rightAdjust)
                    {
                        enableAdditionalScalarCropping = 1;
                    }

                }
                else if(ROTATION_270D == plane->uchRotationAngle)
                {
                    int tmp_left_crop = 0, tmp_left_adjust = 0, tmp_right_crop = 0, tmp_right_adjust = 0;

                    SWAP_W_H(res.HRes, res.VRes);
                    SWAP_W_H(cropRes.HRes, cropRes.VRes);

                    tmp_left_crop = feCropRes.leftCrop;
                    tmp_right_crop = feCropRes.rightCrop;
                    tmp_left_adjust = leftAdjust;
                    tmp_right_adjust = rightAdjust;

                    feCropRes.leftCrop = feCropRes.topCrop & (~0x01);
                    leftAdjust = feCropRes.topCrop & 0x01;
                    feCropRes.rightCrop = feCropRes.bottomCrop &  (~0x01);
                    rightAdjust = feCropRes.bottomCrop & 0x01;

                    feCropRes.bottomCrop = tmp_left_crop + tmp_left_adjust;
                    feCropRes.topCrop = tmp_right_crop + tmp_right_adjust;

                    if(leftAdjust || rightAdjust)
                    {
                        enableAdditionalScalarCropping = 1;
                    }
                }
#endif
                VDBG_DISP("Crop : mode: %d, align_x : %d, align_y : %d, \
                        crop_actv_win.width : %d, crop_actv_win.height : %d,\n",
                        plane->uchClientMode, dataAlign_x, dataAlign_y,
                        plane->crop_actv_win.width, plane->crop_actv_win.height);
                VDBG_DISP("leftCrop : %d,  rightCrop: %d, topCrop : %d, bottomCrop : %d\n",
                        leftCrop, rightCrop, topCrop, bottomCrop);
                VDBG_DISP("Modified leftCrop : %d,  rightCrop: %d, topCrop : %d, bottomCrop : %d\n",
                        feCropRes.leftCrop, feCropRes.rightCrop,
                        feCropRes.topCrop, feCropRes.bottomCrop);
                VDBG_DISP("actual (xXy HXV) : %dX%d %dX%d, crop (xXy HXV) : %dX%d %dX%d\n",
                        actualHStart, actualVStart, res.HRes, res.VRes,
                        actualHStart+feCropRes.leftCrop, actualVStart+feCropRes.topCrop,
                        cropRes.HRes, cropRes.VRes);
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                VDBG_DISP("Adjustment for cropping at scalar - leftAdjust : %d, rightAdjust : %d\n",
                        leftAdjust, rightAdjust);
#endif

                plane->stElPipeCropRes.leftCrop = feCropRes.leftCrop;
                plane->stElPipeCropRes.rightCrop = feCropRes.rightCrop;
                plane->stElPipeCropRes.topCrop = feCropRes.topCrop;
                plane->stElPipeCropRes.bottomCrop = feCropRes.bottomCrop;
                if((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable))
                {
                    //Get the tile details for the given format
                    VPP_OVL_getTileDetails(plane->uchClientMode,
                            plane->m_Aligned_Luma_Width,
                            plane->m_Aligned_Luma_Height,
                            &plane->tile);
                }
                else
                {
                    //Get the tile details for the given format
                    VPP_OVL_getTileDetails(plane->uchClientMode,
                            res.HRes,
                            res.VRes,
                            &plane->tile);
                }

                plane->tileStrideFrac_BL = plane->tile.tileStrideFrac;

                //Init aligned_active_height as it is used in updateVPSize()
                plane->aligned_active_height = res.VRes;
                if((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable))
                {
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                    int leftAdjusty = 0, rightAdjusty = 0, leftAdjustc = 0, rightAdjustc = 0;
                    if(!(IS_GFX_SRC_FORMAT(pinfo->m_srcfmt) || IS_GFX_SRC_FORMAT(plane->srcfmt)))
                    {
                        if((!(VPP_DV_GetDvMode(vpp_obj))) || (PLANE_MAIN != planeID))
                        {
                            //let us do the odd start/width cropping at scalar - fetg
                            //will do only cropping at even pixel boundary
                            //make startx position always even
                            leftAdjusty = plane->m_leftpadadjust_y & 0x01;
                            rightAdjusty = plane->m_rightpad_y & 0x01; //max width always even
                            if(leftAdjusty || rightAdjusty) {
                                enableAdditionalScalarCropping = 1;
                            }
                            plane->m_leftpadadjust_y -= leftAdjusty;
                            plane->m_rightpad_y -= rightAdjusty;

                            //let us do the odd start/width cropping at scalar -
                            //fetg will do only cropping at even pixel boundary
                            //make startx position always even
                            leftAdjustc = plane->m_leftpadadjust_c & 0x01;
                            //max width always even
                            rightAdjustc = plane->m_rightpad_c & 0x01;
                            if(leftAdjustc || rightAdjustc) {
                                enableAdditionalScalarCropping = 1;
                            }
                            plane->m_leftpadadjust_c -= leftAdjustc;
                            plane->m_rightpad_c -= rightAdjustc;

                            leftAdjust = leftAdjusty;
                            rightAdjust = rightAdjusty;
                        }
                    }
#endif
                }

                //Check, if scalar cropping is required or not
                if(plane->srcfmt == SRCFMT_YUV422 ||
                        plane->srcfmt == SRCFMT_YUV420SP ||
                        plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                        plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1
#ifdef CONFIG_AMP_VPP_ROTATION
                        || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt ||
                        plane->srcfmt == SRCFMT_YUV444_DWA
#endif
                        || plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                        plane->srcfmt == SRCFMT_YUV422SP_DWA)
                {
                    //Let us use the crop dimension for scaling,
                    //if cropping is not done at Scalar i.e for all video format
                    feRes = cropRes;
                    scalarCroppingRequired = 0;
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                    if(enableAdditionalScalarCropping) {
                        scalarCroppingRequired = 1;
                        /*Scalar input should be more(equivalent to number
                          of pixels cropped less at FETG) and the extra pixel
                          avialable at the input of scalar will be chopped at scalar chop_ctrl*/
                        feRes.HRes += (leftAdjust+rightAdjust);
                    }
#endif
                }
                else
                {
                    feRes.HRes = res.HRes;
                    feRes.VRes = res.VRes;
                    scalarCroppingRequired = 1;
                }

                // Updated client_res with res, once after applying all
                //the cropping logic and storing the result in res
                client_res.HRes = res.HRes;
                if (!pinfo->m_is_frame_seq && (planeID == PLANE_MAIN))
                {
#ifdef ENABLE_OVP
                    client_res.VRes = res.VRes/2;
#else
                    client_res.VRes = res.VRes;
#endif
                }
                else
                {
                    client_res.VRes = res.VRes;
                }

                if(plane->uiDolbyPresentMode && (PLANE_MAIN == planeID))
                {
                    plane->m_El_crop_recalculation_required = 1;
                    plane->m_Bl_Crop_Out_Width = feRes.HRes;
                    plane->m_Bl_Crop_Out_Height = feRes.VRes;
                }

                if (planeID == PLANE_MAIN)
                {
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA
#ifdef CONFIG_AMP_VPP_ROTATION
                            || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
#endif
                      )
                    {
                        PixelPerBeat = PIXEL_PER_BEAT_YUV_420SP;
#ifndef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                        /*number of pixel/rows to be read from memroy should be
                          multiple of 2 -otherwise color swap will be seen
                          odd number of pixel is not an issue with MAIN -
                          check with design team and enable if required*/
                        if(((actualHStart+feCropRes.leftCrop)  % 2 )) {
                            //enable color swap for odd sized resolution
                            FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 1 );
                        } else {
                            FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 0 );
                        }
#endif
                    } else if(plane->srcfmt==SRCFMT_ARGB32){
                        PixelPerBeat = PIXEL_PER_BEAT_ARGB_8888;
                        //no cropping required at TG, since graphic cropping is done at scalar
                        feCropRes.leftCrop =  feCropRes.rightCrop = \
                                              feCropRes.topCrop = feCropRes.bottomCrop = 0;
                    }
                    else if(plane->srcfmt==SRCFMT_RGB888){
                        PixelPerBeat = PIXEL_PER_BEAT_ARGB_8888;
                        feCropRes.leftCrop =  feCropRes.rightCrop = \
                                              feCropRes.topCrop = feCropRes.bottomCrop = 0;
                    }
                    else {
#ifndef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                        /*number of pixel/rows to be read from memroy should be
                          multiple of 2 -otherwise color swap will be seen
                          odd number of pixel is not an issue with MAIN -
                          check with design team and enable if required*/
                        if(((actualHStart+feCropRes.leftCrop)  % 2 )) {
                            //enable color swap for odd sized resolution
                            vpp_obj->scl_ctrl[planeID].uiUVswapEn=1;
                        } else {
                            vpp_obj->scl_ctrl[planeID].uiUVswapEn=0;
                        }
#endif
                        if (plane->srcfmt == SRCFMT_YUV444_DWA)
                            PixelPerBeat = PIXEL_PER_BEAT_YUV_444_DWA;
                        else if (plane->srcfmt == SRCFMT_YUV444)
                            PixelPerBeat = PIXEL_PER_BEAT_YUV444;
                        else
                            PixelPerBeat = PIXEL_PER_BEAT_YUV_422;
                    }
                    /*calculate crop WPL*/
                    CropWpl = (res.HRes +  PixelPerBeat - 1) / PixelPerBeat;


                    plane->wpl = CropWpl;
                    //Don't require scalar cropping here
                    vpp_obj->scl_ctrl[planeID].leftCrop = vpp_obj->scl_ctrl[planeID].rightCrop = 0;
                    /*If interlaced i/p, then DEINT will construct the
                      full frame and i/p to scalar/plane will be complete height
                      instead of height/2. Otherwise we will be programming number
                      of pixels expected at the input of ADPSCL-1D IFifo to
                      half the actual/expected size */
                    if (!pinfo->m_is_frame_seq && (planeID == PLANE_MAIN))
                    {
                        cropRes.VRes = plane->crop_actv_win.height;
                    }
                    FE_DLR_SetPlaneSize(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_0,
                            &res, &cropRes, plane->srcfmt, plane->bpp);
                    FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_MAIN_0,
                            &client_res, &cropRes, plane->srcfmt, plane->bpp);
#ifdef ENABLE_OVP
                    if (!pinfo->m_is_frame_seq)
                    {
                        FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_MAIN_2,
                                &client_res, &cropRes, plane->srcfmt, plane->bpp);
                        FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_MAIN_3,
                                &client_res, &cropRes, plane->srcfmt, plane->bpp);
                    }
#endif
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA
#ifdef CONFIG_AMP_VPP_ROTATION
                            || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
#endif
                      )
                    {
                        FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_MAIN_1,
                                &client_res, &cropRes, plane->srcfmt, plane->bpp);
                    }

                    if (data_loader_update_required)
                    {
                        FE_DLR_SetDummyTG(vpp_obj, VPP_FE_DLR_MAIN,
                                &res, &feCropRes, plane->frame_type);

                        FE_DLR_SetDummyTG(vpp_obj, VPP_FE_DLR_MAIN, &res, &feCropRes, plane->frame_type);
                        if(cpcbID == CPCB_1) {
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                            VPP_DV_PostCompCropHandler(vpp_obj, plane, &leftAdjust, &rightAdjust, &feRes);
#else
                            VPP_DV_PostCompCropHandler(vpp_obj, plane, NULL, NULL, &feRes);
#endif
                        }
                        FE_DEINT_SetVpTG(vpp_obj, (PVPP_VP_CLIENT_RES)&feRes, plane->frame_type);
                        FE_DEINT_SetVpDMX(vpp_obj, (PVPP_VP_CLIENT_RES)&feRes, plane->frame_type);
                    }

#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                    if(enableAdditionalScalarCropping)
                    {
                        vpp_obj->scl_ctrl[VPP_FRC_SCL_MAIN].leftCrop = leftAdjust;
                        vpp_obj->scl_ctrl[VPP_FRC_SCL_MAIN].rightCrop = rightAdjust;
                        vpp_obj->scl_ctrl[VPP_FRC_SCL_VIP_Y].leftCrop = leftAdjust;
                        vpp_obj->scl_ctrl[VPP_FRC_SCL_VIP_Y].rightCrop = rightAdjust;
                        vpp_obj->scl_ctrl[VPP_FRC_SCL_VIP_UV].leftCrop = leftAdjust;
                        vpp_obj->scl_ctrl[VPP_FRC_SCL_VIP_UV].rightCrop = rightAdjust;
                    }
#endif
                    if(data_loader_update_required)
                    {
                        updateVPSize(vpp_obj, plane, &feRes);
                        plane->vp_updated = 1;
                    }
                }
                else if (planeID == PLANE_PIP)
                {
#ifdef RA_VPP_MP_PIP_dvCompYUV2RGB
                    stDVinpRes.HRes = plane->crop_actv_win.width;
                    stDVinpRes.VRes = plane->crop_actv_win.height;
#endif
                    /*calculate crop WPL*/
                    if (pinfo->m_srcfmt == SRCFMT_YUV422)
                    {
                        PixelPerBeat = PIXEL_PER_BEAT_YUV_422;
#ifndef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                        /*For odd number of pixel, UV swap would be seen
                          for lesser/lower scaling ratio - check with
                          design team and enable the logic*/
                        if((((actualHStart+feCropRes.leftCrop)%2) && feCropRes.rightCrop) ||
                                ((feCropRes.rightCrop) && (feCropRes.leftCrop==0)) )
                        {
                            //enable color swap for odd sized resolution
                            FE_DLR_GFX0_UPS8_Config_UVSwap(vpp_obj, 1 );
                        }
                        else
                        {
                            FE_DLR_GFX0_UPS8_Config_UVSwap(vpp_obj, 0 );
                        }
#endif
                        //Don't require scalar cropping here
                        vpp_obj->scl_ctrl[planeID].leftCrop = vpp_obj->scl_ctrl[planeID].rightCrop = 0;
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                        if(enableAdditionalScalarCropping)
                        {
                            vpp_obj->scl_ctrl[planeID].leftCrop = leftAdjust;
                            vpp_obj->scl_ctrl[planeID].rightCrop = rightAdjust;
                        }
#endif
                    }
                    else if (plane->srcfmt == SRCFMT_YUV444_DWA)
                        PixelPerBeat = PIXEL_PER_BEAT_YUV_444_DWA;
                    else if (plane->srcfmt == SRCFMT_YUV444)
                        PixelPerBeat = PIXEL_PER_BEAT_YUV444;
                    else if (plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt  ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA) {
                        if(plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                                plane->srcfmt == SRCFMT_YUV422SP_DWA)
                            PixelPerBeat = PIXEL_PER_BEAT_YUV_420SP_DWA;
                        else
                            PixelPerBeat = PIXEL_PER_BEAT_YUV_420SP;
#ifndef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                        /*number of pixel/rows to be read from memory should be
                          multiple of 2 -otherwise color swap will be seen
                          For odd number of pixel, UV swap would be seen for
                          lesser/lower scaling ratio -check with design team and enable the logic*/
                        if(((actualHStart+feCropRes.leftCrop)%2)) {
                            //enable color swap for odd sized resolution
                            FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_PIP, 1 );
                        } else {
                            FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_PIP, 0 );
                        }
#endif
                        //Don't require scalar cropping here
                        vpp_obj->scl_ctrl[planeID].leftCrop = vpp_obj->scl_ctrl[planeID].rightCrop = 0;
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                        if(enableAdditionalScalarCropping) {
                            vpp_obj->scl_ctrl[planeID].leftCrop = leftAdjust;
                            vpp_obj->scl_ctrl[planeID].rightCrop = rightAdjust;
                        }
#endif
                    }
                    else
                    {
                        PixelPerBeat = plane->srcfmt==SRCFMT_RGB565?
                            PIXEL_PER_BEAT_RGB_565:PIXEL_PER_BEAT_ARGB_8888;
                        //no cropping required at TG, since graphic cropping is done at scalar
                        feCropRes.leftCrop =  feCropRes.rightCrop = \
                                              feCropRes.topCrop = feCropRes.bottomCrop = 0;
                    }
                    CropWpl = (res.HRes +  PixelPerBeat - 1) / PixelPerBeat;
                    plane->wpl = CropWpl;

                    FE_DLR_SetPlaneSize(vpp_obj, VPP_FE_DLR_PIP,
                            &res, &cropRes, plane->srcfmt, plane->bpp);
                    FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_PIP_0,
                            &client_res, &cropRes, plane->srcfmt, plane->bpp);
#ifdef CONFIG_AMP_VPP_ROTATION
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt  ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA)
#else
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA)
#endif
                    {
                        FE_DLR_SetPlaneSize(vpp_obj, VPP_FE_DLR_PIP,
                                &res, &cropRes, plane->srcfmt, plane->bpp);
                        FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_PIP_1,
                                &client_res, &cropRes, plane->srcfmt, plane->bpp);
                    }
                    if (data_loader_update_required)
                    {
                        FE_DLR_SetDummyTG(vpp_obj, VPP_FE_DLR_PIP, &res, &feCropRes, plane->frame_type);
                        SclOutRes.HRes = plane->crop_scl_actv_win.width;
                        SclOutRes.VRes = plane->crop_scl_actv_win.height;
                        if(cpcbID == CPCB_1) {
#ifdef RA_VPP_MP_PIP_dvCompYUV2RGB
                            VPP_DV_FE_DLR_SetDummyTG(vpp_obj, VPP_FE_DLR_PIP, &stDVinpRes, NULL);
#else
                            VPP_DV_FE_DLR_SetDummyTG(vpp_obj, VPP_FE_DLR_PIP, &SclOutRes, NULL);
#endif
                        }
                    }
                }
                else if (planeID == PLANE_GFX1){
                    /*calculate crop WPL*/
                    if(plane->srcfmt==SRCFMT_ARGB4444 ||
                            plane->srcfmt== SRCFMT_RGB565 ||
                            plane->srcfmt== SRCFMT_ARGB1555 ||
                            plane->srcfmt ==  SRCFMT_ARGB8332)
                        PixelPerBeat = PIXEL_PER_BEAT_ARGB_4444;
                    else
                        PixelPerBeat = PIXEL_PER_BEAT_ARGB_8888;

                    CropWpl = (res.HRes +  PixelPerBeat - 1) / PixelPerBeat;
                    plane->wpl = CropWpl;
                    FE_DLR_SetPlaneSize(vpp_obj, VPP_FE_DLR_GFX1,
                            &res, (PVPP_FE_DLR_INPUT_RES)&feCropRes, plane->srcfmt, plane->bpp);
                    FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_GFX1_0,
                            &client_res, (PVPP_FE_DLR_INPUT_RES)&feCropRes, plane->srcfmt, plane->bpp);

                    if (data_loader_update_required) {
                        FE_DLR_SetDummyTG(vpp_obj, VPP_FE_DLR_GFX1, &res, &feCropRes, plane->frame_type);
                        SclOutRes.HRes = plane->crop_scl_actv_win.width;
                        SclOutRes.VRes = plane->crop_scl_actv_win.height;
                        if(cpcbID == CPCB_1) {
                            VPP_DV_FE_DLR_SetDummyTG(vpp_obj, VPP_FE_DLR_GFX1, &SclOutRes, NULL);
                        }
                    }
                }

                else if (planeID == PLANE_GFX2){
                    /*calculate crop WPL*/
                    if(plane->srcfmt==SRCFMT_ARGB4444 ||
                            plane->srcfmt== SRCFMT_RGB565 ||
                            plane->srcfmt== SRCFMT_ARGB1555 ||
                            plane->srcfmt ==  SRCFMT_ARGB8332)
                        PixelPerBeat = PIXEL_PER_BEAT_ARGB_4444;
                    else
                        PixelPerBeat = PIXEL_PER_BEAT_ARGB_8888;

                    CropWpl = (res.HRes +  PixelPerBeat - 1) / PixelPerBeat;
                    plane->wpl = CropWpl;
                    FE_DLR_SetPlaneSize(vpp_obj, VPP_FE_DLR_GFX2, &res,
                            (PVPP_FE_DLR_INPUT_RES)&feCropRes, plane->srcfmt, plane->bpp);
                    FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_GFX2_0,
                            &client_res, (PVPP_FE_DLR_INPUT_RES)&feCropRes, plane->srcfmt, plane->bpp);

                    if (data_loader_update_required) {
                        FE_DLR_SetDummyTG(vpp_obj, VPP_FE_DLR_GFX2, &res, &feCropRes, plane->frame_type);
                    }
                }

                VDBG_DISP("crop_actv_offset : 0x%x, CropWpl : 0x%x, \
                        PIXEL_PER_BEAT_YUV_422/420SP : 0x%x, plane->wpl: 0x%x\n",
                        crop_actv_offset, CropWpl, PixelPerBeat, plane->wpl);
                VDBG_DISP("width => pinfo->m_buf_stride : %x , plane->wpl*plane->bpp = (%x,%x) => %x\n",
                        pinfo->m_buf_stride, plane->wpl, plane->bpp, plane->wpl*plane->bpp);
                VDBG_DISP("crop_actv_offset : %x, crop_actv_offset_UV : %x\n",
                        crop_actv_offset, crop_actv_offset_UV);

                plane->uiFeDlrInHres = res.HRes;
                plane->uiFeDlrInVres = res.VRes;
            } // <- update data loader

            if(planeID != PLANE_OVP_EL)
            {
                if(uiIsrLocalResID[cpcbID] != vpp_obj->uiResID[cpcbID])
                {
                    scaler_update_required = 1;
                }

                if (planeID == PLANE_MAIN)
                {
                    eVppFrcSclNum = VPP_FRC_SCL_MAIN;
                }
                else if (planeID == PLANE_PIP)
                {
                    eVppFrcSclNum = VPP_FRC_SCL_PIP;
                }
                else if (planeID == PLANE_GFX2) {
                    eVppFrcSclNum = VPP_FRC_SCL_GFX2;
                }
                else { // GFX-1
                    eVppFrcSclNum = VPP_FRC_SCL_OSD;
                }

                iSclCoeffUpdate = vpp_obj->scl_coeff_update[eVppFrcSclNum];

                if(planeID == PLANE_MAIN)
                {
                    /* Special case for VIP as it is used for offline scaling in Main plane */
                    iSclCoeffUpdate |= (vpp_obj->scl_coeff_update[VPP_FRC_SCL_VIP_Y] |
                            vpp_obj->scl_coeff_update[VPP_FRC_SCL_VIP_UV]);
                }

                /* update scaler setting */
                if (data_loader_update_required ||
                        cpcb_source_window_update_required ||
                        scaler_update_required ||
                        scaler_crop_update_required ||
                        iSclCoeffUpdate)
                {
                    if ( (planeID == PLANE_MAIN)
                            || (planeID == PLANE_PIP)
                            || (planeID == PLANE_GFX1)
                            || (planeID == PLANE_GFX2)) {
                        /* resolution before scaling */
                        stVppSclRes.IHRes = plane->crop_actv_win.width;
                        if(scalarCroppingRequired)
                        {
                            //If it is graphic plane then cropping is done at scalar
                            stVppSclRes.IHRes = feRes.HRes;
                            VDBG_DISP("Scalar cropping enabled : \
                                    vpp_obj->scl_ctrl[eVppFrcSclNum].leftCrop = %d, \
                                    vpp_obj->scl_ctrl[eVppFrcSclNum].rightCrop = %d\n",
                                    vpp_obj->scl_ctrl[eVppFrcSclNum].leftCrop,
                                    vpp_obj->scl_ctrl[eVppFrcSclNum].rightCrop);
                            VDBG_DISP("Scalar IHRes:%d, OHRes:%d\n",
                                    stVppSclRes.IHRes, plane->crop_scl_actv_win.width);
                        }
                        stVppSclRes.IVRes = plane->crop_actv_win.height;
#ifdef CONFIG_AMP_VPP_ROTATION
                        if(ROTATION_90D == plane->uchRotationAngle ||
                                ROTATION_270D == plane->uchRotationAngle)
                        {
                            if(scalarCroppingRequired)
                            {
                                stVppSclRes.IHRes = feRes.HRes;
                            }
                            else
                            {
                                stVppSclRes.IHRes = cropRes.HRes;
                            }
                            stVppSclRes.IVRes = cropRes.VRes;
                        }
#endif
                        /* resolution after scaling */
                        stVppSclRes.OHRes = plane->crop_scl_actv_win.width;
                        stVppSclRes.OVRes = plane->crop_scl_actv_win.height;
                        stVppSclRes.x     = plane->crop_scl_actv_win.x;
                        stVppSclRes.y     = plane->crop_scl_actv_win.y;

                        if(VPP_SCL_MODE_INLINE != plane->scl_mode)
                        {
                            /* Inline to Offline mode, current MAIN SCL config
                               is required to program VIP SCL.
                               So update scl_res after offline config */
                            vpp_obj->scl_res[eVppFrcSclNum].IHRes = stVppSclRes.IHRes;
                            vpp_obj->scl_res[eVppFrcSclNum].IVRes = stVppSclRes.IVRes;
                            vpp_obj->scl_res[eVppFrcSclNum].OHRes = stVppSclRes.OHRes;
                            vpp_obj->scl_res[eVppFrcSclNum].OVRes = stVppSclRes.OVRes;
                            VPP_OfflineScl_Config(vpp_obj,  &stVppSclRes, chanID);
                        }
                        else
                        {
                            if(0 == plane->sclLineDropEn)
                            {
                                /* Sequence for inline and inline-To-Offline cases */
                                VPP_OfflineScl_Config(vpp_obj,  &stVppSclRes, chanID);
                                vpp_obj->scl_res[eVppFrcSclNum].IHRes = stVppSclRes.IHRes;
                                vpp_obj->scl_res[eVppFrcSclNum].IVRes = stVppSclRes.IVRes;
                                vpp_obj->scl_res[eVppFrcSclNum].OHRes = stVppSclRes.OHRes;
                                vpp_obj->scl_res[eVppFrcSclNum].OVRes = stVppSclRes.OVRes;
                            }
                            else
                            {
                                /* Sequence for inline and inline-To-Offline cases with line dropping*/
                                vpp_obj->scl_res[eVppFrcSclNum].IHRes = stVppSclRes.IHRes;
                                vpp_obj->scl_res[eVppFrcSclNum].IVRes = stVppSclRes.IVRes;
                                vpp_obj->scl_res[eVppFrcSclNum].OHRes = stVppSclRes.OHRes;
                                vpp_obj->scl_res[eVppFrcSclNum].OVRes = stVppSclRes.OVRes;
                                VPP_OfflineScl_Config(vpp_obj,  &stVppSclRes, chanID);
                            }
                        }
                        is_OfflineConfigured = 1;
#ifdef VPP_OFFLINESCL_USING_VIP
                        if(VPP_SCL_MODE_INLINE == plane->scl_mode ||
                                (1 == plane->sclLineDropEn))
#endif
                        {
                            if ((planeID == PLANE_MAIN) || (planeID == PLANE_PIP))
                            {
                                if (plane->scaler_crop_offset &&
                                        (vpp_obj->scl_res[eVppFrcSclNum].IHRes ==
                                         vpp_obj->scl_res[eVppFrcSclNum].OHRes)) {
                                    vpp_obj->scl_ctrl[eVppFrcSclNum].HScalePos = VPP_SCL_HSCALE_BEFORE;
                                }
                                else
                                {
                                    vpp_obj->scl_ctrl[eVppFrcSclNum].HScalePos = VPP_SCL_HSCALE_AUTO;
                                }
                                /* YUV input for none-OSD plane, ignore CSC in scaler */
                                vpp_obj->scl_ctrl[eVppFrcSclNum].InputClr = 0;
                                /* OSD input (without alpha) */
                                if((planeID == PLANE_PIP) && (vpp_obj->dv[cpcbID].LumaKey))
                                    vpp_obj->scl_ctrl[eVppFrcSclNum].OsdInput = 1;
                                else
                                    /* non-OSD input (without alpha) */
                                    vpp_obj->scl_ctrl[eVppFrcSclNum].OsdInput = 0;
                            }
                            else
                            {
                                vpp_obj->scl_ctrl[eVppFrcSclNum].HScalePos = VPP_SCL_HSCALE_AUTO;
                                /* RGB input for PG plane, do CSC in scaler */
                                vpp_obj->scl_ctrl[eVppFrcSclNum].InputClr = 1;
                                /* OSD input with alpha */
                                vpp_obj->scl_ctrl[eVppFrcSclNum].OsdInput = 1;
                            }
                            /* linear horizontal scaling mode */
                            vpp_obj->scl_ctrl[eVppFrcSclNum].NLEn = chan->scl_nl_enable;
                            /* center value */
                            vpp_obj->scl_ctrl[eVppFrcSclNum].CenterFrac = chan->scl_nl_center;
                            /* center ratio */
                            vpp_obj->scl_ctrl[eVppFrcSclNum].CenterRatio = chan->scl_nl_cratio;
                            vpp_obj->scl_ctrl[eVppFrcSclNum].I565 = 0;
                            vpp_obj->scl_ctrl[eVppFrcSclNum].HTapNum = 12;
                            vpp_obj->scl_ctrl[eVppFrcSclNum].VTapNum = 8;
                            /* dynamically update coefficient table */
                            vpp_obj->scl_ctrl[eVppFrcSclNum].DynamicLoad = 1;
                            vpp_obj->scl_ctrl[eVppFrcSclNum].ForceSel =
                                vpp_obj->scl_force_sel[eVppFrcSclNum];
                            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclLeft    = 0;
                            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclRight   = 0;
                            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclTop     = 0;
                            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclBottom  = 0;
                            /* update scaling mode according to down-scaling ratio */
                            if ((planeID == PLANE_MAIN) || (planeID == PLANE_PIP))
                            {
                                cpcb_source_window_update_required = 1;
                                FRC_SCL_SetDeLrstDelay(vpp_obj, eVppFrcSclNum, 80);
                            }
                            FRC_SCL_SetSclCtrlParams(vpp_obj, eVppFrcSclNum,
                                    &vpp_obj->scl_res[eVppFrcSclNum],
                                    &vpp_obj->scl_ctrl[eVppFrcSclNum]);
                            FRC_SCL_SetFrcParams(vpp_obj, eVppFrcSclNum,
                                    &vpp_obj->scl_res[eVppFrcSclNum]);
                            if(planeID == PLANE_MAIN)
                            {
                                FRC_SCL_SetDeLrstDelay(vpp_obj, VPP_FRC_SCL_DET, 80);

                                memcpy(&vpp_obj->scl_res[VPP_FRC_SCL_DET],
                                        &vpp_obj->scl_res[eVppFrcSclNum],sizeof(VPP_SCL_RES));
                                memcpy(&vpp_obj->scl_ctrl[VPP_FRC_SCL_DET],
                                        &vpp_obj->scl_ctrl[eVppFrcSclNum],sizeof(VPP_SCL_CTRL));

                                /*program for detail scaler*/
                                FRC_SCL_SetSclCtrlParams(vpp_obj, VPP_FRC_SCL_DET,
                                        &vpp_obj->scl_res[VPP_FRC_SCL_DET],
                                        &vpp_obj->scl_ctrl[VPP_FRC_SCL_DET]);
                                FRC_SCL_SetFrcParams(vpp_obj, VPP_FRC_SCL_DET,
                                        &vpp_obj->scl_res[VPP_FRC_SCL_DET]);
                            }
                        }/* End of (VPP_SCL_MODE_INLINE == plane->scl_mode) */
                    } // <- for planes with scaler
                    vpp_obj->scl_coeff_update[eVppFrcSclNum] = 0;
                    vpp_obj->scl_ctrl[eVppFrcSclNum].hv_swap_updated = 0;
                    if(planeID == PLANE_MAIN)
                    {
                        /* Special case for VIP as it is used for offline scaling in Main plane */
                        vpp_obj->scl_coeff_update[VPP_FRC_SCL_VIP_Y] = 0;
                        vpp_obj->scl_coeff_update[VPP_FRC_SCL_VIP_UV] = 0;
                    }
                    /* Programe Dolby input size */
                    switch(planeID)
                    {
                        case PLANE_MAIN:
                            if((VPP_SCL_MODE_INLINE == plane->scl_mode) ||
                                    (VPP_SCL_MODE_INLINE_OFFLINE == plane->scl_mode))
                            {
                                sDBVIPSize.HRes = vpp_obj->scl_res[VPP_FRC_SCL_MAIN].IHRes;
                                sDBVIPSize.VRes = vpp_obj->scl_res[VPP_FRC_SCL_MAIN].IVRes;
                            }
                            else
                            {
                                sDBVIPSize.HRes = vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IHRes;
                                sDBVIPSize.VRes = vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IVRes;
                            }
                            break;
                        case PLANE_PIP:
#ifdef RA_VPP_MP_PIP_dvCompYUV2RGB
                            /*Scalar is after HDR IP's in A0*/
                            sDBVIPSize.HRes = vpp_obj->scl_res[eVppFrcSclNum].IHRes;
                            sDBVIPSize.VRes = vpp_obj->scl_res[eVppFrcSclNum].IVRes;
                            break;
#endif
                        case PLANE_GFX1:
                            sDBVIPSize.HRes = vpp_obj->scl_res[eVppFrcSclNum].OHRes;
                            sDBVIPSize.VRes = vpp_obj->scl_res[eVppFrcSclNum].OVRes;
                            break;
                    }
                    FE_DLR_SetHDRInputSize(vpp_obj, planeID, &sDBVIPSize);
                    VPP_DEBUG_UTIL_printSclChange(vpp_obj, &vpp_obj->scl_res[eVppFrcSclNum],
                            &vpp_obj->scl_ctrl[eVppFrcSclNum],
                            eVppFrcSclNum, plane->scl_mode,
                            plane->sclLineDropEn, __func__, __LINE__);
                } // <- update scaler settings
            }
            if(!is_OfflineConfigured)
            {
                if((PLANE_MAIN == planeID) && (VPP_SCL_MODE_INLINE != plane->scl_mode))
                    VPP_OfflineScl_ClientStart(vpp_obj,  &vpp_obj->sclResCfg, chanID);
            }


            if(PLANE_MAIN == planeID)
            {
                if(((VPP_SCL_MODE_OFFLINE_OFFLINE == plane->scl_mode) ||
                            (VPP_SCL_MODE_INLINE_OFFLINE == plane->scl_mode)) &&
                        (0 == plane->sclLineDropEn))
                {
                    plane->cpcb_source_window_update_required_offline =
                        cpcb_source_window_update_required;
                    plane->cpcb_bg_window_update_required_offline = cpcb_bg_window_update_required;
                    cpcb_bg_window_update_required = 0;
                    cpcb_source_window_update_required = 0;
                }
                else
                {
                    if(plane->cpcb_source_window_update_required_offline)
                    {
                        cpcb_source_window_update_required =
                            plane->cpcb_source_window_update_required_offline;
                        plane->cpcb_source_window_update_required_offline = 0;
                    }
                    if(plane->cpcb_bg_window_update_required_offline)
                    {
                        cpcb_bg_window_update_required = plane->cpcb_bg_window_update_required_offline;
                        plane->cpcb_bg_window_update_required_offline = 0;
                    }
                }
            }

            if((1 == plane->sclModeChanged) &&
                    (VPP_SCL_MODE_OFFLINE_OFFLINE == plane->scl_mode) && (PLANE_MAIN == planeID))
            {
                /* Continues interrupt offline-offline swithing case,
                   we needs to program IPs after main scalar(VMx/CPCB etc)
                   with the values inside offline buffer. So force cpcb
                   update with values in offline buffer */
                cpcb_source_window_update_required = 1;
                pOfflineBuffDescr = &(plane->offline_buff_descrs[plane->rb_buff_index]);
                GaloisMemcpy(&tmp_crop_scl_actv_win, &pOfflineBuffDescr->disp_win, sizeof(VPP_WIN));
                GaloisMemcpy(&tmp_crop_win_final, &pOfflineBuffDescr->crop_win_final, sizeof(VPP_WIN));
                GaloisMemcpy(&tmp_crop_disp_win_final, &pOfflineBuffDescr->crop_disp_win_final,
                        sizeof(VPP_WIN));
            }
            else
            {
                GaloisMemcpy(&tmp_crop_scl_actv_win, &plane->crop_scl_actv_win, sizeof(VPP_WIN));
                GaloisMemcpy(&tmp_crop_win_final, &plane->crop_win_final, sizeof(VPP_WIN));
                GaloisMemcpy(&tmp_crop_disp_win_final, &chan->crop_disp_win_final, sizeof(VPP_WIN));
            }

            /* update CPCB source plane size */
            if (cpcb_source_window_update_required)
            {
                VDBG_IF_DISP("cpcb_source_window_update_required : %d\n",
                        cpcb_source_window_update_required);
                VDBG_IF_DISP_WIN((&plane->crop_win_final));
                VDBG_IF_DISP_WIN((&plane->crop_scl_actv_win));
                VDBG_IF_DISP_WIN((&chan->crop_disp_win));

                /* for Main/PIP/Aux offline mode, plane source window
                   info is in offline frame descriptor */
                if(cpcbID == CPCB_1)
                {
                    if (plane->crop_win_valid)
                        VPP_CPCB_SetPlaneCropWindow(vpp_obj, cpcbID, chan->dvlayerID,
                                plane->crop_win_final.x, plane->crop_win_final.y,
                                plane->crop_win_final.width, plane->crop_win_final.height);
                    else
                        VPP_CPCB_SetPlaneCropWindow(vpp_obj, cpcbID, chan->dvlayerID,
                                plane->crop_scl_actv_win.x, plane->crop_scl_actv_win.y,
                                plane->crop_scl_actv_win.width, plane->crop_scl_actv_win.height);

                    VPP_CPCB_SetPlaneSourceWindow(vpp_obj, cpcbID, chan->dvlayerID,
                            plane->crop_scl_actv_win.x, plane->crop_scl_actv_win.y,
                            plane->crop_scl_actv_win.width, plane->crop_scl_actv_win.height);
                    VPP_CPCB_SetPlaneBGWindow(vpp_obj, cpcbID, chan->dvlayerID,
                            chan->crop_disp_win_final.x, chan->crop_disp_win_final.y,
                            chan->crop_disp_win_final.width, chan->crop_disp_win_final.height);
                    VPP_CPCB_EnablePlane(vpp_obj, cpcbID, chan->dvlayerID, 1);
                }
                if (VPP_IsNgptvServiceEnabled(vpp_obj, planeID)) {
                    VPP_NGPTV_Monitor(vpp_obj, planeID);
                }
            }/*cpcb_source_window_update_required*/

#ifdef ENABLE_EECMU
            // CMU TG programming
            if((planeID == PLANE_MAIN) && (cpcb_source_window_update_required ||
                        (vpp_obj->cmu.cmu_enabled == 1)))
            {
                if(vpp_obj->cmu.cmu_enabled)
                {
                    VPP_CMU_TG_Config(vpp_obj,plane->crop_scl_actv_win.width,
                            plane->crop_scl_actv_win.height);
                    vpp_obj->cmu.cmu_enabled = 2;
                }
            }
#endif
#ifdef ENABLE_DETEE
            if((planeID == PLANE_MAIN) && (cpcb_source_window_update_required ||
                        (vpp_obj->detee.detee_enabled == 1)))
            {
                if(vpp_obj->detee.detee_enabled)
                {
                    VPP_DETEE_ConfigTG(vpp_obj,vpp_obj->scl_res[VPP_FRC_SCL_DET].IHRes,
                            vpp_obj->scl_res[VPP_FRC_SCL_DET].IVRes);
                    if(vpp_obj->cmu.cmu_enabled == 0)
                    {
                        VPP_CMU_TG_Config(vpp_obj,plane->crop_scl_actv_win.width,
                                plane->crop_scl_actv_win.height);
                    }
                    vpp_obj->detee.detee_enabled = 2;
                }
            }
#endif
            if(cpcbID == CPCB_1)
            {
                /* update CPCB bg plane size */
                if (cpcb_bg_window_update_required && !cpcb_source_window_update_required) {
                    VPP_CPCB_SetPlaneBGWindow(vpp_obj, cpcbID, chan->dvlayerID,
                            chan->crop_disp_win_final.x, chan->crop_disp_win_final.y,
                            chan->crop_disp_win_final.width, chan->crop_disp_win_final.height);
                }
            } // <- update VPP setting on frame (2-field)  boundary
        } // <- update VPP setting on frame (2-field)  boundary

        if(planeID == PLANE_MAIN)
        {
            VPP_SCL_SetMainSclScanParams(vpp_obj, VPP_FRC_SCL_MAIN,
                    &vpp_obj->scl_res[planeID], &vpp_obj->scl_ctrl[planeID]);
            VPP_SCL_SetMainSclScanParams(vpp_obj, VPP_FRC_SCL_DET,
                    &vpp_obj->scl_res[planeID], &vpp_obj->scl_ctrl[planeID]);
        }
        else if(planeID == PLANE_PIP)
            VPP_SCL_SetGfxSclScanParams(vpp_obj, VPP_FRC_SCL_PIP,
                    &vpp_obj->scl_res[planeID], &vpp_obj->scl_ctrl[planeID]);
        else if(planeID == PLANE_GFX1)
            VPP_SCL_SetGfxSclScanParams(vpp_obj, VPP_FRC_SCL_OSD,
                    &vpp_obj->scl_res[planeID], &vpp_obj->scl_ctrl[planeID]);
        else if(planeID == PLANE_GFX2)
            VPP_SCL_SetGfxSclScanParams(vpp_obj, VPP_FRC_SCL_GFX2,
                    &vpp_obj->scl_res[planeID], &vpp_obj->scl_ctrl[planeID]);

        /* here, plane->crop_actv_win & plane->crop_actv_offset is cropped display content */
        if ((plane->crop_actv_win.width <= 0) || (plane->crop_actv_win.height <= 0)) {

#if defined(PE_3D_BD)
            if (pDVResinfo->flag_3d) {
                /* recover left view start address, if was updated to right view */
                if ((plane->ss_type == 1) && (pinfo_pbuf_start != 0))
                    pinfo->m_pbuf_start = (ARCH_PTR_TYPE)pinfo_pbuf_start;
            }
#endif
            /* no display content for display */
            return (MV_VPP_OK);
        }

#ifdef VPP_DEBUG
        dhub_status = dhub2d_channel_busy((HDL_dhub2d  *)plane->dmaRdhubID);
        if (dhub_status & (1<<plane->dmaRID[0]))
        {
            if ((planeID == PLANE_MAIN))
            {
                main_dhub_fail ++;
            }
            else if (planeID == PLANE_GFX1)
            {
                gfx0_dhub_fail ++;
            }
        }
#endif
        if (plane->status != STATUS_DISP_PATGEN)
        {
#ifdef CONFIG_AMP_DOLBY_DUMP
            if((PLANE_MAIN == planeID) && (cpcbID == CPCB_1))
            {
                VPP_Ovl_Capture_Handler(vpp_obj, plane);
            }
#endif
            if(cpcbID == CPCB_1) {
                VPP_DV_BE_Handler(vpp_obj, planeID, pinfo, pHdmiTx->videoParams.VideoFmt,\
                    data_loader_update_required);
            }
            m_uchDvCompMode = VPP_DV_GetDvCompMode(vpp_obj);

            if((PLANE_MAIN == planeID) && (cpcbID == CPCB_1))
            {
                VPP_BE_HDR_Process(vpp_obj, pinfo);
                vpp_obj->videofmtUpdated = 0;
            }

            /* start data loader DMA to load display content */
            if (pinfo->m_is_frame_seq || (planeID != PLANE_MAIN))
            {
                /* progressive frame */
#if defined(REPEAT_FRAME_ENHANCE)
                if (planeID == PLANE_MAIN)
                {
                    vpp_obj->main_last_frame = inline_frame;
                    vpp_obj->main_frame_repeat = 0;
                }
#endif
                {
#ifdef CONFIG_AMP_VPP_ROTATION
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA)
#else
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA)
#endif
                    {
                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
#if !defined(VPP_IN_TRUST_ZONE)
                        plane->vbi_cfgq_shm_handle =
                            vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ->handle;
#endif
#ifdef CONFIG_AMP_VPP_ROTATION
                        VPP_DHUB_PARAM_ROTATION stDhubParam;
                        memset(&stDhubParam, 0, sizeof(VPP_DHUB_PARAM_ROTATION));
                        if((plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 &&
                                    8 == pinfo->m_bits_per_pixel) ||
                                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#else
                        if(plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
                        {
                            stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                            stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                            stCfg2NDDMAParam.addr   = (UNSG32)(plane->crop_disp_offset +
                                    plane->tile.tileStartFrac);
                            stCfg2NDDMAParam.width  = (plane->wpl * plane->bpp +
                                    plane->tile.tileStrideFrac)*
                                tiledModeWidHgtFactor*uiCromaAllignedLinedropFactor;
                            stCfg2NDDMAParam.stride = (pinfo->m_buf_stride *
                                    plane->lineDroppingStrides)*
                                tiledModeStrideFactor*uiCromaAllignedLinedropFactor;
                            stCfg2NDDMAParam.height = plane->aligned_active_height/tiledModeWidHgtFactor;
                            stCfg2NDDMAParam.cfgQ   = cfgQ;
                            stCfg2NDDMAParam.mtrREn = 0;
                            stCfg2NDDMAParam.plane  = plane;
                            *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
#if defined(VPP_ON_VELOCE)

                            VPPAPI_DBG_INFO("burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",(plane->wpl*plane->bpp+plane->tile.tileStrideFrac)*tiledModeWidHgtFactor*uiCromaAllignedLinedropFactor, (plane->wpl*plane->bpp+plane->tile.tileStrideFrac)*tiledModeWidHgtFactor*uiCromaAllignedLinedropFactor, 1, (pinfo->m_buf_stride*plane->lineDroppingStrides)*tiledModeStrideFactor*uiCromaAllignedLinedropFactor, plane->aligned_active_height/tiledModeWidHgtFactor);
#endif
                        }
#ifdef CONFIG_AMP_VPP_ROTATION
                        else if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt ||
                                ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                                 (10 == pinfo->m_bits_per_pixel)))
                        {
                            UINT32 uiRotationAngle = plane->uchRotationAngle;

                            stDhubParam.m_uiSrcFtm = pinfo->m_srcfmt;
                            stDhubParam.m_uiSrcOrder = pinfo->m_order;
                            stDhubParam.m_uiBitDepth = pinfo->m_bits_per_pixel;
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                            GetRotationAngle(vpp_obj, planeID, &uiRotationAngle);
#endif
                            stDhubParam.m_uiRotationAngle = uiRotationAngle;

                            if(ROTATION_90D == plane->uchRotationAngle ||
                                    ROTATION_270D == plane->uchRotationAngle)
                            {
                                stDhubParam.m_uiPicW = plane->uiFeDlrInVres;
                                stDhubParam.m_uiPicH = plane->uiFeDlrInHres;
                            }
                            else
                            {
                                stDhubParam.m_uiPicW = plane->uiFeDlrInHres;
                                stDhubParam.m_uiPicH = plane->uiFeDlrInVres;
                            }
                            stDhubParam.m_uiBaseAddr_Y =
                                (UNSG32)(plane->crop_disp_offset+plane->tile.tileStartFrac);
                            stDhubParam.m_uiBaseAddr_UV =
                                (UNSG32)(plane->crop_disp_offset_UV+plane->tile.tileStartFrac);
                            stDhubParam.m_uiStride = pinfo->m_buf_stride * plane->lineDroppingStrides;

                            GetDhubParam(vpp_obj, planeID, &stDhubParam);

                            if(ROTATION_0D == plane->uchRotationAngle)
                            {
                                if((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable))
                                {
                                    PixelPerBeat = PIXEL_PER_BEAT_YUV_420SP;
                                    plane->wpl =
                                        (plane->m_Aligned_Luma_Width + PixelPerBeat - 1)/PixelPerBeat;
                                    plane->aligned_active_height = plane->m_Aligned_Luma_Height;
                                }

                                stCfg2NDDMAParam.dhubId   = (INT)plane->dmaRdhubID;
                                stCfg2NDDMAParam.dmaId    = plane->dmaRID[0];
                                stCfg2NDDMAParam.addr     = (UNSG32)(plane->crop_disp_offset +
                                        plane->tile.tileStartFrac);
                                stCfg2NDDMAParam.width    = (plane->wpl*plane->bpp +
                                        plane->tile.tileStrideFrac) * tiledModeWidHgtFactor *
                                    uiCromaAllignedLinedropFactor;
                                stCfg2NDDMAParam.stride   = (pinfo->m_buf_stride *
                                        plane->lineDroppingStrides) *tiledModeStrideFactor *
                                    uiCromaAllignedLinedropFactor;
                                stCfg2NDDMAParam.height   =
                                    plane->aligned_active_height/tiledModeWidHgtFactor;
                                stCfg2NDDMAParam.cfgQ     = cfgQ;
                                stCfg2NDDMAParam.mtrREn   = plane->m_mtrR__en;
                                stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_Y;
                                stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_Y;
                                stCfg2NDDMAParam.plane  = plane;

                                *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                            }
                            else
                            {
                                *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[0],
                                        stDhubParam.m_uiBaseAddr_Y, stDhubParam.m_uiBurst_Y,
                                        stDhubParam.m_uiStep_1_Y, stDhubParam.m_uiSize_1_Y,
                                        stDhubParam.m_uiStep_2_Y, stDhubParam.m_uiSize_2_Y,(T64b*)cfgQ);
                            }
                        }
#endif
                        else
                        {
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                            UINT32 uiStride = 0;
                            UINT32 uiAddr = 0;
                            if(SCAN_INVERSE_V == plane->uchInverseScanMode ||
                                    SCAN_INVERSE_HV == plane->uchInverseScanMode)
                            {
                                uiStride = ~(pinfo->m_buf_stride*plane->lineDroppingStrides*
                                        uiCromaAllignedLinedropFactor) + 1;
                                uiAddr = ((UNSG32)(plane->crop_disp_offset) & (~0x07)) +
                                    ((plane->aligned_active_height - 1) * pinfo->m_buf_stride *
                                     plane->lineDroppingStrides * uiCromaAllignedLinedropFactor);
                            }
                            else
                            {
                                uiAddr = (UNSG32)(plane->crop_disp_offset) & (~0x07);
                                uiStride = pinfo->m_buf_stride*plane->lineDroppingStrides *
                                    uiCromaAllignedLinedropFactor;
                            }

                            if(VPP_FE_DV_COMP_MODE_TEST_CVM == m_uchDvCompMode)
                            {
                                UINT32 uiRegVal = 0;
                                uiRegVal = ceil(((double)pinfo->m_active_width * 16)/128) *
                                    (pinfo->m_active_height);
                                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                                        vpp_obj->base_addr + RA_Vpp_vidpl +
                                        RA_vidpl_ClientIFm + RA_ClientIF_MR0_word, uiRegVal );
                                vpp_obj->clientif_ctrl0.uCTRL0_packSel_MR0 = 2;
                                vpp_obj->clientif_ctrl0.uCTRL0_packSel_MR1 = 3;
                                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                                        vpp_obj->base_addr + RA_Vpp_vidpl +
                                        RA_vidpl_ClientIFm + RA_ClientIF_CTRL0,
                                        vpp_obj->clientif_ctrl0.u32);
                                *cfgQ_len += START_2NDDMA(plane->dmaRdhubID,
                                        plane->dmaRID[0], uiAddr,
                                        (plane->wpl * plane->bpp + plane->tile.tileStrideFrac *
                                         uiCromaAllignedLinedropFactor) * 2,
                                        (plane->wpl*plane->bpp+plane->tile.tileStrideFrac*
                                         uiCromaAllignedLinedropFactor) * 2, 1, uiStride * 2,
                                        plane->aligned_active_height, (T64b*)cfgQ);
                            }
                            else if ((SRCFMT_YUV422SP_DWA == plane->srcfmt) ||
                                    (SRCFMT_YUV420SP_DWA == plane->srcfmt))
                            {
                                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                                stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                                stCfg2NDDMAParam.addr   = uiAddr;
                                stCfg2NDDMAParam.width  =
                                    (plane->wpl * PIXEL_PER_BEAT_YUV_420SP_DWA *
                                     plane->bpp * 32)/(30*8) +
                                    plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor;
                                stCfg2NDDMAParam.stride = uiStride;
                                stCfg2NDDMAParam.height = plane->aligned_active_height;
                                stCfg2NDDMAParam.cfgQ   = cfgQ;
                                stCfg2NDDMAParam.mtrREn = 0;
                                stCfg2NDDMAParam.plane  = plane;
                                *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                            }
                            else
                            {
                                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                                stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                                stCfg2NDDMAParam.addr   = uiAddr;
                                stCfg2NDDMAParam.width  = plane->wpl * plane->bpp +
                                    plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor;
                                stCfg2NDDMAParam.stride = uiStride;
                                stCfg2NDDMAParam.height = plane->aligned_active_height;
                                stCfg2NDDMAParam.cfgQ   = cfgQ;
                                stCfg2NDDMAParam.mtrREn   = plane->m_mtrR__en;
                                stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_Y;
                                stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_Y;
                                stCfg2NDDMAParam.plane  = plane;
                                *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                            }
#else

                            *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[0],
                                    (UNSG32)(plane->crop_disp_offset),
                                    plane->wpl * plane->bpp + plane->tile.tileStrideFrac *
                                    uiCromaAllignedLinedropFactor,
                                    plane->wpl * plane->bpp + plane->tile.tileStrideFrac*
                                    uiCromaAllignedLinedropFactor, 1, pinfo->m_buf_stride *
                                    plane->lineDroppingStrides * uiCromaAllignedLinedropFactor,
                                    plane->aligned_active_height, (T64b*)cfgQ);
#endif
#if defined(VPP_ON_VELOCE)
                            VPPAPI_DBG_INFO("burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",plane->wpl*plane->bpp+plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor, plane->wpl*plane->bpp+plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor, 1, pinfo->m_buf_stride*plane->lineDroppingStrides*uiCromaAllignedLinedropFactor, plane->aligned_active_height);
#endif
                        }
                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
#ifdef CONFIG_AMP_VPP_ROTATION
                        if((plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 &&
                                    (8 == pinfo->m_bits_per_pixel)) ||
                                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 )
#else
                        if(plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
                        {
                            stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                            stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                            stCfg2NDDMAParam.addr   = (UNSG32)(plane->crop_disp_offset_UV +
                                    plane->tile.tileStartFrac);
                            stCfg2NDDMAParam.width  = (plane->wpl * plane->bpp +
                                    plane->tile.tileStrideFrac)*tiledModeWidHgtFactor;
                            stCfg2NDDMAParam.stride = (pinfo->m_buf_stride_UV *
                                    plane->lineDroppingStrides)*tiledModeStrideFactor;
                            stCfg2NDDMAParam.height = plane->aligned_active_height/
                                (2*tiledModeWidHgtFactor);
                            stCfg2NDDMAParam.cfgQ   = cfgQ;
                            stCfg2NDDMAParam.mtrREn = 0;
                            stCfg2NDDMAParam.plane  = plane;
                            *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
#if defined(VPP_ON_VELOCE)
                            VPPAPI_DBG_INFO("burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",(plane->wpl*plane->bpp+plane->tile.tileStrideFrac)*tiledModeWidHgtFactor, (plane->wpl*plane->bpp+plane->tile.tileStrideFrac)*tiledModeWidHgtFactor, 1, (pinfo->m_buf_stride_UV*plane->lineDroppingStrides)*tiledModeStrideFactor, plane->aligned_active_height/(2*tiledModeWidHgtFactor));
#endif
                        }
#ifdef CONFIG_AMP_VPP_ROTATION
                        else if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt ||
                                ((plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0) &&
                                 (10 == pinfo->m_bits_per_pixel)))
                        {
                            if(ROTATION_0D == plane->uchRotationAngle)
                            {
                                UINT32 CDhubPadHeight = 0;
                                if((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable))
                                {
                                    PixelPerBeat = PIXEL_PER_BEAT_YUV_420SP;
                                    plane->wpl = (plane->m_Aligned_Chroma_Width +
                                            PixelPerBeat - 1) / PixelPerBeat;
                                    CDhubPadHeight = plane->m_Aligned_Luma_Height/2;
                                    CDhubPadHeight = CDhubPadHeight/tiledModeWidHgtFactor;
                                }
                                else
                                {
                                    CDhubPadHeight = plane->aligned_active_height/
                                        (2*tiledModeWidHgtFactor);
                                }

                                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                                stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                                stCfg2NDDMAParam.addr   = (UNSG32)(plane->crop_disp_offset_UV +
                                        plane->tile.tileStartFrac);
                                stCfg2NDDMAParam.width  = (plane->wpl*plane->bpp +
                                        plane->tile.tileStrideFrac)* tiledModeWidHgtFactor;
                                stCfg2NDDMAParam.stride = (pinfo->m_buf_stride_UV *
                                        plane->lineDroppingStrides) * tiledModeStrideFactor;
                                stCfg2NDDMAParam.height = CDhubPadHeight;
                                stCfg2NDDMAParam.cfgQ   = cfgQ;
                                stCfg2NDDMAParam.mtrREn   = plane->m_mtrR__en;
                                stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_UV;
                                stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_UV;
                                stCfg2NDDMAParam.plane  = plane;
                                *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                            }
                            else
                            {
                                *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[1],
                                        stDhubParam.m_uiBaseAddr_UV, stDhubParam.m_uiBurst_UV,
                                        stDhubParam.m_uiStep_1_UV, stDhubParam.m_uiSize_1_UV,
                                        stDhubParam.m_uiStep_2_UV, stDhubParam.m_uiSize_2_UV,
                                        (T64b*)cfgQ);
                            }
                        }

#endif
                        else
                        {
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                            UINT32 uiStride = 0;
                            UINT32 uiAddr = 0;
                            if(SCAN_INVERSE_V == plane->uchInverseScanMode ||
                                    SCAN_INVERSE_HV == plane->uchInverseScanMode)
                            {
                                uiStride = ~(pinfo->m_buf_stride_UV*plane->lineDroppingStrides) + 1;
                                if(SRCFMT_YUV422SP_DWA == plane->srcfmt)
                                {
                                    uiAddr = ((UNSG32)(plane->crop_disp_offset_UV) & (~0x07)) +
                                        ((plane->aligned_active_height - 1) *
                                         pinfo->m_buf_stride_UV * plane->lineDroppingStrides);
                                }
                                else
                                {
                                    uiAddr = ((UNSG32)(plane->crop_disp_offset_UV) & (~0x07)) + \
                                             ((plane->aligned_active_height/2 - 1) *
                                              pinfo->m_buf_stride_UV * plane->lineDroppingStrides);
                                }
                            }
                            else
                            {
                                uiAddr = (UNSG32)(plane->crop_disp_offset_UV) & (~0x07);
                                uiStride = pinfo->m_buf_stride_UV*plane->lineDroppingStrides;
                            }

                            if(VPP_FE_DV_COMP_MODE_TEST_CVM == m_uchDvCompMode)
                            {
                                UINT32 uiRegVal = 0;
                                uiRegVal = ceil(((double)pinfo->m_active_width * 16)/128) *
                                    (pinfo->m_active_height/2);
                                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                                        vpp_obj->base_addr + RA_Vpp_vidpl +
                                        RA_vidpl_ClientIFm + RA_ClientIF_MR1_word, uiRegVal );
                                uiAddr = (UINT32)pinfo->m_pbuf_start +
                                    pinfo->m_active_width * pinfo->m_active_height * 2;
                                *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[1],
                                        uiAddr, (plane->wpl*plane->bpp+plane->tile.tileStrideFrac) * 2,
                                        (plane->wpl*plane->bpp+plane->tile.tileStrideFrac) * 2, 1,
                                        uiStride * 2, plane->aligned_active_height/2, (T64b*)cfgQ);
                            }
                            else if ((SRCFMT_YUV422SP_DWA == plane->srcfmt)||
                                    (SRCFMT_YUV420SP_DWA == plane->srcfmt))
                            {
                                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                                stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                                stCfg2NDDMAParam.addr   = uiAddr;
                                stCfg2NDDMAParam.width  = (plane->wpl *
                                        PIXEL_PER_BEAT_YUV_420SP_DWA * plane->bpp * 32)/(30*8) +
                                    plane->tile.tileStrideFrac;
                                stCfg2NDDMAParam.stride = uiStride;
                                if (SRCFMT_YUV422SP_DWA == plane->srcfmt)
                                    stCfg2NDDMAParam.height = plane->aligned_active_height;
                                else
                                    stCfg2NDDMAParam.height = plane->aligned_active_height/2;
                                stCfg2NDDMAParam.cfgQ   = cfgQ;
                                stCfg2NDDMAParam.mtrREn   = plane->m_mtrR__en;
                                stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_UV;
                                stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_UV;
                                stCfg2NDDMAParam.plane  = plane;
                                *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                            }
                            else
                            {
                                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                                stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                                stCfg2NDDMAParam.addr   = uiAddr;
                                stCfg2NDDMAParam.width  =
                                    plane->wpl*plane->bpp+plane->tile.tileStrideFrac;
                                stCfg2NDDMAParam.stride = uiStride;
                                stCfg2NDDMAParam.height = plane->aligned_active_height/2;
                                stCfg2NDDMAParam.cfgQ   = cfgQ;
                                stCfg2NDDMAParam.mtrREn   = plane->m_mtrR__en;
                                stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_UV;
                                stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_UV;
                                stCfg2NDDMAParam.plane  = plane;
                                *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                            }
#else
                            *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[1],
                                    (UNSG32)(plane->crop_disp_offset_UV),
                                    plane->wpl * plane->bpp + plane->tile.tileStrideFrac,
                                    plane->wpl * plane->bpp + plane->tile.tileStrideFrac, 1,
                                    pinfo->m_buf_stride_UV*plane->lineDroppingStrides,
                                    plane->aligned_active_height/2, (T64b*)cfgQ);
#endif
#if defined(VPP_ON_VELOCE)

                            VPPAPI_DBG_INFO("burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",plane->wpl*plane->bpp+plane->tile.tileStrideFrac, plane->wpl*plane->bpp+plane->tile.tileStrideFrac, 1, pinfo->m_buf_stride_UV*plane->lineDroppingStrides, plane->aligned_active_height/2);
#endif
                        }
                    } else {
                        INT32 iBytesPerWord = 0;
                        if(SRCFMT_YUV422 == plane->srcfmt)
                        {
                            /* TODO Needs review, logically may not to correct
                               for 10Bit case even though the final calculated
                               value is correct.May needs to upgrade word size
                               in (wpl) to fit 10Bit formats */
                            iBytesPerWord = plane->bpp/2;
                        }
                        else if(SRCFMT_RGB888 == plane->srcfmt)
                        {
                            /* TODO Hard fixed for 24bit */
                            iBytesPerWord = 6;
                        }
                        else if(SRCFMT_YUV444 == plane->srcfmt)
                        {
                            if(plane->bits_per_pixel == 8)
                                iBytesPerWord = 12;
                            else
                                iBytesPerWord = 15;
                        }
                        else
                        {
                            /* NOTE: since word size of wpl is 64bits,we can have 8bytes in it.
                               This should be the standard case for all formats,
                               but 10bits formats are not fitting exactly.
                               Currectly tested for ARGB format in PIP path for BG4CDpA0
                             */
                            iBytesPerWord = 8;
                        }
                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                        UINT32 uiStride = 0;
                        UINT32 uiAddr = 0;
                        if(SCAN_INVERSE_V == plane->uchInverseScanMode ||
                                SCAN_INVERSE_HV == plane->uchInverseScanMode)
                        {
                            uiStride = ~(pinfo->m_buf_stride*plane->lineDroppingStrides) + 1;
                            uiAddr = ((UNSG32)(plane->crop_disp_offset) & (~0x07)) +
                                ((plane->aligned_active_height - 1) *
                                 pinfo->m_buf_stride*plane->lineDroppingStrides);
                        }
                        else
                        {
                            uiAddr = (UNSG32)(plane->crop_disp_offset) & (~0x07);
                            uiStride = pinfo->m_buf_stride*plane->lineDroppingStrides;
                        }

                        if((VPP_FE_DV_COMP_MODE_TEST_EDR == m_uchDvCompMode) || \
                              (VPP_FE_DV_COMP_MODE_TEST_DITHER == m_uchDvCompMode))
                        {
                                UINT32 width_in_bytes, width, height;

                                width = pinfo->m_active_width;
                                height = pinfo->m_active_height;

                                FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_OFFLINE_RD_0);
                                FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_OFFLINE_RD_0,
                                        plane->srcfmt, 1);

                                //Program Packsel
                                T32OfflineRd_CTRL3 offlinectrl3;
                                T32OfflineRd_CTRL1 offlinectrl1;
                                uiAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN;
                                GA_REG_WORD32_READ(uiAddr + RA_OfflineRd_CTRL3, &offlinectrl3.u32);
                                GA_REG_WORD32_READ(uiAddr + RA_OfflineRd_CTRL1, &offlinectrl1.u32);


                                offlinectrl3.uCTRL3_packSel = 2;
                                offlinectrl3.uCTRL3_idata_ctrl = 2;
                                offlinectrl3.uCTRL3_ups422to444_bypass = 1;
                                offlinectrl3.uCTRL3_fifosts_ctrl = 1;
                                offlinectrl3.uCTRL3_pixlineTot = width;

                                offlinectrl1.uCTRL1_tot = (width * height * 3)/16;

                                if((pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR422) ||
                                      (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_IPT422))
                                {
                                    offlinectrl3.uCTRL3_idata_sel = 1;
                                    offlinectrl3.uCTRL3_packSel = 2; //2 - YUV422, 3 - YUV444
                                    offlinectrl1.uCTRL1_tot = (width * height * 3)/16;
                                    width_in_bytes = width * 3;
                                    uiStride = width * 3;
                                }
                                else
                                {
                                    //YUV444
                                    offlinectrl3.uCTRL3_idata_sel = 0;
                                    offlinectrl3.uCTRL3_packSel = 3; //2 - YUV422, 3 - YUV444
                                    offlinectrl1.uCTRL1_tot = (width * height * 36)/(16 * 8);
                                    width_in_bytes = (width * 36)/8;
                                    uiStride = (width * 36)/8;
                                }

                                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                                        uiAddr + RA_OfflineRd_CTRL3, offlinectrl3.u32);
                                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                                        uiAddr + RA_OfflineRd_CTRL1, offlinectrl1.u32);

                                stCfg2NDDMAParam.dhubId = plane->edr_dmaRdhubID;
                                stCfg2NDDMAParam.dmaId  = plane->edr_dmaRID;
                                stCfg2NDDMAParam.addr   = (UNSG32)(plane->crop_disp_offset)&(~0x07);
                                stCfg2NDDMAParam.width  = width_in_bytes;
                                stCfg2NDDMAParam.stride = uiStride;
                                stCfg2NDDMAParam.height = height;
                                stCfg2NDDMAParam.cfgQ   = cfgQ;
                                stCfg2NDDMAParam.mtrREn = 0;
                                stCfg2NDDMAParam.plane  = plane;
                                *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

                                cfgQ = cfgQ_shadow + *cfgQ_len * 2;

                        }
                        stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                        stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                        stCfg2NDDMAParam.addr   = uiAddr;
                        stCfg2NDDMAParam.width  = plane->wpl*iBytesPerWord;
                        stCfg2NDDMAParam.stride = uiStride;
                        stCfg2NDDMAParam.height = plane->aligned_active_height;
                        stCfg2NDDMAParam.cfgQ   = cfgQ;
                        stCfg2NDDMAParam.mtrREn = plane->m_mtrR__en;
                        stCfg2NDDMAParam.plane  = plane;
                        stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_Y;
                        stCfg2NDDMAParam.updSemId = plane->m_mtrR_chkSemId_UV;
                        if(0 == vpp_obj->uiDvCompElUsingPip)
                        {
                            *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                        }
#if defined(VPP_ON_VELOCE)
                        VPPAPI_DBG_INFO("burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",plane->wpl*plane->bpp/2, plane->wpl*plane->bpp/2, 1, pinfo->m_buf_stride*plane->lineDroppingStrides, plane->aligned_active_height);
#endif
                    }
                }
            } else {
                /* interlaced input */
#ifdef ENABLE_OVP
                UNSG32 start_addr;
                if(plane->srcfmt == SRCFMT_YUV420SP ||
                        plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                        plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
                {
                    if(1 == pinfo->m_OrgInputInterlaced) {
                        //Always buff1 is valid first
                        if(0 == plane->uchFieldBuffInUse)
                        {
                            uiBuff1Valid = 1;
                        }
                        else
                        {
                            if(pinfo->m_Out_buf0_valid)
                            {
                                uiBuff0Valid = 1;
                            }
                            else
                            {
                                uiBuff1Valid = 1;
                            }
                        }

                        if(uiBuff0Valid)
                        {
                            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                            start_addr = ((UNSG32)((uintptr_t)pinfo->m_pbuf_start)) +
                                pinfo->m_disp_offset + pinfo->m_content_offset;
                            *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[0],
                                    (UNSG32)(start_addr), pinfo->m_buf_stride*2,
                                    pinfo->m_buf_stride, pinfo->m_active_height/2, (T64b*)cfgQ);

                            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                            start_addr = (int)pinfo->m_buf_pbuf_start_UV +
                                pinfo->m_disp_offset + pinfo->m_content_offset;
                            *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[1],
                                    (UNSG32)(start_addr), pinfo->m_buf_stride_UV*2,
                                    pinfo->m_active_width, pinfo->m_active_height/4, (T64b*)cfgQ);

                            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                            start_addr = (int)pinfo->m_pbuf_Out_start_0 +
                                pinfo->m_disp_offset + pinfo->m_content_offset;
                            *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[2],
                                    (UNSG32)(start_addr), pinfo->m_buf_stride,
                                    pinfo->m_active_width, pinfo->m_active_height/2, (T64b*)cfgQ);

                            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                            start_addr = (int)pinfo->m_pbuf_Out_start_UV_0 +
                                pinfo->m_disp_offset + pinfo->m_content_offset;
                            *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[3],
                                    (UNSG32)(start_addr), pinfo->m_buf_stride_UV,
                                    pinfo->m_active_width, pinfo->m_active_height/4,(T64b*)cfgQ);

                            plane->uchFieldBuffInUse = 0;
                            //Mark that current FD is in use by VPP
                            pinfo->m_FrameDesc_RefStat |= VPP_FRAME_IN_USE;
                            //Compare the last frame with current frame
                            if(vpp_obj->main_last_frame  == inline_frame)
                            {
                                plane->uchFieldBuffInUse = 1;
                            }
                            vpp_obj->main_last_frame = inline_frame;
                        }
                        if(uiBuff1Valid)
                        {
                            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                            start_addr = (int)pinfo->m_pbuf_Out_start_1 +
                                pinfo->m_disp_offset + pinfo->m_content_offset;
                            *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[0],
                                    (UNSG32)(start_addr), pinfo->m_buf_stride,
                                    pinfo->m_active_width, pinfo->m_active_height/2, (T64b*)cfgQ);

                            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                            start_addr = (int)pinfo->m_pbuf_Out_start_UV_1 +
                                pinfo->m_disp_offset + pinfo->m_content_offset;
                            *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[1],
                                    (UNSG32)(start_addr), pinfo->m_buf_stride_UV,
                                    pinfo->m_active_width, pinfo->m_active_height/4,(T64b*)cfgQ);

                            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                            start_addr = ((UNSG32)((uintptr_t)pinfo->m_pbuf_start)) +
                                pinfo->m_buf_stride + pinfo->m_disp_offset + pinfo->m_content_offset;
                            *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[2],
                                    (UNSG32)(start_addr), pinfo->m_buf_stride*2,
                                    pinfo->m_active_width, pinfo->m_active_height/2, (T64b*)cfgQ);

                            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                            start_addr = (int)pinfo->m_buf_pbuf_start_UV +
                                pinfo->m_buf_stride_UV + pinfo->m_disp_offset +
                                pinfo->m_content_offset;
                            *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[3],
                                    (UNSG32)(start_addr), pinfo->m_buf_stride_UV*2,
                                    pinfo->m_active_width, pinfo->m_active_height/4,  (T64b*)cfgQ);

                            plane->uchFieldBuffInUse = 1;
                            pinfo->m_FrameDesc_RefStat |= VPP_FRAME_IN_USE;
                        }
                    } else {
                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        start_addr = (int)pinfo->m_pbuf_start + pinfo->m_disp_offset +
                            pinfo->m_content_offset;
                        *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[0],
                                (UNSG32)(start_addr), pinfo->m_buf_stride*2,
                                pinfo->m_active_width, pinfo->m_active_height/2, (T64b*)cfgQ);

                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        start_addr = (int)pinfo->m_buf_pbuf_start_UV + pinfo->m_disp_offset +
                            pinfo->m_content_offset;
                        *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[1],
                                (UNSG32)(start_addr), pinfo->m_buf_stride_UV*2,
                                pinfo->m_active_width, pinfo->m_active_height/4,  (T64b*)cfgQ);

                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        start_addr = (int)pinfo->m_pbuf_start + pinfo->m_buf_stride +
                            pinfo->m_disp_offset + pinfo->m_content_offset;
                        *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[2],
                                (UNSG32)(start_addr), pinfo->m_buf_stride*2,
                                pinfo->m_active_width, pinfo->m_active_height/2, (T64b*)cfgQ);

                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        start_addr = (int)pinfo->m_buf_pbuf_start_UV + pinfo->m_buf_stride_UV +
                            pinfo->m_disp_offset + pinfo->m_content_offset;
                        *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[3],
                                (UNSG32)(start_addr), pinfo->m_buf_stride_UV*2,
                                pinfo->m_active_width, pinfo->m_active_height/4,    (T64b*)cfgQ);
                    }
                }else {
                    cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                    start_addr = (UNSG32)((uintptr_t)pinfo->m_pbuf_start +
                            pinfo->m_disp_offset + pinfo->m_content_offset);
                    *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[0],
                            (UNSG32)(start_addr), pinfo->m_buf_stride*2,
                            pinfo->m_buf_stride, pinfo->m_active_height/2, (T64b*)cfgQ);

                    cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                    start_addr = (UNSG32)((uintptr_t)pinfo->m_pbuf_start  +
                            pinfo->m_disp_offset + pinfo->m_content_offset) + pinfo->m_buf_stride;
                    *cfgQ_len += START_2DDMA(plane->dmaRdhubID, plane->dmaRID[2],
                            (UNSG32)(start_addr), pinfo->m_buf_stride*2, pinfo->m_buf_stride,
                            pinfo->m_active_height/2, (T64b*)cfgQ);
                }
#else /*#ifdef ENABLE_OVP */
                /* interlaced frame */
                int  field_to_be_out = 0, field_to_be = 0;
#ifdef PE_3D_FMT_CONVERT
                if (!pDVResinfo->flag_3d || ((pDVResinfo->flag_3d) && (vpp_obj->dv[cpcbID].viewID==0)))
                {
                    field_to_be_out = -1; //invalid field
#endif

#if defined(REPEAT_FRAME_ENHANCE)
                    if (plane->field == 0)
                    {
                        if (vpp_obj->main_last_frame == inline_frame ||
                                (pinfo->m_flags & VINFO_FLAG_MASK_REPEAT_FRAME))
                        {
                            if(pinfo->m_is_progressive_pic)
                            {
                                vpp_obj->main_frame_repeat = 0;
                                vpp_obj->main_frame_first_field_cnt =0;
#if defined(PULLUP_ENABLE)
                                if (pinfo->m_is_repeat_first_field) plane->field = 1;
#endif
                            }
                            else
                            {
                                if(pinfo->m_flags & VINFO_FLAG_MASK_FIELD_FIRST)
                                {
                                    //Make sure the first frame of the
                                    //sequece can rotated several times.
                                    if(vpp_obj->main_frame_first_field_cnt < 1)
                                    {
                                        vpp_obj->main_frame_first_field_cnt++;
                                        vpp_obj->main_frame_repeat = 0;
                                    }
                                    else
                                    {
                                        vpp_obj->main_frame_repeat = 1;
                                    }
                                }
                                else
                                {
                                    vpp_obj->main_frame_first_field_cnt =0;
                                    vpp_obj->main_frame_repeat = 1;
                                }
                            }
                        }
                        else
                        {
                            vpp_obj->main_frame_repeat = 0;
                            vpp_obj->main_frame_first_field_cnt =0;
                        }
                        vpp_obj->main_last_frame = inline_frame;
                    }
#endif

#if defined(PULLUP_ENABLE)
                    if(pinfo->m_is_progressive_pic)
                    {
                        // progressive frame don't care about top bottom
                        //keep output fields original order
                        if(vpp_obj->main_last_takeout_field == 0)
                            field_to_be_out = 1;
                        else
                            field_to_be_out = 0;
                    }
                    else
#endif
                    {
#if defined(PULLUP_ENABLE)
                        /* first field or repeat field */
                        if ((plane->field == 0) || (plane->field == 2))
#else
                        /* first field */
                        if (plane->field == 0)
#endif
                        {
                            if (pinfo->m_is_top_field_first)
                            { // Changed
#if defined(REPEAT_FRAME_ENHANCE)
                                if (vpp_obj->main_frame_repeat)
                                    field_to_be_out = 0;
                                else
#endif
                                    field_to_be_out = 1;
                            }
                            else
                            {
#if defined(REPEAT_FRAME_ENHANCE)
                                if (vpp_obj->main_frame_repeat)
                                    field_to_be_out = 1;
                                else
#endif
                                    field_to_be_out = 0;
                            }
#if defined(REPEAT_FRAME_ENHANCE)
                            if (vpp_obj->main_frame_repeat == 0 &&
                                    field_to_be_out == vpp_obj->main_last_takeout_field
#ifdef DISP_FIELD_FRC
                                    && ((pinfo -> m_flags & VINFO_FLAG_MASK_FIELD_FRAME) ==0)
#endif
                               )
                            {
                                // two take out the same one, order changed, don't rotate buffers
                                vpp_obj->main_frame_repeat = 2;
                            }
#endif
                        }
#if defined(PULLUP_ENABLE)

                        else if (plane->field == 1)/* second field */
#else

                        else/* second field */
#endif
                        {
                            if (pinfo->m_is_top_field_first)
                            { //Changed
                                field_to_be_out = 0;
                            }
                            else
                            {
                                field_to_be_out = 1;
                            }
                        }
                    }

                    // update field
                    field_to_be = plane->field + 1;
#if defined(PULLUP_ENABLE)
                    if(field_to_be > 2 || (field_to_be == 2 && pinfo->m_is_repeat_first_field ==0))
#else
                    if(field_to_be > 1)
#endif
                    {
                        field_to_be =0;
                    }
                    plane->field = field_to_be;

#ifdef PE_3D_FMT_CONVERT
                }
                else
                {
                    field_to_be_out = vpp_obj->main_last_takeout_field;
                }
#endif

                // Start DMA
                if(field_to_be_out ==0)
                {
                    UNSG32 start_addr;
                    if (pinfo->m_3D_source_fmt == VDEC_BUF_3D_FMT_FP)
                    {
                        // this is a interlaced 3D frame-packing frame
                        if (vpp_obj->dv[cpcbID].viewID==0)
                        {
                            // left view
                            start_addr = (UNSG32)plane->crop_disp_offset +
                                pinfo->m_buf_stride*(1080+45);
                        }
                        else
                        {
                            // right view
                            start_addr = (UNSG32)plane->crop_disp_offset +
                                pinfo->m_buf_stride*(1080+45+540+23);
                        }
                    }
                    else if (pinfo->m_interlace_weave_mode)
                    { // non-weaved interlaced input
                        start_addr = plane->crop_disp_offset +
                            pinfo->m_buf_stride*plane->crop_actv_win.height/2;
                    }
                    else
                    {
#ifdef DISP_FIELD_FRC
                        if(pinfo -> m_is_virtual_frame)
                        {
                            if(pinfo->m_is_top_field_first)
                            {
                                start_addr = (UNSG32)plane->crop_disp_offset -
                                    (UNSG32)pinfo ->m_pbuf_start + (UNSG32)pinfo ->m_sec_field_start;
                            }
                            else
                            {
                                start_addr = plane->crop_disp_offset + pinfo->m_buf_stride;
                            }
                        }
                        else
#endif
                        {
                            start_addr = plane->crop_disp_offset+pinfo->m_buf_stride;
                        }
                        if(pinfo -> m_flags & VINFO_FLAG_MASK_HALF_FIELD)
                            start_addr -= pinfo->m_buf_stride;
                    }
                    FE_DEINT_SetField(vpp_obj, 0);
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
                    {
                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        start_addr = plane->crop_disp_offset + pinfo->m_buf_stride;

                        stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                        stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                        stCfg2NDDMAParam.addr   = (UNSG32)(start_addr);
                        stCfg2NDDMAParam.width  = plane->wpl*plane->bpp+plane->tile.tileStrideFrac;
                        stCfg2NDDMAParam.stride = pinfo->m_buf_stride*2*plane->lineDroppingStrides;
                        stCfg2NDDMAParam.height = plane->aligned_active_height;
                        stCfg2NDDMAParam.cfgQ   = cfgQ;
                        stCfg2NDDMAParam.mtrREn = 0;
                        stCfg2NDDMAParam.plane  = plane;
                        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        start_addr = plane->crop_disp_offset_UV + pinfo->m_buf_stride_UV;

                        stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                        stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                        stCfg2NDDMAParam.addr   = (UNSG32)(start_addr);
                        stCfg2NDDMAParam.width  = plane->wpl*plane->bpp+plane->tile.tileStrideFrac;
                        stCfg2NDDMAParam.stride = pinfo->m_buf_stride_UV*2*plane->lineDroppingStrides;
                        stCfg2NDDMAParam.height = plane->aligned_active_height/2;
                        stCfg2NDDMAParam.cfgQ   = cfgQ;
                        stCfg2NDDMAParam.mtrREn = 0;
                        stCfg2NDDMAParam.plane  = plane;
                        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                    }
                    else
                    {
                        /* get start address for next dump */
                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        /* dump dhub setting to cfgQ */
                        stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                        stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                        stCfg2NDDMAParam.addr   = (UNSG32)(start_addr) & (~0x07);
                        stCfg2NDDMAParam.width  = plane->wpl*plane->bpp/2;
                        stCfg2NDDMAParam.stride = pinfo->m_buf_stride*2*plane->lineDroppingStrides;
                        stCfg2NDDMAParam.height = plane->aligned_active_height;
                        stCfg2NDDMAParam.cfgQ   = cfgQ;
                        stCfg2NDDMAParam.mtrREn = 0;
                        stCfg2NDDMAParam.plane  = plane;
                        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                    }
                    vpp_obj->main_last_takeout_field = 0;
                }
                else
                {

                    UNSG32 start_addr;
                    if (pinfo->m_3D_source_fmt == VDEC_BUF_3D_FMT_FP)
                    {
                        // this is a interlaced 3D frame-packing frame
                        if (vpp_obj->dv[cpcbID].viewID==0)
                        {
                            // left view
                            start_addr = (UNSG32)plane->crop_disp_offset +
                                pinfo->m_buf_stride*(1080+45);
                        }
                        else
                        {
                            // right view
                            start_addr = (UNSG32)plane->crop_disp_offset +
                                pinfo->m_buf_stride*(1080+45+540+23);
                        }
                    }
                    else if (pinfo->m_interlace_weave_mode)
                    { // non-weaved interlaced input
                        start_addr = plane->crop_disp_offset;
                    }
                    else
                    {
#ifdef DISP_FIELD_FRC
                        if(pinfo -> m_is_virtual_frame)
                        {
                            if(pinfo->m_is_top_field_first)
                            {
                                start_addr = plane->crop_disp_offset;
                            }
                            else
                            {
                                start_addr = (UNSG32)plane->crop_disp_offset -
                                    (UNSG32)pinfo ->m_pbuf_start + (UNSG32)pinfo ->m_sec_field_start;
                            }
                        }
                        else
#endif
                        {
                            start_addr = (UNSG32)plane->crop_disp_offset;
                        }
                    }
                    FE_DEINT_SetField(vpp_obj, 1);
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
                    {
                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        start_addr = plane->crop_disp_offset;
                        stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                        stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                        stCfg2NDDMAParam.addr   = (UNSG32)(start_addr);
                        stCfg2NDDMAParam.width  = plane->wpl*plane->bpp+plane->tile.tileStrideFrac;
                        stCfg2NDDMAParam.stride = pinfo->m_buf_stride*2*plane->lineDroppingStrides;
                        stCfg2NDDMAParam.height = plane->aligned_active_height;
                        stCfg2NDDMAParam.cfgQ   = cfgQ;
                        stCfg2NDDMAParam.mtrREn = 0;
                        stCfg2NDDMAParam.plane  = plane;
                        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        start_addr = plane->crop_disp_offset_UV;
                        stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                        stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                        stCfg2NDDMAParam.addr   = (UNSG32)(start_addr);
                        stCfg2NDDMAParam.width  = plane->wpl*plane->bpp+plane->tile.tileStrideFrac;
                        stCfg2NDDMAParam.stride = pinfo->m_buf_stride_UV*2*plane->lineDroppingStrides;
                        stCfg2NDDMAParam.height = plane->aligned_active_height/2;
                        stCfg2NDDMAParam.cfgQ   = cfgQ;
                        stCfg2NDDMAParam.mtrREn = 0;
                        stCfg2NDDMAParam.plane  = plane;
                        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                    }
                    else
                    {
                        /* get start address for next dump */
                        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
                        /* dump dhub setting to cfgQ */
                        stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                        stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                        stCfg2NDDMAParam.addr   = (UNSG32)(start_addr) & (~0x07);
                        stCfg2NDDMAParam.width  = plane->wpl*plane->bpp/2;
                        stCfg2NDDMAParam.stride = pinfo->m_buf_stride*2*plane->lineDroppingStrides;
                        stCfg2NDDMAParam.height = plane->aligned_active_height;
                        stCfg2NDDMAParam.cfgQ   = cfgQ;
                        stCfg2NDDMAParam.mtrREn = 0;
                        stCfg2NDDMAParam.plane  = plane;
                        *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                    }
                    vpp_obj->main_last_takeout_field = 1;
                }
#endif
            }

#if defined(PE_3D_BD)
            if (pDVResinfo->flag_3d)
            {
                /* recover left view start address, if was updated to right view */
                if ((plane->ss_type == 1) && (pinfo_pbuf_start != NULL))
                    pinfo->m_pbuf_start = (ARCH_PTR_TYPE)pinfo_pbuf_start;
            }
#endif
            MV_VPPOBJ_StartChannelDataLoader_Ngptv(vpp_obj, planeID);
            /* start data loader */
            if ((planeID == PLANE_MAIN) || (planeID == PLANE_PIP)) {

                /* start in-line data loader */
                if (planeID == PLANE_MAIN)
                {
#ifdef ENABLE_EECMU
                    if(vpp_obj->cmu.cmu_enabled)
                    {
                        VPP_CMU_TG_Clear(vpp_obj);
                        VPP_CMU_TG_Start(vpp_obj);
                    }
#endif
#ifdef ENABLE_DETEE
                    if(vpp_obj->detee.detee_enabled == 2)
                    {
                        VPP_DETEE_TG_Clear(vpp_obj);
                        VPP_DETEE_TG_Start(vpp_obj);
                    }
#endif
                    /* VP setting update */
                    if (plane->vp_updated)
                    {
                        updateVPSetting(vpp_obj, plane);
                        //plane->vp_updated = 0;
                        if (plane->vp_buff_num == 3 && plane->vp_updated==1)
                        {
                            plane->skip_frame = 1;
                            if(0 == plane->frame_type)
                            {
                                updateVPSize(vpp_obj, plane, &feRes);

                                FE_DEINT_ChangeMode(vpp_obj, DEINT_2D, plane->uchInverseScanMode);
                                plane->uiPseudoDeintMode = DEINT_2D;
                            }
                        }
                        else
                        {
                            if (plane->skip_frame)
                            {
                                plane->skip_frame = 0;
                            }
                        }
                        plane->vp_updated = 0;
                    }

                    if (plane->skip_frame == 1)
                    {
                        plane->skip_frame = 2;
                    }
                    else if (plane->skip_frame == 2)
                    {
                        plane->skip_frame = 3;
                    }
                    else if (plane->skip_frame == 3)
                    {
                        if((0 == plane->frame_type) && (plane->deint_mode == DEINT_3D))
                        {
                            FE_DEINT_ChangeMode(vpp_obj, DEINT_3D, plane->uchInverseScanMode);
                            plane->uiPseudoDeintMode = DEINT_3D;
                        }
                        plane->skip_frame = 0;
                    }

                    if((0 == plane->frame_type) && (0 == plane->skip_frame))
                    {
                        if(pinfo->m_flags & VINFO_FLAG_MASK_SEEK_SHOW)
                        {
                            FE_DEINT_ChangeMode(vpp_obj, DEINT_2D, plane->uchInverseScanMode);
                            plane->uiPseudoDeintMode = DEINT_2D;
                        }
                        else if(plane->deint_mode == DEINT_3D && DEINT_2D == plane->uiPseudoDeintMode)
                        {
                            FE_DEINT_ChangeMode(vpp_obj, DEINT_3D, plane->uchInverseScanMode);
                            plane->uiPseudoDeintMode = DEINT_3D;
                        }
                    }

                    /* VP buffer management */
                    if (plane->vp_buff_num)
                        handleVPBuffer(vpp_obj, plane);

                    VDBG_IF_DISP("VP TOP DHUB size = :  %d,%d\n",
                            plane->crop_actv_win.width, plane->crop_actv_win.height/2);
                    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_0);
                    if(vpp_obj->is_uf_triggered[cpcbID])
                        CLEAR_2DDMA(vpp_obj->plane[PLANE_MAIN].dmaRdhubID,
                                plane->dmaRID[0], (VIP_BCMBUF*)vpp_obj->pVbiClearBcmBuf);
                    FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_0, plane->srcfmt, 1);
#ifdef ENABLE_OVP
                    if (!pinfo->m_is_frame_seq){
                        FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_2);
                        if(vpp_obj->is_uf_triggered[cpcbID])
                            CLEAR_2DDMA(vpp_obj->plane[PLANE_MAIN].dmaRdhubID,
                                    plane->dmaRID[2], (VIP_BCMBUF *)vpp_obj->pVbiClearBcmBuf);
                        FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_2, plane->srcfmt, 1);
                    }
#endif/*#ifdef ENABLE_OVP */
                    if(plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                            plane->srcfmt == SRCFMT_YUV422SP_DWA
#ifdef CONFIG_AMP_VPP_ROTATION
                            || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
#endif
                      )
                    {
                        FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_1);
                        if(vpp_obj->is_uf_triggered[cpcbID])
                            CLEAR_2DDMA(vpp_obj->plane[PLANE_MAIN].dmaRdhubID,
                                    plane->dmaRID[1], (VIP_BCMBUF*)vpp_obj->pVbiClearBcmBuf);
                        FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_1, plane->srcfmt, 1);
#ifdef ENABLE_OVP
                        if (!pinfo->m_is_frame_seq)
                        {
                            FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_3);
                            if(vpp_obj->is_uf_triggered[cpcbID])
                                CLEAR_2DDMA(vpp_obj->plane[PLANE_MAIN].dmaRdhubID,
                                        plane->dmaRID[3],(VIP_BCMBUF*)vpp_obj->pVbiClearBcmBuf);
                            FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_MAIN_3, plane->srcfmt, 1);
                        }
#endif
                    }
                }
                else
                {
                    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_0);
                    if(vpp_obj->is_uf_triggered[cpcbID])
                        CLEAR_2DDMA(vpp_obj->plane[PLANE_PIP].dmaRdhubID,
                                plane->dmaRID[0], (VIP_BCMBUF*)vpp_obj->pVbiClearBcmBuf);
                    FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_0, plane->srcfmt, 1);
                    switch(plane->srcfmt)
                    {
                        case SRCFMT_YUV420SP_TILED_MODE0:
                        case SRCFMT_YUV420SP_TILED_MODE1:
                        case SRCFMT_YUV420SP_TILED_V4H8:
                            FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_EL_PIPE);
                            FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_EL_PIPE, plane->srcfmt, 1);
                            /*Fall Through */
                        case SRCFMT_YUV420SP:
                        case SRCFMT_YUV420SP_DWA:
                        case SRCFMT_YUV422SP_DWA:
                            FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_1);
                            if(vpp_obj->is_uf_triggered[cpcbID])
                                CLEAR_2DDMA(vpp_obj->plane[PLANE_PIP].dmaRdhubID,
                                        plane->dmaRID[1], (VIP_BCMBUF*)vpp_obj->pVbiClearBcmBuf);
                            FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_PIP_1, plane->srcfmt, 1);
                            break;
                        default:
                            /* Do nothing */
                            break;
                    }
                }
            }
            else if (planeID == PLANE_GFX1) {
                FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_GFX1_0);
                if(vpp_obj->is_uf_triggered[cpcbID])
                    CLEAR_2DDMA(vpp_obj->plane[PLANE_GFX1].dmaRdhubID,
                            plane->dmaRID[0], (VIP_BCMBUF*)vpp_obj->pVbiClearBcmBuf);
                FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_GFX1_0, plane->srcfmt, 1);
            } // <- start data loader of each none OSD/AUX plane
#ifdef ENABLE_GFX2_VOP2_PATH
            else if (planeID == PLANE_GFX2) {
                FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_GFX2_0);
                if(vpp_obj->is_uf_triggered[cpcbID])
                    CLEAR_2DDMA(vpp_obj->plane[PLANE_GFX2].dmaRdhubID,
                            plane->dmaRID[0], (VIP_BCMBUF*)vpp_obj->pVbiClearBcmBuf);
                FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_GFX2_0, plane->srcfmt, 1);
            }
#endif
        }//STATUS_DISP_PATGEN
        if (orgRefWidth > 0) {
            plane->ref_win.width = orgRefWidth;
        }
    }// <- start data loader of each none OSD/AUX plane
    VDBG_IF_DISP_ENABLE(0);
    return (MV_VPP_OK);
}

#ifdef VPP_HANDLE_SPURIOUS_INTERRUPTS
/*This function will hand spurious interrups till atleast one plane have valid data cpcbID.
TODO: Here it is assumed like planes are open one after the another in different interrupts. */
static void vpp_HandleSpuriousInterrups(VPP_OBJ *vpp_obj, int cpcbID)
{
    int i = 0;
    int chanID = -1, planeID = -1;
    DV *pDV = NULL;
    PLANE *plane = NULL;
    int params[3];

    if(1 != vpp_obj->force_uf)
    {
        /* Since this is not the starting of first plane, we don't needs to take care anything */
        return MV_VPP_OK;
    }

    pDV = &(vpp_obj->dv[cpcbID]);

    for (i=0; i<pDV->num_of_chans; i++)
    {
        chanID = pDV->chanID[i];
        if (vpp_obj->chan[chanID].status == STATUS_ACTIVE)
        {
            planeID = PLANE_OF_CHAN(vpp_obj, chanID);
            plane = &vpp_obj->plane[planeID];
            if(NULL == plane->prev_curr_frame)
            {
                /* Mute plane and trigger underflow till a valid frame completed exections */
                params[0] = planeID;
                params[1] = cpcbID;
                params[2] = 1;
                //VPP_SetPlaneMute(vpp_obj,params);
                vpp_obj->is_uf_triggered[cpcbID] = 0;
            }
            else
            {
                /* Unmute plane as a valid plane comes */
                params[0] = planeID;
                params[1] = cpcbID;
                params[2] = 0;
                VPP_SetPlaneMute(vpp_obj,params);
                vpp_obj->force_uf = 0;
                break;
            }
        }
    }
}
#endif /*#ifdef VPP_HANDLE_SPURIOUS_INTERRUPTS */

/* interrupt service routine for CPCB TG interrupt */
void VPP_CPCB_ISR_service(VPP_OBJ *vpp_obj, int cpcbID)
{
    int i, cmdq_empty;
    int chanID, planeID;
    DV *pDV;
    CMD cmd;
    PLANE *main_plane;
    PLANE *plane;
    int update_cmd = 1;
    RESOLUTION_INFO  *pResinfo;
    UINT8 m_uchDvCompMode;

    pDV = &(vpp_obj->dv[cpcbID]);
    pDV->vbi_num ++; // VBI counter

    /* if VPP CPCB TG is inactive, ignore interrupts */
    if ((pDV->status == STATUS_INACTIVE) || (pDV->vbi_num<=2)){
        return;
    }

    pResinfo = VPP_GetResInfo(VPP_GetResId(cpcbID));

    if(cpcbID == CPCB_1)
    {
        /* no VPP settting update in main interlaced bottom field input */
        main_plane = &(vpp_obj->plane[PLANE_MAIN]);

        if(CPCB_OF_CHAN(vpp_obj, CHAN_MAIN) == cpcbID)
        {
#if defined(PE_3D_BD)
            RESOLUTION_INFO  *pResinfo;
            pResinfo = VPP_GetResInfo(VPP_GetResId(cpcbID));

            if(pResinfo->flag_3d)
            {
                if(vpp_obj->dv[cpcbID].viewID != 0) update_cmd = 0;
            } else
#endif
#if defined(PULLUP_ENABLE)
            if(main_plane->frame_type == 0 && main_plane->field != 0) update_cmd = 0;
#else
            if(main_plane->frame_type == 0 && main_plane->field == 1) update_cmd = 0;
#endif
        }
    }

    VOP2_TG_Start(vpp_obj, cpcbID, 0);
#ifdef VPP_UNDERFLOW_FIX
    VOP_TG_Enable(vpp_obj, cpcbID, 0);
#endif
    if(update_cmd) {
        /* fetch & run commands received from CPCB command queue */
        while ((cmdq_empty = cmdq_pop(&(pDV->cmdq), &cmd))){
            if (cmd.func){ /* command is valid */
                /* execute command */
                cmd.func(vpp_obj, cmd.params);
                /* invalid command after exectution */
                cmd.func = 0;
            }
        }
    }

    /*DP1 DTG underflow will not be considered for DP1->DP2 cases*/
    VPP_DTGUF_Handler(vpp_obj, cpcbID);

    /*when change cpcb output resolution, status was set to inactive,
      Dhub should be stopped at this moment*/
    if(vpp_obj->dv[cpcbID].status == STATUS_ACTIVE){
        /* go through all active channels belongs to this CPCB, update scaler
           parameter and start plane DMA */
#ifdef VPP_HANDLE_SPURIOUS_INTERRUPTS
        /* Bugfix: Without this spurious interrupts time(ie. begining of first plane)
           we will see scrolling display*/
        vpp_HandleSpuriousInterrups(vpp_obj, cpcbID);
#endif /*#ifdef VPP_HANDLE_SPURIOUS_INTERRUPTS */

#ifndef VPP_UNDERFLOW_FIX
        if(cpcbID == CPCB_1){
            FE_DEINT_VpTG_Enable(vpp_obj, 1);
            if(vpp_obj->m_uchDvTestPathMode)
            {
                VOP_TG_Enable(vpp_obj);
                CPCB_TG_Enable(vpp_obj, cpcbID, 1); //betg start
            }
        }
#endif
        for(i=0; i<pDV->num_of_chans; i++) {
            chanID = pDV->chanID[i];
            /*check whether channel is active */
            if(vpp_obj->chan[chanID].status == STATUS_ACTIVE) {
                planeID = PLANE_OF_CHAN(vpp_obj, chanID);
                plane = &vpp_obj->plane[planeID];
                if ((!pResinfo->flag_3d && (plane->field == 0))
#if defined(PE_3D_BD)
                      || (pResinfo->flag_3d && (vpp_obj->dv[cpcbID].viewID == 0) && (plane->field == 0))
#endif
                  ) {
                    /* top field required */
                    /* top field is used to fetch a new frame(two fields for interlaced input)
                       for display */
                    /* update current frame for active planes in DISP_VIDEO status of this channel */
                    updateFEChannelCurrentFrame(vpp_obj, chanID);
                }
            }
        }
        //Give a chance to mute plane based on SA entries and the current frame
        VPP_ENV_SA_GetAttributes(vpp_obj);

        if(vpp_obj->uiAviomtrKickoff && (cpcbID == CPCB_1))
            VPP_AVIOMTR_kickOff(vpp_obj, VPP_AVIOMTR_CONFIG_VPP);

        VOP2_TG_Start(vpp_obj, cpcbID, 1);

        if(cpcbID == CPCB_1) {
            VPP_BE_HDR_SetOutputMode(vpp_obj, pDV);
            VPP_DV_BE_GfxHandler(vpp_obj, pDV);
            vpp_obj->bIsHDROutput = VPP_BE_IsHDROutput(vpp_obj);
        }

        for (i=0; i<pDV->num_of_chans; i++) {
            chanID = pDV->chanID[i];
            /*check whether channel is active */
            if (vpp_obj->chan[chanID].status == STATUS_ACTIVE) {
                /* start channel data loaders to fetch data of active planes */
                startChannelDataLoader(vpp_obj, chanID);
            }
        }

        if(cpcbID == CPCB_1) {
            VPP_DV_ControlPath(vpp_obj, pDV);
            VPP_DV_EdrHandler(vpp_obj);
        }

#ifdef VPP_UNDERFLOW_FIX
        if(cpcbID == CPCB_1) {
            VOP_TG_Enable(vpp_obj, cpcbID, 1);
            CPCB_TG_Enable(vpp_obj, cpcbID, 1); //betg start
            m_uchDvCompMode = VPP_DV_GetDvCompMode(vpp_obj);
            if(vpp_obj->plane[PLANE_MAIN].status == STATUS_DISP_VIDEO &&
                    (0 == m_uchDvCompMode)) {
                FE_DEINT_VpTG_Enable(vpp_obj, 1);
            }
            else {
                FE_DEINT_VpTG_Enable(vpp_obj, 0);
            }
        }
#else
        if((!vpp_obj->m_uchDvTestPathMode) && (cpcbID == CPCB_1)) {
            CPCB_TG_Enable(vpp_obj, cpcbID, 1); //betg start
            VOP_TG_Enable(vpp_obj);
        }
#endif
    }

    /* Change the uiResID only after processing all the planes
       (we have only one CPCB0, so no checking required)
       so that scalar can be applied to all enabled planes */
    if(uiIsrLocalResID[cpcbID] != vpp_obj->uiResID[cpcbID])
    {
        uiIsrLocalResID[cpcbID] = vpp_obj->uiResID[cpcbID];
        //Reset the canvas change flag after the 1st iteration - SCL & FRAME is prepared
        vpp_obj->resIdUpdated[cpcbID] = 0;
    }


#if defined(PE_3D_BD)
    /* update view ID in 3D case */
    if (pResinfo->flag_3d) {
        if (vpp_obj->dv[cpcbID].viewID == 0) { // left view
            BE_VOP_SetHdmiFrmtViewIDContinuous(vpp_obj, 1);
            vpp_obj->dv[cpcbID].viewID = 1;
        } else {// right view
            BE_VOP_SetHdmiFrmtViewIDContinuous(vpp_obj, 0);
            vpp_obj->dv[cpcbID].viewID = 0;
        }
    }
#endif

#ifdef ENABLE_GFX2_VOP2_PATH
    if (cpcbID == CPCB_OF_VOUT(vpp_obj, VOUT_DSI)) {
        //TODO:Include DP2 pipe related settings here
    }
#endif
    return;
}

/*************************************************************
 * FUNCTION: register VPP interrupt service routine
 * PARAMS: cpcbID - CPCB ID
 * RETURN: none
 *************************************************************/
void VPP_Enable_ISR_Interrupt(VPP_OBJ *vpp_obj, int cpcbID)
{
    VPP_INTR_MSG intr_info = {0, 0};
    INT cpcb_intr;
    intr_info.Enable = 1;

    if (cpcbID == CPCB_1){
        cpcb_intr = avioDhubSemMap_vpp_vppCPCB0_intr;
    }else if(cpcbID == CPCB_2){
        cpcb_intr = vpp_vop2_DTG_frst_intr;
    }else{
        return;
    }
    intr_info.DhubSemMap = cpcb_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
    VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
    /* configure and enable CPCBx interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, cpcb_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, cpcb_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, cpcb_intr,
            0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);

    return;
}

/*************************************************************
 * FUNCTION: register VPP interrupt service routine
 * PARAMS: cpcbID - CPCB ID
 * RETURN: none
 *************************************************************/
void VPP_Enable_VDE_Interrupt(VPP_OBJ *vpp_obj, int cpcbID)
{
    VPP_INTR_MSG intr_info = {0, 0};
    intr_info.Enable = 1;

    if (cpcbID == CPCB_1){
        intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT4_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
        ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
        VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
        semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1, 0);
        semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);
        semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr,
                0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);
        vpp_obj->vde_int_enable[CPCB_1] = 1;
    }

    return;
}

void VPP_Disable_VDE_Interrupt(VPP_OBJ *vpp_obj, int cpcbID)
{
    VPP_INTR_MSG intr_info = {0, 0};
    intr_info.Enable = 0;

    if (cpcbID == CPCB_1){
        semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1, 0);
        semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);
        semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr,
                0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);
        intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT4_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
        ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
        VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
        vpp_obj->vde_int_enable[CPCB_1] = 0;
    }

    return;
}

/**********************************/
/* entry of VPP interrupt service */
/**********************************/

/*************************************************************
 * FUNCTION: register VPP interrupt service routine
 * PARAMS: none
 * RETURN: none
 *************************************************************/
void VPP_ISR_Registration(VPP_OBJ *vpp_obj)
{
    VPP_INTR_MSG intr_info = {0, 0};
    intr_info.Enable = 1;
#ifndef TZ_3_0
    vpp_obj->pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
#endif

    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT3_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
    VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
    /* configure and enable VOUT1 interrupt - HPD Interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr,
            0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);

    intr_info.DhubSemMap = avioDhubSemMap_vpp_CH10_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
    VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
    /* configure and enable Dhub channel #10 interrupt - HDMI audio Dhub channel Interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr,
            0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);

    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT0_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
    VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
    /* configure and enable VOUT0 interrupt - HDCP Interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr,
            0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);

#ifdef VPP_TIME_PROFILE
    ResetCycleCounter();
    EnableCycleCounter(1);
#endif

    return;
}

void VPP_CEC_ISR_Registration(VPP_OBJ *vpp_obj)
{
#ifndef CONFIG_AMP_TEE_ENABLE
    if (berlin_cec_dev >= 0) {
        int cec_intr_en = TRUE;
        ioctl(berlin_cec_dev, CEC_IOCTL_INTR_MSG, &cec_intr_en);
    }
#else
    CEC_Enable_INT(TRUE);
#endif
}

void VPP_ISR_UnRegistration(VPP_OBJ *vpp_obj)
{
    VPP_INTR_MSG intr_info = {0, 0};
    intr_info.Enable = 0;

#ifndef TZ_3_0
    vpp_obj->pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
#endif

    /* configure and enable CPCB0 interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr,
            0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppCPCB0_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
    VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
    VPP_Disable_VDE_Interrupt(vpp_obj, CPCB_1);

    /* configure and enable VOUT1 interrupt - HPD Interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT3_intr,
            0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT3_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
    VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
    /* configure and enable Dhub channel #10 interrupt - HDMI audio Dhub channel Interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_CH10_intr,
            0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_CH10_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
    VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
    /* configure and enable VOUT0 interrupt - HDCP Interrupt */
    semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr, 1, 0);
    semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr);
    semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp_vppOUT0_intr,
            0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, CPU_ID/*cpu id*/);
    intr_info.DhubSemMap = avioDhubSemMap_vpp_vppOUT0_intr;
#ifndef CONFIG_AMP_TEE_ENABLE
    ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
    VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif

    return;
}

void VPP_CEC_ISR_UnRegistration(VPP_OBJ *vpp_obj)
{
#ifndef CONFIG_AMP_TEE_ENABLE
    if (berlin_cec_dev >= 0) {
        int cec_intr_en = FALSE;
        ioctl(berlin_cec_dev, CEC_IOCTL_INTR_MSG, &cec_intr_en);
    }
#else
    CEC_Enable_INT(FALSE);
#endif
}

#ifndef CONFIG_AMP_TEE_ENABLE
static HRESULT VPP_isr_get_message(VPP_OBJ *vpp_obj, MV_CC_MSG_t * pMsg)
{
    HRESULT rc = MV_VPP_OK;
#if defined(VPP_DIAGS)
    INT instat, instat_ag;
    HDL_semaphore *pSemHandle;
    HDL_semaphore *pSemHandle_AG;
    INT vpp_intr = 0;

    pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
    pSemHandle_AG = dhub_semaphore(&AG_dhubHandle.dhub);
    instat = semaphore_chk_full(pSemHandle, -1);
    instat_ag = semaphore_chk_full(pSemHandle_AG, -1);

    if (bTST(instat, avioDhubSemMap_vpp_vppCPCB0_intr)) {
        vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppCPCB0_intr);

        semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr, 1);
        semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppCPCB0_intr);
    }
    if (bTST(instat, avioDhubSemMap_vpp_vppOUT4_intr)) {
        vpp_intr |= bSETMASK(avioDhubSemMap_vpp_vppOUT4_intr);

        semaphore_pop(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr, 1);
        semaphore_clr_full(pSemHandle, avioDhubSemMap_vpp_vppOUT4_intr);
    }

    if (bTST(instat, vpp_vop2_DTG_frst_intr)) {
        vpp_intr |= bSETMASK(vpp_vop2_DTG_frst_intr);

        semaphore_pop(pSemHandle, vpp_vop2_DTG_frst_intr, 1);
        semaphore_clr_full(pSemHandle, vpp_vop2_DTG_frst_intr);

        semaphore_pop(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr14, 1);
        semaphore_clr_full(pSemHandle_AG, avioDhubSemMap_aio64b_aio_intr14);
    }
    if(vpp_intr){
        pMsg->m_MsgID = VPP_CC_MSG_TYPE_VPP;
        pMsg->m_Param1 = vpp_intr;
    }else{
        pMsg->m_MsgID = -1;
        pMsg->m_Param1 = 0;
        MV_OSAL_Task_Sleep(100); //sleep in polling mode
    }
#else
    rc = ioctl(berlin_pe_dev, VPP_IOCTL_TIMING, pMsg);
#ifdef BERLIN_BG2_BCM_SCHE_IN_KERNEL
    unsigned int t_start = 0, t_end = 0;
    GA_REG_WORD32_READ(0xf7e82C00+0x04 + 7*0x14, &t_start);
    while(1) {
        GA_REG_WORD32_READ(0xf7e82C00+0x04 + 7*0x14, &t_end);
        if ((t_end > t_start) && ((t_start - t_end + 0xFFFFFFFF) > 110000/5))
            break;
        if ((t_end < t_start) && ((t_start - t_end) > 110000/5))
            break;
    }
#endif
#endif
    return rc;
}
#endif

#ifndef CONFIG_AMP_TEE_ENABLE
static HRESULT CEC_isr_get_message(VPP_OBJ *vpp_obj, MV_CC_MSG_t * pMsg)
{
    HRESULT rc = MV_VPP_OK;
    if (berlin_cec_dev >= 0)
        rc = ioctl(berlin_cec_dev, CEC_IOCTL_GET_MSG, pMsg);
    return rc;
}
#endif

#ifdef CONFIG_AMP_TEE_ENABLE
void VPP_ISR_Handler(UINT32 msg_id, UINT32 msg_para)
#else
void VPP_ISR_TaskHandler(VPP_OBJ *vpp_obj)
#endif
{
#ifdef CONFIG_AMP_TEE_ENABLE
    VPP_OBJ *vpp_obj = g_vpp->vpp_objs[0];
#endif
    MV_CC_MSG_t msg;
    HRESULT rc = MV_VPP_OK;
    int instat;
    int cpcbID = CPCB_INVALID;
#ifdef VPP_ISR_PROFILE
    int vbi_interval = 0;
    int pts_per_cnt_4 = 0;
    UINT32 delta_time = 0;
    UINT32 end_time = 0;
    UINT32 start_time = 0;
    UINT32 last_start_time = 0;
#endif
    unsigned int EmptySts;

    {
#ifdef CONFIG_AMP_TEE_ENABLE
        msg.m_MsgID = msg_id;
        msg.m_Param1 = msg_para;
#else
        VPP_INTR_INFO intr;
        rc = VPP_isr_get_message(vpp_obj, &msg);
#endif

        if (vpp_obj->status != STATUS_ACTIVE)
            return;

        if (vpp_obj->suspend != POWER_MODE_NORMAL)
            MV_OSAL_Task_Sleep(100); // sleep in suspend mode.

        if (rc == MV_VPP_OK){

            if (VPP_CC_MSG_TYPE_VPP == msg.m_MsgID) {
                instat = msg.m_Param1;
                cpcbID = VBItoCPCB(msg.m_Param1, vpp_obj->m_VoutMode);

                VPP_DEBUG_UTIL_monitorVbiVdeIntrMiss(vpp_obj, instat);
#ifdef VPP_SINGLE_CPU_DBG
                VPPAPI_DBG_INFO("VPP ISR receive msg status=%x\n", msg.m_Param1);
#endif

                /* CPCB-0 VBI interrupt */
                if (bTST(instat, avioDhubSemMap_vpp_vppCPCB0_intr) ||
                        (bTST(instat, vpp_vop2_DTG_frst_intr) && (cpcbID == CPCB_1)))
                {
                    cpcbID = CPCB_1;
#ifdef VPP_ISR_PROFILE
                    MV_VPPOBJ_GetCPCBOutputFrameRate(0, cpcbID, &pts_per_cnt_4);
                    start_time = MV_Time_GetSysTimeMS();
                    if (last_start_time)
                    {
                        delta_time = start_time - last_start_time;
                        vbi_interval = pts_per_cnt_4 /(4 * 90);
                        if (delta_time > (vbi_interval + VPP_ISR_SCHED_TIMEOUT))
                        {
                            VPPAPI_DBG_INFO("vpp isr scheduled timeout, \
                                    delta time :%d ms, vbi interval :%d ms\n", \
                                    delta_time,vbi_interval);
                        }
                    }
                    last_start_time = start_time;
#endif
#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
                    VPPDRV_vppIntrFreqMonitorGen(vpp_obj, VPPDRV_INTR_MONITOR_UINT0_VBI_INTR,
                            VPPDRV_INTR_MONITOR_START, VPPDRV_INTR_MONITOR_NOSHOW);
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR

                    vpp_obj->dv[CPCB_1].vbi_num ++;

#ifndef BERLIN_BG2_BCM_SCHE_IN_KERNEL
                    BCM_SCHED_GetEmptySts(0, &EmptySts);

                    if (EmptySts == 0) {
                        vpp_obj->dv[cpcbID].skip_vde_int = 1;
                        return;
                    } else {
                        vpp_obj->dv[cpcbID].skip_vde_int = 0;
                    }
#endif

#if !defined(CONFIG_AMP_TEE_ENABLE)
                    intr.uiCpcbID = cpcbID;
                    intr.uiDisplayMode = vpp_obj->m_VoutMode;
                    disp_dsr_service(&intr);
#endif
                    cpcbID = CPCB_1;

#ifdef VPP_SINGLE_CPU_TIMING_PROFILE
                    t_start = MV_Time_GetTIMER7();
#endif
                    /* current VBI BCM cfgQ  */
                    VPP_Reset_CFGQ(vpp_obj, cpcbID);

                    MV_OSAL_Mutex_Lock(vpp_obj->bcmbuf_lock);

                    /* select BCM buffer for the CPCB */
                    VPP_Select_Buffer(vpp_obj, cpcbID);

                    //FIXME: need to move to proper location
                    CPCB_DISPTG_ToggleField(vpp_obj, cpcbID);

#if !defined(CONFIG_AMP_TEE_ENABLE)
                    disp_isr_start_service(&intr);
#endif
                    VPP_VMXAPI_Notify(vpp_obj, MV_VPP_VMX_CALLBACK_ISR, NULL);
                    /* call CPCB-0 interrupt service routine */
#if !defined(CONFIG_AMP_TEE_ENABLE)
                    if (!pDVResinfo->flag_3d
                        || (pDVResinfo->output_res].flag_3d
                        && (vpp_obj->dv[cpcbID].viewID == 0))) {
                            clk_isr_service(&intr);
                    }
#endif
                    VPP_CPCB_ISR_service(vpp_obj, cpcbID);

                    if ((vpp_obj->be.hdmiFrmIntrTrig) && (vpp_obj->vout[VOUT_HDMI].dvID == CPCB_1))
                        // Hdmi needs to be informed of the frame interrupt
                        VPP_BE_HDMIProcessFrameInt(vpp_obj);

                    //Check the validaty of the BCM BUFFER
                    VPP_DEBUG_UTIL_checkBcmBuffer(vpp_obj, vpp_obj->pVbiClearBcmBuf,
                            "vbiClear_bcm", VPP_DEBUG_UTIL_BCMBUFF_DUMP_DISABLE);

                    /* generate CPCB-0 CLEAR BCM DHUB cmdQ and,*/
                    rc = VPP_BCMBUF_To_CFGQ(vpp_obj->pVbiClearBcmBuf, vpp_obj->curr_vbi_bcm_cfgQ);
                    if (rc == MV_VPP_OK) {
                        /* switch CLEAR BCM buffer for the CPCB */
                        VPP_Switch_Buffer(vpp_obj, TRUE, cpcbID);
                    }
                    VPP_SCHED_BCMBUF_To_CFGQ(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, \
                            vpp_obj->curr_vbi_bcm_cfgQ);

                    //Check the validaty of the BCM BUFFER
                    VPP_DEBUG_UTIL_checkBcmBuffer(vpp_obj, vpp_obj->pVbiBcmBuf,
                            "vbiSet_bcm", VPP_DEBUG_UTIL_BCMBUFF_DUMP_DISABLE);

                    /* generate CPCB-0 BCM DHUB cmdQ and,*/
                    rc = VPP_BCMBUF_To_CFGQ(vpp_obj->pVbiBcmBuf, vpp_obj->curr_vbi_bcm_cfgQ);

                    VPP_SCHED_BCMBUF_To_CFGQ(vpp_obj->ovp_obj.pOvpSclBcmBuf,
                            vpp_obj->curr_vbi_bcm_cfgQ);

                    VPP_CFGQ_To_CFGQ(vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ,
                            vpp_obj->curr_vbi_bcm_cfgQ);

                    VPP_SCHED_CFGQ_To_CFGQ(vpp_obj->ovp_obj.curr_ovpscl_dma_cfgQ, \
                            vpp_obj->curr_vbi_bcm_cfgQ);

                    MV_OSAL_Mutex_Unlock(vpp_obj->bcmbuf_lock);

                    if (rc == MV_VPP_OK) {
                        /* switch BCM buffer for the CPCB */
                        VPP_Switch_Buffer(vpp_obj, FALSE, cpcbID);
                    }

#ifdef VPP_SINGLE_CPU_TIMING_PROFILE
                    t_start = MV_Time_GetTIMER7();
                    GA_REG_WORD32_READ(0xf7e82C00+0x04 + 7*0x14, &t_start);
                    t_start = DEBUG_TIMER_VALUE - t_start;
#endif

#ifdef ENABLE_GFX2_VOP2_PATH
                    /* commit CPCB-0 BCM DHUB cfgQ to CPCB-0 VBI interrupt */
                    if(IS_VOUT_DISP_MODE(SINGLE_MODE_SEC)) {
                        VPP_BCMDHUB_CFGQ_Commit(vpp_obj->curr_vbi_bcm_cfgQ, CPCB_2, 0/*VBI cfgQ*/);
                        //To disable autopush dyanamically, change resolution is required
                        VPP_AutoPush_Handler(vpp_obj, CPCB_2, 0);
                    } else
#endif
                    {
                        VPP_BCMDHUB_CFGQ_Commit(vpp_obj->curr_vbi_bcm_cfgQ, cpcbID, 0/*VBI cfgQ*/);
                        //To disable autopush dyanamically, change resolution is required
                        VPP_AutoPush_Handler(vpp_obj, cpcbID, 0);
                    }

#ifdef VPP_SINGLE_CPU_TIMING_PROFILE
                    t_end = MV_Time_GetTIMER7();

                    GA_REG_WORD32_READ(0xf7e82C00+0x04 + 7*0x14, &t_end);
                    t_end = DEBUG_TIMER_VALUE - t_end;

                    if (t_end - t_start > 75000)
                    {
                        FDM_isr_trace(1, "ISR cpcb0 start: %x  p2: %x  delay: %.02f ms \n",
                                t_start, t_end, (float)(t_end - t_start)/75000);
                    }
#endif
                    /*Switch the bcm dhub queue*/
                    VPP_Switch_CFGQ(vpp_obj, cpcbID);

#if !defined(CONFIG_AMP_TEE_ENABLE)
                    disp_isr_end_service(&intr);
#endif

#ifdef VPP_ISR_PROFILE
                    MV_VPPOBJ_GetCPCBOutputFrameRate(0, cpcbID, &pts_per_cnt_4);
                    end_time = MV_Time_GetSysTimeMS();
                    delta_time = end_time - start_time;
                    vbi_interval = pts_per_cnt_4 /(4 * 90);

                    if (delta_time > (vbi_interval + VPP_ISR_SCHED_TIMEOUT))
                    {
                        VPPAPI_DBG_INFO("vpp isr takes too much time, \
                                delta time: %d ms, vbi interval :%d ms!\n", \
                                delta_time, vbi_interval);
                    }
#endif
#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
                    VPPDRV_vppIntrFreqMonitorGen(vpp_obj, VPPDRV_INTR_MONITOR_UINT0_VBI_INTR,
                            VPPDRV_INTR_MONITOR_STOP, VPPDRV_INTR_MONITOR_NOSHOW);
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR
                }

                /* CPCB-0 VDE interrupt */
                if (bTST(instat, avioDhubSemMap_vpp_vppOUT4_intr)){
                    cpcbID = CPCB_1;
#ifndef BERLIN_BG2_BCM_SCHE_IN_KERNEL
                    if (vpp_obj->dv[cpcbID].skip_vde_int == 1)
                        return;
#endif
                    /* call CPCB-0 VDE interrupt service routine */
                    if (vpp_obj->vde_int_enable[cpcbID]){

                        /* clear vde_dhub_cfgQ_len */
                        vpp_obj->dv[cpcbID].curr_cpcb_vde_bcm_cfgQ->len = 0;

                        //Check the validaty of the BCM BUFFER
                        VPP_DEBUG_UTIL_checkBcmBuffer(vpp_obj, vpp_obj->pVdeBcmBuf[CPCB_1],
                                "vde_bcm", VPP_DEBUG_UTIL_BCMBUFF_DUMP_DISABLE);

                        /* generate CPCB-0 VDE BCM DHUB cmdQ and,*/
                        VPP_BCMBUF_To_CFGQ(vpp_obj->pVdeBcmBuf[cpcbID],
                                vpp_obj->dv[cpcbID].curr_cpcb_vde_bcm_cfgQ);
                        /* commit CPCB-0 VDE BCM DHUB cfgQ to CPCB-0 VDE interrupt */
                        VPP_BCMDHUB_CFGQ_Commit(vpp_obj->dv[cpcbID].curr_cpcb_vde_bcm_cfgQ,
                                cpcbID, 1/*VDE cfgQ*/);
                        /* reset VDE BCMBUF */
                        VPP_BCMBUF_Reset(vpp_obj->pVdeBcmBuf[cpcbID]);
                    }

                    /* switch BCM buffer for the CPCB */
                    if (vpp_obj->pVdeBcmBuf[cpcbID] == &vpp_obj->vde_bcm_buf[cpcbID][0])
                        vpp_obj->pVdeBcmBuf[cpcbID] = &vpp_obj->vde_bcm_buf[cpcbID][1];
                    else
                        vpp_obj->pVdeBcmBuf[cpcbID] = &vpp_obj->vde_bcm_buf[cpcbID][0];

                    if (vpp_obj->dv[cpcbID].curr_cpcb_vde_bcm_cfgQ ==
                            &vpp_obj->dv[cpcbID].vde_bcm_cfgQ[0])
                        vpp_obj->dv[cpcbID].curr_cpcb_vde_bcm_cfgQ =
                            &(vpp_obj->dv[cpcbID].vde_bcm_cfgQ[1]);
                    else
                        vpp_obj->dv[cpcbID].curr_cpcb_vde_bcm_cfgQ =
                            &(vpp_obj->dv[cpcbID].vde_bcm_cfgQ[0]);
                }
                if (bTST(instat, avioDhubSemMap_vpp_vppOUT3_intr)){
                }
                /* HDMI audio interrupt */
                if (bTST(instat, avioDhubSemMap_vpp_CH10_intr)){
                }

                /* VOUT0 interrupt */
                if (bTST(instat, avioDhubSemMap_vpp_vppOUT0_intr)){
                    /* call HDCP interrupt service routine */
                    switch(VPP_BE_GetHdmiIrqSource(vpp_obj)) {
                        case VPP_HDMI_IRQ_HPD:
                            hpd_debounce_time = GetCurTimeInMSec();
                            break;
                        case VPP_HDMI_IRQ_HDCP:
                            VPP_BE_HDCP_ISR_service(vpp_obj);
                            VPP_BE_HDMI_DSR_service(vpp_obj);
                            break;
                        default:
                            //error case
                            break;
                    }
                }

                /*No HDMITX during SINGLE_MODE_SEC*/
                if (bTST(instat, avioDhubSemMap_vpp_vppCPCB0_intr) &&
                        (vpp_obj->m_VoutMode != VOUT_DISP_SINGLE_MODE_SEC)){
                    VPP_BE_SET_BIT(vpp_obj->be.cpcb0IntSts, VPP_BE_CPCB0_INTR);
                    vpp_obj->disp_dsr_count++;
                    if (hpd_debounce_time) {
                        UINT32 cur_time = GetCurTimeInMSec();
                        UINT32 diff = (cur_time - hpd_debounce_time);
                        if (diff >= HPD_STABLE_CHECK_TIME) {
                            hpd_debounce_time = 0;
                            VPP_BE_HPD_ISR_service(vpp_obj);
                            VPP_BE_HDMI_DSR_service(vpp_obj);
                        }
                    }
                }
                /* VOP2 DTG FRST interrupt for DUAL_MODE_X*/
                if ((vpp_obj->m_VoutMode > VOUT_DISP_SINGLE_MODE_SEC) &&
                        (bTST(instat, vpp_vop2_DTG_frst_intr))){
                    /* current VBI BCM cfgQ  */
                    VPP_Reset_CFGQ(vpp_obj, cpcbID);

                    MV_OSAL_Mutex_Lock(vpp_obj->bcmbuf_lock);

                    /* select BCM buffer for the CPCB */
                    VPP_Select_Buffer(vpp_obj, cpcbID);

                    /* call CPCB-0 interrupt service routine */
                    VPP_CPCB_ISR_service(vpp_obj, cpcbID);

                    /* generate CPCB-0 CLEAR BCM DHUB cmdQ and,*/
                    rc = VPP_BCMBUF_To_CFGQ(vpp_obj->pVbiClearBcmBuf, vpp_obj->curr_vbi_bcm_cfgQ);
                    if (rc == MV_VPP_OK) {
                        /* switch CLEAR BCM buffer for the CPCB */
                        VPP_Switch_Buffer(vpp_obj, TRUE, cpcbID);
                    }
                    /* generate CPCB-0 BCM DHUB cmdQ and,*/
                    rc = VPP_BCMBUF_To_CFGQ(vpp_obj->pVbiBcmBuf, vpp_obj->curr_vbi_bcm_cfgQ);

                    VPP_CFGQ_To_CFGQ(vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ, \
                            vpp_obj->curr_vbi_bcm_cfgQ);

                    MV_OSAL_Mutex_Unlock(vpp_obj->bcmbuf_lock);

                    if (rc == MV_VPP_OK) {
                        /* switch BCM buffer for the CPCB */
                        VPP_Switch_Buffer(vpp_obj, FALSE, cpcbID);
                    }
                    /* commit CPCB-0 BCM DHUB cfgQ to CPCB-0 VBI interrupt */
                    VPP_BCMDHUB_CFGQ_Commit(vpp_obj->curr_vbi_bcm_cfgQ, cpcbID, 0/*VBI cfgQ*/);

                    VPP_AutoPush_Handler(vpp_obj, cpcbID, 0);

                    VPP_Switch_CFGQ(vpp_obj, cpcbID);
                }
            }

#ifdef VPP_SINGLE_CPU_TIMING_PROFILE
            GA_REG_WORD32_READ(0xf7e82C00+0x04 + 7*0x14, &end_sched_ts);
            end_sched_ts = DEBUG_TIMER_VALUE - end_sched_ts;

            if (end_sched_ts - last_sched_ts > 75000)
            {
                //FDM_isr_trace(1, "ISR task start: %8x end:%8x delay: %.02f ms \n",
                //last_sched_ts, end_sched_ts, (float)(end_sched_ts - last_sched_ts)/75000);
            }
#endif
        }
        else
        {
            return;
        }

        if(vpp_obj->vout[VOUT_HDMI].dvID == cpcbID) {
#ifndef CONFIG_AMP_TEE_ENABLE
            /* post semaphore to notify DSR task */
            MV_OSAL_Sem_Post(vpp_obj->vpp_dsr_sem);
#else
            VPP_DSR_Task(vpp_obj);
#endif
        }
    }
    MV_OSAL_Task_Exit(0);
}

#ifdef CONFIG_AMP_TEE_ENABLE
void CEC_ISR_Handler(UINT32 msg_id, UINT32 msg_para)
#else
void CEC_ISR_TaskHandler(VPP_OBJ *vpp_obj)
#endif
{
#ifdef CONFIG_AMP_TEE_ENABLE
    VPP_OBJ *vpp_obj = g_vpp->vpp_objs[0];
#endif
    MV_CC_MSG_t msg;
    HRESULT rc = MV_VPP_OK;
    int instat;

    {
#ifdef CONFIG_AMP_TEE_ENABLE
        msg.m_MsgID = msg_id;
        msg.m_Param1 = msg_para;
#else
        rc = CEC_isr_get_message(vpp_obj, &msg);
#endif

        if (vpp_obj->status != STATUS_ACTIVE)
            return;

        if (vpp_obj->suspend != POWER_MODE_NORMAL)
            MV_OSAL_Task_Sleep(100); // sleep in suspend mode.

        if (rc == MV_VPP_OK){

            if (VPP_CC_MSG_TYPE_CEC == msg.m_MsgID){
                instat = msg.m_Param1;
                /* call CEC interrupt service routine */
                if (vpp_obj->be.enableCec)
                {
                    VPP_BE_CEC_ISRService(vpp_obj,instat);
                    VPP_BE_CEC_DSR_Service (vpp_obj);
                }
            }
        }
        else
        {
            return;
        }

    }

    MV_OSAL_Task_Exit(0);
}

void VPP_DSR_Task(VPP_OBJ *vpp_obj)
{
#ifndef CONFIG_AMP_TEE_ENABLE
    prctl(PR_SET_NAME, "VPP_DSR_Task", 0, 0., 0);

    while(!vpp_obj->vpp_isr_need_exit)
#endif
    {
#ifndef CONFIG_AMP_TEE_ENABLE
        MV_OSAL_Sem_Wait(vpp_obj->vpp_dsr_sem);
#endif
        if (VPP_BE_GET_BIT(vpp_obj->be.cpcb0IntSts, VPP_BE_CPCB0_INTR))
        {
            if ( vpp_obj->hdmi_cpcb0_dsr_cb )
            {
                vpp_obj->hdmi_cpcb0_dsr_cb(0, (void *)vpp_obj, NULL);
            }
            //VPP_HDMI_CPCB0_Dsr_Service();
            VPP_BE_CLR_BIT(vpp_obj->be.cpcb0IntSts,VPP_BE_CPCB0_INTR);
        }

        if ((vpp_obj->be.enableCec) &&
                (VPP_BE_GET_BIT(vpp_obj->be.hdmiIntSts,VPP_BE_HDMI_CEC_INTR)))
        {
            VPP_BE_CEC_DSR_Service (vpp_obj);
            VPP_BE_CLR_BIT(vpp_obj->be.hdmiIntSts,VPP_BE_HDMI_CEC_INTR);
        }

        if (VPP_BE_GET_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_SIG_FIFO_FRMTRIG))
        {
            VPP_BE_HDMI_DSR_service (vpp_obj);
            VPP_BE_CLR_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_SIG_FIFO_FRMTRIG);
        }
    }
    MV_OSAL_Task_Exit(0);
}
