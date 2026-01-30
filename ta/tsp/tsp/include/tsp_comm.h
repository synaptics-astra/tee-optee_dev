/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
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
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

///////////////////////////////////////////////////////////////////////////////
//! \file m2m_wrap.h
//!
//! \brief  This file is used to implement the common functions for both tz/non-tz versions
//!
//! Author:         Wangzl
//! Modified by:
//! Version:        1.0
//! Date:           Mar 10 2017
//!
///////////////////////////////////////////////////////////////////////////////
#ifndef __TSP_COMM_H__
#define __TSP_COMM_H__

#ifdef TA_COMPILE
#include "string.h"
#include "tee_internal_api.h"

#else
#include "dmx_util.h"

#define TA_LOGF(...)  AMPLOG(MODULE_DMX, AMP_LOG_FATAL, __VA_ARGS__)
#define TA_LOGE(...)  AMPLOG(MODULE_DMX, AMP_LOG_ERROR, __VA_ARGS__)
#define TA_LOGH(...)  AMPLOG(MODULE_DMX, AMP_LOG_HIGH, __VA_ARGS__)
#define TA_LOGM(...)  AMPLOG(MODULE_DMX, AMP_LOG_MID, __VA_ARGS__)
#define TA_LOGL(...)  AMPLOG(MODULE_DMX, AMP_LOG_LOW, __VA_ARGS__)
#define TA_LOGI(...)  AMPLOG(MODULE_DMX, AMP_LOG_INFO, __VA_ARGS__)
#define TEE_Result uint32_t

enum {
    TEE_SUCCESS                     = 0x00000000,   /* TEEC_SUCCESS	*/
    TEE_ERROR_CORRUPT_OBJECT        = 0xF0100001,
    TEE_ERROR_CORRUPT_OBJECT_2      = 0xF0100002,
    TEE_ERROR_STORAGE_NOT_AVAILABLE = 0xF0100003,
    TEE_ERROR_STORAGE_NOT_AVAILABLE_2   = 0xF0100004,
    TEE_ERROR_GENERIC               = 0xFFFF0000,   /* TEEC_ERROR_GENERIC*/
    TEE_ERROR_ACCESS_DENIED         = 0xFFFF0001,   /* TEEC_ERROR_ACCESS_DENIED*/
    TEE_ERROR_CANCEL                = 0xFFFF0002,   /* TEEC_ERROR_CANCEL*/
    TEE_ERROR_ACCESS_CONFLICT       = 0xFFFF0003,   /* TEEC_ERROR_ACCESS_CONFLICT*/
    TEE_ERROR_EXCESS_DATA           = 0xFFFF0004,   /* TEEC_ERROR_EXCESS_DATA*/
    TEE_ERROR_BAD_FORMAT            = 0xFFFF0005,   /* TEEC_ERROR_BAD_FORMAT*/
    TEE_ERROR_BAD_PARAMETERS        = 0xFFFF0006,   /* TEEC_ERROR_BAD_PARAMETERS*/
    TEE_ERROR_BAD_STATE             = 0xFFFF0007,   /* TEEC_ERROR_BAD_STATE*/
    TEE_ERROR_ITEM_NOT_FOUND        = 0xFFFF0008,   /* TEEC_ERROR_ITEM_NOT_FOUND*/
    TEE_ERROR_NOT_IMPLEMENTED       = 0xFFFF0009,   /* TEEC_ERROR_NOT_IMPLEMENTED*/
    TEE_ERROR_NOT_SUPPORTED         = 0xFFFF000A,   /* TEEC_ERROR_NOT_SUPPORTED*/
    TEE_ERROR_NO_DATA               = 0xFFFF000B,   /* TEEC_ERROR_NO_DATA*/
    TEE_ERROR_OUT_OF_MEMORY         = 0xFFFF000C,   /* TEEC_ERROR_OUT_OF_MEMORY*/
    TEE_ERROR_BUSY                  = 0xFFFF000D,   /* TEEC_ERROR_BUSY*/
    TEE_ERROR_COMMUNICATION         = 0xFFFF000E,   /* TEEC_ERROR_COMMUNICATION*/
    TEE_ERROR_SECURITY              = 0xFFFF000F,   /* TEEC_ERROR_SECURITY*/
    TEE_ERROR_SHORT_BUFFER          = 0xFFFF0010,   /* TEEC_ERROR_SHORT_BUFFER*/
    TEE_ERROR_EXTERNAL_CANCEL       = 0xFFFF0011,   /* TEEC_ERROR_EXTERNAL_CANCEL*/
    TEE_ERROR_TIMEOUT               = 0xFFFF3001,
    TEE_ERROR_OVERFLOW              = 0xFFFF300F,
    TEE_ERROR_TARGET_DEAD           = 0xFFFF3024,   /* TEEC_ERROR_TARGET_DEAD*/
    TEE_ERROR_STORAGE_NO_SPACE      = 0xFFFF3041,
    TEE_ERROR_MAC_INVALID           = 0xFFFF3071,
    TEE_ERROR_SIGNATURE_INVALID     = 0xFFFF3072,
    TEE_ERROR_TIME_NOT_SET          = 0xFFFF5000,
    TEE_ERROR_TIME_NEEDS_RESET      = 0xFFFF5001,
};

#endif

#ifdef __cplusplus
extern "C"
{
#endif

TEE_Result TSP_FIGO_InitSchedCtx(void);

TEE_Result TSP_FIGO_FreeSchedCtx(void);

TEE_Result TSP_FIGO_Sched(uint32_t uSrcType, uint32_t uTsiPort,
                                uint32_t uScheme, uint32_t uCryptoLevel,
                                uint32_t *pFigoId, uint32_t *pFwIdx,
                                uint32_t *pNeedReload);
TEE_Result TSP_FIGO_UnSched(uint32_t uSrcType, uint32_t uTsiPort,
                                    uint32_t uFigoId);

#ifdef __cplusplus
}
#endif


#endif

