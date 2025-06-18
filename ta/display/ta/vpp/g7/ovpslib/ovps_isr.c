/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#define _OVPSCL_ISR_C_

#include "vpp_module.h"
#include "vpp_fe.h"
#include "vpp_scl.h"
#include "vpp_cpcb.h"
#include "vpp_apifuncs.h"
#include "vpp_be.h"
#include "vpp_cfg.h"
#include "vpp_offlineScl.h"
#include "vpp_isr.h"
#include "vpp.h"
#include "vpp_fe_deint.h"
#include "vpp_fe_dlr.h"
#include "vpp_frc_scl.h"
#include "vpp_be_vop.h"
#include "ovps_bcmbuf.h"
#include "api_avio_dhub.h"
#include "avioDhub.h"
#include "cinclude.h"
#include "hdmiRxPipe.h"
#include "vpp_cpcb_ovl.h"
#include "vpp_be_prv.h"
#include "ovps_api.h"
#include "vpp_avioMtr.h"
#include "ovps_apifuncs.h"
#include "ovps_isr.h"
#include "ovps_frc_scl.h"
#include "vpp_clkGate.h"

#define VPP_OVPS_DHUB_SIZE  16
#define VPP_DBUB_BURST_OPT (256)
#define ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
#define VDBG_IF_DISP_ENABLE(enable)
#ifdef VDBG_IF_DISP_WIN
#undef VDBG_IF_DISP_WIN
#define VDBG_IF_DISP_WIN(window)\
    ovps_printf(OVPS_INFO,"%s[%d]:X=%d,Y=%d,W=%d,H=%d\n",__func__,__LINE__,window->x,window->y,window->width,window->height)
#endif
#define OVPS_BCMBUF_vidpl_Write(regaddr,regvalue)                                \
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf,                            \
            vpp_obj->base_addr + RA_Vpp_vidpl + regaddr,                         \
            regvalue)

#define OVPS_BCMBUF_vidpl_Read(regaddr,holder)                                   \
    GA_REG_WORD32_READ (vpp_obj->base_addr + RA_Vpp_vidpl + regaddr, holder)


#define SWAP_W_H(x,y)   \
{\
    int tmp;    \
    tmp = x;    \
    x = y;      \
    y = tmp;    \
};

#ifdef CONFIG_OVP_WITH_VPPLIB
#define DISP_FRST_TGRST_OVPFLUSH_EN  1
#else
#define DISP_FRST_TGRST_OVPFLUSH_EN  0
#endif

#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
#define SET_ELPACKSEL(uiElPackSel, val)  uiElPackSel = val
#else
#define SET_ELPACKSEL(uiElPackSel, val)
#endif
#define OVPS_SKIP_VBINUM_COUNT (6)
typedef enum
{
    VPP_FE_DLR_ROTATION_SRCFMT_MIN = 0,
    VPP_FE_DLR_ROTATION_SRCFMT_V4H8 = 0,
    VPP_FE_DLR_ROTATION_SRCFMT_MAX
}VPP_FE_DLR_ROTATION_SRCFMT;

typedef enum
{
    VPP_FE_DLR_BIT_DEPTH_MIN = 0,
    VPP_FE_DLR_BIT_DEPTH_8   = 0,
    VPP_FE_DLR_BIT_DEPTH_10  = 1,
    VPP_FE_DLR_BIT_DEPTH_MAX
}VPP_FE_DLR_BIT_DEPTH;

#define VPP_FE_DLR_LUT_REG_MAX 32

extern const UINT8 gVppFeRotationLUT_Y[VPP_FE_DLR_ROTATION_SRCFMT_MAX][VPP_FE_DLR_BIT_DEPTH_MAX][ROTATION_MAX] [VPP_FE_DLR_LUT_REG_MAX];
extern const UINT8 gVppFeRotationLUT_UV[VPP_FE_DLR_ROTATION_SRCFMT_MAX][VPP_FE_DLR_BIT_DEPTH_MAX][ROTATION_MAX] [VPP_FE_DLR_LUT_REG_MAX];

INT OVPS_EOF_Timeout(PVOID param)
{
    g_vpp = (VPP *)param;
    if (!g_vpp) {
        ovps_printf(OVPS_ERR, "%s called with NULL\n", __FUNCTION__);
    }
    OVPS_VPPOVPSOBJ_INIT();

    if (!ovp_obj)
        return MV_VPP_ENODEV;
    // just coming out of suspend? ignore user req
    if (ovp_obj->suspend)
        return MV_VPP_EBADPARAM;

    ovps_printf(OVPS_LOW, "ovpobj: %p mon: %p\r\n", ovp_obj, ovp_obj->ovps_monitor);
    /*Stop timer and notify to CA*/
    MV_Timer_Stop(&ovp_obj->ovps_monitor);
    OVPS_NotifyEvent(ovp_obj, ovp_obj->eventCode, MV_OVPS_EVENT_EOF_TIMEOUT);
    return MV_VPP_OK;
}

INT OVP_FE_DLR_ConfigCrLineCount(VPP_OBJ *vpp_obj, UINT uiPlaneId)
{
#ifdef CONFIG_AMP_VPP_ROTATION
    PLANE *plane = NULL;
    INT crLineCnt = 0;
    UINT32 uiRegAddr = 0;
    UINT32 uiRegVal = 0;

    plane = &(vpp_obj->ovp_obj.plane);

    if ((VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE0 == plane->uchClientMode)
            && (ROTATION_90D == plane->uchRotationAngle || ROTATION_270D == plane->uchRotationAngle)) {
        crLineCnt = 2;
    } else {
        crLineCnt = 3;
    }
    uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL2;
    vpp_obj->stElPipeCtrl2.uCTRL2_cr_line_cnt = crLineCnt;
    uiRegVal = vpp_obj->stElPipeCtrl2.u32;
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr, uiRegVal);
#endif
    return 0;
}

int OVP_Enable_OVPClient(VPP_OBJ *vpp_obj, int enable)
{
    OVPS_PARAMS->readTiledfromOVPclient = enable;
    if (!OVPS_PARAMS->readTiledfromOVPclient) {
        vpp_obj->ovp_obj.plane.dmaRID[0] = avioDhubChMap_vpp128b_MV_R2; // inline read DMA
        vpp_obj->ovp_obj.plane.dmaRID[1] = avioDhubChMap_vpp128b_MV_R3; // inline read DMA
    } else {
        vpp_obj->ovp_obj.plane.dmaRID[0] = avioDhubChMap_vpp128b_OVPSCL_R0; // inline read DMA
        vpp_obj->ovp_obj.plane.dmaRID[1] = avioDhubChMap_vpp128b_OVPSCL_R1; // inline read DMA
    }

    return (MV_VPP_OK);
}

INT OVP_FE_DLR_ConfigDifBlock(VPP_OBJ *vpp_obj, INT ldr_id, INT iSrcFmt, INT iOrder, INT iBitsPerPixel, INT iRotation)
{
#ifdef CONFIG_AMP_VPP_ROTATION
    UINT32 uiRegAddr_Y = 0;
    UINT32 uiRegAddr_C = 0;
    UINT32 uiRotationSrcFmt = 0, uiRotationBitDepth = 0, uiRotationAngle = 0;
    T32DIF_CFG0 stDifCfg0;
    UINT32 j;

    memset(&stDifCfg0, 0, sizeof(T32DIF_CFG0));

    uiRegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DIF2K_Y;
    uiRegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DIF2K_C;

    switch (iSrcFmt) {
        case SRCFMT_YUV420SP_TILED_V4H8:
            {
                uiRotationSrcFmt = VPP_FE_DLR_ROTATION_SRCFMT_V4H8;

                switch (iBitsPerPixel) {
                    //V4H8
                    case 8:
                        {
                            uiRotationBitDepth = VPP_FE_DLR_BIT_DEPTH_8;
                            uiRotationAngle = iRotation;

                            stDifCfg0.uCFG0_sft_rst     = 1;

                            OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
                            OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);

                            stDifCfg0.uCFG0_sft_rst     = 0;
                            stDifCfg0.uCFG0_tile_size   = 0x20;
                            stDifCfg0.uCFG0_PDWN0       = 0;
                            stDifCfg0.uCFG0_PDLVMC0     = 0;
                            stDifCfg0.uCFG0_PDFVSSM0    = 0;
                            stDifCfg0.uCFG0_PDWN1       = 0;
                            stDifCfg0.uCFG0_PDLVMC1     = 0;
                            stDifCfg0.uCFG0_PDFVSSM1    = 0;

                            OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
                            OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);
                        }
                        break;
                    default:
                        break;
                }
            }

        case SRCFMT_YUV420SP_TILED_MODE0:
            {
                uiRotationSrcFmt = VPP_FE_DLR_ROTATION_SRCFMT_V4H8;
                switch (iBitsPerPixel) {
                    case 10:
                        {
                            uiRotationBitDepth = VPP_FE_DLR_BIT_DEPTH_10;
                            uiRotationAngle = iRotation;

                            stDifCfg0.uCFG0_sft_rst     = 1;

                            OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
                            OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);

                            stDifCfg0.uCFG0_sft_rst     = 0;
                            stDifCfg0.uCFG0_tile_size   = 0x18;
                            stDifCfg0.uCFG0_PDWN0       = 0;
                            stDifCfg0.uCFG0_PDWN0       = 0;
                            stDifCfg0.uCFG0_PDLVMC0     = 0;
                            stDifCfg0.uCFG0_PDFVSSM0    = 0;
                            stDifCfg0.uCFG0_PDWN1       = 0;
                            stDifCfg0.uCFG0_PDLVMC1     = 0;
                            stDifCfg0.uCFG0_PDFVSSM1    = 0;

                            OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
                            OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);

                            OVP_FE_DLR_ConfigCrLineCount(vpp_obj, ldr_id);
                        }
                        break;

                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
    for (j = 0; j <= 31; j++) {
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_Y + RA_DIF_DIFLUT + RA_DIF_LUT_DIFLUT0 + j*4, gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][j]);
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr_C + RA_DIF_DIFLUT + RA_DIF_LUT_DIFLUT0 + j*4, gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][j]);
    }
#endif

    return 0;
}

INT OVP_FE_DLR_ConfigElPipe(VPP_OBJ *vpp_obj, INT iSrcFmt, INT iOrder, INT iBitDepth, INT iEnable)
{
    T32EL_PIPE_CTRL1 stElPipeCtrl1;
    T32MAINPL_CTRL4 stMainPlCtrl4;
    T32GFX0_TILE_CTRL stGfx0TileCtrl;
    T32UPS_420_422_CFG0 stUps420_422Cfg0_Y, stUps420_422Cfg0_UV;
    T32UPS_420_422_CFG5 stUps420_422Cfg5_Y, stUps420_422Cfg5_UV;
    T32UPS_420_422_CFG10 stUps420_422Cfg10;
    UINT32 uiRotationAngle = 0;
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
    UINT32 uiElPackSel = 0;
#endif

    stUps420_422Cfg0_Y.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y;
    stUps420_422Cfg0_UV.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C;

    stGfx0TileCtrl.u32 = 0;
    stUps420_422Cfg10.u32 = 0;
    stUps420_422Cfg5_Y.u32 = stUps420_422Cfg5_UV.u32 = 0;
    stElPipeCtrl1.u32 = vpp_obj->stElPipeCtrl1.u32;

    uiRotationAngle = vpp_obj->plane[PLANE_MAIN].uchRotationAngle;

    stElPipeCtrl1.uCTRL1_dif2k_blk_en = 1;
    stUps420_422Cfg5_Y.uCFG5_auto_pixcnt = stUps420_422Cfg5_UV.uCFG5_auto_pixcnt = 1;
    switch (iSrcFmt) {
    case SRCFMT_YUV422:
    case SRCFMT_YUV420SP:
        {
            stUps420_422Cfg0_Y.uCFG0_enable = 0;
            stUps420_422Cfg0_UV.uCFG0_enable = 0;
            stUps420_422Cfg0_Y.uCFG0_out422 = 0;
            stUps420_422Cfg0_UV.uCFG0_out422 = 0;

            stUps420_422Cfg10.uCFG10_tile_en = 0;
            stElPipeCtrl1.uCTRL1_dif2k_blk_en = 0;
            if(8 == iBitDepth) {
                SET_ELPACKSEL(uiElPackSel, 1);
            } else if(10 == iBitDepth) {
                SET_ELPACKSEL(uiElPackSel, 2);
            }
        }
        break;

    case SRCFMT_YUV420SP_TILED_MODE0:
        {
            stUps420_422Cfg0_Y.uCFG0_enable = 1;
            stUps420_422Cfg0_UV.uCFG0_enable = 1;
            stUps420_422Cfg0_Y.uCFG0_out422 = 0;
            stUps420_422Cfg0_UV.uCFG0_out422 = 0;

            stUps420_422Cfg10.uCFG10_tile_en = 1;
            if (8 == iBitDepth) {
                stUps420_422Cfg10.uCFG10_tile_depth =  4;
                stUps420_422Cfg10.uCFG10_tile_width =  8;
                SET_ELPACKSEL(uiElPackSel, 1);
            } else if (10 == iBitDepth) {
                stUps420_422Cfg10.uCFG10_tile_depth =  4;
                stUps420_422Cfg10.uCFG10_tile_width =  6;
                SET_ELPACKSEL(uiElPackSel, 3);
            }

            OVP_FE_DLR_ConfigDifBlock(vpp_obj, PLANE_PIP, iSrcFmt, iOrder, iBitDepth, uiRotationAngle);
        }
        break;

    case SRCFMT_YUV420SP_TILED_MODE1:
        {
            stUps420_422Cfg0_Y.uCFG0_enable = 1;
            stUps420_422Cfg0_UV.uCFG0_enable = 1;
            stUps420_422Cfg0_Y.uCFG0_out422 = 0;
            stUps420_422Cfg0_UV.uCFG0_out422 = 0;

            stUps420_422Cfg10.uCFG10_tile_en = 1;
            stUps420_422Cfg10.uCFG10_tile_depth =  4;
            stUps420_422Cfg10.uCFG10_tile_width =  4;
            SET_ELPACKSEL(uiElPackSel, 1);
            OVP_FE_DLR_ConfigDifBlock(vpp_obj, PLANE_PIP, iSrcFmt, iOrder, iBitDepth, uiRotationAngle);
        }
        break;

    case SRCFMT_YUV420SP_TILED_V4H8:
        {
            stUps420_422Cfg0_Y.uCFG0_enable = 1;
            stUps420_422Cfg0_UV.uCFG0_enable = 1;
            stUps420_422Cfg0_Y.uCFG0_out422 = 0;
            stUps420_422Cfg0_UV.uCFG0_out422 = 0;

            stUps420_422Cfg10.uCFG10_tile_en = 1;
            stUps420_422Cfg10.uCFG10_tile_depth =  4;
            stUps420_422Cfg10.uCFG10_tile_width =  8;
            SET_ELPACKSEL(uiElPackSel, 1);
            OVP_FE_DLR_ConfigDifBlock(vpp_obj, PLANE_PIP, iSrcFmt, iOrder, iBitDepth, uiRotationAngle);
        }
        break;
    default:
        break;
    }

    if (stUps420_422Cfg10.uCFG10_tile_en == 1) {
        memset(&stMainPlCtrl4, 0, sizeof(T32MAINPL_CTRL4));
        stMainPlCtrl4.u32 = vpp_obj->mainpl_ctrl4.u32;
        stMainPlCtrl4.uCTRL4_UPS420_2K_bypass = 0;
        vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4, stMainPlCtrl4.u32);
    }

    if (!OVPS_PARAMS->readTiledfromOVPclient) {
        if (8 == iBitDepth) {
            stElPipeCtrl1.uCTRL1_idatctrl = 0;
        } else if (10 == iBitDepth) {
            stElPipeCtrl1.uCTRL1_idatctrl = 1;
        }
    } else {
        if (8 == iBitDepth) {
            stElPipeCtrl1.uCTRL1_idatctrl = 4;
        } else if (10 == iBitDepth) {
            stElPipeCtrl1.uCTRL1_idatctrl = 5;
        }
    }

    if (!OVPS_PARAMS->readTiledfromOVPclient) {

        if (iEnable) {
            //EL pipe control settings.
            stElPipeCtrl1.uCTRL1_read_en = 1;
            stElPipeCtrl1.uCTRL1_el_rdsel_420SP = 1;
            stElPipeCtrl1.uCTRL1_pip_rd_sel = 0;
            stElPipeCtrl1.uCTRL1_dint_rd_sel = 1;

            stGfx0TileCtrl.uTILE_CTRL_detile_inp_en = 0;
            stGfx0TileCtrl.uTILE_CTRL_infifo_ctrl   = 0;
        } else {
            stUps420_422Cfg5_Y.uCFG5_auto_pixcnt = stUps420_422Cfg5_UV.uCFG5_auto_pixcnt = 0;
            //EL pipe control settings.
            stElPipeCtrl1.uCTRL1_read_en = 0;
            stElPipeCtrl1.uCTRL1_el_rdsel_420SP = 0;
            stElPipeCtrl1.uCTRL1_pip_rd_sel = 0;
            stElPipeCtrl1.uCTRL1_dint_rd_sel = 0;

            stGfx0TileCtrl.uTILE_CTRL_detile_inp_en = 0;
            stGfx0TileCtrl.uTILE_CTRL_infifo_ctrl   = 0;
        }
    }

#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
    //PackSel
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_pack, uiElPackSel);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_pack, uiElPackSel);
#endif

    vpp_obj->stElPipeCtrl1.u32 = stElPipeCtrl1.u32;
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1, stElPipeCtrl1.u32);

    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_TILE_CTRL, stGfx0TileCtrl.u32);

    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG5,         stUps420_422Cfg5_Y.u32);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG5,         stUps420_422Cfg5_UV.u32);

    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG10, stUps420_422Cfg10.u32);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG10, stUps420_422Cfg10.u32);
    vpp_obj->plane[PLANE_PIP].m_uiTileCfg10_C = stUps420_422Cfg10.u32;
    vpp_obj->plane[PLANE_PIP].m_uiTileCfg10_Y = stUps420_422Cfg10.u32;

    vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y = stUps420_422Cfg0_Y.u32;
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG0, stUps420_422Cfg0_Y.u32);
    vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C = stUps420_422Cfg0_UV.u32;
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG0, stUps420_422Cfg0_UV.u32);

    return 0;
}

INT OVP_FE_DLR_SetClientSize(VPP_OBJ *vpp_obj, INT chanID, INT Channel, PVPP_FE_DLR_INPUT_RES pReadSize, PVPP_FE_DLR_INPUT_RES pPlaneSize, UINT8 srcFmt, UINT8 BitsPerPixel) //DONE
{
    UINT32 RegAddr = 0;
    UINT32 RegSizeVal, HeightRead = 0, WidthRead =0;
    INT RetVal = MV_VPP_OK;
    int planeID;

    /*TODO: change this to pixtotal??*/
    if ((Channel < VPP_FE_DLR_CHANNEL_MIN) || (Channel > VPP_FE_DLR_CHANNEL_MAX))
        return MV_VPP_EBADPARAM;

    planeID = PLANE_OF_CHAN(vpp_obj, chanID);

    ovps_printf(OVPS_INFO, "%s:chanID = %d,pReadSize %dx%d, pPlaneSize = %dx%d,srcFmt = %d,BitsPerPixel = %d,readTiledfromOVPclient = %d\n",
            __func__,chanID,pReadSize->HRes,pReadSize->VRes,pPlaneSize->HRes,pPlaneSize->VRes,srcFmt,BitsPerPixel,OVPS_PARAMS->readTiledfromOVPclient);
    if (!OVPS_PARAMS->readTiledfromOVPclient) {
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
        if (Channel == VPP_FE_DLR_CHANNEL_OVP_EL_0)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_Word;
        else if (Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_Word;
#endif
    } else {
        if (Channel == VPP_FE_DLR_CHANNEL_OVP_EL_0)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL3;
        else if (Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL4;
    }

    /*get plane size register value*/

    //FIXME: word calculation for sp hardcoded need to fix this
    if ((vpp_obj->ovp_obj.plane.m_SampleCropEnable&& PLANE_OVP_EL == planeID)) {
        if ( Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1) {
            if ((vpp_obj->ovp_obj.plane.m_SampleCropEnable&& PLANE_OVP_EL == planeID)) {
                WidthRead = vpp_obj->ovp_obj.plane.m_Aligned_Chroma_Width;
                HeightRead  = vpp_obj->ovp_obj.plane.m_Aligned_Luma_Height/2;

            }

            if (srcFmt==SRCFMT_YUV420SP_TILED_MODE0 && INPUT_BIT_DEPTH_10BIT==INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel)) {
                RegSizeVal = (WidthRead * HeightRead * BitsPerPixel * 16 / 15)/(128);
            } else {
                RegSizeVal = ceil(((double)WidthRead * BitsPerPixel)/128) * HeightRead;
            }
        } else {
            if ((vpp_obj->ovp_obj.plane.m_SampleCropEnable&& PLANE_OVP_EL == planeID)) {
                if (Channel == VPP_FE_DLR_CHANNEL_OVP_EL_0) {
                    WidthRead = vpp_obj->ovp_obj.plane.m_Aligned_Luma_Width;
                    HeightRead =  vpp_obj->ovp_obj.plane.m_Aligned_Luma_Height;
                }
            }
            if (srcFmt==SRCFMT_YUV420SP_TILED_MODE0 && INPUT_BIT_DEPTH_10BIT==INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel)) {
                RegSizeVal = (WidthRead * HeightRead * BitsPerPixel * 16 / 15)/128;
            } else {
                RegSizeVal = ceil(((double)WidthRead * BitsPerPixel)/128) * HeightRead;
            }
        }
    } else {
        if (Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1) {
            if ((srcFmt==SRCFMT_YUV420SP_TILED_MODE0 && INPUT_BIT_DEPTH_10BIT==INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel))
                    || srcFmt==SRCFMT_YUV420SP_DWA)
                RegSizeVal = (pReadSize->HRes * pReadSize->VRes * BitsPerPixel * 16 / 15)/(128*2);
            else if (srcFmt==SRCFMT_YUV422SP_DWA)
                RegSizeVal = (pReadSize->HRes * pReadSize->VRes * BitsPerPixel * 16 / 15)/(128);
            else
                RegSizeVal = ceil(((double)pReadSize->HRes * BitsPerPixel)/128) * (pReadSize->VRes/2); //vtotal div by two
        } else {
            if ((srcFmt==SRCFMT_YUV420SP_TILED_MODE0 && INPUT_BIT_DEPTH_10BIT==INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel))
                    || srcFmt==SRCFMT_YUV420SP_DWA || srcFmt==SRCFMT_YUV422SP_DWA)
                RegSizeVal = (pReadSize->HRes * pReadSize->VRes * BitsPerPixel * 16 / 15)/128;
            else
                RegSizeVal = ceil(((double)pReadSize->HRes * BitsPerPixel)/128) * pReadSize->VRes;
        }
    }

    if (!RegAddr) {
        ovps_printf(OVPS_ERR, "Register Addr inval\n");
    } else {
        /*write plane size to BCM buffer*/
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, RegAddr, RegSizeVal)))
            return RetVal;
    }
    return RetVal;
}

INT OVP_FE_DEINT_VpClient_Clear(VPP_OBJ *vpp_obj, VPP_VP_CLIENT_TYPE client_id)
{
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr = 0;
    T32ReadClient_Rd readclient_rd;
    T32WriteClient_Wr writeclient_wr;

    if (client_id==VPP_VP_CLIENT_WRITE) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_WrClientdiW0 + RA_WriteClient_Wr;
        GA_REG_WORD32_READ(RegAddr, &writeclient_wr.u32);
        writeclient_wr.uWr_clear = 1;
        RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, writeclient_wr.u32);
        writeclient_wr.uWr_clear = 0;
        RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, writeclient_wr.u32);
    } else if (client_id==VPP_VP_CLIENT_READ0 || client_id==VPP_VP_CLIENT_READ1){
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
        if (client_id==VPP_VP_CLIENT_READ0)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_Rd;
        else
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_Rd;
        GA_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
#endif
        if (!RegAddr) {
            ovps_printf(OVPS_ERR, "Register Addr invalid\n");
        } else {
            readclient_rd.uRd_clear = 1;
            RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, readclient_rd.u32);
            readclient_rd.uRd_clear = 0;
            RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, readclient_rd.u32);
        }
    }

    return RetVal;
}


