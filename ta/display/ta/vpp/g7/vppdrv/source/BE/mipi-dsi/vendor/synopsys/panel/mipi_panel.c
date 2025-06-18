/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "vpp_api.h"
#include "mipi_hal.h"
#include "mipi_panel.h"
#include "dsih_api.h"

DEFINE_PANEL_FUNCTION(BOE540x960);
DEFINE_PANEL_FUNCTION(LTK1920x1200);
DEFINE_PANEL_FUNCTION(BOE800x1280);
DEFINE_PANEL_FUNCTION(PND1920x1080);
DEFINE_PANEL_FUNCTION(HAIER800x1280);
DEFINE_PANEL_FUNCTION(CUSTOM);
DEFINE_PANEL_FUNCTION(UBQ800x1280);

Panel_Config_t g_config[DISPLAY_PANELID_MAX] = {
    PANEL_FUNCTION_REGISTER(BOE540x960),
    PANEL_FUNCTION_REGISTER(LTK1920x1200),
    PANEL_FUNCTION_REGISTER(BOE800x1280),
    PANEL_FUNCTION_REGISTER(PND1920x1080),
    PANEL_FUNCTION_REGISTER(HAIER800x1280),
    PANEL_FUNCTION_REGISTER(CUSTOM),
    PANEL_FUNCTION_REGISTER(UBQ800x1280),
};

int get_panel_video_timings(struct mipi_dsi_dev *dev, unsigned char PanelID)
{
    if((!dev) || (PanelID >= DISPLAY_PANELID_MAX))
        return MIPI_RET(ENODEV);

    if(g_config[PanelID].get_video_timings)
        return g_config[PanelID].get_video_timings(dev);
    else
        return MIPI_RET(ENODEV);
}

int get_panel_command_timings(struct mipi_dsi_dev *dev, unsigned char PanelID)
{
    if((!dev) || (PanelID >= DISPLAY_PANELID_MAX))
        return MIPI_RET(ENODEV);

    if(g_config[PanelID].get_command_timings)
        return g_config[PanelID].get_command_timings(dev);
    else
        return MIPI_RET(ENODEV);
}

int mipi_panel_init(struct mipi_dsi_dev *dev, unsigned char PanelID)
{
    if((!dev) || (PanelID >= DISPLAY_PANELID_MAX))
        return MIPI_RET(ENODEV);

    if(g_config[PanelID].panel_init)
        g_config[PanelID].panel_init(dev);
    else
        return MIPI_RET(ENODEV);

    return 0;
}

int mipi_panel_standby(struct mipi_dsi_dev *dev, unsigned char PanelID)
{
    return g_config[PanelID].panel_standby(dev);
}

void DSI_BYTE_WRITE(void *dev, unsigned char data1)
{
    uint8_t dsi_command_param[5] = { 0 };
    dsi_command_param[0] = data1;
    dsi_command_param[1] = 0;
    mipi_dsih_gen_wr_packet(dev, 0, 0x5, dsi_command_param, 1);
}

void DSI_SHORT_WRITE(void *dev, unsigned char data1, unsigned char data2)
{
    uint8_t dsi_command_param[5] = { 0 };
    dsi_command_param[0] = data1;
    dsi_command_param[1] = data2;
    mipi_dsih_gen_wr_packet(dev, 0, 0x15, dsi_command_param, 2);
}

void DSI_LONG_WRITE(void *dev, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4)
{
    uint8_t dsi_command_param[6] = { 0 };
    dsi_command_param[0] = 0x04;
    dsi_command_param[1] = 0x00;
    dsi_command_param[2] = data1;
    dsi_command_param[3] = data2;
    dsi_command_param[4] = data3;
    dsi_command_param[5] = data4;
    mipi_dsih_gen_wr_packet(dev, 0, 0x39, dsi_command_param, 6);
}
