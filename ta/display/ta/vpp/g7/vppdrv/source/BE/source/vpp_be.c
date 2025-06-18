/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_BE_C_

#include "vpp_api.h"
#include "vpp_api_types.h"
#include "vpp_module.h"
#include "vpp_cfg.h"
#include "vpp_be.h"
#include "vpp_be_prv.h"
#ifdef CONFIG_VPP_DEV_KEY_SUPPORT
#include "vpp_devkey.h"
#endif
#include "vpp_be_cec_srv.h"
#include "vpp_be_hdcp_2x.h"
#include "vpp_csc_icsc.h"
#include "vpp_cpcb_tg.h"
#include "vpp_be_vop.h"
#include "vpp_be_hdmitx.h"
#include "vpp_be_hdcp.h"
#include "vpp_apifuncs.h"
#include "hdmi_aux_data_pack.h"
#include "hdmi_cmn.h"
#include "hdmi_edid.h"
#include "hdmi_scdc.h"
#include "hdmi_audio.h"
#include "vpp_be_hdmitx_cfg.h"
#include "vpp_hdcp_keyload.h"
#ifndef CONFIG_AMP_TEE_ENABLE
#include "gpio.h"
#if !defined(VPP_DIAGS)
#endif
#endif //CONFIG_AMP_TEE_ENABLE

#include "vpp_gamma.h"
#include "hdtx.h"
#include "eddc.h"
#include "hdmi_eddc.h"
#include "hdmi_scdc.h"
#include "vpp_be_hdmitx_prv.h"
#include "vpp_dv_api.h"
#include "vpp_be_hdr.h"
#ifdef CONFIG_AMP_TEE_ENABLE
#ifndef CONFIG_OPTEE
#include "tee_internal_time_api.h"
#endif
#include "tee_ta_vpp_be.h"
#endif
#include "SNPS_HDMI_tx.h"

#ifdef EARC_RX_ENABLED
#include "vpp_be_earc_vendor.h"
#include "vpp_be_earc.h"
#endif

#define EDDC_PORT 0
#define HDMI_HDCP_VERSION_READ_PORT 0x50
#define HDMI_HDCP_BSTATUS_READ_PORT 0x41
#define CONTIN_MON_INTER 60
#define HDCP2X_RETRY_TIMEOUT 100
#define HDCP2X_STATCHECK_START_TIMEOUT 50
#define FRAME_DELAY 45 /* 45ms-Delay to ensure sink has received atleast one infoframe */
#define HDCP2X_RETRY_TIMEOUT 100
#define HDCP2X_STATCHECK_START_TIMEOUT 50
#define MAX_SUPPORTED_EDID_BLOCKS  8

#if !defined(FPGA)
#define EDID_RAWDATA_DUMP
#endif

#define CHECK_IF_RET_STATUS(val)   \
        do { if(FALSE == val)   \
                return MV_VPP_EBADPARAM;}while (0)

#define CHECK_FOR_TRADITNL_HDR_CLR_PRM(x) \
    (x == VPP_COLOR_PRIM_BT2020 || (x == VPP_COLOR_PRIM_BT709)  ? 1 : 0)

#define CHECK_FOR_TRADITNL_HDR_TRNSFR_CHAR(x) \
    ((x == VPP_TRANSFER_CHAR_BT709 || x == VPP_TRANSFER_CHAR_BT2020_1  || \
                                 x == VPP_TRANSFER_CHAR_BT2020_2) ? 1 : 0)

#ifndef I2C_ENOHANDLE
#define I2C_ENOHANDLE -4
#endif
#ifndef I2C_OK
#define I2C_OK 1
#endif

#define VPP_HDCP_START_DELAY_FRAME_COUNT       40
#define VPP_HDMI_VBI_DELAY_PHY_POWER_UP        35
#define VPP_HDMI_VBI_DELAY_CONFIG_AVUNMUTE     32
#define VPP_HDMI_VBI_DELAY_CONFIG_AUDIO        30
#define VPP_HDMI_MODE_STABLE_CHECK_COUNT       11
#define VPP_HDMI_BSTATUS_SIZE                  2
#define VPP_HDMI_BSTATUS_MASK_HDMI_MODE        0x10

#define HDCP1X_FALLBACK_THRES              3
#define UPSTREAM_START_ALWAYS
#define HDCP_NOTIF_RETRY_CNT               3
#define VPP_BE_SINK_ID_LIST_HEADER_SIZE    2
#define VPP_HDMI_TMDS_4K_FILTER            590

              ///////////////////////////////////////////
              //                                       //
              //    BE functions for Berlin            //
              //                                       //
              ///////////////////////////////////////////

static int HdmiTxReset (VPP_OBJ *vpp_obj);
static int HdmiTxInit (VPP_OBJ *vpp_obj);
static int HdmiTxTerm (VPP_OBJ *vpp_obj);
static int HdmiTxHandleHotPlug (VPP_OBJ *vpp_obj, BOOL rxConnected);
static int HdmiTxConfigAVIInfoFrame (VPP_OBJ *vpp_obj,
                                     int aspRatio, int scanInfo,
                                     int extCalorimetry, BOOL newFrame);
static int HdmiTxCalcSpkrChnMap (VPP_HDMI_SPKR_ALLOC *pInSpkrAlloc,
                                 unsigned char *pOutSpkrMap);
static int HdmiTxEnableEncryption(VPP_OBJ *vpp_obj, BOOL enable);
static int HdmiTxGetEdidInfo (VPP_OBJ *vpp_obj,VPP_HDMI_SINK_CAPS *pEdidInfo);
static int HdmiTxMapPixelReptSupport (UINT32 videoIDCode, VPP_HDMI_SINK_CAPS *pEdidInfo);
static int HdmiTxMapVICToResID(UINT32 videoIDCode, UINT32 *pVideoRes, BOOL *pNewRes);
static int HdmiTxMapVICTo3DResID(UINT32 videoIDCode, UINT32 *p3DVideoRes, UINT32 i3DStructure, UINT32 *p3DStructures);
static int HdmiTxGetLatencyInfo (UINT8 latency, VPP_EDID_LATENCY_STS *pLatencySts, UINT32 *pLatencyVal);
static int HdmiTxTimerCmdCfg (VPP_OBJ *vpp_obj, int timerCmdInterval);
static void HdmiTxSetHdcpSMState(BE_HDMITX_DATA *pHdmiTx, HDMITX_HDCP_SM_STATE state);
#if HDCP2X_ENABLED
static int VPP_BE_BlacklistCurrentSink(VPP_OBJ *vpp_obj);
#endif /* HDCP2X_ENABLED*/
static void VPP_BE_UnmuteAndPowerUp(VPP_OBJ *vpp_obj);
static INT VPP_BE_ConfigDither(VPP_OBJ *vpp_obj, UINT32 uiMode, UINT32 uiBitDepth);

static void DelayInMS(int sec, int DelayTime)
{
#ifndef CONFIG_AMP_TEE_ENABLE

    MV_TimeSpec_t   TimeSpec;

    TimeSpec.tv_sec = sec;
    TimeSpec.tv_nsec = (DelayTime * 1000 * 1000);
    MV_TimeSpec_Delay (&TimeSpec);

#else

    int TotalDelay = (sec * 1000) + DelayTime;
    MV_OSAL_Task_Sleep(TotalDelay);

#endif
}

static void VPP_BE_UnmuteAndPowerUp(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPPHDMI_DBG_INFO("Unmute Video\n");
    VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, FALSE);
    BE_HDMITX_ConfigureScrambler(pHdmiTx);
    VPPHDMI_DBG_INFO("power up phy\n");
    BE_HDMITX_PhyPowerUp(pHdmiTx);

}

#if HDCP2X_ENABLED || defined (CONFIG_VPP_FASTLOGO_SUPPORT)
static bool is4K30HdmiVicSupportedForResIndex(BE_HDMITX_DATA* pHdmiTx)
{
    if(pHdmiTx->edidInfo.sprtedHdmiVICRes & (1 << (RES_4Kx2K30 - RES_MIN_4Kx2K)))
        return true;
    else
        return false;
}
#endif

static bool is4K5060HdmiVicSupportedForResIndex(BE_HDMITX_DATA* pHdmiTx)
{
    if (pHdmiTx->edidInfo.CeaHdmiVSDB.MAX_TMDS_Character_Rate*5 > VPP_HDMI_TMDS_4K_FILTER) {
        for (int i = 0; i < pHdmiTx->edidInfo.svdblocklen; i++) {
             if (pHdmiTx->edidInfo.svd_vics[i] == VPP_HDMI_4K50_VIC ||
                 pHdmiTx->edidInfo.svd_vics[i] == VPP_HDMI_4K5994_4K60_VIC) {
                 return true;
             }
        }
        return false;
    } else {
        return false;
    }
}

#if HDCP_ENABLED
static void HdmiCheckStartDelayedAuth(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx->authRequired) {
        VPPHDMI_DBG_INFO("----starting authenitcation ---\n");
        HdmiTxEnableEncryption (vpp_obj, TRUE);
    } else {
        VPPHDMI_DBG_INFO("----authenitcation disabled.. unmute video ---\n");
        VPP_BE_UnmuteAndPowerUp(vpp_obj);
    }
}

/*******************************************************************
 * FUNCTION: Enable/Disable Encryption
 * PARAMS:   *vpp_obj - pointer to VPP object
 *       :   enable   - enable/disable encryption
 * RETURN:   MV_VPP_OK
 ******************************************************************/
static int HdmiTxRptrDsEnableEncryption(VPP_OBJ *vpp_obj, BOOL enable)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    HDMITX_CMD      cmd;

    HdmiTxTimerCmdCfg (vpp_obj, 0);
    if (enable)
    {
        if ((pHdmiTx->hdcp2xDSEarlyAuthStarted == true) ||
            (pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x) ||
            (pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x) ||
            (pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_AUTO)) {
            pHdmiTx->frameCntr = 3;
            VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_GCP, FALSE);
            VPP_BE_SET_BIT(vpp_obj->be.hdmiFrmIntrTrig, VPP_BE_HDMI_HDCP_FRMTRIG);
        } else {
            /* start tx with minimum time dealy in receving upstream to reduce upstream resume delay
               test case 3C-01-3. Required delay will already be accounted as upstream HPD toggle is
               done only after setting downstream resolution */
            cmd.cmdID = HDMITX_CMD_HDCP2X_START;
            cmd.param = 0;
            HdmiFifoPush (&pHdmiTx->hdcpCmdFifo, (UINT8*)&cmd, TRUE);
        }
    }
    else
    {
        HdmiTxSetHdcpSMState(pHdmiTx, HDCP_SM_STATE_IDLE);
        VPP_BE_CLR_BIT(vpp_obj->be.hdmiFrmIntrTrig, VPP_BE_HDMI_HDCP_FRMTRIG);
        pHdmiTx->frameCntr = -1;
        BE_HDMITX_StopHdcpAuthentication (pHdmiTx);
    }
    return (MV_VPP_OK);
}
#endif

/*******************************************************************
 * FUNCTION: Function to Force the HDMi Hot Plug Event
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int VPP_BE_HdmiTxForceHotplug(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    HDMITX_CMD      cmd;
    BOOL             hpd;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPPHDMI_DBG_INFO(" Forced hotplug ..........................  \n");
    if (pHdmiTx != NULL)
    {
        BE_HDMITX_GetHPDStatus (pHdmiTx, &hpd);
        if (hpd) {

            /* reset all previous commands */
            HdmiFifoReset (&pHdmiTx->hpdcmdFifo);
            HdmiTxTimerCmdCfg (vpp_obj, 0);
            cmd.cmdID = HDMITX_CMD_FORCE_HPD;
            HdmiFifoPush (&pHdmiTx->hpdcmdFifo, (UINT8*)&cmd, FALSE);

            pHdmiTx->forceHPDEnabled = TRUE;
           // Trigger for fifo signalling
           VPP_BE_SET_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_SIG_FIFO_FRMTRIG);
        } else {
            VPPHDMI_DBG_INFO("Skip forced hotplug : HPD = 0 rxConnected = %d\n", pHdmiTx->rxConnected);
            pHdmiTx->rxConnected = FALSE;
        }
    }
    return MV_VPP_OK;
}

/*******************************************************************
 * FUNCTION: Construct HDR info frame packet
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdmiTxConfigHDRInfoFrame (VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA      *pHdmiTx;
    UINT8               dataBuf[31];
    VPP_HDMI_PKT_HDR_INFO_FRAME  *phdrInfo = &vpp_obj->hdrInfo;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    GaloisMemClear (dataBuf, 31);

    if((pHdmiTx->edidInfo.HdrCapInfo.hdr10Info.EOTF_ET_2 == TRUE)
       && ((vpp_obj->transfer_characteristics == VPP_TRANSFER_CHAR_SMPTE_ST_2048)
       || (vpp_obj->transfer_characteristics  == VPP_TRANSFER_CHAR_SMPTE_ST_428_I)))
    {
        phdrInfo->EOTF = HDR_EOTF_SMPTE_2048_LUMI_RNG;
    }
#ifdef CONFIG_AMP_HLG
    else if((pHdmiTx->edidInfo.HdrCapInfo.hdr10Info.EOTF_ET_3 == TRUE)
            && ((vpp_obj->transfer_characteristics == VPP_TRANSFER_CHAR_ARIB_B67) ||
                (vpp_obj->alt_transfer_characteristics == VPP_TRANSFER_CHAR_ARIB_B67)))
    {
        phdrInfo->EOTF = HDR_EOTF_HLG;
    }
#endif
    else {
        phdrInfo->EOTF = HDR_EOTF_SDR_LUMI_RNG;
    }

    VppPackHDRInfoframe(vpp_obj,phdrInfo, dataBuf);
    // Set host packet data
    BE_HDMITX_SetDRMpacket (pHdmiTx, dataBuf);
    return (MV_VPP_OK);
}

static int HdmiGetRxHdcpVersion(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    UINT8 HdmiRxHdcpVersion;
    UINT8 regValue = 0x00;
    UINT16 HdcpVersionOffset = HDMI_HDCP_VERSION_READ_PORT ; /*DDC_VERSION_READ_PORT 0x50 */
    INT32 returnVal;
    INT32 i2cRetryCount = 3;
    do {
           returnVal = EDDC_Read(pHdmiTx, 1 ,HdcpVersionOffset , sizeof(regValue) , &regValue);
           if(returnVal < 0) {
               i2cRetryCount--;
               regValue = 0x00;
               VPPHDMI_DBG_INFO("hdcp version read: i2c return %d error \n",returnVal);
           }
           else {
               VPPHDMI_DBG_INFO("hdcp version read: [0x%x]\n", regValue);
               i2cRetryCount = 0;
           }
       }while(i2cRetryCount > 0);

    if (returnVal < 0) {
        if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
            /* in repeater mode, if version read failed, limit to HDCP1.4
               to avoid possible converter cases */
            pHdmiTx->SinkMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
            pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
            VPPHDMI_DBG_INFO(".........sink hdcp version read fail: limit to HDCP 1.4.. \n");
        } else {
            pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_NONE;
            VPPHDMI_DBG_INFO(".........HDCP version read failed. deferred to later.....\n");
        }
        return 0;
    }

    HdmiRxHdcpVersion = (regValue & 0x04) >> 2 ;
    if(HdmiRxHdcpVersion) {
        pHdmiTx->SinkMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_2_2;
        if (pHdmiTx->SourceMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) {
            pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_2_2;
            VPPHDMI_DBG_INFO(".........sink supported HDCP version : HDCP 2.2/1.4....... \n");
#if HDCP2X_ENABLED
            BE_HDMITX_Hdcp2xPreauth(pHdmiTx);
#endif
        } else {
            if(BE_HDMITX_HDCP2XIsESMAlive(pHdmiTx)) {
                pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_2_2;
                pHdmiTx->SourceMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_2_2;
            } else {
                pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
            }
            VPPHDMI_DBG_INFO(".........sink supported HDCP version : HDCP 2.2/1.4. But scaling back to HDCP1.4 as ESM not activated....... \n");
        }
    }
    else {
        pHdmiTx->SinkMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
        pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
        VPPHDMI_DBG_INFO(".........sink supported HDCP version : HDCP 1.4....... \n");
    }

    VPPHDMI_DBG_INFO(".........current HDCP version selected %s.......\n",
        ((pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_AUTO)? "AUTO":
         (pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2)?"2.2":
         (pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x)?"1.4":"NONE"));

    return MV_VPP_OK;
}

static void HdmiTxSetHdcpSMState(BE_HDMITX_DATA *pHdmiTx, HDMITX_HDCP_SM_STATE state)
{
    switch (state) {
        case HDCP_SM_STATE_IDLE:
            pHdmiTx->sm_state = HDCP_SM_STATE_IDLE;
            break;
        case HDCP_SM_STATE_TRIGGERED:
            if(pHdmiTx->sm_state == HDCP_SM_STATE_IDLE) {
                pHdmiTx->sm_state = HDCP_SM_STATE_TRIGGERED;
            } else {
                pHdmiTx->sm_state = HDCP_SM_STATE_REAUTH;
            }
            break;
        case HDCP_SM_STATE_AUTHENTICATED:
            pHdmiTx->sm_state = HDCP_SM_STATE_AUTHENTICATED;
            break;
        default:
            VPPHDMI_DBG_ERROR("invalid state trasition for HDCP %d->%d\n", pHdmiTx->sm_state, state);
            break;
    }
}

static INT VPP_BE_ConfigDither(VPP_OBJ *vpp_obj, UINT32 uiMode, UINT32 uiBitDepth)
{
    T32DITHER_CFG0 stDitherCfg0;
    memset(&stDitherCfg0, 0, sizeof(T32DITHER_CFG0));

    BE_VOP_ConfigDither(&stDitherCfg0, uiMode, uiBitDepth);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_DITHER, stDitherCfg0.u32);
    return 0;
}

/*******************************************************************
 * FUNCTION: Create VPP BE Hdmi extended objects
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_HdmiExt_Create(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;

    /* init BE_HDMITX_DATA struct, HDMI/HDCP related threads and other OSAL related inits */
    if(HdmiTxInit(vpp_obj) == MV_VPP_ENOMEM)
        return MV_VPP_ENOMEM;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if(BE_HDMITX_init(pHdmiTx) == MV_VPP_ENOMEM)
        return MV_VPP_ENOMEM;

    /*Dyanmic memory to store Edid  */
    pHdmiTx->pEdidArray = VPP_Malloc(VPP_SIZE_OF_EDID_BLK*(MAX_SUPPORTED_EDID_BLOCKS)*sizeof(UINT8));

    if (VPP_HDCP_KEYLOAD_Init(vpp_obj) == MV_VPP_ENOMEM)
        return MV_VPP_ENOMEM;

    /* notify application max supported source hdcp version. To proceed with HDCP1.4 in case HDCP2.2 fw
       initialization failed. */
    if (pHdmiTx->SourceMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) {
        HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2, (MV_VPP_EVENT_HDCP_ENGINE_INIT|MV_VPP_EVENT_HDCP_SRC_VERSION_2_X));
    } else {
        HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2, (MV_VPP_EVENT_HDCP_ENGINE_INIT|MV_VPP_EVENT_HDCP_SRC_VERSION_1_X));
    }

    if(VPP_BE_CEC_Init(vpp_obj) == MV_VPP_ENOMEM)
        return MV_VPP_ENOMEM;

    return MV_VPP_OK;
}

/*******************************************************************
 * FUNCTION: Destroy VPP BE Hdmi extended objects
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_HdmiExt_Destroy(VPP_OBJ *vpp_obj)
{
    VPP_HDCP_KEYLOAD_DeInit(vpp_obj);

    HdmiTxTerm(vpp_obj);

    VPP_BE_CEC_Term(vpp_obj);

    return MV_VPP_OK;
}

/*******************************************************************
 * FUNCTION: Reset all VPP BE blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_Reset(VPP_OBJ *vpp_obj)
{
    UINT32 RegAddr;

    /* reset BE vop */
    BE_VOP_LoadDefaultVal(vpp_obj);

    /* reset HDMI TX */
    HdmiTxReset (vpp_obj);

    VPP_GAMMA_SetDefaults(vpp_obj,VPP_VOP);
    VPP_GAMMA_SetDefaults(vpp_obj,VPP_VOP2);

    /* move to VPP_BE_HdmiExt_Create()
    VPP_BE_CEC_Init(vpp_obj);
    */
    /* Reset CEC, if enabled */
    if (vpp_obj->be.enableCec)
    {
        VPP_BE_CEC_Enable (vpp_obj, TRUE);
    }
    //TTL initilization required for only FPGA platform
    GA_REG_WORD32_WRITE(vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_TTL30_CTRL, 0x1C);
    GA_REG_WORD32_WRITE(vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_TTL30, 0x4058407);
    GA_REG_WORD32_WRITE(vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_LVDS_HDMI_DEBUG, 0x00);
    //GA_REG_WORD32_WRITE(0xf74800EC, 0x1);//veloce
    //GA_REG_WORD32_WRITE(0xF7444400, 0x25);//veloce

    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG0;
    GA_REG_WORD32_READ(RegAddr, &(vpp_obj->hdmi_vop.hdmi_vop_cfg0.u32));
    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG1;
    GA_REG_WORD32_READ(RegAddr, &(vpp_obj->hdmi_vop.hdmi_vop_cfg1.u32));
    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG2;
    GA_REG_WORD32_READ(RegAddr, &(vpp_obj->hdmi_vop.hdmi_vop_cfg2.u32));
    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG3;
    GA_REG_WORD32_READ(RegAddr, &(vpp_obj->hdmi_vop.hdmi_vop_cfg3.u32));
    GA_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_CTRL, &(vpp_obj->vpp_Ctrl.u32));
    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 + RA_VOP2_CTRL0;
    GA_REG_WORD32_READ(RegAddr, &(vpp_obj->vop2_ctrl0.u32));
    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG4;
    GA_REG_WORD32_READ(RegAddr, &(vpp_obj->hdmi_vop.hdmi_vop_cfg4.u32));
    vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_ClampMap_bypass = 1; //TODO Review once ClampMap BDR ready
    vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_DB_edr_hdmi_bypass = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->hdmi_vop.hdmi_vop_cfg4.u32);
    RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG7;
    GA_REG_WORD32_READ(RegAddr, &(vpp_obj->hdmi_vop.hdmi_vop_cfg7.u32));
    VPP_DV_EdrReset(vpp_obj);

    return (MV_VPP_OK);
}

/*********************************************************************
 * FUNCTION: initialize all VPP BE blocks according to configuration
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ********************************************************************/
int VPP_BE_Config(VPP_OBJ *vpp_obj)
{
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: select input for VOUTs in VPP BE blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           voutID - Vout
 *           cpcbout - CPCB output
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetVoutInput(VPP_OBJ *vpp_obj, int voutID, int cpcbout)
{
    VPP_VOP_HDMI_CTRL hdmi_ctrl;

    switch (voutID){
        case VOUT_HDMI:
            hdmi_ctrl.CBlank = 0x800; /* blank */
            hdmi_ctrl.InputSel = cpcbout;
            hdmi_ctrl.OutputAlign = VPP_VOP_8_10_MSB_ALIGN;
            BE_VOP_SetHdmiParams(vpp_obj, &hdmi_ctrl);
            break;
        case VOUT_DSI:
            BE_VOP_SetDSIParams(vpp_obj,cpcbout);
            break;
        default:
            break;
    }
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: set output format for VOUTs in VPP BE blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           voutID - Vout
 *           color_fmt - CPCB output color format
 *           bit_depth - CPCB output bit depth
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetVoutFormat(VPP_OBJ *vpp_obj, int voutID, int color_fmt, int bit_depth)
{
    int round_mode;
    UINT32 regAddr;
    T32VOP_CTRL stVop_ctrl;
#ifndef ENABLE_GFX2_VOP2_PATH
    UINT32 RegAddr;
#else
    UINT32 csc_mode;
#endif


    /*map round mode to register value*/
    if(OUTPUT_BIT_DEPTH_12BIT == bit_depth)
    {
        round_mode = VPP_VOP_DATA_BYPASS;
        vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dither_inctrl = 1;
    }
    else if(OUTPUT_BIT_DEPTH_10BIT == bit_depth)
    {
        round_mode = VPP_VOP_12_TO_10_BITS_ROUND;
        vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dither_inctrl = 0;
    }
    else
    {
        round_mode = VPP_VOP_12_TO_8_BITS_ROUND;
        vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dither_inctrl = 0;
    }
#ifdef EN_VPP_DYNAMIC_CLKGATE
    if(IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE))
    {
        if(VOUT_HDMI == voutID && OUTPUT_COLOR_FMT_RGB888 == color_fmt)
        {
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_csc_bypass = 1;
        }
        else
        {
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_csc_bypass = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
            vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG3, vpp_obj->hdmi_vop.hdmi_vop_cfg3.u32);
    }
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */

    switch (voutID)
    {
        case VOUT_HDMI:
            /*set HDMI output bit depth*/
            BE_VOP_SetRoundMode(vpp_obj, VPP_VOP_HDMI, round_mode);
            vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_DB_ocsc_bypass = 0;
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dns422_420_bypass = 1;
#if !defined(FPGA)
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_input_swap_sel = 0x24;
#else
            if(OUTPUT_COLOR_FMT_RGB888 == color_fmt)
                vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_input_swap_sel = 0x12;
            else
                vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_input_swap_sel = 0x24;
#endif
            vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_edr_dns_sel = 0; //default: select legacy path

            VPP_DV_DitherCfg(vpp_obj, color_fmt, bit_depth);

            if(0 == vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dither_inctrl)
            {   //Dither is in path
                VPP_BE_ConfigDither(vpp_obj, VPP_BE_DITHER_ROUNDING, bit_depth);
            }
            if(vpp_obj->hdmi_bt2020en == TRUE) {

                if(OUTPUT_COLOR_FMT_IPT422 == color_fmt)
                {
                    BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_ENABLE);
                    BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, color_fmt);
                    BE_VOP_SetOutputFmt_420(vpp_obj, FALSE, bit_depth);
                }
                else if(OUTPUT_COLOR_FMT_YCBCR422 == color_fmt)
                {

                    BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_ENABLE);
                    BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, OUTPUT_COLOR_FMT_YCBCR422_R2020);
                    BE_VOP_SetOutputFmt_420(vpp_obj, FALSE, bit_depth);

                }
                else if(OUTPUT_COLOR_FMT_YCBCR444 == color_fmt)
                {

                    BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_BYPASS);
                    BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, OUTPUT_COLOR_FMT_YCBCR444_R2020);
                    BE_VOP_SetOutputFmt_420(vpp_obj, FALSE, bit_depth);

                }
                else if(OUTPUT_COLOR_FMT_YCBCR420 == color_fmt)
                {
                    BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_ENABLE);
                    BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, OUTPUT_COLOR_FMT_YCBCR422_R2020);
                    vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dns422_420_bypass = 0;
                    BE_VOP_SetOutputFmt_420(vpp_obj, TRUE, bit_depth);
                }

            }
            /*set HDMI output DNS mode and CSC mode*/
            else if((OUTPUT_COLOR_FMT_YCBCR422 == color_fmt) || \
                    (OUTPUT_COLOR_FMT_YCBCR422_P3D65 == color_fmt) || \
                    (OUTPUT_COLOR_FMT_YCBCR422_R2020 == color_fmt) || \
                    (OUTPUT_COLOR_FMT_IPT422 == color_fmt))
            {
                BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_ENABLE);
                BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, color_fmt);
                BE_VOP_SetOutputFmt_420(vpp_obj, FALSE, bit_depth);
            }
            else if(OUTPUT_COLOR_FMT_YCBCR444 == color_fmt)
            {
                BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_BYPASS);
                BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, color_fmt);
                BE_VOP_SetOutputFmt_420(vpp_obj, FALSE, bit_depth);
            }
            else if(OUTPUT_COLOR_FMT_YCBCR420 == color_fmt)
            {
                BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_ENABLE);
                BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, color_fmt);
                vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dns422_420_bypass = 0;
                BE_VOP_SetOutputFmt_420(vpp_obj, TRUE, bit_depth);
            }
            else
            {
                BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_BYPASS);
                BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, color_fmt);
                vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_DB_ocsc_bypass = 0;
                BE_VOP_SetOutputFmt_420(vpp_obj, FALSE, bit_depth);
            }
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_csc_bypass       = 1 ;
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dns444_422_bypass = 1;

            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG3, vpp_obj->hdmi_vop.hdmi_vop_cfg3.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG4, vpp_obj->hdmi_vop.hdmi_vop_cfg4.u32);
            break;
        case VOUT_DSI:
#if defined(FPGA)
            //FPGA output configuration
            bit_depth = OUTPUT_BIT_DEPTH_12BIT; //FIXME: avoid hardcode
            regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_CTRL;
            GA_REG_WORD32_READ(regAddr, &stVop_ctrl.u32);
            stVop_ctrl.uCTRL_dp_output_sel = 1;
            stVop_ctrl.uCTRL_dpEnb = 1;
            stVop_ctrl.uCTRL_mode_sel_fpga_regs = 1;
            stVop_ctrl.uCTRL_rnd_fpga_regs = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, regAddr, stVop_ctrl.u32);
#else
            regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_CTRL;
            GA_REG_WORD32_READ(regAddr, &stVop_ctrl.u32);
            stVop_ctrl.uCTRL_dp_output_sel = 0;
            stVop_ctrl.uCTRL_dpEnb = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, regAddr, stVop_ctrl.u32);
