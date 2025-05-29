/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_BE_PRV_H_
#define _VPP_BE_PRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_AMP_TEE_ENABLE
#include "OSAL_api.h"
#else
#include "tee_ta_osal.h"
#endif

#include "vpp_api.h"
#include "hdmi_edid.h"

#define MAX_RES_INFO            64
#define HDMITX_MAX_FIFO_SIZE    10
#define HDMITX_TIMER_CONFIG     50 // in milliseconds
#define EARC_TIMER_CONFIG       40

#define HDMITX_AUDIO_CBIT       0x0
#define HDMITX_AUDIO_UBIT       0x1
#define HDMITX_AUDIO_VBIT       0x2
#define DOLBY_VSIF_LENGTH       0x18
#define DOLBY_LL_VSIF_LENGTH    0x1B
#define SCR_STAT_FRAME_CNT      12
#define HDR10PLUS_VSIF_LENGTH   0x1B

#define HDMITX_INC_TIMER_CMD_CNTR(max,cur) (((cur)==((max)+1))?((max)+1):((cur)+1))

typedef enum tagHDMITX_CMD_ID
{
    HDMITX_CMD_HPD = 0,
    HDMITX_CMD_HDCP_INTERRUPT,
    HDMITX_CMD_HDCP14_START,
    HDMITX_CMD_VERSION_RD,
    HDMITX_CMD_HDCP2X_START,
    HDMITX_CMD_HDCP2X_MONITOR,
    HDMITX_CMD_NOTIFY_ERR,
    HDMITX_CMD_FORCE_HPD,
    HDMITX_CMD_AUDIO_CONFIG,
    HDMITX_CMD_MAX
}HDMITX_CMD_ID;

typedef struct tagHDMITX_CMD
{
    HDMITX_CMD_ID   cmdID;
    int             param;
}HDMITX_CMD;

/*HDR transfer function*/
typedef enum {
       HDCP_VERSION_1_4 = 0 , /*HDCP version 1.4*/
          HDCP_VERSION_2_2 = 1   /*HDCP Version 2.2.*/
}HDCP_VERSION;

/*HDR transfer function*/
typedef enum
{
    VPP_TRANSFER_CHAR_DISABLE = 0,
    VPP_TRANSFER_CHAR_BT709 = 1,
    VPP_TRANSFER_CHAR_BT601 = 6,
    VPP_TRANSFER_CHAR_BT2020_1 = 14,
    VPP_TRANSFER_CHAR_BT2020_2 = 15,
    VPP_TRANSFER_CHAR_SMPTE_ST_2048  = 16,
    VPP_TRANSFER_CHAR_SMPTE_ST_428_I = 17,
    VPP_TRANSFER_CHAR_ARIB_B67 = 18
}HDR_TNFR_FUNC;

/*HDR EOTF */
typedef enum {
    HDR_EOTF_SDR_LUMI_RNG  = 0,
    HDR_EOTF_HDR_LUMI_RNG  = 1,
    HDR_EOTF_SMPTE_2048_LUMI_RNG = 2,
    HDR_EOTF_HLG = 3,
    HDR_EOTF_FUTURE = 4
}HDR_EOTF;

/*******************************************************************
 * FUNCTION: Notifies Hdmi Tx events to application
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdmiTxNotifyEvent (VPP_OBJ *vpp_obj, UINT32 event, UINT32 infoID);

/*******************************************************************
 * FUNCTION: HDMI Tx Task to handle HPD, HDCP interrupts
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdmiTxHdcpTask (void *arg);
int HdmiTxHpdTask(void *arg);

/*******************************************************************
 * FUNCTION: HDMI Tx hot plug detection polling timer
 * PARAMS:   *param - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
void HdmiTxHdcpTimerCb(void *param);

#ifdef __cplusplus
}
#endif

#endif // _VPP_BE_PRV_H_

