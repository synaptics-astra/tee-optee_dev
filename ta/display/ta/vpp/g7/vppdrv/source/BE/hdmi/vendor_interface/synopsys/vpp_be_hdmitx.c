/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_BE_HDMITX_C_
#define _VPP_BE_HDMITX_C_

#include "hdtx.h"
#include "avioGbl.h"

#include "vpp.h"

#include "vpp_be.h"
#include "hdmi_scdc.h"
#ifndef CONFIG_OPTEE
#ifdef CONFIG_AMP_TEE_ENABLE
#include "tee_internal_time_api.h"
#endif
#endif

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_regs.h"
#include "vpp_com.h"
#include "vpp_bcmbuf.h"
#include "vpp_cfg.h"
#include "galois_io.h"

#include "hdmi_cmn.h"
#include "vpp_be_hdmitx.h"

#include "avioGbl.h"
#include "vpp.h"
#include "vpp_be_hdmitx_prv.h"
#include "vpp_be_hdmitx_cfg.h"
#include "vpp_be_prv.h"
#include "hdmi_tx_wrap.h"
#include "hdmi_audio.h"
#include "hdmi_eddc.h"

#include "SNPS_HDMI_tx.h"
#include "access_ipk.h"
#include "hdmi_system.h"
#include "api.h"
#include "system.h"
#include "access.h"
#include "platform.h"
#include "core/audio.h"
#include "core/audio_i2s.h"
#include "core/control.h"
#include "core/fc_acp.h"
#include "core/fc_audio_info.h"
#include "core/fc_avi.h"
#include "core/fc_drm.h"
#include "core/fc_gamut.h"
#include "core/fc_isrc.h"
#include "core/fc_packets.h"
#include "core/fc_spd.h"
#include "core/fc_video.h"
#include "core/irq.h"
#include "identification/identification.h"
#include "core/fc_packets.h"
#include "core/fc_spd.h"
#include "core/fc_video.h"
#include "core/main_controller.h"
#include "core/fc_vsd.h"
#include "core/interrupt/interrupt_reg.h"
#include "core/main_controller.h"
#include "core/packets.h"
#include "edid/edid.h"
#include "hdcp/hdcp.h"
#include "util/util.h"
#include "edid/edid.h"
#include "hdcp/hdcp.h"
#include "hdcp/hdcp_14.h"
#include "i2cm.h"
#include "identification/identification.h"
#include "info_frame_control.h"
#include "phy/phy_reg.h"
#include "phy/phy_316.h"
#include "param_map.h"
#include "scdc/scdc.h"
#include "scdc/scrambling.h"
#include "util/util.h"
#include "info_frame_control.h"
#include "vpp_be_hdcp_2x.h"

#if HDCP2X_ENABLED
#include "hdcp2x_api.h"
#include "hdcp2x_api_wrapper.h"
#endif

#ifdef EARC_RX_ENABLED
#include "vpp_be_earc_vendor.h"
#endif

#define ACP_PACKET_SIZE 	16
#define ISRC_PACKET_SIZE 	16
#define HDMI_PHYCONFIG_3RANGE_SIZE 24
#define HDMI_PHYCONFIG_4RANGE_SIZE 32

#define HDMI_HDCP_BSTATUS_PORT 0x41

#define HDMITX_WRAP_BASE                   (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_BASE)
#define HDMITX_WRAP_VID_CTRL1_ADDR         (HDMITX_WRAP_BASE + RA_HDMI_TX_WRAP_VID_CTRL1)

#if defined (SNPS_ARC_ENABLE)
#define HDMITX_SNPS_ARC_CTRL               (HDMITX_WRAP_BASE + RA_HDMI_TX_WRAP_ARCRX_CTRL)
#endif

#define HDCP_KSV_BUFFER_SIZE               670
#define RI_ERR_THRESHOLD                   2
#define HDCP14_CORE_RESET_TIMEOUT          25
#define HDMITX_SRAMPWR_OFF                 7

/* ============================= STATIC FUNCTIONS ============================== */

/**
 * @short Config HDMI TX Wrapper module register
 * @param[in] Video format. RGB/YCC422/YCC444
 * @brief Configure Output swap select (B/Cr) <--> (R/Cb)
 */
static void hdmi_tx_ConfigHdmiTxWrapperModule(BE_HDMITX_DATA  *pHdmiTx, int videoFmt)
{
    VPPHDMI_DBG_INFO("Wrapper swap, color fmt:%d\n", videoFmt);
    T32HDMI_TX_WRAP_VID_CTRL1 vid_ctrl1;

    GA_REG_WORD32_READ(HDMITX_WRAP_VID_CTRL1_ADDR, &vid_ctrl1.u32);
    if (videoFmt == OUTPUT_COLOR_FMT_RGB888)
    {
        vid_ctrl1.uVID_CTRL_input_comp0_swap = 0x00;
        vid_ctrl1.uVID_CTRL_input_comp2_swap = 0x02;
    }
    else
    {
        vid_ctrl1.uVID_CTRL_input_comp0_swap = 0x02;
        vid_ctrl1.uVID_CTRL_input_comp2_swap = 0x00;
    }
    GA_REG_WORD32_WRITE(HDMITX_WRAP_VID_CTRL1_ADDR, vid_ctrl1.u32);
}

/**
 * @short Set PHY number
 * @param[in] app Main structure
 * @param[in] phy PHY number
 * @return 0 if successful or -1 if failure
 */
static int hdmi_tx_set_phy(SNPS_HDMI_TX *app, int phy, int debug_mode)
{
    VPPHDMI_DBG_INFO("Setup PHY: %s - %s\n", phy_identification(&(app->hdmi_tx), phy), id_phy_string(&(app->hdmi_tx)));
#ifdef PHY_THIRD_PARTY

    if(phy != PHY_MODEL_THIRD_PARTY_ANGELCREEK) {
        VPPHDMI_DBG_ERROR("PHY value outside of range: %d\n", phy);
        return -1;
    }
    struct hdmi_ack_tx_phy * ack_phy = &(app->hdmi_tx.ack_phy);

    //enable external phy commands
    app->flags = CMD_EXTERN;

    ack_phy->status = 1;
    ack_phy->version = 1;
    ack_phy->generation = 1;
    ack_phy->voltage = 400;
    ack_phy->gain = 0;
    ack_phy->sus_clock = 25;
    ack_phy->pwclock = 25; //Mhz
    ack_phy->refclock = 100;
    ack_phy->datarate = 25;
    ack_phy->debug_mode = debug_mode;

    VPPHDMI_DBG_INFO("%s: PHY THIRD_PARTY ACK init\n", __func__);

    phy_preparation(&(app->hdmi_tx), phy);

    if(phy_powerup(&(app->hdmi_tx), phy)){
        VPPHDMI_DBG_INFO("%s: PHY THIRD_PARTY ACK powerup failed \n", __func__);
        return -1;
    }
#else
    if((phy > 500) || (phy < 100)){
        VPPHDMI_DBG_ERROR("PHY value outside of range: %d\n", phy);
        return -1;
    }
#endif
    app->hdmi_tx_phy = phy;
    return 0;
}

static void hdmi_tx_reset_hdcp_params(SNPS_HDMI_TX *app)
{
    hdcp_params_reset(&app->hdmi_tx, &app->mode.pHdcp);
    if (app->mode.pHdcp.mKsvListBuffer != NULL)
        VPP_Memset (app->mode.pHdcp.mKsvListBuffer, 0, (sizeof(u8) * HDCP_KSV_BUFFER_SIZE));
    app->hdmi_tx.hdcp.maxDevices = app->mode.ksv_devices;
    app->mode.pHdcp.mAksv = app->mode.dpk_aksv;
    app->mode.pHdcp.mSwEncKey = app->mode.sw_enc_key;
    app->mode.pHdcp.mKeys = app->mode.dpk_keys;

    // Check if the HDCP was previously enabled by the user
    if(app->hdmi_tx.snps_hdmi_ctrl.hdcp_on == 1)
        app->mode.pHdcp.bypass = FALSE;
    else
        app->mode.pHdcp.bypass = TRUE;
}

static void hdcp_14_stop_authentication(SNPS_HDMI_TX *app)
{
    VPPHDMI_DBG_INFO("stopping and keeping hdcp1.4 core in idle state..\n");
    hdcp_rxdetect(&app->hdmi_tx, 0);
    hdcp_sw_reset(&app->hdmi_tx);
    hdcp_InterruptMask(&app->hdmi_tx, 0xFF);
    hdcp_InterruptClear(&app->hdmi_tx, 0xFF);
    mc_hdcp_clock_disable(&app->hdmi_tx, TRUE);
}

/*-----------------------------------------------------------------------------
 * Description  : This function gets Vendor Specific Infoframe from register
 * Arguments    : pHdmiTx - Pointer to HDMI structure
 *              : pPktData - Pointer to infoframe data
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
static INT be_hdmitx_getVndrSpecInfoFrame(BE_HDMITX_DATA *pHdmiTx, UINT8 *pPktData)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    pPktData[1] = 1;
    pPktData[2] = fc_vsd_get_size(&snps_hdmi->hdmi_tx);
    fc_vsd_get_vendor_OUI(&snps_hdmi->hdmi_tx, &pPktData[4]);
    if (fc_vsd_get_vendor_payload(&snps_hdmi->hdmi_tx, &pPktData[7], 24)) {
        return MV_VPP_EBADPARAM;    /* DEFINE ERROR */
    }
    return MV_VPP_OK;
}

/*-----------------------------------------------------------------------------
 * Description  : This function gets DRM packet data into register
 * Arguments    : pHdmiTx - Pointer to HDMI structure
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
static INT be_hdmitx_getDRMpacket(BE_HDMITX_DATA *pHdmiTx, UINT8 *pPktData)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    pPktData[1] = CEA_HDR_FRM_VERS;
    pPktData[2] = CEA_HDR_FRM_LEN;
    fc_drm_packet_body_read(&snps_hdmi->hdmi_tx, &pPktData[FRM_HDR_LEN], CEA_HDR_FRM_LEN);

    return (MV_VPP_OK);
}

void hdcp_22_esm_ddc_reset(BE_HDMITX_DATA *pHdmiTx)
{
#if HDCP2X_ENABLED
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    /* TCL43S525 TV toggles HPD line on receiving AKE CERT.
    When this happens ESM keeps toggling SCL which causes EDID/HDCP failure.
    Based on Synopsys feedback enable HDCP22 override to stop ESM driving I2C clock */
    hdcp_OverrideHDCP2p2Switch(&snps_hdmi->hdmi_tx, TRUE);
    hdcp_OverrideHDCP2p2SwitchValue(&snps_hdmi->hdmi_tx, 0);
#endif
}

static void hdcp_22_stop_authentication(BE_HDMITX_DATA *pHdmiTx)
{
#if HDCP2X_ENABLED
    VPPHDMI_DBG_INFO("stopping and keeping hdcp2.2 core in idle state..\n");
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    HDCP2X_StopAuthentication(pHdmiTx);
    hdcp_22_esm_ddc_reset(pHdmiTx);
    mc_hdcp_clock_disable(&snps_hdmi->hdmi_tx, TRUE);
#endif
}

#if defined (SNPS_ARC_ENABLE)
static void hdmitx_arcrx_enable(BE_HDMITX_DATA *pHdmiTx, bool state)
{
    T32HDMI_TX_WRAP_ARCRX_CTRL arcrx_ctrl;
    GA_REG_WORD32_READ(HDMITX_SNPS_ARC_CTRL, &arcrx_ctrl.u32);
    arcrx_ctrl.uARCRX_CTRL_arcrx_en = (state==true)?true:false;
    arcrx_ctrl.uARCRX_CTRL_arcrx_mode = true; //common mode
    arcrx_ctrl.uARCRX_CTRL_pwrdn = (state==true)?false:true;
    GA_REG_WORD32_WRITE(HDMITX_SNPS_ARC_CTRL, arcrx_ctrl.u32);
    VPPHDMI_DBG_INFO("enable arc mode : %d : arc_ctrl:0x%x \n", state, arcrx_ctrl.u32);
}
#endif

#if HDCP2X_ENABLED
static void hdcp_22_clearMessage(BE_HDMITX_DATA *pHdmiTx)
{
    u16 messageSize;
    u8 byteRead[2];
    u8 dataBytes[534];

    /*EDDC Master based I2C*/
    if(EDDC_Read(pHdmiTx, EDDC_SLAVE_HDCP, HDCP_PORT_RXSTATUS, 2, byteRead)) {
        return;
    }
    byteRead[1] &= 0x03;

    messageSize = ((byteRead[1]<<8) | byteRead[0]);
    if(messageSize) {
        VPPHDMI_DBG_INFO("-- pending message size to clear : %d\n", messageSize);
        EDDC_Read(pHdmiTx, EDDC_SLAVE_HDCP, HDCP_PORT_READ_MSG, EDDC_READ_LIMIT, dataBytes);
    }
    return;
}
#endif

