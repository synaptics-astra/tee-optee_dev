/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_debug_util.c
//! \brief      Various vpp debug utility routine to debug VPP issues
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_module.h"
#include "vpp_debug_util.h"
#include "vpp_debug_dump.h"
#include "vpp_intr_monitor.h"
#include "vpp_be.h"
#include "vpp_dv_api.h"

#ifndef bTST
#define bTST(x, b) (((x) >> (b)) & 1)
#endif

/*If default configuration for VPP_DEBUG_UTIL_SCL_DEFAULT_CONFIG is VPP_DEBUG_UTIL_DISABLE
 * then following condition allow selective sclID to be enabled on bootup */
#define IS_VPP_DEBUG_UTIL_SCL_CHANGE_MONITOR_ENABLED(sclId) \
            (VPP_FRC_SCL_MAIN==sclId)

void VPP_DEBUG_getUnitID(UINT32 unitID, UINT32 maxID, UINT32 *pUidStart, UINT32 *pUidEnd) {
    //If invalid unit ID; then list details for all plane
    if(unitID >= maxID) {
        *pUidStart = 0;
        *pUidEnd = maxID-1;
    } else {
        *pUidStart = *pUidEnd = unitID;
    }
}

#ifdef VPP_DEBUG_UTIL_ENABLE_printWindowChange
void VPP_DEBUG_UTIL_winChangeInit(VPP_OBJ *vpp_obj) {
    UINT32 i, j, pidStart, pidEnd;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_1, VPP_DEBUG_UTIL_TOGGLE_PP_INIT "\n");

    VPP_DEBUG_getPlaneID(MAX_NUM_PLANES, &pidStart, &pidEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        for(j=0;j<VPP_DEBUG_WINDOWS_PER_PLANE_MAX;j++) {
            VPPDBG_WINCNG_STATE.plane_win[i][j].x = VPPDBG_WINCNG_STATE.plane_win[i][j].y = 0;
            VPPDBG_WINCNG_STATE.plane_win[i][j].width = VPPDBG_WINCNG_STATE.plane_win[i][j].height = 0;
            VPPDBG_WINCNG_STATE.winChangePrintEnable[i][j] = VPP_DEBUG_UTIL_PRINT_WIN_CHANGE_PRINT_DEFAULT;
            VPPDBG_WINCNG_STATE.winChangeMonitorEnable[i][j] = VPP_DEBUG_UTIL_PRINT_WIN_CHANGE_MONITOR_DEFAULT;
        }
    }
}

void VPP_DEBUG_UTIL_winChangeStateReset(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, j, pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        for(j=0;j<VPP_DEBUG_WINDOWS_PER_PLANE_MAX;j++) {
            VPPDBG_WINCNG_STATE.plane_win[i][j].x = VPPDBG_WINCNG_STATE.plane_win[i][j].y = 0;
            VPPDBG_WINCNG_STATE.plane_win[i][j].width = VPPDBG_WINCNG_STATE.plane_win[i][j].height = 0;
            VPPDBG_WINCNG_STATE.winChangePrintEnable[i][j] = 0;
            VPPDBG_WINCNG_STATE.winChangeMonitorEnable[i][j] = 0;
        }
    }
}

void VPP_DEBUG_UTIL_winChangeToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 j, unitStart, unitEnd;
    UINT32 planeID = (command&0xF);
    UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    VPP_DEBUG_getUnitID(unitNum, VPP_DEBUG_WINDOWS_PER_PLANE_MAX, &unitStart, &unitEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        for(j=unitStart;j<=unitEnd;j++) {
            VPPDBG_WINCNG_STATE.winChangeMonitorEnable[i][j] = ! VPPDBG_WINCNG_STATE.winChangeMonitorEnable[i][j];
        }
    }

}

void VPP_DEBUG_UTIL_winChangeTogglePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 j, unitStart, unitEnd;
    UINT32 planeID = (command&0xF);
    UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    VPP_DEBUG_getUnitID(unitNum, VPP_DEBUG_WINDOWS_PER_PLANE_MAX, &unitStart, &unitEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        for(j=unitStart;j<=unitEnd;j++) {
            VPPDBG_WINCNG_STATE.winChangePrintEnable[i][j] = ! VPPDBG_WINCNG_STATE.winChangePrintEnable[i][j];
        }
    }
}
void VPP_DEBUG_UTIL_winChangeShowVar(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 j, unitStart, unitEnd;
    UINT32 planeID = (command&0xF);
    UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    VPP_DEBUG_getUnitID(unitNum, VPP_DEBUG_WINDOWS_PER_PLANE_MAX, &unitStart, &unitEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_1, "Variable for plane : %d , %d to %d\n",  i, unitStart, unitEnd);
        for(j=unitStart;j<=unitEnd;j++) {
            VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_1, "Monitor:%d, Print:%d\n",
                VPPDBG_WINCNG_STATE.winChangeMonitorEnable[i][j], VPPDBG_WINCNG_STATE.winChangePrintEnable[i][j]);
        }
    }
}

void VPP_DEBUG_UTIL_winChangePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, pidStart, pidEnd;
    UINT32 j, unitStart, unitEnd;
    UINT32 planeID = (command&0xF);
    UINT32 unitNum = (command&0xF0)>>4;
    VPP_WIN *win = NULL;

    VPP_DEBUG_getUnitID(unitNum, VPP_DEBUG_WINDOWS_PER_PLANE_MAX, &unitStart, &unitEnd);
    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);
    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_1, "Window details for plane : %d , %d to %d\n", i, unitStart, unitEnd);
        for(j=unitStart;j<=unitEnd;j++) {
            win = &VPPDBG_WINCNG_STATE.plane_win[i][j];
            if(VPP_DEBUG_WINDOWS_REFWIN_FROMISR == unitNum || VPP_DEBUG_WINDOWS_DISPWIN_FROMISR == unitNum) {
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_1, "FromISR: %d, %d, %d, %d\n", win->x, win->y, win->width, win->height);
            } else {
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_1, "x[%d], y[%d], w[%d], h[%d] \n", win->x, win->y, win->width, win->height);
            }
        }
    }
}

int VPP_DEBUG_UTIL_printWindowChange(VPP_OBJ *vpp_obj, VPP_WIN *win, UINT32 planeID, UINT32 unitNum, const char *funcName, UINT32 lineNum) {

    int retVal = 0;
    VPP_WIN *curWin = NULL;

    if((planeID < MAX_NUM_PLANES) && (unitNum < VPP_DEBUG_WINDOWS_PER_PLANE_MAX)) {
        curWin = &VPPDBG_WINCNG_STATE.plane_win[planeID][unitNum];
    }

    if((VPPDBG_WINCNG_STATE.winChangeMonitorEnable[planeID][unitNum] && curWin && win) &&
        (win->x!=curWin->x ||  win->y!=curWin->y || win->width!=curWin->width || win->height!=curWin->height))
    {
        if( VPPDBG_WINCNG_STATE.winChangePrintEnable[planeID][unitNum] ) {
            if(VPP_DEBUG_WINDOWS_REFWIN_FROMISR == unitNum || VPP_DEBUG_WINDOWS_DISPWIN_FROMISR == unitNum) {
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_1, "%s: FromISR:%d, %d, %d, %d, %d, %d\n", funcName?funcName:"-NA-", lineNum, planeID, win->x, win->y, win->width, win->height);
            } else {
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_1, ":%s:%d pln[%d], x[%d], y[%d], w[%d], h[%d] \n", funcName?funcName:"-NA-", lineNum, planeID, win->x, win->y, win->width, win->height);
            }
        }
        curWin->x = win->x;
        curWin->y = win->y;
        curWin->width = win->width;
        curWin->height = win->height;
        retVal = 1;
    }

    return retVal;
}
#endif //VPP_DEBUG_UTIL_ENABLE_printWindowChange


