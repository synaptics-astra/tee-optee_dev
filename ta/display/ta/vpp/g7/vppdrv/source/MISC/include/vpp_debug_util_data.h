/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_debug_util_data.h
//! \brief      data structure for various debug utility
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_DEBUG_UTIL_DATA_H_
#define _VPP_DEBUG_UTIL_DATA_H_

#include "vpp_api.h"
#include "vpp_scl_data.h"
#include "vpp_debug.h"

//Enable/Disable the various debug feature of VPP debug utils
#define VPP_DEBUG_UTIL_ENABLE_printWindowChange
#define VPP_DEBUG_UTIL_ENABLE_printRegDump
//#define VPP_DEBUG_UTIL_ENABLE_printVbufInfo
#define VPP_DEBUG_UTIL_ENABLE_printFrameDrop
#define VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
#define VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo
#define VPP_DEBUG_UTIL_ENABLE_processPrintCmd
#define VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss
#define VPP_DEBUG_UTIL_ENABLE_monitorBcmError
#define VPP_DEBUG_UTIL_ENABLE_validateBcmBuffer
#define VPP_DEBUG_UTIL_ENABLE_FeatureFlag
#define VPP_DEBUG_UTIL_ENABLE_monitorResetCPCB
#define VPP_DEBUG_UTIL_ENABLE_printSclChange
#define VPP_DEBUG_UTIL_ENABLE_frameBuffer

//Boot up monitor configuration(Enable/Disable) for various debug feature of VPP debug utils
#define VPP_DEBUG_UTIL_ENABLE    1
#define VPP_DEBUG_UTIL_DISABLE   0

#define VPP_DEBUG_UTIL_WIN_DEFAULT_CONFIG       VPP_DEBUG_UTIL_DISABLE
#define VPP_DEBUG_UTIL_VBUF_DEFAULT_CONFIG      VPP_DEBUG_UTIL_DISABLE
#define VPP_DEBUG_UTIL_DESC_DEFAULT_CONFIG      VPP_DEBUG_UTIL_DISABLE
#define VPP_DEBUG_UTIL_IMISS_DEFAULT_CONFIG     VPP_DEBUG_UTIL_ENABLE
#define VPP_DEBUG_UTIL_SCL_DEFAULT_CONFIG       VPP_DEBUG_UTIL_DISABLE
#define VPP_DEBUG_UTIL_FB_DEFAULT_CONFIG        VPP_DEBUG_UTIL_DISABLE

#define VPP_DEBUG_UTIL_FF_VALIDATE_BCMBUFF_DEFAULT_CONFIG   VPP_DEBUG_UTIL_DISABLE
#define VPP_DEBUG_UTIL_FF_AUTOPUSH_DEFAULT_CONFIG           VPP_DEBUG_UTIL_ENABLE
#define VPP_DEBUG_UTIL_FF_REFWIN_DEFAULT_CONFIG             VPP_DEBUG_UTIL_ENABLE
#define VPP_DEBUG_UTIL_FF_DISPWIN_DEFAULT_CONFIG            VPP_DEBUG_UTIL_ENABLE
#define VPP_DEBUG_UTIL_FF_PTMTA_DEFAULT_CONFIG              VPP_DEBUG_UTIL_ENABLE

#define VPP_DEBUG_UTIL_PRINT_WIN_CHANGE_PRINT_DEFAULT       VPP_DEBUG_UTIL_WIN_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_WIN_CHANGE_MONITOR_DEFAULT     VPP_DEBUG_UTIL_WIN_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_VBUF_INFO_PRINT_DEFAULT        VPP_DEBUG_UTIL_VBUF_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_VBUF_INFO_MONITOR_DEFAULT      VPP_DEBUG_UTIL_VBUF_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_FRAME_DESC_PRINT_DEFAULT       VPP_DEBUG_UTIL_DESC_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_FRAME_DESC_MONITOR_DEFAULT     VPP_DEBUG_UTIL_DESC_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_IMISS_PRINT_DEFAULT            VPP_DEBUG_UTIL_IMISS_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_IMISS_MONITOR_DEFAULT          VPP_DEBUG_UTIL_IMISS_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_SCL_CHANGE_PRINT_DEFAULT       VPP_DEBUG_UTIL_SCL_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_SCL_CHANGE_MONITOR_DEFAULT     VPP_DEBUG_UTIL_SCL_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_FB_PRINT_DEFAULT               VPP_DEBUG_UTIL_FB_DEFAULT_CONFIG
#define VPP_DEBUG_UTIL_PRINT_FB_MONITOR_DEFAULT             VPP_DEBUG_UTIL_FB_DEFAULT_CONFIG

