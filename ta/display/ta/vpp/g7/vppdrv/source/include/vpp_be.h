/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_BE_H_
#define _VPP_BE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpp_module.h"
#include "vpp_be_cec_srv.h"
#include "vpp_be_cec_srv_prv.h"
#if HDCP2X_ENABLED
#include "hdcp2x_api.h"
#endif

#define VPP_BE_SET_BIT(data,index) \
   ((data) |= (1 << (index)))
#define VPP_BE_GET_BIT(data,index) \
   (((data) >> (index))& 0x1)
#define VPP_BE_CLR_BIT(data,index) \
   ((data) &= ~(1 << (index)))

#define IS_4K_RES(resID, pResInfo) \
    ((resID >= 0 && resID < RES_RESET \
      && pResinfo->active_width >= 3840 && pResinfo->active_height >= 2160) ? 1 : 0)
#define IS_DSI_PANEL_RES(resID) ((resID >= RES_MIN_DSI) && (resID <= RES_MAX_DSI))
#define IS_VOUT_DISP_MODE(mode)   ((vpp_obj->m_VoutMode == VOUT_DISP_##mode))
#define IS_CPCB_VOUT_DISP_MODE(cpcb, mode) ((vpp_obj->m_VoutMode == VOUT_DISP_##mode) && (cpcb == CPCB_1))

#define IS_VOUT_DISP_MODE_DUAL()   ((vpp_obj->m_VoutMode == VOUT_DISP_DUAL_MODE_PIP)\
                                    || (vpp_obj->m_VoutMode == VOUT_DISP_DUAL_MODE_GFX0)\
                                    || (vpp_obj->m_VoutMode == VOUT_DISP_DUAL_MODE_GFX1))


#define VPP_BE_HDMI_HPD_INTR            0
#define VPP_BE_HDMI_HDCP_INTR           1
#define VPP_BE_HDMI_CEC_INTR            2

#define VPP_BE_HDMI_HDCP_FRMTRIG        0
#define VPP_BE_HDMI_HOST_PKT_FRMTRIG    1
#define VPP_BE_HDMI_SIG_FIFO_FRMTRIG    2
#define VPP_BE_HDMI_SIG_SETUNMUTE_FRMTRIG   3
#define VPP_BE_HDMI_SIG_SETMUTE_FRMTRIG 4

#define VPP_BE_CPCB0_INTR                3
#define HDR_TO_NONHDR_CHNAGEOVER_INTVL 300

#define HDMI_HDCP_KEEPOUT_START_VAL 0x1F8
#define VPP_BE_HDCP_STAT_CHECK_TIMER   100
#define VPP_BE_HDCP_RETRY_CHECK_TIMER  500
#define VPP_BE_HDCP_LINKINTEG_RETRY_TIMER  100

#define VPP_FEATURE_HDMITX            (1<<0)
#define VPP_FEATURE_MIPI_DSI          (1<<1)
#define MAX_NUM_FEATURE_CFG           1

/* Mux control for the selection of input data.*/
typedef enum VPP_VOP_SELECT_INPUT_T
{
    VPP_VOP_CPCB0_PROG = 0,
    VPP_VOP_CPCB0_INTER,
    VPP_VOP_CPCB1_CPCB0,
    VPP_VOP_CPCB1_GFX0,
    VPP_VOP_CPCB1_GFX1,
    VPP_VOP_CPCB1_GFX2,
    VPP_VOP_NO_INPUT,
    VPP_VOP_INP_MAX

}VPP_VOP_SELECT_INPUT,*PVPP_VOP_SELECT_INPUT;

typedef enum VPP_COLOR_PRIM_T
{
    VPP_COLOR_PRIM_BT709       = 1,
    VPP_COLOR_PRIM_UNSPECIFIED = 2,
    VPP_COLOR_PRIM_BT470M      = 4,
    VPP_COLOR_PRIM_BT470BG     = 5, //ITU-R BT601-6 625
    VPP_COLOR_PRIM_BT170M      = 6, //ITU-R BT601-6 525
    VPP_COLOR_PRIM_BT240M      = 7, //functionally identical to above
    VPP_COLOR_PRIM_FILM        = 8,
    VPP_COLOR_PRIM_BT2020      = 9,
    VPP_COLOR_PRIM_MAX,
}VPP_COLOR_PRIM;

