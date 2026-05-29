/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#include "config.h"
#include "vdec_com.h"
#include "ovp_cmd.h"
#include "ta_log.h"
#include "ovp_api.h"
#include "Galois_memmap.h"
#include "ovp_module.h"
#include "ovp_common.h"
#include "tee_ta_buffer_check.h"

static TEE_Result OVP_MapAndValidate_Param(void **mappedAddr, TEE_Param *pParams, UINT32 sizeParam, VPP_SHM_BUFFER_TYPE bufType)
{
	TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
#ifdef OVP_ENABLE_PASSSHM_BY_MEMREF
	UINT8 *shmAddr = (UINT8 *)pParams->memref.buffer;
	unsigned int Size = pParams->memref.size;
#else
	UINT8 *shmAddr = (UINT8 *)pParams->value.a;
	unsigned int Size = pParams->value.b;
	VPP_BUFF_HANDLE buffHandle;
#endif
	VPP_SHM_BUFFER_DESC bufDesc;

	/* Validate only valid buffer address/id */
	if (shmAddr) {
		/* Override size with the given sizeParam */
		if (sizeParam)
			Size = sizeParam;

		/* Check the buffer validity based on buffer type */
		VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, bufType);
#ifdef OVP_ENABLE_PASSSHM_BY_MEMREF
		res = VPP_Validate_SingleBuffer(&bufDesc);
#else
		res = VPP_MapAndValidate_SingleBuffer(&buffHandle, &bufDesc);
		shmAddr = buffHandle.virt_addr;
#endif

		if (res != TEE_SUCCESS) {
			OVP_DBG_ERROR("Buffer from CA fail security check");
		} else {
			/* Return back the virt/phy address to access from this TA instance. */
			*mappedAddr = shmAddr;
		}
	}

	return res;
}

static void* OVP_MapAndValidate_VbufInfo(TEE_Param params[4])
{
	TEE_Result res;
	VBUF_INFO *pinfo = NULL;
	/* No need to invoke MV_OVPOBJ_BindSHM_SingleBuffer_getInfo() for bufType & size */
	const VPP_SHM_BUFFER_TYPE bufType = VPP_SHM_BUFFER_TYPE_INPUT;
	unsigned int Size = sizeof(VBUF_INFO);

	res = OVP_MapAndValidate_Param((void**)&pinfo, &params[2], Size, bufType);
	if (res != TEE_SUCCESS) {
		OVP_DBG_ERROR("VBUF_INFO Buffer fail security check");
	}

	return pinfo;
}

VBUF_INFO * MV_OVPOBJ_BindSHM_VbufInfoBuffer(TEE_Param params[4])
{
	VBUF_INFO *pinfo;
	int shmId, planeId;
	TEE_Result res = TEE_SUCCESS;

	shmId = params[0].value.a;
	planeId = params[3].value.a;

	pinfo = OVP_MapAndValidate_VbufInfo(params);
	if (!pinfo) {
		OVP_DBG_ERROR("OVP_MapAndValidate_VbufInfo returns failure!!!\n");
	} else {
		pinfo->m_dv_el_descriptor = (ARCH_PTR_TYPE)NULL;
	}

	return pinfo;
}

