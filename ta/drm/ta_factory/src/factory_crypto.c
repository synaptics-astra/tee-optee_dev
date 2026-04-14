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
//! \file    factory_crypto.c
//! \brief   Factory CA CRYPTO implementations
//! \author  Rohit Tayal
//! \date    Mar 2018
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef CONFIG_OPTEE
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/asn1.h>
#include <openssl/pem.h>
#endif

#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#else
#include "tee_common.h"
#include "tee_internal_common.h"
#include "tee_internal_core_common.h"
#endif

#include "tee_internal_api.h"
#include "factory_ta.h"
#include "factory_def.h"
#include "drm_tee_memory.h"
#include "seccrypto_bcm.h"
#include "assert.h"
#include "factory_crypto.h"

#ifndef CONFIG_OPTEE

extern TEE_Result Bcm_Hal_SignKey(const uint8_t *input_data, uint32_t input_data_length,
                                  uint8_t *keyBuff, uint32_t keyBuffLen, uint8_t *response);

TEE_Result verifySignedMessage(const uint8_t *input_data, uint32_t input_data_length,
                                    uint8_t *cert, uint32_t certLen, uint8_t *signedMsg)
{
    unsigned char buffer[256] = {0};
    EVP_PKEY *pkey = NULL;
    RSA *rsa = NULL;
    unsigned char hashed[32];
    int status = 0;
    TEE_Result result = TEE_SUCCESS;
    BIO     *certbio = NULL;
    X509    *X509_cert = NULL;

    if((NULL == input_data) || (NULL == signedMsg) || (NULL == cert)) {
        FACTORY_ERR("factory :Invalid input params\n");
        result =  TEE_ERROR_BAD_PARAMETERS;
    }

    //initialize openssl
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();

    certbio = BIO_new(BIO_s_mem());
    //Load the certificate from file (PEM).
    certbio = BIO_new_mem_buf((void*)cert, -1);

    X509_cert = PEM_read_bio_X509(certbio, NULL, NULL, NULL);
    if(NULL == X509_cert) {
        FACTORY_ERR("factory :PEM_read_bio_X509 fails to load certificate bio\n");
        result =  TEE_ERROR_GENERIC;
        goto cleanup;
    }

    /* Extract public key from the certificate in PEM format */
    if ((pkey = X509_get_pubkey(X509_cert)) == NULL) {
        FACTORY_ERR("factory : X509_get_pubkey fails to get public key\n");
        result =  TEE_ERROR_GENERIC;
        goto cleanup;
    }

    /* Calculate the SHA 256 of the input data */
    SHA256_CTX sha_ctx;
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, input_data, input_data_length);
    SHA256_Final(hashed, &sha_ctx);

    /* Decrypt the signature message */
    rsa = EVP_PKEY_get1_RSA(pkey);
    status = RSA_public_decrypt(RSA_size(rsa), signedMsg, buffer, rsa, RSA_NO_PADDING);
    if (status == -1) {
        FACTORY_ERR("factory :RSA_public_decrypt failed (%s)\n", ERR_error_string(ERR_get_error(), NULL));
        result = TEE_ERROR_SECURITY;
    }

    /* Verify the signature */
    if(STATUS_ERROR == RSA_verify_PKCS1_PSS(rsa, hashed, EVP_sha256(), buffer, -2)) {
        result = TEE_ERROR_SIGNATURE_INVALID;
        FACTORY_ERR("factory : RSA_verify_PKCS1_PSS fails\n");
    }

cleanup:
    if(NULL != pkey) {
        EVP_PKEY_free(pkey);
    }
    if(NULL != X509_cert) {
        X509_free(X509_cert);
    }
    if(NULL != certbio) {
       BIO_free_all(certbio);
    }
    return result;
}

