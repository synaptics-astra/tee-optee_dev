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
//Command
#define DRM_SE_CA_OEM_ACCESS_SECURE_SRAM 0xF002

//OTP
#define DRM_SE_CA_CMD_OTP_READ_CAS1_JTAG  0xE003
#define DRM_SE_CA_CMD_OTP_READ_CAS2_JTAG  0xE004
#define DRM_SE_CA_CMD_OTP_READ_CAS4_JTAG  0xE005
#define DRM_SE_CA_CMD_OTP_READ_CAS4_NDSID 0xE006
#define DRM_SE_CA_CMD_OTP_READ_AUTO_JTAG  0xE007
#define DRM_SE_CA_CMD_OTP_READ_PUBLIC_DATA  0xE008
#define DRM_SE_CA_CMD_OTP_READ_SOC_UID    0xE009
#define DRM_SE_CA_CMD_OTP_READ_CHIP_REV   0xE00A

#define DRM_SE_CA_CMD_OTP_READ_OTP        0xE00C
#define DRM_SE_CA_CMD_OTP_WRITE_OTP       0xE00D

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
   unsigned char  RKEKID[8];
   unsigned int   K0_REE_HASH[8];
   unsigned int   REE_VERSION;
   unsigned int   BOOT_VERSION;
   unsigned int   JTAGProtectionLevel;
} PUBLIC_OTP_ITEMS;

typedef enum {
    /* ---- USER DATA Section ---- */
    OTP_USER_DATA_0 = 0,                      // 128 bytes = 1024 bits = 32 x uint32_t
    OTP_USER_DATA_1,
    OTP_USER_DATA_2,
    OTP_USER_DATA_3,
    OTP_USER_DATA_4,
    OTP_USER_DATA_5,
    OTP_USER_DATA_6,
    OTP_USER_DATA_7,
    OTP_USER_DATA_8,
    OTP_USER_DATA_9,
    OTP_USER_DATA_10,
    OTP_USER_DATA_11,
    OTP_USER_DATA_12,
    OTP_USER_DATA_13,
    OTP_USER_DATA_14,
    OTP_USER_DATA_15,
    OTP_USER_DATA_16,
    OTP_USER_DATA_17,
    OTP_USER_DATA_18,
    OTP_USER_DATA_19,
    OTP_USER_DATA_20,
    OTP_USER_DATA_21,
    OTP_USER_DATA_22,
    OTP_USER_DATA_23,
    OTP_USER_DATA_24,
    OTP_USER_DATA_25,
    OTP_USER_DATA_26,
    OTP_USER_DATA_27,
    OTP_USER_DATA_28,
    OTP_USER_DATA_29,
    OTP_USER_DATA_30,
    OTP_USER_DATA_31,

    /* ---- OEM Ownership ---- */
    OTP_K0_OEM_HASH_0 = 100,              // 256 bits = 8 x uint32_t
    OTP_K0_OEM_HASH_1,
    OTP_K0_OEM_HASH_2,
    OTP_K0_OEM_HASH_3,
    OTP_K0_OEM_HASH_4,
    OTP_K0_OEM_HASH_5,
    OTP_K0_OEM_HASH_6,
    OTP_K0_OEM_HASH_7,

    OTP_AESK0_0,                          // 128 bits = 4 x uint32_t
    OTP_AESK0_1,
    OTP_AESK0_2,
    OTP_AESK0_3,

    OTP_REE_SEGID,                        // 32 bits (1 x uint32_t)
    OTP_REE_SECURITY_ENABLE,              // 1 bits (1 x uint32_t)
    OTP_BOOT_SECURITY_ENABLE,             // 1 bits (1 x uint32_t)
    OTP_MP_PROVISION_DONE,                // 1 bits (1 x uint32_t)
    OTP_SCS_AREA_SIZE_SEL,                // 2 bits (1 x uint32_t)
    OTP_REE_JTAG_PROTECTION_POLICY,       // 2 bits (1 x uint32_t)
    OTP_REE_VERSION,                      // 32 bits (1 x uint32_t)
    OTP_RKEK_ID_0,                        // 64 bits = 2 x uint32_t
    OTP_RKEK_ID_1,

    /* ---- Synaptics Ownership ---- */
    OTP_EMMC_BOOT_DISABLE = 200,          // 1 bit  (1 x uint32_t)
    OTP_SPI_BOOT_DISABLE,                 // 1 bit  (1 x uint32_t)
    OTP_DOLBY_AUDIO_DISABLE,              // 1 bit  (1 x uint32_t)
    OTP_OEM_AUDIO_CUSTOMER_ID,            // 32 bits (1 x uint32_t)
    OTP_PRODUCTION_CHIP_FLAG,             // 3 bits  (1 x uint32_t)
    OTP_USB_BOOT_DISABLE,                 // 1 bit  (1 x uint32_t)
    OTP_BOOT_VERSION,                     // 32 bits (1 x uint32_t)
    OTP_SOC_UID_0,                        // 64 bits = 2 x uint32_t
    OTP_SOC_UID_1,
    OTP_BOOT_SEGID,                       // 32 bits (1 x uint32_t)
    OTP_TEE_SEGID,                        // 32 bits (1 x uint32_t)
    OTP_TEE_SECURITY_ENABLE,              // 1 bit   (1 x uint32_t)

    OTP_FIELD_MAX
} OTP_FIELD_ID;

