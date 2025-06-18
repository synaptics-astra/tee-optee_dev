/*******************************************************************************
 *                 2018 Synaptics Incorporated. All Rights Reserved            *
 * THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF Synaptics.                   *
 * NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT *
 * OF Synaptics OR ANY THIRD PARTY. Synaptics RESERVES THE RIGHT AT ITS SOLE   *
 * DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO Synaptics.  *
 * THIS CODE IS PROVIDED "AS IS". Synaptics MAKES NO WARRANTIES, EXPRESSED,    *
 * IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.  *
 *                                                                             *
 *******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
//! \file    factory_rpmb.h
//! \brief   Factory TA RPMB header
//! \author  Victor Ku
//! \date    July 2018
////////////////////////////////////////////////////////////////////////////////

#ifndef __FACTORY_RPMB_H__
#define __FACTORY_RPMB_H__

#include "drm_tee_types.h"

/* OTP related command dispatcher
 *
 * Arguments:
 * - commandID      (IN) - command related with OTP
 * - params[4]      (IN) - parameters from normal world
 *
 * Return:
 * TEE_SUCCESS or one of the TEE error code
 */
TEE_Result MV_DRM_RPMB_InvokeCommandEntryPoint(uint32_t commandID,
                                            TEE_Param params[4]);

#endif /* __FACTORY_RPMB_H__ */