static int be_hdmitx_update_bstatus(BE_HDMITX_DATA *pHdmiTx)
{
#if defined (HDCP_REPEATER_MODE)
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    uint8_t bstat[2] = {0};
    uint8_t deviceCount;
    uint8_t *hdcp_ksv_list_buffer;

    if(MV_VPP_OK == EDDC_Read(pHdmiTx, 1 , HDMI_HDCP_BSTATUS_PORT, sizeof(bstat) , bstat)) {
        deviceCount = bstat[0] & BSTATUS_DEVICE_COUNT_MASK;
        hdcp_ksv_list_buffer = snps_hdmi->mode.pHdcp.mKsvListBuffer;
        VPPHDMI_DBG_ERROR("Bstatus : %x:%x\n", bstat[1], bstat[0]);
        snps_hdmi->mode.pHdcp.devCount = deviceCount;
        if (hdcp_ksv_list_buffer != NULL) {
            hdcp_ksv_list_buffer[(deviceCount * 5) + 0] = bstat[0];
            hdcp_ksv_list_buffer[(deviceCount * 5) + 1] = bstat[1];
        } else
            VPPHDMI_DBG_ERROR("Error in KSV buffer\n");
    } else {
        return MV_VPP_EIOFAIL;
    }
#endif
    return MV_VPP_OK;
}

#if HDCP2X_ENABLED
static void BE_HDMITX_ResetTMDS(BE_HDMITX_DATA *pHdmiTx)
{
    BE_HDMITX_SendAVMute(pHdmiTx, 0);
    BE_HDMITX_PowerOffTMDS(pHdmiTx, 1);
    MV_OSAL_Task_Sleep(200);
    BE_HDMITX_ConfigureScrambler(pHdmiTx);
    BE_HDMITX_PowerOffTMDS(pHdmiTx, 0);
    BE_HDMITX_SendAVMute(pHdmiTx, 0);
    MV_OSAL_Task_Sleep(20);
}
#endif

/* =========================================================================================================*/