#ifdef VPP_DEBUG_UTIL_ENABLE_printSclChange
void VPP_DEBUG_UTIL_sclChangeInit(VPP_OBJ *vpp_obj) {
    UINT32 i, sidStart, sidEnd;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_11, VPP_DEBUG_UTIL_TOGGLE_PP_INIT "\n");

    VPP_DEBUG_getSclID(VPP_FRC_SCL_MAX, &sidStart, &sidEnd);
    for(i=sidStart;i<=sidEnd;i++) {
        VPPDBG_SCLCNG_STATE.scl_res[i].IVRes = VPPDBG_SCLCNG_STATE.scl_res[i].IHRes = 0;
        VPPDBG_SCLCNG_STATE.scl_res[i].OVRes = VPPDBG_SCLCNG_STATE.scl_res[i].OHRes = 0;
        VPPDBG_SCLCNG_STATE.scl_mode = VPPDBG_SCLCNG_STATE.sclLineDropEn = -1;
        VPPDBG_SCLCNG_STATE.sclChangePrintEnable[i] = VPP_DEBUG_UTIL_PRINT_SCL_CHANGE_PRINT_DEFAULT;
        VPPDBG_SCLCNG_STATE.sclChangeMonitorEnable[i] = VPP_DEBUG_UTIL_PRINT_SCL_CHANGE_MONITOR_DEFAULT;
        if((!(VPP_DEBUG_UTIL_PRINT_SCL_CHANGE_MONITOR_DEFAULT)) && (IS_VPP_DEBUG_UTIL_SCL_CHANGE_MONITOR_ENABLED(i))) {
            //Even-though global default flag is disabled for monitoring, enable on selected SCL for monitoring
            VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_11, "Enabling moniroting for only scl[%d]\n", i);
            VPPDBG_SCLCNG_STATE.sclChangePrintEnable[i] = VPP_DEBUG_UTIL_ENABLE;
            VPPDBG_SCLCNG_STATE.sclChangeMonitorEnable[i] = VPP_DEBUG_UTIL_ENABLE;
        }
    }
}

void VPP_DEBUG_UTIL_sclChangeStateReset(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, sidStart, sidEnd;
    UINT32 sclID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getSclID(sclID, &sidStart, &sidEnd);
    for(i=sidStart;i<=sidEnd;i++) {
        VPPDBG_SCLCNG_STATE.scl_res[i].IVRes = VPPDBG_SCLCNG_STATE.scl_res[i].IHRes = 0;
        VPPDBG_SCLCNG_STATE.scl_res[i].OVRes = VPPDBG_SCLCNG_STATE.scl_res[i].OHRes = 0;
        VPPDBG_SCLCNG_STATE.scl_mode = VPPDBG_SCLCNG_STATE.sclLineDropEn = -1;
        VPPDBG_SCLCNG_STATE.sclChangePrintEnable[i] = 0;
        VPPDBG_SCLCNG_STATE.sclChangeMonitorEnable[i] = 0;
    }
}

void VPP_DEBUG_UTIL_sclChangeToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, sidStart, sidEnd;
    UINT32 sclID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getSclID(sclID, &sidStart, &sidEnd);

    for(i=sidStart;i<=sidEnd;i++) {
        VPPDBG_SCLCNG_STATE.sclChangeMonitorEnable[i] = ! VPPDBG_SCLCNG_STATE.sclChangeMonitorEnable[i];
    }
}

void VPP_DEBUG_UTIL_sclChangeTogglePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, sidStart, sidEnd;
    UINT32 sclID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getSclID(sclID, &sidStart, &sidEnd);
    for(i=sidStart;i<=sidEnd;i++) {
        VPPDBG_SCLCNG_STATE.sclChangePrintEnable[i] = ! VPPDBG_SCLCNG_STATE.sclChangePrintEnable[i];
    }
}

void VPP_DEBUG_UTIL_sclChangeShowVar(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, sidStart, sidEnd;
    UINT32 sclID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getSclID(sclID, &sidStart, &sidEnd);
    for(i=sidStart;i<=sidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_11, "Variable for plane : %d, Monitor:%d, Print:%d\n", i,
            VPPDBG_SCLCNG_STATE.sclChangeMonitorEnable[i], VPPDBG_SCLCNG_STATE.sclChangePrintEnable[i]);
    }
}

void VPP_DEBUG_UTIL_sclChangePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    UINT32 i, sidStart, sidEnd;
    UINT32 sclID = (command&0xF);
    //UINT32 unitNum = (command&0xF0)>>4;

    VPP_DEBUG_getSclID(sclID, &sidStart, &sidEnd);
    for(i=sidStart;i<=sidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_11, "SCL details for SCL: %d, IRes[%dx%d], ORes[%dx%d], scl_mode:%d, sclLineDropEn:%d\n", i,
            VPPDBG_SCLCNG_STATE.scl_res[i].IHRes, VPPDBG_SCLCNG_STATE.scl_res[i].IVRes,
            VPPDBG_SCLCNG_STATE.scl_res[i].OHRes, VPPDBG_SCLCNG_STATE.scl_res[i].OVRes,
            VPPDBG_SCLCNG_STATE.scl_mode, VPPDBG_SCLCNG_STATE.sclLineDropEn);
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_11, "CTRL: pos[%d], uncrop_hres[%d], hv_Taps[%dx%d], hv_mode[%dx%d], lr_crop[%dx%d]\n",
            VPPDBG_SCLCNG_STATE.scl_ctrl[i].HScalePos, VPPDBG_SCLCNG_STATE.scl_ctrl[i].uncrop_IHRes,
            VPPDBG_SCLCNG_STATE.scl_ctrl[i].HTapNum, VPPDBG_SCLCNG_STATE.scl_ctrl[i].VTapNum,
            VPPDBG_SCLCNG_STATE.scl_ctrl[i].HSclMode, VPPDBG_SCLCNG_STATE.scl_ctrl[i].VSclMode,
            VPPDBG_SCLCNG_STATE.scl_ctrl[i].leftCrop, VPPDBG_SCLCNG_STATE.scl_ctrl[i].rightCrop);
    }
}

int VPP_DEBUG_UTIL_printSclChange(VPP_OBJ *vpp_obj, VPP_SCL_RES *sclWin, VPP_SCL_CTRL *sclCtrl, VPP_FRC_SCL_NUM sclID, INT8 scl_mode, INT8 sclLineDropEn, const char *funcName, UINT32 lineNum) {

    int retVal = 0;
    VPP_SCL_RES *curSclWin = NULL;
    VPP_SCL_CTRL *curSclCtrl = NULL;

    if((sclID < VPP_FRC_SCL_MAX)) {
        curSclWin = &VPPDBG_SCLCNG_STATE.scl_res[sclID];
        curSclCtrl = &VPPDBG_SCLCNG_STATE.scl_ctrl[sclID];
    }

    if((VPPDBG_SCLCNG_STATE.sclChangeMonitorEnable[sclID] && curSclWin && sclWin) &&
        ((sclWin->IHRes!=curSclWin->IHRes ||  sclWin->OHRes!=curSclWin->OHRes || sclWin->IVRes!=curSclWin->IVRes || sclWin->OVRes!=curSclWin->OVRes) ||
        ((VPPDBG_SCLCNG_STATE.scl_mode != scl_mode) || (VPPDBG_SCLCNG_STATE.sclLineDropEn != sclLineDropEn))))
    {
        if( VPPDBG_SCLCNG_STATE.sclChangePrintEnable[sclID] ) {
            VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_11, ":%s:%d scl[%d], IRes[%dx%d], ORes[%dx%d], scl_mode:%d, sclLineDropEn:%d\n",
                funcName?funcName:"-NA-", lineNum, sclID, sclWin->IHRes, sclWin->IVRes, sclWin->OHRes, sclWin->OVRes, VPPDBG_SCLCNG_STATE.scl_mode, VPPDBG_SCLCNG_STATE.sclLineDropEn);
        }
        memcpy(curSclWin, sclWin, sizeof(VPP_SCL_RES));
        memcpy(curSclCtrl, sclCtrl, sizeof(VPP_SCL_CTRL));
        VPPDBG_SCLCNG_STATE.scl_mode = scl_mode;
        VPPDBG_SCLCNG_STATE.sclLineDropEn = sclLineDropEn;
        retVal = 1;
    }

    return retVal;
}
#endif //VPP_DEBUG_UTIL_ENABLE_printSclChange

