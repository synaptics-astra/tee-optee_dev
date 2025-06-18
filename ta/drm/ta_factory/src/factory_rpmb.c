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
//! \file    factory_rpmb.c
//! \brief   Factory TA RPMB implementation
//! \author  Victor Ku
//! \date    July 2018
////////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_OPTEE
#include "tee_internal_api_extensions.h"
#include "tee_api_syna.h"
#else
#include "tee_common.h"
#include "tee_internal_common.h"
#include "tee_internal_core_common.h"
#include "tee_internal_private_api.h"
#endif

#include "tee_internal_api.h"
#include "factory_ta.h"
#include "factory_def.h"

/*
 * RPMB paring
 *
 * Arguments:
 * - paireFlag     (IN/OUT) - OTP flag to indicate if the pair has already been done
 * - rpmbState     (IN/OUT) - QUERY the rpmb state
 * - opMode        (IN/OUT) - query or program the key
 * Return:
 *         TEE_SUCCESS else TEE_ERROR_XXX
 */

static TEE_Result Drm_RPMBPairKey(TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;
    int paireFlag = 0;
    int rpmbState = 0;
    int opMode = 0;

    opMode = params[0].value.a;

    #ifdef CONFIG_OPTEE
    // TODO; Use optee api to get the RPMB state
    #else
    result = TEE_RPMB_PairKey(&paireFlag, &rpmbState, opMode);
    #endif

    if (result) {
        printf("RPMB pair key fail, 0x%x\n", (unsigned int)result);
    } else {
        params[0].value.a = paireFlag;
        params[0].value.b = rpmbState;
    }

    return result;
}

TEE_Result MV_DRM_RPMB_InvokeCommandEntryPoint(uint32_t commandID,
                                               TEE_Param params[4])
{
    TEE_Result result = TEE_SUCCESS;

    /* This dispatcher can only handle RPMB related functions */

    switch(commandID) {
    case DRM_SE_RPMB_PAIR_KEY:
        result = Drm_RPMBPairKey(params);
        break;

    default:
        break;
    }

    return result;
}
