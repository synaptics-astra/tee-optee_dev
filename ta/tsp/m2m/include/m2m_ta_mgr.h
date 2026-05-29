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
//! \file m2m_ta_mgr.h
//!
//! \brief  This file is used to implement the ta session wrapper for m2m ca
//!
//! Author:         Long Xiao
//! Modified by:
//! Version:        1.0
//! Date:           Aug 4 2015
//!
///////////////////////////////////////////////////////////////////////////////
#ifndef __M2M_TA_MGR_H__
#define __M2M_TA_MGR_H__

#include "tsp_cmd.h"
#include "m2m_ta_comm.h"

#define DMX_KEY_MAX_LEN             (32)

#define M2M_TA_CAP_SESSION_NR       256

#define M2M_TA_SESSION_MAGIC       ((0x99ABCDEF))

#define M2MCRYPTO_KEY_USAGE_ENC     0x1
#define M2MCRYPTO_KEY_USAGE_DEC     0x2

typedef enum __aes_param_mode__ {
    AES_PARAM_MODE_ECB      = 0x0,
    AES_PARAM_MODE_CBC      = 0x1,
    AES_PARAM_MODE_OFB      = 0x2,
    AES_PARAM_MODE_CTR      = 0x3,
    AES_PARAM_MODE_RCBC     = 0x4,
    AES_PARAM_MODE_CTR64    = 0x5,
    AES_PARAM_MODE_CTR128   = 0x6
} AES_PARAM_MODE;

typedef enum __aes_param_keylen__ {
    AES_PARAM_KEYLEN_128      = 0x0,
    AES_PARAM_KEYLEN_192      = 0x1,
    AES_PARAM_KEYLEN_256      = 0x2,
} AES_PARAM_KEYLEN;

typedef enum __des_param_mode__ {
    DES_PARAM_MODE_ECB      = 0x0,
    DES_PARAM_MODE_CBC      = 0x1,
    DES_PARAM_MODE_OFB      = 0x2,
    DES_PARAM_MODE_CTR      = 0x3,
} DES_PARAM_MODE;

typedef enum __des_param_keylen__ {
    DES_PARAM_KEYLEN          = 0x0,
    TDES_PARAM_KEYLEN         = 0x1,
    TDES_PARAM_KEYLEN_128     = 0x2,
} DES_PARAM_KEYLEN_T;

typedef enum __dvbcsa_nsk2_sbox_table__ {
    DVBCSA_NSK2_SBOX_TABLE      = 0x1,
} DVBCSA_NSK2_SBOX_TABLE_T;

typedef enum __dvbcsa_conformance_onoff__ {
    DVBCSA_CONFORMANCE_OFF      = 0x0,
    DVBCSA_CONFORMANCE_ON       = 0x1,
} DVBCSA_CONFORMANCE;

typedef struct __sess_crypto_cfg__ {
    M2MCRYPTO_TYPE Cryptoscheme;
    M2MCRYPTO_KEY_OPTION  KeyOption;
    M2MCRYPTO_KEY_OPTION  IVOption;
    UINT16          KeyDTCM;
    UINT32          KeyLength;
    UINT16          IVDTCM;
    UINT32          IVLength;
    // This is the key offset that crypto engine uses
    // There are totally 256 entries in the TSP key table
    UINT16          TSPKeyTblIndex;
    UINT16          TSPIVTblIndex;
    UINT32 HWKeyArea;
    UINT32 bProtected;
    //for IV Policy
    UINT8  InitIV[16];
} SESSION_CRYPTO_CFG;

typedef struct __ta_m2m_session__
{
    UINT32              uMagicNum;
    UINT32              uSessId;
    UINT32              isActive:1;
    UINT32              uFigoSysId;
    M2MCRYPTO_SESSION_TYPE  eType;
    bool                bDummy;
    void                *SessionMutexLock;
    DMX_LIST            *pInCmdList;
    SESSION_CRYPTO_CFG   CryptoConfig;
    BOOL                 bAPIBlocking;
    //! sess Cmd execution is serialised
    UINT32               uSessCmdExecFlag;
    //! reference to crypto server
    VOID                 *pServer;
    //!for pattern mode
    M2MCRYPTO_PATTERN_MODE_CFG PatternCfg;
    //!for residue mode
    M2MCRYPTO_RESIDUE_TYPE  eResidueType;
    //!for IV Policy
    M2MCRYPTO_IV_POLICY  eIVPolicy;

    UINT32               uCmdFinishNum;

    //! Debug context
    UINT32               uHeartBeat;

    VOID                 *pTempVirtAddr;
    VOID                 *pTempPhyAddr;
    UINT32               TempBufferSize;

}TA_M2M_SESS_T, *TA_M2M_SESS_HANDLE;

UINT32 internal_sess_callback(TA_M2M_SESS_HANDLE hSess, UINT16 Tag);

typedef struct __ta_m2m_crypto_srv__ {
     void*          MutexLock;
     UINT32         uSessRefCnt;
     UINT32         uFigoSessRefCnt[TSP_FIGO_NR]; //session cnt running at each figo
     TA_M2M_SESS_T  MgrSessTable[M2M_TA_CAP_SESSION_NR];
     FIGODRV_HANDLE  ufigohdl[TSP_FIGO_NR];
 #ifndef TA_COMPILE
     void*          pTSPM;
 #else
     TA_M2M_BUFF_T   CmdBaseBuf[TSP_FIGO_NR];
     TA_M2M_BUFF_T   BIUImageBuf[TSP_FIGO_NR];
 #endif
}TA_M2M_CRYPTO_T, *TA_M2M_CRYPTO_H;

extern TA_M2M_CRYPTO_T gCryptoSrv;
#define M2M_CHECK_MGRSESS(pSess)                               \
    do{                                                         \
        if (((uintptr_t)(pSess) < (uintptr_t)(&(gCryptoSrv.MgrSessTable[0]))) ||   \
            ((uintptr_t)(pSess) > (uintptr_t)(&(gCryptoSrv.MgrSessTable[M2M_TA_CAP_SESSION_NR-1]))) || \
            ((pSess)->uMagicNum != M2M_TA_SESSION_MAGIC) ||          \
            ((pSess)->isActive != 1))                             \
        {                                                       \
            TA_LOGF( "Invalid session(pSess=%p) handle @ %s() %d\n", pSess, __FUNCTION__, __LINE__);  \
            ret = ERR_ERRPARAM;                                 \
            goto error;                                         \
        }                                                       \
    }while(0)

#define current_figohandle(uFigoId)     (((uFigoId) < TSP_FIGO_NR) ? (gCryptoSrv.ufigohdl[uFigoId]): NULL)

#define M2M_CHECK_RETCODE(ret)  if ((ret) != SUCCESS)     \
                        {TA_LOGF( "Err:%x @Line:%d\n", (uint32_t)ret, __LINE__);goto error;}
#define M2M_CHECK_NULL(ptr)    if ((ptr) == 0)          \
                        {ret = ERR_NULL_PTR;goto error;}

#endif

