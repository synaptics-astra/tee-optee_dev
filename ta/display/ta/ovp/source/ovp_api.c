/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

/************* Header file include ********************************************/
#define _OVP_API_C_

#include "ovp_module.h"
#include "ovp_common.h"
#include "ErrorCode.h"
#include "ovp_api.h"
#include "ovp_apifuncs.h"
#include "galois_io.h"
#include <string.h>
#include "tee_internal_api.h"
#include "tee_api_syna.h"
#include "ovp_deint_param.h"
#include "tee_ta_buffer_check.h"
#include "tee_ta_mmap.h"

/***********************************************
 * FUNCTION: create a OVP
 * PARAMS: None.
 * RETURN: S_OK - succeed
 *	   MV_VPP_ENOMEM - no memory
 ***********************************************/
INT MV_OVP_Create(void)
{
	INT ret;

	OVP_log_init(OVP_LOG_HIGH, OVP_LOG_CONSOLE);

	ovp = TEE_Malloc(sizeof(OVP), TEE_MALLOC_ZEROS);

	if(ovp == NULL) {
		OVP_DBG_ERROR("%s: TEE_Malloc returns NULL\n", __func__);
		return E_OUTOFMEMORY;
	}

	OVP_DBG_ERROR("%s: TEE_Malloc returns %x\n", __func__, ovp);
	/* Update Magic number */
	ovp->m_uiMagicNum  = OVP_MAGIC;

	/* Reset OVP */
	OVP_Reset();
	/* Unmask all interrupts except Fifo Empty Sts */
	GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_IntMask), 0x04);
	/* Configure Deint Core */
	OVP_DEINT_ConfigDeintCore();
	/* Make Number of objects as zero */
	ovp->m_uchNoOfObjs   =	 0;
	/* Create memory for all objects */
	ret = OVP_CreateAllObjs(ovp);
	if (ret != S_OK) {
		OVP_DBG_ERROR("%s: OVP_CreateAllObjs failed %d\n", __func__, ret);
		TEE_Free(ovp);
		ovp = NULL;
		return ret;
	}
	/* Mark status of OVP as active */
	ovp->m_iStatus = OVP_STATUS_ACTIVE;
	/* Make suspend status as false */
	ovp->m_suspend = FALSE;

	return S_OK;
}

/***********************************************
 * FUNCTION: destroy OVP
 * PARAMS: None
 * RETURN: MV_VPP_OK - succeed
 *	   MV_VPP_ENOMEM - no memory
 ***********************************************/
INT MV_OVP_Destroy(void)
{
	OVP_DBG_INFO("Enter %s\n",__FUNCTION__);

	if (NULL == ovp)
		return (E_NOTREADY);

	if (ovp->m_uiMagicNum != OVP_MAGIC)
		return (E_NOTREADY);

	VPP_UnMapUnRegister_SingleBuffer(&ovp->bufHandle);
	/* Close all OVP clients */
	ovp->m_iStatus = OVP_STATUS_INACTIVE;
	/* Destroy memory for objects of OVP */
	OVP_DestroyAllObjs(ovp);
	/* Close OVP */
	TEE_Free(ovp);
	ovp = NULL;

	OVP_DBG_INFO("Exit %s\n",__FUNCTION__);

	return S_OK;
}

/***********************************************
 * FUNCTION: suspend OVP
 * PARAMS: None
 * RETURN: MV_VPP_OK - succeed
 ***********************************************/
INT MV_OVP_Suspend(BOOL optimize)
{
	if (!optimize) {
		/* Full suspend from Kernel */
		/* Mask all interrupts */
		GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_IntMask), 0x3F);
	} else {
		/* Fast standby suspend from Disp */
		if (NULL == ovp || ovp->m_uiMagicNum != OVP_MAGIC) {
			OVP_DBG_ERROR(" OVP device is not Ready\n");
			return (E_NOTREADY);
		}
		ovp->m_suspend = TRUE;
		/* Mask all interrupts */
		GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_IntMask), 0x3F);
		/* Enable OVP mclock gating */
		GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_ClockControl), 0x1);
	}
	return S_OK;
}

/***********************************************
 * FUNCTION: resume OVP
 * PARAMS: None
 * RETURN: MV_VPP_OK - succeed
 ***********************************************/
