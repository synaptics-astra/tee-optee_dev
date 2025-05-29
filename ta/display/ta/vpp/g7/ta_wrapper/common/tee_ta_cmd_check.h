/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_cmd_check.h
//!
//! \brief Implementation of cmd validation/checking functions for TrustZone.
//!
//! Purpose: This is used for check/validate access permission of various
//!               cmd invoked from CA (TW-otherTA & NW-CA) to TA
//!
//!     Version    Date                     Author
//!     V 1.00,    Aug 24 2022,             Shanmugam Ramachandran
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
#ifndef __TEE_TA_CMD_CHECK_H__
#define __TEE_TA_CMD_CHECK_H__

#include "tee_internal_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************
 * FUNCTION: Check origin(CA/TA) of command and its validity to
 *                 allow processing in the calling context
 *
 * IN PARAMS : commandID - VPP command to be checked
 * OUT PARAMS : None
 * RETURN:  TEE_SUCCESS - command to allowed to be processed
 *          TEE_ERROR_BAD_PARAMETERS - cannot invoke/process this command
 ****************************************************************/
TEE_Result VPP_ValidateOrigin(uint32_t commandID);

/***************************************************************
 * FUNCTION: Check if the command origin is from outctrl.ta
 *
 * IN PARAMS : commandID - VPP command to be checked
 * OUT PARAMS : None
 * RETURN:  1 - Invoked from outctrl.ta
 ****************************************************************/
bool VPP_IsAccessFromOutCtrl(uint32_t commandID);

#ifdef __cplusplus
}
#endif

#endif //__TEE_TA_CMD_CHECK_H__
