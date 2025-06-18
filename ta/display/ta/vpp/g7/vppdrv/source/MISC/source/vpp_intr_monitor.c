/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_intr_monitor.c
//! \brief      The VPP interrupt monitorng framework source file
//!             Single code for TZ(CA&TA)/NTZ.
//!             DISPSRV to be added
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_intr_monitor.h"

#if !defined(CONFIG_TEE)
#include "vpp_cfg.h"
#endif

#ifndef CONFIG_OPTEE
#ifdef VPP_IN_TRUST_ZONE
#include "tee_internal_api.h"
#include "tee_ta.h"
#include "tee_internal_core_common.h"
#endif
#endif
#ifndef VPP_IN_TRUST_ZONE
#ifndef VPP_DEBUG_ENABLE_AMP_API
#include "amp_time.h"
#endif
#endif

#ifndef VPP_IN_TRUST_ZONE
#ifndef VPP_DEBUG_ENABLE_AMP_API
#include "amp_time.h"
#endif
#endif

#if defined(ANDROID)
#ifndef DISP_ERROR
#include "isl/amp_logger.h"
#define DISP_ERROR(...)      \
        AMP_Log(MODULE_DISPSRV, AMP_LOG_ERROR, __VA_ARGS__)
#endif
#define VPPDRV_vppIntrPrint(...)            DISP_ERROR(__VA_ARGS__)
#define VPPDRV_vppIntrPrint_Dynamic(...)    if(VPPDBG_INTRMON_STATE.u32VppIntrDynamicPrint) DISP_ERROR(__VA_ARGS__)
#define VPPDRV_vppProfPrint(...)            DISP_ERROR(__VA_ARGS__)
#define VPPDRV_vppProfPrint_Dynamic(...)    if(VPPDBG_PROFMON_STATE.u32VppIntrDynamicPrint) DISP_ERROR(__VA_ARGS__)
#else
#define VPPDRV_vppIntrPrint(...)            VPPAPI_DBG_ERROR(__VA_ARGS__)
#define VPPDRV_vppIntrPrint_Dynamic(...)    if(VPPDBG_INTRMON_STATE.u32VppIntrDynamicPrint) VPPAPI_DBG_ERROR(__VA_ARGS__)
#define VPPDRV_vppProfPrint(...)            VPPAPI_DBG_ERROR(__VA_ARGS__)
#define VPPDRV_vppProfPrint_Dynamic(...)    if(VPPDBG_PROFMON_STATE.u32VppIntrDynamicPrint) VPPAPI_DBG_ERROR(__VA_ARGS__)
#endif

#if defined(CONFIG_TEE)
const char *g_drv_vppIntrPrintPrefix = "VPPDBG:CA:VPP_INTR_DELAY:";
const char *g_drv_vppProfPrintPrefix = "VPPDBG:CA:VPP_PROF_DELAY:";
#elif (CONFIG_AMP_TEE_ENABLE)
const char *g_drv_vppIntrPrintPrefix = "VPPDBG:TA:VPP_INTR_DELAY:";
const char *g_drv_vppProfPrintPrefix = "VPPDBG:TA:VPP_PROF_DELAY:";
#else //CONFIG_AMP_TEE_ENABLE
const char *g_drv_vppIntrPrintPrefix = "VPPDBG:NTZ:VPP_INTR_DELAY:";
const char *g_drv_vppProfPrintPrefix = "VPPDBG:NTZ:VPP_PROF_DELAY:";
#endif //CONFIG_TEE

#ifndef VPP_IN_TRUST_ZONE
#define VPPDRV_VPPDBG_PERIODIC_PRN_FMT_STR1  "%s:%d:%s PERIODIC: Intr:%d, (fps:%d/%d)\n", __func__, __LINE__, g_drv_vppIntrPrintPrefix
#define VPPDRV_VPPDBG_PERIODIC_PRN_FMT_STR2  "%s:%d:%s PERIODIC: unit:%d, Slimit(%d,%d), Plimit(%d,%d)\n", __func__, __LINE__, g_drv_vppIntrPrintPrefix
#else
#define VPPDRV_VPPDBG_PERIODIC_PRN_FMT_STR1  "VPPDBG:TAVPP:PERIODIC: Intr:%d, (fps:%d/%d)\n"
#define VPPDRV_VPPDBG_PERIODIC_PRN_FMT_STR2  "VPPDBG:TAVPP:PERIODIC: unit:%d, Slimit(%d,%d), Plimit(%d,%d)\n"
#endif

#ifdef VPPDRV_VPP_INTR_FREQ_MONITOR

#define VPP_INTR_TOLLANCE_MS    1
#define VPP_INTR_MAX_DELAY      0
#define VPP_INTR_MIN_DELAY      0xFFFFFFFF

//The monitoring function is invoked twice per interrupt
#define VPP_INTR_SKIP_COUNT     (60 * 2)

#define VPP_DEBUG_UTIL_ENABLE    1
#define VPP_DEBUG_UTIL_DISABLE   0