INT BE_HDMITX_init(BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = NULL;
    static struct phy_config_dynamic def_phy_conf[MAX_FREQ_GBPS] = {
    /*  {txterm, vlevctrl, cksymtxctrl, txrescalctrl} */
        {0x0007, 0x0180, 0x8188, 0x0000},   /* LT_1_65GBPS */
        {0x0000, 0x0120, 0x83D8, 0x0000},   /* LT_3_71GBPS */
        {0x0000, 0x0000, 0x8FD4, 0xB4DB},   /* ET_3_71GBPS */
        {0x0000, 0x0000, 0x8FD4, 0xB4DB}    /* GT_3_71GBPS */
    };

    if (pHdmiTx->vendorHdmiTx == NULL) {
        VPP_OSAL_SHM_Dynamic_Malloc(VPP_SHM_SECURE, sizeof(SNPS_HDMI_TX), 32, (VPP_SHM_HANDLE*)&pHdmiTx->vendorHdmiTx);
    }

    snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("Error allocating memory for vendor hdmi structure, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    // Clear app structure
    VPP_Memset(pHdmiTx->vendorHdmiTx, 0, sizeof(SNPS_HDMI_TX));

#ifdef TZ_3_0
    VPP_OSAL_SHM_Dynamic_Malloc(VPP_SHM_SECURE, (sizeof(u8) * HDCP_KSV_BUFFER_SIZE),
                        32, (VPP_SHM_HANDLE*)&snps_hdmi->mode.pHdcp.mKsvListBuffer);
#else
    snps_hdmi->mode.pHdcp.mKsvListBuffer = VPP_Malloc(sizeof(u8) * HDCP_KSV_BUFFER_SIZE);
#endif

    if (snps_hdmi->mode.pHdcp.mKsvListBuffer == NULL) {
        VPPHDMI_DBG_ERROR("Error allocating memory for KSV buffer, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    VPP_Memset (snps_hdmi->mode.pHdcp.mKsvListBuffer, 0, (sizeof(u8) * HDCP_KSV_BUFFER_SIZE));
    /* Copy default phy config values */
    VPP_Memcpy(snps_hdmi->hdmi_tx.phy_conf, def_phy_conf, sizeof(def_phy_conf));

    snps_hdmi->verbose = 2; /*enable logs from snps LOGGER*/

#ifndef CONFIG_AMP_TEE_ENABLE
    /* in TEE env, fn pointers can't be stored in static struct as below, so call directly for now */
    // set device access functions
    strcpy(snps_hdmi->dev_access.name, "HDMI TX Device Access");
    snps_hdmi->dev_access.initialize = hdmitx_initialize;
    snps_hdmi->dev_access.disable = hdmitx_disable;
    snps_hdmi->dev_access.read = hdmitx_read;
    snps_hdmi->dev_access.write = hdmitx_write;

    // set system abstraction functions
    strcpy(snps_hdmi->sys_functions.name, "HDMI TX System Functions");
    snps_hdmi->sys_functions.logger = hdmitx_logger;
    snps_hdmi->sys_functions.sleep = hdmitx_sleep;

    snps_hdmi->board_wrp.configure_audio_pll = audio_clock;
#ifndef PHY_THIRD_PARTY
    snps_hdmi->board_wrp.configure_pixel_clock = pixel_clock;
#endif

    /* register functions */
    register_system_functions(&(snps_hdmi->sys_functions));
    register_bsp_functions(&(snps_hdmi->dev_access));

    register_board_wrappers(&(snps_hdmi->board_wrp));
#endif

    hdmitx_api_init(&(snps_hdmi->hdmi_tx), "HDMI Tx App");

    snps_hdmi->snps_tx_base_address = SNPS_HDMITX_BASE_ADDR; //hardcoded in read/write fns
    snps_hdmi->hdmitx_wrap_base_address = HDMI_TX_WRAPPER_BASE_ADDR;
    snps_hdmi->hdcp22_tx_wrap_base_address = HDCP22_TX_WRAP_BASE_ADDR;

    snps_hdmi->hdmi_tx.snps_hdmi_ctrl.data_enable_polarity = 1;
    snps_hdmi->hdmi_tx.snps_hdmi_ctrl.rxsense_on = false;
    snps_hdmi->mode.pHdcp.bypass = TRUE;//TODO:for now?
    snps_hdmi->mode.pHdcp.hdcp_requested_state = VPP_HDMI_HDCP_VER_NONE;
    snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_INACTIVE;

    /* Keep hdcp in idle state till application enables it */
    snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hdcp_on = FALSE;

    /* Copy default phy config values */
    GaloisMemcpy(snps_hdmi->hdmi_tx.phy_conf, def_phy_conf, sizeof(def_phy_conf));
    return 0;
}
INT BE_HDMITX_Deinit(BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if (snps_hdmi->mode.pHdcp.mKsvListBuffer != NULL)
        VPP_Free (snps_hdmi->mode.pHdcp.mKsvListBuffer);

    if (snps_hdmi != NULL)
        VPP_Free (snps_hdmi);

    return MV_VPP_OK;
}

INT BE_HDMITX_EnableAudioFmt(BE_HDMITX_DATA *pHdmiTx, BOOL EnAudio)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    BE_HDMITX_PacketTxEnableAUDI (&snps_hdmi->hdmi_tx,EnAudio);
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function enables audio packet transfer
 * Arguments    : EnAudio - Flag to enable/disable audio packet transmission
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_EnableAudioPkt (BE_HDMITX_DATA *pHdmiTx, BOOL EnAudio)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (EnAudio)
    {
        BE_HDMITX_PacketTxEnableAUDI (&snps_hdmi->hdmi_tx, TRUE);
    }
    else
    {
        BE_HDMITX_PacketTxEnableAUDI (&snps_hdmi->hdmi_tx,FALSE);
        BE_HDMITX_PacketTxEnableACR (&snps_hdmi->hdmi_tx, FALSE);
    }
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets audio format parameters
 * Arguments    : pAudioFmtParams - Pointer to audio format parameters (Input)
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioFmtParams(BE_HDMITX_DATA *pHdmiTx, VPP_BE_HDMITX_AUDIO_BUS AudioBus,
                                    PVPP_BE_HDMITX_AUDIO_FMT_PARAMS pAudioFmtParams)
{
    int success = TRUE;
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    switch (AudioBus) {
        case VPP_BE_HDMITX_AUDIO_I2S: snps_hdmi->mode.pAudio.mInterfaceType = I2S;
            if(pHdmiTx->audFmtParams.HbrAudio)
                snps_hdmi->mode.pAudio.mInterfaceType = HBR;
            else
                snps_hdmi->mode.pAudio.mInterfaceType = I2S;
        break;
        case VPP_BE_HDMITX_AUDIO_SPDIF: snps_hdmi->mode.pAudio.mInterfaceType = SPDIF;
        break;
        default: snps_hdmi->mode.pAudio.mInterfaceType = INTERFACE_NOT_DEFINED;
        break;
    }

    // Audio - Workaround
    audio_Initialize(&snps_hdmi->hdmi_tx);
    mc_audio_sampler_clock_enable(&snps_hdmi->hdmi_tx, 1);
    success = audio_Configure(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pAudio);
    if(success == FALSE){
        VPPHDMI_DBG_INFO("%s:Audio not configured\n", __func__);
        return MV_VPP_EBADCALL;
    }

    if(pHdmiTx->audFmtParams.HbrAudio)
        audio_i2s_data_enable(&snps_hdmi->hdmi_tx, 0xF);
    else {
        if(pHdmiTx->audNumChannels >> 1 == 0)
            audio_i2s_data_enable(&snps_hdmi->hdmi_tx, 0x1);
        else if(pHdmiTx->audNumChannels >> 1 == 1)
            audio_i2s_data_enable(&snps_hdmi->hdmi_tx, 0x3);
        else if(pHdmiTx->audNumChannels >> 1 == 2)
            audio_i2s_data_enable(&snps_hdmi->hdmi_tx, 0x7);
        else
            audio_i2s_data_enable(&snps_hdmi->hdmi_tx, 0xF);
    }
    BE_HDMITX_PacketTxEnableACR(&snps_hdmi->hdmi_tx, TRUE);

    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets/clears the audio fifo reset
 * Arguments    : bSet - TRUE/FALSE to set/clear the audio fifo reset
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioFifoReset(BE_HDMITX_DATA *pHdmiTx, INT bSet)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    /* Writing 1 resets FIFO. Currently TRUE->FALSE coming from AOUT
       So resets FIFO on the second call( bSet = FALSE */
    if (!bSet)
        audio_i2s_reset_fifo(&snps_hdmi->hdmi_tx);
    return MV_VPP_OK;
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets audio validity bit
 * Arguments    : validityBit - Audio validity bit to be loaded
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioValidityBit(BE_HDMITX_DATA *pHdmiTx, INT vBit)
{
/* TODO: called from setVUC(used AOUT) and set audio fmt */
    return MV_VPP_OK;
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets audio channel status bits
 * Arguments    : pChSts - Pointer to audio chnanel status bits (Input)
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioChStsBits(BE_HDMITX_DATA *pHdmiTx, UINT8 *pChSts)
{
/* TODO: called from set audio VUC(called from AOUT) */
    return (MV_VPP_EBADPARAM);
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets/clears audio user data bits
 * Arguments    : pUserData - Pointer to user data bits (Input)
 *              : If NULL, user data bits are cleared
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetAudioUserBits(BE_HDMITX_DATA *pHdmiTx, UINT8 *pUserData)
{
/* TODO: called from set audio VUC(called from AOUT) */
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function starts/stops loading of VUC data
 * Arguments    : startLoad - TRUE, to start loading of data and FALSE, for
 *              :           - stopping VUC data load
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_StartUCLoad(BE_HDMITX_DATA *pHdmiTx, BOOL startLoad)
{
/* TODO: called from set audio VUC(called from AOUT) */
    return (MV_VPP_OK);
}

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
                                INT HdmiVidFmt)
{
    /* TODO: copy required data to snps struct */
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    INT retVal = MV_VPP_OK;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    hdmi_tx_ConfigHdmiTxWrapperModule(pHdmiTx, pHdmiTx->videoParams.VideoFmt);
    snps_hdmi->mode.pVideo.mHdmi = OutMode - 1;
    snps_hdmi->mode.pVideo.mHdmiVideoFormat = HdmiVidFmt;
    mc_phy_reset(&snps_hdmi->hdmi_tx, TRUE);
    /* as per 3.1.2 in user manual, configuring PHY, put PHY in reset before updating the VP_PR_CD. Reset removed in phyConfigure.
        Phy should already be in power down mode from SetCPCB or SetHdmiVideo */
    if (!api_Configure(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pVideo, &snps_hdmi->mode.pAudio,
            &snps_hdmi->mode.pProduct, &snps_hdmi->mode.pHdcp, snps_hdmi->hdmi_tx_phy)) {
        VPPHDMI_DBG_ERROR("API configure error\n");
        retVal = MV_VPP_EBADPARAM;
    }
#ifdef HDMI_SRV_SUPPORT
    irq_hpd_sense_enable(&snps_hdmi->hdmi_tx);
#endif
    return retVal;
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets HDMI/DVI output mode
 * Arguments    : OutMode - HDMI/DVI Mode
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetMode(BE_HDMITX_DATA *pHdmiTx, VPP_BE_HDMITX_OUT_MODE OutMode)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (OutMode == VPP_BE_HDMITX_OUT_HDMI)
    {
        snps_hdmi->mode.pVideo.mHdmi = HDMI;
        fc_video_DviOrHdmi(&snps_hdmi->hdmi_tx, HDMI);
    }
    else
    {
        snps_hdmi->mode.pVideo.mHdmi = DVI;
        fc_video_DviOrHdmi(&snps_hdmi->hdmi_tx, DVI);
    }

    return (MV_VPP_OK);
}

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
                          VPP_BE_HDMITX_HOST_PKT_TX_MODE PktTxMode)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    switch(pktType) {
        case VPP_HDMI_PKT_ID_ACP :
            if (PktTxMode == VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC) {
                fc_packets_AutoSend(&snps_hdmi->hdmi_tx, TRUE, ACP_TX);
            } else {
                fc_packets_ManualSend (&snps_hdmi->hdmi_tx, ACP_TX);
            }
        break;

        case VPP_HDMI_PKT_ID_ISRC1 :
            if (PktTxMode == VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC) {
                fc_packets_AutoSend(&snps_hdmi->hdmi_tx, TRUE, ISRC1_TX);
            } else {
                fc_packets_ManualSend (&snps_hdmi->hdmi_tx, ISRC1_TX);
            }
        break;

        case VPP_HDMI_PKT_ID_ISRC2 :
            if (PktTxMode == VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC) {
                fc_packets_AutoSend(&snps_hdmi->hdmi_tx, TRUE, ISRC2_TX);
            } else {
                fc_packets_ManualSend (&snps_hdmi->hdmi_tx, ISRC2_TX);
            }
        break;

        case VPP_HDMI_PKT_ID_GAMUT_METADATA :
            fc_gamut_enable_tx(&snps_hdmi->hdmi_tx, TRUE);
        break;

        case VPP_HDMI_PKT_ID_VENDOR_INFOFRAME :
            if (PktTxMode == VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC) {
                fc_packets_AutoSend(&snps_hdmi->hdmi_tx, TRUE, VSD_TX);
            } else {
                fc_packets_ManualSend (&snps_hdmi->hdmi_tx, VSD_TX);
            }
        break;

        case VPP_HDMI_PKT_ID_AVI_INFOFRAME :
            BE_HDMITX_PacketTxEnableAVI (&snps_hdmi->hdmi_tx, TRUE);
        break;

        case VPP_HDMI_PKT_ID_SPD_INFOFRAME :
            if (PktTxMode == VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC) {
                fc_packets_AutoSend(&snps_hdmi->hdmi_tx, TRUE, SPD_TX);
            } else {
                fc_packets_ManualSend (&snps_hdmi->hdmi_tx, VSD_TX);
            }
        break;
        case VPP_HDMI_PKT_ID_AUDIO_INFOFRAME :
            BE_HDMITX_PacketTxEnableAUDI (&snps_hdmi->hdmi_tx, TRUE);
        break;

        case VPP_HDMI_PKT_ID_MPEG_SRC_INFOFRAME :
            /* TODO: (optional, implementation discouraged by CEA-861-E
               Section 6.7).*/
            VPPHDMI_DBG_ERROR ("MPEG InfoFrame not implemented\n");
        break;

        case VPP_HDMI_PKT_ID_HDR_INFOFRAME:
            BE_HDMITX_PacketTxEnableDRM(&snps_hdmi->hdmi_tx, TRUE);
        break;

        default:
            VPPHDMI_DBG_ERROR ("Wrong packet type : 0x%X\n", pktType);
        break;
    }

    return (MV_VPP_OK);
}

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
                             VPP_BE_HDMITX_HOST_PKT_TX_MODE PktTxMode)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (PktTxMode == VPP_BE_HDMITX_HOST_PKT_TX_EVERY_VSYNC)
    {
        switch(pktType) {

            case VPP_HDMI_PKT_ID_ACP :
                packets_StopSendAcp (&snps_hdmi->hdmi_tx);
            break;

            case VPP_HDMI_PKT_ID_ISRC1 :
                packets_StopSendIsrc1(&snps_hdmi->hdmi_tx);
            break;

            case VPP_HDMI_PKT_ID_ISRC2 :
                packets_StopSendIsrc2 (&snps_hdmi->hdmi_tx);
            break;

            case VPP_HDMI_PKT_ID_GAMUT_METADATA :
                fc_gamut_enable_tx (&snps_hdmi->hdmi_tx, FALSE);
            break;

            case VPP_HDMI_PKT_ID_VENDOR_INFOFRAME :
                packets_StopSendVsd (&snps_hdmi->hdmi_tx);
            break;

            case VPP_HDMI_PKT_ID_AVI_INFOFRAME :
                BE_HDMITX_PacketTxEnableAVI (&snps_hdmi->hdmi_tx, FALSE);
            break;

            case VPP_HDMI_PKT_ID_SPD_INFOFRAME :
                packets_StopSendSpd (&snps_hdmi->hdmi_tx);
            break;

            case VPP_HDMI_PKT_ID_AUDIO_INFOFRAME :
                BE_HDMITX_PacketTxEnableAUDI (&snps_hdmi->hdmi_tx,TRUE);
            break;

            case VPP_HDMI_PKT_ID_MPEG_SRC_INFOFRAME :
                /* TODO: (optional, implementation discouraged by CEA-861-E
                   Section 6.7).*/
                VPPHDMI_DBG_ERROR ("MPEF InfoFrame not implemented\n");
            break;

            case VPP_HDMI_PKT_ID_HDR_INFOFRAME:
                BE_HDMITX_PacketTxEnableDRM(&snps_hdmi->hdmi_tx, FALSE);
            break;

            case VPP_HDMI_PKT_ID_RESET_ALL: /* disable all packets */
                fc_gamut_enable_tx (&snps_hdmi->hdmi_tx, FALSE);  // GAMUT
                packets_DisableAllPackets (&snps_hdmi->hdmi_tx);     // ACP, ISRC1, ISRC2, VENDOR, SPD
                BE_HDMITX_PacketsTxDisableAll (&snps_hdmi->hdmi_tx);      // AVI, AUDI
            break;
            default:
                VPPHDMI_DBG_ERROR ("Wrong packet type : 0x%X\n", pktType);
            break;
        }

    }

    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function reports if the packet in the given index
 *              : is being transmitted and if is, in transfer once or repeat
 *              : mode
 * Arguments    : PktIndex   - Packet Index
 *              : TransferEnabled - Pointer to return if transfer is enabled
 *              : for the given index
 *              : pPktTxMode - Pointer to return mode of transfer
 *              : pPktHeader - Packet header
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_GetHostPktTxSts(VPP_OBJ *vpp_obj, UINT8 PktIndex,
                              BOOL *TransferEnabled,
                              VPP_BE_HDMITX_HOST_PKT_TX_MODE *pPktTxMode,
                              UINT8 *pPktHeader)
{
    /*TODO: map to SNPS*/
    return MV_VPP_EUNCONFIG;
}

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
INT BE_HDMITX_GetHostPktTxData(BE_HDMITX_DATA *pHdmiTx, UINT8 pktType,UINT8 *pPktData)
{
    /*TODO: map to SNPS*/
    int ret = MV_VPP_OK;
    if (NULL == pPktData)
        return (MV_VPP_EBADPARAM);

    switch(pktType)
    {
        case VPP_HDMI_PKT_ID_VENDOR_INFOFRAME:
            ret = be_hdmitx_getVndrSpecInfoFrame(pHdmiTx, pPktData);
        break;
        case VPP_HDMI_PKT_ID_HDR_INFOFRAME:
            ret = be_hdmitx_getDRMpacket(pHdmiTx, pPktData);
        break;
    }
    return ret;
}

/*-----------------------------------------------------------------------------
 * Description  : This function clears host packet data at the given index
 * Arguments    : pktIndex   - Packet Index
 *              : bcmType - bcm buffer register programming goes to
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_ClearHostPktTxData(BE_HDMITX_DATA *pHdmiTx, UINT8 pktIndex, UINT8 bcmType)
{
    /*TODO: map to SNPS*/
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets Vendor Specific Infoframe into register
 * Arguments    : pHdmiTx - Pointer to HDMI structure
 *              : vsif - Poniter to vsif structure
 *              : pPktData - Pointer to infoframe data
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetVndrSpecInfoFrame(BE_HDMITX_DATA *pHdmiTx,
                VPP_HDMI_PKT_VNDRSPEC_INFOFRM *vsif, UINT8 *pPktData)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    UINT32 ieeeID = 0;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (NULL == pPktData)
        return (MV_VPP_EBADPARAM);

    if(snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hdmi_on == 0){
        VPPHDMI_DBG_ERROR ("DVI mode selected: packets not configured\n");
        return MV_VPP_EUNCONFIG;
    }
    videoParams_t *video = &snps_hdmi->mode.pVideo;

    video->mHdmiVideoFormat = vsif->HdmiVideoFmt & 0x07;
    video->m3dStructure = vsif->Hdmi_3D_Structure;
    video->m3dExtData = vsif->Hdmi_3D_Ext_Data;

    fc_vsd_set_size(&snps_hdmi->hdmi_tx, pPktData[2]);
    ieeeID = (UINT32)pPktData[4] | (pPktData[5] << 8) | (pPktData[6] <<16);
    VPPHDMI_DBG_INFO("Configure VSIF with IeeeID %X\n", ieeeID);
    fc_vsd_vendor_OUI(&snps_hdmi->hdmi_tx, ieeeID);
    if (fc_vsd_vendor_payload(&snps_hdmi->hdmi_tx, &pPktData[7], 24)) {
        return MV_VPP_EBADPARAM;    /* DEFINE ERROR */
    }
    return MV_VPP_OK;
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets DRM packet data into register
 * Arguments    : pHdmiTx - Pointer to HDMI structure
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetDRMpacket(BE_HDMITX_DATA *pHdmiTx, UINT8 *pPktData)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if(snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hdmi_on == 0){
        VPPHDMI_DBG_ERROR ("DVI mode selected: packets not configured\n");
        return MV_VPP_EUNCONFIG;
    }

    fc_drm_packet_header_version(&snps_hdmi->hdmi_tx, CEA_HDR_FRM_VERS);
    fc_drm_packet_header_length(&snps_hdmi->hdmi_tx, CEA_HDR_FRM_LEN);
    fc_drm_packet_body(&snps_hdmi->hdmi_tx, &pPktData[FRM_HDR_LEN], CEA_HDR_FRM_LEN);
    fc_drm_packet_update(&snps_hdmi->hdmi_tx, TRUE);

    return (MV_VPP_OK);
}

INT BE_HDMITX_UpdateAVIInfoFrame(BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if(snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hdmi_on == 0){
        LOGGER(SNPS_WARN, "DVI mode selected: packets not configured");
        return MV_VPP_EUNCONFIG;
    }

    videoParams_t *video = &snps_hdmi->mode.pVideo;
    video->mRgbQuantizationRange = pHdmiTx->aviParams.RGBQuantRange;
    video->mYccQuantizationRange = pHdmiTx->aviParams.YCCQuantRange;
    video->mColorimetry = pHdmiTx->aviParams.Calorimetry;
    video->mExtColorimetry = pHdmiTx->aviParams.ExtCalorimetry;

    fc_avi_config(&snps_hdmi->hdmi_tx, video);
    return (MV_VPP_OK);
}

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
/* TODO: not required, commented. called from process frameInt */
INT BE_HDMITX_SyncHostPktTxOnceStatus(BE_HDMITX_DATA *pHdmiTx, UINT8 *pPktTxOnceStatus)
{
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function mutes audio and/or video
 * Arguments    : audio - Enable/Disable audio mute
 *              : video - Enable/Disable vidio mute
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_SetMute(BE_HDMITX_DATA *pHdmiTx, BOOL audio, BOOL video)
{
    INT retCode = MV_VPP_OK;
    /* Used for sending FIX color before starting auth and muting audio... check req for SNPS */
    return (retCode);
}

INT BE_HDMITX_SetAudioMute(BE_HDMITX_DATA *pHdmiTx, BOOL mute)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    audio_mute(&snps_hdmi->hdmi_tx, mute);
    return (MV_VPP_OK);
}

INT BE_HDMITX_SetVideoMute(BE_HDMITX_DATA *pHdmiTx, BOOL mute)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    VPPHDMI_DBG_INFO("--fix color from hdmi tx : %d --\n", mute);
    return BE_HDMITX_WrapperEnableFixColor(pHdmiTx, mute);
}

INT BE_HDMITX_EnableHdcpFrameProt(BE_HDMITX_DATA *pHdmiTx, BOOL enable)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if((enable == false) && (pHdmiTx->encBypassOnHdcpFail == false)) {
        VPPHDMI_DBG_ERROR("bypass encryption allowed only if enabled in xml..\n");
        return MV_VPP_EUNSUPPORT;
    }

    /* for dolphin, on hdcp failure, frames are blocked by HW.
       NTS cases required output to be unmuted and controlled by sa_attr.
       Bypass encryption module for acheiving the same */
    hdcp_bypass_encryption(&snps_hdmi->hdmi_tx, (enable)?FALSE:TRUE);
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function returns audio and video mute status
 * Arguments    : pAudio - Pointer to return audio mute status (Output)
 *              : pVideo - Pointer to return video mute status (Output)
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_GetMuteSts(BE_HDMITX_DATA *pHdmiTx, BOOL *pAudio, BOOL *pVideo)
{
    /*TODO: map to SNPS */
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function returns current Rx sense Status
 * Arguments    : pRxSense - Pointer to return Rx sense status
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_GetRxSense(BE_HDMITX_DATA *pHdmiTx,int *pRxSense)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    *pRxSense=phy_rx_sense_state(&snps_hdmi->hdmi_tx);

    return MV_VPP_OK;
}

/*-----------------------------------------------------------------------------
 * Description  : This function returns current Hot Plug Status
 * Arguments    : pHPDStatus - Pointer to return HPD status
 * Returns      : MV_VPP_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT BE_HDMITX_GetHPDStatus(BE_HDMITX_DATA *pHdmiTx, BOOL *pHPDStatus)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    *pHPDStatus = phy_hot_plug_state(&snps_hdmi->hdmi_tx);
    VPPHDMI_DBG_INFO("snps phy status : HPD state %s\n", (*pHPDStatus)?"high":"low");
    if ((*pHPDStatus == 1) && (pHdmiTx->hdmiConfig.enableRxsenseCheck)) {
        /*rx sense is taking more time to cutoff */
        *pHPDStatus = phy_rx_sense_state(&snps_hdmi->hdmi_tx);
        VPPHDMI_DBG_INFO("snps phy status : RXSENSE state %s\n", (*pHPDStatus)?"high":"low");
    }