#if defined(VPP_DEBUG_UTIL_ENABLE_printFrameDrop) || defined(VPP_DEBUG_UTIL_ENABLE_printFrameRepeat)

void VPP_DEBUG_UTIL_frameDescStateInit(VPP_OBJ *vpp_obj, char *pHeaderStr, VPP_DEBUG_OCCURANCE_STATE *pState) {
    int i,j;
    UINT32 pidStart, pidEnd;

    VPP_DEBUG_getPlaneID(MAX_NUM_PLANES, &pidStart, &pidEnd);

    VPP_DEBUG_UTIL_PRINTF(pHeaderStr, VPP_DEBUG_UTIL_TOGGLE_PP_INIT "\n");

    for(i=pidStart;i<=pidEnd;i++) {
        pState->occurCount = 0;
        pState->isrOccurCount = 0;
        pState->isrCurrOccurCount = 0;
        pState->isrTotalCount = 0;
        pState->frameDescTotalCount = 0;
        pState->printEnable = VPP_DEBUG_UTIL_PRINT_FRAME_DESC_PRINT_DEFAULT;
        pState->monitorEnable = VPP_DEBUG_UTIL_PRINT_FRAME_DESC_MONITOR_DEFAULT;
        for(j=0;j<VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE;j++) {
            pState->frameDesc[j] = 0x0;
            pState->frameDescCount[j] = 0x0;
        }
    }
}

void VPP_DEBUG_UTIL_frameDescStateReset(VPP_OBJ *vpp_obj, char *pHeaderStr, VPP_DEBUG_OCCURANCE_STATE *pState, UINT32 command) {
    int i,j;
    UINT32 pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(pHeaderStr, VPP_DEBUG_UTIL_TOGGLE_PP_RESET " plane:%d \n", i);
        pState->occurCount = 0;
        pState->isrOccurCount = 0;
        pState->isrCurrOccurCount = 0;
        pState->isrTotalCount = 0;
        pState->frameDescTotalCount = 0;
        //pState->printEnable = 0;
        //pState->monitorEnable = 0;
        for(j=0;j<VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE;j++) {
            pState->frameDesc[j] = 0x0;
            pState->frameDescCount[j] = 0x0;
        }
    }
}

void VPP_DEBUG_UTIL_frameDescTogglePrint(VPP_OBJ *vpp_obj, char *pHeaderStr, VPP_DEBUG_OCCURANCE_STATE *pState, UINT32 command) {
    int i;
    UINT32 pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(pHeaderStr, VPP_DEBUG_UTIL_TOGGLE_PP_PRINT " plane:%d, %d->%d\n", i,
            pState->printEnable, !pState->printEnable);
        pState->printEnable = !pState->printEnable;
    }
}

void VPP_DEBUG_UTIL_frameDescToggleMonitor(VPP_OBJ *vpp_obj, char *pHeaderStr, VPP_DEBUG_OCCURANCE_STATE *pState, UINT32 command) {
    int i;
    UINT32 pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_OCCURANCE_STATE *pTState = pState + i;
        VPP_DEBUG_UTIL_PRINTF(pHeaderStr, VPP_DEBUG_UTIL_TOGGLE_PP_MONITOR " plane:%d, %d->%d\n", i,
            pTState->monitorEnable, !pTState->monitorEnable);
        pTState->monitorEnable = !pTState->monitorEnable;
    }
}
void VPP_DEBUG_UTIL_frameDescShowVar(VPP_OBJ *vpp_obj, char *pHeaderStr, VPP_DEBUG_OCCURANCE_STATE *pState, UINT32 command) {
    int i;
    UINT32 pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_OCCURANCE_STATE *pTState = pState + i;
        VPP_DEBUG_UTIL_PRINTF(pHeaderStr, VPP_DEBUG_UTIL_TOGGLE_PP_SHOW_VAR " plane:%d, print:%d, monitor:%d\n",
            i, pTState->printEnable, pTState->monitorEnable);
    }

}


void VPP_DEBUG_UTIL_printFrameDetails(VPP_OBJ *vpp_obj, char *pHeaderStr, VPP_DEBUG_OCCURANCE_STATE *pState) {
    int i,j;

    for(i=0;i<MAX_NUM_PLANES;i++) {
        VPP_DEBUG_OCCURANCE_STATE *pTState = pState + i;
        //Note: isrOccurCount-does not have much meaning for framerepeat
        VPP_DEBUG_UTIL_PRINTF(pHeaderStr, "plane:%d, occurCount:%d, isrOccurCount:%d, isrTotalCount:%d\n",
            i, pTState->occurCount, pTState->isrOccurCount, pTState->isrTotalCount);
        for(j=0;j<VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE && j<pTState->frameDescTotalCount;j++) {
            VPP_DEBUG_UTIL_PRINTF(pHeaderStr, "\tDesc#:%d, Desc:%p, count:%d\n",
                j, pTState->frameDesc[j], pTState->frameDescCount[j]);
        }

        pState++;
    }
}

void VPP_DEBUG_UTIL_monitorFrameDescUpdate(VPP_OBJ *vpp_obj, VDEC_VID_BUF_DESC *frm_descr, VPP_DEBUG_OCCURANCE_STATE *pState) {
    int i;
    int foundFlag = 0;

    //Search, if this is a known descriptor
    for(i=0;i<VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE;i++) {
        if(pState->frameDesc[i] == frm_descr) {
            foundFlag = 1;
            break;
        }
    }
    if(!foundFlag) {
        if(pState->frameDescTotalCount < VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE) {
            //This is the first occurance for this descriptor; add to the end of list
            pState->frameDesc[pState->frameDescTotalCount] = frm_descr;
            //Increment occurance count
            pState->frameDescCount[pState->frameDescTotalCount]++;
            pState->frameDescTotalCount++;
        } else {
            //We cannot monitor more than 'VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE' descriptors
        }
    } else {
        //Increment the count associated with the known descriptor
        pState->frameDescCount[i]++;
    }
}
#endif // defined(VPP_DEBUG_UTIL_ENABLE_printFrameDrop) || defined(VPP_DEBUG_UTIL_ENABLE_printFrameRepeat)

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameDrop
void VPP_DEBUG_UTIL_frameDropInit(VPP_OBJ *vpp_obj) {
    VPP_DEBUG_UTIL_frameDescStateInit(vpp_obj, VPP_DEBUG_UTIL_PP_4, &VPPDBG_FDROP_STATE[0]);
}

