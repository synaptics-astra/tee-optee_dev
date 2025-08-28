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
#ifdef CONFIG_TZK
#include "tee_ta.h"
#else
#include "trace.h"
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

#ifdef CONFIG_TZK
TEE_Result HandleCryptoCommand(uint32_t commandID, TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;
    if(DRM_SE_RSA_SIGN_VERIFY == commandID) {
        result = Drm_Sign_Verify_Message(params);
    } else {
        FACTORY_ERR("factory :%s:commandID=%x is not supported\n", __func__, (unsigned int)commandID);
    }
    return result;
}
#endif

/* Invoke a command within a given session */
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(void *sessionContext,
                                                uint32_t commandID,
                                                uint32_t paramTypes,
                                                TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;

#ifdef CONFIG_TZK
#else
    DMSG("CommandID = 0x%x\n", commandID);
#endif

    if (0x0fff == (commandID&0x0fff)) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    /* cmp the command with the defined command */
    switch(commandID & COMMAND_MASK) {
        case COMMAND_MASK:
            result = MV_DRM_COMMAND_InvokeCommandEntryPoint(commandID, params);
            break;
        case OTP_MASK:
            result = MV_DRM_OTP_InvokeCommandEntryPoint(commandID, params);
            break;
#ifdef CONFIG_TZK
        case CRYPTO_MASK:
            result = HandleCryptoCommand(commandID, params);
            break;
#endif
        case RPMB_MASK:
            result = MV_DRM_RPMB_InvokeCommandEntryPoint(commandID, params);
            break;
        default:
            break;
    }
    return result;
}
#ifdef CONFIG_TZK
TA_DEFINE_BEGIN
    .uuid                   = FACTORY_UUID,
    .name                   = "FACTORY",
    .singleInstance         = true,
    .multiSession           = true,
    .instanceKeepAlive      = true,
TA_DEFINE_END
#endif