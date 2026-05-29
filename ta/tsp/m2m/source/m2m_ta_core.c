/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
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
 */

///////////////////////////////////////////////////////////////////////////////
//! \file m2m_ta_core.c
//!
//! \brief  This file is used to implement the ta session wrapper for m2m ca
//!
//! Author:         Long Xiao
//! Modified by:
//! Version:        1.0
//! Date:           Aug 4 2015
//!
///////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "m2m_ta_mgr.h"
#include "m2m_ta_drv.h"
#include "m2m_ta_core.h"
#include "m2m_ta_fw.h"

#ifndef TA_COMPILE
#include "dmx_drmwrap.h"
#include "dmx_teec_api.h"
#else
#include "mem_region_userdata.h"

#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif

#endif

#define MV_M2M_RETRY_CNT            (10)

static UINT16 internal_GetCmdTag(void )
{
    static UINT16 uLoopNum = 0;

    return uLoopNum++;
}

#if 0

static UINT32 internal_sess_lock(TA_M2M_SESS_T *pSess)
{
    UINT32 WaitCnt = 0;

    do {
        if(SUCCESS == MV_OSAL_Mutex_TryLock(pSess->SessionMutexLock)) {
            return SUCCESS;
        }
        DMX_TASK_Sleep(100);
        WaitCnt++;
        if((WaitCnt%MV_M2M_RETRY_CNT) == 0) {
            DMX_HIGH("[Sess(=0x%x)] get lock timeout 1 sec!\n", pSess->uSessId);
        }
    }
    while(WaitCnt < MV_M2M_RETRY_CNT*100);

    return DMX_OSALFAIL;
}

static UINT32 internal_sess_unlock(TA_M2M_SESS_T *pSess)
{
    return MV_OSAL_Mutex_Unlock(pSess->SessionMutexLock);
}

static UINT32 internal_sess_trylock(TA_M2M_SESS_T *pSess)
{
    return MV_OSAL_Mutex_TryLock(pSess->SessionMutexLock);
}

#endif

#ifdef TA_COMPILE
static HRESULT internal_check_CryptoInfo(TA_M2M_BUFF_T *pCryptoInfoBuf)
{
    M2M_DRM_CRYPTO_ST *pCryptoInfo = (M2M_DRM_CRYPTO_ST *)pCryptoInfoBuf->virAddr;

    if (pCryptoInfo == NULL) {
        TA_LOGF( "CryptoInfo is NULL\n");
        return ERR_ERRPARAM;
    }

    if (pCryptoInfo->uiKeyIdLen > 48) {
        TA_LOGF( "KeyId length exceeds the limit\n");
        return ERR_ERRPARAM;
    }
    return SUCCESS;
}
#endif

static TA_M2M_SESS_T *M2M_Session_Alloc(TA_M2M_CRYPTO_H pCryptoSrv)
{
    UINT32 i =0;
    TA_M2M_SESS_T *pSess = NULL;

    TA_LOGH( "Alloc Session Enter\n");
    for (i=0; i<M2M_TA_CAP_SESSION_NR; i++) {
        pSess = &pCryptoSrv->MgrSessTable[i];
        if (pSess->isActive == 0) {
            memset(pSess, 0, sizeof(TA_M2M_SESS_T));
            pSess->uSessId = i;
            pSess->isActive = 1;
            pSess->uMagicNum = M2M_TA_SESSION_MAGIC;

            pSess->uCmdFinishNum = 0;
            pSess->pServer = pCryptoSrv;

            //use default config
            pSess->PatternCfg.pattern_enc = 1;
            pSess->PatternCfg.pattern_clr = 0;
            pSess->eResidueType = M2MCRYPTO_RESIDUE_TYPE_DEFAULT;

            break;
        }
    }

    #if 0
    if(pSess != NULL) {
        #ifndef TA_COMPILE
        ret = MV_OSAL_Mutex_Create(&pSess->SessionMutexLock);
        #else
        ret = TEE_MutexCreate(&pSess->SessionMutexLock, NULL);
        #endif
        M2M_CHECK_RETCODE(ret);
    }
    #endif

    TA_LOGH("Alloc Session success SessId[%d]\n", pSess->uSessId);
    return pSess;
    #if 0
    if(pSess->SessionMutexLock) {
        #ifndef TA_COMPILE
        MV_OSAL_Mutex_Destroy(&pSess->SessionMutexLock);
        #else
        TEE_MutexDestroy(pSess->SessionMutexLock);
        #endif
    }
    #endif
}

static HRESULT M2M_Session_Free(TA_M2M_SESS_T *pSess)
{
    HRESULT ret = SUCCESS;
    TA_LOGH("Free Session Enter\n");
    M2M_CHECK_MGRSESS(pSess);

    if(pSess->SessionMutexLock) {
        #if 0
        #ifndef TA_COMPILE
        MV_OSAL_Mutex_Destroy(&pSess->SessionMutexLock);
        #else
        TEE_MutexDestroy(pSess->SessionMutexLock);
        #endif
        #endif
    }
    pSess->isActive = 0;
    pSess->uSessCmdExecFlag = 0;

    TA_LOGH("Free Sess[%d] exit\n", pSess->uSessId);

error:
    return ret;
}


static HRESULT internal_sess_scheme_mapping(TA_M2M_SESS_T *pSess,
                            M2MCRYPTO_TYPE eScheme,
                            M2M_TA_CRYPTO_CMD *pCryptoCmd)
{
    if(eScheme == M2MCRYPTO_TYPE_AES_128_CBC) {
        pCryptoCmd->uCryptoParam = (AES_PARAM_MODE_CBC|(AES_PARAM_KEYLEN_128<<3));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_AES;
        }
        else {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVAES;
        }
    }
    else if(eScheme == M2MCRYPTO_TYPE_AES_128_ECB) {
        pCryptoCmd->uCryptoParam = (AES_PARAM_MODE_ECB|(AES_PARAM_KEYLEN_128<<3));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_AES;
        }
        else {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVAES;
        }
    }
    else if(eScheme == M2MCRYPTO_TYPE_AES_128_CTR) {
        //The last four bytes in IV are the counter and increase by one for each input word (16-byte).
        //The rest of the IV (the nonce) is kept same for all input data.
        pCryptoCmd->uCryptoParam = (AES_PARAM_MODE_CTR|(AES_PARAM_KEYLEN_128<<3));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_AES;
        } else {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVAES;
        }
    } else if(eScheme == M2MCRYPTO_TYPE_AES_128_CTR64) {
        //The last eight bytes in IV are the counter and increase by one for each input word (16-byte).
        //The rest of the IV (the nonce) is kept same for all input data.
        pCryptoCmd->uCryptoParam = (AES_PARAM_MODE_CTR64|(AES_PARAM_KEYLEN_128<<3));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
             pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_AES;
        } else {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVAES;
        }
    } else if(eScheme == M2MCRYPTO_TYPE_AES_128_CTR128) {
        //The entire 16 bytes in IV are the counter and increase by one for each input word (16-byte).
        pCryptoCmd->uCryptoParam = (AES_PARAM_MODE_CTR128|(AES_PARAM_KEYLEN_128<<3));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
             pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_AES;
        } else {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVAES;
        }
    }
    else if(eScheme == M2MCRYPTO_TYPE_DES_56_ECB) {
        pCryptoCmd->uCryptoParam = (DES_PARAM_MODE_ECB|(DES_PARAM_KEYLEN<<3));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_TDES;
        }
        else {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVTDES;
        }
    }
    else if(eScheme == M2MCRYPTO_TYPE_DVB_CSA_20) {
        pCryptoCmd->uCryptoParam = (DVBCSA_NSK2_SBOX_TABLE|(DVBCSA_CONFORMANCE_OFF<<5));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_DEC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVDVBCSA2;
        }
        else {
            /*seems not support DVBCSA but INVDVBCSA*/
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVDVBCSA2;
        }
    }
    else if(eScheme == M2MCRYPTO_TYPE_TDES_128_ECB) {
         pCryptoCmd->uCryptoParam = (DES_PARAM_MODE_ECB|(TDES_PARAM_KEYLEN_128<<3));
         if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
              pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_TDES;

         }
         else {
             pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVTDES;
         }
    }
    else if(eScheme == M2MCRYPTO_TYPE_TDES_128_CBC) {
        pCryptoCmd->uCryptoParam = (DES_PARAM_MODE_CBC|(TDES_PARAM_KEYLEN_128<<3));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_TDES;
        }
        else {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVTDES;
        }
    }
    else if(eScheme == M2MCRYPTO_TYPE_TDES_128_CTR) {
        pCryptoCmd->uCryptoParam = (DES_PARAM_MODE_CTR|(TDES_PARAM_KEYLEN_128<<3));
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_ENC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_TDES;
        }
        else {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVTDES;
        }
    } else if(eScheme == M2MCRYPTO_TYPE_DVB_CSA_30) {
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_DEC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_INVDVBCSA3;
            TA_LOGH( "pCryptoCmd->eCryptoScheme = 0x%x\n", pCryptoCmd->eCryptoScheme);
        }
    } else if(eScheme == M2MCRYPTO_TYPE_ASA) {
        if(pSess->eType == M2MCRYPTO_SESSION_TYPE_DEC) {
            pCryptoCmd->eCryptoScheme = SESS_CRYPTO_TYPE_ASA;
        }
    }
    else {
        TA_LOGF("Warning, scheme is wrong or not set(=0x%x)\n", eScheme);
        return ERR_ERRPARAM;
    }

