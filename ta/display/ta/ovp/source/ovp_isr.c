/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

/************* Header file include ********************************************/
#define _OVP_ISR_C_

#include "ovp_module.h"
#include "ovp_api.h"
#include "ovp_common.h"
#include "galois_io.h"
#include "vdec_com.h"
#include "ovp_debug.h"

#define FLAG_MASK_HALF_FIELD	  	0x800
#define FLAG_MASK_FIELD_FIRST	  	0x1000
#define FLAG_MASK_TOP_FIELD_ONLY  	0x10000
#define FLAG_MASK_BOTTOM_FIELD_ONLY 	0x20000

typedef enum __FRAME_TYPE__{
	OVP_FRAMETYPE_FULL_FIELD = 0,
	OVP_FRAMETYPE_HALF_FIELD = 1,
}OVP_FRAME_TYPE;

/***********************************************************************
 * FUNCTION: updating the unput buffers.
 * INPUT: vbuf_info -	Object of vbuf_info
 * RETURN:  1	 -   OVP_FRAMETYPE_HALF_FIELD.
 *	    0	 -   OVP_FRAMETYPE_FULL_FIELD
 *******************************************************************/
static OVP_FRAME_TYPE OVP_UpdateInputBuffers(volatile UINT32 *pInputBuf0YAddr,
		volatile UINT32 *pInputBuf0UVAddr,
		volatile UINT32 *pInputBuf1YAddr,
		volatile UINT32 *pInputBuf1UVAddr,
		VBUF_INFO *vbuf_info)
{
	/* Address of even field */
	*pInputBuf0YAddr  = (UINT) vbuf_info->m_pbuf_start;
	*pInputBuf0UVAddr = vbuf_info->m_buf_pbuf_start_UV;
	/* Address of odd field */
	*pInputBuf1YAddr  = (UINT) vbuf_info->m_pbuf_start + vbuf_info->m_buf_stride;
	*pInputBuf1UVAddr= vbuf_info->m_buf_pbuf_start_UV + vbuf_info->m_buf_stride_UV;
	/* Check for half field flag/s and update odd/even field address accordingly */
	if(vbuf_info->m_flags & FLAG_MASK_HALF_FIELD) {
		if (vbuf_info->m_flags & FLAG_MASK_TOP_FIELD_ONLY) {
			*pInputBuf1YAddr  = (UINT) vbuf_info->m_pbuf_start;
			*pInputBuf1UVAddr = vbuf_info->m_buf_pbuf_start_UV;
		} else if(vbuf_info->m_flags & FLAG_MASK_BOTTOM_FIELD_ONLY) {
			*pInputBuf0YAddr  = (UINT) vbuf_info->m_pbuf_start + vbuf_info->m_buf_stride;
			*pInputBuf0UVAddr = vbuf_info->m_buf_pbuf_start_UV + vbuf_info->m_buf_stride_UV;
		} else {
			OVP_DBG_ERROR("Invalid Vbuf Flags[0x%X] \n", vbuf_info->m_flags);
		}
		return OVP_FRAMETYPE_HALF_FIELD;
	}
	return OVP_FRAMETYPE_FULL_FIELD;
}
/***********************************************************************
 * FUNCTION: Prepare hardware command to pushed to OVP for processing.
 * INPUT: pstOvpObj -	Object of OVP
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
static INT OVP_makeOvpCommand(OVP_OBJ *pstOvpObj)
{
	T32OvpCmd_VideoFormat stVidFmt;
	T32OvpCmd_Control     stCmdCtrl;
	UINT uiResInvalid;
	SIE_OvpCmd volatile *pstOvpBufInfo;
#ifndef CONFIG_AMP_TEE_ENABLE
	VDEC_VID_BUF_DESC *frameDescr;
#endif
	VBUF_INFO *vbuf_info;
	UINT uiHeight, uiCurrFdWidth, uiNextFdWidth, uiCurrFdHeight, uiNextFdHeight;

	/* Validate frame count before accessing list */
	if(pstOvpObj->m_uchNoOfFramesWaiting < 2) {
		OVP_DBG_HIGH("No De-interlacing is possible since no. of frames available are %d only \n", pstOvpObj->m_uchNoOfFramesWaiting);
		return E_INVALIDARG;
	}

	/* Validate frame list head is not NULL */
	if (!pstOvpObj->m_stFrameListHead) {
		OVP_DBG_ERROR("Frame list head is NULL\n");
		return E_INVALIDARG;
	}

	memset(&stVidFmt, 0, sizeof(T32OvpCmd_VideoFormat));
	memset(&stCmdCtrl, 0, sizeof(T32OvpCmd_Control));

	/* Program input field addresses */