#endif
#ifdef ENABLE_GFX2_VOP2_PATH
            /*map round mode to register value*/
            if(OUTPUT_BIT_DEPTH_12BIT == bit_depth)
            {
                round_mode = VPP_VOP_DATA_BYPASS;
                vpp_obj->vop2_ctrl0.uCTRL0_dither_bypass = 1;
                vpp_obj->vop2_ctrl0.uCTRL0_dither_out_sel = 0 ;
            }
            else if(OUTPUT_BIT_DEPTH_10BIT == bit_depth)
            {
                round_mode = VPP_VOP_12_TO_10_BITS_ROUND;
                vpp_obj->vop2_ctrl0.uCTRL0_dither_bypass = 0;
                vpp_obj->vop2_ctrl0.uCTRL0_dither_out_sel = 1 ;
            }
            else
            {
                round_mode = VPP_VOP_12_TO_8_BITS_ROUND;
                vpp_obj->vop2_ctrl0.uCTRL0_dither_bypass = 0;
                vpp_obj->vop2_ctrl0.uCTRL0_dither_out_sel = 2 ;
            }
            csc_mode = VPP_CSC_BYPASS_MODE;
            if(color_fmt == OUTPUT_COLOR_FMT_RGB888){
                csc_mode = VPP_CSC_BYPASS_MODE;
            }else if(color_fmt == OUTPUT_COLOR_FMT_YCBCR444){
                csc_mode = VPP_CSC_RGB_TO_YUV_709;
            }else{
                VPPAPI_DBG_ERROR("unsupported color format [%d]:set to default [RGB]\r\n", color_fmt);
            }
            vpp_obj->vop2_ctrl0.uCTRL0_csc_bypass = 1;
            if(csc_mode != VPP_CSC_BYPASS_MODE)
                vpp_obj->vop2_ctrl0.uCTRL0_csc_bypass = 0;

            /*1D LUT GAMMA programming*/
            vpp_obj->vop2_ctrl0.uCTRL0_gamma_bypass = 1;
            regAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP2 + RA_VOP2_CTRL0;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, regAddr, vpp_obj->vop2_ctrl0.u32);

            /*DITHER Programming*/
            if(round_mode != VPP_VOP_DATA_BYPASS)
            {   //Dither is in path
                BE_VOP_SetDsiDitherMode(vpp_obj, VPP_BE_DITHER_ROUNDING, bit_depth);
            }
            /*CSC Programming 12-bit RGB to 8/10-bit RGB*/
            if(csc_mode != VPP_CSC_BYPASS_MODE)
            {
                VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj,VPP_CSC_VOP2,csc_mode);
            }
            VPPAPI_DBG_INFO("VOUT Format vop2_ctrl0[0x%x]\r\n", vpp_obj->vop2_ctrl0.u32);
#else
            vpp_obj->hdmi_vop.hdmi_vop_cfg0.uCFG0_hdmi_align = 2;
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dither_inctrl = 1;
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dns422_420_bypass = 1;
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_input_swap_sel = 0x24;
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_csc_bypass       = 1 ;
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dns444_422_bypass = 1;
            vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_csc_bypass = 1;
            vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_DB_ocsc_bypass = 0;
            vpp_obj->hdmi_vop.hdmi_vop_cfg4.uCFG4_edr_dns_sel = 0;

            RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_HDMI_VOP_CFG0, vpp_obj->hdmi_vop.hdmi_vop_cfg0.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_HDMI_VOP_CFG3, vpp_obj->hdmi_vop.hdmi_vop_cfg3.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_HDMI_VOP_CFG4, vpp_obj->hdmi_vop.hdmi_vop_cfg4.u32);

            round_mode = VPP_VOP_DATA_BYPASS;
            if(OUTPUT_BIT_DEPTH_12BIT != bit_depth) {
                vpp_obj->hdmi_vop.hdmi_vop_cfg3.uCFG3_dither_inctrl = 0;
                if(OUTPUT_BIT_DEPTH_10BIT == bit_depth)
                    round_mode = VPP_VOP_12_TO_10_BITS_ROUND;
                else
                    round_mode = VPP_VOP_12_TO_8_BITS_ROUND;
                VPP_BE_ConfigDither(vpp_obj, VPP_BE_DITHER_ROUNDING, bit_depth);
            }
            BE_VOP_SetRoundMode(vpp_obj, VPP_VOP_HDMI, round_mode);
            BE_VOP_SetHdmiDnsMode(vpp_obj, VPP_VOP_DNS_MODE_BYPASS);
            BE_VOP_LoadCscCoeff(vpp_obj, VPP_VOP_HDMI, color_fmt);
            BE_VOP_SetOutputFmt_420(vpp_obj, FALSE, bit_depth);
#endif //ENABLE_GFX2_VOP2_PATH
            break;
        default:
            break;
    }
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Configure Hdmi Video format
 * INPUT: color_fmt - color format (RGB, YCbCr 444, 422)
 *      : bit_depth - 8/10/12 bit color
 *      : pixel_rept - 1/2/4 repetitions of pixel
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_ConfigHdmiVideoFmt(VPP_OBJ *vpp_obj, int color_fmt,
                              int bit_depth, int pixel_rept)
{
    BE_HDMITX_DATA  *pHdmiTx;
    int             resID;
    int             retCode;
    int             aspRatio;
#ifdef HDMI_SRV_SUPPORT
    int             i;
#endif
    RESOLUTION_INFO  *pResinfo;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    // Check if CPCB path is configured
    resID = VPP_GetResId(CPCB_OF_VOUT(vpp_obj,VOUT_HDMI));
    if (resID == RES_INVALID)
    {
        return (MV_VPP_EBADPARAM);
    }

    if (pHdmiTx->videoParams.DispRes == resID &&
        pHdmiTx->videoParams.VideoFmt == color_fmt &&
        pHdmiTx->videoParams.ColorDepth == bit_depth &&
        (pHdmiTx->videoParams.PixelRpt+1) == pixel_rept)
    {
        return (MV_VPP_OK);
    }

#ifdef HDMI_SRV_SUPPORT
    if (!pHdmiTx->edidInfo.validEdid)
    {
        VPPHDMI_DBG_INFO("error: invalid EDID...not configuring video format");
        HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_VIDEO_CFG_ERR);
        return (MV_VPP_EUNSUPPORT);
    }

    // Check pixel repetition
    if (pixel_rept > 1)
    {
        for (i = 0; i < 4; i++)
        {
            if (pHdmiTx->edidInfo.prInfo[i].resMask & (1 << resID))
            {
                if (!(pHdmiTx->edidInfo.prInfo[i].prSupport & (1 << pixel_rept)))
                {
                    HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_VIDEO_CFG_ERR);
                    return (MV_VPP_EUNSUPPORT);
                }
                break;
            }
        }
        if (i == 4)
        {
            HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_VIDEO_CFG_ERR);
            return (MV_VPP_EUNSUPPORT);
        }
    }
#endif  //HDMI_SRV_SUPPORT
//    VPPAPI_DBG_INFO("configuring hdmi avi info\n");
    // Configure Video Parameters
    pHdmiTx->videoParams.DispRes = resID;

    // Save color_fmt and bit_depth
    pHdmiTx->videoParams.VideoFmt = color_fmt;
    pHdmiTx->videoParams.ColorDepth = bit_depth;
    pHdmiTx->hdmi_outformat = VPP_OUTFORMAT_SDR;
    pHdmiTx->hdmiEarlySignalCount = 0;

    if(vpp_obj->m_HdrMode != VPP_HDR_MODE_ON)
        vpp_obj->m_outformat = VPP_OUTFORMAT_SDR;

    VPP_BE_HDR_SetCapability(vpp_obj);
    pResinfo = VPP_GetResInfo(resID);

    // Set pixel repetition
    pHdmiTx->videoParams.PixelRpt = (VPP_BE_HDMITX_PIXEL_RPT_TYPE)(pixel_rept - 1);

    if ((pResinfo->type == TYPE_SD) &&
        (pResinfo->scan == SCAN_INTERLACED) &&
        (pHdmiTx->videoParams.PixelRpt == VPP_BE_HDMITX_PIXEL_RPT_NONE))
    {
        // Force pixel repetition of 2 for 480i, 576i if no pixel repetition was defined
        pHdmiTx->videoParams.PixelRpt = VPP_BE_HDMITX_PIXEL_RPT_BY_2;
    }

    if (pResinfo->type == TYPE_SD)
        aspRatio = ASPECT_RATIO_4BY3;
    else
        aspRatio = ASPECT_RATIO_16BY9;

    if ((resID >= RES_CVT_RB_MIN) && (resID <= RES_CVT_RB_MAX)) {
        aspRatio = ASPECT_RATIO_NONE;
    }
    // Config AVI packet
    if(vpp_obj->hdmi_bt2020en == TRUE){
        retCode = HdmiTxConfigAVIInfoFrame (vpp_obj,
                    aspRatio, /*SCAN_DATA_NONE*/OVER_SCAN_DATA,
                                TRUE /* No extended calorimetry */, TRUE /* New AVI Infoframe */);
    }else {
        retCode = HdmiTxConfigAVIInfoFrame (vpp_obj,
                    aspRatio, SCAN_DATA_NONE,
                                FALSE /* No extended calorimetry */, TRUE /* New AVI Infoframe */);
    }
    return (retCode);
}

#ifdef CONFIG_BT2020_SUPPORT
/********************************************************************************
 * FUNCTION: Config and Send AVIInofFrame
 * INPUT: vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SendHdmiAVIInfoFrame(VPP_OBJ *vpp_obj)
{
    int retCode;
    BE_HDMITX_DATA  *pHdmiTx;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    // Config AVI packet
    retCode = HdmiTxConfigAVIInfoFrame(vpp_obj, HDMI_USE_EXISTING,
                    HDMI_USE_EXISTING, vpp_obj->hdmi_bt2020en, FALSE);
    if (retCode == MV_VPP_OK)
        BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_AVI_INFOFRAME, 0);

    return (retCode);
}
#endif

/********************************************************************************
 * FUNCTION: Set pixel clock
 * INPUT: *vpp_obj - pointer to VPP object
 *         resID - resolution ID
 *         color_fmt - output color format
 *         PixelRpt - pixel repeat
 * RETURN: MV_VPP_OK - SUCCEED
 ********************************************************************************/
int VPP_BE_SetPixelClock(VPP_OBJ *vpp_obj, int resID, int color_fmt, int PixelRpt)
{
    int clk_ctrl;
    RESOLUTION_INFO  *pResinfo;

    if(resID == RES_INVALID)
        return (MV_VPP_EBADPARAM);

    pResinfo = VPP_GetResInfo(resID);
#if !defined(FPGA)
    if (pResinfo->freq <= 27027){
        if (pResinfo->scan == SCAN_PROGRESSIVE) {
            if (PixelRpt == VPP_BE_HDMITX_PIXEL_RPT_BY_4)
                clk_ctrl = VPP_VOP_CLK_D1;
            else if (PixelRpt == VPP_BE_HDMITX_PIXEL_RPT_BY_2)
                clk_ctrl = VPP_VOP_CLK_D2;
            else
                clk_ctrl = VPP_VOP_CLK_D1;
        } else {
            if (PixelRpt == VPP_BE_HDMITX_PIXEL_RPT_BY_4)
                clk_ctrl = VPP_VOP_CLK_D2;
            else
                clk_ctrl = VPP_VOP_CLK_D4;
        }
    }
    else
    {
        if ((pResinfo->freq >= 593406) && (color_fmt == OUTPUT_COLOR_FMT_YCBCR420))
            clk_ctrl = VPP_VOP_CLK_D2;
        else
            clk_ctrl = VPP_VOP_CLK_D1;
    }

#else
    if (pResinfo->freq >= 593000) {
        if(color_fmt != OUTPUT_COLOR_FMT_YCBCR420)
            clk_ctrl = VPP_VOP_CLK_D1;
        else
            clk_ctrl = VPP_VOP_CLK_D2;
    }
    else if (pResinfo->freq >= 296000)
        clk_ctrl = VPP_VOP_CLK_D2;
    else if (pResinfo->freq >= 148000) {
        if (pResinfo->scan == SCAN_PROGRESSIVE)
            clk_ctrl = VPP_VOP_CLK_D4;
        else
            clk_ctrl = VPP_VOP_CLK_D8; //to be updated
    }
    else if (pResinfo->freq >= 74000)
        clk_ctrl = VPP_VOP_CLK_D8;
    else
        clk_ctrl = VPP_VOP_CLK_D12;

#endif
    BE_VOP_ClkSelect(vpp_obj, VPP_VOP_HDMI, clk_ctrl);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set Hdmi Video format
 * INPUT: *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SetHdmiVideoFmt(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    int             retCode;
    int hdmiVideoFmt = VPP_HDMI_VIDEO_FMT_NO_INFO;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);


    if(VPP_BE_SetPixelClock(vpp_obj,
                pHdmiTx->videoParams.DispRes,
                pHdmiTx->videoParams.VideoFmt,
                pHdmiTx->videoParams.PixelRpt) != MV_VPP_OK) {
        return (MV_VPP_OK);
    }

    /* phy standby audio/video mute is done by VPP_BE_SetHdmiPhyPower by SetCPCB */
    BE_HDMITX_SetVideoMute(pHdmiTx, TRUE);

    // Disable all host packets
    BE_HDMITX_DisableHostPkt (pHdmiTx, 0xFF, VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC);
    BE_HDMITX_EnableAudioPkt(pHdmiTx, FALSE);

    // Configure HDMI VOP Path
    VPP_BE_SetVoutFormat (vpp_obj, VOUT_HDMI,
                          pHdmiTx->videoParams.VideoFmt, pHdmiTx->videoParams.ColorDepth);

    if(VPP_DV_GetCapability(vpp_obj, DV_CAP_EDRCAP) <= VPP_HDMI_SINK_STD_DOVI)
    {
        if(VPP_IsResIdInRange(CPCB_1, RES_MIN_4Kx2K, RES_4Kx2K30))
        {
            hdmiVideoFmt = VPP_HDMI_VIDEO_FMT_EXTENDED_RES_FMT;
        }
    }
    if (pHdmiTx->curOutHdmiMode)
    {
        retCode = BE_HDMITX_SetVideoFmtParams(
                        pHdmiTx, VPP_BE_HDMITX_OUT_HDMI, &pHdmiTx->videoParams, hdmiVideoFmt);
    }
    else
    {
        retCode = BE_HDMITX_SetVideoFmtParams(
                        pHdmiTx, VPP_BE_HDMITX_OUT_DVI, &pHdmiTx->videoParams, hdmiVideoFmt);
    }

    if (retCode != MV_VPP_OK)
    {
        // Error in configuration - leave it muted
        VPPHDMI_DBG_ERROR("Error in BE_HDMITX_SetVideoFmtParams \n");
        return retCode;
    }

    /* TODO: Send AVI Info Frame */
    BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_AVI_INFOFRAME,
                          VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC);
#ifdef XVYCC_SUPPORT
    /* clear local copy of xvYCC data to tigger the data sending */
    GaloisMemClear((void *)(&vpp_obj->xvYCC_data), sizeof(xvYCC_DATA));
#endif

    vpp_obj->videofmtUpdated = 1;

#if HDCP_ENABLED
    if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
        pHdmiTx->hdcp2xDSEarlyAuthStarted = false;
        if ((pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x) ||
            (pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x) ||
            (pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_NONE)) {
            HdmiCheckStartDelayedAuth(vpp_obj);
            pHdmiTx->hdcp2xDSEarlyAuthStarted = true;
            HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2, MV_VPP_EVENT_HDCP_RP_REQ_UPSTM_TOGGLE);
            VPPHDMI_DBG_INFO("----hdcp1.4:starting DS authenitcation ---\n");
        } else {
            VPP_BE_UnmuteAndPowerUp(vpp_obj);
            MV_OSAL_Task_Sleep(20);

#if !defined (UPSTREAM_START_ALWAYS)
            if (pHdmiTx->hdcp2xRptrTopology.rxInfo.dev_cnt == 1) {
                /* enable DS auth always for repeater */
                VPPHDMI_DBG_INFO("----hdcp:starting DS authenitcation ---\n");
                HdmiTxRptrDsEnableEncryption (vpp_obj, TRUE);
                //this is placed below HdmiTxRptrDsEnableEncryption to aviod delay in starting auth
                pHdmiTx->hdcp2xDSEarlyAuthStarted = true;
            } else
#endif
            {
                /* repeater authentication trigerred by upstream device, start downstream
                   authentication when request received from upstream. Unmute to enable output
                   till authentication requested upstream. Notify upstream HPD toggle request */
                VPPHDMI_DBG_INFO("----hdcp:authenitcation controlled by Rx. unmute video ---\n");
                HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2, MV_VPP_EVENT_HDCP_RP_REQ_UPSTM_TOGGLE);
                pHdmiTx->hdcp2xDSEarlyAuthStarted = false;
            }
        }
    } else {
        // Un Mute both audio and video out
        HdmiCheckStartDelayedAuth(vpp_obj);
    }
#else
    VPP_BE_UnmuteAndPowerUp(vpp_obj);
#endif
    HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_RES_CHANGE_DONE);
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Config HDMI audio format
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           sampFreq - Sampling frequency
 *           sampSize - Word Size
 *           mClkFactor - 128/256/384/512 times frequency
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_ConfigHdmiAudioFmt(VPP_OBJ *vpp_obj, int enable,
                              VPP_HDMI_AUDIO_CFG *pAudioCfg)
{
    BE_HDMITX_DATA                  *pHdmiTx;
    CEA_AUDIO_SAMP_FREQ             audSampFreq;
    HDMI_AUD_CHN_STS_DATA           audChnSts;
    VPP_HDMI_SPKR_ALLOC             spkrMap;
    int                             mChannelAlloc = 0;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (!enable)
    {
        GaloisMemClear (&pHdmiTx->audFmtParams, sizeof(VPP_BE_HDMITX_AUDIO_FMT_PARAMS));
        return (MV_VPP_OK);
    }

    // Get video resolution
    if (pHdmiTx->videoParams.DispRes == RES_INVALID)
    {
        HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_AUDIO_CFG_ERR);
        return (MV_VPP_EBADPARAM);
    }

    // Check if in DVI mode
    if (pHdmiTx->curOutHdmiMode == FALSE)
    {

        return (MV_VPP_OK);

    }
    GaloisMemClear (&pHdmiTx->audFmtParams, sizeof(VPP_BE_HDMITX_AUDIO_FMT_PARAMS));

    // Map sampling frequency to CEA type
    pHdmiTx->audSampFreq = CEA_AUDIO_SAMP_FREQ_MAX;
    switch (pAudioCfg->sampFreq)
    {
        case 32000: audSampFreq = CEA_AUDIO_SAMP_FREQ_32_KHZ;   break;
        case 44100: audSampFreq = CEA_AUDIO_SAMP_FREQ_44_1_KHZ; break;
        case 48000: audSampFreq = CEA_AUDIO_SAMP_FREQ_48_KHZ;   break;
        case 88200: audSampFreq = CEA_AUDIO_SAMP_FREQ_88_2_KHZ; break;
        case 96000: audSampFreq = CEA_AUDIO_SAMP_FREQ_96_KHZ;   break;
        case 176400:audSampFreq = CEA_AUDIO_SAMP_FREQ_176_4_KHZ;break;
        case 192000:audSampFreq = CEA_AUDIO_SAMP_FREQ_192_KHZ;  break;
        default: return MV_VPP_EBADPARAM;
    }
    if (pAudioCfg->hbrAudio)
        pAudioCfg->sampFreq *= 4;

    // Map reference clock
    switch (pAudioCfg->mClkFactor)
    {
        case 128:
            pHdmiTx->audFmtParams.InputClk = VPP_BE_HDMITX_AUD_INP_CLK_128fs; break;
        case 256:
            pHdmiTx->audFmtParams.InputClk = VPP_BE_HDMITX_AUD_INP_CLK_256fs; break;
        case 384:
            pHdmiTx->audFmtParams.InputClk = VPP_BE_HDMITX_AUD_INP_CLK_384fs; break;
        case 512:
            pHdmiTx->audFmtParams.InputClk = VPP_BE_HDMITX_AUD_INP_CLK_512fs; break;
        case 64:
            pHdmiTx->audFmtParams.InputClk = VPP_BE_HDMITX_AUD_INP_CLK_64fs; break;
        default:
            VPPAPI_DBG_INFO("Wrong clk factor : %d\n",pAudioCfg->mClkFactor);
            return MV_VPP_EBADPARAM;
    }

    // Check number of channels
    if (!((pAudioCfg->numChannels >=2) && (pAudioCfg->numChannels <= 8)))
    {
        return (MV_VPP_EBADPARAM);
    }
    pHdmiTx->audNumChannels = (pAudioCfg->numChannels - 1) & 0x07;

    // Set port selection
#if defined (NEW_HDMI_TX_IP)
    if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_2)
    {
        pHdmiTx->I2SBusParams.I2SParams.PortCfg = (1 << pAudioCfg->portNum) & 0x0F;
    }
    else if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_4 || pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_3)
    {
        pHdmiTx->I2SBusParams.I2SParams.PortCfg = 0x03;
    }
    else if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_5 || pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_6)
    {
        pHdmiTx->I2SBusParams.I2SParams.PortCfg = 0x07;
    }
    else
    {
        pHdmiTx->I2SBusParams.I2SParams.PortCfg = 0x0F;
    }
#else
    if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_2)
    {
        pHdmiTx->I2SBusParams.I2SParams.PortCfg = (1 << pAudioCfg->portNum) & 0x0F;
    }
    else
    {
        pHdmiTx->I2SBusParams.I2SParams.PortCfg = 0x0F;
    }
#endif //NEW_HDMI_TX
    if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_2)
        pHdmiTx->audFmtParams.Layout     = 0;
    else
        pHdmiTx->audFmtParams.Layout     = 1;
    pHdmiTx->audFmtParams.WordLength = pAudioCfg->sampSize;

    // Calculate ACR N value based on video format timing and audio frequency
    if (!VppHdmiGetAcrNVal (audSampFreq,pHdmiTx->videoParams.DispRes,&pHdmiTx->audFmtParams.AcrN))
    {
        HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_AUDIO_CFG_ERR);
        return MV_VPP_EBADPARAM;
    }
    // CTS is set to 0 for the hardware to calculate
    pHdmiTx->audFmtParams.AcrCts     = 0;

    // Configure HBR audio
#if defined (NEW_HDMI_TX_IP)
    if(pAudioCfg->hbrAudio)
    {
    if(pHdmiTx->audFmtParams.Layout)
         pHdmiTx->audFmtParams.HbrAudio=0x03; // HBR audio is recived on 4 I2S interface
    else
        pHdmiTx->audFmtParams.HbrAudio=0x01; // HBR audio is recived on single I2S interface
    }
    else
     pHdmiTx->audFmtParams.HbrAudio   = pAudioCfg->hbrAudio; // if hbr is disabled
#else
    pHdmiTx->audFmtParams.HbrAudio   = pAudioCfg->hbrAudio;
#endif // NEW_HDMI_TX
    pHdmiTx->audFmtParams.AudioFmt   = pAudioCfg->audioFmt;

    if (pHdmiTx->audVUCCfg.cBitsCfg == VPP_HDMI_VUC_CFG_SET_DEFAULT)
    {
        // Channel status info
        GaloisMemClear (&audChnSts, sizeof(HDMI_AUD_CHN_STS_DATA));
        audChnSts.NonPcm     = (pAudioCfg->audioFmt >= CEA_AUDIO_FMT_AC3 && pAudioCfg->audioFmt <= CEA_AUDIO_FMT_WMA_PRO) ? 1 : 0;
        audChnSts.SampFreq   = pAudioCfg->sampFreq;
        audChnSts.WordLength = pAudioCfg->sampSize;
        BE_HDMITX_PackAudioChnSts(pHdmiTx, audChnSts);
    }

    // Configure audio info frame
    GaloisMemClear (&spkrMap, sizeof(VPP_HDMI_SPKR_ALLOC));
#if defined (NEW_HDMI_TX_IP)
    if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_8)
    {
        // Init default speaker map for 8 channel audio (corresponds to 0x1F from spec)
        spkrMap.FlFr = 1;
        spkrMap.Lfe = 1;
        spkrMap.Fc = 1;
        spkrMap.RlRr = 1;
        spkrMap.Rc = 0;
        spkrMap.FlcFrc = 1;
        spkrMap.RlcRrc = 0;
    }
    else if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_7)
    {
        // Init default speaker map for 7 channel audio (corresponds to 0x1D from spec)
        spkrMap.FlFr = 1;
        spkrMap.Lfe = 1;
        spkrMap.Fc = 0;
        spkrMap.RlRr = 1;
        spkrMap.Rc = 0;
        spkrMap.FlcFrc = 1;
        spkrMap.RlcRrc = 0;
    }
    if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_6)
    {
        // Init default speaker map for 6 channel audio (corresponds to 0x0B from spec)
        spkrMap.FlFr = 1;
        spkrMap.Lfe = 1;
        spkrMap.Fc = 1;
        spkrMap.RlRr = 1;
        spkrMap.Rc = 0;
        spkrMap.FlcFrc = 0;
        spkrMap.RlcRrc = 0;
    }
    if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_5)
    {
        // Init default speaker map for 5 channel audio (corresponds to 0x09 from spec)
        spkrMap.FlFr = 1;
        spkrMap.Lfe = 1;
        spkrMap.Fc = 0;
        spkrMap.RlRr = 1;
        spkrMap.Rc = 0;
        spkrMap.FlcFrc = 0;
        spkrMap.RlcRrc = 0;
    }
    if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_4)
    {
        // Init default speaker map for 4 channel audio (corresponds to 0x08 from spec)
        spkrMap.FlFr = 1;
        spkrMap.Lfe = 0;
        spkrMap.Fc = 0;
        spkrMap.RlRr = 1;
        spkrMap.Rc = 0;
        spkrMap.FlcFrc = 0;
        spkrMap.RlcRrc = 0;
    }
    else if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_3)
    {
        // Init default speaker map for 3 channel audio (corresponds to 0x1 from spec)
        spkrMap.FlFr = 1;
        spkrMap.Lfe = 1;
        spkrMap.Fc = 0;
        spkrMap.RlRr = 0;
        spkrMap.Rc = 0;
        spkrMap.FlcFrc = 0;
        spkrMap.RlcRrc = 0;
    }
    else
    {
        // For all other cases, init default map to enable only FL/FR
        spkrMap.FlFr = 1;
    }
#else
    if (pHdmiTx->audNumChannels == CEA_AUDIO_CHNLS_8)
    {
        // Init default speaker map for 8 channel audio (corresponds to 0x1F from spec)
        spkrMap.FlFr = 1;
        spkrMap.Lfe = 1;
        spkrMap.Fc = 1;
        spkrMap.RlRr = 1;
        spkrMap.Rc = 0;
        spkrMap.FlcFrc = 1;
        spkrMap.RlcRrc = 0;
    }
    else
    {
        // For all other cases, init default map to enable only FL/FR
        spkrMap.FlFr = 1;
    }
#endif //NEW_HDMI_TX
    pHdmiTx->audSampFreq = audSampFreq;
    HdmiTxCalcSpkrChnMap (&spkrMap, (unsigned char *)&mChannelAlloc);
    /* this converts the spkr map to channel map required for audio infoframe.
       refer : pg 79 if cea 861 : E
       for 2 channel flfr=1 mChannelAlloc = 0;
    */
    BE_HDMITX_ConfigHdmiAudioFmt(pHdmiTx, mChannelAlloc, 0, 0);
    return (MV_VPP_OK);
}


