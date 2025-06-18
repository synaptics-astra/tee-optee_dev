/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __VPP_BE_HDCP_H__
#define __VPP_BE_HDCP_H__

#include "com_type.h"

/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 * TypeDefs
 *-----------------------------------------------------------------------------
 */
typedef enum tagHDCP_STATE
{
    HDCP_STATE_AUTH_IDLE = 0x00,
    HDCP_STATE_AUTH_AKSV_TRIG,
    HDCP_STATE_AUTH_ANGEN_TRIG,
    HDCP_STATE_AUTH_PUB_KEY_EXCNGED,
    HDCP_STATE_RPTR_CHK,
    HDCP_STATE_AUTH_DONE,
    HDCP_STATE_AUTH_ERR,
    HDCP_STATE_AUTH_ERR_PWROFF,
}HDCP_STATE;

typedef enum tagHDCP_RETURNS
{
    HDCP_ERR_OK = 0x00,
    HDCP_ERR_INVALID_STATE,
    HDCP_ERR_INVALID_DATA,
    HDCP_ERR_TX_PRV_KEYS_NOT_STORED,
    HDCP_ERR_RX_DEV_NOT_RDY,
    HDCP_ERR_HDMI_MODE_UNSUPPORTED,
    HDCP_ERR_DATA_TXRX_FAILED,
    HDCP_ERR_INT_UNSUPPORTED,
    HDCP_ERR_KEY_REVOKED,
    HDCP_ERR_AUTH_FAILURE,
    HDCP_ERR_NOMEM,
}HDCP_RETURNS;

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
HDCP_RETURNS BE_HDCP_StartHdcp(VPP_OBJ *vpp_obj, UINT32 *pAuthState, BOOL curOutHdmiMode);
HDCP_RETURNS BE_HDCP_StopHdcp(VPP_OBJ *vpp_obj, UINT32 *pAuthState);
HDCP_RETURNS BE_HDCP_HandleInterrupt(VPP_OBJ *vpp_obj,UINT32 *pAuthState);
HDCP_RETURNS BE_HDCP_EnableEncryption(VPP_OBJ *vpp_obj,BOOL Enable,BOOL bufWrite);
HDCP_RETURNS BE_HDCP_ConfigureSRM(VPP_OBJ *vpp_obj,UINT8 *pSrmBuf, int srmBufLen, BOOL forceUpdate, BOOL *pSrmUpdated, UINT32 *pErrorInfo);

#endif