#ifndef CONFIG_AMP_TEE_ENABLE
	frameDescr = ( VDEC_VID_BUF_DESC *)pstOvpObj->m_stFrameListHead->m_pbufInFrameDescr;	/* Frame at the beginning of list */
	if (!frameDescr) {
		OVP_DBG_ERROR("Frame descriptor is NULL\n");
		return E_INVALIDARG;
	}
	vbuf_info = frameDescr->user_data;
#else
	vbuf_info = (VBUF_INFO *) pstOvpObj->m_stFrameListHead->m_pbufInFrameDescr;
#endif
	if (!vbuf_info) {
		OVP_DBG_ERROR("Video buffer info is NULL\n");
		return E_INVALIDARG;
	}

	//Update FrameDesc info as per OVP configuration.
	if(!pstOvpObj->m_uchFrameOrField) {
		vbuf_info->m_is_frame_seq = 0;
		vbuf_info->m_OrgInputInterlaced = 0;
	} else {
		vbuf_info->m_is_frame_seq = 1;
		vbuf_info->m_OrgInputInterlaced = 1;
	}
	pstOvpBufInfo = &pstOvpObj->pOvp_cmd_obj->stOvpBufInfo;
	/*u32OvpCmd_InputBuffer[2|3] refers uInputBuffer_InputBuf0[Y|UV]Addr
	 *u32OvpCmd_InputBuffer[4|5] refers uInputBuffer_InputBuf1[Y|UV]Addr*/
	if(OVP_FRAMETYPE_HALF_FIELD == OVP_UpdateInputBuffers(&pstOvpBufInfo->u32OvpCmd_InputBuffer2,
				&pstOvpBufInfo->u32OvpCmd_InputBuffer3,
				&pstOvpBufInfo->u32OvpCmd_InputBuffer4,
				&pstOvpBufInfo->u32OvpCmd_InputBuffer5,
				vbuf_info)) {
		OvpFrameUpdateHalfFieldCnt(pstOvpObj, vbuf_info->m_flags & FLAG_MASK_TOP_FIELD_ONLY);
	}

	//Get Width and Height of current frame
	uiCurrFdWidth	=   vbuf_info->m_active_width;
	uiCurrFdHeight	=   vbuf_info->m_active_height;

	OVP_DBG_INFO("buff0_Y = %x, UV = %x, Buff1_Y = %x, UV = %x\n", pstOvpBufInfo->uInputBuffer_InputBuf0YAddr, pstOvpBufInfo->uInputBuffer_InputBuf0UVAddr, pstOvpBufInfo->uInputBuffer_InputBuf1YAddr, pstOvpBufInfo->uInputBuffer_InputBuf1UVAddr);

	/* Next frame - Frame at position 1 of the list */
	/* Validate that next frame exists in list */
	if (!pstOvpObj->m_stFrameListHead->pNext) {
		OVP_DBG_ERROR("Next frame not available in list\n");
		return E_INVALIDARG;
	}
#ifndef CONFIG_AMP_TEE_ENABLE
	frameDescr = ( VDEC_VID_BUF_DESC *)pstOvpObj->m_stFrameListHead->pNext->m_pbufInFrameDescr;
	if (!frameDescr) {
		OVP_DBG_ERROR("Next frame descriptor is NULL\n");
		return E_INVALIDARG;
	}
	vbuf_info = frameDescr->user_data;
#else
	vbuf_info = (VBUF_INFO *) pstOvpObj->m_stFrameListHead->pNext->m_pbufInFrameDescr;
