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
//! \file m2m_crypto_types.h
//!
//! \brief This file define all common data structure, enum and API for M2M
//!        crypto.
//!
//! Purpose:
//!
//!
//! Version, Date and Author :
//!   V 1.00,    March 22 2013,    Yongquan Jia
//!
//! Note: This file owner is Yongquan.
////////////////////////////////////////////////////////////////////////////////
#ifndef __M2M_CRYPTO_TYPES_H__
#define __M2M_CRYPTO_TYPES_H__

#ifndef CONFIG_OPTEE
#include "types.h"
#endif

/** \enum M2M_DRMSCHEME_TYPE
 *  \brief m2m drm scheme type definition.
 */
typedef enum {
    M2M_DRMSCHEME_INVALID,      /**< Invalid type */
    M2M_DRMSCHEME_HDCP2X,       /**< HDCP 2.0 */
    M2M_DRMSCHEME_DTCPIP,       /**< DTCP on IP */
    M2M_DRMSCHEME_CIPLUS,       /**< CI+ */
    M2M_DRMSCHEME_WIDEVINE,     /**< WideVine */
    M2M_DRMSCHEME_PLAYREADY,    /**< Playready */
    M2M_DRMSCHEME_VUDU,         /**< VUDU */
    M2M_DRMSCHEME_VRMTX,        /**< VRMTX */
    M2M_DRMSCHEME_NDS,          /**< SYNAMEDIA*/
    M2M_DRMSCHEME_MLA,          /**< MARLIN Audio */
    M2M_DRMSCHEME_MOBITV,        /**< MOBITV */
    M2M_DRMSCHEME_PVR_C0,        /**< PVR_C0 */
    M2M_DRMSCHEME_DUMMY,        /**< DUMMY */
    M2M_DRMSCHEME_STUB,         /**< STUB */
    M2M_DRMSCHEME_PVR_GENERAL,  /**< PVR_GENERAL */
    M2M_DRMSCHEME_MARLIN,       /**< MARLIN */
    M2M_DRMSCHEME_RAW,          /**< RAW */
    M2M_DRMSCHEME_NAGRA,         /**< NAGRA */
    M2M_DRMSCHEME_CENC3,        /**< CENC3 */
    M2M_DRMSCHEME_ALTICAST,     /**< ALTICAST */
    M2M_DRMSCHEME_IRDETO,       /**< IRDETO */
    M2M_DRMSCHEME_GKL,          /**< GKL */

    M2M_DRMSCHEME_CUSTOMER_BASE = 100, /**< customer define DRM scheme, >100 */
} M2M_DRMSCHEME_TYPE;

/** \def KEY_ID_MAX_LEN
    \brief This macro function used to define key ID max length.
 */
#define KEY_ID_MAX_LEN              (48)

/** \typedef M2M_DRM_CRYPTO_ST
    \brief Type definition for AMP CRYPTO structure
 */
typedef struct {
    M2M_DRMSCHEME_TYPE  uiSchemeType;           /**< The Scheme type of DRM */
    uint32_t  uiSessionID;            /**< The identify of Session */
    uint8_t   uiKeyID[KEY_ID_MAX_LEN];/**< The identify Key */
    uint32_t  uiKeyIdLen;             /**< The length of key id */
} M2M_DRM_CRYPTO_ST;

// In case caller TA not change
typedef M2M_DRM_CRYPTO_ST M2M_CRYPTO_ST;

#endif  /* __M2M_CRYPTO_TYPES_H__ */