typedef enum VPP_COLOR_STANDARD_T
{
    VPP_COLOR_STD_BT2020RGB  = 1,
    VPP_COLOR_STD_BT2020YCC  = 2,
    VPP_COLOR_STD_BT2020cYCC = 4,
}VPP_COLOR_STANDARD;

typedef enum VPP_STREAM_STATE_T
{
    VPP_SDR   = 0,
    VPP_HDR   = 1,
    VPP_DOLBY = 2,
    VPP_MAX   = 2,
}VPP_STREAM_STATE;

typedef enum VPP_STREAM_EOTF_T
{
    VPP_EOTF_SDR_GAMMA = 0,
    VPP_EOTF_HDR_GAMMA = 1,
    VPP_EOTF_HDR_SMPTE2084 = 2,
    VPP_EOTF_FUTURE = 3
}VPP_STREAM_EOTF;

typedef enum VPP_COLOR_RANGE_T
{
    VPP_COLOR_RANGE_NARROW = 0,
    VPP_COLOR_RANGE_FULL   = 1,
    VPP_COLOR_RANGE_SDI    = 2,
    VPP_COLOR_RANGE_MAX,
}VPP_COLOR_RANGE;

typedef enum VPP_HDMI_AV_PORT_T
{
    VPP_HDMI_AV_PORT_VIDEO  = 1,
    VPP_HDMI_AV_PORT_AUDIO  = 2,
    VPP_HDMI_AV_PORT_GCP    = 3,
    VPP_HDMI_AV_PORT_ALL    = 4,
}VPP_HDMI_AV_PORT;

typedef enum VPP_HDMI_SINK_CAP_T
{
    VPP_HDMI_SINK_NON_DOVI    = 0x00,
    VPP_HDMI_SINK_STD_DOVI    = 0x01,
    VPP_HDMI_SINK_STD_LL      = 0x02,
    VPP_HDMI_SINK_LL_DOVI     = 0x03,
    VPP_HDMI_SINK_DPCLR       = 0x10,
    VPP_HDMI_SINK_DPCLR_DOVI  = 0x11,
    VPP_HDMI_SINK_DPCLR_LL    = 0x12,
    VPP_HDMI_SINK_DPCLR_LL_DOVI = 0x13,
}VPP_HDMI_SINK_CAP;

typedef enum VPP_HDMI_IRQ_SOURCE_T
{
    VPP_HDMI_IRQ_UNDEFINED = 0,
    VPP_HDMI_IRQ_HPD,
    VPP_HDMI_IRQ_HDCP,
}VPP_HDMI_IRQ_SOURCE;

typedef enum VPP_DITHER_MODES_T
{
    VPP_BE_DITHER_MIN    =   0,
    VPP_BE_DITHER_BYPASS =   VPP_BE_DITHER_MIN,
    VPP_BE_DITHER_TRUNCATE,
    VPP_BE_DITHER_ROUNDING,
    VPP_BE_DITHER_1D_ERR_DIFFUSION,
    VPP_BE_DITHER_MAX
}VPP_DITHER_MODES;

typedef enum VPP_HDMI_VIC_CODES_T
{
    VPP_HDMI_4K50_VIC = 96,
    VPP_HDMI_4K5994_4K60_VIC
}VPP_HDMI_VIC_CODES;

/*************************************************/
/********************** BE APIs ******************/
/*************************************************/

/*******************************************************************
 * FUNCTION: Reset all VPP BE blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_Reset(VPP_OBJ *vpp_obj);

/*********************************************************************
 * FUNCTION: initialize all VPP BE blocks according to configuration
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ********************************************************************/
int VPP_BE_Config(VPP_OBJ *vpp_obj);

