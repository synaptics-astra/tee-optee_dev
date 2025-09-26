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
//! \brief: factory utility
//!
//!
//! Version    Date                     Author
//! V 0.01,    Mar 22 2021,             Cheney Yang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tee_client_api.h"

#ifdef CONFIG_TZK
#if defined(ANDROID)
const char* FACTORY_TA_PATH = "/vendor/firmware/ta";
#else
const char* FACTORY_TA_PATH = "/opt/syna/ta";
#endif
const char* FACTORY_TA_NAME = "libfactory.ta";
#endif

const char* OTP_DUMP_FILE = "/data/otp_dump.bin";

#define FACTORY_TA_UUID {0x1316a183, 0x894d, 0x43fe, \
    {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x20}}

#define DRM_SE_CA_CMD_OTP_READ_CAS1_JTAG  0xE003
#define DRM_SE_CA_CMD_OTP_READ_CAS2_JTAG  0xE004
#define DRM_SE_CA_CMD_OTP_READ_CAS4_JTAG  0xE005
#define DRM_SE_CA_CMD_OTP_READ_CAS4_NDSID 0xE006
#define DRM_SE_CA_CMD_OTP_READ_AUTO_JTAG  0xE007
#define DRM_SE_CA_CMD_OTP_READ_PUBLIC_DATA  0xE008
#define DRM_SE_CA_CMD_OTP_READ_SOC_UID    0xE009
#define DRM_SE_CA_CMD_OTP_READ_CHIP_REV   0xE00A

#define DRM_CAS4_PUB_OTP_LEN 8

#define CAS1_JTAG_NAME "CAS1 JTAG"
#define CAS2_JTAG_NAME "CAS2 JTAG"
#define CAS4_JTAG_NAME "CAS4 JTAG"
#define AUTO_JTAG_NAME "AUTO JTAG"

#define PUBLIC_OTP_SIZE 256

typedef struct {
   unsigned int   TA_Vendor_SegID[6];
   unsigned char  TA_Vendor_GID[6];
   unsigned char  Security_Indicators;
   unsigned int   BOOT_SegID;
   unsigned char  Lock_Boot_SegID;
   unsigned char  MP_Provision_Done;
   unsigned char  REE_Security_Enable;
   unsigned char  TEE_Segmentation_Version_Enable;
   unsigned char  Boot_Security_Enable;
   unsigned int   TEE_SegID;
   unsigned char  SCS_Total_Area_Size_Sel;
   unsigned char  Background_Check_Enable;
   unsigned char  JtagProg_Disable;
   unsigned int   REE_SegID;
   unsigned int   ConcurrencyControl;
   unsigned char  v_PubOtpMinConfVer;
   unsigned char  v_PubOtpRsaIndex;
   unsigned short v_PubOtpVID;
   unsigned short v_PubOtpOID;
   unsigned char  OtpWrtPwdProt;
   unsigned char  JTAG_Protection;
   unsigned char  JTAG_Protection_dup;
   unsigned char  BSCAN_Protection;
   unsigned char  BSCAN_Protection_dup;
   unsigned char  STEST_Protection;
   unsigned char  STEST_Protection_dup;
} PUBLIC_OTP_ITEMS;

typedef signed int HRESULT;
#define S_OK 0
#define S_FALSE 1

static const TEEC_UUID TEEC_FACTORY_TA_UUID = FACTORY_TA_UUID;
static bool bInitialized = false;
static TEEC_Context context;
static TEEC_Session session;

void UsageHelp(int argc, char* argv[])
{
    printf("Usage:\n");
    printf("    %s category case ...\n", argv[0]);
    printf("    category:\n");
    printf("        0: read JTAG status (check CAS automatically)\n");
    printf("        1: read CAS4 PUBLIC OTP\n");
    printf("            case:\n");
    printf("                0: read CAS4 NDSID\n");
    printf("        2: read encrypted public OTP items to %s\n", OTP_DUMP_FILE);
    printf("        3: read SOC_UID from OTP\n");
    printf("        4: read chip revision from OTP\n");
}

