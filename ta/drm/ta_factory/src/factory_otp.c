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
//! \file    factory_otp.c
//! \brief   Factory TA OTP implementation
//! \author  Frank Ho
//! \date    Dec 2017
////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#ifndef CONFIG_OPTEE
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#endif

#ifdef CONFIG_OPTEE
#include "tee_internal_api_extensions.h"
#include "tee_api_syna.h"
#include "trace.h"
#else
#include "tee_common.h"
#include "tee_internal_common.h"
#include "tee_internal_core_common.h"
#endif

#include "tee_internal_api.h"
#include "factory_ta.h"
#include "factory_def.h"
#include "factory_otp.h"
#include "drm_tee_memory.h"
#include "seccrypto_bcm.h"
#include "assert.h"

#define DRM_CAS4_OTP_TAG_NDS_ID 0

#ifdef SYNAPTICS_TEST_OTP_DUMP_RSA
static UINT8 otp_dump_rsa_modulus[256] = {
0xd9,0x82,0x6e,0x60,0x2c,0x83,0x2b,0xb5,0x79,0x40,0x00,0xbb,0x27,0x8f,0x7d,0xe5,
0x26,0x4c,0x56,0xc7,0xb6,0xb3,0x1b,0x47,0x98,0x74,0x7d,0x58,0x74,0xdf,0xb0,0x15,
0xd4,0xfb,0x38,0x9a,0x1d,0x9d,0x1a,0x5a,0x1b,0xa4,0x0d,0xb4,0x1e,0xe6,0x68,0x67,
0xc1,0x4e,0x5e,0x4b,0x4b,0x0c,0xc2,0xfb,0x13,0x27,0x3c,0x7d,0x9f,0xad,0x34,0x5a,
0x2f,0xd2,0xd2,0xca,0x87,0x28,0xd9,0xaf,0x5b,0xd8,0xea,0x18,0xc2,0x6d,0x71,0x2f,
0x54,0x70,0x90,0x07,0x35,0xfa,0x63,0x0c,0xdd,0x58,0x43,0x3a,0x6d,0x65,0x8e,0x51,
0xe7,0xe4,0x60,0x42,0xa2,0x5f,0x1d,0x89,0xe6,0x5a,0xf0,0xf0,0xcf,0x05,0xa3,0x4a,
0xa5,0xd7,0x10,0xc0,0x95,0x98,0xdb,0xe5,0xa0,0x90,0x88,0x85,0xc7,0xe7,0x66,0xef,
0x71,0x58,0xd4,0x37,0x52,0x03,0xf6,0x55,0x74,0x34,0x15,0x85,0x1d,0x00,0x7d,0x44,
0xb8,0xe1,0x45,0x62,0x0a,0xbd,0xad,0x95,0xd3,0x56,0x4e,0xde,0x53,0x25,0x8a,0x98,
0x67,0xf4,0x2a,0xbe,0x4c,0xf2,0x85,0xa4,0x37,0x0c,0x04,0x20,0x99,0xae,0x7a,0x65,
0xa4,0x59,0x70,0x63,0x6a,0x97,0x2b,0x7d,0x6b,0x91,0xd1,0x74,0x9f,0x28,0x1c,0x13,
0xa0,0xca,0xd2,0xde,0x51,0x6f,0xfd,0xde,0x08,0x2e,0x8e,0xf7,0xe8,0x40,0x31,0x14,
0xd7,0x4b,0xf9,0xb1,0xb2,0xf3,0xd7,0xd8,0xae,0x88,0x5d,0x8c,0x0c,0x3e,0xcb,0x0b,
0x3a,0x9d,0x30,0xa1,0x8b,0x57,0xd3,0x2d,0x3c,0x3c,0x23,0xd3,0xd4,0xd3,0x8d,0x6d,
0x45,0xca,0x20,0x31,0xdc,0x72,0x52,0xca,0x0b,0x9e,0xe5,0xc2,0x21,0x4f,0x86,0xf5
};
#else
static UINT8 otp_dump_rsa_modulus[256] = {
0xae,0xa9,0xe0,0x14,0x2d,0xf2,0xb5,0xe7,0x42,0xd4,0x23,0x15,0x43,0x70,0x54,0xb1,
0xd6,0x90,0x29,0xd8,0xcd,0x32,0x15,0x34,0x38,0x27,0x06,0xcd,0x8a,0xe5,0xb0,0x37,
0x29,0x11,0x8c,0xe2,0x28,0x5d,0x11,0xfb,0x01,0x68,0xb8,0x89,0xcd,0x1e,0xe3,0x1f,
0x30,0xff,0x16,0x6f,0xbd,0x4e,0x9f,0x13,0x4f,0x0b,0x75,0xac,0x97,0x43,0x65,0x83,
0x8a,0xec,0x45,0x5d,0x26,0x74,0x8c,0x85,0x13,0x90,0x18,0x20,0x5d,0x27,0x30,0x35,
0x2e,0x76,0x8c,0xa9,0xcd,0xbd,0x8c,0x71,0x3c,0xb5,0xc5,0x74,0xca,0x1b,0xc0,0x0a,
0x50,0x2f,0x8e,0xcf,0x02,0xcb,0x75,0xd5,0x5d,0xc9,0x51,0xc8,0x6b,0x71,0x0c,0x3a,
0x72,0x51,0x27,0x7f,0x64,0x9e,0x17,0xe3,0x42,0x5d,0xf2,0x80,0xd3,0x00,0xcc,0xe8,
0x29,0x71,0xff,0xfe,0xf2,0x96,0xc6,0xc2,0x0c,0x22,0x05,0x8c,0xa3,0x74,0x7d,0xd5,
0xd2,0x18,0xb9,0x18,0xb5,0x79,0xc0,0xe1,0x0c,0xf3,0x05,0x91,0x24,0x1a,0x50,0x76,
0x7a,0x00,0x01,0x4c,0x7d,0x19,0x2e,0xcf,0x94,0x47,0x79,0x58,0x6a,0xd7,0x75,0xd9,
0x4f,0x61,0x03,0xdc,0x88,0xa3,0x32,0xa7,0xad,0x82,0x48,0xa9,0xb3,0xdb,0xcb,0x55,
0xef,0xe6,0x60,0xf6,0xf3,0xdd,0x31,0x25,0x6d,0x9d,0x41,0x43,0xfe,0x91,0xf6,0x16,
0xc8,0x10,0x29,0x95,0x35,0xed,0xf5,0x99,0x90,0xf7,0x1b,0x82,0x77,0xc4,0xb1,0x65,
0xa0,0x2b,0xca,0x69,0x4c,0xe7,0x20,0x9d,0x0a,0x6d,0xc6,0xbd,0x62,0xd2,0xd9,0x11,
0xe2,0x92,0x3d,0x13,0x89,0x72,0x41,0xbc,0xc5,0x5c,0xe6,0xcf,0x5f,0xe0,0x53,0x0d
};
#endif