//For TZ build enable the macro 'VPP_DEBUG_PRINT_ON_CONSOLE' in ta_amp Makefile or cat /dev/tzlogger
#define VPP_DEBUG_UTIL_PRINTF(VPP_DGB_PREFIX, ...) VPP_log(VPP_LOG_HDMI, VPP_LOG_INFO, VPP_DGB_PREFIX, __VA_ARGS__)

//Debug log prefix for various debug feature of VPP debug utils
#define VPP_DEBUG_UTIL_printPrefix_printWindowChange    "VPPDBG:WIN:"
#define VPP_DEBUG_UTIL_printPrefix_printRegDump         "VPPDBG:REG:"
#define VPP_DEBUG_UTIL_printPrefix_printVbufInfo        "VPPDBG:PBUFI:"
#define VPP_DEBUG_UTIL_printPrefix_printFrameDrop       "VPPDBG:FDROP:"
#define VPP_DEBUG_UTIL_printPrefix_printFrameRepeat     "VPPDBG:FRPT:"
#define VPP_DEBUG_UTIL_printPrefix_StoreVbufInfo        "VPPDBG:SBUFI:"
#define VPP_DEBUG_UTIL_printPrefix_processPrintCmd      "VPPDBG:PPRN:"
#define VPP_DEBUG_UTIL_printPrefix_printBcmBufDump      "VPPDBG:BCMBUF:"
#define VPP_DEBUG_UTIL_printPrefix_printVbiVdeIntrMiss  "VPPDBG:IMISS:"
#define VPP_DEBUG_UTIL_printPrefix_printFeatureFlag     "VPPDBG:FFLAG:"
#define VPP_DEBUG_UTIL_printPrefix_printSclChange       "VPPDBG:SCL:"
#define VPP_DEBUG_UTIL_printPrefix_frameBuffer          "VPPDBG:FB:"
#define VPP_DEBUG_UTIL_printPrefix_THDR                 "VPPDBG:THDR:"
#define VPP_DEBUG_UTIL_PP_2_VPP                          VPP_DEBUG_UTIL_PP_2 "VPP"
#define VPP_DEBUG_UTIL_PP_2_VPP_VPP                     "VPPDBG:REG:VPP"
#define VPP_DEBUG_UTIL_PP_2_VPP_MP                      "VPPDBG:REG:MP"
#define VPP_DEBUG_UTIL_PP_2_VPP_GFX                     "VPPDBG:REG:GFX"
#define VPP_DEBUG_UTIL_PP_2_VPP_HDRX                    "VPPDBG:REG:HDRX"
#define VPP_DEBUG_UTIL_PP_2_VPP_MTR                     "VPPDBG:REG:MTR"

#define VPP_DEBUG_UTIL_PP_1 VPP_DEBUG_UTIL_printPrefix_printWindowChange
#define VPP_DEBUG_UTIL_PP_2 VPP_DEBUG_UTIL_printPrefix_printRegDump
#define VPP_DEBUG_UTIL_PP_3 VPP_DEBUG_UTIL_printPrefix_printVbufInfo
#define VPP_DEBUG_UTIL_PP_4 VPP_DEBUG_UTIL_printPrefix_printFrameDrop
#define VPP_DEBUG_UTIL_PP_5 VPP_DEBUG_UTIL_printPrefix_printFrameRepeat
#define VPP_DEBUG_UTIL_PP_6 VPP_DEBUG_UTIL_printPrefix_StoreVbufInfo
#define VPP_DEBUG_UTIL_PP_7 VPP_DEBUG_UTIL_printPrefix_processPrintCmd
#define VPP_DEBUG_UTIL_PP_8 VPP_DEBUG_UTIL_printPrefix_printBcmBufDump
#define VPP_DEBUG_UTIL_PP_9 VPP_DEBUG_UTIL_printPrefix_printVbiVdeIntrMiss
#define VPP_DEBUG_UTIL_PP_10    VPP_DEBUG_UTIL_printPrefix_printFeatureFlag
#define VPP_DEBUG_UTIL_PP_11    VPP_DEBUG_UTIL_printPrefix_printSclChange
#define VPP_DEBUG_UTIL_PP_12    VPP_DEBUG_UTIL_printPrefix_frameBuffer
#define VPP_DEBUG_UTIL_PP_13    VPP_DEBUG_UTIL_printPrefix_THDR

//Debug log prefix for various generic sub-command for any PRINT_CMD
#define VPP_DEBUG_UTIL_TOGGLE_PP_INIT       "TOGGLE INIT"
#define VPP_DEBUG_UTIL_TOGGLE_PP_RESET      "TOGGLE RESET"
#define VPP_DEBUG_UTIL_TOGGLE_PP_PRINT      "TOGGLE PRINT"
#define VPP_DEBUG_UTIL_TOGGLE_PP_MONITOR    "TOGGLE MONITOR"
#define VPP_DEBUG_UTIL_TOGGLE_PP_SHOW_VAR   "SHOW VAR"