/* ---- Lookup Table ---- */
typedef struct {
    int value;
    const char *name;
} otp_field_entry_t;

static const otp_field_entry_t otp_field_table[] = {
#define X(v) {v, #v}
    /* USER DATA Section */
    X(OTP_USER_DATA_0),  X(OTP_USER_DATA_1),  X(OTP_USER_DATA_2),  X(OTP_USER_DATA_3),
    X(OTP_USER_DATA_4),  X(OTP_USER_DATA_5),  X(OTP_USER_DATA_6),  X(OTP_USER_DATA_7),
    X(OTP_USER_DATA_8),  X(OTP_USER_DATA_9),  X(OTP_USER_DATA_10), X(OTP_USER_DATA_11),
    X(OTP_USER_DATA_12), X(OTP_USER_DATA_13), X(OTP_USER_DATA_14), X(OTP_USER_DATA_15),
    X(OTP_USER_DATA_16), X(OTP_USER_DATA_17), X(OTP_USER_DATA_18), X(OTP_USER_DATA_19),
    X(OTP_USER_DATA_20), X(OTP_USER_DATA_21), X(OTP_USER_DATA_22), X(OTP_USER_DATA_23),
    X(OTP_USER_DATA_24), X(OTP_USER_DATA_25), X(OTP_USER_DATA_26), X(OTP_USER_DATA_27),
    X(OTP_USER_DATA_28), X(OTP_USER_DATA_29), X(OTP_USER_DATA_30), X(OTP_USER_DATA_31),

    /* OEM Ownership */
    X(OTP_K0_OEM_HASH_0), X(OTP_K0_OEM_HASH_1), X(OTP_K0_OEM_HASH_2), X(OTP_K0_OEM_HASH_3),
    X(OTP_K0_OEM_HASH_4), X(OTP_K0_OEM_HASH_5), X(OTP_K0_OEM_HASH_6), X(OTP_K0_OEM_HASH_7),
    X(OTP_AESK0_0), X(OTP_AESK0_1), X(OTP_AESK0_2), X(OTP_AESK0_3),
    X(OTP_REE_SEGID), X(OTP_REE_SECURITY_ENABLE), X(OTP_BOOT_SECURITY_ENABLE),
    X(OTP_MP_PROVISION_DONE), X(OTP_SCS_AREA_SIZE_SEL), X(OTP_REE_JTAG_PROTECTION_POLICY),
    X(OTP_REE_VERSION), X(OTP_RKEK_ID_0), X(OTP_RKEK_ID_1),

    /* Synaptics Ownership */
    X(OTP_EMMC_BOOT_DISABLE), X(OTP_SPI_BOOT_DISABLE), X(OTP_DOLBY_AUDIO_DISABLE),
    X(OTP_OEM_AUDIO_CUSTOMER_ID), X(OTP_PRODUCTION_CHIP_FLAG), X(OTP_USB_BOOT_DISABLE),
    X(OTP_BOOT_VERSION), X(OTP_SOC_UID_0), X(OTP_SOC_UID_1),
    X(OTP_BOOT_SEGID), X(OTP_TEE_SEGID), X(OTP_TEE_SECURITY_ENABLE),

    X(OTP_FIELD_MAX)
#undef X
};

