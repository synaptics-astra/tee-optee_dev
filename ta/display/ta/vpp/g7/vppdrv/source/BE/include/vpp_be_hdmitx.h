/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPP_BE_HDMITX_H__
#define __VPP_BE_HDMITX_H__

#define NEW_HDMI_TX_IP

#include "com_type.h"
#include "hdmi_audio.h"
#include "vpp_api.h"
#include "SNPS_HDMI_tx.h"
#if HDCP2X_ENABLED
#include "hdcp2x_api.h"
#endif
/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */
#define VPP_BE_HDMITX_MAX_PKT_INDEX     6
#define HDCP_PORT_RXSTATUS              0x70
#define HDCP_PORT_READ_MSG              0x80
#define EDDC_READ_LIMIT                 0x8

#define HDCP1X_FALLBACK_THRES              3
#define HDCP_LINKERR_RST_THRES             3
#define HDCP_LINKERR_RESET_ENABLED         0

#define HDCP14_INTR_MASK_AUTH_PASS         0x80
#define HDCP14_INTR_MASK_AUTH_FAIL         0x40
#define HDCP14_INTR_MASK_I2C_FAIL          0x18
#define HDCP14_INTR_MASK_AUTH_KSV_FAIL     0x02

/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */

typedef enum tagDDC_SLAVES {
    EDDC_SLAVE_EDID,
    EDDC_SLAVE_HDCP,
    EDDC_SLAVE_SCDC,
} DDC_SLAVES;

typedef struct _HDMI_SINK_BLACKLIST {
    BOOL  bl_option_present;
    /* fix for TVs sensitive to pll jitter on 50.94 */
    UINT8 count_59_94_fix; //number of sinks identified
    //list holding the product id of effected sinks from xml
    UINT8 prod_id_59_94_fix[HDMI_MAX_SINK_COUNT][HDMI_SINK_ID_SIZE];
    UINT8 count_59_94_dyn; //number of sinks added dynamically
    //list holding the product id of effected sinks dynamically added
    UINT8 prod_id_59_94_dyn[HDMI_MAX_SINK_COUNT][HDMI_SINK_ID_SIZE];
    UINT8 fix_59_94_active;

}HDMI_SINK_BLACKLIST;

typedef enum tagVPP_BE_HDMITX_OUT_MODE
{
    VPP_BE_HDMITX_OUT_UNDEF = 0x00,
    VPP_BE_HDMITX_OUT_DVI,
    VPP_BE_HDMITX_OUT_HDMI,
}VPP_BE_HDMITX_OUT_MODE;

typedef enum tagVPP_BE_HDMITX_AUDIO_BUS
{
    VPP_BE_HDMITX_AUDIO_I2S,
    VPP_BE_HDMITX_AUDIO_SPDIF,
    VPP_BE_HDMITX_AUDIO_BUS_MAX
}VPP_BE_HDMITX_AUDIO_BUS, *PVPP_BE_HDMITX_AUDIO_BUS;

// Audio Input clock sampling freq types
typedef enum tagVPP_BE_HDMITX_AUDIO_INP_CLK_SEL
{
    VPP_BE_HDMITX_AUD_INP_CLK_128fs = 0,
    VPP_BE_HDMITX_AUD_INP_CLK_256fs,
    VPP_BE_HDMITX_AUD_INP_CLK_384fs,
    VPP_BE_HDMITX_AUD_INP_CLK_512fs,
    VPP_BE_HDMITX_AUD_INP_CLK_64fs,
    VPP_BE_HDMITX_AUD_INP_CLK_MAX
}VPP_BE_HDMITX_AUDIO_INP_CLK_SEL;

// Color Depth
typedef enum tagVPP_BE_HDMITX_BIT_DEPTH
{
    VPP_BE_HDMITX_BIT_DEPTH_8 = 4,
    VPP_BE_HDMITX_BIT_DEPTH_10,
    VPP_BE_HDMITX_BIT_DEPTH_12,
}VPP_BE_HDMITX_BIT_DEPTH;