#ifdef VPP_INTR_FREQ_MONITOR_BOOTUP_ENABLE
#define VPP_INTR_FREQ_MONITOR_DEFAULT_CONFIG   VPP_DEBUG_UTIL_ENABLE
#else
#define VPP_INTR_FREQ_MONITOR_DEFAULT_CONFIG   VPP_DEBUG_UTIL_DISABLE
#endif

#define VPP_DEBUG_UTIL_PRINT_INTR_MON_PRINT_DEFAULT     VPP_INTR_FREQ_MONITOR_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_INTR_MON_MONITOR_DEFAULT   VPP_INTR_FREQ_MONITOR_DEFAULT_CONFIG

#ifdef VPP_INTR_FREQ_MONITOR_PERIODIC_PRN_BOOTUP_ENABLE
//Enable periodic print on bootup - 10sec periodically
#define VPP_INTR_FREQ_MONITOR_PERIODIC_PRN_DEFAULT_CONFIG   10
#else
//Disable periodic print on bootup
#define VPP_INTR_FREQ_MONITOR_PERIODIC_PRN_DEFAULT_CONFIG   0
#endif

//Disable periodic print on bootup
#define VPP_DEBUG_UTIL_PRINT_INTR_PERIOD_PRINT_DEFAULT      VPP_INTR_FREQ_MONITOR_PERIODIC_PRN_DEFAULT_CONFIG

UINT32 VPPDRV_vppProfShowDelaysGen(VPP_OBJ *vpp_obj, int resetInfo);
void VPPDRV_vppProfInitReset(VPP_OBJ *vpp_obj);

static void VPPDRV_vppIntrFreqMonitorInitEntry(VPP_OBJ *vpp_obj, int i) {
    VPPDBG_INTRMON_STATE.u32MinScheduleDelay[i] = VPP_INTR_MIN_DELAY;
    VPPDBG_INTRMON_STATE.u32MaxScheduleDelay[i] = VPP_INTR_MAX_DELAY;
    VPPDBG_INTRMON_STATE.u32MinProcessingDelay[i] = VPP_INTR_MIN_DELAY;
    VPPDBG_INTRMON_STATE.u32MaxProcessingDelay[i] = VPP_INTR_MAX_DELAY;
    VPPDBG_INTRMON_STATE.u32Count[i] = 0;
}

static void VPPDRV_vppIntrFreqMonitorInit(VPP_OBJ *vpp_obj) {
    int i;

    for(i=0;i<VPPDRV_INTR_MAX_PROFILING_UNIT;i++) {
        VPPDRV_vppIntrFreqMonitorInitEntry(vpp_obj, i);
    }
}

static int get_fps_from_fpstable(VPP_OBJ *vpp_obj) {

    int pts_per_cnt_4;
    int fps;    //fps*1000

#if !defined(CONFIG_TEE)
    int resID = VPP_GetResId(CPCB_1);
    RESOLUTION_INFO  *pResinfo;

    pResinfo = VPP_GetResInfo(resID);
    pts_per_cnt_4 =  pResinfo->pts_per_cnt_4;
#else
    MV_VPPOBJ_GetCPCBOutputFrameRate((int)vpp_obj, 0, &pts_per_cnt_4);
#endif
    fps = ((4.0*90)/pts_per_cnt_4) * 1000000;

    return fps;
}

UINT32 VPPDRV_vppIntrGetCurrFPS(VPP_OBJ *vpp_obj)
{
    UINT32 fps;

    //Reset the min/max counters after interrupt stabilizes
    VPPDBG_INTRMON_STATE.u32StartingIteration = VPPDBG_INTRMON_STATE.u32VppIntrSkipCount;

    VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec =  VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS = 0;

    {
        fps = get_fps_from_fpstable(vpp_obj);

        VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS = fps;
        VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec = ((1.0*1000000000)/fps)*1000;
        #ifdef VPP_INTR_TOLLANCE_MS
            VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec = (VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec)+(VPP_INTR_TOLLANCE_MS*1000000);
        #else
            VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec = (VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec)*((100+VPP_INTR_TOLLANCE_PERCENTAGE)/100.0);
        #endif

        VPPDRV_vppIntrPrint("%s:%d: %s fps=%d/%dns\n", __func__, __LINE__, g_drv_vppIntrPrintPrefix, fps, VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec);

    }
    VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIterMax = (VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintCount*VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS/1000);

    return fps;
}

UINT32 VPPDRV_vppIntrMonGetStatus(VPP_OBJ *vpp_obj) {
    return VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable;
}

void VPPDRV_vppIntrMonSetStatus(VPP_OBJ *vpp_obj, UINT32 state) {
    VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable = state;
}


