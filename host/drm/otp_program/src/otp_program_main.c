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
//! \file: otp_program_main.c
//!
//! \brief: program OTP tool
//!
//!
//! Version    Date                     Author
//! V 0.02,    Aug 15 2019,             Cheney Yang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include "otp_program_main.h"
#include "DRM_FIRSTBOOT_AGENT_Api.h"
#include "DRM_OTP_Agent_API.h"
#include "tee_client_api.h"

#define EVT_PACKAGE_SIZE 1280

#ifdef CONFIG_TZK
#if defined(ANDROID)
const char* FACTORY_TA_PATH = "/vendor/firmware/ta";
#else
const char* FACTORY_TA_PATH = "/opt/syna/ta";
#endif
const char* FACTORY_TA_NAME = "libfactory.ta";
#endif

void print_usage(char* ptr_cmd_name)
{
    printf("usage: %s [-f] OTP_PACK_FILE\n", ptr_cmd_name);
    return;
}

HRESULT MV_OTP_Full_Factory_Flow(UCHAR* otp_pack_file)
{
    HRESULT lRes  = S_OK;
    FILE *pFile = NULL;
    unsigned int uImageSize = 0;
    unsigned char *pbImage = NULL;
    size_t uReadSize = 0;

    if (mv_drm_firstboot_check_keyinjection() != S_OK) {
        printf("Error! Key injection is already done on this board\n");
        return PROVISION_ERROR_RUN_TWICE;
    }
    pFile = fopen((const char *)otp_pack_file, "rb");
    if (pFile == NULL) {
        printf("Failed to open %s\n", otp_pack_file);
        return PROVISION_ERROR_OPENFILE;
    }
    fseek(pFile, 0, SEEK_END);
    uImageSize = (unsigned int)ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    pbImage = (unsigned char *)malloc(uImageSize);
    if (pbImage == NULL) {
        printf("Failed to malloc the memory\n");
        return PROVISION_ERROR_MEMORY;
    }
    uReadSize = fread(pbImage, 1, uImageSize, pFile);
    if (uReadSize != uImageSize) {
        printf("Failed to read init image from the file\n");
        return PROVISION_ERROR_READFILE;
    }

    if (mv_drm_firstboot_check_initimage(pbImage, uImageSize) != S_OK) {
        printf("Integrity check of input file failed\n");
        return PROVISION_ERROR_CHECK_IMAGE;
    }

    if (mv_drm_firstboot_write_appkeys(pbImage, uImageSize) != S_OK) {
        printf("Failed to write application keys\n");
        return PROVISION_ERROR_WRITE_APP_KEYS;
    }

    if (mv_drm_firstboot_check_appkeys(pbImage, uImageSize) != S_OK) {
        printf("Failed to check application keys\n");
        return PROVISION_ERROR_CHECK_APPKEYS;
    }

    //write nand
    if (mv_drm_load_secure_boot_keys(pbImage, uImageSize) != S_OK) {
        printf("Cannot read secure boot key stores\n");
        return PROVISION_ERROR_LOAD_SECURE_BOOT_KEYS;
    }

    if (mv_drm_firstboot_load_production_preboot() != S_OK) {
        printf("Unable to load production.subimg\n");
        return PROVISION_ERROR_READ_PROD_BCM;
    }

    if (mv_drm_firstboot_update_preboot() != S_OK) {
        printf("Unable to update preboot\n");
        return PROVISION_ERROR_UPDATE_PREBOOT;
    }

    if (mv_drm_firstboot_update_key1_key2() != S_OK) {
        printf("Unable to update key1st and key2nd\n");
        return PROVISION_ERROR_UPDATE_KEYS;
    }

    //write otp
    if (mv_drm_load_otp_layout(pbImage, uImageSize) != S_OK) {
        printf("Cannot read OTP layout\n");
        return PROVISION_ERROR_READ_OTP;
    }
    if (MV_OTP_BCM_Layout_Program((UCHAR *)"/factory_setting/otp_pack.bin") != S_OK) {
        printf("Cannot program OTP!\n");
        return PROVISION_ERROR_OTP;
    }

    mv_drm_firstboot_init_finish();

    return lRes;
}

int main(int argc, char *argv[])
{
    int hr = 0;
    char *pack_path = NULL;
    FILE *pFile = NULL;
    unsigned int pack_len = 0;
#ifdef CONFIG_TZK
    TEEC_Result result = TEEC_ERROR_ITEM_NOT_FOUND;
    TEEC_Context context;
    char *ta_path = NULL;
    char ta_file_path[128] = {0};
#endif

    if (argc == 2) {
        pack_path = argv[1];
    } else {
        print_usage(argv[0]);
        return -1;
    }

#ifdef CONFIG_TZK
    result = TEEC_InitializeContext(NULL, &context);
    if (result == TEEC_SUCCESS) {
        snprintf(ta_file_path, 128, "%s/%s", FACTORY_TA_PATH, FACTORY_TA_NAME);
        result = TEEC_LoadTA(&context, ta_file_path);
        if (result == TEEC_ERROR_ACCESS_CONFLICT) {
            printf("%s: [%s] TA has already been loaded\n", __FUNCTION__, ta_file_path);
            result = TEEC_SUCCESS;
        } else if (result != TEEC_SUCCESS) {
            printf("%s: Failed to load the TA [%s]: %x\n", __FUNCTION__, ta_file_path, result);
            TEEC_FinalizeContext(&context);
            return -2;
        }
    } else {
        printf("Failed to initialize tee\n");
        return -3;
    }
#endif

    pFile = fopen((const char*)pack_path, "rb");
    if (pFile == NULL) {
        printf("Error! Failed to open: %s\n", pack_path);
        return -4;
    }
    fseek(pFile, 0, SEEK_END);
    pack_len = ftell(pFile);
    fclose(pFile);

    if (pack_len == EVT_PACKAGE_SIZE) {
        printf("EVT factory flow\n");
        hr = MV_OTP_BCM_Layout_Program((unsigned char*) pack_path);
    } else {
        printf("Full factory flow\n");
        hr = MV_OTP_Full_Factory_Flow((unsigned char*) pack_path);
    }
    if (hr) {
        printf("%s,%d fail to invoke progam :%#.8x\n", __FUNCTION__, __LINE__, hr);
        return -5;
    } else {
            printf("Success to program otp package\n");
    }

    return 0;
}