INT OVP_FE_DEINT_VpClient_Enable(VPP_OBJ *vpp_obj, VPP_VP_CLIENT_TYPE client_id, UINT8 Enable)
{
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr = 0;
    T32ReadClient_Rd readclient_rd;
    T32WriteClient_Wr writeclient_wr;

    if (client_id==VPP_VP_CLIENT_WRITE) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_WrClientdiW0 + RA_WriteClient_Wr;
        GA_REG_WORD32_READ(RegAddr, &writeclient_wr.u32);
        writeclient_wr.uWr_start = Enable;
        RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, RegAddr, writeclient_wr.u32);
        writeclient_wr.uWr_start = 0;
        RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, RegAddr, writeclient_wr.u32);
    } else if (client_id==VPP_VP_CLIENT_READ0 || client_id==VPP_VP_CLIENT_READ1){
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
        if (client_id==VPP_VP_CLIENT_READ0)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_Rd;
        else
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_Rd;
        GA_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
#endif
        if (!RegAddr) {
            ovps_printf(OVPS_ERR, "Register Addr invalid\n");
        } else {
            readclient_rd.uRd_start = Enable;
            RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, RegAddr, readclient_rd.u32);
            readclient_rd.uRd_start = 0;
            RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, RegAddr, readclient_rd.u32);
        }
    }

    return RetVal;
}

INT OVP_FE_DEINT_VpTG_Enable(VPP_OBJ *vpp_obj, UINT8 enable)
{
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr;
    T32VPTOP_CTRL_vptg_ctrl vptop_ctrl_vptg_ctrl;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_vptg_ctrl;
    GA_REG_WORD32_READ(RegAddr , &vptop_ctrl_vptg_ctrl.u32);

    if (0 == enable) {
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_clear = 1;
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_clear = 0;
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
    } else {
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_clear = 1;
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_clear = 0;
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);

        vptop_ctrl_vptg_ctrl.uvptg_ctrl_start = 1;
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
        vptop_ctrl_vptg_ctrl.uvptg_ctrl_start = 0;
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, RegAddr, vptop_ctrl_vptg_ctrl.u32);
    }

    return RetVal;
}

void OVPs_updateFEChannelCurrentFrame(VPP_OBJ *vpp_obj, int chanID,int planeID )
{
    PLANE *plane;
    VPP_OVP_OBJ *ovp_obj = &g_vpp->vpp_objs[0]->ovp_obj;
    OVPS_CLIENT_REQUEST *client = NULL;
    int ret_val;

    plane = &ovp_obj->plane;

    /* go through all active planes in */
    if ( OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ngptv_bypass == 0) {
        ovps_printf(OVPS_ERR, "planeID = %d,NGPTV not bypassed\n",planeID);
        return;
    }
#ifdef CONFIG_OVP_WITH_VPPLIB
    /* Push previous frame to out q */
    if (plane->prev_curr_frame != NULL) {
        client = (OVPS_CLIENT_REQUEST *)plane->prev_curr_frame;
        plane->prev_curr_frame = NULL;
    }

    if (plane->curr_frame != NULL) {
        plane->prev_curr_frame = ovp_obj->client_info;
        plane->curr_frame = NULL;
    }
#else
    if (plane->curr_frame != NULL) {
        client = (OVPS_CLIENT_REQUEST *)ovp_obj->client_info;
        plane->curr_frame = NULL; /*reset the curr_frame and try to pop a new frame*/
    }
#endif
    if (client != NULL) {
        MV_OSAL_Mutex_Lock(ovp_obj->client_obj_lock);
        if (frmq_push(&plane->outputq, client)) {
            ovp_obj->client_obj[client->m_uchClientId].outputQCount++;
        } else {
            ovps_printf(OVPS_ERR, "OutputQ Full\n");
        }
        MV_OSAL_Mutex_Unlock(ovp_obj->client_obj_lock);
    }
    /* try to get a new frame descriptor from frame queue */

    /*parallel pop request possible from ISR and ReleaseRequest, use locks*/
    MV_OSAL_Mutex_Lock(ovp_obj->ovps_inputQ_lock);
    /* try to get a new frame descriptor from frame queue */
    ret_val = frmq_pop(&plane->inputq, (void **)&client);
    if (ret_val)
        frmq_pop_commit(&plane->inputq);
    MV_OSAL_Mutex_Unlock(ovp_obj->ovps_inputQ_lock);
    if (ret_val) {
        MV_OSAL_Mutex_Lock(ovp_obj->client_obj_lock);
        if (ovp_obj->client_obj[client->m_uchClientId].rqstCount)
            ovp_obj->client_obj[client->m_uchClientId].rqstCount--;
        else
            ovps_printf(OVPS_ERR,"Mismatch in rqstCount\n");
        MV_OSAL_Mutex_Unlock(ovp_obj->client_obj_lock);

        /* new frame descriptor found */
        if (OVPS_Update_frame(vpp_obj,client) != MV_VPP_OK) {
            ovps_printf(OVPS_ERR,"Invalid Frame details\n");
            return ;
        }
        plane->frm_count ++;
        /* update current frame descriptor with new frame descriptor */
        plane->curr_frame = client->pinfo;
    }
}

/******************************************************************************************
 * FUNCTION: crop display content
 * PARAMS: *crop_actv_win - (O)pointer to cropped display content size in memory
 *         *crop_actv_offset - (O)pointer to cropped display content offset
 *         *crop_scl_actv_win - (O)pointer to cropped display content size in display window
 *         *pinfo - (I)pointer to display content info in memory
 *         *ref_win - (I)pointer to reference window
 *         *disp_win - (I)pointer to display window
 *         *cpcb_resinfo - (I)pointer to end display resolution info
 * RETURN: none
 *****************************************************************************************/
void ovps_cropDispContent(VPP_WIN *crop_actv_win, int *crop_actv_offset,
        VPP_WIN *crop_scl_actv_win, VBUF_INFO *pinfo, VPP_WIN *ref_win,
        VPP_WIN *disp_win, RESOLUTION_INFO *cpcb_resinfo, int dataAlign_x,
        int dataAlign_y, int bpp, PLANE *plane)
{
    int tmp;
    float bytesPerPixel = bpp/8.0;
    VPP_TILE tile;
    int ref_win_backup_width = 0, ref_win_backup_height = 0, ref_win_backup_x = 0, ref_win_backup_y = 0;
    int BL_Width = 0, BL_height = 0, Bl_TileStrideFrac = 0, BL_LeftPad = 0, BL_TopPad = 0;
    int mtr_pan_scan_x_offset = 0, mtr_pan_scan_y_offset = 0;
    int mtr_pan_scan_x_crop = 0, mtr_pan_scan_y_crop = 0;

    ref_win_backup_width = ref_win->width;
    ref_win_backup_height = ref_win->height;
    ref_win_backup_x = ref_win->x;
    ref_win_backup_y = ref_win->y;

    if ((plane->m_SeparateElcropRequired)&&(plane->m_El_crop_recalculation_required)) {
        ref_win->width = ref_win->width/2;
        ref_win->height = ref_win->height/2;
        ref_win->x = ref_win->x/2;
        ref_win->y = ref_win->y/2;
#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
        pinfo->m_active_left = -ref_win->x;
        pinfo->m_active_top = -ref_win->y;
#endif
        if ((plane->m_mtrR__en_el)&&(plane->m_SampleCropEnable_el)) {
            BL_Width = plane->m_Aligned_Luma_Width;
            BL_height = plane->m_Aligned_Luma_Height;
            BL_LeftPad = plane->m_leftpad_y;
            BL_TopPad = plane->m_toppad_y;
            plane->m_leftpad_y = pinfo->m_luma_left_ofst;
            plane->m_toppad_y = pinfo->m_luma_top_ofst;
        }

        Bl_TileStrideFrac = plane->tile.tileStrideFrac;
    }

    /* crop display content in reference window */

    /* initially set crop display content same with the original */
    crop_actv_win->x = pinfo->m_active_left;
    crop_actv_win->y = pinfo->m_active_top;
    crop_actv_win->width = pinfo->m_active_width;
    crop_actv_win->height = pinfo->m_active_height;
    *crop_actv_offset = 0;
    plane->m_leftpadadjust_y = 0;
    plane->m_toppadadjust_y = 0;
    VPP_OVL_getTileDetails(plane->uchClientMode, ref_win->width, ref_win->height, &tile);

    if (plane->m_SeparateElcropRequired) {
        plane->tileStrideFrac_EL = plane->tile.tileStrideFrac;
    }
    VDBG_IF_DISP_WIN(ref_win);
    VDBG_IF_DISP_WIN(disp_win);
    VDBG_IF_DISP_WIN(crop_actv_win);
    VDBG_IF_DISP_WIN(crop_scl_actv_win);
    ovps_printf(OVPS_INFO,"crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",*crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8), (int)((*crop_actv_offset)/(bytesPerPixel)));

    /* horizontal cropping */
    if (crop_actv_win->x < 0) {
        /* crop left side */
        tmp = -crop_actv_win->x;
        tmp -= tmp%dataAlign_x;

        if ((pinfo->m_srcfmt == SRCFMT_ARGB32)||
           (pinfo->m_srcfmt == SRCFMT_ARBG2101010)||
           (pinfo->m_srcfmt == SRCFMT_YUV422)||
           (pinfo->m_srcfmt == SRCFMT_ARGB4444)||
           (pinfo->m_srcfmt == SRCFMT_RGB565)||
           (pinfo->m_srcfmt == SRCFMT_ARGB1555)||
           (pinfo->m_srcfmt == SRCFMT_YUV420SP) ||
           (pinfo->m_srcfmt == SRCFMT_YUV444_DWA)) {

           *crop_actv_offset -= ((-tmp) * bytesPerPixel);
        } else if (pinfo->m_srcfmt == SRCFMT_YUV420SP_DWA ||
                pinfo->m_srcfmt == SRCFMT_YUV422SP_DWA) {

            *crop_actv_offset -= (((-tmp) * 10 * 32)/(30 * 8));
        } else { //TILED MODE

           *crop_actv_offset -= ((-tmp/tile.width) * tile.bytesPerTile);
        }

        ovps_printf(OVPS_INFO,"x:crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",
                     *crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8), (int)((*crop_actv_offset)/(bytesPerPixel)));

        crop_actv_win->width += crop_actv_win->x;
        crop_actv_win->x = 0;
    }

    if ((crop_actv_win->x + crop_actv_win->width) > ref_win->width) {
        /* crop right side */
        crop_actv_win->width -= (crop_actv_win->x + crop_actv_win->width - ref_win->width);
    }

    if (crop_actv_win->width <= 0)
        return;

    crop_scl_actv_win->x = crop_actv_win->x * disp_win->width / ref_win->width + disp_win->x;
    crop_scl_actv_win->width = crop_actv_win->width * disp_win->width / ref_win->width;

    /* horizontal cropping of display content */
    if (crop_scl_actv_win->x < 0) {
        /* crop left side */
        tmp = crop_scl_actv_win->x * ref_win->width / disp_win->width;
        crop_actv_win->width += tmp;
#ifdef CONFIG_AMP_VPP_ROTATION
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
        {   if (plane->m_SampleCropEnable || plane->m_SampleCropEnable_el) {
                mtr_pan_scan_x_offset = (-tmp)/dataAlign_x;
                mtr_pan_scan_x_crop = ((-tmp))%dataAlign_x;
                plane->m_ref_win_update_pan_scan = 1;
            } else {
                *crop_actv_offset -= ((tmp/tile.width) * tile.bytesPerTile);
            }
        } else {
            *crop_actv_offset -= (tmp * bytesPerPixel);
        }
        ovps_printf(OVPS_INFO,"cx:crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",*crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8), (int)((*       crop_actv_offset)/(bytesPerPixel)));
        crop_scl_actv_win->width += crop_scl_actv_win->x;
        crop_scl_actv_win->x = 0;
    }
    if ((crop_scl_actv_win->x + crop_scl_actv_win->width) > cpcb_resinfo->active_width) {
        /* crop right side */
        tmp = crop_scl_actv_win->x + crop_scl_actv_win->width - cpcb_resinfo->active_width;
        crop_actv_win->width -= tmp * ref_win->width / disp_win->width;
        crop_scl_actv_win->width -= tmp;
        plane->m_ref_win_update_pan_scan = 1;
    }

    if ((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
        pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
        SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)&&(plane->m_SampleCropEnable || plane->m_SampleCropEnable_el)) {
        int xalign = 0;
        int temp = -(pinfo->m_active_left);
        int AlignedWidth = 0;
        if (temp < 0)
            temp = -temp;
        xalign = dataAlign_x;

        if (!plane->m_SeparateElcropRequired) {
            plane->uDhubCropOffset_y.u32_Tile_Xoffset = mtr_pan_scan_x_offset + ((temp + plane->m_leftpad_y)/xalign);
        } else {
            plane->el_uDhubCropOffset_y.u32_Tile_Xoffset = mtr_pan_scan_x_offset + ((temp + plane->m_leftpad_y)/xalign);
        }
        plane->m_leftpadadjust_y = ((mtr_pan_scan_x_crop + temp + plane->m_leftpad_y)%xalign);

        if (pinfo->m_bits_per_pixel == 8) {
            AlignedWidth = ((crop_actv_win->width  + plane->m_leftpadadjust_y + 63)/64)*64;
            plane->m_rightpad_y = AlignedWidth - (crop_actv_win->width + plane->m_leftpadadjust_y);
            plane->m_Aligned_Luma_Width = AlignedWidth;
        } else if (pinfo->m_bits_per_pixel == 10) {
            AlignedWidth = ((crop_actv_win->width + plane->m_leftpadadjust_y + 47)/48)*48;
            plane->m_rightpad_y = AlignedWidth - (crop_actv_win->width + plane->m_leftpadadjust_y);
            plane->m_Aligned_Luma_Width = AlignedWidth;
        }
    }

    /* vertical cropping */
    if (crop_actv_win->y < 0) {
        /* crop top side */
        tmp = -crop_actv_win->y;
        tmp -= tmp%dataAlign_y;
#ifdef CONFIG_AMP_VPP_ROTATION
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
            *crop_actv_offset -= (-tmp * ((int)(pinfo->m_buf_stride/VPP_TILED_MODE_STRIDE_FACTOR)));
        else
            *crop_actv_offset -= (-tmp * ((int)pinfo->m_buf_stride));
        ovps_printf(OVPS_INFO,"y:crop_actv_offset : 0x%x, y:%d, bits_per_pixels= %d, pixel = %d\n",*crop_actv_offset, crop_actv_win->y, (int)(bytesPerPixel*8), (int)((*crop_actv_offset)/(bytesPerPixel)));
        crop_actv_win->height += crop_actv_win->y;
        crop_actv_win->y = 0;
    }

    if ((crop_actv_win->y + crop_actv_win->height) > ref_win->height) {
        /* crop bottom side */
        crop_actv_win->height -= (crop_actv_win->y + crop_actv_win->height - ref_win->height);
    }

    if (crop_actv_win->height <= 0)
        return;

    crop_scl_actv_win->y = crop_actv_win->y * disp_win->height / ref_win->height + disp_win->y;
    crop_scl_actv_win->height = crop_actv_win->height * disp_win->height / ref_win->height;

    /* vertical cropping of display content */
    if (crop_scl_actv_win->y < 0) {
        /* crop top side */
        tmp = crop_scl_actv_win->y * ref_win->height / disp_win->height;
        crop_actv_win->height += tmp;
#ifdef CONFIG_AMP_VPP_ROTATION
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
        {
            if (plane->m_SampleCropEnable || plane->m_SampleCropEnable_el) {
                mtr_pan_scan_y_offset = (-tmp)/dataAlign_y;
                mtr_pan_scan_y_crop = (-tmp)%(dataAlign_y*2);
                plane->m_ref_win_update_pan_scan = 1;
            } else {
                *crop_actv_offset -= tmp * ((int)(pinfo->m_buf_stride/VPP_TILED_MODE_STRIDE_FACTOR));
            }
        } else {
            *crop_actv_offset -= tmp * ((int)pinfo->m_buf_stride);
        }
        ovps_printf(OVPS_INFO,"cy:crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",*crop_actv_offset, crop_actv_win->x, (int)(bytesPerPixel*8), (int)((*       crop_actv_offset)/(bytesPerPixel)));
        crop_scl_actv_win->height += crop_scl_actv_win->y;
        crop_scl_actv_win->y = 0;
    }
    if ((crop_scl_actv_win->y + crop_scl_actv_win->height) > cpcb_resinfo->active_height) {
        /* crop bottom side */
        tmp = crop_scl_actv_win->y + crop_scl_actv_win->height - cpcb_resinfo->active_height;
        crop_actv_win->height -= tmp * ref_win->height / disp_win->height;
        plane->m_ref_win_update_pan_scan = 1;
        crop_scl_actv_win->height -= tmp;
    }

    if ((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
       pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
       SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)&&(plane->m_SampleCropEnable || plane->m_SampleCropEnable_el)) {
        int yalign = 0;
        int temp = -(pinfo->m_active_top);
        int AlignedHeight = 0;
        if (temp < 0)
            temp = -temp;
        yalign = 8;//dataAlign_y;
        if (!plane->m_SeparateElcropRequired) {
            plane->uDhubCropOffset_y.u32_Tile_Yoffset = mtr_pan_scan_y_offset + (((temp/yalign)*2) + (plane->m_toppad_y/4));
        } else {
            plane->el_uDhubCropOffset_y.u32_Tile_Yoffset = mtr_pan_scan_y_offset + (((temp/yalign)*2) + (plane->m_toppad_y/4));
        }
        plane->m_toppadadjust_y = ((temp+mtr_pan_scan_y_crop)%yalign);
        AlignedHeight = ((crop_actv_win->height + plane->m_toppadadjust_y + 7)/8)*8;
        plane->m_bottompad_y = AlignedHeight - (crop_actv_win->height + plane->m_toppadadjust_y);
        plane->m_Aligned_Luma_Height = AlignedHeight;

    }

    if (plane->m_SeparateElcropRequired) {
        if (plane->m_mtrR__en_el && plane->m_SampleCropEnable_el) {
            plane->m_El_Aligned_Luma_Width = plane->m_Aligned_Luma_Width;
            plane->m_El_Aligned_Luma_Height = plane->m_Aligned_Luma_Height;
            plane->m_Aligned_Luma_Width = BL_Width;
            plane->m_Aligned_Luma_Height = BL_height;
            plane->m_leftpad_y = BL_LeftPad;
            plane->m_toppad_y = BL_TopPad;
        }
        plane->tile.tileStrideFrac = Bl_TileStrideFrac;
    }

    ref_win->width = ref_win_backup_width;
    ref_win->height = ref_win_backup_height;
    ref_win->x = ref_win_backup_x;
    ref_win->y = ref_win_backup_y;

    VDBG_IF_DISP_WIN(crop_actv_win);
    VDBG_IF_DISP_WIN(crop_scl_actv_win);
    ovps_printf(OVPS_INFO, "crop_actv_offset : 0x%x, x:%d, bits_per_pixels= %d, pixel = %d\n",
            *crop_actv_offset, crop_actv_win->x,
            (int)(bytesPerPixel*8), (int)((*crop_actv_offset)/(bytesPerPixel)));
    return;
}