#define VPP_DEBUG_WINDOWS_REFWIN            0
#define VPP_DEBUG_WINDOWS_REFWIN_FROMISR    1
#define VPP_DEBUG_WINDOWS_DISPWIN           2
#define VPP_DEBUG_WINDOWS_DISPWIN_FROMISR   3
#define VPP_DEBUG_WINDOWS_CROPWIN           4

#define VPP_DEBUG_FRAMEDROP_STATE_PROCESS   0
#define VPP_DEBUG_FRAMEDROP_STATE_START     1

//Various generic sub command for any PRINT_CMD - Sub command is stored in LSB nibble
#define VPP_DEBUG_UTIL_PRINT_CMD_PRINT              0x0
#define VPP_DEBUG_UTIL_PRINT_CMD_RESET              0x1
#define VPP_DEBUG_UTIL_PRINT_CMD_TOGGLE_PRINTS      0x2
#define VPP_DEBUG_UTIL_PRINT_CMD_TOGGLE_MONITOR     0x3
#define VPP_DEBUG_UTIL_PRINT_CMD_SHOW_VARIABLES     0x4
#define VPP_DEBUG_UTIL_PRINT_CMD_SET_SKIP_COUNT     0x5
#define VPP_DEBUG_UTIL_PRINT_CMD_MAX                0x6

//The various commands of the PRINT_CMD
#define VPP_DEBUG_UTIL_PRINT_CMD_REGDUMP            0x10
#define VPP_DEBUG_UTIL_PRINT_CMD_FRAME_REPEAT       0x20
#define VPP_DEBUG_UTIL_PRINT_CMD_FRAME_DROP         0x30
#define VPP_DEBUG_UTIL_PRINT_CMD_VPP_INTR_STATS     0x40
#define VPP_DEBUG_UTIL_PRINT_CMD_VBUFINFO           0x50
#define VPP_DEBUG_UTIL_PRINT_CMD_DISP_INTR_STATS    0x60
#define VPP_DEBUG_UTIL_PRINT_CMD_WIN_CHANGE         0x70
#define VPP_DEBUG_UTIL_PRINT_CMD_VPP_INTR_STATS_CA  0x80
#define VPP_DEBUG_UTIL_PRINT_CMD_AVPLL              0x90
#define VPP_DEBUG_UTIL_PRINT_CMD_INTR_MISS          0xA0
#define VPP_DEBUG_UTIL_PRINT_CMD_FEATURES           0xB0
#define VPP_DEBUG_UTIL_PRINT_CMD_SCL_CHANGE         0xC0
#define VPP_DEBUG_UTIL_PRINT_CMD_FRAME_BUFFER       0xD0
#define VPP_DEBUG_UTIL_PRINT_CMD_MASK               0xF0

//Note MSB 4 bit used for Module numbering
#define VPP_DEBUG_UTIL_getMainCmdNdx(MODULE_NUM)    (MODULE_NUM>>4)
#define VPP_DEBUG_UTIL_MAX                          (VPP_DEBUG_UTIL_getMainCmdNdx(VPP_DEBUG_UTIL_PRINT_CMD_MASK)+1)

//Various macro for VPP_DEBUG_UTIL_ENABLE_printWindowChange
#define VPP_DEBUG_WINDOWS_PER_PLANE_MAX         5

//Various macro for VPP_DEBUG_UTIL_ENABLE_printFrameDrop & VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
#define VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE     10
#define VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE   10

//Function prototype for handling various command from user
typedef void(*VPP_DEBUG_UTIL_CMD_FUNC)(void *vpp_obj, UINT32 command);

//Various macro for VPP_DEBUG_UTIL_featureFlag
typedef enum __VPP_DEBUG_UTIL_FF__ {
    VPP_DEBUG_UTIL_FF_VALIDATE_BCMBUFF,
    VPP_DEBUG_UTIL_FF_AUTOPUSH,
    VPP_DEBUG_UTIL_FF_REFWIN,
    VPP_DEBUG_UTIL_FF_DISPWIN,
    VPP_DEBUG_UTIL_FF_PTMTA,
    VPP_DEBUG_UTIL_FF_MAX
}VPP_DEBUG_UTIL_FF;

