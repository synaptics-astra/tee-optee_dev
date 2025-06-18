/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_debug_util_fb.c
//! \brief      vpp debug utility routine for frame buffer and/or VBUFINFO
//! \brief      Currently implemented : FB_BACKUP_FIRST_BUILTIN_FRAME
//! \brief      TBD:FB_BACKUP_PRELOADED_FRAME-VBUF&Frame from preloaded location
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_module.h"
#include "vpp_debug_util.h"
#include "vpp_debug_dump.h"

//Enable/Disable MAIN plane to participate in FB backup mode
#define VPP_FB_BACKUP_MODE_MAIN_ENABLE  1
//Enable/Disable GFX (GFX0/GFX1) plane to participate in FB backup mode
#define VPP_FB_BACKUP_MODE_GFX_ENABLE   1

//Enable to backup only the 1st built-in frame shared with the driver
//Otherwise backup any 1st frame - note GFX plane are given non-displayable frame from DISPSRV as 1st frame
//Enabling this will wait for built-in frame to be pushed to respective plane
#define VPP_FB_BACKUP_FIRST_BUILTIN_FRAME    1
//Show backup frame for GFX only after pushing non-builtin frame to MAIN; i.e after playback starts
#define VPP_FB_BACKUP_MODE_GFX_ENABLE_ON_MAIN_NONBUILTIN   1
#ifdef ENABLE_PIP_GFX0_PATH
#define VPP_ENABLE_IS_PLANE_ENABLED_FOR_BACKUP(planeID) \
        ((VPP_FB_BACKUP_MODE_MAIN_ENABLE && (planeID == PLANE_MAIN)) || \
         (VPP_FB_BACKUP_MODE_GFX_ENABLE && ((planeID == PLANE_PIP)||(planeID == PLANE_GFX1))))

#define VPP_ENABLE_IS_PLANE_ENABLED_FOR_SWAP(planeID, mainPlane_buitin_status) \
        ((VPP_FB_BACKUP_MODE_MAIN_ENABLE && (planeID == PLANE_MAIN)) || \
         (VPP_FB_BACKUP_MODE_GFX_ENABLE && (!VPP_FB_BACKUP_MODE_GFX_ENABLE_ON_MAIN_NONBUILTIN || \
                                            (VPP_FB_BACKUP_MODE_GFX_ENABLE_ON_MAIN_NONBUILTIN && mainPlane_buitin_status && ((planeID == PLANE_PIP)||(planeID == PLANE_GFX1))))))

#else
#define VPP_ENABLE_IS_PLANE_ENABLED_FOR_BACKUP(planeID) \
        ((VPP_FB_BACKUP_MODE_MAIN_ENABLE && (planeID == PLANE_MAIN)) || \
         (VPP_FB_BACKUP_MODE_GFX_ENABLE && (planeID == PLANE_GFX1)))

#define VPP_ENABLE_IS_PLANE_ENABLED_FOR_SWAP(planeID, mainPlane_buitin_status) \
        ((VPP_FB_BACKUP_MODE_MAIN_ENABLE && (planeID == PLANE_MAIN)) || \
         (VPP_FB_BACKUP_MODE_GFX_ENABLE && (!VPP_FB_BACKUP_MODE_GFX_ENABLE_ON_MAIN_NONBUILTIN || \
                                            (VPP_FB_BACKUP_MODE_GFX_ENABLE_ON_MAIN_NONBUILTIN && mainPlane_buitin_status && (planeID == PLANE_GFX1)))))
#endif

#ifdef VPP_DEBUG_UTIL_ENABLE_frameBuffer
void VPP_DEBUG_UTIL_frameBufferInit(VPP_OBJ *vpp_obj) {
    UINT32 i, pidStart, pidEnd;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_12, VPP_DEBUG_UTIL_TOGGLE_PP_INIT "\n");

    VPP_DEBUG_getPlaneID(MAX_NUM_PLANES, &pidStart, &pidEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        VPPDBG_FB_STATE.pinfo_init_flag[i] = VPPDBG_FB_STATE.pinfo_swap_flag[i] = 0;
        memset(&VPPDBG_FB_STATE.pinfo[i], 0, sizeof(VBUF_INFO));
        VPPDBG_FB_STATE.frameBufferPrintEnable[i] = VPP_DEBUG_UTIL_PRINT_FB_PRINT_DEFAULT;
        VPPDBG_FB_STATE.frameBufferMonitorEnable[i] = VPP_DEBUG_UTIL_PRINT_FB_MONITOR_DEFAULT;
    }
    VPPDBG_FB_STATE.non_builtinFrame_pushedToMain = 0;
}