/*******************************************************************
 * FUNCTION: select input for VOUTs in VPP BE blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           voutID - Vout
 *           cpcbout - CPCB output ID
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetVoutInput(VPP_OBJ *vpp_obj, int voutID, int cpcbout);

/*******************************************************************
 * FUNCTION: select input for VOUTs in VPP BE blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           voutID - Vout
 *           color_fmt - CPCB output color format
 *           bit_depth - CPCB output bit depth
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetVoutFormat(VPP_OBJ *vpp_obj, int voutID, int color_fmt, int bit_depth);

/*******************************************************************
 * FUNCTION: set HDMI resolution
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           resID - resolution ID
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdmiResolution(VPP_OBJ *vpp_obj, int resID);

/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT: *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SetHdmiVideoFmt(VPP_OBJ *vpp_obj);

/*******************************************************************
 * FUNCTION: set HDMI audio format
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           sampFreq - Sampling frequency
 *           sampSize - Word Size
 *           mClkFactor - 128/256/384/512 times frequency
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdmiAudioFmt(VPP_OBJ *vpp_obj, int enable);

/********************************************************************************
 * FUNCTION: Register for HDCP event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdcpRegisterCB(VPP_OBJ *vpp_obj, UINT32 eventCode,
                           void *cbFnAddr, void *contextParam);

/********************************************************************************
 * FUNCTION: Unregister for HDCP event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdcpUnregisterCB(VPP_OBJ *vpp_obj);

/*******************************************************************
 * FUNCTION: Configure Audio VUC data
 * PARAMS:   *vpp_obj       - pointer to VPP object
 *           pVUCCfg        - pointer to VUC config structure
 *           loadOnAudioCfg - 1 for loading these data when audio
 *                          - is configured subsequently
 *                          - 0 for loading it on the next VBI
 *           pVUCUpdateReqd - pointer to indicate if update is
 *                          - required on the next VBI interrupt
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_ConfigHdmiAudioVUCBits(VPP_OBJ *vpp_obj,
                                  VPP_HDMI_AUDIO_VUC_CFG *pVUCCfg,
                                  int loadOnAudioCfg, int *pVUCUpdateReqd);

/*******************************************************************
 * FUNCTION: Set Audio VUC data
 * PARAMS:   *vpp_obj         - pointer to VPP object
 *           vucUpdateSetting - audio VUC bit update setting
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdmiAudioVUCBits(VPP_OBJ *vpp_obj, int vucUpdateSetting);

/*******************************************************************
 * FUNCTION: mute Hdmi video/audio
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           video - video/audio to be muted
 *           mute - 1: mute 0:un-mute
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdmiMute (VPP_OBJ *vpp_obj,  VPP_HDMI_AV_PORT port, int muteEn);

/********************************************************************************
 * FUNCTION: Set Hdmi output mode (Hdmi/Dvi)
 * INPUT: hdmiMode - 0:DVI, 1:HDMI
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SetHdmiMode(VPP_OBJ *vpp_obj, int hdmiMode);

/********************************************************************************
 * FUNCTION: Configure Hdmi Video format
 * INPUT: color_fmt - color format (RGB, YCbCr 444, 422)
 *      : bit_depth - 8/10/12 bit color
 *      : pixel_rept - 1/2/4 repetitions of pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_ConfigHdmiVideoFmt(VPP_OBJ *vpp_obj, int color_fmt,
                              int bit_depth, int pixel_rept);

/*******************************************************************
 * FUNCTION: set HDMI audio format
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           sampFreq - Sampling frequency
 *           sampSize - Word Size
 *           mClkFactor - 128/256/384/512 times frequency
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_ConfigHdmiAudioFmt(VPP_OBJ *vpp_obj, int enable,
                              VPP_HDMI_AUDIO_CFG *pAudioCfg);

/********************************************************************************
 * FUNCTION: Set HDMI Video Information
 * INPUT: aspect ratio
 *      : scan info
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SetHdmiVideoInfo(VPP_OBJ *vpp_obj, int aspRatio, int scanInfo);

/********************************************************************************
 * FUNCTION: Set HDMI Audio Information
 * INPUT: Speaker Mapping
 *      : Level Shift Value
 *      : Down Mix Inhibit Flag
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SetHdmiAudioInfo(VPP_OBJ *vpp_obj, int spkrAlloc,
                            int lvlShiftVal, int downMixInhFlag);

/********************************************************************************
 * FUNCTION: Register for HDMI event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiRegisterCB(VPP_OBJ *vpp_obj, UINT32 eventCode,
                          MV_VPP_EVENT_CALLBACK cbFnAddr, void* contextParam);

/********************************************************************************
 * FUNCTION: Unregister for HDMI event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiUnregisterCB(VPP_OBJ *vpp_obj);

/********************************************************************************
 * FUNCTION: Format HDMI Auxillary Packet
 * INPUT: enable    - To handle packet enable/disable
 *      : pktType   - Packet type
 *      : pAuxPkt   - Pointer to aux packet data (Input)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiFormatAuxPkt(VPP_OBJ *vpp_obj, BOOL enable, VPP_HDMI_PKT_ID pktType,
                            VPP_HDMI_PKT *pAuxPkt);

/********************************************************************************
 * FUNCTION: Send HDMI Auxillary Packet
 * INPUT: pktType    - Packet type
 *      : repeatPkt  - Transmit packet once/repeat every VSync
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiSendAuxPkt(VPP_OBJ *vpp_obj, VPP_HDMI_PKT_ID pktType,
                          BOOL repeatPkt);

/********************************************************************************
 * FUNCTION: Stop HDMI Auxillary Packet
 * INPUT: pktType   - Packet type
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiStopAuxPkt(VPP_OBJ *vpp_obj, VPP_HDMI_PKT_ID pktType);

/********************************************************************************
 * FUNCTION: Configure SRM data buffer
 * INPUT: pSrmBuf - Pointer to SRM data buffer
 *      : pSignValid - Pointer to return if the signature is valid on the given
 *      :            - SRM data (Output)
 *      : pVernNum - Pointer to return version number of SRM data (output)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_ConfigureSRM(VPP_OBJ *vpp_obj, VPP_HDCP_SRM_DATA *pSrmData,
                        VPP_DEV_SIGN_DATA *pDevSignData, VPP_HDCP_SRM_INFO *pSrmInfo);

/********************************************************************************
 * FUNCTION: Get SRM Version
 * INPUT: pHdcp22SrmVersion - HDCP 2.2 SRM File version
 *      : pHdcp14SrmVersion - HDCP 1.4 SRM File version
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_GetSRMVersion(VPP_OBJ *vpp_obj, UINT16 *pHdcp22SrmVersion, UINT16 *pHdcp14SrmVersion);

/********************************************************************************
 * FUNCTION: Enable/Disable SRM Check
 * INPUT   : enable - Enable/Disable flag for SRM check control
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_EnableSRMCheck(VPP_OBJ *vpp_obj, int enable);

/*******************************************************************
 * FUNCTION: Enable Encryption on HDMI
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           Enable   - enable/disable encryption
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_EnableHdcp(VPP_OBJ *vpp_obj, BOOL Enable);

/*******************************************************************
 * FUNCTION: Get HDCP State on HDMI
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pHDCPState   - Pointer return state
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_GetHdcpState(VPP_OBJ *vpp_obj, int *pHDCPState);

/*******************************************************************
 * FUNCTION: Returns HPD status
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pHpdStatus - Pointer returned status
 * RETURN: MV_VPP_OK
 *         MV_EBADPARAM - invalid parameters
 ******************************************************************/