int VPP_BE_EnableHdmiAudioFmt(VPP_OBJ *vpp_obj, int enable)
{

    BE_HDMITX_DATA                  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if(enable) {
        BE_HDMITX_EnableAudioFmt(pHdmiTx, TRUE);
    } else {
        BE_HDMITX_EnableAudioFmt(pHdmiTx, FALSE);
    }
    return (MV_VPP_OK);
}
/*******************************************************************
 * FUNCTION: set HDMI audio format
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           enable   - Enable/Disable audio
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdmiAudioFmt(VPP_OBJ *vpp_obj, int enable)
{
    BE_HDMITX_DATA                  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (!enable)
    {
        // Stop Audio info frame and disable audio
        VPP_BE_SetHdmiMute(vpp_obj, VPP_HDMI_AV_PORT_AUDIO, TRUE);
        BE_HDMITX_DisableHostPkt (pHdmiTx, VPP_HDMI_PKT_ID_AUDIO_INFOFRAME,
                                      VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC);
        BE_HDMITX_EnableAudioPkt(pHdmiTx, FALSE);
        return (MV_VPP_OK);
    }

    // Get video resolution
    if (pHdmiTx->videoParams.DispRes == RES_INVALID)
    {
        return (MV_VPP_EBADPARAM);
    }

    VPP_BE_SetHdmiMute(vpp_obj, VPP_HDMI_AV_PORT_AUDIO, TRUE);
    BE_HDMITX_DisableHostPkt (pHdmiTx, VPP_HDMI_PKT_ID_AUDIO_INFOFRAME,
                                      VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC);
    BE_HDMITX_EnableAudioPkt(pHdmiTx, FALSE);

    // Set audio format parameters
    BE_HDMITX_SetAudioFmtParams(pHdmiTx, VPP_BE_HDMITX_AUDIO_I2S, &pHdmiTx->audFmtParams);

    // Load VUC
    VPP_BE_SetHdmiAudioVUCBits(vpp_obj,
                              ((1 << HDMITX_AUDIO_VBIT) | (1 << HDMITX_AUDIO_UBIT) |
                               (1 << HDMITX_AUDIO_CBIT)));

    BE_HDMITX_EnableAudioPkt(pHdmiTx, TRUE);

    // Send Audio info frame
    BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_AUDIO_INFOFRAME,
                          VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC);

    // Unmute audio
    VPP_BE_SetHdmiMute(vpp_obj, VPP_HDMI_AV_PORT_AUDIO, FALSE);
    return (MV_VPP_OK);
}

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
                                  int loadOnAudioCfg, int *pVUCUpdateReqd)
{
    /* AOUT uses this */
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Set Audio VUC data
 * PARAMS:   *vpp_obj         - pointer to VPP object
 *           vucUpdateSetting - audio VUC bit update setting
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdmiAudioVUCBits(VPP_OBJ *vpp_obj, int vucUpdateSetting)
{
    /* AOUT uses this */
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: mute Hdmi video/audio
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           video - video/audio to be muted
 *           mute - 1: mute 0:un-mute
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdmiMute (VPP_OBJ *vpp_obj,  VPP_HDMI_AV_PORT port, int muteEn)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    switch (port) {
    case VPP_HDMI_AV_PORT_VIDEO:
        BE_HDMITX_SetVideoMute(pHdmiTx, muteEn);
        break;

    case VPP_HDMI_AV_PORT_AUDIO:
        BE_HDMITX_SetAudioMute(pHdmiTx, muteEn);
        break;

    case VPP_HDMI_AV_PORT_GCP:
        BE_HDMITX_SendAVMute (pHdmiTx, muteEn);
        break;

    case VPP_HDMI_AV_PORT_ALL:
    default:
        if (muteEn) {
            BE_HDMITX_SendAVMute (pHdmiTx, muteEn);  /* Send AVMute to sink, sink decides
                                                        display should be muted or not */
            BE_HDMITX_SetVideoMute(pHdmiTx, muteEn); /* Forces fix color output */
            BE_HDMITX_SetAudioMute(pHdmiTx, muteEn); /* Forces flat signal for audio */
        } else {
            BE_HDMITX_SetVideoMute(pHdmiTx, muteEn);
            BE_HDMITX_SetAudioMute(pHdmiTx, muteEn);
            BE_HDMITX_SendAVMute (pHdmiTx, muteEn);
        }
        break;
    }
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set Hdmi output mode (Hdmi/Dvi)
 * INPUT: hdmiMode - 0:DVI, 1:HDMI
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SetHdmiMode(VPP_OBJ *vpp_obj, int hdmiMode)
{
    /* TODO: add code if required */
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set HDMI Video Information
 * INPUT: aspect ratio
 *      : scan info
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SetHdmiVideoInfo(VPP_OBJ *vpp_obj, int aspRatio, int scanInfo)
{
    BE_HDMITX_DATA  *pHdmiTx;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx->videoParams.DispRes == RES_INVALID)
    {
        // Hdmi Video path is not configured
        return (MV_VPP_EUNCONFIG);
    }
    HdmiTxConfigAVIInfoFrame(vpp_obj, aspRatio, scanInfo,
                     HDMI_USE_EXISTING /* Use existing calorimtery information */,
                     FALSE);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Set HDMI Audio Information
 * INPUT: Speaker Mapping
 *      : Level Shift Value
 *      : Down Mix Inhibit Flag
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_SetHdmiAudioInfo(VPP_OBJ *vpp_obj, int spkrAlloc,
                            int lvlShiftVal, int downMixInhFlag)
{
    BE_HDMITX_DATA          *pHdmiTx;
    int                     mChannelAlloc = 0;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if ((pHdmiTx->videoParams.DispRes == RES_INVALID) ||
        (pHdmiTx->audSampFreq == CEA_AUDIO_SAMP_FREQ_MAX))
    {
        // Hdmi Video path/ audio path is not configured
        return (MV_VPP_EUNCONFIG);
    }

    HdmiTxCalcSpkrChnMap ((VPP_HDMI_SPKR_ALLOC*)&spkrAlloc, (unsigned char *)&mChannelAlloc);
    BE_HDMITX_ConfigHdmiAudioFmt(pHdmiTx, mChannelAlloc, lvlShiftVal, downMixInhFlag);
    return (MV_VPP_OK);
}
/********************************************************************************
 * FUNCTION: Register for HDCP event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdcpRegisterCB(VPP_OBJ *vpp_obj, UINT32 eventCode,
        void *cbFnAddr, void *contextParam)
{
    BE_HDMITX_DATA          *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    pHdmiTx->eventCode2 = eventCode;
    pHdmiTx->appCbFn2 = (MV_VPP_EVENT_CALLBACK)cbFnAddr;
    pHdmiTx->contextParam2 = (void*)contextParam;
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Unregister for HDCP event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdcpUnregisterCB(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA          *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    pHdmiTx->eventCode2 = 0;
    pHdmiTx->appCbFn2 = NULL;
    pHdmiTx->contextParam2 = NULL;

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Register for HDMI event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiRegisterCB(VPP_OBJ *vpp_obj, UINT32 eventCode,
                          MV_VPP_EVENT_CALLBACK cbFnAddr, void* contextParam)
{
    BE_HDMITX_DATA          *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    pHdmiTx->eventCode = eventCode;
    pHdmiTx->appCbFn = (MV_VPP_EVENT_CALLBACK)cbFnAddr;
    pHdmiTx->contextParam = (void*)contextParam;
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Unregister for HDMI event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiUnregisterCB(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA          *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    pHdmiTx->eventCode = 0;
    pHdmiTx->appCbFn = NULL;
    pHdmiTx->contextParam = NULL;

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Format HDMI Auxillary Packet
 * INPUT: enable    - To handle packet enable/disable
 *      : pktType   - Packet type
 *      : pAuxPkt   - Pointer to aux packet data (Input)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiFormatAuxPkt(VPP_OBJ *vpp_obj, BOOL enable,
                            VPP_HDMI_PKT_ID pktType,
                            VPP_HDMI_PKT *pAuxPkt)
{
    BE_HDMITX_DATA          *pHdmiTx;
    UINT8                   dataBuf[31];
    BOOL                    retCode = FALSE;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    if (enable)
    {
        GaloisMemClear (dataBuf, 31);
        switch (pktType)
        {
            case VPP_HDMI_PKT_ID_ACP:
                retCode = BE_HDMITX_SetACPPacket (pHdmiTx, &pAuxPkt->acpPkt);
                break;

            case VPP_HDMI_PKT_ID_ISRC1:
                retCode = BE_HDMITX_SetISRC1Packet (pHdmiTx, &pAuxPkt->isrc1Pkt);
                break;

            case VPP_HDMI_PKT_ID_ISRC2:
                retCode = BE_HDMITX_SetISRC2Packet (pHdmiTx, &pAuxPkt->isrc2Pkt);
                break;

            case VPP_HDMI_PKT_ID_VENDOR_INFOFRAME:
                retCode = VppPackVndrSpecInfoFrame(vpp_obj, &pAuxPkt->vndrSpecInfoFrm, dataBuf);
                CHECK_IF_RET_STATUS(retCode);
                retCode = BE_HDMITX_SetVndrSpecInfoFrame(pHdmiTx, &pAuxPkt->vndrSpecInfoFrm, dataBuf);
                break;

            case VPP_HDMI_PKT_ID_SPD_INFOFRAME:
                retCode = BE_HDMITX_SetSPDInfoFrame (pHdmiTx, &pAuxPkt->spdInfoFrm);
                break;

            case VPP_HDMI_PKT_ID_MPEG_SRC_INFOFRAME:
                //retCode = VppPackMpegSrcInfoFrame (&pAuxPkt->mpegSrcInfoFrm, dataBuf);
                break;

            case VPP_HDMI_PKT_ID_GAMUT_METADATA:
                retCode = BE_HDMITX_SetGMDPacket (pHdmiTx, &pAuxPkt->gmdPkt);
                if (retCode == MV_VPP_OK)
                {
                    // Update AVI Infoframe
                    HdmiTxConfigAVIInfoFrame(vpp_obj, HDMI_USE_EXISTING,
                                             HDMI_USE_EXISTING, TRUE, FALSE);
                }
                break;

            case VPP_HDMI_PKT_ID_HDR_INFOFRAME:
                retCode = VppPackHDRInfoframe(vpp_obj, &pAuxPkt->hdrInfoFrm, dataBuf);
                CHECK_IF_RET_STATUS(retCode);
                retCode = BE_HDMITX_SetDRMpacket (pHdmiTx, dataBuf);
            break;

            default:
                return (MV_VPP_EBADPARAM);
        }

        if (retCode != MV_VPP_OK)
            return (MV_VPP_EBADPARAM);
    }
    else
    {
        // While packet is disabled, only gamut meta data holds significance
        if (pktType == VPP_HDMI_PKT_ID_GAMUT_METADATA)
        {
            // Update AVI Infoframe
            HdmiTxConfigAVIInfoFrame(vpp_obj, HDMI_USE_EXISTING,
                                             HDMI_USE_EXISTING, FALSE, FALSE);
        }
    }
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Send HDMI Auxillary Packet
 * INPUT: pktType    - Packet type
 *      : repeatPkt  - Transmit packet once/repeat every VSync
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiSendAuxPkt(VPP_OBJ *vpp_obj, VPP_HDMI_PKT_ID pktType,
                          BOOL repeatPkt)
{
    BE_HDMITX_DATA          *pHdmiTx;
    VPP_BE_HDMITX_HOST_PKT_TX_MODE txMode;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    txMode = (repeatPkt == 0)?
                VPP_BE_HDMITX_HOST_PKT_TX_ONCE:VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC;

    // Start transfer of aux packet
    if (pktType == VPP_HDMI_PKT_ID_GAMUT_METADATA)
    {
        // If it is gamut metadata packet - update AVI infoframe with
        // extended calorimetry information
        BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_AVI_INFOFRAME, VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC);
    }
    BE_HDMITX_SendHostPkt(pHdmiTx, pktType, txMode);
    if (repeatPkt == FALSE)
    {
        // Set frame interrupt trigger to sync HW and SW status of host packet
        // control
        VPP_BE_SET_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_HOST_PKT_FRMTRIG);
    }
    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Stop HDMI Auxillary Packet
 * INPUT: pktType   - Packet type
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_HdmiStopAuxPkt(VPP_OBJ *vpp_obj, VPP_HDMI_PKT_ID pktType)
{
    BE_HDMITX_DATA          *pHdmiTx;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    if (pktType == VPP_HDMI_PKT_ID_GAMUT_METADATA)
    {
        // If it is gamut metadata packet - update AVI infoframe
        BE_HDMITX_SendHostPkt(pHdmiTx, VPP_HDMI_PKT_ID_AVI_INFOFRAME,
                                VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC);
    }
    BE_HDMITX_DisableHostPkt (pHdmiTx, pktType, VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC);
    return (MV_VPP_OK);
}

#ifdef CONFIG_VPP_DEV_KEY_SUPPORT
static int HdmiTxHandleSRMDevSign (VPP_OBJ *vpp_obj, VPP_HDCP_SRM_DATA *pSrmData, VPP_DEV_SIGN_DATA *pDevSignData)
{
#ifdef SYNOPSYS_HDMI_TO_BE_MAPPED
#if HDCP2X_ENABLED
#if defined(CONFIG_VPP_DEV_KEY_SUPPORT)
    BE_HDMITX_DATA  *pHdmiTx;
    UINT8           *signBuf = NULL;
    UINT32          signBufLen;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    signBuf = VPP_Malloc(MV_VPP_DEV_SIGNATURE_SIZE);
    VPP_Memset (signBuf, 0, MV_VPP_DEV_SIGNATURE_SIZE);
    if (pHdmiTx->devKeyValid) {
        // Sign content
        if (BE_HDCP2X_SignHMacSHA256 (vpp_obj,
                pSrmData->pBuf, pSrmData->bufLen,
                pHdmiTx->pDevKeyAddr, VPP_DEVKEY_SIZE,
                &signBuf, &signBufLen) != HDCP2X_ERR_OK) {
            VPPHDMI_DBG_ERROR("Failed to sign srm data with device key\n");
            if(signBuf)
                VPP_Free(signBuf);
            return MV_VPP_EBADPARAM;
        }
        if (pDevSignData->signSts == VPP_DEV_SIGN_NONE) {
            VPP_Memcpy (pDevSignData->signature, signBuf, signBufLen);
            pDevSignData->signSts = VPP_DEV_SIGN_VALID;
            VPPHDMI_DBG_INFO("Device signature applied\n");
        }
        else if (pDevSignData->signSts == VPP_DEV_SIGN_UNVERIFIED) {
            if (memcmp (pDevSignData->signature, signBuf, signBufLen) == 0) {
                VPPHDMI_DBG_INFO("Device signature verified\n");
                pDevSignData->signSts = VPP_DEV_SIGN_VALID;
            }
            else {
                VPPHDMI_DBG_ERROR("Device signature verification failed\n");
                pDevSignData->signSts = VPP_DEV_SIGN_INVALID;
            }
        }
    }
    if(signBuf)
        VPP_Free(signBuf);
#else
    pDevSignData->signSts = VPP_DEV_SIGN_NONE;
#endif
#endif
#endif //SYNOPSYS_HDMI_TO_BE_MAPPED
    return MV_VPP_OK;
}
#endif
/********************************************************************************
 * FUNCTION: Configure SRM data buffer
 * INPUT: pSrmData - Pointer to SRM data and length (Input)
 *      : pDevSignData - Pointer to device signature to check content
 *      :              - validity (Input)
 *      : pVernNum - Pointer to return SRM info parsed and updated (output)
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_ConfigureSRM(VPP_OBJ *vpp_obj, VPP_HDCP_SRM_DATA *pSrmData,
                        VPP_DEV_SIGN_DATA *pDevSignData, VPP_HDCP_SRM_INFO *pSrmInfo)
{
    int                     ret = MV_VPP_OK;
    BE_HDMITX_DATA          *pHdmiTx;
    HDMITX_HDCP_SRM_DATA    *pBESrmData;
    HDCP_RETURNS            hdcpRetCode;
    UINT8                   srmId;
    BOOL                    srmUpdated = FALSE;
    BOOL                    srmParseSts = FALSE;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    VPP_Memset (pSrmInfo, 0, sizeof(VPP_HDCP_SRM_INFO));
    pSrmInfo->valid = FALSE;

#ifdef CONFIG_VPP_DEV_KEY_SUPPORT
    // Device Signature Handling
    if ((HdmiTxHandleSRMDevSign(vpp_obj, pSrmData, pDevSignData) != MV_VPP_OK) ||
        (pDevSignData->signSts != VPP_DEV_SIGN_VALID)) {
        VPPHDMI_DBG_ERROR("Device Signature failed\n");
        return MV_VPP_EBADPARAM;
    }
#endif

    // SRM Data Handling
    srmId = (pSrmData->pBuf[0] & 0xF0) >> 4;
    pSrmInfo->errorInfo = MV_VPP_HDCP_SRM_ERROR_NONE;
    if (srmId == MV_VPP_HDCP_SRM_ID_1X) {
        VPPHDMI_DBG_INFO("HDCP SRM 1.x file\n");
        hdcpRetCode = BE_HDCP_ConfigureSRM (vpp_obj, pSrmData->pBuf, pSrmData->bufLen,
                                            pSrmData->bForceUpdate, &srmUpdated,
                                            &pSrmInfo->errorInfo);
        srmParseSts = (hdcpRetCode == HDCP_ERR_OK)?TRUE:FALSE;
        pBESrmData = &pHdmiTx->hdcpSrm[HDMITX_HDCP_SRM_TYPE_1X];
    }
#if HDCP2X_ENABLED
    /* to be ported to HDCP2X TA */
#endif
    else {
        pSrmInfo->errorInfo = MV_VPP_HDCP_SRM_ERROR_INVALID_DATA;
        ret = MV_VPP_EBADPARAM;
    }
    VPPHDMI_DBG_INFO("errorInfo = %u.\n", pSrmInfo->errorInfo);

    if (srmParseSts == TRUE) {
        pSrmInfo->updated = srmUpdated;
        pSrmInfo->valid = TRUE;
        pSrmInfo->ID = pBESrmData->id;
        pSrmInfo->version = pBESrmData->version;
        pSrmInfo->generation = pBESrmData->generation;
        VPPHDMI_DBG_INFO("SRM parsed ID: [%d] version [%d] generation [%d]\n",
                        pSrmInfo->ID, pSrmInfo->version, pSrmInfo->generation);
    }
    return (ret);
}

/********************************************************************************
 * FUNCTION: Get SRM Version
 * INPUT: pHdcp22SrmVersion - HDCP 2.2 SRM File version
 *      : pHdcp14SrmVersion - HDCP 1.4 SRM File version
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_GetSRMVersion(VPP_OBJ *vpp_obj, UINT16 *pHdcp22SrmVersion, UINT16 *pHdcp14SrmVersion)
{
    BE_HDMITX_DATA          *pHdmiTx;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL) {
        return MV_VPP_EBADPARAM;
    }

    *pHdcp22SrmVersion = 0;
    *pHdcp14SrmVersion = 0;

    *pHdcp14SrmVersion = pHdmiTx->hdcpSrm[HDMITX_HDCP_SRM_TYPE_1X].version;

#if HDCP2X_ENABLED
    *pHdcp22SrmVersion = pHdmiTx->hdcpSrm[HDMITX_HDCP_SRM_TYPE_2X].version;
#endif
    VPPHDMI_DBG_INFO("SRM Version: HDCP 2.2[%d] HDCP 1.4[%d]\n",
                      *pHdcp22SrmVersion, *pHdcp14SrmVersion);
    return MV_VPP_OK;
}

/********************************************************************************
 * FUNCTION: Enable/Disable SRM Check
 * INPUT   : enable - Enable/Disable flag for SRM check control
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_EnableSRMCheck(VPP_OBJ *vpp_obj, int enable)
{
    BE_HDMITX_DATA          *pHdmiTx;

    VPPHDMI_DBG_INFO("EnableSRMCheck [%d]\n", enable);
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL) {
        return MV_VPP_EBADPARAM;
    }

    pHdmiTx->enableSrmCheck = (BOOL)enable;
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Enable Encryption on HDMI
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           Enable   - enable/disable encryption
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_EnableHdcp(VPP_OBJ *vpp_obj, BOOL Enable)
{
#if HDCP_ENABLED
/* Call this api from MV_VPPOBJ_EnableHDCP only. For internel purpose, use HdmiTxEnableEncryption */
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if(Enable == pHdmiTx->authRequired) {
        VPPHDMI_DBG_INFO("HDCP authentication state already in %d.. ignoring attempt to reset\n", Enable);
        return (MV_VPP_OK);
    }

    if(Enable == FALSE) {
        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
        DelayInMS(0, FRAME_DELAY);    /* make sure sink receives avmute */
        HdmiTxEnableEncryption (vpp_obj, FALSE);
        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, FALSE);
        pHdmiTx->authRequired = FALSE;
    } else {
        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
        HdmiTxEnableEncryption (vpp_obj, TRUE);
        pHdmiTx->authRequired = TRUE;
    }
#endif
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Get HDCP State on HDMI
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pHDCPState   - Pointer return state
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_GetHdcpState(VPP_OBJ *vpp_obj, int *pHDCPState)
{
    HDCP_ENGAGE_STATE current_hdcp_state = HDCP_STATE_INACTIVE;
    BE_HDMITX_DATA  *pHdmiTx;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if ((pHDCPState == NULL) || (pHdmiTx ==NULL))
        return (MV_VPP_EBADPARAM);

    current_hdcp_state = BE_HDMITX_GetActiveHdcpState(pHdmiTx);

    /* map to legacy states */
    switch(current_hdcp_state) {
    case HDCP_STATE_1_4_AUTHENTICATED:
        *pHDCPState = HDCP_STATE_AUTH_DONE;
    break;

    case HDCP_STATE_1_4_IN_PROGRESS:
        *pHDCPState = HDCP_STATE_AUTH_AKSV_TRIG;
    break;
    case HDCP_STATE_1_4_ERR_REAUTH:
        *pHDCPState = HDCP_STATE_AUTH_ERR;
    break;

#if HDCP2X_ENABLED
    case HDCP_STATE_2_2_AUTHENTICATED_RX:
    case HDCP_STATE_2_2_AUTHENTICATED_RPTR:
        /*TODO: check if we can query repeater/receiver state from ESM. If repeater can be known, add mapping for same*/
        *pHDCPState = ((HDCP2X_TX_STATE_KS&0xFF) << 16) | ((HDCP2X_TX_SUB_STATE_SKE_SEND_EKS&0xFF) << 8) | (HDCP2X_TX_AUTH_DONE&0xFF);
    break;

    case HDCP_STATE_2_2_IN_PROGRESS:
    case HDCP_STATE_2_2_ERR_REAUTH:
        *pHDCPState = ((HDCP2X_TX_STATE_AKE&0xFF) << 16) | ((HDCP2X_TX_SUB_STATE_AKE_INIT&0xFF) << 8) | (HDCP2X_TX_AUTH_IDLE & 0xFF);
    break;
#endif

    default:
        *pHDCPState = HDCP_STATE_AUTH_IDLE;
    break;
    }

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Get HDCP Version
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pHDCPVersion - Pointer return Version
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_GetHdcpVersion(VPP_OBJ *vpp_obj, int *pHDCPVersion)
{
    /* this api will be depricated soon, use VPP_BE_GetCurrentHdcpVersion for getting active HDCP version */
    return VPP_BE_GetCurrentHdcpVersion(vpp_obj, (VPP_HDMI_HDCP_VERSION *)pHDCPVersion);
}

/*******************************************************************
 * FUNCTION: Get Current HDCP Version Negotiated
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pHDCPVersion - Pointer return Version
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_GetCurrentHdcpVersion(VPP_OBJ *vpp_obj, VPP_HDMI_HDCP_VERSION *pHDCPVersion)
{
    BE_HDMITX_DATA  *pHdmiTx;
    HDCP_ENGAGE_STATE current_hdcp_state = HDCP_STATE_INACTIVE;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pHDCPVersion == NULL || pHdmiTx == NULL) {
        return (MV_VPP_EBADPARAM);
    }

    *pHDCPVersion = VPP_HDMI_HDCP_VER_NONE;

    if(!pHdmiTx->rxConnected) {
        return MV_VPP_ENOSINKCNCTED;
    }

    if(!pHdmiTx->authRequired) {
        /* return success with version set as None if HDCP is disabled*/
        return MV_VPP_OK;
    }

    if(pHdmiTx->sm_state < HDCP_SM_STATE_REAUTH) {
        return MV_VPP_ENODEV;
    }

    current_hdcp_state = BE_HDMITX_GetActiveHdcpState(pHdmiTx);
    if (current_hdcp_state == HDCP_STATE_1_4_AUTHENTICATED) {
        *pHDCPVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
    }
#if HDCP2X_ENABLED
    else if (current_hdcp_state == HDCP_STATE_2_2_AUTHENTICATED_RX) {
        *pHDCPVersion = VPP_HDMI_HDCP_VER_HDCP_2_3;
    } else if (current_hdcp_state == HDCP_STATE_2_2_AUTHENTICATED_RPTR) {
        /* restrict the effective hdcp version as lowest present in the topology */
        if(pHdmiTx->hdcp2xRptrTopology.rxInfo.downstream_1x_dev) {
            *pHDCPVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
        } else {
            *pHDCPVersion = VPP_HDMI_HDCP_VER_HDCP_2_3;
        }
    }
#endif
    else {
        *pHDCPVersion = VPP_HDMI_HDCP_VER_NONE;
    }

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Get Current HDCP capacity of the Sink
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pHDCPVersion - Pointer return Version
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_GetSinkHdcpcapability(VPP_OBJ *vpp_obj, int *pHDCPVersion)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pHDCPVersion == NULL || pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    /*Intilize the Version to HDCP_NONE*/
    *pHDCPVersion = VPP_HDMI_HDCP_VER_NONE;

    if(!pHdmiTx->rxConnected)
    {
        return MV_VPP_ENOSINKCNCTED;
    }

    *pHDCPVersion = pHdmiTx->SinkMaxSuprtedhdcpVersion;

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Set HDCP Version
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           iHDCPVersion - Set the Hdcp Version
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdcpVersion(VPP_OBJ *vpp_obj, int iHDCPVersion)
{
    /* application selects hdcp version to be enabled. The version selected by application
     * is retained within the driver across resolution changes. If application selects any hdcp version
     * not supported by sink, the output will remain muted and error notification will be send.
     */
#if HDCP_ENABLED
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPPHDMI_DBG_INFO("Enabling HDCP version - %s\n",  (iHDCPVersion == VPP_HDMI_HDCP_VER_AUTO)? "AUTO Selection":
                                                       (iHDCPVersion == VPP_HDMI_HDCP_VER_HDCP_2_2)? "HDCP2.2":
                                                       (iHDCPVersion == VPP_HDMI_HDCP_VER_HDCP_2_3)? "HDCP2.3":
                                                       (iHDCPVersion == VPP_HDMI_HDCP_VER_HDCP_1_x)? "HDCP1.4":
                                                       (iHDCPVersion == VPP_HDMI_HDCP_VER_NONE)? "None": "invalid") ;

    VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
    DelayInMS(0, FRAME_DELAY);    /* make sure sink receives avmute */

    HdmiTxEnableEncryption (vpp_obj, FALSE);

    if (iHDCPVersion == VPP_HDMI_HDCP_VER_HDCP_2_3)
        iHDCPVersion = VPP_HDMI_HDCP_VER_HDCP_2_2;

    pHdmiTx->requestedHdcpVersion = iHDCPVersion;

    if(iHDCPVersion == VPP_HDMI_HDCP_VER_NONE) {
        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, FALSE);
        pHdmiTx->authRequired = FALSE;
    } else {
        HdmiTxEnableEncryption (vpp_obj, TRUE);
        pHdmiTx->authRequired = TRUE;
    }
    VPP_Memset(&pHdmiTx->hdcpErrEvent,0, sizeof(HDMITX_HDCP_ERR_EVENT));
#endif
    return MV_VPP_OK;
}

/*******************************************************************
 * FUNCTION: Set Downstream HDCP Version in case of repeater mode
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           iHDCPVersion - Set the Hdcp Version
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetRptrDsHdcpVersion(VPP_OBJ *vpp_obj, int iHDCPVersion)
{
#if HDCP2X_ENABLED
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_TXRX) {
        VPPHDMI_DBG_ERROR("downstream version switch permitted only in repeater mode\n");
        return MV_VPP_EUNSUPPORT;
    }

    VPPHDMI_DBG_INFO("Setting downstream HDCP version as %s\n",  (iHDCPVersion == VPP_HDMI_HDCP_VER_AUTO)? "AUTO Selection":
                                                       (iHDCPVersion == VPP_HDMI_HDCP_VER_HDCP_2_2)? "HDCP2.2":
                                                       (iHDCPVersion == VPP_HDMI_HDCP_VER_HDCP_1_x)? "HDCP1.4":
                                                       (iHDCPVersion == VPP_HDMI_HDCP_VER_NONE)? "None": "invalid") ;

    /* TODO: confirm if content blocking required from Tx side when repeater auth in progress.
             if downstream authentication fails, upstream will fail and content protection
             falls under upstream content provider
    */

    /* TCL 55C815 requires avmute before transitioning to 4K60*/
    VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
    DelayInMS(0, FRAME_DELAY);    /* make sure sink receives avmute */

    HdmiTxRptrDsEnableEncryption(vpp_obj, FALSE);

    pHdmiTx->requestedHdcpVersion = iHDCPVersion;

    if(iHDCPVersion == VPP_HDMI_HDCP_VER_NONE) {
        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, FALSE);
    } else {
        HdmiTxRptrDsEnableEncryption (vpp_obj, TRUE);
    }
    pHdmiTx->authRequired = TRUE;
    VPP_Memset(&pHdmiTx->hdcpErrEvent,0, sizeof(HDMITX_HDCP_ERR_EVENT));
    return MV_VPP_OK;
#else
    return (MV_VPP_EUNSUPPORT);
#endif
}

/*******************************************************************
 * FUNCTION: Get HDCP Ksv (KSV+BKSV) on HDMI
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pHDCPKsv   - Pointer return Key
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_BE_GetHdcpKsv(VPP_OBJ *vpp_obj, VPP_HDCP_KSV_DATA *pHDCPKsv)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
    {
        // Not initialized - Do nothing
        return (MV_VPP_EUNCONFIG);
    }
    return BE_HDMITX_GetHdcpKsv(pHdmiTx, pHDCPKsv);
}

/*******************************************************************
 * FUNCTION: Set HDCP repeater mode
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           Mode   - HDCP mode (0 for Transimitter, 1 for Repeater)
 * RETURN: MV_VPP_OK
 ******************************************************************/

