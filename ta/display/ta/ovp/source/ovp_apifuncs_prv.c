/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

/************* Header file include ********************************************/
#define _OVP_API_FUNCS_PRV_C_
#include <stdio.h>
#include "ovp_api.h"
#include "ovp_common.h"
#include "galois_io.h"
#include "vdec_com.h"
#include "ovp_apifuncs.h"
#include "ovp_module.h"
#ifdef CONFIG_AMP_TEE_ENABLE
#include "ta_log.h"
#endif
#include "ovp_debug.h"

#define printk printf
/***********************************************************************
 * FUNCTION: allocates memory for all OVP OBJs
 * INPUT: pOVP	-   OVP data structure
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_CreateAllObjs(OVP *pOvp)
{
	OVP_OBJ *pstOvpObj;
	INT iCount;
	INT allocated_count;

	if (pOvp == NULL)
		return E_BADVALUE;

	pOvp->m_uchNoOfObjs = 0;

	for(iCount = 0; iCount < MAX_OVP_OBJECTS; iCount++) {
		/* Allocate Memory for the object */
		pOvp->m_pOvp_objs[iCount] = (OVP_OBJ *) TEE_Malloc(sizeof(OVP_OBJ), TEE_MALLOC_ZEROS);
		if (pOvp->m_pOvp_objs[iCount] == NULL) {
			OVP_DBG_ERROR("TEE_Malloc failed for OVP object %d\n", iCount);
			for (allocated_count = 0; allocated_count < iCount; allocated_count++) {
				TEE_Free(pOvp->m_pOvp_objs[allocated_count]);
				pOvp->m_pOvp_objs[allocated_count] = NULL;
			}
			return E_OUTOFMEMORY;
		}
		pOvp->m_pOvp_objs[iCount]->m_iValid = 0;

		/* Initialize to Zero */
		pstOvpObj =  pOvp->m_pOvp_objs[iCount];

		pstOvpObj->m_uchOvpClientId = -1;
		pstOvpObj->pOvp_cmd_obj = NULL;
		OvpResetFrameCnt(pstOvpObj);
	}

	pOvp->m_uchNoOfObjs = MAX_OVP_OBJECTS;
	return S_OK;
}

/***********************************************************************
 * FUNCTION: Free memory for all OVP OBJs
 * INPUT: pOVP	-   OVP data structure
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_DestroyAllObjs(OVP *pOvp)
{
	INT iCount;
	OVP_OBJ *pstOvpObj;

	if(NULL == pOvp)
		return E_BADVALUE;

	for(iCount = 0; iCount < MAX_OVP_OBJECTS; iCount++) {
		pstOvpObj = pOvp->m_pOvp_objs[iCount];
		if (pstOvpObj == NULL)
			continue;

		OVP_LIST_DeleteFullList(pstOvpObj);
		pstOvpObj->m_iValid = 0;
		pstOvpObj->m_uchOvpClientId = -1;
		pstOvpObj->pOvp_cmd_obj = NULL;
		TEE_Free(pstOvpObj);
		pOvp->m_pOvp_objs[iCount] = NULL;
	}
	pOvp->m_uchNoOfObjs = 0;

	return S_OK;
}

/***********************************************************************
 * FUNCTION: Create a new data structre to store frames for a client.
 * INPUT: pOVP	-   OVP data structure
 *	  iClientId -	Client id for which list need to be created.
 *	  iOvpObjIndex	-   Index of OVP object.
 * RETURN:  S_OK    -	Success.
 *******************************************************************/

INT OVP_LIST_CreateClientList(OVP *pOvp, INT iClientId, INT *piOvpObjIndex)
{
	OVP_OBJ *pstOvpObj;
	UINT32 iCount;

	if (pOvp == NULL || piOvpObjIndex == NULL)
		return E_BADVALUE;

	for(iCount = 0; iCount < pOvp->m_uchNoOfObjs; iCount++) {
		if(!pOvp->m_pOvp_objs[iCount]->m_iValid)
			break;
	}

	if (iCount >= pOvp->m_uchNoOfObjs) {
		OVP_DBG_ERROR("No free OVP object available for client %d\n", iClientId);
		return E_OUTOFMEMORY;
	}

	pstOvpObj =  pOvp->m_pOvp_objs[iCount];
	pstOvpObj->m_iValid = 1;
	pstOvpObj->m_uchOvpClientId = (UINT8)iClientId;  /* Type-safe assignment */
	pstOvpObj->m_stFrameListHead = NULL;
	pstOvpObj->m_uchNoOfFramesWaiting = 0;

	*piOvpObjIndex = iCount;
	return S_OK;
}

