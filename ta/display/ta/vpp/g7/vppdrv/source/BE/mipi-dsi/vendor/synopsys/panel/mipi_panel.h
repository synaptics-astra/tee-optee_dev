/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _MIPI_PANEL_H_
#define _MIPI_PANEL_H_
#include "dsih_core.h"
#include "includes.h"
#include "mipi_panel.h"

#define  DEFINE_FUNCTION(type, name) \
    extern int type ##_## name(struct mipi_dsi_dev *dev)

#define  DEFINE_PANEL_FUNCTION(type) \
    DEFINE_FUNCTION(type, get_video_timings);\
    DEFINE_FUNCTION(type, get_command_timings);\
    DEFINE_FUNCTION(type, panel_init);\
    DEFINE_FUNCTION(type, panel_standby);

#define  PANEL_FUNCTION_REGISTER(type)\
{type ## _ ## get_video_timings, type ## _ ## get_command_timings, type ## _ ## panel_init, type ## _ ## panel_standby}

typedef int (*panel_funcptr) (struct mipi_dsi_dev *);

typedef struct Panel_Config_T_
{
    panel_funcptr get_video_timings;
    panel_funcptr get_command_timings;
    panel_funcptr panel_init;
    panel_funcptr panel_standby;
}Panel_Config_t;

int get_panel_video_timings(struct mipi_dsi_dev *dev, unsigned char PanelID);
int get_panel_command_timings(struct mipi_dsi_dev *dev, unsigned char PanelID);
int mipi_panel_init(struct mipi_dsi_dev *dev, unsigned char PanelID);
void DSI_BYTE_WRITE(void *dev, unsigned char data1);
void DSI_SHORT_WRITE(void *dev, unsigned char data1, unsigned char data2);
void DSI_LONG_WRITE(void *dev, unsigned char data1, unsigned char data2, unsigned char data3,unsigned char data4);
int mipi_panel_standby(struct mipi_dsi_dev *dev, unsigned char PanelID);
int Panel_SendCmd(struct mipi_dsi_dev *dev, VPP_MIPI_CMD_TYPE enCmdType);
int Panel_SendCmdBuf(struct mipi_dsi_dev *dev, UINT8 *pcmd, UINT16 bufsize);
#endif