int VPP_BE_GetHPDStatus (VPP_OBJ *vpp_obj, int *pHpdStatus);

/*******************************************************************
 * FUNCTION: Returns Raw Edid of Hdmi sink connected
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pRawEdid - Pointer to Raw Edid data
 *                        to return hdmi raw edid info
 * RETURN: MV_VPP_OK
 *         MV_EBADPARAM - invalid parameters
 ******************************************************************/
int VPP_BE_GetRawEdid (VPP_OBJ *vpp_obj, VPP_HDMI_RAW_EDID *pRawEdid);

/*******************************************************************
 * FUNCTION: Notifies Hdmi Tx events to application
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdcpNotifyEvent (VPP_OBJ *vpp_obj, UINT32 event, UINT32 infoID);

/*******************************************************************
 * FUNCTION: Returns capabilities of Hdmi sink connected
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pSinkCaps - Pointer to sink capabilities structure
 *                        to return hdmi sink info
 * RETURN: MV_VPP_OK
 *         MV_EBADPARAM - invalid parameters
 ******************************************************************/
int VPP_BE_GetSinkCaps (VPP_OBJ *vpp_obj, VPP_HDMI_SINK_CAPS *pSinkCaps);

/*******************************************************************
 * FUNCTION: ISR Service to service HDCP interrupts
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
void VPP_BE_HDCP_ISR_service(VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION : Deferred service routine for HPD/HDCP interrupt to signal the
 *          : waiting task on the interrupt
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
void VPP_BE_HDMI_DSR_service(VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION : CPCB frame interrupt for HDMI to be used as a counter for
 *          : starting HDCP
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
void VPP_BE_HDMIProcessFrameInt(VPP_OBJ *vpp_obj);

/*******************************************************************
 * FUNCTION: ISR Service to service HPD interrupt
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
void VPP_BE_HPD_ISR_service(VPP_OBJ *vpp_obj);
void VPP_BE_ProcessEarcHPD(VPP_OBJ *vpp_obj, int vhpd);

int VPP_BE_EnableHdmiAudioFmt(VPP_OBJ *vpp_obj, int enable);
int VPP_BE_HdmiExt_Create(VPP_OBJ *vpp_obj);
int VPP_BE_HdmiExt_Destroy(VPP_OBJ *vpp_obj);
int HdmiTxMapVICTo4KResID(UINT32 videoIDCode, UINT32 *p4KVideoRes );
int VPP_BE_GetHdcpVersion(VPP_OBJ *vpp_obj, int *pHDCPVersion);
int VPP_BE_SetHdcpVersion(VPP_OBJ *vpp_obj, int iHDCPVersion);
int VPP_BE_GetCurrentHdcpVersion(VPP_OBJ *vpp_obj,VPP_HDMI_HDCP_VERSION *pHDCPVersion);
int VPP_BE_GetSinkHdcpcapability(VPP_OBJ *vpp_obj,int *pHDCPVersion);
/*******************************************************************
 * FUNCTION: Construct HDR info frame packet
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdmiTxConfigHDRInfoFrame (VPP_OBJ *vpp_obj);

/*******************************************************************
 * FUNCTION: Function to Force the HDMi Hot Plug Event
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int VPP_BE_HdmiTxForceHotplug(VPP_OBJ *vpp_obj);

/******************************************************************************
* FUNCTION : Initiates transmission of given message on CEC line
* PARAMS   : *vpp_obj  - Pointer to VPP object
*          : pCecMsg   - Pointer to CEC message (Input)
* RETURN   : MV_VPP_OK, on success
*          : Error code, otherwise
*****************************************************************************/
int VPP_BE_CEC_SendMessage (VPP_OBJ *vpp_obj, PVPP_CEC_MSG pCecMsg);
//int VPP_BE_CEC_Reset_SemSts(VPP_OBJ *vpp_obj);

