/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_cmd_check.c
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

#include "tee_ta_cmd_check.h"
#include "tee_ta_osal.h"
#include "vpp_cmd.h"
#include "vpp_module.h"
#include "ta_log.h"
#include "vpp_debug_util.h"
#if !defined(CONFIG_VPP_FASTLOGO_SUPPORT)
#include "outctrl_cmd.h"
static TEE_UUID output_control_ta_uuid=TA_OUTCTRL_UUID;
#endif //CONFIG_VPP_FASTLOGO_SUPPORT

/***************************************************************
 * FUNCTION: Check if command to be invoked from only other TA
 *
 * IN PARAMS : commandID - VPP command to be checked
 * OUT PARAMS : None
 * RETURN:  1 - command to be invoked from other TA only
 ****************************************************************/
static bool VPP_IS_TrustedWorldCommand(uint32_t commandID) {
    bool isTwdCommand = false;
    int i, nCommands;
    //Following command is allowed from other TA
    uint32_t TwdCommArray [] = {
        VPP_WATERMARK,
        VPP_ENABLEHDCP,
        VPP_ENABLESRMCHECK,
#ifdef CONFIG_AMP_DOLBY_DUMP
        VPP_DVFRAMEDUMPENABLE,
#endif
        VPP_SETHDCPVERSION,
        VPP_DEBUG_CMD,
    };

    nCommands = sizeof(TwdCommArray)/sizeof(uint32_t);

    for (i = 0; i < nCommands; i++) {
        if (commandID == TwdCommArray[i]) {
            isTwdCommand = true;
            break;
        }
    }

    return isTwdCommand;
}

/***************************************************************
 * FUNCTION: Check if command can be invoked from other TA also
 *
 * IN PARAMS : commandID - VPP command to be checked
 * OUT PARAMS : None
 * RETURN:  1 - command to be invoked from CA & other TA also
 ****************************************************************/
static bool VPP_IS_ExceptionalCommand(uint32_t commandID) {
    bool isExCommand = false;
    int i, nCommands;
    //Following CA commands exceptionally allowed from TA
    uint32_t ExCommArray [] = {
#ifdef TZ_3_0
        VPP_INITIATE_VPPS,
#endif
        VPP_GETCURHDCPVERSION,
        VPP_GETHDCPSTATE,
        VPP_GETHDCPVERSION,
        VPP_GETHPDSTATUS,
        VPP_GETHDMIPOWERSTATUS,
        VPP_GETSTATUS,
        NXG_COMMAND_HANDLER,
    };

    nCommands = sizeof(ExCommArray)/sizeof(uint32_t);
    for (i = 0; i < nCommands; i++) {
        if (commandID == ExCommArray[i]) {
            isExCommand = true;
            break;
        }
    }
    return isExCommand;
}

TEE_Result VPP_ValidateOrigin(uint32_t commandID) {
    TEE_Result res;
    TEE_Identity identity;
    bool isTwdCommand;

    isTwdCommand = VPP_IS_TrustedWorldCommand(commandID);
    res = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
    if (res != TEE_SUCCESS) {
        VPPAPI_DBG_FIELD("Currently client UUID verification fail 0x%x!\n", res);
        res = TEE_ERROR_NOT_SUPPORTED;
        return res;
    }

    //Check that NWd commands are invoked from only CA or CA should not invoke TWd command
    if (identity.login == TEE_LOGIN_USER) {
        /* Do not alow Trusted commands for user login */
        if (isTwdCommand) {
            VPPAPI_DBG_FIELD("CA cannot invoke this command : %x, login:%x \n", commandID, identity.login);
            res = TEE_ERROR_BAD_PARAMETERS;
            return res;
        }
    } else {
        /*Allow commands from exception list in the secure world.*/
        if (VPP_IS_ExceptionalCommand(commandID)) {
            isTwdCommand = true;
        }

        if (!isTwdCommand) {
            //Check that TWd commands are invoked from only TA or TA should not invoke NWd command
            VPPAPI_DBG_FIELD("TA cannot invoke this command : %x, login:%x \n", commandID, identity.login);
            res = TEE_ERROR_BAD_PARAMETERS;
            return res;
        }
    }

    return res;
}

#if !defined(CONFIG_VPP_FASTLOGO_SUPPORT)
bool VPP_IsAccessFromOutCtrl(uint32_t commandID) {
    TEE_Result res;
    TEE_Identity identity;
    TEE_UUID *uuid;

    res = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
    if (res != SUCCESS) {
        VPPAPI_DBG_ERROR("Currently client UUID verification fail 0x%x!\n", res);
        return false;
    } else {
        uuid = &identity.uuid;
        if (memcmp(uuid, &output_control_ta_uuid,sizeof(TEE_UUID))) {
            VPPAPI_DBG_ERROR("Command :%d restricted to outctrl TA \n", commandID);
            return false;
        } else {
            return true;
        }
    }
}
#endif //CONFIG_VPP_FASTLOGO_SUPPORT
