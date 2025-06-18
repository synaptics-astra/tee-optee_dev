/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_debug_util_func.h
//! \brief      Function defination for various debug utility
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_DEBUG_UTIL_FUNC_H_
#define _VPP_DEBUG_UTIL_FUNC_H_

#include "vpp_api.h"
#include "vpp_scl_data.h"
#include "vpp_debug.h"
#include "vpp_debug_util_data.h"

//Get the valid SCL ID
#define VPP_DEBUG_getSclID(sclId, pSidStart, pSidEnd)       \
            VPP_DEBUG_getUnitID(sclId, VPP_FRC_SCL_MAX, pSidStart, pSidEnd)

//Get the valid Plane ID
#define VPP_DEBUG_getPlaneID(planeID, pPidStart, pPidEnd)       \
            VPP_DEBUG_getUnitID(planeID, MAX_NUM_PLANES, pPidStart, pPidEnd)

#define VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(MODULE) \
    void VPP_DEBUG_UTIL_ ##MODULE## Init(VPP_OBJ *vpp_obj);                             \
    void VPP_DEBUG_UTIL_ ##MODULE## StateReset(VPP_OBJ *vpp_obj, UINT32 command);       \
    void VPP_DEBUG_UTIL_ ##MODULE## TogglePrint(VPP_OBJ *vpp_obj, UINT32 command);      \
    void VPP_DEBUG_UTIL_ ##MODULE## ToggleMonitor(VPP_OBJ *vpp_obj, UINT32 command);    \
    void VPP_DEBUG_UTIL_ ##MODULE## ShowVar(VPP_OBJ *vpp_obj, UINT32 command);          \
    void VPP_DEBUG_UTIL_ ##MODULE## Print(VPP_OBJ *vpp_obj, UINT32 command);

#define VPP_DEBUG_UTIL_INIT_FUNCTION_POINTERS(MODULE_NUM, MODULE_NAME) \
    VPPDBG_CMD_FUNC[MODULE_NUM][VPP_DEBUG_UTIL_PRINT_CMD_RESET]         = (VPP_DEBUG_UTIL_CMD_FUNC)VPP_DEBUG_UTIL_ ##MODULE_NAME## StateReset;      \
    VPPDBG_CMD_FUNC[MODULE_NUM][VPP_DEBUG_UTIL_PRINT_CMD_TOGGLE_PRINTS] = (VPP_DEBUG_UTIL_CMD_FUNC)VPP_DEBUG_UTIL_ ##MODULE_NAME## TogglePrint;     \
    VPPDBG_CMD_FUNC[MODULE_NUM][VPP_DEBUG_UTIL_PRINT_CMD_TOGGLE_MONITOR]= (VPP_DEBUG_UTIL_CMD_FUNC)VPP_DEBUG_UTIL_ ##MODULE_NAME## ToggleMonitor;   \
    VPPDBG_CMD_FUNC[MODULE_NUM][VPP_DEBUG_UTIL_PRINT_CMD_SHOW_VARIABLES]= (VPP_DEBUG_UTIL_CMD_FUNC)VPP_DEBUG_UTIL_ ##MODULE_NAME## ShowVar;         \
    VPPDBG_CMD_FUNC[MODULE_NUM][VPP_DEBUG_UTIL_PRINT_CMD_PRINT]         = (VPP_DEBUG_UTIL_CMD_FUNC)VPP_DEBUG_UTIL_ ##MODULE_NAME## Print;

//Get the valid unit-id for the given maximum maxID
void VPP_DEBUG_getUnitID(UINT32 unitID, UINT32 maxID, UINT32 *pUidStart, UINT32 *pUidEnd);

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameDrop
    VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(frameDrop)
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameDrop

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
    VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(frameRepeat)
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameRepeat

#ifdef VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo
    VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(vbufInfo)
#endif //VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo

#ifdef VPP_DEBUG_UTIL_ENABLE_printWindowChange
    VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(winChange)
#endif //VPP_DEBUG_UTIL_ENABLE_printWindowChange

#ifdef VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss
    VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(vbiVdeIntrMiss)
#endif //VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss

#ifdef VPP_DEBUG_UTIL_ENABLE_FeatureFlag
    VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(featureFlag)
#endif //VPP_DEBUG_UTIL_ENABLE_FeatureFlag

#ifdef VPP_DEBUG_UTIL_ENABLE_printSclChange
    VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(sclChange)
#endif //VPP_DEBUG_UTIL_ENABLE_printSclChange

#ifdef VPP_DEBUG_UTIL_ENABLE_frameBuffer
    VPP_DEBUG_UTIL_DEFINE_FUNCTIONS(frameBuffer)
#endif //VPP_DEBUG_UTIL_ENABLE_frameBuffer

#endif //_VPP_DEBUG_UTIL_FUNC_H_

