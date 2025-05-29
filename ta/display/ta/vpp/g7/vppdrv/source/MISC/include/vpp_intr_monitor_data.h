/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_intr_monitor_data.h
//! \brief      The VPP interrupt monitorng framework header file for data
//!             Single code for TZ(CA&TA)/NTZ.
//!             DISPSRV to be added
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_INTR_MONITOR_DATA_H_
#define _VPP_INTR_MONITOR_DATA_H_

//Number of profiling unit/blocks allowed
#define VPPDRV_INTR_MAX_PROFILING_UNIT  10

typedef struct ___VPPDRV_INTR_MONITOR__ {
    UINT32 u32MinScheduleDelay[VPPDRV_INTR_MAX_PROFILING_UNIT];
    UINT32 u32MaxScheduleDelay[VPPDRV_INTR_MAX_PROFILING_UNIT];
    UINT32 u32MinProcessingDelay[VPPDRV_INTR_MAX_PROFILING_UNIT];
    UINT32 u32MaxProcessingDelay[VPPDRV_INTR_MAX_PROFILING_UNIT];
    UINT32 u32Count[VPPDRV_INTR_MAX_PROFILING_UNIT];
    UINT32 u32VppIntrCurrFPS;
    UINT32 u32VppIntrMaxDelayNsec;
    UINT32 u32StartingIteration;
    UINT32 u32VppIntrDynamicPrint;
    UINT32 u32VppIntrMonitorEnable;
    UINT32 u32VppIntrSkipCount;
    UINT32 u32VppIntriPeriodicPrintCount;
    UINT32 u32VppIntriPeriodicPrintIter;
    UINT32 u32VppIntriPeriodicPrintIterMax;
    UINT8 u8VppIntrSchedFlag;
    UINT8 u8VppIntrProcessFlag;
}VPPDRV_INTR_MONITOR;

#endif //_VPP_INTR_MONITOR_DATA_H_

