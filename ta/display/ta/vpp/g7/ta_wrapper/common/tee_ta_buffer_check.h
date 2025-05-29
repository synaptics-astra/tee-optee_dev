/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_buffer_check.h
//!
//! \brief Implementation of buffer validation/checking functions for TrustZone.
//!
//! Purpose: This is used for check/validate access permission of various
//!               buffers shared from CA to TA (i/p & o/p)
//!
//!     Version    Date                     Author
//!     V 1.00,    Aug 24 2022,             Shanmugam Ramachandran
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
#ifndef __TEE_TA_BUFFER_CHECK_H__
#define __TEE_TA_BUFFER_CHECK_H__

#include "com_type.h"
#include "tee_internal_api.h"
#ifndef CONFIG_OPTEE
#include "tee_internal_private_api.h"
#else
#include "tee_api_syna.h"
#endif
#include "tee_ta_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VPP_INIT_SHM_BUFFER_DESC(BUF_DESC, ADDR, SIZE, TYPE)   do { \
            BUF_DESC.bufAddr = ADDR;     \
            BUF_DESC.bufSize = SIZE;     \
            BUF_DESC.bufType = TYPE;     \
        } while(0)

typedef enum __VPP_SHM_BUFFER_TYPE__ {
    VPP_SHM_BUFFER_TYPE_INPUT           = 0x01,
    VPP_SHM_BUFFER_TYPE_OUTPUT          = 0x02,
    VPP_SHM_BUFFER_TYPE_INOUT           = 0x03,
    VPP_SHM_BUFFER_TYPE_SECURE_INPUT    = 0x04,
    VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT   = 0x08,
}VPP_SHM_BUFFER_TYPE;

typedef struct __VPP_SHM_BUFFER_DESC__ {
    //address of the buffer to be validated
    void *bufAddr;
    //size of the buffer
    size_t bufSize;
    //type of the buffer: input/output/secure/non-secure
    VPP_SHM_BUFFER_TYPE bufType;
} VPP_SHM_BUFFER_DESC;

/***************************************************************
 * FUNCTION: Check the input buffer is valid
 *
 * IN PARAMS : VPP_SHM_BUFFER_DESC - details of buffer to be validated
 *               - Phy address, size & buffer type
 *           : outputToNonsecure - 1 : additional check for secure buffer
 *                                     fail if not secure buffer
 * OUT PARAMS : None
 * RETURN:  TEE_SUCCESS - buffer is valid to access in TA
 *          TEE_ERROR_BAD_PARAMETERS - invalid buffer
 ****************************************************************/
TEE_Result VPP_ValidateInput(VPP_SHM_BUFFER_DESC *pBuffDesc, bool outputToNonsecure);

/***************************************************************
 * FUNCTION: Check the output buffer is valid
 *
 * IN PARAMS : VPP_SHM_BUFFER_DESC - details of buffer to be validated
 *               - Phy address, size & buffer type
 *           : inputFromSecure - 1 : additional check for non-secure buffer
 *                                     fail if not non-secure buffer
 * OUT PARAMS : None
 * RETURN:  TEE_SUCCESS - buffer is valid to access in TA
 *          TEE_ERROR_BAD_PARAMETERS - invalid buffer
 ****************************************************************/
TEE_Result VPP_ValidateOutput(VPP_SHM_BUFFER_DESC *pBuffDesc, bool inputFromSecure);

/***************************************************************
 * FUNCTION: Check the single buffer based on buffer type
 *
 * IN PARAMS : VPP_SHM_BUFFER_DESC - details of buffer to be validated
 *               - Phy address, size & buffer type
 *
 * OUT PARAMS : None
 * RETURN:  TEE_SUCCESS - buffer is valid to access in TA
 *          TEE_ERROR_BAD_PARAMETERS - invalid buffer
 ****************************************************************/
TEE_Result VPP_Validate_SingleBuffer(VPP_SHM_BUFFER_DESC *pBuffDesc);

/***************************************************************
 * FUNCTION: Check the buffer based on buffer type, map and initialize buffer-handle
 *
 * IN PARAMS : VPP_SHM_BUFFER_DESC - details of buffer to be validated
 *               - Phy address, size & buffer type
 *
 * OUT PARAMS : pBuffHandle - buffer handle
 *
 * RETURN:  TEE_SUCCESS - buffer is valid to access in TA
 *          TEE_ERROR_BAD_PARAMETERS - invalid buffer
 ****************************************************************/
TEE_Result VPP_MapAndValidate_SingleBuffer(VPP_BUFF_HANDLE *pBuffHandle, VPP_SHM_BUFFER_DESC *pBuffDesc);

/***************************************************************
 * FUNCTION: UnMap and UnRegister buffer-handle
 *
 * IN PARAMS : VPP_SHM_BUFFER_DESC - details of buffer to be UnMap/UnRegister
 *               - mem-id
 *
 * OUT PARAMS : pBuffHandle - buffer handle
 *
 * RETURN:  TEE_SUCCESS - UnMapped/UnRegistered
 *          TEE_ERROR_BAD_PARAMETERS - invalid buffer
 *          TEE_XXX - UnMap/UnRegister specific error
 ****************************************************************/
#define VPP_UnMapUnRegister_SingleBuffer(pBuffHandle) 	VPP_BUFF_PutHandle(pBuffHandle)

#ifdef __cplusplus
}
#endif //__TEE_TA_BUFFER_CHECK_H__

#endif //__TEE_TA_BUFFER_CHECK_H__