void VPP_DEBUG_UTIL_frameDropStateReset(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_frameDescStateReset(vpp_obj, VPP_DEBUG_UTIL_PP_4, &VPPDBG_FDROP_STATE[0], command);
}
void VPP_DEBUG_UTIL_frameDropTogglePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_frameDescTogglePrint(vpp_obj, VPP_DEBUG_UTIL_PP_4, &VPPDBG_FDROP_STATE[0], command);
}
void VPP_DEBUG_UTIL_frameDropToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_frameDescToggleMonitor(vpp_obj, VPP_DEBUG_UTIL_PP_4, &VPPDBG_FDROP_STATE[0], command);
}
void VPP_DEBUG_UTIL_frameDropShowVar(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_frameDescShowVar(vpp_obj, VPP_DEBUG_UTIL_PP_4, &VPPDBG_FDROP_STATE[0], command);
}

void VPP_DEBUG_UTIL_monitorFrameDrop(VPP_OBJ *vpp_obj,UINT32 state, UINT32 planeID, VDEC_VID_BUF_DESC *frm_descr) {
    VPP_DEBUG_OCCURANCE_STATE *pState = &VPPDBG_FDROP_STATE[planeID];

    if(!pState->monitorEnable || !frm_descr) {
        return;
    }

    if(VPP_DEBUG_FRAMEDROP_STATE_START == state) {
        //Check if we are invoked multiple times for the given plane, in previous ISR
        if(pState->isrCurrOccurCount > 1 ) {
            pState->occurCount += (pState->isrCurrOccurCount-1);
            pState->isrOccurCount++;
        }
        //Increment total isr count
        pState->isrTotalCount++;
        //Reset the variable for current ISR
        pState->isrCurrOccurCount = 0;
    } else {
        //If we are in processing mode, then count the occurance
        if(pState->isrCurrOccurCount > 0 ) {
            #ifndef CONFIG_AMP_TEE_ENABLE
                VBUF_INFO *pinfo = (VBUF_INFO *)frm_descr->user_data;
            #else
                VBUF_INFO *pinfo = (VBUF_INFO *)frm_descr;
            #endif

            //We can print/show the current details here
            if(pState->printEnable && pinfo) {
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_4, "DROP:" "P-%d,O-%d,I-%d,F-%x/%p\n", planeID, pState->isrCurrOccurCount, pState->isrOccurCount, frm_descr, pinfo->m_pbuf_start);
            }

            VPP_DEBUG_UTIL_monitorFrameDescUpdate(vpp_obj, pState->prevIsrDesc[planeID], pState);
        }
        pState->isrCurrOccurCount++;
        pState->prevIsrDesc[planeID] = frm_descr;
    }
}
void VPP_DEBUG_UTIL_frameDropPrint(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_printFrameDetails(vpp_obj, VPP_DEBUG_UTIL_PP_4, &VPPDBG_FDROP_STATE[0]);
}

#endif //VPP_DEBUG_UTIL_ENABLE_printFrameDrop

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
void VPP_DEBUG_UTIL_frameRepeatInit(VPP_OBJ *vpp_obj) {
    VPP_DEBUG_UTIL_frameDescStateInit(vpp_obj, VPP_DEBUG_UTIL_PP_5, &VPPDBG_FREPT_STATE[0]);
}

void VPP_DEBUG_UTIL_frameRepeatStateReset(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_frameDescStateReset(vpp_obj, VPP_DEBUG_UTIL_PP_5, &VPPDBG_FREPT_STATE[0], command);
}
void VPP_DEBUG_UTIL_frameRepeatTogglePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_frameDescTogglePrint(vpp_obj, VPP_DEBUG_UTIL_PP_5, &VPPDBG_FREPT_STATE[0], command);
}
void VPP_DEBUG_UTIL_frameRepeatToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_frameDescToggleMonitor(vpp_obj, VPP_DEBUG_UTIL_PP_5, &VPPDBG_FREPT_STATE[0], command);
}
void VPP_DEBUG_UTIL_frameRepeatShowVar(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_frameDescShowVar(vpp_obj, VPP_DEBUG_UTIL_PP_5, &VPPDBG_FREPT_STATE[0], command);
}

void VPP_DEBUG_UTIL_monitorFrameRepeat(VPP_OBJ *vpp_obj, UINT32 state, UINT32 planeID, VDEC_VID_BUF_DESC *frm_descr) {
    VPP_DEBUG_OCCURANCE_STATE *pState = &VPPDBG_FREPT_STATE[planeID];
    VBUF_INFO *pinfo;

    if(!frm_descr) {
        return;
    }

    #ifndef CONFIG_AMP_TEE_ENABLE
        pinfo = (VBUF_INFO *)frm_descr->user_data;
    #else
        pinfo = (VBUF_INFO *)frm_descr;
    #endif


    if((!pState->monitorEnable) || (!pinfo)) {
        return;
    }

    //Increment total isr count
    pState->isrTotalCount++;

    if(pState->prevIsrDesc[planeID] == frm_descr) {
        VPP_DEBUG_UTIL_monitorFrameDescUpdate(vpp_obj, frm_descr, pState);
        //ok collect how many times same frame is repeated for this block/iteration
        pState->isrOccurCount++;
        //We can repeat once pre isr, so  this variable is not useful, but use it to track # of framerepeat per block
        pState->isrCurrOccurCount++;
        if(pState->newFrameFlag[planeID]) {
            //Increment occurance count - one increment per block of repeat frame
            //Note: built-in frame will be repeated multiple times.
            //Thus occureance count will tell # of times builtin frame pushed to the plane
            pState->newFrameFlag[planeID] = 0;
            pState->occurCount++;
        }
    } else {
        //We got new/different frame - so increment occurance count for this plane
        if(!pState->newFrameFlag[planeID]) {
            pState->newFrameFlag[planeID]++;
            //Reset the isr occur count
            pState->isrOccurCount = 0;
        }
        //We got new/different frame - so reset the framerepeat count for this block, log any previous frame repeat
        if(pState->isrCurrOccurCount) {
            //1 for the 1st time + # of times repeated - nonzero sum of prev & current difference is any issue scenario/pattern
            UINT32 currentframeRepeatDiff = (pState->prevFrameRepeatCount[planeID]) - (pState->isrCurrOccurCount+1);
            UINT32 sumDiff = currentframeRepeatDiff + pState->prevFrameRepeatDiff[planeID];
            if(pState->printEnable) {
                if(sumDiff) {
                    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_5, "REPEAT:" "P-%d,O-%d,F-%x/%p,D-<%02d,%02d>\n",
                        planeID, pState->isrCurrOccurCount+1, pState->prevIsrDesc[planeID], pinfo->m_pbuf_start,
                        currentframeRepeatDiff, currentframeRepeatDiff + pState->prevFrameRepeatDiff[planeID]);
                } else {
                    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_5, "REPEAT:" "P-%d,O-%d,F-%x/%p\n",
                        planeID, pState->isrCurrOccurCount+1, pState->prevIsrDesc[planeID], pinfo->m_pbuf_start);
                }
            }
            pState->prevFrameRepeatDiff[planeID] = currentframeRepeatDiff;
            pState->prevFrameRepeatCount[planeID] =  pState->isrCurrOccurCount+1;
            pState->isrCurrOccurCount=0;
        }
    }

    //Take copy of the current selected frame
    pState->prevIsrDesc[planeID] = frm_descr;
}

void VPP_DEBUG_UTIL_frameRepeatPrint(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_printFrameDetails(vpp_obj, VPP_DEBUG_UTIL_PP_5, &VPPDBG_FREPT_STATE[0]);
}
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameRepeat


#ifdef VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo
void VPP_DEBUG_UTIL_vbufInfoInit(VPP_OBJ *vpp_obj) {
    int i,j;
    UINT32 pidStart, pidEnd;

    VPP_DEBUG_getPlaneID(MAX_NUM_PLANES, &pidStart, &pidEnd);

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, VPP_DEBUG_UTIL_TOGGLE_PP_INIT "\n");

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_VBUFINFO_STATE *planeVbufInfo = &VPPDBG_VBUF_STATE[i];
        planeVbufInfo->count = 0;
        planeVbufInfo->nextNdx = 0;
        planeVbufInfo->prevNdx = 0;
        planeVbufInfo->countTotal = 0;
        planeVbufInfo->printEnable = VPP_DEBUG_UTIL_PRINT_VBUF_INFO_PRINT_DEFAULT;
        planeVbufInfo->monitorEnable = VPP_DEBUG_UTIL_PRINT_VBUF_INFO_MONITOR_DEFAULT;
        for(j=0;j<VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE;j++) {
            memset(&planeVbufInfo->pVbufInfo[j], 0x0, sizeof(VBUF_INFO));
        }
    }
}

void VPP_DEBUG_UTIL_vbufInfoStateReset(VPP_OBJ *vpp_obj, UINT32 command) {
    int i,j;
    UINT32 pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_VBUFINFO_STATE *planeVbufInfo = &VPPDBG_VBUF_STATE[i];
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, VPP_DEBUG_UTIL_TOGGLE_PP_RESET " plane:%d, print\n", i,  planeVbufInfo->printEnable);
        planeVbufInfo->count = 0;
        planeVbufInfo->nextNdx = 0;
        planeVbufInfo->prevNdx = 0;
        planeVbufInfo->countTotal = 0;
        //planeVbufInfo->printEnable = 0;
        //planeVbufInfo->monitorEnable = 0;
        for(j=0;j<VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE;j++) {
            memset(&planeVbufInfo->pVbufInfo[j], 0x0, sizeof(VBUF_INFO));
        }
    }
}

void VPP_DEBUG_UTIL_vbufInfoTogglePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    int i;
    UINT32 pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, VPP_DEBUG_UTIL_TOGGLE_PP_PRINT " plane:%d, %d->%d\n", i,
            VPPDBG_VBUF_STATE[i].printEnable, !VPPDBG_VBUF_STATE[i].printEnable);
        VPPDBG_VBUF_STATE[i].printEnable = !VPPDBG_VBUF_STATE[i].printEnable;
    }
}

void VPP_DEBUG_UTIL_vbufInfoToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command) {
    int i;
    UINT32 pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, VPP_DEBUG_UTIL_TOGGLE_PP_MONITOR " plane:%d, %d->%d\n", i,
            VPPDBG_VBUF_STATE[i].monitorEnable, !VPPDBG_VBUF_STATE[i].monitorEnable);
        VPPDBG_VBUF_STATE[i].monitorEnable = !VPPDBG_VBUF_STATE[i].monitorEnable;
    }
}

void VPP_DEBUG_UTIL_vbufInfoShowVar(VPP_OBJ *vpp_obj, UINT32 command) {
    int i;
    UINT32 pidStart, pidEnd;
    UINT32 planeID = (command&0xF);

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, VPP_DEBUG_UTIL_TOGGLE_PP_SHOW_VAR " plane:%d, print:%d, monitor:%d\n",
            i, VPPDBG_VBUF_STATE[i].printEnable, VPPDBG_VBUF_STATE[i].monitorEnable);
    }

}

UINT32 VPP_DEBUG_UTIL_isSameVbufInfo(VPP_OBJ *vpp_obj, VBUF_INFO *pVbufInfo1, VBUF_INFO *pVbufInfo2) {
    UINT32 isSame = 1;

    if ((pVbufInfo1->builtinFrame != pVbufInfo2->builtinFrame)) {
        isSame = 0;
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "MISMATCH-0\n");
    } else if ((pVbufInfo1->m_srcfmt != pVbufInfo2->m_srcfmt) ||
                (pVbufInfo1->m_order != pVbufInfo2->m_order) ||
                (pVbufInfo1->m_bits_per_pixel != pVbufInfo2->m_bits_per_pixel) ||
                (pVbufInfo1->m_luma_left_ofst != pVbufInfo2->m_luma_left_ofst) ||
                (pVbufInfo1->m_luma_top_ofst != pVbufInfo2->m_luma_top_ofst) ||
                (pVbufInfo1->m_chroma_left_ofst != pVbufInfo2->m_chroma_left_ofst) ||
                (pVbufInfo1->m_chroma_top_ofst != pVbufInfo2->m_chroma_top_ofst) ||
                (pVbufInfo1->m_is_compressed != pVbufInfo2->m_is_compressed) ||
                ((pVbufInfo1->m_colorprimaries  < VPP_COLOR_PRIM_MAX) && (pVbufInfo1->m_colorprimaries  != pVbufInfo2->m_colorprimaries))) {
        isSame = 0;
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "MISMATCH-1\n");
    } else if ((pVbufInfo1->m_content_offset != pVbufInfo2->m_content_offset) ||
                (pVbufInfo1->m_content_width != pVbufInfo2->m_content_width) ||
                (pVbufInfo1->m_content_height != pVbufInfo2->m_content_height) ||
                (pVbufInfo1->m_buf_stride != pVbufInfo2->m_buf_stride) ||
                (pVbufInfo1->m_active_left != pVbufInfo2->m_active_left) ||
                (pVbufInfo1->m_active_top != pVbufInfo2->m_active_top) ||
                (pVbufInfo1->m_active_width != pVbufInfo2->m_active_width) ||
                (pVbufInfo1->m_active_height != pVbufInfo2->m_active_height) ||
                (pVbufInfo1->m_disp_offset != pVbufInfo2->m_disp_offset) ||
                (pVbufInfo1->m_is_frame_seq != pVbufInfo2->m_is_frame_seq)) {
        isSame = 0;
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "MISMATCH-2\n");
    } else if (pVbufInfo1->m_dv_present_mode != pVbufInfo2->m_dv_present_mode) {
        isSame = 0;
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "MISMATCH-3\n");
    } else if ((pVbufInfo1->m_pan_left[0] != pVbufInfo2->m_pan_left[0]) ||
                (pVbufInfo1->m_pan_top[0] != pVbufInfo2->m_pan_top[0]) ||
                (pVbufInfo1->m_pan_width[0] != pVbufInfo2->m_pan_width[0]) ||
                (pVbufInfo1->m_pan_height[0] != pVbufInfo2->m_pan_height[0])) {
        isSame = 0;
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "MISMATCH-4\n");
    } else if ((pVbufInfo1->m_pan_left[1] != pVbufInfo2->m_pan_left[1]) ||
                (pVbufInfo1->m_pan_top[1] != pVbufInfo2->m_pan_top[1]) ||
                (pVbufInfo1->m_pan_width[1] != pVbufInfo2->m_pan_width[1]) ||
                (pVbufInfo1->m_pan_height[1] != pVbufInfo2->m_pan_height[1])) {
        isSame = 0;
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "MISMATCH-5\n");
    } else if ((pVbufInfo1->transfer_characteristics != pVbufInfo2->transfer_characteristics)) {
        isSame = 0;
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "MISMATCH-6\n");
    }

    return isSame;
}