/***********************************************************************
 * FUNCTION: Delete data structre which used to store frames for a client.
 * INPUT: pOVP	-   OVP data structure
 *	  iClientId -	Client id for which list need to be created.
 *	  iOvpObjIndex	-   Index of OVP object.
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_LIST_DeleteClientList(OVP *pOvp, INT iOvpObjIndex)
{
	OVP_OBJ *pstOvpObj;

	if(pOvp == NULL)
		return E_BADVALUE;
	if (iOvpObjIndex < 0 || iOvpObjIndex >= pOvp->m_uchNoOfObjs)
		return E_BADVALUE;

	pstOvpObj = pOvp->m_pOvp_objs[iOvpObjIndex];
	if (pstOvpObj == NULL)
		return E_BADVALUE;

	OVP_LIST_DeleteFullList(pstOvpObj);
	pstOvpObj->m_iValid = 0;
	pstOvpObj->m_uchOvpClientId = -1;
	pstOvpObj->m_uchNoOfFramesWaiting = 0;

	return S_OK;
}
/***********************************************************************
 * FUNCTION: Add frame in the data structure
 * INPUT: pstOvpObj  -	 OVP object.
 *	  pstFrameDesc - Frame descritor to be stored in data structure.
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_LIST_AddFrameDesc(OVP_OBJ *pstOvpObj, void *pstFrameDesc)
{
	OVP_FRAMEDESC_LIST *pstTempList;

	/* Validate input parameters */
	if (!pstOvpObj || !pstFrameDesc) {
		OVP_DBG_ERROR("Invalid parameters: pstOvpObj=%p, pstFrameDesc=%p\n", pstOvpObj, pstFrameDesc);
		return E_INVALIDARG;
	}

	pstTempList = pstOvpObj->m_stFrameListHead;

	/* Check whether any frame is present in the list or not */
	if(pstTempList == NULL) {
		/* No frames are present in the list. So, allocate one list */
		pstTempList = (OVP_FRAMEDESC_LIST *) TEE_Malloc(sizeof(OVP_FRAMEDESC_LIST), TEE_MALLOC_ZEROS);
		if (pstTempList == NULL) {
			OVP_DBG_ERROR("TEE_Malloc failed for list node\n");
			return E_OUTOFMEMORY;
		}
		pstOvpObj->m_stFrameListHead = pstTempList;
		pstOvpObj->m_stFrameListHead->pNext = NULL;
		pstOvpObj->m_stFrameListHead->pPrev = NULL;

	} else {
		/* Traverse till end of list */
		for(pstTempList = pstOvpObj->m_stFrameListHead; pstTempList->pNext != NULL; pstTempList = pstTempList->pNext);

		/* Allocate memory for node */
		pstTempList->pNext = (OVP_FRAMEDESC_LIST *) TEE_Malloc(sizeof(OVP_FRAMEDESC_LIST), TEE_MALLOC_ZEROS);
		if (pstTempList->pNext == NULL) {
			OVP_DBG_ERROR("TEE_Malloc failed for list node\n");
			return E_OUTOFMEMORY;
		}

		pstTempList->pNext->pNext = NULL;
		pstTempList->pNext->pPrev = pstTempList;

		/* Move to next node */
		pstTempList = pstTempList->pNext;
	}

	/* Store Client ID of the frame received */
	pstTempList->m_uchClientId = pstOvpObj->m_uchOvpClientId;
	/* Store Frame descriptor */
	pstTempList->m_pbufInFrameDescr =   (void *)pstFrameDesc;

	return S_OK;
}

/***********************************************************************
 * FUNCTION: Get the Frame for which OVP command done interrupt is set.
 * INPUT: pstOvpObj  -	 OVP object.
 *	  pstFrameDesc - Frame descritor for which command is processed.
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_LIST_GetListNode(OVP_OBJ *pstOvpObj, OVP_FRAMEDESC_LIST *pstNode)
{
	OVP_FRAMEDESC_LIST *pstTempList;
	VDEC_VID_BUF_DESC *frameDescr;
	VBUF_INFO *vbuf_info;

	if (pstOvpObj == NULL || pstNode == NULL || pstOvpObj->pOvp_cmd_obj == NULL)
		return E_BADVALUE;

	pstTempList = pstOvpObj->m_stFrameListHead;
	if(!pstTempList) {
		printf("%s: pstTempList NULL !!\n", __func__);
		return E_INVALIDARG;
	}

	/* Traverse all frames in list to find matching buffer address */
	while (pstTempList != NULL) {
		frameDescr = ( VDEC_VID_BUF_DESC *)pstTempList->m_pbufInFrameDescr;
#ifndef CONFIG_AMP_TEE_ENABLE
		vbuf_info = (VBUF_INFO *)frameDescr->user_data;
#else
		vbuf_info = (VBUF_INFO *)frameDescr;
#endif
		/* Validate vbuf_info before accessing */
		if (vbuf_info) {
			SIE_OvpCmd volatile *pstOvpBufInfo;

			pstOvpBufInfo = &pstOvpObj->pOvp_cmd_obj->stOvpBufInfo;
			/* Compare the address for which command done interrupt is set */
			if(((UINT)vbuf_info->m_pbuf_start == pstOvpBufInfo->uInputBuffer_InputBuf0YAddr)||\
					((UINT)vbuf_info->m_pbuf_start + vbuf_info->m_buf_stride == \
					 pstOvpBufInfo->uInputBuffer_InputBuf0YAddr))
			{
				/* Get the frame and client id */
				pstNode->m_uchClientId = pstTempList->m_uchClientId;
				pstNode->m_pbufInFrameDescr =	pstTempList->m_pbufInFrameDescr;
				return S_OK;
			}
		}
		pstTempList = pstTempList->pNext;
	}

	return E_INVALIDARG;
}