UINT32 VPPDRV_vppIntrFreqMonitorGen(VPP_OBJ *vpp_obj, int unitNum, int start, int show)
{
#ifndef VPP_IN_TRUST_ZONE
    #ifndef VPP_DEBUG_ENABLE_AMP_API
    static struct timespec prevFrame_time[VPPDRV_INTR_MAX_PROFILING_UNIT];
    struct timespec  currFrame_time, ts_delta;
    #else
    static struct timeval prevFrame_time[VPPDRV_INTR_MAX_PROFILING_UNIT];
    struct timeval currFrame_time;
    #endif
#else
    static TEE_Time prevFrame_time[VPPDRV_INTR_MAX_PROFILING_UNIT];
    TEE_Time currFrame_time;
#endif

    UINT32 *pMinDelay;
    UINT32 *pMaxDelay;
    UINT32 tmpMaxDiffDelay;
    UINT32 timeDiffinNS;
    UINT32 showFlag;

    if(!VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable) {
        //Profiling not enabled, then return immediatly
       return 0;
    }

    if((unitNum >= VPPDRV_INTR_MAX_PROFILING_UNIT) || (!VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS)) {
        //Invalid unit to profile; so return immediatly
        return 0;
    }

    //We might have multiple unit for monitoring, so apply skipping for only 1st unit-VPPDRV_INTR_MONITOR_UINT0_VBI_INTR
    if(VPPDBG_INTRMON_STATE.u32StartingIteration) {
        if(unitNum == VPPDRV_INTR_MONITOR_UINT0_VBI_INTR) {
            VPPDBG_INTRMON_STATE.u32StartingIteration--;
            if(!VPPDBG_INTRMON_STATE.u32StartingIteration) {
                //Reset all min/max delay counts
                VPPDRV_vppIntrFreqMonitorInit(vpp_obj);
            }
        }
        //Please wait for stabilisation time
        return 0;
    }

    if(start) {
        pMinDelay = &VPPDBG_INTRMON_STATE.u32MinScheduleDelay[unitNum];
        pMaxDelay = &VPPDBG_INTRMON_STATE.u32MaxScheduleDelay[unitNum];
        tmpMaxDiffDelay = VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec;
        VPPDBG_INTRMON_STATE.u32Count[unitNum]++;
        showFlag = VPPDBG_INTRMON_STATE.u8VppIntrSchedFlag;
    } else {
        pMinDelay = &VPPDBG_INTRMON_STATE.u32MinProcessingDelay[unitNum];
        pMaxDelay = &VPPDBG_INTRMON_STATE.u32MaxProcessingDelay[unitNum];
        //We can have same or different timeout for scheduling & processing
        tmpMaxDiffDelay = VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec;
        showFlag = VPPDBG_INTRMON_STATE.u8VppIntrProcessFlag;
    }
    {
#ifndef VPP_IN_TRUST_ZONE
        #ifndef VPP_DEBUG_ENABLE_AMP_API
            AMP_GetCurrentTime(&currFrame_time);
            MV_TimeSpec_Normaized(&ts_delta, currFrame_time.tv_sec-prevFrame_time[unitNum].tv_sec, currFrame_time.tv_nsec-prevFrame_time[unitNum].tv_nsec);
            timeDiffinNS = ts_delta.tv_nsec;
            //double timeDeltaMs = ts_delta.tv_nsec/1000000.0;
        #else
            gettimeofday(&currFrame_time,NULL);
            //timeDiffinNS = ((currFrame_time.tv_sec - prevFrame_time[unitNum].tv_sec)*1000) + ((currFrame_time.tv_usec - prevFrame_time[unitNum].tv_usec)/1000);
            timeDiffinNS = (((currFrame_time.tv_sec - prevFrame_time[unitNum].tv_sec)) + ((currFrame_time.tv_usec - prevFrame_time[unitNum].tv_usec))) * 1000;
        #endif
#else

        TEE_GetREETime(&currFrame_time);
        timeDiffinNS = ((currFrame_time.seconds*1000 + currFrame_time.millis) - (prevFrame_time[unitNum].seconds*1000 + prevFrame_time[unitNum].millis)) * 1000 * 1000;
        //timeDiffinMS = ((currFrame_time.seconds*1000 + currFrame_time.millis) - (prevFrame_time[unitNum].seconds*1000 + prevFrame_time[unitNum].millis)) ;
#endif
    }

    {
        //Init the variables for the 1st iteration, Start processing from 2'nd interrupt onwards
        if(VPPDBG_INTRMON_STATE.u32Count[unitNum]>1) {
            if(timeDiffinNS > *pMaxDelay) {
                *pMaxDelay = timeDiffinNS;
            }
            if(timeDiffinNS < *pMinDelay) {
                *pMinDelay = timeDiffinNS;
            }
            if(show || (showFlag && (timeDiffinNS >= tmpMaxDiffDelay))) {
#ifndef VPP_IN_TRUST_ZONE
                VPPDRV_vppIntrPrint_Dynamic("%s:%d: %s %s timeout(ns)[unit=%d,intr=%d]: %d(fps:%d/%d)\n",
                        __func__, __LINE__, g_drv_vppIntrPrintPrefix, start? "scheduled" : "processing",
                        unitNum, VPPDBG_INTRMON_STATE.u32Count[unitNum], timeDiffinNS, VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS, tmpMaxDiffDelay);
                VPPDRV_vppIntrPrint_Dynamic("%s:%d: %s : Schedule_limit(min:%d,max:%d), Process_limit(min:%d,max:%d)\n",
                        __func__, __LINE__, g_drv_vppIntrPrintPrefix,
                        VPPDBG_INTRMON_STATE.u32MinScheduleDelay[unitNum], VPPDBG_INTRMON_STATE.u32MaxScheduleDelay[unitNum],
                        VPPDBG_INTRMON_STATE.u32MinProcessingDelay[unitNum], VPPDBG_INTRMON_STATE.u32MaxProcessingDelay[unitNum]);
#else
                VPPDRV_vppIntrPrint_Dynamic("VPPDBG:TAVPP:%s U%d C%d D%d\n", start? "S" : "P", unitNum, VPPDBG_INTRMON_STATE.u32Count[unitNum], timeDiffinNS );
#endif
            }
        }

        if(start) {
            prevFrame_time[unitNum] = currFrame_time;

            //Show minimum information periodically
            if((VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintCount > 0) && (unitNum == VPPDRV_INTR_MONITOR_UINT0_VBI_INTR)) {
                if(VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIter >= (VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIterMax)) {
                    VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIter = 0x0;
                    VPPDRV_vppIntrPrint_Dynamic(VPPDRV_VPPDBG_PERIODIC_PRN_FMT_STR1,
                        VPPDBG_INTRMON_STATE.u32Count[VPPDRV_INTR_MONITOR_UINT0_VBI_INTR],
                        VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS, VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec);
                    for(unitNum=0;unitNum<VPPDRV_INTR_MAX_PROFILING_UNIT;unitNum++) {
                        if(VPPDBG_INTRMON_STATE.u32Count[unitNum]) {
                            VPPDRV_vppIntrPrint(VPPDRV_VPPDBG_PERIODIC_PRN_FMT_STR2, unitNum,
                                VPPDBG_INTRMON_STATE.u32MinScheduleDelay[unitNum], VPPDBG_INTRMON_STATE.u32MaxScheduleDelay[unitNum],
                                VPPDBG_INTRMON_STATE.u32MinProcessingDelay[unitNum], VPPDBG_INTRMON_STATE.u32MaxProcessingDelay[unitNum]);
                        }
                    }
                } else {
                    VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIter++;
                }
            }
        }
    }

    return timeDiffinNS;
}

