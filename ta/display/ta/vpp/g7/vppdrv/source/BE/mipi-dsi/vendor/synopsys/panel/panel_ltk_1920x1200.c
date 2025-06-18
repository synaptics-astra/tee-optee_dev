// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "dsih_api.h"
#include "dsih_core.h"
#include "mipi_panel.h"
#include "mipi_hal.h"

int LTK1920x1200_get_video_timings(struct mipi_dsi_dev *dev)
{
    dsih_dpi_video_t *param;

    if (dev == NULL){
        return MIPI_RET(ENODEV);
    }
    param = &dev->dpi_video;

    /* 960 x 540 | BURST | PClk 60.667MHz | ByteClk 91MHz| 24bpp */
    param->display_type = 0;
    param->no_of_lanes = 4;
    param->virtual_channel = 0;
    param->video_mode = VIDEO_BURST_WITH_SYNC_PULSES;
    param->receive_ack_packets = 0; /* enable receiving of ack packets */
    param->is_18_loosely = 0;
    param->data_en_polarity = 1;

    param->h_polarity = 0;
    param->v_polarity = 0;

    param->h_sync_pixels = 25;   /* min 4 pixels */
    param->h_back_porch_pixels = 62;
    param->v_sync_lines = 4;
    param->v_back_porch_lines = 18;
    param->color_coding = COLOR_CODE_24BIT;
    param->h_active_pixels = 1920;
    param->v_active_lines = 1200;
    param->h_total_pixels = 2044;
    param->v_total_lines = 1224;
    param->eotp_rx_en = 0;
    param->eotp_tx_en = 1;

    param->non_continuous_clock = 1;
    param->no_of_chunks = 0;
    param->null_packet_size = 0xFFF;
    param->dpi_lp_cmd_en = 1;
    param->byte_clock = 111250; /* KHz  */
    param->pixel_clock = 150000;  /* dpi_clock */

    return 0;
}

int LTK1920x1200_get_command_timings(struct mipi_dsi_dev *dev)
{
    return MIPI_RET(EUNSUPPORT);
}

int LTK1920x1200_panel_init(struct mipi_dsi_dev *dev)
{
    uint8_t dsi_command_param[100] = { 0 };

    mipi_dsih_cmd_mode(dev, 1);

    dsi_command_param[0] = 0x11;
    dsi_command_param[1] = 0x0;
    mipi_dsih_gen_wr_packet(dev, 0, 0x5, dsi_command_param, 2);
    mdelay(120);

    dsi_command_param[0] = 0x29;
    dsi_command_param[1] = 0x0;
    mipi_dsih_gen_wr_packet(dev, 0, 0x5, dsi_command_param, 2);

    return 0;
}

int LTK1920x1200_panel_standby(struct mipi_dsi_dev *dev)
{
    return 0;
}