int VPP_BE_SetHdcpRepeaterMode(VPP_OBJ *vpp_obj, UINT32 mode)
{
#if defined (HDCP_REPEATER_MODE)
    BE_HDMITX_DATA  *pHdmiTx;
    INT retVal;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL) {
        // Not initialized - Do nothing
        return (MV_VPP_EUNCONFIG);
    }

    VPPHDMI_DBG_INFO("RepeaterMode = %d, Mode = %d\n", pHdmiTx->activeHdcpMode, mode);
    if (pHdmiTx->activeHdcpMode == mode) {
        VPPHDMI_DBG_INFO("skip resetting to same mode..\n");
        return MV_VPP_OK;
    }

    VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
    DelayInMS(0, FRAME_DELAY);
    /*set mode in HDCP TA first */
    retVal = BE_HDMITX_HDCP2XSetOpMode(pHdmiTx, mode);
    /*TODO: check if it can be recovered to earlier version if fallback*/
    if ((retVal != MV_VPP_OK) && (retVal != MV_VPP_EHDCPNOOP)) {
        VPPHDMI_DBG_INFO("error updating mode in hdcp2xta... skipping mode switch %d\n", mode);
        return retVal;
    }

    /* Load HDCP1.4 RX keys since it get erased after clock reset */
    VPP_HDCP_KEYLOAD_LoadKey(vpp_obj, VPP_HDCP_KEYSTORE_HDCPRX);

    pHdmiTx->activeHdcpMode = mode;
    if (mode == VPP_HDMI_HDCP_MODE_TXRX) {
        pHdmiTx->planeControlOnHdcpFail = pHdmiTx->hdmiConfig.mutePlaneOnHdcpFailure;
        pHdmiTx->encBypassOnHdcpFail = (pHdmiTx->hdmiConfig.mutePlaneOnHdcpFailure)?true:false;
        /*reset HDCP to auto as in repeater mode upstream can request HDCP disable downstream*/
        pHdmiTx->requestedHdcpVersion = VPP_HDMI_HDCP_VER_AUTO;
        /*TODO: set HDCP version to override possible HDCP version set by upstream on repeater mode.
                simulate by triggering error downstream, disable HDCP upstream, then switch HDCP mode */
    } else {
        pHdmiTx->planeControlOnHdcpFail = false;
        pHdmiTx->encBypassOnHdcpFail = true;
    }
    BE_HDMITX_EnableHdcpFrameProt(pHdmiTx, true); //reset the current state
    VPP_BE_HdmiTxForceHotplug(vpp_obj);
    return (MV_VPP_OK);
#else
    return (MV_VPP_EUNSUPPORT);
#endif
}

/*******************************************************************
 * FUNCTION: Returns HPD status
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pHpdStatus - Pointer returned status
 * RETURN: MV_VPP_OK
 *         MV_EBADPARAM - invalid parameters
 ******************************************************************/
int VPP_BE_GetHPDStatus (VPP_OBJ *vpp_obj, int *pHpdStatus)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pHpdStatus == NULL)
        return (MV_VPP_EBADPARAM);

    if (pHdmiTx->hdmiHpdControlOff) {
        *pHpdStatus = pHdmiTx->rxConnected;
         return MV_VPP_OK;
    }
#ifdef HDMI_SRV_SUPPORT
    *pHpdStatus = pHdmiTx->rxConnected;
#else
    /*read actual hpd state from core*/
    BE_HDMITX_GetHPDStatus(pHdmiTx, (BOOL *)pHpdStatus);
    /* when HDMI_SRV_SUPPORT is active, thread will be running to process HPD and read edid
       here, sync the HPD state when HPD is polled
     */
    VPPHDMI_DBG_INFO("check for  pending HPD state change: %d/%d\n",
            pHdmiTx->sinkConnChangeCount, pHdmiTx->polledSinkConnChangeCount);
    if (pHdmiTx->sinkConnChangeCount != pHdmiTx->polledSinkConnChangeCount) {
        VPP_BE_ProcessHPDStateChange (vpp_obj);
        if (pHdmiTx->polledSinkConnChangeCount == 0) {
            /* first time interrupt enable is setting the bit, clear it explicitely */
            BE_HDMITX_ClearSinkDetectionInterrupts(pHdmiTx);
        }
        pHdmiTx->polledSinkConnChangeCount = pHdmiTx->sinkConnChangeCount;
    }
#endif
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Returns essential sink capbilities
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pSinkFeatureMap - Pointer to sink capabilities bitmask
 *                        to return hdmi connected sink info
 * RETURN: MV_VPP_OK
 *         MV_VPP_ENOSINKCNCTED - No Sink Connected
 ******************************************************************/
int VPP_BE_GetSinkFeatureMap (VPP_OBJ *vpp_obj, VPP_HDMI_SINKCAP_BITMASK *pSinkFeatureMap)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    int i, fps60 = 0, fhd = 0;
    INT *pSinkCapsMask = (INT *)pSinkFeatureMap;

    *pSinkCapsMask = 0x00;

    if (pHdmiTx->edidInfo.validEdid == false)
        return MV_VPP_ENOSINKCNCTED;

    if (pHdmiTx->edidInfo.hdmiMode)
        *pSinkCapsMask |= (1<<VPP_HDMI_SINKCAP_BITMASK_HDMIMODE);

    if (is4K5060HdmiVicSupportedForResIndex(pHdmiTx))
        *pSinkCapsMask |= (1<<VPP_HDMI_SINKCAP_BITMASK_FULL4K);

    if (is4K30HdmiVicSupportedForResIndex(pHdmiTx))
        *pSinkCapsMask |= (1<<VPP_HDMI_SINKCAP_BITMASK_4K30);

    for (i = 0; i < pHdmiTx->edidInfo.resCnt; i++)  {
        if ((pHdmiTx->edidInfo.resInfo[i].refreshRate == 60) ||
            (pHdmiTx->edidInfo.prefResInfo.refreshRate == 30))
            fps60 = true;
        if ((pHdmiTx->edidInfo.resInfo[i].hActive == 1920) &&
            (pHdmiTx->edidInfo.resInfo[i].vActive == 1080))
            fhd = true;
    }

    if (!fps60)
        *pSinkCapsMask |= (1<<VPP_HDMI_SINKCAP_BITMASK_PREF50FPS);

    if (fhd)
        *pSinkCapsMask |= (1<<VPP_HDMI_SINKCAP_BITMASK_FHD);

    return MV_VPP_OK;
}

/*******************************************************************
 * FUNCTION: Returns capabilities of Hdmi sink connected
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pSinkCaps - Pointer to sink capabilities structure
 *                        to return hdmi sink info
 * RETURN: MV_VPP_OK
 *         MV_EBADPARAM - invalid parameters
 ******************************************************************/
int VPP_BE_GetSinkCaps (VPP_OBJ *vpp_obj, VPP_HDMI_SINK_CAPS *pSinkCaps)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pSinkCaps == NULL)
        return (MV_VPP_EBADPARAM);

    if (!pHdmiTx->edidInfo.validEdid)
        return MV_VPP_ENOSINKCNCTED;

    GaloisMemcpy (pSinkCaps, &pHdmiTx->edidInfo, sizeof(VPP_HDMI_SINK_CAPS));

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Returns Raw Edid of Hdmi sink connected
 * PARAMS:   *vpp_obj - pointer to VPP object
 *           *pRawEdid - Pointer to Raw Edid data
 *                        to return hdmi raw edid info
 * RETURN: MV_VPP_OK
 *         MV_EBADPARAM - invalid parameters
 ******************************************************************/
int VPP_BE_GetRawEdid (VPP_OBJ *vpp_obj, VPP_HDMI_RAW_EDID *pRawEdid)
{

    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    UINT8 Total_Edid_Blocks = 0;

    if (pRawEdid == NULL)
        return (MV_VPP_EBADPARAM);

    if(pHdmiTx->pEdidArray == NULL){
        VPPHDMI_DBG_INFO("Edid Array empty\n");
        return (MV_VPP_EBADPARAM);
    }

    pRawEdid->IsValid = pHdmiTx->edidInfo.validEdid;
    Total_Edid_Blocks = pHdmiTx->Extblks + 1;
    if (Total_Edid_Blocks > MAX_SUPPORTED_EDID_BLOCKS) {
        //Current Max supported Edid blocks is 1 basic + 7 extended blocks = 8
        VPPHDMI_DBG_INFO("Printing of more than 8 Edid Blocks is currently not supported\n");
        Total_Edid_Blocks = MAX_SUPPORTED_EDID_BLOCKS;
    }
    VPP_Memcpy (pRawEdid->DataBuf, pHdmiTx->pEdidArray,VPP_SIZE_OF_EDID_BLK * Total_Edid_Blocks);
    VPPHDMI_DBG_INFO("[%d] blocks of edid copied\n",Total_Edid_Blocks);
    pRawEdid->Extblks = pHdmiTx->Extblks;

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: ISR Service to service HDCP interrupts
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
void VPP_BE_HDCP_ISR_service(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    HDMITX_CMD      cmd;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pHdmiTx != NULL)
    {
        cmd.cmdID = HDMITX_CMD_HDCP_INTERRUPT;
        cmd.param = 0;
        HdmiFifoPush (&pHdmiTx->hdcpCmdFifo, (UINT8*)&cmd, FALSE);
    }
}

/*******************************************************************
 * FUNCTION: ISR Service to service HPD interrupt
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
void VPP_BE_HPD_ISR_service(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

#ifdef HDMI_SRV_SUPPORT
    HDMITX_CMD      cmd;
    BOOL            hpd;

    if (pHdmiTx != NULL)
    {
        BE_HDMITX_ClearSinkDetectionInterrupts(pHdmiTx);
        if(pHdmiTx->phyStateFromApp == FALSE)
        {
            VPPHDMI_DBG_INFO("HDMITX phy in standby. Ignoring HPD interrupts!!!\n");
            return;
        }
        BE_HDMITX_GetHPDStatus (pHdmiTx, &hpd);

        if (hpd != pHdmiTx->rxConnected)
        {
            VPPHDMI_DBG_INFO("..............HPD=%d...............\n", hpd);

            cmd.cmdID = HDMITX_CMD_HPD;
            cmd.param = hpd;
            HdmiFifoPush (&pHdmiTx->hpdcmdFifo, (UINT8*)&cmd, FALSE);
        }
        else {
            VPPHDMI_DBG_INFO("..............HPD=%d rxConnected=%d...............\n", hpd, pHdmiTx->rxConnected);
        }
    }
#else
    BE_HDMITX_ClearSinkDetectionInterrupts(pHdmiTx);
    pHdmiTx->sinkConnChangeCount++;
#endif
}


void VPP_BE_ProcessEarcHPD(VPP_OBJ *vpp_obj, int vhpd)
{
    BE_HDMITX_DATA  *pHdmiTx;

    if((vpp_obj == NULL) || (vpp_obj->be.hdmiTx == NULL)) {
        VPPHDMI_DBG_ERROR("null pointer error.. avoid earc hpd\n");
        return;
    }

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPP_BE_HPD_ISR_service(vpp_obj);
    HdmiFifoSignal(&pHdmiTx->hpdcmdFifo);
}

/******************************************************************************
 * FUNCTION : Deferred service routine for HPD/HDCP interrupt to signal the
 *          : waiting task on the interrupt
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
void VPP_BE_HDMI_DSR_service(VPP_OBJ *vpp_obj)
{
#ifdef HDMI_SRV_SUPPORT
    BE_HDMITX_DATA  *pHdmiTx;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (vpp_obj->status == STATUS_ACTIVE && pHdmiTx != NULL){
        HdmiFifoSignal(&pHdmiTx->hpdcmdFifo);
        HdmiFifoSignal(&pHdmiTx->hdcpCmdFifo);
    }
#endif
}

/******************************************************************************
 * FUNCTION : CPCB frame interrupt for HDMI to be used as a counter for
 *          : starting HDCP
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
void VPP_BE_HDMIProcessFrameInt(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    HDMITX_CMD      cmd;
    UINT8           temp;
    UINT8           regValue[2];

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (vpp_obj->status == STATUS_ACTIVE && pHdmiTx != NULL)
    {
        if (VPP_BE_GET_BIT(vpp_obj->be.hdmiFrmIntrTrig, VPP_BE_HDMI_HDCP_FRMTRIG))
        {
            EDDC_Read(pHdmiTx, EDDC_SLAVE_HDCP, HDMI_HDCP_BSTATUS_READ_PORT, VPP_HDMI_BSTATUS_SIZE, &regValue[0]);
            VPPHDMI_DBG_INFO("hdcp start countdown [%d] [0x%x:0x%x]\n", pHdmiTx->frameCntr, regValue[0], regValue[1]);
            pHdmiTx->frameCntr--;

            if (pHdmiTx->frameCntr < VPP_HDMI_VBI_DELAY_PHY_POWER_UP) {
                /* Note: any other frame dependent triggering should happen within
                 *      (VPP_HDMI_VBI_DELAY_PHY_POWER_UP-VPP_HDMI_MODE_STABLE_CHECK_COUNT)
                 *      else, could get skipped. update the check point if it needs to be modified
                 */
                if (regValue[1] & VPP_HDMI_BSTATUS_MASK_HDMI_MODE) {
                    pHdmiTx->hdmiLockCheck = (pHdmiTx->hdmiLockCheck > 0) ? (pHdmiTx->hdmiLockCheck-1) : -1;
                } else {
                    pHdmiTx->hdmiLockCheck = VPP_HDMI_MODE_STABLE_CHECK_COUNT;
                }
            }

            if (pHdmiTx->frameCntr == VPP_HDMI_VBI_DELAY_PHY_POWER_UP) {
                /* if TMDS enabled before VBI interrupt start, for resolutions like 1080i, distortion
                   seen for couple frames. Wait till the pipeline programming is done to enable TMDS */
                VPPHDMI_DBG_INFO("power up phy... \n");
                BE_HDMITX_ConfigureScrambler(pHdmiTx);
                BE_HDMITX_PhyPowerUp(pHdmiTx);
                pHdmiTx->hdmiLockCheck = VPP_HDMI_MODE_STABLE_CHECK_COUNT;
            } else if (pHdmiTx->frameCntr == VPP_HDMI_VBI_DELAY_CONFIG_AVUNMUTE) {
                VPPHDMI_DBG_INFO("av-unmte.. \n");
                VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_GCP, FALSE);
            } else if (pHdmiTx->frameCntr == VPP_HDMI_VBI_DELAY_CONFIG_AUDIO) {
                /* Configure audio after powering up phy */
                VPPHDMI_DBG_INFO("Configure audio\n");
                cmd.cmdID = HDMITX_CMD_AUDIO_CONFIG;
                cmd.param = 0;
                HdmiFifoPush (&pHdmiTx->hdcpCmdFifo, (UINT8*)&cmd, TRUE);
            } else if ((pHdmiTx->frameCntr == 0) || (pHdmiTx->hdmiLockCheck == 0))
            {
                VPPHDMI_DBG_INFO("%d:%d - selected hdcp version: %d Max session hdcp version : %d \n",
                        pHdmiTx->frameCntr, pHdmiTx->hdmiLockCheck,
                        pHdmiTx->requestedHdcpVersion, pHdmiTx->SessionMaxSuprtedhdcpVersion);

                pHdmiTx->hdcp2xErrCnt = 0;
                if(pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_NONE) {
                    cmd.cmdID = HDMITX_CMD_VERSION_RD;  /* No valid hdcp version read yet. read again from hdmitx task */
                } else if(pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x) {
                    cmd.cmdID = HDMITX_CMD_HDCP14_START;
                }
                else if((pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_AUTO) &&
                          (pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2)) {
                    cmd.cmdID = HDMITX_CMD_HDCP2X_START;
                }
                else if(pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) {
                    if(pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) {
                        cmd.cmdID = HDMITX_CMD_HDCP2X_START;
                    } else {
                        /* enable fallback to HDCP1.4 if HDCP2x requested and sink doesn't support */
                        cmd.cmdID = HDMITX_CMD_HDCP14_START;
                    }
                }
                else if((pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_AUTO)) {
                    cmd.cmdID = HDMITX_CMD_HDCP14_START;
                }

                cmd.param = 0;
                HdmiFifoPush (&pHdmiTx->hdcpCmdFifo, (UINT8*)&cmd, FALSE);
                VPP_BE_CLR_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_HDCP_FRMTRIG);
                pHdmiTx->frameCntr = -1;
                pHdmiTx->hdmiLockCheck = -1;

                // Trigger for fifo signalling
                VPP_BE_SET_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_SIG_FIFO_FRMTRIG);
            }
            else if (pHdmiTx->frameCntr < 0)
            {
                VPP_BE_CLR_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_HDCP_FRMTRIG);
                pHdmiTx->frameCntr = -1;
            }
        }

        if (VPP_BE_GET_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_HOST_PKT_FRMTRIG))
        {
            // Synchronize Host packet control
            BE_HDMITX_SyncHostPktTxOnceStatus(pHdmiTx, &temp);
            if (temp == 0x00)
                VPP_BE_CLR_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_HOST_PKT_FRMTRIG);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////LOCAL STATIC FUNCTIONS/////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
/*******************************************************************
 * FUNCTION: Reset Hdmi Tx components
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int HdmiTxReset (VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTxData;
    INT rxConnected;

    /* Reset data store */
    pHdmiTxData = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    pHdmiTxData->videoParams.DispRes = RES_INVALID;
    pHdmiTxData->videoParams.VideoFmt = OUTPUT_COLOR_FMT_INVALID;
    pHdmiTxData->videoParams.ColorDepth = OUTPUT_BIT_DEPTH_INVALID;
    pHdmiTxData->videoParams.PixelRpt = VPP_BE_HDMITX_PIXEL_RPT_NONE;
    pHdmiTxData->audSampFreq = CEA_AUDIO_SAMP_FREQ_MAX;
    pHdmiTxData->frameCntr = -1;
    pHdmiTxData->hdmi_outformat = VPP_OUTFORMAT_SDR;
    pHdmiTxData->hdmiEarlySignalCount = 0;
    pHdmiTxData->activeHdcpMode = VPP_HDMI_HDCP_MODE_TXRX;
    pHdmiTxData->planeControlOnHdcpFail = true;
    pHdmiTxData->encBypassOnHdcpFail = false;
    pHdmiTxData->hdmiConfig.enableRxsenseCheck = false;
    pHdmiTxData->hdmiConfig.mutePlaneOnHdcpFailure = false;
    pHdmiTxData->hdmiConfig.restrictUhdInHdcp14 = false;
    pHdmiTxData->hdmiConfig.enableDynFPSSwitch = false;
    pHdmiTxData->hdmiConfig.hdcpFallbackDwnstrm1_4 = false;
    pHdmiTxData->hdmiConfig.hdcpFallbackOn2xAuthFailure = true;
    pHdmiTxData->hdmiConfig.errNotifRetryCnt1x = HDCP_NOTIF_RETRY_CNT;
    pHdmiTxData->hdmiConfig.errNotifRetryCnt2x = HDCP_NOTIF_RETRY_CNT;
    HdmiFifoReset (&pHdmiTxData->hdcpCmdFifo);
    HdmiTxTimerCmdCfg (vpp_obj, 0);

    BE_HDMITX_reset(pHdmiTxData);

    /* Reset Hdmi Tx    */
    //BE_HDMITX_LoadDefaultVal(pHdmiTxData);  //TODO:

#ifndef VPP_DIAG
    DelayInMS(0, 10); // delay 10ms to allow HDMI PHY power stable
#endif

    /*Initialize the hardware EDDC Master with standard Speed*/
    EDDC_InitializeMaster(pHdmiTxData, 0);

#ifdef HDMI_SRV_SUPPORT
    MV_Timer_Start (&pHdmiTxData->hpdTimer);
    // Read HPD to check if receiver is already connected
    BE_HDMITX_GetHPDStatus (pHdmiTxData, (BOOL *)&rxConnected);
    BE_HDMITX_ClearSinkDetectionInterrupts(pHdmiTxData);
    BE_HDMITX_UpdateHPDStateChange(pHdmiTxData, rxConnected);
#else
    if(pHdmiTxData->hdmiHpdControlOff) {
        /* This will be used at the time of fastlogo and recovery mode bootup flow */
        rxConnected = TRUE;
        pHdmiTxData->rxConnected = TRUE;
        pHdmiTxData->curOutHdmiMode = TRUE; // HDMI Mode
        BE_HDMITX_UpdateHPDStateChange(pHdmiTxData, rxConnected);
    } else {
        // increment sinkConnChangeCount to handle bootup HDMI connection state
        pHdmiTxData->sinkConnChangeCount++;
        VPP_BE_GetHPDStatus(vpp_obj, &rxConnected);
    }
#if defined VPP_HDMI_FORCE_CONFIGURE_WITHOUT_SINK
    pHdmiTxData->curOutHdmiMode = true;
#endif
#endif //HDMI_SRV_SUPPORT
    return (MV_VPP_OK);
}

void VPP_BE_ProcessHPDStateChange(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTxData;
    BOOL rxConnected = false;
    pHdmiTxData = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    BE_HDMITX_ClearSinkDetectionInterrupts(pHdmiTxData);
    BE_HDMITX_GetHPDStatus (pHdmiTxData, &rxConnected);

#ifndef HDMI_SRV_SUPPORT
    if (rxConnected == false) {
        printf("hdmi disconnected\n");
        BE_HDMITX_PowerOffTMDS(pHdmiTxData, 1);
    } else {
        printf("hdmi connected\n");
        pHdmiTxData->rxConnected = HdmiTxHandleHotPlug (vpp_obj, rxConnected);
        if (BE_HDMITX_ConfigureScrambler(pHdmiTxData)) {
            MV_OSAL_Task_Sleep(5);
            BE_HDMITX_PowerOffTMDS(pHdmiTxData, 0);
            MV_OSAL_Task_Sleep(FRAME_DELAY);
            VPP_BE_SetHdmiAudioFmt(vpp_obj, true);
        }
    }
    BE_HDMITX_UpdateHPDStateChange(pHdmiTxData, rxConnected);
    printf("hdmi connection handled, interrupt enabled. \n");
#else
    pHdmiTxData->rxConnected = HdmiTxHandleHotPlug (vpp_obj, rxConnected);
    VPPHDMI_DBG_INFO(" handled hdmi connection state %d : effective state %d\n",
            rxConnected, pHdmiTxData->rxConnected);
    BE_HDMITX_UpdateHPDStateChange(pHdmiTxData, pHdmiTxData->rxConnected);
#endif
}

/*******************************************************************
 * FUNCTION: Initialize Hdmi Tx - allocate memory for data storage
 *         : and create required OS components
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int HdmiTxInit (VPP_OBJ *vpp_obj)
{

    BE_HDMITX_DATA  *pHdmiTxData;
#if defined(HDMI_SRV_SUPPORT) || defined(EARC_RX_ENABLED)
    HRESULT         retVal;
#endif

    // Allocate memory for data store
    VPP_OSAL_SHM_Dynamic_Malloc(VPP_SHM_SECURE, sizeof(BE_HDMITX_DATA), 32, (VPP_SHM_HANDLE*)&pHdmiTxData);

    if (pHdmiTxData == NULL)
    {
        return MV_VPP_ENOMEM;
    }

    GaloisMemClear (pHdmiTxData, sizeof(BE_HDMITX_DATA));

    pHdmiTxData->curOutHdmiMode = TRUE;
    pHdmiTxData->phyStateFromApp = TRUE;

    /* initialize only during init since config gets updated only on bootup */
    pHdmiTxData->hdmiConfig.enableRxsenseCheck = false;
    pHdmiTxData->hdmiConfig.mutePlaneOnHdcpFailure = false;
    pHdmiTxData->hdmiConfig.isUpdated = false;

    /* set the variables with one time set and values to be retained here */
#if HDCP_ENABLED
    pHdmiTxData->requestedHdcpVersion = VPP_HDMI_HDCP_VER_AUTO;
    pHdmiTxData->authRequired = TRUE;
#if HDCP2X_ENABLED
    pHdmiTxData->SourceMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_2_2;
#else
    pHdmiTxData->SourceMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
#endif

#else
    pHdmiTxData->requestedHdcpVersion = VPP_HDMI_HDCP_VER_NONE;
    pHdmiTxData->authRequired = FALSE;
#endif

#ifndef HDMI_SRV_SUPPORT   //override auth status for fastlogo
    pHdmiTxData->authRequired = FALSE;
#endif

    pHdmiTxData->sm_state = HDCP_SM_STATE_IDLE;
    pHdmiTxData->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_NONE;

    // Save the address in vpp_obj
    vpp_obj->be.hdmiTx = pHdmiTxData;

    // Create task, queue
    if (!HdmiFifoCreate (HDMITX_MAX_FIFO_SIZE, sizeof(HDMITX_CMD), &pHdmiTxData->hdcpCmdFifo))
    {
        HdmiTxTerm (vpp_obj);
        return MV_VPP_ENOMEM;
    }
    // Create task, queue
    if (!HdmiFifoCreate (HDMITX_MAX_FIFO_SIZE, sizeof(HDMITX_CMD), &pHdmiTxData->hpdcmdFifo))
    {
        HdmiTxTerm (vpp_obj);
        return MV_VPP_ENOMEM;
    }

#ifdef HDMI_SRV_SUPPORT
    vpp_obj->be.hdmiHdcpTaskExit = FALSE;
    vpp_obj->be.hdmihpdTaskExit = FALSE;
#ifndef CONFIG_AMP_TEE_ENABLE
    retVal = MV_OSAL_Task_Create_Detailed(&pHdmiTxData->hdcpTskHandle,
                       HdmiTxHdcpTask,
                       (void*)(vpp_obj),
                       MV_OSAL_TASK_PRIORITY_12,
                       "HdmiTx Task",
                       NULL,
                       0,
                       0);
    retVal = MV_OSAL_Task_Create_Detailed(&pHdmiTxData->hpdtskHandle,
            HdmiTxHpdTask,
            (void*)(vpp_obj),
            MV_OSAL_TASK_PRIORITY_12,
            "HdmiTx HpdTask",
            NULL,
            0,
            0);
#else
    retVal = VPP_CreateHdmiTask(); //do a callback to client to create task FIXME: proper location??
    if (retVal != MV_VPP_OK)
    {
        HdmiTxTerm (vpp_obj);
        return MV_VPP_ENOMEM;
    }

    retVal = VPP_CreateHdmiHpdTask();
#endif

    if (retVal != MV_VPP_OK)
    {
        HdmiTxTerm (vpp_obj);
        return MV_VPP_ENOMEM;
    }
#ifdef TZ_3_0
    retVal = MV_Timer_Create (&pHdmiTxData->hpdTimer, HDMITX_TIMER_CONFIG, HdmiTxHdcpTimerCb, (void*)(g_vpp));
#else
    retVal = MV_Timer_Create (&pHdmiTxData->hpdTimer, HDMITX_TIMER_CONFIG, HdmiTxHdcpTimerCb, (void*)(vpp_obj));
#endif
    if (retVal != MV_VPP_OK)
    {
        HdmiTxTerm (vpp_obj);
        return MV_VPP_ENOMEM;
    }

#ifdef SYNOPSYS_HDMI_TO_BE_MAPPED   /* TODO: error fix ion_alloc error on FPGA */
#ifdef CONFIG_VPP_DEV_KEY_SUPPORT
    VPP_DEVKEY_AllocMem (&pHdmiTxData->devKeyHandle, &pHdmiTxData->pDevKeyAddr);
    if (pHdmiTxData->pDevKeyAddr != NULL) {
        pHdmiTxData->devKeyValid = FALSE;
        if (VPP_DEVKEY_Generate (pHdmiTxData->pDevKeyAddr) == VPP_DEVKEY_SIZE) {
            pHdmiTxData->devKeyValid = TRUE;
        }
    }
#endif
#endif
#endif //HDMI_SRV_SUPPORT

#ifdef EARC_RX_ENABLED
    VPPHDMI_DBG_INFO("Initializing eARC-Rx...\n");
    retVal = BE_EARCRX_Init(vpp_obj);
    if(retVal) {
        VPPHDMI_DBG_ERROR("error initializing earc module... disable earc rx support : 0x%x\n", retVal);
        pHdmiTxData->earcContext.initialized = false;
        return MV_VPP_OK;  //return with earc feature disabled
    } else {
        VPPHDMI_DBG_ERROR("earc-rx module initialized.. \n", retVal);
        pHdmiTxData->earcContext.initialized = true;
    }

#ifdef TZ_3_0
    retVal = MV_Timer_Create (&pHdmiTxData->earcContext.timer, EARC_TIMER_CONFIG, BE_EARCRX_TimerCb, (void*)(g_vpp));
#else
    retVal = MV_Timer_Create (&pHdmiTxData->earcContext.timer, EARC_TIMER_CONFIG, BE_EARCRX_TimerCb, (void*)(vpp_obj));
#endif
    if (retVal != MV_VPP_OK) {
        VPPHDMI_DBG_ERROR("error creating timer for earc\n");
        pHdmiTxData->earcContext.initialized = false;
        return MV_VPP_OK;
    }

    /* set initial earc mode */
    VPP_BE_eARCSetMode(vpp_obj, EARC_MODE_NONE);