typedef signed int HRESULT;
#define S_OK 0
#define S_FALSE 1

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

static const TEEC_UUID TEEC_FACTORY_TA_UUID = FACTORY_TA_UUID;
static bool bInitialized = false;
static TEEC_Context context;
static TEEC_Session session;

void UsageHelp(int argc, char* argv[])
{
    printf("Usage:\n");
    printf("    %s category case ...\n", argv[0]);
    printf("\n");
    printf("    category:\n");
    printf("        0: read JTAG status (check CAS automatically)\n");
    printf("        1: read CAS4 PUBLIC OTP\n");
    printf("            case:\n");
    printf("                0: read CAS4 NDSID\n");
    printf("        2: read encrypted public OTP items to %s\n", OTP_DUMP_FILE);
    printf("        3: read SOC_UID from OTP\n");
    printf("        4: read chip revision from OTP\n");
    printf("        5: secure SRAM access\n");
    printf("            Usage: %s 5 <action> <idx> [value]\n", argv[0]);
    printf("            Actions:\n");
    printf("                read/r: Read 64-bit value from the specified buffer index.\n");
    printf("                write/w: Write 64-bit value to the specified buffer index.\n");
    printf("            Indices:\n");
    printf("                idx0/0: Currently the ONLY supported index.\n");
    printf("                (Note: idx > 0 will return 'no support')\n");
    printf("            Value:\n");
    printf("                [64-bit Hex] : Required for write (e.g., 0x12345678AABBCCDD).\n");
    printf("\n");
    printf("                %s 5 read 0\n", argv[0]);
    printf("                %s 5 write idx0 0x55556666\n", argv[0]);
    printf("        6: OTP utility commands\n");
    printf("            usage:\n");
    printf("                %s 6 otpread  <otp_index>\n", argv[0]);
    printf("                %s 6 otpread    0\n", argv[0]);
    printf("                %s 6 otpread    1\n", argv[0]);
    printf("\n");
    printf("                %s 6 otpwrite <otp_index> <data> <mask>\n", argv[0]);
    printf("                %s 6 otpwrite 1 0x500      0xFFF\n", argv[0]);
    printf("                %s 6 otpwrite 2 0x1A2B3C4D 0xFFFFFFFF\n", argv[0]);
    printf("                %s 6 otpwrite 3 0x1A2B0000 0xFFFF0000\n", argv[0]);
    printf("                %s 6 otpwrite 4 0x00010000 0x00010000\n", argv[0]);
    printf("\n");
    printf("            Note:\n");
    printf("                - otp_index: can be decimal or hex (e.g., 10 or 0xA)\n");
    printf("                - otpread  : read OTP value at index\n");
    printf("                             return:  0          : Success\n");
    printf("                             return:  0xFF000001 : STATUS_FAILURE\n");
    printf("                             return:  0xFF000050 : STATUS_OTP_INVALID_IDX\n");
    printf("                             return:  0xFF000052 : STATUS_OTP_ERROR_WRITEONLY_FIELD\n");
    printf("                - otpwrite : write OTP with data and mask\n");
    printf("                             return:  0          : Success\n");
    printf("                             return:  0xFF000001 : STATUS_FAILURE\n");
    printf("                             return:  0xFF000050 : STATUS_OTP_INVALID_IDX\n");
    printf("                             return:  0xFF000051 : STATUS_OTP_ERROR_READONLY_FIELD\n");

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
    printf("REE_VERSION: 0x%08X\n", otp->REE_VERSION);
    printf("BOOT_VERSION: 0x%08X\n", otp->BOOT_VERSION);
    printf("JTAGProtectionLevel: 0x%08X\n", otp->JTAGProtectionLevel);

    printf("RKEKID: %02x%02x%02x%02x%02x%02x%02x%02x\n", otp->RKEKID[7], otp->RKEKID[6],
           otp->RKEKID[5], otp->RKEKID[4], otp->RKEKID[3], otp->RKEKID[2], otp->RKEKID[1],
           otp->RKEKID[0]);

    printf("K0_REE_HASH: ");
    for (i = 0; i < 8; i++)
        printf("0x%08X ", otp->K0_REE_HASH[i]);

    printf("\n");
}

