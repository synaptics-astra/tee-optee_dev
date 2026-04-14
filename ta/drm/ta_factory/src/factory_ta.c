/*******************************************************************************
 *                 2017 Synaptics Incorporated. All Rights Reserved            *
 * THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF Synaptics.                   *
 * NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT *
 * OF Synaptics OR ANY THIRD PARTY. Synaptics RESERVES THE RIGHT AT ITS SOLE   *
 * DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO Synaptics.  *
 * THIS CODE IS PROVIDED "AS IS". Synaptics MAKES NO WARRANTIES, EXPRESSED,    *
 * IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.  *
 *                                                                             *
 *******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
//! \file    factory_ta.c
//! \brief   Factory TA entry point implementation
//! \author  Frank Ho
//! \date    Dec 2017
////////////////////////////////////////////////////////////////////////////////

#include "tee_internal_api.h"

#ifdef CONFIG_OPTEE
#include "trace.h"
#else
#include "tee_ta.h"
#endif

#include "factory_ta.h"
#include "factory_def.h"
#include "factory_otp.h"
#include "factory_crypto.h"
#include "factory_rpmb.h"
#include "factory_command.h"

/* Create a new instance for the trusted application */
TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    return TEE_SUCCESS;
}

/* Destory the instance of the trusted application */
void TA_EXPORT TA_DestroyEntryPoint(void)
{
    return;
}

/* open a session with the trusted application when a client requests */
TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(uint32_t paramTypes,
                                              TEE_Param params[4],
                                              void **sessionContext)
{
    return TEE_SUCCESS;
}

/* Close a client session with the trusted application */
void TA_EXPORT TA_CloseSessionEntryPoint(void *sessionContext)
{
    return;
}

/* Invoke a command within a given session */
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(void *sessionContext,
                                                uint32_t commandID,
                                                uint32_t paramTypes,
                                                TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;

#ifdef CONFIG_OPTEE
    DMSG("CommandID = 0x%x\n", commandID);
#endif

    if (0x0fff == (commandID&0x0fff)) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    /* cmp the command with the defined command */
    switch(commandID & COMMAND_MASK) {
        case COMMAND_MASK:
            result = MV_DRM_COMMAND_InvokeCommandEntryPoint(commandID, paramTypes, params);
            break;
        case OTP_MASK:
            result = MV_DRM_OTP_InvokeCommandEntryPoint(commandID, params);
            break;
        case CRYPTO_MASK:
            result = MV_DRM_CRYPTO_InvokeCommandEntryPoint(commandID, params);
            break;
        case RPMB_MASK:
            result = MV_DRM_RPMB_InvokeCommandEntryPoint(commandID, params);
            break;
        default:
            break;
    }
    return result;
}
#ifndef CONFIG_OPTEE
TA_DEFINE_BEGIN
    .uuid                   = FACTORY_UUID,
    .name                   = "FACTORY",
    .singleInstance         = true,
    .multiSession           = true,
    .instanceKeepAlive      = true,
TA_DEFINE_END
#endif