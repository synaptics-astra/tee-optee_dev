/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright © 2013-2017 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated (“Synaptics”). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS,” AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS’ TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

////////////////////////////////////////////////////////////////////////////////
//! \file    factory_crypto_bcm.c
//! \brief   Interface API implementation to call BCM PI
//! \author  Rohit Tayal
//! \date    Mar 2018
////////////////////////////////////////////////////////////////////////////////

#include "bcm_mailbox.h"
#include "bcm_primitive.h"
#include "drm_bcm_com.h"
#ifndef CONFIG_OPTEE
#include "tee_common.h"
#include "tee_log.h"
#endif
#include "tee_internal_api.h"
#include "bcm_status.h"
#include "drm_tee_memory.h"
#include <seccrypto_bcm.h>
#include <string.h>

#define PKCS_PSS_SHA256  256
#define CACHE_LINE_SIZE 64


#ifndef CONFIG_OPTEE
TEE_Result Bcm_Hal_SignKey(const uint8_t *input_data, uint32_t input_data_length,
                                    uint8_t *keyBuff, uint32_t keyBuffLen, uint8_t *response)
{
    uint8_t *pMsg, *uiPhyAddr, *uiKeyPhyAddr, *uPhySin, *uPhyCtx, *uSig = NULL, *uCtx, *keyBCM;
    DRM_TEE_SHM_HANDLE hMemShmHandle = INVALID_SHM_HANDLE , hSignShmHandle = INVALID_SHM_HANDLE;
    TEE_Result ret = TEE_SUCCESS;
    HRESULT res;
    uint32_t uLength=PKCS_PSS_SHA256, uMemLen, CtxLen = 4,offset;

    uMemLen = input_data_length + keyBuffLen + CtxLen;

    uint32_t align_size = (uMemLen &~(CACHE_LINE_SIZE-1)) +
                                                  CACHE_LINE_SIZE*3;

    pMsg = DRM_TEE_SHM_Malloc(align_size, &hMemShmHandle);

    if(pMsg == NULL) {
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto Cleanup;
    }
    memcpy(pMsg, input_data, input_data_length);

    uiPhyAddr = TEE_VirtToPhys( (void *)pMsg);

    uint32_t align_size1 = (input_data_length &~(CACHE_LINE_SIZE-1)) +
                                                  CACHE_LINE_SIZE;

    keyBCM = pMsg + align_size1;
    offset = 0;

    offset = ((uint32_t)keyBCM) % CACHE_LINE_SIZE;
    if(offset != 0) {
      offset = offset + (CACHE_LINE_SIZE - offset);
    }
    keyBCM = keyBCM + offset;
    memcpy(keyBCM, keyBuff, keyBuffLen);
    uiKeyPhyAddr = TEE_VirtToPhys((void *)keyBCM);
    uint32_t align_size2 = (keyBuffLen &~(CACHE_LINE_SIZE-1)) +
                                                  CACHE_LINE_SIZE;

    uCtx = keyBCM + align_size2;
    offset = ((uint32_t)uCtx % CACHE_LINE_SIZE);
    if(offset != 0) {
      offset = offset + (CACHE_LINE_SIZE - offset);
    }

    uCtx += offset;
    uPhyCtx = TEE_VirtToPhys((void *)uCtx);

    uSig = DRM_TEE_SHM_Malloc(uLength, &hSignShmHandle);

    if(uSig == NULL) {
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto Cleanup;
    }
    uPhySin = TEE_VirtToPhys((void *)uSig);

    TEE_FlushCache(pMsg, align_size);
    TEE_InvalidateCache(uSig, uLength);
    res = BCM_RSA_CISCODRM_SIGNKEY(uiKeyPhyAddr, keyBuffLen, uPhyCtx, uiPhyAddr,
                                input_data_length, uPhySin);
    if (res != S_OK) {
        ret = TEE_ERROR_GENERIC;
        goto Cleanup;
    }
    TEE_InvalidateCache(uSig, uLength);
    memcpy(response, uSig, uLength);
Cleanup:
    DRM_TEE_SHM_Free(hMemShmHandle);
    DRM_TEE_SHM_Free(hSignShmHandle);
    return ret;
}

#endif