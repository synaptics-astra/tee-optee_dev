/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef _OVP_APIFUNCS_H_
#define _OVP_APIFUNCS_H_

#ifdef __cplusplus
extern "C" {
#endif

void OVP_INPUT_Task(void);
INT OVP_Reset(void);
INT OVP_PushFrame(OVP_MSG_HEADER_T *pstOvpMsgHeader);
INT OVP_ProcessFrame(OVP_MSG_HEADER_T *pstOvpMsgHeader, UINT uiIntrStat);

#ifdef __cplusplus
}
#endif
#endif