/***********************************************************************
 * FUNCTION: Delete the frame at the beginning of list
 * INPUT: pstOvpObj  -	 OVP object.
 *	  pstFrameDesc - Frame descritor for which command is processed.
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_LIST_DeleteNode(OVP_OBJ *pstOvpObj, UINT32 uiPosition)
{
	OVP_FRAMEDESC_LIST *pstTempList;
	UINT32 uiNoOfNodesInList = 0;

	if (pstOvpObj == NULL)
		return E_BADVALUE;

	pstTempList = pstOvpObj->m_stFrameListHead;

	if(NULL == pstTempList)
		return E_INVALIDARG;

	for (; pstTempList != NULL; pstTempList = pstTempList->pNext) {
		if (uiNoOfNodesInList == uiPosition)
			break;
		uiNoOfNodesInList++;
	}

	if (pstTempList == NULL)
		return E_INVALIDARG;

	if (pstTempList->pPrev != NULL)
		pstTempList->pPrev->pNext = pstTempList->pNext;
	else
		pstOvpObj->m_stFrameListHead = pstTempList->pNext;

	if (pstTempList->pNext != NULL)
		pstTempList->pNext->pPrev = pstTempList->pPrev;

	TEE_Free(pstTempList);

	return S_OK;
}
/***********************************************************************
 * FUNCTION: Delete complete
 * INPUT: pstOvpObj  -	 OVP object.
 *
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_LIST_DeleteFullList(OVP_OBJ *pstOvpObj)
{
	OVP_FRAMEDESC_LIST *pstTempList;
	OVP_FRAMEDESC_LIST *pstNextNode;

	if (pstOvpObj == NULL)
		return E_BADVALUE;

	pstTempList = pstOvpObj->m_stFrameListHead;

	if(pstTempList == NULL)
		return S_OK;

	while (pstTempList != NULL) {
		pstNextNode = pstTempList->pNext;
		TEE_Free(pstTempList->m_pbufInFrameDescr);
		TEE_Free(pstTempList);
		pstTempList = pstNextNode;
	}

	pstOvpObj->m_stFrameListHead = NULL;
	pstOvpObj->m_uchNoOfFramesWaiting = 0;

	return S_OK;
}
/***********************************************************************
 * FUNCTION: Delete the frame at the beginning of list
 * INPUT: pstOvpObj  -	 OVP object.
 *	  pstFrameDesc - Frame descritor for which command is processed.
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_LIST_GetNode(OVP_OBJ *pstOvpObj, UINT32 uiFrameDescNum, OVP_FRAMEDESC_LIST *pstNode)
{
	OVP_FRAMEDESC_LIST *pstTempList;
	UINT32 uiLoopCount = 0;

	pstTempList = pstOvpObj->m_stFrameListHead;

	if(pstTempList == NULL) {
		OVP_DBG_ERROR("Frame list is empty\n");
		return E_INVALIDARG;
	}

	/* Validate parameter bounds */
	if (uiFrameDescNum >= MAX_FRAMES_PENDING) {
		OVP_DBG_ERROR("Frame descriptor number %u exceeds maximum %u\n", uiFrameDescNum, MAX_FRAMES_PENDING);
		return E_INVALIDARG;
	}

	/* Check if only one node is present in the list */
	if(pstTempList->pNext == NULL) {
		if (uiFrameDescNum != 0) {
			OVP_DBG_ERROR("Requested frame descriptor %u not found\n", uiFrameDescNum);
			return E_INVALIDARG;
		}
		/* Get the frame and client id */
		pstNode->m_uchClientId = pstTempList->m_uchClientId;
		pstNode->m_pbufInFrameDescr =	pstTempList->m_pbufInFrameDescr;
		return S_OK;
	}

	/* Go to the node number */
	for(pstTempList = pstOvpObj->m_stFrameListHead; pstTempList != NULL; pstTempList = pstTempList->pNext) {
		if(uiLoopCount == uiFrameDescNum)
		{
			/* Get the frame and client id */
			pstNode->m_uchClientId = pstTempList->m_uchClientId;
			pstNode->m_pbufInFrameDescr =	pstTempList->m_pbufInFrameDescr;
			return S_OK;
		}
		uiLoopCount++;
	}

	OVP_DBG_ERROR("Frame descriptor %u not found in list\n", uiFrameDescNum);
	return E_INVALIDARG;
}
/***********************************************************************
 * FUNCTION: Reset OVP Hardware
 * INPUT:   None
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_Reset(void)
{
	/* Reset OVP */
	GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_Reset), 0x01);
	GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_Reset), 0x00);

	return S_OK;
}
