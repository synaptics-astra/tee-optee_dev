/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_obj.h
//! \brief      store/retrieve the VPP_OBJ to/from TZK globaldata
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef __VPP_OBJ_H__
#define __VPP_OBJ_H__

#include "vpp_module.h"

#ifdef VPPOBJ_LOGGER
#define VPPOBJ_LOGI(...)     VPPAPI_DBG_INFO(__VA_ARGS__)
#define VPPOBJ_LOGE(...)     VPPAPI_DBG_ERROR(__VA_ARGS__)
#else
#define VPPOBJ_LOGI(...)
#define VPPOBJ_LOGE(...)
#endif

#ifdef VPPOBJ_LOGGER_DETAILED
#define VPPOBJ_LOGL(...)     VPPAPI_DBG_INFO(__VA_ARGS__)
#else
#define VPPOBJ_LOGL(...)
#endif

#define VPPOBJ_LOGF(...)     VPPAPI_DBG_FIELD(__VA_ARGS__)

int MV_VPP_GetVppObjAddr(void);

int MV_VPP_InitVppObj(VPP_INIT_PARM *vpp_init_parm);

void MV_VPP_DeInitVppObj(void);

void VPP_TA_IncCreateInstanceCount(void);
#endif //__VPP_OBJ_H__