#ifdef EARC_RX_ENABLED
    if(*pHPDStatus == 1) {
        if(MV_VPP_OK == BE_EARCRX_GetEarcHpdState(pHdmiTx, pHPDStatus)) {
            VPPHDMI_DBG_INFO("earc hpd state : %s\n", (*pHPDStatus)?"high":"low");
        }
    }
#endif

    return (MV_VPP_OK);
}

INT BE_HDMITX_GetHdmiHpdStatus(BE_HDMITX_DATA *pHdmiTx, BOOL *pHPDStatus)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    *pHPDStatus = phy_hot_plug_state(&snps_hdmi->hdmi_tx);

    // TODO: Check if rxsense checking to be included based on interop results
    return (MV_VPP_OK);
}

VOID BE_HDMITX_ClearSinkDetectionInterrupts(BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    uint8_t clear_mask = 0;
    uint8_t ih_phy_intr = dev_read(&snps_hdmi->hdmi_tx, IH_PHY_STAT0);

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return;
    }

    //VPPHDMI_DBG_INFO("ih_phy_intr : 0x%x\n", ih_phy_intr);
    //VPPHDMI_DBG_INFO("phy_stat : 0x%x\n", dev_read(&snps_hdmi->hdmi_tx, 0x0000C010));
    //VPPHDMI_DBG_INFO("phy_pol : 0x%x\n", dev_read(&snps_hdmi->hdmi_tx, 0x0000C01C));

    /* do not clear interrupt without inverting polarity first */
    if(decode_is_phy_rx_s0(ih_phy_intr)) {
        phy_rx_s0_detected(&snps_hdmi->hdmi_tx);
        clear_mask |= IH_PHY_STAT0_RX_SENSE_0_MASK;
    }

    if(decode_is_phy_rx_s1(ih_phy_intr)) {
        phy_rx_s1_detected(&snps_hdmi->hdmi_tx);
        clear_mask |= IH_PHY_STAT0_RX_SENSE_1_MASK;
    }

    if(decode_is_phy_rx_s2(ih_phy_intr)) {
        phy_rx_s2_detected(&snps_hdmi->hdmi_tx);
        clear_mask |= IH_PHY_STAT0_RX_SENSE_2_MASK;
    }

    if(decode_is_phy_rx_s3(ih_phy_intr)) {
        phy_rx_s3_detected(&snps_hdmi->hdmi_tx);
        clear_mask |= IH_PHY_STAT0_RX_SENSE_3_MASK;
    }

    if(decode_is_phy_hpd(ih_phy_intr)) {
        phy_hot_plug_detected(&snps_hdmi->hdmi_tx);
        clear_mask |= IH_PHY_STAT0_HPD_MASK;
    }

   // VPPHDMI_DBG_INFO("phy_stat : 0x%x\n", dev_read(&snps_hdmi->hdmi_tx, 0x0000C010));
   // VPPHDMI_DBG_INFO("phy_pol : 0x%x\n", dev_read(&snps_hdmi->hdmi_tx, 0x0000C01C));
   // VPPHDMI_DBG_INFO("irq_clr : 0x%x\n", clear_mask);
    dev_write(&snps_hdmi->hdmi_tx, IH_PHY_STAT0, clear_mask);
   // VPPHDMI_DBG_INFO("ih_phy_intr : 0x%x\n", dev_read(&snps_hdmi->hdmi_tx, 0x00000410));
}


INT BE_HDMITX_SendAVMute(BE_HDMITX_DATA *pHdmiTx, BOOL mute)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    if (snps_hdmi->mode.pVideo.mHdmi == HDMI) {
        VPPHDMI_DBG_INFO("GCP avmute : %s\n", mute?"set":"clear");
        api_avmute(&snps_hdmi->hdmi_tx, mute);  //sets GCP packet and HDCP avmute
        BE_HDMITX_PacketTxEnableGCP (&snps_hdmi->hdmi_tx, TRUE);
    }
    return (MV_VPP_OK);
}

INT BE_HDMITX_GetGCPEnableStatus(BE_HDMITX_DATA *pHdmiTx, BOOL *pState)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    BE_HDMITX_PacketTxEnableGCPStatus (&snps_hdmi->hdmi_tx, pState);
    return (MV_VPP_OK);
}

INT BE_HDMITX_GetAVMuteSts(BE_HDMITX_DATA *pHdmiTx, BOOL *pState)
{
    /*TODO: map to SNPS */
    return (MV_VPP_OK);
}

/*****************************************************************************/
/* HDMI PHY Setting                                                          */
/*****************************************************************************/

/*-----------------------------------------------------------------------------
 * Description  : This function enables/disables HDMI TMDS
 * Arguments    : Enable - True/False to control TMDS
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
int BE_HDMIPHY_EnableTmds (BE_HDMITX_DATA *pHdmiTx, BOOL Enable)
{
    /*TODO: map to SNPS */
    return (MV_VPP_OK);
}

/*****************************************************************************/
/* STATIC FUNCTIONS                                                          */
/*****************************************************************************/

