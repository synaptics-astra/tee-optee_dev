/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#include "config.h"
#include "tee_internal_api.h"
#include "com_type.h"
#include "ovp_cmd.h"
#include "ta_log.h"
#include "ovp_api.h"
#include "Galois_memmap.h"
#include "ovp_module.h"
#include "ovp_common.h"
#include "vdec_com.h"

#define OVP_TA_LOG TA_LOGI
/* #define OVP_TA_LOG printk */

static int MV_OVPOBJ_BindSHM(TEE_Param params[4]);

static bool ovp_param_type_is_value(uint32_t paramTypes, uint32_t index)
{
	uint32_t type;

	type = TEE_PARAM_TYPE_GET(paramTypes, index);
	return type == TEE_PARAM_TYPE_VALUE_INPUT ||
		type == TEE_PARAM_TYPE_VALUE_OUTPUT ||
		type == TEE_PARAM_TYPE_VALUE_INOUT;
}

static bool ovp_param_type_is_memref(uint32_t paramTypes, uint32_t index)
{
	uint32_t type;

	type = TEE_PARAM_TYPE_GET(paramTypes, index);
	return type == TEE_PARAM_TYPE_MEMREF_INPUT ||
		type == TEE_PARAM_TYPE_MEMREF_OUTPUT ||
		type == TEE_PARAM_TYPE_MEMREF_INOUT;
}

static TEE_Result ovp_validate_command_params(uint32_t commandID,
		uint32_t paramTypes,
		TEE_Param params[4])
{
	switch (commandID) {
	case OVP_CREATE:
		if (!ovp_param_type_is_value(paramTypes, 0) ||
		    !ovp_param_type_is_value(paramTypes, 1))
			return TEE_ERROR_BAD_PARAMETERS;
		break;
	case OVP_DESTROY:
	case OVP_SUSPEND:
	case OVP_RESUME:
	case OVP_GET_CLR_INTR_STS:
		if (!ovp_param_type_is_value(paramTypes, 0))
			return TEE_ERROR_BAD_PARAMETERS;
		break;
	case OVP_GETFRAMESWAIT:
		if (!ovp_param_type_is_value(paramTypes, 0) ||
		    !ovp_param_type_is_value(paramTypes, 1))
			return TEE_ERROR_BAD_PARAMETERS;
		break;
	case OVP_PASSSHM:
		if (!ovp_param_type_is_value(paramTypes, 0) ||
		    !ovp_param_type_is_memref(paramTypes, 1))
			return TEE_ERROR_BAD_PARAMETERS;
		if (params[1].memref.size < sizeof(OVP_MSG_HEADER_T))
			return TEE_ERROR_BAD_PARAMETERS;
		break;
	default:
		return TEE_ERROR_NOT_SUPPORTED;
	}

	return TEE_SUCCESS;
}

/*
 * OVP module pipeline connection configuration
 * from input ports to output ports for different
 * profiles
 */

TEE_Result OVP_ValidateInput(const void *buffer, size_t size, bool outputToNonsecure)
{
	TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
	TEE_MemRegion mr;

	if(TEE_SUCCESS != TEE_FindMemRegion(&mr,(void *)buffer, size))
		return res;
	if (TEE_MR_IS_RESTRICTED(&mr))
		return res;
	if (outputToNonsecure && TEE_MR_IS_SECURE(&mr))
		return res;
	return TEE_SUCCESS;
}

TEE_Result OVP_ValidateOutput(const void *buffer, size_t size, bool inputFromSecure)
{
	TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
	TEE_MemRegion mr;

	if(TEE_SUCCESS != TEE_FindMemRegion(&mr,(void *)buffer, size))
		return res;
	if (TEE_MR_IS_RESTRICTED(&mr))
		return res;
	if (inputFromSecure && TEE_MR_IS_NONSECURE(&mr))
		return res;
	return TEE_SUCCESS;
}