void VPP_DEBUG_UTIL_storeVbufInfo(VPP_OBJ *vpp_obj, UINT32 planeID, VBUF_INFO *pVbufInfo) {
    int storeInfo = 0;
    int currNdx;
    VPP_DEBUG_VBUFINFO_STATE *planeVbufInfo = &VPPDBG_VBUF_STATE[planeID];
    VBUF_INFO *pVbufInfoPrev = &planeVbufInfo->pVbufInfo[planeVbufInfo->prevNdx];
    VBUF_INFO *pVbufInfoCurr = &planeVbufInfo->pVbufInfo[planeVbufInfo->nextNdx];

    if((planeVbufInfo->monitorEnable) && ((0 == planeVbufInfo->count) ||
        (!VPP_DEBUG_UTIL_isSameVbufInfo(vpp_obj, pVbufInfoPrev, pVbufInfo))) ) {
        storeInfo = 1;
    }

    if (storeInfo) {
        //VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "COPYING %d->%d: %x<-%x : %d:%x\n",planeID, planeVbufInfo->prevNdx, pVbufInfoCurr, pVbufInfo);
        //copy current vbufinfo into backup
        *pVbufInfoCurr = *pVbufInfo;

        currNdx = planeVbufInfo->nextNdx;

        //move the pointer & counter
        if(planeVbufInfo->count < VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE) {
            planeVbufInfo->count++;
        }
        planeVbufInfo->countTotal++;
        planeVbufInfo->prevNdx = planeVbufInfo->nextNdx;
        planeVbufInfo->nextNdx = (planeVbufInfo->nextNdx+1)%VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE;

        //If printing enabled, then print the vbufinfo
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "STORED plane:%d, prev:%d, next:%d, total:%d/%d, MAX:%d, at-Ndx:%d\n",
                planeID, planeVbufInfo->prevNdx, planeVbufInfo->nextNdx, planeVbufInfo->count,
                planeVbufInfo->countTotal, VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE, currNdx);
        if(planeVbufInfo->printEnable) {
            VPP_DEBUG_UTIL_printVbufInfo_opt(vpp_obj, planeID, pVbufInfo);
        }
    }
}

void VPP_DEBUG_UTIL_vbufInfoPrint(VPP_OBJ *vpp_obj, UINT32 command) {
    int i, j, k, n;
    int startNdx, endNdx;
    UINT32 ndx = (command&0xF);
    UINT32 planeID = (command&0xF0)>>4;
    UINT32 pidStart, pidEnd;

    VPP_DEBUG_getPlaneID(planeID, &pidStart, &pidEnd);

    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_VBUFINFO_STATE *planeVbufInfo = &VPPDBG_VBUF_STATE[i];
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "PRINT plane:%d, total:%d/%d, prev:%d, next:%d, print:%d\n", i,
            planeVbufInfo->count, planeVbufInfo->countTotal, planeVbufInfo->prevNdx, planeVbufInfo->nextNdx, planeVbufInfo->printEnable);
    }

    //VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "PRINT Plane Iteration : %d->%d : ndx:%d\n",pidStart, pidEnd, ndx);
    for(i=pidStart;i<=pidEnd;i++) {
        VPP_DEBUG_VBUFINFO_STATE *planeVbufInfo = &VPPDBG_VBUF_STATE[i];
        if(planeVbufInfo->count) {
            n = 1;
            if(ndx == 0xF) {
                //If index is maxvalue=0xF; then iterate through all VBUFINFO
                if(planeVbufInfo->count < VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE) {
                    startNdx=0; endNdx=planeVbufInfo->prevNdx;
                } else {
                    //startNdx=planeVbufInfo->prevNdx; endNdx=planeVbufInfo->nextNdx;
                    startNdx=planeVbufInfo->nextNdx; endNdx=planeVbufInfo->prevNdx;
                }
                n = planeVbufInfo->count;
            } else if (ndx >= VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE) {
                //If valid index, does not have VBUFINFO; then show last one e.g. index from 0xr0 to 0xA(VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE)
                startNdx = endNdx = planeVbufInfo->prevNdx;
            } else {
                //If valud index, then show only
                startNdx = endNdx = ndx;
            }

            //VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "PRINT VBUF Iteration(user ndx:%x) : %d->%d (n-%d)\n", ndx, startNdx, endNdx, n);
            for(j=startNdx, k=0; k<n; k++) {
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_6, "PRINT plane:%d, prev:%d, next:%d, total:%d, MAX:%d, Showing:%d\n",
                    i, planeVbufInfo->prevNdx, planeVbufInfo->nextNdx, planeVbufInfo->count, VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE, j);
                VPP_DEBUG_UTIL_printVbufInfo(vpp_obj, i, &planeVbufInfo->pVbufInfo[j]);
                //Move the pointer
                j = (j+1) % VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE;
            }
        }
    }
}

#endif //VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo

#ifdef VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss

void VPP_DEBUG_UTIL_vbiVdeIntrMissInit(VPP_OBJ *vpp_obj) {
    VPP_DEBUG_INTR_INSTANCE_STATE *pState = &VPPDBG_INTRINS_STATE;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_9, VPP_DEBUG_UTIL_TOGGLE_PP_INIT "\n");

    pState->intrCount = pState->vbiIntrCount = pState->vdeIntrCount = 0;
    pState->tempVbiIntrCount = pState->tempVdeIntrCount = 0;

    pState->printEnable = VPP_DEBUG_UTIL_PRINT_IMISS_PRINT_DEFAULT;
    pState->monitorEnable = VPP_DEBUG_UTIL_PRINT_IMISS_MONITOR_DEFAULT;
}

void VPP_DEBUG_UTIL_vbiVdeIntrMissStateReset(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_INTR_INSTANCE_STATE *pState = &VPPDBG_INTRINS_STATE;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_9, VPP_DEBUG_UTIL_TOGGLE_PP_RESET "\n");

    pState->intrCount = pState->vbiIntrCount = pState->vdeIntrCount = 0;
    //These are internal variables; We shoule reset these variable by user command
    //pState->tempVbiIntrCount = pState->tempVdeIntrCount = 0;
}

void VPP_DEBUG_UTIL_vbiVdeIntrMissTogglePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_INTR_INSTANCE_STATE *pState = &VPPDBG_INTRINS_STATE;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_9, VPP_DEBUG_UTIL_TOGGLE_PP_PRINT " %d->%d\n",
        pState->printEnable, !pState->printEnable);
    pState->printEnable = !pState->printEnable;
}

void VPP_DEBUG_UTIL_vbiVdeIntrMissToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_INTR_INSTANCE_STATE *pState = &VPPDBG_INTRINS_STATE;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_9, VPP_DEBUG_UTIL_TOGGLE_PP_MONITOR " %d->%d\n",
        pState->monitorEnable, !pState->monitorEnable);
    pState->monitorEnable = !pState->monitorEnable;
}

void VPP_DEBUG_UTIL_vbiVdeIntrMissShowVar(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_INTR_INSTANCE_STATE *pState = &VPPDBG_INTRINS_STATE;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_9, VPP_DEBUG_UTIL_TOGGLE_PP_SHOW_VAR " print:%d, monitor:%d\n",
        pState->printEnable, pState->monitorEnable);
}

void VPP_DEBUG_UTIL_vbiVdeIntrMissPrintDetails(VPP_OBJ *vpp_obj) {
    VPP_DEBUG_INTR_INSTANCE_STATE *pState = &VPPDBG_INTRINS_STATE;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_9, "intrCount:%d, vbiIntrCount:%d, vdeIntrCount:%d, vbiIntrMissCount:%d, vdeIntrMissCount:%d\n",
        pState->intrCount, pState->vbiIntrCount, pState->vdeIntrCount, pState->tempVbiIntrMissCount, pState->tempVdeIntrMissCount);
}