// Pixel repetition types
typedef enum tagVPP_BE_HDMITX_PIXEL_RPT_TYPE
{
    VPP_BE_HDMITX_PIXEL_RPT_NONE = 0,
    VPP_BE_HDMITX_PIXEL_RPT_BY_2 = 1,
    VPP_BE_HDMITX_PIXEL_RPT_BY_3,
    VPP_BE_HDMITX_PIXEL_RPT_BY_4,
    VPP_BE_HDMITX_PIXEL_RPT_BY_5,
    VPP_BE_HDMITX_PIXEL_RPT_BY_6,
    VPP_BE_HDMITX_PIXEL_RPT_BY_7,
    VPP_BE_HDMITX_PIXEL_RPT_BY_8,
    VPP_BE_HDMITX_PIXEL_RPT_BY_9,
    VPP_BE_HDMITX_PIXEL_RPT_BY_10,
    VPP_BE_HDMITX_PIXEL_RPT_MAX
}VPP_BE_HDMITX_PIXEL_RPT_TYPE;

// HDMI Packet Types
typedef enum tagVPP_BE_HDMITX_HOST_PKT_TYPE
{
    VPP_BE_HDMITX_PKT_NONE  = 0x00,
    VPP_BE_HDMITX_NULL_PKT,
    VPP_BE_HDMITX_ACR_PKT,
    VPP_BE_HDMITX_AUD_SAMP_PKT,
    VPP_BE_HDMITX_GC_PKT,
    VPP_BE_HDMITX_ACP_PKT,
    VPP_BE_HDMITX_ISRC1_PKT,
    VPP_BE_HDMITX_ISRC2_PKT,
    VPP_BE_HDMITX_1BIT_AUD_SAMP_PKT,
    VPP_BE_HDMITX_DST_AUD_PKT,
    VPP_BE_HDMITX_HBR_AUD_PKT,
    VPP_BE_HDMITX_GAMUT_METADATA_PKT,
    VPP_BE_HDMITX_VENDOR_INFOFRM_PKT = 0x81,
    VPP_BE_HDMITX_AVI_INFOFRM_PKT,
    VPP_BE_HDMITX_SPD_INFOFRM_PKT,
    VPP_BE_HDMITX_AUD_INFOFRM_PKT,
    VPP_BE_HDMITX_MPEG_INFOFRM_PKT,
    VPP_BE_HDMITX_MAX_PKT,
}VPP_BE_HDMITX_HOST_PKT_TYPE, *PVPP_BE_HDMITX_HOST_PKT_TYPE;

// Host packet transfer mode
typedef enum tagVPP_BE_HDMITX_HOST_PKT_TX_MODE
{
    VPP_BE_HDMITX_HOST_PKT_TX_ONCE = 0,
    VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC,
    VPP_BE_HDMITX_HOST_PKT_TX_MAX_MODE
}VPP_BE_HDMITX_HOST_PKT_TX_MODE,*PVPP_BE_HDMITX_HOST_PKT_TX_MODE;

typedef union tagVPP_BE_HDMITX_AUDIO_BUS_PARAMS
{
    struct
    {
        BOOL        ClkDelay;
        BOOL        DataShift;
        BOOL        DataJustification;
        BOOL        LeftChnWordSelEdge;
        BOOL        DataLatchingEdge;
        UINT8       PortCfg; // Flag
    }I2SParams;
}VPP_BE_HDMITX_AUDIO_BUS_PARAMS, *PVPP_BE_HDMITX_AUDIO_BUS_PARAMS;

typedef struct tagVPP_BE_HDMITX_AUDIO_FMT_PARAMS
{
    BOOL                                Layout;
    UINT8                               WordLength;
    VPP_BE_HDMITX_AUDIO_INP_CLK_SEL     InputClk;
    UINT32                              AcrN;
    UINT32                              AcrCts;
    UINT8                               HbrAudio;
    UINT8                               AudioFmt;
}VPP_BE_HDMITX_AUDIO_FMT_PARAMS, *PVPP_BE_HDMITX_AUDIO_FMT_PARAMS;

