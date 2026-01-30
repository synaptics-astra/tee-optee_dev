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
//! \file tsp_nsk.h
//!
//! \brief TSP NSK header file
//!
//!
//!     Version    Date                     Author
//!     V 0.1,     July 31 2020             Jie Xi
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "tee_internal_api.h"

#ifndef _TEE_TSP_NSK_H_
#define _TEE_TSP_NSK_H_

#define LOCAL_D_MASK       0x1000000
#define ESA_MASK           0x2000000
#define LOCAL_S_MASK       0x4000000

#define EVEN_PARITY_MASK   0x10000
#define ODD_PARITY_MASK    0x20000
#define CLEAR_PARITY_MASK  0x40000

#define KEY_INDEX_MASK     0x0001
#define IV1_MASK           0x0002
#define IV2_MASK           0x0004
#define ALGO_MASK          0x0008
#define SCB_OUT_MASK       0x0010

#define SCB_OUT_EVEN       0x2
#define SCB_OUT_ODD        0x3
#define SCB_OUT_CLEAR      0x0
#define IV_LENGTH          16

typedef struct {
    // Indicate which fields are valid.
    // The valid masks are IV1_MASK, IV2_MASK,
    // ALGO_MASK and their combinations.
    unsigned int validMask;
    unsigned int algoId;
    unsigned char iv1[IV_LENGTH];
    unsigned char iv2[IV_LENGTH];
} NSKOneCrypto;

typedef struct {
    // Indicate which fields are valid.
    // The valid masks are LOCAL_D_MASK,
    // ESA_MASK, LOCAL_S_MASK, KEY_INDEX_MASK, SCB_OUT_MASK
    // and their combinations.
    unsigned int validMask;
    unsigned int keyIdx;
    unsigned int scbOut;
    NSKOneCrypto localD;
    NSKOneCrypto ESA;
    NSKOneCrypto localS;
} NSKOneParity;

typedef struct {
    // Indicate which parities are valid.
    // The valid masks are EVEN_PARITY_MASK, ODD_PARITY_MASK,
    // CLEAR_PARITY_MASK and their combinations.
    unsigned int validMask;
    NSKOneParity even;
    NSKOneParity odd;
    NSKOneParity clear;
} NSKCmChannel;

TEE_Result TSPTA_AddNSKCmChannel(void* sessionContext,
                                        uint32_t  paramTypes,
                                        TEE_Param params[4]);

TEE_Result TSPTA_RmNSKCmChannel(void* sessionContext,
                                       uint32_t  paramTypes,
                                       TEE_Param params[4]);

TEE_Result TSPTA_UpdateNSKCmChannel(void* sessionContext,
                                           uint32_t  paramTypes,
                                           TEE_Param params[4]);

TEE_Result TSPTA_ConnectNSKCmChannel(void* sessionContext,
                                       uint32_t  paramTypes,
                                       TEE_Param params[4]);

TEE_Result TSPTA_DisconnectNSKCmChannel(void* sessionContext,
                                       uint32_t  paramTypes,
                                       TEE_Param params[4]);
TEE_Result TSPTA_GetNSKCmChannelProp(void* sessionContext,
                                        uint32_t  paramTypes,
                                        TEE_Param params[4]);
TEE_Result TSPTA_GetNSKCmChannelKeyRight(void* sessionContext,
                                        uint32_t uCmChIndex,
                                        uint32_t  *pbProtected);

#endif /* _TEE_TSP_NSK_H_ */

