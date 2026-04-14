/*******************************************************************************
 *                 2017 Synaptics Incorporated. All Rights Reserved            *
 * THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF Synaptics.                   *
 * NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT *
 * OF Synaptics OR ANY THIRD PARTY. Synaptics RESERVES THE RIGHT AT ITS SOLE   *
 * DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO Synaptics.  *
 * THIS CODE IS PROVIDED "AS IS". Synaptics MAKES NO WARRANTIES, EXPRESSED,    *
 * IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.  *
 *                                                                             *
 *******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
//! \file    factory_crypto.h
//! \brief   Factory TA Crypto header
//! \author  Rohit Tayal
//! \date    Mar 2018
////////////////////////////////////////////////////////////////////////////////

#ifndef __FACTORY_CRYPTO_H__
#define __FACTORY_CRYPTO_H__

#include "drm_tee_types.h"

#define PKCS_PSS_SHA_256_SIGN  256
#define AES_GCM_TAG_SIZE     16
#define AES_GCM_IV_SIZE      12
#define STATUS_OK 1
#define STATUS_ERROR 0

/* Crypto related command dispatcher
 *
 * Arguments:
 * - commandID      (IN) - command related with CRYPTO
 * - params[4]      (IN) - parameters from normal world
 *
 * Return:
 * TEE_SUCCESS or one of the TEE error code
 */
TEE_Result MV_DRM_CRYPTO_InvokeCommandEntryPoint(uint32_t commandID,
                                              TEE_Param params[4]);
TEE_Result Drm_GenerateSecureKey_V2(TEE_Param params[4]);
#endif /* __FACTORY_CRYPTO_H__ */
