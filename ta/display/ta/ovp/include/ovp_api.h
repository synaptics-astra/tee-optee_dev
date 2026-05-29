/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef _OVP_API_H_
#define _OVP_API_H_

#include "com_type.h"

#define OVP_FRAME_IN_USE    1

typedef struct OVP_MSG_HEADER_S
{
	UINT8 m_uchBuffID;	/* Buffer Id to identify to which client this buffer belongs to */
	UINT8 m_uchOutputWholeFrame;	/* Output as a frame or interpolated field */
	UINT8 m_uchOutputOneField;  /* Output single/two frames/fields */
	UINT8 m_uchInBitDepth;	    /* Bit-depth of input data (bits per pixel-8/10) */
	UINT8 m_uchOutBitDepth;     /* Bit Depth of generated data */
	UINT8 m_uchPriority;	    /* Priority of this client */
	void *pstFrameDesc;	/* Input Frame descriptors */
	INT32 m_uiErrorCode;	/* Error code of the processed frame */
} OVP_MSG_HEADER_T;

typedef enum
{
	OVP_STATUS_MIN	=   0,
	OVP_STATUS_INACTIVE,
	OVP_STATUS_ACTIVE,
	OVP_STATUS_RUNNING,
	OVP_STATUS_NOT_RUNNING,
	OVP_STATUS_MAX
} OVP_STATUS_T;

typedef enum
{
	OVP_OBJECT_STATUS_MIN	=   0,
	OVP_OBJECT_STATUS_STARTED,
	OVP_OBJECT_STATUS_STOPPED,
	OVP_OBJECT_STATUS_MAX
} OVP_OBJECT_STATUS_T;

INT MV_OVP_Create(void);
INT MV_OVP_Destroy(void);
INT MV_OVP_Suspend(BOOL optimize);
INT MV_OVP_Resume(BOOL optimize);
INT MV_OVP_PushFrame(OVP_MSG_HEADER_T *pstOvpMsgHeader);
INT MV_OVP_ProcessFrame(OVP_MSG_HEADER_T *pstOvpMsgHeader, UINT uiIntrSts);
INT MV_OVP_ReleaseBuffer(UINT32 uiBuffId, OVP_MSG_HEADER_T *pstOvpMsgHeader);
INT MV_OVP_GetNoOfFramesWaiting(UINT32 uiBuffId, UINT32 *puiFramesWaiting);
INT MV_OVP_RegisterIsrTaskFunc(void *pFIsrTaskFunc);
INT MV_OVP_Get_Clr_IntrSts(UINT32 *puiIntrSts);
int MV_OVP_InitOvpCmdObj(unsigned int phShm, unsigned int shmSize);

#endif /* _OVP_API_H_ */