/********************************************************************************
 * function: config and send aviinofframe
 * input: vpp_obj - pointer to vpp object
 * return: mv_vpp_ok - succeed
 *         mv_ebadparam - invalid parameters
 ********************************************************************************/
int VPP_BE_SendHdmiAVIInfoFrame(VPP_OBJ *vpp_obj);

/*******************************************************************
 * FUNCTION: Updates content stream info received from SA
 * PARAMS:   *vpp_obj - pointer to VPP object
 * PARAMS:   vpp_sa_stream_type - stream type from SA
 * RETURN:   MV_VPP_OK
 ******************************************************************/
#if HDCP2X_ENABLED
int VPP_BE_UpdateContentStrmInfo(VPP_OBJ *vpp_obj, HDCP2X_STEAM_TYPE vpp_sa_stream_type);
#endif

/*******************************************************************
* FUNCTION: Get HDMI Phy power State
* PARAMS:   *vpp_obj - pointer to VPP object
*           *pPowerState   - Pointer return state
* RETURN: MV_VPP_OK
******************************************************************/
int VPP_BE_GetHdmiPowerStatus(VPP_OBJ *vpp_obj, int * pPowerState);

/*******************************************************************
* FUNCTION: Get if the output mode is HDMI
* PARAMS:   *pinfo - pointer to VBUF_INFO
*           *vpp_obj   - pointer to VPP object
* RETURN: MV_VPP_OK
******************************************************************/
bool VPP_BE_IsHDROutput(VPP_OBJ *vpp_obj);

/********************************************************************************
 * FUNCTION: Set pixel clock
 * INPUT: *vpp_obj - pointer to VPP object
 *         resID - resolution ID
 *         color_fmt - output color format
 *         PixelRpt - pixel repeat
 * RETURN: MV_VPP_OK - SUCCEED
 ********************************************************************************/
int VPP_BE_SetPixelClock(VPP_OBJ *vpp_obj, int resID, int color_fmt, int PixelRpt);

int VPP_BE_SetHdmiPhyPower(VPP_OBJ *vpp_obj, BOOL state);

int VPP_BE_HDMI_EnableOutputFixColor(VPP_OBJ *vpp_obj, int enable);