void ovps_cropDispContent_UV(VPP_WIN *crop_actv_win, int *crop_actv_offset,
        VPP_WIN *crop_scl_actv_win, VBUF_INFO *pinfo, VPP_WIN *ref_win,
        VPP_WIN *disp_win, RESOLUTION_INFO *cpcb_resinfo, int dataAlign_x,
        int dataAlign_y, int bpp, PLANE *plane)
{
    int tmp;
    float bytesPerPixel = bpp/8.0;
    VPP_TILE tile;
    int ref_win_backup_width = 0, ref_win_backup_height = 0, ref_win_backup_x = 0, ref_win_backup_y = 0;
    int BL_Width = 0, BL_Height = 0, BL_LeftPad = 0, BL_TopPad = 0;
    int mtr_pan_scan_x_offset = 0, mtr_pan_scan_y_offset = 0;
    int mtr_pan_scan_x_crop = 0, mtr_pan_scan_y_crop = 0;

    ref_win_backup_width = ref_win->width;
    ref_win_backup_height = ref_win->height;
    ref_win_backup_x = ref_win->x;
    ref_win_backup_y = ref_win->y;

    if ((plane->m_SeparateElcropRequired)&&(plane->m_El_crop_recalculation_required)) {
        ref_win->width = ref_win->width/2;
        ref_win->height = ref_win->height/2;
        ref_win->x = ref_win->x/2;
        ref_win->y = ref_win->y/2;

#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
        pinfo->m_active_left = -ref_win->x;
        pinfo->m_active_top = -ref_win->y;
#endif
        if ((plane->m_mtrR__en_el)&&(plane->m_SampleCropEnable_el)) {
            BL_Width = plane->m_Aligned_Chroma_Width;
            BL_Height = plane->m_Aligned_Chroma_Height;
            BL_LeftPad = plane->m_leftpad_c;
            BL_TopPad = plane->m_toppad_c;
            plane->m_leftpad_c = pinfo->m_chroma_left_ofst;
            plane->m_toppad_c = pinfo->m_chroma_top_ofst;
        }
    }

    /* crop display content in reference window */

    /* initially set crop display content same with the original */
    crop_actv_win->x = pinfo->m_active_left;
    crop_actv_win->y = pinfo->m_active_top/2;
    //since y is already divide by 2, align should also be be divide by 2
    dataAlign_y = dataAlign_y>4 ? dataAlign_y/2 : dataAlign_y;
    crop_actv_win->width = pinfo->m_active_width;
    crop_actv_win->height = pinfo->m_active_height/2;
    if (pinfo->m_srcfmt == SRCFMT_YUV422SP_DWA) {
        crop_actv_win->y = pinfo->m_active_top;
        crop_actv_win->height = pinfo->m_active_height;
    }
    *crop_actv_offset = 0;
    plane->m_leftpadadjust_c = 0;
    plane->m_toppadadjust_c = 0;

    VPP_OVL_getTileDetails(plane->uchClientMode, ref_win->width, ref_win->height/2, &tile);

    /* horizontal cropping */
    if (crop_actv_win->x < 0) {
        /* crop left side */
        tmp = -crop_actv_win->x;
        tmp -= tmp%dataAlign_x;

        if ((pinfo->m_srcfmt == SRCFMT_YUV420SP)) {
            *crop_actv_offset -= ((-tmp) * bytesPerPixel);
        } else if (pinfo->m_srcfmt == SRCFMT_YUV420SP_DWA ||
                pinfo->m_srcfmt == SRCFMT_YUV422SP_DWA) {
            *crop_actv_offset -= (((-tmp) * 10 * 32)/(30 * 8));
        } else { //TILED MODE
            *crop_actv_offset -= ((-tmp/tile.width) * tile.bytesPerTile);
        }

        crop_actv_win->width += crop_actv_win->x;
        crop_actv_win->x = 0;
    }

    if ((crop_actv_win->x + crop_actv_win->width) > ref_win->width) {
        /* crop right side */
        crop_actv_win->width -= (crop_actv_win->x + crop_actv_win->width - ref_win->width);
    }

    if (crop_actv_win->width <= 0)
        return;

    crop_scl_actv_win->x = crop_actv_win->x * disp_win->width / ref_win->width + disp_win->x;
    crop_scl_actv_win->width = crop_actv_win->width * disp_win->width / ref_win->width;

    /* horizontal cropping of display content */
    if (crop_scl_actv_win->x < 0) {
        /* crop left side */

        tmp = crop_scl_actv_win->x * ref_win->width / disp_win->width;
        crop_actv_win->width += tmp;
#ifdef CONFIG_AMP_VPP_ROTATION
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
        {
            if (plane->m_SampleCropEnable || plane->m_SampleCropEnable_el) {
                mtr_pan_scan_x_offset  = (-tmp)/dataAlign_x;
                mtr_pan_scan_x_crop = ((-tmp))%dataAlign_x;
            } else {
                *crop_actv_offset -= ((tmp/tile.width) * tile.bytesPerTile);
            }
        } else {
            *crop_actv_offset -= tmp * pinfo->m_bytes_per_pixel;
        }
        crop_scl_actv_win->width += crop_scl_actv_win->x;
        crop_scl_actv_win->x = 0;
    }
    if ((crop_scl_actv_win->x + crop_scl_actv_win->width) > cpcb_resinfo->active_width) {
        /* crop right side */
        tmp = crop_scl_actv_win->x + crop_scl_actv_win->width - cpcb_resinfo->active_width;
        crop_actv_win->width -= tmp * ref_win->width / disp_win->width;
        crop_scl_actv_win->width -= tmp;
    }

    if ((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
        pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
        SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)&&(plane->m_SampleCropEnable || plane->m_SampleCropEnable_el)) {
        int xalign = 0;
        int temp = -(pinfo->m_active_left);
        int AlignedWidth = 0;
        if (temp < 0)
            temp = -temp;
        xalign = dataAlign_x;
        if (!plane->m_SeparateElcropRequired) {
           plane->uDhubCropOffset_c.u32_Tile_Xoffset = mtr_pan_scan_x_offset + ((temp + plane->m_leftpad_c)/xalign);
        } else {
           plane->el_uDhubCropOffset_c.u32_Tile_Xoffset = mtr_pan_scan_x_offset + ((temp + plane->m_leftpad_c)/xalign);
        }
        plane->m_leftpadadjust_c = ((temp + mtr_pan_scan_x_crop + plane->m_leftpad_c)%xalign);

        if (pinfo->m_bits_per_pixel == 8) {
            AlignedWidth = ((crop_actv_win->width + plane->m_leftpadadjust_c + 63)/64)*64;
            plane->m_rightpad_c = AlignedWidth - (crop_actv_win->width + plane->m_leftpadadjust_c);
            plane->m_Aligned_Chroma_Width = AlignedWidth;
        } else if (pinfo->m_bits_per_pixel == 10) {
            AlignedWidth = ((crop_actv_win->width + plane->m_leftpadadjust_c + 47)/48)*48;
            plane->m_rightpad_c = AlignedWidth - (crop_actv_win->width + plane->m_leftpadadjust_c);
            plane->m_Aligned_Chroma_Width = AlignedWidth;
        }

    }

    /* vertical cropping */
    if (crop_actv_win->y < 0) {
        /* crop top side */
        tmp = -crop_actv_win->y;
        tmp -= tmp%dataAlign_y;
#ifdef CONFIG_AMP_VPP_ROTATION
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
            *crop_actv_offset -= (-tmp * ((int)(pinfo->m_buf_stride_UV/VPP_TILED_MODE_STRIDE_FACTOR)));
        else
            *crop_actv_offset -= (-tmp * ((int)pinfo->m_buf_stride_UV));
        ovps_printf(OVPS_INFO,"y:crop_actv_offset_uv : 0x%x, y:%d, bits_per_pixels= %d, pixel = %d\n",*crop_actv_offset, crop_actv_win->y, (int)(bytesPerPixel*8), (int)((*crop_actv_offset)/(bytesPerPixel)));
        crop_actv_win->height += crop_actv_win->y;
        crop_actv_win->y = 0;
    }

    if ((crop_actv_win->y + crop_actv_win->height) > (ref_win->height/2)) {
        /* crop bottom side */
        crop_actv_win->height -= (crop_actv_win->y + crop_actv_win->height - (ref_win->height/2));
    }

    if (crop_actv_win->height <= 0)
        return;

    crop_scl_actv_win->y = crop_actv_win->y * disp_win->height / ref_win->height + (disp_win->y/2);
    crop_scl_actv_win->height = crop_actv_win->height * disp_win->height / ref_win->height;

    /* vertical cropping of display content */
    if (crop_scl_actv_win->y < 0) {
        /* crop top side */
        tmp = crop_scl_actv_win->y * ref_win->height / disp_win->height;
        crop_actv_win->height += tmp;
#ifdef CONFIG_AMP_VPP_ROTATION
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
        if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
        {
            if (plane->m_SampleCropEnable || plane->m_SampleCropEnable_el) {
                mtr_pan_scan_y_offset  = (-tmp)/dataAlign_y;
                mtr_pan_scan_y_crop = (-tmp)%dataAlign_y;
            } else {
                *crop_actv_offset -= tmp * ((int)(pinfo->m_buf_stride_UV/VPP_TILED_MODE_STRIDE_FACTOR));
            }
        } else {
            *crop_actv_offset -= tmp * ((int)pinfo->m_buf_stride_UV);
        }
        ovps_printf(OVPS_INFO,"y:crop_actv_offset_uv : 0x%x, y:%d, bits_per_pixels= %d, pixel = %d\n",*crop_actv_offset, crop_actv_win->y, (int)(bytesPerPixel*8), (int)((*     crop_actv_offset)/(bytesPerPixel)));
        crop_scl_actv_win->height += crop_scl_actv_win->y;
        crop_scl_actv_win->y = 0;
    }
    if ((crop_scl_actv_win->y + crop_scl_actv_win->height) > (cpcb_resinfo->active_height/2)) {
        /* crop bottom side */
        tmp = crop_scl_actv_win->y + crop_scl_actv_win->height - (cpcb_resinfo->active_height/2);
        crop_actv_win->height -= tmp * ref_win->height / disp_win->height;
        crop_scl_actv_win->height -= tmp;
    }

    if ((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
       pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
       SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)&&(plane->m_SampleCropEnable || plane->m_SampleCropEnable_el)) {
        int yalign = 0;
        int temp = -(pinfo->m_active_top/2);
        int AlignedHeight = 0;
        if (temp < 0)
            temp = -temp;
        yalign = dataAlign_y;

        if (!plane->m_SeparateElcropRequired) {
            plane->uDhubCropOffset_c.u32_Tile_Yoffset = mtr_pan_scan_y_offset + ((temp + plane->m_toppad_c)/yalign);
        } else {
            plane->el_uDhubCropOffset_c.u32_Tile_Yoffset = mtr_pan_scan_y_offset + ((temp + plane->m_toppad_c)/yalign);
        }
        plane->m_toppadadjust_c = ((temp+mtr_pan_scan_y_crop)%yalign);
        AlignedHeight = (((crop_actv_win->height) + plane->m_toppadadjust_c + 3)/4)*4;
        plane->m_bottompad_c = AlignedHeight - ((crop_actv_win->height) + plane->m_toppadadjust_c);
        plane->m_Aligned_Chroma_Height = AlignedHeight;
    }

    if ((plane->m_SeparateElcropRequired)&&(plane->m_SampleCropEnable_el)&&(plane->m_mtrR__en_el)) {
        plane->m_El_Aligned_Chroma_Width = plane->m_Aligned_Chroma_Width;
        plane->m_El_Aligned_Chroma_Height = plane->m_Aligned_Chroma_Height;
        plane->m_Aligned_Chroma_Width = BL_Width;
        plane->m_Aligned_Chroma_Height = BL_Height;
        plane->m_leftpad_c = BL_LeftPad;
        plane->m_toppad_c = BL_TopPad;
    }

    ref_win->width = ref_win_backup_width;
    ref_win->height = ref_win_backup_height;
    ref_win->x = ref_win_backup_x;
    ref_win->y = ref_win_backup_y;

    VDBG_IF_DISP_WIN(crop_actv_win);
    VDBG_IF_DISP_WIN(crop_scl_actv_win);
    ovps_printf(OVPS_INFO,"crop_actv_offset_uv : 0x%x, y:%d, bits_per_pixels= %d, pixel = %d\n",*crop_actv_offset, crop_actv_win->y, (int)(bytesPerPixel*8), (int)((*crop_actv_offset)/(bytesPerPixel)));

    return;
}

/* Calclute the virtual stride(MTR stride) from DMA stride */
UINT32 ovpsMtrR_ConfigStride(UINT8 *semId, UINT32 stride)
{
    int mtrRStride = 0;
    int val;

    val = (stride - 1 ) >> 12; /* Min MTR stride is 2^12. So discarding the same, which is not required to set for MTR stride */
    while (val > 0) {
        mtrRStride++;
        val = val >> 1;
    }

    /* MTR stide is from 2-4 bit position */
    *semId &= (~0x1C);
    if (mtrRStride < 5)
        *semId |= (mtrRStride << 2);
    else
        return MV_VPP_EBADPARAM;
    return MV_VPP_OK;
}

int ovpsSetDhubChannelCfg(VPP_OBJ *vpp_obj, INT dmaId, UINT8 uchInverseScanMode, SIGN32 mtuByte)
{
    HDL_dhub *dhub = &VPP_dhubHandle.dhub;
    UINT32 RegAddr;
    T32dHubChannel_CFG  cfg;

    switch (mtuByte) {
        case dHubChannel_CFG_MTU_8byte  : dhub->MTUb[dmaId] = 3; break;
        case dHubChannel_CFG_MTU_32byte : dhub->MTUb[dmaId] = 5; break;
        case dHubChannel_CFG_MTU_128byte: dhub->MTUb[dmaId] = 7; break;
        case dHubChannel_CFG_MTU_4096byte : dhub->MTUb[dmaId] = 12; break;
    }
    cfg.u32 = 0;
    cfg.uCFG_MTU = mtuByte;
    cfg.uCFG_QoS = VPP_config[dmaId].chanQos;
    cfg.uCFG_selfLoop = VPP_config[dmaId].chanSelfLoop;
    switch (uchInverseScanMode) {
        case 0:
            //Normal Scanning
            cfg.uCFG_hScan = 0; cfg.uCFG_vScan = 0;
            break;
        case 1:
            //Only H inverse scan
            cfg.uCFG_hScan = 1;
            cfg.uCFG_vScan = 0;
            break;
        case 2:
            //Only V inverse Scan
            cfg.uCFG_hScan = 0;
            cfg.uCFG_vScan = 1;
            break;
        case 3:
            //Both HV inverse
            cfg.uCFG_hScan = 1;
            cfg.uCFG_vScan = 1;
            break;
        default :
            return MV_VPP_EBADPARAM;
    }
    RegAddr = dhub->ra + RA_dHubReg_ARR + dmaId*sizeof(SIE_dHubChannel);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, RegAddr, cfg.u32);
    VPP_config[dmaId].chanMtuSize = mtuByte;

    return MV_VPP_OK;
}

typedef struct OVPS_DHUB_CFG_2NDDMA_PARAM_s
{
    /* Input params */
    INT dhubId;
    INT dmaId;
    UNSG32 addr;
    SIGN32 width; /* Number of bytes in a line or tile line */
    SIGN32 stride; /* Number of bytes in a line or tile line */
    SIGN32 height; /* Number of lines or tile lines */
    INT *cfgQ;
    UINT8 mtrREn;
    UINT8 updSemId;
    UINT8 chkSemId;
    PLANE *plane;
    /* Output params */
}OVPS_DHUB_CFG_2NDDMA_PARAM;

/* calculate 2NDDMA params to support VPP_DBUB_BURST_OPT */
UNSG32 ovpsDhubCfg2NDDMA(VPP_OBJ *vpp_obj, OVPS_DHUB_CFG_2NDDMA_PARAM *pstCfg2NDDMAParam)
{
    SIGN32 burst = 0, step1 = 0, size1 = 0, step2 = 0, size2 = 0;
    SIGN32 mtu = 0;
    UNSG32 addr;
    UNSG32 RetVal;

    if ((0 == pstCfg2NDDMAParam->plane->uchInverseScanMode || 2 == pstCfg2NDDMAParam->plane->uchInverseScanMode) &&
            ((1 == pstCfg2NDDMAParam->mtrREn) || !(pstCfg2NDDMAParam->width & (VPP_DBUB_BURST_OPT-1)))) {
        /* if width is multiple of VPP_DBUB_BURST_OPT. This is a must for compressed input*/
        /* H flip and HV flip can't support 256B burst */
        burst = VPP_DBUB_BURST_OPT;
        step1 = VPP_DBUB_BURST_OPT;
        size1 = pstCfg2NDDMAParam->width / VPP_DBUB_BURST_OPT;
        mtu   = dHubChannel_CFG_MTU_4096byte;
    } else if (VPP_IS_MULTIPLE_OF_MMU_BURST(pstCfg2NDDMAParam->width)) {
        /* MMU only enabled case(eg. 420SP on BG5CTpZ1), width should be multiple of VPP_DHUB_BURST_MMU_ONLY */
        /* For non-MTR, MMU enabled cases burst size need to be VPP_DHUB_BURST_MMU_ONLY. Otherwise
         * horizontal lines are observed in dolphin */
        burst = VPP_DHUB_BURST_MMU_ONLY;
        mtu   = AVIO_DHUB_MTU_MMU_ONLY;
        step1 = burst;
        size1 = pstCfg2NDDMAParam->width / burst;
    } else {
        /* Uncompressed and widht is not a multiple of VPP_DBUB_BURST_OPT */
        burst = pstCfg2NDDMAParam->width;
        step1 = pstCfg2NDDMAParam->width;
        size1 = 1;
        mtu   = dHubChannel_CFG_MTU_128byte;
    }

    if (pstCfg2NDDMAParam->mtrREn) {

        if ((pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_MV_R0)
            || (pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_GFX_PIP_R0)
            || (pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_OVPSCL_R0)) {
            // Luma addr calulation.
            addr  = VPP_AVIOMTR_DHUB_ADDR + ( (pstCfg2NDDMAParam->plane->m_toppad_y / pstCfg2NDDMAParam->plane->tile.height) * VPP_AVIOMTR_DHUB_STRIDE );
        } else if ((pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_MV_R1)
            || (pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_PIP_R1)
            || (pstCfg2NDDMAParam->dmaId == avioDhubChMap_vpp128b_OVPSCL_R1)) {
            // Chroma addr calulation.
            addr  = VPP_AVIOMTR_DHUB_ADDR + ( (pstCfg2NDDMAParam->plane->m_toppad_c / pstCfg2NDDMAParam->plane->tile.height) * VPP_AVIOMTR_DHUB_STRIDE );
        } else {
            addr  = VPP_AVIOMTR_DHUB_ADDR;
        }

        step2 = VPP_AVIOMTR_DHUB_STRIDE;
    } else {
        addr = pstCfg2NDDMAParam->addr;
        step2 = pstCfg2NDDMAParam->stride;
    }

    size2 = pstCfg2NDDMAParam->height;

    ovpsSetDhubChannelCfg(vpp_obj, pstCfg2NDDMAParam->dmaId, pstCfg2NDDMAParam->plane->uchInverseScanMode, mtu);

    ovps_printf(OVPS_MID, "Addr = 0x%x, plane ID = %d, burst %d, step1 %d, size1 %d, step2 %d, size2 %d, mtr=%d, dmaid=%d\n", addr, pstCfg2NDDMAParam->plane->chanID, burst, step1, size1, step2, size2,pstCfg2NDDMAParam->mtrREn,pstCfg2NDDMAParam->dmaId);

    if (0 == pstCfg2NDDMAParam->mtrREn) {
        RetVal =  START_2NDDMA(pstCfg2NDDMAParam->dhubId, pstCfg2NDDMAParam->dmaId, addr, burst, step1, size1, step2, size2, (T64b*)pstCfg2NDDMAParam->cfgQ);
    } else {
        RetVal =  START_2NDDMA_MTRR(pstCfg2NDDMAParam->dhubId, pstCfg2NDDMAParam->dmaId, addr, burst, step1, size1, step2, size2, pstCfg2NDDMAParam->chkSemId, pstCfg2NDDMAParam->updSemId, (T64b*)pstCfg2NDDMAParam->cfgQ);
    }

    return RetVal;
}

#ifdef CONFIG_AMP_VPP_ROTATION
typedef struct OVPS_DHUB_PARAM_ROTATION_s
{
    //Input Parameters
    UINT32 m_uiSrcFtm;
    UINT32 m_uiSrcOrder;
    UINT32 m_uiBitDepth;
    UINT32 m_uiRotationAngle;   //Rotation Angle
    UINT32 m_uiPicW;    //Active Picture width
    UINT32 m_uiPicH;    //Active Picture Height
    UINT32 m_uiStride;  //Stride
    UINT32 m_uiBurst_Y;
    UINT32 m_uiBurst_UV;
    UINT32 m_uiTileW_Y;
    UINT32 m_uiTileW_UV;
    UINT32 m_uiTileH;
    //Output Paramters
    UINT32 m_uiSize_0_Y;  //Size 0
    UINT32 m_uiSize_0_UV;  //Size 0
    UINT32 m_uiStep_0;  //Step 0
    UINT32 m_uiSize_1_Y;
    UINT32 m_uiSize_1_UV;
    UINT32 m_uiStep_1_Y;
    UINT32 m_uiStep_1_UV;
    UINT32 m_uiSize_2_Y;
    UINT32 m_uiSize_2_UV;
    UINT32 m_uiStep_2_Y;
    UINT32 m_uiStep_2_UV;
    UINT32 m_uiBaseAddr_Y;  //Base Address
    UINT32 m_uiBaseAddr_UV;  //Base Address
}OVPS_DHUB_PARAM_ROTATION;


static int GetDhubParam(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, OVPS_DHUB_PARAM_ROTATION *pstDhubParam)
{

    if (NULL == pstDhubParam) {
        return -1;
    }
    if ((SRCFMT_YUV420SP_TILED_V4H8 != pstDhubParam->m_uiSrcFtm) && (SRCFMT_YUV420SP_TILED_MODE0 != pstDhubParam->m_uiSrcFtm)) {
        return 0;
    }
    switch (pstDhubParam->m_uiSrcFtm) {
        case SRCFMT_YUV420SP_TILED_V4H8:
            {
                switch (pstDhubParam->m_uiRotationAngle) {
                    case ROTATION_0D:
                        {
                            //Working values --> burst 7680, step1 7680, size1 1, step2 7680, size2 270
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);

                        }
                        break;
                    case ROTATION_90D:
                        {

                            pstDhubParam->m_uiBurst_Y = 16;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 4;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            if (pstDhubParam->m_uiPicW > 2160) {
                                pstDhubParam->m_uiPicW = pstDhubParam->m_uiPicW / 2;
                                pstDhubParam->m_uiStride = pstDhubParam->m_uiStride * 2;
                            }
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_1_Y =  (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiStep_1_UV =  (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_1_Y - 1) * pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + ( pstDhubParam->m_uiSize_1_UV - 1) * pstDhubParam->m_uiStride;
                            //Do not change it.
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileH, pstDhubParam->m_uiTileW_Y);
                        }
                        break;
                    case ROTATION_180D:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1; //2's complement
                            pstDhubParam->m_uiStep_1_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1; //2's complement
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiStride + ( pstDhubParam->m_uiSize_1_Y - 1) *  pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + (pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiStride + ( pstDhubParam->m_uiSize_1_UV - 1) *  pstDhubParam->m_uiBurst_UV;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    case ROTATION_270D:
                        {
                            pstDhubParam->m_uiBurst_Y = 16;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 4;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            if (pstDhubParam->m_uiPicW > 2160) {
                                pstDhubParam->m_uiPicW = pstDhubParam->m_uiPicW / 2;
                                pstDhubParam->m_uiStride = pstDhubParam->m_uiStride * 2;
                            }
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_2_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1; //2's complement
                            pstDhubParam->m_uiStep_2_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1; //2's complement
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + ( pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiBurst_UV;
                            //Do not change it.
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileH, pstDhubParam->m_uiTileW_Y);
                        }
                        break;
                    case ROTATION_H_FLIP:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1; //2's complement
                            pstDhubParam->m_uiStep_1_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1; //2's complement
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_1_Y - 1) * pstDhubParam->m_uiSize_0_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + ( pstDhubParam->m_uiSize_1_UV - 1) * pstDhubParam->m_uiSize_0_UV ;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    case ROTATION_V_FLIP:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + ( pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);

                        }
                        break;
                    default:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                }

            }
            break;
        case SRCFMT_YUV420SP_TILED_MODE0:
            {
                switch (pstDhubParam->m_uiRotationAngle) {
                    case ROTATION_0D:
                        {
                            //Working values --> burst 7680, step1 7680, size1 1, step2 7680, size2 270
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 6;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);

                        }
                        break;
                    case ROTATION_90D:
                        {
                            pstDhubParam->m_uiBurst_Y = 16;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 3;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            if (pstDhubParam->m_uiPicW > 2160) {
                                pstDhubParam->m_uiPicW = pstDhubParam->m_uiPicW / 2;
                                pstDhubParam->m_uiStride = pstDhubParam->m_uiStride * 2;
                            }
                            pstDhubParam->m_uiPicH = (pstDhubParam->m_uiPicH / 32) * 32;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_1_Y =  (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiStep_1_UV =  (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_1_Y - 1) * pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + ( pstDhubParam->m_uiSize_1_UV - 1) * pstDhubParam->m_uiStride;
                            //Do not change it.
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileH, pstDhubParam->m_uiTileW_Y);
                        }
                        break;
                    case ROTATION_180D:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 6;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1; //2's complement
                            pstDhubParam->m_uiStep_1_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1; //2's complement
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiStride + ( pstDhubParam->m_uiSize_1_Y - 1) *  pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + (pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiStride + ( pstDhubParam->m_uiSize_1_UV - 1) *  pstDhubParam->m_uiBurst_UV;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    case ROTATION_270D:
                        {
                            pstDhubParam->m_uiBurst_Y = 16;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 3;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiPicH = (pstDhubParam->m_uiPicH / 32) * 32;
                            if (pstDhubParam->m_uiPicW > 2160) {
                                pstDhubParam->m_uiPicW = pstDhubParam->m_uiPicW / 2;
                                pstDhubParam->m_uiStride = pstDhubParam->m_uiStride * 2;
                            }
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_2_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1; //2's complement
                            pstDhubParam->m_uiStep_2_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1; //2's complement
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + ( pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiBurst_UV;
                            //Do not change it.
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileH, pstDhubParam->m_uiTileW_Y);
                        }
                        break;
                    case ROTATION_H_FLIP:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 6;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = (~(pstDhubParam->m_uiBurst_Y)) + 1; //2's complement
                            pstDhubParam->m_uiStep_1_UV = (~(pstDhubParam->m_uiBurst_UV)) + 1; //2's complement
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_1_Y - 1) * pstDhubParam->m_uiSize_0_Y ;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + ( pstDhubParam->m_uiSize_1_UV - 1) * pstDhubParam->m_uiSize_0_UV ;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                    case ROTATION_V_FLIP:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 6;
                            pstDhubParam->m_uiTileW_UV = 6;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = (~(pstDhubParam->m_uiStride)) + 1;   //2's complement of stride
                            pstDhubParam->m_uiBaseAddr_Y = pstDhubParam->m_uiBaseAddr_Y + ( pstDhubParam->m_uiSize_2_Y - 1) * pstDhubParam->m_uiStride;
                            pstDhubParam->m_uiBaseAddr_UV = pstDhubParam->m_uiBaseAddr_UV + ( pstDhubParam->m_uiSize_2_UV - 1) * pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);

                        }
                        break;
                    default:
                        {
                            pstDhubParam->m_uiBurst_Y = 32;
                            pstDhubParam->m_uiBurst_UV = 32;
                            pstDhubParam->m_uiTileW_Y = 8;
                            pstDhubParam->m_uiTileW_UV = 8;
                            pstDhubParam->m_uiTileH = 4;
                            pstDhubParam->m_uiSize_0_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiSize_0_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiStep_0 = 1;
                            pstDhubParam->m_uiSize_1_Y = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_Y - 1) / pstDhubParam->m_uiTileW_Y;
                            pstDhubParam->m_uiSize_1_UV = (pstDhubParam->m_uiPicW + pstDhubParam->m_uiTileW_UV - 1) / pstDhubParam->m_uiTileW_UV;
                            pstDhubParam->m_uiStep_1_Y = pstDhubParam->m_uiBurst_Y;
                            pstDhubParam->m_uiStep_1_UV = pstDhubParam->m_uiBurst_UV;
                            pstDhubParam->m_uiSize_2_Y = (pstDhubParam->m_uiPicH + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiSize_2_UV = (pstDhubParam->m_uiPicH/2 + pstDhubParam->m_uiTileH - 1) / pstDhubParam->m_uiTileH;
                            pstDhubParam->m_uiStep_2_Y = pstDhubParam->m_uiStep_2_UV = pstDhubParam->m_uiStride;
                            FE_DLR_ConfigTilePackSel(vpp_obj, uiPlaneId, pstDhubParam->m_uiTileW_Y, pstDhubParam->m_uiTileH);
                        }
                        break;
                }
            }
        default:
            break;
    }
    return 0;
}