#if (BERLIN_CHIP_VERSION>=BERLIN_VS640)
    /*  TspAesParam/TspTdesParam adds the usage filed on vs640
    TspAesParam_Usage_M2M = TspTdesParam_Usage_M2M = 1
    */
    pCryptoCmd->uCryptoParam |= (0x1 << 6);
#endif
    pCryptoCmd->eInputSessType = pSess->eType;
    pCryptoCmd->eInputCryptoType = eScheme;

    return SUCCESS;
}

#ifndef TA_COMPILE
static BOOL internal_sess_secure_rule_checker(TA_M2M_SESS_T *pSess,
                            M2MCRYPTO_MEM_H hInput,
                            M2MCRYPTO_MEM_H hOutput)
{
    return TRUE;
}

#else
#define TALTS_UUID      {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x03}}
#define CCL_UUID        {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x05}}
#define NAGRA_TA_UUID_HIGH_BYTES \
    {0x120f30a6, 0x836d, 0x11e8, {0xad, 0xc0, 0xfa, 0x7a, 0xe0, 0x1b}}
#define NAGRA_TA_UUID_LOW_BYTE_NUM 2
//nagra added wrong uuid and unwilling to modify it, could be removed if nagra fix it
#define NAGRA_TA_UUID_NEW {0x120f30a6, 0x11e8, 0x836d, {0x7a, 0xfa, 0xc0, 0xad, 0x00, 0x00, 0x01, 0x00}}
#define VERIMATRIX_UUID {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x07}}
#define PR32_UUID       {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x0c}}
#define PR40_UUID       {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x0d}}
#define CISCO_UUID      {0x63736943, 0x206F, 0x5356, {0x53, 0x53, 0x20, 0x54, 0x41, 0x20, 0x20, 0x20}}
#define PR25_UUID       {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x0b}}
#define WV_UUID         {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x0e}}
#define NSK_HDI_UUID    {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x33}}
#define IRDETO_UUID     {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x37}}
// Nagra rule
/*
     Crypto operation    P   Source      Destination result
     secSessionEncrypt   No  unprotected unprotected OK
     secSessionEncrypt   No  unprotected protected   Fail
     secSessionEncrypt   No  protected   unprotected Fail
     secSessionEncrypt   No  protected   protected   Fail
     secSessionEncrypt   Yes unprotected unprotected Fail
     secSessionEncrypt   Yes unprotected protected   Fail
     secSessionEncrypt   Yes protected   unprotected OK
     secSessionEncrypt   Yes protected   protected   OK
     secSessionDecrypt   No  unprotected unprotected OK
     secSessionDecrypt   No  unprotected protected   Fail
     secSessionDecrypt   No  protected   unprotected Fail
     secSessionDecrypt   No  protected   protected   Fail
     secSessionDecrypt   Yes unprotected unprotected Fail
     secSessionDecrypt   Yes unprotected protected   OK
     secSessionDecrypt   Yes protected   unprotected Fail
     secSessionDecrypt   Yes protected   protected   OK
*/
/* Crypto Protected  Source  Destination result
static UINT32 Nagra_RuleTbl[16] = {1, 0, 0, 0,   \
                                   0, 0, 1, 1,   \
                                   1, 0, 0, 0,   \
                                   0, 1, 0, 1};  */


//MRVL unified rule
//Notes:  Clr key, protected, not exist

/*Below MRVL rule is not aligned with Nagra, Nagra key table
  has usage rule, can control it is used for enc or dec.

  Decryption  Yes         DRM         S           S     MRVL Fail (Nagra Pass)
  Decryption  No          DRM         S           S     MRVL Pass (Nagra Fail)
  Decryption  No          DRM         NS          S     MRVL Pass (Nagra Fail) - Playready


  Operation   Protected Key Option  In Buffer   Out Buffer  Result

  Encryption  No          Clear       NS          NS          Pass
  Encryption  No          Clear       NS          S           Fail
  Encryption  No          Clear       S           NS          Fail
  Encryption  No          Clear       S           S           Pass

  Encryption  No          DRM         NS          NS          Pass
  Encryption  No          DRM         NS          S           Fail
  Encryption  No          DRM         S           NS          Fail
  Encryption  No          DRM         S           S           Fail

  Encryption  Yes         Clear       NS          NS          Fail
  Encryption  Yes         Clear       NS          S           Fail
  Encryption  Yes         Clear       S           NS          Fail
  Encryption  Yes         Clear       S           S           Fail

  Encryption  Yes         DRM         NS          NS          Fail
  Encryption  Yes         DRM         NS          S           Fail
  Encryption  Yes         DRM         S           NS          Pass
  Encryption  Yes         DRM         S           S           Pass

  Decryption  No          Clear       NS          NS          Pass
  Decryption  No          Clear       NS          S           Fail
  Decryption  No          Clear       S           NS          Fail
  Decryption  No          Clear       S           S           Pass

  Decryption  No          DRM         NS          NS          Pass
  Decryption  No          DRM         NS          S           Pass
  Decryption  No          DRM         S           NS          Fail
  Decryption  No          DRM         S           S           Pass

  Decryption  Yes         Clear       NS          NS          Fail
  Decryption  Yes         Clear       NS          S           Fail
  Decryption  Yes         Clear       S           NS          Fail
  Decryption  Yes         Clear       S           S           Fail

  Decryption  Yes         DRM         NS          NS          Fail
  Decryption  Yes         DRM         NS          S           Pass
  Decryption  Yes         DRM         S           NS          Fail
  Decryption  Yes         DRM         S           S           Fail
*/

static UINT8 MRVL_RuleTbl[32] =  {1,    0,    0,     1,   \
                                  1,    0,    0,     0,   \
                                  0,    0,    0,     0,   \
                                  0,    0,    1,     1,   \
                                  1,    0,    0,     1,   \
                                  1,    1,    0,     1,   \
                                  0,    0,    0,     0,   \
                                  0,    1,    0,     0};

static TEE_Result internal_check_mem_attr(UINT32 uMemId, BOOL *bSecure)
{
    TEE_Result ret = TEE_SUCCESS;

    if (TEE_Check_Memory_Attr(uMemId, MEMATTR_NONSECURE | MEMATTR_ION)) {
        *bSecure = FALSE;
    } else if (TEE_Check_Memory_Attr(uMemId, MEMATTR_SECURE | MEMATTR_ION)) {
        *bSecure = TRUE;
    } else {
        ret = TEE_ERROR_BAD_PARAMETERS;
    }
    return ret;
}