static void printPublicOtp(const PUBLIC_OTP_ITEMS *otp) {
    int i;

    printf("TA_Vendor_SegID: ");
    for (i = 0; i < 6; i++) {
        printf("0x%08X ", otp->TA_Vendor_SegID[i]);
    }
    printf("\n");

    printf("TA_Vendor_GID: ");
    for (i = 0; i < 6; i++) {
        printf("0x%02X ", otp->TA_Vendor_GID[i]);
    }
    printf("\n");

    printf("Security_Indicators: 0x%02X\n", otp->Security_Indicators);
    printf("BOOT_SegID: 0x%08X\n", otp->BOOT_SegID);
    printf("Lock_Boot_SegID: 0x%02X\n", otp->Lock_Boot_SegID);
    printf("MP_Provision_Done: 0x%02X\n", otp->MP_Provision_Done);
    printf("REE_Security_Enable: 0x%02X\n", otp->REE_Security_Enable);
    printf("TEE_Segmentation_Version_Enable: 0x%02X\n", otp->TEE_Segmentation_Version_Enable);
    printf("Boot_Security_Enable: 0x%02X\n", otp->Boot_Security_Enable);
    printf("TEE_SegID: 0x%08X\n", otp->TEE_SegID);
    printf("SCS_Total_Area_Size_Sel: 0x%02X\n", otp->SCS_Total_Area_Size_Sel);
    printf("Background_Check_Enable: 0x%02X\n", otp->Background_Check_Enable);
    printf("JtagProg_Disable: 0x%02X\n", otp->JtagProg_Disable);
    printf("REE_SegID: 0x%08X\n", otp->REE_SegID);
    printf("ConcurrencyControl: 0x%08X\n", otp->ConcurrencyControl);
    printf("v_PubOtpMinConfVer: 0x%02X\n", otp->v_PubOtpMinConfVer);
    printf("v_PubOtpRsaIndex: 0x%02X\n", otp->v_PubOtpRsaIndex);
    printf("v_PubOtpVID: 0x%04X\n", otp->v_PubOtpVID);
    printf("v_PubOtpOID: 0x%04X\n", otp->v_PubOtpOID);
    printf("OtpWrtPwdProt: 0x%02X\n", otp->OtpWrtPwdProt);
    printf("JTAG_Protection: 0x%02X\n", otp->JTAG_Protection);
    printf("JTAG_Protection_dup: 0x%02X\n", otp->JTAG_Protection_dup);
    printf("BSCAN_Protection: 0x%02X\n", otp->BSCAN_Protection);
    printf("BSCAN_Protection_dup: 0x%02X\n", otp->BSCAN_Protection_dup);
    printf("STEST_Protection: 0x%02X\n", otp->STEST_Protection);
    printf("STEST_Protection_dup: 0x%02X\n", otp->STEST_Protection_dup);
}

