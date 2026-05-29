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

#ifdef CONFIG_OPTEE
#include "tee_internal_api_extensions.h"
#include "tee_api_syna.h"
#else
#include "tee_common.h"
#include "tee_internal_common.h"
#include "tee_internal_core_common.h"
#include "tee_internal_private_api.h"
#endif

#include "tee_internal_api.h"
#include "factory_ta.h"
#include "factory_def.h"
#include "drm_tee_memory.h"
#include "seccrypto_bcm.h"
#include "assert.h"
#include "factory_crypto.h"
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

static TEE_Result DRM_TA_AES_GCM_ENCRY_DECRY(TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    TEE_OperationHandle op = TEE_HANDLE_NULL;
    TEE_ObjectHandle key_obj = TEE_HANDLE_NULL;

    /* 1. Parse parameters from Normal World (Linux Kernel) */
    bool is_encrypt = (params[0].value.a != 0);
    uint8_t *payload_buf = (uint8_t *)params[1].memref.buffer;
    /* Security Check: Ensure buffer is large enough for the tag */
    if (params[1].memref.size < AES_GCM_TAG_SIZE)
        return TEE_ERROR_BAD_PARAMETERS;

    size_t data_len = params[1].memref.size - AES_GCM_TAG_SIZE;
    uint8_t *tag_buf = payload_buf + data_len;
    uint8_t *iv_buf = (uint8_t *)params[2].memref.buffer;
    uint8_t *aad_buf = (uint8_t *)params[3].memref.buffer;
    size_t aad_len = params[3].memref.size;
    size_t tag_len = AES_GCM_TAG_SIZE;

    FACTORY_LOG("AES-GCM %s | Data: %zu, AAD: %zu\n", is_encrypt ? "ENC" : "DEC", data_len, aad_len);

    /* 2. Derive Hardware-Backed Secure Key */
    res = TEE_AllocateTransientObject(TEE_TYPE_AES, 128, &key_obj);
    if (res != TEE_SUCCESS) goto __EXIT;

    /* Get Secure Key from BCM*/
    TEE_Param key_gen_params[4];
    uint8_t secure_key_buf[16] = {0};
    memset(key_gen_params, 0, sizeof(key_gen_params));
    key_gen_params[1].memref.buffer = (uint32_t)secure_key_buf;
    key_gen_params[1].memref.size = sizeof(secure_key_buf);

    res = Drm_GenerateSecureKey_V2(key_gen_params);
    if (res != TEE_SUCCESS) {
        FACTORY_LOG("BCM Key Generation failed: 0x%08lx\n", res);
        goto __EXIT;
    }

    TEE_Attribute attr;
    TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, secure_key_buf, sizeof(secure_key_buf));
    res = TEE_PopulateTransientObject(key_obj, &attr, 1);
    if (res != TEE_SUCCESS) goto __EXIT;

    /* 3. Setup AEAD Operation */
    uint32_t mode = is_encrypt ? TEE_MODE_ENCRYPT : TEE_MODE_DECRYPT;
    res = TEE_AllocateOperation(&op, TEE_ALG_AES_GCM, mode, 128);
    if (res != TEE_SUCCESS) goto __EXIT;

    res = TEE_SetOperationKey(op, key_obj);
    if (res != TEE_SUCCESS) goto __EXIT;

    /* Initialize AE session: 12B IV, 128b Tag */
    res = TEE_AEInit(op, iv_buf, AES_GCM_IV_SIZE, AES_GCM_TAG_SIZE<<3, 0, 0);
    if (res != TEE_SUCCESS) goto __EXIT;

    if (aad_len > 0) {
        TEE_AEUpdateAAD(op, aad_buf, aad_len);
    }

    /* 4. Execute Processing */
    if (is_encrypt) {
        /* Encryption using in-place processing */
        res = TEE_AEEncryptFinal(op,
                                 payload_buf, data_len,
                                 payload_buf, &data_len,
                                 tag_buf, &tag_len);
    } else {
        /* Decryption using memory isolation for stability */
        size_t out_len = (uint32_t)data_len;
        uint8_t *tmp_plain = TEE_Malloc(data_len, 0);
        uint8_t *tmp_tag = TEE_Malloc(AES_GCM_TAG_SIZE, 0);

        if (!tmp_plain || !tmp_tag) {
            FACTORY_LOG("Failed to allocate temp buffers\n");
            res = TEE_ERROR_OUT_OF_MEMORY;
            goto __FREE_TMP;
        }

        /* Capture tag from Shared Memory to Private Secure Memory */
        memcpy(tmp_tag, tag_buf, AES_GCM_TAG_SIZE);

        res = TEE_AEDecryptFinal(op, payload_buf, data_len,
                                 tmp_plain, &out_len,
                                 tmp_tag, AES_GCM_TAG_SIZE);

        if (res == TEE_SUCCESS) {
            memcpy(payload_buf, tmp_plain, data_len);
        } else if (res == TEE_ERROR_MAC_INVALID) {
            FACTORY_LOG("Integrity Check Failed (MAC Mismatch)\n");
        }

    __FREE_TMP:
        if (tmp_plain) TEE_Free(tmp_plain);
        if (tmp_tag)   TEE_Free(tmp_tag);
    }

