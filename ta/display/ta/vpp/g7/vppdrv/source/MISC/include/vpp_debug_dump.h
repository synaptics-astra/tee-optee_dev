/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_debug_dump.h
//! \brief      Various vpp debug dump utility routine to debug VPP issues
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef _VPP_DEBUG_DUMP_H_
#define _VPP_DEBUG_DUMP_H_

#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_debug_util_data.h"

//Various option for dumpFlag parameter - isBcmBufferValid
#define VPP_DEBUG_UTIL_BCMBUFF_DUMP_DISABLE     0
#define VPP_DEBUG_UTIL_BCMBUFF_DUMP_ENABLE      1

#ifdef VPP_DEBUG_UTIL_ENABLE_printRegDump
void VPP_DEBUG_UTIL_printRegDumpMain(VPP_OBJ *vpp_obj, UINT32 printMainSubCmd);
#else //VPP_DEBUG_UTIL_ENABLE_printRegDump
#define VPP_DEBUG_UTIL_printRegDump(...)
#endif //VPP_DEBUG_UTIL_ENABLE_printRegDump

#if defined(VPP_DEBUG_UTIL_ENABLE_printVbufInfo) || defined(VPP_DEBUG_UTIL_ENABLE_StoreVbufInfo)
void VPP_DEBUG_UTIL_printVbufInfo(VPP_OBJ *vpp_obj, UINT32 planeID, VBUF_INFO *pVBInfo);
void VPP_DEBUG_UTIL_printVbufInfo_opt(VPP_OBJ *vpp_obj, UINT32 planeID, VBUF_INFO *pVBInfo);
#else //VPP_DEBUG_UTIL_ENABLE_printVbufInfo
#define VPP_DEBUG_UTIL_printVbufInfo(...)
#define VPP_DEBUG_UTIL_printVbufInfo_opt(...)
#endif //VPP_DEBUG_UTIL_ENABLE_printVbufInfo

#ifdef VPP_DEBUG_UTIL_ENABLE_monitorBcmError
void VPP_DEBUG_UTIL_monitorBcmErrorStart(BCMBUF *pbcmbuf, const char *pPrnPrefix, BOOL dumpFlag);
void VPP_DEBUG_UTIL_monitorBcmError(BCMBUF *pbcmbuf, const char *pPrnPrefix, BOOL dumpFlag);
#else //VPP_DEBUG_UTIL_ENABLE_monitorBcmError
#define VPP_DEBUG_UTIL_monitorBcmErrorStart(...)
#define VPP_DEBUG_UTIL_monitorBcmError(...)
#endif //VPP_DEBUG_UTIL_ENABLE_monitorBcmError

void VPP_DEBUG_UTIL_bcmBufferDump(BCMBUF *pbcmbuf, const char *pPrnPrefix);

#ifdef VPP_DEBUG_UTIL_ENABLE_validateBcmBuffer
BOOL VPP_DEBUG_UTIL_isRawBcmBufferValid(UINT32 start, UINT32 size, const char *pPrnPrefix, BOOL dumpFlag);
BOOL VPP_DEBUG_UTIL_isBcmBufferValid(BCMBUF *pbcmbuf, const char *pPrnPrefix, BOOL dumpFlag);
void VPP_DEBUG_UTIL_checkBcmBuffer(VPP_OBJ *vpp_obj, BCMBUF *pbcmbuf, const char *pPrnPrefix, BOOL dumpFlag);
void VPP_DEBUG_UTIL_checkRawBcmBuffer(VPP_OBJ *vpp_obj, UINT32 start, UINT32 size, const char *pPrnPrefix, BOOL dumpFlag);
#else //VPP_DEBUG_UTIL_ENABLE_validateBcmBuffer
#define VPP_DEBUG_UTIL_isRawBcmBufferValid(...)
#define VPP_DEBUG_UTIL_isBcmBufferValid(...)
#define VPP_DEBUG_UTIL_checkBcmBuffer(...)
#define VPP_DEBUG_UTIL_checkRawBcmBuffer(...)
#endif //VPP_DEBUG_UTIL_ENABLE_validateBcmBuffer

#ifdef VPP_DEBUG_UTIL_ENABLE_monitorResetCPCB
void VPP_DEBUG_UTIL_monitorResetCPCBStart(VPP_OBJ *vpp_obj, const char *pPrnPrefix, BOOL dumpFlag);
void VPP_DEBUG_UTIL_monitorResetCPCB(VPP_OBJ *vpp_obj, const char *pPrnPrefix, BOOL dumpFlag);
#else //VPP_DEBUG_UTIL_ENABLE_monitorResetCPCB
#define VPP_DEBUG_UTIL_monitorResetCPCBStart(...)
#define VPP_DEBUG_UTIL_monitorResetCPCB(...)
#endif //VPP_DEBUG_UTIL_ENABLE_monitorResetCPCB

#endif //_VPP_DEBUG_DUMP_H_