#endif
	if (!vbuf_info) {
		OVP_DBG_ERROR("Next frame video buffer info is NULL\n");
		return E_INVALIDARG;
	}

	//Update FrameDesc info as per OVP configuration.
	if(!pstOvpObj->m_uchFrameOrField) {
		vbuf_info->m_is_frame_seq = 0;
		vbuf_info->m_OrgInputInterlaced = 0;
	} else {
		vbuf_info->m_is_frame_seq = 1;
		vbuf_info->m_OrgInputInterlaced = 1;
	}
	/* u32OvpCmd_InputBuffer[6|7] refers uInputBuffer_InputBuf2[Y|UV]Addr
	 *u32OvpCmd_InputBuffer[8|9] refers uInputBuffer_InputBuf3[Y|UV]Addr*/
	OVP_UpdateInputBuffers(&pstOvpBufInfo->u32OvpCmd_InputBuffer6,
			&pstOvpBufInfo->u32OvpCmd_InputBuffer7,
			&pstOvpBufInfo->u32OvpCmd_InputBuffer8,
			&pstOvpBufInfo->u32OvpCmd_InputBuffer9,
			vbuf_info);
	//Get Width and height of Next Frame
	uiNextFdWidth	=   vbuf_info->m_active_width;
	uiNextFdHeight	=   vbuf_info->m_active_height;

	OVP_DBG_INFO("buff2_Y = %x, UV = %x, Buff3_Y = %x, UV = %x\n", pstOvpBufInfo->uInputBuffer_InputBuf2YAddr, pstOvpBufInfo->uInputBuffer_InputBuf2UVAddr, pstOvpBufInfo->uInputBuffer_InputBuf3YAddr, pstOvpBufInfo->uInputBuffer_InputBuf3UVAddr);

	//Check whether resolution of both frames is same or not.
	if((uiCurrFdWidth != uiNextFdWidth) || (uiCurrFdHeight != uiNextFdHeight)) {
		OVP_DBG_ERROR("uiCurrFdWidth = %d, uiCurrFdHeight = %d, uiNextFdWidth = %d, uiNextFdHeight = %d\n", uiCurrFdWidth, uiCurrFdHeight, uiNextFdWidth, uiNextFdHeight);
		OVP_DBG_ERROR("Resolution of curr and next frames are not equal.\n");
		uiResInvalid = 1;
	} else
		uiResInvalid = 0;

	//Programm Input HV size and Stride.
	uiHeight = vbuf_info->m_active_height;

	if(vbuf_info->m_active_height % 4) {
		//Height should be multiple of 4.
		uiHeight = (vbuf_info->m_active_height / 4) * 4;
	}
	pstOvpBufInfo->uInputBuffer_InputHSize     =	vbuf_info->m_active_width;
	pstOvpBufInfo->uInputBuffer_InputVSize     =	uiHeight; //For 480I, height need to be given as 480.
	pstOvpBufInfo->uInputBuffer_InputYStride   =	vbuf_info->m_buf_stride * 2;	//Need to read only one field
	pstOvpBufInfo->uInputBuffer_InputUVStride  =	vbuf_info->m_buf_stride_UV * 2;

	OVP_DBG_INFO("width = %d, Height = %d, Stride_Y=%d, Stride_UV=%d\n", pstOvpBufInfo->uInputBuffer_InputHSize, pstOvpBufInfo->uInputBuffer_InputVSize, pstOvpBufInfo->uInputBuffer_InputYStride, pstOvpBufInfo->uInputBuffer_InputUVStride);

#ifndef CONFIG_AMP_TEE_ENABLE
	frameDescr = ( VDEC_VID_BUF_DESC *)pstOvpObj->m_stFrameListHead->m_pbufInFrameDescr;
	vbuf_info = frameDescr->user_data;
#else
	vbuf_info = (VBUF_INFO *) pstOvpObj->m_stFrameListHead->m_pbufInFrameDescr;
