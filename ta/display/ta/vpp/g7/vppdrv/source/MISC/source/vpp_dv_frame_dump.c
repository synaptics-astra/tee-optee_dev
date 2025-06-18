/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_dv_frame_dump.c
//! \brief      The VPP DV frame dump framework source file
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_dv_frame_dump.h"
#include "vpp_cfg.h"
#include "vpp_fe_dlr.h"
#include "vpp_isr.h"

/******* vpp frame dump APIs ********/
/*********************************************************
 * FUNCTION: Initialize the VPP dump framework
 * PARAMS: *pDumpConfig - pointer to a VPP DUMP Config structure
 *
 ********************************************************/
#ifdef CONFIG_AMP_DOLBY_DUMP
void VPP_DV_FrameDump_Init(VPP_OBJ *vpp_obj, VPP_DV_FRAME_DUMP *pDumpConfig) {
    int i;

    if(!vpp_obj->m_uchDvEnableDump) {
        return;
    }

    //Copy the configuration to local variable
    vpp_obj->vpp_fd_config = *pDumpConfig;
    vpp_obj->vpp_fd_init = 1;
    vpp_obj->vpp_fd_consumedLastFrame = 0;

    //Initialize the local configuration variable to default state
    vpp_obj->vpp_fd_config.currentNdx   = 0;
    vpp_obj->vpp_fd_config.nextNdx      = 0;

    for(i=0; i<VPP_DV_FRAME_DUMP_MAX_USED_BITS_WORDS; i++) {
        vpp_obj->vpp_fd_config.usedFrames[i]   = 0;
    }

    //Skip frame is decremented and to maintain uniformity b/w New/Curr frame; increament skipFrame by 1
    if(vpp_obj->vpp_fd_config.skipFrames)
        vpp_obj->vpp_fd_config.skipFrames++;
}

/*********************************************************
 * FUNCTION: Get the Configuration and status of VPP dump framework
 * PARAM: *pDumpConfig - pointer to a VPP DUMP Config/Status structure
 * RETURN:  1: VPP frame dump module is initilaized, 0: not initialized
 ********************************************************/
int VPP_DV_FrameDump_GetConfig(VPP_OBJ *vpp_obj, VPP_DV_FRAME_DUMP *pDumpConfig) {

    if(vpp_obj->m_uchDvEnableDump) {
        *pDumpConfig = vpp_obj->vpp_fd_config;
    }
    return vpp_obj->vpp_fd_init;
}

/*********************************************************
 * FUNCTION: Move the ndx and get the next/new buffer for dumping
 * PARAMS: vpp_obj - VPP object containing plane details - offline buffer
 * RETURN: phy-address of the next/new dump buffer
 * NOTE: 1. When ndx is 0, then we may have to do VOP RESET/FE_DLR_VopRst()
 *          for DITHER dump path
 ********************************************************/
unsigned int VPP_DV_FrameDump_GetNewFrame(VPP_OBJ *vpp_obj) {

    if(vpp_obj->vpp_fd_init) {
        ++vpp_obj->vpp_frame_count;
        //Please skip any frames first
        if(vpp_obj->vpp_fd_config.skipFrames) {
            vpp_obj->vpp_fd_config.skipFrames--;
        }

        //if all frames are skipped, then move the indexes
        if(!vpp_obj->vpp_fd_config.skipFrames) {
            //Get the current Ndx; The Next buffer is current buffer
            vpp_obj->vpp_fd_config.currentNdx = vpp_obj->vpp_fd_config.nextNdx;

            //Move the nextNdx to next buffer
            if((vpp_obj->vpp_fd_config.nextNdx+1) >= vpp_obj->vpp_fd_config.nFrames) {
                if(vpp_obj->vpp_fd_config.overwriteFlag) {
                    //Overwrite allowed; then cycle the buffers
                    vpp_obj->vpp_fd_config.nextNdx = 0;
                } else {
                    //Overwrite not allowed and is consumed already once
                    if(VPP_DV_FRAME_DUMP_GET_USED_BIT(vpp_obj->vpp_fd_config.usedFrames, vpp_obj->vpp_fd_config.currentNdx)) {
                        vpp_obj->vpp_fd_consumedLastFrame=1;
                    }
                }
            } else {
                vpp_obj->vpp_fd_config.nextNdx++;
            }
        }
    }

    return VPP_DV_FrameDump_GetCurrFrame(vpp_obj);
}