UINT32 VPPDRV_vppIntrFreqMonitor(VPP_OBJ *vpp_obj, int start) {
    return VPPDRV_vppIntrFreqMonitorGen(vpp_obj, VPPDRV_INTR_MONITOR_UINT0_VBI_INTR, start, VPPDRV_INTR_MONITOR_NOSHOW);
}

UINT32 VPPDRV_vppIntrShowDelaysGen(VPP_OBJ *vpp_obj, int resetInfo) {
    int unitNum;
    int printMainSubCmd = resetInfo&0x0F;

    if(!printMainSubCmd) {
        for(unitNum=0;unitNum<VPPDRV_INTR_MAX_PROFILING_UNIT;unitNum++) {
            if(VPPDBG_INTRMON_STATE.u32Count[unitNum]) {
                VPPDRV_vppIntrPrint("%s:%d: %s timeout(ns)[unit=%d,intr=%d]: (fps:%d/%d)\n", __func__, __LINE__, g_drv_vppIntrPrintPrefix,
                        unitNum, VPPDBG_INTRMON_STATE.u32Count[unitNum], VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS, VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec);
                VPPDRV_vppIntrPrint("%s:%d: %s : Schedule_limit(min:%d,max:%d), Process_limit(min:%d,max:%d)\n", __func__, __LINE__, g_drv_vppIntrPrintPrefix,
                        VPPDBG_INTRMON_STATE.u32MinScheduleDelay[unitNum], VPPDBG_INTRMON_STATE.u32MaxScheduleDelay[unitNum], VPPDBG_INTRMON_STATE.u32MinProcessingDelay[unitNum], VPPDBG_INTRMON_STATE.u32MaxProcessingDelay[unitNum]);
            }
        }
    }

    if(printMainSubCmd == VPPDRV_INTR_CMD_TOGGLE_MONITOR) {
        VPPDRV_vppIntrPrint("%s:%d:%s Toggle interrupt monitoring %d->%d\n",  __func__, __LINE__, g_drv_vppIntrPrintPrefix,
            VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable, !VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable);
        //Enable/Disable intr monitoring
        VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable = !VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable;
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_CLEAR_COUNTERS) {
        VPPDRV_vppIntrPrint("%s:%d:%s Reset all min/max delay counts\n",  __func__, __LINE__, g_drv_vppIntrPrintPrefix);
        //Reset all min/max delay counts
        VPPDRV_vppIntrFreqMonitorInit(vpp_obj);
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_TOGGLE_PRINTS) {
        VPPDRV_vppIntrPrint("%s:%d:%s Toggle dynamic display %d->%d\n",  __func__, __LINE__, g_drv_vppIntrPrintPrefix,
            VPPDBG_INTRMON_STATE.u32VppIntrDynamicPrint, !VPPDBG_INTRMON_STATE.u32VppIntrDynamicPrint);
        VPPDBG_INTRMON_STATE.u32VppIntrDynamicPrint = !VPPDBG_INTRMON_STATE.u32VppIntrDynamicPrint;
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SHOW_VARIABLES) {
        VPPDRV_vppIntrPrint("%s:%d:%s: dynamic display : %d, intr monitoring : %d, skip_intr:%d, iter_inter:%d, CurrFPS:%d, MaxDelay:%d, PeriodicPrint:%d/%d->%d, Flag:S:%d/P:%d\n",
            __func__, __LINE__, g_drv_vppIntrPrintPrefix,
            VPPDBG_INTRMON_STATE.u32VppIntrDynamicPrint, VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable, VPPDBG_INTRMON_STATE.u32VppIntrSkipCount,
            VPPDBG_INTRMON_STATE.u32StartingIteration, VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS, VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec,
            VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIter, VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintCount, VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIterMax,
            VPPDBG_INTRMON_STATE.u8VppIntrSchedFlag, VPPDBG_INTRMON_STATE.u8VppIntrProcessFlag);
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SET_SKIP_COUNT) {
        VPPDRV_vppIntrPrint("%s:%d:%s: skip_intr:%d->%d\n",
            __func__, __LINE__, g_drv_vppIntrPrintPrefix,
            VPPDBG_INTRMON_STATE.u32VppIntrSkipCount, (resetInfo&0xFFFFFF00)>>8);
        VPPDBG_INTRMON_STATE.u32VppIntrSkipCount = (resetInfo&0xFFFFFF00)>>8;
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SET_PERIODIC_PRINT_COUNT) {
        VPPDRV_vppIntrPrint("%s:%d:%s: Periodicintr:%d->%d\n",
            __func__, __LINE__, g_drv_vppIntrPrintPrefix,
            VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintCount, (resetInfo&0xFFFFFF00)>>8);
        VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintCount = (resetInfo&0xFFFFFF00)>>8;
        VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIterMax = (VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintCount*VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS/1000);
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SET_SKIP_SCHED_LOG) {
        VPPDRV_vppIntrPrint("%s:%d:%s: SchedFlag:%d->%d\n",
            __func__, __LINE__, g_drv_vppIntrPrintPrefix,
            VPPDBG_INTRMON_STATE.u8VppIntrSchedFlag, (resetInfo&0xFFFFFF00)>>8);
        VPPDBG_INTRMON_STATE.u8VppIntrSchedFlag = (resetInfo&0xFFFFFF00)>>8;
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SET_SKIP_PROCESS_LOG) {
        VPPDRV_vppIntrPrint("%s:%d:%s: ProcessFlag:%d->%d\n",
            __func__, __LINE__, g_drv_vppIntrPrintPrefix,
            VPPDBG_INTRMON_STATE.u8VppIntrProcessFlag, (resetInfo&0xFFFFFF00)>>8);
        VPPDBG_INTRMON_STATE.u8VppIntrProcessFlag = (resetInfo&0xFFFFFF00)>>8;
    }


    VPPDRV_vppProfShowDelaysGen(vpp_obj, resetInfo);
    return 0;
}