static int GetRotationAngle(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, UINT32 *puiRotationAngle)
{
    UINT32 uiRet = MV_VPP_OK;
    PLANE *plane = NULL;
    UINT32 uiRotationAngle = 0;

    if (NULL == puiRotationAngle) {
        return MV_VPP_EBADPARAM;
    }
    plane = &(vpp_obj->plane[uiPlaneId]);

    uiRotationAngle = plane->uchRotationAngle;

    if (ROTATION_0D == plane->uchRotationAngle) {
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
        switch (plane->uchInverseScanMode) {
            case VPP_SCAN_INVERSE_NORMAL:
                uiRotationAngle = ROTATION_0D;
                break;
            case VPP_SCAN_INVERSE_H:
                uiRotationAngle = ROTATION_H_FLIP;
                break;
            case VPP_SCAN_INVERSE_V:
                uiRotationAngle = ROTATION_V_FLIP;
                break;
            case VPP_SCAN_INVERSE_HV:
                uiRotationAngle = ROTATION_180D;
                break;
            default:
                uiRotationAngle = ROTATION_0D;
                break;
        }
#endif
    } else {
        uiRotationAngle = plane->uchRotationAngle;
    }

    *puiRotationAngle = uiRotationAngle;

    return uiRet;
}
#endif

/* This funcion is used to configure Decompression based on input frame */
static int vppConfigDecompression(VPP_OBJ *vpp_obj, VBUF_INFO *pinfo, PLANE *plane)
{
    INT planeID;

    planeID = PLANE_OF_CHAN(vpp_obj, plane->chanID);

    if (pinfo->m_is_compressed != plane->m_mtrR__en) {
        plane->m_mtrR__en = pinfo->m_is_compressed;
        if (plane->m_mtrR__en) {
            plane->m_mtrR_updSemId_Y  |= 0x1; /* Compression mode */
            plane->m_mtrR_updSemId_UV |= 0x1; /* Compression mode */
        } else {
            plane->m_mtrR_updSemId_Y  &= (~0x1); /* Bypass mode */
            plane->m_mtrR_updSemId_UV &= (~0x1); /* Bypass mode */
        }
    }

    if (1 == plane->m_mtrR__en) {
        plane->m_mtrR_updSemId_Y  ^= 2; /* Needs to toggle 2nd bit for every new frame */
        plane->m_mtrR_updSemId_UV ^= 2; /* Needs to toggle 2nd bit for every new frame */
        ovpsMtrR_ConfigStride(&plane->m_mtrR_updSemId_Y, pinfo->m_buf_stride);
        ovpsMtrR_ConfigStride(&plane->m_mtrR_updSemId_UV, pinfo->m_buf_stride_UV);
    }

    VPP_AVIOMTR_UpdateDesc(vpp_obj, VPP_AVIOMTR_CONFIG_OVPS, pinfo, planeID);

    return MV_VPP_OK;
}

#ifdef CONFIG_AMP_VPP_ROTATION
#define VPP_FE_DUMMY_TG_SIZE_H_OFF_P 60
#else
#define VPP_FE_DUMMY_TG_SIZE_H_OFF_P 20
#endif
#define VPP_FE_DUMMY_TG_SIZE_V_OFF_P 6
#define VPP_FE_DUMMY_TG_IHRES_OFF 46
#define VPP_FE_DUMMY_TG_HB_FE_OFF 8 //VPP_FE_DUMMY_TG_HB_BE + 1
#define VPP_FE_DUMMY_TG_HB_BE     7 //VPP_FE_DUMMY_TG_HS_BE + 4
#define VPP_FE_DUMMY_TG_VB0_FE_OFF 3//VPP_FE_DUMMY_TG_VS0_BE + 1
#define VPP_FE_DUMMY_TG_VB0_BE     2 //VPP_FE_DUMMY_TG_VS0_BE
#define VPP_FE_DUMMY_TG_SIZE_H_OFF_I 26
#define VPP_FE_DUMMY_TG_SIZE_V_OFF_I 10

int OVPSCL_ISR_SetELTGParams(VPP_OBJ *vpp_obj, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes, int progressive)
{
    UINT32 RegAddr;
    UINT32 RegVal;
    INT RetVal = MV_VPP_OK;

    INT Crop1_Start_X = 0, Crop1_End_X = 0, Crop1_Start_Y = 0, Crop1_End_Y = 0;

    UINT32 leftcropy = 0,rightcropy = 0,topcropy = 0,bottomcropy = 0;
    UINT32 leftcropc = 0,rightcropc = 0,topcropc = 0,bottomcropc = 0;

    leftcropy = vpp_obj->ovp_obj.plane.m_leftpadadjust_y;
    rightcropy = vpp_obj->ovp_obj.plane.m_rightpad_y;
    topcropy = vpp_obj->ovp_obj.plane.m_toppadadjust_y;
    bottomcropy = vpp_obj->ovp_obj.plane.m_bottompad_y;

    leftcropc = vpp_obj->ovp_obj.plane.m_leftpadadjust_c;
    rightcropc = vpp_obj->ovp_obj.plane.m_rightpad_c;
    topcropc = vpp_obj->ovp_obj.plane.m_toppadadjust_c;
    bottomcropc = vpp_obj->ovp_obj.plane.m_bottompad_c;

    ovps_printf(OVPS_MID, "in EL TG set %d %d %d %d %d %d %d %d\n",leftcropy,leftcropc,rightcropy, rightcropc, topcropy, topcropc, bottomcropy, bottomcropc);
    /* Update the 420_422_cfg registers for getting black pixel padding */
    if (vpp_obj->ovp_obj.plane.m_SampleCropEnable) {
        pTotRes->HRes = vpp_obj->ovp_obj.plane.m_Aligned_Luma_Width;
        pTotRes->VRes = vpp_obj->ovp_obj.plane.m_Aligned_Luma_Height;

        /* FETG Cropping for compressed inputs will be done in two stages : CROP1 & CROP2*/
        /* CROP1 will program the HB, HB_OUT VB,VB_OUT registers */
        /* CROP2 will program the HB_Y,VB_Y,HB_C,VB_C registers */

        /* ###### CROP 1 ###### */
        /* STARTX = max(leftcropy,leftcropc) ENDX = IHRES-min(rightcropy,rightcropc) */
        Crop1_Start_X = (leftcropy > leftcropc)?leftcropy:leftcropc;
        Crop1_End_X = (rightcropy < rightcropc)?(pTotRes->HRes-rightcropy):(pTotRes->HRes-rightcropc);

        Crop1_Start_Y = topcropy;
        Crop1_End_Y = pTotRes->VRes - bottomcropy;

        /* FETG MAIN SIZE register programming */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl +  RA_vidpl_EL_PIPE + RA_EL_PIPE_feTG_EL;
        if (progressive) {
            RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_IHRES_OFF + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
        } else {
            RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
        }
        /* write TG size to BCM buffer */
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
            return RetVal;

        /* calculate the H blank front edge value */
        RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);
        /*write the data to BCM buffer*/
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
            return RetVal;

        /* calculate the V blank front edge value */
        RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);
        /*writer the data to BCM buffer */
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
            return RetVal;

        /* Program the HB_Y,VB0_Y,HB_C,VB0_C registers */
        RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
        /* write the data to BCM buffer */
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
            return RetVal;

        RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
        /* write the data to BCM buffer */
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
            return RetVal;

        RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
        /* write the data to BCM buffer */
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
            return RetVal;

        RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
        /* write the data to BCM buffer */
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
            return RetVal;

        /* Program the HB_OUT & VB0_OUT registers */
        RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
        /* write the data to BCM buffer */
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
            return RetVal;

        RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
        /* write the data to BCM buffer */
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
            return RetVal;

    } else { /* Normal cropping case */

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_feTG_EL;

        if (progressive) {
            RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
        } else {
            RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
        }

        /*write TG size to BCM buffer*/
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
            return RetVal;

        /*calculate the H blank front edge value*/
        RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

        /*write the data to BCM buffer*/
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
            return RetVal;

        /*calculate the V blank front edge value*/
        RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);

        /*writer the data to BCM buffer*/
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
            return RetVal;


        RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_FE_DUMMY_TG_HB_FE_OFF + (((pCropRes->leftCrop)+VPP_FE_DUMMY_TG_HB_BE) << 16);

        /*write the data to BCM buffer*/
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
            return RetVal;
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
            return RetVal;

        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
            return RetVal;

        RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((pCropRes->topCrop+VPP_FE_DUMMY_TG_VB0_BE) << 16);

        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
            return RetVal;
        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
            return RetVal;

        if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
            return RetVal;
    }
    return RetVal;

}

INT OVPSCL_ISR_SetDummyTG_RasterScan(VPP_OBJ *vpp_obj, INT ldr_id,
        PVPP_FE_DLR_INPUT_RES pTotRes,
        PVPP_FE_DLR_INPUT_CROP pCropRes,
        int progressive)
{
    UINT32 RegAddr;
    UINT32 RegVal;
    INT RetVal = MV_VPP_OK;
    if (!OVPS_PARAMS->readTiledfromOVPclient) {
        T32RASTERSCAN_RD_CTRL1 stRdCtrl1;

        stRdCtrl1.u32 = vpp_obj->uiRasterScanRdCtrl1;
        stRdCtrl1.uCTRL1_crop_de_en = 1;
        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_CTRL1, stRdCtrl1.u32);
        vpp_obj->uiRasterScanRdCtrl1 = stRdCtrl1.u32;
    }
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_rsFeTG;

    if (progressive) {
        RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
    } else {
        RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
    }

    /*write TG size to BCM buffer*/
    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);

    /*writer the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
        return RetVal;

    RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_FE_DUMMY_TG_HB_FE_OFF + (((pCropRes->leftCrop)+VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
        return RetVal;
    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
        return RetVal;

    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
        return RetVal;

    RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((pCropRes->topCrop+VPP_FE_DUMMY_TG_VB0_BE) << 16);

    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
        return RetVal;
    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
        return RetVal;

    if (MV_VPP_OK != (RetVal = OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
        return RetVal;
    return RetVal;
}

void OVPSCL_ISR_SetTGParams(VPP_OBJ *vpp_obj, UINT32 width, UINT32 height, UINT32 luma)
{
    T32TG_SIZE tg_size;
    T32TG_HB tg_hb;
    T32TG_HB_CR tg_hb_cr;
    T32TG_HB_CR2 tg_hb_cr2;
    T32TG_VB0 tg_vb0;
    T32TG_VB0_CR tg_vb0_cr;
    T32TG_VB0_CR2 tg_vb0_cr2;
    UINT32 uiRegAddr;

    if (luma == 1)
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_OTG_Y;
    else
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_OTG_UV;
    tg_size.uSIZE_Y = height+6;
    tg_size.uSIZE_X = width+40; /* Changed from 20 to 40 to fix 4K SCL issues */
    tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = width+8;
    tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
    tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = height+3;
    tg_vb0.uVB0_BE = tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 2;
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf,(uiRegAddr + RA_TG_SIZE), tg_size.u32);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf,(uiRegAddr + RA_TG_HB), tg_hb.u32);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf,(uiRegAddr + RA_TG_HB_CR), tg_hb_cr.u32);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf,(uiRegAddr + RA_TG_HB_CR2), tg_hb_cr2.u32);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf,(uiRegAddr + RA_TG_VB0), tg_vb0.u32);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf,(uiRegAddr + RA_TG_VB0_CR), tg_vb0_cr.u32);
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf,(uiRegAddr + RA_TG_VB0_CR2), tg_vb0_cr2.u32);
}

void OVPSCL_ISR_SetSclParams(VPP_OBJ *vpp_obj, VPP_WIN* inp_source, VPP_WIN* ovp_output, INT32 eVppFrcSclNum)
{
#ifdef VPP_OVPS_EN_UV_PACK_SEL
    if (VPP_FRC_SCL_OVP_UV == eVppFrcSclNum) {
        vpp_obj->scl_res[eVppFrcSclNum].IHRes = inp_source->width/2;
        vpp_obj->scl_res[eVppFrcSclNum].OHRes = ovp_output->width/2;
    } else
#endif
    {
        vpp_obj->scl_res[eVppFrcSclNum].IHRes = inp_source->width;
        vpp_obj->scl_res[eVppFrcSclNum].OHRes = ovp_output->width;
    }

    if (eVppFrcSclNum == VPP_FRC_SCL_OVP_Y) {
        ovps_printf(OVPS_MID, "%d: in:%dx%d,out:%dx%d\n",eVppFrcSclNum,inp_source->width,inp_source->height,ovp_output->width,ovp_output->height);
        vpp_obj->scl_res[eVppFrcSclNum].IVRes = inp_source->height;
        vpp_obj->scl_res[eVppFrcSclNum].OVRes = ovp_output->height;
    } else {
        vpp_obj->scl_res[eVppFrcSclNum].IVRes = inp_source->height/2;
        vpp_obj->scl_res[eVppFrcSclNum].OVRes = ovp_output->height/2;
    }

    vpp_obj->scl_ctrl[eVppFrcSclNum].HScalePos = VPP_SCL_HSCALE_AUTO;
    vpp_obj->scl_ctrl[eVppFrcSclNum].InputClr = 0;
    vpp_obj->scl_ctrl[eVppFrcSclNum].OsdInput = 0;
    vpp_obj->scl_ctrl[eVppFrcSclNum].NLEn = 0;
    vpp_obj->scl_ctrl[eVppFrcSclNum].CenterFrac = 0;
    vpp_obj->scl_ctrl[eVppFrcSclNum].CenterRatio = 0;
    vpp_obj->scl_ctrl[eVppFrcSclNum].I565 = 0;
    vpp_obj->scl_ctrl[eVppFrcSclNum].HTapNum = 12;
    vpp_obj->scl_ctrl[eVppFrcSclNum].VTapNum = 8;
    vpp_obj->scl_ctrl[eVppFrcSclNum].DynamicLoad = 1; /* dynamically update coefficient table */
    vpp_obj->scl_ctrl[eVppFrcSclNum].ForceSel = vpp_obj->scl_force_sel[eVppFrcSclNum];

    /* In offline case VIP SCL will take care of cropping. So no need to do cropping on MAIN SCL */
    vpp_obj->scl_ctrl[eVppFrcSclNum].leftCrop  = 0;
    vpp_obj->scl_ctrl[eVppFrcSclNum].rightCrop = 0;

    OVPSCL_FRC_SCL_SetSclCtrlParams(vpp_obj, eVppFrcSclNum, &vpp_obj->scl_res[eVppFrcSclNum], &vpp_obj->scl_ctrl[eVppFrcSclNum]);
    OVPSCL_FRC_SCL_SetFrcParams(vpp_obj, eVppFrcSclNum, &vpp_obj->scl_res[eVppFrcSclNum]);
}

/***************************************************************************************
 * FUNCTION: Get bits per pixel with given inputs.
 *
 * RETURN:  xx - Total number of bits per pixel (succeed)
 *          0 - Bad paramters.
 ***************************************************************************************/
INT OVPS_GetBpp(VPP_OBJ *vpp_obj, PLANE *plane)
{
   UINT8 uchDataFmt = 0;   //Total number of bits per pixel

   if (8 ==  plane->bits_per_pixel) {
        switch (plane->srcfmt) {
            case SRCFMT_YUV420SP:
                uchDataFmt = 8;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_8BIT_420_SP;
                break;
            case SRCFMT_YUV422:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_422_YC;
                break;
            case SRCFMT_ARGB32:
            case SRCFMT_ARGB32_PM:
                uchDataFmt = 32;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_32BIT_ARGB;
                break;

            case SRCFMT_ARBG2101010:
                uchDataFmt = 32;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_32BIT_ARGB;
                break;

            case SRCFMT_ARGB4444:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_ARGB4444;
                break;

            case SRCFMT_RGB565:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_RGB565;
                break;

            case SRCFMT_ARGB1555:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_ARGB1555;
                break;

            case SRCFMT_ARGB8332:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_ARGB8332;
                break;

            case SRCFMT_YUV420SP_TILED_MODE0:
                uchDataFmt = 8;
                plane->tile.width = 8;
                plane->tile.height = 4;
                plane->tile.bytesPerTile = plane->tile.width*plane->tile.height;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE0;
                break;
            case SRCFMT_YUV420SP_TILED_MODE1:
                uchDataFmt = 8;
                plane->tile.width = 4;
                plane->tile.height = 4;
                plane->tile.bytesPerTile = plane->tile.width*plane->tile.height;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_MODE1;
                break;
#ifdef CONFIG_AMP_VPP_ROTATION
            case SRCFMT_YUV420SP_TILED_V4H8:
                uchDataFmt = 8;
                plane->tile.width = 8;
                plane->tile.height = 4;
                plane->tile.bytesPerTile = plane->tile.width*plane->tile.height;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_8BIT_420_SP_TILED_V4H8;
                break;
#endif

            default:
                uchDataFmt = 16;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_422_YC;
                break;
        }
    } else if (10 ==  plane->bits_per_pixel) {
        switch (plane->srcfmt) {
            case SRCFMT_YUV420SP:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_420_SP;
                break;
            case SRCFMT_YUV422:
                uchDataFmt = 20;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_20BIT_422_YC;
                break;
            case SRCFMT_YUV420SP_TILED_MODE0:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE0;
                break;
            case SRCFMT_YUV420SP_TILED_MODE1:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE1;
                break;
            case SRCFMT_YUV420SP_DWA:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_DWA_420_SP;
                break;
            case SRCFMT_YUV422SP_DWA:
                uchDataFmt = 10;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_DWA_422_SP;
                break;
            case SRCFMT_YUV444_DWA:
                uchDataFmt = 32;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_10BIT_DWA_444;
                break;
            default:
                uchDataFmt = 20;
                plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_20BIT_422_YC;
                break;
        }
    } else if (16 ==  plane->bits_per_pixel) {
        uchDataFmt = 16;
        plane->uchClientMode = VPP_UNIQUE_FORMAT_ID_16BIT_420_SP;
    } else {
       VDB_print("iBitsPerPixel is wrong : %d \n", plane->bits_per_pixel);
    }

    return uchDataFmt;
}

/********************************************************************************
 * FUNCTION: initialize OVPS
 * PARAMS: *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EBADPARAM - invalid parameter
 *         MV_VPP_EUNCONFIG - plane not activated yet
 *******************************************************************************/
int OVPSCL_ISR_ConfigDefaultPipe(VPP_OBJ *vpp_obj)
{
    UINT32 RegAddr;
    T32OVPSCLWRAP_CTRL5 stOvpSclWrapCtrl5;
    T32OVPSCLWRAP_CFG8 stOvpSclWrapCfg8;
    T32OVPSCLWRAP_CFGOCSC0 stOvpSclWrapCfgOcsc0;
    T32RASTERSCAN_RD_CTRL2 stRasterScanRdCtrl2;
    T32avioGbl_BLANK_CG1 stAvioGblBlankCg1;

    RegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_BLANK_CG1;
    GA_REG_WORD32_READ(RegAddr, &stAvioGblBlankCg1.u32);
    stAvioGblBlankCg1.uBLANK_CG_gfx0_DV_blank_cg_en_reg           =  1;
    stAvioGblBlankCg1.uBLANK_CG_ovpScl_blank_cg_mode              =  0;
    stAvioGblBlankCg1.uBLANK_CG_ovp_vpp_blank_cg_mode             =  0;
    stAvioGblBlankCg1.uBLANK_CG_hdmitx_en                         =  1;
    stAvioGblBlankCg1.uBLANK_CG_dsitx_en                          =  1;
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf,
            RegAddr, stAvioGblBlankCg1.u32);

#ifdef RA_avioGbl_BLANK_CG
    RegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_BLANK_CG;
    vpp_obj->avioGbl_blank_cg.uBLANK_CG_ovpScl_blank_cg_en_reg    = 0;
    vpp_obj->avioGbl_blank_cg.uBLANK_CG_ovp_cg_en   = 0;
    vpp_obj->avioGbl_blank_cg.uBLANK_CG_ovpScl_blank_cg_en_reg   = 0;
    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr,
            vpp_obj->avioGbl_blank_cg.u32);
#endif

    RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL5;
    OVPS_BCMBUF_vidpl_Read(RegAddr, &stOvpSclWrapCtrl5.u32);
    stOvpSclWrapCtrl5.uCTRL5_dither_bypass = 1;
    OVPS_BCMBUF_vidpl_Write(RegAddr,stOvpSclWrapCtrl5.u32);

    RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CFG8;
    OVPS_BCMBUF_vidpl_Read(RegAddr, &stOvpSclWrapCfg8.u32);
    stOvpSclWrapCfg8.uCFG8_UV_swap_regs = 1;
    stOvpSclWrapCfg8.uCFG8_sclOutrdy_sts0_en = 1;
    stOvpSclWrapCfg8.uCFG8_sclOutrdy_sts1_en = 1;
    stOvpSclWrapCfg8.uCFG8_dither_bypass = 1;
    stOvpSclWrapCfg8.uCFG8_csc_bypass = 1;
    stOvpSclWrapCfg8.uCFG8_sclOfifo_Y_stsCtrl = 0;
    stOvpSclWrapCfg8.uCFG8_sclOfifo_UV_stsCtrl = 0;
#ifdef VPP_OVPS_EN_UV_PACK_SEL
    stOvpSclWrapCfg8.uCFG8_UVPack_sel = 1;
#else
    stOvpSclWrapCfg8.uCFG8_UVPack_sel = 0;
#endif
    OVPS_BCMBUF_vidpl_Write(RegAddr, stOvpSclWrapCfg8.u32);

    RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CFGOCSC0;
    OVPS_BCMBUF_vidpl_Read(RegAddr, &stOvpSclWrapCfgOcsc0.u32);
    stOvpSclWrapCfgOcsc0.uCFGOCSC0_DB_ocsc_bypass = 1;
    stOvpSclWrapCfgOcsc0.uCFGOCSC0_DB_dither_bypass = 1;
    OVPS_BCMBUF_vidpl_Write(RegAddr, stOvpSclWrapCfgOcsc0.u32);

    RegAddr = RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_CTRL2;
    OVPS_BCMBUF_vidpl_Read(RegAddr, &stRasterScanRdCtrl2.u32);
    stRasterScanRdCtrl2.uCTRL2_hdeUV_mask_en = 1;
    stRasterScanRdCtrl2.uCTRL2_hde_mask_initval1 = 1;
    OVPS_BCMBUF_vidpl_Write(RegAddr, stRasterScanRdCtrl2.u32);
    return MV_VPP_OK;

}
/********************************************************************************
 * FUNCTION: start all active plane data loader of a VPP FE channel
 * PARAMS: *vpp_obj - pointer to VPP object
 *         chanID - channel
 * RETURN: MV_VPP_OK - succeed
 *         MV_VPP_EBADPARAM - invalid parameter
 *         MV_VPP_EUNCONFIG - plane not activated yet
 *******************************************************************************/
int OVPSCL_ISR_startChannelDataLoader(VPP_OBJ *vpp_obj, int chanID)
{
    int planeID;
    int crop_actv_offset = 0, crop_actv_offset_UV = 0;
    int CropWpl, PixelPerBeat;
    int crop_recalculation_required;
    int data_loader_update_required;
    int data_loader_crop_required;
    CHAN *chan;
    PLANE *plane;
    VDEC_VID_BUF_DESC *inline_frame;
    VBUF_INFO *pinfo;
    VPP_WIN crop_actv_win, crop_scl_actv_win;
    VPP_FE_DLR_INPUT_RES res;
    VPP_VP_CLIENT_RES client_res;
    VPP_FE_DLR_INPUT_RES feRes;
    VPP_FE_DLR_INPUT_RES cropRes;
    OVPS_DHUB_CFG_2NDDMA_PARAM stCfg2NDDMAParam;
    UNSG32 uiPackSel = 0;
    RESOLUTION_INFO  *pResinfo;

#ifdef VPP_DEBUG
    int dhub_status;
#endif

    int *cfgQ, *cfgQ_shadow;
    int *cfgQ_len;

    /*If stride is already multipled by a factor then no need to multiple stride otherwise apply factor
     *Note: Whenever linedropping is enabled in tiled mode, Y Alignment should be 4 (hgt of a tile)
     *      then ensure Ensure tiledModeStrideFactor & tiledModeWidHgtFactor are 4 or multiple of 4*/
    int tiledModeStrideFactor=1;
    int tiledModeWidHgtFactor=1;
    unsigned int uiCromaAllignedLinedropFactor = 1;

    UINT32 RegAddr;

    chan = &vpp_obj->ovp_obj.chan;

    cfgQ_shadow = vpp_obj->ovp_obj.curr_ovpscl_dma_cfgQ->addr;
    cfgQ_len = &(vpp_obj->ovp_obj.curr_ovpscl_dma_cfgQ->len);

    /******************************************************/
    /* 1. get frame descriptor and crop display content   */
    /******************************************************/
    /* get the right inline frame to start DMA */
    planeID = PLANE_OF_CHAN(vpp_obj, chanID);
    plane = &vpp_obj->ovp_obj.plane;

    inline_frame = plane->curr_frame;

    if (planeID == PLANE_OVP_EL) {
        if (!((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER)
                    || (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_TILE))) {
            return (MV_VPP_OK);
        }
    }

    crop_recalculation_required = 0;
    data_loader_update_required = 0;
    data_loader_crop_required = 0;
    plane->m_SeparateElcropRequired = 0;
    plane->sclModeChanged = 0;
    plane->m_dolby_ref_win_updated = 0;
    plane->dolbyLineDropEn = 0;
    plane->m_ref_win_update_pan_scan = 0;
#ifndef CONFIG_AMP_TEE_ENABLE
    /* parse inline frame and start inline DMA for it */
    pinfo = (VBUF_INFO *)inline_frame->user_data;
#else
    pinfo = (VBUF_INFO *)inline_frame;
#endif
#ifdef CONFIG_AMP_VPP_ROTATION
    if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt)
