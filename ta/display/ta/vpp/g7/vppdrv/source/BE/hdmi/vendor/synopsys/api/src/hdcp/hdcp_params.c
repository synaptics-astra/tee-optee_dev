// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "hdcp/hdcp_params.h"
#include "vpp_osal.h"

void hdcp_params_reset(hdmi_tx_dev_t *dev, hdcpParams_t * params)
{
	params->bypass = TRUE;
	params->mEnable11Feature = 0;
	params->mRiCheck = 0;
	params->mI2cFastMode = 0;
	params->mEnhancedLinkVerification = 0;
	params->maxDevices = 0;

	if(params->mKsvListBuffer != NULL)
		VPP_OSAL_SHM_Dynamic_Free((VPP_SHM_HANDLE)params->mKsvListBuffer);
	params->mKsvListBuffer = NULL;
}
