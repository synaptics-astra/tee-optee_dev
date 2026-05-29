/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "dsih_dphy.h"
#include "vpp_api.h"
#include "vpp_cfg.h"
#include "vpp_module.h"
#include "dsih_core.h"
#include "dsih_api.h"

#include "dsih_hal.h"

#define VPP_MIPI_INIT_PANEL_PARAM(VAR) pdev->dpi_video.VAR = pConfigParams->initparams.VAR
#define VPP_MIPI_INFO_PANEL_PARAM(VAR1, VAR2) pdev->dpi_video.VAR1 = pConfigParams->infoparams.resInfo.VAR2
#define VPP_MIPI_CMD_PANEL_PARAM(VAR) pdev->dpi_video.DsiPanelcmd.VAR = pConfigParams->vppMipiCmd.VAR

void VPP_BE_MIPI_DsiInit(VPP_OBJ *vpp_obj)
{
    struct mipi_dsi_dev *pdev = (struct mipi_dsi_dev *)(vpp_obj->dsiInfo.pmipi_dsi_dev);

    mipi_dsi_init(pdev);
}

int VPP_BE_MIPIOBJ_Create(VPP_OBJ *vpp_obj)
{
    int size;

    size = sizeof(struct mipi_dsi_dev);
    if (!vpp_obj->dsiInfo.pmipi_dsi_dev) {
        VPP_SHM32_MALLOC(VPP_SHM_SECURE,size,(VPP_SHM_HANDLE *)&vpp_obj->dsiInfo.pmipi_dsi_dev);
        if (!vpp_obj->dsiInfo.pmipi_dsi_dev) {
            return (MV_VPP_ENOMEM);
        }
    }

    return 0;
}

int VPP_BE_MIPIOBJ_Config(VPP_OBJ *vpp_obj)
{
    struct mipi_dsi_dev *dev;

    dev = (struct mipi_dsi_dev *)vpp_obj->dsiInfo.pmipi_dsi_dev;

    // Zero the device
    VPP_Memset(dev, 0, sizeof(struct mipi_dsi_dev));
    dev->pHandle = (void *)vpp_obj; //Store handle for BCM

    VPP_BE_MIPI_DsiInit(vpp_obj);

    return 0;
}

int VPP_BE_MIPI_PanelInit(VPP_OBJ *vpp_obj, int PanelID)
{
    struct mipi_dsi_dev *pdev = (struct mipi_dsi_dev *)(vpp_obj->dsiInfo.pmipi_dsi_dev);
    int ret = MV_VPP_OK;

    dsi_platform_init(pdev, PanelID, VIDEO_MODE, 0);
#ifdef START_VPG
    start_video_pattern(pdev, 1, 0);
#endif
    return ret;
}

int VPP_BE_MIPI_PanelStandby(VPP_OBJ *vpp_obj, int PanelID)
{
    struct mipi_dsi_dev *pdev = (struct mipi_dsi_dev *)(vpp_obj->dsiInfo.pmipi_dsi_dev);
    int ret;

    ret = mipi_panel_standby(pdev, PanelID);

    return ret;
}

int VPP_BE_MIPI_PanelDeInit(VPP_OBJ *vpp_obj)
{
    return mipi_dsih_close(vpp_obj->dsiInfo.pmipi_dsi_dev);
}

void VPP_BE_MIPI_Config(VPP_OBJ *vpp_obj,
        VPP_MIPI_CONFIG_PARAMS *pConfigParams)
{
    struct mipi_dsi_dev *pdev = (struct mipi_dsi_dev *)(vpp_obj->dsiInfo.pmipi_dsi_dev);

    /* Copy the params to the vpp_obj->mipi_dev */
    pdev->dpi_video.display_type = 0;

    VPP_MIPI_INIT_PANEL_PARAM(no_of_lanes);
    VPP_MIPI_INIT_PANEL_PARAM(virtual_channel);
    VPP_MIPI_INIT_PANEL_PARAM(video_mode);
    VPP_MIPI_INIT_PANEL_PARAM(receive_ack_packets);
    VPP_MIPI_INIT_PANEL_PARAM(is_18_loosely);
    VPP_MIPI_INIT_PANEL_PARAM(data_en_polarity);
    VPP_MIPI_INIT_PANEL_PARAM(h_polarity);
    VPP_MIPI_INIT_PANEL_PARAM(v_polarity);
    VPP_MIPI_INIT_PANEL_PARAM(eotp_rx_en);
    VPP_MIPI_INIT_PANEL_PARAM(eotp_tx_en);
    VPP_MIPI_INIT_PANEL_PARAM(color_coding);
    VPP_MIPI_INIT_PANEL_PARAM(non_continuous_clock);
    VPP_MIPI_INIT_PANEL_PARAM(no_of_chunks);
    VPP_MIPI_INIT_PANEL_PARAM(null_packet_size);
    VPP_MIPI_INIT_PANEL_PARAM(dpi_lp_cmd_en);
    VPP_MIPI_INIT_PANEL_PARAM(byte_clock);

    VPP_MIPI_CMD_PANEL_PARAM(bufsize);

    VPP_MIPI_INFO_PANEL_PARAM(h_sync_pixels, hsyncwidth);
    VPP_MIPI_INFO_PANEL_PARAM(h_back_porch_pixels, hbackporch);
    VPP_MIPI_INFO_PANEL_PARAM(v_sync_lines, vsyncwidth);
    VPP_MIPI_INFO_PANEL_PARAM(v_back_porch_lines, vbackporch);
    VPP_MIPI_INFO_PANEL_PARAM(pixel_clock, freq);
    VPP_MIPI_INFO_PANEL_PARAM(h_active_pixels, active_width);
    VPP_MIPI_INFO_PANEL_PARAM(v_active_lines, active_height);

    pdev->dpi_video.DsiPanelcmd.pcmd = (UINT8*)pConfigParams->vppMipiCmd.pcmd;
    pdev->dpi_video.v_total_lines = pConfigParams->infoparams.resInfo.active_height +
        pConfigParams->infoparams.resInfo.vfrontporch +
        pConfigParams->infoparams.resInfo.vsyncwidth +
        pConfigParams->infoparams.resInfo.vbackporch;

    pdev->dpi_video.h_total_pixels = pConfigParams->infoparams.resInfo.width;
}