/* Data structure for - VPP_DEBUG_UTIL_ENABLE_printWindowChange */
typedef struct __VPP_DEBUG_WIN_CHANGE_STATE__ {
    UINT8 winChangePrintEnable[MAX_NUM_PLANES][VPP_DEBUG_WINDOWS_PER_PLANE_MAX];
    UINT8 winChangeMonitorEnable[MAX_NUM_PLANES][VPP_DEBUG_WINDOWS_PER_PLANE_MAX];
    VPP_WIN plane_win[MAX_NUM_PLANES][VPP_DEBUG_WINDOWS_PER_PLANE_MAX];
}VPP_DEBUG_WIN_CHANGE_STATE;

/* Data structure for - VPP_DEBUG_UTIL_ENABLE_printSclChange */
typedef struct __VPP_DEBUG_SCL_CHANGE_STATE__ {
    UINT8 sclChangePrintEnable[VPP_FRC_SCL_MAX];
    UINT8 sclChangeMonitorEnable[VPP_FRC_SCL_MAX];
    VPP_SCL_RES scl_res[VPP_FRC_SCL_MAX];
    VPP_SCL_CTRL scl_ctrl[VPP_FRC_SCL_MAX];
    INT8 scl_mode, sclLineDropEn;
}VPP_DEBUG_SCL_CHANGE_STATE;

/* Data structure for - VPP_DEBUG_UTIL_ENABLE_printFrameDrop & VPP_DEBUG_UTIL_ENABLE_printFrameRepeat */
typedef struct __VPP_DEBUG_OCCURANCE_STATE__ {
    UINT32 occurCount;
    UINT32 isrOccurCount;
    UINT32 isrCurrOccurCount;
    UINT32 isrTotalCount;
    UINT32 frameDescTotalCount;
    UINT32 printEnable;
    UINT32 monitorEnable;
    VDEC_VID_BUF_DESC *frameDesc[VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE];
    UINT32 frameDescCount[VPP_DEBUG_UTIL_MAX_FRAMES_PER_PLANE];

    //Instance specific variables
    VDEC_VID_BUF_DESC *prevIsrDesc[MAX_NUM_PLANES];

    //Variable specifically for Frame repeat
    UINT32 newFrameFlag[MAX_NUM_PLANES];
    UINT32 prevFrameRepeatDiff[MAX_NUM_PLANES];
    UINT32 prevFrameRepeatCount[MAX_NUM_PLANES];
}VPP_DEBUG_OCCURANCE_STATE;

/* Data structure for - VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo */
typedef struct __VPP_DEBUG_VBUFINFO_STATE__ {
    UINT32 count;
    UINT32 nextNdx;
    UINT32 prevNdx;
    UINT32 printEnable;
    UINT32 monitorEnable;
    UINT32 countTotal;
    VBUF_INFO pVbufInfo[VPP_DEBUG_UTIL_MAX_VBUFINFO_PER_PLANE];
}VPP_DEBUG_VBUFINFO_STATE;

/* Data structure for - VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss*/
typedef struct __VPP_DEBUG_INTR_INSTANCE_STATE__ {
    UINT32 intrCount, vbiIntrCount, vdeIntrCount;
    //Internal Variable to count the number of interrupt w.r.t to other interrupt
    UINT32 tempVbiIntrCount, tempVdeIntrCount;
    //External Variable to count the number of interrupt miss
    UINT32 tempVbiIntrMissCount, tempVdeIntrMissCount;
    UINT32 printEnable;
    UINT32 monitorEnable;
}VPP_DEBUG_INTR_INSTANCE_STATE;

/* Data structure for - VPP_DEBUG_UTIL_featureFlag */
typedef struct __VPP_DEBUG_FEATURE_FLAG_STATE__ {
    UINT32 printEnable;
    UINT32 monitorEnable;
}VPP_DEBUG_FEATURE_FLAG_STATE;

/* Data structure for - various monitors*/
typedef struct __VPP_DEBUG_GEN_MONITOR__{
    UINT32 bcmErrorMonitorCount;
    UINT32 resetCPCBMonitorCount;
}VPP_DEBUG_GEN_MONITOR;

typedef struct __VPP_DEBUG_FRAME_BUFFER_STATE__ {
    UINT8 frameBufferPrintEnable[MAX_NUM_PLANES];
    UINT8 frameBufferMonitorEnable[MAX_NUM_PLANES];
    VBUF_INFO pinfo[MAX_NUM_PLANES];
    //Flag to indicate if VBUF_INFO is taken backup
    BOOL pinfo_init_flag[MAX_NUM_PLANES];
    //Flag to indicate if VBUF_INFO swapping is allowed
    BOOL pinfo_swap_flag[MAX_NUM_PLANES];
    //Flag to indicate if non built in frame is pushed to MAIN
    BOOL non_builtinFrame_pushedToMain;
}VPP_DEBUG_FRAME_BUFFER_STATE;

#endif //_VPP_DEBUG_UTIL_DATA_H_