UINT32 VPPDRV_vppIntrShowDelays(VPP_OBJ *vpp_obj) {
    return VPPDRV_vppIntrShowDelaysGen(vpp_obj, 0);
}

void VPPDRV_vppIntrResetSkipCount(VPP_OBJ *vpp_obj) {
    VPPDBG_INTRMON_STATE.u32VppIntrSkipCount        = VPP_INTR_SKIP_COUNT;
}

void VPPDRV_vppIntrInit(VPP_OBJ *vpp_obj) {
    //Reset all min/max delay counts
    VPPDRV_vppIntrFreqMonitorInit(vpp_obj);
    VPPDBG_INTRMON_STATE.u8VppIntrSchedFlag         = 0x1;
    VPPDBG_INTRMON_STATE.u8VppIntrProcessFlag       = 0x1;
    VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS          = 0x0;
    VPPDBG_INTRMON_STATE.u32VppIntrMaxDelayNsec     = 0x0;
#ifdef VPPDRV_INTR_MONITOR_FREEZE_ON_BOOTUP
    //On bootup delay monitoring for long period
    VPPDBG_INTRMON_STATE.u32VppIntrSkipCount        = (VPP_INTR_SKIP_COUNT * 10);
#else
    VPPDBG_INTRMON_STATE.u32VppIntrSkipCount        = VPP_INTR_SKIP_COUNT;
#endif
    VPPDBG_INTRMON_STATE.u32StartingIteration       = VPPDBG_INTRMON_STATE.u32VppIntrSkipCount;
    VPPDBG_INTRMON_STATE.u32VppIntrDynamicPrint     = VPP_DEBUG_UTIL_PRINT_INTR_MON_PRINT_DEFAULT;
    VPPDBG_INTRMON_STATE.u32VppIntrMonitorEnable    = VPP_DEBUG_UTIL_PRINT_INTR_MON_MONITOR_DEFAULT;
    VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintCount = VPP_DEBUG_UTIL_PRINT_INTR_PERIOD_PRINT_DEFAULT;
    VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIter  = 0;
    VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintIterMax = (VPPDBG_INTRMON_STATE.u32VppIntriPeriodicPrintCount*VPPDBG_INTRMON_STATE.u32VppIntrCurrFPS/1000);

    VPPDRV_vppProfInitReset(vpp_obj);
}


