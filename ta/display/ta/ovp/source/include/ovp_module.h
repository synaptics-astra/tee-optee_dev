/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef _OVP_MODULE_H_
#define _OVP_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ovp_internal.h"
#include "ovp_deint_param.h"
#include "com_type.h"
#include "ovp.h"
#include "tee_ta_buffer.h"

/* OVP module magic number */
#define OVP_MAGIC    0xBA11FACE
#define SHM_ALIGN_SIZE_4_KB 4096
#define OVP_SHM_ALIGN_ROUNDUP(size)  ((size + SHM_ALIGN_SIZE_4_KB - 1) & (~(SHM_ALIGN_SIZE_4_KB - 1)))

/* Match below define with linux kernel source
 * drivers/soc/berlin/modules/ovp/v4l2/ovp_v4l2.h
 * Shared Memory allocated per context for HW CMD
 */
#define OVPD_HW_CMD_SHM_SIZE_PER_CTX	1024

/************************************************/
/*********** OVP object data structures *********/
/************************************************/
typedef struct OVP_CMD_OBJ_T {
    UINT    m_uiShmHandle;
#ifdef CONFIG_AMP_TEE_ENABLE
    UINT    m_uiPhyAddr;
#endif
    SIE_OvpCmd volatile stOvpBufInfo;
}__attribute__ ((packed)) OVP_CMD_OBJ;

typedef struct OVP_OBJ_T
{
    UINT   m_uchPriority;
    UINT   m_uchOvpClientId;       /* To which client this object belongs to */
    UINT   m_uchInBitDepth;        /* Bit-depth of input data. */
    UINT   m_uchOutBitDepth;       /* Bit-depth of output data. */
    UINT   m_uchFrameOrField;      /* Full frame output or interpolated field output */
    UINT   m_uchOneOrTwo;          /* Whether to generate one or two fields/frames. */
    UINT   m_uchNoOfFramesWaiting; /* Number frames waiting for de-interlacing in m_pbufInFrameDescr. */

    INT     m_iStatus;              /* Status of object: Active/Inactive. */
    INT     m_iValid;               /* Whether this object is valid or not */

    OVP_FRAMEDESC_LIST  *m_stFrameListHead; /* Frames waiting */

    UINT32 frame_inCnt, frame_outCnt, frame_halfFldInCnt, frame_BottomFieldInCnt;
    UINT32 last_frame_addr, last_frame_cnt;
    UINT32 last_frame_width, last_frame_height;
    UINT32 last_frame_size;

    DI_FW_IDATA_STRUCT volatile m_stDiFwIData;    /* De-int Firmware Data */
    DI_FW_REG_STRUCT   volatile m_stDiFwReg;    /* De-int Firmware Registers */
    OVP_MADI    m_stMadiReg;        /* TODO: May need to delete later */
    OVP_CMD_OBJ *pOvp_cmd_obj;
}__attribute__ ((packed)) OVP_OBJ;

/************************************************/
/******** VPP container data structures *********/
/************************************************/
typedef struct OVP_T
{
    UINT    m_uchNoOfObjs;      /* Number of OVP objects created. */
    INT     m_uchCurrObjUnderExec;  /* Current object under execution */
    UINT    m_uiMagicNum;        /* Magic number to identify whether OVP is initialized or not */
    INT     m_iStatus;          /* Status of OVP */
    INT     m_ovp_need_to_exit;
    BOOL    m_suspend;
    OVP_OBJ *m_pOvp_objs[MAX_OVP_OBJECTS];
    VPP_BUFF_HANDLE bufHandle; /* Required to unmap/unregister during cleanup*/
}__attribute__ ((packed)) OVP;

/************************************************/
/*********** global VPP container ***************/
/************************************************/
#ifdef _OVP_API_C_
OVP *ovp;
#else
extern OVP *ovp;
#endif

void OVP_DEINT_ConfigDeintCore(void);
INT OVP_CreateAllObjs(OVP *pOvp);
INT OVP_DestroyAllObjs(OVP *pOvp);
INT OVP_LIST_GetNode(OVP_OBJ *pstOvpObj, UINT32 uiFrameDescNum, OVP_FRAMEDESC_LIST *pstNode);
INT OVP_LIST_DeleteNode(OVP_OBJ *pstOvpObj, UINT32 uiPosition);
INT OVP_LIST_DeleteClientList(OVP *pOvp, INT iOvpObjIndex);
INT OVP_LIST_CreateClientList(OVP *pOvp, INT iClientId, INT *piOvpObjIndex);
INT OVP_LIST_GetListNode(OVP_OBJ *pstOvpObj, OVP_FRAMEDESC_LIST *pstNode);
INT OVP_LIST_DeleteFullList(OVP_OBJ *pstOvpObj);
void OVP_DEINT_FwReset(OVP_OBJ *pstOvpObj);
INT OVP_LIST_AddFrameDesc(OVP_OBJ *pstOvpObj, void *pstFrameDesc);
void diFwProcess(OVP_OBJ *pstOvpObj);
void diSendDataToHw(OVP_OBJ *pstOvpObj);

#ifdef __cplusplus
}
#endif

#endif