#endif /* EARC_RX_ENABLED */

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Terminate Hdmi Tx - Cleanup all created components
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdmiTxTerm (VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTxData;
    int             i;

    if (vpp_obj->be.hdmiTx == 0)
    {
        // Not initialized - Do nothing
        return (MV_VPP_EUNCONFIG);
    }

    pHdmiTxData = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    //Release EDID memory
    if (pHdmiTxData->pEdidArray) {
        VPP_Free(pHdmiTxData->pEdidArray);
    }

    BE_HDMITX_StopHdcpAuthentication(pHdmiTxData);
    // Release SRM memory
    for (i = 0; i < MAX_HDMITX_HDCP_SRM_TYPE; i++) {
        if (pHdmiTxData->hdcpSrm[i].valid && pHdmiTxData->hdcpSrm[i].revocList != NULL) {
            VPP_Free (pHdmiTxData->hdcpSrm[i].revocList);
            pHdmiTxData->hdcpSrm[i].revocList = NULL;
        }
    }

#ifdef HDMI_SRV_SUPPORT
    // Delete Semaphores, Task
    if (pHdmiTxData->hpdTimer != NULL)
    {
        MV_Timer_Destroy (&pHdmiTxData->hpdTimer);
        pHdmiTxData->hpdTimer = NULL;
    }

    vpp_obj->be.hdmiHdcpTaskExit = TRUE;
    vpp_obj->be.hdmihpdTaskExit = TRUE;
#ifndef CONFIG_AMP_TEE_ENABLE
    if (pHdmiTxData->hpdtskHandle != NULL)
    {
        MV_OSAL_Task_Join(&pHdmiTxData->hpdtskHandle);
        pHdmiTxData->hpdtskHandle = NULL;
    }
    if (pHdmiTxData->hdcpTskHandle != NULL)
    {
        vpp_obj->be.hdmiHdcpTaskExit = TRUE;
        MV_OSAL_Task_Join(&pHdmiTxData->hdcpTskHandle);
        pHdmiTxData->hdcpTskHandle = NULL;
    }
#else
    VPP_DeleteHdmiHpdTask();
    VPP_DeleteHdmiTask();

#ifdef EARC_RX_ENABLED
    if(pHdmiTxData->earcContext.initialized == true) {
        MV_Timer_Destroy(&pHdmiTxData->earcContext.timer);
    }
#endif //EARC_RX_ENABLED

#endif
#endif //HDMI_SRV_SUPPORT

    HdmiFifoDelete (&pHdmiTxData->hpdcmdFifo);
    HdmiFifoDelete (&pHdmiTxData->hdcpCmdFifo);

    BE_HDMITX_HDCP2XDeInitCtx(pHdmiTxData);

    BE_HDMITX_Deinit(pHdmiTxData);
    // Release Memory
    VPP_Free (pHdmiTxData);

    //Clean up EDDC
    Destroy_EDDC(vpp_obj);

#ifdef DISPLAY_SERVICE_PORTING_ISSUE //FIXME: causing ion allocation failed
#ifdef CONFIG_VPP_DEV_KEY_SUPPORT
    // Dummy call for now - osal needs to be changed
    VPP_DEVKEY_FreeMem (pHdmiTxData->devKeyHandle);
#endif
#endif

    // Clean up in vpp_obj
    vpp_obj->be.hdmiTx = 0;

    return (MV_VPP_OK);
}

static int HdmiTxLoadFailSafeEdid(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    GaloisMemClear(&pHdmiTx->edidInfo, sizeof(pHdmiTx->edidInfo));

    pHdmiTx->edidInfo.hdmiMode  = TRUE;
    pHdmiTx->edidInfo.sprtedDispRes = (1<<RES_525I60)   |
                                      (1<<RES_525I5994) |
                                      (1<<RES_625I50)   |
                                      (1<<RES_525P60)   |
                                      (1<<RES_525P5994) |
                                      (1<<RES_625P50)   |
                                      (1<<RES_720P60)   |
                                      (1<<RES_720P5994) |
                                      (1<<RES_720P50);

     pHdmiTx->edidInfo.YCbCr444Sprt = 1;
     pHdmiTx->edidInfo.YCbCr422Sprt = 1;
     pHdmiTx->edidInfo.DC30bitSprt = 0;
     pHdmiTx->edidInfo.DC36bitSprt = 0;
     pHdmiTx->edidInfo.YCbCr444AtDC= 0;
     pHdmiTx->edidInfo.maxTMDSClock= 0;
     pHdmiTx->edidInfo.AISprt = 1;

     pHdmiTx->edidInfo.prInfo[0].resMask = (1 << RES_NTSC_M) |
                                           (1 << RES_NTSC_J) |
                                           (1 << RES_PAL_M)  |
                                           (1 << RES_525I60) |
                                           (1 << RES_525I5994);
     pHdmiTx->edidInfo.prInfo[1].resMask = (1 << RES_PAL_BGH) |
                                           (1 << RES_625I50);
     pHdmiTx->edidInfo.prInfo[2].resMask = (1 << RES_525P60) |
                                           (1 << RES_525P5994);
     pHdmiTx->edidInfo.prInfo[3].resMask = (1 << RES_625P50);
     pHdmiTx->edidInfo.prInfo[0].prSupport = (1 << 2) | (1 << 4);
     pHdmiTx->edidInfo.prInfo[1].prSupport = (1 << 2) | (1 << 4);
     pHdmiTx->edidInfo.prInfo[2].prSupport = (1 << 1) | (1 << 2) | (1 << 4);
     pHdmiTx->edidInfo.prInfo[3].prSupport = (1 << 1) | (1 << 2) | (1 << 4);
     pHdmiTx->edidInfo.calInfo.xvYCC601 = 1;
     pHdmiTx->edidInfo.calInfo.xvYCC709 = 1;

     pHdmiTx->edidInfo.cecPhyAddr = 0xFFFF;

     pHdmiTx->edidInfo.audioInfo[0].audioFmt = VPP_HDMI_AUDIO_FMT_PCM;
     pHdmiTx->edidInfo.audioInfo[0].maxNumChnls = 2;
     pHdmiTx->edidInfo.audioInfo[0].wdLenSprt.WdLen16 = 1;
     pHdmiTx->edidInfo.audioInfo[0].freqSprt.Fs32KHz = 1;
     pHdmiTx->edidInfo.audioInfo[0].freqSprt.Fs44_1KHz = 1;
     pHdmiTx->edidInfo.audioInfo[0].freqSprt.Fs48KHz = 1;

     pHdmiTx->edidInfo.spkrAlloc.FlFr = 1;

     return MV_VPP_OK;
}

static bool HdmiTxCheckSinkInList(UINT8 *sinkList, UINT8 sinkCount, UINT8 *mnfrName, UINT8 *prodId)
{
    UINT8 index = 0;
    UINT8 compRes = 0;
    UINT8 *sinkProdIdTag;

    for (index = 0; index < sinkCount; index++) {
        sinkProdIdTag = sinkList+(index*HDMI_SINK_ID_SIZE);
        compRes = memcmp(sinkProdIdTag, mnfrName, 3);
        if (compRes != 0){
            continue;
        }
        compRes = memcmp(sinkProdIdTag+3, prodId, 4);
        if (compRes != 0){
            continue;
        }
        VPPHDMI_DBG_INFO("===> matching device model found:%s\n", sinkProdIdTag);
        return true;
    }
    return false;
}

static void HdmiTxSinkSpecificConfig(VPP_OBJ *vpp_obj)
{
    bool isModelConnected = false;
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);;
    HDMI_SINK_BLACKLIST  *pSinkSpecificConfig = &pHdmiTx->sinkSpecificConfig;

    if(pSinkSpecificConfig->bl_option_present) {
        VPPHDMI_DBG_INFO("check for Tv model specific configs..\n");
    } else {
        VPPHDMI_DBG_INFO(" no sink specific check loaded\n");
        return;
    }

    pSinkSpecificConfig->fix_59_94_active = false;
    if(pSinkSpecificConfig->count_59_94_fix) {
        isModelConnected = HdmiTxCheckSinkInList(&pSinkSpecificConfig->prod_id_59_94_fix[0][0],
                              pSinkSpecificConfig->count_59_94_fix,
                              pHdmiTx->edidInfo.mnfrName,
                              pHdmiTx->edidInfo.productCode);
        if(isModelConnected) {
            VPPHDMI_DBG_INFO("connected sink have issues with 4K59.94, so restrict to 4K60\n");
            pSinkSpecificConfig->fix_59_94_active = true;
        }
    }

    if(pSinkSpecificConfig->count_59_94_dyn) {
        isModelConnected = HdmiTxCheckSinkInList(&pSinkSpecificConfig->prod_id_59_94_dyn[0][0],
                              pSinkSpecificConfig->count_59_94_dyn,
                              pHdmiTx->edidInfo.mnfrName,
                              pHdmiTx->edidInfo.productCode);
        if(isModelConnected) {
            VPPHDMI_DBG_INFO("connected sink have detected issues with 4K59.94, so restrict to 4K60\n");
            pSinkSpecificConfig->fix_59_94_active = true;
        }
    }
}

/*******************************************************************
 * FUNCTION: Handle hot plug event
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   TRUE-> connected, FALSE->disconnected
 ******************************************************************/
int HdmiTxHandleHotPlug (VPP_OBJ *vpp_obj, BOOL rxConnected)
{
    BE_HDMITX_DATA  *pHdmiTx;
    int             retVal;
    int retries;
    BOOL curHpdStatus;
#ifdef EDID_RAWDATA_DUMP
    int loopa, loopb;
    char edidArray[400];
    char *pArray = edidArray;
#endif
    UINT32  Addr = SCDC_OFFSET_TMDS_CONFIG;
    UINT16  Length = 1;
    UINT8   DataBuf;
#ifdef EARC_RX_ENABLED
    BOOL    hdmiHpdState;
#endif
    SCDC_TX_DATA scdcTxdata;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    int rxsen = -1;
    UINT8 CsArray[8];
    UINT8 *pCsArray = CsArray;
    UINT8 start_edid = 1;
    UINT32 eventCode;

#ifdef HDMI_SRV_SUPPORT
    /* polled ampless case, re-read without checking rxConnected */
    if(rxConnected == pHdmiTx->rxConnected) {
        VPPHDMI_DBG_INFO("request for same hpd state %d, ignoring \n", rxConnected);
        return rxConnected;
    }
#endif

    pHdmiTx->edidInfo.validEdid = FALSE;
    if (rxConnected)
    {
        VPPHDMI_DBG_INFO(".... HPD is high ....\n");

        //Reset EDR/HDR status
        vpp_obj->HDR_enable = FALSE;
        vpp_obj->HLG_enable = FALSE;
        vpp_obj->HDR10PLUS_enable = FALSE;


        // Receiver plugged in
        pHdmiTx->videoParams.DispRes = RES_INVALID;
        pHdmiTx->audSampFreq = CEA_AUDIO_SAMP_FREQ_MAX;
        pHdmiTx->frameCntr   = -1;

        pHdmiTx->hdmi_outformat = VPP_OUTFORMAT_SDR;
        pHdmiTx->hdmiEarlySignalCount = 0;
        // Read EDID and parse it
        // Don't read edid again for same sink resume
        BE_HDMITX_GetRxSense(pHdmiTx,&rxsen);
        if(pHdmiTx->resumeInProgress == 1)
        {
            start_edid = 0;
            VPP_Memset(pCsArray, 0, sizeof(CsArray));
            VPPHDMI_DBG_INFO("EDID is not being read during resume\n");
            BE_HDMITX_ReadChecksum(pHdmiTx,pHdmiTx->Extblks,pCsArray);
            for(int i = 0;i <= pHdmiTx->Extblks;i++){
                if(CsArray[i] != pHdmiTx->EdidCurrentchecksum[i]){
                    start_edid = 1;
                    VPPHDMI_DBG_INFO("checksum not equal\n");
                    break;
                }
            }
            if(rxsen == 1){
                pHdmiTx->resumeInProgress = 0;
            }
         }
        if(start_edid == 1)
        {
            retries = 20;
            BE_HDMITX_I2CBusClear(pHdmiTx);
#ifdef EARC_RX_ENABLED
            if(pHdmiTx->earcContext.initialized == true) {
                BE_EARCRX_SetHdmiConnState(pHdmiTx, true);
            }
#endif
            while (retries)
            {
                BE_HDMITX_GetHPDStatus (pHdmiTx, &curHpdStatus);
                if (!curHpdStatus )
                {
                    VPPHDMI_DBG_INFO("HPD is pulled down !!!\n");
                    return FALSE;
                }
                retVal = HdmiTxGetEdidInfo (vpp_obj, &pHdmiTx->edidInfo);
                if (retVal == MV_VPP_OK) {
                    VPPHDMI_DBG_INFO("EDID read successfully \n");
#ifdef EDID_RAWDATA_DUMP
                    VPPHDMI_DBG_INFO("Printing edid values with [%d] extended blocks\n",pHdmiTx->Extblks);
                    for (loopa=0; loopa <= pHdmiTx->Extblks;loopa++) {
                        for( loopb=0; loopb < VPP_SIZE_OF_EDID_BLK ;loopb++) {
                            sprintf(pArray, "%2x,", pHdmiTx->pEdidArray[loopb+(loopa*(VPP_SIZE_OF_EDID_BLK))]);
                            pArray = pArray+3;
                        }
                        VPPHDMI_DBG_INFO("Raw Edid of block [%d]\n",loopa);
                        VPPHDMI_DBG_INFO("%s\n", edidArray);
                        pArray = edidArray;
                    }
#endif
#ifdef EARC_RX_ENABLED
                    if(pHdmiTx->earcContext.initialized == true) {
                        BE_EARCRX_CheckClockRecEnable(pHdmiTx);
                        BE_EARCRX_ResetMode(pHdmiTx);
                    }
#endif

                    HdmiGetRxHdcpVersion(vpp_obj);

                    //Update the Source Version of TX in SCDC
                    if(pHdmiTx->edidInfo.CeaHdmiVSDB.HfvsdbFeatures.SCDC_Present) {
                        //Until HF VDB 2.0 is implemented by all sink devices we will check if SCDC present by checking scdc version
                        scdcTxdata.SinkVersion = 0x0;
                        retVal = VPP_BE_SCDC_GetSinkVersion(vpp_obj, &scdcTxdata);
                        if((retVal == MV_VPP_OK) && (scdcTxdata.SinkVersion == 0)) {
                            VPPHDMI_DBG_INFO("SCDC sink version : %d\n", scdcTxdata.SinkVersion);
                        } else if (pHdmiTx->SinkMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x) {
                            /*force SCDC Bit 7 cleared to confirm LGTV that source is HDMI 2.0*/
                            retVal = SCDC_I2C_regRead(vpp_obj, Addr, &DataBuf, Length);
                            //Mask other bits and set bit 7 to zero
                            DataBuf = (DataBuf & 0x7F);
                            retVal = SCDC_I2C_regWrite(vpp_obj, Addr, &DataBuf, Length);
                            VPPHDMI_DBG_INFO("force SCDC Bit 7 clear: %d\n", retVal);
                        }

                        scdcTxdata.SourceVersion = 0x1;
                        VPP_BE_SCDC_UpdateSourceVersion(vpp_obj, &scdcTxdata, SCDC_SOURCE_VERSION_WRITE);
                    } else {
                        VPPHDMI_DBG_INFO("no SCDC_Present\n");
                    }
                    break;
                }
                // Each time reading EDID 256 byte will take over 200ms, only try enough time if i2c read failed
                if (retVal == MV_VPP_EIOFAIL)
                    retries--;
                else
                    retries -= 5;

                if (retries <= 0)
                {
                    HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_EDID_READ_ERR);
                    VPPHDMI_DBG_ERROR("CPU1 >>> HDMI E-EDID read fails, rx !!!\n");
                    HdmiTxLoadFailSafeEdid(vpp_obj);
                    break;
                }
#ifndef VPP_DIAG
                DelayInMS(0, 10);
#endif

            }
        } else {
            VPPHDMI_DBG_INFO("Dolby Vision data updated\n");
            VPP_DV_GetEdidInfo(vpp_obj, pHdmiTx->pEdidArray + VPP_SIZE_OF_EDID_BLK);
        }
        pHdmiTx->edidInfo.validEdid = TRUE;
        pHdmiTx->curOutHdmiMode = pHdmiTx->edidInfo.hdmiMode;

        HdmiTxSinkSpecificConfig(vpp_obj);
#ifdef XVYCC_SUPPORT
        if (pHdmiTx->edidInfo.calInfo.xvYCC601 || pHdmiTx->edidInfo.calInfo.xvYCC709){
            vpp_obj->xvYCC_capable = TRUE;
        }
#endif
        vpp_obj->BT2020_capability = ((pHdmiTx->edidInfo.calInfo.BT2020RGB)|(pHdmiTx->edidInfo.calInfo.BT2020YCC << 1)| \
                                       (pHdmiTx->edidInfo.calInfo.BT2020cYCC << 2));

        /*HDR capabilities valid only if BT.2020 supported*/
        if(pHdmiTx->edidInfo.calInfo.BT2020YCC) {
            if(*(UCHAR*)(&pHdmiTx->edidInfo.HdrCapInfo.hdr10Info) != 0x0)
            {
                if(pHdmiTx->edidInfo.HdrCapInfo.hdr10Info.EOTF_ET_2 ||
                        pHdmiTx->edidInfo.HdrCapInfo.hdr10Info.EOTF_ET_0)
                {
                    vpp_obj->HDR_capable = TRUE;
                }
                if(pHdmiTx->edidInfo.HdrCapInfo.hdr10Info.EOTF_ET_3)
                {
                    vpp_obj->HLG_capable = TRUE;
                    VPPHDMI_DBG_INFO("HLG capable sink \n");
                }
            }
#ifdef CONFIG_AMP_VPP_HDR10_PLUS
            if(pHdmiTx->edidInfo.HdrCapInfo.isHDR10PlusSupported)
            {
                vpp_obj->HDR10PLUS_capable = TRUE;
                VPPHDMI_DBG_INFO("HDR10+ capable sink \n");
            }
#endif
        } else {
            vpp_obj->HDR_capable = FALSE;
            vpp_obj->HLG_capable = FALSE;
            vpp_obj->HDR10PLUS_capable = FALSE;
        }
        VPP_DV_HandleHPD(vpp_obj, TRUE, pHdmiTx->edidInfo.calInfo.BT2020YCC);
        eventCode = MV_VPP_EVENT_HDMI_SINK_CONNECTED;
        if (pHdmiTx->forceHPDEnabled)
            eventCode |= MV_VPP_EVENT_HDMI_PARAM_FORCE_HPD << MV_VPP_EVENT_HDMI_PARAM_OFFSET;
        HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, eventCode);

        pHdmiTx->forceHPDEnabled = FALSE;
        return TRUE;
    }

    else
    {
        VPPHDMI_DBG_INFO(".... HPD is low ....\n");
#ifdef EARC_RX_ENABLED
        BE_HDMITX_GetHdmiHpdStatus(pHdmiTx, &hdmiHpdState);
        if((pHdmiTx->earcContext.initialized == true) && (hdmiHpdState == false)) {
            /* if HPD 0 is triggered by earc hpd, avoid setting conn state as it will reset discovery to idle1*/
            BE_EARCRX_SetHdmiConnState(pHdmiTx, false);
        }
#endif
        pHdmiTx->rxConnected = false; /* set it here to make sure HPD high won't be missed on state check*/
        HdmiTxSetHdcpSMState(pHdmiTx, HDCP_SM_STATE_IDLE);
        /*TODO: Kill TMDS Signals on HPD Low*/
        VPP_BE_SetHdmiPhyPower(vpp_obj, FALSE);
        BE_HDMITX_EnableInterrupts(pHdmiTx, IRQ_SINKDET, false);

        /*TODO: HDCP disable */

        pHdmiTx->edidInfo.validEdid = FALSE;
        pHdmiTx->curOutHdmiMode = FALSE;
#ifdef XVYCC_SUPPORT
        vpp_obj->xvYCC_capable = FALSE;
#endif
        /*Disable BT202*/
        vpp_obj->BT2020_capability = 0x0;
        /*Disable HDR & HLG*/
        vpp_obj->HDR_capable = FALSE;
        vpp_obj->HDR_enable = FALSE;
        vpp_obj->HLG_enable = FALSE;
        vpp_obj->HLG_capable = FALSE;
        vpp_obj->HDR10PLUS_enable = FALSE;
        vpp_obj->HDR10PLUS_capable = FALSE;
        vpp_obj->HDR_Status = FALSE;
        pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_NONE;
        VPP_DV_HandleHPD(vpp_obj, FALSE, FALSE);
        eventCode = MV_VPP_EVENT_HDMI_SINK_DISCONNECTED;
        if (pHdmiTx->forceHPDEnabled)
            eventCode |= MV_VPP_EVENT_HDMI_PARAM_FORCE_HPD << MV_VPP_EVENT_HDMI_PARAM_OFFSET;
        HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, eventCode);
        VPP_Memset(&pHdmiTx->hdcpErrEvent,0, sizeof(HDMITX_HDCP_ERR_EVENT));

#ifdef EARC_RX_ENABLED
        /* Set the eARC rx to EARC mode to be able to detect eARC on next HPD 1 */
        if(pHdmiTx->earcContext.initialized == true &&
                pHdmiTx->earcContext.userMode == EARC_MODE_EARC) {
            BE_EARCRX_SetMode(pHdmiTx, EARC_MODE_EARC);
        }

#endif /* EARC_RX_ENABLED */
        if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
            VPPHDMI_DBG_INFO("reset requested HDCP version to Auto for repeater\n");
            pHdmiTx->requestedHdcpVersion = VPP_HDMI_HDCP_VER_AUTO;
        }
    }

    return FALSE;

}

/*******************************************************************
 * FUNCTION: Construct AVI info frame packet
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
static int HdmiTxConfigAVIInfoFrame (VPP_OBJ *vpp_obj,
                                     int aspRatio, int scanInfo,
                                     int extCalorimetry, BOOL newFrame)
{
    BE_HDMITX_DATA      *pHdmiTx;
    int resID;
    RESOLUTION_INFO  *pResinfo;
    UINT8 edr_capable, edr_enable, edr_state, dv_ll;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    edr_capable = VPP_DV_GetCapability(vpp_obj, DV_CAP_EDRCAP);
    edr_enable = VPP_DV_GetCapability(vpp_obj, DV_CAP_EDREN);
    edr_state = VPP_DV_GetCapability(vpp_obj, DV_CAP_STDDV);
    dv_ll = VPP_DV_GetCapability(vpp_obj, DV_CAP_LL);
    resID = pHdmiTx->videoParams.DispRes;
    pResinfo = VPP_GetResInfo(resID);

    // Create AVI Infoframe packet to be sent
    if (newFrame) {
        // Set color space
        if ((pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR444) || \
                (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_IPT444) ||\
                (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR444_P3D65) || \
                (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR444_R2020))
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_YCBCR_444;
        else if ((pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR422) || \
                (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_IPT422) || \
                (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR422_P3D65) || \
                (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR422_R2020))
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_YCBCR_422;
        else if ((pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR420) || \
                (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR420_P3D65) || \
                (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_YCBCR420_R2020))
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_YCBCR_420;
        else
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_RGB;
    }

    //Do no change setting if playback is in progress
    if(edr_enable && edr_state && (vpp_obj->m_outformat == VPP_OUTFORMAT_DOVI))
    {
        if (!pHdmiTx->edidInfo.HdrCapInfo.dolbyvisionvsvdb.is422_12bitSupported)
        {
            //Update AVI also to send RGB-888
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_RGB;
            pHdmiTx->aviParams.RGBQuantRange = CEA_RGB_QUANT_FULL_RANGE;
        }
        else
        {
            //Update AVI also to send YUV422
            pHdmiTx->aviParams.ColorSpace = CEA_COLSP_YCBCR_422;
            pHdmiTx->aviParams.YCCQuantRange = CEA_YCC_QUANT_FULL_RANGE;
        }
    }
    else
    {
        pHdmiTx->aviParams.RGBQuantRange = VPP_HDMI_GET_RGB_RANGE(pHdmiTx, pResinfo);
        pHdmiTx->aviParams.YCCQuantRange=CEA_YCC_QUANT_LTD_RANGE;
        if((vpp_obj->m_outformat == VPP_OUTFORMAT_DOVI) && (dv_ll == TRUE) && \
            (pHdmiTx->videoParams.VideoFmt == OUTPUT_COLOR_FMT_RGB888) && \
            (edr_capable & VPP_HDMI_SINK_STD_LL))
        {
            pHdmiTx->aviParams.RGBQuantRange = CEA_RGB_QUANT_FULL_RANGE;
        }
    }

    // Set calorimetry
    if (extCalorimetry != HDMI_USE_EXISTING)
    {
        if (extCalorimetry == TRUE)
        {
            pHdmiTx->aviParams.Calorimetry = CEA_CAL_EXT_FMT;

            if(vpp_obj->hdmi_bt2020en == TRUE){
                pHdmiTx->aviParams.ExtCalorimetry = CEA_EXT_CAL_BT2020YCC_FMT;
            }
            else if (pResinfo->type == TYPE_SD) {
                pHdmiTx->aviParams.ExtCalorimetry = CEA_EXT_CAL_XVYCC_601_FMT;
            }
            else {
                pHdmiTx->aviParams.ExtCalorimetry = CEA_EXT_CAL_XVYCC_709_FMT;
            }
        }
        else
        {
            if (pResinfo->type == TYPE_SD) {
                pHdmiTx->aviParams.Calorimetry = CEA_CAL_ITU601_FMT;
            }
            else {
                pHdmiTx->aviParams.Calorimetry = CEA_CAL_ITU709_FMT;
            }
            pHdmiTx->aviParams.ExtCalorimetry = CEA_EXT_CAL_MAX; // Reserved
        }
        VPPHDMI_DBG_INFO("AVI IF :: Calorimetry :%d, ExtCalorimetry :%d\n",
                        pHdmiTx->aviParams.Calorimetry, pHdmiTx->aviParams.ExtCalorimetry);
    }

    BE_HDMITX_ConfigHdmiVideoFmt(pHdmiTx, aspRatio, scanInfo, !newFrame);
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Compute Speaker Channel Mapping for audio info frame
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdmiTxCalcSpkrChnMap (VPP_HDMI_SPKR_ALLOC *pInSpkrAlloc,
                                 unsigned char *pOutSpkrMap)
{
    *pOutSpkrMap = 0;

    if (pInSpkrAlloc->FlFr == FALSE)
    {
        // Error
        return (MV_VPP_EBADPARAM);
    }

    // Bit 0 of the channel map
    *pOutSpkrMap |= (pInSpkrAlloc->Lfe << 0);

    // Bit 1 of the channel map
    *pOutSpkrMap |= (pInSpkrAlloc->Fc << 1);

    if (pInSpkrAlloc->FlcFrc == FALSE && pInSpkrAlloc->RlcRrc == FALSE)
    {
        if (pInSpkrAlloc->Rc == TRUE)
        {
            *pOutSpkrMap |= (1 << 2);
        }
        if (pInSpkrAlloc->RlRr == TRUE)
        {
            *pOutSpkrMap |= (1 << 3);
        }
    }
    else if (pInSpkrAlloc->RlcRrc == TRUE)
    {
        if ((pInSpkrAlloc->Rc == TRUE || pInSpkrAlloc->FlcFrc == TRUE) &&
            (pInSpkrAlloc->RlRr == FALSE))
        {
            return (MV_VPP_EBADPARAM);
        }
        *pOutSpkrMap |= (1 << 4);
    }
    else if (pInSpkrAlloc->FlcFrc == TRUE)
    {
        if (pInSpkrAlloc->RlRr == TRUE && pInSpkrAlloc->Rc == TRUE)
        {
            // Mutually exclusive
            return (MV_VPP_EBADPARAM);
        }

        *pOutSpkrMap |= (1 << 4);
        if (pInSpkrAlloc->Rc == TRUE)
        {
            *pOutSpkrMap |= (1 << 3);
        }
        else
        {
            *pOutSpkrMap |= (1 << 2);
        }

        if (pInSpkrAlloc->RlRr == TRUE)
        {
            *pOutSpkrMap |= (1 << 3);
        }
    }
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Notifies Hdmi Tx events to application
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdmiTxNotifyEvent (VPP_OBJ *vpp_obj, UINT32 event, UINT32 infoID)
{
    BE_HDMITX_DATA  *pHdmiTx;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    if (pHdmiTx->appCbFn != NULL)
        VPP_CB_FUNC(pHdmiTx->appCbFn,(void *)vpp_obj,event, (void *)&infoID, pHdmiTx->contextParam);
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Notifies Hdcp events to application
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdcpNotifyEvent (VPP_OBJ *vpp_obj, UINT32 event, UINT32 infoID)
{
    BE_HDMITX_DATA  *pHdmiTx;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    if (pHdmiTx->appCbFn2 != NULL){
        VPP_CB_FUNC(pHdmiTx->appCbFn2, vpp_obj, event, (void *)&infoID, pHdmiTx->contextParam2);
    }
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Checks if HDCP should fallback to lower version
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   None
 ******************************************************************/
#if HDCP2X_ENABLED
static void HdcpFallbackCheck(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

#ifndef HDCP_FALLBACK_FOR_RPTR
    if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
    /* For repeater mode, fallback check is not required. Till HDCP converter is implemented.
       If downstream fallback actived with upstream version restriction, it will cause CTS failure */
        VPPHDMI_DBG_INFO("no fallback in repeater mode..\n");
        return;
    }
