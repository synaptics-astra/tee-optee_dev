/*******************************************************************************
 *                 2021 Synaptics Incorporated. All Rights Reserved            *
 * THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF Synaptics.                   *
 * NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT *
 * OF Synaptics OR ANY THIRD PARTY. Synaptics RESERVES THE RIGHT AT ITS SOLE   *
 * DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO Synaptics.  *
 * THIS CODE IS PROVIDED "AS IS". Synaptics MAKES NO WARRANTIES, EXPRESSED,    *
 * IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.  *
 *                                                                             *
 *******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
//! \file    factory_command.c
//! \brief   Factory TA command implementation
//! \author  Victor Ku
//! \date    July 2021
////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#ifdef CONFIG_TZK
#include "tee_common.h"
#include "tee_internal_common.h"
#include "tee_internal_core_common.h"
#include "tee_internal_private_api.h"
#else
#include "tee_internal_api_extensions.h"
#include "tee_api_syna.h"
#endif
#include "tee_internal_api.h"
#include "factory_ta.h"
#include "factory_def.h"
#include "drm_tee_memory.h"
#include "seccrypto_bcm.h"
#include "assert.h"

static HRESULT MV_DRMLIB_GENX_OEM_COMMAND(UINT8* pbOemCommand, UINT32 uOemCommandLen)
{
    HRESULT lRes = S_OK;
    TEE_Result ret = TEE_SUCCESS;
    UINT8*  pbOemCommandSHM = NULL;
    DRM_TEE_SHM_HANDLE pbOemCommandhmhanle = INVALID_SHM_HANDLE;

    pbOemCommandSHM = (UINT8*)DRM_TEE_SHM_Malloc(uOemCommandLen, &pbOemCommandhmhanle);
    if (NULL == pbOemCommandSHM) {
        FACTORY_ERR("Out of memory\n");
        lRes = E_OUTOFMEMORY;
        return lRes;
    }

    memcpy(pbOemCommandSHM, pbOemCommand, uOemCommandLen);
    ret = TEE_FlushCache(pbOemCommandSHM, uOemCommandLen);
    if (TEE_SUCCESS != ret) {
       FACTORY_ERR("Failed to flush cache\n");
       lRes = E_FAIL;
       goto Func_Ret;
    }

    //Send BCM command
    lRes = BCM_GENX_OEM_COMMAND(pbOemCommandSHM, uOemCommandLen);
    if (S_OK != lRes) {
        FACTORY_ERR("Failed to call BCM command\n");
        goto Func_Ret;
    }

Func_Ret:
    if (NULL != pbOemCommandSHM){
        DRM_TEE_SHM_Free(pbOemCommandhmhanle);
    }

    return lRes;
}

static TEE_Result MV_DRM_GENX_OEM_COMMAND(UINT8* pbOemCommand, UINT32 uOemCommandSize)
{
    TEE_Result result = TEE_SUCCESS;
    HRESULT lRes = S_OK;

    lRes = MV_DRMLIB_GENX_OEM_COMMAND(pbOemCommand, uOemCommandSize);
    if (S_OK != lRes)
    {
        result = TEE_ERROR_GENERIC;
        FACTORY_LOG("Encounter error while running OEM command !!\n");
    }
    else
    {
        result = TEE_SUCCESS;
        FACTORY_LOG("Success to run OEM command\n");
    }

    return result;
}

static TEE_Result Drm_GENX_OEM_COMMAND(TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;
    uint8_t  *pbOemCommand = NULL;
    uint32_t uOemCommandSize= 0;

    FACTORY_LOG("Start running OEM command to bump OTP version\n");

    pbOemCommand = (uint8_t *)params[0].memref.buffer;
    uOemCommandSize = params[0].memref.size;

    if (TEE_SUCCESS != drm_mem_check_nosecure(pbOemCommand, uOemCommandSize))
    {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    result = MV_DRM_GENX_OEM_COMMAND(pbOemCommand, uOemCommandSize);

    return result;
}

TEE_Result MV_DRM_COMMAND_InvokeCommandEntryPoint(uint32_t commandID,
                                               TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;

    /* This dispatcher can only handle command related functions */

    switch(commandID) {
    case DRM_SE_GENX_OEM_COMMAND:
        result = Drm_GENX_OEM_COMMAND(params);
        break;

    default:
        break;
    }

    return result;
}