void VPPDRV_vppProfInit(VPP_OBJ *vpp_obj) {
    int i;

    for(i=0;i<VPPDRV_INTR_MAX_PROFILING_UNIT;i++) {
        VPPDBG_PROFMON_STATE.u32MinScheduleDelay[i] = VPP_INTR_MIN_DELAY;
        VPPDBG_PROFMON_STATE.u32MaxScheduleDelay[i] = VPP_INTR_MAX_DELAY;
        VPPDBG_PROFMON_STATE.u32MinProcessingDelay[i] = VPP_INTR_MIN_DELAY;
        VPPDBG_PROFMON_STATE.u32MaxProcessingDelay[i] = VPP_INTR_MAX_DELAY;
        VPPDBG_PROFMON_STATE.u32Count[i] = 0;
    }

#ifdef VPP_INTR_FREQ_MONITOR_BOOTUP_ENABLE
    VPPDBG_PROFMON_STATE.u32VppIntrMonitorEnable = 1;
    VPPDBG_PROFMON_STATE.u32VppIntrDynamicPrint = 1;
#endif
    VPPDBG_PROFMON_STATE.u8VppIntrSchedFlag         = 0x0;
    VPPDBG_PROFMON_STATE.u8VppIntrProcessFlag       = 0x1;
}

void VPPDRV_vppProfInitReset(VPP_OBJ *vpp_obj) {

    memcpy(&(VPPDBG_PROFMON_STATE), &(VPPDBG_INTRMON_STATE), sizeof(VPPDRV_INTR_MONITOR));

    //Log delay > 10ms
    VPPDBG_PROFMON_STATE.u32VppIntrMaxDelayNsec = 10*1000*1000;

    VPPDRV_vppProfInit(vpp_obj);
}