#endif

	//Programm Output HV size and Stride.
	pstOvpBufInfo->uOutputBuffer_OutputHSize    =	vbuf_info->m_active_width;
	pstOvpBufInfo->uOutputBuffer_OutputVSize    =	uiHeight;
	pstOvpBufInfo->uOutputBuffer_OutputYStride  =	vbuf_info->m_buf_stride;
	pstOvpBufInfo->uOutputBuffer_OutputUVStride =	vbuf_info->m_buf_stride_UV;

	//Program  Output filed addresses
	pstOvpBufInfo->uOutputBuffer_OutputBuf0YAddr	 = vbuf_info->m_pbuf_Out_start_1;

	pstOvpBufInfo->uOutputBuffer_OutputBuf0UVAddr = vbuf_info->m_pbuf_Out_start_UV_1;

	//Mark that Buff1 is valid.
	vbuf_info->m_Out_buf1_valid =	1;

	//If the frames are of different resolutions, then mark this buffer as invalid.
	if(uiResInvalid)
		vbuf_info->m_Out_buf1_valid = 0;

	//Mark that Frmae is in Use.
	vbuf_info->m_FrameDesc_RefStat |= OVP_FRAME_IN_USE;

	//Update Bit depth and height
	vbuf_info->m_bits_per_pixel = pstOvpObj->m_uchOutBitDepth;
	vbuf_info->m_active_height   = uiHeight;

	OVP_DBG_INFO("Y_base: %x, UV_base: %x width = %d, Height = %d, Stride_Y=%d, Stride_UV=%d\n", pstOvpBufInfo->uOutputBuffer_OutputBuf0YAddr, pstOvpBufInfo->uOutputBuffer_OutputBuf0UVAddr, pstOvpBufInfo->uOutputBuffer_OutputHSize, pstOvpBufInfo->uOutputBuffer_OutputVSize, pstOvpBufInfo->uOutputBuffer_OutputYStride , pstOvpBufInfo->uOutputBuffer_OutputUVStride);

#ifndef CONFIG_AMP_TEE_ENABLE
	frameDescr = ( VDEC_VID_BUF_DESC *)pstOvpObj->m_stFrameListHead->pNext->m_pbufInFrameDescr;
	vbuf_info = frameDescr->user_data;
#else
	vbuf_info = (VBUF_INFO *) pstOvpObj->m_stFrameListHead->pNext->m_pbufInFrameDescr;
#endif

	pstOvpBufInfo->uOutputBuffer_OutputBuf1YAddr	 = vbuf_info->m_pbuf_Out_start_0;
	pstOvpBufInfo->uOutputBuffer_OutputBuf1UVAddr = vbuf_info->m_pbuf_Out_start_UV_0;

	if(pstOvpObj->m_uchOneOrTwo)
		vbuf_info->m_Out_buf0_valid = 0;
	else if(0 == pstOvpObj->m_uchOneOrTwo) {
		if(uiResInvalid) {
			//If the frames are of different resolutions, then mark this buffer as invalid.
			vbuf_info->m_Out_buf1_valid = 0;
		} else
			vbuf_info->m_Out_buf0_valid = 1;
	}

	//Mark that Buff1 is invalid.
	vbuf_info->m_Out_buf1_valid =	0;
	//Mark that Frmae is in Use.
	vbuf_info->m_FrameDesc_RefStat |= OVP_FRAME_IN_USE;

	vbuf_info->m_bits_per_pixel = pstOvpObj->m_uchOutBitDepth;
	vbuf_info->m_active_height   = uiHeight;

	//Fill Video Format support
	pstOvpBufInfo->uVideoFormat_InputUVSample =   0;  //420
	pstOvpBufInfo->uVideoFormat_InputPlanes   =   1;  //SP

	if(8 == pstOvpObj->m_uchInBitDepth)
		pstOvpBufInfo->uVideoFormat_InputBitDepth =   0;  //8-bit

	if(10 == pstOvpObj->m_uchInBitDepth)
		pstOvpBufInfo->uVideoFormat_InputBitDepth =   1;  //10-bit

	pstOvpBufInfo->uVideoFormat_InputTile	   =   0;  //Raster scan
	pstOvpBufInfo->uVideoFormat_InputInterlaced   =   1;  //Input is interlaced
	pstOvpBufInfo->uVideoFormat_OutputUVSample    =   0;  //420
	pstOvpBufInfo->uVideoFormat_OutputPlanes      =   1;  //SP

	if(8 == pstOvpObj->m_uchOutBitDepth)
		pstOvpBufInfo->uVideoFormat_OutputBitDepth    =   0;  //8-bit

	if(10 == pstOvpObj->m_uchOutBitDepth)
		pstOvpBufInfo->uVideoFormat_OutputBitDepth    =   1;  //10-bit

	OVP_DBG_INFO("In BitDepth= %d, OutBitDepth = %d\n", pstOvpObj->m_uchInBitDepth, pstOvpObj->m_uchOutBitDepth);
	//Program Control information
	pstOvpBufInfo->uControl_IOC  =   1;
	pstOvpBufInfo->uControl_OutputWholeFrame =   pstOvpObj->m_uchFrameOrField;
	pstOvpBufInfo->uControl_OutputOneField   =   pstOvpObj->m_uchOneOrTwo;

	return S_OK;
}