/********************************************************************************
 * FUNCTION: Configure and initialise the panel
 * INPUT: handle - VPP object handle
 *        resID - resID for which Panel has to be configured
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_ConfigMIPIResinfo(int handle, int resID)
{
    VPP_OBJ *vpp_obj;
    CPCB_TG_TIMING_PARAMS *ptgCalibrationParams;
    RESOLUTION_INFO  *pResinfo;
    VPP_MIPI_CONFIG_PARAMS  *pResConfig, *pConfigParams;
    int i;

    if (g_vpp->magic != VPP_MAGIC) {
        return (MV_VPP_EUNCONFIG);
    }

    if (handle<0 || handle>=MAX_NUM_OBJECTS)
        return (MV_VPP_EBADPARAM);

    vpp_obj = g_vpp->vpp_objs[handle];
    if (!vpp_obj)
        return (MV_VPP_ENODEV);

    pResConfig = (VPP_MIPI_CONFIG_PARAMS*) vpp_obj->dsiInfo.MipiCfg.vppMipiCfgPA;

    for ( i=0; i < vpp_obj->dsiInfo.MipiCfg.noOfresID; i++ ) {
        if (resID ==  pResConfig[i].initparams.resId)
        {
            break;
        }
    }

    if ( i == vpp_obj->dsiInfo.MipiCfg.noOfresID) {
        return (MV_VPP_ERANGEPARAM);
    }

    pConfigParams = &pResConfig[i];

    pResinfo = VPP_GetResInfo(RES_DSI_CUSTOM);
    VPP_Memcpy(pResinfo, &pConfigParams->infoparams.resInfo, sizeof(RESOLUTION_INFO));

    pResinfo->width = pConfigParams->infoparams.resInfo.active_width +
                                                pConfigParams->infoparams.resInfo.hfrontporch
                                                + pConfigParams->infoparams.resInfo.hsyncwidth
                                                + pConfigParams->infoparams.resInfo.hbackporch;
    pResinfo->height = pConfigParams->infoparams.resInfo.active_height
                                                + pConfigParams->infoparams.resInfo.vfrontporch
                                                + pConfigParams->infoparams.resInfo.vsyncwidth
                                                + pConfigParams->infoparams.resInfo.vbackporch;

    ptgCalibrationParams = &vpp_obj->dsiInfo.dsiTgCalibrationParams;
    VPP_Memcpy(ptgCalibrationParams, &pConfigParams->infoparams.tgParams, sizeof(CPCB_TG_TIMING_PARAMS));

    VPP_BE_MIPI_Config(vpp_obj,pConfigParams);

    return MV_VPP_OK;
}

int VPP_BE_MIPI_PanelSendCmd(VPP_OBJ *vpp_obj, VPP_MIPI_CMD_TYPE enCmdType)
{
    struct mipi_dsi_dev *pdev = (struct mipi_dsi_dev *)(vpp_obj->dsiInfo.pmipi_dsi_dev);

    return Panel_SendCmd(pdev, enCmdType);
}

int VPP_BE_MIPI_PanelSendCmdBuf(VPP_OBJ *vpp_obj, UINT8 *pcmd, UINT16 bufsize)
{
    struct mipi_dsi_dev *pdev = (struct mipi_dsi_dev *)(vpp_obj->dsiInfo.pmipi_dsi_dev);

    return Panel_SendCmdBuf(pdev, pcmd, bufsize);
}