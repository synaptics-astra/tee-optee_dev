/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file osal_ta.c
//!
//! \brief Implementation of VPP commands for TrustZone.
//!
//! Purpose: This is used for controlling access to VPP API in TrustZone.
//!
//!     Version    Date                     Author
//!     V 1.00,    Dec 23 2013,             Vijay Kumar Raju P
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////


/*
 * test tee time api
 */
#include "vpp_cmd.h"
#include "log.h"

#include "tee_ta_osal.h"
#include "tee_internal_api.h"
#include "ErrorCode.h"




TEE_Result MV_OSAL_Handler(
        void*           sessionContext,
        uint32_t        commandID,
        uint32_t        paramTypes,
        TEE_Param       params[4])
{
    if(params[0].value.a==HANDLE_OSAL_TIMER){
        typedef void (*timer_cb_func_t)(void *);
        timer_cb_func_t timer_cb_func;

        timer_cb_func = (timer_cb_func_t)params[1].value.a;
        timer_cb_func((void *)params[2].value.a);
    }

    return TEE_SUCCESS;
}