TEE_Result Drm_Sign_Verify_Message(TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;
    uint8_t  *pKeyShm = NULL, *pCertShm = NULL;
    const uint8_t *pInputMsgShm = NULL;
    uint32_t uKeySize = 0,uCertSize = 0,uInputMsgSize = 0;
    uint8_t *pSignMsg = NULL;

    pInputMsgShm =  (uint8_t *)params[0].memref.buffer;
    uInputMsgSize = params[0].memref.size;
    pKeyShm =  (uint8_t *)params[1].memref.buffer;
    uKeySize = params[1].memref.size;
    pCertShm =  (uint8_t *)params[2].memref.buffer;
    uCertSize = params[2].memref.size;

    if(pInputMsgShm == NULL || pKeyShm == NULL || pCertShm == NULL) {
        FACTORY_ERR(" factory : Invalid Input Parameter \n");
        result =  TEE_ERROR_BAD_PARAMETERS;
    }

    pSignMsg = (uint8_t *) TEE_Malloc(PKCS_PSS_SHA_256_SIGN, TEE_MALLOC_FLAG_DEFAULT);

    if(NULL == pSignMsg) {
        FACTORY_ERR(" factory : %s:%d:TEE_Malloc Fails of size [256] \n",__func__,__LINE__);
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }

    /* Step 1 : sign the message using BCM PI */
    result = Bcm_Hal_SignKey(pInputMsgShm, uInputMsgSize,
                              pKeyShm, uKeySize,
                              pSignMsg);
    if(TEE_SUCCESS != result) {
        FACTORY_ERR("factory :%s:%d:Fails to Sign the Messages \n",__func__,__LINE__);
        result = TEE_ERROR_ACCESS_DENIED;
        goto cleanup;
    }

    /* Step 2 : Verify the Signed message using OpenSSL */
    result = verifySignedMessage(pInputMsgShm , uInputMsgSize, pCertShm, uCertSize, pSignMsg);

    if(TEE_SUCCESS != result) {
        FACTORY_ERR("factory :%s:%d:Fails to verify the signed Messages \n",__func__,__LINE__);
        result = TEE_ERROR_SIGNATURE_INVALID;
        goto cleanup;
    }

cleanup:
    if(NULL != pSignMsg) {
        TEE_Free(pSignMsg);
    }
    return result;
}

#endif