typedef struct tagVPP_BE_HDMITX_VIDEO_FMT_PARAMS
{
    // RGB/YCbCr
    INT  VideoFmt;
    // Display Resolution
    INT  DispRes;
    // Color Depth
    INT ColorDepth;
    // Pixel Repetition
    VPP_BE_HDMITX_PIXEL_RPT_TYPE PixelRpt;
}VPP_BE_HDMITX_VIDEO_FMT_PARAMS, *PVPP_BE_HDMITX_VIDEO_FMT_PARAMS;

typedef struct tagVPP_BE_HDMITX_AVI_UPDATE_PARAMS
{
    CEA_COL_SPACE_FMT       ColorSpace;
    CEA_PICT_ASP_RATIO      AspectRatio;
    CEA_CALORIMETRY_INFO    Calorimetry;
    CEA_EXT_CAL_INFO        ExtCalorimetry;
    CEA_RGB_QUANT_RANGE     RGBQuantRange;
    CEA_YCC_QUANT_RANGE     YCCQuantRange;
}VPP_BE_HDMITX_AVI_UPDATE_PARAMS, *PVPP_BE_HDMITX_AVI_UPDATE_PARAMS;

typedef struct tagVPP_BE_HDMITX_AUDIO_CHN_STS
{
    UINT8       CpBit;
    UINT8       PreEmphasisInfo;
    UINT8       CategoryCode;
    UINT8       SrcNum;
    UINT8       ChnNum;
    UINT8       SampFreq;
    UINT8       ClkAccuracy;
    UINT8       MaxAudSampLen;
    UINT8       WordLength; //Based on MaxAudSampLen
    UINT8       OrigSampFreq;
}VPP_BE_HDMITX_AUDIO_CHN_STS, *PVPP_BE_HDMITX_AUDIO_CHN_STS;

//TMDS frequency range used for PHY DAMP setting
typedef enum tagHDMITX_FREQ_RANGE
{
    FREQ_UPTO_38 = 0,
    FREQ_39_TO_74P5,
    FREQ_75_TO_148,
    FREQ_149_TO_222,
    FREQ_223_TO_297,
    FREQ_298_TO_594,
    MAX_FREQ_RANGE
}HDMITX_FREQ_RANGE;

typedef enum tagVPP_BE_HDMITX_IRQ_SOURCE {
    IRQ_HPD = 0,
    IRQ_RXSENSE,
    IRQ_SINKDET,           /* enable hpd and rxsense */
    IRQ_HDCP14,
    IRQ_HDCP22,
}VPP_BE_HDMITX_IRQ_SOURCE;

typedef enum tagVPP_BE_HDCP14_INTR_STAT {
    HDCP14_AUTH_FAIL,
    HDCP14_AUTH_KSV_FAIL,
    HDCP14_AUTH_COMM_ERR,
    HDCP14_AUTH_PASS,
    HDCP14_AUTH_INTERNAL,
    HDCP14_AUTH_UNDEFINED = 99,
}VPP_BE_HDCP14_INTR_STAT;

typedef struct tagHDMITX_HDCP_ERR_EVENT
{
    unsigned short gTxRxErrFlg : 1;
    unsigned short gDevNotRdyFlg : 1;
    unsigned short gRdBksvErrFlg : 1;
    unsigned short gRiErrFlg : 1;
    unsigned short gR0ErrFlg : 1;
    unsigned short gAuthSuccess : 1;
    unsigned short gViErrFlg : 1;
    unsigned short gTopErrFlg : 1;
    unsigned short gDelayErrFlg : 1;
    unsigned short gDsRepFlg : 1;
}HDMITX_HDCP_ERR_EVENT;

