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
//! \file amp_m2mcrypto_api.h
//!
//! \brief This file define all common data structure, enum and API for
//!        AMP M2M Crypto.
//!
//!
//!     Version     Date                    Author
//!     V 1.00,     Apr. 1 2014,       Weizhao Jiang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
#ifndef __M2M_CRYPTO_API_H__
#define __M2M_CRYPTO_API_H__
///////////////////////////////////////////////////////////////////////////////
//! [Part1] Header file include
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//! [Part2] Local macros, type definitions
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/** \struct _M2MCRYPTO_MEMOBJ
 *  \typedef M2MCRYPTO_MEM_H
 *  \brief The structure of memory handle to indicate both AMP SHM handle/offset
 *         and size. It will be used in API SyncUpdate()/AsyncUpdate() to
 *         indicate the IO data block in SHM.
 */
typedef struct __m2m_mem_handle__ {
    uint32_t    hShm_memid;
    uint32_t    uOffset;
    uint32_t    uSize;
} M2MCRYPTO_MEMOBJ, *M2MCRYPTO_MEM_H;

typedef struct {
    uint32_t  uiKeyOption;
    uint32_t  uiKeyLength;
    uint32_t  uiKeyTblIndex;
    uint32_t  uiIVOption;
    uint32_t  uiIVLength;
    uint32_t  uiIVTblIndex;
} M2M_KEY_INFO_ST;

/** \enum M2MCRYPTO_KEY_OPTION
 *  \brief The key option is indicating what's the key and where it is from.
 *         The option will be used case by case in SetKey().
 */
typedef enum __m2m_key_option__ {
        M2MCRYPTO_KEY_INVALID       = 0x0,      /**< invalid case*/
        M2MCRYPTO_KEY_CONTENT       = 0x1,      /**< Content key from Apps*/
        M2MCRYPTO_KEY_INDEX         = 0x2,      /**< Key table index from Apps*/
        M2MCRYPTO_KEY_FROM_DRM      = 0x3,      /**< Key from DRM*/
        M2MCRYPTO_KEY_MAX,
} M2MCRYPTO_KEY_OPTION;

/** \enum M2MCRYPTO_TYPE
 *  \brief To enumerate all the dmx crypto type.
 */
typedef enum __m2m_crypto_type__ {
        M2MCRYPTO_TYPE_INVALID        = 0x0,      /**< Invalid case*/
        M2MCRYPTO_TYPE_AES_128_ECB    = 0x1,      /**< for AES_128_ECB case */
        M2MCRYPTO_TYPE_AES_128_CBC    = 0x2,      /**< for AES_128_CBC case */
        M2MCRYPTO_TYPE_AES_128_CTR    = 0x3,      /**< for AES_128_CTR case */
        M2MCRYPTO_TYPE_TDES_128_ECB   = 0x4,      /**< for TDES_128_ECB case */
        M2MCRYPTO_TYPE_TDES_128_CBC   = 0x5,      /**< for TDES_128_CBC case */
        M2MCRYPTO_TYPE_TDES_128_CTR   = 0x6,      /**< for TDES_128_CTR case */
        M2MCRYPTO_TYPE_DVB_CSA_30     = 0x7,      /**< for DVB_CSA_30 case */
        M2MCRYPTO_TYPE_ASA            = 0x8,      /**< for ASA case */
        M2MCRYPTO_TYPE_DES_56_ECB     = 0x9,      /**< for DES_56_ECB case */
        M2MCRYPTO_TYPE_DVB_CSA_20     = 0xa,      /**< for DVB_CSA_20 case */
        M2MCRYPTO_TYPE_AES_128_CTR64  = 0xb,      /**< for AES_128_CTR64 case */
        M2MCRYPTO_TYPE_AES_128_CTR128 = 0xc,      /**< for AES_128_CTR128 case */
        M2MCRYPTO_TYPE_MAX                        /**< Max value */
} M2MCRYPTO_TYPE;

/** \enum M2MCRYPTO_SESSION_TYPE
 *  \brief To enumerate all the dmx crypto type.
 */
typedef enum __m2m_sess_type__ {
        M2MCRYPTO_SESSION_TYPE_INVALID= 0x0,    /**< Invalid case*/
        M2MCRYPTO_SESSION_TYPE_ENC    = 0x1,    /**< for encryption session */
        M2MCRYPTO_SESSION_TYPE_DEC    = 0x2,    /**< for decryption session */
        M2MCRYPTO_SESSION_TYPE_MAX              /**< Max value */
} M2MCRYPTO_SESSION_TYPE;

/** \enum M2MCRYPTO_PATTERN_MODE_CFG
 *  \brief The structure of M2M pattern mode, includes the proporitons of encrypted/clear data
 */
typedef struct __m2m_pattern_mode_cfg {
        unsigned char      pattern_enc;                           /**< The ratio of encrypted data */
        unsigned char      pattern_clr;                            /**< The ratio of clear data */
} M2MCRYPTO_PATTERN_MODE_CFG;

/** \enum M2MCRYPTO_RESIDUE_TYPE
*  \brief all the residue types which M2M can support.
*/
typedef enum M2M_RESIDUE_TYPE {
        M2MCRYPTO_RESIDUE_TYPE_DEFAULT   =0x0,    /**< for CBC the default type is clear , for CTR it is encrypted */
        M2MCRYPTO_RESIDUE_TYPE_PATTERN   =0x1,    /**<  for pattern mode */
        M2MCRYPTO_RESIDUE_TYPE_EXOR      =0x2,    /**< do exor operation, vudu/ATIS use this */
        M2MCRYPTO_RESIDUE_TYPE_RP        =0x3,    /**< for marlin case */
        M2MCRYPTO_RESIDUE_TYPE_CTS       =0x4,    /**< widevine legacy use cts */
        M2MCRYPTO_RESIDUE_TYPE_MAX                /**< Number of total types. */
} M2MCRYPTO_RESIDUE_TYPE;

typedef enum M2M_IV_POLICY {
        M2MCRYPTO_IV_POLICY_DEFAULT   =0x0,    /**< the default iv policy is writeback*/
        M2MCRYPTO_IV_POLICY_RESET     =0x1,    /**< reset iv for each update*/
        M2MCRYPTO_IV_POLICY_MAX                /**< Number of total types. */
} M2MCRYPTO_IV_POLICY;

#ifdef __cplusplus
}
#endif

#endif  /*__M2M_CRYPTO_API_H__*/