static TEE_Result Drm_GenerateSecureKey(TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    TEE_UUID ta_uuid = FACTORY_UUID;
    UINT8 APP_ID_KEY[16] = { 0x10, 0x00, 0x00, 0x02,  'A','P','P','K', 0,0,0,0,0,0,0,0 };
    UINT32 app_variant = 0;
    UINT8 secure_key[16] = {0};
    UINT8 *pbSHMBuf = NULL;
    UINT32 size = sizeof(ta_uuid) + sizeof(APP_ID_KEY) + sizeof(app_variant) + sizeof(secure_key);
    UINT32 pta_uuid_pysical_addr = 0;
    UINT32 papp_id_key_pysical_addr = 0;
    UINT32 psecure_key_pysical_addr = 0;
    UINT8 *pkey;
    UINT8 *pbuf = NULL;
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE;

    app_variant = params[0].value.a;

    pbSHMBuf = DRM_TEE_SHM_Malloc(size, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }
    memcpy(pbSHMBuf, &ta_uuid, sizeof(ta_uuid));
    memcpy(pbSHMBuf + sizeof(ta_uuid), APP_ID_KEY, sizeof(APP_ID_KEY));
    memcpy(pbSHMBuf + sizeof(ta_uuid) + sizeof(APP_ID_KEY), &app_variant, sizeof(app_variant));
    memcpy(pbSHMBuf + sizeof(ta_uuid) + sizeof(APP_ID_KEY) + sizeof(app_variant), secure_key, sizeof(secure_key));

    pta_uuid_pysical_addr = (UINT32)TEE_VirtToPhys(&pbSHMBuf[0]);
    papp_id_key_pysical_addr = (UINT32)TEE_VirtToPhys(&pbSHMBuf[sizeof(ta_uuid)]);
    psecure_key_pysical_addr = (UINT32)TEE_VirtToPhys(&pbSHMBuf[sizeof(ta_uuid) + sizeof(APP_ID_KEY) + sizeof(app_variant)]);
    pkey = &pbSHMBuf[sizeof(ta_uuid) + sizeof(APP_ID_KEY) + sizeof(app_variant)];

    if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, size)) {
        FACTORY_ERR("Failed to flush cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    result = BCM_GENERATE_SECUREKEY(pta_uuid_pysical_addr, papp_id_key_pysical_addr, app_variant, psecure_key_pysical_addr);
    if (result != TEE_SUCCESS) {
        FACTORY_ERR("Failed to call BCM command (error %lX)\n", result);
        goto Func_Ret;
    }

    if (TEE_InvalidateCache(pbSHMBuf, size) != TEE_SUCCESS) {
        FACTORY_ERR("Failed to invalidate cache\n");
        result = TEE_ERROR_GENERIC;
    }

#ifdef CONFIG_OPTEE
    DMSG("app_variant = %d", app_variant);
    DMSG("secure_key = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
         pkey[0], pkey[1], pkey[2], pkey[3],
         pkey[4], pkey[5], pkey[6], pkey[7],
         pkey[8], pkey[9], pkey[10], pkey[11],
         pkey[12], pkey[13], pkey[14], pkey[15]);
#endif

    // return secure_key to CA
    pbuf = (UINT8*)params[1].memref.buffer;
    memcpy(pbuf, pkey, params[1].memref.size);

Func_Ret:
    DRM_TEE_SHM_Free(pbshmhandle);

    return result;
}

TEE_Result Drm_GenerateSecureKey_V2(TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;
    TEE_UUID ta_uuid = FACTORY_UUID;
    UINT8 APP_ID_KEY[16] = { 0x10, 0x00, 0x00, 0x02,  'A','P','P','K', 0,0,0,0,0,0,0,0 };
    UINT32 app_variant = 0;
    UINT8 secure_key[16] = {0};
    UINT8 *pbSHMBuf = NULL;
    UINT32 size = sizeof(ta_uuid) + sizeof(APP_ID_KEY) + sizeof(app_variant) + sizeof(secure_key);
    UINT32 pta_uuid_pysical_addr = 0;
    UINT32 papp_id_key_pysical_addr = 0;
    UINT32 psecure_key_pysical_addr = 0;
    UINT8 *pkey;
    UINT8 *pbuf = NULL;
    DRM_TEE_SHM_HANDLE pbshmhandle = INVALID_SHM_HANDLE;

    app_variant = params[0].value.a;

    pbSHMBuf = DRM_TEE_SHM_Malloc(size, &pbshmhandle);
    if (NULL == pbSHMBuf) {
        FACTORY_ERR("Out of memory\n");
        result = TEE_ERROR_OUT_OF_MEMORY;
        goto Func_Ret;
    }
    memcpy(pbSHMBuf, &ta_uuid, sizeof(ta_uuid));
    memcpy(pbSHMBuf + sizeof(ta_uuid), APP_ID_KEY, sizeof(APP_ID_KEY));
    memcpy(pbSHMBuf + sizeof(ta_uuid) + sizeof(APP_ID_KEY), &app_variant, sizeof(app_variant));
    memcpy(pbSHMBuf + sizeof(ta_uuid) + sizeof(APP_ID_KEY) + sizeof(app_variant), secure_key, sizeof(secure_key));

    pta_uuid_pysical_addr = (UINT32)TEE_VirtToPhys(&pbSHMBuf[0]);
    papp_id_key_pysical_addr = (UINT32)TEE_VirtToPhys(&pbSHMBuf[sizeof(ta_uuid)]);
    psecure_key_pysical_addr = (UINT32)TEE_VirtToPhys(&pbSHMBuf[sizeof(ta_uuid) + sizeof(APP_ID_KEY) + sizeof(app_variant)]);
    pkey = &pbSHMBuf[sizeof(ta_uuid) + sizeof(APP_ID_KEY) + sizeof(app_variant)];

    if (TEE_SUCCESS != TEE_FlushCache(pbSHMBuf, size)) {
        FACTORY_ERR("Failed to flush cache\n");
        result = TEE_ERROR_GENERIC;
        goto Func_Ret;
    }

    result = BCM_GENERATE_SECUREKEY_V2(pta_uuid_pysical_addr, papp_id_key_pysical_addr, app_variant, psecure_key_pysical_addr);
    if (result != TEE_SUCCESS) {
        FACTORY_ERR("Failed to call BCM command (error %lX)\n", result);
        goto Func_Ret;
    }

    if (TEE_InvalidateCache(pbSHMBuf, size) != TEE_SUCCESS) {
        FACTORY_ERR("Failed to invalidate cache\n");
        result = TEE_ERROR_GENERIC;
    }
    // return secure_key to CA
    pbuf = (UINT8*)params[1].memref.buffer;
    memcpy(pbuf, pkey, params[1].memref.size);

Func_Ret:
    DRM_TEE_SHM_Free(pbshmhandle);

    return result;
}

TEE_Result MV_DRM_CRYPTO_InvokeCommandEntryPoint(uint32_t commandID,
                                              TEE_Param params[4])
{
    TEE_Result result = TEE_ERROR_GENERIC;

    /* This dispatcher can only handle CRYPTO related functions */
    assert(CRYPTO_MASK == (commandID & CRYPTO_MASK));

    switch (commandID) {
#ifndef CONFIG_OPTEE
    case DRM_SE_RSA_SIGN_VERIFY:
        result = Drm_Sign_Verify_Message(params);
        break;
#endif
    case DRM_SE_GEN_SECURE_KEY:
        result = Drm_GenerateSecureKey(params);
        break;
    default:
        break;
    }
    return result;
}
