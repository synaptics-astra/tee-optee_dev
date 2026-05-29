/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#include "ovp_module.h"
#include "ovp_common.h"
#include "tee_ta_buffer_check.h"

TEE_Result VPP_ValidateInput(VPP_SHM_BUFFER_DESC *pBuffDec, bool outputToNonsecure)
{
	TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
	TEE_MemRegion mr;

	if(TEE_SUCCESS != TEE_FindMemRegion(&mr, pBuffDec->bufAddr, pBuffDec->bufSize))
		return res;
	if (TEE_MR_IS_RESTRICTED(&mr))
		return res;
	if (outputToNonsecure && TEE_MR_IS_SECURE(&mr))
		return res;
	return TEE_SUCCESS;
}

TEE_Result VPP_ValidateOutput(VPP_SHM_BUFFER_DESC *pBuffDec, bool inputFromSecure)
{
	TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
	TEE_MemRegion mr;

	if(TEE_SUCCESS != TEE_FindMemRegion(&mr, pBuffDec->bufAddr, pBuffDec->bufSize))
		return res;
	if (TEE_MR_IS_RESTRICTED(&mr))
		return res;
	if (inputFromSecure && TEE_MR_IS_NONSECURE(&mr))
		return res;
	return TEE_SUCCESS;
}

TEE_Result VPP_Validate_SingleBuffer(VPP_SHM_BUFFER_DESC *pBuffDec)
{
	TEE_Result res = TEE_SUCCESS;

	if (pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_INPUT ||
			pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_SECURE_INPUT) {

		res = VPP_ValidateInput(pBuffDec, 0);

		/* If secure, then check further input rights */
		if (res == TEE_SUCCESS && (pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_SECURE_INPUT))
			res = TEE_CheckMemoryInputRights(pBuffDec->bufAddr, pBuffDec->bufSize, TEE_SECURE, TEE_MT_COPY);
	}

	if (res == TEE_SUCCESS && (pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_OUTPUT))
		res = VPP_ValidateOutput(pBuffDec, 0);

	if (res == TEE_SUCCESS && (pBuffDec->bufType & VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT))
		res = TEE_CheckMemoryOutputRights(pBuffDec->bufAddr, pBuffDec->bufSize, TEE_NONSECURE, TEE_MT_COPY);

	return res;
}

TEE_Result VPP_MapAndValidate_SingleBuffer(VPP_BUFF_HANDLE *pBuffHandle, VPP_SHM_BUFFER_DESC *pBuffDesc)
{
	UINT32 shmSize;
	const UINT8 bIsMapToAllInstances = 0;
	const UINT8 bIsPhyAddrNeeded = 1;

	/* memid/phy-addr is passed in pBuffDesc->bufAddr */
	shmSize = OVP_SHM_ALIGN_ROUNDUP(pBuffDesc->bufSize);

	VPP_BUFF_GetHandle(pBuffHandle, (UINT32)pBuffDesc->bufAddr, shmSize, bIsMapToAllInstances, bIsPhyAddrNeeded);

	/* Retrieve the value from VPP_BUFF_HANDLE */
	pBuffDesc->bufAddr = pBuffHandle->virt_addr;

	return VPP_Validate_SingleBuffer(pBuffDesc);
}