UINT32 VPPDRV_vppProfMonitorGen(VPP_OBJ *vpp_obj, int unitNum, int start, int show)
{
#ifndef VPP_IN_TRUST_ZONE
    #ifndef VPP_DEBUG_ENABLE_AMP_API
    static struct timespec prevFrame_time[VPPDRV_INTR_MAX_PROFILING_UNIT];
    struct timespec  currFrame_time, ts_delta;
    #else
    static struct timeval prevFrame_time[VPPDRV_INTR_MAX_PROFILING_UNIT];
    struct timeval currFrame_time;
    #endif
#else
    static TEE_Time prevFrame_time[VPPDRV_INTR_MAX_PROFILING_UNIT];
    TEE_Time currFrame_time;
#endif

    UINT32 *pMinDelay;
    UINT32 *pMaxDelay;
    UINT32 tmpMaxDiffDelay;
    UINT32 timeDiffinNS;
    UINT32 showFlag;
    UINT32 retVal = 0;

    if(!vpp_obj || !VPPDBG_PROFMON_STATE.u32VppIntrMonitorEnable) {
        //Profiling not enabled, then return immediatly
       return retVal;
    }

    if((unitNum >= VPPDRV_INTR_MAX_PROFILING_UNIT)) {
        //Invalid unit to profile; so return immediatly
        return retVal;
    }

    if(start) {
        pMinDelay = &VPPDBG_PROFMON_STATE.u32MinScheduleDelay[unitNum];
        pMaxDelay = &VPPDBG_PROFMON_STATE.u32MaxScheduleDelay[unitNum];
        tmpMaxDiffDelay = VPPDBG_PROFMON_STATE.u32VppIntrMaxDelayNsec;
        VPPDBG_PROFMON_STATE.u32Count[unitNum]++;
        showFlag = VPPDBG_PROFMON_STATE.u8VppIntrSchedFlag;
    } else {
        pMinDelay = &VPPDBG_PROFMON_STATE.u32MinProcessingDelay[unitNum];
        pMaxDelay = &VPPDBG_PROFMON_STATE.u32MaxProcessingDelay[unitNum];
        //We can have same or different timeout for scheduling & processing
        tmpMaxDiffDelay = VPPDBG_PROFMON_STATE.u32VppIntrMaxDelayNsec;
        showFlag = VPPDBG_PROFMON_STATE.u8VppIntrProcessFlag;
    }

    {
#ifndef VPP_IN_TRUST_ZONE
        #ifndef VPP_DEBUG_ENABLE_AMP_API
            AMP_GetCurrentTime(&currFrame_time);
            MV_TimeSpec_Normaized(&ts_delta, currFrame_time.tv_sec-prevFrame_time[unitNum].tv_sec, currFrame_time.tv_nsec-prevFrame_time[unitNum].tv_nsec);
            timeDiffinNS = ts_delta.tv_nsec;
            //double timeDeltaMs = ts_delta.tv_nsec/1000000.0;
        #else
            gettimeofday(&currFrame_time,NULL);
            //timeDiffinNS = ((currFrame_time.tv_sec - prevFrame_time[unitNum].tv_sec)*1000) + ((currFrame_time.tv_usec - prevFrame_time[unitNum].tv_usec)/1000);
            timeDiffinNS = (((currFrame_time.tv_sec - prevFrame_time[unitNum].tv_sec)) + ((currFrame_time.tv_usec - prevFrame_time[unitNum].tv_usec))) * 1000;
        #endif
#else

        TEE_GetREETime(&currFrame_time);
        timeDiffinNS = ((currFrame_time.seconds*1000 + currFrame_time.millis) - (prevFrame_time[unitNum].seconds*1000 + prevFrame_time[unitNum].millis)) * 1000 * 1000;
        //timeDiffinMS = ((currFrame_time.seconds*1000 + currFrame_time.millis) - (prevFrame_time[unitNum].seconds*1000 + prevFrame_time[unitNum].millis)) ;
#endif
    }

    {
        //Init the variables for the 1st iteration, Start processing from 2'nd interrupt onwards
        if(VPPDBG_PROFMON_STATE.u32Count[unitNum]>1) {
            if(timeDiffinNS > *pMaxDelay) {
                *pMaxDelay = timeDiffinNS;
            }
            if(timeDiffinNS < *pMinDelay) {
                *pMinDelay = timeDiffinNS;
            }
            //Log only processing delay timeout
            if(show || (showFlag && (timeDiffinNS >= tmpMaxDiffDelay))) {
#ifndef VPP_IN_TRUST_ZONE
                VPPDRV_vppProfPrint_Dynamic("%s:%d: %s %s timeout(ns)[unit=%d,intr=%d]: %d(fps:%d/%d)\n",
                        __func__, __LINE__, g_drv_vppProfPrintPrefix, start? "scheduled" : "processing",
                        unitNum, VPPDBG_PROFMON_STATE.u32Count[unitNum], timeDiffinNS, VPPDBG_PROFMON_STATE.u32VppIntrCurrFPS, tmpMaxDiffDelay);
                VPPDRV_vppProfPrint_Dynamic("%s:%d: %s : Schedule_limit(min:%d,max:%d), Process_limit(min:%d,max:%d)\n",
                        __func__, __LINE__, g_drv_vppProfPrintPrefix,
                        VPPDBG_PROFMON_STATE.u32MinScheduleDelay[unitNum], VPPDBG_PROFMON_STATE.u32MaxScheduleDelay[unitNum],
                        VPPDBG_PROFMON_STATE.u32MinProcessingDelay[unitNum], VPPDBG_PROFMON_STATE.u32MaxProcessingDelay[unitNum]);
#else
                VPPDRV_vppProfPrint_Dynamic("VPPDBG:TAVPP:%s U%d C%d D%d\n", start? "S" : "P", unitNum, VPPDBG_PROFMON_STATE.u32Count[unitNum], timeDiffinNS );
#endif
                retVal = 1;
            }
        }
        if(start) {
            prevFrame_time[unitNum] = currFrame_time;
        }
    }

    return retVal;
}