#else
    if (pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
    {
        tiledModeStrideFactor=VPP_TILED_MODE_STRIDE_FACTOR==1?4:1;
        tiledModeWidHgtFactor=1*4;
    }

#ifdef ENABLE_REFWIN_FOR_FPGA_TESTING
    //FIXME:graphics plane need not calculate the display offset; instead send x,y through pinfo->m_active_left, pinfo->m_active_top
    //FIXME:Note application can set its own left per frame in pinfo->m_active_left, pinfo->m_active_top and they are also expected to be -ve number
    pinfo->m_active_left = -plane->ref_win.x;
    pinfo->m_active_top  = -plane->ref_win.y;
#endif //ENABLE_REFWIN_FOR_FPGA_TESTING

    /* Configure the DHUB for compressed inputs */
    vppConfigDecompression(vpp_obj, pinfo, plane);

    pResinfo = VPP_GetResInfo(vpp_obj->ovp_obj.output_res);

    ovps_printf(OVPS_INFO, "%s : %d\n",__func__,__LINE__);
    ovps_printf(OVPS_INFO, "SRC FRM(PLANE) Details planeID = %d, srcfmt  = %d, \
            order = %d,bits_per_pixel = %d,m_leftpad_y = %d,m_toppad_y = %d,   \
            m_leftpad_c = %d,m_toppad_c = %d,bpp = %d\n",
            planeID,plane->srcfmt, plane->order, plane->bits_per_pixel,
            plane->m_leftpad_y, plane->m_toppad_y, plane->m_leftpad_c,
            plane->m_toppad_c,plane->bpp );
    ovps_printf(OVPS_INFO, "SRC FRM(pinfo) Details planeID = %d, srcfmt = %d,  \
            order = %d,bits_per_pixel = %d,m_leftpad_y = %d,m_toppad_y = %d,  \
            m_leftpad_c = %d,m_toppad_c = %d\n",
            planeID,pinfo->m_srcfmt, pinfo->m_order, pinfo->m_bits_per_pixel,
            pinfo->m_luma_left_ofst, pinfo->m_luma_top_ofst,
            pinfo->m_chroma_left_ofst,  pinfo->m_chroma_top_ofst);
    ovps_printf(OVPS_INFO, "plane->frame_type = %d,flag_3d = %d,plane->field= %d\n",
            plane->frame_type, pResinfo->flag_3d,
            plane->field);

    /* no VPP settting update in interlaced bottom field input */

    if ((plane->frame_type != 0)) {
        data_loader_update_required = 1;
        /* For compressed inputs check the padding and update data loader */
        if ((pinfo->m_luma_left_ofst != plane->m_leftpad_y)||
                (pinfo->m_luma_top_ofst != plane->m_toppad_y)||
                (pinfo->m_chroma_left_ofst != plane->m_leftpad_c)||
                (pinfo->m_chroma_top_ofst != plane->m_toppad_c)) {
            data_loader_update_required = 1;
            crop_recalculation_required = 1;
            if (((SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt) ||
                        ((pinfo->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0) &&
                         (10 == pinfo->m_bits_per_pixel))) &&
                    (ROTATION_0D == plane->uchRotationAngle)) {
                if (IS_MTR_ENABLED(plane->m_mtrR__en)) {
                    if (pinfo->m_luma_left_ofst != 0 || pinfo->m_chroma_left_ofst != 0) {
                        plane->m_SampleCropEnable = 1;
                    }
                } else {
                    plane->m_SampleCropEnable = 0;
                    plane->m_rightpad_y = 0;
                    plane->m_bottompad_y = 0;
                    plane->m_rightpad_c = 0;
                    plane->m_bottompad_c = 0;

                }
            } else {
                plane->m_SampleCropEnable = 0;
                plane->m_leftpad_y = 0;
                plane->m_leftpadadjust_y = 0;
                plane->m_toppad_y = 0;
                plane->m_toppadadjust_y = 0;
                plane->m_leftpad_c = 0;
                plane->m_leftpadadjust_c = 0;
                plane->m_toppad_c = 0;
                plane->m_toppadadjust_c = 0;
                plane->m_rightpad_y = 0;
                plane->m_bottompad_y = 0;
                plane->m_rightpad_c = 0;
                plane->m_bottompad_c = 0;
                plane->m_Aligned_Luma_Width = 0;
                plane->m_Aligned_Luma_Height = 0;
                plane->m_Aligned_Chroma_Width = 0;
                plane->m_Aligned_Chroma_Height = 0;

            }
        }
        /* update current data format in vpp structure */
        plane->srcfmt = pinfo->m_srcfmt;
        plane->order = pinfo->m_order;
        plane->bits_per_pixel = pinfo->m_bits_per_pixel;
        plane->m_leftpad_y = pinfo->m_luma_left_ofst;
        plane->m_toppad_y = pinfo->m_luma_top_ofst;
        plane->m_leftpad_c = pinfo->m_chroma_left_ofst;
        plane->m_toppad_c = pinfo->m_chroma_top_ofst;
        //invoke getBPP to get alignment details for cropping
        plane->bpp = OVPS_GetBpp(vpp_obj, plane);
        plane->tile.tileStartFrac = 0;

        //Try to trigger the delayed GetClkSignature for format change
        VPP_ClkGate_triggerDelayOperation(vpp_obj, VPP_CLKGATE_DLY_OPT_SRCFMT);
        if (planeID == PLANE_OVP_EL) {

            OVP_FE_DLR_ConfigElPipe(vpp_obj, plane->srcfmt, plane->order, plane->bpp, 1);

#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
            plane->uchInverseScanModeUpdated = 1;
#endif

#ifdef CONFIG_AMP_VPP_ROTATION
            if (SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt || ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) && (10 == pinfo->m_bits_per_pixel))) {
                //Configure LUT
            }
#endif
            if (!OVPS_PARAMS->readTiledfromOVPclient) {
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
                if ((plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_V4H8) &&
                            (8 == pinfo->m_bits_per_pixel)) {
                    uiPackSel = 1;
                } else if ((plane->srcfmt == SRCFMT_YUV420SP) &&
                        (10 == pinfo->m_bits_per_pixel)) {
                    uiPackSel = 2;
                } else if ((plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0) &&
                        (10 == pinfo->m_bits_per_pixel)) {
                    uiPackSel = 3;
                } else {
                    uiPackSel = 1;
                    ovps_printf(OVPS_ERR, "unhandled color format..!!!\n");
                }
                OVPS_BCMBUF_vidpl_Write(RA_vidpl_VPTOP_CTRL +
                        RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_pack, uiPackSel);
                OVPS_BCMBUF_vidpl_Write(RA_vidpl_VPTOP_CTRL +
                        RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_pack, uiPackSel);
#endif
            } else {
                if ((plane->srcfmt == SRCFMT_YUV420SP ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_V4H8) &&
                            (8 == pinfo->m_bits_per_pixel)) {
                    uiPackSel = 0;
                } else if ((plane->srcfmt == SRCFMT_YUV420SP) &&
                        (10 == pinfo->m_bits_per_pixel)) {
                    uiPackSel = 1;
                } else if ((plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0) &&
                        (10 == pinfo->m_bits_per_pixel)) {
                    uiPackSel = 2;
                } else {
                    uiPackSel = 0;
                    ovps_printf(OVPS_ERR, "unhandled color format..!!!\n");
                }
                OVPS_PARAMS->ovpsclwrap_ctrl1.uCTRL1_packSel_R0 = uiPackSel;
                OVPS_PARAMS->ovpsclwrap_ctrl2.uCTRL2_packSel_R1 = uiPackSel;
                OVPS_BCMBUF_vidpl_Write(RA_vidpl_OVPSCLWRAP +
                        RA_OVPSCLWRAP_CTRL1, OVPS_PARAMS->ovpsclwrap_ctrl1.u32);
                OVPS_BCMBUF_vidpl_Write(RA_vidpl_OVPSCLWRAP +
                        RA_OVPSCLWRAP_CTRL2, OVPS_PARAMS->ovpsclwrap_ctrl2.u32);
            }

        }
        /* display content format changed, recalculate cropping */
        crop_recalculation_required = 1;

        if ((pinfo->m_disp_offset + pinfo->m_content_offset) != plane->disp_offset) {
            plane->disp_offset = (pinfo->m_disp_offset + pinfo->m_content_offset);
            /* display content location & size changed, recalculate cropping */
            crop_recalculation_required = 1;
        }

        /* check whether reference window is changed */
        if (plane->ref_win_updated) {
            VDBG_IF_DISP_ENABLE(1);
            //Try to trigger the delayed GetClkSignature for ref/disp win
            VPP_ClkGate_triggerDelayOperation(vpp_obj, VPP_CLKGATE_DLY_OPT_REF_DISP_WIN);
            plane->ref_win_updated = 0;

            if (IS_MTR_ENABLED(plane->m_mtrR__en)&&(plane->m_SampleCropEnable == 0)) {
                if (pinfo->m_chroma_left_ofst != 0 || pinfo->m_chroma_left_ofst != 0) {
                    plane->m_SampleCropEnable = 1;
                }
            }

            /* reference window changed, recalculate cropping */
            crop_recalculation_required = 1;
            /*Let us update loder each time  refwin is changed */
            data_loader_update_required = 1;
        }

        /* check whether display window is changed */
        //Try to trigger the delayed GetClkSignature for ref/disp win
        /* display window changed, recalculate cropping */
        crop_recalculation_required = 1;
        /*Any change in crop calculation may require to change CROP control register in FE/SCL*/
        data_loader_update_required = 1;

        chan->disp_win_final = OVPS_PARAMS->ovp_win;

        ovps_printf(OVPS_INFO,"CROP CALCULATION Done , data_loader_update_required : %d \n",data_loader_update_required);

        if (crop_recalculation_required) {

            int dataAlign_x = VPP_OVL_getDataAlignBytes_X(plane->uchClientMode, pinfo->m_content_width);
            int dataAlign_y = VPP_OVL_getDataAlignBytes_Y(plane->uchClientMode, pinfo->m_content_height);

#ifdef CONFIG_AMP_VPP_ROTATION
            if (ROTATION_90D == plane->uchRotationAngle || ROTATION_270D ==  plane->uchRotationAngle) {
                SWAP_W_H(dataAlign_x, dataAlign_y);
                if ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) && (10 == pinfo->m_bits_per_pixel)) {
                    dataAlign_x = 24;
                    dataAlign_y = 96;
                }
            }
            if ((ROTATION_0D == plane->uchRotationAngle)&&(plane->m_SampleCropEnable)&&(IS_MTR_ENABLED(plane->m_mtrR__en))) {
                if (SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt) {
                    /* Multiple of compressed tile width*/
                    dataAlign_x = 64;
                } else if ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) && (10 == pinfo->m_bits_per_pixel)) {
                    /* Multiple of compressed tile width*/
                    dataAlign_x = 48;
                }
                dataAlign_y = 4;
            }
#endif

#ifdef ENABLE_SCALAR_LINEDROPPING
            int org_ref_win_height = plane->ref_win.height;
            int org_ref_win_width = plane->ref_win.width;
#ifdef CONFIG_AMP_VPP_ROTATION
            if (ROTATION_90D == plane->uchRotationAngle || ROTATION_270D ==  plane->uchRotationAngle) {
                SWAP_W_H(plane->ref_win.width, plane->ref_win.height);
            }
#endif
            if (1 == plane->sclLineDropEn) {
#ifdef ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA
                int vpp_sys_clk = 600000;
                int mult_fact_vratio = 100;
                int req_vscale_ratio = 1;
                int max_vscale_ratio = 1;
                int org_lineDrop;

                max_vscale_ratio = mult_fact_vratio;
                max_vscale_ratio *= vpp_sys_clk;
                max_vscale_ratio /= pResinfo->freq;
                max_vscale_ratio *= pResinfo->active_width; //active width or htotal???
                max_vscale_ratio /= plane->ref_win.width;
                plane->lineDroppingStrides = 0;
                do {
                    plane->lineDroppingStrides++;
                    req_vscale_ratio = mult_fact_vratio;
                    req_vscale_ratio *= plane->ref_win.height;
                    req_vscale_ratio /= plane->lineDroppingStrides;
                    req_vscale_ratio /= chan->disp_win_final.height;

                } while (req_vscale_ratio > max_vscale_ratio);

                org_lineDrop = plane->lineDroppingStrides;
                ovps_printf(OVPS_INFO,"lineDroppingStrides : %d, srcfmt : %d, bpp : %d\n",plane->lineDroppingStrides,  plane->srcfmt, plane->bpp);
                //Limit linedropping when stride size(in bytes) is bigger then allowed limit VPP_LINEDROP_STRIDE_MAX(16bits-0xFFFF)
                //e.g : 10bit tiled mode : whenever hgt < 541, we see garbage, i.e /4, so avoid going above /3 factor
#ifdef CONFIG_AMP_VPP_ROTATION
                if (plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt)
#else
                if (plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
                {
                    if ((pinfo->m_buf_stride*plane->lineDroppingStrides)*tiledModeStrideFactor > VPP_LINEDROP_STRIDE_MAX)
                        plane->lineDroppingStrides = VPP_LINEDROP_STRIDE_MAX/(pinfo->m_buf_stride*tiledModeStrideFactor);
                    } else {
                        if ((pinfo->m_buf_stride*plane->lineDroppingStrides) > VPP_LINEDROP_STRIDE_MAX)
                            plane->lineDroppingStrides = VPP_LINEDROP_STRIDE_MAX/pinfo->m_buf_stride;
                    }
                if (org_lineDrop != plane->lineDroppingStrides) {
                    ovps_printf(OVPS_INFO, "********** %d %s >>  Stride Limitaion ****************** \n", __LINE__, __func__);
                    ovps_printf(OVPS_INFO,"Changing lineDroppingStrides from : %d to %d\n",org_lineDrop, plane->lineDroppingStrides);
                }

                plane->ref_win.height/=plane->lineDroppingStrides;

#else //ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA

                int max_downscale_hratio = 1;
                int mult_fact_hratio = 1000;
                int lineDrop4KSel = is4KInputLineDroppingRequired(vpp_obj, chanID);
                int tileYAlign = 1

                if (SCAN_PROGRESSIVE == pResinfo->scan)
                    max_downscale_hratio = 2*mult_fact_hratio;
                else
                    max_downscale_hratio = 4*mult_fact_hratio;

                //check if hscale ratio is > 2 to enable line dropping
                if (plane->ref_win.height > chan->disp_win_final.height &&
                        (((plane->ref_win.height*mult_fact_hratio/(chan->disp_win_final.height*1)) > max_downscale_hratio) || ( 1 == lineDrop4KSel))) {
                    int lineDropSel;    //Line selected for processing in line drop case
                    int tempStride = 0;
#ifdef CONFIG_AMP_VPP_ROTATION
                    if (plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt)
#else
                    if (plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
                    {
                        tileYAlign = 4;
                        tiledModeStrideFactor=VPP_TILED_MODE_STRIDE_FACTOR==1?4:1;
                        tiledModeWidHgtFactor=1*4;
                    }
#ifdef ENABLE_CROMA_ALLIGNED_LINEDROP
#ifdef CONFIG_AMP_VPP_ROTATION
                    if ((SRCFMT_YUV420SP == plane->srcfmt) || (SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) || (SRCFMT_YUV420SP_DWA == plane->srcfmt) ||
                            (SRCFMT_YUV420SP_TILED_MODE1== plane->srcfmt) || (SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt))
#else
                    if ((SRCFMT_YUV420SP == plane->srcfmt) || (SRCFMT_YUV420SP_DWA == plane->srcfmt) ||
                                (SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) || (SRCFMT_YUV420SP_TILED_MODE1== plane->srcfmt))
#endif
                    {
                        uiCromaAllignedLinedropFactor = 2;
                    }
#endif
                    //enable line dropping
                    if (1 == lineDrop4KSel) {
                        plane->lineDroppingStrides = 2;
                        plane->ref_win.height/=plane->lineDroppingStrides;
                    } else {

                        plane->lineDroppingStrides = plane->ref_win.height/chan->disp_win_final.height;
                        ovps_printf(OVPS_INFO,"lineDroppingStrides : %d, srcfmt : %d, bpp : %d\n",plane->lineDroppingStrides,  plane->srcfmt, plane->bpp);
                        int org_lineDrop = plane->lineDroppingStrides;
                        //Limit linedropping when stride size(in bytes) is bigger then allowed limit VPP_LINEDROP_STRIDE_MAX(16bits-0xFFFF)
                        //e.g : 10bit tiled mode : whenever hgt < 541, we see garbage, i.e /4, so avoid going above /3 factor
                        tempStride = (pinfo->m_buf_stride*plane->lineDroppingStrides)*tiledModeStrideFactor*uiCromaAllignedLinedropFactor;
                        if (tempStride > VPP_LINEDROP_STRIDE_MAX)
                            plane->lineDroppingStrides = VPP_LINEDROP_STRIDE_MAX/(pinfo->m_buf_stride*tiledModeStrideFactor*uiCromaAllignedLinedropFactor);

                        if (org_lineDrop != plane->lineDroppingStrides) {
                            ovps_printf(OVPS_INFO, "********** %d %s >>  Stride Limitaion ****************** \n", __LINE__, __func__);
                            ovps_printf(OVPS_INFO,"Changing lineDroppingStrides from : %d to %d\n",org_lineDrop, plane->lineDroppingStrides);
                        }

                        //reduce the ref_win height by same amount
                        plane->ref_win.height/=plane->lineDroppingStrides;
                    }
                    lineDropSel = getLineDroppingStartY(plane);
                    /*pinfo->m_active_top store the starty as -ve number
                      so add lineDropSel as -ve number to move the startY by required number of lines */
                    ovps_printf(OVPS_INFO,"pinfo->m_active_top : %d\n", pinfo->m_active_top);
                    pinfo->m_active_top  = pinfo->m_active_top + (-lineDropSel*tileYAlign);
                    ovps_printf(OVPS_INFO,"lineDroppingStrides        : %d\n", plane->lineDroppingStrides);
                    ovps_printf(OVPS_INFO,"lineDroppingStyle->LineSel : %d->%d\n", plane->lineDroppingStyle,lineDropSel);
                    ovps_printf(OVPS_INFO,"temp ref_win.height : %d\n", plane->ref_win.height);
                    ovps_printf(OVPS_INFO,"pinfo->m_active_top : %d\n", pinfo->m_active_top);
                } else {
                    plane->lineDroppingStrides = 0;
                }
#endif //!ENABLE_SCALAR_LINEDROPPING_METHOD_FORMULA
            }
#ifdef CONFIG_AMP_VPP_ROTATION
            if (ROTATION_90D == plane->uchRotationAngle || ROTATION_270D ==  plane->uchRotationAngle) {
                SWAP_W_H(plane->ref_win.width, plane->ref_win.height);
            }
#endif
#endif //ENABLE_SCALAR_LINEDROPPING

            /* crop display content */
#ifdef CONFIG_AMP_VPP_ROTATION
            if (SRCFMT_YUV420SP == pinfo->m_srcfmt || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == pinfo->m_srcfmt || SRCFMT_YUV420SP_DWA == pinfo->m_srcfmt || SRCFMT_YUV422SP_DWA == pinfo->m_srcfmt)
#else
            if (SRCFMT_YUV420SP == pinfo->m_srcfmt || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1
                        || SRCFMT_YUV420SP_DWA == pinfo->m_srcfmt || SRCFMT_YUV422SP_DWA == pinfo->m_srcfmt)
#endif
            {

                ovps_cropDispContent_UV(&crop_actv_win, &crop_actv_offset_UV, &crop_scl_actv_win, pinfo, &plane->ref_win, &chan->disp_win_final, pResinfo, dataAlign_x, dataAlign_y, plane->bpp, plane);
            }
            ovps_cropDispContent(&crop_actv_win, &crop_actv_offset, &crop_scl_actv_win, pinfo, &plane->ref_win, &chan->disp_win_final, pResinfo, dataAlign_x, dataAlign_y, plane->bpp, plane);


            if (IS_MTR_ENABLED(plane->m_mtrR__en)) {
                if (plane->m_leftpad_y != plane->m_leftpad_c) {
                    // Crop-2 is required for the case when Luma and Chroma left-offsets are not same.
                    vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en = 1;
                    vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en = 1;
                    plane->m_SampleCropEnable = 1;
                } else {
                    vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en = 0;
                    vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en = 0;
                }
                if (plane->m_Aligned_Luma_Width > plane->m_Aligned_Chroma_Width) {
                    if (plane->uDhubCropOffset_c.u32_Tile_Xoffset != 0) {
                        plane->m_leftpadadjust_c += (plane->m_Aligned_Luma_Width - plane->m_Aligned_Chroma_Width);
                    } else {
                        plane->m_rightpad_c += (plane->m_Aligned_Luma_Width - plane->m_Aligned_Chroma_Width);
                    }
                    plane->m_Aligned_Chroma_Width = plane->m_Aligned_Luma_Width;
                    plane->uDhubCropOffset_c.u32_Tile_Xoffset = plane->uDhubCropOffset_y.u32_Tile_Xoffset;
                } else if (plane->m_Aligned_Luma_Width < plane->m_Aligned_Chroma_Width) {
                    if (plane->uDhubCropOffset_y.u32_Tile_Xoffset != 0) {
                        plane->m_leftpadadjust_y += (plane->m_Aligned_Chroma_Width - plane->m_Aligned_Luma_Width);
                    } else {
                        plane->m_rightpad_y += (plane->m_Aligned_Chroma_Width - plane->m_Aligned_Luma_Width);
                    }
                    plane->m_Aligned_Luma_Width = plane->m_Aligned_Chroma_Width;
                    plane->uDhubCropOffset_y.u32_Tile_Xoffset = plane->uDhubCropOffset_c.u32_Tile_Xoffset;
                }

            }
#ifdef ENABLE_SCALAR_LINEDROPPING
            if ((1 == plane->sclLineDropEn) && plane->lineDroppingStrides) {
#ifdef CONFIG_AMP_VPP_ROTATION
                if (ROTATION_90D == plane->uchRotationAngle || ROTATION_270D ==  plane->uchRotationAngle) {
                    plane->ref_win.width = org_ref_win_width;
                } else
#endif
                {
                    //restore the ref_win.height to the user specified one
                    plane->ref_win.height = org_ref_win_height;
                }
            } else
#endif //ENABLE_SCALAR_LINEDROPPING
            {
                //Let us keep lineDroppingStrides = 1 for default case ; i.e no dropping
                plane->lineDroppingStrides = 1;
            }
            ovps_printf(OVPS_INFO,"lineDroppingStrides : %d\n", plane->lineDroppingStrides);

            if ((crop_actv_win.width != plane->crop_actv_win.width) || (crop_actv_win.height != plane->crop_actv_win.height)) {
                /* update plane->crop_actv_win */
                plane->crop_actv_win.width = crop_actv_win.width;
                plane->crop_actv_win.height = crop_actv_win.height;
                data_loader_update_required = 1;
                ovps_printf(OVPS_INFO, "##################### DATA LODER UPDATE 0[%dx%d] #######################\n",crop_actv_win.width,crop_actv_win.height);
            }

            if ((crop_scl_actv_win.x != plane->crop_scl_actv_win.x)
                    || (crop_scl_actv_win.y != plane->crop_scl_actv_win.y)
                    || (crop_scl_actv_win.width != plane->crop_scl_actv_win.width)
                    || (crop_scl_actv_win.height != plane->crop_scl_actv_win.height)) {
                /* update plane->crop_scl_actv_win */
                plane->crop_scl_actv_win.x = crop_scl_actv_win.x;
                plane->crop_scl_actv_win.y = crop_scl_actv_win.y;
                plane->crop_scl_actv_win.width = crop_scl_actv_win.width;
                plane->crop_scl_actv_win.height = crop_scl_actv_win.height;
                data_loader_update_required = 1;
                ovps_printf(OVPS_INFO, "##################### DATA LODER UPDATE 1 [%dx%d]#######################\n",crop_scl_actv_win.width,crop_scl_actv_win.height);
            }

            /* update plane->crop_actv_offset */
            plane->crop_actv_offset = crop_actv_offset;
            plane->crop_actv_offset_UV = crop_actv_offset_UV;

        } // <- crop calculation

        if ((plane->crop_actv_win.width <= 0) || (plane->crop_actv_win.height <= 0)) {
            ovps_printf(OVPS_INFO, "return :%s : %d\n",__func__,__LINE__);
            return 0;
        }

        /* update data loader if frame type changes */
        if (plane->frame_type == -1) {
            data_loader_update_required = 1;
            // update current frame type
            plane->frame_type = 1; // force to progressive
        }

        if (!(IS_MTR_ENABLED(plane->m_mtrR__en))) {
            crop_actv_offset = (int)pinfo->m_pbuf_start+/*pinfo->m_disp_offset + pinfo->m_content_offset+*/plane->crop_actv_offset;
            crop_actv_offset_UV = (int)pinfo->m_buf_pbuf_start_UV+/*pinfo->m_disp_offset + pinfo->m_content_offset+*/plane->crop_actv_offset_UV;
        }

        ovps_printf(OVPS_INFO, "-----%08x, %08x, %08x, %08x, %x, %x = %x \n", pinfo->m_pbuf_start, pinfo->m_disp_offset, pinfo->m_content_offset, plane->crop_actv_offset, plane->tile.tileStartFrac, crop_actv_offset, plane->crop_disp_offset);
        //Please add the offset calculated for TILED moved in previous iteration, since the final cropped disp offset is stored in plane->crop_disp_offset
        if (plane->crop_disp_offset != (crop_actv_offset)) {
            ovps_printf(OVPS_INFO, "Adjusted crop_actv_offset:%x,%x, crop_actv_offset_UV:%x,%x\n",crop_actv_offset,plane->crop_disp_offset,crop_actv_offset_UV,plane->crop_disp_offset_UV);
            plane->crop_disp_offset = crop_actv_offset;
            plane->crop_disp_offset_UV = crop_actv_offset_UV;
            //data_loader_crop_required = 1;
        }

        ovps_printf(OVPS_INFO, "data_loader_update_required : %d , data_loader_crop_required : %d\n",data_loader_update_required, data_loader_crop_required);
        /* update plane data loader resolution */
        if (data_loader_update_required || data_loader_crop_required){
            VPP_FE_DLR_INPUT_CROP feCropRes;
            int actualHStart,actualVStart;
            int topCrop, bottomCrop;
            int leftCrop, rightCrop;
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
            int leftAdjust=0, rightAdjust=0, enableAdditionalScalarCropping=0;
#endif
            int dataAlign_x = 0, dataAlign_y = 0;
            //input to FETG
            res.HRes = plane->crop_actv_win.width;
            res.VRes = plane->crop_actv_win.height;

            actualHStart = pinfo->m_active_left<0? -pinfo->m_active_left: pinfo->m_active_left;
            actualVStart = pinfo->m_active_top<0? -pinfo->m_active_top: pinfo->m_active_top;
            dataAlign_x = VPP_OVL_getDataAlignBytes_X(plane->uchClientMode, pinfo->m_content_width);
            dataAlign_y = VPP_OVL_getDataAlignBytes_Y(plane->uchClientMode, pinfo->m_content_height);
#ifdef CONFIG_AMP_VPP_ROTATION
            if (ROTATION_90D == plane->uchRotationAngle || ROTATION_270D ==  plane->uchRotationAngle) {
                SWAP_W_H(dataAlign_x, dataAlign_y);
                if ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) && (10 == pinfo->m_bits_per_pixel)) {
                    dataAlign_x = 24;
                    dataAlign_y = 96;
                }
            }