__EXIT:
    if (res != TEE_SUCCESS && res != TEE_ERROR_MAC_INVALID)
        FACTORY_LOG("Process Failed: 0x%08lx\n", res);

    /* Final Cleanup */
    memset(secure_key_buf, 0, sizeof(secure_key_buf));
    if (op != TEE_HANDLE_NULL) TEE_FreeOperation(op);
    if (key_obj != TEE_HANDLE_NULL) TEE_FreeTransientObject(key_obj);
    return res;
}
static TEE_Result DRM_TA_ACCESS_SECURE_SRAM(uint32_t param_types, TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    /* buffer setting */
    uint32_t size = sizeof(uint64_t);
    uint64_t *pbSHMBuf = NULL; // share memony buf addr
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE; // mem alloc handle

    // Extract parameters from CA
    uint32_t idx = params[0].value.a;
    uint32_t p1_type = TEE_PARAM_TYPE_GET(param_types, 1);
    bool isWrite = (p1_type == TEE_PARAM_TYPE_VALUE_INPUT);

    //Alloc Secure Shared Memory (SHM) for BCM Hardware Communication
    pbSHMBuf = DRM_TEE_SHM_Malloc(size, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }
    if (isWrite) {
        // get 64-bit value from CA parameters
        uint64_t valToWrite = ((uint64_t)params[1].value.b << 32) | params[1].value.a;
        *pbSHMBuf = valToWrite;

        // Flush cache to ensure BCM hardware reads the updated data from physical RAM
        if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, size)) {
            FACTORY_ERR("Failed to flush cache\n");
            result = E_FAIL;
            goto Func_Ret;
        }

        // Send Mailbox to BCM
        result = BCM_ACCESS_SECURITY_SRAM(BCM_SRAM_WRITE, idx, pbSHMBuf);
        if (result != TEE_SUCCESS) {
            FACTORY_ERR("SRAM Access: BCM Write failed: 0x%lx\n", result);
        }
    } else {
        /* READ */
        memset(pbSHMBuf, 0, size);
        if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, size)) {
            FACTORY_ERR("Failed to flush cache\n");
            result = E_FAIL;
            goto Func_Ret;
        }

        // Request data from BCM Hardware
        result = BCM_ACCESS_SECURITY_SRAM(BCM_SRAM_READ, idx, pbSHMBuf);
        if (result != TEE_SUCCESS) {
            FACTORY_ERR("SRAM Access: BCM Read failed 0x%lx.\n", result);
            goto Func_Ret;
        }

        // Invalidate cache to ensure CPU reads the fresh data written by BCM hardware
        if (TEE_InvalidateCache(pbSHMBuf, size) != TEE_SUCCESS) {
            FACTORY_ERR("Failed to invalidate cache\n");
            result = TEE_ERROR_GENERIC;
            goto Func_Ret;
        }

        // Pass the 64-bit result back to CA
        uint64_t readVal = *pbSHMBuf;
        params[1].value.a = (uint32_t)(readVal & 0xFFFFFFFF);
        params[1].value.b = (uint32_t)(readVal >> 32);
    }

Func_Ret:
    DRM_TEE_SHM_Free(pbshmhandle);

    return result;
}

TEE_Result MV_DRM_COMMAND_InvokeCommandEntryPoint(uint32_t commandID,
                                                uint32_t paramTypes,
                                               TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;

    /* This dispatcher can only handle command related functions */

    switch(commandID) {
    case DRM_SE_GENX_OEM_COMMAND:
        result = Drm_GENX_OEM_COMMAND(params);
        break;
    case DRM_SE_OEM_AES_GCM_ENCRY_DECRY:
        result = DRM_TA_AES_GCM_ENCRY_DECRY(params);
        break;
    case DRM_SE_OEM_ACCESS_SECURE_SRAM:
        result = DRM_TA_ACCESS_SECURE_SRAM(paramTypes,params);
        break;
    default:
        break;
    }

    return result;
}
