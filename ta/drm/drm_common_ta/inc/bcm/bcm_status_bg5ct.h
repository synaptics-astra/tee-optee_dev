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

#ifndef INCLUDE_BCM_STATUS_BG5CT_H
#define INCLUDE_BCM_STATUS_BG5CT_H

typedef enum _ASN1_STATUS
{
    STATUS_ASN1_INVALID_FORMAT = 0x01000000,
} ASN1_STATUS;

typedef enum _SSTORE_STATUS
{
    STATUS_SSTORE_INVALID_MAGIC = 0x02000000,
    STATUS_SSTORE_SHORT_PAYLOAD_BUF,
    STATUS_SSTORE_INVALID_ROOTK,
    STATUS_SSTORE_INVALID_DIGEST,
    STATUS_SSTORE_INVALID_MARKET,
    STATUS_SSTORE_INVALID_VERSION,
    STATUS_SSTORE_INVALID_HASH,
    STATUS_SSTORE_INVALID_PAYLOAD_SIZE,
    STATUS_SSTORE_INVALID_SSTORE_ID,
} SSTORE_STATUS;

typedef enum _MISC_STATUS
{
    STATUS_MISC_INVALID_PARAMETER = 0x03000000,
    STATUS_MISC_ILLEGAL_KEY,
    STATUS_MISC_PERMISSION_DENIED,
} MISC_STATUS;

typedef enum _WIDEVINE_STATUS
{
    STATUS_WV_INVALID_SESSION = 0x04000000,
    STATUS_WV_TOO_MANY_SESSIONS,
    STATUS_WV_NO_DEVICE_KEY,
    STATUS_WV_INVALID_PARAM,
    STATUS_WV_SIGNATURE_FAILURE,
    STATUS_WV_INVALID_NONCE,
    STATUS_WV_TOO_MANY_KEYS,
    STATUS_WV_SHORT_BUFFER,
    STATUS_WV_BAD_MAGIC,
    STATUS_WV_BAD_CRC,
    STATUS_WV_VERIFY_FAIL,
    STATUS_WV_INVALID_RSA_KEY,
} WIDEVINE_STATUS;

typedef enum _GTVCA_STATUS
{
    STATUS_GTV_CA_INVALID_PARAM = 0x05000000,
    STATUS_GTV_CA_INVALID_KEY,
    STATUS_GTV_CA_NOT_IMPL,
} GTVCA_STATUS;

#endif //INCLUDE_BCM_STATUS_BG5CT_H
