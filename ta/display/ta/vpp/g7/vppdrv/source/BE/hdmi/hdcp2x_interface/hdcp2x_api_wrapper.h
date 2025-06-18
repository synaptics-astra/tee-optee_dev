/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __HDCP2X_API_WRAPPER_H__
#define __HDCP2X_API_WRAPPER_H__

/************************************************************************/
/******************* global function declarations ***********************/
/************************************************************************/
TEE_Result HDCP2X_InitCtx(TEE_TASessionHandle* sessionContext);
void HDCP2X_DeInitCtx(TEE_TASessionHandle sessionContext);
bool HDCP2X_IsESMAlive(BE_HDMITX_DATA *pHdmiTx);
INT HDCP2X_StopAuthentication(BE_HDMITX_DATA *pHdmiTx);
INT HDCP2X_StartAuthentication(BE_HDMITX_DATA *pHdmiTx);
INT HDCP2X_GetAuthenticationState(BE_HDMITX_DATA *pHdmiTx, HDCP2X_ESM_MONITOR_RET *esm_monitor_state);
INT HDCP2X_GetTopology(BE_HDMITX_DATA *pHdmiTx);
INT HDCP2X_UpdateContentStrmInfo(BE_HDMITX_DATA *pHdmiTx, BOOL hdcp_content_type);
INT HDCP2X_Resume(BE_HDMITX_DATA *pHdmiTx);
INT HDCP2X_Suspend(BE_HDMITX_DATA *pHdmiTx);
INT HDCP2X_LoadDownstreamCapability(BE_HDMITX_DATA *pHdmiTx);
INT HDCP2X_SetMode(BE_HDMITX_DATA *pHdmiTx, INT opMode);
#endif  //__HDCP2X_API_WRAPPER_H__