/***********************************************************************
 * FUNCTION: Commit prepared hardware command to OVP Hardware.
 * INPUT: pstOvpObj -	OVP object
 *
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
static INT OVP_commitCmd(OVP_OBJ *pstOvpObj)
{
	UINT uiIntrSts, uiCmdAddr, uiShmOffset;

	GA_REG_WORD32_READ(OVP_REG(RA_OvpReg_IntStatus), &uiIntrSts);

	//check cmd FIFO
	if((uiIntrSts & INT_STATUS_CMD_ADDR_FULL)) {
		OVP_DBG_ERROR("OVP Err: command FIFO full %x! \n", uiIntrSts);
		return E_OUTOFMEMORY;
	}

	uiShmOffset = offsetof(OVP_CMD_OBJ, stOvpBufInfo);
#ifdef CONFIG_AMP_TEE_ENABLE
	TEE_FlushCache(&pstOvpObj->pOvp_cmd_obj->stOvpBufInfo, sizeof(SIE_OvpCmd));
	uiCmdAddr = pstOvpObj->pOvp_cmd_obj->m_uiPhyAddr + uiShmOffset;
#else
	//AMP_SHM_CleanCache(pstOvpObj->m_uiShmHandle, uiShmOffset ,sizeof(SIE_OvpCmd));
	uiCmdAddr =  VPP_SHM_GetPhysicalAddress(pstOvpObj->m_uiShmHandle, 0);
#endif
	GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_CmdAddrPush),uiCmdAddr);

	//check cmd done
	GA_REG_WORD32_READ(OVP_REG(RA_OvpReg_IntStatus), &uiIntrSts);

	while((uiIntrSts & INT_STATUS_CMD_ADDR_DONE)) {
		OVP_DBG_INFO("The intr sts = %x in %s\n", uiIntrSts, __FUNCTION__);
		GA_REG_WORD32_READ(OVP_REG(RA_OvpReg_IntStatus), &uiIntrSts);
	}

	return S_OK;
}

/***********************************************************************
 * FUNCTION: Receive input frame and process.
 * INPUT: pstOvpMsgHeader   -	Message header whihc has details of
 *	    frames and configuration of OVP.
 * RETURN:  S_OK    -	Success.
 *******************************************************************/