INT MV_OVP_Resume(BOOL optimize)
{
	HRESULT Ret = S_OK;
	if (!optimize) {
		/* full Resume from Kernel */
		/* Reset OVP */
		Ret = OVP_Reset();
		if (Ret != S_OK) {
			OVP_DBG_ERROR("OVP_Reset failed with error %d\n", Ret);
			return Ret;
		}

		/* Unmask all interrupts except Fifo Empty Sts */
		GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_IntMask), 0x04);

		/* Configure Deint Core */
		OVP_DEINT_ConfigDeintCore();
	} else {
		/* Fast standby Resume from Disp */
		/* Disable OVP mclock gating */
		GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_ClockControl), 0x0);
		/* Unmask all interrupts except Fifo Empty Sts */
		GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_IntMask), 0x04);
		ovp->m_suspend = FALSE;
	}
	return Ret;
}

/*********************************************************
 * FUNCTION: Input frame to OVP
 * PARAMS: pstOvpMsgHeader  - OVP message header which has
 *	    information regarding the frame to be processed.
 * RETURN: MV_VPP_OK - succeed
 *	   MV_VPP_ENOMEM - no memory
 **********************************************************/

INT MV_OVP_PushFrame(OVP_MSG_HEADER_T *pstOvpMsgHeader)
{
	if (NULL == ovp || ovp->m_uiMagicNum != OVP_MAGIC) {
		OVP_DBG_ERROR(" OVP device is not Ready\n");
		return (E_NOTREADY);
	}
	if (NULL == pstOvpMsgHeader) {
		OVP_DBG_ERROR(" Invalid parameter pstOvpMsgHeader \n");
		return (E_BADVALUE);
	}
	/* Validate frame descriptor pointer */
	if (NULL == pstOvpMsgHeader->pstFrameDesc) {
		OVP_DBG_ERROR(" Frame descriptor is NULL \n");
		return (E_BADVALUE);
	}

	if (ovp->m_suspend)
		return (E_NOTREADY);
	else
		return OVP_PushFrame(pstOvpMsgHeader);
}

/*********************************************************
 * FUNCTION: Processed frame from OVP
 * PARAMS: pstOvpMsgHeader  - OVP message header which has
 *	    information regarding the processed frame.
 * RETURN: MV_VPP_OK - succeed
 *	   MV_VPP_ENOMEM - no memory
 **********************************************************/
INT MV_OVP_ProcessFrame(OVP_MSG_HEADER_T *pstOvpMsgHeader, UINT32 uiIntrSts)
{
	if (NULL == ovp || ovp->m_uiMagicNum != OVP_MAGIC ) {
		OVP_DBG_ERROR(" OVP device is not Ready\n");
		return (E_NOTREADY);
	}
	if (NULL == pstOvpMsgHeader) {
		OVP_DBG_ERROR(" Invalid parameter pstOvpMsgHeader \n");
		return (E_BADVALUE);
	}
	/* Call internal function and return actual error code */
	return OVP_ProcessFrame(pstOvpMsgHeader, uiIntrSts);
}

/*********************************************************
 * FUNCTION: Get No. of frames Waiting in the Queue for processing.
 * PARAMS: uiBuffId	-   Id of the Requestor
 *	   puiFramesWaiting -	No. Of frames waiting.
 * RETURN: MV_VPP_OK - succeed
 *	   MV_VPP_ENOMEM - no memory
 **********************************************************/

INT MV_OVP_GetNoOfFramesWaiting(UINT32 uiBuffId, UINT32 *puiFramesWaiting)
{
	UINT32 uiLoopCount, uiNoOfFramesWaiting;

	if (NULL == ovp || ovp->m_uiMagicNum != OVP_MAGIC ) {
		OVP_DBG_ERROR(" OVP device is not Ready\n");
		return (E_NOTREADY);
	}
	if (NULL == puiFramesWaiting) {
		OVP_DBG_ERROR(" Invalid parameter pstOvpMsgHeader \n");
		return (E_BADVALUE);
	}
	//Check whether Buff ID is valid or not
	for (uiLoopCount = 0; uiLoopCount < ovp->m_uchNoOfObjs; uiLoopCount++) {
		if (ovp->m_pOvp_objs[uiLoopCount]->m_uchOvpClientId == uiBuffId) {
			uiNoOfFramesWaiting = ovp->m_pOvp_objs[uiLoopCount]->m_uchNoOfFramesWaiting;
			break;
		}
	}
	//Return error if OVP object is not found with received buff ID
	if (uiLoopCount >= ovp->m_uchNoOfObjs) {
		/* Object may have been deleted after last release_buffer drain; not an error */
		OVP_DBG_MID("Received Buff ID %d not found; treating as 0 frames waiting\n", uiBuffId);
		/* return (E_BADVALUE); */
		uiNoOfFramesWaiting = 0;
	}
	/* Get the number of frames waiting in the queue. */
	*puiFramesWaiting = uiNoOfFramesWaiting;

	return S_OK;
}