typedef enum tagHDMITX_HDCP_RETURN_STATE
{
    HDCP_SET_IDLE_STATE,
    HDCP14_AUTH_RETRY,
    HDCP14_AUTH_INPROGESS,
    HDCP14_AUTH_DONE,
    HDCP22_AUTH_RETRY,
    HDCP22_AUTH_LINK_INTEG,
    HDCP22_AUTH_INPROGESS,
    HDCP22_AUTH_DONE,
}HDMITX_HDCP_RETURN_STATE;

#define MAX_HDMITX_HDCP_SRM_TYPE 2
typedef enum tagHDMITX_HDCP_SRM_TYPE
{
    HDMITX_HDCP_SRM_TYPE_1X,
    HDMITX_HDCP_SRM_TYPE_2X,
}HDMITX_HDCP_SRM_TYPE;

typedef struct tagHDMITX_HDCP_SRM_DATA
{
    BOOL    integrityChkFailed;
    UINT8   valid;
    UINT8   id;
    UINT16  version;
    UINT16  generation;
    UINT32  numOfDevices;
    UINT8*  revocList;
}HDMITX_HDCP_SRM_DATA;

typedef enum tagHDMITX_HDCP_SM_STATE
{
    HDCP_SM_STATE_IDLE,
    HDCP_SM_STATE_TRIGGERED,
    HDCP_SM_STATE_REAUTH,
    HDCP_SM_STATE_AUTHENTICATED,
}HDMITX_HDCP_SM_STATE;

#ifdef EARC_RX_ENABLED
typedef struct tagHDMITX_EARC_CONTEXT
{
    int                     initialized;
    int                     enabled;
    EARC_MODE               userMode;
    void                    *drv_obj;
    MV_HANDLE_Timer_t       timer;
    UINT32                  eventCode;
    void                    *contextParam;
    EARC_EVT_INFO           recentAudioCfgEvt;
    MV_VPP_EVENT_CALLBACK   cb_func;
}HDMITX_EARC_CONTEXT;
#endif