INT OVP_PushFrame(OVP_MSG_HEADER_T *pstOvpMsgHeader)
{
	OVP_OBJ *pstOvpObj;
	HRESULT uiRetStat = S_OK;
	UINT uiLoopCount;
	UINT uiOvpObjIndex;

	//Check whether OVP obj is already created for this client or not.
	for(uiLoopCount = 0; uiLoopCount < ovp->m_uchNoOfObjs; uiLoopCount++) {
		if(ovp->m_pOvp_objs[uiLoopCount]->m_uchOvpClientId == pstOvpMsgHeader->m_uchBuffID) {
			//OVP OBJ is already avaialble
			break;
		}
	}

	uiOvpObjIndex = uiLoopCount;

	//Create Object if no object is available with received buffer ID
	if(uiLoopCount >= ovp->m_uchNoOfObjs) {
		uiRetStat = OVP_LIST_CreateClientList(ovp, pstOvpMsgHeader->m_uchBuffID,(INT *)&uiOvpObjIndex);
		if (uiRetStat != S_OK) {
			OVP_DBG_ERROR("Failed to create client list for buff ID %d (status: %d)\n", pstOvpMsgHeader->m_uchBuffID, uiRetStat);
			return uiRetStat;
		}
		OVP_DBG_ERROR("Object is not found. Creating Object for buff ID %d [@index%d]\n", pstOvpMsgHeader->m_uchBuffID, uiOvpObjIndex);
		OvpResetFrameCnt(ovp->m_pOvp_objs[uiOvpObjIndex]);
		//Reset Firmware for newly created OVP object
		OVP_DEINT_FwReset(ovp->m_pOvp_objs[uiOvpObjIndex]);
		//Mark Status as Active.
		ovp->m_pOvp_objs[uiOvpObjIndex]->m_iStatus = OVP_OBJECT_STATUS_STARTED;
	}

	pstOvpObj = ovp->m_pOvp_objs[uiOvpObjIndex];

	//Get details received in message.
	pstOvpObj->m_uchInBitDepth	= pstOvpMsgHeader->m_uchInBitDepth;
	pstOvpObj->m_uchOutBitDepth	= pstOvpMsgHeader->m_uchOutBitDepth;
	pstOvpObj->m_uchFrameOrField	= pstOvpMsgHeader->m_uchOutputWholeFrame;
	pstOvpObj->m_uchOneOrTwo	= pstOvpMsgHeader->m_uchOutputOneField;
	pstOvpObj->m_uchPriority	= pstOvpMsgHeader->m_uchPriority;

	//Add received frame to List
	uiRetStat |= OVP_LIST_AddFrameDesc(pstOvpObj, (void*) pstOvpMsgHeader->pstFrameDesc);

	//Update number of frames in the queue.
	pstOvpObj->m_uchNoOfFramesWaiting++;
	OvpFrameUpdateInCnt(pstOvpObj);

	//Check whether OVP has 4 fields for de-interlacing or no
	OVP_DBG_INFO("m_uchNoOfFramesWaiting = %d\n", pstOvpObj->m_uchNoOfFramesWaiting);

	if(pstOvpObj->m_uchNoOfFramesWaiting < 2)
		return uiRetStat;

	//Check whether OVP is active or not.
	if(OVP_STATUS_RUNNING != ovp->m_iStatus) {
		diFwProcess(pstOvpObj);
		//Do Firmware Processing
		diSendDataToHw(pstOvpObj);
		//Prepare OVP command
		OVP_makeOvpCommand(pstOvpObj);
		//Commit command to Hardware
		OVP_commitCmd(pstOvpObj);
		//Update Current object under execution
		ovp->m_uchCurrObjUnderExec = uiOvpObjIndex;

		ovp->m_iStatus = OVP_STATUS_RUNNING;
		OVP_DBG_MID("Marking OVP status as RUNNING\n");
	}
	return uiRetStat;
}