/*********************************************************
 * FUNCTION: Release Requested Frame of a buffer ID.
 * PARAMS: pstOvpMsgHeader  - OVP message header which has
 *	    information regarding the processed frame.
 * RETURN: MV_VPP_OK - succeed
 *	   MV_VPP_ENOMEM - no memory
 **********************************************************/
INT MV_OVP_ReleaseBuffer(UINT32 uiBuffId, OVP_MSG_HEADER_T *pstOvpMsgHeader)
{
	UINT32 uiLoopCount, uiNoOfFramesWaiting;
	OVP_OBJ *pstOvpObj;
	OVP_FRAMEDESC_LIST stFrameDesc;
	INT ret;

	memset(&stFrameDesc, 0x0, sizeof(OVP_FRAMEDESC_LIST));
	uiNoOfFramesWaiting = 0;

	if (NULL == ovp) {
		OVP_DBG_ERROR("NULL ovp\n");
		return (E_NOTREADY);
	}
	if (ovp->m_uiMagicNum != OVP_MAGIC) {
		OVP_DBG_ERROR("ovp magic incorrect %x\n", ovp->m_uiMagicNum);
		return (E_NOTREADY);
	}

	if (NULL == pstOvpMsgHeader) {
		OVP_DBG_ERROR("Msg Header is invalid\n");
		return (E_BADVALUE);
	}
	//Check whether Buff ID is valid or not
	for (uiLoopCount = 0; uiLoopCount < ovp->m_uchNoOfObjs; uiLoopCount++) {
		if (ovp->m_pOvp_objs[uiLoopCount]->m_uchOvpClientId == uiBuffId) {
			uiNoOfFramesWaiting = ovp->m_pOvp_objs[uiLoopCount]->m_uchNoOfFramesWaiting;
			break;
		}
	}
	//Return error if OVP object is not found with received buff ID
	if (uiLoopCount >= ovp->m_uchNoOfObjs) {
		OVP_DBG_ERROR("Received Buff ID %d is invalid\n", uiBuffId);
		return (E_BADVALUE);
	}
	if (!uiNoOfFramesWaiting) {
		OVP_DBG_HIGH("No more frames are waiting in queue \n");
		return S_OK;
	}

	if (ovp->m_iStatus == OVP_STATUS_RUNNING &&
			uiNoOfFramesWaiting > 1) {
		OVP_DBG_ERROR("uiNoOfFramesWaiting: %d\n", uiNoOfFramesWaiting);
		return (E_NOTREADY);
	}

	pstOvpObj = ovp->m_pOvp_objs[uiLoopCount];
	/* Mark status as stopped */
	pstOvpObj->m_iStatus = OVP_OBJECT_STATUS_STOPPED;

	/* Get Node */
	ret = OVP_LIST_GetNode(pstOvpObj, uiNoOfFramesWaiting - 1, &stFrameDesc);
	if (ret != S_OK)
		return ret;

	pstOvpMsgHeader->m_uchBuffID = pstOvpObj->m_uchOvpClientId;
	pstOvpMsgHeader->m_uchOutputWholeFrame	=   pstOvpObj->m_uchFrameOrField;
	pstOvpMsgHeader->m_uchOutputOneField	=   pstOvpObj->m_uchOneOrTwo;
	pstOvpMsgHeader->m_uchInBitDepth	=   pstOvpObj->m_uchInBitDepth;
	pstOvpMsgHeader->m_uchOutBitDepth	=   pstOvpObj->m_uchOutBitDepth;
	pstOvpMsgHeader->pstFrameDesc		=   stFrameDesc.m_pbufInFrameDescr;
	pstOvpMsgHeader->m_uchPriority		=   pstOvpObj->m_uchPriority ;
	pstOvpMsgHeader->m_uiErrorCode		=   0;

	ret = OVP_LIST_DeleteNode(pstOvpObj, uiNoOfFramesWaiting - 1);
	if (ret != S_OK)
		return ret;

	pstOvpObj->m_uchNoOfFramesWaiting--;

	/* If no frames are present for this object, then make client ID as invalid. */
	if (pstOvpObj->m_uchNoOfFramesWaiting <= 0) {
		/* Delete Client list */
		OVP_LIST_DeleteClientList(ovp, uiLoopCount);
		pstOvpObj->m_uchOvpClientId = -1;
		OVP_DBG_MID("No. of Frames waiting are %d and No. of Objects are %d\n", pstOvpObj->m_uchNoOfFramesWaiting, ovp->m_uchNoOfObjs);
		OVP_DBG_ERROR("Deleting resources for Buff Id is %d (%x)\n", pstOvpMsgHeader->m_uchBuffID, pstOvpMsgHeader->pstFrameDesc);
	}

	/* Frame descriptor ownership transfers to caller; do NOT free here */

	return S_OK;
}

