// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "dsih_api.h"
#include "dsih_core.h"
#include "mipi_panel.h"
#include "mipi_hal.h"

#define DSI_CMD_DELAY         0xFF
#define DSI_CMD_BYTE_WRITE    0x5
#define DSI_CMD_SHORT_WRITE   0x15
#define DSI_CMD_LONG_WRITE    0x39
#define GEN_CMD_BYTE_WRITE    0x13
#define GEN_CMD_SHORT_WRITE   0x23
#define GEN_CMD_LONG_WRITE    0x29

#define MAX_DSI_LONG_WRITE_PARAMS 256

static int getCmdBufferInfo(struct mipi_dsi_dev *dev, VPP_MIPI_CMD_TYPE enCmdType,
                            VPP_MIPI_CMD_PARAMS *pstCmdBufInfo)
{
    int retval = MV_VPP_EUNSUPPORT;
    VPP_MIPI_CMD_HEADER *pMipiHeader;
    UINT16 cmdstartOffset;

    cmdstartOffset = (enCmdType == VPP_CMD_TYPE_INIT) ? MIPI_INIT_CMD_START : MIPI_STANDBY_CMD_START;
    pMipiHeader = (VPP_MIPI_CMD_HEADER*) &dev->dpi_video.DsiPanelcmd.pcmd[cmdstartOffset];
    if (pMipiHeader->cmd_type == enCmdType) {
        pstCmdBufInfo->pcmd = (ARCH_PTR_TYPE)&dev->dpi_video.DsiPanelcmd.pcmd[cmdstartOffset + MIPI_CMD_HEADER_SIZE];
        pstCmdBufInfo->bufsize = pMipiHeader->cmd_size;
        retval = MV_VPP_OK;
    }

    return retval;
}

int Panel_SendCmd(struct mipi_dsi_dev *dev, VPP_MIPI_CMD_TYPE enCmdType)
{
    int retval;
    VPP_MIPI_CMD_PARAMS stCmdBufInfo;

    /* Get the buffer start and size*/
    retval = getCmdBufferInfo(dev, enCmdType, &stCmdBufInfo);
    if (MV_VPP_OK == retval) {
        retval = Panel_SendCmdBuf(dev, (UINT8*)stCmdBufInfo.pcmd, stCmdBufInfo.bufsize);
    }

    return retval;
}

int Panel_SendCmdBuf(struct mipi_dsi_dev *dev, UINT8 *pcmd, UINT16 bufsize)
{
    int i = 0, retval = MV_VPP_OK;

    mipi_dsih_cmd_mode(dev, 1);

    while (i < bufsize)
    {
        /*Op-code*/
        switch (pcmd[i])
        {
            case DSI_CMD_DELAY:
                {
                    UINT32 delay;
                    delay = pcmd[i+1] |
                        (pcmd[i+2] << 8) |
                        (pcmd[i+3] << 16) |
                        (pcmd[i+4] << 24);
                    udelay(delay);
                    i+=5;
                }
                break;

            case DSI_CMD_BYTE_WRITE:
            case DSI_CMD_SHORT_WRITE:
            case GEN_CMD_BYTE_WRITE:
            case GEN_CMD_SHORT_WRITE:
                {
                    mipi_dsih_gen_wr_packet(dev, 0,
                            pcmd[i],
                            &pcmd[i+2],
                            pcmd[i+1]);
                    i = i + pcmd[i+1] + 2;
                }
                break;

            case GEN_CMD_LONG_WRITE:
            case DSI_CMD_LONG_WRITE:
                {
                    UINT8 dsi_cmd[MAX_DSI_LONG_WRITE_PARAMS];
                    dsi_cmd[0] = pcmd[i+1];
                    dsi_cmd[1] = 0;
                    VPP_Memcpy(&dsi_cmd[2], &pcmd[i+2], dsi_cmd[0]);
                    mipi_dsih_gen_wr_packet(dev, 0,
                            pcmd[i],
                            dsi_cmd,
                            pcmd[i+1]+2);
                    i = i + pcmd[i+1] + 2;
                }
                break;

            default:
                {
                    retval = MV_VPP_ENOTIMPL;
                    mipi_dbg_print(MIPI_ERROR, "Unsupported Command[%d]\n", pcmd[i]);
                }
                break;
        }

        if (MV_VPP_OK != retval)
            break;
    }
    mipi_dsih_cmd_mode(dev, 0);

    return retval;
}

int CUSTOM_get_video_timings(struct mipi_dsi_dev *dev)
{
    return (MV_VPP_OK);
}

int CUSTOM_get_command_timings(struct mipi_dsi_dev *dev, unsigned char PanelID)
{
    return MIPI_RET(EUNSUPPORT);
}

int CUSTOM_panel_init(struct mipi_dsi_dev *dev)
{
    int retval = MV_VPP_OK;

    mipi_dbg_print(MIPI_INFO, "CUSTOM_panel_init... \n");

    return (retval);
}

int CUSTOM_panel_standby(struct mipi_dsi_dev *dev)
{
    int retval = MV_VPP_OK;

    mipi_dbg_print(MIPI_INFO, "CUSTOM_panel_standby...[%d] \n",retval);

    return (retval);
}