#endif
            /* For compressed inputs  Horizontal alignment is 64 & vertical alignment is 4 (64x4 compressed tile)*/
            if ((ROTATION_0D == plane->uchRotationAngle)&& IS_MTR_ENABLED(plane->m_mtrR__en)) {
                if (SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt) {
                    /* Multiple of compressed tile width*/
                    dataAlign_x = 64;
                } else if ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) && (10 == pinfo->m_bits_per_pixel)) {
                    /* Multiple of compressed tile width*/
                    dataAlign_x = 48;
                }
                dataAlign_y = 8;
            }
            //aligned input to FETG-Crop & scalar
            //Initially crop and actual are same, If required cropRes will become > final resolution
            cropRes = res;
            //vpp_obj->scl_ctrl[planeID].leftCrop/rightCrop : For only PIP/GFX plane with GFX pipe
            leftCrop = actualHStart%dataAlign_x;
            rightCrop = res.HRes%dataAlign_x;

            if (leftCrop) {
                actualHStart -= leftCrop;
            }
            //Hres should be multiple of pixel granularity : res.HRes > cropRes.HRes
            if (rightCrop || leftCrop) {
                res.HRes = ((res.HRes + leftCrop + (dataAlign_x-1)) / dataAlign_x ) * dataAlign_x;
#ifdef ENABLE_FE_FULLCROP
#ifdef CONFIG_AMP_VPP_ROTATION
                if (pinfo->m_srcfmt == SRCFMT_ARGB32 || plane->srcfmt == SRCFMT_ARGB32 ||
                        pinfo->m_srcfmt == SRCFMT_ARGB4444 || plane->srcfmt == SRCFMT_ARGB4444 ||
                        pinfo->m_srcfmt == SRCFMT_ARBG2101010 || plane->srcfmt == SRCFMT_ARBG2101010 ||
                        pinfo->m_srcfmt == SRCFMT_RGB565|| plane->srcfmt == SRCFMT_RGB565 ||
                        pinfo->m_srcfmt == SRCFMT_ARGB1555|| plane->srcfmt == SRCFMT_ARGB1555 ||
                        plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                        SRCFMT_YUV420SP == plane->srcfmt || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt)
#else
                if (pinfo->m_srcfmt == SRCFMT_ARGB32 || plane->srcfmt == SRCFMT_ARGB32 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB4444 || plane->srcfmt == SRCFMT_ARGB4444 ||
                            pinfo->m_srcfmt == SRCFMT_ARBG2101010 || plane->srcfmt == SRCFMT_ARBG2101010 ||
                            pinfo->m_srcfmt == SRCFMT_RGB565|| plane->srcfmt == SRCFMT_RGB565 ||
                            pinfo->m_srcfmt == SRCFMT_ARGB1555|| plane->srcfmt == SRCFMT_ARGB1555 ||
                            plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                            SRCFMT_YUV420SP == plane->srcfmt)
#endif
                    //For tiled format, we will have always read complete stride
#else
                if ((pinfo->m_srcfmt == SRCFMT_ARGB32 || plane->srcfmt == SRCFMT_ARGB32) ||(pinfo->m_srcfmt == SRCFMT_ARBG2101010 || plane->srcfmt == SRCFMT_ARBG2101010)||(pinfo->m_srcfmt == SRCFMT_ARGB4444 || plane->srcfmt == SRCFMT_ARGB4444) ||(pinfo->m_srcfmt == SRCFMT_RGB565 || plane->srcfmt == SRCFMT_RGB565) ||(pinfo->m_srcfmt == SRCFMT_ARGB1555|| plane->srcfmt == SRCFMT_ARGB1555))
#endif
                {
                    //chop according to final HRes to be configured into readclient
                    //exclude leftcrop from final res to get right chop
                    rightCrop = res.HRes - plane->crop_actv_win.width - leftCrop;
                } else {
                    //This is not required for GFX, but required for less reading in 422,420SP cases
                    rightCrop = (res.HRes - cropRes.HRes) - leftCrop;
                }
            }
            plane->m_BL_scl_crop = 0;
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
            if (!(pinfo->m_srcfmt == SRCFMT_ARGB32 || plane->srcfmt == SRCFMT_ARGB32 || pinfo->m_srcfmt == SRCFMT_ARBG2101010 || plane->srcfmt == SRCFMT_ARBG2101010 || pinfo->m_srcfmt == SRCFMT_ARGB4444 || plane->srcfmt == SRCFMT_ARGB4444 || pinfo->m_srcfmt == SRCFMT_RGB565 || plane->srcfmt == SRCFMT_RGB565) || pinfo->m_srcfmt == SRCFMT_ARGB1555|| plane->srcfmt == SRCFMT_ARGB1555) {
                //let us do the odd start/width cropping at scalar - fetg will do only cropping at even pixel boundary
                leftAdjust = leftCrop & 0x01;   //make startx position always even
                rightAdjust = rightCrop & 0x01; //max width always even
                if (leftAdjust || rightAdjust) {
                    enableAdditionalScalarCropping = 1;
                }
                leftCrop-=leftAdjust;
                rightCrop-=rightAdjust;
            }
#endif
#ifdef CONFIG_AMP_VPP_ROTATION
            if (ROTATION_180D == plane->uchRotationAngle || ROTATION_H_FLIP == plane->uchRotationAngle) {
                SWAP_W_H(leftCrop, rightCrop);
                SWAP_W_H(leftAdjust, rightAdjust);
            }
#endif
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
            if (VPP_SCAN_INVERSE_H == plane->uchInverseScanMode || VPP_SCAN_INVERSE_HV == plane->uchInverseScanMode) {
                if (SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt || ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) && (10 == pinfo->m_bits_per_pixel))) {
                    //Swap only if no rotation is set. If rotation is set, then do not swap them
                    if (ROTATION_0D == plane->uchRotationAngle) {
                        SWAP_W_H(leftCrop, rightCrop);
                        SWAP_W_H(leftAdjust, rightAdjust);
                    }
                } else {
                    SWAP_W_H(leftCrop, rightCrop);
                    SWAP_W_H(leftAdjust, rightAdjust);
                }
            }
#endif

            feCropRes.leftCrop = leftCrop;
            feCropRes.rightCrop = rightCrop;
            vpp_obj->scl_ctrl[planeID].leftCrop = leftCrop;
            vpp_obj->scl_ctrl[planeID].rightCrop = rightCrop;

            topCrop = actualVStart%dataAlign_y;
            bottomCrop = res.VRes%dataAlign_y;
            if (topCrop) {
                actualVStart -= topCrop;
            }
            //Vres should be multiple of pixel row granularity : res.VRes > cropRes.VRes
            if (topCrop || bottomCrop) {
                res.VRes = ((res.VRes + topCrop + (dataAlign_y-1)) / dataAlign_y ) * dataAlign_y;
                bottomCrop = (res.VRes - cropRes.VRes) - topCrop;
            }
            feCropRes.topCrop = topCrop;
            feCropRes.bottomCrop = bottomCrop;

#ifdef CONFIG_AMP_VPP_ROTATION
            if (ROTATION_90D == plane->uchRotationAngle) {
                int tmp_left_crop = 0, tmp_left_adjust = 0, tmp_right_crop = 0, tmp_right_adjust = 0;
                SWAP_W_H(res.HRes, res.VRes);
                SWAP_W_H(cropRes.HRes, cropRes.VRes);

                tmp_left_crop = feCropRes.leftCrop;
                tmp_right_crop = feCropRes.rightCrop;
                tmp_left_adjust = leftAdjust;
                tmp_right_adjust = rightAdjust;

                feCropRes.leftCrop = feCropRes.bottomCrop & (~0x01);
                leftAdjust = feCropRes.bottomCrop & 0x01;
                feCropRes.rightCrop = feCropRes.topCrop &  (~0x01);
                rightAdjust = feCropRes.topCrop & 0x01;

                feCropRes.topCrop = tmp_left_crop + tmp_left_adjust;
                feCropRes.bottomCrop = tmp_right_crop + tmp_right_adjust;
                if (leftAdjust || rightAdjust) {
                    enableAdditionalScalarCropping = 1;
                }

            } else if (ROTATION_270D == plane->uchRotationAngle) {
                int tmp_left_crop = 0, tmp_left_adjust = 0, tmp_right_crop = 0, tmp_right_adjust = 0;

                SWAP_W_H(res.HRes, res.VRes);
                SWAP_W_H(cropRes.HRes, cropRes.VRes);

                tmp_left_crop = feCropRes.leftCrop;
                tmp_right_crop = feCropRes.rightCrop;
                tmp_left_adjust = leftAdjust;
                tmp_right_adjust = rightAdjust;

                feCropRes.leftCrop = feCropRes.topCrop & (~0x01);
                leftAdjust = feCropRes.topCrop & 0x01;
                feCropRes.rightCrop = feCropRes.bottomCrop &  (~0x01);
                rightAdjust = feCropRes.bottomCrop & 0x01;

                feCropRes.bottomCrop = tmp_left_crop + tmp_left_adjust;
                feCropRes.topCrop = tmp_right_crop + tmp_right_adjust;

                if (leftAdjust || rightAdjust) {
                    enableAdditionalScalarCropping = 1;
                }
            }
#endif
            ovps_printf(OVPS_INFO,"Crop : mode: %d, align_x : %d, align_y : %d, crop_actv_win.width : %d, crop_actv_win.height : %d,\n",
                    plane->uchClientMode, dataAlign_x, dataAlign_y, plane->crop_actv_win.width, plane->crop_actv_win.height);
            ovps_printf(OVPS_INFO,"leftCrop : %d,  rightCrop: %d, topCrop : %d, bottomCrop : %d\n",
                    leftCrop, rightCrop, topCrop, bottomCrop);
            ovps_printf(OVPS_INFO,"Modified leftCrop : %d,  rightCrop: %d, topCrop : %d, bottomCrop : %d\n",
                    feCropRes.leftCrop, feCropRes.rightCrop, feCropRes.topCrop, feCropRes.bottomCrop);
            ovps_printf(OVPS_INFO,"actual (xXy HXV) : %dX%d %dX%d, crop (xXy HXV) : %dX%d %dX%d\n", actualHStart, actualVStart, res.HRes, res.VRes,
                    actualHStart+feCropRes.leftCrop, actualVStart+feCropRes.topCrop, cropRes.HRes, cropRes.VRes);
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
            ovps_printf(OVPS_INFO,"Adjustment for cropping at scalar - leftAdjust : %d, rightAdjust : %d\n",leftAdjust, rightAdjust);
#endif

            plane->stElPipeCropRes.leftCrop = feCropRes.leftCrop;
            plane->stElPipeCropRes.rightCrop = feCropRes.rightCrop;
            plane->stElPipeCropRes.topCrop = feCropRes.topCrop;
            plane->stElPipeCropRes.bottomCrop = feCropRes.bottomCrop;

            if ((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable)) {
                //Get the tile details for the given format
                VPP_OVL_getTileDetails(plane->uchClientMode,
                        plane->m_Aligned_Luma_Width,
                        plane->m_Aligned_Luma_Height,
                        &plane->tile);
            } else {
                //Get the tile details for the given format
                VPP_OVL_getTileDetails(plane->uchClientMode,
                        res.HRes,
                        res.VRes,
                        &plane->tile);
            }

            plane->tileStrideFrac_BL = plane->tile.tileStrideFrac;

            //Init aligned_active_height as it is used in updateVPSize()
            plane->aligned_active_height = res.VRes;
            if ((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable)) {
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                int leftAdjusty = 0, rightAdjusty = 0, leftAdjustc = 0, rightAdjustc = 0;
                if (!(pinfo->m_srcfmt == SRCFMT_ARGB32 || plane->srcfmt == SRCFMT_ARGB32 || pinfo->m_srcfmt == SRCFMT_ARBG2101010 || plane->srcfmt == SRCFMT_ARBG2101010 || pinfo->m_srcfmt == SRCFMT_ARGB4444 || plane->srcfmt == SRCFMT_ARGB4444 || pinfo->m_srcfmt == SRCFMT_RGB565 || plane->srcfmt == SRCFMT_RGB565 || pinfo->m_srcfmt == SRCFMT_ARGB1555|| plane->srcfmt == SRCFMT_ARGB1555 || plane->srcfmt == SRCFMT_YUV444_DWA)) {
                        //let us do the odd start/width cropping at scalar - fetg will do only cropping at even pixel boundary
                        leftAdjusty = plane->m_leftpadadjust_y & 0x01;   //make startx position always even
                        rightAdjusty = plane->m_rightpad_y & 0x01; //max width always even
                        if (leftAdjusty || rightAdjusty) {
                            enableAdditionalScalarCropping = 1;
                        }
                        plane->m_leftpadadjust_y -= leftAdjusty;
                        plane->m_rightpad_y -= rightAdjusty;

                        //let us do the odd start/width cropping at scalar - fetg will do only cropping at even pixel boundary
                        leftAdjustc = plane->m_leftpadadjust_c & 0x01;   //make startx position always even
                        rightAdjustc = plane->m_rightpad_c & 0x01; //max width always even
                        if (leftAdjustc || rightAdjustc) {
                            enableAdditionalScalarCropping = 1;
                        }
                        plane->m_leftpadadjust_c -= leftAdjustc;
                        plane->m_rightpad_c -= rightAdjustc;

                        leftAdjust = leftAdjusty;
                        rightAdjust = rightAdjusty;
                }
#endif
            }

            //Check, if scalar cropping is required or not
#ifdef CONFIG_AMP_VPP_ROTATION
            if (plane->srcfmt == SRCFMT_YUV422 || plane->srcfmt == SRCFMT_YUV420SP ||
                    plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                    plane->srcfmt == SRCFMT_YUV420SP_DWA || plane->srcfmt == SRCFMT_YUV422SP_DWA ||
                    SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt || plane->srcfmt == SRCFMT_YUV444_DWA)
#else
            if (plane->srcfmt == SRCFMT_YUV422 || plane->srcfmt == SRCFMT_YUV420SP ||
                    plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1
                    || plane->srcfmt == SRCFMT_YUV420SP_DWA || plane->srcfmt == SRCFMT_YUV422SP_DWA)
#endif
            {
                //Let us use the crop dimension for scaling, if cropping is not done at Scalar i.e for all video format
                feRes = cropRes;
#ifdef ENABLE_SCALAR_CROPPING_FOR_ODD_STARTX_WIDTH
                if (enableAdditionalScalarCropping) {
                    //Scalar input should be more(equivalent to number of pixels cropped less at FETG)
                    //and the extra pixel avialable at the input of scalar will be chopped at scalar chop_ctrl
                    feRes.HRes += (leftAdjust+rightAdjust);
                }
#endif
            } else  {
                feRes = res;
            }

            // Updated client_res with res, once after applying all the cropping logic and storing the result in res
            client_res.HRes = res.HRes;
            client_res.VRes = res.VRes;
            ovps_printf(OVPS_INFO, "!!!!!!!!!!!!!! %dx%d !!!!!!!!!! [%d]\n",res.HRes,res.VRes,planeID);
            if (planeID == PLANE_OVP_EL) {
                /*calculate crop WPL*/
                if (plane->srcfmt == SRCFMT_YUV420SP || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt) {
                    PixelPerBeat = PIXEL_PER_BEAT_YUV_420SP;

                }
                CropWpl = (res.HRes +  PixelPerBeat - 1) / PixelPerBeat;
                plane->wpl = CropWpl;
                OVP_FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_OVP_EL_0, (PVPP_FE_DLR_INPUT_RES)&client_res, &cropRes, plane->srcfmt, plane->bpp);
#ifdef CONFIG_AMP_VPP_ROTATION
                if (plane->srcfmt == SRCFMT_YUV420SP || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 || SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt)
