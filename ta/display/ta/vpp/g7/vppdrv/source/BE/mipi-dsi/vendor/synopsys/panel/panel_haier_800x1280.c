// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "dsih_api.h"
#include "dsih_core.h"
#include "mipi_panel.h"
#include "mipi_hal.h"

#ifndef Generic_Short_Write_1P
#define Generic_Long_Write_3P(data1, data2, data3, data4) DSI_LONG_WRITE(dev, data1, data2, data3, data4)
#define Generic_Short_Write_1P(data1, data2) DSI_SHORT_WRITE(dev, data1, data2)
#define DCS_Short_Write_NP(data1)  DSI_BYTE_WRITE(dev, data1)
#endif

int HAIER800x1280_get_video_timings(struct mipi_dsi_dev *dev)
{
    dsih_dpi_video_t *param;

    if (dev == NULL){
        return MIPI_RET(ENODEV);
    }
    param = &dev->dpi_video;

    /* 800 x 1280 | BURST | PClk 74.784MHz | ByteClk 62MHz| 24bpp */
    param->display_type = 0;
    param->no_of_lanes = 4;
    param->virtual_channel = 0;
    param->video_mode = VIDEO_BURST_WITH_SYNC_PULSES;
    param->receive_ack_packets = 0; /* enable receiving of ack packets */
    param->is_18_loosely = 0;
    param->data_en_polarity = 1;

    param->h_polarity = 1;
    param->v_polarity = 1;

    param->h_sync_pixels = 30;   /* min 4 pixels */
    param->h_back_porch_pixels = 60;
    param->v_sync_lines = 2;
    param->v_back_porch_lines = 14;
    param->color_coding = COLOR_CODE_24BIT;
    param->h_active_pixels = 800;
    param->v_active_lines = 1280;
    param->h_total_pixels = 794;/*62500/(1312*60)*/
    param->v_total_lines = 1312;
    param->eotp_rx_en = 0;
    param->eotp_tx_en = 1;

    param->non_continuous_clock = 1;
    param->no_of_chunks = 0;
    param->null_packet_size = 0xFFF;
    param->dpi_lp_cmd_en = 1;
    param->byte_clock = 62500; /* KHz  */
    param->pixel_clock = 74784;  /* dpi_clock */

    return 0;
}

int HAIER800x1280_get_command_timings(struct mipi_dsi_dev *dev)
{
    return MIPI_RET(EUNSUPPORT);
}

