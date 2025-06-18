/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_debug_data.h
//! \brief      data structure for various debug modules
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_DEBUG_DATA_H_
#define _VPP_DEBUG_DATA_H_

#include "vpp_debug_util_data.h"
#include "vpp_intr_monitor_data.h"

typedef struct __VPP_DEBUG_DATA__ {
    VPPDRV_INTR_MONITOR vppdbg_intrStatus;
    VPPDRV_INTR_MONITOR vppdbg_profStatus;
    VPP_DEBUG_WIN_CHANGE_STATE VPP_DEBUG_winChangeState;
    VPP_DEBUG_OCCURANCE_STATE VPP_DEBUG_frameDropOccurState[MAX_NUM_PLANES];
    VPP_DEBUG_OCCURANCE_STATE VPP_DEBUG_frameRepeatOccurState[MAX_NUM_PLANES];
    VPP_DEBUG_VBUFINFO_STATE VPP_DEBUG_vbufInfoState[MAX_NUM_PLANES];
    VPP_DEBUG_INTR_INSTANCE_STATE VPP_DEBUG_intrInstanceState;
    VPP_DEBUG_FEATURE_FLAG_STATE VPP_DEBUG_featureFlagState[VPP_DEBUG_UTIL_FF_MAX];
    VPP_DEBUG_GEN_MONITOR VPP_DEBUG_monitors;
    VPP_DEBUG_SCL_CHANGE_STATE VPP_DEBUG_SclChangeState;
    VPP_DEBUG_FRAME_BUFFER_STATE VPP_DEBUG_frameBufferState;
    VPP_DEBUG_UTIL_CMD_FUNC cmdFunc[VPP_DEBUG_UTIL_MAX][VPP_DEBUG_UTIL_PRINT_CMD_MAX];
}VPP_DEBUG_DATA;

#define VPPDBG_PROFMON_STATE        g_vpp->vpp_objs[0]->vppdbg_data.vppdbg_profStatus
#define VPPDBG_INTRMON_STATE        g_vpp->vpp_objs[0]->vppdbg_data.vppdbg_intrStatus
#define VPPDBG_WINCNG_STATE         g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_winChangeState
#define VPPDBG_FDROP_STATE          g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_frameDropOccurState
#define VPPDBG_FREPT_STATE          g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_frameRepeatOccurState
#define VPPDBG_VBUF_STATE           g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_vbufInfoState
#define VPPDBG_INTRINS_STATE        g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_intrInstanceState
#define VPPDBG_FF_STATE             g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_featureFlagState
#define VPPDBG_GMON_STATE           g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_monitors
#define VPPDBG_SCLCNG_STATE         g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_SclChangeState
#define VPPDBG_FB_STATE             g_vpp->vpp_objs[0]->vppdbg_data.VPP_DEBUG_frameBufferState
#define VPPDBG_CMD_FUNC             g_vpp->vpp_objs[0]->vppdbg_data.cmdFunc

#endif //_VPP_DEBUG_DATA_H_