/* ---- Dump Implementation ---- */
static void do_otp_idx_dump(void)
{
    printf("                %-8s %-40s\n", "otp_index", "Name");
    printf("                -----------------------------------------------\n");
    for (size_t i = 0; i < ARRAY_SIZE(otp_field_table); ++i)
        printf("                %-8d %s\n", otp_field_table[i].value, otp_field_table[i].name);
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
HRESULT _access_secure_SRAM(bool isWrite, uint32_t idx, uint64_t *val)
{
    HRESULT lRes = S_FALSE;
    TEEC_Result result = TEEC_ERROR_GENERIC;
    TEEC_Operation operation;
    uint32_t err_origin = 0;

    if (idx != 0) {
        printf("Error: Only buffer index 0 is supported currently.\n");
        return S_FALSE;
    }

    lRes = Factory_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    memset(&operation, 0, sizeof(operation));

    if (isWrite) {
        //Write - Split 64-bit 'val' into two 32-bit parameters (a and b)
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,  //idx
                                                TEEC_VALUE_INPUT,  //value
                                                TEEC_NONE,
                                                TEEC_NONE);
        operation.params[0].value.a = idx;
        operation.params[1].value.a = (uint32_t)(*val & 0xFFFFFFFF);
        operation.params[1].value.b = (uint32_t)((*val >> 32) & 0xFFFFFFFF);
    } else {
        // Mode: Read - Pass index as input, expect 64-bit result as output
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,  // idx
                                                TEEC_VALUE_OUTPUT, // val_low, val_high
                                                TEEC_NONE,
                                                TEEC_NONE);
        operation.params[0].value.a = idx;
    }

    result = TEEC_InvokeCommand(
                &session,
                DRM_SE_CA_OEM_ACCESS_SECURE_SRAM, // Command ID
                &operation,
                &err_origin);

    if (result == TEEC_SUCCESS) {
        if (!isWrite) {
            printf("Read SRAM[idx %d] = 0x%08x%08x\n", idx, operation.params[1].value.b, operation.params[1].value.a);
        } else {
            printf("Write SRAM[idx %d] success.\n", idx);
        }
        lRes = S_OK;
    } else {
        printf("%s: TEEC_InvokeCommand failed. ret=0x%08x, origin=0x%08x\n", __FUNCTION__, result, err_origin);
        lRes = S_FALSE;
    }

__EXIT:
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

HRESULT _otp_read(uint32_t idx, uint32_t *val, uint32_t *mask)
{
    HRESULT      lRes    = S_FALSE;
    TEEC_Result  result  = TEEC_ERROR_GENERIC;
    uint32_t     origin  = 0;
    TEEC_Operation operation;

    if (!val || !mask) {
        printf("%s: invalid output pointer(s)\n", __FUNCTION__);
        return S_FALSE;
    }

    if (idx >= OTP_FIELD_MAX) {
        printf("%s: invalid otp index %u\n", __FUNCTION__, idx);
        return S_FALSE;
    }

    *val  = 0;
    *mask = 0;

    lRes = Factory_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        lRes = S_FALSE;
        goto __EXIT;
    }

    memset(&operation, 0, sizeof(operation));
    operation.paramTypes = TEEC_PARAM_TYPES(
        TEEC_VALUE_INPUT,   /* p0: idx in a */
        TEEC_VALUE_OUTPUT,  /* p1: val out in a */
        TEEC_VALUE_OUTPUT,  /* p2: mask out in a */
        TEEC_NONE
    );

    operation.params[0].value.a = (uint32_t)idx;
    operation.started = 1;

    result = TEEC_InvokeCommand(
        &session,
        DRM_SE_CA_CMD_OTP_READ_OTP,
        &operation,
        &origin);

    *val  = operation.params[1].value.a;
    *mask = operation.params[2].value.a;

    if (result == TEEC_SUCCESS) {
        lRes = S_OK;
    } else {
        printf("%s:%d: Invoke failed, res=0x%08x origin=%u\n",
               __FUNCTION__, __LINE__, result, origin);
        lRes = S_FALSE;
    }

    printf("%s: idx=%d, val=0x%08X, mask=0x%08X\n", __FUNCTION__, idx, *val, *mask);