#endif

    if(pHdmiTx->sm_state == HDCP_SM_STATE_REAUTH) {
        ++pHdmiTx->hdcp2xRetryCount;
    } else {
        /* reset the count if not because of failure and HPD*/
        pHdmiTx->hdcp2xRetryCount = 0;
    }

    if (pHdmiTx->hdmiConfig.hdcpFallbackOn2xAuthFailure  &&
        pHdmiTx->hdcp2xRetryCount >= HDCP1X_FALLBACK_THRES) {
        VPPHDMI_DBG_INFO("trigger fallback to hdcp1.4 on hdcp2.2 failure\n");
        BE_HDMITX_TriggerHdcpFallback(pHdmiTx);
        VPPHDMI_DBG_INFO("reset audio before fallback to hdcp1.4.\n");
        VPP_BE_SetHdmiAudioFmt(vpp_obj, true);

#if defined (HDCP_FALLBACK_FOR_RPTR)
        if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
            /*restrict sink hdcp version to 1.4 due to repeated error on HDCP2.2*/
            pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
            pHdmiTx->SinkMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
            VPPHDMI_DBG_INFO("notify upstream to switch to hdcp1.4.\n");
            HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2, MV_VPP_EVENT_HDCP_RP_REQ_UPSTM_TOGGLE);
        }
#endif
    }
}
#endif
/*******************************************************************
 * FUNCTION: Enable/Disable Encryption
 * PARAMS:   *vpp_obj - pointer to VPP object
 *       :   enable   - enable/disable encryption
 * RETURN:   MV_VPP_OK
 ******************************************************************/
static int HdmiTxEnableEncryption(VPP_OBJ *vpp_obj, BOOL enable)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    HdmiTxTimerCmdCfg (vpp_obj, 0);
    if (enable)
    {
        pHdmiTx->frameCntr = VPP_HDCP_START_DELAY_FRAME_COUNT;
        pHdmiTx->hdmiLockCheck = -1;
        VPP_BE_SET_BIT(vpp_obj->be.hdmiFrmIntrTrig, VPP_BE_HDMI_HDCP_FRMTRIG);
    }
    else
    {
        HdmiTxSetHdcpSMState(pHdmiTx, HDCP_SM_STATE_IDLE);
        VPP_BE_CLR_BIT(vpp_obj->be.hdmiFrmIntrTrig, VPP_BE_HDMI_HDCP_FRMTRIG);
        pHdmiTx->frameCntr = -1;
        BE_HDMITX_StopHdcpAuthentication (pHdmiTx);
    }
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Configure timer command generation
 * PARAMS:   *vpp_obj         - pointer to VPP object
 *       :   timerCmdInterval - Time in milliseconds - timer command
 *       :                      would be generated in this interval
 * RETURN:   MV_VPP_OK
 ******************************************************************/
static int HdmiTxTimerCmdCfg (VPP_OBJ *vpp_obj, int timerCmdInterval)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    pHdmiTx->timerCmdCfg = (timerCmdInterval/HDMITX_TIMER_CONFIG);
    pHdmiTx->timerCmdCntr = 0;
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Get EDID info from receiver
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
static int HdmiTxGetEdidInfo (VPP_OBJ *vpp_obj,VPP_HDMI_SINK_CAPS *pEdidInfo)
{
    UINT8           dataBuf[128];
    UINT8           blkMapData[128];
    EDID_DATA       parsedData;
    EDID_DATA       parsedMapData;
    EDID_BLK_TYPE   edidBlkType;
    EDID_RETURNS    edidRet;
    UINT32          ceaExtnBlkNum;
    INT             retVal;
    UINT8           numExtns;
    BOOL            addResEntry;
    BOOL            firstCEAExt;
    INT             i,j,k;
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPP_Memset((UINT8*)pEdidInfo, 0, sizeof(VPP_HDMI_SINK_CAPS));
    pEdidInfo->resCnt = 0;

    // Initialize pixel repetition resolution mask for the four SD resolutions
    // 480i resolution
    pEdidInfo->prInfo[0].resMask = (1 << RES_NTSC_M) | (1 << RES_NTSC_J) |
                                   (1 << RES_PAL_M)  | (1 << RES_525I60) |
                                   (1 << RES_525I5994);
    // 576i resolution
    pEdidInfo->prInfo[1].resMask = (1 << RES_PAL_BGH) | (1 << RES_625I50);

    // 480p resolution
    pEdidInfo->prInfo[2].resMask = (1 << RES_525P60) | (1 << RES_525P5994);

    // 576p resolution
    pEdidInfo->prInfo[3].resMask = (1 << RES_625P50);

    *(unsigned char*)(&pEdidInfo->spkrAlloc) = 0xFF;
    VPP_Memset((UINT8*)(&pEdidInfo->calInfo), 0, sizeof(VPP_HDMI_CALORIMETRY_INFO));
    *(unsigned char*)(&pEdidInfo->CeaHdmiVSDB)  = 0xFF;

    // Read EDID block via I2C
    retVal = BE_HDMITX_ReadEdidBlock (pHdmiTx, 0, dataBuf);
    pHdmiTx->EdidCurrentchecksum[0]=dataBuf[127];
    if (retVal)
    {
        VPPHDMI_DBG_ERROR("BE_HDMITX_ReadEdidBlock failed, retVal=0x%x\n", retVal);
        return MV_VPP_EIOFAIL;
    }
    // Parse Basic Block
    VPP_Memset((UINT8*)&parsedData, 0, sizeof(EDID_DATA));
    edidRet = ParseEdid(dataBuf,&edidBlkType,&parsedData, FALSE);
    if (edidRet != EDID_ERR_OK || edidBlkType != EDID_BASIC_BLK)
    {
        VPPHDMI_DBG_ERROR("ParseEdid Basic Block failed, edidRet=%d, edidBlkType=%d", edidRet, edidBlkType);
        return MV_VPP_EBADPARAM;
    }

    // Save required basic block info
    VPP_Memcpy(&pEdidInfo->monitorName, &parsedData.BasicBlk.MonitorInfo.MonitorName, 14);
    VPP_Memcpy(&pEdidInfo->mnfrName, &parsedData.BasicBlk.GenInfo.MnfrName, 4);
    pEdidInfo->mnfrWeek = parsedData.BasicBlk.GenInfo.MnfrWeek;
    pEdidInfo->mnfrYear = parsedData.BasicBlk.GenInfo.MnfrYear;
    VPP_Memcpy(&pEdidInfo->productCode, &parsedData.BasicBlk.GenInfo.ProductCode, 5);
    pEdidInfo->serialNum = parsedData.BasicBlk.GenInfo.SerialNum;

    pEdidInfo->minVRate = parsedData.BasicBlk.MonitorInfo.MinVRate;
    pEdidInfo->maxVRate = parsedData.BasicBlk.MonitorInfo.MaxVRate;
    pEdidInfo->minHRate = parsedData.BasicBlk.MonitorInfo.MinHRate;
    pEdidInfo->maxHRate = parsedData.BasicBlk.MonitorInfo.MaxHRate;
    pEdidInfo->maxPixelClock = parsedData.BasicBlk.MonitorInfo.MaxPixelClock*10;

    pEdidInfo->sprtedDispRes = 0;
    for (i = 0; i < parsedData.BasicBlk.NumSpprtedVidFmts; i++)
    {
        addResEntry = TRUE;
        if (parsedData.BasicBlk.VideoResInfo[i].VideoIDCode != 0)
        {
            HdmiTxMapVICToResID (parsedData.BasicBlk.VideoResInfo[i].VideoIDCode,
                                 &pEdidInfo->sprtedDispRes,&addResEntry);
            HdmiTxMapPixelReptSupport (parsedData.BasicBlk.VideoResInfo[i].VideoIDCode, pEdidInfo);
        }
        if ((pEdidInfo->resCnt < MAX_RES_INFO) && (addResEntry == TRUE))
        {
            pEdidInfo->resInfo[pEdidInfo->resCnt].hActive = parsedData.BasicBlk.VideoResInfo[i].HActive;
            pEdidInfo->resInfo[pEdidInfo->resCnt].vActive = parsedData.BasicBlk.VideoResInfo[i].VActive;
            pEdidInfo->resInfo[pEdidInfo->resCnt].refreshRate = parsedData.BasicBlk.VideoResInfo[i].RefreshRate;
            pEdidInfo->resInfo[pEdidInfo->resCnt].interlaced = parsedData.BasicBlk.VideoResInfo[i].FrmFmt;
            pEdidInfo->resInfo[pEdidInfo->resCnt].AspRatio = parsedData.BasicBlk.VideoResInfo[i].AspRatio;
            if (parsedData.BasicBlk.PrefTimingEntry == i)
                VPP_Memcpy(&pEdidInfo->prefResInfo,&pEdidInfo->resInfo[pEdidInfo->resCnt],sizeof(VPP_HDMI_RES_INFO));
            pEdidInfo->resCnt++;
        }
    }
    pEdidInfo->gamma = (float)(parsedData.BasicBlk.DispParams.Gamma + 100)/100;
    pEdidInfo->maxHSize = parsedData.BasicBlk.DispParams.MaxHImageSize*10;
    pEdidInfo->maxVSize = parsedData.BasicBlk.DispParams.MaxVImageSize*10;

    numExtns = parsedData.BasicBlk.NumExtns;

    if (pHdmiTx->pEdidArray == NULL) {
          VPPHDMI_DBG_ERROR("Memory could not be allocated for raw EDID storage\n");
          return MV_VPP_ENOMEM;
    }
    VPP_Memset((UINT8 *)pHdmiTx->pEdidArray, 0, (VPP_SIZE_OF_EDID_BLK*(MAX_SUPPORTED_EDID_BLOCKS)*sizeof(UINT8)));
    //Copy raw EDID
    VPP_Memcpy((UINT8 *)pHdmiTx->pEdidArray, dataBuf, VPP_SIZE_OF_EDID_BLK);
    if (numExtns == 0)
        return (MV_VPP_OK);

    ceaExtnBlkNum = 1;
    j = 0;
    firstCEAExt = TRUE;

    VPP_Memset((UINT8*)&parsedData, 0, sizeof(EDID_DATA));
    retVal = BE_HDMITX_ReadEdidBlock (pHdmiTx,1, dataBuf);
    pHdmiTx->EdidCurrentchecksum[1] = dataBuf[127];
    VPP_Memcpy((UINT8 *)pHdmiTx->pEdidArray + VPP_SIZE_OF_EDID_BLK, dataBuf, VPP_SIZE_OF_EDID_BLK);

    if (retVal) {
        VPPHDMI_DBG_ERROR("Failed to read Block Map, retVal=0x%x\n", retVal);
        return retVal;
    }
    retVal = EdidGetBlkType(dataBuf,&edidBlkType);
    if (retVal) {
        VPPHDMI_DBG_ERROR("Failed to get block type, retVal=0x%x\n", retVal);
        return retVal;
    }
    if (edidBlkType == EDID_BLOCK_MAP) {
        VPP_Memcpy(blkMapData, dataBuf, 128);
    } else if (edidBlkType == EDID_CEA_TIMING_EXTN) {
        numExtns = 1;   //numExtns > 1 only in case of map block
        VPP_DV_GetEdidInfo(vpp_obj, pHdmiTx->pEdidArray + VPP_SIZE_OF_EDID_BLK);
        edidRet = ParseEdid(dataBuf,&edidBlkType,&parsedData, firstCEAExt);
        if (edidRet != EDID_ERR_OK || edidBlkType != EDID_CEA_TIMING_EXTN)
        {
            VPPHDMI_DBG_ERROR("Failed to Parse CEA Extension Block %d, retVal=%d, edidBlkType=%d\n",
                                i, retVal, edidBlkType);
            return MV_VPP_EBADPARAM;
        }
        firstCEAExt = FALSE;
    } else {
        return MV_VPP_EBADPARAM; //block 1 should be either CEA or MAP
    }

    pHdmiTx->Extblks = 1;
    for (i = 2; (i <= numExtns) || (i <= parsedData.CEAExtnBlk.ExtBlkCnt); i++) {
        if (numExtns > 1) {
            VPP_Memset((UINT8*)&parsedMapData, 0, sizeof(EDID_DATA));
            parsedMapData.BlkMap.CurBlkNum = ceaExtnBlkNum;
            edidRet = ParseEdid(blkMapData,&edidBlkType,&parsedMapData, FALSE);
            if (edidRet != EDID_ERR_OK || edidBlkType != EDID_BLOCK_MAP) {
                VPPHDMI_DBG_ERROR("Failed to Parse Block Map retVal=%d\n", retVal);
                return MV_VPP_EBADPARAM;
            }

            if (parsedMapData.BlkMap.CeaExtnBlkNum == 0) {
                break;
            }
            ceaExtnBlkNum = parsedMapData.BlkMap.CeaExtnBlkNum;
        } else {
            ceaExtnBlkNum = i;
        }
        // Parse CEA Extension Block
        retVal = BE_HDMITX_ReadEdidBlock(pHdmiTx,ceaExtnBlkNum, dataBuf);
        pHdmiTx->EdidCurrentchecksum[ceaExtnBlkNum] = dataBuf[127];
        VPP_Memcpy((UINT8 *)pHdmiTx->pEdidArray + (VPP_SIZE_OF_EDID_BLK * i), dataBuf, VPP_SIZE_OF_EDID_BLK);
        pHdmiTx->Extblks++;
        if (retVal)
        {
            VPPHDMI_DBG_ERROR("Failed to read CEA Extension Block %d, retVal=%d\n",
                                i, retVal);
            return retVal;
        }

        VPP_DV_GetEdidInfo(vpp_obj, pHdmiTx->pEdidArray + (VPP_SIZE_OF_EDID_BLK * i));
        edidRet = ParseEdid(dataBuf,&edidBlkType,&parsedData, firstCEAExt);
        if((edidRet == EDID_ERR_OK) && (edidBlkType == EDID_UNDEF_BLK))
        {
            VPPHDMI_DBG_ERROR("Cannot Parse Undefined Block(Type:%d)\n",dataBuf[0]);
            continue;
        }
        else if (edidRet != EDID_ERR_OK || edidBlkType != EDID_CEA_TIMING_EXTN)
        {
            VPPHDMI_DBG_ERROR("Failed to Parse CEA Extension Block %d, retVal=%d, edidBlkType=%d\n",
                                    i, retVal, edidBlkType);
            return MV_VPP_EBADPARAM;
        }
        firstCEAExt = FALSE;
    }

    // Store the AV info
    if (parsedData.CEAExtnBlk.VndrBlkFound)
    {
        pEdidInfo->hdmiMode = parsedData.CEAExtnBlk.HdmiMode;
        pEdidInfo->DC36bitSprt = parsedData.CEAExtnBlk.FeatureSprt.DC36bit;
        pEdidInfo->DC30bitSprt = parsedData.CEAExtnBlk.FeatureSprt.DC30bit;
        pEdidInfo->YCbCr444AtDC = parsedData.CEAExtnBlk.FeatureSprt.YCbCr444AtDC;
        pEdidInfo->AISprt = parsedData.CEAExtnBlk.FeatureSprt.SupportsAI;
        pEdidInfo->cecPhyAddr = parsedData.CEAExtnBlk.CecPhyAddr;
        pEdidInfo->maxTMDSClock = parsedData.CEAExtnBlk.MaxTMDSClock;

        // Latency Info
        VPP_Memset (&pEdidInfo->latencyInfo, 0, sizeof(VPP_EDID_LATENCY_INFO));
        if (parsedData.CEAExtnBlk.HdmiLatencyFields.Latency_Fields_Present)
        {
            HdmiTxGetLatencyInfo (parsedData.CEAExtnBlk.HdmiLatencyFields.Video_Latency,
                                  &pEdidInfo->latencyInfo.prgVidLatencySts,
                                  &pEdidInfo->latencyInfo.prgVidLatencyValue);

            HdmiTxGetLatencyInfo (parsedData.CEAExtnBlk.HdmiLatencyFields.Audio_Latency,
                                  &pEdidInfo->latencyInfo.prgAudLatencySts,
                                  &pEdidInfo->latencyInfo.prgAudLatencyValue);
        }

        if (parsedData.CEAExtnBlk.HdmiLatencyFields.I_Latency_Fields_Present)
        {
            HdmiTxGetLatencyInfo (parsedData.CEAExtnBlk.HdmiLatencyFields.Interlaced_Video_Latency,
                                  &pEdidInfo->latencyInfo.intVidLatencySts,
                                  &pEdidInfo->latencyInfo.intVidLatencyValue);

            HdmiTxGetLatencyInfo (parsedData.CEAExtnBlk.HdmiLatencyFields.Interlaced_Audio_Latency,
                                  &pEdidInfo->latencyInfo.intAudLatencySts,
                                  &pEdidInfo->latencyInfo.intAudLatencyValue);
        }
        else
        {
            pEdidInfo->latencyInfo.intVidLatencySts     = pEdidInfo->latencyInfo.prgVidLatencySts;
            pEdidInfo->latencyInfo.intVidLatencyValue   = pEdidInfo->latencyInfo.prgVidLatencyValue;

            pEdidInfo->latencyInfo.intAudLatencySts     = pEdidInfo->latencyInfo.prgAudLatencySts;
            pEdidInfo->latencyInfo.intAudLatencyValue   = pEdidInfo->latencyInfo.prgAudLatencyValue;
        }
    }
    pEdidInfo->YCbCr444Sprt = parsedData.CEAExtnBlk.FeatureSprt.YCbCr444Sprt;
    pEdidInfo->YCbCr422Sprt = parsedData.CEAExtnBlk.FeatureSprt.YCbCr422Sprt;

    for (i = 0; i < parsedData.CEAExtnBlk.NumSpprtedVidFmts; i++)
    {
        pEdidInfo->svdblocklen = parsedData.CEAExtnBlk.NumSpprtedVidFmts;
        pEdidInfo->svd_vics[i] = (UINT8)parsedData.CEAExtnBlk.VideoResInfo[i].VideoIDCode;
        addResEntry = TRUE;
        if (parsedData.CEAExtnBlk.VideoResInfo[i].VideoIDCode != 0)
        {
            HdmiTxMapVICToResID (parsedData.CEAExtnBlk.VideoResInfo[i].VideoIDCode,
                                 &pEdidInfo->sprtedDispRes,&addResEntry);
            HdmiTxMapPixelReptSupport (parsedData.CEAExtnBlk.VideoResInfo[i].VideoIDCode, pEdidInfo);
        }
        if (pEdidInfo->resCnt < MAX_RES_INFO)
        {
            if (addResEntry == TRUE)
            {
                pEdidInfo->resInfo[pEdidInfo->resCnt].hActive = parsedData.CEAExtnBlk.VideoResInfo[i].HActive;
                pEdidInfo->resInfo[pEdidInfo->resCnt].vActive = parsedData.CEAExtnBlk.VideoResInfo[i].VActive;
                pEdidInfo->resInfo[pEdidInfo->resCnt].refreshRate = parsedData.CEAExtnBlk.VideoResInfo[i].RefreshRate;
                pEdidInfo->resInfo[pEdidInfo->resCnt].interlaced = parsedData.CEAExtnBlk.VideoResInfo[i].FrmFmt;
                pEdidInfo->resInfo[pEdidInfo->resCnt].AspRatio = parsedData.CEAExtnBlk.VideoResInfo[i].AspRatio;
                if (parsedData.CEAExtnBlk.PrefTimingEntry == i)
                    VPP_Memcpy(&pEdidInfo->prefResInfo,&pEdidInfo->resInfo[pEdidInfo->resCnt],sizeof(VPP_HDMI_RES_INFO));
                pEdidInfo->resCnt++;
            } else {
                /* check if need to update VideoResInfo */
                for (k = 0; k < pEdidInfo->resCnt; k++)
                {
                    if ((pEdidInfo->resInfo[k].hActive == parsedData.CEAExtnBlk.VideoResInfo[i].HActive) &&
                        (pEdidInfo->resInfo[k].vActive == parsedData.CEAExtnBlk.VideoResInfo[i].VActive) &&
                        (pEdidInfo->resInfo[k].refreshRate == parsedData.CEAExtnBlk.VideoResInfo[i].RefreshRate) &&
                        (pEdidInfo->resInfo[k].interlaced == parsedData.CEAExtnBlk.VideoResInfo[i].FrmFmt))
                    {
                        pEdidInfo->resInfo[k].AspRatio = parsedData.CEAExtnBlk.VideoResInfo[i].AspRatio;
                    }
                }
            }
        }
    }

    // Save HDMI 3D capability information
    pEdidInfo->support_3D = parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_3D_Present;
    // according to HDMI 1.4 spec, 1080p24 and 720p60/50 (frame packing) are mandatory resolutions to be supported by 3D receivers
    if (pEdidInfo->sprtedDispRes & ((1<<RES_720P60)|(1<<RES_1080I60))) {
        pEdidInfo->sprted3DDispRes = (1<<(RES_720P60_3D-FIRST_RES_3D))
                                   | (1<<(RES_720P5994_3D-FIRST_RES_3D))
                                   | (1<<(RES_1080P24_3D-FIRST_RES_3D))
                                   | (1<<(RES_1080P2398_3D-FIRST_RES_3D));
        pEdidInfo->sprted3DStructures[RES_720P60_3D-FIRST_RES_3D] = (1 << VPP_HDMI_3D_STRUCT_FRAME_PACKING);
        pEdidInfo->sprted3DStructures[RES_720P5994_3D-FIRST_RES_3D] = (1 << VPP_HDMI_3D_STRUCT_FRAME_PACKING);
        pEdidInfo->sprted3DStructures[RES_1080P24_3D-FIRST_RES_3D] = (1 << VPP_HDMI_3D_STRUCT_FRAME_PACKING);
        pEdidInfo->sprted3DStructures[RES_1080P2398_3D-FIRST_RES_3D] = (1 << VPP_HDMI_3D_STRUCT_FRAME_PACKING);
    } else {
        pEdidInfo->sprted3DDispRes = (1<<(RES_720P50_3D-FIRST_RES_3D))
                                   | (1<<(RES_1080P24_3D-FIRST_RES_3D))
                                   | (1<<(RES_1080P2398_3D-FIRST_RES_3D));
        pEdidInfo->sprted3DStructures[RES_720P50_3D-FIRST_RES_3D] = (1 << VPP_HDMI_3D_STRUCT_FRAME_PACKING);
        pEdidInfo->sprted3DStructures[RES_1080P24_3D-FIRST_RES_3D] = (1 << VPP_HDMI_3D_STRUCT_FRAME_PACKING);
        pEdidInfo->sprted3DStructures[RES_1080P2398_3D-FIRST_RES_3D] = (1 << VPP_HDMI_3D_STRUCT_FRAME_PACKING);
    }
    //Add VIC for 4k
    if(parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_VIC_Len > 0 ) {
        pEdidInfo->hdmi_VIC_Len = parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_VIC_Len;
        for (i=0; i<parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_VIC_Len; i++) {
             HdmiTxMapVICTo4KResID(parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_VIC[i], &pEdidInfo->sprtedHdmiVICRes);
        }
    }
    // additional 3D formats supported
    if (parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_3D_Multi_Present == 0x01) {
        for (i=parsedData.CEAExtnBlk.FirstSVDEntry; i<parsedData.CEAExtnBlk.LastSVDEntry; i++)
            HdmiTxMapVICTo3DResID (parsedData.CEAExtnBlk.VideoResInfo[i].VideoIDCode, &pEdidInfo->sprted3DDispRes, parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_3D_Structure_All, pEdidInfo->sprted3DStructures);
    } else if (parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_3D_Multi_Present == 0x02) {
        for (i=parsedData.CEAExtnBlk.FirstSVDEntry; i<parsedData.CEAExtnBlk.LastSVDEntry; i++) {
            if (!(parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_3D_Mask & (1<<(i-parsedData.CEAExtnBlk.FirstSVDEntry))))
                continue;
            HdmiTxMapVICTo3DResID (parsedData.CEAExtnBlk.VideoResInfo[i].VideoIDCode, &pEdidInfo->sprted3DDispRes, parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_3D_Structure_All, pEdidInfo->sprted3DStructures);
        }
    }

    for (i=0; i<parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_2D_VIC_Num; i++) {
        HdmiTxMapVICTo3DResID (parsedData.CEAExtnBlk.VideoResInfo[parsedData.CEAExtnBlk.FirstSVDEntry+parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_2D_VIC_Order[i]].VideoIDCode, &pEdidInfo->sprted3DDispRes, (1<<parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_3D_Structure[i]) | (parsedData.CEAExtnBlk.Hdmi3DFeatureSprt.Hdmi_3D_Detail[i]<<16), pEdidInfo->sprted3DStructures);
    }

    // Save LPCM and compressed audio information
    for (i = 0; i < parsedData.CEAExtnBlk.NumSprtedAudFmts; i++)
    {
        if (parsedData.CEAExtnBlk.AudioInfo[i].AudioFmt == EDID_AUD_FMT_LPCM)
        {
            pEdidInfo->audioInfo[j].audioFmt = parsedData.CEAExtnBlk.AudioInfo[i].AudioFmt;
            *(unsigned char*)(&pEdidInfo->audioInfo[j].freqSprt) = parsedData.CEAExtnBlk.AudioInfo[i].Frequency;
            *(unsigned char*)(&pEdidInfo->audioInfo[j].wdLenSprt) =
                (parsedData.CEAExtnBlk.AudioInfo[i].AudioRate.UnComprAudRate.Sprt16Bps << 1) |
                (parsedData.CEAExtnBlk.AudioInfo[i].AudioRate.UnComprAudRate.Sprt20Bps << 2) |
                (parsedData.CEAExtnBlk.AudioInfo[i].AudioRate.UnComprAudRate.Sprt24Bps << 3);
            pEdidInfo->audioInfo[j].maxNumChnls = parsedData.CEAExtnBlk.AudioInfo[i].MaxNumChnls;
        }
        else
        {
            pEdidInfo->audioInfo[j].audioFmt = parsedData.CEAExtnBlk.AudioInfo[i].AudioFmt;
            *(unsigned char*)(&pEdidInfo->audioInfo[j].freqSprt) = parsedData.CEAExtnBlk.AudioInfo[i].Frequency;
            pEdidInfo->audioInfo[j].maxBitRate = parsedData.CEAExtnBlk.AudioInfo[i].AudioRate.CompAudRate;
            pEdidInfo->audioInfo[j].maxNumChnls = parsedData.CEAExtnBlk.AudioInfo[i].MaxNumChnls;
        }
        j++;
        if (j >= 15) {
            break;
        }
    }
    if (parsedData.CEAExtnBlk.BasicAudioSprt && (j < 15)) {
        /* basic audio support */
        pEdidInfo->audioInfo[j].audioFmt = EDID_AUD_FMT_LPCM;
        *(unsigned char*)(&pEdidInfo->audioInfo[j].freqSprt) = (1 << CEA_AUDIO_SAMP_FREQ_32_KHZ) |
                                                              (1 << CEA_AUDIO_SAMP_FREQ_44_1_KHZ) |
                                                              (1 << CEA_AUDIO_SAMP_FREQ_48_KHZ);
        *(unsigned char*)(&pEdidInfo->audioInfo[j].wdLenSprt) = (1 << CEA_AUDIO_SAMP_SIZE_16_BITS);
        pEdidInfo->audioInfo[j].maxNumChnls = 2;
    }

    // Store speaker allocation map
    if (*(unsigned char*)(&parsedData.CEAExtnBlk.SpkrAlloc) != 0xFF)
        VPP_Memcpy(&pEdidInfo->spkrAlloc, &parsedData.CEAExtnBlk.SpkrAlloc, sizeof(VPP_HDMI_SPKR_ALLOC));

    //Check Vertical and Horizontal Image size
    if ((pEdidInfo->maxHSize == 0) &&
        (pEdidInfo->maxVSize == 0)) {
        pEdidInfo->maxHSize = parsedData.CEAExtnBlk.HImgSize;
        pEdidInfo->maxVSize = parsedData.CEAExtnBlk.VImgSize;
    }

    // Store calorimetry information
    if (parsedData.CEAExtnBlk.CalorimetryInfo.xvYCC601 || parsedData.CEAExtnBlk.CalorimetryInfo.xvYCC709 || parsedData.CEAExtnBlk.CalorimetryInfo.sYCC601 || \
       parsedData.CEAExtnBlk.CalorimetryInfo.AdobeYCC601 || parsedData.CEAExtnBlk.CalorimetryInfo.AdobeRGB || parsedData.CEAExtnBlk.CalorimetryInfo.BT2020RGB || \
       parsedData.CEAExtnBlk.CalorimetryInfo.BT2020YCC || parsedData.CEAExtnBlk.CalorimetryInfo.BT2020cYCC || parsedData.CEAExtnBlk.CalorimetryInfo.MD0|| \
       parsedData.CEAExtnBlk.CalorimetryInfo.MD1 || parsedData.CEAExtnBlk.CalorimetryInfo.MD2 || parsedData.CEAExtnBlk.CalorimetryInfo.MD3)
        VPP_Memcpy(&pEdidInfo->calInfo, &parsedData.CEAExtnBlk.CalorimetryInfo, sizeof(VPP_HDMI_CALORIMETRY_INFO));

     // Store Quantization information and 420 supported resolutions
     VPP_Memcpy(&pEdidInfo->capInfo, &parsedData.CEAExtnBlk.CapabilityInfo, sizeof(VPP_HDMI_CAPABILITY_INFO));
     VPP_Memcpy(&pEdidInfo->YCbCr420_capInfo, &parsedData.CEAExtnBlk.YCbCr420CapInfo, sizeof(VPP_HDMI_YCbCr420_CAP_INFO));
     VPP_Memcpy(&pEdidInfo->CeaHdmiVSDB, &parsedData.CEAExtnBlk.CeaHdmiVSDB, sizeof(EDID_CEA_HFVSDB));
     VPP_Memcpy(&pEdidInfo->YCbCr420_capmapInfo, &parsedData.CEAExtnBlk.YCbCrCapMapInfo, sizeof(EDID_YCbCr420_CAP_MAP_INFO));
     VPP_Memcpy(&pEdidInfo->HdrCapInfo.hdr10Info, &parsedData.CEAExtnBlk.HDRMetadataInfo, sizeof(HDR_METADATA_INFO));
     VPP_Memcpy(&pEdidInfo->HdrCapInfo.dolbyvisionvsvdb, &parsedData.CEAExtnBlk.DolbyVisionVSVDB, sizeof(EDID_DOLBYVISION_CAP));
     VPP_Memcpy(&pEdidInfo->HdrCapInfo, &parsedData.CEAExtnBlk.HdrCapInfo, sizeof(EDID_HDR_CAPABILITY));
     VPP_Memcpy(&pEdidInfo->dolbyAudioCap, &parsedData.CEAExtnBlk.DolbyAudioVSADB, sizeof(EDID_DOLBY_AUDIO_CAP));
     return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Update pixel repetition support based on resolution info
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
static int HdmiTxMapPixelReptSupport (UINT32 videoIDCode, VPP_HDMI_SINK_CAPS *pEdidInfo)
{
    switch (videoIDCode)
    {
        case 6: // 1440*480i  @59.94/60Hz 4:3
        case 7: // 1440*480i  @59.94/60Hz 16:9
        {
            // Pixel repetition of 2 is supported
            pEdidInfo->prInfo[0].prSupport |= (1 << 2);
        }
        break;

        case 21: // 1440*576i  @50Hz 4:3
        case 22: // 1440*576i  @50Hz 16:9
        {
            // Pixel repetition of 2 is supported
            pEdidInfo->prInfo[1].prSupport |= (1 << 2);
        }
        break;

        case 10: // 2880*480i  @59.94/60Hz 4:3
        case 11: // 2880*480i  @59.94/60Hz 16:9
        {
            // Pixel repetition of 4 is supported
            pEdidInfo->prInfo[0].prSupport |= (1 << 4);
        }
        break;

        case 25: // 2880*576i  @50Hz 4:3
        case 26: // 2880*576i  @50Hz 16:9
        {
            // Pixel repetition of 4 is supported
            pEdidInfo->prInfo[1].prSupport |= (1 << 4);
        }
        break;

        case 2: // 720*480p   @59.94/60Hz 4:3
        case 3: // 720*480p   @59.94/60Hz 16:9
        {
            // Pixel repetition of 1 is supported
            pEdidInfo->prInfo[2].prSupport |= (1 << 1);
        }
        break;
        case 14: // 1440*480p  @59.94/60Hz 4:3
        case 15: // 1440*480p  @59.94/60Hz 16:9
        {
            // Pixel repetition of 2 is supported
            pEdidInfo->prInfo[2].prSupport |= (1 << 2);
        }
        break;

        case 35: // 2880*480p  @59.94/60Hz 4:3
        case 36: // 2880*480p  @59.94/60Hz 16:9
        {
            // Pixel repetition of 4 is supported
            pEdidInfo->prInfo[2].prSupport |= (1 << 4);
        }
        break;

        case 17: // 720*576p   @50Hz 4:3
        case 18: // 720*576p   @50Hz 16:9
        {
            // Pixel repetition of 1 is supported
            pEdidInfo->prInfo[3].prSupport |= (1 << 1);
        }
        break;
        case 29: // 1440*576p  @50Hz 4:3
        case 30: // 1440*576p  @50Hz 16:9
        {
            // Pixel repetition of 2 is supported
            pEdidInfo->prInfo[3].prSupport |= (1 << 2);
        }
        break;

        case 37: // 2880*576p  @50Hz 4:3
        case 38: // 2880*576p  @50Hz 16:9
        {
            // Pixel repetition of 4 is supported
            pEdidInfo->prInfo[3].prSupport |= (1 << 4);
        }
        break;

        default: break;
    }
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Map Video ID code to resolution ID
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
static int HdmiTxMapVICToResID(UINT32 videoIDCode, UINT32 *pVideoRes, BOOL *pNewRes)
{
    UINT32 resMask = 0;

    // VideoIDCode from CEA Spec - If any other resolutions are to
    // be handled, it should be added here
    switch (videoIDCode)
    {
        case 1: // 640x480p  @59.94/60Hz 4:3
            resMask = (1 << RES_VGA_480P60)/* | (1 << RES_VGA_480P5994)*/;
            break;

        case 6: // 1440*480i  @59.94/60Hz 4:3
        case 7: // 1440*480i  @59.94/60Hz 16:9
            resMask = (1 << RES_NTSC_M)
                    | (1 << RES_NTSC_J)
                    | (1 << RES_PAL_M)
                    | (1 << RES_525I60)
                    | (1 << RES_525I5994);
            break;

        case 21: // 1440*576i  @50Hz 4:3
        case 22: // 1440*576i  @50Hz 16:9
            resMask = (1 << RES_PAL_BGH)
                    | (1 << RES_625I50);
            break;

        case 2: // 720*480p   @59.94/60Hz 4:3
        case 3: // 720*480p   @59.94/60Hz 16:9
        case 14: // 1440*480p  @59.94/60Hz 4:3
        case 15: // 1440*480p  @59.94/60Hz 16:9
        case 35: // 2880*480p  @59.94/60Hz 4:3
        case 36: // 2880*480p  @59.94/60Hz 16:9
            resMask = (1 << RES_525P60)
                    | (1 << RES_525P5994);
            break;

        case 17: // 720*576p   @50Hz 4:3
        case 18: // 720*576p   @50Hz 16:9
        case 29: // 1440*576p  @50Hz 4:3
        case 30: // 1440*576p  @50Hz 16:9
        case 37: // 2880*576p  @50Hz 4:3
        case 38: // 2880*576p  @50Hz 16:9
            resMask = (1 << RES_625P50);
            break;

        case 4: // 1280*720p  @59.94/60Hz 16:9
            resMask = (1 << RES_720P60)
                    | (1 << RES_720P5994);
            break;

        case 19: // 1280*720p  @50Hz 16:9
            resMask = (1 << RES_720P50);
            break;

        case 5: // 1920*1080i @59.94/60Hz 16:9
            resMask = (1 << RES_1080I60)
                    | (1 << RES_1080I5994);
            break;

        case 20: // 1920*1080i @50Hz       16:9
            resMask = (1 << RES_1080I50);
            break;

        case 34: // 1920*1080p @29.97/30Hz 16:9
            resMask = (1 << RES_1080P30)
                    | (1 << RES_1080P2997);
            break;

        case 33: // 1920*1080p @25Hz 16:9
            resMask = (1 << RES_1080P25);
            break;

        case 32: // 1920*1080p @23.97/24Hz 16:9
            resMask = (1 << RES_1080P24)
                    | (1 << RES_1080P2398);
            break;

        case 16: // 1920*1080p @59.94/60Hz 16:9
            resMask = (1 << RES_1080P60)
                    | (1 << RES_1080P5994);
            break;

        case 31: // 1920*1080p @50Hz 16:9
            resMask = (1 << RES_1080P50);
            break;

        default:
            break;
    }

    if (*pVideoRes & resMask) {
        *pNewRes = FALSE;
    }
    else {
        *pNewRes = TRUE;
        *pVideoRes |= resMask;
    }
    *pVideoRes |= resMask;
    return (MV_VPP_OK);
}

static int HdmiTxMapVICTo3DResID(UINT32 videoIDCode, UINT32 *p3DVideoRes, UINT32 i3DStructure, UINT32 *p3DStructures)
{
    UINT32 resMask = 0;

    // VideoIDCode from CEA Spec - If any other resolutions are to
    // be handled, it should be added here
    switch (videoIDCode)
    {
        case 1: // 640x480p  @59.94/60Hz 4:3
        case 6: // 1440*480i  @59.94/60Hz 4:3
        case 7: // 1440*480i  @59.94/60Hz 16:9
        case 21: // 1440*576i  @50Hz 4:3
        case 22: // 1440*576i  @50Hz 16:9
        case 2: // 720*480p   @59.94/60Hz 4:3
        case 3: // 720*480p   @59.94/60Hz 16:9
        case 14: // 1440*480p  @59.94/60Hz 4:3
        case 15: // 1440*480p  @59.94/60Hz 16:9
        case 35: // 2880*480p  @59.94/60Hz 4:3
        case 36: // 2880*480p  @59.94/60Hz 16:9
        case 17: // 720*576p   @50Hz 4:3
        case 18: // 720*576p   @50Hz 16:9
        case 29: // 1440*576p  @50Hz 4:3
        case 30: // 1440*576p  @50Hz 16:9
        case 37: // 2880*576p  @50Hz 4:3
        case 38: // 2880*576p  @50Hz 16:9
            break;

        case 4: // 1280*720p  @59.94/60Hz 16:9
            resMask = (1 << (RES_720P60_3D - FIRST_RES_3D))
                    | (1 << (RES_720P5994_3D - FIRST_RES_3D));
            p3DStructures[RES_720P60_3D - FIRST_RES_3D] |= i3DStructure;
            p3DStructures[RES_720P5994_3D - FIRST_RES_3D] |= i3DStructure;
            break;

        case 19: // 1280*720p  @50Hz 16:9
            resMask = (1 << (RES_720P50_3D - FIRST_RES_3D));
            p3DStructures[RES_720P50_3D - FIRST_RES_3D] |= i3DStructure;
            break;

        case 5: // 1920*1080i @59.94/60Hz 16:9
            resMask = (1 << (RES_1080I60_FP - FIRST_RES_3D))
                    | (1 << (RES_1080I5994_FP - FIRST_RES_3D));
            p3DStructures[RES_1080I60_FP - FIRST_RES_3D] |= i3DStructure;
            p3DStructures[RES_1080I5994_FP - FIRST_RES_3D] |= i3DStructure;
            break;

        case 20: // 1920*1080i @50Hz       16:9
            resMask = (1 << (RES_1080I50_FP - FIRST_RES_3D));
            p3DStructures[RES_1080I50_FP - FIRST_RES_3D] |= i3DStructure;
            break;

        case 34: // 1920*1080p @29.97/30Hz 16:9
            resMask = (1 << (RES_1080P30_3D - FIRST_RES_3D))
                    | (1 << (RES_1080P2997_3D - FIRST_RES_3D));
            p3DStructures[RES_1080P30_3D - FIRST_RES_3D] |= i3DStructure;
            p3DStructures[RES_1080P2997_3D - FIRST_RES_3D] |= i3DStructure;
            break;

        case 33: // 1920*1080p @25Hz 16:9
            resMask = (1 << (RES_1080P25_3D - FIRST_RES_3D));
            p3DStructures[RES_1080P25_3D - FIRST_RES_3D] |= i3DStructure;
            break;
        case 32: // 1920*1080p @23.97/24Hz 16:9
            resMask = (1 << (RES_1080P24_3D - FIRST_RES_3D))
                    | (1 << (RES_1080P2398_3D - FIRST_RES_3D));
            p3DStructures[RES_1080P24_3D - FIRST_RES_3D] |= i3DStructure;
            p3DStructures[RES_1080P2398_3D - FIRST_RES_3D] |= i3DStructure;
            break;

        default:
            break;
    }

    *p3DVideoRes |= resMask;

    return (MV_VPP_OK);
}

int HdmiTxMapVICTo4KResID(UINT32 videoIDCode, UINT32 *p4KVideoRes )
{
    UINT32 resMask = 0;
    switch (videoIDCode)
    {
        case 1:
            resMask = (1 << (RES_4Kx2K2997 - RES_MIN_4Kx2K))
                    | (1 << (RES_4Kx2K30 - RES_MIN_4Kx2K));
            break;
        case 2:
            resMask = (1 << (RES_4Kx2K25 - RES_MIN_4Kx2K));
           break;
        case 3:
            resMask = (1 << (RES_4Kx2K2398 -RES_MIN_4Kx2K ))
                    | (1 << (RES_4Kx2K24 - RES_MIN_4Kx2K));
           break;
        case 4:
            resMask = (1 << (RES_4Kx2K24_SMPTE - RES_MIN_4Kx2K));
           break;

        default:
            break;
    }
    *p4KVideoRes |= resMask;
    return (MV_VPP_OK);
}

static int HdmiTxGetLatencyInfo (UINT8 latency, VPP_EDID_LATENCY_STS *pLatencySts, UINT32 *pLatencyVal)
{
    if (latency == 0)
    {
        *pLatencySts = VPP_EDID_LATENCY_STS_INVALID;
    }
    else if (latency == 255)
    {
        *pLatencySts = VPP_EDID_LATENCY_STS_UNSUPP_AV;
    }
    else
    {
        *pLatencySts = VPP_EDID_LATENCY_STS_VALID;
        *pLatencyVal = (latency - 1) * 2;
    }
    return (MV_VPP_OK);
}

int VPP_BE_TASK_Reset_Sem(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    MV_OSAL_Sem_Post(pHdmiTx->hdcpCmdFifo.fifoSem);
#ifdef CONFIG_AMP_TEE_ENABLE
    MV_OSAL_Sem_Post(pHdmiTx->hpdcmdFifo.fifoSem);
#endif
  //  VPP_BE_CEC_Reset_SemSts(vpp_obj);
    return (MV_VPP_OK);
}

int HdmiTxHpdTask (void *arg)
{
    BE_HDMITX_DATA  *pHdmiTx;
    HDMITX_CMD      cmd;
    VPP_OBJ         *vpp_obj = (VPP_OBJ*)arg;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
#ifndef CONFIG_AMP_TEE_ENABLE
    while (vpp_obj->be.hdmihpdTaskExit == FALSE)
#endif
    {
        HdmiFifoWaitForSignal (&pHdmiTx->hpdcmdFifo);
        while (HdmiFifoPop (&pHdmiTx->hpdcmdFifo, (UINT8*)&cmd)) {
            switch (cmd.cmdID)
            {
                case HDMITX_CMD_HPD:
                    {
                        if (cmd.param == FALSE) {
                            /* reset all previous commands */
                            HdmiFifoReset (&pHdmiTx->hdcpCmdFifo);
                            HdmiFifoReset (&pHdmiTx->hpdcmdFifo);
                            HdmiTxTimerCmdCfg (vpp_obj, 0);
                        }
                        VPPHDMI_DBG_INFO("HdmiTxHandleHotPlug called from HPD Task....\r\n");
                        pHdmiTx->rxConnected = HdmiTxHandleHotPlug (vpp_obj, cmd.param);
                        BE_HDMITX_UpdateHPDStateChange(pHdmiTx, pHdmiTx->rxConnected);
                    }
                    break;
                case HDMITX_CMD_FORCE_HPD:
                    {
                        /* Force HPD: make HPD Low and then HIGH */
                        pHdmiTx->rxConnected = HdmiTxHandleHotPlug (vpp_obj, FALSE);
                        BE_HDMITX_UpdateHPDStateChange(pHdmiTx, pHdmiTx->rxConnected);

                        pHdmiTx->rxConnected = HdmiTxHandleHotPlug (vpp_obj, TRUE);
                        BE_HDMITX_UpdateHPDStateChange(pHdmiTx, pHdmiTx->rxConnected);
                    }
                    break;
                default:
                    break;
            }
        }
    }

#ifndef CONFIG_AMP_TEE_ENABLE
    // Exit Task
    MV_OSAL_Task_Exit(NULL);
#endif

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: HDMI Tx Task to handle HPD, HDCP interrupts
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int HdmiTxHdcpTask (void *arg)
{
    /* use this as hdcp task as its not used for any other functionality*/
    int             retVal;
    BE_HDMITX_DATA  *pHdmiTx;
    HDMITX_CMD      cmd;
    HDMITX_CMD      pushCmd;
    VPP_OBJ         *vpp_obj = (VPP_OBJ*)arg;
    VPP_BE_HDCP14_INTR_STAT hdcp14_intr_stat;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

#ifndef CONFIG_AMP_TEE_ENABLE
    while (vpp_obj->be.hdmiHdcpTaskExit == FALSE)
#endif
    {
        HdmiFifoWaitForSignal (&pHdmiTx->hdcpCmdFifo);
        while (HdmiFifoPop (&pHdmiTx->hdcpCmdFifo, (UINT8*)&cmd)) {
        switch (cmd.cmdID)
        {
            case HDMITX_CMD_HDCP_INTERRUPT:
            {
                hdcp14_intr_stat = BE_HDMITX_HDCP14_InterruptHandler(pHdmiTx);
                if (HDCP14_AUTH_FAIL == hdcp14_intr_stat) {
                    VPPHDMI_DBG_ERROR("restarting authentication\n");
                    if (pHdmiTx->sm_state == HDCP_SM_STATE_AUTHENTICATED) {
                        HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                         MV_VPP_EVENT_HDCP_AUTH_ERR | MV_VPP_EVENT_HDCP_RI_ERR);
                        BE_HDMITX_HDCP14_InterruptEnable(pHdmiTx, HDCP14_INTR_MASK_AUTH_FAIL, false);
                    } else if (pHdmiTx->hdcp14ErrCnt++ == pHdmiTx->hdmiConfig.errNotifRetryCnt1x) {
                        VPPHDMI_DBG_ERROR("disable intr & trigger HDCP1.4 auth fail notification\n");
                        HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                         MV_VPP_EVENT_HDCP_AUTH_ERR | MV_VPP_EVENT_HDCP_1X_AUTH_FAIL);
                        BE_HDMITX_HDCP14_InterruptEnable(pHdmiTx, HDCP14_INTR_MASK_AUTH_FAIL, false);
                    }
                    HdmiTxSetHdcpSMState(pHdmiTx, HDCP_SM_STATE_TRIGGERED);
                    /* TODO: add notification here if required */
                    /* Core will do auto restart of authenitcation for HDCP1.4. Add manual if required. */
                } else if (HDCP14_AUTH_KSV_FAIL == hdcp14_intr_stat) {
                    /* no need to retry for sending notification for ksv error as non-recoverable */
                    HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                     MV_VPP_EVENT_HDCP_AUTH_ERR | MV_VPP_EVENT_HDCP_1X_AUTH_FAIL);
                } else if (HDCP14_AUTH_COMM_ERR == hdcp14_intr_stat) {
                    if (pHdmiTx->hdcpDDCErrCnt++ == pHdmiTx->hdmiConfig.errNotifRetryCnt1x){
                        HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                         MV_VPP_EVENT_HDCP_AUTH_ERR | MV_VPP_EVENT_HDCP_I2C_ERROR);
                        BE_HDMITX_HDCP14_InterruptEnable(pHdmiTx, HDCP14_INTR_MASK_I2C_FAIL, false);
                    }
                    HdmiTxSetHdcpSMState(pHdmiTx, HDCP_SM_STATE_TRIGGERED);
                    /* TODO:Stop auth, add EDDC reset here or check for hpd status before retriggering auth.*/
                } else if (HDCP14_AUTH_PASS == hdcp14_intr_stat) {
                    HdmiTxSetHdcpSMState(pHdmiTx, HDCP_SM_STATE_AUTHENTICATED);
                    VPPHDMI_DBG_INFO("hdcp1.4 authenitcation pass.. unmute video. (Receiver Done) \n");
                    BE_HDMITX_EnableHdcpFrameProt(pHdmiTx,TRUE);
                    VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, FALSE);
                    HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                     (MV_VPP_EVENT_HDCP_AUTH_SUCCESS|MV_VPP_EVENT_HDCP_SRC_VERSION_1_X));
                    /* unmask hdcp failure interrupt */
                    BE_HDMITX_HDCP14_InterruptEnable(pHdmiTx, HDCP14_INTR_MASK_AUTH_FAIL|HDCP14_INTR_MASK_I2C_FAIL, true);
                }

                if (hdcp14_intr_stat != HDCP14_AUTH_PASS) {
                    if(pHdmiTx->encBypassOnHdcpFail) {
                        VPPHDMI_DBG_INFO("bypass encryption on hdcp1.4 failure as content blocking handled using sa_attr or upstream device\n");
                        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, FALSE);
                        BE_HDMITX_EnableHdcpFrameProt(pHdmiTx,FALSE);
                    } else {
                        /*enable GCP here as well as color screen will be send
                          by core in case of HDCP failure */
                        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
                        BE_HDMITX_EnableHdcpFrameProt(pHdmiTx, TRUE);
                    }
                }
            }
            break;

            case HDMITX_CMD_VERSION_RD:
            {
                HdmiGetRxHdcpVersion(vpp_obj);
                if(pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_NONE) {
                    VPPHDMI_DBG_INFO("Hdcp version read failed .. continue with HDCP1.4 \n");
                    pHdmiTx->SessionMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
                }

                if((pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x) ||
                   (pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x)) {
                    VPPHDMI_DBG_INFO("Hdcp version set to 1.4 \n");
                    pushCmd.cmdID = HDMITX_CMD_HDCP14_START;
                }
                else if((pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_AUTO) &&
                        (pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2)) {
                    pushCmd.cmdID = HDMITX_CMD_HDCP2X_START;
                }
                else if(pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) {
                    if(pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) {
                        pushCmd.cmdID = HDMITX_CMD_HDCP2X_START;
                    } else {
                        pushCmd.cmdID = HDMITX_CMD_NOTIFY_ERR;
                        //cmd.param = HDCP_VERSION_UNSUPPORTED;       //TODO: add code
                    }
                }
                else if(pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_AUTO) {
                    VPPHDMI_DBG_INFO("Hdcp version set to 1.4 \n");
                    pushCmd.cmdID = HDMITX_CMD_HDCP14_START;
                } else {
                    VPPHDMI_DBG_ERROR("invalid selection : selected hdcp version : %d . max supported for this session: %d \n",
                            pHdmiTx->requestedHdcpVersion, pHdmiTx->SessionMaxSuprtedhdcpVersion);
                    break;
                }
                pushCmd.param = 0;
                HdmiFifoPush (&pHdmiTx->hdcpCmdFifo, (UINT8*)&pushCmd, FALSE);
                VPP_BE_SET_BIT(vpp_obj->be.hdmiFrmIntrTrig,VPP_BE_HDMI_SIG_FIFO_FRMTRIG);
            }
            break;

            case HDMITX_CMD_HDCP14_START:
            {
                if ((pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) ||
                    (pHdmiTx->requestedHdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_3)) {
                    VPPHDMI_DBG_INFO("hdcp2.2 requested fallback to hdcp1.4 as sink don't support\n");
                    HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                     MV_VPP_EVENT_HDCP_AUTH_ERR|MV_VPP_EVENT_HDCP_SINKCAP_ERR);
                } else {
                    VPPHDMI_DBG_INFO("hdcp1.4 start request received\n");
                }
                HdmiTxTimerCmdCfg (vpp_obj,0);
                HdmiTxSetHdcpSMState(pHdmiTx, HDCP_SM_STATE_TRIGGERED);
                BE_HDMITX_SetHdcpVersion(pHdmiTx, VPP_HDMI_HDCP_VER_HDCP_1_x);
                VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE); /*resend AVMute to aviod flash */

                if (pHdmiTx->authRequired) {
                    pHdmiTx->hdcp14ErrCnt = 0;
                    pHdmiTx->hdcpDDCErrCnt = 0;
                    retVal = BE_HDMITX_StartHdcpAuthentication(pHdmiTx);
                    if (retVal != MV_VPP_OK) {
                        VPPHDMI_DBG_INFO("start hdcp: error [0x%x]..retrying\n", retVal);
                    }
                } else {
                    VPPHDMI_DBG_ERROR("unexpected state: hdcp14 start with auth disabled.....\n");
                }
                pHdmiTx->timerCmdCntr = 0;
            }
            break;