static UINT8 otp_dump_rsa_exponent[256] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01
};

typedef struct {
    UINT32 BitMap;
    UINT8  RkekId[8];
    UINT8  RetryMap[8];
    UINT8  FailMap[8];
} otp_srv_resp;

/*
 * Burn OTP layout
 *
 * Arguments:
 * - pbKeyBuffer     (IN) - Buffer for OTP layout
 * - uKeyLength      (IN) - length of the OTP layout
 *
 * Return:
 *         STATUS_SUCCESS or one of the BCM error code
 */
static HRESULT MV_DRMLIB_OTP_BurnLayout(UINT8 *pbLayout, UINT32 uLayoutLen)
{
    HRESULT lRes = S_OK;
    TEE_Result ret = TEE_SUCCESS;
    UINT8 *pbLayoutSHM = NULL;
    otp_srv_resp *pbRespSHM = NULL;
    DRM_TEE_SHM_HANDLE pbLayoutshmhandle = INVALID_SHM_HANDLE;
    DRM_TEE_SHM_HANDLE pbRespshmhandle = INVALID_SHM_HANDLE;

    /* prepare layout */
    pbLayoutSHM = DRM_TEE_SHM_Malloc(uLayoutLen, &pbLayoutshmhandle);
    if (NULL == pbLayoutSHM)
    {
        FACTORY_ERR("Out of memory\n");
        lRes = E_OUTOFMEMORY;
        return lRes;
    }
    memset(pbLayoutSHM, 0, uLayoutLen);
    memcpy(pbLayoutSHM, pbLayout, uLayoutLen);
    ret = TEE_FlushCache(pbLayoutSHM, uLayoutLen);
    if (TEE_SUCCESS != ret)
    {
        FACTORY_ERR("Failed to flush cache\n");
        lRes = E_FAIL;
        goto Func_Ret;
    }

    /* prepare response buffer */
    pbRespSHM = (otp_srv_resp *)DRM_TEE_SHM_Malloc(sizeof(otp_srv_resp), &pbRespshmhandle);
    if (NULL == pbRespSHM)
    {
        FACTORY_ERR("Out of memory\n");
        lRes = E_OUTOFMEMORY;
        return lRes;
    }
    memset(pbRespSHM, 0, sizeof(otp_srv_resp));
    ret = TEE_FlushCache(pbRespSHM, sizeof(otp_srv_resp));
    if (TEE_SUCCESS != ret)
    {
        FACTORY_ERR("Failed to flush cache\n");
        lRes = E_FAIL;
        goto Func_Ret;
    }

    /* Send BCM command */
    lRes = BCM_OTP_BURNLAYOUT(pbLayoutSHM, uLayoutLen, (UINT8*)pbRespSHM, sizeof(otp_srv_resp));
    if (S_OK != lRes)
    {
        FACTORY_ERR("Failed to call BCM command\n");
        FACTORY_ERR("bitMap = 0x%x\n", pbRespSHM->BitMap);
        #ifdef CONFIG_OPTEE
        DMSG("BCM cmd return = 0x%x\n", lRes);
        #endif
        goto Func_Ret;
    }

Func_Ret:
    DRM_TEE_SHM_Free(pbLayoutshmhandle);
    DRM_TEE_SHM_Free(pbRespshmhandle);
    return lRes;
}

