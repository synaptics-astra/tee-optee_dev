// SPDX-License-Identifier: GPL-2.0
/**
 * Copyright (C) 2025 Synopsys, Inc.
 *
 * @file dsih_displays.c
 * @brief included as a part of MIPI DSI Host driver
 * included as a part of Synopsys MIPI DSI Host controller driver
 *
 * @author Luis Oliveira <luis.oliveira@synopsys.com>
 */

#include "dsih_displays.h"
#include "dsih_api.h"
#include "dsih_hal.h"
#include "dsih_dphy.h"
#include "mipi_panel.h"
#include "mipi_hal.h"

#ifdef DISPLAYS
void copy_dpi_param_changes(dsih_dpi_video_t * from_param,
                dsih_dpi_video_t * to_param)
{
    to_param->no_of_lanes = from_param->no_of_lanes;
    to_param->non_continuous_clock = from_param->non_continuous_clock;
    to_param->virtual_channel = from_param->virtual_channel;
    to_param->video_mode = from_param->video_mode;
    to_param->byte_clock = from_param->byte_clock;
    to_param->pixel_clock = from_param->pixel_clock;
    to_param->color_coding = from_param->color_coding;
    to_param->is_18_loosely = from_param->is_18_loosely;
    to_param->h_polarity = from_param->h_polarity;
    to_param->h_active_pixels = from_param->h_active_pixels;
    to_param->h_sync_pixels = from_param->h_sync_pixels;
    to_param->h_back_porch_pixels = from_param->h_back_porch_pixels;
    to_param->h_total_pixels = from_param->h_total_pixels;
    to_param->v_polarity = from_param->v_polarity;
    to_param->v_active_lines = from_param->v_active_lines;
    to_param->v_sync_lines = from_param->v_sync_lines;
    to_param->v_back_porch_lines = from_param->v_back_porch_lines;
    to_param->v_total_lines = from_param->v_total_lines;
    to_param->max_hs_to_lp_cycles = from_param->max_hs_to_lp_cycles;
    to_param->max_lp_to_hs_cycles = from_param->max_lp_to_hs_cycles;
    to_param->max_clk_hs_to_lp_cycles = from_param->max_clk_hs_to_lp_cycles;
    to_param->max_clk_lp_to_hs_cycles = from_param->max_clk_lp_to_hs_cycles;
    to_param->eotp_rx_en = from_param->eotp_rx_en;
    to_param->eotp_tx_en = from_param->eotp_tx_en;
    to_param->no_of_chunks = from_param->no_of_chunks;
    to_param->null_packet_size = from_param->null_packet_size;
    to_param->dpi_lp_cmd_en = from_param->dpi_lp_cmd_en;
}

void copy_edpi_param_changes(dsih_cmd_mode_video_t * from_param,
                 dsih_cmd_mode_video_t * to_param)
{
    to_param->color_coding = from_param->color_coding;
    to_param->virtual_channel = from_param->virtual_channel;
    to_param->lp = from_param->lp;
    to_param->te = from_param->te;
    to_param->bta = from_param->bta;
    to_param->h_start = from_param->h_start;
    to_param->h_active_pixels = from_param->h_active_pixels;
    to_param->packet_size = from_param->packet_size;
    to_param->v_start = from_param->v_start;
    to_param->v_active_lines = from_param->v_active_lines;
    to_param->no_of_lanes = from_param->no_of_lanes;
    to_param->byte_clock = from_param->byte_clock;
    to_param->pixel_clock = from_param->pixel_clock;
}

/****************************************/
/*    look up tables                    */
/****************************************/
int pre_video_mode(struct mipi_dsi_dev *dev, unsigned mode, int lanes)
{
    if (dev == NULL)
        return MIPI_RET(ENODEV);

    if (!get_panel_video_timings(dev, mode)) {
        if (dsi_get_transition_times(dev) == TRUE) {
            return 0;
        } else {
            mipi_dbg_print(MIPI_ERROR,"error configuring video - freq out of range\n");
        }
    }
    mipi_dbg_print(MIPI_ERROR,"Invalid configuration" "\n");

    return 1;
}

int pre_command_mode(struct mipi_dsi_dev *dev, unsigned mode, int lanes)
{
    if (dev == NULL)
        return MIPI_RET(ENODEV);

    if (get_panel_command_timings(dev, mode))
        return 0;

    mipi_dbg_print(MIPI_ERROR, "Invalid configuration\n");

    return 1;
}

int dsi_screen_init(struct mipi_dsi_dev *dev, int screen, int video_mode)
{
    if (mipi_panel_init(dev, screen)) {
        mipi_dbg_print(MIPI_INFO,"%s:Screen not recognized!\n", FUNC_NAME);
    }
    return 1;
}

int dsi_user_screen_init(struct mipi_dsi_dev *dev, uint8_t vc, uint8_t *cmd, uint32_t length)
{
    uint32_t index = 0, cmd_length = 0, delay;
    uint8_t opcode;

    mipi_dsih_cmd_mode(dev, 1);
    while (length != 0) {
        //<opcode[1byte]><length[2byte]><payload[N byte]><delay[4 byte]>
        opcode = cmd[0];
        cmd_length = (cmd[2] << 8) | cmd[1];
        if ((!opcode) || (cmd_length > length)) {
            mipi_dbg_print(MIPI_ERROR,"%s: Invalid cmd: OP: 0x%x len: 0x%x!\n", \
                    FUNC_NAME, opcode, cmd_length);
            break;
        }
        index = 1 + 2 + cmd_length;
        delay = cmd[index] | (cmd[index + 1] << 8) | (cmd[index + 2] << 16) | (cmd[index + 3] << 24);

        if (cmd_length > 2)
            mipi_dsih_gen_wr_packet(dev, vc, opcode, &cmd[index + 1], cmd_length + 2);
        else
            mipi_dsih_gen_wr_packet(dev, vc, opcode, &cmd[index + 3], cmd_length);
        udelay(delay);

        cmd = cmd + cmd_length + 4;
        length = length - (cmd_length + 4 + 3);
    }
    return 0;
}
#endif