void VPP_DEBUG_UTIL_monitorVbiVdeIntrMiss(VPP_OBJ *vpp_obj, UINT32 intrInstat) {
    VPP_DEBUG_INTR_INSTANCE_STATE *pState = &VPPDBG_INTRINS_STATE;
    const UINT32 VPP_DEBUG_UTIL_VBI_INTR_MASK = avioDhubSemMap_vpp128b_vpp_inr0;
    const UINT32 VPP_DEBUG_UTIL_VDE_INTR_MASK = avioDhubSemMap_vpp128b_vpp_inr0;

    if((!pState->monitorEnable)) {
        return;
    }

    pState->intrCount++;

    if (bTST(intrInstat, VPP_DEBUG_UTIL_VBI_INTR_MASK)) {
        pState->vbiIntrCount++;
        pState->tempVbiIntrCount++;
        if(pState->tempVdeIntrCount>1) {
            //Few VBI interrupt missed;  Show how many VDE occured during this time
            pState->tempVbiIntrMissCount++;
            if(pState->printEnable) {
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_9, "VBI: currMissCnt:%d TotMissCnt:%d\n",
                    pState->tempVdeIntrCount, pState->tempVbiIntrMissCount);
            }
        }
        pState->tempVdeIntrCount = 0;
    }
    if (bTST(intrInstat, VPP_DEBUG_UTIL_VDE_INTR_MASK)) {
        pState->vdeIntrCount++;
        pState->tempVdeIntrCount++;
        if(pState->tempVbiIntrCount>1) {
            //Few VDE interrupt missed; Show how many VBI occured during this time
            pState->tempVdeIntrMissCount++;
            if(pState->printEnable) {
                VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_9, "VDE: currMissCnt:%d TotMissCnt:%d\n",
                    pState->tempVbiIntrCount, pState->tempVdeIntrMissCount);
            }
        }
        pState->tempVbiIntrCount = 0;

    }
}

void VPP_DEBUG_UTIL_vbiVdeIntrMissPrint(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_UTIL_vbiVdeIntrMissPrintDetails(vpp_obj);
}

#endif //VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss

#ifdef VPP_DEBUG_UTIL_ENABLE_FeatureFlag

const char *g_VppDbgUtilFFStrArray[VPP_DEBUG_UTIL_FF_MAX] = {
    "BCMBUFF",
    "AUTOPUSH",
    "REFWIN",
    "DISPWIN",
    "PTMTA",
};

const BOOL g_FFPrintDefaultArray[VPP_DEBUG_UTIL_FF_MAX] = {
    VPP_DEBUG_UTIL_FF_VALIDATE_BCMBUFF_DEFAULT_CONFIG,
    VPP_DEBUG_UTIL_FF_AUTOPUSH_DEFAULT_CONFIG,
    VPP_DEBUG_UTIL_FF_REFWIN_DEFAULT_CONFIG,
    VPP_DEBUG_UTIL_FF_DISPWIN_DEFAULT_CONFIG,
    VPP_DEBUG_UTIL_FF_PTMTA_DEFAULT_CONFIG,
};

const BOOL g_FFMonitorDefaultArray[VPP_DEBUG_UTIL_FF_MAX] = {
    VPP_DEBUG_UTIL_FF_VALIDATE_BCMBUFF_DEFAULT_CONFIG,
    VPP_DEBUG_UTIL_FF_AUTOPUSH_DEFAULT_CONFIG,
    VPP_DEBUG_UTIL_FF_REFWIN_DEFAULT_CONFIG,
    VPP_DEBUG_UTIL_FF_DISPWIN_DEFAULT_CONFIG,
    VPP_DEBUG_UTIL_FF_PTMTA_DEFAULT_CONFIG,
};

void VPP_DEBUG_UTIL_featureFlagInit(VPP_OBJ *vpp_obj) {
    int i;
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_10, VPP_DEBUG_UTIL_TOGGLE_PP_INIT "\n");

    for(i=0;i<VPP_DEBUG_UTIL_FF_MAX;i++) {
        VPP_DEBUG_FEATURE_FLAG_STATE *pState = &VPPDBG_FF_STATE[i];
        pState->printEnable = g_FFPrintDefaultArray[i];
        pState->monitorEnable = g_FFMonitorDefaultArray[i];
    }
}

void VPP_DEBUG_UTIL_featureFlagStateReset(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_FEATURE_FLAG_STATE *pState = &VPPDBG_FF_STATE[command];

    if(command >= VPP_DEBUG_UTIL_FF_MAX) {
        return;
    }

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_10, VPP_DEBUG_UTIL_TOGGLE_PP_RESET "\n");

    pState->printEnable = g_FFPrintDefaultArray[command];
    pState->monitorEnable = g_FFMonitorDefaultArray[command];
}

void VPP_DEBUG_UTIL_featureFlagTogglePrint(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_FEATURE_FLAG_STATE *pState = &VPPDBG_FF_STATE[command];

    if(command >= VPP_DEBUG_UTIL_FF_MAX) {
        return;
    }

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_10, VPP_DEBUG_UTIL_TOGGLE_PP_PRINT " %s:%d->%d\n",
        g_VppDbgUtilFFStrArray[command], pState->printEnable, !pState->printEnable);
    pState->printEnable = !pState->printEnable;
}

void VPP_DEBUG_UTIL_featureFlagToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_FEATURE_FLAG_STATE *pState = &VPPDBG_FF_STATE[command];

    if(command >= VPP_DEBUG_UTIL_FF_MAX) {
        return;
    }

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_10, VPP_DEBUG_UTIL_TOGGLE_PP_MONITOR "/Feature: %s:%d->%d\n",
        g_VppDbgUtilFFStrArray[command], pState->monitorEnable, !pState->monitorEnable);
    pState->monitorEnable = !pState->monitorEnable;
}

void VPP_DEBUG_UTIL_featureFlagShowVar(VPP_OBJ *vpp_obj, UINT32 command) {
    VPP_DEBUG_FEATURE_FLAG_STATE *pState = &VPPDBG_FF_STATE[command];

    if(command >= VPP_DEBUG_UTIL_FF_MAX) {
        return;
    }

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_10, VPP_DEBUG_UTIL_TOGGLE_PP_SHOW_VAR " %s: print:%d, feature:%d\n",
        g_VppDbgUtilFFStrArray[command], pState->printEnable, pState->monitorEnable);
}

void VPP_DEBUG_UTIL_featureFlagPrint(VPP_OBJ *vpp_obj, UINT32 command) {
    int i;

    //Show all FeatureFlag status
    for(i=0;i<VPP_DEBUG_UTIL_FF_MAX;i++) {
        VPP_DEBUG_FEATURE_FLAG_STATE *pState = &VPPDBG_FF_STATE[i];
        VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_10, VPP_DEBUG_UTIL_TOGGLE_PP_SHOW_VAR " %s: print:%d, feature:%d\n",
            g_VppDbgUtilFFStrArray[i], pState->printEnable, pState->monitorEnable);
    }
}

BOOL VPP_DEBUG_UTIL_isFeatureFlagEnabled(VPP_OBJ *vpp_obj, VPP_DEBUG_UTIL_FF feature) {
    VPP_DEBUG_FEATURE_FLAG_STATE *pState = &VPPDBG_FF_STATE[feature];
    BOOL retFlag;

    if(feature >= VPP_DEBUG_UTIL_FF_MAX) {
        //Default: feature enabled
        retFlag = 1;
    } else {
        retFlag = pState->monitorEnable;
    }

    return retFlag;
}