int BE_HDMITX_GetHdmiPowerStatus(BE_HDMITX_DATA *pHdmiTx, int *pPowerState)
{
    SNPS_HDMI_TX *snps_hdmi = NULL;

    if((NULL == pHdmiTx) || (NULL == pHdmiTx->vendorHdmiTx)){
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    snps_hdmi = pHdmiTx->vendorHdmiTx;
    *pPowerState = phy_phase_lock_loop_state(&snps_hdmi->hdmi_tx);
    return MV_VPP_OK;
}

INT BE_HDMITX_ConfigHdmiVideoFmt(BE_HDMITX_DATA  *pHdmiTx, int aAspRatio,
                                    int scanInfo, BOOL updateInfoFrame)
{
    LOG_TRACE();
    double refreshRate = 0.0;
    int color_fmt;
    int bit_depth;
    int resID;
    CEA_RES_INFO        ResInfo;
    dtd_t dtd;
    RESOLUTION_INFO  *pResinfo;

    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    resID = pHdmiTx->videoParams.DispRes;

    // Set aspect ratio
    if (aAspRatio != HDMI_USE_EXISTING)
    {
        if (aAspRatio == ASPECT_RATIO_4BY3)
            pHdmiTx->aviParams.AspectRatio = CEA_PICT_AR_4_3;
        else if (aAspRatio == ASPECT_RATIO_16BY9)
            pHdmiTx->aviParams.AspectRatio = CEA_PICT_AR_16_9;
        else
            pHdmiTx->aviParams.AspectRatio = CEA_PICT_AR_NONE;
    }

    // Set scan information
    if (scanInfo != HDMI_USE_EXISTING)
    {
        if (scanInfo == OVER_SCAN_DATA)
            snps_hdmi->mode.pVideo.mScanInfo = CEA_OVER_SCAN_DATA;
        else if (scanInfo == UNDER_SCAN_DATA)
            snps_hdmi->mode.pVideo.mScanInfo = CEA_UNDER_SCAN_DATA;
        else
            snps_hdmi->mode.pVideo.mScanInfo = CEA_SCAN_DATA_NONE;
    }

    // Set calorimetry
    snps_hdmi->mode.pVideo.mColorimetry = pHdmiTx->aviParams.Calorimetry;
    snps_hdmi->mode.pVideo.mExtColorimetry = pHdmiTx->aviParams.ExtCalorimetry;

    pResinfo = VPP_GetResInfo(resID);

    if ((pResinfo->type == TYPE_HD) && ( !IS_RES_NEED_REDUCED_BLANKING(resID)))  {
        pHdmiTx->aviParams.AspectRatio = CEA_PICT_AR_16_9; // force asp ratio of HD resolution to 16x9
    }

    // Get Video ID code
    ResInfo.HActive     = pResinfo->active_width;
    ResInfo.VActive     = pResinfo->active_height;
    switch (resID)
    {
        case RES_525I60:
        case RES_525P60:
        case RES_720P60:
        case RES_1080I60:
        case RES_1080P60:
        case RES_VGA_480P60:
        case RES_720P60_3D:
        case RES_1080I60_FP:
        case RES_4Kx2K60:
        case RES_4Kx2K60_SMPTE:
        case RES_CVT_RB_3840_1600P60:
        case RES_CVT_RB_1920_1600P60:
        case RES_CVT_RB_3440_1440P60:
        case RES_CVT_RB_1720_1440P60:
        case RES_CVT_RB_1920_2160P60:
        case RES_CVT_RB_1920x1200P60:
        case RES_CUSTOM_2880x1440P60:
        case RES_VESA_800x600P60:
        case RES_VESA_1280x768P60:
        case RES_VESA_1360x768P60:
        case RES_VESA_1024x768P60:
        case RES_CUSTOM_1280x800P60:
        case RES_VESA_1280x1024P60:
        case RES_VESA_1440X900P60:
        case RES_CVT_1400x1050P60:
        case RES_VESA_1680x1050P60:
        case RES_VESA_1600x1200P60:
        case RES_CUSTOM_1366X768P60:
        case RES_CVT_RB_2560X1440P60:
            ResInfo.RefreshRate = 60;
            refreshRate = 60;
            break;
        case RES_525I5994:
        case RES_525P5994:
        case RES_720P5994:
        case RES_1080I5994:
        case RES_1080P5994:
        case RES_VGA_480P5994:
        case RES_720P5994_3D:
        case RES_1080I5994_FP:
        case RES_4Kx2K5994:
        case RES_4Kx2K5994_SMPTE:
            ResInfo.RefreshRate = 60;
            refreshRate = 59.940;
            break;
        case RES_625I50:
        case RES_625P50:
        case RES_720P50:
        case RES_1080I50:
        case RES_1080P50:
        case RES_720P50_3D:
        case RES_1080I50_FP:
        case RES_4Kx2K50:
        case RES_4Kx2K50_SMPTE:
            ResInfo.RefreshRate = 50;
            refreshRate = 50;
            break;
        case RES_1080P24:
        case RES_1080P24_3D:
        case RES_4Kx2K24:
        case RES_4Kx2K24_SMPTE:
            ResInfo.RefreshRate = 24;
            refreshRate = 24;
            break;
        case RES_1080P2398:
        case RES_1080P2398_3D:
        case RES_4Kx2K2398:
        case RES_4Kx2K2398_SMPTE:
            ResInfo.RefreshRate = 24;
            refreshRate = 23.976;
            break;
        case RES_1080P25:
        case RES_1080P25_3D:
        case RES_4Kx2K25:
        case RES_4Kx2K25_SMPTE:
            ResInfo.RefreshRate = 25;
            refreshRate = 25;
            break;
        case RES_1080P30:
        case RES_1080P30_3D:
        case RES_4Kx2K30:
        case RES_4Kx2K30_SMPTE:
            ResInfo.RefreshRate = 30;
            refreshRate = 30;
            break;
        case RES_1080P2997:
        case RES_1080P2997_3D:
        case RES_4Kx2K2997:
        case RES_4Kx2K2997_SMPTE:
            ResInfo.RefreshRate = 30;
            refreshRate = 29.970;
            break;
        case RES_720P120:
        case RES_1080P120:
            ResInfo.RefreshRate = 120;
            refreshRate = 120;
            break;
        default:
            ResInfo.RefreshRate = 0;
            break;
    }

    if (pHdmiTx->videoParams.PixelRpt == VPP_BE_HDMITX_PIXEL_RPT_BY_2)
    {
        ResInfo.HActive *= 2;
    }
    else if (pHdmiTx->videoParams.PixelRpt == VPP_BE_HDMITX_PIXEL_RPT_BY_4)
    {
        ResInfo.HActive *= 4;
    }
    ResInfo.FrmFmt   = (pResinfo->scan == SCAN_INTERLACED);
    ResInfo.AspRatio = pHdmiTx->aviParams.AspectRatio;

    if((resID == RES_1080I60_FP) || (resID == RES_1080I5994_FP) || (resID == RES_1080I50_FP)) {
        ResInfo.HActive     = 1920;
        ResInfo.VActive     = 1080;
        ResInfo.FrmFmt   = 1;
    }

    //Get VIC code
    GetVideoIDForRes (&ResInfo);
    VPPHDMI_DBG_INFO("-- filling up dtd--- resId : %d\n", ResInfo.VideoIDCode);
    if(ResInfo.VideoIDCode == 0){
        VPPHDMI_DBG_ERROR("error getting video id for selected video params[%dx%d R[%d] AR[%d]]\n" ,
                           ResInfo.HActive, ResInfo.VActive,ResInfo.RefreshRate,ResInfo.AspRatio);
        return (MV_VPP_EUNSUPPORT);
    }

    if(dtd_fill(&snps_hdmi->hdmi_tx, &dtd, ResInfo.VideoIDCode, refreshRate) == FALSE){
        VPPHDMI_DBG_INFO("Could not fill DTD with CEA VIC ");
        return (MV_VPP_EUNSUPPORT);
    }

    color_fmt = SYN_mapVppColorFmt(pHdmiTx->videoParams.VideoFmt);
    bit_depth = SYN_mapVppBitDepth(pHdmiTx->videoParams.ColorDepth);

    if (!(snps_hdmi->mode.pVideo.mDtd.mCode == dtd.mCode &&
        snps_hdmi->mode.pVideo.mDtd.mPixelClock == dtd.mPixelClock &&
        snps_hdmi->mode.pVideo.mEncodingOut == color_fmt &&
        snps_hdmi->mode.pVideo.mColorResolution == bit_depth /*&&
        (pHdmiTx->videoParams.PixelRpt+1) == pixel_rept*/))
    {
        GaloisMemcpy (&snps_hdmi->mode.pVideo.mDtd, &dtd, sizeof(dtd_t));
    }
    snps_hdmi->mode.pVideo.mEncodingOut = color_fmt;
    snps_hdmi->mode.pVideo.mEncodingIn = color_fmt;
    snps_hdmi->mode.pVideo.mColorResolution = bit_depth;

    snps_hdmi->mode.pVideo.mColorSpace = pHdmiTx->aviParams.ColorSpace;
    snps_hdmi->mode.pVideo.mRgbQuantizationRange = pHdmiTx->aviParams.RGBQuantRange;
    snps_hdmi->mode.pVideo.mYccQuantizationRange = pHdmiTx->aviParams.YCCQuantRange;
    // Set pixel repetition
    snps_hdmi->mode.pVideo.mPixelRepetitionFactor = pHdmiTx->videoParams.PixelRpt;

    /*TODO: pack AVI infoframe packed inside api_Configure->video_configure*/
    if (updateInfoFrame) {
        fc_avi_config(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pVideo);
    }

    return MV_VPP_OK;
}

INT BE_HDMITX_ConfigHdmiAudioFmt(BE_HDMITX_DATA  *pHdmiTx, int aChannelAllocation, int aLvlShiftVal, int aDownMixInhFlag)
{
    double audSampFreq;
    int clkFactor;

    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    switch (pHdmiTx->audSampFreq)
    {
        case CEA_AUDIO_SAMP_FREQ_32_KHZ: audSampFreq = 32.000;   break;
        case CEA_AUDIO_SAMP_FREQ_44_1_KHZ: audSampFreq = 44.100; break;
        case CEA_AUDIO_SAMP_FREQ_48_KHZ: audSampFreq = 48.000;   break;
        case CEA_AUDIO_SAMP_FREQ_88_2_KHZ: audSampFreq = 88.200; break;
        case CEA_AUDIO_SAMP_FREQ_96_KHZ: audSampFreq = 96.000;   break;
        case CEA_AUDIO_SAMP_FREQ_176_4_KHZ:audSampFreq = 176.400;break;
        case CEA_AUDIO_SAMP_FREQ_192_KHZ:audSampFreq = 192.000;  break;
        default:
            return MV_VPP_EBADPARAM;
    }
    snps_hdmi->mode.pAudio.mSamplingFrequency = audSampFreq;
    // Map reference clock
    switch (pHdmiTx->audFmtParams.InputClk)
    {
        case VPP_BE_HDMITX_AUD_INP_CLK_128fs: clkFactor = 128; break;
        case VPP_BE_HDMITX_AUD_INP_CLK_256fs: clkFactor = 256; break;
        case VPP_BE_HDMITX_AUD_INP_CLK_384fs: clkFactor = 384; break;
        case VPP_BE_HDMITX_AUD_INP_CLK_512fs: clkFactor = 512; break;
        case VPP_BE_HDMITX_AUD_INP_CLK_64fs:  clkFactor = 64; break;
        default: return MV_VPP_EBADPARAM;
    }
    snps_hdmi->mode.pAudio.mClockFsFactor = clkFactor;
    snps_hdmi->mode.pAudio.mChannelAllocation = aChannelAllocation;
    snps_hdmi->mode.pAudio.mLevelShiftValue = aLvlShiftVal & 0x0F;
    snps_hdmi->mode.pAudio.mDownMixInhibitFlag = aDownMixInhFlag;
    if (pHdmiTx->audFmtParams.WordLength > 24)
        snps_hdmi->mode.pAudio.mSampleSize = 24;
    else
        snps_hdmi->mode.pAudio.mSampleSize = pHdmiTx->audFmtParams.WordLength;

    if(pHdmiTx->audFmtParams.HbrAudio || pHdmiTx->audFmtParams.AudioFmt != CEA_AUDIO_FMT_PCM)
        snps_hdmi->mode.pAudio.mGpaInsertPucv = FALSE;
    else
        snps_hdmi->mode.pAudio.mGpaInsertPucv = TRUE;

    snps_hdmi->mode.pAudio.mCodingType = pHdmiTx->audFmtParams.AudioFmt;

    /* TODO: config audio info frame, need to verify arguments */
    fc_audio_info_config(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pAudio);
    return MV_VPP_OK;
}

BOOL BE_HDMITX_PackAudioChnSts(BE_HDMITX_DATA  *pHdmiTx, HDMI_AUD_CHN_STS_DATA pAudioChnSts)
{

    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return FALSE;
    }

    //pDataBuf[0] = (pAudioChnSts); // Leave all 0s for now
    snps_hdmi->mode.pAudio.mIecPcmMode = pAudioChnSts.NonPcm;
    snps_hdmi->mode.pAudio.mIecCategoryCode = pAudioChnSts.CategoryCode;
    snps_hdmi->mode.pAudio.mIecSourceNumber = pAudioChnSts.SrcNum;
    /* remaining values are taken from audio structure */
    /* TODO: check whether we need to right to register here. or audio_Configure will handle 8*/
    return TRUE;
}

INT BE_HDMITX_phy_standby(BE_HDMITX_DATA *pHdmiTx)
{
    u8 data = 0;
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    VPPHDMI_DBG_INFO("setting hdmi tx in standby mode..\n");
    api_Standby(&snps_hdmi->hdmi_tx);
    scdc_write(&snps_hdmi->hdmi_tx, 0x20, 1, &data);
    return MV_VPP_OK;
}

BOOL BE_HDMITX_phy_getState(BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    return phy_get_powerstat(&snps_hdmi->hdmi_tx);
}

INT BE_HDMITX_reset(BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    audio_reset(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pAudio);
    product_reset(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pProduct);
    video_params_reset(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pVideo);
    hdmi_tx_reset_hdcp_params(snps_hdmi);

    // Disable all interrupts
    irq_mask_all(&snps_hdmi->hdmi_tx);
    irq_mute(&snps_hdmi->hdmi_tx); /* disable interrupts */

    /* internally reads the phy model using config2_id::phytype and prints*/
    if(hdmi_tx_set_phy(snps_hdmi, 316, 0/*TODO: phy, phy_debug*/)){
            VPPHDMI_DBG_ERROR("HDMI phy set failed\n");
            return MV_VPP_EUNCONFIG;
    }

    /*if sink connected, read edid and configure VGA*/

    if (audio_Initialize(&snps_hdmi->hdmi_tx) != TRUE) {
        VPPHDMI_DBG_ERROR("%s:audio_Initialize error", __func__);
        return MV_VPP_EUNCONFIG;
    }

    if (packets_Initialize(&snps_hdmi->hdmi_tx) != TRUE) {
        VPPHDMI_DBG_ERROR("%s:packets_Initialize error", __func__);
        return MV_VPP_EUNCONFIG;
    }

    if (hdcp_initialize(&snps_hdmi->hdmi_tx) != TRUE) {
        VPPHDMI_DBG_ERROR("%s:Could not initialize HDCP\n", __func__);
        return MV_VPP_EUNCONFIG;
    }

    if (control_Initialize(&snps_hdmi->hdmi_tx) != TRUE) {
        VPPHDMI_DBG_ERROR("%s:control_Initialize error", __func__);
        return MV_VPP_EUNCONFIG;
    }

    if (phy_initialize(&snps_hdmi->hdmi_tx, snps_hdmi->hdmi_tx_phy) != TRUE) {
        VPPHDMI_DBG_ERROR("%s:phy_initialize error", __func__);
        return MV_VPP_EUNCONFIG;
    }

    control_InterruptClearAll(&snps_hdmi->hdmi_tx);
    // Always due to HDCP and Scrambler
    fc_video_hdcp_keepout(&snps_hdmi->hdmi_tx, TRUE);

    /* configure i2c clocks */
    EDDC_InitializeMaster(pHdmiTx, 0);	/* use mode 1 for fast */

    /* enable hpd interrupt */
#ifdef HDMI_SRV_SUPPORT
    irq_hpd_sense_enable(&snps_hdmi->hdmi_tx);
#endif

#if defined (SNPS_ARC_ENABLE)
    hdmitx_arcrx_enable(pHdmiTx,true);
#endif

#if HDCP2X_ENABLED
    if(TEE_SUCCESS != HDCP2X_InitCtx(&pHdmiTx->hdcp2xSessionHandle)) {
        VPPHDMI_DBG_ERROR("Error initiating TA session to hdcp2x. limit support to HDCP1.4\n");
        pHdmiTx->SourceMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
    } else {
        if(!HDCP2X_IsESMAlive(pHdmiTx)) {
            VPPHDMI_DBG_ERROR("HDCP 2.2 Initialization Failed!!!! Continue with HDCP1.4 as max supported....\n");
            pHdmiTx->SourceMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
        }
    }
#else
    VPPHDMI_DBG_INFO("HDCP2X disabled for the product\n");
    pHdmiTx->SourceMaxSuprtedhdcpVersion = VPP_HDMI_HDCP_VER_HDCP_1_x;
#endif

    // Notify the application, max supported hdcp version by source
    VPPHDMI_DBG_INFO("HDMI CORE version = %x. Max supported hdcp version from source : %s \n",
                     (id_design(&snps_hdmi->hdmi_tx) << 8) | id_revision(&snps_hdmi->hdmi_tx),
                     (pHdmiTx->SourceMaxSuprtedhdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2)?"HDCP2.2":"HDCP1.4");
    return MV_VPP_OK;
}

INT BE_HDMITX_SetACPPacket(BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_ACP *pAcpPktData)
{
    LOG_TRACE();

    u8 newFields[ACP_PACKET_SIZE];
    u16 i = 0;
    u8 length;
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (pAcpPktData == NULL) {
        return MV_VPP_EBADPARAM;
    }
    if (pAcpPktData->dataLen > 28) {
        return MV_VPP_EBADPARAM;
    }

    length = pAcpPktData->dataLen;
    //fc_packets_AutoSend(dev, 0, ACP_TX);

    fc_acp_type(&snps_hdmi->hdmi_tx, pAcpPktData->type);

    if (pAcpPktData->type == VPP_HDMI_PKT_ACP_DVD_AUDIO ||
        pAcpPktData->type == VPP_HDMI_PKT_ACP_SACD)
    {
        GaloisMemcpy (&newFields[0], pAcpPktData->dataBuf, pAcpPktData->dataLen);
    }
    if (length < ACP_PACKET_SIZE) {
        for (i = length; i < ACP_PACKET_SIZE; i++) {
            newFields[i] = 0;    /* Padding */
        }
        length = ACP_PACKET_SIZE;
    }
    fc_acp_type_dependent_fields(&snps_hdmi->hdmi_tx, newFields, length);
    return MV_VPP_OK;
}