typedef struct BE_HDMITX_DATA_T {
#ifndef CONFIG_AMP_TEE_ENABLE
    MV_OSAL_HANDLE_TASK_t   hpdtskHandle;
    MV_OSAL_HANDLE_TASK_t   hdcpTskHandle;
#endif

    // Handles
    MV_HANDLE_Timer_t       hpdTimer;
    HDMI_FIFO               hdcpCmdFifo;
    HDMI_FIFO               hpdcmdFifo;

    // Timer config
    UINT32                  timerCmdCfg;
    UINT32                  timerCmdCntr;

    // EDID Info
    VPP_HDMI_SINK_CAPS      edidInfo;

    // Video Settings
    VPP_BE_HDMITX_VIDEO_FMT_PARAMS videoParams;
    VPP_BE_HDMITX_AVI_UPDATE_PARAMS aviParams;

    // Audio Settings
    VPP_BE_HDMITX_AUDIO_FMT_PARAMS  audFmtParams;
    VPP_HDMI_AUDIO_VUC_CFG          audVUCCfg;
    CEA_AUDIO_SAMP_FREQ             audSampFreq;
    CEA_AUDIO_CHN_CNT               audNumChannels;

    // Authentication State
    BOOL                    authRequired;

    // Rx Connected State
    volatile BOOL           rxConnected;

    // Current output mode
    BOOL                    curOutHdmiMode;

    // Application callback settings
    MV_VPP_EVENT_CALLBACK   appCbFn;
    UINT32                  eventCode;
    void                    *contextParam;
    MV_VPP_EVENT_CALLBACK   appCbFn2;
    UINT32                  eventCode2;
    void                    *contextParam2;

    int                     frameCntr;
    UINT32                  phyclkratio;

    int                     requestedHdcpVersion;
    int                     SessionMaxSuprtedhdcpVersion;
    int                     SourceMaxSuprtedhdcpVersion;
    int                     SinkMaxSuprtedhdcpVersion;

    HDMITX_HDCP_ERR_EVENT   hdcpErrEvent;
    BOOL                    enableSrmCheck;
    HDMITX_HDCP_SRM_DATA    hdcpSrm[MAX_HDMITX_HDCP_SRM_TYPE];


#ifdef CONFIG_VPP_DEV_KEY_SUPPORT
    uintptr_t               devKeyHandle;
    // This will be changed after osal is updated to return shmHandle
    UINT8*                  pDevKeyAddr;
    BOOL                    devKeyValid;
#endif

    // I2S Bus Params
    VPP_BE_HDMITX_AUDIO_BUS_PARAMS I2SBusParams;

    BOOL                    phyStateFromApp;
    SNPS_HDMI_TX            *vendorHdmiTx;
    TEE_TASessionHandle     hdcp2xSessionHandle;

    HDMITX_HDCP_SM_STATE       sm_state;
    uint8_t         hdcpDDCErrCnt;
    uint8_t         hdcp14ErrCnt;
    uint8_t         hdcp2xErrCnt;
#if HDCP2X_ENABLED
    HDCP2X_DOWNSTREAM_TOPOLOGY hdcp2xRptrTopology;
#endif

#ifdef EARC_RX_ENABLED
    HDMITX_EARC_CONTEXT        earcContext;
#endif

    HDMI_CONFIG                hdmiConfig;
    HDMI_SINK_BLACKLIST        sinkSpecificConfig;
    int             hdcp2xRetryCount;
#if HDCP_LINKERR_RESET_ENABLED
    int             linkErrRstCount;
#endif
    UINT8           hdmiEarlySignalCount;
    UINT8           hdmi_outformat;
    int             hdcp2xDSEarlyAuthStarted;
    VPP_HDMI_HDCP_MODE activeHdcpMode;
    BOOL            planeControlOnHdcpFail;
    BOOL            encBypassOnHdcpFail;
    INT             hdmiLockCheck;
    int             resumeInProgress;
    UINT8           EdidCurrentchecksum[8];
    int             Extblks;
    BOOL            forceHPDEnabled;
    UINT8           *pEdidArray;
    BOOL            tmds_reset;
#ifndef HDMI_SRV_SUPPORT
    UINT32          sinkConnChangeCount;
    UINT32          polledSinkConnChangeCount;
#endif
    BOOL            hdmiHpdControlOff;
} BE_HDMITX_DATA, *PBE_HDMITX_DATA;


/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */

INT BE_HDMITX_EnableAudioFmt(BE_HDMITX_DATA *pHdmiTx, BOOL EnAudio);
/*-----------------------------------------------------------------------------
 * Description  : This function enables audio packet transfer
 * Arguments    : EnAudio - Flag to enable/disable audio packet transmission
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_EnableAudioPkt(BE_HDMITX_DATA *pHdmiTx, BOOL EnAudio);

/*-----------------------------------------------------------------------------
 * Description  : This function sets audio format parameters
 * Arguments    : pAudioFmtParams - Pointer to audio format parameters (Input)
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioFmtParams(BE_HDMITX_DATA *pHdmiTx, VPP_BE_HDMITX_AUDIO_BUS AudioBus,
                                PVPP_BE_HDMITX_AUDIO_FMT_PARAMS pAudioFmtParams);

/*-----------------------------------------------------------------------------
 * Description  : This function sets/clears the audio fifo reset
 * Arguments    : bSet - TRUE/FALSE to set/clear the audio fifo reset
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioFifoReset(BE_HDMITX_DATA *pHdmiTx, INT bSet);

/*-----------------------------------------------------------------------------
 * Description  : This function sets audio validity bit
 * Arguments    : validityBit - Audio validity bit to be loaded
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioValidityBit(BE_HDMITX_DATA *pHdmiTx, INT vBit);

/*-----------------------------------------------------------------------------
 * Description  : This function sets audio channel status bits
 * Arguments    : pChSts - Pointer to audio chnanel status bits (Input)
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioChStsBits(BE_HDMITX_DATA *pHdmiTx, UINT8 *pChSts);

/*-----------------------------------------------------------------------------
 * Description  : This function sets/clears audio user data bits
 * Arguments    : pUserData - Pointer to user data bits (Input)
 *              : If NULL, user data bits are cleared
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioUserBits(BE_HDMITX_DATA *pHdmiTx, UINT8 *pUserData);

/*-----------------------------------------------------------------------------
 * Description  : This function starts/stops loading of VUC data
 * Arguments    : startLoad - TRUE, to start loading of data and FALSE, for
 *              :           - stopping VUC data load
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_StartUCLoad(BE_HDMITX_DATA *pHdmiTx, BOOL startLoad);

/*-----------------------------------------------------------------------------
 * Description  : This function sets video format parameters
 * Arguments    : pVideoParams - Pointer to video format parameters (Input)
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetVideoFmtParams(BE_HDMITX_DATA *pHdmiTx,
                                VPP_BE_HDMITX_OUT_MODE OutMode,
                                PVPP_BE_HDMITX_VIDEO_FMT_PARAMS pVideoParams,
                                INT HdmiVidFmt);

/*-----------------------------------------------------------------------------
 * Description  : This function sets HDMI/DVI output mode
 * Arguments    : OutMode - HDMI/DVI Mode
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetMode(BE_HDMITX_DATA *pHdmiTx, VPP_BE_HDMITX_OUT_MODE OutMode);

/*-----------------------------------------------------------------------------
 * Description  : This function fills the given data in the buffer of the
 *              : specified index and enables transfer
 * Arguments    : PktTxMode - Packet transfer mode
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SendHostPkt(BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_ID pktType,
                          VPP_BE_HDMITX_HOST_PKT_TX_MODE PktTxMode);

/*-----------------------------------------------------------------------------
 * Description  : This function disables the packet transfer in the given
 *              : index
 * Arguments    : PktTxMode  - Mode of transfer
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_DisableHostPkt(BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_ID pktType,
                             VPP_BE_HDMITX_HOST_PKT_TX_MODE PktTxMode);

/*-----------------------------------------------------------------------------
 * Description  : This function sets DRM packet data
 * Arguments    : pPktData   - Pointer to return packet data
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetDRMpacket(BE_HDMITX_DATA *pHdmiTx, UINT8 *pPktData);

/*-----------------------------------------------------------------------------
 * Description  : This function sets Vendor Specific infoframe
 * Arguments    : pPktData   - Pointer to return packet data
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetVndrSpecInfoFrame(BE_HDMITX_DATA *pHdmiTx,
                VPP_HDMI_PKT_VNDRSPEC_INFOFRM *vsif, UINT8 *pPktData);

/*-----------------------------------------------------------------------------
 * Description  : This function returns the stored host packet data at the
 *              : given index
 * Arguments    : pPktData   - Pointer to return packet data
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : Packet data is returned only if transfer is enabled for the
 *              : given packet
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_GetHostPktTxData(BE_HDMITX_DATA *pHdmiTx, UINT8 pktType,UINT8 *pPktData);

/*-----------------------------------------------------------------------------
 * Description  : This function clears host packet data at the given index
 * Arguments    : pktIndex   - Packet Index
 *              : bcmType - bcm buffer register programming goes to
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_ClearHostPktTxData(BE_HDMITX_DATA *pHdmiTx, UINT8 pktIndex, UINT8 bcmType);

/*-----------------------------------------------------------------------------
 * Description  : This function synchronizes internal software status
 *              : of host packet control in case of transmit once packets
 * Arguments    : pPktTxOnceStatus - Pointer to return transmit once status
 *              :                  - of each packet (Output)
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SyncHostPktTxOnceStatus(BE_HDMITX_DATA *pHdmiTx, UINT8 *pPktTxOnceStatus);

/*-----------------------------------------------------------------------------
 * Description  : This function mutes audio and/or video
 * Arguments    : audio - Enable/Disable audio mute
 *              : video - Enable/Disable vidio mute
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetMute(BE_HDMITX_DATA *pHdmiTx, BOOL Audio, BOOL Video);

/*-----------------------------------------------------------------------------
 * Description  : This function mutes audio
 * Arguments    : mute - Enable/Disable audio mute
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioMute(BE_HDMITX_DATA *pHdmiTx, BOOL mute);

/*-----------------------------------------------------------------------------
 * Description  : This function mutes video
 * Arguments    : mute - Enable/Disable video mute
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetVideoMute(BE_HDMITX_DATA *pHdmiTx, BOOL mute);

/*-----------------------------------------------------------------------------
 * Description  : This function returns audio and video mute status
 * Arguments    : pAudio - Pointer to return audio mute status (Output)
 *              : pVideo - Pointer to return video mute status (Output)
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_GetMuteSts(BE_HDMITX_DATA *pHdmiTx, BOOL *pAudio, BOOL *pVideo);

/*-----------------------------------------------------------------------------
 * Description  : This function returns current Hot Plug Status
 * Arguments    : pHPDStatus - Pointer to return HPD status
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_GetHPDStatus(BE_HDMITX_DATA *pHdmiTx, BOOL *pHPDStatus);

INT BE_HDMITX_SendAVMute(BE_HDMITX_DATA *pHdmiTx, BOOL mute);

INT BE_HDMITX_GetGCPEnableStatus(BE_HDMITX_DATA *pHdmiTx, BOOL *pstatus);

INT BE_HDMITX_GetAVMuteSts(BE_HDMITX_DATA *pHdmiTx, BOOL *pState);


/* HDMI PHY APIs */