/* ***************************************************
 * Create a new instance for the trusted application *
 * ***************************************************/
TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
	TEE_Result result =TEE_SUCCESS;
	void *va;

#ifdef CONFIG_OPTEE
	result = TEE_TA_Register_RegRegion((void*)MEMMAP_OVP_REG_BASE, MEMMAP_OVP_REG_SIZE, 0, 0, &va);
#else
	result = TEE_Map_Register((void*)MEMMAP_OVP_REG_BASE, MEMMAP_OVP_REG_SIZE, 0, 0, &va);
#endif

	if ( result != TEE_SUCCESS )
		OVP_TA_LOG("OVP TEE_Map_Register Fail Err %x \n", result );

	return result;
}

/* *************************************************
 * Destory the instance of the trusted application *
 * *************************************************/
void TA_EXPORT TA_DestroyEntryPoint(void)
{
	OVP_TA_LOG("%s \n", __FUNCTION__);

	return;
}

/* ********************************************************************
 * open a session with the trusted application when a client requests *
 * ********************************************************************/
TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(uint32_t paramTypes,
		TEE_Param params[4],
		void **sessionContext)
{
	OVP_TA_LOG("%s \n", __FUNCTION__);

	return TEE_SUCCESS;
}

/* *****************************************************
 * Close a client session with the trusted application *
 * *****************************************************/
void TA_EXPORT TA_CloseSessionEntryPoint(void *sessionContext)
{
	OVP_TA_LOG("%s \n", __FUNCTION__);
	return;
}

/* *****************************************
 * Invoke a command within a given session *
 * *****************************************/
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(void *sessionContext,
		uint32_t commandID,
		uint32_t paramTypes,
		TEE_Param params[4])
{
	TEE_Result res = TEE_SUCCESS;//TEE_ERROR_NOT_SUPPORTED;
	INT ret;

	(void)sessionContext;

	res = ovp_validate_command_params(commandID, paramTypes, params);
	if (res != TEE_SUCCESS)
		return res;

	/*    OVP_TA_LOG("%s, cmd:%d, para0a=%x, para0b=%x, para1a=%x, para1b=%x, para2a=%x, para2b=%x\n",
	      __FUNCTION__, commandID, params[0].value.a, params[0].value.b
	      , params[1].value.a, params[1].value.b, params[2].value.a, params[2].value.b);*/

	switch (commandID) {

		case OVP_CREATE:
			{
				UINT32 shmAddr, shmSize;

				shmAddr = params[1].value.a;
				shmSize = params[1].value.b;

				ret = MV_OVP_Create();
				if (ret == S_OK)
					ret = MV_OVP_InitOvpCmdObj(shmAddr, shmSize);
				if (ret != S_OK)
					MV_OVP_Destroy();
				params[0].value.a = ret;
			}
			break;

		case OVP_DESTROY:
			params[0].value.a = MV_OVP_Destroy();
			break;

		case OVP_GETFRAMESWAIT:
			{
				UINT32 noofframes = 0;
				params[1].value.a = MV_OVP_GetNoOfFramesWaiting(params[0].value.a, &noofframes);
				params[0].value.b = noofframes;
			}
			break;

		case OVP_SUSPEND:
			{
				params[0].value.a = MV_OVP_Suspend(params[0].value.b);
			}
			break;

		case OVP_RESUME:
			{
				params[0].value.a = MV_OVP_Resume(params[0].value.b);
			}
			break;

		case OVP_PASSSHM:
			res = MV_OVPOBJ_BindSHM(params);
			break;

		case OVP_GET_CLR_INTR_STS:
			{
				UINT32 uiIntrSts = 0;
				params[0].value.a = MV_OVP_Get_Clr_IntrSts(&uiIntrSts);
				params[0].value.b = uiIntrSts;

			}
			break;

		default:
			res = TEE_ERROR_NOT_SUPPORTED;
			break;
	}
	return res;
}