INT BE_HDMITX_SetISRC1Packet (BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_ISRC1 *pIsrc1PktData)
{
    u8 newCodes[ISRC_PACKET_SIZE];

    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (pIsrc1PktData == NULL) {
        return MV_VPP_EBADPARAM;
    }


    GaloisMemcpy (newCodes, pIsrc1PktData->upc_ean_fld, ISRC_PACKET_SIZE);

    fc_isrc_status(&snps_hdmi->hdmi_tx, pIsrc1PktData->sts);
    fc_isrc_valid(&snps_hdmi->hdmi_tx, pIsrc1PktData->valid);
    fc_isrc_cont(&snps_hdmi->hdmi_tx, pIsrc1PktData->cont);
    fc_isrc_isrc1_codes(&snps_hdmi->hdmi_tx, newCodes, ISRC_PACKET_SIZE);	/* first part only */
    return MV_VPP_OK;
}

INT BE_HDMITX_SetISRC2Packet (BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_ISRC2 *pIsrc2PktData)
{

    u8 newCodes[ISRC_PACKET_SIZE];
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (pIsrc2PktData == NULL) {
        return MV_VPP_EBADPARAM;
    }
    GaloisMemcpy (newCodes, pIsrc2PktData->upc_ean_fld, ISRC_PACKET_SIZE);

    fc_isrc_isrc2_codes(&snps_hdmi->hdmi_tx, newCodes, ISRC_PACKET_SIZE);
    /*TODO: should we change fc_isrc_cont to 1 */
    return MV_VPP_OK;
}

INT BE_HDMITX_SetSPDInfoFrame (BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_SPD_INFOFRM *pSPDData)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (pSPDData == NULL)
    {
        return MV_VPP_EBADPARAM;
    }
    fc_spd_VendorName(&snps_hdmi->hdmi_tx, pSPDData->vendorName, 8);
    fc_spd_ProductName(&snps_hdmi->hdmi_tx, pSPDData->prodDescChar, 16);

    fc_spd_SourceDeviceInfo(&snps_hdmi->hdmi_tx, pSPDData->srcDev);
    return MV_VPP_OK;
}

INT BE_HDMITX_SetGMDPacket (BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_PKT_GMD *pGmdPktData)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (pGmdPktData == NULL) {
        return MV_VPP_EBADPARAM;
    }

    fc_gamut_Profile(&snps_hdmi->hdmi_tx, pGmdPktData->gdbProfile);
    //fc_gamut_AffectedSeqNo(&snps_hdmi->hdmi_tx, (fc_gamut_CurrentSeqNo(dev) + 1) % 16); /* sequential */
    fc_gamut_AffectedSeqNo(&snps_hdmi->hdmi_tx, pGmdPktData->affectedGamutSeqNum); /* TODO: always 0 in vpp_isr.c */
    fc_gamut_Content(&snps_hdmi->hdmi_tx, pGmdPktData->gbdData, 28);
    fc_gamut_UpdatePacket(&snps_hdmi->hdmi_tx); /* set next_field to 1 */

    return MV_VPP_OK;
}

INT BE_HDMITX_SetHdcpVersion(BE_HDMITX_DATA *pHdmiTx, VPP_HDMI_HDCP_VERSION hdcpVersion)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hdcp_on = true;
    snps_hdmi->mode.pHdcp.hdcp_requested_state = hdcpVersion;
    if (hdcpVersion == VPP_HDMI_HDCP_VER_HDCP_1_x) {
        VPPHDMI_DBG_INFO("setting snps hdcp requested state to HDCP1.4 and current state to in progress\n");
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_1_4_IN_PROGRESS;
    } else if (hdcpVersion == VPP_HDMI_HDCP_VER_HDCP_2_2) {
        VPPHDMI_DBG_INFO("setting snps hdcp requested state to HDCP2.2 and current state to in progress\n");
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_2_2_IN_PROGRESS;
    }
    return MV_VPP_OK;
}

HDCP_ENGAGE_STATE BE_HDMITX_GetActiveHdcpState (BE_HDMITX_DATA *pHdmiTx)
{
    HDCP_ENGAGE_STATE authState = HDCP_STATE_INACTIVE;
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return HDCP_STATE_INACTIVE;
    }

    authState =  snps_hdmi->mode.pHdcp.hdcp_current_state;

    return authState;
}

void BE_HDMITX_StopHdcpAuthentication (BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return;
    }

    if(snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hdcp_on == true) {
        hdcp_14_stop_authentication(snps_hdmi);
        hdcp_22_stop_authentication(pHdmiTx);
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_INACTIVE;
        snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hdcp_on = false;      /* check this variable on start of auth*/
    } else {
        VPPHDMI_DBG_INFO("HDCP is in disable state alreay..\n");
    }
}

#if HDCP2X_ENABLED
INT BE_HDMITX_Hdcp2xPreauth (BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    VPPHDMI_DBG_INFO("BE_HDMITX_Hdcp2xPreauth called\n");
    //hdcp_22_clearMessage(pHdmiTx);
    mc_hdcp_clock_disable(&snps_hdmi->hdmi_tx, FALSE);

    /* configure for hdcp2.2 */
    snps_hdmi->mode.pHdcp.hdcp_requested_state = VPP_HDMI_HDCP_VER_HDCP_2_2;
    hdcp_configure(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pHdcp, &snps_hdmi->mode.pVideo);
    HDCP2X_LoadDownstreamCapability(pHdmiTx);
    return MV_VPP_OK;
}
#endif

INT BE_HDMITX_StartHdcpAuthentication (BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    if (snps_hdmi->mode.pHdcp.hdcp_requested_state == VPP_HDMI_HDCP_VER_HDCP_1_x) {

        VPP_Memset (snps_hdmi->mode.pHdcp.mKsvListBuffer, 0, (sizeof(u8) * HDCP_KSV_BUFFER_SIZE));
        // Read Bstatus bfore HDCP Authentication, sometimes auth failure interrupts comes late
        be_hdmitx_update_bstatus(pHdmiTx);
        //hdcp_interrupt_clear(); done inside the config
        hdcp_configure(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pHdcp, &snps_hdmi->mode.pVideo);
    }
#if HDCP2X_ENABLED
    else if (snps_hdmi->mode.pHdcp.hdcp_requested_state == VPP_HDMI_HDCP_VER_HDCP_2_2) {
        hdcp_22_clearMessage(pHdmiTx);
        mc_hdcp_clock_disable(&snps_hdmi->hdmi_tx, FALSE);

        /* configure for hdcp2.2 */
        hdcp_configure(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pHdcp, &snps_hdmi->mode.pVideo);

        /* start hdcp2.2 */
        if ( HDCP2X_ERR_OK != HDCP2X_StartAuthentication(pHdmiTx)) {
            /* TODO : add proper error definitions for hdcp to be handled by vpp_be.c */
            VPPHDMI_DBG_ERROR("Error starting hdcp2.2 ... \n");
            snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_2_2_ERR_REAUTH;
            return MV_VPP_EBADCALL;
        } else {
            VPPHDMI_DBG_ERROR("Hdcp2.2 authentication started.... \n");
            snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_2_2_IN_PROGRESS;
            return MV_VPP_OK;
        }
    }
#else
    return MV_VPP_EUNSUPPORT;
#endif
    return MV_VPP_OK;
}

#if HDCP2X_ENABLED
HDMITX_HDCP_RETURN_STATE BE_HDMITX_CheckHdcp2xAuthStatus(BE_HDMITX_DATA *pHdmiTx)
{
    INT retVal = MV_VPP_OK;
    HDMITX_HDCP_RETURN_STATE ret_auth_state;
    HDCP2X_ESM_MONITOR_RET esm_monitor_status;
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    retVal = HDCP2X_GetAuthenticationState(pHdmiTx, &esm_monitor_status);
    if (retVal != MV_VPP_OK) {
       VPPHDMI_DBG_ERROR("error getting authentication status from ESM....: 0x%x \n", retVal);
       return HDCP14_AUTH_RETRY;
    }

    switch (esm_monitor_status) {
    case HDCP2X_ESM_MON_ERR_ESM_AUTHENTICATED:
        retVal = HDCP2X_GetTopology(pHdmiTx);

        if((pHdmiTx->hdcp2xRptrTopology.rxInfo.downstream_1x_dev) && (pHdmiTx->hdmiConfig.hdcpFallbackDwnstrm1_4 == 1)) {
            //If sink of AVR is 1.4 fallback to hdcp 1.4
            VPPHDMI_DBG_INFO("Fallback to hdcp 1.4 due to DS sinks in AVR..\n");
            ret_auth_state = HDCP14_AUTH_RETRY;
            break;
        }
        if((retVal == MV_VPP_OK) && (pHdmiTx->hdcp2xRptrTopology.rxInfo.dev_cnt > 1)) {
            VPPHDMI_DBG_INFO("HDCP2X authenticated with downstream repeater:%d\n", pHdmiTx->hdcp2xRptrTopology.rxInfo.dev_cnt);
            snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_2_2_AUTHENTICATED_RPTR;
        } else {
            VPPHDMI_DBG_INFO("HDCP2X authenticated with downstream receiver. retVal:%d\n", retVal);
            snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_2_2_AUTHENTICATED_RX;
        }
        ret_auth_state = HDCP22_AUTH_DONE;
    break;

    case HDCP2X_ESM_MON_ERR_TIMEOUT:
    case HDCP2X_ESM_MON_CSM_UPDATED:
        ret_auth_state = HDCP22_AUTH_INPROGESS;
    break;

    case HDCP2X_ESM_MON_ERR_AUTH_FAILURE:
    case HDCP2X_ESM_MON_ERR_AUTH_START_FAILURE:
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_2_2_ERR_REAUTH;
        hdcp_22_clearMessage(pHdmiTx);
        ret_auth_state = HDCP22_AUTH_RETRY;
    break;

    case HDCP2X_ESM_MON_ERR_HPD:
        hdcp_22_stop_authentication(pHdmiTx);
        ret_auth_state = HDCP_SET_IDLE_STATE;
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_INACTIVE;
    break;

    case HDCP2X_ESM_MON_ERR_LINK_INTEGRITY_FAIL:
        if(pHdmiTx->activeHdcpMode == VPP_HDMI_HDCP_MODE_RPTR) {
            //rx side will restart auth
            hdcp_22_stop_authentication(pHdmiTx);
            snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_INACTIVE;
        } else {
            snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_2_2_ERR_REAUTH;
        }
        ret_auth_state = HDCP22_AUTH_LINK_INTEG;
    break;

    case HDCP2X_ESM_MON_ERR_HLC_ABORT:
    case HDCP2X_ESM_MON_ERR_SEQ_ERROR:
    case HDCP2X_ESM_MON_ERR_SINK_UNSUPPOTED:
        ret_auth_state = HDCP14_AUTH_RETRY;
    break;

    default:
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_2_2_ERR_REAUTH;
        ret_auth_state = HDCP22_AUTH_RETRY;
    break;
    }

    return ret_auth_state;
}

void BE_HDMITX_TriggerHdcpFallback(BE_HDMITX_DATA  *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_1_4_IN_PROGRESS;
    hdcp_22_stop_authentication(pHdmiTx);
    BE_HDMITX_ResetTMDS(pHdmiTx);
}
#endif