__EXIT:
    Factory_TZFinalize();
    return lRes;
}

HRESULT _otp_write(uint32_t idx, uint32_t val, uint32_t mask)
{
    HRESULT        lRes    = S_FALSE;
    TEEC_Result    result  = TEEC_ERROR_GENERIC;
    uint32_t       origin  = 0;
    TEEC_Operation operation;
    unsigned       attempt = 0;

    printf("%s: idx=%d, val=0x%08X, mask=0x%08X\n", __FUNCTION__, idx, val, mask);

    if (idx >= OTP_FIELD_MAX) {
        printf("%s: invalid otp index %u\n", __FUNCTION__, idx);
        return S_FALSE;
    }

    lRes = Factory_TZInitialize();
    if (lRes != S_OK) {
        printf("TZ initialize failed\n");
        return S_FALSE;
    }

    memset(&operation, 0, sizeof(operation));
    operation.paramTypes = TEEC_PARAM_TYPES(
        TEEC_VALUE_INPUT,   /* p0: idx  */
        TEEC_VALUE_INPUT,   /* p1: val  */
        TEEC_VALUE_INPUT,   /* p2: mask */
        TEEC_NONE
    );

    operation.params[0].value.a = idx;
    operation.params[1].value.a = val;
    operation.params[2].value.a = mask;

    operation.started = 1;

    result = TEEC_InvokeCommand(
            &session,
            DRM_SE_CA_CMD_OTP_WRITE_OTP,
            &operation,
            &origin);

    if (result == TEEC_SUCCESS) {
        lRes = S_OK;
    } else {
        printf("%s: Invoke failed (idx=%d, val=0x%08X, mask=0x%08X), "
               "res=0x%08X origin=%u\n",
               __FUNCTION__, idx, val, mask, result, origin);
        lRes = S_FALSE;
    }

    Factory_TZFinalize();
    return lRes;
}

static int parse_u32(const char *s, uint32_t *out)
{
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0);  /* base 0: auto 0x, 0, or 10 */
    if (end == s || *end != '\0' || v > 0xFFFFFFFFUL) {
        return -1;
    }
    *out = (uint32_t)v;
    return 0;
}