static BOOL internal_sess_secure_rule_checker(TA_M2M_SESS_T *pSess,
                                                     TA_M2M_BUFF_T *pInputBuf,
                                                     TA_M2M_BUFF_T *pOutputBuf)
{
    TEE_Result ret = TEE_SUCCESS;
    TEE_Identity identity;
    uint8_t i = 0;
    BOOL bKeyOption = FALSE;
    BOOL bCipher= FALSE, bProtected = FALSE;
    BOOL bInput = FALSE, bOutput = FALSE;
    BOOL bLegalTA = FALSE;
    uint32_t bValidKeyUser = FALSE;
    uint32_t uRuleId = 0;
    TEE_UUID taltsuuid = TALTS_UUID;
    TEE_UUID ccluuid = CCL_UUID;
    TEE_UUID nagra_ta_uuid_high_bytes = NAGRA_TA_UUID_HIGH_BYTES;
    TEE_UUID nagra_ta_uuid_new = NAGRA_TA_UUID_NEW;
    TEE_UUID vmxuuid = VERIMATRIX_UUID;
    TEE_UUID pr32uuid = PR32_UUID;
    TEE_UUID pr40uuid = PR40_UUID;
    TEE_UUID ciscouuid = CISCO_UUID;
    TEE_UUID pr25uuid = PR25_UUID;
    TEE_UUID wvuuid = WV_UUID;
    TEE_UUID nskhdiuuid = NSK_HDI_UUID;
    TEE_UUID irdetouuid = IRDETO_UUID;

    TA_LOGH("SESS[%p][%d][%d]\n", pSess, pSess->uSessId, pSess->eType);

    ret = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
    if(ret != TEE_SUCCESS){
       TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", ret);
       return FALSE;
    }

    /* For specific TA call, do secure rule check; CA->TA, uuid is zero */
    /* CA/TA call M2M TA follows unified secure rule */
    if (identity.login == TEE_LOGIN_TRUSTED_APP) {
        TA_LOGH("Current client UUID: 0x%x, 0x%x, 0x%x",    \
                  identity.uuid.timeLow, identity.uuid.timeMid, identity.uuid.timeHiAndVersion);
        for(i = 0; i < 8; i++) {
             TA_LOGH("0x%x", identity.uuid.clockSeqAndNode[i]);
        }
        TA_LOGH("\n");

        if ( memcmp(&identity.uuid, &taltsuuid, sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &ccluuid,   sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &nagra_ta_uuid_high_bytes, (sizeof(TEE_UUID)-NAGRA_TA_UUID_LOW_BYTE_NUM)) && \
             memcmp(&identity.uuid, &nagra_ta_uuid_new,   sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &vmxuuid,   sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &ciscouuid, sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &pr32uuid,  sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &pr40uuid,  sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &pr25uuid,  sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &wvuuid, sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &nskhdiuuid, sizeof(TEE_UUID)) && \
             memcmp(&identity.uuid, &irdetouuid,  sizeof(TEE_UUID))) {
             TA_LOGF("Un-authorized TA, refuse service\n");
             return false;
        } else {
            bLegalTA = true;
            //Special handle for Nagra/VMX TA, need remove after Nagra/VMX TA remove extra copy
            if(!memcmp(&identity.uuid, &taltsuuid, sizeof(TEE_UUID)) || \
               !memcmp(&identity.uuid, &ccluuid, sizeof(TEE_UUID)) || \
               !memcmp(&identity.uuid, &nagra_ta_uuid_high_bytes, (sizeof(TEE_UUID)-NAGRA_TA_UUID_LOW_BYTE_NUM)) || \
               !memcmp(&identity.uuid, &nagra_ta_uuid_new, sizeof(TEE_UUID)) || \
               !memcmp(&identity.uuid, &vmxuuid, sizeof(TEE_UUID)) || \
               !memcmp(&identity.uuid, &nskhdiuuid, sizeof(TEE_UUID))) {
                TA_LOGH("Nagra/VMX/NSK TA called, bypass\n");
                return TRUE;
            }
        }
    }


    if(pSess->CryptoConfig.HWKeyArea == HW_KEYAREA_GKL) {
        TA_LOGH("GKL key table, bypass\n");
        return TRUE;
    }

    if(pSess->CryptoConfig.KeyOption == M2MCRYPTO_KEY_INDEX) {
        ret = Drm_SE_KtMgr_Validate_KeyUser(pSess->CryptoConfig.HWKeyArea,    \
                     pSess->CryptoConfig.TSPKeyTblIndex, &identity.uuid, &bValidKeyUser);
        if((ret != TEE_SUCCESS) || !bValidKeyUser){
           TA_LOGF( "Drm_SE_Validate_KeyUser fail, bValid %d, ret 0x%x\n", bValidKeyUser, ret);
           return FALSE;
        }
    }

    bCipher = (pSess->eType == M2MCRYPTO_SESSION_TYPE_DEC) ? TRUE:FALSE;
    bProtected = pSess->CryptoConfig.bProtected;
    if(bLegalTA) {
        // Key option regarded as index(protected clr key/index) for legal TA calling
        bKeyOption = TRUE;
        TA_LOGH("Legal TA calling\n");
    } else {
        bKeyOption = (pSess->CryptoConfig.KeyOption == M2MCRYPTO_KEY_INDEX)? TRUE:FALSE;
        TA_LOGH("REE CA calling\n");
    }
    ret = internal_check_mem_attr(pInputBuf->memID, &bInput);
    if (ret != TEE_SUCCESS) {
      TA_LOGE("SESS[%d] Input buffer[%"PRIx32"] not physical continuous\n",
            pSess->uSessId, pInputBuf->memID);
        return FALSE;
    }

    ret = internal_check_mem_attr(pOutputBuf->memID, &bOutput);
    if (ret != TEE_SUCCESS) {
      TA_LOGE("SESS[%d] Output buffer[%"PRIx32"] not physical continuous\n",
            pSess->uSessId, pOutputBuf->memID);
        return FALSE;
    }

    uRuleId = bCipher<<4|bProtected<<3|bKeyOption<<2|bInput<<1|bOutput;
    TA_LOGH("[Schm 0x%x][Index 0x%02x] Cipher,Protected,bKeyOption,Src,Dest:%d%d%d%d%d-%d\n",
                 pSess->CryptoConfig.Cryptoscheme, pSess->CryptoConfig.TSPKeyTblIndex, \
                 bCipher, bProtected, bKeyOption, bInput, bOutput, MRVL_RuleTbl[uRuleId]);
    if(pSess->CryptoConfig.HWKeyArea != HW_KEYAREA_NAGRA) {
        if(MRVL_RuleTbl[uRuleId] == 0) {
            TA_LOGF("SESS[%d] RuleID[0x%x]: Break TSP key table secure rule alg 0x%x\n",
                pSess->uSessId, uRuleId, pSess->CryptoConfig.Cryptoscheme);
            return FALSE;
        }
    } else {
        if(MRVL_RuleTbl[uRuleId] == 0) {
            //11111
            if(uRuleId != 0x1F) {
                TA_LOGF("SESS[%d] RuleID[0x%x]: Break Nagra key table secure rule alg 0x%x\n",
                      pSess->uSessId, uRuleId, pSess->CryptoConfig.Cryptoscheme);
                return FALSE;
            }
        }
        if(MRVL_RuleTbl[uRuleId] == 1) {
            //10111 and 10101
            if((uRuleId == 0x17) || (uRuleId == 0x15)) {
                TA_LOGF("SESS[%d] RuleID[0x%x]: Break Nagra key table secure rule alg 0x%x\n",
                      pSess->uSessId, uRuleId, pSess->CryptoConfig.Cryptoscheme);
                return FALSE;
            }
        }
    }
    return true;
}

#endif

UINT32 internal_sess_callback(TA_M2M_SESS_HANDLE hSess, UINT16 Tag)
{
    HRESULT ret = ERR_ERRPARAM;

    M2M_CHECK_MGRSESS(hSess);
    ret = M2M_TA_Session_Postprocess(hSess, Tag);
    M2M_CHECK_RETCODE(ret);
error:
    return ret;
}