INT BE_HDMITX_GetIrqSource(BE_HDMITX_DATA *pHdmiTx)
{
    uint32_t decode = 0;
    uint8_t phy_decode = 0;
    uint8_t hdcp_irq = 0;
    int execute = 0;

    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    // add hdcp2x interrupts from hdcp22reg_sts
    decode = read_interrupt_decode(&snps_hdmi->hdmi_tx);
    hdcp_irq = hdcp_interrupt_status(&snps_hdmi->hdmi_tx) & ~(hdcp_InterruptMaskStatus(&snps_hdmi->hdmi_tx));

    VPPHDMI_DBG_INFO("decode:0x%x  hdcp_irq:0x%x \n",decode, hdcp_irq);
    if(hdcp_irq != 0){
        //hdcp_interrupt_clear(&snps_hdmi->hdmi_tx, hdcp_irq);
        VPPHDMI_DBG_INFO("hdcp interrupt status : 0x%x\n", hdcp_irq);
        return VPP_HDMI_IRQ_HDCP;
    }
    if(decode_is_fc_stat0(decode)){
        irq_clear_source(&snps_hdmi->hdmi_tx, AUDIO_PACKETS);
    }

    if(decode_is_fc_stat1(decode)){
        irq_clear_source(&snps_hdmi->hdmi_tx, OTHER_PACKETS);
    }

    if(decode_is_fc_stat2_vp(decode)){
        // TODO: mask this for now...
        irq_mute_source(&snps_hdmi->hdmi_tx, PACKETS_OVERFLOW);
        irq_mute_source(&snps_hdmi->hdmi_tx, VIDEO_PACKETIZER);
    }
    if(decode_is_as_stat0(decode)){
        irq_clear_source(&snps_hdmi->hdmi_tx, AUDIO_SAMPLER);
    }
    if(decode_is_phy(decode)){

        irq_read_stat(&snps_hdmi->hdmi_tx, PHY, &phy_decode);
        //LOGGER(SNPS_TRACE, "%s:PHY interrupt 0x%08x: ih_stat 0x%x phy_intr 0x%x phy_stat 0x%x", __func__, decode, phy_decode, dev_read(&snps_hdmi->hdmi_tx, 0xC014), dev_read(&snps_hdmi->hdmi_tx, 0xC010));
        //hdmitx_sleep(20);

        if(decode_is_phy_lock(phy_decode)){
            irq_clear_bit(&snps_hdmi->hdmi_tx, PHY, IH_PHY_STAT0_TX_PHY_LOCK_MASK);
        }

        if(decode_is_phy_rx_s0(phy_decode)) {
            //LOGGER(SNPS_WARN, "Rx sense 0 received");
            //phy_rx_s0_detected(&snps_hdmi->hdmi_tx);
            execute = 1;
        }

        if(decode_is_phy_rx_s1(phy_decode)) {
            //LOGGER(SNPS_WARN, "Rx sense 1 received");
            //phy_rx_s1_detected(&snps_hdmi->hdmi_tx);
            execute = 1;
        }

        if(decode_is_phy_rx_s2(phy_decode)) {
            //LOGGER(SNPS_WARN, "Rx sense 2 received");
            //phy_rx_s2_detected(&snps_hdmi->hdmi_tx);
            execute = 1;
        }

        if(decode_is_phy_rx_s3(phy_decode)) {
            //LOGGER(SNPS_WARN, "Rx sense 3 received");
            //phy_rx_s3_detected(&snps_hdmi->hdmi_tx);
            execute = 1;
        }

        if(decode_is_phy_hpd(phy_decode)) {
            //LOGGER(SNPS_WARN, "HPD received");
            //phy_hot_plug_detected(&snps_hdmi->hdmi_tx);
            execute = 1;
        }

        if(execute) {
            return VPP_HDMI_IRQ_HPD;
        } else {
            return VPP_HDMI_IRQ_UNDEFINED; //phy lock interrupt need not be processed
        }

    }
    if(decode_is_i2c_stat0(decode)){
        uint8_t state = 0;

        irq_read_stat(&snps_hdmi->hdmi_tx, I2C_DDC, &state);

        // I2Cmastererror - I2Cmasterdone
        if(state & 0x3){
            irq_clear_bit(&snps_hdmi->hdmi_tx, I2C_DDC, IH_I2CM_STAT0_I2CMASTERDONE_MASK);
            //The I2C communication interrupts must be masked - they will be handled by polling in the eDDC block
            LOGGER(SNPS_INFO, "%s:I2C DDC interrupt received 0x%x - mask interrupt", __func__, state);
        }
        // SCDC_READREQ
        else if(state & 0x4){
            irq_clear_bit(&snps_hdmi->hdmi_tx, I2C_DDC, IH_I2CM_STAT0_SCDC_READREQ_MASK);
        }
    }
    return VPP_HDMI_IRQ_UNDEFINED;
}

INT BE_HDMITX_ReadEdidBlock(BE_HDMITX_DATA *pHdmiTx, UINT32 blockNum, UINT8 *pDataBuf)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    if(blockNum == 0)
        return edid_read(&snps_hdmi->hdmi_tx, pDataBuf);
    else
        return edid_extension_read(&snps_hdmi->hdmi_tx, blockNum, pDataBuf);
}

INT BE_HDMITX_ReadChecksum(BE_HDMITX_DATA *pHdmiTx, int blks, UINT8 *pChecksum)
{
     SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    edid_cs_read(&snps_hdmi->hdmi_tx, blks, pChecksum);
    return MV_VPP_OK;
}

INT BE_HDMITX_UpdateHPDStateChange(BE_HDMITX_DATA *pHdmiTx, int rxConnected)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX yet to be initialized..\n");
        return MV_VPP_EUNCONFIG;
    }
    if (rxConnected) {
        snps_hdmi->mode.hpd = 1;
        snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hpd = 1;
#if defined(HDMI_SRV_SUPPORT)
        audio_reset(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pAudio);
        product_reset(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pProduct);
        video_params_reset(&snps_hdmi->hdmi_tx, &snps_hdmi->mode.pVideo);
#endif
        snps_hdmi->mode.pVideo.mHdmi = pHdmiTx->curOutHdmiMode;
    } else {
        snps_hdmi->mode.hpd = 0;
        snps_hdmi->hdmi_tx.snps_hdmi_ctrl.hpd = 0;
    }

    if(!pHdmiTx->hdmiHpdControlOff) {
        /*Disable HPD interrupt for android boot fastlogo*/
        irq_hpd_sense_enable(&snps_hdmi->hdmi_tx);
    }

    return MV_VPP_OK;
}

INT BE_HDMITX_EnableInterrupts(BE_HDMITX_DATA *pHdmiTx, VPP_BE_HDMITX_IRQ_SOURCE irq_src, BOOL enable)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    switch (irq_src) {
        case IRQ_SINKDET:
            if(enable == true) {
                irq_unmask_bit(&snps_hdmi->hdmi_tx, PHY, IH_PHY_STAT0_HPD_MASK |
                                                     IH_PHY_STAT0_RX_SENSE_0_MASK |
                                                     IH_PHY_STAT0_RX_SENSE_1_MASK |
                                                     IH_PHY_STAT0_RX_SENSE_2_MASK |
                                                     IH_PHY_STAT0_RX_SENSE_3_MASK);
                phy_interrupt_unmask(&snps_hdmi->hdmi_tx, PHY_STAT0_HPD_MASK |
                                                          PHY_STAT0_RX_SENSE_0_MASK |
                                                          PHY_STAT0_RX_SENSE_1_MASK |
                                                          PHY_STAT0_RX_SENSE_2_MASK |
                                                          PHY_STAT0_RX_SENSE_3_MASK);
            } else {
                irq_mask_bit(&snps_hdmi->hdmi_tx, PHY, IH_PHY_STAT0_HPD_MASK |
                                                     IH_PHY_STAT0_RX_SENSE_0_MASK |
                                                     IH_PHY_STAT0_RX_SENSE_1_MASK |
                                                     IH_PHY_STAT0_RX_SENSE_2_MASK |
                                                     IH_PHY_STAT0_RX_SENSE_3_MASK);
                phy_interrupt_mask(&snps_hdmi->hdmi_tx, PHY_STAT0_HPD_MASK |
                                                        PHY_STAT0_RX_SENSE_0_MASK |
                                                        PHY_STAT0_RX_SENSE_1_MASK |
                                                        PHY_STAT0_RX_SENSE_2_MASK |
                                                        PHY_STAT0_RX_SENSE_3_MASK);
            }
        break;

        default:
            VPPHDMI_DBG_ERROR("selected interrupt %d not populated..\n", irq_src);
        break;
    }

    return MV_VPP_OK;
}

void BE_HDMITX_HDCP14_Reset(BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    VPPHDMI_DBG_INFO("trigger hdcp14 reset on repeated Ri failure(%d)\n", RI_ERR_THRESHOLD);
    hdcp_rxdetect(&snps_hdmi->hdmi_tx, false);
    hdcp_bypass_encryption(&snps_hdmi->hdmi_tx, false);
    MV_OSAL_Task_Sleep(HDCP14_CORE_RESET_TIMEOUT);
    hdcp_sw_reset(&snps_hdmi->hdmi_tx);
    MV_OSAL_Task_Sleep(HDCP14_CORE_RESET_TIMEOUT*2);
    hdcp_rxdetect(&snps_hdmi->hdmi_tx, true);
    MV_OSAL_Task_Sleep(HDCP14_CORE_RESET_TIMEOUT);
}

void BE_HDMITX_HDCP14_InterruptEnable(BE_HDMITX_DATA *pHdmiTx, uint8_t mask, int enable)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    uint8_t hdcp_irq_mask = 0;

    hdcp_irq_mask = hdcp_InterruptMaskStatus(&snps_hdmi->hdmi_tx);
    if (enable) {
        hdcp_irq_mask &= ~mask;
    } else {
        hdcp_irq_mask |= mask;
    }
    VPPHDMI_DBG_INFO("setting HDCP inter mask to : 0x%x\n", hdcp_irq_mask);
    hdcp_InterruptMask(&snps_hdmi->hdmi_tx, hdcp_irq_mask);
}

VPP_BE_HDCP14_INTR_STAT BE_HDMITX_HDCP14_InterruptHandler(BE_HDMITX_DATA *pHdmiTx)
{
    uint8_t hdcp_irq = 0;
    int hdcp_auth_stat = 0;
    int retVal = HDCP14_AUTH_FAIL;
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return HDCP14_AUTH_UNDEFINED;
    }

    hdcp_irq = hdcp_interrupt_status(&snps_hdmi->hdmi_tx);

    if (hdcp_irq & HDCP14_INTR_MASK_AUTH_PASS){
        /* handle this first as failure interrupt will be masked to avoid continuous trigger
           and if auth pass eventually, fail status will also be set */
        if (pHdmiTx->sm_state == HDCP_SM_STATE_REAUTH) {
            snps_hdmi->RiErrCnt++;
            if (snps_hdmi->RiErrCnt >= RI_ERR_THRESHOLD) {
               BE_HDMITX_HDCP14_Reset(pHdmiTx);
               snps_hdmi->RiErrCnt = 0;
            }
        } else {
            snps_hdmi->RiErrCnt = 0;
        }

        VPPHDMI_DBG_INFO("---HDCP 1.4 AUTHENTICATION PASS--\n");
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_1_4_AUTHENTICATED;
        retVal = HDCP14_AUTH_PASS;
    }else if (hdcp_irq & HDCP14_INTR_MASK_AUTH_FAIL) {
        VPPHDMI_DBG_INFO("---hdcp 1.4 authentication failure interrupt triggered--\n");
        /* mask hdcp 1.4 auth failure interrupt to avoid continuous retriggering of interrupts.
           core will auto restart authentication on failure. Interrupt will be re-enabled on
           next hdcp config
        */
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_1_4_ERR_REAUTH;
        be_hdmitx_update_bstatus(pHdmiTx);
        retVal = HDCP14_AUTH_FAIL;
    } else if (hdcp_irq & HDCP14_INTR_MASK_I2C_FAIL) {
        VPPHDMI_DBG_INFO("---hdcp 1.4 I2c errors seen--\n");
        snps_hdmi->mode.pHdcp.hdcp_current_state = HDCP_STATE_1_4_ERR_REAUTH;
        retVal = HDCP14_AUTH_COMM_ERR;
    } else if (hdcp_irq & HDCP14_INTR_MASK_AUTH_KSV_FAIL) {
        VPPHDMI_DBG_INFO("---hdcp 1.4 ksv verification interrupt--\n");
        retVal = hdcp_event_handler(&snps_hdmi->hdmi_tx, &hdcp_auth_stat);
        if (HDCP_KSV_LIST_READY == retVal) {
            retVal = HDCP14_AUTH_INTERNAL;
            VPPHDMI_DBG_INFO("---ksv validation done.--\n");
        } else {
            VPPHDMI_DBG_INFO("---ksv validation failed: error - 0x%x.--\n", retVal);
            retVal = HDCP14_AUTH_KSV_FAIL;
        }
    }
    hdcp_interrupt_clear(&snps_hdmi->hdmi_tx, hdcp_irq);
    return retVal;
}

INT BE_HDMITX_I2CBusClear(BE_HDMITX_DATA *pHdmiTx)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    i2c_bus_clear(&snps_hdmi->hdmi_tx);
    return MV_VPP_OK;
}