int main(int argc, char *argv[])
{
    int status = -1;
    uint32_t category = (uint32_t)-1;
    uint32_t category_case = (uint32_t)-1;

    if (argc < 2) {
        UsageHelp(argc, argv);
        return status;
    }

    /* Parse category */
    if (parse_u32(argv[1], &category) != 0) {
        fprintf(stderr, "Invalid <category>: '%s'\n", argv[1]);
        UsageHelp(argc, argv);
        return status;
    }

    switch (category) {
    case 0:
        if (argc != 2) { UsageHelp(argc, argv); break; }
        status = _read_jtag_status(0);
        break;

    case 1:
        if (argc != 3) { UsageHelp(argc, argv); break; }
        if (parse_u32(argv[2], &category_case) != 0) {
            fprintf(stderr, "Invalid <case>: '%s'\n", argv[2]);
            break;
        }
        status = _read_cas4_pubOtp((int)category_case);
        break;

    case 2:
        if (argc != 2) { UsageHelp(argc, argv); break; }
        status = _read_public_otp_items();
        break;

    case 3:
        if (argc != 2) { UsageHelp(argc, argv); break; }
        status = _read_SOC_UID();
        break;

    case 4:
        if (argc != 2) { UsageHelp(argc, argv); break; }
        status = _read_chip_rev();
        break;
    case 5:
        // Basic argument count check
        if (argc < 4) { UsageHelp(argc, argv); break; }
        uint32_t idx = 0;
        unsigned long long val_check = 0;
        uint64_t val = 0;
        char* action = argv[2];
        char* endptr;

        // Parse index
        if (strncmp(argv[3], "idx", 3) == 0) {
            idx = (uint32_t)strtoul(argv[3] + 3, NULL, 10);
        } else {
            idx = (uint32_t)strtoul(argv[3], NULL, 10);
        }

        // Branch user action
        if (strcasecmp(action, "write") == 0 || strcasecmp(action, "w") == 0) {
            // Write action requires 5 argument: value
            if (argc < 5) {
                printf("Error: Write action requires a hex value (e.g., 0x55556666).\n");
                break;
            }

            // Convert string to 64-bit integer
            val_check = strtoull(argv[4], &endptr, 16);

            // Data validation: Check for 64-bit overflow and format errors
            if(val_check == ~0ULL) {
                printf("Error: Value is out of 64-bit range (Overflow)!\n");
                break;
            }
            if(endptr == argv[4] || *endptr != '\0') {
                printf("Error: Invalid hex format '%s'.\n", argv[4]);
                break;
            }
            val = (uint64_t)val_check;
            status = _access_secure_SRAM(true, idx, &val);

        } else if (strcasecmp(action, "read") == 0 || strcasecmp(action, "r") == 0) {
            status = _access_secure_SRAM(false, idx, &val);
        } else {
            printf("Error: Invalid action '%s'. Use 'read' or 'write'.\n", action);
            UsageHelp(argc, argv);
        }
        break;

    case 6: {
        if (argc < 4) { UsageHelp(argc, argv); do_otp_idx_dump(); break; }

        const char *cmd = argv[2];

        if (strcmp(cmd, "otpread") == 0) {
            /* ./factory_util 6 otpread otp_index */
            if (argc != 4) { UsageHelp(argc, argv); do_otp_idx_dump(); break; }

            uint32_t idx = 0, val = 0, mask = 0;
            if (parse_u32(argv[3], &idx) != 0) {
                fprintf(stderr, "Invalid <otp_index>: '%s'\n", argv[3]);
                break;
            }

            status = _otp_read(idx, &val, &mask);
            if (status == 0) {
                printf("OTP[%u] = 0x%08X, mask=0x%08X\n", idx, val, mask);
            } else {
                fprintf(stderr, "otpread failed for index %u (status=%d)\n", idx, status);
            }

        } else if (strcmp(cmd, "otpwrite") == 0) {
            /* ./factory_util 6 otpwrite otp_index data mask */
            if (argc != 6) { UsageHelp(argc, argv); do_otp_idx_dump(); break; }

            uint32_t idx = 0, data = 0, mask = 0;
            if (parse_u32(argv[3], &idx)  != 0) { fprintf(stderr, "Invalid <otp_index>: '%s'\n", argv[3]); break; }
            if (parse_u32(argv[4], &data) != 0) { fprintf(stderr, "Invalid <data>: '%s'\n",      argv[4]); break; }
            if (parse_u32(argv[5], &mask) != 0) { fprintf(stderr, "Invalid <mask>: '%s'\n",      argv[5]); break; }

            status = _otp_write(idx, data, mask);
            if (status == 0) {
                printf("Wrote OTP[%u]: data=0x%08X mask=0x%08X\n", idx, data, mask);
            } else {
                fprintf(stderr, "otpwrite failed for index %u (status=%d)\n", idx, status);
            }

        } else {
            fprintf(stderr, "Unknown subcommand for category 6: '%s'\n", cmd);
            UsageHelp(argc, argv);
        }
        break;
    }

    default:
        UsageHelp(argc, argv);
        break;
    }

    return status;
}