#else
                if (plane->srcfmt == SRCFMT_YUV420SP || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 || plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
                {
                    OVP_FE_DLR_SetClientSize(vpp_obj, chanID,VPP_FE_DLR_CHANNEL_OVP_EL_1, (PVPP_FE_DLR_INPUT_RES)&client_res, &cropRes, plane->srcfmt, plane->bpp);
                }
                if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) && (OVPS_PARAMS->readTiledfromOVPclient)) {

                    T32RASTERSCAN_RD_CTRL1 stRdCtrl1;
                    stRdCtrl1.u32 = vpp_obj->uiRasterScanRdCtrl1;
                    if (plane->bpp == 10)
                        stRdCtrl1.uCTRL1_ups420_idat_sel = 2;
                    else
                        stRdCtrl1.uCTRL1_ups420_idat_sel = 0;
                    OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_CTRL1, stRdCtrl1.u32);
                    vpp_obj->uiRasterScanRdCtrl1 = stRdCtrl1.u32;
                    OVPSCL_ISR_SetDummyTG_RasterScan(vpp_obj, PLANE_MAIN, &res, (PVPP_FE_DLR_INPUT_CROP)&feCropRes, plane->frame_type);
                }
                else
                    OVPSCL_ISR_SetELTGParams(vpp_obj, &res, &feCropRes, plane->frame_type);

            }
            plane->uiFeDlrInHres = res.HRes;
            plane->uiFeDlrInVres = res.VRes;

        } // <- update data loader


    } // <- update VPP setting on frame (2-field)  boundary

    /* here, plane->crop_actv_win & plane->crop_actv_offset is cropped display content */
    if ((plane->crop_actv_win.width <= 0) || (plane->crop_actv_win.height <= 0)) {

        ovps_printf(OVPS_INFO, "%s : %d [%dx%d]\n",__func__,__LINE__,plane->crop_actv_win.width,plane->crop_actv_win.height);

        /* no display content for display */
        return (MV_VPP_OK);
    }

    ovps_printf(OVPS_INFO, "planeID =  %d,crop_actv_offset : 0x%x, CropWpl : 0x%x, PIXEL_PER_BEAT_YUV_422/420SP : 0x%x, plane->wpl: 0x%x\b\n",
            planeID,crop_actv_offset, CropWpl, PixelPerBeat, plane->wpl);
    ovps_printf(OVPS_INFO, "width => pinfo->m_buf_stride : %x , plane->wpl*plane->bpp = (%x,%x) => %x\n\n",
            pinfo->m_buf_stride, plane->wpl, plane->bpp, plane->wpl*plane->bpp);

    ovps_printf(OVPS_INFO, "crop_actv_offset : %x, crop_actv_offset_UV : %x\n",crop_actv_offset, crop_actv_offset_UV);
    ovps_printf(OVPS_INFO, "%s : %d,status = %d[%dx%d][%dx%d]\n",__func__,__LINE__,plane->status,crop_actv_win.width,crop_actv_win.height,plane->crop_actv_win.width,plane->crop_actv_win.height);
    ovps_printf(OVPS_INFO, "%s : %d\n",__func__,__LINE__);

    /* start data loader DMA to load display content */
    if (pinfo->m_is_frame_seq || (planeID != PLANE_MAIN)) {

#ifdef CONFIG_AMP_VPP_ROTATION
        if (plane->srcfmt == SRCFMT_YUV420SP ||
                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt  ||
                plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                plane->srcfmt == SRCFMT_YUV422SP_DWA)
#else
        if (plane->srcfmt == SRCFMT_YUV420SP ||
                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 ||
                plane->srcfmt == SRCFMT_YUV420SP_DWA ||
                plane->srcfmt == SRCFMT_YUV422SP_DWA)
#endif
        {
#ifdef CONFIG_AMP_VPP_ROTATION
            OVPS_DHUB_PARAM_ROTATION stDhubParam;
            memset(&stDhubParam, 0, sizeof(OVPS_DHUB_PARAM_ROTATION));
#endif
            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
#ifdef CONFIG_AMP_VPP_ROTATION
            if ((plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 &&
                        8 == pinfo->m_bits_per_pixel) ||
                    plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#else
            if (plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                    plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
            {
                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                stCfg2NDDMAParam.addr   = (UNSG32)(plane->crop_disp_offset+plane->tile.tileStartFrac);
                stCfg2NDDMAParam.width  = (plane->wpl*plane->bpp+plane->tile.tileStrideFrac) *
                    tiledModeWidHgtFactor*uiCromaAllignedLinedropFactor;
                stCfg2NDDMAParam.stride = (pinfo->m_buf_stride*plane->lineDroppingStrides) *
                    tiledModeStrideFactor*uiCromaAllignedLinedropFactor;
                stCfg2NDDMAParam.height = plane->aligned_active_height/tiledModeWidHgtFactor;
                stCfg2NDDMAParam.cfgQ   = cfgQ;
                stCfg2NDDMAParam.mtrREn = 0;
                stCfg2NDDMAParam.plane  = plane;
                *cfgQ_len += ovpsDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                ovps_printf(OVPS_MID, "OVP:burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",
                        (plane->wpl*plane->bpp+plane->tile.tileStrideFrac) *
                        tiledModeWidHgtFactor*uiCromaAllignedLinedropFactor,
                        (plane->wpl*plane->bpp+plane->tile.tileStrideFrac) *
                        tiledModeWidHgtFactor*uiCromaAllignedLinedropFactor,
                        1, (pinfo->m_buf_stride*plane->lineDroppingStrides) *
                        tiledModeStrideFactor*uiCromaAllignedLinedropFactor,
                        plane->aligned_active_height/tiledModeWidHgtFactor);
            }
#ifdef CONFIG_AMP_VPP_ROTATION
            else if (SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt ||
                    ((SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt) &&
                     (10 == pinfo->m_bits_per_pixel))) {
                UINT32 uiRotationAngle = plane->uchRotationAngle;

                stDhubParam.m_uiSrcFtm = pinfo->m_srcfmt;
                stDhubParam.m_uiSrcOrder = pinfo->m_order;
                stDhubParam.m_uiBitDepth = pinfo->m_bits_per_pixel;
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                GetRotationAngle(vpp_obj, planeID, &uiRotationAngle);
#endif
                stDhubParam.m_uiRotationAngle = uiRotationAngle;

                if (ROTATION_90D == plane->uchRotationAngle || ROTATION_270D == plane->uchRotationAngle) {
                    stDhubParam.m_uiPicW = plane->uiFeDlrInVres;
                    stDhubParam.m_uiPicH = plane->uiFeDlrInHres;
                } else {
                    stDhubParam.m_uiPicW = plane->uiFeDlrInHres;
                    stDhubParam.m_uiPicH = plane->uiFeDlrInVres;
                }
                stDhubParam.m_uiBaseAddr_Y = (UNSG32)(plane->crop_disp_offset+plane->tile.tileStartFrac);
                stDhubParam.m_uiBaseAddr_UV = (UNSG32)(plane->crop_disp_offset_UV+plane->tile.tileStartFrac);
                stDhubParam.m_uiStride = pinfo->m_buf_stride * plane->lineDroppingStrides;

                GetDhubParam(vpp_obj, planeID, &stDhubParam);

                if (ROTATION_0D == plane->uchRotationAngle) {
                    if ((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable)) {
                        PixelPerBeat = PIXEL_PER_BEAT_YUV_420SP;
                        plane->wpl = (plane->m_Aligned_Luma_Width +  PixelPerBeat - 1) / PixelPerBeat;
                        plane->aligned_active_height = plane->m_Aligned_Luma_Height;
                    }

                    stCfg2NDDMAParam.dhubId   = (INT)plane->dmaRdhubID;
                    stCfg2NDDMAParam.dmaId    = plane->dmaRID[0];
                    stCfg2NDDMAParam.addr     = (UNSG32)(plane->crop_disp_offset+  plane->tile.tileStartFrac);
                    stCfg2NDDMAParam.width    = (plane->wpl*plane->bpp+plane->tile.tileStrideFrac)*tiledModeWidHgtFactor * uiCromaAllignedLinedropFactor;
                    stCfg2NDDMAParam.stride   = (pinfo->m_buf_stride *plane->lineDroppingStrides) *tiledModeStrideFactor * uiCromaAllignedLinedropFactor;
                    stCfg2NDDMAParam.height   = plane->aligned_active_height/tiledModeWidHgtFactor;
                    stCfg2NDDMAParam.cfgQ     = cfgQ;
                    stCfg2NDDMAParam.mtrREn   = plane->m_mtrR__en;
                    stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_Y;
                    stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_Y;
                    stCfg2NDDMAParam.plane  = plane;

                    *cfgQ_len += ovpsDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                } else {
                    *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[0],
                            stDhubParam.m_uiBaseAddr_Y, stDhubParam.m_uiBurst_Y,
                            stDhubParam.m_uiStep_1_Y, stDhubParam.m_uiSize_1_Y,
                            stDhubParam.m_uiStep_2_Y, stDhubParam.m_uiSize_2_Y, (T64b*)cfgQ);
                }
            }
#endif
            else {
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                UINT32 uiStride = 0;
                UINT32 uiAddr = 0;
                if (VPP_SCAN_INVERSE_V == plane->uchInverseScanMode ||
                        VPP_SCAN_INVERSE_HV == plane->uchInverseScanMode) {
                    uiStride = ~(pinfo->m_buf_stride*plane->lineDroppingStrides *
                            uiCromaAllignedLinedropFactor) + 1;
                    uiAddr = ((UNSG32)(plane->crop_disp_offset) & (~0x07)) +
                        ((plane->aligned_active_height - 1) *
                         pinfo->m_buf_stride*plane->lineDroppingStrides *
                         uiCromaAllignedLinedropFactor);
                } else {
                    uiAddr = (UNSG32)(plane->crop_disp_offset) & (~0x07);
                    uiStride = pinfo->m_buf_stride*plane->lineDroppingStrides*uiCromaAllignedLinedropFactor;
                }

                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                stCfg2NDDMAParam.dmaId  = plane->dmaRID[0];
                stCfg2NDDMAParam.addr   = uiAddr;
                stCfg2NDDMAParam.width  = plane->wpl*plane->bpp+plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor;
                stCfg2NDDMAParam.stride = uiStride;
                stCfg2NDDMAParam.height = plane->aligned_active_height;
                stCfg2NDDMAParam.cfgQ   = cfgQ;
                stCfg2NDDMAParam.mtrREn = plane->m_mtrR__en;
                stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_Y;
                stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_Y;
                stCfg2NDDMAParam.plane  = plane;
                *cfgQ_len += ovpsDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
#else
                *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[0],
                        (UNSG32)(plane->crop_disp_offset),
                        plane->wpl*plane->bpp+plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor,
                        plane->wpl*plane->bpp+plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor,
                        1, pinfo->m_buf_stride*plane->lineDroppingStrides*uiCromaAllignedLinedropFactor,
                        plane->aligned_active_height, (T64b*)cfgQ);
#endif
                ovps_printf(OVPS_MID, "OVP:burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",
                        plane->wpl*plane->bpp+plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor,
                        plane->wpl*plane->bpp+plane->tile.tileStrideFrac*uiCromaAllignedLinedropFactor,
                        1, pinfo->m_buf_stride*plane->lineDroppingStrides*uiCromaAllignedLinedropFactor,
                        plane->aligned_active_height);
            }
            cfgQ = cfgQ_shadow + *cfgQ_len * 2;
#ifdef CONFIG_AMP_VPP_ROTATION
            if ((plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 &&
                        (8 == pinfo->m_bits_per_pixel)) ||
                    plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1 )
#else
            if (plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0 ||
                    plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE1)
#endif
            {
                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                stCfg2NDDMAParam.addr   = (UNSG32)(plane->crop_disp_offset_UV+plane->tile.tileStartFrac);
                stCfg2NDDMAParam.width  = (plane->wpl*plane->bpp+plane->tile.tileStrideFrac)*tiledModeWidHgtFactor;
                stCfg2NDDMAParam.stride = (pinfo->m_buf_stride_UV*plane->lineDroppingStrides)*tiledModeStrideFactor;
                stCfg2NDDMAParam.height = plane->aligned_active_height/(2*tiledModeWidHgtFactor);
                stCfg2NDDMAParam.cfgQ   = cfgQ;
                stCfg2NDDMAParam.mtrREn = 0;
                stCfg2NDDMAParam.plane  = plane;
                *cfgQ_len += ovpsDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                ovps_printf(OVPS_MID, "OVP:burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",
                        (plane->wpl*plane->bpp+plane->tile.tileStrideFrac)*tiledModeWidHgtFactor,
                        (plane->wpl*plane->bpp+plane->tile.tileStrideFrac)*tiledModeWidHgtFactor,
                        1, (pinfo->m_buf_stride_UV*plane->lineDroppingStrides)*tiledModeStrideFactor,
                        plane->aligned_active_height/(2*tiledModeWidHgtFactor));
            }
#ifdef CONFIG_AMP_VPP_ROTATION
            else if (SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt ||
                    ((plane->srcfmt == SRCFMT_YUV420SP_TILED_MODE0) &&
                     (10 == pinfo->m_bits_per_pixel))) {
                if (ROTATION_0D == plane->uchRotationAngle) {
                    UINT32 CDhubPadHeight = 0;
                    if ((IS_MTR_ENABLED(plane->m_mtrR__en))&&(plane->m_SampleCropEnable)) {
                        PixelPerBeat = PIXEL_PER_BEAT_YUV_420SP;
                        plane->wpl = (plane->m_Aligned_Chroma_Width +  PixelPerBeat - 1) / PixelPerBeat;
                        CDhubPadHeight = /*plane->m_Aligned_Chroma_Height*/plane->m_Aligned_Luma_Height/2;
                        CDhubPadHeight = CDhubPadHeight/tiledModeWidHgtFactor;
                    } else {
                        CDhubPadHeight = plane->aligned_active_height/(2*tiledModeWidHgtFactor);
                    }

                    stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                    stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                    stCfg2NDDMAParam.addr   = (UNSG32)(plane->crop_disp_offset_UV + plane->tile.tileStartFrac);
                    stCfg2NDDMAParam.width  = (plane->wpl*plane->bpp+plane->tile.tileStrideFrac)* tiledModeWidHgtFactor;
                    stCfg2NDDMAParam.stride = (pinfo->m_buf_stride_UV *plane->lineDroppingStrides) * tiledModeStrideFactor;
                    stCfg2NDDMAParam.height = CDhubPadHeight;
                    stCfg2NDDMAParam.cfgQ   = cfgQ;
                    stCfg2NDDMAParam.mtrREn   = plane->m_mtrR__en;
                    stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_UV;
                    stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_UV;
                    stCfg2NDDMAParam.plane  = plane;
                    *cfgQ_len += ovpsDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
                } else {
                    *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[1],
                            stDhubParam.m_uiBaseAddr_UV, stDhubParam.m_uiBurst_UV,
                            stDhubParam.m_uiStep_1_UV, stDhubParam.m_uiSize_1_UV,
                            stDhubParam.m_uiStep_2_UV, stDhubParam.m_uiSize_2_UV,
                            (T64b*)cfgQ);
                }
            }
#endif
            else {
#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
                UINT32 uiStride = 0;
                UINT32 uiAddr = 0;
                if (VPP_SCAN_INVERSE_V == plane->uchInverseScanMode ||
                        VPP_SCAN_INVERSE_HV == plane->uchInverseScanMode) {
                    uiStride = ~(pinfo->m_buf_stride_UV*plane->lineDroppingStrides) + 1;
                    if (SRCFMT_YUV422SP_DWA == plane->srcfmt) {
                        uiAddr = ((UNSG32)(plane->crop_disp_offset_UV) & (~0x07)) +
                            ((plane->aligned_active_height - 1) *
                             pinfo->m_buf_stride_UV * plane->lineDroppingStrides);
                    } else {
                        uiAddr = ((UNSG32)(plane->crop_disp_offset_UV) & (~0x07)) +
                            ((plane->aligned_active_height/2 - 1) *
                             pinfo->m_buf_stride_UV * plane->lineDroppingStrides);
                    }
                } else {
                    uiAddr = (UNSG32)(plane->crop_disp_offset_UV) & (~0x07);
                    uiStride = pinfo->m_buf_stride_UV*plane->lineDroppingStrides;
                }
                stCfg2NDDMAParam.dhubId = (INT)plane->dmaRdhubID;
                stCfg2NDDMAParam.dmaId  = plane->dmaRID[1];
                stCfg2NDDMAParam.addr   = uiAddr;
                stCfg2NDDMAParam.width  = plane->wpl*plane->bpp+plane->tile.tileStrideFrac;
                stCfg2NDDMAParam.stride = uiStride;
                stCfg2NDDMAParam.height = plane->aligned_active_height/2;
                stCfg2NDDMAParam.cfgQ   = cfgQ;
                stCfg2NDDMAParam.mtrREn = plane->m_mtrR__en;
                stCfg2NDDMAParam.chkSemId = plane->m_mtrR_chkSemId_UV;
                stCfg2NDDMAParam.updSemId = plane->m_mtrR_updSemId_UV;
                stCfg2NDDMAParam.plane  = plane;
                *cfgQ_len += ovpsDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);
#else
                *cfgQ_len += START_2NDDMA(plane->dmaRdhubID, plane->dmaRID[1],
                        (UNSG32)(plane->crop_disp_offset_UV),
                        plane->wpl * plane->bpp + plane->tile.tileStrideFrac,
                        plane->wpl * plane->bpp + plane->tile.tileStrideFrac,
                        1, pinfo->m_buf_stride_UV*plane->lineDroppingStrides,
                        plane->aligned_active_height/2, (T64b*)cfgQ);
#endif
                ovps_printf(OVPS_MID, "OVP:burst %d, step1 %d, size1 %d, step2 %d, size2 %d\n",
                        plane->wpl*plane->bpp+plane->tile.tileStrideFrac,
                        plane->wpl*plane->bpp+plane->tile.tileStrideFrac,
                        1, pinfo->m_buf_stride_UV*plane->lineDroppingStrides,
                        plane->aligned_active_height/2);
            }
        }
    }

    /* start data loader */
    if (planeID == PLANE_OVP_EL) {
        ovps_printf(OVPS_INFO, "srcfmt = %d,readTiledfromOVPclient = %d,ovp_source = %d\n",
                plane->srcfmt,OVPS_PARAMS->readTiledfromOVPclient,
                OVPS_PARAMS->ovp_source);

        switch (plane->srcfmt) {
            case SRCFMT_YUV420SP_TILED_MODE0:
            case SRCFMT_YUV420SP_TILED_MODE1:
            case SRCFMT_YUV420SP_TILED_V4H8:
            case SRCFMT_YUV420SP:
                {
                    if (!OVPS_PARAMS->readTiledfromOVPclient) {
                        OVP_FE_DEINT_VpClient_Clear(vpp_obj, VPP_VP_CLIENT_READ0);
                        OVP_FE_DEINT_VpClient_Clear(vpp_obj, VPP_VP_CLIENT_READ1);
                        OVP_FE_DEINT_VpClient_Enable(vpp_obj, VPP_VP_CLIENT_READ0, 1);
                        OVP_FE_DEINT_VpClient_Enable(vpp_obj, VPP_VP_CLIENT_READ1, 1);
                    } else {
                        ovps_printf(OVPS_INFO, "OVP Start RD Client  .............\n");
                        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL0;
                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR0_clear = 1;
                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR1_clear = 1;
                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR0_start = 0;
                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR1_start = 0;
                        OVPS_BCMBUF_vidpl_Write (RegAddr, OVPS_PARAMS->ovpsclwrap_ctrl0.u32);

                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR0_clear = 0;
                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR1_clear = 0;
                        OVPS_BCMBUF_vidpl_Write (RegAddr, OVPS_PARAMS->ovpsclwrap_ctrl0.u32);

                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR0_start = 1;
                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR1_start = 1;
                        OVPS_BCMBUF_vidpl_Write (RegAddr, OVPS_PARAMS->ovpsclwrap_ctrl0.u32);

                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR0_start = 0;
                        OVPS_PARAMS->ovpsclwrap_ctrl0.uCTRL0_ClientR1_start = 0;
                        OVPS_BCMBUF_vidpl_Write (RegAddr, OVPS_PARAMS->ovpsclwrap_ctrl0.u32);
                    }

                    if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) &&
                            (OVPS_PARAMS->readTiledfromOVPclient)) {
                        ovps_printf(OVPS_INFO, "OVP Start RD  Client  TG .............\n");
                        RegAddr = RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_CTRL0;
                        OVPS_BCMBUF_vidpl_Write (RegAddr, 2);
                        OVPS_BCMBUF_vidpl_Write (RegAddr, 0);
                        OVPS_BCMBUF_vidpl_Write (RegAddr, 1);
                        OVPS_BCMBUF_vidpl_Write (RegAddr, 0);
                    } else {
                        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL0;
                        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, 2);
                        OVPS_BCMBUF_Write(vpp_obj->ovp_obj.pOvpSclClearBcmBuf, RegAddr, 0);
                        RegAddr = RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL0;
                        OVPS_BCMBUF_vidpl_Write (RegAddr, 1);
                        OVPS_BCMBUF_vidpl_Write (RegAddr, 0);
                    }
                }
                break;
            default:
                /* Do nothing */
                break;
        }
    } // <- start data loader of each none OSD/AUX plane
    VDBG_IF_DISP_ENABLE(0);
    ovps_printf(OVPS_INFO, "%s : %d\n",__func__,__LINE__);

    return (MV_VPP_OK);
}

int ovps_print_client_req(OVPS_CLIENT_REQUEST *pstOvpsRequest)
{
    VBUF_INFO *vbuf_info  = NULL,*out_vbuf = NULL;
#ifndef CONFIG_AMP_TEE_ENABLE
    VDEC_VID_BUF_DESC * frame_desc = NULL,*frame_desc_ovp = NULL;
#endif
    if (pstOvpsRequest == NULL) {
        ovps_printf(OVPS_ERR,"Invalid Address\n");
        return -1;
    }
#ifndef CONFIG_AMP_TEE_ENABLE
    frame_desc = (VDEC_VID_BUF_DESC*)pstOvpsRequest->pinfo;
    vbuf_info = (VBUF_INFO *)frame_desc->user_data;
    frame_desc_ovp = (VDEC_VID_BUF_DESC *)vbuf_info->m_OvpSclFrameDesc;
    out_vbuf = (VBUF_INFO *)frame_desc_ovp->user_data;
#else
    vbuf_info = (VBUF_INFO *)pstOvpsRequest->pinfo;
    out_vbuf = (VBUF_INFO *)vbuf_info->m_OvpSclFrameDesc;
    ovps_printf(OVPS_MID,"DEST FRM = 0x%08X,SRC FRM = = 0x%08X\n",out_vbuf,vbuf_info);
#endif

    ovps_printf(OVPS_INFO,"SRC FRM:\n========\n\n");
    ovps_printf(OVPS_INFO,"\t RES = %dx%d\n",vbuf_info->m_active_width,vbuf_info->m_active_height);
    ovps_printf(OVPS_INFO,"\t Bitdepth = %d\n",vbuf_info->m_bits_per_pixel);
    ovps_printf(OVPS_INFO,"\t srcfmt = %d\n",vbuf_info->m_srcfmt);

    ovps_printf(OVPS_INFO,"DEST FRM:\n========\n\n");
    ovps_printf(OVPS_INFO,"\t RES = %dx%d\n",out_vbuf->m_active_width,out_vbuf->m_active_height);
    ovps_printf(OVPS_INFO,"\t Bitdepth = %d\n",out_vbuf->m_bits_per_pixel);
    ovps_printf(OVPS_INFO,"\t fmt = %d\n",out_vbuf->m_srcfmt);

    return 0;
}

/*FIXME: Need to update based on frame*/
int OVPS_Update_frame(VPP_OBJ *vpp_obj,OVPS_CLIENT_REQUEST *pstOvpsRequest)
{
    VBUF_INFO *vbuf_info  = NULL,*out_vbuf = NULL;
#ifndef CONFIG_AMP_TEE_ENABLE
    VDEC_VID_BUF_DESC * frame_desc = NULL,*frame_desc_ovp = NULL;
#endif
    PLANE* plane = NULL;

    if (OVPS_PARAMS->readTiledfromOVPclient == 0)
        OVP_Enable_OVPClient(vpp_obj,1);

    ovps_print_client_req(pstOvpsRequest);

#ifndef CONFIG_AMP_TEE_ENABLE
    frame_desc = (VDEC_VID_BUF_DESC*)pstOvpsRequest->pinfo;
    vbuf_info = (VBUF_INFO *)frame_desc->user_data;
    frame_desc_ovp = (VDEC_VID_BUF_DESC *)vbuf_info->m_OvpSclFrameDesc;
    out_vbuf = (VBUF_INFO *)frame_desc_ovp->user_data;
#else
    vbuf_info = (VBUF_INFO *)pstOvpsRequest->pinfo;
    out_vbuf = (VBUF_INFO *)vbuf_info->m_OvpSclFrameDesc;
#endif

    //Need dynamic update based on input fmt
    if (vbuf_info->m_srcfmt == SRCFMT_YUV420SP)
        OVPS_PARAMS->ovp_source = OVPS_RD_CLIENT_OVP_RASTER;
    else
        OVPS_PARAMS->ovp_source = OVPS_RD_CLIENT_OVP_TILE;

    plane = &(vpp_obj->ovp_obj.plane);
    /* Align width to have crop at dHub boundary */
    plane->ref_win.width = (((int)((vbuf_info->m_active_width) / VPP_OVPS_DHUB_SIZE))*VPP_OVPS_DHUB_SIZE);
    plane->ref_win.height = vbuf_info->m_active_height;

    plane->ref_win_updated = 1;

    OVPS_PARAMS->ovp_win.width = out_vbuf->m_active_width;
    OVPS_PARAMS->ovp_win.height = out_vbuf->m_active_height;
    OVPS_PARAMS->ovp_output_bit_depth = out_vbuf->m_bits_per_pixel;

    OVPS_PARAMS->ovp_share_vidbuf = 0;
    OVPS_PARAMS->ovp_win_outfmt = out_vbuf->m_srcfmt;
    vpp_obj->ovp_obj.output_res =  RES_4Kx2K60;

    vpp_obj->ovp_obj.client_info = (void*)pstOvpsRequest;

    return 0;
}

int OVPS_Interrupt_Enable(VPP_OBJ *vpp_obj)
{
#ifndef VPP_DIAGS
    VPP_INTR_MSG intr_info = {0, 0};
#endif
    ovps_printf(OVPS_INFO, "%s : %d\n",__func__,__LINE__);

    if (vpp_obj->ovp_obj.intr_en == 0) {
#ifndef VPP_DIAGS
        intr_info.Enable = 1;
#ifndef TZ_3_0
        vpp_obj->pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
#endif

        intr_info.DhubSemMap = avioDhubSemMap_vpp128b_vpp_inr12;
#ifndef CONFIG_AMP_TEE_ENABLE
        ioctl(berlin_pe_dev, VPP_IOCTL_INTR_MSG, &intr_info);
#else
        VPP_Enable_INT(intr_info.DhubSemMap, intr_info.Enable);
#endif
        //Y-EOF
        semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr12, 1, 0);
        semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr12);
        semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr12,
                0/*empty*/, 1/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);

        //CB-EOF
        semaphore_cfg(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr11, 1, 0);
        semaphore_clr_full(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr11);
        semaphore_intr_enable(vpp_obj->pSemHandle, avioDhubSemMap_vpp128b_vpp_inr11,
                0/*empty*/, 0/*full*/, 0/*almost empty*/, 0/*almost full*/, 0/*cpu id*/);

        //Global Interrupt
        GA_REG_WORD32_WRITE((vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_INTR_EN), 0x3);
#endif
        vpp_obj->ovp_obj.intr_en = 1;
    }
    return 0;
}

int OVPS_TG_clear(VPP_OBJ *vpp_obj)
{
    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclClearBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x800);
    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclClearBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x0);

    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclClearBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x208);
    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclClearBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x000);

    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclClearBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x0C0);
    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclClearBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x000);

    return 0;
}

int OVPS_TG_start(VPP_OBJ *vpp_obj)
{
    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x030);
    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x104);
    OVPS_BCMBUF_Write (vpp_obj->ovp_obj.pOvpSclBcmBuf, (vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl), 0x001);

    return 0;
}

void OVPS_dHub_Clear(VPP_OBJ *vpp_obj)
{
    PLANE *plane = &vpp_obj->ovp_obj.plane;

    //clear dhub
    CLEAR_2NDDMA_DIRECT(plane->dmaRdhubID, plane->dmaRID[0], vpp_obj->ovp_obj.pOvpSclClearBcmBuf);
    CLEAR_2NDDMA_DIRECT(plane->dmaRdhubID, plane->dmaRID[1], vpp_obj->ovp_obj.pOvpSclClearBcmBuf);
    CLEAR_2DDMA_DIRECT((HDL_dhub2d *)OVPS_PARAMS->ovp_y_dmaWdhubID, OVPS_PARAMS->ovp_y_dmaWID,
            vpp_obj->ovp_obj.pOvpSclClearBcmBuf);
    CLEAR_2DDMA_DIRECT((HDL_dhub2d *)OVPS_PARAMS->ovp_uv_dmaWdhubID, OVPS_PARAMS->ovp_uv_dmaWID,
            vpp_obj->ovp_obj.pOvpSclClearBcmBuf);
}

