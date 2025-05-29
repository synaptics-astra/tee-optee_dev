/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_debug_util.h
//! \brief      Various vpp debug utility routine to debug VPP issues
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_DEBUG_UTIL_H_
#define _VPP_DEBUG_UTIL_H_

#include "vpp_api.h"
#include "vpp_debug_util_data.h"
#include "vpp_debug_util_func.h"

#if defined(VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo)
void VPP_DEBUG_UTIL_storeVbufInfo(VPP_OBJ *vpp_obj, UINT32 planeID, VBUF_INFO *pVbufInfo);
#else //VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo
#define VPP_DEBUG_UTIL_storeVbufInfo(...)
#endif //VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo

#ifdef VPP_DEBUG_UTIL_ENABLE_printWindowChange
int VPP_DEBUG_UTIL_printWindowChange(VPP_OBJ *vpp_obj, VPP_WIN *win, UINT32 planeID, UINT32 unitNum, const char *funcName, UINT32 lineNum);
#else //VPP_DEBUG_UTIL_ENABLE_printWindowChange
#define VPP_DEBUG_UTIL_printWindowChange(...)
#endif //VPP_DEBUG_UTIL_ENABLE_printWindowChange

#ifdef VPP_DEBUG_UTIL_ENABLE_printSclChange
int VPP_DEBUG_UTIL_printSclChange(VPP_OBJ *vpp_obj, VPP_SCL_RES *sclWin, VPP_SCL_CTRL *sclCtrl, VPP_FRC_SCL_NUM sclID, INT8 scl_mode, INT8 sclLineDropEn, const char *funcName, UINT32 lineNum);
#else //VPP_DEBUG_UTIL_ENABLE_printSclChange
#define VPP_DEBUG_UTIL_printSclChange(...)
#endif //VPP_DEBUG_UTIL_ENABLE_printSclChange

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameDrop
void VPP_DEBUG_UTIL_monitorFrameDrop(VPP_OBJ *vpp_obj, UINT32 state, UINT32 planeID, VDEC_VID_BUF_DESC *frm_descr);
void VPP_DEBUG_UTIL_printFrameDrop(VPP_OBJ *vpp_obj);
#else //VPP_DEBUG_UTIL_ENABLE_printFrameDrop
#define VPP_DEBUG_UTIL_monitorFrameDrop(...)
#define VPP_DEBUG_UTIL_printFrameDrop(...)
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameDrop

#ifdef VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
void VPP_DEBUG_UTIL_monitorFrameRepeat(VPP_OBJ *vpp_obj, UINT32 state, UINT32 planeID, VDEC_VID_BUF_DESC *frm_descr);
void VPP_DEBUG_UTIL_printFrameRepeat(VPP_OBJ *vpp_obj);
#else //VPP_DEBUG_UTIL_ENABLE_printFrameRepeat
#define VPP_DEBUG_UTIL_monitorFrameRepeat(...)
#define VPP_DEBUG_UTIL_printFrameRepeat(...)
#endif //VPP_DEBUG_UTIL_ENABLE_printFrameRepeat

#ifdef VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss
void VPP_DEBUG_UTIL_monitorVbiVdeIntrMiss(VPP_OBJ *vpp_obj, UINT32 intrInstat);
void VPP_DEBUG_UTIL_printVbiVdeIntrMiss(VPP_OBJ *vpp_obj);
#else //VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss
#define VPP_DEBUG_UTIL_monitorFrameRepeat(...)
#define VPP_DEBUG_UTIL_printVbiVdeIntrMiss(...)
#endif //VPP_DEBUG_UTIL_ENABLE_printVbiVdeIntrMiss

#ifdef VPP_DEBUG_UTIL_ENABLE_FeatureFlag
BOOL VPP_DEBUG_UTIL_isFeatureFlagEnabled(VPP_OBJ *vpp_obj, VPP_DEBUG_UTIL_FF feature);
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_VALIDATE_BCMBUFF(vpp_obj)  VPP_DEBUG_UTIL_isFeatureFlagEnabled(vpp_obj, VPP_DEBUG_UTIL_FF_VALIDATE_BCMBUFF)
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_AUTOPUSH(vpp_obj)          VPP_DEBUG_UTIL_isFeatureFlagEnabled(vpp_obj, VPP_DEBUG_UTIL_FF_AUTOPUSH)
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_REFWIN(vpp_obj)            VPP_DEBUG_UTIL_isFeatureFlagEnabled(vpp_obj, VPP_DEBUG_UTIL_FF_REFWIN)
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_DISPWIN(vpp_obj)           VPP_DEBUG_UTIL_isFeatureFlagEnabled(vpp_obj, VPP_DEBUG_UTIL_FF_DISPWIN)
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_PTMTA(vpp_obj)             VPP_DEBUG_UTIL_isFeatureFlagEnabled(vpp_obj, VPP_DEBUG_UTIL_FF_PTMTA)
void VPP_DEBUG_UTIL_printFeatureFlag(VPP_OBJ *vpp_obj);
#else //VPP_DEBUG_UTIL_ENABLE_FeatureFlag
#define VPP_DEBUG_UTIL_isFeatureFlagEnabled(...)            1
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_VALIDATE_BCMBUFF(...)  0
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_AUTOPUSH(...)          1
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_REFWIN(vpp_obj)        1
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_DISPWIN(vpp_obj)       1
#define VPP_DEBUG_UTIL_IS_FF_ENABLED_PTMTA(vpp_obj)         1
#define VPP_DEBUG_UTIL_printFeatureFlag(...)
#endif //VPP_DEBUG_UTIL_ENABLE_FeatureFlag

#ifdef VPP_DEBUG_UTIL_ENABLE_frameBuffer
void VPP_DEBUG_UTIL_getVbufInfo(VPP_OBJ *vpp_obj, UINT32 planeID, VBUF_INFO **pinfo);
#else //VPP_DEBUG_UTIL_ENABLE_frameBuffer
#define VPP_DEBUG_UTIL_getVbufInfo(...)
#endif //VPP_DEBUG_UTIL_ENABLE_frameBuffer

#ifdef VPP_DEBUG_UTIL_ENABLE_processPrintCmd
void VPP_DEBUG_UTIL_processPrintCmd(VPP_OBJ *vpp_obj, UINT32 printCmd);
#else //VPP_DEBUG_UTIL_ENABLE_processPrintCmd
#define VPP_DEBUG_UTIL_ENABLE_processPrintCmd(...)
#endif //VPP_DEBUG_UTIL_ENABLE_processPrintCmd

void VPP_DEBUG_UTIL_init(VPP_OBJ *vpp_obj);
#endif //_VPP_DEBUG_UTIL_H_