/*********************************************************
 * FUNCTION: Registers OVP components ISR handle. Required
 *	     Only when TZ is enabled.
 * PARAMS:   Isr task function handle
 * RETURN: MV_VPP_OK - succeed
 **********************************************************/
INT MV_OVP_RegisterIsrTaskFunc(void *pFIsrTaskFunc)
{
	return S_OK;
}

/*********************************************************
 * FUNCTION: Read and clear interrupt status. Required only
 *	     in TZ, as registers can't be accesses from
 *	     non TZ world.
 * PARAMS:  puiIntrSts - Interrupt status.
 * RETURN: MV_VPP_OK - succeed
 **********************************************************/

INT MV_OVP_Get_Clr_IntrSts(UINT32 *puiIntrSts)
{
#ifdef CONFIG_AMP_TEE_ENABLE
	UINT32 uiIntrSts;
	if (NULL == puiIntrSts) {
		return (E_BADVALUE);
	}
	/* Read interrupt status */
	GA_REG_WORD32_READ(OVP_REG(RA_OvpReg_IntStatus), &uiIntrSts);

	*puiIntrSts = uiIntrSts;

	/* Clear interrupts */
	GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_IntStatus), 0x3F);
#endif
	return S_OK;
}

int MV_OVP_InitOvpCmdObj(UINT32 phShm, UINT32 shmSize)
{
	UINT32 uiLoopCount;
	TEE_Result ret;
	VPP_SHM_BUFFER_DESC bufDesc;
	UINT32 required_size;
	UINT8 *virt_addr;

	OVP_DBG_INFO("%s: phShm: 0x%x, uiShmSize: %d \n", __func__, phShm, shmSize);

	if (ovp == NULL || ovp->m_uiMagicNum != OVP_MAGIC)
		return E_NOTREADY;

	required_size = MAX_OVP_OBJECTS * OVPD_HW_CMD_SHM_SIZE_PER_CTX;
	if (shmSize < required_size)
		return E_BADVALUE;

	VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)phShm, shmSize, VPP_SHM_BUFFER_TYPE_INPUT);
	ret = VPP_MapAndValidate_SingleBuffer(&ovp->bufHandle, &bufDesc);
	if (ret != TEE_SUCCESS)
		return ret;

	virt_addr = (UINT8 *)ovp->bufHandle.virt_addr;
	if (virt_addr == NULL)
		return TEE_ERROR_BAD_STATE;
	/* Retrieve the value from VPP_BUFF_HANDLE */
	for (uiLoopCount = 0; uiLoopCount < ovp->m_uchNoOfObjs; uiLoopCount++) {
		ovp->m_pOvp_objs[uiLoopCount]->pOvp_cmd_obj = (OVP_CMD_OBJ *)(virt_addr + (uiLoopCount * OVPD_HW_CMD_SHM_SIZE_PER_CTX));
		ovp->m_pOvp_objs[uiLoopCount]->pOvp_cmd_obj->m_uiPhyAddr = ovp->bufHandle.phy_addr + (uiLoopCount  * OVPD_HW_CMD_SHM_SIZE_PER_CTX);
		OVP_DBG_ERROR("%s: pOvp_cmd_obj[%d]:%x, phy_addr: %x\n", __func__, uiLoopCount, ovp->m_pOvp_objs[uiLoopCount]->pOvp_cmd_obj,
				ovp->m_pOvp_objs[uiLoopCount]->pOvp_cmd_obj->m_uiPhyAddr);
	}

	return TEE_SUCCESS;
}