#if HDCP2X_ENABLED
            case HDMITX_CMD_HDCP2X_START:   /*HDCP 2.2. chnages*/
            {
                BE_HDMITX_SetHdcpVersion(pHdmiTx, VPP_HDMI_HDCP_VER_HDCP_2_2);
                HdmiTxSetHdcpSMState(pHdmiTx, HDCP_SM_STATE_TRIGGERED);
                VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_VIDEO, TRUE);
                VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_AUDIO, TRUE);

                if(pHdmiTx->authRequired) {
                    HdmiTxTimerCmdCfg (vpp_obj, 0);
                    retVal = BE_HDMITX_StartHdcpAuthentication(pHdmiTx);
                    if (retVal != MV_VPP_OK) {
                        if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
                            VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_GCP, TRUE); /* to aviod fixed color from esm */
                            if (pHdmiTx->hdcp2xDSEarlyAuthStarted) {
                                VPPHDMI_DBG_ERROR("toggle US HPD as DS auth start failed %d\n", pHdmiTx->sm_state);
                                HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2, MV_VPP_EVENT_HDCP_RP_REQ_UPSTM_TOGGLE);
                                /* retry happens when triggered from US*/
                                pHdmiTx->hdcp2xDSEarlyAuthStarted = false;
                            } else {
                                /* auth failed when triggerd from US, let it retry and fallback to 1.4 if required*/
                                HdcpFallbackCheck (vpp_obj);
                                HdmiTxTimerCmdCfg (vpp_obj, HDCP2X_RETRY_TIMEOUT);
                            }
                        } else {
                            VPPHDMI_DBG_ERROR("HDCP 2.2 authentication failed. restart authentication. %d\n", pHdmiTx->sm_state);
                            HdcpFallbackCheck (vpp_obj);
                            HdmiTxTimerCmdCfg (vpp_obj, HDCP2X_RETRY_TIMEOUT);
                        }
                    } else {
                        VPPHDMI_DBG_ERROR("start hdcp2.2 authentication status monitor\n");
                        HdmiTxTimerCmdCfg (vpp_obj, HDCP2X_STATCHECK_START_TIMEOUT);
                    }
                }
            }
            break;

            case HDMITX_CMD_HDCP2X_MONITOR:
            {
                HDMITX_HDCP_RETURN_STATE ret_auth_state;
                HdmiTxTimerCmdCfg (vpp_obj, 0);
                ret_auth_state = BE_HDMITX_CheckHdcp2xAuthStatus(pHdmiTx);

                switch (ret_auth_state) {

                case HDCP22_AUTH_DONE:
                    VPPHDMI_DBG_INFO("HDCP 2.2 Tx: AUTHENICATION PASS . SENDING ENCRYPTED DATA\n");
                    pHdmiTx->hdcp2xRetryCount = 0;
                    VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, FALSE);
                    HdmiTxTimerCmdCfg (vpp_obj, VPP_BE_HDCP_STAT_CHECK_TIMER);  /*link integrity check*/
                    HdmiTxSetHdcpSMState (pHdmiTx, HDCP_SM_STATE_AUTHENTICATED);
                    HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                    (MV_VPP_EVENT_HDCP_AUTH_SUCCESS | MV_VPP_EVENT_HDCP_SRC_VERSION_2_X));
                    if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
                        if (pHdmiTx->hdcp2xDSEarlyAuthStarted) {
                            VPPHDMI_DBG_ERROR("toggle US HPD as DS auth passed\n");
                            HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2, MV_VPP_EVENT_HDCP_RP_REQ_UPSTM_TOGGLE);
                            pHdmiTx->hdcp2xDSEarlyAuthStarted = false;
                        }
                    }
                break;

                case HDCP22_AUTH_INPROGESS:
                    HdmiTxTimerCmdCfg (vpp_obj, VPP_BE_HDCP_STAT_CHECK_TIMER);  /*timeout in esm, no new message or waiting*/
                break;

                case HDCP22_AUTH_RETRY:
                    if ((pHdmiTx->hdcp2xErrCnt++ == pHdmiTx->hdmiConfig.errNotifRetryCnt2x) &&
                        (pHdmiTx->hdmiConfig.hdcpFallbackOn2xAuthFailure == false)) {
                        HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                         MV_VPP_EVENT_HDCP_AUTH_ERR | MV_VPP_EVENT_HDCP_2X_AUTH_FAIL);
                    }
                    if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
                        if (pHdmiTx->hdcp2xDSEarlyAuthStarted) {
                            HdmiTxTimerCmdCfg (vpp_obj, 0);
                            VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
                            HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2, MV_VPP_EVENT_HDCP_RP_REQ_UPSTM_TOGGLE);
                            VPPHDMI_DBG_INFO("initial hdcp auth attempt failed, wait for upstream to restart auth:\n");
                            pHdmiTx->hdcp2xDSEarlyAuthStarted = false;
                        } else {
                            VPPHDMI_DBG_INFO("hdcp2.2 DS authentication error. retrying...\n");
                            HdmiTxTimerCmdCfg (vpp_obj, VPP_BE_HDCP_RETRY_CHECK_TIMER);
                            VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
                            HdcpFallbackCheck(vpp_obj);
                        }
                    } else {
                        VPPHDMI_DBG_INFO("hdcp2.2 authentication error. retrying...\n");
                        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);
                        HdcpFallbackCheck(vpp_obj);
                        HdmiTxTimerCmdCfg (vpp_obj, VPP_BE_HDCP_RETRY_CHECK_TIMER);
                    }
                break;

                case HDCP22_AUTH_LINK_INTEG:
                    VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);

                    if ((pHdmiTx->hdmiConfig.enableDynFPSSwitch) && (pHdmiTx->videoParams.DispRes == RES_4Kx2K5994)) {
                        /* fractional mode for 4K 59.94 have issues on certain TV models.
                           force 4K60 for the same via force hpd (for rptr HPD will handle upstrm restart)*/
                        VPPHDMI_DBG_INFO("add TV model to blacklist\n");
                        VPP_BE_BlacklistCurrentSink(vpp_obj);
                    } else {
                        if (pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
                            VPPHDMI_DBG_INFO("link integrity error on repeater mode. halt and notify upstream...\n");
                            HdcpNotifyEvent(vpp_obj, pHdmiTx->eventCode2, MV_VPP_EVENT_HDCP_DS_REAUTH_REQ);
                        } else {
                            VPPHDMI_DBG_INFO("hdcp2.2 link integrity error. retrying...\n");
                            HdcpNotifyEvent (vpp_obj, pHdmiTx->eventCode2,
                                             MV_VPP_EVENT_HDCP_AUTH_ERR | MV_VPP_EVENT_HDCP_REAUTH_REQ);
                            HdmiTxTimerCmdCfg (vpp_obj, VPP_BE_HDCP_LINKINTEG_RETRY_TIMER);
                        }
                    }
                break;

                case HDCP_SET_IDLE_STATE:
                    VPPHDMI_DBG_INFO("sink asserted hpd low. set state to idle\n");
                break;

                case HDCP14_AUTH_RETRY:
                    VPPHDMI_DBG_INFO("request to fallback to hdcp1.4 \n");
                    BE_HDMITX_TriggerHdcpFallback(pHdmiTx);
                    VPPHDMI_DBG_INFO("setting audio fmt after tmds reset\n");
                    VPP_BE_SetHdmiAudioFmt(vpp_obj, true);
                    HdmiTxTimerCmdCfg (vpp_obj, VPP_BE_HDCP_STAT_CHECK_TIMER);
                break;
                default:
                break;
                }
            }
            break;
