/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CPCB_COMMON_H_
#define _VPP_CPCB_COMMON_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"

#ifdef VPP_ENABLE_DEBUG_LOG_CPCB
#define VPP_CPCB_DBG_LOG(...)  VPPAPI_DBG_INFO(__VA_ARGS__)
#else
#define VPP_CPCB_DBG_LOG(...)
#endif //VPP_ENABLE_DEBUG_LOG_CPCB

/**********************************************************************
*Data structure definitions for CPCB TG block driver                                                *
***********************************************************************/

//temporarily put here
typedef struct VPP_POINT
{
    UINT32 X;
    UINT32 Y;

}VPP_POINT, *PVPP_POINT;

typedef struct VPP_WINDOW_T
{
    VPP_POINT TopLeft;
    VPP_POINT BottomRight;

}VPP_WINDOW, *PVPP_WINDOW;

#endif
