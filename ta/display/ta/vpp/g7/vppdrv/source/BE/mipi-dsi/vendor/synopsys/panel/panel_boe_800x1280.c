// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "dsih_api.h"
#include "dsih_core.h"
#include "mipi_panel.h"
#include "mipi_hal.h"

#ifndef SSD_Single
#define SSD_Single(data1, data2) DSI_SHORT_WRITE(dev, data1, data2)
#define SSD_Number(data)
#define SSD_CMD(data1)  DSI_BYTE_WRITE(dev, data1)
#endif
/*TD4101 display driver*/
int BOE800x1280_get_video_timings(struct mipi_dsi_dev *dev)
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

    param->h_polarity = 1;
    param->v_polarity = 1;

    param->h_sync_pixels = 20;   /* min 4 pixels */
    param->h_back_porch_pixels = 23;
    param->v_sync_lines = 4;
    param->v_back_porch_lines = 22;
    param->color_coding = COLOR_CODE_24BIT;
    param->h_active_pixels = 800;
    param->v_active_lines = 1280;
    param->h_total_pixels = 882;
    param->v_total_lines = 1327;
    param->eotp_rx_en = 0;
    param->eotp_tx_en = 1;

    param->non_continuous_clock = 1;
    param->no_of_chunks = 0;
    param->null_packet_size = 0xFFF;
    param->dpi_lp_cmd_en = 1;
    param->byte_clock = 62500; /* KHz  */
    param->pixel_clock = 70220;  /* dpi_clock */

    return 0;
}

int BOE800x1280_get_command_timings(struct mipi_dsi_dev *dev)
{
    return MIPI_RET(EUNSUPPORT);
}

