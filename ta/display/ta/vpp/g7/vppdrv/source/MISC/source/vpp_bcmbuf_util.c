/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "vpp_module.h"
#include "vpp_debug_util.h"

void VPP_Reset_CFGQ(VPP_OBJ *vpp_obj, int cpcbID)
{
    /* current VBI BCM cfgQ  */
    vpp_obj->curr_vbi_bcm_cfgQ = vpp_obj->dv[cpcbID].curr_cpcb_vbi_bcm_cfgQ;
    /* reset vbi bcm & dma cfgQ length */
    vpp_obj->dv[cpcbID].curr_cpcb_vbi_bcm_cfgQ->len = 0;
    vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ->len = 0;
}

void VPP_Select_Buffer(VPP_OBJ *vpp_obj, int cpcbID)
{
    /* select BCM buffer for the CPCB */
    vpp_obj->pVbiBcmBuf = vpp_obj->pVbiBcmBufCpcb[cpcbID];
    /* select BCM sub-buffer to dump register settings */
    VPP_BCMBUF_Select(vpp_obj->pVbiBcmBuf, cpcbID);

    /* select CLEAR BCM buffer for the CPCB */
    vpp_obj->pVbiClearBcmBuf = vpp_obj->pVbiClearBcmBufCpcb[cpcbID];
    /* select CLEAR BCM sub-buffer to dump register settings */
    VPP_BCMBUF_Select(vpp_obj->pVbiClearBcmBuf, cpcbID);
}

void VPP_Switch_Buffer(VPP_OBJ *vpp_obj, BOOL isClearBuf, int cpcbID)
{
    /* switch CLEAR BCM buffer for the CPCB */
    if(isClearBuf) {
        if (vpp_obj->pVbiClearBcmBufCpcb[cpcbID]==&vpp_obj->vbi_clear_bcm_buf[cpcbID][0])
            vpp_obj->pVbiClearBcmBufCpcb[cpcbID]=&vpp_obj->vbi_clear_bcm_buf[cpcbID][1];
        else
            vpp_obj->pVbiClearBcmBufCpcb[cpcbID]=&vpp_obj->vbi_clear_bcm_buf[cpcbID][0];
    } else {
        if (vpp_obj->pVbiBcmBufCpcb[cpcbID] == &vpp_obj->vbi_bcm_buf[cpcbID][0])
            vpp_obj->pVbiBcmBufCpcb[cpcbID] = &vpp_obj->vbi_bcm_buf[cpcbID][1];
        else
            vpp_obj->pVbiBcmBufCpcb[cpcbID] = &vpp_obj->vbi_bcm_buf[cpcbID][0];
    }
}

void VPP_Switch_CFGQ(VPP_OBJ *vpp_obj, int cpcbID)
{
    if (vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ == &vpp_obj->dv[cpcbID].vbi_dma_cfgQ[0])
        vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ = &(vpp_obj->dv[cpcbID].vbi_dma_cfgQ[1]);
    else
        vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ = &(vpp_obj->dv[cpcbID].vbi_dma_cfgQ[0]);

    if (vpp_obj->dv[cpcbID].curr_cpcb_vbi_bcm_cfgQ == &vpp_obj->dv[cpcbID].vbi_bcm_cfgQ[0])
        vpp_obj->dv[cpcbID].curr_cpcb_vbi_bcm_cfgQ = &(vpp_obj->dv[cpcbID].vbi_bcm_cfgQ[1]);
    else
        vpp_obj->dv[cpcbID].curr_cpcb_vbi_bcm_cfgQ = &(vpp_obj->dv[cpcbID].vbi_bcm_cfgQ[0]);
}

void VPP_AutoPush_Handler(VPP_OBJ *vpp_obj, int cpcbID, int intrtype)
{
#ifdef CONFIG_VPP_AUTOPUSH_SUPPORT
    int bcm_cpcbid = (vpp_obj->m_VoutMode == VOUT_DISP_SINGLE_MODE_SEC) ? CPCB_1: cpcbID;
    //To disable autopush dyanamically, change resolution is required
    if(VPP_DEBUG_UTIL_IS_FF_ENABLED_AUTOPUSH(vpp_obj))
    {
        /*Enable autopush if not enabled*/
        if(vpp_obj->dv[bcm_cpcbid].curr_cpcb_vbi_bcm_cfgQ->len != 0
                && vpp_obj->dv[bcm_cpcbid].curr_cpcb_vbi_dma_cfgQ->len != 0)
        {
            if(vpp_obj->enable_vbibcm_autopush[cpcbID] == FALSE)
            {
                vpp_obj->enable_vbibcm_autopush[cpcbID] = TRUE;
                VPP_BCMDHUB_AutoPush(cpcbID, intrtype, 1);
            }
        }
        else
        {
            if(vpp_obj->enable_vbibcm_autopush[cpcbID] == TRUE)
            {
                vpp_obj->enable_vbibcm_autopush[cpcbID] = FALSE;
                VPP_BCMDHUB_AutoPush(cpcbID, intrtype, 0);
            }
        }
    }
#endif
}