/***********************************************************************
 * FUNCTION: ISR handler for OVP.
 * INPUT: pstOvpMsgHeader   -	Message header whihc has details of
 *	    frames and configuration of OVP.
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_ProcessFrame(OVP_MSG_HEADER_T *pstOvpMsgHeader, UINT uiIntrStat)
{
	OVP_OBJ *pstOvpObj;
	HRESULT uiRetStat = S_OK;
	UINT uiLoopCount, reg_val, uiAreFramesAvailable, uiOvpObjIndex;
#ifndef CONFIG_AMP_TEE_ENABLE
	VDEC_VID_BUF_DESC *frameDescr;
#endif
	VBUF_INFO *vbuf_info;
	OVP_FRAMEDESC_LIST stFrameDesc;
	SIE_OvpCmd volatile *pstOvpBufInfo;

	//Check for error interrupt
	if(uiIntrStat & (INT_STATUS_CMD_ADDR_ERROR | INT_STATUS_CMD_ADDR_AXI_ERROR))
		OVP_DBG_ERROR("Received Error interrupt: 0x%x\n", uiIntrStat);

	//Check which clients command done interrupt is set
	for(uiLoopCount = 0; uiLoopCount < ovp->m_uchNoOfObjs; uiLoopCount++) {
		pstOvpObj = ovp->m_pOvp_objs[uiLoopCount];
		pstOvpBufInfo = &pstOvpObj->pOvp_cmd_obj->stOvpBufInfo;

		TEE_FlushCache(pstOvpBufInfo, sizeof(SIE_OvpCmd));
		//AMP_SHM_CleanCache(pstOvpObj->m_uiShmHandle, 0, sizeof(OVP_OBJ));

		if(pstOvpBufInfo->uReturn_Done & 0x1) {
			uiOvpObjIndex = uiLoopCount;

			pstOvpBufInfo->uReturn_Done = 0;

			//clear cmd done int
			GA_REG_WORD32_READ(OVP_REG(RA_OvpReg_IntStatus), &reg_val);
			GA_REG_WORD32_WRITE(OVP_REG(RA_OvpReg_IntStatus), (reg_val | INT_STATUS_CMD_ADDR_DONE));

			OVP_DBG_LOW("Found Object for received interrupt at %d.\n", uiOvpObjIndex);
			break;
		}
	}

	if(uiLoopCount >= ovp->m_uchNoOfObjs) {
		OVP_DBG_ERROR("No object is found for interrupt and intr status = 0x%x. No. of objects are %d\n",uiIntrStat, ovp->m_uchNoOfObjs);
		for(uiLoopCount = 0; uiLoopCount < ovp->m_uchNoOfObjs; uiLoopCount++) {
			pstOvpObj = ovp->m_pOvp_objs[uiLoopCount];
			OVP_DBG_ERROR("Client Id in OVP driver is %d and requested buff ID from OVPP component is %d\n", pstOvpObj->m_uchOvpClientId, pstOvpMsgHeader->m_uchBuffID);
		}
		return uiRetStat;
	}

	if(pstOvpMsgHeader == NULL) {
		uiRetStat |= E_BADVALUE;
		OVP_DBG_ERROR("Msg header is invalid\n");
		return uiRetStat;
	}

	//Get the exact frame to be pushed
	memset(&stFrameDesc, 0, sizeof(OVP_FRAMEDESC_LIST));
	uiRetStat = OVP_LIST_GetListNode(pstOvpObj, &stFrameDesc);
	if (uiRetStat != S_OK) {
		OVP_DBG_ERROR("Failed to get list node from OVP object (status: %d)\n", uiRetStat);
		pstOvpMsgHeader->m_uiErrorCode = uiRetStat;
		return uiRetStat;
	}

	pstOvpMsgHeader->m_uchBuffID = stFrameDesc.m_uchClientId;
	pstOvpMsgHeader->m_uchOutputWholeFrame	=   pstOvpObj->m_uchFrameOrField;
	pstOvpMsgHeader->m_uchOutputOneField	=   pstOvpObj->m_uchOneOrTwo;
	pstOvpMsgHeader->m_uchInBitDepth	=   pstOvpObj->m_uchInBitDepth;
	pstOvpMsgHeader->m_uchOutBitDepth	=   pstOvpObj->m_uchOutBitDepth;
	pstOvpMsgHeader->pstFrameDesc		=   stFrameDesc.m_pbufInFrameDescr;
	pstOvpMsgHeader->m_uchPriority		=   pstOvpObj->m_uchPriority ;
	pstOvpMsgHeader->m_uiErrorCode		=   uiRetStat;

	OVP_DBG_LOW("m_uchBuffID = %d, m_uchOutputWholeFrame = %d, m_uchOutputOneField = %d, \
			m_uchInBitDepth = %d, m_uchOutBitDepth = %d, pstFrameDesc = 0x%x,  \
			m_uchPriority = %d, m_uiErrorCode = %d\n",
			pstOvpMsgHeader->m_uchBuffID, pstOvpMsgHeader->m_uchOutputWholeFrame,
			pstOvpMsgHeader->m_uchOutputOneField, pstOvpMsgHeader->m_uchInBitDepth,
			pstOvpMsgHeader->m_uchOutBitDepth, pstOvpMsgHeader->pstFrameDesc,
			pstOvpMsgHeader->m_uchPriority, pstOvpMsgHeader->m_uiErrorCode);

	//Delete the frame descriptor form list
	uiRetStat |= OVP_LIST_DeleteNode(pstOvpObj, 0);
	//Mark that Frmae is not in Use.
#ifndef CONFIG_AMP_TEE_ENABLE
	frameDescr  = ( VDEC_VID_BUF_DESC *) pstOvpMsgHeader->pstFrameDesc;
	vbuf_info = frameDescr->user_data;
#else
	vbuf_info = (VBUF_INFO *) pstOvpMsgHeader->pstFrameDesc;
#endif

	vbuf_info->m_FrameDesc_RefStat &= (~0 ^ OVP_FRAME_IN_USE);

	//Reduce no. of frames waiting by one.
	if(pstOvpObj->m_uchNoOfFramesWaiting > 0)
		pstOvpObj->m_uchNoOfFramesWaiting--;

	OvpFrameUpdateOutCnt(pstOvpObj);
	OvpFrameUpdate(pstOvpObj, (UINT)vbuf_info->m_pbuf_start, vbuf_info->m_active_width * vbuf_info->m_active_height * 1.5, vbuf_info->m_active_width, vbuf_info->m_active_height);

	//Check whether any more frames are ready for de-interlacing or not.
	if(uiLoopCount == ovp->m_uchNoOfObjs - 1)
		uiLoopCount = 0;
	else
		uiLoopCount = uiOvpObjIndex + 1;

	uiAreFramesAvailable = 0;
	/* Scan all objects unconditionally — do not pre-gate on the just-served
	 * object's count, which would wrongly suppress scheduling other clients. */
	do {
		pstOvpObj = ovp->m_pOvp_objs[uiLoopCount];

		if(OVP_OBJECT_STATUS_STARTED == pstOvpObj->m_iStatus) {
			if(pstOvpObj->m_uchNoOfFramesWaiting >= 2) {
				uiOvpObjIndex = uiLoopCount;
				uiAreFramesAvailable = 1;
				break;
			}
		}
		//Check whether traversing of all objects is completed or not
		if(uiOvpObjIndex == uiLoopCount) {
			//No more frames are waiting for de-interlacing
			uiAreFramesAvailable = 0;
			break;
		}

		//Reset LoopCount if we reach end of objects
		if(uiLoopCount == ovp->m_uchNoOfObjs - 1)
			uiLoopCount = 0;
		else
			uiLoopCount++;
	} while(1);

	/* Frame descriptor ownership transfers to caller; do NOT free here */

	//If no more frames are waiting Update status to INACTIVE
	if(!uiAreFramesAvailable)
	{
		OVP_DBG_MID("Marking OVP status as NOT RUNNING \n");
		ovp->m_iStatus = OVP_STATUS_NOT_RUNNING;
		return uiRetStat;
	}
	diFwProcess(pstOvpObj);
	//If frames are waiting prepare OVP command and push to Hardware.
	OVP_makeOvpCommand(pstOvpObj);
	//Do Firmware Processing
	diSendDataToHw(pstOvpObj);
	//Commit command to Hardware
	OVP_commitCmd(pstOvpObj);

	ovp->m_uchCurrObjUnderExec  =	uiOvpObjIndex;

	ovp->m_iStatus = OVP_STATUS_RUNNING;

	return uiRetStat;
}

/***********************************************************************
 * FUNCTION: Release all OVP clients.
 * INPUT: OVP
 *
 * RETURN:  S_OK    -	Success.
 *******************************************************************/
INT OVP_ReleaseObjects(OVP *pstOvp)
{
	UINT32 uiLoopCount;
	OVP_OBJ *pstOvpObj;

	if(NULL == pstOvp)
		return (E_BADVALUE);

	//Check whether Buff ID is valid or not
	for(uiLoopCount = 0; uiLoopCount < ovp->m_uchNoOfObjs; uiLoopCount++) {
		pstOvpObj = ovp->m_pOvp_objs[uiLoopCount];
		//Mark Status as Stopped;
		pstOvpObj->m_iStatus = OVP_OBJECT_STATUS_STOPPED;
		/*  Delete Full list in the revesr order to make sure that
		    any previous command is not in progress */
		OVP_LIST_DeleteFullList(pstOvpObj);
	}

	return S_OK;
}