int BOE800x1280_panel_init(struct mipi_dsi_dev *dev)
{
    mipi_dbg_print(MIPI_INFO, "BOE800x1280_panel_init...\r\n");
    mipi_dsih_cmd_mode(dev, 1);

    //Page0
    SSD_Single(0xE0,0x00);
    //--- PASSWORD  ----//
    SSD_Single(0xE1,0x93);
    SSD_Single(0xE2,0x65);
    SSD_Single(0xE3,0xF8);
    SSD_Single(0x80,0x03);


    //--- Page1  ----//
    SSD_Single(0xE0,0x01);
    //Set VCOM
    SSD_Single(0x00,0x00);
    SSD_Single(0x01,0x6F);
    //Set VCOM_Reverse
    //SSD_Single(0x03,0x00);
    //SSD_Single(0x04,0xA0);
    //Set Gamma Power, VGMP,VGMN,VGSP,VGSN
    SSD_Single(0x17,0x00);
    SSD_Single(0x18,0xAF);//4.3V
    SSD_Single(0x19,0x01);//0.3V
    SSD_Single(0x1A,0x00);//
    SSD_Single(0x1B,0xAF);//4.3V
    SSD_Single(0x1C,0x01);//0.3V

    //Set Gate Power
    SSD_Single(0x1F,0x3E);     //VGH_R  = 15V
    SSD_Single(0x20,0x28);     //VGL_R  = -12V
    SSD_Single(0x21,0x28);     //VGL_R2 = -12V
    SSD_Single(0x22,0x7E);     //PA[6]=1, PA[5]=1, PA[4]=1, PA[0]=0
    //SETPANEL
    SSD_Single(0x35,0x26);  //SAP=0110

    //SETPANEL
    SSD_Single(0x37,0x09);  //SS=1,BGR=1
    //SET RGBCYC
    SSD_Single(0x38,0x04);  //JDT=100 column inversion
    SSD_Single(0x39,0x00);  //RGB_N_EQ1, 0x12
    SSD_Single(0x3A,0x01);  //RGB_N_EQ2, 0x18
    SSD_Single(0x3C,0x7C);  //SET EQ3 for TE_H
    SSD_Single(0x3D,0xFF);  //SET CHGEN_ON,
    SSD_Single(0x3E,0xFF);  //SET CHGEN_OFF,
    SSD_Single(0x3F,0x7F);  //SET CHGEN_OFF2,

    //Set TCON
    SSD_Single(0x40,0x06);  //RSO=800 RGB
    SSD_Single(0x41,0xA0);  //LN=640->1280 line
    SSD_Single(0x42,0x81);  //SLT
    SSD_Single(0x43,0x08);  //VFP=8
    SSD_Single(0x44,0x0B);  //VBP=12
    SSD_Single(0x45,0x28);  //HBP=40
    //--- power voltage  ----//
    SSD_Single(0x55,0x0F);  //DCDCM=0001, JD PWR_IC
    SSD_Single(0x57,0x69);
    SSD_Single(0x59,0x0A);  //VCL = -2.9V
    SSD_Single(0x5A,0x28);  //VGH = 15V
    SSD_Single(0x5B,0x14);  //VGL = -11V

    //--- Gamma  ----//
    SSD_Single(0x5D,0x7C);
    SSD_Single(0x5E,0x65);
    SSD_Single(0x5F,0x55);
    SSD_Single(0x60,0x47);
    SSD_Single(0x61,0x43);
    SSD_Single(0x62,0x32);
    SSD_Single(0x63,0x34);
    SSD_Single(0x64,0x1C);
    SSD_Single(0x65,0x33);
    SSD_Single(0x66,0x31);
    SSD_Single(0x67,0x30);
    SSD_Single(0x68,0x4E);
    SSD_Single(0x69,0x3C);
    SSD_Single(0x6A,0x44);
    SSD_Single(0x6B,0x35);
    SSD_Single(0x6C,0x31);
    SSD_Single(0x6D,0x23);
    SSD_Single(0x6E,0x11);
    SSD_Single(0x6F,0x00);
    SSD_Single(0x70,0x7C);
    SSD_Single(0x71,0x65);
    SSD_Single(0x72,0x55);
    SSD_Single(0x73,0x47);
    SSD_Single(0x74,0x43);
    SSD_Single(0x75,0x32);
    SSD_Single(0x76,0x34);
    SSD_Single(0x77,0x1C);
    SSD_Single(0x78,0x33);
    SSD_Single(0x79,0x31);
    SSD_Single(0x7A,0x30);
    SSD_Single(0x7B,0x4E);
    SSD_Single(0x7C,0x3C);
    SSD_Single(0x7D,0x44);
    SSD_Single(0x7E,0x35);
    SSD_Single(0x7F,0x31);
    SSD_Single(0x80,0x23);
    SSD_Single(0x81,0x11);
    SSD_Single(0x82,0x00);


    //Page2, for GIP
    SSD_Single(0xE0,0x02);
    //GIP_L Pin mapping
    SSD_Single(0x00,0x1E);//1  VDS
    SSD_Single(0x01,0x1E);//2  VDS
    SSD_Single(0x02,0x41);//3  STV2
    SSD_Single(0x03,0x41);//4  STV2
    SSD_Single(0x04,0x43);//5  STV4
    SSD_Single(0x05,0x43);//6  STV4
    SSD_Single(0x06,0x1F);//7  VSD
    SSD_Single(0x07,0x1F);//8  VSD
    SSD_Single(0x08,0x35);//9  GCL
    SSD_Single(0x09,0x1F);//10
    SSD_Single(0x0A,0x15);//11 GCH
    SSD_Single(0x0B,0x15);//12 GCH
    SSD_Single(0x0C,0x1F);//13
    SSD_Single(0x0D,0x47);//14 CLK8
    SSD_Single(0x0E,0x47);//15 CLK8
    SSD_Single(0x0F,0x45);//16 CLK6
    SSD_Single(0x10,0x45);//17 CLK6
    SSD_Single(0x11,0x4B);//18 CLK4
    SSD_Single(0x12,0x4B);//19 CLK4
    SSD_Single(0x13,0x49);//20 CLK2
    SSD_Single(0x14,0x49);//21 CLK2
    SSD_Single(0x15,0x1F);//22 VGL


    //GIP_R Pin mapping
    SSD_Single(0x16,0x1E);//1  VDS
    SSD_Single(0x17,0x1E);//2  VDS
    SSD_Single(0x18,0x40);//3  STV1
    SSD_Single(0x19,0x40);//4  STV1
    SSD_Single(0x1A,0x42);//5  STV3
    SSD_Single(0x1B,0x42);//6  STV3
    SSD_Single(0x1C,0x1F);//7  VSD
    SSD_Single(0x1D,0x1F);//8  VSD
    SSD_Single(0x1E,0x35);//9  GCL
    SSD_Single(0x1F,0x1F);//10
    SSD_Single(0x20,0x15);//11 GCH
    SSD_Single(0x21,0x15);//12 GCH
    SSD_Single(0x22,0x1F);//13
    SSD_Single(0x23,0x46);//14 CLK7
    SSD_Single(0x24,0x46);//15 CLK7
    SSD_Single(0x25,0x44);//16 CLK5
    SSD_Single(0x26,0x44);//17 CLK5
    SSD_Single(0x27,0x4A);//18 CLK3
    SSD_Single(0x28,0x4A);//19 CLK3
    SSD_Single(0x29,0x48);//20 CLK1
    SSD_Single(0x2A,0x48);//21 CLK1
    SSD_Single(0x2B,0x1F);//22 VGL

    //GIP_L_GS Pin mapping
    SSD_Single(0x2C,0x1F);//1  VDS
    SSD_Single(0x2D,0x1F);//2  VDS
    SSD_Single(0x2E,0x00);//3  STV2
    SSD_Single(0x2F,0x00);//4  STV2
    SSD_Single(0x30,0x00);//5  STV4
    SSD_Single(0x31,0x00);//6  STV4
    SSD_Single(0x32,0x1E);//7  VSD
    SSD_Single(0x33,0x1E);//8  VSD
    SSD_Single(0x34,0x35);//9  GCL
    SSD_Single(0x35,0x1F);//10
    SSD_Single(0x36,0x15);//11 GCH
    SSD_Single(0x37,0x15);//12 GCH
    SSD_Single(0x38,0x1F);//13
    SSD_Single(0x39,0x08);//14 CLK8
    SSD_Single(0x3A,0x08);//15 CLK8
    SSD_Single(0x3B,0x0A);//16 CLK6
    SSD_Single(0x3C,0x0A);//17 CLK6
    SSD_Single(0x3D,0x04);//18 CLK4
    SSD_Single(0x3E,0x04);//19 CLK4
    SSD_Single(0x3F,0x06);//20 CLK2
    SSD_Single(0x40,0x06);//21 CLK2
    SSD_Single(0x41,0x1F);//22 VGL

    //GIP_R_GS Pin mapping
    SSD_Single(0x42,0x1F);//1  VDS
    SSD_Single(0x43,0x1F);//2  VDS
    SSD_Single(0x44,0x01);//3  STV1
    SSD_Single(0x45,0x01);//4  STV1
    SSD_Single(0x46,0x01);//5  STV3
    SSD_Single(0x47,0x01);//6  STV3
    SSD_Single(0x48,0x1E);//7  VSD
    SSD_Single(0x49,0x1E);//8  VSD
    SSD_Single(0x4A,0x35);//9  GCL
    SSD_Single(0x4B,0x1F);//10
    SSD_Single(0x4C,0x15);//11 GCH
    SSD_Single(0x4D,0x15);//12 GCH
    SSD_Single(0x4E,0x1F);//13
    SSD_Single(0x4F,0x09);//14 CLK7
    SSD_Single(0x50,0x09);//15 CLK7
    SSD_Single(0x51,0x0B);//16 CLK5
    SSD_Single(0x52,0x0B);//17 CLK5
    SSD_Single(0x53,0x05);//18 CLK3
    SSD_Single(0x54,0x05);//19 CLK3
    SSD_Single(0x55,0x07);//20 CLK1
    SSD_Single(0x56,0x07);//21 CLK1
    SSD_Single(0x57,0x1F);//22 VGL

    //GIP Timing
    SSD_Single(0x58,0x40);
    SSD_Single(0x5B,0x30); //STV_NUM,STV_S0
    SSD_Single(0x5C,0x0F); //STV_S0
    SSD_Single(0x5D,0x30); //STV_W / S1
    SSD_Single(0x5E,0x01); //STV_S2
    SSD_Single(0x5F,0x02); //STV_S3
    SSD_Single(0x63,0x14); //SETV_ON
    SSD_Single(0x64,0x6A); //SETV_OFF
    SSD_Single(0x67,0x73);
    SSD_Single(0x68,0x11);//CKV_S0
    SSD_Single(0x69,0x14);
    SSD_Single(0x6A,0x6A);
    SSD_Single(0x6B,0x08); //Dummy clk

    SSD_Single(0x6C,0x00);
    SSD_Single(0x6D,0x00);
    SSD_Single(0x6E,0x00);
    SSD_Single(0x6F,0x88);

    SSD_Single(0x77,0xDD);//VEN_EN=1
    SSD_Single(0x79,0x0E);//0x0C
    SSD_Single(0x7A,0x0F);//VEN_S0
    SSD_Single(0x7D,0x14);//VEN_ON
    SSD_Single(0x7E,0x82); //VEN_OFF


    //Page4
    SSD_Single(0xE0,0x04);
    SSD_Single(0x09,0x11);
    SSD_Single(0x0E,0x48);
    SSD_Single(0x2B,0x2B);
    SSD_Single(0x2D,0x03);//defult 0x01
    SSD_Single(0x2E,0x44);

    //Page0
    SSD_Single(0xE0,0x00);

    SSD_Single(0xE6,0x02);
    SSD_Single(0xE7,0x0C);


    //SLP OUT
    SSD_Number(0x01);
    SSD_CMD(0x11);      // SLPOUT
    mdelay(120);


    //DISP ON
    SSD_Number(0x01);
    SSD_CMD(0x29);      // DSPON
    mdelay(5);

    //--- TE----//
    SSD_Single(0x35,0x00);
    return 0;
}

int BOE800x1280_panel_standby(struct mipi_dsi_dev *dev)
{
    return 0;
}