#endif

            case HDMITX_CMD_NOTIFY_ERR:
            {
                //TODO: enable this notification once the eventcodes are defined.
                //HdmiTxNotifyEvent (vpp_obj, pHdmiTx->eventCode, MV_VPP_EVENT_HDMI_HDCP_SINK_NOTCAPABLE);
            }
            break;

            case HDMITX_CMD_AUDIO_CONFIG:
            {
                VPP_BE_SetHdmiAudioFmt(vpp_obj, true);
            }
            break;

            default:
                break;
        }
        }
    }

#ifndef CONFIG_AMP_TEE_ENABLE
    // Exit Task
    MV_OSAL_Task_Exit(NULL);
#endif

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: HDMI Tx hot plug detection polling timer
 * PARAMS:   *param - pointer to VPP object
 * RETURN:   MV_VPP_OK
 ******************************************************************/
void HdmiTxHdcpTimerCb(void *param)
{
    BE_HDMITX_DATA  *pHdmiTx;
    HDMITX_CMD      cmd;
    HDCP_ENGAGE_STATE  hdcp_state;
#ifdef TZ_3_0
    g_vpp = (VPP*) param;
    VPP_OBJ         *vpp_obj = g_vpp->vpp_objs[0];
#else
    VPP_OBJ         *vpp_obj = (VPP_OBJ*)param;
#endif
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx != NULL)
    {
        if (pHdmiTx->timerCmdCfg != 0)
        {
           pHdmiTx->timerCmdCntr = HDMITX_INC_TIMER_CMD_CNTR(pHdmiTx->timerCmdCfg,pHdmiTx->timerCmdCntr);
           if (pHdmiTx->timerCmdCntr == pHdmiTx->timerCmdCfg)
           {
               hdcp_state = BE_HDMITX_GetActiveHdcpState(pHdmiTx);
               if ((hdcp_state == HDCP_STATE_2_2_AUTHENTICATED_RX) ||
                   (hdcp_state == HDCP_STATE_2_2_AUTHENTICATED_RPTR) ||
                   (hdcp_state == HDCP_STATE_2_2_IN_PROGRESS)) {
                   cmd.cmdID = HDMITX_CMD_HDCP2X_MONITOR;
               } else if(hdcp_state == HDCP_STATE_2_2_ERR_REAUTH) {
                   cmd.cmdID = HDMITX_CMD_HDCP2X_START;
               } else if(hdcp_state == HDCP_STATE_1_4_IN_PROGRESS) {
                   cmd.cmdID = HDMITX_CMD_HDCP14_START;
               } else {
                   VPPHDMI_DBG_ERROR("invalid state in hdcp request:%d : %d\n", hdcp_state, pHdmiTx->timerCmdCfg);
                   pHdmiTx->timerCmdCfg = 0;
                   return;
               }
               cmd.param = 0;
               HdmiFifoPush (&pHdmiTx->hdcpCmdFifo, (UINT8*)&cmd, TRUE);
           }
       }
    }
}

#if HDCP2X_ENABLED
int VPP_BE_UpdateContentStrmInfo(VPP_OBJ *vpp_obj, HDCP2X_STEAM_TYPE vpp_sa_hdcp_content_type)
{
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    /* TODO:add blocking based on downstream type */
    if (pHdmiTx == NULL)
        return (MV_VPP_EUNCONFIG);
    return BE_HDMITX_UpdateContentStrmInfo(pHdmiTx, vpp_sa_hdcp_content_type);
}
#endif

int VPP_BE_GetHdmiPowerStatus(VPP_OBJ *vpp_obj, int *pPowerState)
{
    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    return BE_HDMITX_GetHdmiPowerStatus(pHdmiTx, pPowerState);
}

bool VPP_BE_IsHDROutput(VPP_OBJ *vpp_obj)
{
    if((vpp_obj->m_outformat == VPP_OUTFORMAT_HDR) || \
        (vpp_obj->m_outformat == VPP_OUTFORMAT_HLG) || \
        (vpp_obj->m_outformat == VPP_OUTFORMAT_HDR10PLUS))
        return true;
    else
        return false;
}

INT VPP_BE_SetGammaCorrection(VPP_OBJ *vpp_obj, INT Channel, INT GmMode)
{
    VPP_GAMMA_LUT_PARAMS  GammaLUTParams;

    GammaLUTParams.ModeId = GmMode;
    return VPP_GAMMA_LoadLUT(vpp_obj, Channel, &GammaLUTParams);
}

INT VPP_BE_SetGammaEnable(VPP_OBJ *vpp_obj, INT Channel, INT Enable)
{
    VPP_GAMMA_Enable(vpp_obj, Channel,Enable);

    return (MV_VPP_OK);
}

int VPP_BE_PrintGamma(VPP_OBJ *vpp_obj, int chanID,INT location )
{
    INT16 gammaVal[6] = { -1, -1, -1, -1 , -1 , -1 };

    VPP_Gamma_ReadValue(vpp_obj, chanID, location, gammaVal);
    VPPAPI_DBG_INFO("Gamma Values B0 = %d,B1 = %d,G0 =%d G1 = %d,R0 = %d,R1 = %d \n"
        , gammaVal[0], gammaVal[1], gammaVal[2], gammaVal[3], gammaVal[4], gammaVal[5]);

    return (MV_VPP_OK);
}

int VPP_BE_SetGamma(VPP_OBJ *vpp_obj, int chanID,INT gmMode)
{
    vpp_obj->chan[chanID].gmMode = gmMode;
    VPP_Gamma_LoadRegister(vpp_obj, chanID, gmMode);

    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Configures the PHY state as set by the app/disp.
             Tracks the state set by app against the phy states set within the driver
 * PARAMS:   *vpp_obj - pointer to VPP object
             enable - Expected state 0-off 1-on
 * RETURN:   MV_VPP_OK
 ******************************************************************/

int VPP_BE_ConfigHdmiPhyStateChange(VPP_OBJ *vpp_obj, BOOL enable)
{
    /* call this from upper layer before setting the phy state if the values set have to be retained.
     * i.e not to wakeup through internal seq if in suspend state by app */

    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pHdmiTx->phyStateFromApp == enable) {
        VPPHDMI_DBG_ERROR("Tx phy already in state %d. not resetting to same state \n", enable);
        return MV_VPP_EBADCALL;
    }

    pHdmiTx->phyStateFromApp = enable;

    /* add any changes required before going to PHY off/on here */
    return (MV_VPP_OK);
}

/*******************************************************************
 * FUNCTION: Returns the PHY state as set by the app/disp.
             Tracks the state set by app against the phy states set within the driver
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN:   Expected state 0-off 1-on
 ******************************************************************/

BOOL VPP_BE_GetHdmiPhyExpectedState(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    return pHdmiTx->phyStateFromApp;
}

/*******************************************************************
 * FUNCTION: Asynchronous call to set PHY state.
 * PARAMS:   *vpp_obj - pointer to VPP object
             state - Expected state 0-off 1-on
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int VPP_BE_SetHdmiPhyPower(VPP_OBJ *vpp_obj, BOOL state)
{
    CMD cmd;
    int cpcbID;
    int params[2];
    HDMITX_HDCP_SM_STATE curr_hdcp_state = HDCP_SM_STATE_IDLE;

    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if((state == TRUE) && (pHdmiTx->phyStateFromApp == FALSE)) {
        VPPHDMI_DBG_ERROR("ignoring wakeup request when HDMI is set to standby by display service\n");
        return MV_VPP_EBADCALL;
    }

    if(state == FALSE) {
        curr_hdcp_state = pHdmiTx->sm_state;
        if (!BE_HDMITX_phy_getState(pHdmiTx)) {
            VPPHDMI_DBG_INFO("HDMI Tx PHY already in standby state..\n");
            return MV_VPP_OK;
        }
        VPP_BE_SetHdmiMute (vpp_obj, VPP_HDMI_AV_PORT_ALL, TRUE);

        DelayInMS(0, FRAME_DELAY); /* make sure sink receives avmute */
        HdmiTxEnableEncryption (vpp_obj, FALSE);
        VPPHDMI_DBG_INFO("disable encryption before going to standby.. \n");
        if(curr_hdcp_state == HDCP_SM_STATE_AUTHENTICATED) {
            DelayInMS(0, 2*FRAME_DELAY); /* make sure sink receives atleast two unencrypted frames */
        }
        BE_HDMITX_phy_standby(pHdmiTx);
    } else {
        if ((cpcbID = CPCB_OF_VOUT(vpp_obj, VOUT_HDMI)) == CPCB_INVALID) {
           /* Output is not connected to any DV in configuration */
           return (MV_VPP_EUNSUPPORT);
        }

        params[0] = VPP_HDMI_FORCE_HDMI_HOTPLUG;
        make_cmd(&cmd, (FUNCPTR)VPP_SetHdmiCmd, 1, params);

        MV_OSAL_Mutex_Lock(vpp_obj->dv[cpcbID].cmdq_lock);
        ISSUE_CMD(&vpp_obj->dv[cpcbID].cmdq, &cmd);

        /* commit commands */
        cmdq_commit(&vpp_obj->dv[cpcbID].cmdq);

        MV_OSAL_Mutex_Unlock(vpp_obj->dv[cpcbID].cmdq_lock);
    }


    return (MV_VPP_OK);
}

int VPP_BE_GetHdmiIrqSource(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if(pHdmiTx == NULL)
        return (MV_VPP_EUNCONFIG);

    return BE_HDMITX_GetIrqSource(pHdmiTx);
}

int VPP_BE_HDMI_ClearSCDCReservedBit(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA  *pHdmiTx;
    if(vpp_obj == NULL)
        return (MV_VPP_ENODEV);
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if(pHdmiTx == NULL)
        return (MV_VPP_EUNCONFIG);

    UINT32  Addr = SCDC_OFFSET_TMDS_CONFIG;
    UINT16  Length = 1;
    UINT8   DataBuf;
    SCDC_TX_DATA scdcTxdata;
    int     retVal = MV_VPP_OK;

    retVal = VPP_BE_SCDC_GetSinkVersion(vpp_obj, &scdcTxdata);
    if(retVal == MV_VPP_OK) {
        if(scdcTxdata.SinkVersion == 0x1) {
            //For LG TV will clear bit 7 (reserved bit) of offset 0x20 (TMDS CNTRL CONFIG) to clear the pop up JIRA:6953
            retVal = SCDC_I2C_regRead(vpp_obj, Addr, &DataBuf, Length);
            if (retVal != MV_VPP_OK) {
                VPPHDMI_DBG_ERROR("SCDC: Read failed for address %x\n", Addr);
                return retVal;
            }
            //Mask other bits and set bit 7 to zero
            DataBuf = (DataBuf & 0x7F);
            retVal = SCDC_I2C_regWrite(vpp_obj, Addr, &DataBuf, Length);
            if (retVal != MV_VPP_OK) {
                VPPHDMI_DBG_ERROR("SCDC: Write failed for address %x\n", Addr);
                return retVal;
            }
        } else {
            VPPHDMI_DBG_INFO("SCDC: SinkVersion = %x\n", scdcTxdata.SinkVersion);
        }
    } else {
        VPPHDMI_DBG_ERROR("SCDC: Get sink version failure\n");
    }
    return retVal;
}

int VPP_BE_HDMI_ScramblingEnable(VPP_OBJ *vpp_obj, BOOL enable)
{
    BE_HDMITX_DATA  *pHdmiTx;
    if(vpp_obj == NULL)
        return (MV_VPP_ENODEV);
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if(pHdmiTx == NULL)
        return (MV_VPP_EUNCONFIG);

    return BE_HDMITX_ScramblingEnable(pHdmiTx, enable);
}

int VPP_BE_SetHdmiPhy(VPP_OBJ *vpp_obj, unsigned int * phySetting, unsigned int PhyBufSize)
{
    BE_HDMITX_DATA  *pHdmiTx;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if(pHdmiTx == NULL)
        return (MV_VPP_EUNCONFIG);

    return BE_HDMITX_SetHdmiPhy(pHdmiTx, (unsigned char*)phySetting, PhyBufSize);
}

int VPP_BE_SetHDMIConfig(VPP_OBJ *vpp_obj,
                            HDMI_CONFIG *config,
                            unsigned int bufSize)
{
    BE_HDMITX_DATA  *pHdmiTx;
    if((vpp_obj == NULL) || (vpp_obj->be.hdmiTx == NULL))
        return (MV_VPP_ENODEV);

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (!pHdmiTx->hdmiConfig.isUpdated) {
#if HDCP_ENABLED
        VPPAPI_DBG_INFO("HDCP is enabled in current profile..\n");
#else
        VPPAPI_DBG_INFO("HDCP is disabled in current profile..\n");
#endif

#if !defined (HDCP2X_ENABLED)
        /* Disable mutePlaneOnHdcpFailure if HDCP is not supported */
        config->mutePlaneOnHdcpFailure = false;
#endif
        VPPAPI_DBG_INFO("updating hdmi config:sinkBasedContentType:%d mutePlaneOnHdcpFailure:%d \n \
                         rxSensEnable:%d restrictUhdInHdcp14:%d enableDynFPSSwitch:%d \n \
                         hdcpFallbackDwnstrm1_4:%d hdcpFallbackOn2xAuthFailure:%d \n \
                         errNotifRetryCnt1x:%d errNotifRetryCnt2x:%d\n",
                          config->sinkBasedContentType,
                          config->mutePlaneOnHdcpFailure,
                          config->enableRxsenseCheck,
                          config->restrictUhdInHdcp14,
                          config->enableDynFPSSwitch,
                          config->hdcpFallbackDwnstrm1_4,
                          config->hdcpFallbackOn2xAuthFailure,
                          config->errNotifRetryCnt1x,
                          config->errNotifRetryCnt2x);

        if ((config->hdcpFallbackOn2xAuthFailure == true) &&
            (config->errNotifRetryCnt2x > HDCP1X_FALLBACK_THRES)){
            config->errNotifRetryCnt2x = HDCP1X_FALLBACK_THRES;
        }
        config->errNotifRetryCnt1x = (config->errNotifRetryCnt1x)?config->errNotifRetryCnt1x:HDCP_NOTIF_RETRY_CNT;
        config->errNotifRetryCnt2x = (config->errNotifRetryCnt2x)?config->errNotifRetryCnt2x:HDCP_NOTIF_RETRY_CNT;

        GaloisMemcpy(&pHdmiTx->hdmiConfig, config, bufSize);
        /*set the bootup values based on xml entry. Setrptrmode should happen after this */
        pHdmiTx->planeControlOnHdcpFail = pHdmiTx->hdmiConfig.mutePlaneOnHdcpFailure;
        pHdmiTx->encBypassOnHdcpFail = (pHdmiTx->hdmiConfig.mutePlaneOnHdcpFailure)?true:false;
        BE_HDMITX_EnableRxSense(pHdmiTx, ((HDMI_CONFIG *)config)->enableRxsenseCheck);
    } else {
        VPPAPI_DBG_INFO("skip update as config set already\n");
    }
    return (MV_VPP_OK);
}

int VPP_BE_Resume(VPP_OBJ *vpp_obj)
{
    if ((vpp_obj == NULL) || (vpp_obj->be.hdmiTx == NULL))
        return (MV_VPP_ENODEV);

    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    VPP_HDCP_KEYLOAD_LoadKey(vpp_obj, VPP_HDCP_KEYSTORE_MAX);
    return BE_HDMITX_HDCP2X_Resume(pHdmiTx);
}

int VPP_BE_Suspend(VPP_OBJ *vpp_obj)
{
    if ((vpp_obj == NULL) || (vpp_obj->be.hdmiTx == NULL))
        return (MV_VPP_ENODEV);

    BE_HDMITX_DATA  *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    if (pHdmiTx->SourceMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) {
        return BE_HDMITX_HDCP2X_Suspend(pHdmiTx);
    } else {
        return MV_VPP_OK;
    }
}

#if HDCP2X_ENABLED
/* sink specific workarounds to handle hdcp2x link integrity errors */
static int VPP_BE_BlacklistCurrentSink(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA         *pHdmiTx;
    HDMI_SINK_BLACKLIST    *pSinkSpecificConfig;
    UINT32 index = 0;
    bool isModelListed = false;

    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    pSinkSpecificConfig = &pHdmiTx->sinkSpecificConfig;

    isModelListed = HdmiTxCheckSinkInList(&pSinkSpecificConfig->prod_id_59_94_dyn[0][0],
                              pSinkSpecificConfig->count_59_94_dyn,
                              pHdmiTx->edidInfo.mnfrName,
                              pHdmiTx->edidInfo.productCode);
    if(isModelListed) {
        VPPHDMI_DBG_INFO("sink Id already added in dynamic list: %s%s\n",
                              pHdmiTx->edidInfo.mnfrName,
                              pHdmiTx->edidInfo.productCode);
        return MV_VPP_OK;
    }
    pSinkSpecificConfig->bl_option_present = true;
    if(pSinkSpecificConfig->count_59_94_dyn < HDMI_MAX_SINK_COUNT) {
        index = pSinkSpecificConfig->count_59_94_dyn;
        pSinkSpecificConfig->count_59_94_dyn++;
    } else {
            /* currently connected sink is given more priority than xml values */
        index = HDMI_MAX_SINK_COUNT-1;
        VPPHDMI_DBG_INFO("sink ID at index %d overwritten\n");
        pSinkSpecificConfig->count_59_94_dyn = HDMI_MAX_SINK_COUNT;
    }
    VPPHDMI_DBG_INFO("dynamically add sink %s%s to blacklist and toggle HPD : index:%d\n",
                        pHdmiTx->edidInfo.mnfrName, pHdmiTx->edidInfo.productCode, index);
    VPP_Memcpy(pSinkSpecificConfig->prod_id_59_94_dyn[index],
                         pHdmiTx->edidInfo.mnfrName, 3);
    VPP_Memcpy(&pSinkSpecificConfig->prod_id_59_94_dyn[index][3],
                         pHdmiTx->edidInfo.productCode, 4);

    VPP_UpdateDynamicList(pHdmiTx->edidInfo.mnfrName, pHdmiTx->edidInfo.productCode);
    VPP_BE_HdmiTxForceHotplug(vpp_obj);
    return (MV_VPP_OK);
}
#endif /*HDCP2X_ENABLED*/

int VPP_BE_SetEDIDBlackList(VPP_OBJ *vpp_obj,
                            UINT8 *sinkList,
                            unsigned int bufSize)
{
    /* expected format <number of bytes for the issue type (including self), issue_id, <edid1-manufacturer ID 3butes, product ID 2bytes>, <edid2>...,
                       <number of bytes> <issueId> <edid list>, ... */
    UINT8 blockSize;
    UINT8 blockIssueId;
    UINT8 sinkID[HDMI_SINK_ID_SIZE+1];
    UINT8 *prodId;
    UINT8 *countId;
    BE_HDMITX_DATA         *pHdmiTx;
    HDMI_SINK_BLACKLIST    *pSinkSpecificConfig;
    pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);
    pSinkSpecificConfig = &pHdmiTx->sinkSpecificConfig;

    pSinkSpecificConfig->bl_option_present = false;
    pSinkSpecificConfig->count_59_94_fix = 0;

#if defined(ENABLE_BLACKLIST_DEBUG)
    for (int index1 = 0 ; index1 < bufSize; index1++) {
        VPPHDMI_DBG_INFO(" 0x%x\n", sinkList[index1]);
    }
#endif

    sinkID[HDMI_SINK_ID_SIZE] = '\0';
    for (int index1 = 0 ; index1 < bufSize;) {
        blockSize = sinkList[index1];
        blockIssueId = sinkList[index1+1];
        if ((blockSize + index1) > bufSize) {
            VPPHDMI_DBG_ERROR("sink blacklist info not in expected format \n");
            pSinkSpecificConfig->bl_option_present = false;
            return MV_VPP_EBADPARAM;
        }
        VPPHDMI_DBG_INFO("checking for issue ID: 0x%x size :0x%x\n", blockIssueId, blockSize);
        if (blockSize == 0) {
            VPPHDMI_DBG_ERROR("invalid blockSize \n");
            return MV_VPP_EBADPARAM;
        }
        switch (blockIssueId) {
        case SINK_PLL_59_94_FRAC_MODE_XML:
        case SINK_PLL_59_94_FRAC_MODE_DYN:
            prodId  = (blockIssueId == SINK_PLL_59_94_FRAC_MODE_XML)? &pSinkSpecificConfig->prod_id_59_94_fix[0][0] : &pSinkSpecificConfig->prod_id_59_94_dyn[0][0];
            countId = (blockIssueId == SINK_PLL_59_94_FRAC_MODE_XML)? &pSinkSpecificConfig->count_59_94_fix:&pSinkSpecificConfig->count_59_94_dyn;

            VPPAPI_DBG_INFO("parsing list of deviceId for fract pll 4K59.94 case-%d: size : %d - %d:%d\n", blockIssueId,
                             blockSize-VPP_BE_SINK_ID_LIST_HEADER_SIZE,
                             (blockSize-VPP_BE_SINK_ID_LIST_HEADER_SIZE) % HDMI_SINK_ID_SIZE,
                             (blockSize-VPP_BE_SINK_ID_LIST_HEADER_SIZE) / HDMI_SINK_ID_SIZE);

            index1 += VPP_BE_SINK_ID_LIST_HEADER_SIZE;
            if (0 != ((blockSize-VPP_BE_SINK_ID_LIST_HEADER_SIZE) % HDMI_SINK_ID_SIZE)) {
                VPPAPI_DBG_ERROR("sink blacklist info for 59.94 not in expected format \n");
                pSinkSpecificConfig->bl_option_present = false;
                return MV_VPP_EBADPARAM;
            }

            *countId = (blockSize-VPP_BE_SINK_ID_LIST_HEADER_SIZE) / HDMI_SINK_ID_SIZE;
            if (*countId > HDMI_MAX_SINK_COUNT) {
                VPPAPI_DBG_INFO("truncate the list for fract pll 4K59.94 to : %d\n", HDMI_MAX_SINK_COUNT);
                *countId = HDMI_MAX_SINK_COUNT;
            }

            for (int index2 = 0; index2 < *countId; index2++){
                GaloisMemcpy(prodId+(index2*HDMI_SINK_ID_SIZE), &sinkList[index1], HDMI_SINK_ID_SIZE);
                GaloisMemcpy(sinkID, prodId+(index2*HDMI_SINK_ID_SIZE), HDMI_SINK_ID_SIZE);
                VPPAPI_DBG_INFO("blackList ID added: %s\n", sinkID);
                index1 += HDMI_SINK_ID_SIZE;
            }
            pSinkSpecificConfig->bl_option_present = true;
            pSinkSpecificConfig->fix_59_94_active = false;
        break;

        case SINK_DUMMY_MODE_ID:
            VPPHDMI_DBG_INFO("dummy mode check... %d\n", blockSize-2);
            index1 += blockSize;
        break;

        default:
            VPPHDMI_DBG_ERROR("unrecognized error ID(%d), ignore the list\n", blockIssueId);
            pSinkSpecificConfig->bl_option_present = false;
            return MV_VPP_EBADPARAM;
        break;

        }
    }

    return MV_VPP_OK;
}

#if HDCP2X_ENABLED
static bool is420SupportedForResIndex(VPP_OBJ *vpp_obj, int resID)
{
    int index;
    int vicMap;
    BE_HDMITX_DATA* pHdmiTx;
    pHdmiTx = vpp_obj->be.hdmiTx;

    vicMap = ((resID == RES_4Kx2K60)||(resID == RES_4Kx2K5994)) ? 97 : 96;
    for (index = 0; index < 31; index++) {
        if(pHdmiTx->edidInfo.YCbCr420_capmapInfo.VideoIdCode[index] == vicMap) {
            return true;
        }
        if (pHdmiTx->edidInfo.YCbCr420_capmapInfo.VideoIdCode[index] == 0) {
            break;
        }
    }

    for (index = 0; index < 16; index++) {
        if(pHdmiTx->edidInfo.YCbCr420_capInfo.VideoIdCode[index] == vicMap) {
            return true;
        }
        if(pHdmiTx->edidInfo.YCbCr420_capInfo.VideoIdCode[index] == 0) {
            break;
        }
    }

    return false;
}

#endif

void VPP_BE_IsSpecialSink(VPP_OBJ *vpp_obj, int *resIDUpdate, int *bitDepthUpdate, int *colorFmtUpdate)
{
#if HDCP2X_ENABLED
    BE_HDMITX_DATA* pHdmiTx;
    pHdmiTx = vpp_obj->be.hdmiTx;

    if(pHdmiTx->hdmiConfig.restrictUhdInHdcp14 &&
       (pHdmiTx->SessionMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x)) {
        /* xml entry restrictUhdInHdcp14 restricts UHD resolutions in HDCP1.4 TV.
         * 4K60 422/444/RGB -> 4K420 8bit or 4K30 8bit or 1080p as per sink support
         * 4K24/25/30 deepcolor -> 8bit
         * 4K420 deepcolor -> 8bit */
        if(((*resIDUpdate == RES_4Kx2K60) || (*resIDUpdate == RES_4Kx2K5994) || (*resIDUpdate == RES_4Kx2K50)) &&
            (*colorFmtUpdate != OUTPUT_COLOR_FMT_YCBCR420)) {
            if(is420SupportedForResIndex(vpp_obj, *resIDUpdate)) {
                *colorFmtUpdate = OUTPUT_COLOR_FMT_YCBCR420;
                *bitDepthUpdate = OUTPUT_BIT_DEPTH_8BIT;
            } else if (is4K30HdmiVicSupportedForResIndex(pHdmiTx)) {
                *resIDUpdate = RES_4Kx2K30;
                *bitDepthUpdate = OUTPUT_BIT_DEPTH_8BIT;
            } else {
                *resIDUpdate = RES_1080P60;
            }
            VPPAPI_DBG_INFO("restrict 6G resolutions on HDCP1.4 sink: updated to %d:%d:%d\n",
                             *resIDUpdate, *bitDepthUpdate, *colorFmtUpdate);
        } else if((*bitDepthUpdate != OUTPUT_BIT_DEPTH_8BIT) &&
                (*resIDUpdate >= RES_4Kx2K2398 && *resIDUpdate <= RES_4Kx2K30)) {
            VPPAPI_DBG_INFO("restrict 4K30 deep color resolutions on HDCP1.4 sink: updated to %d:%d:%d\n",
                             *resIDUpdate, *bitDepthUpdate, *colorFmtUpdate);
            *bitDepthUpdate = OUTPUT_BIT_DEPTH_8BIT;
        } else if((*bitDepthUpdate != OUTPUT_BIT_DEPTH_8BIT) && (*colorFmtUpdate == OUTPUT_COLOR_FMT_YCBCR420)) {
            *bitDepthUpdate = OUTPUT_BIT_DEPTH_8BIT;
            VPPAPI_DBG_INFO("restrict 420 deep color resolutions on HDCP1.4 sink: updated to %d:%d:%d\n",
                             *resIDUpdate, *bitDepthUpdate, *colorFmtUpdate);
        }
    }

    if (pHdmiTx->sinkSpecificConfig.fix_59_94_active && (*resIDUpdate == RES_4Kx2K5994)) {
        VPPAPI_DBG_INFO("update res to 4K60 for special sink\n");
        *resIDUpdate = RES_4Kx2K60;
    }
#endif
}
