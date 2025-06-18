/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_APIFUNCS_C_

#include "vbuf.h"

#include "vpp_module.h"
#include "vpp_cfg.h"
#include "vpp_fe.h"
#include "vpp_fe_dlr.h"
#include "vpp_fe_deint.h"
#include "vpp_fe_patgen.h"
#include "vpp_scl.h"
#include "vpp_frc_scl.h"
#include "vpp_cpcb.h"
#include "vpp_cpcb_tg.h"
#include "vpp_csc_icsc.h"
#include "vpp_be.h"
#include "vpp_be_hdr.h"

#include "vpp_apifuncs.h"
#ifndef FPGA
#include "vpp_apifuncs_wrapper.h"
#endif
#include "vpp_cpcb_ovl.h"
#include "vpp_be_vop.h"
#include "vpp_be_hdmitx.h"
#include "vpp_dv_api.h"
#include "vpp_debug.h"

#include "Galois_memmap.h"

#include "vpp.h"
#include "ovps_api.h"

#ifdef CONFIG_AMP_TEE_ENABLE
#include "com_type.h"
#include "ta_log.h"
#include "vpp_cmd.h"
#include "tee_internal_api.h"
#endif

#include "vpp_vmx_apifuncs.h"
#if defined(ANDROID) || defined(LOGCAT)
#include <cutils/log.h>
#else /* ANDROID */
#define ALOGD printf
#endif/*Linux*/

#include "vpp_clkGate.h"
#include "vpp_intr_monitor.h"
#include "api_avio_dhub.h"
#include "vpp_cmu.h"
#include "vpp_env_ur.h"
#ifdef CONFIG_AMP_DCM
#include "vpp_fe_dcm_common.h"
#endif
#ifdef ENABLE_EECMU
#include "vpp_cmu_ftdc.h"
#include "vpp_cmu_ee.h"
#include "vpp_cmu_ace.h"
#endif
#ifdef ENABLE_DETEE
#include "vpp_gdetee.h"
#endif

#include "vpp_pq.h"

#define VPP_OBJ_DISPOUT_PARAM_UPDATE(VAR1, VAR2) cpcb->dispOutParams.VAR1 = VAR2

/**
 * CHAN to DVID Mapping
 * CHAN/MODE      0            1              2             3           4
 * MAIN         CPCB_1        CPCB_1         CPCB_1        CPCB_1      CPCB_1
 * PIP          CPCB_1        CPCB_1         CPCB_2        CPCB_1      CPCB_1
 * GFX1         CPCB_1        CPCB_1         CPCB_1        CPCB_2      CPCB_1
 * GFX2       CPCB_INVALID   CPCB_INVALID  CPCB_INVALID  CPCB_INVALID  CPCB_2
 */
static const int chan_to_cpcb[CHAN_AUX][VOUT_DISP_MODE_MAX] = {
    {CPCB_1,       CPCB_1,       CPCB_1,       CPCB_1,       CPCB_1}, /*CHAN_MAIN*/
    {CPCB_1,       CPCB_1,       CPCB_2,       CPCB_1,       CPCB_1}, /*CHAN_PIP*/
    {CPCB_1,       CPCB_1,       CPCB_1,       CPCB_2,       CPCB_1}, /*CHAN_GFX1*/
    {CPCB_INVALID, CPCB_INVALID, CPCB_INVALID, CPCB_INVALID, CPCB_INVALID} /*CHAN_GFX2*/
};

/**
 * VOUT to DVID Mapping
 * VOUT/MODE      0             1              2             3           4
 * HDMI          CPCB_1       CPCB_INVALID   CPCB_1       CPCB_1      CPCB_1
 * DSI           CPCB_INVALID CPCB_1         CPCB_2       CPCB_2      CPCB_2
 */
static const int vout_to_cpcb[MAX_NUM_VOUTS][VOUT_DISP_MODE_MAX] = {
    {CPCB_1,       CPCB_INVALID, CPCB_1,       CPCB_1,       CPCB_1},
    {CPCB_INVALID, CPCB_INVALID, CPCB_INVALID, CPCB_INVALID, CPCB_INVALID},
    {CPCB_INVALID, CPCB_INVALID, CPCB_INVALID, CPCB_INVALID, CPCB_INVALID},
    {CPCB_INVALID, CPCB_1,       CPCB_2,       CPCB_2,       CPCB_2}
};

static const int mode_to_chan[VOUT_DISP_MODE_MAX] = {
    MAX_NUM_CHANS, //all channels active
    MAX_NUM_CHANS, //all channels active
    CHAN_PIP,      //PIP channel active in CPCB2
    CHAN_GFX1,     //GFX1 channel active in CPCB2
    CHAN_GFX2};    //GFX2 channel active in CPCB2

#ifdef ENABLE_GFX2_VOP2_PATH
static const int mode_to_vout[VOUT_DISP_MODE_MAX] = {
    VPP_VOP_NO_INPUT,    /*overlay output on HDMI*/
    VPP_VOP_CPCB1_CPCB0, /*overlay output on MIPI*/
    VPP_VOP_CPCB1_GFX0,  /*PIP on MIPI*/
    VPP_VOP_CPCB1_GFX1,  /*GFX0 on MIPI*/
    VPP_VOP_CPCB1_GFX2   /*GFX1 on MIPI*/
};
#endif

int cropDispWindow(VPP_WIN *disp_win, RESOLUTION_INFO *resinfo, VPP_WIN *crop_disp_win, CHAN *chan)
{
    /* initialize cropped display window */
    crop_disp_win->x = disp_win->x;
    crop_disp_win->y = disp_win->y;
    crop_disp_win->width = disp_win->width;
    crop_disp_win->height = disp_win->height;

    /* horizontal cropping of display window */
    if (crop_disp_win->x < 0) {
        /* crop left side */
        crop_disp_win->width += crop_disp_win->x;
        crop_disp_win->x = 0;
    }
    if ((crop_disp_win->x + crop_disp_win->width) > resinfo->active_width) {
        /* crop right side */
        crop_disp_win->width -= (crop_disp_win->x + crop_disp_win->width - resinfo->active_width);
    }

    /* vertical cropping of display window */
    if (crop_disp_win->y < 0) {
        /* crop top side */
        crop_disp_win->height += crop_disp_win->y;
        crop_disp_win->y = 0;
    }
    if ((crop_disp_win->y + crop_disp_win->height) > resinfo->active_height) {
        /* crop bottom side */
        crop_disp_win->height -= (crop_disp_win->y + crop_disp_win->height - resinfo->active_height);
    }

    /* set background window with cropped display window */
    if ((crop_disp_win->width < 0) || (crop_disp_win->height < 0)) {
        crop_disp_win->x = 0;
        crop_disp_win->y = 0;
        crop_disp_win->width = 0;
        crop_disp_win->height = 0;
    }

    chan->crop_disp_win_final = chan->crop_disp_win;
    #ifdef VPP_ENABLE_INTERLACED_OUTPUT_SCALING_CROPING
        if(SCAN_INTERLACED == resinfo->scan) {
            chan->crop_disp_win_final.y = (int)(chan->crop_disp_win_final.y/2);
            chan->crop_disp_win_final.height  = (chan->crop_disp_win_final.height+1)/2;
        }
    #endif //VPP_ENABLE_INTERLACED_OUTPUT_SCALING_CROPING

    return (MV_VPP_OK);

}

#ifdef CONFIG_AMP_TEE_ENABLE
int AVPLL_GetVcoFreq(int hdmiMode, int freqIndex)
{
    uint32_t cb_cmdID = VPP_CB_GETVCOFREQ;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_OUTPUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];

    cb_params[0].value.a = hdmiMode;
    cb_params[0].value.b = freqIndex;
    cb_params[1].value.a = 0xdeadbeef;

    VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);

    return cb_params[1].value.a;
}

int AVPLL_SetVideoFreq(int avpllGroupId, int freqIndex, int hdmiMode, int frameRate, float overSampleRate, int chId)
{
    TEE_Result result;
    uint32_t cb_cmdID = VPP_CB_SETVIDEOFREQ;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_OUTPUT);

    TEE_Param cb_params[4];

    cb_params[0].value.a = avpllGroupId;
    cb_params[0].value.b = freqIndex;
    cb_params[1].value.a = hdmiMode;
    cb_params[1].value.b = frameRate;
    cb_params[2].value.a = (int)(overSampleRate * 100.0);
    cb_params[2].value.b = chId;
    cb_params[3].value.a = 0xdeadbeef;

    result = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);

    return result;
}

int AVPLL_GetChannelOutputFreq(int AVPLLIndex, int chID)
{
    uint32_t cb_cmdID = VPP_CB_GETOUTPUTFREQ;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_OUTPUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];

    cb_params[0].value.a = AVPLLIndex;
    cb_params[0].value.b = chID;
    cb_params[1].value.a = 0xdeadbeef;

    VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return cb_params[1].value.a;
}

int AVPLL_SetClockFreq(int avpllGroupId, int vco_freq_index, unsigned int target_freq, int chId)
{
    uint32_t cb_cmdID = VPP_CB_DIAG_SETCLKFREQ;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_OUTPUT,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];

    cb_params[0].value.a = avpllGroupId;
    cb_params[0].value.b = vco_freq_index;
    cb_params[1].value.a = target_freq;
    cb_params[1].value.b = chId;
    cb_params[2].value.a = 0xdeadbeef;

    VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);

    return cb_params[2].value.a;
}

void diag_clock_change_otherClk(unsigned int index, unsigned int pllSwitch, unsigned int pllSelect, unsigned int divider)
{
    uint32_t cb_cmdID = VPP_CB_DIAG_CHNGOTHRCLK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];

    cb_params[0].value.a = index;
    cb_params[0].value.b = pllSwitch;
    cb_params[1].value.a = pllSelect;
    cb_params[1].value.b = divider;

    VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
}

void VPP_Enable_INT(UINT32 DhubSemMap, UINT32 Enable)
{
    uint32_t cb_cmdID = VPP_CB_ENABLEINT;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];

    cb_params[0].value.a = DhubSemMap;
    cb_params[0].value.b = Enable;

    VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
}

void CEC_Enable_INT(UINT32 Enable)
{
    uint32_t cb_cmdID = CEC_CB_ENABLEINT;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];

    cb_params[0].value.a = Enable;

    VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
}

int VPP_Receive_CECData(VPP_CEC_RX_MSG_BUF *rx_buf)
{
    uint32_t cb_cmdID = CEC_CB_RDCEC_MSG;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
               TEE_PARAM_TYPE_VALUE_INOUT,
               TEE_PARAM_TYPE_VALUE_INOUT,
               TEE_PARAM_TYPE_VALUE_INOUT);
    uint32_t parama, paramb;
    TEE_Param cb_params[4];
    int buflen, index;

    memset(cb_params, 0, sizeof(cb_params));
    cb_params[0].value.b = 0xdeadbeef;

    VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    rx_buf->len = cb_params[2].value.a;
    buflen = 0;
    for(index = 0; index < 2; index++)
    {
        parama = cb_params[index].value.a;
        paramb = cb_params[index].value.b;
        rx_buf->buf[buflen + 0] = parama & 0xFF;
        rx_buf->buf[buflen + 1] = (parama >> 8) & 0xFF;
        rx_buf->buf[buflen + 2] = (parama >> 16)& 0xFF;
        rx_buf->buf[buflen + 3] = (parama >> 24)& 0xFF;
        rx_buf->buf[buflen + 4] = paramb & 0xFF;
        rx_buf->buf[buflen + 5] = (paramb >> 8) & 0xFF;
        rx_buf->buf[buflen + 6] = (paramb >> 16)& 0xFF;
        rx_buf->buf[buflen + 7] = (paramb >> 24)& 0xFF;
        buflen = buflen + 8;
    }
    return cb_params[3].value.b;
}

#endif

/*******************************************************************
 * FUNCTION: set CPCB background color
 * INPUT: cpcbID - id of the CPCB
 *           bgcolor - CPCB background color value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 * Note: this function has to be called before enabling a plane
 *       which belongs to that DV.
 *******************************************************************/