HRESULT Factory_TZInitialize(void)
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
#endif
    /* ========================================================================
       [3] Open session with TEE application
     ======================================================================== */
    /* Open a Session with the TEE application. */
    result = TEEC_OpenSession(
              &context,
              &session,
              &TEEC_FACTORY_TA_UUID,
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

void Factory_TZFinalize(void)
{
    if (!bInitialized)
        return;
    bInitialized = false;

    TEEC_CloseSession(&session);
    TEEC_FinalizeContext(&context);
}

int _read_jtag_status(int category_case)
{
    HRESULT lRes = S_FALSE;
    TEEC_Result result = TEEC_ERROR_GENERIC;
    uint32_t cmdid;
    char cmdname[16];
    TEEC_Operation operation;
    TEEC_SharedMemory hShm;
    unsigned char jtag_status;

    switch (category_case) {
    case 0:
        cmdid = DRM_SE_CA_CMD_OTP_READ_AUTO_JTAG;
        strcpy(cmdname, AUTO_JTAG_NAME);
        break;
    default:
        goto __EXIT;
    }

    lRes = Factory_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    hShm.size = 1;
    hShm.flags = TEEC_MEM_OUTPUT;
    result = TEEC_AllocateSharedMemory(
                   &context,
                   &hShm);
    if (result != TEEC_SUCCESS) {
        printf("TEEC allocate SHM fail[ret=0x%x]\n", result);
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.params[0].memref.parent = &hShm;
    operation.params[0].memref.size = hShm.size;
    operation.params[0].memref.offset = 0;
    operation.started = 1;

    result = TEEC_InvokeCommand(
            &session,
            cmdid,
            &operation,
            NULL);

    if (result == TEEC_SUCCESS) {
       memcpy(&jtag_status, hShm.buffer, 1);
       printf("Read %s status: %d\n", cmdname, jtag_status);
    }
    else {
       printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
       lRes = S_FALSE;
    }

__EXIT:
    TEEC_ReleaseSharedMemory(&hShm);
    Factory_TZFinalize();
    return lRes;
}

HRESULT _read_cas4_pubOtp(int category_case)
{
    HRESULT lRes = S_FALSE;
    TEEC_Result result = TEEC_ERROR_GENERIC;
    char ndsid[DRM_CAS4_PUB_OTP_LEN];
    TEEC_Operation operation;
    TEEC_SharedMemory hShm;

    /* param category_case  - reserved for future use */

    lRes = Factory_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_OUTPUT,
                                            TEEC_NONE,
                                            TEEC_NONE,
                                            TEEC_NONE);
    hShm.size = DRM_CAS4_PUB_OTP_LEN;
    hShm.flags = TEEC_MEM_OUTPUT;
    result = TEEC_AllocateSharedMemory(
                   &context,
                   &hShm);
    if (result != TEEC_SUCCESS) {
        printf("TEEC allocate SHM fail[ret=0x%x]\n", result);
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.params[0].memref.parent = &hShm;
    operation.params[0].memref.size = hShm.size;
    operation.params[0].memref.offset = 0;
    operation.started = 1;

    result = TEEC_InvokeCommand(
            &session,
            DRM_SE_CA_CMD_OTP_READ_CAS4_NDSID,
            &operation,
            NULL);

    if (result == TEEC_SUCCESS) {
        memcpy(ndsid, hShm.buffer, DRM_CAS4_PUB_OTP_LEN);
        printf("CAS4 NDS_ID: %02x%02x%02x%02x%02x%02x%02x%02x\n",
                ndsid[0], ndsid[1], ndsid[2], ndsid[3], \
                ndsid[4], ndsid[5], ndsid[6], ndsid[7]);
        lRes = S_OK;
    } else {
        printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
        lRes = S_FALSE;
    }

__EXIT:
    TEEC_ReleaseSharedMemory(&hShm);
    Factory_TZFinalize();
    return lRes;
}

HRESULT _read_public_otp_items(void)
{
    HRESULT lRes = S_FALSE;
    TEEC_Result result = TEEC_ERROR_GENERIC;
    char pubOtp[256];
    TEEC_Operation operation;
    TEEC_SharedMemory hShm;
    FILE *file = NULL;

    lRes = Factory_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_OUTPUT,
                                            TEEC_NONE,
                                            TEEC_NONE,
                                            TEEC_NONE);
    hShm.size = 256;
    hShm.flags = TEEC_MEM_OUTPUT;
    result = TEEC_AllocateSharedMemory(
                   &context,
                   &hShm);
    if (result != TEEC_SUCCESS) {
        printf("TEEC allocate SHM fail[ret=0x%x]\n", result);
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.params[0].memref.parent = &hShm;
    operation.params[0].memref.size = hShm.size;
    operation.params[0].memref.offset = 0;
    operation.started = 1;

    result = TEEC_InvokeCommand(
            &session,
            DRM_SE_CA_CMD_OTP_READ_PUBLIC_DATA,
            &operation,
            NULL);

    if (result == TEEC_SUCCESS) {
        memcpy(pubOtp, hShm.buffer, 256);
#ifdef CONFIG_TZK
        file = fopen(OTP_DUMP_FILE, "w+");
        if (file != NULL) {
            fwrite(pubOtp, 256, 1, file);
            fclose(file);
            lRes = S_OK;
        } else {
            lRes = S_FALSE;
        }
#else
        printPublicOtp((PUBLIC_OTP_ITEMS *)pubOtp);
        lRes = S_OK;
#endif
    } else {
        printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
        lRes = S_FALSE;
    }

__EXIT:
    TEEC_ReleaseSharedMemory(&hShm);
    Factory_TZFinalize();
    return lRes;
}

HRESULT _read_SOC_UID(void)
{
    HRESULT lRes = S_FALSE;
    TEEC_Result result = TEEC_ERROR_GENERIC;
    char socUID[8];
    int socUID_size = 8;
    TEEC_Operation operation;
    TEEC_SharedMemory hShm;
    FILE *file = NULL;

    lRes = Factory_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_OUTPUT,
                                            TEEC_NONE,
                                            TEEC_NONE,
                                            TEEC_NONE);
    hShm.size = socUID_size;
    hShm.flags = TEEC_MEM_OUTPUT;
    result = TEEC_AllocateSharedMemory(
                   &context,
                   &hShm);
    if (result != TEEC_SUCCESS) {
        printf("TEEC allocate SHM fail[ret=0x%x]\n", result);
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.params[0].memref.parent = &hShm;
    operation.params[0].memref.size = hShm.size;
    operation.params[0].memref.offset = 0;
    operation.started = 1;

    result = TEEC_InvokeCommand(
            &session,
            DRM_SE_CA_CMD_OTP_READ_SOC_UID,
            &operation,
            NULL);

    if (result == TEEC_SUCCESS) {
        memcpy(socUID, hShm.buffer, socUID_size);
        printf("SOC_UID: %02x%02x%02x%02x%02x%02x%02x%02x\n", socUID[0], socUID[1], socUID[2], socUID[3], \
                                                              socUID[4], socUID[5], socUID[6], socUID[7]);
    } else {
        printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
        lRes = S_FALSE;
    }

__EXIT:
    TEEC_ReleaseSharedMemory(&hShm);
    Factory_TZFinalize();
    return lRes;
}

HRESULT _read_chip_rev(void)
{
    HRESULT lRes = S_FALSE;
    TEEC_Result result = TEEC_ERROR_GENERIC;
    TEEC_Operation operation;
    TEEC_SharedMemory hShm;
    unsigned char chip_rev;

    lRes = Factory_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_OUTPUT,
                                            TEEC_NONE,
                                            TEEC_NONE,
                                            TEEC_NONE);
    hShm.size = 1;
    hShm.flags = TEEC_MEM_OUTPUT;
    result = TEEC_AllocateSharedMemory(
                   &context,
                   &hShm);
    if (result != TEEC_SUCCESS) {
        printf("TEEC allocate SHM fail[ret=0x%x]\n", result);
        lRes = S_FALSE;
        goto __EXIT;
    }

    operation.params[0].memref.parent = &hShm;
    operation.params[0].memref.size = hShm.size;
    operation.params[0].memref.offset = 0;
    operation.started = 1;

    result = TEEC_InvokeCommand(
            &session,
            DRM_SE_CA_CMD_OTP_READ_CHIP_REV,
            &operation,
            NULL);

    if (result == TEEC_SUCCESS) {
        memcpy(&chip_rev, hShm.buffer, 1);
        printf("Chip revision: %d\n", chip_rev);
    } else {
        printf("%s:%d: ret=0x%08x\n", __FUNCTION__, __LINE__, result);
        lRes = S_FALSE;
    }

__EXIT:
    TEEC_ReleaseSharedMemory(&hShm);
    Factory_TZFinalize();
    return lRes;
}

int main(int argc, char *argv[])
{
    int status = -1;
    int category = -1;
    int category_case = -1;

    if (argc != 2 && argc != 3) {
       UsageHelp(argc, argv);
       return status;
    }

    category = strtol(argv[1], NULL, 10);
    if (argc == 3)
        category_case = strtol(argv[2], NULL, 10);

    switch (category) {
    case 0:
       status = _read_jtag_status(0);
       break;
    case 1:
        status = _read_cas4_pubOtp(category_case);
        break;
    case 2:
        status = _read_public_otp_items();
        break;
    case 3:
        status = _read_SOC_UID();
        break;
    case 4:
        status = _read_chip_rev();
        break;
    default:
       UsageHelp(argc, argv);
       break;
    }

    return status;
}