/*********************************************************
 * FUNCTION: Get the current buffer for dumping
 * PARAMS: vpp_obj - VPP object containing plane details - offline buffer
 * RETURN: phy-address of the current dump buffer
 ********************************************************/
unsigned int VPP_DV_FrameDump_GetCurrFrame(VPP_OBJ *vpp_obj) {
    PLANE *plane = &(vpp_obj->plane[PLANE_MAIN]);
    unsigned int bufferAddr = (unsigned int)plane->offline_buff_descrs[0].addr;    //always use the 1st offline buffer

    if(vpp_obj->vpp_fd_init && !vpp_obj->vpp_fd_config.skipFrames) {
        //Don't allow overwrite, if not enable and we have consumed all frames
        //Better use offline buffer instead of overwriting dump buffer
        if((!vpp_obj->vpp_fd_config.overwriteFlag && !vpp_obj->vpp_fd_consumedLastFrame) || vpp_obj->vpp_fd_config.overwriteFlag)
        {
            bufferAddr = vpp_obj->vpp_fd_config.phyAddr + (vpp_obj->vpp_fd_config.currentNdx * vpp_obj->vpp_fd_config.frameSize);
            //Updated used flag
            VPP_DV_FRAME_DUMP_SET_USED_BIT(vpp_obj->vpp_fd_config.usedFrames, vpp_obj->vpp_fd_config.currentNdx);
        }

    }

    return bufferAddr;
}
/*********************************************************
 * FUNCTION: Overlay Screen Capture Function
 * PARAMS: vpp_obj - VPP object containing plane details - offline buffer
 ********************************************************/
void VPP_Ovl_Capture_Handler(VPP_OBJ *vpp_obj,PLANE *plane)
{
    RESOLUTION_INFO  *pResinfo;

    if(VPP_SCL_MODE_INLINE == plane->scl_mode)
    {
        VBUF_INFO *pinfo_bl = NULL;
        VDEC_VID_BUF_DESC *inline_frame;
        inline_frame = plane->curr_frame;
        if(inline_frame != NULL && !(vpp_obj->vpp_fd_config.forcecapture))
        {
#ifndef CONFIG_AMP_TEE_ENABLE
            pinfo_bl = (VBUF_INFO *)inline_frame->user_data;
#else
            pinfo_bl = (VBUF_INFO *)inline_frame;
#endif
            if(pinfo_bl->builtinFrame)
            {
                return;
            }
        }
        VOID *buf_address = (VOID*)VPP_DV_FrameDump_GetCurrFrame(vpp_obj);
        UINT32 uiWidth, uiHeight, resID;
        resID = VPP_GetResId(CPCB_OF_VOUT(vpp_obj,VOUT_HDMI));

        pResinfo = VPP_GetResInfo(resID);
        uiWidth  = pResinfo->active_width;
        uiHeight = pResinfo->active_height;
        if(vpp_obj->m_uchDvTestPathMode && (plane->offline_buff_descrs[0].addr != buf_address))
        {
            if(vpp_obj->m_uchDvTestPathModeUpdate)
            {
                if((vpp_obj->m_uchDvTestPath == VPP_WRITEBACK_MODE_DITHER) || (vpp_obj->m_uchDvTestPath == VPP_WRITEBACK_MODE_EDR))
                {
                    FE_DLR_ConfigOfflineRD(vpp_obj, uiWidth, uiHeight);
                }
                VPP_VIP_TG_SetTGParams(vpp_obj, uiWidth, uiHeight);
                configVppWriteBackMode(vpp_obj, vpp_obj->m_uchDvTestPath, uiWidth, uiHeight);
                vpp_obj->m_uchDvTestPathModeUpdate = 0;
            }
            startVppWriteBackClients(vpp_obj, vpp_obj->m_uchDvTestPath, uiWidth, uiHeight);
        }
        vpp_obj->vpp_fd_config.Errorstatus=FALSE;
    }
    else
    {
        vpp_obj->vpp_fd_config.Errorstatus=TRUE;
    }
}
#endif //CONFIG_AMP_DOLBY_DUMP