int VPP_SetCPCBBgColor(VPP_OBJ *vpp_obj, int *in_params)
{
    VPP_CPCB_SetBasePlaneColor(vpp_obj, in_params[0], in_params[1]);

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set CPCB(for Berlin) or DV(for Galois) output resolution
 * INPUT: cpcbID - CPCB(for Berlin) or DV(for Galois) id
 *        resID - id of DV output resolution
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 * Note: this function has to be called before enabling a plane
 *       which belongs to that DV.
 *******************************************************************/
int VPP_SetCPCBOutputResolution(VPP_OBJ *vpp_obj, int *in_params)
{
    int cpcbID = in_params[0];
    int resID = in_params[1];
    int vidFmt = in_params[2];
    int bit_depth = in_params[3];
    int pixel_repeat = in_params[4];
    int quiescent = in_params[5];
    int mode = in_params[6];
    int dispID = in_params[7];
    CHAN *chan;
    DV *cpcb;
    RESOLUTION_INFO *new_resinfo = NULL;
    int i, cpcbout;
    int x, y, width, height;
    VPP_CPCB0_OUTPUT_RES res_tot, res_act;
    VPP_WIN *disp_win = NULL;
    RESOLUTION_INFO  *pResinfo;

    cpcb = &(vpp_obj->dv[cpcbID]);
    pResinfo = new_resinfo = VPP_GetResInfo(resID);

    if(resID != RES_RESET){
        /* set FRC-SCL OSD CSC: RGB->YCbCr709 for HD, RGB->YCbCr601 for SD */
        if (new_resinfo->type == TYPE_HD)
            i = VPP_SCL_CSC_sRGB_TO_YUV_709; // HD
        else
            i = VPP_SCL_CSC_sRGB_TO_YUV_601; // SD

        if (vpp_obj->chan[CHAN_GFX1].dvID == cpcbID)
            FRC_SCL_SetOsdFmt(vpp_obj, VPP_FRC_SCL_OSD, i);
    }

    /* set CPCB resolution when CPCB is inactive */
    VPP_CPCB_SetOutputResolution(vpp_obj, cpcbID, resID, vidFmt);
    //TODO: Use static table to disable the CPCB
#ifdef ENABLE_GFX2_VOP2_PATH
    if(IS_CPCB_VOUT_DISP_MODE(cpcbID, SINGLE_MODE_PRI) && (resID != RES_RESET)) {
        /*Maitain same resID for single mode cases, to handle switching to dual output*/
        VPP_SetResId(CPCB_2, VPP_GetResId(cpcbID));
    }
    else if(IS_CPCB_VOUT_DISP_MODE(cpcbID, SINGLE_MODE_SEC)) {
        VPP_CPCB_SetOutputResolution(vpp_obj, CPCB_2, resID, vidFmt);
        if(resID == RES_RESET){
            vpp_obj->dv[CPCB_2].vbi_num = 0; // reset VBI counter
            vpp_obj->dv[CPCB_2].status = STATUS_INACTIVE;
            vpp_obj->dv[CPCB_2].output_res_updated = 0;
            /*Stop bcm autopush here*/
            vpp_obj->enable_vbibcm_autopush[CPCB_2] = FALSE;
            VPP_BCMDHUB_AutoPush(CPCB_2, 0, 0); // for vbi intr
        }
    }
#endif
    /*when reset cpcb, do not update disp_win*/
    if(resID == RES_RESET) {
        vpp_obj->dv[cpcbID].vbi_num = 0; // reset VBI counter
        vpp_obj->dv[cpcbID].status = STATUS_INACTIVE;
        vpp_obj->dv[cpcbID].output_res_updated = 0;
        /*Stop bcm autopush here*/
        vpp_obj->enable_vbibcm_autopush[cpcbID] = FALSE;
        VPP_BCMDHUB_AutoPush(cpcbID, 0, 0); // for vbi intr
        return (MV_VPP_OK);
    }

    /* update all video/graphics plane window in active state connected to this CPCB
     * Note: previous/current canvas for each plane may be different for each plane and depends on the
     *    - previous display mode (HDMI/DSI) - to which o/p the plane is connected &
     *    - respective res_id of that o/p(HDMI/DSI) - the res-id of the connected o/p
     */
    for(i = FIRST_CHAN; i < MAX_NUM_CHANS; i++) {
        PLANE *plane = &(vpp_obj->plane[i]);

        chan = &(vpp_obj->chan[i]);
        if((chan->dvID != cpcbID) || ((i > CHAN_GFX2) && (chan->status == STATUS_INACTIVE))) {
            continue;
        }

        if(RES_INVALID != cpcb->dispOutParams.uiResId) {
            /*Previous channel resolution*/
            RESOLUTION_INFO *old_resinfo = VPP_GetResInfo(cpcb->dispOutParams.uiResId);

            //Force crop calculation upon resolution change
            plane->ref_win_updated = 1;
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
            if(i != PLANE_MAIN) {
                plane->ref_win_user_init = 1;
            }
            disp_win = &chan->disp_win_user;
#else
            disp_win = &chan->disp_win;
#endif
            /* calculate channel display window in new end display window */
            if((mode >= VOUT_DISP_DUAL_MODE_PIP) && (cpcbID == CPCB_2)) {
                /*Make fullscreen, if single plane connected to secondary display*/
                x = 0;
                y = 0;
                width = new_resinfo->active_width;
                height = new_resinfo->active_height;
            }
            else {
                x = new_resinfo->active_width * disp_win->x/ old_resinfo->active_width;
                y = new_resinfo->active_height * disp_win->y /old_resinfo->active_height;
                width = new_resinfo->active_width * disp_win->width / old_resinfo->active_width;
                height = new_resinfo->active_height * disp_win->height / old_resinfo->active_height;
            }
            if ((disp_win->x != x)||
                    (disp_win->y != y)||
                    (disp_win->width != width)||
                    (disp_win->height != height)
#ifdef VPP_ENABLE_INTERLACED_OUTPUT_SCALING_CROPING
                    /*Force setDispWin for Interlaced output, switching back to progressive output*/
                    || (SCAN_INTERLACED == new_resinfo->scan) || (new_resinfo->scan != old_resinfo->scan)
#endif
               )
            {
                /* update channel display window */
                disp_win->x = x;
                disp_win->y = y;
                disp_win->width = width;
                disp_win->height = height;

#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
                /* In DV cert streams L5 metadat carries disp_win offsets,
                   so dispwin will be changed in ISR in that case */
                /* Keeping backup of user_disp_win so that when switched to
                   a non dolby/dolby stream without L5 metadata user setting will be intact */
                chan->disp_win_user_back = chan->disp_win_user;
                chan->disp_win_user_updated = 1; /* channel display window is update */
#else
                chan->disp_win_updated = 1; /* channel display window is update */
#endif
                /* crop display window */
                cropDispWindow(disp_win, new_resinfo, &(chan->crop_disp_win), chan);
            }
        }
    }

    /*set BE fifo, if aux channel is connected to this cpcb*/
    if (CPCB_1 == cpcbID){
        res_tot.HRes = new_resinfo->width;
        res_tot.VRes = new_resinfo->height;
        res_act.HRes = new_resinfo->active_width;
        res_act.VRes = new_resinfo->active_height;
        VPP_SCL_SetBeFifo(vpp_obj, (PVPP_CPCB0_OUTPUT_RES)&res_tot, (PVPP_CPCB0_OUTPUT_RES)&res_act);
    }

    VPP_OBJ_DISPOUT_PARAM_UPDATE(uiDispId, dispID);
    VPP_OBJ_DISPOUT_PARAM_UPDATE(uiResId, resID);
    VPP_OBJ_DISPOUT_PARAM_UPDATE(uiBitDepth, bit_depth);
    VPP_OBJ_DISPOUT_PARAM_UPDATE(uiColorFmt, vidFmt);
    VPP_OBJ_DISPOUT_PARAM_UPDATE(iPixelRepeat, pixel_repeat);
    VPP_OBJ_DISPOUT_PARAM_UPDATE(uiDisplayMode, mode);

    cpcb->output_res_updated = 1; /* CPCB output resolution updated */

    /* set VOUT intput according to CPCB resolution (interlaced or progressive) */
    for(i = 0; i < cpcb->num_of_vouts; i++) {
        if(cpcbID == CPCB_1) {
            if (new_resinfo->scan == SCAN_PROGRESSIVE)
                cpcbout = VPP_VOP_CPCB0_PROG; /* CPCB-0 progressive */
            else
                cpcbout = VPP_VOP_CPCB0_INTER; /* CPCB-0 interlaced */

            VPP_BE_SetVoutInput(vpp_obj, cpcb->voutID[i], cpcbout);
        }
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    /*configure plane to CPCB routing*/
    VPP_BE_SetVoutInput(vpp_obj, VOUT_DSI, mode_to_vout[mode]);
#endif

    if(vpp_obj->vout[VOUT_HDMI].dvID == cpcbID && quiescent) {
        VPP_BE_SetPixelClock(vpp_obj, resID, vidFmt, pixel_repeat);
        VPP_BE_SetVoutFormat(vpp_obj, VOUT_HDMI, vidFmt, bit_depth);
    }
    else if(vpp_obj->vout[VOUT_DSI].dvID == cpcbID){
        if(IS_CPCB_VOUT_DISP_MODE(cpcbID, SINGLE_MODE_SEC)) {
            BE_VOP_ClkSelect(vpp_obj, VPP_VOP_HDMI, VPP_CPCB_CLK_D1);
            VPP_BE_SetVoutFormat(vpp_obj, VOUT_HDMI, OUTPUT_COLOR_FMT_RGB888, OUTPUT_BIT_DEPTH_12BIT);
        }
        VPP_BE_SetVoutFormat(vpp_obj, VOUT_DSI, OUTPUT_COLOR_FMT_RGB888, bit_depth);
    }

#if defined(PE_3D_BD)
    /* the next VBI interrupt would be for left view, and clear viewID for 2D resolutions*/
    if (vpp_obj->vout[VOUT_HDMI].dvID == cpcbID) {
        vpp_obj->dv[cpcbID].viewID = 0;

        if (pResinfo->flag_3d) {
            BE_VOP_SetHdmiFrmtPackingMode(vpp_obj, 0/*progressive frame packing*/, 0);
            BE_VOP_SetHdmiFrmtEnable(vpp_obj, 1);
        }else{
            BE_VOP_SetHdmiFrmtEnable(vpp_obj, 0);
        }
    }
#endif

#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
    VPPDRV_vppIntrGetCurrFPS(vpp_obj);
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: open a window of a video/graphics plane for display.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of the plane
 *        bgcolor - background color of window
 *        alpha - alpha of window
 *        x - start x-coordination of window in pixel in end display resolution
 *        y - start y-coordination of window in pixel in end display resolution
 *        width - width of window in pixel in end display resolution
 *        height - height of window in pixel in end display resolution
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_OpenDispWindow(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    int cpcbID, chanID;
    PLANE *plane;
    CHAN *chan;
#ifdef VPP_UNDERFLOW_FIX
    int iCount = 0, iAllPlaneStatus = 0;
#endif
    VPP_WIN *disp_win = NULL;
    RESOLUTION_INFO  *pResinfo;

    VPP_ClkGate_Plane(vpp_obj, planeID, 0);
    plane = &(vpp_obj->plane[planeID]);
    chanID = plane->chanID;
    chan = &(vpp_obj->chan[chanID]);
    cpcbID = chan->dvID;

#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
    disp_win = &chan->disp_win_user;
#else
    disp_win = &chan->disp_win;
#endif

    /* update video/graphics channel display window */
    disp_win->x = in_params[3];
    disp_win->y = in_params[4];
    disp_win->width = in_params[5];
    disp_win->height = in_params[6];

#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
    chan->disp_win_user_updated = 1; /* channel display window is update */
#else
    chan->disp_win_updated = 1; /* channel display window is update */
#endif
    if (0) {//BG4CT has scalar support for all planes
        plane->ref_win.width = in_params[5];
        plane->ref_win.height = in_params[6];
    }

    // Dont set plane mute if coming out of suspend/resume
    if (in_params[8] == POWER_MODE_NORMAL) {
        if (VPP_ENV_SA_IsMuteAllowed(vpp_obj, planeID, 0)) {
            /* un-mute channel */
            plane->mute = 0;
        }
    }
    pResinfo = VPP_GetResInfo(VPP_GetResId(cpcbID));

    //if(PLANE_VMX != planeID) //FIXME: VPP_SW_BRINGUP
    if(cpcbID == CPCB_1 && PLANE_VMX != planeID)
    {
        /* crop display window */
        cropDispWindow(disp_win, pResinfo, &(chan->crop_disp_win), chan);

        /* set video/grahpics channel attribute: alpha and background color */
        /* in_params[2]: alpha */
        /* in_params[1]: bgcolor, 0xUYV if OSD overlay in YUV domain */
        VPP_CPCB_SetPlaneAttribute(vpp_obj, cpcbID, chan->dvlayerID, in_params[2], in_params[1], in_params[7]);
#ifdef VPP_UNDERFLOW_FIX
        for(iCount = 0; iCount < MAX_NUM_PLANES; iCount++)
        {
            if(STATUS_INACTIVE == vpp_obj->plane[iCount].status)
            {
                iAllPlaneStatus++;
            }
        }

        if(MAX_NUM_PLANES == iAllPlaneStatus)
        {
            VPP_CPCB_DBG_LOG("\n All Planes are closed. cpcb0_ctrl2 = 0x%x n", vpp_obj->cpcb0_ctrl2.u32 );
            if(vpp_obj->cpcb0_ctrl2.u32 & 0xD)
            {
                VPP_CPCB_EnablePlane(vpp_obj, cpcbID, CPCB1_PLANE_1, 0);
                VPP_CPCB_EnablePlane(vpp_obj, cpcbID, CPCB1_PLANE_2, 0);
                VPP_CPCB_EnablePlane(vpp_obj, cpcbID, CPCB1_PLANE_3, 0);
            }
            //All planes are closed. So, disable clock enable for other planes
        }
#endif

        VPP_CPCB_EnablePlane(vpp_obj, cpcbID, chan->dvlayerID, 1);

    }
    /* reset plane data format/order */
    plane->srcfmt = -1;
    plane->order = -1;
    plane->bits_per_pixel = -1;

    /* reset plane current display content window */
    plane->actv_win.x = 0;
    plane->actv_win.y = 0;
    plane->actv_win.width = 0;
    plane->actv_win.height = 0;
    plane->disp_offset = 0;
    plane->crop_disp_offset = 0;

    plane->crop_actv_offset = 0;
    plane->crop_actv_win.x = 0;
    plane->crop_actv_win.y = 0;
    plane->crop_actv_win.width = 0;
    plane->crop_actv_win.height = 0;

    plane->crop_scl_actv_win.x = 0;
    plane->crop_scl_actv_win.y = 0;
    plane->crop_scl_actv_win.width = 0;
    plane->crop_scl_actv_win.height = 0;

    plane->scaler_crop_offset = 0;

    plane->crop_win_valid = 0;
    plane->crop_win.x = 0;
    plane->crop_win.y = 0;
    plane->crop_win.width = 0;
    plane->crop_win.height = 0;
    plane->crop_win_final = plane->crop_win;

    /* reset video/grahpics plane field to top field */
    plane->frame_type = -1; // invalid
    plane->field = 0; // top field
    plane->vp_buff_w_index = -1;
    plane->vp_buff_r0_index = -1;
    plane->vp_buff_r1_index = -1;
    plane->vp_buff_num = 0;

    plane->skip_frame = 0;

    /* set channel display window variable */
    chan->disp_win_attr.bgcolor = in_params[1];
    chan->disp_win_attr.alpha = in_params[2];
    chan->disp_win_attr.globalAlphaFlag = in_params[7];

    /* set video/graphics plane & channel in active status */
    plane->status = STATUS_ACTIVE;
    chan->status = STATUS_ACTIVE;

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: change a window of a video/graphics plane.
 *           the window is defined in end display resolution
 * INPUT: planeID - id of the plane
 *        bgcolor - background color of window
 *        alpha - alpha of window
 *        x - start x-coordination of window in pixel in end display resolution
 *        y - start y-coordination of window in pixel in end display resolution
 *        width - width of window in pixel in end display resolution
 *        height - height of window in pixel in end display resolution
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_ChangeDispWindow(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    int cpcbID, chanID;
    PLANE *plane;
    CHAN *chan;
    VPP_WIN *disp_win = NULL;
    RESOLUTION_INFO  *pResinfo;

    plane = &(vpp_obj->plane[planeID]);
    chanID = plane->chanID;
    chan = &(vpp_obj->chan[chanID]);
    cpcbID = chan->dvID;

    if(PLANE_VMX == planeID)
    {
        plane->crop_scl_actv_win.x = in_params[3];
        plane->crop_scl_actv_win.y = in_params[4];
        plane->crop_scl_actv_win.width = in_params[5];
        plane->crop_scl_actv_win.height = in_params[6];

        plane->disp_win_updated = 1;

        return (MV_VPP_OK);
    }
    pResinfo = VPP_GetResInfo(VPP_GetResId(cpcbID));

    /* set video plane background window */
    if (in_params[3] != -1)
    {
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
        disp_win = &chan->disp_win_user;
#else
        disp_win = &chan->disp_win;
#endif
        if ((disp_win->x != in_params[3])
         || (disp_win->y != in_params[4])
         || (disp_win->width != in_params[5])
         || (disp_win->height != in_params[6]))
        {
            disp_win->x = in_params[3];
            disp_win->y = in_params[4];
            disp_win->width = in_params[5];
            disp_win->height = in_params[6];

            /* update video/graphics channel display window */
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
            chan->disp_win_user_updated = 1;
            /* In DV cert streams L5 metadat carries disp_win offsets, so dispwin will be changed in ISR in that case */
            /* Keeping backup of user_disp_win so that when switched to a non dolby/dolby stream without L5 metadata user setting will be intact */
            chan->disp_win_user_back = chan->disp_win_user;
#else
            chan->disp_win_updated = 1; /* channel display window is update */
#endif
            /* crop display window */
            cropDispWindow(disp_win, pResinfo, &(chan->crop_disp_win), chan);
        }
    }
    /* set video plane attribute: alpha and background color */
    /* in_params[2]: alpha */
    /* in_params[1]: bgcolor, 0xUYV if OSD overlay in YUV domain */
    if (in_params[1] == -1)
        in_params[1] = chan->disp_win_attr.bgcolor;

    if (in_params[2] == -1)
        in_params[2] = chan->disp_win_attr.alpha;
    if (in_params[7] == -1)
        in_params[7] = chan->disp_win_attr.globalAlphaFlag;

    if((in_params[1] != chan->disp_win_attr.bgcolor) || (in_params[2] != chan->disp_win_attr.alpha) ||
        (in_params[7]!= chan->disp_win_attr.globalAlphaFlag)){
        VPPAPI_DBG_INFO("VPP_CPCB_SetPlaneAttribute : %x, %d\n",in_params[2], in_params[7]);
        if (!plane->mute)
            VPP_CPCB_SetPlaneAttribute(vpp_obj, cpcbID, chan->dvlayerID, in_params[2], in_params[1], in_params[7]);

        /* update channel display window attribute variable */
        chan->disp_win_attr.bgcolor = in_params[1];
        chan->disp_win_attr.alpha = in_params[2];
        chan->disp_win_attr.globalAlphaFlag = in_params[7];
    }

    return (MV_VPP_OK);
}


/****************************************************************************
 * FUNCTION: close display window of a video/graphics plane in displaying
 * INPUT: planeID - id of the plane
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ***************************************************************************/
int VPP_CloseDispWindow(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    int cpcbID, chanID;
    VDEC_VID_BUF_DESC *frame;
    PLANE *plane;
    CHAN *chan;
#ifdef VPP_UNDERFLOW_FIX
    int iCount = 0, iAllPlaneStatus = 0;
#endif

    plane = &(vpp_obj->plane[planeID]);
    chanID = plane->chanID;
    chan = &(vpp_obj->chan[chanID]);
    cpcbID = chan->dvID;

    /* set video plane & channel in inactive status */
    plane->status = STATUS_INACTIVE;
    chan->status = STATUS_INACTIVE;

    /* close display window */
    if(PLANE_VMX == planeID) {
        int VmxParams[MAX_NUM_PARAMS];
        VmxParams[0] = VPP_VMX_ENABLE_WATERMARK;
        VmxParams[1] = 0;
        VPP_VMXAPI_SetCmd(vpp_obj, VmxParams);

        VmxParams[0] = VPP_VMX_ENABLE_BGWATERMARK;
        VmxParams[1] = 0;
        VPP_VMXAPI_SetCmd(vpp_obj, VmxParams);

        VmxParams[0] = VPP_VMX_ENABLE_BYPASS;
        VmxParams[1] = 1;
        VPP_VMXAPI_SetCmd(vpp_obj, VmxParams);
    } else {
        /* set plane source window to 0 to disable video plane input */
        VPP_CPCB_SetPlaneCropWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
        VPP_CPCB_SetPlaneSourceWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
        /* set plane background window to 0 to close video plane window */
        VPP_CPCB_SetPlaneBGWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
#ifdef VPP_UNDERFLOW_FIX
        for(iCount = 0; iCount < MAX_NUM_PLANES; iCount++) {
            if(STATUS_INACTIVE == vpp_obj->plane[iCount].status) {
                iAllPlaneStatus++;
            }
        }
        if(MAX_NUM_PLANES == iAllPlaneStatus) {
            VPP_CPCB_DBG_LOG("All planes are closed.\n");
            VPP_CPCB_EnablePlane(vpp_obj, cpcbID, CPCB1_PLANE_1, 1);
            VPP_CPCB_EnablePlane(vpp_obj, cpcbID, CPCB1_PLANE_2, 1);
            VPP_CPCB_EnablePlane(vpp_obj, cpcbID, CPCB1_PLANE_3, 1);
        } else
#endif
        {
            VPP_CPCB_DBG_LOG("Disabling clock\n");

            VPP_CPCB_EnablePlane(vpp_obj, cpcbID, chan->dvlayerID, 0);
        }
    }

    /* recycle back buffer current frame if there is */
    if (plane->prev_curr_frame != plane->curr_frame && plane->prev_curr_frame) {
        frmq_push(&(plane->outputq), plane->prev_curr_frame);
    }

    /* recycle current frame if there is */
    if (plane->curr_frame) {
        frmq_push(&(plane->outputq), plane->curr_frame);
    }

    /* flush plane's frame queue and recycle frame descriptors */
    while (frmq_pop(&(plane->inputq), (void **)&frame)) {
        frmq_pop_commit(&(plane->inputq));
        frmq_push(&(plane->outputq), frame);
    }

    //Reset Prev and curr pointers - Avoid recycling same DBUF twice
    plane->prev_curr_frame = NULL;
    plane->curr_frame = NULL;

    VPP_ClkGate_Plane(vpp_obj, planeID, 1);

    return (MV_VPP_OK);
}

/*******************************************************************************
 * FUNCTION: set crop window for a plane
 * INPUT: planeID - id of the plane
 *        x - start x of crop window in pixel
 *        y - start y of crop window in pixel
 *        width - width of crop window in pixel
 *        height - height of crop window in pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 *****************************************************************************/
int VPP_SetCropWindow(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    int chanID, cpcbID;
    RESOLUTION_INFO  *pResinfo;

    chanID = vpp_obj->plane[planeID].chanID;
    cpcbID = CPCB_OF_CHAN(vpp_obj, chanID);
    if(cpcbID == CPCB_INVALID)
        cpcbID = CPCB_1;

    /* set plane crop window valid */
    vpp_obj->plane[planeID].crop_win_valid = 1;
    vpp_obj->plane[planeID].crop_win.x = in_params[1];
    vpp_obj->plane[planeID].crop_win.y = in_params[2];
    vpp_obj->plane[planeID].crop_win.width = in_params[3];
    vpp_obj->plane[planeID].crop_win.height = in_params[4];

    vpp_obj->plane[planeID].crop_win_final = vpp_obj->plane[planeID].crop_win;
    VDB_print("VPP_SetCropWindow updated - %d, %d, %d, %d\n", in_params[1],in_params[2],in_params[3], in_params[4]);
    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[cpcbID]);
    #ifdef VPP_ENABLE_INTERLACED_OUTPUT_SCALING_CROPING
        if(SCAN_INTERLACED == pResinfo->scan) {
            vpp_obj->plane[planeID].crop_win_final.y = (int)(vpp_obj->plane[planeID].crop_win.y/2);
            vpp_obj->plane[planeID].crop_win_final.height = (vpp_obj->plane[planeID].crop_win.height+1)/2;
            VDB_print("Interlaced output correction : cropwin(y,h) : org(%d, %d) -> new(%d, %d)\n",
                in_params[2],in_params[4], vpp_obj->plane[planeID].crop_win_final.y, vpp_obj->plane[planeID].crop_win_final.height);
    }
#endif //VPP_ENABLE_INTERLACED_OUTPUT_SCALING_CROPING

    /* program plane crop window */
    VPP_CPCB_SetPlaneCropWindow(vpp_obj, vpp_obj->chan[chanID].dvID, vpp_obj->chan[chanID].dvlayerID,
        vpp_obj->plane[planeID].crop_win_final.x, vpp_obj->plane[planeID].crop_win_final.y,
        vpp_obj->plane[planeID].crop_win_final.width, vpp_obj->plane[planeID].crop_win_final.height);

    return (MV_VPP_OK);
}


/*******************************************************************************
 * FUNCTION: set reference window for a plane
 * INPUT: planeID - id of the plane
 *        width - width of reference window in pixel
 *        height - height of reference window in pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 *****************************************************************************/
int VPP_SetRefWindow(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    PLANE *plane;
    VBUF_INFO *pinfo;
    int result = MV_VPP_OK;
    VPP_WIN *ref_win = NULL;
    INT *ref_win_update = NULL;
    BOOL pos_update_en = FALSE;

    plane = &(vpp_obj->plane[planeID]);

#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
    ref_win = &plane->ref_win;
    ref_win_update = &plane->ref_win_updated;
    pos_update_en = TRUE;
#else
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
    ref_win = &plane->ref_win_user;
    ref_win_update = &plane->ref_win_user_init;
#else
    ref_win = &plane->ref_win;
    ref_win_update = &plane->ref_win_updated;
#endif
    if(planeID == PLANE_GFX1) {
        pos_update_en = TRUE;
    }
#endif

    /*return if no change in ref_window*/
    if ((ref_win->width == in_params[3]) && (ref_win->height == in_params[4])) {
        if(pos_update_en) {
            if((ref_win->x == in_params[1]) && (ref_win->y == in_params[2])) {
                return (MV_VPP_OK);
            }
        }
        else {
            return (MV_VPP_OK);
        }
    }

    pinfo = VPP_GetFrame(plane);
    if(!(pinfo && pinfo->m_active_width && pinfo->m_active_height)) {
        /* Scaler settings will be applied only if valid frame present.
         * If new frame settings received during NULL frame display, reset and
         * override old settings (ref_win), as the setting intended and to be applied
         * for next valid frame.
         */
        plane->ref_win_updated = 0;
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
        plane->ref_win_user_init = 0;
#endif
    }
    /* plane reference window is changed */
    /*Don't allow SetRefWin if SetRefWinFromISR is processed in this interrupt
      To make sure, both DISPSRV and VPP on same scaler setting,
      report error for unapplied settings*/
    if(*ref_win_update) {
        result = MV_VPP_ESWSTATEWRONG;
        VDB_print("VPP_SetRefWindow - not applied\n");
    }
    else {
        if(pos_update_en) {
            ref_win->x = in_params[1];
            ref_win->y = in_params[2];
        }
        ref_win->width = in_params[3];
        ref_win->height = in_params[4];
        *ref_win_update = 1;
#ifdef CONFIG_HARDCODE_BUILT_IN_FRAME_SCL_TO_1_1
        if(pos_update_en) {
            plane->ref_win_user_back = *ref_win;
        }
        else {
            plane->ref_win_user_back.width = ref_win->width;
            plane->ref_win_user_back.height = ref_win->height;
        }
#endif
         VDB_print("VPP_SetRefWindow updated - %d, %d, %d, %d\n",
                  in_params[1],in_params[2],in_params[3], in_params[4]);
         VDB_print("SETREFWIN: plane [%d], x[%d], y[%d], width[%d], height[%d] \n",
                  planeID, ref_win->x, ref_win->y, ref_win->width, ref_win->height);
    }
    return result;
}

/*******************************************************************
 * FUNCTION: enable/disable linear horizontal scaling
 * INPUT: planeID - id of the plane
 *        enable - 1: enable; 0: disable
 *        center - center value [0, 255]
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int VPP_SetScalerNonLinearControl(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    int chanID;
    PLANE *plane;
    CHAN *chan;

    plane = &(vpp_obj->plane[planeID]);
    chanID = plane->chanID;
    chan = &(vpp_obj->chan[chanID]);

    chan->scl_nl_enable = in_params[1];
    chan->scl_nl_center = in_params[2];
    chan->scl_nl_cratio = in_params[3];

    /* update scaler */
    if(PLANE_MAIN == planeID)
    {
        vpp_obj->scl_coeff_update[VPP_FRC_SCL_MAIN] = 1;
        vpp_obj->scl_coeff_update[VPP_FRC_SCL_VIP_Y] = 1;
        vpp_obj->scl_coeff_update[VPP_FRC_SCL_VIP_UV] = 1;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if(PLANE_PIP == planeID)
        vpp_obj->scl_coeff_update[VPP_FRC_SCL_PIP] = 1;
#endif
    else
        vpp_obj->scl_coeff_update[VPP_FRC_SCL_OSD] = 1;

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: display the display mode for a plane
 * INPUT: planeID - id of the plane
 *        mode - DISP_STILL_PIC: display logo, DISP_FRAME: display video
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *******************************************************************/
int VPP_SetDisplayMode(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    PLANE *plane;

    plane = &(vpp_obj->plane[planeID]);

    //TODO: Use static tables
    if ((plane->status == STATUS_INACTIVE)
#ifdef ENABLE_EL_PIPE_PATH
            && (planeID != PLANE_OVP_EL)
#endif
        )
        return (MV_VPP_OK);

    if (in_params[1] == DISP_STILL_PIC){
        if ((planeID == PLANE_MAIN) && (plane->status == STATUS_DISP_PATGEN))
            FE_PATGEN_Enable(vpp_obj, 0); // disable patgen

        if (plane->status != STATUS_DISP_LOGO){
            /* set grahpics plane & channel in DISP_LOG status */
            plane->status = STATUS_DISP_LOGO;
#ifdef VPP_ENABLE_FRAME_CAPTURE
            if(plane->CaptureVidFrm.gCapStartFlag == true){
                MV_OSAL_Sem_Post(vpp_obj->vpp_FrameCapture_sem);
            }
#endif
        }
    } else if (in_params[1] == DISP_FRAME){
        if ((planeID == PLANE_MAIN) && (plane->status == STATUS_DISP_PATGEN))
            FE_PATGEN_Enable(vpp_obj, 0); // disable patgen

        /* 3D VNR is disabled in still picture mode, this help to recover 3D VNR mode during Logo to frame transition if 3D mode is set and the coming frame is progressive */
        if ((planeID == PLANE_MAIN) && (plane->status == STATUS_DISP_LOGO)){
            plane->vp_updated = 1;
        }

        if (plane->status != STATUS_DISP_VIDEO){
            /* set grahpics plane & channel in DISP_VIDEO status */
            plane->status = STATUS_DISP_VIDEO;
        }
    } else if (in_params[1] == DISP_PATGEN){
        if ((planeID == PLANE_MAIN) && (plane->status != STATUS_DISP_PATGEN)){
            FE_PATGEN_Enable(vpp_obj, 1); // enable patgen

            /* set main in DISP_PATGEN status */
            plane->status = STATUS_DISP_PATGEN;
#ifdef VPP_ENABLE_FRAME_CAPTURE
            if(plane->CaptureVidFrm.gCapStartFlag == true){
                MV_OSAL_Sem_Post(vpp_obj->vpp_FrameCapture_sem);
            }
#endif
        }
    }
#ifdef ENABLE_EL_PIPE_PATH
    if(planeID == PLANE_OVP_EL)
    {
        int chanID;
        CHAN *chan;

        plane = &(vpp_obj->plane[planeID]);
        chanID = plane->chanID;
        chan = &(vpp_obj->chan[chanID]);
        chan->status = STATUS_ACTIVE;
    }
#endif

    return (MV_VPP_OK);
}

/***************************************************
 * FUNCTION: set plane border alpha
 * PARAMS:  planeID - id of the plane
 *          alpha - border alpha value
 * RETURN: MV_VPP_OK - succeed
 ***********************************************/
int VPP_SetPlaneBorderAlpha(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    int alpha = in_params[1];
    int cpcbID, chanID;
    PLANE *plane;
    CHAN *chan;

    plane = &(vpp_obj->plane[planeID]);
    chanID = plane->chanID;
    chan = &(vpp_obj->chan[chanID]);
    cpcbID = chan->dvID;

    plane->borderAlpha= alpha;

    VPP_OVL_SetBorderAlpha(vpp_obj, cpcbID, chan->dvlayerID+1, alpha);

    return (MV_VPP_OK);
}

/***************************************************
 * FUNCTION: set alpha polarity of a plane
 * PARAMS:  planeID - plane ID
 *          src_pol - source alpha polarity
 *          bg_pol - border alpha polarity
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int VPP_SetPlaneAlphaPolarity(VPP_OBJ *vpp_obj, int *in_params)
{
    PLANE *plane = &(vpp_obj->plane[in_params[0]]);
    CHAN *chan = &(vpp_obj->chan[plane->chanID]);

    plane->src_pol = in_params[1];
    plane->bg_pol  = in_params[2];

    VPP_CPCB_SetPlaneAlphaPolarity(vpp_obj, chan->dvID, chan->dvlayerID, in_params[1], in_params[2]);

    return (MV_VPP_OK);
}
int VPP_SetPlaneMuteState(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef MUTE_OPTIMISED
    int cpcbID, chanID;
    int planeID = in_params[0];
    int mute = in_params[2];
    PLANE *plane;
    CHAN *chan;
    plane = &(vpp_obj->plane[planeID]);
    chanID = plane->chanID;
    chan = &(vpp_obj->chan[chanID]);
    cpcbID = chan->dvID;
    VPPAPI_DBG_INFO("VPP_SetPlaneMuteState entry mute =%d \n",mute);
    if(mute ==0)
    {
        if(plane->status != STATUS_INACTIVE)
        {
            VPPAPI_DBG_INFO("VPP_SetPlaneMuteState : plane is already in unmuted state\n");
            return -1;
        }
        VPP_ClkGate_Plane(vpp_obj, planeID, 0);
        //unmute the plane
        VPP_CPCB_EnablePlane(vpp_obj, cpcbID, chan->dvlayerID, 1);
        plane->status = STATUS_ACTIVE;
        chan->status = STATUS_ACTIVE;
        plane->ref_win_updated = 1;
        plane->disp_win_updated=1;
        plane->status = plane->status_bk;
    }
    else
    {
        //mute the plane
        if(plane->status == STATUS_INACTIVE)
        {
            VPPAPI_DBG_INFO("the plane is already inactive \n");
            return -1;
        }
        plane->status_bk = plane->status;
        VPPAPI_DBG_INFO("the statusbk =%d \n",plane->status_bk);
        plane->status = STATUS_INACTIVE;
        chan->status = STATUS_INACTIVE;
        /* set plane source window to 0 to disable video plane input */
        VPP_CPCB_SetPlaneCropWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
        VPP_CPCB_SetPlaneSourceWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
        /* set plane background window to 0 to close video plane window */
        VPP_CPCB_SetPlaneBGWindow(vpp_obj, cpcbID, chan->dvlayerID, 0, 0, 0, 0);
        VPP_CPCB_EnablePlane(vpp_obj, cpcbID, chan->dvlayerID, 0);
        plane->crop_scl_actv_win.x=0;
        plane->crop_scl_actv_win.y = 0;
        plane->crop_scl_actv_win.width = 0;
        plane->crop_scl_actv_win.height = 0;
        VPP_ClkGate_Plane(vpp_obj, planeID, 1);
    }
#endif //MUTE_OPTIMISED
    return (MV_VPP_OK);
}

/***************************************************
 * FUNCTION: mute/un-mute a plane
 * PARAMS:  planeID - plane to mute/un-mute
 *          mute - 1: mute, 0: un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int VPP_SetPlaneMute(VPP_OBJ *vpp_obj, int *in_params)
{
    int zorder1_4;
    int planeID = in_params[0];
    int cpcbID = in_params[1];
    int mute = in_params[2];
    PLANE *plane = &(vpp_obj->plane[planeID]);

    // update mute status
    plane->mute = mute;
#ifdef MUTE_OPTIMISED
    VPP_SetPlaneMuteState(vpp_obj,in_params);
#else
    zorder1_4 = (vpp_obj->chan[CHAN_MAIN].zorder & 0xFF)
              | ((vpp_obj->chan[CHAN_PIP].zorder & 0xFF) << 8)
              | ((vpp_obj->chan[CHAN_GFX1].zorder & 0xFF) << 16);

    VPP_CPCB_SetZOrder(vpp_obj, cpcbID, zorder1_4);
#endif
    return (MV_VPP_OK);
}
/***************************************************
 * FUNCTION: set hue/saturation/brightness/contrast
 * INPUT:    cscChannel - plane to adjust BCHS
 *           mode - the item to be handled
 *           value - the value to be set
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ***********************************************/
INT VPP_SetPQOCSC(VPP_OBJ *vpp_obj, int *in_params)
{
    VPP_CHANNEL_TYPE cscChannel = in_params[0];
    ENUM_PICTURE_CTRL mode = in_params[1];
    float value;

    VPPAPI_DBG_INFO("BCHS mode=%d cscChan=%d value=%d \n",mode,cscChannel,in_params[2]);

    switch(mode)
    {
        case PICTURE_CTRL_CONTRAST:
            value = (float)VPP_OCSC_CONTRAST_INT2FLOAT(in_params[2]);
            vpp_obj->gContrast[cscChannel] = value;
        break;

        case PICTURE_CTRL_HUE:
            value = (float)VPP_OCSC_HUE_INT2FLOAT(in_params[2]);
            vpp_obj->gHue[cscChannel] = value;
        break;

        case PICTURE_CTRL_SATURATION:
            value = (float)VPP_OCSC_SATURATION_INT2FLOAT(in_params[2]);
            vpp_obj->gSaturation[cscChannel] = value;
        break;

        case PICTURE_CTRL_BRIGHTNESS:
            value = (float)VPP_OCSC_BRIGHTNESS_INT2FLOAT(in_params[2]);
            vpp_obj->gBrightness[cscChannel] = value;
        break;

        default:
            return (MV_VPP_EBADPARAM);
    }

    if (IS_VPP_VALID_OCSC_PICTURE_CTRL_VAL(mode, value)) {
        VPPAPI_DBG_ERROR("BCHS Invalid mode=%d Chan=%d value=%f %d \n",mode,cscChannel,in_params[2]);
        return (MV_VPP_EBADPARAM);
    }
    vpp_obj->videofmtUpdated = 1;
    return (MV_VPP_OK);
}

/***************************************************
 * FUNCTION: set hue/saturation/brightness/contrast
 * INPUT:  planeID - plane to adjust pq
 *           mode - the item to be handled
 *           value - the value to be set
 *           OvpOrMain - to set on main or in global way
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ***********************************************/
INT VPP_SetPQICSC(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    ENUM_PICTURE_CTRL mode = in_params[1];
    int value = in_params[2];
    VPPDBG_PQ_LOG("mode=%d  planeID=%d value=%d \n", mode,planeID,value);

    switch(mode)
    {
        case PICTURE_CTRL_CONTRAST:
            VPP_CSC_ICSC_SetContrast(vpp_obj, planeID, value);
        break;

        case PICTURE_CTRL_HUE:
            VPP_CSC_ICSC_SetHue(vpp_obj, planeID, value);
        break;

        case PICTURE_CTRL_SATURATION:
            VPP_CSC_ICSC_SetSaturation(vpp_obj, planeID, value);
        break;

        case PICTURE_CTRL_BRIGHTNESS:
           VPP_CSC_ICSC_SetBrightness(vpp_obj, planeID, value);
        break;

        default:
            VPPAPI_DBG_INFO("%s %d Not supported \n",__FUNCTION__,__LINE__);
            return (MV_VPP_EBADPARAM);
    }

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: change z-order for all CPCBs
 * INPUT: NULL
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int VPP_ChangeZOrder(VPP_OBJ *vpp_obj, int *in_params)
{

    VPP_CPCB_SetZOrder(vpp_obj, in_params[0], in_params[1]);

    return (MV_VPP_OK);
}

/*****************************************************************************
 * FUNCTION: set csc mode for gfx plane (mainly for AYUV32 input)
 * INPUT: planeID - id of the GFX plane
 *        csc_mode - csc mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured or plane not active
 *         MV_EFRAMEQFULL - frame queue is full
 *****************************************************************************/
int VPP_SetGfxCsc(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
//    int csc_mode = in_params[1];

    switch (planeID)
    {
        case PLANE_GFX1:
//            FRC_SCL_SetOsdFmt(vpp_obj, VPP_FRC_SCL_GFX0, csc_mode);
            break;
        default:
            return (MV_VPP_EBADPARAM);
    }

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set the source of video output
 * INPUT: voutID - id of the Vout
 *            cpcbID - id of the CPCB
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int VPP_SetVoutSource(VPP_OBJ *vpp_obj, int *in_params)
{
    int voutID = in_params[0];
    int cpcbID = in_params[1];
    int interlaced = in_params[2];
    DV *cpcb;
    RESOLUTION_INFO *resinfo;
    int cpcbout;
    int res;

    cpcb = &(vpp_obj->dv[cpcbID]);
    if (OUTPUT_AUTO_SELECT == interlaced) {
        resinfo = VPP_GetResInfo(cpcb->dispOutParams.uiResId);
        interlaced = resinfo->scan;
    }

    if (cpcbID == CPCB_1) {
        if (interlaced == SCAN_PROGRESSIVE)
            cpcbout = VPP_VOP_CPCB0_PROG; /* CPCB-0 progressive */
        else
            cpcbout = VPP_VOP_CPCB0_INTER; /* CPCB-0 interlaced */

        VPP_BE_SetVoutInput(vpp_obj, voutID, cpcbout);
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    /*configure plane to CPCB routing*/
    VPP_BE_SetVoutInput(vpp_obj, VOUT_DSI, mode_to_vout[vpp_obj->m_VoutMode]);
#endif

    res = cpcb->dispOutParams.uiResId;

    if (interlaced == OUTPUT_PROGRESSIVE){
        switch (res){
            case RES_NTSC_M:
            case RES_NTSC_J:
            case RES_PAL_M:
            case RES_525I60:
            case RES_525I5994:
                res = RES_525P60;
                break;

            case RES_PAL_BGH:
            case RES_625I50:
                res = RES_625P50;
                break;

            case RES_1080I60:
            case RES_1080I5994:
                res = RES_1080P60;
                break;

            case RES_1080I50:
                res = RES_1080P50;
                break;

            default:
                break;
        }
    }else{
        switch (res){
            case RES_525P60:
            case RES_525P5994:
                res = RES_525I60;
                break;

            case RES_625P50:
                res = RES_625I50;
                break;

            case RES_1080P60:
            case RES_1080P5994:
                res = RES_1080I60;
                break;

            case RES_1080P50:
                res = RES_1080I50;
                break;

            default:
                break;
        }
    }

    if (cpcbout != VPP_VOP_NO_INPUT) {
        //TODO: Use static tables to disable VOUT ID
        if (voutID == VOUT_DSI) {
            VPP_BE_SetVoutFormat(vpp_obj, VOUT_DSI, OUTPUT_COLOR_FMT_RGB888, OUTPUT_BIT_DEPTH_8BIT);
        }
    }

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set Deint Param
 * INPUT: in_params - contain cmd and DS contain parameter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetDeintEvalCommand(VPP_OBJ *vpp_obj, int *in_params)
{
        int RetStatus=0;
#ifdef DEINT_ENABLE
    int Cadence;
    VPP_DEINTUSER_CAD_PARAMS param;
    VPP_DINT_PRESET_MODE_PARAMS    PresetParam;
    PLANE *plane;

        plane = &vpp_obj->plane[PLANE_MAIN];
        switch(in_params[0])
        {
                case VPP_CMD_DEINT_SET_USERCAD:
                {
            if(in_params[1]==0){
            Cadence=vpp_obj->DEINTUserCad1.Cadence;
            param.Enable=vpp_obj->DEINTUserCad1.Enable;
            param.Length=vpp_obj->DEINTUserCad1.Length;
            param.Pattern=vpp_obj->DEINTUserCad1.Pattern;
                        RetStatus = FE_DEINT_ConfigUserCadence(vpp_obj,Cadence,&param);
            }
            else{
                        Cadence=vpp_obj->DEINTUserCad2.Cadence;
                        param.Enable=vpp_obj->DEINTUserCad2.Enable;
                        param.Length=vpp_obj->DEINTUserCad2.Length;
                        param.Pattern=vpp_obj->DEINTUserCad2.Pattern;
                        RetStatus = FE_DEINT_ConfigUserCadence(vpp_obj,Cadence,&param);
            }
                }
                break;
                case VPP_CMD_DEINT_SET_EXCLCTRL:
                {
                        RetStatus = FE_DEINT_SetExclCtrlParams(vpp_obj,(VPP_DEINTEXCL_CTRL_PARAMS*)&(vpp_obj->DEINTExclCtrl));
                }
                break;
        case VPP_CMD_DEINT_SET_LPFCTRL:
        {
            //FIXME: new vpp - following part of ee??
//            RetStatus = FE_DET_SetLpfCtrl(vpp_obj,&(vpp_obj->DEINTLPFCtrl.LPFCtrl));
//            RetStatus = FE_DET_SetLpfCtrlParams(vpp_obj,&(vpp_obj->DEINTLPFCtrl.LPFParam));

        }
        break;
                case VPP_CMD_DEINT_SET_FILM_CADCTRL:
                {
                        RetStatus = FE_DEINT_SetFilmCadenceCtrl(vpp_obj,vpp_obj->FilmCadence);
                }
                break;
                case VPP_CMD_DEINT_SET_PRESET_MODE:
                {

            plane->DeintPreset.ViMode = vpp_obj->DEINTPreset.DeintPresetModeParam.ViMode;
            plane->DeintPreset.ChrMotVidMode = vpp_obj->DEINTPreset.DeintPresetModeParam.ChrMotVidMode;
            plane->DeintPreset.ChrMotFlmMode = vpp_obj->DEINTPreset.DeintPresetModeParam.ChrMotFlmMode;
            plane->vp_updated=2; // no need to  skip frame
            //RetStatus = FE_DEINT_SetPresetMode(vpp_obj,vpp_obj->DEINTPreset.DeintPresetMode,&(vpp_obj->DEINTPreset.DeintPresetModeParam));
                }
                break;
                case VPP_CMD_DEINT_SET_FORCED_2D:
                {
//                        RetStatus = FE_DEINT_SetForced2DCtrl(vpp_obj,(VPP_DEINT_FORCED2D_CTRL*)&(vpp_obj->DEINTF2DCtrl));
                }
                break;
                case VPP_CMD_DEINT_SET_HARD_SOFT_FILM_MODE:
                {
//                        RetStatus = FE_DEINT_SetFilmModeEntry(vpp_obj,(VPP_DEINT_FILM_MODE_ENTRY_PARAM*)&(vpp_obj->DEINTFilmMode));
                }
                break;
                case VPP_CMD_DEINT_SET_OSD_CTRL:
                {
                        RetStatus = FE_DEINT_SetOSDCtrl(vpp_obj,vpp_obj->DEINTOSDCtrl);
                }
                break;
                case VPP_CMD_DEINT_SET_DEINT_ADV_CTRL:
                {
                        RetStatus = FE_DEINT_SetAdvancedModeParams(vpp_obj,&(vpp_obj->DEINTADVCtrl));
                }
                break;
                default:
                {
                }
                break;
        }
#endif
        return RetStatus;
}

/****************************************************************
 * FUNCTION: set De-interlacer and VNR working mode
 * INPUT: deint_mode - de-interlacer mode
 *        vnr_mode - VNR mode
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 *****************************************************************/
int VPP_SetDeintMode(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef DEINT_ENABLE
    PLANE *plane;

    plane = &vpp_obj->plane[PLANE_MAIN];

    if (in_params[0] != -1) {
        if (plane->deint_mode != in_params[0]) {
            plane->deint_mode = in_params[0];
            plane->vp_updated = 1;
        }
    }

    if(in_params[0] == -1 && in_params[1] != -1) // only VNR (no need to skip frame)
    plane->vp_updated = 2;
#endif
    return (MV_VPP_OK);
}

/********************************************************
 * FUNCTION: set pattern size for patgen in main channel
 * PARAMS:  width - width of pattern
 *          height - height of pattern
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *********************************************************/
int VPP_SetPatternSize(VPP_OBJ *vpp_obj, int *in_params)
{
    VPP_PATGEN_TIMING timing;
    VBUF_INFO *pinfo;
    PLANE *plane = &vpp_obj->plane[PLANE_MAIN];

    /* build a dummy frame descriptor for pattern */
#ifndef CONFIG_AMP_TEE_ENABLE
    pinfo = (VBUF_INFO *)plane->dummy_frame->user_data;
#else
    pinfo = (VBUF_INFO *)plane->dummy_frame;
#endif
    pinfo->m_srcfmt = -1; // invalid
    pinfo->m_order = -1; // invalid
    pinfo->m_active_left = 0;
    pinfo->m_active_top = 0;
    pinfo->m_active_width = in_params[0];
    pinfo->m_active_height = in_params[1];
    pinfo->m_is_frame_seq = 1; // progressive

    timing.hRes = in_params[0];
    timing.vRes = in_params[1];
    timing.hSw = 1;
    timing.vSw = 2;
    timing.hSt = 1;
    timing.vSt = 2;
    timing.hTot = timing.hRes + 7;
    timing.vTot = timing.vRes + 5;
    FE_PATGEN_SetTiming(vpp_obj, &timing);

    return (MV_VPP_OK);
}

/********************************************************
 * FUNCTION: set pattern frame type
 * PARAMS:  type - progressive or interlaced
 *          cadence - normal, 3-2 cad or 2-2 cad
 *          rff - repeat field flag
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int VPP_SetPatternFrameType(VPP_OBJ *vpp_obj, int *in_params)
{
    VBUF_INFO *pinfo;
    PLANE *plane = &vpp_obj->plane[PLANE_MAIN];

    /* build a dummy frame descriptor for pattern */
#ifndef CONFIG_AMP_TEE_ENABLE
    pinfo = (VBUF_INFO *)plane->dummy_frame->user_data;
#else
    pinfo = (VBUF_INFO *)plane->dummy_frame;
#endif
    if (in_params[0] == PATGEN_PROG)
        pinfo->m_is_frame_seq = 1; // progressive
    else
        pinfo->m_is_frame_seq = 0; // interlaced

    FE_PATGEN_SetFrameType(vpp_obj, in_params[0], in_params[1], in_params[2]);

    return (MV_VPP_OK);
}

/********************************************************
 * FUNCTION: set pattern data
 * PARAMS:  mode - mode
 *          hPith - horizontal pitch
 *          vPith - vertical pitch
 *          hColPith - horizontal color pitch
 *          vColPith - vertical color pitch
 *          colSeed1 - color seed for frame 1
 *          colSeed2 - color seed for frame 2
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ********************************************************/
int VPP_SetPatternData(VPP_OBJ *vpp_obj, int *in_params)
{
    VPP_PATGEN_DATA_CTRL data_ctrl;
    int hmode = in_params[0] & 0x0f;
    int vmode = (in_params[0] >> 4) & 0x0f;

    data_ctrl.hPith = in_params[1];
    data_ctrl.vPith = in_params[2];
    data_ctrl.hColPith = in_params[3];
    data_ctrl.vColPith = in_params[4];
    data_ctrl.ColSeed1 = in_params[5];
    data_ctrl.ColSeed2 = in_params[6];
    FE_PATGEN_SetDataCtrl(vpp_obj, &data_ctrl);

    FE_PATGEN_SetMode(vpp_obj, hmode, vmode);

    return (MV_VPP_OK);
}

/***************************************************************************************
 * FUNCTION: set phase DDA counter ( sub-pixel shifting)
 * PARAMS: scalerId - scaler index
 *               hv - horizontal or vertical
 *               counter - DDA counter
 * RETURN:  1 - succeed
 *                others - failed to set phase DDA counter
 ***************************************************************************************/
int VPP_SetScalerPhaseDda(VPP_OBJ *vpp_obj, int *in_params )
{
    VPP_FRC_SCL_NUM scalerId;
    int hv;
    int counter;

    scalerId = (VPP_FRC_SCL_NUM)in_params[0];
    hv = in_params[1];
    counter = in_params[2];

    FRC_SCL_SetPhaseDda(vpp_obj, scalerId, hv, counter);

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set HDMI command
 * INPUT:  param0 - mi command to be executed
 *      :  param(1..n) - parameters specific to the command
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_VPP_EBADPARAM - invalid parameters
 *******************************************************************/
int VPP_SetHdmiCmd(VPP_OBJ *vpp_obj, int *in_params)
{
    switch (in_params[0])
    {
        case VPP_HDMI_SET_VIDEO_FMT:
        {
            VPP_BE_SetHdmiVideoFmt (vpp_obj);
        }
        break;

        case VPP_HDMI_SET_AUDIO_FMT:
        {
            VPP_BE_SetHdmiAudioFmt (vpp_obj,
                                    in_params[1]/* Audio enable */);
        }
        break;

        case VPP_HDMI_SET_AUDIO_VUC_BITS:
        {
            VPP_BE_SetHdmiAudioVUCBits (vpp_obj, in_params[1]);
        }
        break;

        case VPP_HDMI_SEND_HOST_PKT:
        {
            VPP_BE_HdmiSendAuxPkt (vpp_obj, (VPP_HDMI_PKT_ID)in_params[1],
                                   in_params[2]);
        }
        break;

        case VPP_HDMI_STOP_HOST_PKT:
        {
            VPP_BE_HdmiStopAuxPkt (vpp_obj, in_params[1]);
        }
        break;

        case VPP_HDMI_FORCE_HDMI_HOTPLUG:
        {
            VPP_BE_HdmiTxForceHotplug(vpp_obj);
        }
        break;

        default:
            return (MV_VPP_EBADPARAM);
    }
    return (MV_VPP_OK);
}

#ifdef XVYCC_SUPPORT
/*******************************************************************
 * FUNCTION: enable\disable xvYCC
 * INPUT:  param0 - 1: enable; 0:disable
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_VPP_EBADPARAM - invalid parameters
 *******************************************************************/
int VPP_xvYCCEnable(VPP_OBJ *vpp_obj, int *in_params)
{
    if (in_params[0])
        vpp_obj->xvYCC_enable = TRUE;
    else
        vpp_obj->xvYCC_enable = FALSE;
    return (MV_VPP_OK);
}
#endif

int VPP_HDMIOutputModeSupport(VPP_OBJ *vpp_obj, int mode)
{
    int color_fmt, bit_depth;
    int rc = MV_VPP_EUNSUPPORT;
    BOOL hdr_flag = FALSE;
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    color_fmt = pHdmiTx->videoParams.VideoFmt;
    bit_depth = pHdmiTx->videoParams.ColorDepth;
    hdr_flag = (IS_DV_CertTest() ? (mode == VPP_HDMI_OUTPUT_MODE_HDR10) :
                   ((mode == VPP_HDMI_OUTPUT_MODE_HDR10) && vpp_obj->HDR_enable));

    /*output mode supported only for Dolby vision IP enabled cases*/
    if (mode > VPP_HDMI_OUTPUT_MODE_SDR2020) {
        /* mode not supported */
        rc = MV_VPP_EUNSUPPORT;
    }
    else if (((mode == VPP_HDMI_OUTPUT_MODE_DOVI) && VPP_DV_GetCapability(vpp_obj, DV_CAP_EDREN)) ||
            ((mode == VPP_HDMI_OUTPUT_MODE_SDR2020) &&
             isBT2020HDMIOutputSupported(color_fmt, bit_depth)) ||
            hdr_flag || (mode == VPP_HDMI_OUTPUT_MODE_SDR)) {
        rc = MV_VPP_OK;
    }
    else {
        rc = MV_VPP_EUNSUPPORT;
    }
    return rc;
}

/*******************************************************************
 * FUNCTION: set Luma Key
 * INPUT:  enable - enable control
 *         low_key - lower key
 *         up_key - upper key
 * RETURN: MV_VPP_OK - SUCCEED
 *******************************************************************/
int VPP_SetLumaKey(VPP_OBJ *vpp_obj, int *in_params)
{
    //FIXME: BG4CT porting - ovl to adaptive scl????
    return (MV_VPP_OK);
}


#if defined(PE_3D_BD)
int VPP_SetSSType(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID;
    int chanID;
    int cpcbID;

    for (planeID = FIRST_PLANE; planeID < MAX_NUM_PLANES; planeID++){
        /* plane is not active */
//        if (vpp_obj->plane[planeID].status == STATUS_INACTIVE)
//            continue;
        /* plane is not connected to any channel in configuration */
        if ((chanID = CHAN_OF_PLANE(vpp_obj, planeID)) == CHAN_INVALID)
            continue;
        /* plane is not connected to any CPCB in configuration */
        if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) == CPCB_INVALID)
            continue;

        if (in_params[0] & (1 << planeID)){
            if (in_params[1] & (1 << planeID)){
                vpp_obj->plane[planeID].ss_type = 1;
            }else{
                vpp_obj->plane[planeID].ss_type = 0;
            }
        }
    }
    return (MV_VPP_OK);
}

int VPP_SetSSDefaultOffset(VPP_OBJ *vpp_obj, int *in_params)
{
    vpp_obj->plane[in_params[0]].ss_offset_default = (INT8)in_params[1];

    return (MV_VPP_OK);
}

int VPP_SetSSOffsetSeqIDRef(VPP_OBJ *vpp_obj, int *in_params)
{
    vpp_obj->plane[in_params[0]].ss_offset_seq_id = in_params[1];

    return (MV_VPP_OK);
}

int VPP_SetSSOffsetMode(VPP_OBJ *vpp_obj, int *in_params)
{
    vpp_obj->plane[in_params[0]].ss_offset_mode = in_params[1];

    return (MV_VPP_OK);
}

#endif
/****************************************************************************
 * FUNCTION: set GFX0/PG plane input mode
 * INPUT: planeID - id of the plane
 *        mode - GFX_INPUT_MODE_NORMAL, GFX_INPUT_MODE_MATTE
 * RETURN: MV_VPP_OK - SUCCEED
 ***************************************************************************/
int VPP_SetGfxInputMode(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];

    if (planeID == PLANE_GFX1)
    {
        vpp_obj->scl.gfxInputMode0 = in_params[1];
        FRC_SCL_SetAlphaDivider(vpp_obj, VPP_FRC_SCL_OSD, in_params[1]);
    }
    return (MV_VPP_OK);
}

/****************************************************************
 * FUNCTION: write register group
 *****************************************************************/
int VPP_WriteRegGroup(VPP_OBJ *vpp_obj, int *in_params)
{
    UINT32 RegAddr = 0, i;
    //update register local copy also
    for (i = 0; i < in_params[0]; i++)
    {
        RegAddr = (UINT32)in_params[(i << 1) + 1 + 1];
        RegAddr = RegAddr - 0xF7F60000;
        RegAddr = RegAddr >> 2;
    }


    return VPP_BCMBUF_WriteBlock(vpp_obj->pVbiBcmBuf,
            (UINT32 *)(in_params + 1),
            (in_params[0] << 1) * sizeof(int));
}

/***************************************************
 * FUNCTION: Read SCL Coeff LUTs
 * PARAMS:  planeID - plane to mute/un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int VPP_ReadSclCoeff(VPP_OBJ *vpp_obj, int *in_params)
{
    int planeID = in_params[0];
    uintptr_t paddr = 0;
    char *paramsOffset = NULL;
    INT16 (* hlut)[8] = NULL;
    INT16 (* vlut)[8] = NULL;

    paramsOffset = (char *)(&in_params[1]);
    VPP_Memcpy(&paddr, paramsOffset, sizeof(uintptr_t));
    hlut = (INT16 (*)[8])paddr;
    paramsOffset += sizeof(uintptr_t);

    VPP_Memcpy(&paddr, paramsOffset, sizeof(uintptr_t));
    vlut = (INT16 (*)[8])paddr;
    paramsOffset += sizeof(uintptr_t);

    FRC_SCL_ReadSclCoeff(vpp_obj, planeID, hlut, vlut);
    FRC_SCL_PrintSclCoeffShadow(vpp_obj, planeID, hlut, vlut);

    return (MV_VPP_OK);
}

/***************************************************
 * FUNCTION: Write SCL Coeff LUTs
 * PARAMS:  planeID - plane to mute/un-mute
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_ENODEV - no device
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 ***********************************************/
int VPP_WriteSclCoeff(VPP_OBJ *vpp_obj, int *in_params)
{
    int UnitNum = in_params[0];

    /* To force same coeff reload, make current index as invalid */
    vpp_obj->hscl_coeff[UnitNum] = -1; // invalid
    vpp_obj->vscl_coeff[UnitNum] = -1; // invalid

    FRC_SCL_SetSclCoeff(vpp_obj, UnitNum, &vpp_obj->scl_res[UnitNum], &vpp_obj->scl_ctrl[UnitNum]);

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: Write the val to addr on interrupt
 * INPUT: addr - address to which needs to be write
 *        val  - value to be written
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_RegWrite(VPP_OBJ *vpp_obj, int *in_params)
{
    unsigned int addr, val;

    addr = in_params[0];
    val  = in_params[1];
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, addr, val);

    return (MV_VPP_OK);
}

int VPP_DumpRegs(VPP_OBJ *vpp_obj, int *in_params)
{
    VPP_DEBUG_DumpRegs(vpp_obj->base_addr, VPP_DEBUG_REG_DUMP_ENABLE_ALL);
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION: Enable/disable dynamic clock gating if IPs not used or softbypass mode
 * INPUT: enable - enable/disable value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_EUNSUPPORT - plane not connected in configuration
 *         MV_ECMDQFULL - command queue is full
 ******************************************************************************/
int VPP_enClkGate(VPP_OBJ *vpp_obj, int *in_params)
{
    VPP_ClkGate_Enable(vpp_obj, in_params[0]);
    return (MV_VPP_OK);
}

int VPP_SetThdrVitmCmd(VPP_OBJ *vpp_obj, int *in_params)
{
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Load/Reload SCL Coeffs
 * INPUT: in_params - contain cmd
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetSclCoeffs(VPP_OBJ *vpp_obj, int *in_params)
{
    int RetStatus=0;
    switch(in_params[0])
    {
        case VPP_CMD_SCL_LOAD_CUST_COEFFS:
            {
                INT coeffID = in_params[1];
                INT hv = in_params[2];
                INT UnitNum = in_params[3];
                PLANE *plane = &vpp_obj->plane[PLANE_MAIN];

                VPPAPI_DBG_INFO("enter \n");

                if (UnitNum == VPP_FRC_SCL_MAIN || UnitNum == VPP_FRC_SCL_VIP_Y || UnitNum == VPP_FRC_SCL_VIP_UV)
                    plane = &vpp_obj->plane[PLANE_MAIN];
#ifdef ENABLE_PIP_GFX0_PATH //TODO: Use static table to disable plane
                else if (UnitNum == VPP_FRC_SCL_PIP)
                    plane = &vpp_obj->plane[PLANE_PIP];
#endif
                else if (UnitNum == VPP_FRC_SCL_OSD)
                    plane = &vpp_obj->plane[PLANE_GFX1];
                else
                    ASSERT(0);

                VPPAPI_DBG_INFO("enter \n");

                RetStatus = Customize_Coefficients(vpp_obj, in_params[1] /*coeffID */, in_params[2]/* hv */, in_params[3] /*UnitNum */, TRUE);

                if((plane->status == STATUS_DISP_VIDEO || plane->status == STATUS_DISP_LOGO) && (
#if defined(SCL_DEBUG)
                            coeffID == 0xFF ||
#endif
                            (hv == 0 && coeffID == vpp_obj->hscl_coeff[UnitNum]) || (hv == 1 && coeffID == vpp_obj->vscl_coeff[UnitNum])))
                {
                    if(hv == 0)
                        vpp_obj->hscl_coeff_update[UnitNum] = 1;
                    else if (hv == 1)
                        vpp_obj->vscl_coeff_update[UnitNum] = 1;

                    vpp_obj->scl_coeff_update[UnitNum] = 1;
                }
            }
            break;
        case VPP_CMD_SCL_RELOAD_DEF_COEFFS:
            {
                INT coeffID = in_params[1];
                INT hv = in_params[2];
                INT UnitNum = in_params[3];
                PLANE *plane = &vpp_obj->plane[PLANE_MAIN];


                if (UnitNum == VPP_FRC_SCL_MAIN || UnitNum == VPP_FRC_SCL_VIP_Y || UnitNum == VPP_FRC_SCL_VIP_UV)
                    plane = &vpp_obj->plane[PLANE_MAIN];
#ifdef ENABLE_PIP_GFX0_PATH //TODO: Use static table to disable plane
                else if (UnitNum == VPP_FRC_SCL_PIP)
                    plane = &vpp_obj->plane[PLANE_PIP];
#endif
                else if (UnitNum == VPP_FRC_SCL_OSD)
                    plane = &vpp_obj->plane[PLANE_GFX1];
                else
                    ASSERT(0);

                RetStatus = Customize_Coefficients(vpp_obj, in_params[1] /*coeffID */, in_params[2]/* hv */, in_params[3] /*UnitNum */, FALSE);

                if((plane->status == STATUS_DISP_VIDEO || plane->status == STATUS_DISP_LOGO) && (
#if defined(SCL_DEBUG)
                            coeffID == 0xFF ||
#endif
                            (hv == 0 && coeffID == vpp_obj->hscl_coeff[UnitNum]) || (hv == 1 && coeffID == vpp_obj->vscl_coeff[UnitNum])))
                {
                    if(hv == 0)
                        vpp_obj->hscl_coeff_update[UnitNum] = 1;
                    else if (hv == 1)
                        vpp_obj->vscl_coeff_update[UnitNum] = 1;

                    vpp_obj->scl_coeff_update[UnitNum] = 1;
                }
            }
            break;

        default:
            {
            }
            break;
    }

    return RetStatus;
}

int VPP_ConfigInverseScan(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
    UINT32 uiPlaneId = 0, uiInvScanMode = 0;
    PLANE *plane = NULL;

    uiPlaneId = in_params[0];
    uiInvScanMode = in_params[1];

    plane = &(vpp_obj->plane[uiPlaneId]);

    plane->uchInverseScanMode = uiInvScanMode;
    plane->uchInverseScanModeUpdated = 1;
#endif
    return MV_VPP_OK;

}

int VPP_SetRotationAngle(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef CONFIG_AMP_VPP_ROTATION
    int iPlaneId = in_params[0];
    int iRotationAngle = in_params[1];
#ifdef VPP_FORCE_DISABLE_ROTATION_FOR_COMPRESSED_FRAME
    vpp_obj->plane[iPlaneId].uchUserRotationAngle = iRotationAngle;
#endif
    vpp_obj->plane[iPlaneId].uchRotationAngle = iRotationAngle;
    vpp_obj->plane[iPlaneId].uchRotationAngleUpdate = 1;
#endif
    return 0;
}

int VPP_EnableDvDump(VPP_OBJ *vpp_obj, int *in_params)
{
    vpp_obj->m_uchDvEnableDump = in_params[0];
    return 0;
}

int VPP_SetDvDataDump(VPP_OBJ *vpp_obj, int *in_params)
{
    int iPath = in_params[0];
    int iEnable = in_params[1];

    if((vpp_obj->m_uchDvTestPathMode != iEnable) || (vpp_obj->m_uchDvTestPath != iPath))
    {
        //Accept the command only if their is change in operation - enable/disable
        //Note: we cannot enable/disable during playback
        vpp_obj->m_uchDvTestPathMode = iEnable;
        vpp_obj->m_uchDvTestPath = iPath;
        vpp_obj->m_uchDvTestPathModeUpdate = 1;
    }

    return 0;
}

int VPP_ClearDhubCommand(VPP_OBJ *vpp_obj, int *in_params)
{
    int cpcb = in_params[0];
    int chanID, cpcbID;
    PLANE *plane = NULL;
    int Count = 0;

    //TODO: Use static table to disable the plane
#ifdef ENABLE_GFX2_VOP2_PATH
    for(Count = 0; Count <= PLANE_GFX2; Count++)
#else
    for(Count = 0; Count <= PLANE_GFX1; Count++)
#endif
    {
        if ((chanID = CHAN_OF_PLANE(vpp_obj, Count)) == CHAN_INVALID){
            /* plane is not connected to any channel in configuration */
            continue;
        }
        if ((cpcbID = CPCB_OF_CHAN(vpp_obj, chanID)) != cpcb) {
            /* plane is not connected to any CPCB in configuration */
            continue;
        }
        plane = &(vpp_obj->plane[Count]);
        if(STATUS_INACTIVE == plane->status || STATUS_INACTIVE_PENDING == plane->status)
        {
            continue;
        }

        if(SRCFMT_YUV420SP == plane->srcfmt || SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 ==  plane->srcfmt)
        {
            VPPAPI_DBG_INFO("Clearing Dhub channels 0x%x and 0x%x of plane[%d] cpcbID[%d]\n", plane->dmaRID[0], plane->dmaRID[1],Count,cpcbID);
            CLEAR_2NDDMA(plane->dmaRdhubID, plane->dmaRID[0], (VIP_BCMBUF *)vpp_obj->pVbiClearBcmBuf);
            CLEAR_2NDDMA(plane->dmaRdhubID, plane->dmaRID[1], (VIP_BCMBUF *)vpp_obj->pVbiClearBcmBuf);
            if(VPP_DV_GetDvCompMode(vpp_obj))
            {
                CLEAR_2NDDMA(plane->dmaRdhubID, plane->dmaRID[2], (VIP_BCMBUF *)vpp_obj->pVbiClearBcmBuf);
                CLEAR_2NDDMA(plane->dmaRdhubID, plane->dmaRID[3], (VIP_BCMBUF *)vpp_obj->pVbiClearBcmBuf);
            }
        }
        else
        {
            VPPAPI_DBG_INFO("Clearing Dhub channel [%d] plane[%d] cpcbID[%d]\n", plane->dmaRID[0], Count,cpcbID);
            CLEAR_2NDDMA(plane->dmaRdhubID, plane->dmaRID[0], (VIP_BCMBUF *)vpp_obj->pVbiClearBcmBuf);
        }
    }
    return 0;
}

int VPP_SetDcmPQ(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef CONFIG_AMP_DCM
    SetDcmPQValues(vpp_obj, in_params[0], in_params[1], in_params[2], in_params[3]);
#endif

    return MV_VPP_OK;
}

int VPP_SetDcmAoiWin(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef CONFIG_AMP_DCM
    int planeID = in_params[0];
    int iAoiEn = in_params[1];
    PLANE *plane;

    plane = &(vpp_obj->plane[planeID]);

    if( plane->m_uiDcmAoiEnable !=  in_params[1] ||
            plane->m_stDcmAoiWin.x != in_params[2] ||
            plane->m_stDcmAoiWin.y != in_params[3] ||
            plane->m_stDcmAoiWin.width != in_params[4] ||
            plane->m_stDcmAoiWin.height != in_params[5])
    {
        plane->m_stDcmAoiWin.x = in_params[2];
        plane->m_stDcmAoiWin.y = in_params[3];
        plane->m_stDcmAoiWin.width = in_params[4];
        plane->m_stDcmAoiWin.height = in_params[5];
        plane->m_uiDcmAoiEnable = iAoiEn;

        plane->m_uiDcmAoiWinUpdated = 1;
    }
#endif

    return 0;
}

void VPP_SetColorTransformGain(VPP_OBJ *vpp_obj, int *in_params)
{
    VPPAPI_DBG_INFO("SetCT[r:%d][g:%d][b:%d]\n", in_params[0], in_params[1], in_params[2]);

    vpp_obj->clrtemp.Rgain = (float)VPP_OCSC_GAIN_INT2FLOAT(in_params[0]);
    vpp_obj->clrtemp.Ggain = (float)VPP_OCSC_GAIN_INT2FLOAT(in_params[1]);
    vpp_obj->clrtemp.Bgain = (float)VPP_OCSC_GAIN_INT2FLOAT(in_params[2]);

    VPPAPI_DBG_INFO("SetCT[%.7g][%.7g][%.7g]\n", vpp_obj->clrtemp.Rgain, \
        vpp_obj->clrtemp.Ggain, vpp_obj->clrtemp.Bgain);

    vpp_obj->videofmtUpdated = 1;
}

UINT32 VPP_GetGcsc(VPP_OBJ *vpp_obj, int *in_params)
{
   VPP_CHANNEL_TYPE cscChannel;
   UINT32 mode, uvalue;
   float fvalue;

   cscChannel = in_params[0];
   mode = in_params[1];

    switch(mode)
    {
        case PICTURE_CTRL_CONTRAST:
            fvalue = VPP_OCSC_CONTRAST_FLOAT2INT(vpp_obj->gContrast[cscChannel]);
        break;

        case PICTURE_CTRL_HUE:
            fvalue = VPP_OCSC_HUE_FLOAT2INT(vpp_obj->gHue[cscChannel]);
        break;

        case PICTURE_CTRL_SATURATION:
            fvalue = VPP_OCSC_SATURATION_FLOAT2INT(vpp_obj->gSaturation[cscChannel]);
        break;

        case PICTURE_CTRL_BRIGHTNESS:
            fvalue = VPP_OCSC_BRIGHTNESS_FLOAT2INT(vpp_obj->gBrightness[cscChannel]);
        break;

        default:
            fvalue = 0xdeadbeef;
            return (MV_VPP_EBADPARAM);
    }
        uvalue = (UINT32)(fvalue+0.5);

   return (uvalue);
}

int VPP_SetOfflineParams(VPP_OBJ *vpp_obj, int *in_params)
{
    UINT32 uiPlaneId = in_params[0];

    vpp_obj->plane[uiPlaneId].offline_scl_bpp = in_params[1];
    vpp_obj->plane[uiPlaneId].offline_scl_srcfmt = in_params[2];

    VPPAPI_DBG_INFO("Offline SCL:plane = %d, bpp = %d, srcfmt =  %d \n",uiPlaneId,vpp_obj->plane[uiPlaneId].offline_scl_bpp,vpp_obj->plane[uiPlaneId].offline_scl_srcfmt);

    return 0;
}

/********************************************************************************
 * FUNCTION: set GDet mode
 * INPUT:  mode -
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetGDetParams(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef ENABLE_DETEE
    return VPP_DETEE_SetGDetParamsMode(vpp_obj, in_params[0]);
#else
    return (MV_VPP_OK);
#endif
}

/********************************************************************************
 * FUNCTION: set EE mode
 * INPUT:  mode -
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetEEParams(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef ENABLE_EECMU
    if (VPP_CMU_EE_NEW_LTI_CTI_WITH_CUSTOM_GAIN_CONTROL == in_params[0]) {
        vpp_obj->cmu.lti_gain = in_params[1];
        vpp_obj->cmu.cti_gain = in_params[2];
    }
    return CMU_EE_SetEEParamsMode(vpp_obj, in_params[0]);
#else
    return (MV_VPP_OK);
#endif
}

/********************************************************************************
 * FUNCTION: Set ACE Param
 * INPUT: in_params - contain cmd and DS contain parameter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetAceEvalCommand(VPP_OBJ *vpp_obj, int *in_params)
{
        int RetStatus=0;
#ifdef ENABLE_EECMU
        switch(in_params[0])
        {
                case VPP_CMD_ACE_SET_ENABLE:
                {
                        RetStatus = VPP_CMU_ACE_Enable(vpp_obj,in_params[1],vpp_obj->cmu.ACEParam.Params.Enable);
                }
                break;
                case VPP_CMD_ACE_SET_MODE:
                {
                        RetStatus = VPP_CMU_ACE_SetParams(vpp_obj,in_params[1],&(vpp_obj->cmu.ACEParam.Params),vpp_obj->cmu.ACEParam.Range,vpp_obj->cmu.ACEParam.Mode,1);
                }
                break;
                case VPP_CMD_ACE_SET_PARAM:
                {
                        RetStatus = VPP_CMU_ACE_SetParams(vpp_obj,in_params[1],&(vpp_obj->cmu.ACEParam.Params),vpp_obj->cmu.ACEParam.Range,vpp_obj->cmu.ACEParam.Mode,0);
                }
                break;
                case VPP_CMD_ACE_SET_CLR_LEVEL:
                {
                        RetStatus = VPP_CMU_ACE_SetColorCompensationLevel(vpp_obj,in_params[1],vpp_obj->cmu.ACEParam.Params.ColorCompensationLevel);
                }
                break;
                case VPP_CMD_ACE_SET_WFACTOR_1:
                {
                        RetStatus = VPP_CMU_ACE_SetWeightingFactor(vpp_obj,in_params[1],VPP_CMU_ACE_REGION1,vpp_obj->cmu.ACEParam.Params.WeightingFactor1);
                }
                break;
                case VPP_CMD_ACE_SET_WFACTOR_2:
                {
                        RetStatus = VPP_CMU_ACE_SetWeightingFactor(vpp_obj,in_params[1],VPP_CMU_ACE_REGION2,vpp_obj->cmu.ACEParam.Params.WeightingFactor2);
                }
                break;
                case VPP_CMD_ACE_SET_WFACTOR_3:
                {
                        RetStatus = VPP_CMU_ACE_SetWeightingFactor(vpp_obj,in_params[1],VPP_CMU_ACE_REGION3,vpp_obj->cmu.ACEParam.Params.WeightingFactor3);
                }
                break;
                case VPP_CMD_ACE_SET_THRSLD_0:
                {
                        RetStatus = VPP_CMU_ACE_SetThreshHold(vpp_obj,in_params[1],VPP_CMU_ACE_THOLD0,vpp_obj->cmu.ACEParam.Params.ThresHold0);
                }
                break;
                case VPP_CMD_ACE_SET_THRSLD_1:
                {
                        RetStatus = VPP_CMU_ACE_SetThreshHold(vpp_obj,in_params[1],VPP_CMU_ACE_THOLD1,vpp_obj->cmu.ACEParam.Params.ThresHold1);
                }
                break;
                case VPP_CMD_ACE_SET_THRSLD_2:
                {
                        RetStatus = VPP_CMU_ACE_SetThreshHold(vpp_obj,in_params[1],VPP_CMU_ACE_THOLD2,vpp_obj->cmu.ACEParam.Params.ThresHold2);
                }
                break;
                case VPP_CMD_ACE_SET_THRSLD_3:
                {
                        RetStatus = VPP_CMU_ACE_SetThreshHold(vpp_obj,in_params[1],VPP_CMU_ACE_THOLD3,vpp_obj->cmu.ACEParam.Params.ThresHold3);
                }
                break;
                case VPP_CMD_ACE_SET_THRSLD_4:
                {
                        RetStatus = VPP_CMU_ACE_SetThreshHold(vpp_obj,in_params[1],VPP_CMU_ACE_THOLD4,vpp_obj->cmu.ACEParam.Params.ThresHold4);
                }
                break;
                case VPP_CMD_ACE_SET_THRSLD_5:
                {
                        RetStatus = VPP_CMU_ACE_SetThreshHold(vpp_obj,in_params[1],VPP_CMU_ACE_THOLD5,vpp_obj->cmu.ACEParam.Params.ThresHold5);
                }
                break;
                case VPP_CMD_ACE_SET_OUTPUT_0:
                {
                        RetStatus = VPP_CMU_ACE_SetOutput(vpp_obj,in_params[1],VPP_CMU_ACE_OUTPUT0,vpp_obj->cmu.ACEParam.Params.OutPut0);
                }
                break;
                case VPP_CMD_ACE_SET_OUTPUT_5:
                {
                        RetStatus = VPP_CMU_ACE_SetOutput(vpp_obj,in_params[1],VPP_CMU_ACE_OUTPUT5,vpp_obj->cmu.ACEParam.Params.OutPut5);
                }
                break;
                case VPP_CMD_ACE_SET_GUARDBAND_2:
                {
                        RetStatus = VPP_CMU_ACE_SetGuardBand(vpp_obj,in_params[1],VPP_CMU_ACE_GUARDBAND2,vpp_obj->cmu.ACEParam.Params.GuardBand2);
                }
                break;
                case VPP_CMD_ACE_SET_GUARDBAND_3:
                {
                        RetStatus = VPP_CMU_ACE_SetGuardBand(vpp_obj,in_params[1],VPP_CMU_ACE_GUARDBAND3,vpp_obj->cmu.ACEParam.Params.GuardBand3);
                }
                break;
                case VPP_CMD_ACE_SET_MAX_SLOPE_1:
                {
                        RetStatus = VPP_CMU_ACE_SetMaxSlope(vpp_obj,in_params[1],VPP_CMU_ACE_MAXSLOPE1,vpp_obj->cmu.ACEParam.Params.MaximumSlope1);
                }
                break;
                case VPP_CMD_ACE_SET_MAX_SLOPE_2:
                {
                        RetStatus = VPP_CMU_ACE_SetMaxSlope(vpp_obj,in_params[1],VPP_CMU_ACE_MAXSLOPE2,vpp_obj->cmu.ACEParam.Params.MaximumSlope2);
                }
                break;
                case VPP_CMD_ACE_SET_MAX_SLOPE_3:
                {
                        RetStatus = VPP_CMU_ACE_SetMaxSlope(vpp_obj,in_params[1],VPP_CMU_ACE_MAXSLOPE3,vpp_obj->cmu.ACEParam.Params.MaximumSlope3);
                }
                break;
                default:
                {
                }
                break;
        }
#endif
        return RetStatus;
}

/********************************************************************************
 * FUNCTION: Set FTDC Param
 * INPUT: in_params - contain cmd and DS contain parameter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetFTDCEvalCommand(VPP_OBJ *vpp_obj, int *in_params)
{
    int RetStatus=0;
#ifdef ENABLE_EECMU
    RetStatus = VPP_CMU_FTDC_SetParams(vpp_obj, &(vpp_obj->cmu.FTDCParam));
#endif
    return RetStatus;
}

/********************************************************************************
 * FUNCTION: Set Ee Param
 * INPUT: in_params - contain cmd and DS contain parameter
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int VPP_SetEeEvalCommand(VPP_OBJ *vpp_obj, int *in_params)
{

    int RetStatus=0;

#ifdef ENABLE_DETEE
    switch(in_params[0])
    {
    case VPP_CMD_DETEE_GDET_CTRL:
    {
        VPP_DETEE_SetGDetCtrl(vpp_obj, (PVPP_DETEE_GDET_CTRL)&(vpp_obj->detee.DETEEGDetCtrl));
    }
    break;
    case VPP_CMD_DETEE_GDET_PARAMS:
    {
        VPP_DETEE_SetGDetParams(vpp_obj, (PVPP_DETEE_GDET_PARAMS)&(vpp_obj->detee.DETEEGDetParam));
    }
    break;
    default:
    {
    }
    break;
    }
#endif

    return RetStatus;
}

/*******************************************************************
 * FUNCTION: set CMU control
 * INPUT: chanID - id of the channel
 *        CtrlSel - CMU control selection
 *        CtrlVal - CMU control value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_VPP_EBADCALL - channel is not active
 *         MV_VPP_EBADPARAM - invalid parameters
 *******************************************************************/
int VPP_SetCmuControl(VPP_OBJ *vpp_obj, int *in_params)
{
#ifdef ENABLE_EECMU
    int chanID = in_params[0];

    switch(in_params[1]){
        case VPP_CMU_SET_ACE:
            vpp_obj->chan[chanID].aceLevel = in_params[2];
            VPP_CMU_SetAceLevel(vpp_obj, chanID, in_params[2], in_params[3]);
            break;

        case VPP_CMU_SET_BREN:
            vpp_obj->chan[chanID].brLevel = in_params[2];
            VPP_CMU_SetBrightnessLevel(vpp_obj, chanID, in_params[2]);
            break;

        case VPP_CMU_SET_CLRSAT:
            //For Sat VPP_CMU_COLOR_ALL, it set globle saturation change
            vpp_obj->chan[chanID].colorLevel[in_params[2]] = in_params[3];
            VPP_CMU_SetColorLevel(vpp_obj, chanID, in_params[2], in_params[3]);
            break;

        case VPP_CMU_SET_CLRHUE:
            //For Hue VPP_CMU_COLOR_ALL, it set all individual color hue
            if(in_params[2] == VPP_CMU_COLOR_ALL)
            {
                int i;

                for(i=0; i<VPP_CMU_COLOR_ALL; i++)
                    vpp_obj->chan[chanID].colorHue[i] = in_params[3];
            }
            else
                vpp_obj->chan[chanID].colorHue[in_params[2]] = in_params[3];

            VPP_CMU_SetColorHue(vpp_obj, chanID, in_params[2], in_params[3]);
            break;

        case VPP_CMU_SET_ICR:
            vpp_obj->chan[chanID].icrMode = in_params[2];
            VPP_CMU_SetIcrMode(vpp_obj, chanID, in_params[2]);
            break;

        case VPP_CMU_SET_FTDC:
            vpp_obj->chan[chanID].ftdcMode = in_params[2];
            VPP_CMU_SetFtdcMode(vpp_obj, chanID, in_params[2]);
            break;
        case VPP_CMU_SET_FT_TAPER:
            vpp_obj->chan[chanID].enFTTaper = in_params[2];
            VPP_CMU_EnableFleshToneTaper(vpp_obj, chanID, in_params[2]);
            break;
        default :
            return (MV_VPP_EBADPARAM);
    }
#endif
    return (MV_VPP_OK);
}


int VPP_PrintGamma(VPP_OBJ *vpp_obj, int *in_params )
{

    VPP_BE_PrintGamma(vpp_obj, in_params[0],in_params[1]);

    return (MV_VPP_OK);
}

int VPP_SetGamma(VPP_OBJ *vpp_obj, int *in_params)
{

    VPP_BE_SetGamma(vpp_obj, in_params[0],in_params[1]);

    return (MV_VPP_OK);

}

int VPP_SetGammaEnable(VPP_OBJ *vpp_obj, int *in_params)
{

    VPP_BE_SetGammaEnable(vpp_obj, in_params[0],in_params[1]);

    return (MV_VPP_OK);

}

void VPP_ResetPlane(VPP_OBJ *vpp_obj, unsigned int mode)
{
    VPP_ReConfigPath(vpp_obj, mode);
    VPP_CPCB_ResetPlane(vpp_obj, mode_to_chan[mode]);
}

/*
 * Reconfigures the plane to CPCB mapping *
 * Reset TG's before calling this function*
 */
int VPP_ReConfigPath(VPP_OBJ *vpp_obj, unsigned int uiDisplayMode)
{
    int i, j;

    /* config channels */
    for(i = FIRST_CHAN; i < MAX_NUM_CHANS; i++) {
        /* set DV ID this channel belongs to */
        vpp_obj->chan[i].dvID = (i <= CHAN_GFX2) ? chan_to_cpcb[i][uiDisplayMode]: CPCB_1;
    } // <- config channels

    /* config DVs */
    for(i = FIRST_CPCB; i < MAX_NUM_CPCBS; i++) {
        /* set channel IDs which belong to this DV */
        vpp_obj->dv[i].num_of_chans = 0;
        for(j = FIRST_CHAN; j < MAX_NUM_CHANS; j++){
            if (vpp_obj->chan[j].dvID == i)
                vpp_obj->dv[i].chanID[vpp_obj->dv[i].num_of_chans++] = j;
        }

        /* set VOUT IDs which belong to this DV */
        vpp_obj->dv[i].num_of_vouts = 0;
        for(j = FIRST_VOUT; j < MAX_NUM_VOUTS; j++){
            if (vout_to_cpcb[j][uiDisplayMode] == i)
                vpp_obj->dv[i].voutID[vpp_obj->dv[i].num_of_vouts++] = j;
        }
#if defined(PE_3D_BD)
        // initialize view ID
        vpp_obj->dv[i].viewID = 0;
#endif
    } // <- config DVs

    /* config VOUTs */
    for(i = FIRST_VOUT; i < MAX_NUM_VOUTS; i++) {
        /* set DV ID this VOUT belongs to */
        vpp_obj->vout[i].dvID = vout_to_cpcb[i][uiDisplayMode];
    } // <- config VOUTs

    return (MV_VPP_OK);
}

int VPP_AxiLatencyMeterParams(VPP_OBJ *vpp_obj, int *in_params)
{
    unsigned int uiEnable = in_params[0], uiCaptureCycles = in_params[1], uiHistWidth = in_params[2],    uiHistHeight = in_params[3];
    unsigned int uiNumOfFrames = in_params[4];

    vpp_obj->stAxiLmeter.uiCaptureCycles = uiCaptureCycles;
    vpp_obj->stAxiLmeter.uiHistWidth = uiHistWidth;
    vpp_obj->stAxiLmeter.uiHistHeight = uiHistHeight;
    vpp_obj->stAxiLmeter.uiEnable = uiEnable;
    vpp_obj->stAxiLmeter.uiNumOfFrames = uiNumOfFrames;
    vpp_obj->stAxiLmeter.uiUpdateRequired = 1;

    return 0;
}

VBUF_INFO *VPP_GetFrame(PLANE *plane)
{
    VDEC_VID_BUF_DESC *inline_frame;
    VBUF_INFO *pinfo;

    if (plane->status == STATUS_DISP_VIDEO)
        inline_frame = (void*)plane->curr_frame;
    else if (plane->status == STATUS_DISP_LOGO)
        inline_frame = (void*)plane->curr_still_picture;
    else if (plane->status == STATUS_DISP_PATGEN)
        inline_frame = (void*)plane->dummy_frame;
    else
        inline_frame = NULL;

#ifndef CONFIG_AMP_TEE_ENABLE
    if(inline_frame)
        pinfo = (VBUF_INFO *)inline_frame->user_data;
#else
    pinfo = (VBUF_INFO *)inline_frame;
#endif

    return pinfo;
}

int GetVCOFreq(int resID, int bit_depth)
{
    int avpll_freq_index, deep_color_index;
    RESOLUTION_INFO  *pResinfo;

    if (resID == RES_RESET)
        return (-1);

    pResinfo = VPP_GetResInfo(resID);
    if ((pResinfo->freq >= 25175) && (pResinfo->freq <= 593406)){
        avpll_freq_index = 5;
        deep_color_index = 0;
    }
    else
        return (-1);

#if defined(VPP_DIAGS)
    return (diag_getVCOFreq(deep_color_index, avpll_freq_index));
#else
    return (AVPLL_GetVcoFreq(deep_color_index, avpll_freq_index));
#endif
}

#ifdef ENABLE_GFX2_VOP2_PATH
static void VPP_SetVop2Clk (VPP_OBJ *vpp_obj, int cpcbID, unsigned int dp2_freq)
{
    dp2_freq *= 1000;
    if ((cpcbID == CPCB_2) || ((cpcbID == CPCB_1) && IS_VOUT_DISP_MODE(SINGLE_MODE_SEC)))
    {
        AVPLL_SetClockFreq(AVPLL_A, 7, dp2_freq, 2);
    }
}
#endif

void VPP_SetClk(VPP_OBJ *vpp_obj, int cpcbID, int resID)
{
    RESOLUTION_INFO  *pResinfo;

    pResinfo = VPP_GetResInfo(resID);

#if !defined(FPGA)
    unsigned int dp1_freq;
    unsigned int clk_divider;

    dp1_freq = pResinfo->freq * 1000;

    if(resID == RES_DSI_1920x1080P5994)
        dp1_freq = pResinfo->freq * 1000;

    /* for interlaced frame divide the clock by 2 */
    clk_divider = (pResinfo->scan)? 2 : 1;

    if (cpcbID == CPCB_1) {
        AVPLL_SetClockFreq(AVPLL_A, 7, dp1_freq / clk_divider,1);
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    VPP_SetVop2Clk(vpp_obj, cpcbID, pResinfo->freq);
#endif /* ENABLE_GFX2_VOP2_PATH */
#endif
}