void VPP_DEBUG_UTIL_frameBufferStateReset(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 planeID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        VPPDBG_FB_STATE.pinfo_init_flag[i] = VPPDBG_FB_STATE.pinfo_swap_flag[i] = 0;
        memset(&VPPDBG_FB_STATE.pinfo[i], 0, sizeof(VBUF_INFO));
        VPPDBG_FB_STATE.frameBufferPrintEnable[i] = 0;
        VPPDBG_FB_STATE.frameBufferMonitorEnable[i] = 0;
    }
    VPPDBG_FB_STATE.non_builtinFrame_pushedToMain = 0;
}

void VPP_DEBUG_UTIL_frameBufferToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 planeID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPPDBG_FB_STATE.frameBufferMonitorEnable[i] = ! VPPDBG_FB_STATE.frameBufferMonitorEnable[i];
    }
}

void VPP_DEBUG_UTIL_frameBufferTogglePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 planeID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        VPPDBG_FB_STATE.frameBufferPrintEnable[i] = ! VPPDBG_FB_STATE.frameBufferPrintEnable[i];
    }
}

void VPP_DEBUG_UTIL_frameBufferShowVar(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 planeID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_12, "Variable for plane : %d, Monitor:%d, Print:%d\n", i,
            VPPDBG_FB_STATE.frameBufferMonitorEnable[i], VPPDBG_FB_STATE.frameBufferPrintEnable[i]);
    }
}

void VPP_DEBUG_UTIL_frameBufferPrint(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 planeID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_12, "FB details for Plane: %d, init[%d], swap[%d]\n", i,
            VPPDBG_FB_STATE.pinfo_init_flag[i], VPPDBG_FB_STATE.pinfo_swap_flag[i]);
        VPP_DEBUG_UTIL_printVbufInfo(vpp_obj, planeID, &VPPDBG_FB_STATE.pinfo[i]);
    }
}

void VPP_DEBUG_UTIL_getVbufInfo(VPP_OBJ *vpp_obj, UINT32 planeID, VBUF_INFO **pinfo) {
    VBUF_INFO *fb_pinfo = NULL;

    if((planeID < MAX_NUM_PLANES)) {
        fb_pinfo = &VPPDBG_FB_STATE.pinfo[planeID];
    }

    //donot process for invalid parameter
    if(!fb_pinfo || !pinfo) {
        return;
    }

    if((VPPDBG_FB_STATE.frameBufferMonitorEnable[planeID]))
    {
#if (VPP_FB_BACKUP_MODE_GFX_ENABLE_ON_MAIN_NONBUILTIN == 1)
        if((planeID == PLANE_MAIN) && ((*pinfo)->builtinFrame == 0) && (!VPPDBG_FB_STATE.non_builtinFrame_pushedToMain)) {
            UINT32 i, pidStart, pidEnd;

            VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_12, "Pushed non-builtin frame or non-422 frame to MAIN : %d \n",
                VPPDBG_FB_STATE.non_builtinFrame_pushedToMain);

            VPPDBG_FB_STATE.non_builtinFrame_pushedToMain = 1;

            //Enable swapping now
            VPP_DEBUG_getPlaneID(MAX_NUM_PLANES, &pidStart, &pidEnd);
            for(i=pidStart;i<=pidEnd;i++) {
                VPPDBG_FB_STATE.pinfo_swap_flag[i] = 1;
            }
        }
#endif //VPP_FB_BACKUP_MODE_GFX_ENABLE_ON_MAIN_NONBUILTIN

        if((VPP_ENABLE_IS_PLANE_ENABLED_FOR_BACKUP(planeID)))
        {
            if(!VPPDBG_FB_STATE.pinfo_init_flag[planeID]) {
                if( (!VPP_FB_BACKUP_FIRST_BUILTIN_FRAME) || (VPP_FB_BACKUP_FIRST_BUILTIN_FRAME && ((*pinfo)->builtinFrame == 1)) ) {
                    memcpy(&VPPDBG_FB_STATE.pinfo[planeID], *pinfo, sizeof(VBUF_INFO));
                    VPPDBG_FB_STATE.pinfo_init_flag[planeID] = 1;
                    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_12, "Taken backup : plane:%d, %d\n",planeID, VPPDBG_FB_STATE.pinfo_init_flag[planeID]);
                }
            } else {
                 if(VPP_ENABLE_IS_PLANE_ENABLED_FOR_SWAP(planeID, VPPDBG_FB_STATE.non_builtinFrame_pushedToMain) &&
                    (VPPDBG_FB_STATE.pinfo_swap_flag[planeID]) ) {
                    //VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_12, "change vbufinfo : %x->%x\n", *pinfo, &VPPDBG_FB_STATE.pinfo[planeID]);
                    *pinfo = &VPPDBG_FB_STATE.pinfo[planeID];
                 }
            }
        }
    }
}
#endif //VPP_DEBUG_UTIL_ENABLE_frameBuffer