int VPP_BE_HDMI_ClearSCDCReservedBit(VPP_OBJ *vpp_obj);

int VPP_BE_HDMI_ScramblingEnable(VPP_OBJ *vpp_obj, BOOL enable);

int VPP_BE_GetHdmiIrqSource(VPP_OBJ *vpp_obj);

int VPP_BE_MIPIOBJ_Create(VPP_OBJ *vpp_obj);

int VPP_BE_MIPIOBJ_Config(VPP_OBJ *vpp_obj);

int VPP_BE_MIPI_PanelInit(VPP_OBJ *vpp_obj, int PanelID);

int VPP_BE_MIPI_PanelDeInit(VPP_OBJ *vpp_obj);

int VPP_BE_MIPI_PanelStandby(VPP_OBJ *vpp_obj, int PanelID);

BOOL VPP_BE_GetHdmiPhyExpectedState(VPP_OBJ *vpp_obj);

int VPP_BE_ConfigHdmiPhyStateChange(VPP_OBJ *vpp_obj, BOOL enable);

int VPP_BE_SetHdmiPhy(VPP_OBJ *vpp_obj, unsigned int * phySetting,
                            unsigned int PhyBufSize);

int VPP_BE_SetHDMIConfig(VPP_OBJ *vpp_obj, HDMI_CONFIG *config,
                            unsigned int bufSize);

void VPP_BE_ProcessHPDStateChange(VPP_OBJ *vpp_obj);

int VPP_BE_HdmiPhyRead(VPP_OBJ *vpp_obj, int offset, int *value);

int VPP_BE_HdmiPhyWrite(VPP_OBJ *vpp_obj, int offset, int value);

int VPP_BE_SetHdmiPhyTestMode(VPP_OBJ *vpp_obj, int mode);

int VPP_BE_SetGammaCorrection(VPP_OBJ *vpp_obj, int Channel, int GmMode);

void ProcessBootupHPDState(VPP_OBJ *vpp_obj);

int VPP_BE_SetHdmiPhy(VPP_OBJ *vpp_obj, unsigned int * phySetting, unsigned int PhyBufSize);

int VPP_BE_PrintGamma(VPP_OBJ *vpp_obj, int chanID, INT location);

int VPP_BE_SetGamma(VPP_OBJ *vpp_obj, int chanID,INT gmMode);

INT VPP_BE_SetGammaEnable(VPP_OBJ *vpp_obj, INT Channel, INT Enable);

int VPP_BE_GetHdmiIrqSource(VPP_OBJ *vpp_obj);

int VPP_BE_Resume(VPP_OBJ *vpp_obj);

int VPP_BE_Suspend(VPP_OBJ *vpp_obj);

void VPP_BE_MIPI_Config(VPP_OBJ *vpp_obj, VPP_MIPI_CONFIG_PARAMS *pConfigParams);
int VPP_BE_SetEDIDBlackList(VPP_OBJ *vpp_obj, UINT8 *sinkList,
        unsigned int bufSize);

int VPP_BE_ConfigMIPIResinfo(int handle, int resID);

void VPP_BE_MIPI_DsiInit(VPP_OBJ *vpp_obj);

int VPP_BE_GetHdcpKsv(VPP_OBJ *vpp_obj, VPP_HDCP_KSV_DATA *pHDCPKsv);

int VPP_BE_SetHdcpRepeaterMode(VPP_OBJ *vpp_obj, UINT32 Mode);

int VPP_BE_SetRptrDsHdcpVersion(VPP_OBJ *vpp_obj, int iHDCPVersion);

void VPP_BE_IsSpecialSink(VPP_OBJ *vpp_obj, int *resIDUpdate, int *bitDepthUpdate, int *colorFmtUpdate);

int VPP_BE_GetSinkFeatureMap (VPP_OBJ *vpp_obj, VPP_HDMI_SINKCAP_BITMASK *pSinkFeatureMap);

void VPP_BE_HdmiPowerDown(VPP_OBJ *vpp_obj);

int VPP_BE_MIPI_PanelSendCmd(VPP_OBJ *vpp_obj, VPP_MIPI_CMD_TYPE enCmdType);
int VPP_BE_MIPI_PanelSendCmdBuf(VPP_OBJ *vpp_obj, UINT8 *pcmd, UINT16 bufsize);

#ifdef __cplusplus
}
#endif

#endif

