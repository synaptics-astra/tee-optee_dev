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

////////////////////////////////////////////////////////////////////////////////
//! \file m2m_ta_drv.h
//!
//! \brief This file is used to defnine some common data type for tsp crypto lib.
//!
//!
//!     Version    Date                Author
//!     V 1.00,    Aug 4 2015,        Long Xiao
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
#ifndef __M2M_TA_DRV_H__
#define __M2M_TA_DRV_H__

#include "m2m_ta_comm.h"

typedef HRESULT (*TSP_CRYPTO_CB)(UINT32 handle, UINT16 Tag);

typedef enum __sess_cmd_state__ {
  SESS_CMD_STATE_INVALID    = 0x0,
  SESS_CMD_STATE_LOADED     = 0x1,
  SESS_CMD_STATE_PUSHED     = 0x2,
  SESS_CMD_STATE_PROCESSED  = 0x3,
  SESS_CMD_STATE_MAX
} SESS_CMD_STATE;

typedef enum __sess_crypto_type__ {
    SESS_CRYPTO_TYPE_COPY           = 0x0,      /**< COPY */
    SESS_CRYPTO_TYPE_CRC            = 0x1,      /**< CRC */
    SESS_CRYPTO_TYPE_INVDVBCSA2     = 0x2,      /**< INVDVBCSA2 */
    SESS_CRYPTO_TYPE_INVDVBCSA3     = 0x3,      /**< INVDVBCSA3 */
    SESS_CRYPTO_TYPE_AES            = 0x4,      /**< AES */
    SESS_CRYPTO_TYPE_INVAES         = 0x5,      /**< INVAES */
    SESS_CRYPTO_TYPE_TDES           = 0x6,      /**< TDES */
    SESS_CRYPTO_TYPE_INVTDES        = 0x7,      /**< INVTDES */
    SESS_CRYPTO_TYPE_C2             = 0x8,      /**< C2 */
    SESS_CRYPTO_TYPE_INVC2          = 0x9,      /**< INVC2 */
    SESS_CRYPTO_TYPE_WMMAC          = 0xa,      /**< WMMAC */
    SESS_CRYPTO_TYPE_INVWMMAC       = 0xb,      /**< INVWMMAC */
    SESS_CRYPTO_TYPE_ARIBMULTI2     = 0xc,      /**< ARIBMULTI2 */
    SESS_CRYPTO_TYPE_INVARIBMULTI2  = 0xd,      /**< INVARIBMULTI2 */
    SESS_CRYPTO_TYPE_RC4            = 0xe,      /**< RC4 */
    SESS_CRYPTO_TYPE_CSS            = 0xf,      /**< CSS */
    SESS_CRYPTO_TYPE_ASA            = 0x10,     /**< ASA */
    SESS_CRYPTO_TYPE_MAX                        /**< Max value */
} SESS_CRYPTO_TYPE;


typedef struct __sess_crypto_cmd__ {
    UINT32          hSess;
    SESS_CMD_STATE  eState;
    UINT16          Tag;
    VOID            *InPhyAddr;
    UINT32          uInSize;
    VOID            *OutPhyAddr;
    UINT32          uOutSize;
#ifndef TA_COMPILE
    M2MCRYPTO_MEMOBJ InMemObj;
    M2MCRYPTO_MEMOBJ OutMemObj;
#else
    TA_M2M_BUFF_T    inputBuf;
    TA_M2M_BUFF_T    outputBuf;
#endif
    M2MCRYPTO_SESSION_TYPE eInputSessType;
    M2MCRYPTO_TYPE eInputCryptoType;
    SESS_CRYPTO_TYPE eCryptoScheme;
    UINT32          uCryptoParam;
    UINT32          uKeyAddr;
    UINT32          uIVAddr;

    //!for pattern mode
    M2MCRYPTO_PATTERN_MODE_CFG PatternCfg;
    //!for residue mode
    M2MCRYPTO_RESIDUE_TYPE  eResidueType;
    //! Debug context
} M2M_TA_CRYPTO_CMD;

typedef enum __tsp_cmd_group_state__ {
    TSP_CMD_GROUP_LOADED    = 0x0,
    TSP_CMD_GROUP_PUSHING   = 0x1,
    TSP_CMD_GROUP_PUSHED    = 0x2,
    TSP_CMD_GROUP_DONE      = 0x4,
} TSP_CMD_GROUP_STATE;

typedef enum __tsp_group_push_ret__ {
    TSP_GROUP_PUSH_DONE     = 0x0,
    TSP_GROUP_PUSH_QFULL     = 0x1,
    TSP_GROUP_PUSH_UNEXPECT  = 0x2
} TSP_GROUP_PUSH_RET;

typedef struct __tsp_crypto_cmd_group__ {
    UINT32                  hSess;
    UINT32                  ufigoId;
    UINT16                  uiGroupID;
    TSP_CMD_GROUP_STATE     eState;
    UINT32                  uiCmdNum;
    UINT32                  uFirstCmdPos;   /*The first cmd offset from base in SHM.*/
    UINT32                  uLastCmdPos;    /*The last cmd offset from base in SHM.*/
} TSP_CRYPTO_CMD_GROUP;

typedef struct __tsp_crypto_cmd_q__ {
    UINT32          hCmdMem;
    void            *pCmdBaseVirtAddr;
    void            *pCmdBasePhyAddr;
    UINT32          uSize;
    UINT32          uCmdQWrPos;     /*The write offset from base in SHM.*/
} TSP_CRYPTO_CMD_Q;

typedef struct __tsp_crypto_cfg__ {
    UINT32      uiMaxCmdNumInPool;
    UINT32      uiMaxBlkSize;
    BOOL        bInited;
    UINT32      uFigoSysId;
    INT         uTaskId;
    UINT32      uFWIdx;
} TSP_CRYPTO_CFG;

typedef struct __tsp_crypto_lib__ {
    DMX_LIST          *pGroupList;    /*The node is GROUP*/
    TSP_CRYPTO_CB      pCallback;
    TSP_CRYPTO_CFG     Libcfg[TSP_FIGO_NR];
    TSP_CRYPTO_CMD_Q   CmdQ[TSP_FIGO_NR];
}TSP_CRYPTO_LIB;

HRESULT internal_CmdPostProcess(FIGODRV_HANDLE  hFigoDrv);
HRESULT TSP_CryptoInit(void *pCallback, int32_t uFigoId, void *pCmdBaseBuf, UINT16 uCmdQueueBase,
    UINT16  uCmdQueueSize, UINT16 uCmdCntBase, UINT16 uLocalDataIn, UINT16 uLocalDataOut, UINT16 uM2MCryptoCmdBase);
HRESULT TSP_CryptoDeinit(int32_t uFigoId);
HRESULT TSP_CryptoAllocFigoTask(int32_t uFigoId);
HRESULT TSP_CryptoFreeFigoTask(int32_t uFigoId);
HRESULT TSP_CryptoStartFw(int32_t *pFigoId);
HRESULT TSP_CryptoStopFw(int32_t uFigoId);
HRESULT TSP_CryptoProcess(M2M_TA_CRYPTO_CMD *pDatChunkCmd, int32_t uFigoId);

#endif