VBUF_INFO * MV_OVPOBJ_BindSHM_VbufInfoBuffer(TEE_Param params[4]);

static int MV_OVPOBJ_BindSHM(TEE_Param params[4])
{
	int shmId;

	shmId = params[0].value.a;
	TEE_Result res = TEE_SUCCESS;

	switch(shmId)
	{
		case OVP_SHM_PUSHFRAME:
			{
				UINT8 *shmAddr;
				OVP_MSG_HEADER_T *pstOvpMsgHeader;
				VBUF_INFO *pVBufInfo, *pTmpVBufInfo;

				shmAddr = (UINT8 *)params[1].memref.buffer;
				pstOvpMsgHeader = (OVP_MSG_HEADER_T *)shmAddr;

				res = OVP_ValidateInput(shmAddr, sizeof(OVP_MSG_HEADER_T), 0);
				if(res != TEE_SUCCESS) {
					break;
				}

				pTmpVBufInfo = MV_OVPOBJ_BindSHM_VbufInfoBuffer(params);
				if (pTmpVBufInfo == NULL) {
					res = TEE_ERROR_BAD_PARAMETERS;
					break;
				}

				pVBufInfo = TEE_Malloc(sizeof(VBUF_INFO), TEE_MALLOC_ZEROS);
				if(!pVBufInfo) {
					OVP_TA_LOG("%s: NULL pVBufInfo\n", __func__);
					res = TEE_ERROR_NOT_SUPPORTED;
					break;
				}
				TEE_MemMove(pVBufInfo, pTmpVBufInfo, sizeof(VBUF_INFO));

				pstOvpMsgHeader->pstFrameDesc = pVBufInfo;
				params[0].value.b = MV_OVP_PushFrame(pstOvpMsgHeader);
				if (params[0].value.b != S_OK) {
					OVP_TA_LOG("%s: MV_OVP_PushFrame failed with status %d\n", __func__, params[0].value.b);
					TEE_Free(pVBufInfo);
					res = TEE_ERROR_BAD_PARAMETERS;
				}
			}
			break;

		case OVP_SHM_PROCESSFRAME:
			{
				UINT8 *shmAddr;
				OVP_MSG_HEADER_T *pstOvpMsgHeader;

				shmAddr = (UINT8 *)params[1].memref.buffer;
				pstOvpMsgHeader = (OVP_MSG_HEADER_T *)shmAddr;

				res = OVP_ValidateInput(pstOvpMsgHeader, sizeof(OVP_MSG_HEADER_T), 0);
				if(res != TEE_SUCCESS)
					break;

				params[0].value.b = MV_OVP_ProcessFrame(pstOvpMsgHeader, params[0].value.b);
				if (params[0].value.b == S_OK)
					TEE_Free(pstOvpMsgHeader->pstFrameDesc);
			}
			break;

		case OVP_SHM_RELEASEBUFF:
			{
				UINT8 *shmAddr;
				OVP_MSG_HEADER_T *pstOvpMsgHeader;

				shmAddr = (UINT8 *)params[1].memref.buffer;
				pstOvpMsgHeader = (OVP_MSG_HEADER_T *)shmAddr;

				res = OVP_ValidateInput(pstOvpMsgHeader, sizeof(OVP_MSG_HEADER_T), 0);
				if(res != TEE_SUCCESS)
					break;

				params[0].value.b = MV_OVP_ReleaseBuffer(params[0].value.b, pstOvpMsgHeader);
			}
			break;

		default:
			res = TEE_ERROR_NOT_SUPPORTED;
			break;
	}

	if (res != TEE_SUCCESS)
		params[0].value.b = E_BADVALUE;
	return res;
}

#ifndef CONFIG_OPTEE
TA_DEFINE_BEGIN
.uuid		   = TAOVP_UUID,
	.name		   = "OVP",
	.singleInstance    = false,
	.multiSession	   = false,
	.instanceKeepAlive = false,
	TA_DEFINE_END
#endif