/* interrupt service routine for CPCB TG interrupt */
void OVPSCL_ISR_service(VPP_OBJ *vpp_obj, int cpcbID)
{
    int chanID;
    PLANE *plane;
    UINT32 RegAddr;
    INT32 ovp_width_inbyts = 0;
    UINT32 value;

    if (OVPS_PARAMS->ovp_win_updated) {
        ovps_printf(OVPS_ERR, "%s : %d, OVPS running with VPP,\n",__func__,__LINE__);
        return;
    }
    int *cfgQ;
    int *cfgQ_shadow = vpp_obj->ovp_obj.curr_ovpscl_dma_cfgQ->addr;
    int *cfgQ_len = &(vpp_obj->ovp_obj.curr_ovpscl_dma_cfgQ->len);
    VDEC_VID_BUF_DESC *inline_frame, *frm_descr_ovp;
    VBUF_INFO *pinfo, *pinfo_ovp;
    T32WriteClient_NonStdRes stNonStdRes;

    plane = &vpp_obj->ovp_obj.plane;
    chanID = CHAN_OVP_EL;

    inline_frame = plane->curr_frame ;
#ifndef CONFIG_AMP_TEE_ENABLE
    pinfo = (VBUF_INFO *)inline_frame->user_data;
#else
    pinfo = (VBUF_INFO *)inline_frame;
#endif
    frm_descr_ovp = (VDEC_VID_BUF_DESC *) pinfo->m_OvpSclFrameDesc;

    OVPSCL_ISR_ConfigDefaultPipe(vpp_obj);

    if (frm_descr_ovp != NULL) {
#ifndef CONFIG_AMP_TEE_ENABLE
        pinfo_ovp = (VBUF_INFO*) frm_descr_ovp->user_data;
#else
        pinfo_ovp = (VBUF_INFO*) frm_descr_ovp;
#endif
        vpp_obj->stElPipeCtrl2.uCTRL2_tgEn_rstEn = DISP_FRST_TGRST_OVPFLUSH_EN;
        RegAddr =  RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL2;
        OVPS_BCMBUF_vidpl_Write (RegAddr, vpp_obj->stElPipeCtrl2.u32);

        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_tg_ctrl;
        OVPS_BCMBUF_vidpl_Write (RegAddr, 0x002);
        OVPS_BCMBUF_vidpl_Write (RegAddr, 0x000);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC
            + RA_SCL1D_YC_CFG20;
        OVPS_BCMBUF_vidpl_Write (RegAddr, 0x300);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC
            + RA_SCL1D_YC_CFG20;
        OVPS_BCMBUF_vidpl_Write (RegAddr, 0x300);

        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_scl1d_Inpix;
        value = vpp_obj->ovp_obj.plane.ref_win.height *
            vpp_obj->ovp_obj.plane.ref_win.width;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_scl1d_Inpix;
#ifndef VPP_OVPS_EN_UV_PACK_SEL
        value = vpp_obj->ovp_obj.plane.ref_win.height *
            vpp_obj->ovp_obj.plane.ref_win.width/2;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
#else
        value = vpp_obj->ovp_obj.plane.ref_win.height/2 *
            vpp_obj->ovp_obj.plane.ref_win.width/2;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OCSCOFifo_pix;
        value = vpp_obj->ovp_obj.plane.ref_win.height *
            vpp_obj->ovp_obj.plane.ref_win.width;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
#endif

        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_scl1d_Outpix;
        value = OVPS_PARAMS->ovp_win.height * OVPS_PARAMS->ovp_win.width;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_scl1d_Outpix;
#ifdef VPP_OVPS_EN_UV_PACK_SEL
        value = OVPS_PARAMS->ovp_win.height/2 * OVPS_PARAMS->ovp_win.width/2;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);

        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_sclOFIFO_YInpix;
        value =  OVPS_PARAMS->ovp_win.height * OVPS_PARAMS->ovp_win.width;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_sclOFIFO_UVInpix;
        value = OVPS_PARAMS->ovp_win.height*OVPS_PARAMS->ovp_win.width;
#else
        value = OVPS_PARAMS->ovp_win.height * OVPS_PARAMS->ovp_win.width/2;
#endif
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);

        /* Allow scaler bypass for 4K to 4K */
        if ((vpp_obj->ovp_obj.plane.ref_win.height == OVPS_PARAMS->ovp_win.height) &&
                (vpp_obj->ovp_obj.plane.ref_win.width == OVPS_PARAMS->ovp_win.width) &&
                (OVPS_PARAMS->ovp_win.width > 1920)) {
            OVPS_PARAMS->ovp_y_scl_ctrl.uCTRL_scl1d_YC_bypass = 1;
            OVPS_PARAMS->ovp_uv_scl_ctrl.uCTRL_scl1d_YC_bypass = 1;
        } else {
            OVPSCL_ISR_SetSclParams(vpp_obj, &(vpp_obj->ovp_obj.plane.ref_win), &OVPS_PARAMS->ovp_win, VPP_FRC_SCL_OVP_Y);
            OVPSCL_ISR_SetSclParams(vpp_obj, &(vpp_obj->ovp_obj.plane.ref_win), &OVPS_PARAMS->ovp_win, VPP_FRC_SCL_OVP_UV);
        }
        if (OVPS_PARAMS->ovp_output_bit_depth == 10) {
            OVPS_PARAMS->ovp_y_scl_ctrl.uCTRL_dither_bypass = 1;
            OVPS_PARAMS->ovp_uv_scl_ctrl.uCTRL_dither_bypass = 1;
        } else {
            OVPS_PARAMS->ovp_y_scl_ctrl.uCTRL_dither_bypass = 0;
            OVPS_PARAMS->ovp_uv_scl_ctrl.uCTRL_dither_bypass = 0;
        }
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_CTRL;
        value = OVPS_PARAMS->ovp_y_scl_ctrl.u32;
        OVPS_BCMBUF_vidpl_Write(RegAddr,value);

        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_CTRL;
        value = OVPS_PARAMS->ovp_uv_scl_ctrl.u32;
        OVPS_BCMBUF_vidpl_Write(RegAddr,value);

        if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) && (!OVPS_PARAMS->readTiledfromOVPclient))
            OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ovpscl_ictrl_sel = 1;
        else if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) && (OVPS_PARAMS->readTiledfromOVPclient))

        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ovpscl_ictrl_sel = 6;
    else if (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_MAIN_TILE)
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ovpscl_ictrl_sel = 2;
    else if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_PIP_TILE) || (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_TILE))
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ovpscl_ictrl_sel = 3;
    else
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ovpscl_ictrl_sel = 4;

    if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_MAIN_RASTER) || (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_MAIN_TILE))
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_clk_en_sel = 0;
    else if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_PIP_TILE) || ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) && (!OVPS_PARAMS->readTiledfromOVPclient)) || (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_TILE))
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_clk_en_sel = 1;
    else if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) && (OVPS_PARAMS->readTiledfromOVPclient)) {
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_clk_en_sel = 3;
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_rdsel_420SP = 1;
    } else
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_clk_en_sel = 2;

    if (OVPS_PARAMS->ovp_output_bit_depth == 10) {
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_luma_data_sel = 0;
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_chroma_data_sel = 0;
    } else {
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_luma_data_sel = 1;
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_chroma_data_sel = 1;
    }

    OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_420sp_wrbk = 1;
    OVPS_PARAMS->ovpsclwrap_ctrl1.uCTRL1_ovp_flush_sel = DISP_FRST_TGRST_OVPFLUSH_EN;
    OVPS_PARAMS->ovpsclwrap_ctrl1.uCTRL1_ovp_intr_sel = 1;

    RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL;
    OVPS_BCMBUF_vidpl_Write (RegAddr, OVPS_PARAMS->ovpsclwrap_ctrl.u32);
    RegAddr =  RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL1;
    OVPS_BCMBUF_vidpl_Write (RegAddr, OVPS_PARAMS->ovpsclwrap_ctrl1.u32);
    OVPSCL_ISR_SetTGParams(vpp_obj, OVPS_PARAMS->ovp_win.width, OVPS_PARAMS->ovp_win.height, 1);
#ifdef VPP_OVPS_EN_UV_PACK_SEL
        OVPSCL_ISR_SetTGParams(vpp_obj, OVPS_PARAMS->ovp_win.width/2, OVPS_PARAMS->ovp_win.height/2, 0);
#else
        OVPSCL_ISR_SetTGParams(vpp_obj, OVPS_PARAMS->ovp_win.width, OVPS_PARAMS->ovp_win.height/2, 0);
#endif
        OVPS_PARAMS->ovp_win_stride = (((OVPS_PARAMS->ovp_win.width*OVPS_PARAMS->ovp_output_bit_depth)+127)/128)*16;
        ovp_width_inbyts = ((OVPS_PARAMS->ovp_win.width * OVPS_PARAMS->ovp_output_bit_depth)+7)/8;
        if ((OVPS_PARAMS->ovp_win.width*OVPS_PARAMS->ovp_output_bit_depth)%128) {
            stNonStdRes.uNonStdRes_enable = 1;
            stNonStdRes.uNonStdRes_pixlineTot = OVPS_PARAMS->ovp_win.width;
        } else {
            stNonStdRes.uNonStdRes_enable = 0;
        }
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_WR0 + RA_WriteClient_NonStdRes;
        OVPS_BCMBUF_vidpl_Write (RegAddr, stNonStdRes.u32);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_WR1 + RA_WriteClient_NonStdRes;
        OVPS_BCMBUF_vidpl_Write (RegAddr, stNonStdRes.u32);
        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
        OVPS_DHUB_CFG_2NDDMA_PARAM stCfg2NDDMAParam;
        stCfg2NDDMAParam.dhubId = OVPS_PARAMS->ovp_y_dmaWdhubID;
        stCfg2NDDMAParam.dmaId  = avioDhubChMap_vpp128b_OVPSCL_W0;
        stCfg2NDDMAParam.addr   = (UNSG32)pinfo_ovp->m_pbuf_start;
        stCfg2NDDMAParam.width  = ovp_width_inbyts;
        stCfg2NDDMAParam.stride = OVPS_PARAMS->ovp_win_stride;
        stCfg2NDDMAParam.height = OVPS_PARAMS->ovp_win.height;
        stCfg2NDDMAParam.cfgQ   = cfgQ;
        stCfg2NDDMAParam.mtrREn = 0;
        stCfg2NDDMAParam.plane  = plane;
        *cfgQ_len += ovpsDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

        cfgQ = cfgQ_shadow + *cfgQ_len * 2;
        stCfg2NDDMAParam.dhubId = OVPS_PARAMS->ovp_uv_dmaWdhubID;
        stCfg2NDDMAParam.dmaId  = avioDhubChMap_vpp128b_OVPSCL_W1;
        stCfg2NDDMAParam.addr   = pinfo_ovp->m_buf_pbuf_start_UV;
        stCfg2NDDMAParam.width  = ovp_width_inbyts;
        stCfg2NDDMAParam.stride = OVPS_PARAMS->ovp_win_stride;
        stCfg2NDDMAParam.height = OVPS_PARAMS->ovp_win.height/2;
        stCfg2NDDMAParam.cfgQ   = cfgQ;
        stCfg2NDDMAParam.mtrREn = 0;
        stCfg2NDDMAParam.plane  = plane;
        *cfgQ_len += ovpsDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

        pinfo_ovp->m_buf_stride = OVPS_PARAMS->ovp_win_stride;
        pinfo_ovp->m_buf_stride_UV = OVPS_PARAMS->ovp_win_stride;
        pinfo_ovp->m_bits_per_pixel = OVPS_PARAMS->ovp_output_bit_depth;
        pinfo_ovp->m_active_height = OVPS_PARAMS->ovp_win.height;
        pinfo_ovp->m_content_height = OVPS_PARAMS->ovp_win.height;
        pinfo_ovp->m_active_width = OVPS_PARAMS->ovp_win.width;
        pinfo_ovp->m_content_width = OVPS_PARAMS->ovp_win.width;

        ovps_printf(OVPS_MID, "$$$$$$ %x,%x,%d,%d,%d,%d,%d,%d $$$$$$$$\n",pinfo_ovp->m_pbuf_start,pinfo_ovp->m_buf_pbuf_start_UV,pinfo_ovp->m_buf_stride,pinfo_ovp->m_buf_stride_UV,
                pinfo_ovp->m_bits_per_pixel,pinfo_ovp->m_active_height,pinfo_ovp->m_active_width,OVPS_PARAMS->ovp_source);

        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_WR0 + RA_WriteClient_pix;
        value =  OVPS_PARAMS->ovp_win.height*OVPS_PARAMS->ovp_win.width;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_WR1 + RA_WriteClient_pix;
#ifdef VPP_OVPS_EN_UV_PACK_SEL
        value = OVPS_PARAMS->ovp_win.height/2 * OVPS_PARAMS->ovp_win.width/2;
#else
        value = OVPS_PARAMS->ovp_win.height * OVPS_PARAMS->ovp_win.width/2;
#endif
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_WR0 + RA_WriteClient_pack;
        value = (OVPS_PARAMS->ovp_output_bit_depth == 8) ? 0:1;
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
        RegAddr = RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_WR1 + RA_WriteClient_pack;
#ifdef VPP_OVPS_EN_UV_PACK_SEL
        value = (OVPS_PARAMS->ovp_output_bit_depth == 8) ? 3:4;
#else
        value = (OVPS_PARAMS->ovp_output_bit_depth == 8)? 0:1;
#endif
        OVPS_BCMBUF_vidpl_Write (RegAddr, value);
        if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) || (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_TILE)) {
            vpp_obj->stElPipeCtrl1.uCTRL1_clken_ctrl2 = 0;
            vpp_obj->stElPipeCtrl1.uCTRL1_clken_ctrl3 = 1;
            vpp_obj->stElPipeCtrl1.uCTRL1_clken_ctrl4 = 1;
            RegAddr = RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;
            OVPS_BCMBUF_vidpl_Write (RegAddr, vpp_obj->stElPipeCtrl1.u32);
        }

        OVPS_TG_clear(vpp_obj);

        if (!OVPS_PARAMS->readTiledfromOVPclient) {
            if ((OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) || (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_TILE)) {
                T32EL_PIPE_CTRL1 stElPipeCtrl1;

                stElPipeCtrl1.u32 = vpp_obj->stElPipeCtrl1.u32;
                stElPipeCtrl1.uCTRL1_read_en = 1;
                stElPipeCtrl1.uCTRL1_el_rdsel_420SP = 1;
                stElPipeCtrl1.uCTRL1_pip_rd_sel = 0;
                stElPipeCtrl1.uCTRL1_dint_rd_sel = 1;

                vpp_obj->stElPipeCtrl1.u32 = stElPipeCtrl1.u32;
                RegAddr = RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;
                OVPS_BCMBUF_vidpl_Write (RegAddr, stElPipeCtrl1.u32);
            }
        } else {
            if (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_RASTER) {
                T32RASTERSCAN_RD_CTRL1 stRdCtrl1;

                stRdCtrl1.u32 = vpp_obj->uiRasterScanRdCtrl1;
                stRdCtrl1.uCTRL1_read_sel = 3;
                stRdCtrl1.uCTRL1_fifo_sts_ctrl = 3;
                stRdCtrl1.uCTRL1_fifo_full_ctrl1 = 1;
                stRdCtrl1.uCTRL1_fifo_full_ctrl2 = 1;
                stRdCtrl1.uCTRL1_tgEn_rstEn = DISP_FRST_TGRST_OVPFLUSH_EN;
                RegAddr = RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_CTRL1;
                OVPS_BCMBUF_vidpl_Write (RegAddr, stRdCtrl1.u32);
                vpp_obj->uiRasterScanRdCtrl1 = stRdCtrl1.u32;
            }
            if (OVPS_PARAMS->ovp_source == OVPS_RD_CLIENT_OVP_TILE) {
                T32EL_PIPE_CTRL1 stElPipeCtrl1;

                stElPipeCtrl1.u32 = vpp_obj->stElPipeCtrl1.u32;
                stElPipeCtrl1.uCTRL1_clken_ctrl0 = 0;
                stElPipeCtrl1.uCTRL1_clken_ctrl1 = 0;
                stElPipeCtrl1.uCTRL1_clken_ctrl2 = 0;
                stElPipeCtrl1.uCTRL1_clken_ctrl3 = 1;
                stElPipeCtrl1.uCTRL1_clken_ctrl4 = 1;
                stElPipeCtrl1.uCTRL1_clken_ctrl5 = 1;
                stElPipeCtrl1.uCTRL1_clken_ctrl6 = 1;

                stElPipeCtrl1.uCTRL1_ovp_rd_sel = 1;

                vpp_obj->stElPipeCtrl1.u32 = stElPipeCtrl1.u32;
                RegAddr = RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;
                OVPS_BCMBUF_vidpl_Write (RegAddr, stElPipeCtrl1.u32);
            }
        }
        OVPS_Interrupt_Enable(vpp_obj);
    }
    OVPSCL_ISR_startChannelDataLoader(vpp_obj, chanID);

    if (vpp_obj->ovp_obj.uiAviomtrKickoffOvps) {
        VPP_AVIOMTR_kickOff(vpp_obj, VPP_AVIOMTR_CONFIG_OVPS);
    }

    /* Start OVP IN/OP TG*/
    OVPS_TG_start(vpp_obj);

    return;
}

static void OVPS_BCMBuf_Swap(VPP_OVP_OBJ *ovp_obj)
{
    /* switch BCM buffer for the CPCB */
    if (ovp_obj->pOvpSclClearBcmBuf == &ovp_obj->ovpscl_clear_bcm_buf[0])
        ovp_obj->pOvpSclClearBcmBuf = &ovp_obj->ovpscl_clear_bcm_buf[1];
    else
        ovp_obj->pOvpSclClearBcmBuf = &ovp_obj->ovpscl_clear_bcm_buf[0];

    if (ovp_obj->pOvpSclBcmBuf == &ovp_obj->ovpscl_bcm_buf[0])
        ovp_obj->pOvpSclBcmBuf = &ovp_obj->ovpscl_bcm_buf[1];
    else
        ovp_obj->pOvpSclBcmBuf = &ovp_obj->ovpscl_bcm_buf[0];

    if (ovp_obj->curr_ovpscl_dma_cfgQ == &ovp_obj->ovpscl_dma_cfgQ[0])
        ovp_obj->curr_ovpscl_dma_cfgQ = &(ovp_obj->ovpscl_dma_cfgQ[1]);
    else
        ovp_obj->curr_ovpscl_dma_cfgQ = &(ovp_obj->ovpscl_dma_cfgQ[0]);

    if (ovp_obj->curr_ovpscl_bcm_cfgQ == &ovp_obj->ovpscl_bcm_cfgQ[0])
        ovp_obj->curr_ovpscl_bcm_cfgQ = &(ovp_obj->ovpscl_bcm_cfgQ[1]);
    else
        ovp_obj->curr_ovpscl_bcm_cfgQ = &(ovp_obj->ovpscl_bcm_cfgQ[0]);
}

static void OVPS_BCMBuf_Reset(VPP_OVP_OBJ *ovp_obj)
{
    /* reset vbi bcm & dma cfgQ length */
    ovp_obj->curr_ovpscl_bcm_cfgQ->len = 0;
    ovp_obj->curr_ovpscl_dma_cfgQ->len = 0;

    /* select BCM buffer for the CPCB */
    OVPS_BCMBUF_Select(ovp_obj->pOvpSclBcmBuf, CPCB_1);

    /* select CLEAR BCM buffer for the CPCB */
    OVPS_BCMBUF_Select(ovp_obj->pOvpSclClearBcmBuf, CPCB_1);

}

static int OVPS_BCMBuf_Prepare(VPP_OVP_OBJ *ovp_obj)
{
    int rc;

    rc = OVPS_BCMBUF_To_CFGQ(ovp_obj->pOvpSclClearBcmBuf, ovp_obj->curr_ovpscl_bcm_cfgQ);
    if (rc != MV_VPP_OK) {
        ovps_printf(OVPS_ERR, "pOvpSclClearBcmBuf Commit failed\n");
    }

    /* generate CPCB-0 BCM DHUB cmdQ and,*/
    rc = OVPS_BCMBUF_To_CFGQ(ovp_obj->pOvpSclBcmBuf, ovp_obj->curr_ovpscl_bcm_cfgQ);
    if (rc != MV_VPP_OK) {
        ovps_printf(OVPS_ERR, "pOvpSclBcmBuf Commit failed\n");
    }
    rc = OVPS_CFGQ_To_CFGQ(ovp_obj->curr_ovpscl_dma_cfgQ, ovp_obj->curr_ovpscl_bcm_cfgQ);
    if (rc != MV_VPP_OK) {
        ovps_printf(OVPS_ERR, "curr_ovpscl_dma_cfgQ Commit failed\n");
    }
    return rc;
}

void OVPS_Process(VPP_OBJ *vpp_obj, bool isSkipUpdateFE)
{
    HRESULT rc = S_OK;
    int chanID,planeID;
    PLANE *plane;
    VPP_OVP_OBJ *ovp_obj = &g_vpp->vpp_objs[0]->ovp_obj;

    if (vpp_obj->status != STATUS_ACTIVE)
        return;

    /*If there is pending BCM request in VPP, wait for it to complete*/
    if ((ovp_obj->m_BCM_Sched == BCM_SCHED_Q0) && (ovp_obj->m_vpp_sched_request))
        return;

    plane = &ovp_obj->plane;
    chanID = CHAN_OVP_EL;
    planeID = PLANE_OVP_EL;

    if (plane == NULL) {
        ovps_printf(OVPS_ERR, "Invalid plane\n");
        return;
    }
    MV_Timer_Stop(&ovp_obj->ovps_monitor);

    if (!isSkipUpdateFE) {
        OVPs_updateFEChannelCurrentFrame(vpp_obj, chanID, planeID);
    }

    if (plane->curr_frame == NULL) {
        if (ovp_obj->suspend) {
            MV_OSAL_Sem_Post(ovp_obj->ovps_sem_handle);
        }
        ovps_printf(OVPS_MID, "Input Q is empty\n");
        ovp_obj->in_progress = 0;
        return;
    } else {
        ovp_obj->in_progress = 1;
    }

    /*Submit OVPS BCM Buffer request to VPP*/
    if (ovp_obj->m_BCM_Sched == BCM_SCHED_Q0) {
        ovp_obj->m_vpp_sched_request = 1;
        OVPS_BCMBuf_Swap(ovp_obj);
    }

    OVPS_BCMBuf_Reset(ovp_obj);

    /*Clear dHub in case of new/resubmit request*/
    if (isSkipUpdateFE)
        OVPS_dHub_Clear(vpp_obj);

    OVPSCL_ISR_service(vpp_obj, CPCB_1);

    if (ovp_obj->m_BCM_Sched == BCM_SCHED_Q12) {
        OVPS_BCMBuf_Prepare(ovp_obj);

        /*Q12 is shared between VPP and OVPS*/
        MV_OSAL_Mutex_Lock(vpp_obj->bcmbuf_lock);
        rc = OVPS_BCMDHUB_CFGQ_Commit( ovp_obj->curr_ovpscl_bcm_cfgQ,CPCB_1,0);
        MV_OSAL_Mutex_Unlock(vpp_obj->bcmbuf_lock);
        if (rc != MV_VPP_OK) {
            ovps_printf(OVPS_ERR, "curr_ovpscl_bcm_cfgQ Commit failed\n");
        }
        OVPS_BCMBuf_Swap(ovp_obj);

        /* whenever set format is called, few Interrupt(VBI,VDE,OVPS) delays
         * around ~100msec to ~50msec which cause for OVPS moitor timeout
         * So avoided timer start for few VBINUM(OVPS_SKIP_VBINUM_COUNT)
         */
        if (vpp_obj->dv[CPCB_1].vbi_num > OVPS_SKIP_VBINUM_COUNT) {
            MV_Timer_Start(&ovp_obj->ovps_monitor);
        }
    }
}