INT BE_HDMITX_WrapperEnableFixColor(BE_HDMITX_DATA *pHdmiTx, int enable)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    UINT32 base_addr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_BASE;
    T32HDMI_TX_WRAP_VID_CTRL vid_ctrl;
    T32HDMI_TX_WRAP_VID_CTRL1 vid_ctrl1;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    GA_REG_WORD32_READ(base_addr + RA_HDMI_TX_WRAP_VID_CTRL, &vid_ctrl.u32);
    GA_REG_WORD32_READ(base_addr + RA_HDMI_TX_WRAP_VID_CTRL1, &vid_ctrl1.u32);
    /* enable fix color transmission from hdmi tx wrapper.
       Below values configure black screen for different formats
     */
    if((snps_hdmi->hdmi_tx.snps_hdmi_ctrl.encoding == YCC422)
        || (snps_hdmi->hdmi_tx.snps_hdmi_ctrl.encoding == YCC444)) {
        vid_ctrl.uVID_CTRL_fix_clr_ch0 = 0x800;
        vid_ctrl.uVID_CTRL_fix_clr_ch1 = 0x00;
        vid_ctrl1.uVID_CTRL_fix_clr_ch2 = 0x800;
    } else if (snps_hdmi->hdmi_tx.snps_hdmi_ctrl.encoding == YCC420) {
        vid_ctrl.uVID_CTRL_fix_clr_ch0 = 0x00;
        vid_ctrl.uVID_CTRL_fix_clr_ch1 = 0x00;
        vid_ctrl1.uVID_CTRL_fix_clr_ch2 = 0x800;
    } else {
        vid_ctrl.uVID_CTRL_fix_clr_ch0 = 0x00;
        vid_ctrl.uVID_CTRL_fix_clr_ch1 = 0x00;
        vid_ctrl1.uVID_CTRL_fix_clr_ch2 = 0x00;
    }
    vid_ctrl.uVID_CTRL_fix_clr_sel = enable;
    GA_REG_WORD32_WRITE(base_addr + RA_HDMI_TX_WRAP_VID_CTRL1, vid_ctrl1.u32);
    GA_REG_WORD32_WRITE(base_addr + RA_HDMI_TX_WRAP_VID_CTRL, vid_ctrl.u32);
    return (MV_VPP_OK);
}

INT BE_HDMITX_ScramblingEnable(BE_HDMITX_DATA *pHdmiTx, BOOL enable)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    scrambling(&snps_hdmi->hdmi_tx, enable);
    return (MV_VPP_OK);
}

INT BE_HDMITX_SetHdmiPhy(BE_HDMITX_DATA *pHdmiTx, unsigned char * phySetting, unsigned int PhyBufSize)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    struct phy_config_dynamic *conf_dyn;
    int i, size, offset;
    size = sizeof(struct phy_config_dynamic);
    VPPHDMI_DBG_ERROR("Size : %d : %d\n", size, PhyBufSize);
    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    if (!((PhyBufSize == HDMI_PHYCONFIG_3RANGE_SIZE) ||
            (PhyBufSize == HDMI_PHYCONFIG_4RANGE_SIZE))) {
        VPPHDMI_DBG_ERROR("Buffer size is wrong : %d\n", PhyBufSize);
        return MV_VPP_EBADPARAM;
    }
    conf_dyn = snps_hdmi->hdmi_tx.phy_conf;
    offset = 0;
    for (i = 0; i < MAX_FREQ_GBPS ; i++) {
        if (offset < (PhyBufSize - size))
            offset = i * size;
        else /* Copy 3.71GBPS values for GT_3.71GBPS values if PHY settings is is 24 */
            offset = (i-1) * size;
        conf_dyn[i].txterm = phySetting[offset] & 0x7;
        conf_dyn[i].vlevctrl = (phySetting[++offset] & 0x1F) << 5;
        conf_dyn[i].cksymtxctrl = (phySetting[++offset] & 0xF) << 0;
        conf_dyn[i].cksymtxctrl |= (phySetting[++offset] & 0xF) << 4;
        conf_dyn[i].cksymtxctrl |= (phySetting[++offset] & 0x3) << 8;
        conf_dyn[i].cksymtxctrl |= ((phySetting[++offset] & 0xF) << 10) | (1 << 15);
        conf_dyn[i].txrescalctrl = phySetting[++offset] & 0xFF;
        conf_dyn[i].txrescalctrl |= (phySetting[++offset] & 0xFF) << 8;
        VPPHDMI_DBG_INFO("txterm[%d] : 0x%X\n", i, conf_dyn[i].txterm);
        VPPHDMI_DBG_INFO("vlevctrl[%d] : 0x%X\n", i, conf_dyn[i].vlevctrl);
        VPPHDMI_DBG_INFO("cksymtxctrl[%d] : 0x%X\n", i, conf_dyn[i].cksymtxctrl);
        VPPHDMI_DBG_INFO("txrescalctrl[%d] : 0x%X\n\n", i, conf_dyn[i].txrescalctrl);
    }
    return MV_VPP_OK;
}

INT BE_HDMITX_EnableRxSense(BE_HDMITX_DATA *pHdmiTx, int enable)
{
#ifdef HDMI_SRV_SUPPORT
    SNPS_HDMI_TX *snps_hdmi = NULL;
#endif
    if((NULL == pHdmiTx) || (NULL == pHdmiTx->vendorHdmiTx)){
        VPPHDMI_DBG_ERROR("hdmitx elements not initialized \n");
        return MV_VPP_EUNCONFIG;
    }

    pHdmiTx->hdmiConfig.enableRxsenseCheck = (enable)? true: false;
#ifdef HDMI_SRV_SUPPORT
    snps_hdmi = pHdmiTx->vendorHdmiTx;
    VPPHDMI_DBG_INFO("Setting Rxsense enable to : %d..\n", pHdmiTx->hdmiConfig.enableRxsenseCheck);
    irq_rxsense_control(&snps_hdmi->hdmi_tx, pHdmiTx->hdmiConfig.enableRxsenseCheck);
#endif
    return (MV_VPP_OK);
}

#if HDCP2X_ENABLED
INT BE_HDMITX_UpdateContentStrmInfo(BE_HDMITX_DATA *pHdmiTx, HDCP2X_STEAM_TYPE hdcp_content_type)
{
    return HDCP2X_UpdateContentStrmInfo(pHdmiTx, hdcp_content_type);
}
#endif

bool BE_HDMITX_HDCP2XIsESMAlive(BE_HDMITX_DATA *pHdmiTx)
{
#if HDCP2X_ENABLED
    return HDCP2X_IsESMAlive(pHdmiTx);
#else
    return FALSE;
#endif
}

INT BE_HDMITX_PhyPowerUp(BE_HDMITX_DATA *pHdmiTx)
{
    bool phyStatus;
    phyStatus = phy316_powerUp(&pHdmiTx->vendorHdmiTx->hdmi_tx);
    if(phyStatus == false) {
        VPPHDMI_DBG_ERROR("phy powerup failed \n");
        return MV_VPP_EIOFAIL;
    }

    return MV_VPP_OK;
}

unsigned char BE_HDMITX_ConfigureScrambler(BE_HDMITX_DATA *pHdmiTx)
{
    return phy316_configureScrambler(&pHdmiTx->vendorHdmiTx->hdmi_tx);
}


int BE_HDMITX_PowerOffTMDS(BE_HDMITX_DATA *pHdmiTx, int enable)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }

    return api_PowerOffTMDS(&snps_hdmi->hdmi_tx, enable) ? MV_VPP_OK: MV_VPP_EIOFAIL;
}

#if HDCP2X_ENABLED
void BE_HDMITX_HDCP2XDeInitCtx(BE_HDMITX_DATA *pHdmiTx)
{
    HDCP2X_DeInitCtx(pHdmiTx->hdcp2xSessionHandle);
}

int BE_HDMITX_HDCP2X_Resume(BE_HDMITX_DATA *pHdmiTx)
{
    return HDCP2X_Resume(pHdmiTx);
}

int BE_HDMITX_HDCP2X_Suspend(BE_HDMITX_DATA *pHdmiTx)
{
    return HDCP2X_Suspend(pHdmiTx);
}
#else
void BE_HDMITX_HDCP2XDeInitCtx(BE_HDMITX_DATA *pHdmiTx)
{
}

int BE_HDMITX_HDCP2X_Resume(BE_HDMITX_DATA *pHdmiTx)
{
    return MV_VPP_OK;
}

int BE_HDMITX_HDCP2X_Suspend(BE_HDMITX_DATA *pHdmiTx)
{
    return MV_VPP_OK;
}
#endif

INT BE_HDMITX_GetHdcpKsv(BE_HDMITX_DATA *pHdmiTx, VPP_HDCP_KSV_DATA *pHDCPKsv)
{
#if defined (HDCP_REPEATER_MODE)
    hdcpParams_t *hdcp_params;
    u16 bstatus = 0;
    int dev_count;
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;
    u8 devCount;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    hdcp_params = &snps_hdmi->hdmi_tx.hdcp;
    if (hdcp_params->mKsvListBuffer == NULL)
    {/* TODO: */
        return MV_VPP_EUNCONFIG;
    }
    VPP_Memset(pHDCPKsv, 0, sizeof(VPP_HDCP_KSV_DATA));
    devCount = hdcp_params->devCount;
    bstatus = hdcp_params->mKsvListBuffer[devCount*5] | (hdcp_params->mKsvListBuffer[devCount*5 + 1] <<8);
    pHDCPKsv->BStatus.Depth = (bstatus & BSTATUS_DEPTH_MASK) >> 8;
    pHDCPKsv->BStatus.MaxDeviceExceeded = (bstatus & BSTATUS_MAX_DEVS_EXCEEDED_MASK) >> 7;
    pHDCPKsv->BStatus.MaxCascadeExceeded = (bstatus & BSTATUS_MAX_CASCADE_EXCEEDED_MASK) >> 11;
    VPPHDMI_DBG_INFO("Bstatus : %x, devCount :%d, depth:%d, maxDev:%d, maxDCas:%d\n",
                        bstatus, devCount, pHDCPKsv->BStatus.Depth,
                        pHDCPKsv->BStatus.MaxDeviceExceeded, pHDCPKsv->BStatus.MaxCascadeExceeded);
    dev_count = bstatus & BSTATUS_DEVICE_COUNT_MASK;
    pHDCPKsv->KsvLen = dev_count *5;
    if ((dev_count + 1) >= hdcp_params->maxDevices) {
        pHDCPKsv->BStatus.MaxDeviceExceeded = 1;
        VPPHDMI_DBG_INFO("KSV Max device count exceeded\n");
    } else if (pHDCPKsv->BStatus.Depth >= 7) {
        pHDCPKsv->BStatus.MaxCascadeExceeded = 1;
        VPPHDMI_DBG_INFO("KSV Max cascade devices exceeded\n");
    }
    VPP_Memcpy(pHDCPKsv->KsvBuf, &hdcp_params->mKsvListBuffer[0], pHDCPKsv->KsvLen);
    hdcp_BksvRead(&snps_hdmi->hdmi_tx, pHDCPKsv->KsvBuf + pHDCPKsv->KsvLen);
    pHDCPKsv->KsvLen += 5;
#endif
    return MV_VPP_OK;
}

INT BE_HDMITX_HDCP2XSetOpMode(BE_HDMITX_DATA *pHdmiTx, INT opMode)
{
#if HDCP2X_ENABLED
    return HDCP2X_SetMode(pHdmiTx, opMode);
#else
    return MV_VPP_OK;
#endif
}

void BE_HDMITX_SRAM_PowerOff(void)
{
#if HDCP2X_ENABLED
    unsigned int addr = 0;

    /* AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_BASE is mapped in tee_ta_vpp_memmap
       to power down HDCP22 SRAM for Tx power off */
    addr = MEMMAP_AVIO_REG_BASE+ AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_BASE +
           RA_HDCP22_WRAP_AIC_SRAMPWR;
    GA_REG_WORD32_WRITE(addr, HDMITX_SRAMPWR_OFF);

    addr = MEMMAP_AVIO_REG_BASE+ AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_BASE +
           RA_HDCP22_WRAP_TCM_SRAMPWR;
    GA_REG_WORD32_WRITE(addr, HDMITX_SRAMPWR_OFF);

    addr = HDMITX_WRAP_BASE + RA_HDMI_TX_WRAP_DPKMEM_SRAMPWR;
    GA_REG_WORD32_WRITE(addr, HDMITX_SRAMPWR_OFF);

    addr = HDMITX_WRAP_BASE + RA_HDMI_TX_WRAP_revoclist_SRAMPWR;
    GA_REG_WORD32_WRITE(addr, HDMITX_SRAMPWR_OFF);
#endif
}

INT BE_HDMITX_PowerOff(BE_HDMITX_DATA  *pHdmiTx)
{
#ifdef EARC_RX_ENABLED
    T32HDMI_TX_WRAP_ARCRX_CTRL arcrx_ctrl;
#endif //EARC_RX_ENABLED

    /*SRAM powerdown*/
    BE_HDMITX_SRAM_PowerOff();

    /* PHY powerdown */
    BE_HDMITX_phy_standby(pHdmiTx);

#ifdef EARC_RX_ENABLED
    /*ARC powerdown*/
    GA_REG_WORD32_READ(HDMITX_SNPS_ARC_CTRL, &arcrx_ctrl.u32);
    arcrx_ctrl.uARCRX_CTRL_arcrx_en = false;
    arcrx_ctrl.uARCRX_CTRL_pwrdn = true;
    GA_REG_WORD32_WRITE(HDMITX_SNPS_ARC_CTRL, arcrx_ctrl.u32);
#endif //EARC_RX_ENABLED

    return MV_VPP_OK;
}
#endif // _VPP_BE_HDMITX_C_
