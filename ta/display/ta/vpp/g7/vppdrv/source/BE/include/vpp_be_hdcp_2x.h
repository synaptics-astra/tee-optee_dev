/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __VPP_BE_HDCP2X_H__
#define __VPP_BE_HDCP2X_H__

#ifndef CONFIG_AMP_TEE_ENABLE
#include "OSAL_api.h"
#endif
#ifdef CONFIG_AMP_TEE_ENABLE
#include "tee_ta_osal.h"
#endif

#include "vpp_be_hdmitx.h"
/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 * TypeDefs
 *-----------------------------------------------------------------------------
 */

typedef enum tagDWNSTRM_DEV_VERSION{
    DWNSTRM_NOT_CONN,
    DWNSTRM_DEV_1_4,
    DWNSTRM_DEV_2_0,
    DWNSTRM_DEV_2_2,
    DWNSTRM_DEV_UNSUPPORTED,
} DWNSTRM_DEV_VERSION;

enum HDCP_TX_SUB_STATES{
    HDCP2X_TX_SUB_STATE_IDLE,
    HDCP2X_TX_SUB_STATE_AKE_INIT,
    HDCP2X_TX_SUB_STATE_AKE_SEND_CERT,
    HDCP2X_TX_SUB_STATE_AKE_SEND_H_PRIME,
    HDCP2X_TX_SUB_STATE_AKE_SEND_PAIRING_INFO,
    HDCP2X_TX_SUB_STATE_LC_INIT,
    HDCP2X_TX_SUB_STATE_LC_SEND_LPRIME,
    HDCP2X_TX_SUB_STATE_SKE_SEND_EKS,
    HDCP2X_TX_SUB_STATE_REPEATER_TOPOLOGY,
    HDCP2X_TX_SUB_STATE_REPEATER_CONTENT_MANAGEMENT
};

enum HDCP_TX_MAIN_STATES{
    HDCP2X_TX_STATE_IDLE,
    HDCP2X_TX_STATE_AKE,
    HDCP2X_TX_STATE_LC,
    HDCP2X_TX_STATE_KS,
    HDCP2X_TX_STATE_REPEATER_AUTH
};

enum HDCP2X_TX_AUTH_STATE {
    HDCP2X_TX_AUTH_IDLE,
    HDCP2X_TX_AUTH_DONE,
};

#endif // __VPP_BE_HDCP_H__

