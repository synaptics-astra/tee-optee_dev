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
//! \file drm_contentkey.h
//! \brief Header file for DRM Content key
///////////////////////////////////////////////////////////////////////////////

#ifndef __DRM_CONTENTKEY_H__
#define __DRM_CONTENTKEY_H__

#include "drm_types.h"
#include "drm_list.h"
#include "drm_scheme.h"

#define DRM_HWKEYINDEX_INVALID      ((UINT32) -1)

typedef enum
{
    DRMCKTYPE_INAVLID,
    DRMCKTYPE_AES,
    DRMCKTYPE_DES,
    DRMCKTYPE_TDES,
    DRMCKTYPE_DVBCSA2,
    DRMCKTYPE_DVBCSA2_CONFORMANCE,
    DRMCKTYPE_DVBCSA3,
    DRMCKTYPE_MAX,
}MV_DRMContentKeyAlgType;

typedef enum
{
    DRMCKMODE_INVALID,
    DRMCKMODE_ECB,
    DRMCKMODE_CBC,
    DRMCKMODE_OFB,
    DRMCKMODE_CTR,
    DRMCKMODE_CTR64,
    DRMCKMODE_CTR128,
    DRMCKMODE_MAX,
} MV_DRMContentKeyAlgMode;

typedef enum
{
    DRMCKVTYPE_INVALID,
    DRMCKVTYPE_RAWDATA,
    DRMCKVTYPE_HWINDEX,
    DRMCKVTYPE_MAX,
} MV_DRMContentKeyValueType;

//The mode for TS residual and solitary
typedef enum
{
    RESIDUALMODE_INVALID,
    RESIDUALMODE_NO,
    RESIDUALMODE_CLR,
    RESIDUALMODE_CTS,
    RESIDUALMODE_RP,
    RESIDUALMODE_EXOR,
    RESIDUALMODE_MAX,
} MV_DRMResidualMode;

typedef enum
{
    DRMCKPOLARITY_INVALID,
    DRMCKPOLARITY_EVEN,
    DRMCKPOLARITY_ODD,
    DRMCKPOLARITY_MAX,
} MV_DRMContentKeyPolarity;

typedef enum {
    DRMKEYUSAGE_INVALID,
    DRMKEYUSAGE_DEC,
    DRMKEYUSAGE_ENC,
    DRMKEYUSAGE_MAX,
} MV_DRMKEYUSAGE;

#define MV_HW_KEYAREA_GENERAL   0
#define MV_HW_KEYAREA_CASEC     1

typedef struct
{
    UINT32 e_AlgType;
    UINT32 e_AlgMode;
    UINT32 e_ResidualMode;
    UINT32 e_SolitaryMode;
    UINT32 e_Polarity;
    UINT32 e_ValueType;
    UINT32 e_IVType;
    UINT32 e_KeyUsage;
    UINT32 uKeyIDLen;
    UINT32 uHWKeyIndex;
    UINT32 uHWIVIndex;
    UINT32 uKeyDataLen;
    UINT32 uIVLen;
    UINT32 uHWKeyArea;
}MV_DRMContentKeyUnitAttribute;

typedef struct MV_DRMContentKeyUnit MV_DRMContentKeyUnit;
struct MV_DRMContentKeyUnit
{
    MV_DRMListNode super;
    MV_DRMContentKeyAlgType e_AlgType;
    MV_DRMContentKeyAlgMode e_AlgMode;
    MV_DRMResidualMode e_ResidualMode;
    MV_DRMResidualMode e_SolitaryMode;
    MV_DRMContentKeyPolarity e_Polarity;
    MV_DRMContentKeyValueType e_ValueType;
    MV_DRMContentKeyValueType e_IVType;
    MV_DRMKEYUSAGE e_KeyUsage;
    UINT8 *pbKeyID;
    UINT32  uKeyIDLen;
    UINT32 uHWKeyIndex;
    UINT32 uHWIVIndex;
    UINT32 uKeyDataLen;
    UINT8 *bKeyData;
    UINT32 uIVLen;
    UINT8 *bIVData;
    UINT32 uHWKeyArea;
    UINT32 uKeyUnitID;
    UINT8 *(*getContentKeyInfo)(MV_DRMContentKeyUnit *t);
    MV_DRMContentKeyAlgType (*getContentKeyAlgType)(MV_DRMContentKeyUnit *t);
    MV_DRMContentKeyAlgMode (*getContentKeyAlgMode)(MV_DRMContentKeyUnit *t);
    HRESULT (*getContentKeyIndex)(MV_DRMContentKeyUnit *t, UINT8 *pbKeyID,
            UINT32 *uKeyIdxLen);
    MV_DRMContentKeyValueType (*getContentKeyValueType)(MV_DRMContentKeyUnit *t);
    HRESULT (*getContentKeyHWIndex)(MV_DRMContentKeyUnit *t, UINT32 *uHWIndex);
    HRESULT (*getContentKeyRawData)(MV_DRMContentKeyUnit *t, UINT8 *bBuf,
            UINT32 *uKeyLen);
    HRESULT (*getContentKeyIVData)(MV_DRMContentKeyUnit *t, UINT8 *bBuf,
            UINT32 *uIVLen);
};

typedef struct MV_DRMContentKeys MV_DRMContentKeys;
struct MV_DRMContentKeys
{
    MV_DRMContentKeyUnit o_ContentKeyListHead;
    MV_DRMSchemeType o_DRMSchemeType;
    UINT32 uSessionID;
    //The total key unit numbers in the 'key unit' link list;
    UINT32 uContentKeyCount;
    HRESULT (*getContentKeyUnitbyKeyID)(MV_DRMContentKeys *t,
            UINT8 *pKeyID, UINT32 uKeyIDLen,
            MV_DRMContentKeyUnit **pKeyUnit);
    HRESULT (*delContentKeyUnitbyKeyID)(MV_DRMContentKeys *t,
            UINT8 *pKeyID, UINT32 uKeyIDLen);
    HRESULT (*getContentKeyUnitbyKeyIndex)(MV_DRMContentKeys *t,
            UINT32 uKeyIndex,  MV_DRMContentKeyUnit **pKeyUnit);
    HRESULT (*getContentKeyUnitbyKeyUnitID)(MV_DRMContentKeys *t,
        UINT32 uKeyIdentity,  MV_DRMContentKeyUnit **pKeyUnit);
    //assign uKeyUnitCounter to key unit Identity when create a new key unit, monotonically increasing;
    UINT32 uKeyUnitCounter;
};

typedef struct MV_DRMSessionCKSet MV_DRMSessionCKSet;
struct MV_DRMSessionCKSet
{
    MV_DRMContentKeys * pSessionKeys;
};

#endif /* __DRM_CONTENTKEY_H__ */