#endif //VPP_DEBUG_UTIL_ENABLE_FeatureFlag

#ifdef VPP_DEBUG_UTIL_ENABLE_processPrintCmd
void VPP_DEBUG_UTIL_processPrintCmd(VPP_OBJ *vpp_obj, UINT32 printCmd) {
    int printMainCmd = printCmd&0xF0;
    int printSubCmd = (printCmd&0xFF00)>>8;
    int printSubCmd1 = (printCmd&0xFF0000)>>16;
    int printSubCmd2 = (printCmd&0xFF000000)>>24;
    int printMainSubCmd = printCmd&0x0F;

    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_7, "COMMAND:MAIN: %x => %x, %x\n",printCmd, printMainCmd, printMainSubCmd);
    VPP_DEBUG_UTIL_PRINTF(VPP_DEBUG_UTIL_PP_7, "COMMAND:SUB: %x, %x, %x\n", printSubCmd, printSubCmd1, printSubCmd2);

    //MSB nibble - cmd, LSB nibble - sub cmd
    switch(printMainCmd) {
#ifdef VPP_DEBUG_UTIL_ENABLE_printRegDump
        case VPP_DEBUG_UTIL_PRINT_CMD_REGDUMP:
            VPP_DEBUG_UTIL_printRegDumpMain(vpp_obj, printMainSubCmd);
        break;
#endif //VPP_DEBUG_UTIL_ENABLE_printRegDump

#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
        case VPP_DEBUG_UTIL_PRINT_CMD_VPP_INTR_STATS:
            //LSB nibble contains the command for vpp_intr_monitor module
            //Pass complete command to the vpp_intr_monitor module
            VPPDRV_vppIntrShowDelaysGen(vpp_obj, printCmd);
        break;
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameDrop
        case VPP_DEBUG_UTIL_PRINT_CMD_FRAME_REPEAT:
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameDrop
#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
        case VPP_DEBUG_UTIL_PRINT_CMD_FRAME_DROP:
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
#ifdef VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo
        case VPP_DEBUG_UTIL_PRINT_CMD_VBUFINFO:
#endif //VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo
#ifdef VPP_DEBUG_UTIL_ENABLE_printWindowChange
        case VPP_DEBUG_UTIL_PRINT_CMD_WIN_CHANGE:
#endif //VPP_DEBUG_UTIL_ENABLE_printWindowChange
#ifdef VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss
        case VPP_DEBUG_UTIL_PRINT_CMD_INTR_MISS:
#endif //VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss
#ifdef VPP_DEBUG_UTIL_ENABLE_FeatureFlag
        case VPP_DEBUG_UTIL_PRINT_CMD_FEATURES:
#endif //VPP_DEBUG_UTIL_ENABLE_FeatureFlag
#ifdef VPP_DEBUG_UTIL_ENABLE_printSclChange
        case VPP_DEBUG_UTIL_PRINT_CMD_SCL_CHANGE:
#endif //VPP_DEBUG_UTIL_ENABLE_printSclChange
#ifdef VPP_DEBUG_UTIL_ENABLE_frameBuffer
        case VPP_DEBUG_UTIL_PRINT_CMD_FRAME_BUFFER:
#endif //VPP_DEBUG_UTIL_ENABLE_frameBuffer
        default:
        printMainCmd = VPP_DEBUG_UTIL_getMainCmdNdx(printMainCmd);
        if((printMainCmd<VPP_DEBUG_UTIL_MAX) && (printMainSubCmd<VPP_DEBUG_UTIL_PRINT_CMD_MAX)) {
            //Note VPP_DEBUG_UTIL_PRINT_CMD_PRINT=0, so following translation is not required
            //printMainSubCmd = (printMainSubCmd==0?VPP_DEBUG_UTIL_PRINT_CMD_PRINT:printMainSubCmd);

            void(*VPP_DEBUG_UTIL_CMD_FUNCTION)(VPP_OBJ *vpp_obj, UINT32 command) =
                (void (*)(VPP_OBJ *, UINT32))VPPDBG_CMD_FUNC[printMainCmd][printMainSubCmd];

            if(VPP_DEBUG_UTIL_CMD_FUNCTION) {
                //Got a valid pointer - feature is enabled so invoke the function
                VPP_DEBUG_UTIL_CMD_FUNCTION(vpp_obj, printSubCmd);
            }
        }
        break;
    }
}
#endif //VPP_DEBUG_UTIL_ENABLE_processPrintCmd

void VPP_DEBUG_UTIL_init(VPP_OBJ *vpp_obj) {
    int utilCnt, cmdCnt;

    //Initialize all the command pointer to NULL
    for(utilCnt=0;utilCnt<VPP_DEBUG_UTIL_MAX;utilCnt++) {
        for(cmdCnt=0;cmdCnt<VPP_DEBUG_UTIL_PRINT_CMD_MAX;cmdCnt++) {
            VPPDBG_CMD_FUNC[utilCnt][cmdCnt] = (VPP_DEBUG_UTIL_CMD_FUNC)NULL;
        }
    }
#ifdef VPP_DEBUG_UTIL_ENABLE_printRegDump
#endif //VPP_DEBUG_UTIL_ENABLE_printRegDump

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameDrop
    VPP_DEBUG_UTIL_frameRepeatInit(vpp_obj);
    utilCnt = VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_FRAME_DROP);
    VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(utilCnt, frameDrop);
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameDrop

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
    VPP_DEBUG_UTIL_frameDropInit(vpp_obj);
    utilCnt = VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_FRAME_REPEAT);
    VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(utilCnt, frameRepeat);
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameRepeat

#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR
    VPPDRV_vppIntrInit(vpp_obj);
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR

#ifdef VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo
    VPP_DEBUG_UTIL_vbufInfoInit(vpp_obj);
    utilCnt = VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_VBUFINFO);
    VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(utilCnt, vbufInfo);
#endif //VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo

#ifdef VPP_DEBUG_UTIL_ENABLE_printWindowChange
    VPP_DEBUG_UTIL_winChangeInit(vpp_obj);
    utilCnt = VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_WIN_CHANGE);
    VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(utilCnt, winChange);
#endif //VPP_DEBUG_UTIL_ENABLE_printWindowChange

#ifdef VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss
    VPP_DEBUG_UTIL_vbiVdeIntrMissInit(vpp_obj);
    utilCnt = VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_INTR_MISS);
    VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(utilCnt, vbiVdeIntrMiss);
#endif //VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss

#ifdef VPP_DEBUG_UTIL_ENABLE_FeatureFlag
    VPP_DEBUG_UTIL_featureFlagInit(vpp_obj);
    utilCnt = VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_FEATURES);
    VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(utilCnt, featureFlag);
#endif //VPP_DEBUG_UTIL_ENABLE_FeatureFlag

#ifdef VPP_DEBUG_UTIL_ENABLE_printSclChange
    VPP_DEBUG_UTIL_sclChangeInit(vpp_obj);
    utilCnt = VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_SCL_CHANGE);
    VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(utilCnt, sclChange);
#endif //VPP_DEBUG_UTIL_ENABLE_printSclChange

#ifdef VPP_DEBUG_UTIL_ENABLE_frameBuffer
    VPP_DEBUG_UTIL_frameBufferInit(vpp_obj);
    utilCnt = VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_FRAME_BUFFER);
    VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(utilCnt, frameBuffer);
#endif //VPP_DEBUG_UTIL_ENABLE_frameBuffer

}