HRESULT M2M_TA_Core_Session_Open(TA_M2M_CRYPTO_H hCryptoSrv,
                M2MCRYPTO_SESSION_TYPE eType,
                TA_M2M_SESS_HANDLE *phSession)
{
    TA_M2M_SESS_T *pSess = NULL;
    HRESULT ret;

    TA_LOGH( "Open enter\n");

    M2M_CHECK_NULL(hCryptoSrv);
    M2M_CHECK_NULL(phSession);

    pSess = (TA_M2M_SESS_T *)M2M_Session_Alloc(hCryptoSrv);
    if (!pSess) {
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    pSess->pInCmdList = DMX_LIST_CREATE(NULL, NULL);
    TA_LOGH("Session %p, Id: %d, pInCmdList %p\n", pSess, pSess->uSessId,pSess->pInCmdList);
    pSess->eType = eType;

    *phSession = pSess;

    TA_LOGH( "Open success Sess [%p][%d]\n", pSess, pSess->uSessId);
    return TEE_SUCCESS;

error:
    if (pSess != NULL) {
        if(pSess->pInCmdList != NULL) {
            DMX_LIST_DESTROY(pSess->pInCmdList);
        }
        M2M_Session_Free(pSess);
    }
    TA_LOGF( "Open fail (ret=0x%x)\n", ret);
    return ret;
}

static HRESULT Internal_Session_DTCM_Free(TA_M2M_SESS_HANDLE hSess)
{
    HRESULT ret = SUCCESS;
#ifdef TA_COMPILE
    TEE_Identity identity;
#endif

    TA_LOGH( "Free Sess DTCM [%p][%d]\n", hSess, hSess->uSessId);
    M2M_CHECK_MGRSESS(hSess);

#ifdef TA_COMPILE
    ret = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
    if(ret != TEE_SUCCESS){
        TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", ret);
        return FALSE;
    }
    /* TA->TA call, content key(app/drm) allocated from secure dtcm */
    if (identity.login == TEE_LOGIN_TRUSTED_APP) {
        if((hSess->CryptoConfig.KeyDTCM != 0) && (hSess->CryptoConfig.KeyDTCM != 0xFFFF)) {
            M2M_TA_Dtcm_Mem_Free(hSess->CryptoConfig.KeyDTCM, hSess->uFigoSysId, DTCM_MEM_TYPE_SECURE);
        }
    } else {
        if((hSess->CryptoConfig.KeyDTCM != 0) && (hSess->CryptoConfig.KeyDTCM != 0xFFFF)) {
            M2M_TA_Dtcm_Mem_Free(hSess->CryptoConfig.KeyDTCM, hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
        }
    }
#else
    if((hSess->CryptoConfig.KeyDTCM != 0) && (hSess->CryptoConfig.KeyDTCM != 0xFFFF)) {
        M2M_TA_Dtcm_Mem_Free(hSess->CryptoConfig.KeyDTCM, hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
    }
#endif

    if((hSess->CryptoConfig.IVDTCM != 0) && (hSess->CryptoConfig.IVDTCM != 0xFFFF)) {
        M2M_TA_Dtcm_Mem_Free(hSess->CryptoConfig.IVDTCM,  hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
    }

    TA_LOGH( "DTCM Free success\n");
    return ret;
error:
    TA_LOGF( "DTCM Free fail(ret=0x%x)\n", ret);
    return ret;

}

HRESULT M2M_TA_Core_Session_Close(TA_M2M_SESS_HANDLE hSess)
{
    HRESULT ret = SUCCESS;

    TA_LOGH( "Close Sess [%p][%d]\n", hSess, hSess->uSessId);

    M2M_CHECK_MGRSESS(hSess);
    if(hSess->pInCmdList != NULL) {
        DMX_LIST_DESTROY(hSess->pInCmdList);
    }
    Internal_Session_DTCM_Free(hSess);
    M2M_Session_Free(hSess);
    TA_LOGH( "Deinit success\n");
    return ret;
error:
    TA_LOGF( "Deinit fail(ret=0x%x)\n", ret);
    return ret;
}

HRESULT M2M_TA_Core_AllocFigoTask(UINT32 uFigoId)
{
    return TSP_CryptoAllocFigoTask(uFigoId);
}

HRESULT M2M_TA_Core_FreeFigoTask(UINT32 uFigoId)
{
    return TSP_CryptoFreeFigoTask(uFigoId);
}

HRESULT M2M_TA_Core_StartFw(UINT32 *pFigoId)
{
    return TSP_CryptoStartFw((int32_t *)pFigoId);
}

HRESULT M2M_TA_Core_StopFw(UINT32 uFigoId)
{
    return TSP_CryptoStopFw(uFigoId);
}

HRESULT M2M_TA_Core_Session_SetScheme(TA_M2M_SESS_HANDLE hSess, M2MCRYPTO_TYPE eScheme)
{
    HRESULT ret = ERR_ERRPARAM;

    TA_LOGH( "SetScheme Enter(eScheme=0x%x)\n",eScheme);

    M2M_CHECK_MGRSESS(hSess);

    if((eScheme>M2MCRYPTO_TYPE_INVALID) && (eScheme<M2MCRYPTO_TYPE_MAX)) {
        hSess->CryptoConfig.Cryptoscheme = eScheme;
    }
    else {
        ret = ERR_ERRPARAM;
        TA_LOGF("SetScheme Wrong scheme(=0x%x)\n", eScheme);
        goto error;
    }
    TA_LOGH("SetScheme Successe\n");
    return SUCCESS;
error:
    TA_LOGF("SetScheme fail(ret=0x%x)\n", ret);
    return ret;
}

#ifndef TA_COMPILE
HRESULT M2M_TA_Core_Session_Config(TA_M2M_SESS_HANDLE hSess,
                            M2MCRYPTO_KEY_OPTION  KeyOption,
                            UINT32          TSPKeyTblIndex,
                            UINT8           *pKeyData,
                            UINT32          KeyLength,
                            M2MCRYPTO_KEY_OPTION  IVOption,
                            UINT32          TSPIVTblIndex,
                            UINT8           *pIVData,
                            UINT32          IVLength,
                            M2M_DRM_CRYPTO_ST   *pCryptInfo)
#else
HRESULT M2M_TA_Core_Session_Config(TA_M2M_SESS_HANDLE hSess,
                            M2MCRYPTO_KEY_OPTION  KeyOption,
                            UINT32          TSPKeyTblIndex,
                            TA_M2M_BUFF_T  *pKeyBuf,
                            M2MCRYPTO_KEY_OPTION  IVOption,
                            UINT32          TSPIVTblIndex,
                            TA_M2M_BUFF_T  *pIVBuf,
                            TA_M2M_BUFF_T  *pCryptInfoBuf)
#endif
{
    HRESULT ret = ERR_ERRPARAM;
    DMX_KEY_CTX KeyContext;
    FIGODRV_HANDLE hFigoDrv = current_figohandle(hSess->uFigoSysId);
#ifdef TA_COMPILE
    TEE_Identity identity;
    TEE_MemRegion region;
    UINT8 *pKeyData = pKeyBuf->virAddr;
    UINT8 *pIVData = pIVBuf->virAddr;
    UINT32 KeyLength = pKeyBuf->uBufSize;
    UINT32 IVLength = pIVBuf->uBufSize;
    M2M_DRM_CRYPTO_ST  *pCryptInfo = (M2M_DRM_CRYPTO_ST *)pCryptInfoBuf->virAddr;
#endif

    TA_LOGH( "CryptoConfig Enter\n");

    M2M_CHECK_MGRSESS(hSess);

    if(KeyLength > DMX_KEY_MAX_LEN) {
        KeyLength = DMX_KEY_MAX_LEN;
    }
    if(IVLength > DMX_KEY_MAX_LEN) {
        IVLength = DMX_KEY_MAX_LEN;
    }

    KeyContext.bKeyFromDrm   = FALSE;
    KeyContext.bIVKeyFromDrm = FALSE;

    hSess->CryptoConfig.KeyOption = KeyOption;
    if(KeyOption == M2MCRYPTO_KEY_CONTENT) {
        #ifdef TA_COMPILE
        ret = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
        if(ret != TEE_SUCCESS){
            TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", ret);
            return ret;
        }
        /* TA->TA call, clear key allocated from secure dtcm */
        if (identity.login == TEE_LOGIN_TRUSTED_APP) {
            if((hSess->CryptoConfig.KeyDTCM == 0) || (hSess->CryptoConfig.KeyDTCM == 0xFFFF)) {
                hSess->CryptoConfig.KeyDTCM = M2M_TA_Dtcm_Mem_Alloc(KeyLength, 8, hSess->uFigoSysId, DTCM_MEM_TYPE_SECURE);
            }
        } else {
            if(pKeyBuf->eMemType != TSP_MEM_PARAM_MEMREF) {
                ret = TEE_FindMemRegion(&region, pKeyData, KeyLength);
                M2M_CHECK_RETCODE(ret);
                if(MR_IS_SECURE(&region)) {
                    TA_LOGF( "CA Key memref shm is secure\n");
                    ret = ERR_ERRPARAM;
                    goto error;
                }
                if (!TEE_MR_USER_IS_ION(&region)) {
                    TA_LOGF("KeyData buffer not allocated from ION\n");
                    ret = ERR_ERRPARAM;
                    goto error;
                }
            }
            if((hSess->CryptoConfig.KeyDTCM == 0) || (hSess->CryptoConfig.KeyDTCM == 0xFFFF)) {
                hSess->CryptoConfig.KeyDTCM = M2M_TA_Dtcm_Mem_Alloc(KeyLength, 8, hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
            }
        }
        #else
        if((hSess->CryptoConfig.KeyDTCM == 0) || (hSess->CryptoConfig.KeyDTCM == 0xFFFF)) {
            hSess->CryptoConfig.KeyDTCM = M2M_TA_Dtcm_Mem_Alloc(KeyLength, 8, hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
        }
        #endif
        if(hSess->CryptoConfig.KeyDTCM == 0xFFFF) {
            TA_LOGF( "No DTCM Memory\n");
            ret = ERR_NOHWRSC;
            goto error;
        }
        TA_LOGI("Key Dump: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
                "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
                pKeyData[0], pKeyData[1], pKeyData[2], pKeyData[3],
                pKeyData[4], pKeyData[5], pKeyData[6], pKeyData[7],
                pKeyData[8], pKeyData[9], pKeyData[10], pKeyData[11],
                pKeyData[12], pKeyData[13], pKeyData[14], pKeyData[15]);
        M2M_TA_LoadDataToDTCM(hFigoDrv, hSess->CryptoConfig.KeyDTCM,
                pKeyData, KeyLength);
    } else if(KeyOption == M2MCRYPTO_KEY_INDEX) {
        if (pCryptInfo != NULL) {
            switch(pCryptInfo->uiSchemeType)
            {
                case M2M_DRMSCHEME_NAGRA:
                    hSess->CryptoConfig.HWKeyArea = HW_KEYAREA_NAGRA;
                    TA_LOGH("M2M_DRMSCHEME_NAGRA Case\n");
                    break;
                case M2M_DRMSCHEME_NDS:
                    hSess->CryptoConfig.HWKeyArea = HW_KEYAREA_NSK2_M2M;
                    TA_LOGH("M2M_DRMSCHEME_NDS Case\n");
                    break;
                case M2M_DRMSCHEME_IRDETO:
                    hSess->CryptoConfig.HWKeyArea = HW_KEYAREA_IFCP_OTT;
                    TA_LOGH("M2M_DRMSCHEME_IRDETO Case\n");
                    //need to check if the keyIndex was used for OTT
                    if(!VALIDATE_IFCP_KEY_IN_PARTITION(TSPKeyTblIndex, OTT)) {
                        TA_LOGF("error: keyIndex(0x%x) is not in IFCP OTT range\n", TSPKeyTblIndex);
                        ret = ERR_ERRPARAM;
                        goto error;
                    }
                    break;
                case M2M_DRMSCHEME_GKL:
                    hSess->CryptoConfig.HWKeyArea = HW_KEYAREA_GKL;
                    TA_LOGH("M2M_DRMSCHEME_GKL Case\n");
                    break;
                default:
                    hSess->CryptoConfig.HWKeyArea = HW_KEYAREA_GENERAL;
                    break;
            }
        } else {
            TA_LOGF( "CryptInfo is NULL\n");
            ret = ERR_ERRPARAM;
            goto error;
        }
        // JIRA20106: confirm keytable index is 16bytes (128bits) aligned
        if (TSPKeyTblIndex % 16 != 0) {
            TA_LOGF("KeyIndex is not 16byte aligned\n");
            ret = ERR_ERRPARAM;
            goto error;
        }
        //32 bytes per Nagra key table, (32-keysize) offset stores key
        hSess->CryptoConfig.TSPKeyTblIndex = TSPKeyTblIndex;
    } else if(KeyOption == M2MCRYPTO_KEY_FROM_DRM) {
        KeyContext.bKeyFromDrm = TRUE;
#ifdef TA_COMPILE
        ret = internal_check_CryptoInfo(pCryptInfoBuf);
        M2M_CHECK_RETCODE(ret);
        ret = Drm_SE_KeyMgr_GetContentKeyInfo(pCryptInfo, &KeyContext);
#else
        ret = DMX_DrmWrap_GetKeyCtx(pCryptInfo, &KeyContext, FALSE);
#endif
        M2M_CHECK_RETCODE(ret);
        TA_LOGI("KeyContext.KeyOption = 0x%x, KeyContext.TSPKeyTblIndex = 0x%x\n",
            KeyContext.KeyOption, KeyContext.KeyTblIndex);
        TA_LOGI("KeyContext.HWKeyArea = 0x%x\n", KeyContext.HWKeyArea);
        hSess->CryptoConfig.KeyOption = KeyContext.KeyOption;
        hSess->CryptoConfig.HWKeyArea = KeyContext.HWKeyArea;
        if(KeyContext.KeyOption == M2MCRYPTO_KEY_CONTENT) {
            if((hSess->CryptoConfig.KeyDTCM == 0) || (hSess->CryptoConfig.KeyDTCM == 0xFFFF)) {
                #ifdef TA_COMPILE
                ret = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
                if(ret != TEE_SUCCESS){
                    TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", ret);
                    return ret;
                }
                /* TA->TA call, clear key allocated from secure dtcm */
                if (identity.login == TEE_LOGIN_TRUSTED_APP) {
                    hSess->CryptoConfig.KeyDTCM = M2M_TA_Dtcm_Mem_Alloc(KeyLength, 8, hSess->uFigoSysId, DTCM_MEM_TYPE_SECURE);
                } else {
                    hSess->CryptoConfig.KeyDTCM = M2M_TA_Dtcm_Mem_Alloc(KeyLength, 8, hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
                }
                #else
                hSess->CryptoConfig.KeyDTCM = M2M_TA_Dtcm_Mem_Alloc(KeyLength, 8, hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
                #endif

                if(hSess->CryptoConfig.KeyDTCM == 0xFFFF) {
                    TA_LOGF("Not DTCM Memory\n");
                    ret = ERR_NOHWRSC;
                    goto error;
                }
            }
            M2M_TA_LoadDataToDTCM(hFigoDrv, hSess->CryptoConfig.KeyDTCM,
                KeyContext.KeyContent, KeyContext.KeyLength);
        } else if(KeyContext.KeyOption == M2MCRYPTO_KEY_INDEX) {
            if ((pCryptInfo->uiSchemeType == M2M_DRMSCHEME_IRDETO) &&
                !VALIDATE_IFCP_KEY_IN_PARTITION(KeyContext.KeyTblIndex, OTT)) {
                TA_LOGF("Error: keyIndex(0x%x) is not in IFCP OTT range\n", KeyContext.KeyTblIndex);
                ret = ERR_ERRPARAM;
                goto error;
            }
            // JIRA20106: confirm keytable index is 16bytes (128bits) aligned
            if (KeyContext.KeyTblIndex % 16 != 0) {
                TA_LOGF("KeyIndex is not 16byte aligned\n");
                ret = ERR_ERRPARAM;
                goto error;
            }
            //32 bytes per Nagra key table, (32-keysize) offset stores key
            hSess->CryptoConfig.TSPKeyTblIndex = KeyContext.KeyTblIndex;
        }
    } else {
        ret = ERR_ERRPARAM;
        TA_LOGF("KeyOption config wrong(=0x%x)\n", KeyOption);
        goto error;
    }

    if(hSess->CryptoConfig.KeyOption == M2MCRYPTO_KEY_INDEX &&
        hSess->CryptoConfig.HWKeyArea == HW_KEYAREA_GENERAL &&
        hSess->CryptoConfig.TSPKeyTblIndex >= TSP_KEY_TABLE_MAX_OFFSET) {
        ret = ERR_ERRPARAM;
        TA_LOGE("Invalid key index(=0x%x)\n", hSess->CryptoConfig.TSPKeyTblIndex);
        goto error;
    }

    hSess->CryptoConfig.IVOption = IVOption;
    if(IVOption == M2MCRYPTO_KEY_CONTENT) {
        #ifdef TA_COMPILE
        ret = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
        if(ret != TEE_SUCCESS){
             TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", ret);
             return ret;
        }
        if((identity.login != TEE_LOGIN_TRUSTED_APP) && pIVBuf->eMemType != TSP_MEM_PARAM_MEMREF) {
            ret = TEE_FindMemRegion(&region, pIVData, IVLength);
            M2M_CHECK_RETCODE(ret);
            if(MR_IS_SECURE(&region)) {
                TA_LOGF( "CA IV memref shm is secure\n");
                ret = ERR_ERRPARAM;
                goto error;
            }
            if (!TEE_MR_USER_IS_ION(&region)) {
                TA_LOGF("IVData buffer not allocated from ION\n");
                ret = ERR_ERRPARAM;
                goto error;
            }
        }
        #endif
        if((hSess->CryptoConfig.IVDTCM == 0) || (hSess->CryptoConfig.IVDTCM == 0xFFFF)) {
            hSess->CryptoConfig.IVDTCM = M2M_TA_Dtcm_Mem_Alloc(IVLength, 8, hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
        }
        if(hSess->CryptoConfig.IVDTCM == 0xFFFF) {
            TA_LOGF("Not DTCM Memory\n");
            ret = ERR_NOHWRSC;
            goto error;
        }
        hSess->CryptoConfig.IVLength = IVLength;
        TA_LOGI("IV(Policy: 0x%x) Dump: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, "
                "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
                hSess->eIVPolicy, pIVData[0], pIVData[1], pIVData[2], pIVData[3],
                pIVData[4], pIVData[5], pIVData[6], pIVData[7],
                pIVData[8], pIVData[9], pIVData[10], pIVData[11],
                pIVData[12], pIVData[13], pIVData[14], pIVData[15]);
        memcpy(hSess->CryptoConfig.InitIV, pIVData, IVLength);
        M2M_TA_LoadDataToDTCM(hFigoDrv, hSess->CryptoConfig.IVDTCM,
            pIVData, IVLength);
    } else if(IVOption == M2MCRYPTO_KEY_INDEX) {
        hSess->CryptoConfig.TSPIVTblIndex = TSPIVTblIndex;
    } else if(IVOption == M2MCRYPTO_KEY_FROM_DRM) {
        KeyContext.bIVKeyFromDrm = TRUE;
#ifdef TA_COMPILE
        ret = internal_check_CryptoInfo(pCryptInfoBuf);
        M2M_CHECK_RETCODE(ret);
        ret = Drm_SE_KeyMgr_GetContentKeyInfo(pCryptInfo, &KeyContext);
#else
        ret = DMX_DrmWrap_GetKeyCtx(pCryptInfo, &KeyContext, FALSE);
#endif
        M2M_CHECK_RETCODE(ret);
        TA_LOGI("KeyContext.KeyOption = 0x%x, KeyContext.TSPIVTblIndex = 0x%x\n",
            KeyContext.IVOption, KeyContext.IVTblIndex);
        hSess->CryptoConfig.IVOption = KeyContext.IVOption;
        if(KeyContext.IVOption == M2MCRYPTO_KEY_CONTENT) {
            if((hSess->CryptoConfig.IVDTCM == 0) || (hSess->CryptoConfig.IVDTCM == 0xFFFF)) {
                hSess->CryptoConfig.IVDTCM = M2M_TA_Dtcm_Mem_Alloc(IVLength, 8, hSess->uFigoSysId, DTCM_MEM_TYPE_NONSECURE);
                if(hSess->CryptoConfig.IVDTCM == 0xFFFF) {
                    TA_LOGF("No DTCM Memory\n");
                    ret = ERR_NOHWRSC;
                    goto error;
                }
            }
            hSess->CryptoConfig.IVLength = KeyContext.IVKeyLen;
            M2M_TA_LoadDataToDTCM(hFigoDrv, hSess->CryptoConfig.IVDTCM,
                KeyContext.IVData, KeyContext.IVKeyLen);
        } else if(KeyContext.IVOption == M2MCRYPTO_KEY_INDEX) {
            hSess->CryptoConfig.TSPIVTblIndex = KeyContext.IVTblIndex;
        }
    } else {
        if(hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_AES_128_ECB || \
           hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_DES_56_ECB  || \
           hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_TDES_128_ECB|| \
           hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_DVB_CSA_30  || \
           hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_ASA) {
           TA_LOGI("skip IV set in ECB and other mode\n");
        } else {
            ret = ERR_ERRPARAM;
            TA_LOGF("IVOption config wrong(=0x%x)\n", IVOption);
            goto error;
        }
    }

    if(hSess->CryptoConfig.IVOption == M2MCRYPTO_KEY_INDEX &&
        hSess->CryptoConfig.HWKeyArea == HW_KEYAREA_GENERAL &&
        hSess->CryptoConfig.TSPIVTblIndex >= TSP_KEY_TABLE_MAX_OFFSET) {
        ret = ERR_ERRPARAM;
        TA_LOGE("Invalid key index(=0x%x) for iv\n", hSess->CryptoConfig.TSPIVTblIndex);
        goto error;
    }

#ifdef TA_COMPILE
    if(hSess->CryptoConfig.KeyOption == M2MCRYPTO_KEY_INDEX &&
        hSess->CryptoConfig.HWKeyArea != HW_KEYAREA_NSK2_M2M &&
        hSess->CryptoConfig.HWKeyArea != HW_KEYAREA_GKL) {
        uint32_t bProtected = 1;
        BOOL bMatchKeyUsage = FALSE;
        uint32_t uKeyUsage = M2MCRYPTO_KEY_USAGE_ENC | M2MCRYPTO_KEY_USAGE_DEC;

        ret = Drm_SE_KtMgr_GetKeyRight(hSess->CryptoConfig.HWKeyArea,    \
                                        hSess->CryptoConfig.TSPKeyTblIndex,
                                        &bProtected, &uKeyUsage);
        if(ret == TEE_SUCCESS){
            hSess->CryptoConfig.bProtected = bProtected;
            TA_LOGH("TSPIndex=0x%02x,HWKeyArea=0x%x,%d 0x%x\n", hSess->CryptoConfig.TSPKeyTblIndex, \
                          hSess->CryptoConfig.HWKeyArea, hSess->CryptoConfig.bProtected, uKeyUsage);
            if((hSess->eType == M2MCRYPTO_SESSION_TYPE_ENC && (uKeyUsage & M2MCRYPTO_KEY_USAGE_ENC)) ||
                (hSess->eType == M2MCRYPTO_SESSION_TYPE_DEC && (uKeyUsage & M2MCRYPTO_KEY_USAGE_DEC))) {
                bMatchKeyUsage = TRUE;
            }
            if(!bMatchKeyUsage) {
                TA_LOGF("key usage mismatch: TSPIndex=0x%02x,HWKeyArea=0x%x,uKeyUsage=0x%x\n",
                          hSess->CryptoConfig.TSPKeyTblIndex, hSess->CryptoConfig.HWKeyArea, uKeyUsage);
                ret = ERR_ERRPARAM;
                goto error;
            }
        } else{
            TA_LOGF("TSPIndex=0x%02x,HWKeyArea=0x%x,ret=0x%x\n",
                          hSess->CryptoConfig.TSPKeyTblIndex, hSess->CryptoConfig.HWKeyArea, ret);
            //hSess->CryptoConfig.bProtected = 1;
            goto error;
        }
    } else {
        // DTCM key not protected
        hSess->CryptoConfig.bProtected = 0;
    }
#endif

    TA_LOGI("CryptoConfig Success\n");
    return SUCCESS;

error:
    Internal_Session_DTCM_Free(hSess);
    TA_LOGF("CryptoConfig fail(ret=0x%x)\n", ret);
    return ret;
}

HRESULT M2M_TA_Core_Session_SetPatternMode(TA_M2M_SESS_HANDLE hSess,
                                        M2MCRYPTO_PATTERN_MODE_CFG *pCfg)
{
    HRESULT ret = ERR_ERRPARAM;

    TA_LOGH( "SetPattern Enter\n");

    M2M_CHECK_MGRSESS(hSess);

    if((pCfg->pattern_enc + pCfg->pattern_clr > M2M_PATTERNMODE_MAXVALUE)
        || (pCfg->pattern_enc > M2M_PATTERNMODE_MAXVALUE)
        || (pCfg->pattern_clr > M2M_PATTERNMODE_MAXVALUE)) {
        TA_LOGE( "enc+clr=0x%x should not be bigger than %d\n",
            pCfg->pattern_enc + pCfg->pattern_clr, M2M_PATTERNMODE_MAXVALUE);
        ret = ERR_ERRPARAM;
        goto error;
    }
    else {
        hSess->PatternCfg.pattern_enc = pCfg->pattern_enc;
        hSess->PatternCfg.pattern_clr = pCfg->pattern_clr;
        if(hSess->PatternCfg.pattern_clr) {
            hSess->eResidueType = M2MCRYPTO_RESIDUE_TYPE_PATTERN;
        }else {
            hSess->eResidueType = M2MCRYPTO_RESIDUE_TYPE_DEFAULT;
        }
    }

    TA_LOGH("SetPattern Successe\n");
    return SUCCESS;
error:
    TA_LOGF("SetPattern fail(ret=0x%x)\n", ret);
    return ret;
}

HRESULT M2M_TA_Core_Session_SetResidueMode(TA_M2M_SESS_HANDLE hSess,
                                        M2MCRYPTO_RESIDUE_TYPE eType)
{
    HRESULT ret = ERR_ERRPARAM;

    TA_LOGH( "SetResidueMode Enter\n");

    M2M_CHECK_MGRSESS(hSess);

    if((eType>=M2MCRYPTO_RESIDUE_TYPE_DEFAULT) && (eType<M2MCRYPTO_RESIDUE_TYPE_MAX)) {
        hSess->eResidueType = eType;
    }
    else {
        ret = ERR_ERRPARAM;
        TA_LOGF("SetResidueMode Wrong mode(=0x%x)\n", eType);
        goto error;
    }

    TA_LOGH("SetResidueMode Successe\n");
    return SUCCESS;
error:
    TA_LOGF("SetResidueMode fail(ret=0x%x)\n", ret);
    return ret;
}

HRESULT M2M_TA_Core_Session_SetIVPolicy(TA_M2M_SESS_HANDLE hSess,
                                        M2MCRYPTO_IV_POLICY ePolicy)
{
    HRESULT ret = ERR_ERRPARAM;

    TA_LOGH( "SetIVPolicy Enter\n");

    M2M_CHECK_MGRSESS(hSess);

    if((ePolicy>=M2MCRYPTO_IV_POLICY_DEFAULT) && (ePolicy<M2MCRYPTO_IV_POLICY_MAX)) {
        hSess->eIVPolicy = ePolicy;
    }
    else {
        ret = ERR_ERRPARAM;
        TA_LOGF("SetIVPolicy Wrong value(=0x%x)\n", ePolicy);
        goto error;
    }

    TA_LOGH("SetIVPolicy Successe\n");
    return SUCCESS;
error:
    TA_LOGF("SetIVPolicy fail(ret=0x%x)\n", ret);
    return ret;
}

#ifndef TA_COMPILE
HRESULT M2M_TA_Core_Session_Update(TA_M2M_SESS_HANDLE hSess,
                                   M2MCRYPTO_MEM_H hInput,
                                   M2MCRYPTO_MEM_H hOutput)
{
    HRESULT ret = ERR_ERRPARAM;
    M2M_TA_CRYPTO_CMD *pCryptoCmd = NULL;
    void *pPhyAddr;
    AMP_SHM_TYPE ShmType;

    UINT16 KeyTblMask = TSP_KEY_TABLE_BASE_ADDR_MASK;

    M2M_CHECK_MGRSESS(hSess);
    TA_LOGI("SESS[%d] Update enter\n", hSess->uSessId);

    if(internal_sess_secure_rule_checker(hSess, hInput, hOutput) == FALSE) {
        /*secure rule check fail*/
        ret = ERR_SECURE_RULE;
        TA_LOGF("SESS[%d]Secure rule check fail\n", hSess->uSessId);
        goto error;
    }
    pCryptoCmd = (M2M_TA_CRYPTO_CMD *)dmx_malloc(sizeof(M2M_TA_CRYPTO_CMD));
    if(pCryptoCmd == NULL) {
        ret = DMX_NULL_PTR;
        TA_LOGF("SESS[%d] Allocate node fail\n", hSess->uSessId);
        goto error;
    }
    dmx_zeromem(pCryptoCmd, sizeof(M2M_TA_CRYPTO_CMD));
    ret = internal_sess_scheme_mapping(hSess, hSess->CryptoConfig.Cryptoscheme, pCryptoCmd);
    DMX_CHECK_RETCODE(ret);

    if(hSess->CryptoConfig.KeyOption == M2MCRYPTO_KEY_CONTENT) {
        pCryptoCmd->uKeyAddr = hSess->CryptoConfig.KeyDTCM;
    }
    else if(hSess->CryptoConfig.KeyOption == M2MCRYPTO_KEY_INDEX) {
        switch(hSess->CryptoConfig.HWKeyArea)
        {
            case HW_KEYAREA_NAGRA:
                //32 bytes per Nagra key table, (32-keysize) offset stores key
                //Nagra needs to change TspOffset in DRMSE_KT MGR to x+(32-keysize),
                //then remove m2m +16 and tsp driver hardcode nagra offset
                KeyTblMask = NAGRA_KEY_TABLE_BASE_ADDR_MASK;
                break;
            case HW_KEYAREA_NSK2_M2M:
                KeyTblMask = NSK2_M2M_KEY_TABLE_BASE_ADDR_MASK;
                break;
            case HW_KEYAREA_IFCP_BRD:
            case HW_KEYAREA_IFCP_OTT:
            case HW_KEYAREA_IFCP_PVR:
                KeyTblMask = IFCP_KEY_TABLE_BASE_ADDR_MASK;
                break;
            case HW_KEYAREA_GKL:
                KeyTblMask = GKL_KEY_TABLE_BASE_ADDR_MASK;
                break;
            default:
                break;
        }
        if(DRM_KEY_TABLE_BASE_INVALID == KeyTblMask) {
           TA_LOGF("Product did not support the hardware key table(0x%x)!\n", hSess->CryptoConfig.HWKeyArea);
           return ERR_ERRPARAM;
        }
        pCryptoCmd->uKeyAddr = hSess->CryptoConfig.TSPKeyTblIndex|KeyTblMask;
    } else {
        ret = ERR_ERRPARAM;
        TA_LOGF("KeyOption not config(=0x%x)\n", hSess->CryptoConfig.KeyOption);
        goto error;
    }
    TA_LOGI("KeyTblMask = 0x%x\n", KeyTblMask);

    if(hSess->CryptoConfig.IVOption == M2MCRYPTO_KEY_CONTENT) {
        pCryptoCmd->uIVAddr = hSess->CryptoConfig.IVDTCM;
        if(hSess->eIVPolicy == M2MCRYPTO_IV_POLICY_RESET) {
             FIGODRV_HANDLE hFigoDrv = current_figohandle(hSess->uFigoSysId);
             M2M_TA_LoadDataToDTCM(hFigoDrv, hSess->CryptoConfig.IVDTCM,
                 hSess->CryptoConfig.InitIV, hSess->CryptoConfig.IVLength);
             TA_LOGM("reset IV to the init data\n");
        }
    }
    else if(hSess->CryptoConfig.IVOption == M2MCRYPTO_KEY_INDEX) {
        pCryptoCmd->uIVAddr = hSess->CryptoConfig.TSPIVTblIndex|KeyTblMask;
    } else {
        if(hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_AES_128_ECB  || \
            hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_DES_56_ECB  || \
            hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_TDES_128_ECB|| \
            hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_DVB_CSA_30  || \
            hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_ASA) {
              TA_LOGI("skip IV set in ECB and other mode\n");
        } else {
              ret = ERR_ERRPARAM;
              TA_LOGF("IVOption not config(=0x%x)\n", hSess->CryptoConfig.IVOption);
              goto error;
        }
    }

    ret = AMP_SHM_GetPhysicalAddress(hInput->hSHM, hInput->uOffset, (void **)&pPhyAddr);
    DMX_CHECK_RETCODE(ret);
    pCryptoCmd->InPhyAddr = pPhyAddr;
    pCryptoCmd->uInSize = hInput->uSize;
    dmx_mcopy(&pCryptoCmd->InMemObj, hInput, sizeof(M2MCRYPTO_MEMOBJ));

    /*Clean Cache*/
    AMP_SHM_GetStatus(hInput->hSHM, NULL, &ShmType, NULL);
    if(AMP_SHM_FLAG_SECURE & ShmType) {
        TA_LOGF("TZ not supported\n");
    }
    else {
       ret = AMP_SHM_CleanCache(hInput->hSHM,
                hInput->uOffset,
                hInput->uSize);
       DMX_CHECK_RETCODE(ret);
    }

    /*Invalid Cache*/
    AMP_SHM_GetStatus(hOutput->hSHM, NULL, &ShmType, NULL);
    if(AMP_SHM_FLAG_SECURE & ShmType) {
        TA_LOGF("TZ not supported\n");
    }
    else {
       ret = AMP_SHM_InvalidateCache(hOutput->hSHM,
                hOutput->uOffset,
                hOutput->uSize);
       DMX_CHECK_RETCODE(ret);
    }
    ret = AMP_SHM_GetPhysicalAddress(hOutput->hSHM, hOutput->uOffset, &pPhyAddr);
    DMX_CHECK_RETCODE(ret);
    pCryptoCmd->OutPhyAddr = pPhyAddr;
    pCryptoCmd->uOutSize = hOutput->uSize;
    dmx_mcopy(&pCryptoCmd->OutMemObj, hOutput, sizeof(M2MCRYPTO_MEMOBJ));
    pCryptoCmd->eState = SESS_CMD_STATE_LOADED;
    pCryptoCmd->hSess = (UINT32)hSess;
    pCryptoCmd->Tag= internal_GetCmdTag();
    pCryptoCmd->PatternCfg.pattern_enc = hSess->PatternCfg.pattern_enc;
    pCryptoCmd->PatternCfg.pattern_clr= hSess->PatternCfg.pattern_clr;
    pCryptoCmd->eResidueType = hSess->eResidueType;

    /*push to session cmd list*/
    if(hSess->pInCmdList != NULL) {
        DMX_LIST_PUSH_BACK(hSess->pInCmdList, pCryptoCmd);
    }
    else {
        ret = DMX_NULL_PTR;
        TA_LOGF("SESS[%d] pInCmdList is NULL\n", hSess->uSessId);
        goto error;
    }

    TA_LOGH("SESS[%d] Update[offset=0x%x, size=0x%x] success\n",
        hSess->uSessId, hInput->uOffset, hInput->uSize);

    return SUCCESS;
error:
    if(pCryptoCmd) {
        dmx_free(pCryptoCmd);
    }
    TA_LOGF("Update fail(ret=0x%x)\n", ret);
    return ret;

}
#else
HRESULT M2M_TA_Core_Session_Update(TA_M2M_SESS_HANDLE hSess,
                                   TA_M2M_BUFF_T *pInputBuf,
                                   TA_M2M_BUFF_T *pOutputBuf)
{
    HRESULT ret = ERR_ERRPARAM;
    M2M_TA_CRYPTO_CMD *pCryptoCmd = NULL;

    UINT16 KeyTblMask = TSP_KEY_TABLE_BASE_ADDR_MASK;

    M2M_CHECK_MGRSESS(hSess);
    TA_LOGH("SESS[%d] Update enter\n", hSess->uSessId);

    if(internal_sess_secure_rule_checker(hSess, pInputBuf, pOutputBuf) == FALSE) {
        /*secure rule check fail*/
        TA_LOGF("SESS[%p]Secure rule check fail\n", hSess);
        ret = ERR_SECURE_RULE;
        goto error;
    }
    pCryptoCmd = (M2M_TA_CRYPTO_CMD *)dmx_malloc(sizeof(M2M_TA_CRYPTO_CMD));
    if(pCryptoCmd == NULL) {
        ret = ERR_NULL_PTR;
        TA_LOGF("SESS[%d] Allocate node fail\n", hSess->uSessId);
        goto error;
    }

    //Met a crash using memset(pCryptoCmd, sizeof(M2M_TA_CRYPTO_CMD))
    memset(pCryptoCmd, 0, sizeof(M2M_TA_CRYPTO_CMD));
    ret = internal_sess_scheme_mapping(hSess, hSess->CryptoConfig.Cryptoscheme, pCryptoCmd);
    M2M_CHECK_RETCODE(ret);

    if(hSess->CryptoConfig.KeyOption == M2MCRYPTO_KEY_CONTENT) {
        pCryptoCmd->uKeyAddr = hSess->CryptoConfig.KeyDTCM;
    }
    else if(hSess->CryptoConfig.KeyOption == M2MCRYPTO_KEY_INDEX) {
        switch(hSess->CryptoConfig.HWKeyArea)
        {
            case HW_KEYAREA_NAGRA:
                //32 bytes per Nagra key table, (32-keysize) offset stores key
                //Nagra needs to change TspOffset in DRMSE_KT MGR to x+(32-keysize),
                //then remove m2m +16 and tsp driver hardcode nagra offset
                KeyTblMask = NAGRA_KEY_TABLE_BASE_ADDR_MASK;
                break;
            case HW_KEYAREA_NSK2_M2M:
                KeyTblMask = NSK2_M2M_KEY_TABLE_BASE_ADDR_MASK;
                break;
            case HW_KEYAREA_IFCP_BRD:
            case HW_KEYAREA_IFCP_OTT:
            case HW_KEYAREA_IFCP_PVR:
                KeyTblMask = IFCP_KEY_TABLE_BASE_ADDR_MASK;
                break;
            case HW_KEYAREA_GKL:
                KeyTblMask = GKL_KEY_TABLE_BASE_ADDR_MASK;
                break;
            default:
                break;
        }
        if(DRM_KEY_TABLE_BASE_INVALID == KeyTblMask) {
           TA_LOGF("Product did not support the hardware key table(0x%x)!\n", hSess->CryptoConfig.HWKeyArea);
           return ERR_ERRPARAM;
        }
        pCryptoCmd->uKeyAddr = hSess->CryptoConfig.TSPKeyTblIndex|KeyTblMask;
    } else {
        ret = ERR_ERRPARAM;
        TA_LOGF("KeyOption not config(=0x%x)\n", hSess->CryptoConfig.KeyOption);
        goto error;
    }
    TA_LOGI("KeyTblMask = 0x%x\n", KeyTblMask);

    if(hSess->CryptoConfig.IVOption == M2MCRYPTO_KEY_CONTENT) {
        pCryptoCmd->uIVAddr = hSess->CryptoConfig.IVDTCM;
        if(hSess->eIVPolicy == M2MCRYPTO_IV_POLICY_RESET) {
             FIGODRV_HANDLE hFigoDrv = current_figohandle(hSess->uFigoSysId);
             M2M_TA_LoadDataToDTCM(hFigoDrv, hSess->CryptoConfig.IVDTCM,
                 hSess->CryptoConfig.InitIV, hSess->CryptoConfig.IVLength);
             TA_LOGM("reset IV to the init data\n");
        }
    }
    else if(hSess->CryptoConfig.IVOption == M2MCRYPTO_KEY_INDEX) {
        pCryptoCmd->uIVAddr = hSess->CryptoConfig.TSPIVTblIndex|KeyTblMask;
    } else {
        if(hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_AES_128_ECB  || \
            hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_DES_56_ECB  || \
            hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_TDES_128_ECB|| \
            hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_DVB_CSA_30  || \
            hSess->CryptoConfig.Cryptoscheme == M2MCRYPTO_TYPE_ASA) {
              TA_LOGI("skip IV set in ECB and other mode\n");
        } else {
              ret = ERR_ERRPARAM;
              TA_LOGF("IVOption not config(=0x%x)\n", hSess->CryptoConfig.IVOption);
              goto error;
        }
    }

    pCryptoCmd->InPhyAddr = pInputBuf->phyAddr;
    pCryptoCmd->uInSize = pInputBuf->uBufSize;

    TA_LOGH("SESS[%d] Update enter, %p, 0x%x, %p, 0x%x\n", hSess->uSessId,    \
           pInputBuf->phyAddr, pInputBuf->uBufSize,
           pOutputBuf->phyAddr, pOutputBuf->uBufSize);

    TA_LOGH("%s: input mem %p\n", __FUNCTION__, pInputBuf->phyAddr);
    ret = TEE_FlushCache(pInputBuf->virAddr, pInputBuf->uBufSize);
    M2M_CHECK_RETCODE(ret);

    TA_LOGH("%s: output mem %p\n", __FUNCTION__, pOutputBuf->phyAddr);
    ret = TEE_InvalidateCache(pOutputBuf->virAddr, pOutputBuf->uBufSize);
    M2M_CHECK_RETCODE(ret);

    pCryptoCmd->OutPhyAddr = pOutputBuf->phyAddr;
    pCryptoCmd->uOutSize = pOutputBuf->uBufSize;
    pCryptoCmd->eState = SESS_CMD_STATE_LOADED;
    pCryptoCmd->hSess = (uintptr_t)hSess;
    pCryptoCmd->Tag= internal_GetCmdTag();
    pCryptoCmd->PatternCfg.pattern_enc = hSess->PatternCfg.pattern_enc;
    pCryptoCmd->PatternCfg.pattern_clr= hSess->PatternCfg.pattern_clr;
    pCryptoCmd->eResidueType = hSess->eResidueType;
    memcpy(&pCryptoCmd->inputBuf, pInputBuf, sizeof(TA_M2M_BUFF_T));
    memcpy(&pCryptoCmd->outputBuf, pOutputBuf, sizeof(TA_M2M_BUFF_T));

    /*push to session cmd list*/
    if(hSess->pInCmdList != NULL) {
        ret = DMX_LIST_PUSH_BACK(hSess->pInCmdList, pCryptoCmd);
        if(ret != SUCCESS) {
            TA_LOGF("SESS[%d] ret = 0x%x\n", hSess->uSessId, ret);
            goto error;
        }
    }
    else {
        ret = ERR_NULL_PTR;
        TA_LOGF("SESS[%d] pInCmdList is NULL\n", hSess->uSessId);
        goto error;
    }

    TA_LOGH("SESS[%d] Update[offset=%p, size=0x%x] success\n",
        hSess->uSessId, pInputBuf->phyAddr, pInputBuf->uBufSize);

    return SUCCESS;
error:
    if(pCryptoCmd) {
        dmx_free(pCryptoCmd);
    }
    TA_LOGF("Update fail(ret=0x%x)\n", ret);
    return ret;
}

#endif

HRESULT M2M_TA_Core_Session_Feed(TA_M2M_SESS_HANDLE hSess)
{
    HRESULT ret = ERR_ERRPARAM;
    M2M_TA_CRYPTO_CMD *pCryptoCmd = NULL;
    DMX_LIST_NODE* pCmdNode = NULL;

    M2M_CHECK_MGRSESS(hSess);

    pCmdNode = NULL;
    while((DMX_LIST_NEXT(hSess->pInCmdList, &pCmdNode) == SUCCESS)
        && (pCmdNode != NULL))
    {
        pCryptoCmd =(M2M_TA_CRYPTO_CMD *)pCmdNode->pData;
        if(pCryptoCmd->eState == SESS_CMD_STATE_LOADED) {
            ret = TSP_CryptoProcess(pCryptoCmd, hSess->uFigoSysId);
            if(ret == ERR_CMDQ_PUSHED) {
                ret = SUCCESS;
                if(pCryptoCmd->eState == SESS_CMD_STATE_LOADED) {
                    pCryptoCmd->eState = SESS_CMD_STATE_PUSHED;
                }
            }
            else {
                TA_LOGI("CMD Q is full\n");
            }
        }
    }

    return SUCCESS;
error:
    return ret;
}

HRESULT M2M_TA_Core_Session_Postprocess(TA_M2M_SESS_HANDLE hSess, FIGODRV_HANDLE  hFigoDrv)
{
    HRESULT ret = ERR_ERRPARAM;
    M2M_TA_CRYPTO_CMD *pCryptoCmd = NULL;
    DMX_LIST_NODE* pCmdNode = NULL;

    M2M_CHECK_MGRSESS(hSess);
    M2M_CHECK_NULL(hFigoDrv);
    ret = internal_CmdPostProcess(hFigoDrv);

    pCmdNode = NULL;
    hSess->uCmdFinishNum = 0;
    while((DMX_LIST_NEXT(hSess->pInCmdList, &pCmdNode) == SUCCESS) && (pCmdNode != NULL)) {
        pCryptoCmd =(M2M_TA_CRYPTO_CMD *)pCmdNode->pData;
        if(pCryptoCmd->eState == SESS_CMD_STATE_PROCESSED) {
            DMX_LIST_REMOVE(hSess->pInCmdList, pCmdNode);
#ifdef TA_COMPILE
            tspbuf_release(&pCryptoCmd->inputBuf);
            tspbuf_release(&pCryptoCmd->outputBuf);
#endif
            dmx_free(pCryptoCmd);
            pCmdNode = NULL;
            // judge is session has been processed or not
            hSess->uCmdFinishNum++;
        } else {
            break;
        }
    }
    if(hSess->uCmdFinishNum) {
        ret = TSPFW_Crypto_CheckCmdErrState(hFigoDrv);
    }
error:
    return ret;
}


#if 0
HRESULT M2M_TA_Core_Session_Routine(TA_M2M_SESS_HANDLE hSess)
{
    HRESULT ret = ERR_ERRPARAM;
    M2M_TA_CRYPTO_CMD *pCryptoCmd = NULL;
    DMX_LIST_NODE* pCmdNode = NULL;

    M2M_CHECK_MGRSESS(hSess);

    M2M_TA_Core_Session_Feed();
    M2M_TA_Core_Session_Postprocess();

    return SUCCESS;
error:
    return ret;
}
#endif

HRESULT M2M_TA_Session_Postprocess(TA_M2M_SESS_HANDLE hSess, UINT16 uCmdTag)
{
    HRESULT ret = ERR_ERRPARAM;
    M2M_TA_CRYPTO_CMD *pCryptoCmd = NULL;
    DMX_LIST_NODE* pCmdNode = NULL;

    M2M_CHECK_MGRSESS(hSess);

    pCmdNode = NULL;
    while((DMX_LIST_NEXT(hSess->pInCmdList, &pCmdNode) == SUCCESS) && (pCmdNode != NULL)) {
        pCryptoCmd =(M2M_TA_CRYPTO_CMD *)pCmdNode->pData;
        if((pCryptoCmd->eState == SESS_CMD_STATE_PUSHED) && (pCryptoCmd->Tag == uCmdTag))
        {
            pCryptoCmd->eState = SESS_CMD_STATE_PROCESSED;
            /*Cmd process is done, 1) INV cache; 2) dump the output data*/
            /*1. Invalidate Cache*/

    #ifndef TA_COMPILE
            AMP_SHM_TYPE ShmType;
            AMP_SHM_GetStatus(pCryptoCmd->OutMemObj.hSHM, NULL, &ShmType, NULL);
            if(AMP_SHM_FLAG_SECURE & ShmType) {
                 TA_LOGF("TZ not supported\n");
            }
            else {
               ret = AMP_SHM_InvalidateCache(pCryptoCmd->OutMemObj.hSHM,
                        pCryptoCmd->OutMemObj.uOffset,
                        pCryptoCmd->OutMemObj.uSize);
               M2M_CHECK_RETCODE(ret);
            }

    #else
            TA_LOGH("%s: out mem %p\n", __FUNCTION__, pCryptoCmd->OutPhyAddr);
            ret = TEE_InvalidateCache(pCryptoCmd->outputBuf.virAddr,pCryptoCmd->uOutSize);
            M2M_CHECK_RETCODE(ret);
    #endif

            /*2. dump the output data*/
            //internal_IODumptoFile(hSess, &pCryptoCmd->OutMemObj, hSess->fpOutData);
            break;
        }
    }

    return SUCCESS;
error:
    TA_LOGF("Post Process error ret=0x%x\n", ret);
    return ret;

}

