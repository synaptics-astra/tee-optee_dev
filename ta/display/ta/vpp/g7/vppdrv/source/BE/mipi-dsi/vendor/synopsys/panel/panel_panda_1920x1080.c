// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "dsih_api.h"
#include "dsih_core.h"
#include "mipi_panel.h"
#include "mipi_hal.h"

int PND1920x1080_get_video_timings(struct mipi_dsi_dev *dev)
{
    dsih_dpi_video_t *param;

    if (dev == NULL){
        return MIPI_RET(ENODEV);
    }
    param = &dev->dpi_video;

    /* 1920 x 1080 | BURST | PClk 138.5MHz | ByteClk 125MHz| 24bpp */
    param->display_type = 0;
    param->no_of_lanes = 4;
    param->virtual_channel = 0;
    param->video_mode = VIDEO_BURST_WITH_SYNC_PULSES;
    param->receive_ack_packets = 0; /* enable receiving of ack packets */
    param->is_18_loosely = 0;
    param->data_en_polarity = 1;

    param->h_polarity = 1;
    param->v_polarity = 1;

    param->h_sync_pixels = 32;   /* min 4 pixels */
    param->h_back_porch_pixels = 80;
    param->v_sync_lines = 5;
    param->v_back_porch_lines = 23;
    param->color_coding = COLOR_CODE_24BIT;
    param->h_active_pixels = 1920;
    param->v_active_lines = 1080;
    param->h_total_pixels = 2080;
    param->v_total_lines = 1111;
    param->eotp_rx_en = 0;
    param->eotp_tx_en = 1;

    param->non_continuous_clock = 1;
    param->no_of_chunks = 0;
    param->null_packet_size = 0xFFF;
    param->dpi_lp_cmd_en = 1;
    param->byte_clock = 125000; /*KHz  */
    param->pixel_clock = 138500;  /* dpi_clock */

    return (MV_VPP_OK);
}

int PND1920x1080_get_command_timings(struct mipi_dsi_dev *dev)
{
    return MIPI_RET(EUNSUPPORT);
}

int PND1920x1080_panel_init(struct mipi_dsi_dev *dev)
{
    return (MV_VPP_OK);
}
int PND1920x1080_panel_standby(struct mipi_dsi_dev *dev)
{
    return (MV_VPP_OK);
}