int HAIER800x1280_panel_init(struct mipi_dsi_dev *dev)
{
    mipi_dbg_print(MIPI_INFO, "HAIER800x1280_panel_init...\r\n");
    mipi_dsih_cmd_mode(dev, 1);

    Generic_Long_Write_3P(0xFF,0x98,0x81,0x03);
    Generic_Short_Write_1P(0x01,0x00);
    Generic_Short_Write_1P(0x02,0x00);
    Generic_Short_Write_1P(0x03,0x73);
    Generic_Short_Write_1P(0x04,0xD7);
    Generic_Short_Write_1P(0x05,0x00);
    Generic_Short_Write_1P(0x06,0x08);
    Generic_Short_Write_1P(0x07,0x11);
    Generic_Short_Write_1P(0x08,0x00);
    Generic_Short_Write_1P(0x09,0x3F);
    Generic_Short_Write_1P(0x0a,0x00);
    Generic_Short_Write_1P(0x0b,0x00);
    Generic_Short_Write_1P(0x0c,0x00);
    Generic_Short_Write_1P(0x0d,0x00);
    Generic_Short_Write_1P(0x0e,0x00);
    Generic_Short_Write_1P(0x0f,0x3F);
    Generic_Short_Write_1P(0x10,0x3F);
    Generic_Short_Write_1P(0x11,0x00);
    Generic_Short_Write_1P(0x12,0x00);
    Generic_Short_Write_1P(0x13,0x00);
    Generic_Short_Write_1P(0x14,0x00);
    Generic_Short_Write_1P(0x15,0x00);
    Generic_Short_Write_1P(0x16,0x00);
    Generic_Short_Write_1P(0x17,0x00);
    Generic_Short_Write_1P(0x18,0x00);
    Generic_Short_Write_1P(0x19,0x00);
    Generic_Short_Write_1P(0x1a,0x00);
    Generic_Short_Write_1P(0x1b,0x00);
    Generic_Short_Write_1P(0x1c,0x00);
    Generic_Short_Write_1P(0x1d,0x00);
    Generic_Short_Write_1P(0x1e,0x40);
    Generic_Short_Write_1P(0x1f,0x80);
    Generic_Short_Write_1P(0x20,0x06);
    Generic_Short_Write_1P(0x21,0x01);
    Generic_Short_Write_1P(0x22,0x00);
    Generic_Short_Write_1P(0x23,0x00);
    Generic_Short_Write_1P(0x24,0x00);
    Generic_Short_Write_1P(0x25,0x00);
    Generic_Short_Write_1P(0x26,0x00);
    Generic_Short_Write_1P(0x27,0x00);
    Generic_Short_Write_1P(0x28,0x33);
    Generic_Short_Write_1P(0x29,0x33);
    Generic_Short_Write_1P(0x2a,0x00);
    Generic_Short_Write_1P(0x2b,0x00);
    Generic_Short_Write_1P(0x2c,0x00);
    Generic_Short_Write_1P(0x2d,0x00);
    Generic_Short_Write_1P(0x2e,0x00);
    Generic_Short_Write_1P(0x2f,0x00);
    Generic_Short_Write_1P(0x30,0x00);
    Generic_Short_Write_1P(0x31,0x00);
    Generic_Short_Write_1P(0x32,0x00);
    Generic_Short_Write_1P(0x33,0x00);
    Generic_Short_Write_1P(0x34,0x00);
    Generic_Short_Write_1P(0x35,0x00);
    Generic_Short_Write_1P(0x36,0x00);
    Generic_Short_Write_1P(0x37,0x00);
    Generic_Short_Write_1P(0x38,0x00);
    Generic_Short_Write_1P(0x39,0x00);
    Generic_Short_Write_1P(0x3a,0x00);
    Generic_Short_Write_1P(0x3b,0x00);
    Generic_Short_Write_1P(0x3c,0x00);
    Generic_Short_Write_1P(0x3d,0x00);
    Generic_Short_Write_1P(0x3e,0x00);
    Generic_Short_Write_1P(0x3f,0x00);
    Generic_Short_Write_1P(0x40,0x00);
    Generic_Short_Write_1P(0x41,0x00);
    Generic_Short_Write_1P(0x42,0x00);
    Generic_Short_Write_1P(0x43,0x00);
    Generic_Short_Write_1P(0x44,0x00);
    Generic_Short_Write_1P(0x50,0x01);
    Generic_Short_Write_1P(0x51,0x23);
    Generic_Short_Write_1P(0x52,0x44);
    Generic_Short_Write_1P(0x53,0x67);
    Generic_Short_Write_1P(0x54,0x89);
    Generic_Short_Write_1P(0x55,0xab);
    Generic_Short_Write_1P(0x56,0x01);
    Generic_Short_Write_1P(0x57,0x23);
    Generic_Short_Write_1P(0x58,0x45);
    Generic_Short_Write_1P(0x59,0x67);
    Generic_Short_Write_1P(0x5a,0x89);
    Generic_Short_Write_1P(0x5b,0xab);
    Generic_Short_Write_1P(0x5c,0xcd);
    Generic_Short_Write_1P(0x5d,0xef);
    Generic_Short_Write_1P(0x5e,0x00);
    Generic_Short_Write_1P(0x5f,0x0C);
    Generic_Short_Write_1P(0x60,0x0C);
    Generic_Short_Write_1P(0x61,0x0F);
    Generic_Short_Write_1P(0x62,0x0F);
    Generic_Short_Write_1P(0x63,0x0E);
    Generic_Short_Write_1P(0x64,0x0E);
    Generic_Short_Write_1P(0x65,0x06);
    Generic_Short_Write_1P(0x66,0x07);
    Generic_Short_Write_1P(0x67,0x0D);
    Generic_Short_Write_1P(0x68,0x02);
    Generic_Short_Write_1P(0x69,0x02);
    Generic_Short_Write_1P(0x6a,0x02);
    Generic_Short_Write_1P(0x6b,0x02);
    Generic_Short_Write_1P(0x6c,0x02);
    Generic_Short_Write_1P(0x6d,0x02);
    Generic_Short_Write_1P(0x6e,0x0D);
    Generic_Short_Write_1P(0x6f,0x02);
    Generic_Short_Write_1P(0x70,0x02);
    Generic_Short_Write_1P(0x71,0x05);
    Generic_Short_Write_1P(0x72,0x01);
    Generic_Short_Write_1P(0x73,0x08);
    Generic_Short_Write_1P(0x74,0x00);
    Generic_Short_Write_1P(0x75,0x0C);
    Generic_Short_Write_1P(0x76,0x0C);
    Generic_Short_Write_1P(0x77,0x0F);
    Generic_Short_Write_1P(0x78,0x0F);
    Generic_Short_Write_1P(0x79,0x0E);
    Generic_Short_Write_1P(0x7a,0x0E);
    Generic_Short_Write_1P(0x7b,0x06);
    Generic_Short_Write_1P(0x7c,0x07);
    Generic_Short_Write_1P(0x7d,0x0D);
    Generic_Short_Write_1P(0x7e,0x02);
    Generic_Short_Write_1P(0x7f,0x02);
    Generic_Short_Write_1P(0x80,0x02);
    Generic_Short_Write_1P(0x81,0x02);
    Generic_Short_Write_1P(0x82,0x02);
    Generic_Short_Write_1P(0x83,0x02);
    Generic_Short_Write_1P(0x84,0x0D);
    Generic_Short_Write_1P(0x85,0x02);
    Generic_Short_Write_1P(0x86,0x02);
    Generic_Short_Write_1P(0x87,0x05);
    Generic_Short_Write_1P(0x88,0x01);
    Generic_Short_Write_1P(0x89,0x08);
    Generic_Short_Write_1P(0x8A,0x00);
    Generic_Long_Write_3P(0xFF,0x98,0x81,0x04);
    Generic_Short_Write_1P(0x6E,0x3B);
    Generic_Short_Write_1P(0x6F,0x57);
    Generic_Short_Write_1P(0x3A,0xA4);
    Generic_Short_Write_1P(0x35,0x17);
    Generic_Short_Write_1P(0x8D,0x1F);
    Generic_Short_Write_1P(0x87,0xBA);
    Generic_Short_Write_1P(0xB2,0xD1);
    Generic_Short_Write_1P(0x88,0x0B);
    Generic_Short_Write_1P(0x38,0x01);
    Generic_Short_Write_1P(0x39,0x00);
    Generic_Short_Write_1P(0xB5,0x07);
    Generic_Short_Write_1P(0x31,0x75);
    Generic_Long_Write_3P(0xFF,0x98,0x81,0x01);
    Generic_Short_Write_1P(0x22,0x0A);
    Generic_Short_Write_1P(0x31,0x09);
    //Generic_Short_Write_1P(0x53,0x62);
    //Generic_Short_Write_1P(0x55,0x40);
    Generic_Short_Write_1P(0x50,0x86);
    Generic_Short_Write_1P(0x51,0x82);
    Generic_Short_Write_1P(0x60,0x27);
    Generic_Short_Write_1P(0x62,0x20);
    Generic_Short_Write_1P(0xA0,0x00);
    Generic_Short_Write_1P(0xA1,0x12);
    Generic_Short_Write_1P(0xA2,0x21);
    Generic_Short_Write_1P(0xA3,0x12);
    Generic_Short_Write_1P(0xA4,0x15);
    Generic_Short_Write_1P(0xA5,0x27);
    Generic_Short_Write_1P(0xA6,0x1C);
    Generic_Short_Write_1P(0xA7,0x1E);
    Generic_Short_Write_1P(0xA8,0x7D);
    Generic_Short_Write_1P(0xA9,0x1C);
    Generic_Short_Write_1P(0xAA,0x2B);
    Generic_Short_Write_1P(0xAB,0x6C);
    Generic_Short_Write_1P(0xAC,0x1B);
    Generic_Short_Write_1P(0xAD,0x19);
    Generic_Short_Write_1P(0xAE,0x4D);
    Generic_Short_Write_1P(0xAF,0x23);
    Generic_Short_Write_1P(0xB0,0x29);
    Generic_Short_Write_1P(0xB1,0x4D);
    Generic_Short_Write_1P(0xB2,0x5A);
    Generic_Short_Write_1P(0xB3,0x23);
    Generic_Short_Write_1P(0xC0,0x00);
    Generic_Short_Write_1P(0xC1,0x14);
    Generic_Short_Write_1P(0xC2,0x21);
    Generic_Short_Write_1P(0xC3,0x11);
    Generic_Short_Write_1P(0xC4,0x16);
    Generic_Short_Write_1P(0xC5,0x28);
    Generic_Short_Write_1P(0xC6,0x1C);
    Generic_Short_Write_1P(0xC7,0x1D);
    Generic_Short_Write_1P(0xC8,0x7B);
    Generic_Short_Write_1P(0xC9,0x1D);
    Generic_Short_Write_1P(0xCA,0x29);
    Generic_Short_Write_1P(0xCB,0x6B);
    Generic_Short_Write_1P(0xCC,0x19);
    Generic_Short_Write_1P(0xCD,0x18);
    Generic_Short_Write_1P(0xCE,0x4B);
    Generic_Short_Write_1P(0xCF,0x21);
    Generic_Short_Write_1P(0xD0,0x29);
    Generic_Short_Write_1P(0xD1,0x4D);
    Generic_Short_Write_1P(0xD2,0x5B);
    Generic_Short_Write_1P(0xD3,0x23);
    Generic_Long_Write_3P(0xFF,0x98,0x81,0x00);
    Generic_Short_Write_1P(0x35,0x00);

    DCS_Short_Write_NP(0x11);
    mdelay(120);
    DCS_Short_Write_NP(0x29);
    mdelay(10);

    return 0;
}

int HAIER800x1280_panel_standby(struct mipi_dsi_dev *dev)
{
    return 0;
}