static TEE_Result MV_OTP_BurnLayout(UINT8* pbLayout, UINT32 uLayoutSize)
{
    TEE_Result result = TEE_SUCCESS;
    HRESULT lRes = S_OK;

    lRes = MV_DRMLIB_OTP_BurnLayout(pbLayout, uLayoutSize);
    if (S_OK != lRes)
    {
        result = TEE_ERROR_GENERIC;
    }
    else
    {
        result = TEE_SUCCESS;
    }

    return result;
}

static TEE_Result Drm_OTPBurnLayout(TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;
    uint8_t  *pbLayout = NULL;
    uint32_t uLayoutSize= 0;

    pbLayout = (uint8_t *)params[0].memref.buffer;
    uLayoutSize = params[0].memref.size;

    if (TEE_SUCCESS != drm_mem_check_nosecure(pbLayout, uLayoutSize))
    {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    result = MV_OTP_BurnLayout(pbLayout, uLayoutSize);

    return result;
}

static TEE_Result Drm_OTPReadRKEKID(TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;
    UINT8 *buffer = NULL;
    UINT32 size = 0;
    UINT8 rkekid[8] = {0};

    buffer = (UINT8*)params[0].memref.buffer;
    size = params[0].memref.size;

    result = TEE_GetOtpRawData(46, rkekid);

#if 0 //print RKEK_ID
    printf("RKEKID: %02x%02x%02x%02x %02x%02x%02x%02x\n", rkekid[0], rkekid[1], rkekid[2], rkekid[3], \
                                                          rkekid[4], rkekid[5], rkekid[6], rkekid[7]);
#else // return RKEK_ID to CA
    memcpy(buffer, rkekid, size);
#endif

    return result;
}


static TEE_Result Drm_OTPReadJtag(TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    HRESULT lRes;
    UINT8 *buffer = NULL;
    UINT8 size = 0;
    UINT8 *pbSHMBuf = NULL;
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE;

    buffer = (UINT8*)params[0].memref.buffer;
    size = params[0].memref.size;

    pbSHMBuf = DRM_TEE_SHM_Malloc(size, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }

    memset(pbSHMBuf, 0, size);
    if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, size)) {
        FACTORY_ERR("Failed to flush cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    lRes = BCM_READ_OTP_JTAG(pbSHMBuf);
    if (S_OK != lRes) {
        FACTORY_ERR("Failed to call BCM command (error %X)\n", lRes);
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    if (TEE_InvalidateCache(pbSHMBuf, size) != TEE_SUCCESS) {
        FACTORY_ERR("Failed to invalidate cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    memcpy(buffer, pbSHMBuf, size);
    result = TEE_SUCCESS;

Func_Ret:
    if (INVALID_SHM_HANDLE != pbshmhandle) {
        DRM_TEE_SHM_Free(pbshmhandle);
    }

    return result;
}

static TEE_Result Drm_BCMReadOTP(UINT32 otpIndex, UINT8 *pbBuf, UINT32 uBufLen)
{
    TEE_Result result = TEE_ERROR_BAD_STATE;
    HRESULT lRes;
    UINT8 *pbSHMBuf = NULL;
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE;

    /* prepare  */
    pbSHMBuf = DRM_TEE_SHM_Malloc(uBufLen, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }

    memset(pbSHMBuf, 0, uBufLen);
    if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, uBufLen)) {
        FACTORY_ERR("Failed to flush cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    /* Send BCM command */
    lRes = BCM_READ_PUB_OTP(otpIndex, pbSHMBuf, uBufLen);
    if (S_OK != lRes) {
        FACTORY_ERR("Failed to call BCM command (error %X)\n", lRes);
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    if (TEE_InvalidateCache(pbSHMBuf, uBufLen) != TEE_SUCCESS) {
        FACTORY_ERR("Failed to invalidate cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    memcpy(pbBuf, pbSHMBuf, uBufLen);
    result = TEE_SUCCESS;

Func_Ret:
    if (INVALID_SHM_HANDLE != pbshmhandle) {
        DRM_TEE_SHM_Free(pbshmhandle);
    }
    return result;
}

static TEE_Result Drm_OTPReadCas4PubId(TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;
    UINT8 *buffer = NULL;
    UINT32 size = 0;

    buffer = (UINT8*)params[0].memref.buffer;
    size = params[0].memref.size;

   result = Drm_BCMReadOTP(DRM_CAS4_OTP_TAG_NDS_ID, buffer, size);

#if 0 //print CAS4_ID
    FACTORY_ERR("NSK cas4Otp: %02x%02x%02x%02x %02x%02x%02x%02x\n",
            buffer[0], buffer[1], buffer[2], buffer[3], \
            buffer[4], buffer[5], buffer[6], buffer[7]);
#endif

    return result;
}

static TEE_Result Drm_OTPReadSOC_UID(TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    HRESULT lRes;
    UINT8 *buffer = NULL;
    UINT32 size = 0;
    UINT32 SHMSize = 0;
    UINT8 *pbSHMBuf = NULL;
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE;
    int socUID_size = 8;

    buffer = (UINT8*)params[0].memref.buffer;
    size = params[0].memref.size;

    if (size != socUID_size) {
        FACTORY_ERR("Buffer size is not %d\n", socUID_size);
        result = TEE_ERROR_BAD_PARAMETERS;
        goto Func_Ret;
    }

    SHMSize = socUID_size;

    pbSHMBuf = DRM_TEE_SHM_Malloc(SHMSize, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }

    memset(pbSHMBuf, 0, SHMSize);
    if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, size)) {
        FACTORY_ERR("Failed to flush cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    lRes = BCM_READ_SOC_UID(pbSHMBuf, SHMSize);
    if (S_OK != lRes) {
        FACTORY_ERR("Failed to call BCM command (error %X)\n", lRes);
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    if (TEE_InvalidateCache(pbSHMBuf, SHMSize) != TEE_SUCCESS) {
        FACTORY_ERR("Failed to invalidate cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    memcpy(buffer, pbSHMBuf, 256);
#if 0
    printf("SOC_UID: %02x%02x%02x%02x %02x%02x%02x%02x\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
#endif
    result = TEE_SUCCESS;

Func_Ret:
    if (INVALID_SHM_HANDLE != pbshmhandle) {
        DRM_TEE_SHM_Free(pbshmhandle);
    }

    return result;

}

static TEE_Result Drm_OTPReadPublicItems(TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    HRESULT lRes;
    UINT8 *buffer = NULL;
    UINT32 size = 0;
    UINT32 SHMSize = 0;
    UINT8 *pbSHMBuf = NULL;
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE;
    PUBLIC_OTP_ITEMS pubOtp;
    UINT8 readByte = 0;
#ifndef CONFIG_OPTEE
    UINT8 clrBuffer[256];
    UINT8 encBuffer[256];
    RSA *rsa;
    UINT32 rsa_ret;
#endif

    buffer = (UINT8*)params[0].memref.buffer;
    size = params[0].memref.size;

    if (size != 256) {
        FACTORY_ERR("Buffer size is not 256\n");
        result = TEE_ERROR_BAD_PARAMETERS;
        goto Func_Ret;
    }

    SHMSize = PUBLIC_OTP_SIZE;
    pbSHMBuf = DRM_TEE_SHM_Malloc(SHMSize, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }

    memset(pbSHMBuf, 0, SHMSize);
    if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, size)) {
        FACTORY_ERR("Failed to flush cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    lRes = BCM_READ_PUBLIC_OTP_ITEMS(pbSHMBuf, SHMSize);
    if (S_OK != lRes) {
        FACTORY_ERR("Failed to call BCM command (error %X)\n", lRes);
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    if (TEE_InvalidateCache(pbSHMBuf, SHMSize) != TEE_SUCCESS) {
        FACTORY_ERR("Failed to invalidate cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    // RKEK_ID, pbSHMBuf[0]
    memcpy((unsigned char *)pubOtp.RKEKID, pbSHMBuf, 8);

    //parse and update pbSHMBuf to pubOtp, basing on Dolphin_OTP_Layout-v062.xlsx
    //Gen2 row 64~66, pbSHMBuf[8]
    memcpy((UINT8*)pubOtp.TA_Vendor_SegID, pbSHMBuf + 8, 24);

    //Gen2 row 74, pbSHMBuf[32]
    memcpy((UINT8*)pubOtp.TA_Vendor_GID, pbSHMBuf + 32, 8);

    //Gen2 row 96 byte 2, pbSHMBuf[40 + 2]
    memcpy((UINT8*)&pubOtp.Security_Indicators, pbSHMBuf + 40 + 2, 1);

    //Gen3 row 7 byte 4~7, pbSHMBuf[56]
    memcpy((UINT8*)&pubOtp.BOOT_SegID, pbSHMBuf + 56 + 4, 4);

    //Gen3 row 8 byte 4 bit 4~7, pbSHMBuf[64]
    readByte = (*(UINT8*)(pbSHMBuf + 64 + 4) & 0xf0) >> 4;
    memcpy((UINT8*)&pubOtp.Lock_Boot_SegID, &readByte, 1);

    //Gen3 row 8 byte 7 bit 2, pbSHMBuf[64]
    readByte = (*(UINT8*)(pbSHMBuf + 64 + 7) & 0x04) >> 2;
    memcpy((UINT8*)&pubOtp.MP_Provision_Done, &readByte, 1);

    //Gen3 row 9 byte 4 bit 3, pbSHMBuf[72]
    readByte = (*(UINT8*)(pbSHMBuf + 72 + 4) & 0x08) >> 3;
    memcpy((UINT8*)&pubOtp.REE_Security_Enable, &readByte, 1);

    //Gen3 row 9 byte 4 bit 2, pbSHMBuf[72]
    readByte = (*(UINT8*)(pbSHMBuf + 72 + 4) & 0x04) >> 2;
    memcpy((UINT8*)&pubOtp.TEE_Segmentation_Version_Enable, &readByte, 1);

    //Gen3 row 9 byte 7 bit 0, pbSHMBuf[72]
    readByte = *(UINT8*)(pbSHMBuf + 72 + 7) & 0x01;
    memcpy((UINT8*)&pubOtp.Boot_Security_Enable, &readByte, 1);

    //Gen3 row 13 byte 4~7, pbSHMBuf[80]
    memcpy((UINT8*)&pubOtp.TEE_SegID, pbSHMBuf + 80 + 4, 4);

    //Gen3 row 27 byte 0 bit 6~7, pbSHMBuf[88]
    readByte = (*(UINT8*)(pbSHMBuf + 88) & 0xC0) >> 6;
    memcpy((UINT8*)&pubOtp.SCS_Total_Area_Size_Sel, &readByte, 1);

    //Gen3 row 27 byte 1 bit 2, pbSHMBuf[88 + 1]
    readByte = (*(UINT8*)(pbSHMBuf + 88 + 1) & 0x04) >> 2;
    memcpy((UINT8*)&pubOtp.Background_Check_Enable, &readByte, 1);

    //Gen3 row 29 byte 2 bit 4, pbSHMBuf[96 + 2]
    readByte = (*(UINT8*)(pbSHMBuf + 96 + 2) & 0x10) >> 4;
    memcpy((UINT8*)&pubOtp.JtagProg_Disable, &readByte, 1);

    //Gen3 row 29 byte 4~7, pbSHMBuf[96 + 4]
    memcpy((UINT8*)&pubOtp.REE_SegID, pbSHMBuf + 96 + 4, 4);

    //CAS4 row 19 byte 0~3, pbSHMBuf[104]
    memcpy((UINT8*)&pubOtp.ConcurrencyControl, pbSHMBuf + 104, 4);

    //CAS4 row 21 byte 2 bit 2~7, pbSHMBuf[112 + 2]
    readByte = (*(UINT8*)(pbSHMBuf + 112 + 2) & 0xfc) >> 2;
    memcpy((UINT8*)&pubOtp.v_PubOtpMinConfVer, &readByte, 1);

    //CAS4 row 21 byte 3 bit 4~7, pbSHMBuf[112 + 3]
    readByte = (*(UINT8*)(pbSHMBuf + 112 + 3) & 0xf0) >> 4;
    memcpy((UINT8*)&pubOtp.v_PubOtpRsaIndex, &readByte, 1);

    //CAS4 row 21 byte 4~5, pbSHMBuf[112 + 4]
    memcpy((UINT8*)&pubOtp.v_PubOtpVID, pbSHMBuf + 112 + 4, 2);

    //CAS4 row 21 byte 6~7, pbSHMBuf[112 + 6]
    memcpy((UINT8*)&pubOtp.v_PubOtpOID, pbSHMBuf + 112 + 6, 2);

    //CAS4 row 26 byte 0 bit 4, pbSHMBuf[120]
    readByte = (*(UINT8*)(pbSHMBuf + 120) & 0x10) >> 4;
    memcpy((UINT8*)&pubOtp.OtpWrtPwdProt, &readByte, 1);

    //CAS4 row 26 byte 1 bit 0~1, pbSHMBuf[120 + 1]
    readByte = *(UINT8*)(pbSHMBuf + 120 + 1) & 0x03;
    memcpy((UINT8*)&pubOtp.JTAG_Protection, &readByte, 1);

    //CAS4 row 26 byte 1 bit 2~3, pbshmbuf[120 + 1]
    readByte = (*(UINT8*)(pbSHMBuf + 120 + 1) & 0x0c) >> 2;
    memcpy((UINT8*)&pubOtp.JTAG_Protection_dup, &readByte, 1);

    //CAS4 row 26 byte 2 bit 0~1, pbSHMBuf[120 + 2]
    readByte = *(UINT8*)(pbSHMBuf + 120 + 2) & 0x03;
    memcpy((UINT8*)&pubOtp.BSCAN_Protection, &readByte, 1);

    //CAS4 row 26 byte 2 bit 2~3, pbshmbuf[120 + 2]
    readByte = (*(UINT8*)(pbSHMBuf+ 120 + 2) & 0x0c) >> 2;
    memcpy((UINT8*)&pubOtp.BSCAN_Protection_dup, &readByte, 1);

    //CAS4 row 26 byte 3 bit 0~1, pbSHMBuf[120 + 3]
    readByte = *(UINT8*)(pbSHMBuf + 120 + 3) & 0x03;
    memcpy((UINT8*)&pubOtp.STEST_Protection, &readByte, 1);

    //CAS4 row 26 byte 3 bit 2~3, pbshmbuf[120 + 2]
    readByte = (*(UINT8*)(pbSHMBuf+ 120 + 3) & 0x0c) >> 2;
    memcpy((UINT8*)&pubOtp.STEST_Protection_dup, &readByte, 1);

    // K0_REE_HASH, pbSHMBuf[128] - pbSHMBuf[159]
    memcpy(&pubOtp.K0_REE_HASH, pbSHMBuf + 128, 8 * sizeof(unsigned int));

    // REE_VERSION, pbSHMBuf[160]
    memcpy(&pubOtp.REE_VERSION, pbSHMBuf + 160, sizeof(unsigned int));

    // BOOT_VERSION, pbSHMBuf[164]
    memcpy(&pubOtp.BOOT_VERSION, pbSHMBuf + 164, sizeof(unsigned int));

    // JTAGProtectionLevel, pbSHMBuf[168]
    memcpy(&pubOtp.JTAGProtectionLevel, pbSHMBuf + 168, sizeof(unsigned int));

#ifndef CONFIG_OPTEE
    //use RSA pub to encrypt otp dump data
    memset(clrBuffer, 0, 256);
    memset(encBuffer, 0, 256);
    memcpy(clrBuffer, &pubOtp, sizeof(PUBLIC_OTP_ITEMS));

    rsa = RSA_new();
    if (rsa == NULL) {
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

#ifdef OPENSSL_111Q
    BIGNUM *n = BN_bin2bn(otp_dump_rsa_modulus, 256, NULL);
    BIGNUM *e = BN_bin2bn(otp_dump_rsa_exponent, 256, NULL);
    rsa_ret = RSA_set0_key(rsa, n, e, NULL);
    if (rsa_ret != 1) {
        FACTORY_ERR("RSA_set0_key failed(%d)\n", rsa_ret);
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }
#else
    rsa->n = BN_bin2bn(otp_dump_rsa_modulus, 256, NULL);
    rsa->e = BN_bin2bn(otp_dump_rsa_exponent, 256, NULL);
#endif
    rsa_ret = RSA_public_encrypt(256, clrBuffer, encBuffer, rsa, RSA_NO_PADDING);
    if (rsa_ret != 256) {
        FACTORY_ERR("RSA_public_encrypt failed (%s)\n", ERR_error_string(ERR_get_error(), NULL));
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    memcpy(buffer, encBuffer, 256);
#else
    memcpy(buffer, &pubOtp, 256);
#endif
    result = TEE_SUCCESS;

Func_Ret:
    if (INVALID_SHM_HANDLE != pbshmhandle) {
        DRM_TEE_SHM_Free(pbshmhandle);
    }

    return result;

}

static TEE_Result Drm_OTPReadChipRev(TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    UINT8 *buffer = NULL;
    UINT32 size = 0;
    UINT8 otp_buf[8] = {0};
    UINT8 rev = 0;

    buffer = (UINT8*)params[0].memref.buffer;
    size = params[0].memref.size;
    if (size != 1)
        return TEE_ERROR_EXCESS_DATA;

    result = TEE_GetOtpRawData(5, otp_buf);
    rev = (otp_buf[5] >> 5) & 0x03;
    memcpy(buffer, &rev, size);

    return result;
}

static TEE_Result Drm_OTPWriteRKEKID(TEE_Param params[4])
{
    HRESULT lRes = E_FAIL;
    TEE_Result result = TEE_ERROR_GENERIC;
    UINT8 *buffer = NULL;
    UINT32 size = 0;
    UINT8 *pbSHMBuf = NULL;
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE;

    buffer = (uint8_t *)params[0].memref.buffer;
    size = params[0].memref.size;

    pbSHMBuf = DRM_TEE_SHM_Malloc(size, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }
    memset(pbSHMBuf, 0, size);
    memcpy(pbSHMBuf, buffer, size);
    result = TEE_FlushCache(pbSHMBuf, size);
    if (TEE_SUCCESS != result)
    {
        FACTORY_ERR("Failed to flush cache\n");
        lRes = E_FAIL;
        goto Func_Ret;
    }

    /* Send BCM command */
    lRes = BCM_WRITE_RKEKID(pbSHMBuf, size);
    if (S_OK != lRes)
    {
        FACTORY_ERR("Failed to call BCM command\n");
        goto Func_Ret;
    }

Func_Ret:
    DRM_TEE_SHM_Free(pbshmhandle);
    return lRes;
}

static TEE_Result Drm_OTPReadOTP(TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    UINT32 size = 2 * sizeof(UINT32);
    UINT32 type = 0;
    UINT32 *pdata = NULL; //pointer to data in shared memory
    UINT32 *pmask = NULL; //pointer to mask in shared memory
    UINT8 *pbSHMBuf = NULL;
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE;

    type = params[0].value.a;

    pbSHMBuf = DRM_TEE_SHM_Malloc(size, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }

    memset(pbSHMBuf, 0, size);
    if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, size)) {
        FACTORY_ERR("Failed to flush cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    pdata = (UINT32 *)pbSHMBuf;
    pmask = (UINT32 *)(pbSHMBuf + sizeof(UINT32));

    result = BCM_READ_OTP(type, pdata, pmask);

    if (TEE_InvalidateCache(pbSHMBuf, size) != TEE_SUCCESS) {
        FACTORY_ERR("Failed to invalidate cache\n");
        result = TEE_ERROR_GENERIC;
    }

    params[1].value.a = *pdata;
    params[1].value.b = 0;      /* not used */
    params[2].value.a = *pmask;
    params[2].value.b = 0;      /* not used */

    //DMSG("Drm_OTPReadOTP: type=0x%08X, data=0x%08X, mask=0x%08X\n", type, *pdata, *pmask);

Func_Ret:
    DRM_TEE_SHM_Free(pbshmhandle);

    return result;
}

static TEE_Result Drm_OTPWriteOTP(TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    HRESULT lRes = E_FAIL;
    UINT32 type = 0;
    UINT32 data = 0;
    UINT32 mask = 0;

    type = params[0].value.a;
    data = params[1].value.a;
    mask = params[2].value.a;

    //DMSG("Drm_OTPWriteOTP: type=0x%08X, data=0x%08X, mask=0x%08X\n", type, data, mask);

    lRes = BCM_WRITE_OTP(type, data, mask);
    if (S_OK != lRes) {
        FACTORY_ERR("Failed to call BCM command (error %X)\n", lRes);
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    } else {
        result = TEE_SUCCESS;
    }

Func_Ret:
    return result;
}


TEE_Result MV_DRM_OTP_InvokeCommandEntryPoint(uint32_t commandID,
                                              TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;

    /* This dispatcher can only handle OTP related functions */
    assert(OTP_MASK == (commandID & OTP_MASK));

    switch (commandID) {
    case DRM_SE_OTP_BURNLAYOUT:
        result = Drm_OTPBurnLayout(params);
        break;
    case DRM_SE_OTP_READ_RKEKID:
        result = Drm_OTPReadRKEKID(params);
        break;
    case DRM_SE_OTP_READ_AUTO_JTAG:
        result = Drm_OTPReadJtag(params);
        break;
    case DRM_SE_OTP_READ_CAS4_NDSID:
        result = Drm_OTPReadCas4PubId(params);
        break;
    case DRM_SE_OTP_READ_PUBLIC_ITEMS:
        result = Drm_OTPReadPublicItems(params);
        break;
    case DRM_SE_OTP_READ_SOC_UID:
        result = Drm_OTPReadSOC_UID(params);
        break;
    case DRM_SE_OTP_READ_CHIP_REV:
        result = Drm_OTPReadChipRev(params);
        break;
    case DRM_SE_OTP_WRITE_RKEKID:
        result = Drm_OTPWriteRKEKID(params);
        break;
    case DRM_SE_OTP_READ_OTP:
        result = Drm_OTPReadOTP(params);
        break;
    case DRM_SE_OTP_WRITE_OTP:
        result = Drm_OTPWriteOTP(params);
        break;
    default:
        break;
    }

    return result;
}
