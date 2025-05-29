/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_intr_monitor.h
//! \brief      The VPP interrupt monitorng framework header file
//!             Single code for TZ(CA&TA)/NTZ.
//!             DISPSRV to be added
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_INTR_MONITOR_H_
#define _VPP_INTR_MONITOR_H_

#include "vpp_api.h"
#include "vpp_intr_monitor_data.h"
#if !defined(CONFIG_TEE)
#include "vpp_module.h"
#endif

#if defined(TZ_CA_VPP_INTR_FREQ_MONITOR) || defined(TZ_VPP_INTR_FREQ_MONITOR) || defined (NTZ_VPP_INTR_FREQ_MONITOR)
#define VPPDRV_VPP_INTR_FREQ_MONITOR
#endif

#define VPPDRV_PROF_MONITOR_UINT0_TEE_CB_PROF      0
#define VPPDRV_PROF_MONITOR_UINT1_PTM_TA_PROF      1
#define VPPDRV_PROF_MONITOR_UINT2_VMX_TA_PROF      2
#define VPPDRV_PROF_MONITOR_UINT3_HDCP_TA_PROF     3

#define VPPDRV_INTR_MONITOR_UINT0_VBI_INTR      0
#define VPPDRV_INTR_MONITOR_UINT1_VDE_INTR      1
#define VPPDRV_INTR_MONITOR_UINT2_CA_TA_ONLY    2


#define VPPDRV_INTR_CMD_CLEAR_COUNTERS  0x1
#define VPPDRV_INTR_CMD_TOGGLE_PRINTS   0x2
#define VPPDRV_INTR_CMD_TOGGLE_MONITOR  0x3
#define VPPDRV_INTR_CMD_SHOW_VARIABLES  0x4
#define VPPDRV_INTR_CMD_SET_SKIP_COUNT  0x5
#define VPPDRV_INTR_CMD_SET_PERIODIC_PRINT_COUNT    0x6
#define VPPDRV_INTR_CMD_SET_SKIP_SCHED_LOG      0x7
#define VPPDRV_INTR_CMD_SET_SKIP_PROCESS_LOG    0x8

/*Enable/Disable VYSC monitor on bootup until initialization is complete
 * During bootup , setres may be invoked multiple times and etc.
 */
#define VPPDRV_INTR_MONITOR_FREEZE_ON_BOOTUP

//Start & Stop of the profiling block
#define VPPDRV_INTR_MONITOR_START   1
#define VPPDRV_INTR_MONITOR_STOP    0

//Show the profiling log - for each start/stop block
//This is useful to profile section of code invoked only once or only few iteration
#define VPPDRV_INTR_MONITOR_SHOW    1
#define VPPDRV_INTR_MONITOR_NOSHOW  0

#if defined(CONFIG_TEE)
//This is only for CA
typedef struct __VPP_OBJ__ {
    VPPDRV_INTR_MONITOR vppdbg_intrStatus;
    VPPDRV_INTR_MONITOR vppdbg_profStatus;
}VPP_OBJ;
extern VPP_OBJ g_vppdrv_vppobj;
#define VPPDBG_INTRMON_STATE        vpp_obj->vppdbg_intrStatus
#define VPPDBG_PROFMON_STATE        vpp_obj->vppdbg_profStatus
#else
#include "vpp_module.h"
#endif //CONFIG_TEE

#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR

//Invoke this, whenever resolution is changed, Resolution change will reset profiling data
UINT32 VPPDRV_vppIntrGetCurrFPS(VPP_OBJ *vpp_obj);
//Invoke this for start/end of the profiling block
UINT32 VPPDRV_vppIntrFreqMonitorGen(VPP_OBJ *vpp_obj, int unitNum, int start, int show);
//Invoke this to show the log of profiling data & also clear based on parameter
UINT32 VPPDRV_vppIntrShowDelaysGen(VPP_OBJ *vpp_obj, int resetInfo);

//Get the current interrupt monitoring state
UINT32 VPPDRV_vppIntrMonGetStatus(VPP_OBJ *vpp_obj);
//Set/Restore the current interrupt monitoring state
void VPPDRV_vppIntrMonSetStatus(VPP_OBJ *vpp_obj, UINT32 state);

/*Reset the skip count to the minimum interrupt
 * To be triggered after bootup stabilization
 * Note: MV_VPPOBJ_GetCurrentHDCPVersion - may be VPP API triggered on bootup
 */
void VPPDRV_vppIntrResetSkipCount(VPP_OBJ *vpp_obj);

void VPPDRV_vppIntrInit(VPP_OBJ *vpp_obj);

UINT32 VPPDRV_vppProfMonitorGen(VPP_OBJ *vpp_obj, int unitNum, int start, int show);
#else //!VPPDRV_VPP_INTR_FREQ_MONITOR

#define VPPDRV_vppIntrFreqMonitorGen(...)       1
#define VPPDRV_vppIntrShowDelaysGen(...)        1
#define VPPDRV_vppIntrMonGetStatus(...)         1
#define VPPDRV_vppIntrMonSetStatus(...)
#define VPPDRV_vppIntrResetSkipCount(...)
#define VPPDRV_vppIntrInit(...)
#define VPPDRV_vppProfMonitorGen(...) 0

#endif //!VPPDRV_VPP_INTR_FREQ_MONITOR

#endif //_VPP_INTR_MONITOR_H_