UINT32 VPPDRV_vppProfShowDelaysGen(VPP_OBJ *vpp_obj, int resetInfo) {
    int unitNum;
    int printMainSubCmd = resetInfo&0x0F;

    if(!printMainSubCmd) {
        for(unitNum=0;unitNum<VPPDRV_INTR_MAX_PROFILING_UNIT;unitNum++) {
            if(VPPDBG_PROFMON_STATE.u32Count[unitNum]) {
                VPPDRV_vppProfPrint("%s:%d: %s timeout(ns)[unit=%d,intr=%d]: (fps:%d/%d)\n", __func__, __LINE__, g_drv_vppProfPrintPrefix,
                        unitNum, VPPDBG_PROFMON_STATE.u32Count[unitNum], VPPDBG_PROFMON_STATE.u32VppIntrCurrFPS, VPPDBG_PROFMON_STATE.u32VppIntrMaxDelayNsec);
                VPPDRV_vppProfPrint("%s:%d: %s : Schedule_limit(min:%d,max:%d), Process_limit(min:%d,max:%d)\n", __func__, __LINE__, g_drv_vppProfPrintPrefix,
                        VPPDBG_PROFMON_STATE.u32MinScheduleDelay[unitNum], VPPDBG_PROFMON_STATE.u32MaxScheduleDelay[unitNum], VPPDBG_PROFMON_STATE.u32MinProcessingDelay[unitNum], VPPDBG_PROFMON_STATE.u32MaxProcessingDelay[unitNum]);
            }
        }
    }

    if(printMainSubCmd == VPPDRV_INTR_CMD_TOGGLE_MONITOR) {
        VPPDRV_vppProfPrint("%s:%d:%s Toggle interrupt monitoring %d->%d\n",  __func__, __LINE__, g_drv_vppProfPrintPrefix,
            VPPDBG_PROFMON_STATE.u32VppIntrMonitorEnable, !VPPDBG_PROFMON_STATE.u32VppIntrMonitorEnable);
        //Enable/Disable intr monitoring
        VPPDBG_PROFMON_STATE.u32VppIntrMonitorEnable = !VPPDBG_PROFMON_STATE.u32VppIntrMonitorEnable;
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_CLEAR_COUNTERS) {
        VPPDRV_vppProfPrint("%s:%d:%s Reset all min/max delay counts\n",  __func__, __LINE__, g_drv_vppProfPrintPrefix);
        //Reset all min/max delay counts
        VPPDRV_vppProfInit(vpp_obj);
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_TOGGLE_PRINTS) {
        VPPDRV_vppProfPrint("%s:%d:%s Toggle dynamic display %d->%d\n",  __func__, __LINE__, g_drv_vppProfPrintPrefix,
            VPPDBG_PROFMON_STATE.u32VppIntrDynamicPrint, !VPPDBG_PROFMON_STATE.u32VppIntrDynamicPrint);
        VPPDBG_PROFMON_STATE.u32VppIntrDynamicPrint = !VPPDBG_PROFMON_STATE.u32VppIntrDynamicPrint;
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SHOW_VARIABLES) {
        VPPDRV_vppProfPrint("%s:%d:%s: dynamic display : %d, intr monitoring : %d, skip_intr:%d, iter_inter:%d, CurrFPS:%d, MaxDelay:%d, PeriodicPrint:%d/%d->%d, Flag:S:%d/P:%d\n",
            __func__, __LINE__, g_drv_vppProfPrintPrefix,
            VPPDBG_PROFMON_STATE.u32VppIntrDynamicPrint, VPPDBG_PROFMON_STATE.u32VppIntrMonitorEnable, VPPDBG_PROFMON_STATE.u32VppIntrSkipCount,
            VPPDBG_PROFMON_STATE.u32StartingIteration, VPPDBG_PROFMON_STATE.u32VppIntrCurrFPS, VPPDBG_PROFMON_STATE.u32VppIntrMaxDelayNsec,
            VPPDBG_PROFMON_STATE.u32VppIntriPeriodicPrintIter, VPPDBG_PROFMON_STATE.u32VppIntriPeriodicPrintCount, VPPDBG_PROFMON_STATE.u32VppIntriPeriodicPrintIterMax,
            VPPDBG_PROFMON_STATE.u8VppIntrSchedFlag, VPPDBG_PROFMON_STATE.u8VppIntrProcessFlag);
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SET_SKIP_COUNT) {
        VPPDRV_vppProfPrint("%s:%d:%s: skip_intr:%d->%d\n",
            __func__, __LINE__, g_drv_vppProfPrintPrefix,
            VPPDBG_PROFMON_STATE.u32VppIntrSkipCount, (resetInfo&0xFFFFFF00)>>8);
        VPPDBG_PROFMON_STATE.u32VppIntrSkipCount = (resetInfo&0xFFFFFF00)>>8;
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SET_PERIODIC_PRINT_COUNT) {
        VPPDRV_vppProfPrint("%s:%d:%s: PeriodicPrint:%d->%d\n",
            __func__, __LINE__, g_drv_vppProfPrintPrefix,
            VPPDBG_PROFMON_STATE.u32VppIntriPeriodicPrintCount, (resetInfo&0xFFFFFF00)>>8);
        VPPDBG_PROFMON_STATE.u32VppIntriPeriodicPrintCount = (resetInfo&0xFFFFFF00)>>8;
        VPPDBG_PROFMON_STATE.u32VppIntriPeriodicPrintIterMax = (VPPDBG_PROFMON_STATE.u32VppIntriPeriodicPrintCount*VPPDBG_PROFMON_STATE.u32VppIntrCurrFPS/1000);
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SET_SKIP_SCHED_LOG) {
        VPPDRV_vppProfPrint("%s:%d:%s: SchedFlag:%d->%d\n",
            __func__, __LINE__, g_drv_vppProfPrintPrefix,
            VPPDBG_PROFMON_STATE.u8VppIntrSchedFlag, (resetInfo&0xFFFFFF00)>>8);
        VPPDBG_PROFMON_STATE.u8VppIntrSchedFlag = (resetInfo&0xFFFFFF00)>>8;
    }
    if(printMainSubCmd == VPPDRV_INTR_CMD_SET_SKIP_PROCESS_LOG) {
        VPPDRV_vppProfPrint("%s:%d:%s: ProcessFlag:%d->%d\n",
            __func__, __LINE__, g_drv_vppProfPrintPrefix,
            VPPDBG_PROFMON_STATE.u8VppIntrProcessFlag, (resetInfo&0xFFFFFF00)>>8);
        VPPDBG_PROFMON_STATE.u8VppIntrProcessFlag = (resetInfo&0xFFFFFF00)>>8;
    }

    return 0;
}
#endif //VPPDRV_VPP_INTR_FREQ_MONITOR

