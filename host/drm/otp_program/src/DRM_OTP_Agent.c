/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
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
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
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
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
////////////////////////////////////////////////////////////////////////////////
//! \file: DRM_OTP_Agent.c
//!
//! \brief: program OTP tool API
//!
//!
//! Version    Date                     Author
//! V 0.01,    Apr 16 2019,             Cheney Yang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "DRM_OTP_Agent_API.h"
#include "tee_client_api.h"

#define FACTORY_UUID {0x1316a183, 0x894d, 0x43fe, \
    {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x20}}
#define DRM_SE_CA_CMD_OTP_BURNLAYOUT 0xE001

typedef struct {
    UINT32   BitMap;
    UINT8    RkekId[8];
    UINT32   Reserved0;
} DRM_OTP_CLI_RESP;

static const TEEC_UUID TA_UUID = FACTORY_UUID;
static bool bInitialized = false;
static TEEC_Context context;
static TEEC_Session session;

HRESULT OTP_TZInitialize(void)
{
  HRESULT lRes = S_OK;
  TEEC_Result result;

  if (bInitialized)
    return S_OK;
  bInitialized = true;
  /* ========================================================================
     [1] Connect to TEE
     ======================================================================== */
  result = TEEC_InitializeContext(NULL, &context);
  if (result != TEEC_SUCCESS) {
    printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
    lRes = S_FALSE;
    goto cleanup1;
  }

  /* ========================================================================
     [2] Open session with TEE application
     ======================================================================== */
  /* Open a Session with the TEE application. */
  result = TEEC_OpenSession(
          &context,
          &session,
          &TA_UUID,
          TEEC_LOGIN_USER,
          NULL,    /* No connection data needed for TEEC_LOGIN_USER. */
          NULL,    /* No payload, and do not want cancellation. */
          NULL);

  if (result != TEEC_SUCCESS) {
    printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
    lRes = S_FALSE;
    goto cleanup2;
  }

  return lRes;
cleanup2:
  TEEC_FinalizeContext(&context);
cleanup1:
  bInitialized = false;
  return lRes;
}

void OTP_TZFinalize(void)
{
  if (!bInitialized)
    return;
  bInitialized = false;

  TEEC_CloseSession(&session);
  TEEC_FinalizeContext(&context);
}

HRESULT MV_OTP_BCM_Layout_Program(UCHAR* otp_pack_file)
{
    HRESULT lRes  = S_OK;
    FILE   *pFile = NULL;
    UINT8  *pOtpPack   = NULL;
    UINT32  lenOtpPack = 0;
    DRM_OTP_CLI_RESP  *pResp       = NULL;
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_SharedMemory ShmOtpPack;
    TEEC_Operation operation;

    if (NULL == otp_pack_file)
    {
        lRes = S_FALSE;
        goto __EXIT;
    }

    // handle otp layout stuff
    pFile = fopen((const char*)otp_pack_file, "rb");
    if (NULL == pFile)
    {
        printf("Fail to open:%s\n", otp_pack_file);
        lRes = S_FALSE;
        goto __EXIT;
    }

    fseek(pFile, 0, SEEK_END);
    lenOtpPack = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    lRes = OTP_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    ShmOtpPack.size = lenOtpPack;
    ShmOtpPack.flags = TEEC_MEM_INPUT;
    result = TEEC_AllocateSharedMemory(&context, &ShmOtpPack);
    if (result != TEEC_SUCCESS) {
        printf("TEEC_AllocateSharedMemory ShmOtpPack fail\n");
        TEEC_ReleaseSharedMemory(&ShmOtpPack);
        lRes = S_FALSE;
        goto __EXIT;
    }

    pOtpPack = (UINT8*)ShmOtpPack.buffer;
    if (lenOtpPack != fread(pOtpPack, 1, lenOtpPack, pFile)) {
        printf("Fail to read %d data\n", lenOtpPack);
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.params[0].memref.parent = &ShmOtpPack;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = ShmOtpPack.size;
    operation.started = 1;
    result = TEEC_InvokeCommand(
            &session,
            DRM_SE_CA_CMD_OTP_BURNLAYOUT,
            &operation,
            NULL);

    if (result != TEEC_SUCCESS) {
        printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
        lRes = S_FALSE;
        goto __EXIT;
    }

__EXIT:
    if (NULL != pFile)
    {
        fclose(pFile);
        pFile = NULL;
    }

    TEEC_ReleaseSharedMemory(&ShmOtpPack);

    return lRes;
}
