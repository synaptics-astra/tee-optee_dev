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
//! \file: main.c
//!
//! \brief: write RKEKID
//!
//!
//! Version    Date                     Author
//! V 0.01,    May 21 2025,             Cheney Yang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tee_client_api.h"

#ifdef CONFIG_TZK
#if defined(ANDROID)
const char* FACTORY_TA_PATH = "/vendor/firmware/ta";
#else
const char* FACTORY_TA_PATH = "/opt/syna/ta/";
#endif
const char* FACTORY_TA_NAME = "libfactory.ta";
#endif

#define TA_FACTORY_CA_UUID {0x1316a183, 0x894d, 0x43fe, \
    {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x20}}
#define FACTORY_CA_CMD_OTP_WRITE_RKEKID 0xE00B

typedef signed int HRESULT;
#define S_OK 0
#define S_FALSE 1

static const TEEC_UUID TA_UUID = TA_FACTORY_CA_UUID;
static bool bInitialized = false;
static TEEC_Context context;
static TEEC_Session session;

void UsageHelp(int argc, char* argv[])
{
    printf("Usage:\n");
    printf("    %s 8 bytes rkekid start with 0x\n", argv[0]);
}

HRESULT OTP_TZInitialize(void)
{
    HRESULT lRes = S_OK;
    TEEC_Result result;
    char ta_file_path[128] = {0};

    if (bInitialized)
        return S_OK;
    bInitialized = true;
    /* ========================================================================
       [1] Connect to TEE
       ======================================================================== */
    result = TEEC_InitializeContext(NULL,&context);
    if (result != TEEC_SUCCESS) {
        printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
        lRes = S_FALSE;
        goto cleanup1;
    }

    #ifdef CONFIG_TZK
    /* ========================================================================
       [2] load TA
       ======================================================================== */
    snprintf(ta_file_path, 128, "%s/%s", FACTORY_TA_PATH, FACTORY_TA_NAME);
    result = TEEC_LoadTA(&context, ta_file_path);
    if (result == TEEC_ERROR_ACCESS_CONFLICT) {
        printf("%s: [%s] TA has already been loaded\n", __FUNCTION__, ta_file_path);
        result = TEEC_SUCCESS;
    } else if (result != TEEC_SUCCESS) {
        printf("%s: Failed to load the TA [%s]: %x\n", __FUNCTION__, ta_file_path, result);
        TEEC_FinalizeContext(&context);
        lRes = S_FALSE;
        goto cleanup2;
    }
    #endif //CONFIG_TZK
    /* ========================================================================
       [3] Open session with TEE application
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

int main(int argc, char *argv[])
{
    HRESULT lRes = S_FALSE;
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;
    TEEC_SharedMemory hShm;
    unsigned char rkekid[8];
    int rkek_id_size = 8;
    unsigned long long value = 0;
    unsigned int i = 0;
    bool shm_allocated = false;

    lRes = OTP_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    if (argc != 2) {
        UsageHelp(argc, argv);
        goto __EXIT;
    }

    value = strtoull(argv[1], NULL, 16);
    for (i = 0; i < 8; i++)
        rkekid[i] = (value >> (i * 8)) & 0xFF;

    hShm.size = rkek_id_size;
    hShm.flags = TEEC_MEM_INPUT;
    result = TEEC_AllocateSharedMemory(&context, &hShm);
    if (result != TEEC_SUCCESS) {
        printf("TEEC allocate SHM fail[ret=0x%x]\n", result);
        goto __EXIT;
    }
    shm_allocated = true;

    memcpy((unsigned char*)hShm.buffer, rkekid, rkek_id_size);
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.params[0].memref.parent = &hShm;
    operation.params[0].memref.size = hShm.size;
    operation.params[0].memref.offset = 0;
    operation.started = 1;

    result = TEEC_InvokeCommand(
            &session,
            FACTORY_CA_CMD_OTP_WRITE_RKEKID,
            &operation,
            NULL);

    if (result == TEEC_SUCCESS) {
       printf("\nwrite RKEKID successfully\n");
       lRes = S_OK; }
    else {
       printf("\n%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
       lRes = S_FALSE;
    }

__EXIT:
    if (shm_allocated) {
        TEEC_ReleaseSharedMemory(&hShm);
    }

    OTP_TZFinalize();
    if (lRes != S_OK) {
        printf("Fail to write RKEKID\n");
        return -1;
    }
    return lRes;
}