/*-----------------------------------------------------------------------------
 * Description  : This function enables/disables HDMI TMDS
 * Arguments    : Enable - True/False to control TMDS
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMIPHY_EnableTmds (BE_HDMITX_DATA *pHdmiTx, BOOL Enable);

INT BE_HDMITX_ConfigHdmiVideoFmt(BE_HDMITX_DATA  *pHdmiTx, int aAspRatio,
                                    int scanInfo, BOOL updateInfoFrame);

INT BE_HDMITX_ConfigHdmiAudioFmt(BE_HDMITX_DATA  *pHdmiTx, int aChannelAllocation, int aLvlShiftVal, int aDownMixInhFlag);

BOOL BE_HDMITX_PackAudioChnSts(BE_HDMITX_DATA  *pHdmiTx, HDMI_AUD_CHN_STS_DATA pAudioChnSts);

INT BE_HDMITX_reset(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_phy_standby(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_init(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_Deinit(BE_HDMITX_DATA *pHdmiTx);

INT BE_HDMITX_WrapperEnableFixColor(BE_HDMITX_DATA *pHdmiTx, int enable);
INT BE_HDMITX_ScramblingEnable(BE_HDMITX_DATA *pHdmiTx, BOOL enable);

INT BE_HDMITX_SetACPPacket(BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_ACP *pAcpPktData);
INT BE_HDMITX_SetISRC1Packet (BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_ISRC1 *pIsrc1PktData);
INT BE_HDMITX_SetISRC2Packet (BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_ISRC2 *pIsrc2PktData);
INT BE_HDMITX_SetSPDInfoFrame (BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_SPD_INFOFRM *pSPDData);
INT BE_HDMITX_SetGMDPacket (BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_GMD *pGmdPktData);

int BE_HDMITX_GetHdmiPowerStatus( BE_HDMITX_DATA *pHdmiTx, int *pPowerState);
INT BE_HDMITX_EnableInterrupts(BE_HDMITX_DATA *pHdmiTx, VPP_BE_HDMITX_IRQ_SOURCE irq_src, BOOL enable);

INT BE_HDMITX_UpdateHPDStateChange(BE_HDMITX_DATA *pHdmiTx, int rxConnected);
INT BE_HDMITX_I2CBusClear(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_ReadEdidBlock(BE_HDMITX_DATA *pHdmiTx, UINT32 blockNum, UINT8 *pDataBuf);
INT BE_HDMITX_ReadChecksum(BE_HDMITX_DATA *pHdmiTx,int blks, UINT8 *pChecksum);
INT BE_HDMITX_EnableHdcpFrameProt(BE_HDMITX_DATA *pHdmiTx, BOOL enable);
INT BE_HDMITX_SetHdcpVersion(BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_HDCP_VERSION hdcpVersion);
INT BE_HDMITX_UpdateContentStrmInfo(BE_HDMITX_DATA *pHdmiTx, BOOL hdcp_content_type);
INT BE_HDMITX_EnableRxSense(BE_HDMITX_DATA *pHdmiTx, int enable);
INT BE_HDMITX_UpdateAVIInfoFrame(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_SetHdmiPhy(BE_HDMITX_DATA *pHdmiTx, unsigned char * phySetting, unsigned int PhyBufSize);
INT BE_HDMITX_PowerOffTMDS(BE_HDMITX_DATA *pHdmiTx, int enable);
INT BE_HDMITX_PhyPowerUp(BE_HDMITX_DATA *pHdmiTx);
void BE_HDMITX_TriggerHdcpFallback(BE_HDMITX_DATA  *pHdmiTx);
INT BE_HDMITX_Hdcp2xPreauth(BE_HDMITX_DATA *pHdmiTx);

void BE_HDMITX_TriggerHdcpFallback(BE_HDMITX_DATA  *pHdmiTx);
VOID BE_HDMITX_ClearSinkDetectionInterrupts(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_GetIrqSource(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_StartHdcpAuthentication (BE_HDMITX_DATA *pHdmiTx);
void BE_HDMITX_StopHdcpAuthentication (BE_HDMITX_DATA *pHdmiTx);
void BE_HDMITX_HDCP14_InterruptEnable(BE_HDMITX_DATA *pHdmiTx, uint8_t mask, int enable);
VPP_BE_HDCP14_INTR_STAT BE_HDMITX_HDCP14_InterruptHandler(BE_HDMITX_DATA *pHdmiTx);
HDCP_ENGAGE_STATE BE_HDMITX_GetActiveHdcpState (BE_HDMITX_DATA *pHdmiTx);
HDMITX_HDCP_RETURN_STATE BE_HDMITX_CheckHdcp2xAuthStatus(BE_HDMITX_DATA *pHdmiTx);
void BE_HDMITX_HDCP2XDeInitCtx(BE_HDMITX_DATA *pHdmiTx);
bool BE_HDMITX_HDCP2XIsESMAlive(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_GetHdmiHpdStatus(BE_HDMITX_DATA *pHdmiTx, BOOL *pHPDStatus);
INT BE_HDMITX_GetRxSense(BE_HDMITX_DATA *pHdmiTx,int *pRxSense);
int BE_HDMITX_HDCP2X_Resume(BE_HDMITX_DATA *pHdmiTx);
int BE_HDMITX_HDCP2X_Suspend(BE_HDMITX_DATA *pHdmiTx);
INT BE_HDMITX_GetHdcpKsv(BE_HDMITX_DATA *pHdmiTx, VPP_HDCP_KSV_DATA *pHDCPKsv);
INT BE_HDMITX_HDCP2XSetOpMode(BE_HDMITX_DATA *pHdmiTx, INT opMode);
BOOL BE_HDMITX_phy_getState(BE_HDMITX_DATA *pHdmiTx);
unsigned char BE_HDMITX_ConfigureScrambler(BE_HDMITX_DATA *pHdmiTx);
#endif // __VPP_BE_HDMITX_H__


