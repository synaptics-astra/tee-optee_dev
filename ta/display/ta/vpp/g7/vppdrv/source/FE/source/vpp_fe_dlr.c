/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_FE_DLR_C_

#include "Galois_memmap.h"
#include "mtr.h"
#include "vpp_regs.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_fe_dlr.h"
#include "vpp_fe_dlr_prv.h"
#include "vpp_cpcb_tg.h"
#include "vpp_be.h"
#include "vpp_be_hdr.h"
#include "vpp_frc_scl.h"
#include "api_dhub.h"
//FIXME: need to be removed when api's move
#include "vpp_csc_icsc.h"
#include "vpp_cfg.h"
#include "vpp_clkGate.h"
#include "vpp_dv_api.h"

#define VPP_IS_UPS_422_to_444_NEEDED(src_fmt)        (VPP_IS_420SP_FMT(src_fmt) || \
                                                        (src_fmt == SRCFMT_YUV422)) ?1 :0


#define CONFIG_MV_AMP_VPP_UPS_420_422_SFT_RST

#ifndef CONFIG_AMP_VPP_INVERSE_SCAN
static const INT8 argb32_bitmap_table[ORDER_ARGB32_MAX][32] = {
    {//Order
        24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4 ,5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,   //ORDER_ARGB
    },
    {
        24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19, 20, 21, 22, 23, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4 ,5, 6, 7,   //ORDER_ABGR
    },
    {
        0, 1, 2, 3, 4 ,5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,   //ORDER_RGBA
    },
    {
        16, 17, 18, 19, 20, 21, 22, 23, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4 ,5, 6, 7, 24, 25, 26, 27, 28, 29, 30, 31,   //ORDER_BGRA
    },
};
#endif

BOOL isTiledFormat(UINT32 fmt)
{
    BOOL rc = FALSE;
    switch(fmt) {
        case SRCFMT_YUV420SP_TILED_MODE0:
        case SRCFMT_YUV420SP_TILED_MODE1:
#ifdef CONFIG_AMP_VPP_ROTATION
        case SRCFMT_YUV420SP_TILED_V4H8:
#endif
            rc = TRUE;
            break;
        default:
            break;
    }
    return rc;
}

static const INT COLOR_PRIM_TO_CSC_MODE[VPP_COLOR_PRIM_MAX] = {
    VPP_CSC_YUV_709_TO_sRGB,
    VPP_CSC_YUV_709_TO_sRGB,
    VPP_CSC_YUV_709_TO_sRGB,
    VPP_CSC_YUV_709_TO_sRGB,
    VPP_CSC_YUV_601_TO_sRGB,
    VPP_CSC_YUV_601_TO_sRGB,
    VPP_CSC_YUV_601_TO_sRGB,
    VPP_CSC_YUV_601_TO_sRGB,
    VPP_CSC_YUV_709_TO_sRGB,
    VPP_CSC_YUV_2020_TO_sRGB
};

/***************************************************************************************
 * FUNCTION: load default values of data loader to BCM buffer
 * PARAMS:  Channel - indicate the channel
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_LoadDefaultVal(VPP_OBJ *vpp_obj, INT Channel)
{
    UINT32 RegAddr;

    if((Channel < VPP_FE_DLR_CHANNEL_MIN) || (Channel > VPP_FE_DLR_CHANNEL_MAX))
        return MV_VPP_EBADPARAM;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_CTRL0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->clientif_ctrl0.u32);//TODO: mirroring required???

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_gfxScl; //Mirroring required as scl start clear also in this reg
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_gfxscl.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_gfxScl;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx1_gfxscl.u32);
#ifdef ENABLE_GFX2_VOP2_PATH
    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_gfxScl;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx2_gfxscl.u32);
#endif
    //GFX1 in ARGB
    //T32GFX1_CTRL gfx1_ctrl; -- Store it in vpp_obj for later use in setBitMap
    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx1_ctrl.u32);
    vpp_obj->gfx1_ctrl.uCTRL_bitmap40_en = 0;
    vpp_obj->gfx1_ctrl.uCTRL_hdrLut_bypass = 0;
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->gfx1_ctrl.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL1;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx1_ctrl1.u32);
    vpp_obj->gfx1_ctrl1.uCTRL1_DBVIP_GFX1_bypass = 1;
    /* To fix 1080i sawtooth issue. This needs to be improved by enabling only if interlaced frame. */
    if(Channel == VPP_FE_DLR_CHANNEL_GFX1_0){
        /* GFX: Graphics Scalar Clock Enable control : fall back option for previous logic */
        vpp_obj->gfx1_ctrl1.uCTRL1_fall_bk_en = 1;
    }
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->gfx1_ctrl1.u32);
#ifdef ENABLE_OVPSCL
    if(Channel == VPP_FE_DLR_CHANNEL_MAIN_0) {
        T32OVPSCLWRAP_CTRL ovpsclwrap_ctrl;
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL;
        VPP_REG_WORD32_READ(RegAddr, &ovpsclwrap_ctrl.u32);
        ovpsclwrap_ctrl.uCTRL_ovpscl_ictrl_sel = 6; /*Select OVPS by default to avoid issues*/
        VPP_REG_WORD32_WRITE(RegAddr, ovpsclwrap_ctrl.u32);
    }
#endif
#ifdef ENABLE_GFX2_VOP2_PATH
    //GFX2 in ARGB
    //T32GFX1_CTRL gfx1_ctrl; -- Store it in vpp_obj for later use in setBitMap
    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_CTRL;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx2_ctrl.u32);
    vpp_obj->gfx2_ctrl.uCTRL_bitmap40_en = 0;
    vpp_obj->gfx2_ctrl.uCTRL_hdrLut_bypass = 1;
    //vpp_obj->gfx2_ctrl.uCTRL_fifo_sts_ctrl = 1;
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->gfx2_ctrl.u32);

    VPP_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_CTRL1, &vpp_obj->gfx2_ctrl1.u32);
#endif
    //PIP in 422
    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL1;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_ctrl1.u32);

    vpp_obj->gfx0_ctrl1.uCTRL1_hdrLut_bypass = 0;
    /* To fix 1080i sawtooth issue. This needs to be improved by enabling only for interlaced frame. */
    if(Channel == VPP_FE_DLR_CHANNEL_PIP_0 || Channel == VPP_FE_DLR_CHANNEL_PIP_1) {
        /* PIP: Graphics Scalar Clock Enable control : fall back option for previous logic */
        vpp_obj->gfx0_ctrl1.uCTRL1_fall_bk_en = 1;
    }
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->gfx0_ctrl1.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_ctrl0.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_PIP_PIPE + RA_PIP_PIPE_CTRL1;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_pip_pipe_ctrl1.u32);
    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_ctrl.u32);//TODO: mirroring required???
    vpp_obj->gfx0_ctrl.uCTRL_bitmap40_en = 0;
    vpp_obj->gfx0_ctrl.uCTRL_ups8_bypass = 0;
    vpp_obj->gfx0_ctrl.uCTRL_csc_bypass = 0;
    vpp_obj->gfx0_ctrl.uCTRL_fifo_sts_ctrl = 1;
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_VPP_ENABLE_PLANE_CLKGATE)
    if(IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE)) {
        vpp_obj->gfx0_ctrl.uCTRL_ups8_bypass = 1;
        vpp_obj->gfx0_ctrl.uCTRL_csc_bypass = 1;
    }
    vpp_obj->gfx0_ctrl.uCTRL_pip_gfx_en = 0;//for argb32
#else
    vpp_obj->gfx0_ctrl.uCTRL_pip_gfx_en = 1;//for yuv422
#endif
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->gfx0_ctrl.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_CTRL;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_ups420_ctrl.u32);
    vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_ups420_bypass = 0;
    vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_ups422_data_ctrl = 1;
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->gfx0_ups420_ctrl.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_DB_CTRL;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_DB_ctrl.u32);//TODO: Review once DB specific code inplace
    vpp_obj->gfx0_DB_ctrl.uDB_CTRL_DBVIP_GFX0_bypass = 1;
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->gfx0_DB_ctrl.u32);

    //MAIN plane
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->mainpl_ctrl4.u32);
    vpp_obj->mainpl_ctrl4.uCTRL4_UPS420_4K_bypass = 0;
    vpp_obj->mainpl_ctrl4.uCTRL4_adpScl_bypass = 0;
    vpp_obj->mainpl_ctrl4.uCTRL4_bitmap40_bypass = 1;
    vpp_obj->mainpl_ctrl4.uCTRL4_vmxIP_bypass = 1;
    vpp_obj->mainpl_ctrl4.uCTRL4_NGPTVIP_bypass = 1;
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->mainpl_ctrl4.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y);
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C);
#ifdef ENABLE_EL_PIPE_PATH
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y);
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C);
#endif

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_SP + RA_UPS_420_422_SP_CFG0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0);
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_VPP_ENABLE_PLANE_CLKGATE)
    vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0 |= MSK32UPS_420_422_SP_CFG0_enable;
#else
    vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0 &= ~MSK32UPS_420_422_SP_CFG0_enable;
#endif
    VPP_REG_WORD32_WRITE(RegAddr, vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0);

#ifdef ENABLE_EL_PIPE_PATH
    /* Backup of UPS CFG registers for PIP*/
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG6;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_Y);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG6;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_C);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG10;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].m_uiTileCfg10_Y);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG10;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].m_uiTileCfg10_C);
#endif

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->mainplCtrl0.u32);
#ifdef ENABLE_PIP_GFX0_PATH
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->stElPipeCtrl1.u32);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL2;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->stElPipeCtrl2.u32);
#endif
    /*  Backup of UPS CFG registers for MAIN */
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG6;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].m_uiTileCfg6_Y);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG6;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].m_uiTileCfg6_C);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG10;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].m_uiTileCfg10_Y);

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG10;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].m_uiTileCfg10_C);

#ifdef ENABLE_RASTERSCAN
    //Read RasterScan Ctrl1 register.
    GA_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_CTRL1, &vpp_obj->uiRasterScanRdCtrl1);
#endif

    VPP_DV_FE_CompLoadDefaultVal(vpp_obj, Channel);

    return MV_VPP_OK;
}

/// %unsigned 3  ups420_idat_ctrl          0x0
///        ###
///        * Data control enable for ups420_422
///        * [000] : Input data is 10 bpc (YUV420, YUV444, RGB101010)
///        * [001] : Input data is 8bpc(YUV420, YUV444,    RGB888)
///        * [010] : For DV TEST path (8bpc)
///        * [011] : For DV TEST path (10bpc)
///        * [100] : Input data is YUV 422 8bpc
///        * [101] : Input data is YUV 422 10bpc
///        * [110] : Input data is YUV 422 12bpc
///        * [111] : Input data is ARGB2101010 /ARGB8888/ YUV444 DWA
UINT8 INTERNAL_FE_DLR_GetUPS420DataCtrl(UINT8 src_fmt, UINT8 BitsPerPixel)
{
    UINT8 idat_ctrl = 0;

    if(src_fmt == SRCFMT_YUV422) {
        if(BitsPerPixel == 8 || BitsPerPixel == 16) {
            idat_ctrl = 4;
        }
        else if(BitsPerPixel == 10 || BitsPerPixel == 20) {
            idat_ctrl = 5;
        }
        else {
            idat_ctrl = 6;
        }
    }
    else if((src_fmt == SRCFMT_YUV420SP) || \
            (src_fmt == SRCFMT_YUV420SP_TILED_MODE0) || \
            (src_fmt == SRCFMT_YUV420SP_TILED_MODE1) || \
            (src_fmt == SRCFMT_YUV420SP_TILED_V4H8) || \
            (src_fmt == SRCFMT_YUV420SP_TILED_AUTO_MODE) || \
            (src_fmt == SRCFMT_YUV420SP_DWA)) {
        if(BitsPerPixel == 8)
            idat_ctrl = 1;
        else
            idat_ctrl = 0;
    }
    else if((src_fmt == SRCFMT_RGB888) || (src_fmt == SRCFMT_YUV444)) {
        if((BitsPerPixel == 8) || (BitsPerPixel == 24))
            idat_ctrl = 1;
        else
            idat_ctrl = 0;
    }
    else if((src_fmt == SRCFMT_ARGB32) || (src_fmt == SRCFMT_ARGB32_PM) || \
            (src_fmt == SRCFMT_ARBG2101010) || (src_fmt == SRCFMT_YUV444_DWA)) {
        idat_ctrl = 7;
    }
    return idat_ctrl;
}

UINT8 INTERNAL_FE_DLR_GetBitDepth(UINT8 src_fmt, UINT8 BitsPerPixel)
{

    if(src_fmt==SRCFMT_YUV422) {
        if(BitsPerPixel == 16 || BitsPerPixel == 8)
            return INPUT_BIT_DEPTH_8BIT;
        else if(BitsPerPixel == 20 || BitsPerPixel == 10)
            return INPUT_BIT_DEPTH_10BIT;
    } else if(src_fmt==SRCFMT_YUV444) {
        if(BitsPerPixel == 24)
            return INPUT_BIT_DEPTH_8BIT;
        else if(BitsPerPixel == 30)
            return INPUT_BIT_DEPTH_10BIT;
    } else if(src_fmt==SRCFMT_YUV420SP || src_fmt==SRCFMT_YUV420SP_TILED_MODE0 || src_fmt==SRCFMT_YUV420SP_TILED_MODE1 || \
        src_fmt==SRCFMT_YUV420SP_DWA || src_fmt==SRCFMT_YUV422SP_DWA || src_fmt == SRCFMT_YUV444) {
        if(BitsPerPixel==8)
            return INPUT_BIT_DEPTH_8BIT;
        else if(BitsPerPixel==10)
            return INPUT_BIT_DEPTH_10BIT;
    } else if(IS_GFX_SRC_FORMAT(src_fmt)) {
        if(BitsPerPixel == 32 || BitsPerPixel == 16 || BitsPerPixel == 24)
            return INPUT_BIT_DEPTH_8BIT;
        else if((BitsPerPixel == 40) || (BitsPerPixel == 30))
            return INPUT_BIT_DEPTH_10BIT;
    }
#ifdef CONFIG_AMP_VPP_ROTATION
    else if(SRCFMT_YUV420SP_TILED_V4H8 == src_fmt)
        return INPUT_BIT_DEPTH_8BIT;
#endif
    else if (SRCFMT_YUV444_DWA == src_fmt)
        return INPUT_BIT_DEPTH_32BIT;

    return INPUT_BIT_DEPTH_8BIT;
}

UINT8 INTERNAL_FE_DLR_GetPackSel(INT Channel, UINT8 srcFmt, UINT8 BitsPerPixel)
{
    UINT8 retval = 0;
    UINT8 main_channel_0_packsel_list[] = {8, 10, 16, 10, 20, 32,24,30};
    UINT8 main_channel_1_packsel_list[] = {8, 10, 10, 16};
    UINT8 pip_channel_0_packsel_list[] = {8, 16, 32, 10, 10, 20, 30, 24};
    UINT8 pip_channel_1_packsel_list[] = {8, 10, 12, 15, 16, 20, 24, 30, 32};
    UINT8 gfx1_channel_0_packsel_list[] = {8, 16, 32, 24};
    UINT8 gfx2_channel_0_packsel_list[] = {8, 16, 32, 24};
    UINT8 readclient_channel_packsel_list[] = {8, 10, 12, 15, 16, 20, 24, 30, 32};
    INT count, size;
    UINT8 *bit_per_pixel_supported = NULL;

    /* Since there are different packsel for same BitsPerPixel; excetptions are handled differently */
    if(((srcFmt == SRCFMT_YUV420SP_TILED_MODE0) && (10 == BitsPerPixel)) || \
            srcFmt==SRCFMT_YUV420SP_DWA || srcFmt==SRCFMT_YUV422SP_DWA) {
        /* V4H6 cases */
        if(Channel == VPP_FE_DLR_CHANNEL_MAIN_0)
            return 3;
        else if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1)
            return 2;

    }
    //if(srcFmt==SRCFMT_YUV420SP_TILED_MODE0 && INPUT_BIT_DEPTH_10BIT==INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel))
        //BitsPerPixel = 32; //for tiled 10bit mode packsel is 32bit
    if (srcFmt==SRCFMT_YUV444_DWA) {
        if(Channel == VPP_FE_DLR_CHANNEL_MAIN_0)
            return 5;
        if(Channel == VPP_FE_DLR_CHANNEL_PIP_0)
            return 2;
    }

    if(Channel == VPP_FE_DLR_CHANNEL_MAIN_0){
        bit_per_pixel_supported = main_channel_0_packsel_list;
        size = sizeof(main_channel_0_packsel_list)/sizeof(UINT8);
    } else if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1) {
        bit_per_pixel_supported = main_channel_1_packsel_list;
        size = sizeof(main_channel_1_packsel_list)/sizeof(UINT8);
    } else if(Channel == VPP_FE_DLR_CHANNEL_PIP_0) {
        bit_per_pixel_supported = pip_channel_0_packsel_list;
        size = sizeof(pip_channel_0_packsel_list)/sizeof(UINT8);
    } else if(Channel == VPP_FE_DLR_CHANNEL_PIP_1) {
        bit_per_pixel_supported = pip_channel_1_packsel_list;
        size = sizeof(pip_channel_1_packsel_list)/sizeof(UINT8);
    } else if(Channel == VPP_FE_DLR_CHANNEL_GFX1_0) {
        bit_per_pixel_supported = gfx1_channel_0_packsel_list;
        size = sizeof(gfx1_channel_0_packsel_list)/sizeof(UINT8);
    } else if(Channel == VPP_FE_DLR_CHANNEL_GFX2_0) {
        bit_per_pixel_supported = gfx2_channel_0_packsel_list;
        size = sizeof(gfx2_channel_0_packsel_list)/sizeof(UINT8);
    } else {
        bit_per_pixel_supported = readclient_channel_packsel_list;
        size = sizeof(readclient_channel_packsel_list)/sizeof(UINT8);
    }

    if ((srcFmt==SRCFMT_YUV420SP) && (BitsPerPixel==10))
        if(Channel == VPP_FE_DLR_CHANNEL_PIP_0)
            return 4;

    if ((srcFmt==SRCFMT_YUV422) && (BitsPerPixel==20))
        if(Channel == VPP_FE_DLR_CHANNEL_PIP_0)
            return 5;

    if (((srcFmt==SRCFMT_YUV420SP_TILED_MODE0) && (BitsPerPixel==10))||
            srcFmt==SRCFMT_YUV420SP_DWA || srcFmt==SRCFMT_YUV422SP_DWA)
        if(Channel == VPP_FE_DLR_CHANNEL_PIP_1)
            return 2;

    for(count=0; count<size; count++){
        if(BitsPerPixel == bit_per_pixel_supported[count])
            return count;
        }

    return retval;
}


/***************************************************************************************
 * FUNCTION: set the size of the plane.
 * PARAMS:  Channel - id of a front-end dataloader channel.
 *                pPlaneSize - pointer to the struct of horizontal and vertial size.
 *                CropWpl - beat per line and crop
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetClientSize(VPP_OBJ *vpp_obj, INT chanID, INT Channel, PVPP_FE_DLR_INPUT_RES pReadSize, PVPP_FE_DLR_INPUT_RES pPlaneSize, UINT8 srcFmt, UINT8 BitsPerPixel) //DONE
{
    UINT32 RegAddr = 0;
    UINT32 RegSizeVal, HeightRead = 0, WidthRead =0;
    INT RetVal = MV_VPP_OK;
    int planeID = -1;

    /*TODO: change this to pixtotal??*/
    if((Channel < VPP_FE_DLR_CHANNEL_MIN) || (Channel > VPP_FE_DLR_CHANNEL_MAX))
        return MV_VPP_EBADPARAM;

    /* TODO: handle invalid channels with vpp_channel_map[Channel].is_valid */

    planeID = PLANE_OF_CHAN(vpp_obj, chanID);

    if(Channel == VPP_FE_DLR_CHANNEL_MAIN_0)
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_MR0_word;
    else if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1)
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_MR1_word;
#ifdef ENABLE_PIP_GFX0_PATH
    else if(Channel == VPP_FE_DLR_CHANNEL_PIP_0)
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_Rd_word0;
    else if(Channel == VPP_FE_DLR_CHANNEL_PIP_1)
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_RdClientPipR1 + RA_ReadClient_Word;
#endif
    else if(Channel == VPP_FE_DLR_CHANNEL_GFX1_0)
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_Rd_word0;
#ifdef ENABLE_GFX2_VOP2_PATH
    else if(Channel == VPP_FE_DLR_CHANNEL_GFX2_0)
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_Rd_word0;
#endif
    else if(Channel == VPP_FE_DLR_CHANNEL_VMX_0)
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_RdClientVmxVm + RA_ReadClient_Word;
    if(!OVPS_PARAMS->readTiledfromOVPclient) {
#if defined(RA_VPTOP_CTRL_RdClientdiR0) && defined(RA_VPTOP_CTRL_RdClientdiR1)
        if(Channel == VPP_FE_DLR_CHANNEL_OVP_EL_0)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_Word;
        else if(Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_Word;
#endif
    }
#ifdef ENABLE_EL_PIPE_PATH
    else {
        if(Channel == VPP_FE_DLR_CHANNEL_OVP_EL_0)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL3;
        else if(Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1)
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL4;
    }
#endif

    //FIXME: word calculation for sp hardcoded need to fix this
    if(vpp_obj->plane[planeID].m_SampleCropEnable &&
            ((PLANE_PIP == planeID) || (PLANE_MAIN == planeID) || (PLANE_OVP_EL == planeID))) {
#ifdef ENABLE_OVP
        if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1 ||
                Channel == VPP_FE_DLR_CHANNEL_PIP_1 ||
                Channel == VPP_FE_DLR_CHANNEL_MAIN_3 ||
                Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1)
#else
        if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1 ||
                Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1 ||
                Channel == VPP_FE_DLR_CHANNEL_PIP_1)
#endif
        {
            if((vpp_obj->plane[PLANE_PIP].m_SampleCropEnable&& PLANE_PIP == planeID)
                    ||(vpp_obj->plane[PLANE_MAIN].m_SampleCropEnable&& PLANE_MAIN == planeID)
                    ||(vpp_obj->plane[PLANE_OVP_EL].m_SampleCropEnable&& PLANE_OVP_EL == planeID)) {
                if(Channel == VPP_FE_DLR_CHANNEL_PIP_1) {
                    WidthRead = vpp_obj->plane[PLANE_PIP].m_Aligned_Chroma_Width;
                    HeightRead  = vpp_obj->plane[PLANE_PIP].m_Aligned_Luma_Height/2;
                } else if(Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1) {
                    WidthRead = vpp_obj->plane[PLANE_OVP_EL].m_Aligned_Chroma_Width;
                    HeightRead  = vpp_obj->plane[PLANE_OVP_EL].m_Aligned_Luma_Height/2;
                } else {
                    WidthRead = vpp_obj->plane[PLANE_MAIN].m_Aligned_Chroma_Width;
                    HeightRead  = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Height/2;
                }
            }

            if(srcFmt==SRCFMT_YUV420SP_TILED_MODE0 && INPUT_BIT_DEPTH_10BIT==INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel)) {
                RegSizeVal = (WidthRead * HeightRead * BitsPerPixel * 16 / 15)/(128);
            } else {
                RegSizeVal = (((WidthRead * BitsPerPixel) + 127)/128) * HeightRead;
            }
        } else {
            if((vpp_obj->plane[PLANE_PIP].m_SampleCropEnable&& PLANE_PIP == planeID)
                    ||(vpp_obj->plane[PLANE_MAIN].m_SampleCropEnable&& PLANE_MAIN == planeID)
                    ||(vpp_obj->plane[PLANE_OVP_EL].m_SampleCropEnable&& PLANE_OVP_EL == planeID)) {
                if(Channel == VPP_FE_DLR_CHANNEL_PIP_0) {
                    WidthRead = vpp_obj->plane[PLANE_PIP].m_Aligned_Luma_Width;
                    HeightRead =  vpp_obj->plane[PLANE_PIP].m_Aligned_Luma_Height;
                } else if(Channel == VPP_FE_DLR_CHANNEL_OVP_EL_0) {
                    WidthRead = vpp_obj->plane[PLANE_OVP_EL].m_Aligned_Luma_Width;
                    HeightRead =  vpp_obj->plane[PLANE_OVP_EL].m_Aligned_Luma_Height;
                } else {
                    WidthRead = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Width;
                    HeightRead = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Height;
                }
            }
            if(srcFmt==SRCFMT_YUV420SP_TILED_MODE0 && INPUT_BIT_DEPTH_10BIT==INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel)) {
                RegSizeVal = (WidthRead * HeightRead * BitsPerPixel * 16 / 15)/128;
            } else {
                RegSizeVal = (((WidthRead * BitsPerPixel)+127)/128) * HeightRead;
            }
        }
    } else {
#ifdef ENABLE_OVP
        if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1 || Channel == VPP_FE_DLR_CHANNEL_PIP_1 || Channel == VPP_FE_DLR_CHANNEL_MAIN_3
                || Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1)
#else
        if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1 || Channel == VPP_FE_DLR_CHANNEL_PIP_1 || Channel == VPP_FE_DLR_CHANNEL_OVP_EL_1)
#endif
        {
            if((srcFmt==SRCFMT_YUV420SP_TILED_MODE0 && INPUT_BIT_DEPTH_10BIT==INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel))
                    || srcFmt==SRCFMT_YUV420SP_DWA)
                RegSizeVal = (pReadSize->HRes * pReadSize->VRes * BitsPerPixel * 16 / 15)/(128*2);
            else if (srcFmt==SRCFMT_YUV422SP_DWA)
                RegSizeVal = (pReadSize->HRes * pReadSize->VRes * BitsPerPixel * 16 / 15)/(128);
            else
                    RegSizeVal = (((pReadSize->HRes * BitsPerPixel)+127)/128) * (pReadSize->VRes/2);
        }else {
            if(((srcFmt == SRCFMT_YUV420SP_TILED_MODE0) && (INPUT_BIT_DEPTH_10BIT == INTERNAL_FE_DLR_GetBitDepth(srcFmt, BitsPerPixel))) \
                    || (srcFmt == SRCFMT_YUV420SP_DWA) || (srcFmt == SRCFMT_YUV422SP_DWA))
                RegSizeVal = (pReadSize->HRes * pReadSize->VRes * BitsPerPixel * 16 / 15)/128;
            else
                    RegSizeVal = (((pReadSize->HRes * BitsPerPixel)+127)/128) * pReadSize->VRes;
        }
    }
    if(Channel == VPP_FE_DLR_CHANNEL_VMX_0) {
        RegSizeVal = (((pReadSize->HRes * BitsPerPixel)+127)/128) * pReadSize->VRes;
    }

#ifdef ENABLE_GFX2_VOP2_PATH
    if(Channel == VPP_FE_DLR_CHANNEL_GFX2_0) {
        RegSizeVal = RegSizeVal*2; //64 Bit Dhub
    }
#endif
    /*write plane size to BCM buffer*/
    if(RegAddr != 0) {
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegSizeVal);
    }

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set the input size in pixels for DBVIP
 * PARAMS:  id - id of a channel.
 *                pDBVIPSize - pointer to the struct of horizontal and vertial size.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/

INT FE_DLR_SetHDRInputSize(VPP_OBJ *vpp_obj, INT id, PVPP_FE_DLR_INPUT_RES pDBVIPSize)
{
    UINT32 RegAddrDcm = 0;
    INT RetVal = MV_VPP_OK;
    T32VDCM_RESO dcm_res;
    dcm_res.u32 = 0;
    switch(id) {
        case VPP_FE_DLR_MAIN:
            RegAddrDcm = vpp_obj->dcm_base_addr_mp + RA_VPP_MP_VDCM + RA_VDCM_RESO;
            break;
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FE_DLR_PIP:
            RegAddrDcm = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_VPP_GDCM_GFX0 + RA_GDCM_RESO;
            break;
#endif
        case VPP_FE_DLR_GFX1:
            RegAddrDcm = vpp_obj->dcm_base_addr_gfx + RA_VPP_GFX_VOP_VPP_GDCM_GFX1 + RA_GDCM_RESO;
            break;
          default:
            return RetVal;
            break;
    }

    SET32VDCM_RESO_HORZ(dcm_res.u32,pDBVIPSize->HRes);
    SET32VDCM_RESO_VERT(dcm_res.u32,pDBVIPSize->VRes);
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddrDcm,dcm_res.u32)))
        return RetVal;

    VPP_DV_FE_SetInputSize(vpp_obj, id, pDBVIPSize);

    return MV_VPP_OK;
}

INT FE_DLR_SetPlaneSize(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pReadSize, PVPP_FE_DLR_INPUT_RES pPlaneSize, UINT8 srcFmt, UINT8 BitsPerPixel)
{
    UINT32 RegAddr;
    UINT32 RegSizeVal;
    VPP_FE_DLR_INPUT_RES res;
    INT RetVal = MV_VPP_OK;

    if(ldr_id == VPP_FE_DLR_MAIN){ //FIXME: hack
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL3;
        RegSizeVal = pPlaneSize->HRes * pPlaneSize->VRes;
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegSizeVal)))
            return RetVal;

        /* Programming shared line buffer memory selection */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0;
        if(pReadSize->HRes > 1920) {
            vpp_obj->mainplCtrl0.uCTRL0_memlb_mode = 1;
        } else {
            vpp_obj->mainplCtrl0.uCTRL0_memlb_mode = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->mainplCtrl0.u32);
    }
    if(ldr_id == VPP_FE_DLR_PIP){ //FIXME: hack
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_gfx_pix;
        if(IS_GFX_SRC_FORMAT(srcFmt)) {
            //if it is graphics plane, then input to scalar fifo is same as readsize, since we have to crop inside scalar
            RegSizeVal = pReadSize->HRes * pReadSize->VRes;
        } else {
            RegSizeVal = pPlaneSize->HRes * pPlaneSize->VRes;
        }
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegSizeVal)))
            return RetVal;
        if((SRCFMT_YUV420SP_TILED_MODE0 == srcFmt) || (SRCFMT_YUV420SP_TILED_MODE1 == srcFmt) || (SRCFMT_YUV420SP_TILED_V4H8 == srcFmt)) {
            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_pipInpix;
            T32GFX0_pipInpix stGfx0PipInPix;
            stGfx0PipInPix.u32 = 0;

            stGfx0PipInPix.upipInpix_tot = pPlaneSize->HRes * pPlaneSize->VRes;
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stGfx0PipInPix.u32)))
                return RetVal;
        }

    }
    if(ldr_id == VPP_FE_DLR_GFX1){ //FIXME: hack
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_gfx_pix;
        RegSizeVal = pReadSize->HRes * pReadSize->VRes;

        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegSizeVal)))
            return RetVal;
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    if(ldr_id == VPP_FE_DLR_GFX2){
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_gfx_pix;
        RegSizeVal = pReadSize->HRes * pReadSize->VRes;
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegSizeVal)))
            return RetVal;
    }
#endif
    if((ldr_id == VPP_FE_DLR_MAIN) || ((ldr_id == VPP_FE_DLR_PIP) && (!IS_GFX_SRC_FORMAT(srcFmt)))) {
        res.HRes = pPlaneSize->HRes;
        res.VRes = pPlaneSize->VRes;
    } else if((ldr_id == VPP_FE_DLR_GFX1) || (IS_GFX_SRC_FORMAT(srcFmt) && (ldr_id == VPP_FE_DLR_PIP))) {
        res.HRes = pReadSize->HRes;
        res.VRes = pReadSize->VRes;
    }
    FE_DLR_SetHDRInputSize(vpp_obj, ldr_id, &res);

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set input data format and order.
 * PARAMS:  Channel - id of a front-end dataloader channel.
 *                pDataFmt - pointer to the struct of data format and order.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetPlanePackSel(VPP_OBJ *vpp_obj, INT Channel, UINT8 srcFmt, UINT8 BitsPerPixel)
{
    UINT32 RegAddr = 0;
    INT RetVal = MV_VPP_OK;
    UINT32 RegPackselVal;

    if((Channel < VPP_FE_DLR_CHANNEL_MIN) || (Channel > VPP_FE_DLR_CHANNEL_MAX))
        return MV_VPP_EBADPARAM;
    if(Channel == VPP_FE_DLR_CHANNEL_PIP_0) {
        if((srcFmt == SRCFMT_YUV420SP)&&(BitsPerPixel==10))
            vpp_obj->gfx0_ctrl1.uCTRL1_gfx0_ovpidat_ctrl = 1;
        else
            vpp_obj->gfx0_ctrl1.uCTRL1_gfx0_ovpidat_ctrl = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL1, vpp_obj->gfx0_ctrl1.u32);
    }
    RegPackselVal = INTERNAL_FE_DLR_GetPackSel(Channel, srcFmt, BitsPerPixel);

    if(Channel == VPP_FE_DLR_CHANNEL_MAIN_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_CTRL0;
        vpp_obj->clientif_ctrl0.uCTRL0_packSel_MR0 = RegPackselVal;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->clientif_ctrl0.u32);
        return RetVal;
    } else if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_CTRL0;
        vpp_obj->clientif_ctrl0.uCTRL0_packSel_MR1 = RegPackselVal;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->clientif_ctrl0.u32);
        return RetVal;
    } else if(Channel == VPP_FE_DLR_CHANNEL_PIP_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL;
        vpp_obj->gfx0_ctrl.uCTRL_packSelR0 = RegPackselVal;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ctrl.u32);
        return RetVal;
    } else if(Channel == VPP_FE_DLR_CHANNEL_PIP_1) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_RdClientPipR1 + RA_ReadClient_pack;
    } else if(Channel == VPP_FE_DLR_CHANNEL_GFX1_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL;
        vpp_obj->gfx1_ctrl.uCTRL_packSelR0 = RegPackselVal;
        /*write input data fromat to BCM buffer*/
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx1_ctrl.u32);
        return RetVal;
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    else if(Channel == VPP_FE_DLR_CHANNEL_GFX2_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_CTRL;
        vpp_obj->gfx2_ctrl.uCTRL_packSelR0 = RegPackselVal;
        /*write input data fromat to BCM buffer*/
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx2_ctrl.u32);
        return RetVal;
    }
#endif
    else if(Channel == VPP_FE_DLR_CHANNEL_VMX_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_RdClientVmxVm + RA_ReadClient_pack;
        RegPackselVal = 0;
    }

    /*write input data format to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, RegPackselVal)))
        return RetVal;

    return RetVal;
}

#ifdef CONFIG_AMP_VPP_INVERSE_SCAN
INT FE_DLR_SetBitmap(VPP_OBJ *vpp_obj, UINT uiPlaneId)
{
    UINT32 RegAddr = 0;
    PLANE *plane = NULL;
    UINT32 uiInvScanMode = 0, uiClientMode = 0, uiSrcOrder = 0, uiBitMapToLoad = 0, uiBitMapId = 0, uiBitMap10_IsNeeded = 0, uiBitMap10Id = 0;

    T32BITMAP40_SEL     stBitMap40Sel = {0};
    T32BITMAP40_SEL1    stBitMap40Sel1 = {0};
    T32BITMAP40_SEL2    stBitMap40Sel2 = {0};
    T32BITMAP40_SEL3    stBitMap40Sel3 = {0};
    T32BITMAP40_SEL4    stBitMap40Sel4 = {0};
    T32BITMAP40_SEL5    stBitMap40Sel5 = {0};
    T32BITMAP40_SEL6    stBitMap40Sel6 = {0};
    T32BITMAP40_SEL7    stBitMap40Sel7 = {0};

    int bitmap_bypass = 0;

    T32MAINPL_CTRL7     stMainPlCtrl7;

    T32BITMAP12_SEL     stBitMap12Sel;
    T32BITMAP12_SEL1     stBitMap12Sel1;
    stBitMap12Sel.u32 = stBitMap12Sel1.u32 = 0;

    stMainPlCtrl7.u32 = vpp_obj->stMainPlCtrl7.u32;

    T32MAINPL_INVSCAN   stMainInvScan;
    T32GFX0_INVSCAN     stGfx0InvScan;
    T32GFX1_INVSCAN     stGfx1InvScan;
    T32MAINPL_CTRL4     stMainPlCtrl4;
    T32GFX0_CTRL        stGfx0Ctrl;
    T32GFX1_CTRL        stGfx1Ctrl;
    INT32              iDhubID = 0, iDhubChan0 = 0, iDhubChan1 = 0;

    stMainPlCtrl4.u32 = vpp_obj->mainpl_ctrl4.u32;
    stMainPlCtrl7.u32 = vpp_obj->stMainPlCtrl7.u32;
    stGfx0Ctrl.u32  =   vpp_obj->gfx0_ctrl.u32;
    stGfx1Ctrl.u32  =   vpp_obj->gfx1_ctrl.u32;
    plane = &(vpp_obj->plane[uiPlaneId]);

    uiInvScanMode = plane->uchInverseScanMode;
    uiClientMode = plane->uchClientMode;
    uiSrcOrder = plane->order;

    iDhubID    = (INT32)plane->dmaRdhubID;
    iDhubChan0 = plane->dmaRID[0];
    iDhubChan1 = plane->dmaRID[1];

    if(PLANE_MAIN == uiPlaneId) {
        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 0);
    }


    switch(uiClientMode) {
        case VPP_UNIQUE_FORMAT_ID_8BIT_420_SP:
        case VPP_UNIQUE_FORMAT_ID_16BIT_420_SP:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan0, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan1, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            FRC_SCL_Set_CSwap(vpp_obj, uiPlaneId, 0);
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 1;
                                        FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT_MAIN;
                            if(uiClientMode != VPP_UNIQUE_FORMAT_ID_16BIT_420_SP)
                                uiBitMapId  =   VPP_FE_DLR_32_BITMAP_ID_8BIT_420_SP;
                            else
                                uiBitMapId  =   VPP_FE_DLR_32_BITMAP_ID_16BIT_420_SP;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
                        }
                        break;
                    case PLANE_PIP:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 1;
                                        stGfx0InvScan.uINVSCAN_bitmap8R_Rd1_en = 1;
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                        uiBitMap10_IsNeeded = 1;
                                        FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_PIP, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        stGfx0InvScan.uINVSCAN_bitmap8R_Rd1_en = 0;
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   0;
                                        uiBitMap10_IsNeeded = 1;
                                        FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_PIP, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }
                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  =   VPP_FE_DLR_40_BITMAP_ID_8BIT_420_SP;
                            uiBitMap10Id = VPP_FE_DLR_10_BITMAP_ID_8BIT_420_SP;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_INVSCAN, stGfx0InvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL, stGfx0Ctrl.u32);

                            vpp_obj->gfx0_ctrl.u32 = stGfx0Ctrl.u32;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case VPP_UNIQUE_FORMAT_ID_10BIT_420_SP:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan0, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan1, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            FRC_SCL_Set_CSwap(vpp_obj, uiPlaneId, 0);
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        bitmap_bypass = 0;
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 1;
                                        FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        bitmap_bypass = 1;
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }
                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT_MAIN;
                            uiBitMapId  =  VPP_FE_DLR_32_BITMAP_ID_10BIT_420_SP;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case VPP_UNIQUE_FORMAT_ID_24BIT_YUV444:
        case VPP_UNIQUE_FORMAT_ID_30BIT_YUV444:
            {
                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                    }
                                    break;
                                default:
                                    break;
                            }
                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_8BIT_444;
                            if (uiSrcOrder == ORDER_VYU) {
                                stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                uiBitMapToLoad = VPP_FE_DLR_BITMAP_INVALID;
                                uiBitMapId  = VPP_FE_DLR_BITMAP_INVALID;
                            }

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
                        }
                        break;
                default:
                        break;
                }
            }
            break;
        case VPP_UNIQUE_FORMAT_ID_16BIT_422_YC:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan0, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT_MAIN;
                            uiBitMapId  = VPP_FE_DLR_32_BITMAP_ID_16BIT_422_YC;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
                        }
                        break;

                    case PLANE_PIP:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_GFX0_UPS8_Config_UVSwap(vpp_obj, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   0;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_GFX0_UPS8_Config_UVSwap(vpp_obj, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_422_YC;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_INVSCAN, stGfx0InvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL, stGfx0Ctrl.u32);

                            vpp_obj->gfx0_ctrl.u32 = stGfx0Ctrl.u32;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case VPP_UNIQUE_FORMAT_ID_20BIT_422_YC:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan0, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT_MAIN;
                            uiBitMapId  = VPP_FE_DLR_32_BITMAP_ID_20BIT_422_YC;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
                        }
                        break;
                    case PLANE_PIP:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_GFX0_UPS8_Config_UVSwap(vpp_obj, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   0;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_GFX0_UPS8_Config_UVSwap(vpp_obj, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  = VPP_FE_DLR_32_BITMAP_ID_20BIT_422_YC;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_INVSCAN, stGfx0InvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL, stGfx0Ctrl.u32);

                            vpp_obj->gfx0_ctrl.u32 = stGfx0Ctrl.u32;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;
        case VPP_UNIQUE_FORMAT_ID_10BIT_DWA_420_SP:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *)iDhubID, iDhubChan0, uiInvScanMode,(UNSG32)vpp_obj->pVbiBcmBuf);
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *)iDhubID, iDhubChan1, uiInvScanMode,(UNSG32)vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            FRC_SCL_Set_CSwap(vpp_obj, uiPlaneId, 0);
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stMainPlCtrl7.uCTRL7_sp_dwa_10b_normal_mode = 0;
                                        stMainPlCtrl7.uCTRL7_sp_dwa_10b_inverse_mode = 1;
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 1;
                                        FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stMainPlCtrl7.uCTRL7_sp_dwa_10b_normal_mode = 1;
                                        stMainPlCtrl7.uCTRL7_sp_dwa_10b_inverse_mode = 0;
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT_MAIN;
                            uiBitMapId  =   VPP_FE_DLR_32_BITMAP_ID_10BIT_420SP_DWA;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL7 , stMainPlCtrl7.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
                            vpp_obj->stMainPlCtrl7.u32 = stMainPlCtrl7.u32;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;

        case VPP_UNIQUE_FORMAT_ID_10BIT_DWA_444:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *)iDhubID, iDhubChan0, uiInvScanMode,(UNSG32)vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            FRC_SCL_Set_CSwap(vpp_obj, uiPlaneId, 0);
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT_MAIN;
                            uiBitMapId  =  VPP_FE_DLR_32_BITMAP_ID_10BIT_444_DWA;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;


        case VPP_UNIQUE_FORMAT_ID_10BIT_DWA_422_SP:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *)iDhubID, iDhubChan0, uiInvScanMode,(UNSG32)vpp_obj->pVbiBcmBuf);
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *)iDhubID, iDhubChan1, uiInvScanMode,(UNSG32) vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            FRC_SCL_Set_CSwap(vpp_obj, uiPlaneId, 0);
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stMainPlCtrl7.uCTRL7_sp_dwa_10b_normal_mode = 0;
                                        stMainPlCtrl7.uCTRL7_sp_dwa_10b_inverse_mode = 1;
#ifdef BA_MAINPL_CTRL7_yuv422_sp_en
                                        stMainPlCtrl7.uCTRL7_yuv422_sp_en = 1;
#else
                                        stMainPlCtrl7.uCTRL7_yuv422_sp_dwa_10b_en = 1;
#endif
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 1;
                                        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stMainPlCtrl7.uCTRL7_sp_dwa_10b_normal_mode = 1;
                                        stMainPlCtrl7.uCTRL7_sp_dwa_10b_inverse_mode = 0;
#ifdef BA_MAINPL_CTRL7_yuv422_sp_en
                                        stMainPlCtrl7.uCTRL7_yuv422_sp_en = 1;
#else
                                        stMainPlCtrl7.uCTRL7_yuv422_sp_dwa_10b_en = 1;
#endif
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT_MAIN;
                            uiBitMapId  =   VPP_FE_DLR_32_BITMAP_ID_10BIT_422SP_DWA;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL7 , stMainPlCtrl7.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
                            vpp_obj->stMainPlCtrl7.u32 = stMainPlCtrl7.u32;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
        case VPP_UNIQUE_FORMAT_ID_16BIT_ARGB4444:
        {
            dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *)iDhubID, iDhubChan0, uiInvScanMode, (UNSG32)vpp_obj->pVbiBcmBuf);

            switch(uiPlaneId) {
                case PLANE_PIP:
                    {
                        switch(uiInvScanMode) {
                            case VPP_FE_DLR_SCAN_INVERSE_H:
                            case VPP_FE_DLR_SCAN_INVERSE_HV:
                                {
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                    stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                            case VPP_FE_DLR_SCAN_INVERSE_V:
                                {
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                    stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            default:
                                break;
                        }

                        uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                        uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_ARGB4444;

                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_INVSCAN, stGfx0InvScan.u32);
                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL, stGfx0Ctrl.u32);

                        vpp_obj->gfx0_ctrl.u32 = stGfx0Ctrl.u32;
                    }
                    break;

                case PLANE_GFX1:
                    {
                        switch(uiInvScanMode) {
                            case VPP_FE_DLR_SCAN_INVERSE_H:
                            case VPP_FE_DLR_SCAN_INVERSE_HV:
                                {
                                    stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                    stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                            case VPP_FE_DLR_SCAN_INVERSE_V:
                                {
                                    stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                    stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            default:
                                break;
                        }

                        uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                        uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_ARGB4444;

                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_INVSCAN, stGfx1InvScan.u32);
                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL, stGfx1Ctrl.u32);

                        vpp_obj->gfx1_ctrl.u32 = stGfx1Ctrl.u32;
                    }

                default:
                    break;
             }
         }
        break;
        case VPP_UNIQUE_FORMAT_ID_16BIT_ARGB1555:
        {
            dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *)iDhubID, iDhubChan0, uiInvScanMode, (UNSG32)vpp_obj->pVbiBcmBuf);

            switch(uiPlaneId) {
                case PLANE_PIP:
                    {
                        switch(uiInvScanMode) {
                            case VPP_FE_DLR_SCAN_INVERSE_H:
                            case VPP_FE_DLR_SCAN_INVERSE_HV:
                                {
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                    stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                            case VPP_FE_DLR_SCAN_INVERSE_V:
                                {
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                    stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            default:
                                break;
                        }

                        uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                        uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_ARGB1555;

                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_INVSCAN, stGfx0InvScan.u32);
                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL, stGfx0Ctrl.u32);

                        vpp_obj->gfx0_ctrl.u32 = stGfx0Ctrl.u32;
                    }
                    break;

                case PLANE_GFX1:
                    {
                        switch(uiInvScanMode) {
                            case VPP_FE_DLR_SCAN_INVERSE_H:
                            case VPP_FE_DLR_SCAN_INVERSE_HV:
                                {
                                    stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                    stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                            case VPP_FE_DLR_SCAN_INVERSE_V:
                                {
                                    stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                    stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            default:
                                break;
                        }

                        uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                        uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_ARGB1555;

                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_INVSCAN, stGfx1InvScan.u32);
                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL, stGfx1Ctrl.u32);

                        vpp_obj->gfx1_ctrl.u32 = stGfx1Ctrl.u32;
                    }

                default:
                    break;
             }
         }
        break;

        case VPP_UNIQUE_FORMAT_ID_16BIT_ARGB8332:
        {
            dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *)iDhubID, iDhubChan0, uiInvScanMode, (UNSG32)vpp_obj->pVbiBcmBuf);

            switch(uiPlaneId) {
                case PLANE_PIP:
                    {
                        switch(uiInvScanMode) {
                            case VPP_FE_DLR_SCAN_INVERSE_H:
                            case VPP_FE_DLR_SCAN_INVERSE_HV:
                                {
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                    stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                            case VPP_FE_DLR_SCAN_INVERSE_V:
                                {
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                    stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                    stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            default:
                                break;
                        }

                        uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                        uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_ARGB8332;

                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_INVSCAN, stGfx0InvScan.u32);
                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL, stGfx0Ctrl.u32);

                        vpp_obj->gfx0_ctrl.u32 = stGfx0Ctrl.u32;
                    }
                    break;

                case PLANE_GFX1:
                    {
                        switch(uiInvScanMode) {
                            case VPP_FE_DLR_SCAN_INVERSE_H:
                            case VPP_FE_DLR_SCAN_INVERSE_HV:
                                {
                                    stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                    stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                            case VPP_FE_DLR_SCAN_INVERSE_V:
                                {
                                    stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                    stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                }
                                break;
                            default:
                                break;
                        }

                        uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                        uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_ARGB8332;

                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_INVSCAN, stGfx1InvScan.u32);
                        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL, stGfx1Ctrl.u32);

                        vpp_obj->gfx1_ctrl.u32 = stGfx1Ctrl.u32;
                    }

                default:
                    break;
             }
         }
        break;

        case VPP_UNIQUE_FORMAT_ID_32BIT_ARGB:
        case VPP_UNIQUE_FORMAT_ID_RGB888:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan0, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        switch(uiInvScanMode) {
                            case VPP_FE_DLR_SCAN_INVERSE_H:
                            case VPP_FE_DLR_SCAN_INVERSE_HV:
                                {
                                    stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                    stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                    stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 1;
                                    FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 1);
                                }
                                break;
                            case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                            case VPP_FE_DLR_SCAN_INVERSE_V:
                                {
                                    stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                    stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                    stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                    FE_DLR_420_422_UPS_Config_UVSwap(vpp_obj, VPP_FE_DLR_MAIN, 0);
                                }
                                break;
                            default:
                                break;
                        }
                            stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);
                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_32BIT_ARGB;

                        break;
                    case PLANE_PIP:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_32BIT_ARGB;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_INVSCAN, stGfx0InvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL, stGfx0Ctrl.u32);

                            vpp_obj->gfx0_ctrl.u32 = stGfx0Ctrl.u32;
                        }
                        break;

                    case PLANE_GFX1:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_32BIT_ARGB;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_INVSCAN, stGfx1InvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL, stGfx1Ctrl.u32);

                            vpp_obj->gfx1_ctrl.u32 = stGfx1Ctrl.u32;
                        }

                    default:
                        break;
                }
            }
            break;
        case VPP_UNIQUE_FORMAT_ID_16BIT_RGB565:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan0, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_PIP:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stGfx0InvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        stGfx0Ctrl.uCTRL_bitmap40_en    =   1;
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_RGB565;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_INVSCAN, stGfx0InvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL, stGfx0Ctrl.u32);

                            vpp_obj->gfx0_ctrl.u32 = stGfx0Ctrl.u32;
                        }
                        break;

                    case PLANE_GFX1:
                        {
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stGfx1InvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stGfx1Ctrl.uCTRL_bitmap40_en    =   1;
                                    }
                                    break;
                                default:
                                    break;
                            }

                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT;
                            uiBitMapId  = VPP_FE_DLR_40_BITMAP_ID_16BIT_RGB565;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_INVSCAN, stGfx1InvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL, stGfx1Ctrl.u32);

                            vpp_obj->gfx1_ctrl.u32 = stGfx1Ctrl.u32;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;
        case VPP_UNIQUE_FORMAT_ID_24BIT_422_YC:
            {
                dhub_channel_enable_InverseScan_vppBcm((HDL_dhub2d *) iDhubID, iDhubChan0, uiInvScanMode, (UNSG32) vpp_obj->pVbiBcmBuf);

                switch(uiPlaneId) {
                    case PLANE_MAIN:
                        {
                            //TODO:  Need to check if uiInvScanMode support is required ?
                            switch(uiInvScanMode) {
                                case VPP_FE_DLR_SCAN_INVERSE_H:
                                case VPP_FE_DLR_SCAN_INVERSE_HV:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 1);
                                    }
                                    break;
                                case VPP_FE_DLR_SCAN_INVERSE_NORMAL:
                                case VPP_FE_DLR_SCAN_INVERSE_V:
                                    {
                                        stMainPlCtrl4.uCTRL4_bitmap40_bypass = 1;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd0_en = 0;
                                        stMainInvScan.uINVSCAN_bitmap128_Rd1_en = 0;
                                        FE_DLR_422_444_UPSConfig_UVSwap(vpp_obj, uiPlaneId, 0);
                                    }
                                    break;
                                default:
                                    break;
                            }

                            // TODO: assign appropriate values to below field.
                            uiBitMapToLoad = VPP_FE_DLR_BITMAP_40_BIT_MAIN;
                            uiBitMapId  = VPP_FE_DLR_32_BITMAP_ID_20BIT_422_YC;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_INVSCAN , stMainInvScan.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4 , stMainPlCtrl4.u32);

                            vpp_obj->mainpl_ctrl4.u32 = stMainPlCtrl4.u32;
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
    stMainPlCtrl4.uCTRL4_bitmap40_bypass = bitmap_bypass;
    if(uiBitMapId == VPP_FE_DLR_20_BITMAP_ID_8BIT_420_SP)
        uiBitMapId = VPP_FE_DLR_32_BITMAP_ID_8BIT_420_SP;
    if(uiBitMapId == VPP_FE_DLR_20_BITMAP_ID_16BIT_420_SP)
        uiBitMapId = VPP_FE_DLR_20_BITMAP_ID_16BIT_420_SP;
    if(uiBitMapId == VPP_FE_DLR_20_BITMAP_ID_16BIT_422_YC)
        uiBitMapId = VPP_FE_DLR_32_BITMAP_ID_16BIT_422_YC;
    if(uiBitMapId == VPP_FE_DLR_20_BITMAP_ID_20BIT_422_YC)
        uiBitMapId = VPP_FE_DLR_32_BITMAP_ID_20BIT_422_YC;
    if(VPP_FE_DLR_BITMAP_40_BIT_MAIN == uiBitMapToLoad) {
        stBitMap40Sel.uSEL_BIT_POS0 =   gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][0];
        stBitMap40Sel.uSEL_BIT_POS1 =   gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][1];
        stBitMap40Sel.uSEL_BIT_POS2 =   gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][2];
        stBitMap40Sel.uSEL_BIT_POS3 =   gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][3];
        stBitMap40Sel.uSEL_BIT_POS4 =   gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][4];

        stBitMap40Sel1.uSEL_BIT_POS5  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][5];
        stBitMap40Sel1.uSEL_BIT_POS6  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][6];
        stBitMap40Sel1.uSEL_BIT_POS7  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][7];
        stBitMap40Sel1.uSEL_BIT_POS8  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][8];
        stBitMap40Sel1.uSEL_BIT_POS9  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][9];

        stBitMap40Sel2.uSEL_BIT_POS10  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][10];
        stBitMap40Sel2.uSEL_BIT_POS11  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][11];
        stBitMap40Sel2.uSEL_BIT_POS12  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][12];
        stBitMap40Sel2.uSEL_BIT_POS13  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][13];
        stBitMap40Sel2.uSEL_BIT_POS14  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][14];

        stBitMap40Sel3.uSEL_BIT_POS15  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][15];
        stBitMap40Sel3.uSEL_BIT_POS16  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][16];
        stBitMap40Sel3.uSEL_BIT_POS17  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][17];
        stBitMap40Sel3.uSEL_BIT_POS18  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][18];
        stBitMap40Sel3.uSEL_BIT_POS19  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][19];

        stBitMap40Sel4.uSEL_BIT_POS20  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][20];
        stBitMap40Sel4.uSEL_BIT_POS21  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][21];
        stBitMap40Sel4.uSEL_BIT_POS22  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][22];
        stBitMap40Sel4.uSEL_BIT_POS23  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][23];
        stBitMap40Sel4.uSEL_BIT_POS24  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][24];

        stBitMap40Sel5.uSEL_BIT_POS25  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][25];
        stBitMap40Sel5.uSEL_BIT_POS26  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][26];
        stBitMap40Sel5.uSEL_BIT_POS27  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][27];
        stBitMap40Sel5.uSEL_BIT_POS28  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][28];
        stBitMap40Sel5.uSEL_BIT_POS29  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][29];

        stBitMap40Sel6.uSEL_BIT_POS30  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][30];
        stBitMap40Sel6.uSEL_BIT_POS31  = gVppFeBitMap_32Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][31];

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_bitmap40;

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL, stBitMap40Sel.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL1, stBitMap40Sel1.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL2, stBitMap40Sel2.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL3, stBitMap40Sel3.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL4, stBitMap40Sel4.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL5, stBitMap40Sel5.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL6, stBitMap40Sel6.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL7, stBitMap40Sel7.u32);

    } else if (VPP_FE_DLR_BITMAP_40_BIT == uiBitMapToLoad) {

        stBitMap40Sel.uSEL_BIT_POS0   = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][0];
        stBitMap40Sel.uSEL_BIT_POS1   = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][1];
        stBitMap40Sel.uSEL_BIT_POS2   = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][2];
        stBitMap40Sel.uSEL_BIT_POS3   = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][3];
        stBitMap40Sel.uSEL_BIT_POS4   = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][4];

        stBitMap40Sel1.uSEL_BIT_POS5  = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][5];
        stBitMap40Sel1.uSEL_BIT_POS6  = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][6];
        stBitMap40Sel1.uSEL_BIT_POS7  = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][7];
        stBitMap40Sel1.uSEL_BIT_POS8  = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][8];
        stBitMap40Sel1.uSEL_BIT_POS9  = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][9];

        stBitMap40Sel2.uSEL_BIT_POS10 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][10];
        stBitMap40Sel2.uSEL_BIT_POS11 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][11];
        stBitMap40Sel2.uSEL_BIT_POS12 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][12];
        stBitMap40Sel2.uSEL_BIT_POS13 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][13];
        stBitMap40Sel2.uSEL_BIT_POS14 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][14];

        stBitMap40Sel3.uSEL_BIT_POS15 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][15];
        stBitMap40Sel3.uSEL_BIT_POS16 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][16];
        stBitMap40Sel3.uSEL_BIT_POS17 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][17];
        stBitMap40Sel3.uSEL_BIT_POS18 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][18];
        stBitMap40Sel3.uSEL_BIT_POS19 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][19];

        stBitMap40Sel4.uSEL_BIT_POS20 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][20];
        stBitMap40Sel4.uSEL_BIT_POS21 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][21];
        stBitMap40Sel4.uSEL_BIT_POS22 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][22];
        stBitMap40Sel4.uSEL_BIT_POS23 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][23];
        stBitMap40Sel4.uSEL_BIT_POS24 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][24];

        stBitMap40Sel5.uSEL_BIT_POS25 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][25];
        stBitMap40Sel5.uSEL_BIT_POS26 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][26];
        stBitMap40Sel5.uSEL_BIT_POS27 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][27];
        stBitMap40Sel5.uSEL_BIT_POS28 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][28];
        stBitMap40Sel5.uSEL_BIT_POS29 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][29];

        stBitMap40Sel6.uSEL_BIT_POS30 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][30];
        stBitMap40Sel6.uSEL_BIT_POS31 = gVppFeBitMap_40Bit[uiInvScanMode][uiBitMapId][uiSrcOrder][31];

        if(PLANE_PIP == uiPlaneId) {
            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_BITMAP40;
        } else if(PLANE_GFX1 == uiPlaneId) {
            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_BITMAP40;
        } else if(PLANE_MAIN == uiPlaneId) {
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_bitmap40;
        }

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL,  stBitMap40Sel.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL1, stBitMap40Sel1.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL2, stBitMap40Sel2.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL3, stBitMap40Sel3.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL4, stBitMap40Sel4.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL5, stBitMap40Sel5.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL6, stBitMap40Sel6.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL7, stBitMap40Sel7.u32);
        if(uiBitMap10_IsNeeded) { //FIXME: VPP_SW_BRINGUP: Change uiBitMap10_IsNeeded to uiBitMap12_IsNeeded
            stBitMap12Sel.uSEL_BIT_POS0 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][0];
            stBitMap12Sel.uSEL_BIT_POS1 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][1];
            stBitMap12Sel.uSEL_BIT_POS2 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][2];
            stBitMap12Sel.uSEL_BIT_POS3 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][3];
            stBitMap12Sel.uSEL_BIT_POS4 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][4];
            stBitMap12Sel.uSEL_BIT_POS5 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][5];
            stBitMap12Sel.uSEL_BIT_POS6 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][6];
            stBitMap12Sel.uSEL_BIT_POS7 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][7];

            stBitMap12Sel1.uSEL_BIT_POS8 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][8];
            stBitMap12Sel1.uSEL_BIT_POS9 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][9];
            stBitMap12Sel1.uSEL_BIT_POS10 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][10];
            stBitMap12Sel1.uSEL_BIT_POS11 = gVppFeBitMap_10Bit[uiInvScanMode][uiBitMap10Id][uiSrcOrder][11];

            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_BITMAP12 + RA_BITMAP12_SEL, stBitMap12Sel.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_BITMAP12 + RA_BITMAP12_SEL1, stBitMap12Sel1.u32);

        }
    }
    return (MV_VPP_OK);
}
#else
INT FE_DLR_SetBitmap(VPP_OBJ *vpp_obj, INT ldr_id, INT src_fmt, INT order)
{
    UINT8 *bitmap_table = NULL;
    UINT32 RegAddr;

    T32BITMAP40_SEL bitmap40_sel = {0};
    T32BITMAP40_SEL1 bitmap40_sel1 = {0};
    T32BITMAP40_SEL2 bitmap40_sel2 = {0};
    T32BITMAP40_SEL3 bitmap40_sel3 = {0};
    T32BITMAP40_SEL4 bitmap40_sel4 = {0};
    T32BITMAP40_SEL5 bitmap40_sel5 = {0};
    T32BITMAP40_SEL6 bitmap40_sel6 = {0};
    T32BITMAP40_SEL7 bitmap40_sel7 = {0};

    if(ldr_id == VPP_FE_DLR_MAIN) {
        RegAddr = vpp_obj->base_addr +  RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4;
        if(src_fmt == SRCFMT_ARGB32 || src_fmt == SRCFMT_RGB888 ||
           src_fmt == SRCFMT_ARGB32_PM || src_fmt == SRCFMT_XRGB32) {
            vpp_obj->mainpl_ctrl4.uCTRL4_bitmap40_bypass = 0;
            bitmap_table = &argb32_bitmap_table[order][0];
        } else if(src_fmt==SRCFMT_ARBG2101010) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB4444) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_RGB565) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB1555) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB8332) {
            //TODO: VPP_SW_BRINGUP
        } else {
            vpp_obj->mainpl_ctrl4.uCTRL4_bitmap40_bypass = 1;
            bitmap_table = NULL;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->mainpl_ctrl4.u32);
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_bitmap40;
    } else if(ldr_id == VPP_FE_DLR_PIP) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL;
        if(src_fmt == SRCFMT_ARGB32 || src_fmt == SRCFMT_RGB888  ||
           src_fmt == SRCFMT_ARGB32_PM || src_fmt == SRCFMT_XRGB32) {
            vpp_obj->gfx0_ctrl.uCTRL_bitmap40_en = 1;
            bitmap_table = &argb32_bitmap_table[order][0];
        } else if(src_fmt==SRCFMT_ARBG2101010) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB4444) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_RGB565) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB1555) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB8332) {
            //TODO: VPP_SW_BRINGUP
        } else {
            vpp_obj->gfx0_ctrl.uCTRL_bitmap40_en = 0;
            bitmap_table = NULL;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ctrl.u32);
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_BITMAP40;
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    else if(ldr_id == VPP_FE_DLR_GFX2) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_CTRL;
        if(src_fmt == SRCFMT_ARGB32 || src_fmt == SRCFMT_RGB888 ||
           src_fmt == SRCFMT_ARGB32_PM || src_fmt == SRCFMT_XRGB32) {
            vpp_obj->gfx2_ctrl.uCTRL_bitmap40_en = 1;
            bitmap_table = &argb32_bitmap_table[order][0];
        } else if(src_fmt==SRCFMT_ARBG2101010) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB4444) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_RGB565) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB1555) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB8332) {
            //TODO: VPP_SW_BRINGUP
        } else {
            vpp_obj->gfx2_ctrl.uCTRL_bitmap40_en = 0;
            bitmap_table = NULL;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx2_ctrl.u32);
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_BITMAP40;
    }
#endif
    else if(ldr_id == VPP_FE_DLR_GFX1) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL;
        if(src_fmt == SRCFMT_ARGB32 || src_fmt == SRCFMT_RGB888 ||
           src_fmt == SRCFMT_ARGB32_PM || src_fmt == SRCFMT_XRGB32) {
            vpp_obj->gfx1_ctrl.uCTRL_bitmap40_en = 1;
            bitmap_table = &argb32_bitmap_table[order][0];
        } else if(src_fmt==SRCFMT_ARBG2101010) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB4444) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_RGB565) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB1555) {
            //TODO: VPP_SW_BRINGUP
        } else if(src_fmt==SRCFMT_ARGB8332) {
            //TODO: VPP_SW_BRINGUP
        } else {
            vpp_obj->gfx1_ctrl.uCTRL_bitmap40_en = 0;
            bitmap_table = NULL;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx1_ctrl.u32);
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_BITMAP40;
    }


    if(bitmap_table==NULL)
        return MV_VPP_EBADPARAM;


    bitmap40_sel.uSEL_BIT_POS0 = bitmap_table[0];
    bitmap40_sel.uSEL_BIT_POS1 = bitmap_table[1];
    bitmap40_sel.uSEL_BIT_POS2 = bitmap_table[2];
    bitmap40_sel.uSEL_BIT_POS3 = bitmap_table[3];
    bitmap40_sel.uSEL_BIT_POS4 = bitmap_table[4];

    bitmap40_sel1.uSEL_BIT_POS5 = bitmap_table[5];
    bitmap40_sel1.uSEL_BIT_POS6 = bitmap_table[6];
    bitmap40_sel1.uSEL_BIT_POS7 = bitmap_table[7];
    bitmap40_sel1.uSEL_BIT_POS8 = bitmap_table[8];
    bitmap40_sel1.uSEL_BIT_POS9 = bitmap_table[9];

    bitmap40_sel2.uSEL_BIT_POS10 = bitmap_table[10];
    bitmap40_sel2.uSEL_BIT_POS11 = bitmap_table[11];
    bitmap40_sel2.uSEL_BIT_POS12 = bitmap_table[12];
    bitmap40_sel2.uSEL_BIT_POS13 = bitmap_table[13];
    bitmap40_sel2.uSEL_BIT_POS14 = bitmap_table[14];

    bitmap40_sel3.uSEL_BIT_POS15 = bitmap_table[15];
    bitmap40_sel3.uSEL_BIT_POS16 = bitmap_table[16];
    bitmap40_sel3.uSEL_BIT_POS17 = bitmap_table[17];
    bitmap40_sel3.uSEL_BIT_POS18 = bitmap_table[18];
    bitmap40_sel3.uSEL_BIT_POS19 = bitmap_table[19];

    bitmap40_sel4.uSEL_BIT_POS20 = bitmap_table[20];
    bitmap40_sel4.uSEL_BIT_POS21 = bitmap_table[21];
    bitmap40_sel4.uSEL_BIT_POS22 = bitmap_table[22];
    bitmap40_sel4.uSEL_BIT_POS23 = bitmap_table[23];
    bitmap40_sel4.uSEL_BIT_POS24 = bitmap_table[24];

    bitmap40_sel5.uSEL_BIT_POS25 = bitmap_table[25];
    bitmap40_sel5.uSEL_BIT_POS26 = bitmap_table[26];
    bitmap40_sel5.uSEL_BIT_POS27 = bitmap_table[27];
    bitmap40_sel5.uSEL_BIT_POS28 = bitmap_table[28];
    bitmap40_sel5.uSEL_BIT_POS29 = bitmap_table[29];

    bitmap40_sel6.uSEL_BIT_POS30 = bitmap_table[30];
    bitmap40_sel6.uSEL_BIT_POS31 = bitmap_table[31];

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL, bitmap40_sel.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL1, bitmap40_sel1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL2, bitmap40_sel2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL3, bitmap40_sel3.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL4, bitmap40_sel4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL5, bitmap40_sel5.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL6, bitmap40_sel6.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_BITMAP40_SEL7, bitmap40_sel7.u32);

    return (MV_VPP_OK);
}
#endif

/***************************************************************************************
 * FUNCTION: enable the channel.
 * PARAMS:  Channel - id of a front-end dataloader channel.
 *                Enable - 1:enable, 0:no effect.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_EnableChannel(VPP_OBJ *vpp_obj, INT Channel, UINT8 srcFmt, BOOL Enable) //DONE
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;
    T32ReadClient_Rd readclient_rd;
#ifdef ENABLE_EL_PIPE_PATH
    T32EL_PIPE_CTRL0 stElPipeCtrl0;
#endif
    T32OfflineRd_CTRL0 stOfflineRdCtrl0;
    T32OfflineRd_CTRL2 stOfflineRdCtrl2;
    THDMI_VOP_CFG9 stEdrHdmiRdCtrl;

    if((Channel < VPP_FE_DLR_CHANNEL_MIN) || (Channel > VPP_FE_DLR_CHANNEL_MAX))
        return MV_VPP_EBADPARAM;

    if(Channel == VPP_FE_DLR_CHANNEL_MAIN_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_MR0;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
    } else if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_MR1;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
    } else if(Channel == VPP_FE_DLR_CHANNEL_PIP_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_gfxScl; //Mirroring required as scl start clear also in this reg
        vpp_obj->gfx0_gfxscl.ugfxScl_client_start = Enable;
        vpp_obj->gfx0_gfxscl.ugfxScl_scl_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_gfxscl.u32);
        vpp_obj->gfx0_gfxscl.ugfxScl_client_start = 0;
        vpp_obj->gfx0_gfxscl.ugfxScl_scl_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_gfxscl.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_PIP_PIPE + RA_PIP_PIPE_CTRL0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, 1);
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, 0);
    } else if(Channel == VPP_FE_DLR_CHANNEL_PIP_1) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_RdClientPipR1 + RA_ReadClient_Rd;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
    } else if(Channel == VPP_FE_DLR_CHANNEL_GFX1_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_gfxScl;
        vpp_obj->gfx1_gfxscl.ugfxScl_client_start = Enable;
        vpp_obj->gfx1_gfxscl.ugfxScl_scl_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx1_gfxscl.u32);
        vpp_obj->gfx1_gfxscl.ugfxScl_client_start = 0;
        vpp_obj->gfx1_gfxscl.ugfxScl_scl_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx1_gfxscl.u32);
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    else if(Channel == VPP_FE_DLR_CHANNEL_GFX2_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_gfxScl;
        vpp_obj->gfx2_gfxscl.ugfxScl_client_start = Enable;
        vpp_obj->gfx2_gfxscl.ugfxScl_scl_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx2_gfxscl.u32);
        vpp_obj->gfx2_gfxscl.ugfxScl_client_start = 0;
        vpp_obj->gfx2_gfxscl.ugfxScl_scl_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx2_gfxscl.u32);
    }
#endif
    else if(Channel == VPP_FE_DLR_CHANNEL_VMX_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_RdClientVmxVm + RA_ReadClient_Rd;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_start = Enable;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, readclient_rd.u32);
    } else if(VPP_FE_DLR_CHANNEL_EL_PIPE == Channel) {
#ifdef ENABLE_EL_PIPE_PATH
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL0;
        VPP_REG_WORD32_READ(RegAddr, &stElPipeCtrl0.u32);
        stElPipeCtrl0.uCTRL0_tg_start = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stElPipeCtrl0.u32);
        stElPipeCtrl0.uCTRL0_tg_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stElPipeCtrl0.u32);
#endif
    } else if(VPP_FE_DLR_CHANNEL_OFFLINE_RD_0 == Channel) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN + RA_OfflineRd_CTRL2;
        VPP_REG_WORD32_READ(RegAddr, &stOfflineRdCtrl2.u32);
        stOfflineRdCtrl2.uCTRL2_tg_start = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stOfflineRdCtrl2.u32);
        stOfflineRdCtrl2.uCTRL2_tg_start = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stOfflineRdCtrl2.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN + RA_OfflineRd_CTRL0;
        VPP_REG_WORD32_READ(RegAddr, &stOfflineRdCtrl0.u32);
        stOfflineRdCtrl0.uCTRL0_client_start = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stOfflineRdCtrl0.u32);
        stOfflineRdCtrl0.uCTRL0_client_start = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stOfflineRdCtrl0.u32);
    } else if(VPP_FE_DLR_CHANNEL_EDR_0 == Channel) {
        RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG9;
        VPP_REG_WORD32_READ(RegAddr, &stEdrHdmiRdCtrl.u32[0]);
        stEdrHdmiRdCtrl.uCFG9_edrClient_start = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stEdrHdmiRdCtrl.u32[0]);
        stEdrHdmiRdCtrl.uCFG9_edrClient_start = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stEdrHdmiRdCtrl.u32[0]);
    }
    VPP_DV_FE_EnableChannel(vpp_obj, Channel, Enable);
    /*FIXME: use T32ClientIF_MR0 and T32ClientIF_MR1 */

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: clear all the FIFOs and pipline data inside the channel.
 * PARAMS:  Channel - id of a front-end dataloader channel.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_ClearChannel(VPP_OBJ *vpp_obj, INT Channel) //DONE
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;
    T32ReadClient_Rd readclient_rd;
#ifdef CONFIG_MV_AMP_VPP_UPS_420_422_SFT_RST
    T32UPS_420_422_CFG0 ups_420_422_cfg0;
#endif
#ifdef ENABLE_PIP_GFX0_PATH
    T32UPS_420_422_SP_CFG0 ups_420_422_sp_cfg;
#endif
#ifdef ENABLE_EL_PIPE_PATH
    T32EL_PIPE_CTRL0 stElPipeCtrl0;
#endif
    T32OfflineRd_CTRL0 stOfflineRdCtrl0;
    T32OfflineRd_CTRL2 stOfflineRdCtrl2;
    THDMI_VOP_CFG9 stEdrHdmiRdCtrl;

    if((Channel < VPP_FE_DLR_CHANNEL_MIN) || (Channel > VPP_FE_DLR_CHANNEL_MAX))
        return MV_VPP_EBADPARAM;

    if(Channel == VPP_FE_DLR_CHANNEL_MAIN_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_MR0;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, readclient_rd.u32);

        if((SRCFMT_YUV420SP == vpp_obj->plane[PLANE_MAIN].srcfmt) ||
                (SRCFMT_YUV420SP_TILED_MODE0 == vpp_obj->plane[PLANE_MAIN].srcfmt)||
                (SRCFMT_YUV420SP_DWA == vpp_obj->plane[PLANE_MAIN].srcfmt)||
                (SRCFMT_YUV420SP_TILED_MODE1 == vpp_obj->plane[PLANE_MAIN].srcfmt)
#ifdef CONFIG_AMP_VPP_ROTATION
                || (SRCFMT_YUV420SP_TILED_V4H8 == vpp_obj->plane[PLANE_MAIN].srcfmt)
#endif
        ) {
            if(SRCFMT_YUV420SP != vpp_obj->plane[PLANE_MAIN].srcfmt) {
                RegAddr = vpp_obj->base_addr + RA_Vpp_rst;
                vpp_obj->vpp_rst.urst_detileups420 = 0;
                vpp_obj->vpp_rst.urst_memlb_ups420 = 0;
                RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->vpp_rst.u32);
                vpp_obj->vpp_rst.urst_detileups420 = 1;
                vpp_obj->vpp_rst.urst_memlb_ups420 = 1;
                RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->vpp_rst.u32);
                if(VPP_DV_GetDvCompMode(vpp_obj)) {
                    vpp_obj->vpp_rst.urst_detileups420_elpipe = 0;
                    vpp_obj->vpp_rst.urst_memlb_ups420_el = 0;
                    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->vpp_rst.u32);
                    vpp_obj->vpp_rst.urst_detileups420_elpipe = 1;
                    vpp_obj->vpp_rst.urst_memlb_ups420_el = 1;
                    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->vpp_rst.u32);
                }
            }
#ifdef CONFIG_MV_AMP_VPP_UPS_420_422_SFT_RST
            ups_420_422_cfg0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y;
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG0;
            ups_420_422_cfg0.uCFG0_sft_rst = 1;
            //VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            ups_420_422_cfg0.uCFG0_sft_rst = 0;
            //VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y = ups_420_422_cfg0.u32;

            ups_420_422_cfg0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C;
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG0;
            ups_420_422_cfg0.uCFG0_sft_rst = 1;
            //VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            ups_420_422_cfg0.uCFG0_sft_rst = 0;
            //VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C = ups_420_422_cfg0.u32;
#endif //CONFIG_MV_AMP_VPP_UPS_420_422_SFT_RST
        }
    } else if(Channel == VPP_FE_DLR_CHANNEL_MAIN_1) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_MR1;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, readclient_rd.u32);
    } else if(Channel == VPP_FE_DLR_CHANNEL_PIP_0) {
#ifdef ENABLE_PIP_GFX0_PATH
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_gfxScl; //Mirroring required as scl start clear also in this reg
        vpp_obj->gfx0_gfxscl.ugfxScl_client_clear = 1;
        vpp_obj->gfx0_gfxscl.ugfxScl_scl_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->gfx0_gfxscl.u32);
        vpp_obj->gfx0_gfxscl.ugfxScl_client_clear = 0;
        vpp_obj->gfx0_gfxscl.ugfxScl_scl_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->gfx0_gfxscl.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_PIP_PIPE + RA_PIP_PIPE_CTRL0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, 2);
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, 0);

        if(SRCFMT_YUV420SP == vpp_obj->plane[PLANE_PIP].srcfmt) {
            ups_420_422_sp_cfg.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0;
            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_SP + RA_UPS_420_422_SP_CFG0;
            ups_420_422_sp_cfg.uCFG0_sft_rst = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_sp_cfg.u32);
            ups_420_422_sp_cfg.uCFG0_sft_rst = 0;
            VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_sp_cfg.u32);
            //vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0 = ups_420_422_sp_cfg.u32;
        }
        if((SRCFMT_YUV420SP_TILED_MODE0 == vpp_obj->plane[PLANE_PIP].srcfmt)||
                (SRCFMT_YUV420SP_TILED_MODE1 == vpp_obj->plane[PLANE_PIP].srcfmt)
#ifdef CONFIG_AMP_VPP_ROTATION
                || (SRCFMT_YUV420SP_TILED_V4H8 == vpp_obj->plane[PLANE_PIP].srcfmt)
#endif
        ) {
#ifdef ENABLE_EL_PIPE_PATH
            RegAddr = vpp_obj->base_addr + RA_Vpp_rst;
            vpp_obj->vpp_rst.urst_detileups420_elpipe = 0;
            RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->vpp_rst.u32);
            vpp_obj->vpp_rst.urst_detileups420_elpipe = 1;
            RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->vpp_rst.u32);

#ifdef CONFIG_MV_AMP_VPP_UPS_420_422_SFT_RST
            ups_420_422_cfg0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y;
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG0;
            ups_420_422_cfg0.uCFG0_sft_rst = 1;
            //VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            ups_420_422_cfg0.uCFG0_sft_rst = 0;
            //VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y = ups_420_422_cfg0.u32;

            ups_420_422_cfg0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C;
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG0;
            ups_420_422_cfg0.uCFG0_sft_rst = 1;
            //VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            ups_420_422_cfg0.uCFG0_sft_rst = 0;
            //VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C = ups_420_422_cfg0.u32;
#endif //CONFIG_MV_AMP_VPP_UPS_420_422_SFT_RST
#endif //ENABLE_EL_PIPE_PATH
        }
#endif
    } else if(Channel == VPP_FE_DLR_CHANNEL_PIP_1) {
#ifdef ENABLE_PIP_GFX0_PATH
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_RdClientPipR1 + RA_ReadClient_Rd;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, readclient_rd.u32);
#endif
    } else if(Channel == VPP_FE_DLR_CHANNEL_GFX1_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_gfxScl;
        vpp_obj->gfx1_gfxscl.ugfxScl_client_clear = 1;
        vpp_obj->gfx1_gfxscl.ugfxScl_scl_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->gfx1_gfxscl.u32);
        vpp_obj->gfx1_gfxscl.ugfxScl_client_clear = 0;
        vpp_obj->gfx1_gfxscl.ugfxScl_scl_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->gfx1_gfxscl.u32);
    } else if(Channel == VPP_FE_DLR_CHANNEL_GFX2_0) {
#ifdef ENABLE_GFX2_VOP2_PATH
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_gfxScl;
        vpp_obj->gfx2_gfxscl.ugfxScl_client_clear = 1;
        vpp_obj->gfx2_gfxscl.ugfxScl_scl_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->gfx2_gfxscl.u32);
        vpp_obj->gfx2_gfxscl.ugfxScl_client_clear = 0;
        vpp_obj->gfx2_gfxscl.ugfxScl_scl_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->gfx2_gfxscl.u32);
#endif
    } else if(Channel == VPP_FE_DLR_CHANNEL_VMX_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_RdClientVmxVm + RA_ReadClient_Rd;
        VPP_REG_WORD32_READ(RegAddr, &readclient_rd.u32);
        readclient_rd.uRd_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, readclient_rd.u32);
        readclient_rd.uRd_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, readclient_rd.u32);
    } else if(VPP_FE_DLR_CHANNEL_EL_PIPE == Channel) {
#ifdef ENABLE_EL_PIPE_PATH
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL0;
        VPP_REG_WORD32_READ(RegAddr, &stElPipeCtrl0.u32);
        stElPipeCtrl0.uCTRL0_tg_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stElPipeCtrl0.u32);
        stElPipeCtrl0.uCTRL0_tg_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stElPipeCtrl0.u32);
#endif
    } else if(VPP_FE_DLR_CHANNEL_OFFLINE_RD_0 == Channel) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN + RA_OfflineRd_CTRL2;
        VPP_REG_WORD32_READ(RegAddr, &stOfflineRdCtrl2.u32);
        stOfflineRdCtrl2.uCTRL2_tg_clear = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stOfflineRdCtrl2.u32);
        stOfflineRdCtrl2.uCTRL2_tg_clear = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stOfflineRdCtrl2.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN + RA_OfflineRd_CTRL0;
        VPP_REG_WORD32_READ(RegAddr, &stOfflineRdCtrl0.u32);
        stOfflineRdCtrl0.uCTRL0_client_clear = 1;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stOfflineRdCtrl0.u32);
        stOfflineRdCtrl0.uCTRL0_client_clear = 0;
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stOfflineRdCtrl0.u32);
    } else if(VPP_FE_DLR_CHANNEL_EDR_0 == Channel) {
        RegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_VOP + RA_VOP_HDMI_VOP + RA_HDMI_VOP_CFG9;
        VPP_REG_WORD32_READ(RegAddr, &stEdrHdmiRdCtrl.u32[0]);
        stEdrHdmiRdCtrl.uCFG9_edrClient_clear = 1;
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stEdrHdmiRdCtrl.u32[0]);
        stEdrHdmiRdCtrl.uCFG9_edrClient_clear = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, stEdrHdmiRdCtrl.u32[0]);
    }
    VPP_DV_FE_ClearChannel(vpp_obj, Channel);

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set dummy TG for main video channel.
 * PARAMS: TotRes - input total resolution.
 *                progressive - 1: prgressive, 0: interlaced.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetDummyTG_main(VPP_OBJ *vpp_obj, INT ldr_id,
        PVPP_FE_DLR_INPUT_RES pTotRes,
        PVPP_FE_DLR_INPUT_CROP pCropRes,
        int progressive)
{
    UINT32 RegAddr;
    UINT32 RegVal;
    INT RetVal = MV_VPP_OK;
    INT Crop1_Start_X = 0, Crop1_End_X = 0, Crop1_Start_Y = 0, Crop1_End_Y = 0;
    INT Luma_Crop2_Start_X = 0, Luma_Crop2_End_X = 0, Luma_Crop2_Start_Y = 0, Luma_Crop2_End_Y = 0;
    INT Chroma_Crop2_Start_X = 0, Chroma_Crop2_End_X = 0, Chroma_Crop2_Start_Y = 0, Chroma_Crop2_End_Y = 0;
    INT Chroma_Pix_Off = -1, Luma_Pix_Off = -1, Luma_Line_Off = -1, Chroma_Line_Off = -1;

    T32UPS_420_422_CFG6 stUPS_420_422_CFG6;
    T32UPS_420_422_CFG6 stUPS_420_422_CFG6_1;
    T32UPS_420_422_CFG0 stUPS_420_422_CFG0;

    UINT32 leftcropy = 0,rightcropy = 0,topcropy = 0,bottomcropy = 0;
    UINT32 leftcropc = 0,rightcropc = 0,topcropc = 0,bottomcropc = 0;

    leftcropy = vpp_obj->plane[PLANE_MAIN].m_leftpadadjust_y;
    rightcropy = vpp_obj->plane[PLANE_MAIN].m_rightpad_y;
    topcropy = vpp_obj->plane[PLANE_MAIN].m_toppadadjust_y;
    bottomcropy = vpp_obj->plane[PLANE_MAIN].m_bottompad_y;

    leftcropc = vpp_obj->plane[PLANE_MAIN].m_leftpadadjust_c;
    rightcropc = vpp_obj->plane[PLANE_MAIN].m_rightpad_c;
    topcropc = vpp_obj->plane[PLANE_MAIN].m_toppadadjust_c;
    bottomcropc = vpp_obj->plane[PLANE_MAIN].m_bottompad_c;

    /* Update the 420_422_cfg registers for getting black pixel padding */
    // Y cropping
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG6;
    stUPS_420_422_CFG6.u32 = vpp_obj->plane[PLANE_MAIN].m_uiTileCfg6_Y;
    stUPS_420_422_CFG6.uCFG6_left_crop = leftcropy;
    stUPS_420_422_CFG6.uCFG6_right_crop = rightcropy;
    stUPS_420_422_CFG6.uCFG6_bot_crop = bottomcropy;
    stUPS_420_422_CFG6.uCFG6_top_crop  = topcropy;

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG6.u32)))
        return RetVal;
    vpp_obj->plane[PLANE_MAIN].m_uiTileCfg6_Y = stUPS_420_422_CFG6.u32;

    //Cr cropping
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG6;
    stUPS_420_422_CFG6_1.u32 = vpp_obj->plane[PLANE_MAIN].m_uiTileCfg6_C;
    stUPS_420_422_CFG6_1.uCFG6_left_crop = leftcropc;
    stUPS_420_422_CFG6_1.uCFG6_right_crop = rightcropc;
    stUPS_420_422_CFG6_1.uCFG6_bot_crop = bottomcropc;
    stUPS_420_422_CFG6_1.uCFG6_top_crop  = topcropc;

    /* Write the data to BCM buffer */
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG6_1.u32)))
        return RetVal;
    vpp_obj->plane[PLANE_MAIN].m_uiTileCfg6_C = stUPS_420_422_CFG6_1.u32;

      if(vpp_obj->plane[PLANE_MAIN].m_SampleCropEnable) {
        /* use_blank & crop_blank programming  for Y & C*/
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG0;
        stUPS_420_422_CFG0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y;
        stUPS_420_422_CFG0.uCFG0_use_blank = 1;
        stUPS_420_422_CFG0.uCFG0_crop_blank = 0;

        /* Write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0.u32)))
            return RetVal;
        vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y = stUPS_420_422_CFG0.u32;


        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG0;
        stUPS_420_422_CFG0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C;
        stUPS_420_422_CFG0.uCFG0_use_blank = 1;
        stUPS_420_422_CFG0.uCFG0_crop_blank = 0;

        /* Write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0.u32)))
            return RetVal;
        vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C = stUPS_420_422_CFG0.u32;
        pTotRes->HRes = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Width;
        pTotRes->VRes = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Height;

        /* FETG Cropping for compressed inputs will be done in two stages : CROP1 & CROP2*/
        /* CROP1 will program the HB, HB_OUT VB,VB_OUT registers */
        /* CROP2 will program the HB_Y,VB_Y,HB_C,VB_C registers */

        /* ###### CROP 1 ###### */
        /* STARTX = max(leftcropy,leftcropc) ENDX = IHRES-min(rightcropy,rightcropc) */
        Crop1_Start_X = (leftcropy > leftcropc)?leftcropy:leftcropc;
        Crop1_End_X = (rightcropy < rightcropc)?(pTotRes->HRes-rightcropy):(pTotRes->HRes-rightcropc);

        Crop1_Start_Y = topcropy;
        Crop1_End_Y = pTotRes->VRes - bottomcropy;
        if(vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en) {
            /* ###### CROP2 ###### */
            Luma_Crop2_Start_X = 0;
            Luma_Crop2_End_X = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Width;
            Luma_Crop2_Start_Y = 0;
            Luma_Crop2_End_Y = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Height;

            /* ###### CROP2 ###### */
            Chroma_Crop2_Start_X = 0;
            Chroma_Crop2_End_X = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Width;
            Chroma_Crop2_Start_Y = 0;
            Chroma_Crop2_End_Y = vpp_obj->plane[PLANE_MAIN].m_Aligned_Luma_Height;

            /* Calculation of pixel offset for CROP2 */
            /* chroma has more garbage to the left of actual image than luma */
            if(vpp_obj->plane[PLANE_MAIN].m_leftpad_y < vpp_obj->plane[PLANE_MAIN].m_leftpad_c) { //(leftcropy < leftcropc)
                Chroma_Pix_Off = 0; /* no offset for chroma */
                Luma_Pix_Off = vpp_obj->plane[PLANE_MAIN].m_leftpad_c - vpp_obj->plane[PLANE_MAIN].m_leftpad_y;//(leftcropc - leftcropy); /*delay luma read by the difference in the crop*/
            } else { /* luma has equal or more garbage to the left of actual image than chroma */
                Luma_Pix_Off = 0; /* no offset for luma */
                Chroma_Pix_Off = vpp_obj->plane[PLANE_MAIN].m_leftpad_y - vpp_obj->plane[PLANE_MAIN].m_leftpad_c;//(leftcropy - leftcropc);/*delay in chroma read by the difference in crop */
            }

            /* Calculation of Line offset for CROP2 */
            /* chroma has more garbage to the top of actual image than luma */
            if(vpp_obj->plane[PLANE_MAIN].m_toppad_y < vpp_obj->plane[PLANE_MAIN].m_toppad_c) { //(topcropy < topcropc)
                Chroma_Line_Off = 0; /* no offset for chroma */
                Luma_Line_Off = vpp_obj->plane[PLANE_MAIN].m_toppad_c - vpp_obj->plane[PLANE_MAIN].m_toppad_y;//(topcropc - topcropy);/* delay Luma read by the difference in the crop */
            } else { /* luma has equal or more garbage to the top of actual image than chroma */
                Luma_Line_Off = 0;/* no offset for luma */
                Chroma_Line_Off = vpp_obj->plane[PLANE_MAIN].m_toppad_y - vpp_obj->plane[PLANE_MAIN].m_toppad_c;//(topcropy - topcropc);
            }
        }

        /* FETG MAIN SIZE register programming */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_feTG;
        if (progressive) {
            RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_IHRES_OFF + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
        } else {
            RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
        }
        /* write TG size to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
            return RetVal;

        /* calculate the H blank front edge value */
        RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);
        /*write the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
            return RetVal;

        /* calculate the V blank front edge value */
        RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);
        /*writer the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
            return RetVal;

        if(vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en) {
            UINT32 Val = 0, Addr = 0;
            vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en = 1;
            Val = vpp_obj->mainplCtrl0.u32;
            Addr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0;
            /* Programming the crop2 enable register */
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, Addr, Val);

            /* Program the HB_Y,VB0_Y,HB_C,VB0_C registers */
            RegVal = (Luma_Crop2_End_X + Luma_Pix_Off + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Luma_Crop2_Start_X + Luma_Pix_Off + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
                return RetVal;

            RegVal = (Luma_Crop2_End_Y + Luma_Line_Off + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Luma_Crop2_Start_Y + Luma_Line_Off + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
                return RetVal;

            RegVal = (Chroma_Crop2_End_X + Chroma_Pix_Off + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Chroma_Crop2_Start_X + Chroma_Pix_Off + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
                return RetVal;

            RegVal = (Chroma_Crop2_End_Y + Chroma_Line_Off + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Chroma_Crop2_Start_Y + Chroma_Line_Off + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
                return RetVal;
        } else {

            UINT32 Val = 0, Addr = 0;
            vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en = 0;
            Val = vpp_obj->mainplCtrl0.u32;
            Addr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0;
            /* Programming the crop2 enable register */
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, Addr, Val);

            /* Program the HB_Y,VB0_Y,HB_C,VB0_C registers */
            RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
                return RetVal;

            RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
                return RetVal;

            RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
                return RetVal;

            RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
                return RetVal;

        }

        /* Program the HB_OUT & VB0_OUT registers */
        RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
        /* write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
            return RetVal;

        RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
        /* write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
            return RetVal;

    } else { /* Normal cropping case */

        /* revert use_blank & crop_blank programming which was done in m_SampleCropEnable case. */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG0;
        stUPS_420_422_CFG0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y;
        stUPS_420_422_CFG0.uCFG0_use_blank = 0;
        stUPS_420_422_CFG0.uCFG0_crop_blank = 1;
        /* Write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0.u32)))
            return RetVal;
        vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y = stUPS_420_422_CFG0.u32;

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG0;
        stUPS_420_422_CFG0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C;
        stUPS_420_422_CFG0.uCFG0_use_blank = 0;
        stUPS_420_422_CFG0.uCFG0_crop_blank = 1;
        /* Write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0.u32)))
            return RetVal;
        vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C = stUPS_420_422_CFG0.u32;

        UINT32 Val = 0, Addr = 0;
        vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en = 0;
        Val = vpp_obj->mainplCtrl0.u32;
        Addr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0;
        /* Programming the crop2 enable register */
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, Addr, Val);
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_feTG;

        if (progressive) {
            RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
        } else {
            RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
        }

        /*write TG size to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
            return RetVal;

        /*calculate the H blank front edge value*/
        RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

        /*write the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
            return RetVal;

        /*calculate the V blank front edge value*/
        RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);

        /*writer the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
            return RetVal;

        RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_FE_DUMMY_TG_HB_FE_OFF + (((pCropRes->leftCrop)+VPP_FE_DUMMY_TG_HB_BE) << 16);

        /*write the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
            return RetVal;
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
            return RetVal;

        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
            return RetVal;

        RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((pCropRes->topCrop+VPP_FE_DUMMY_TG_VB0_BE) << 16);

        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
            return RetVal;
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
            return RetVal;

        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
            return RetVal;
    }
    return RetVal;
}

INT FE_DLR_SetDummyTG_offlineRD(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes)
{
    UINT32 RegAddr;
    UINT32 RegVal;
    INT RetVal = MV_VPP_OK;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN + RA_OfflineRd_UPSTG;

    RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);

    /*write TG size to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB), RegVal)))
        return RetVal;
    RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_FE_DUMMY_TG_HB_FE_OFF + (((pCropRes->leftCrop)+VPP_FE_DUMMY_TG_HB_BE) << 16);
    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);

    /*writer the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0), RegVal)))
        return RetVal;
    RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((pCropRes->topCrop+VPP_FE_DUMMY_TG_VB0_BE) << 16);
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR), RegVal)))
        return RetVal;

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set dummy TG for PIP video channel.
 * PARAMS: TotRes - input total resolution.
 *                progressive - 1: prgressive, 0: interlaced.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetDummyTG_pip(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes,
        PVPP_FE_DLR_INPUT_CROP pCropRes, int progressive)
{
    INT RetVal = MV_VPP_OK;
#ifdef ENABLE_PIP_GFX0_PATH
    INT Crop1_Start_X = 0, Crop1_End_X = 0, Crop1_Start_Y = 0, Crop1_End_Y = 0;
    INT Crop2_Start_X = 0, Crop2_End_X = 0, Crop2_Start_Y = 0, Crop2_End_Y = 0;
    INT Chroma_Pix_Off = -1, Luma_Pix_Off = -1, Luma_Line_Off = -1, Chroma_Line_Off = -1;
    UINT32 FeTGInWdt, FeTGInHgt,RegAddr,RegVal;
    T32UPS_420_422_CFG6 stUPS_420_422_CFG6 = {0};
    T32UPS_420_422_CFG6 stUPS_420_422_CFG6_1 = {0};
    T32UPS_420_422_CFG0 stUPS_420_422_CFG0 = {0};
    T32UPS_420_422_CFG0 stUPS_420_422_CFG0_1 = {0};
    PLANE *plane = NULL;
    UINT32 backuplcrop = 0, backuprcrop = 0,backuptcrop = 0, backupbcrop = 0;
    UINT32 backwidth = 0, backheight = 0;

    UINT32 leftcropy = 0,rightcropy = 0,topcropy = 0,bottomcropy = 0;
    UINT32 leftcropc = 0,rightcropc = 0,topcropc = 0,bottomcropc = 0;

    leftcropy = vpp_obj->plane[PLANE_PIP].m_leftpadadjust_y;
    rightcropy = vpp_obj->plane[PLANE_PIP].m_rightpad_y;
    topcropy = vpp_obj->plane[PLANE_PIP].m_toppadadjust_y;
    bottomcropy = vpp_obj->plane[PLANE_PIP].m_bottompad_y;

    leftcropc = vpp_obj->plane[PLANE_PIP].m_leftpadadjust_c;
    rightcropc = vpp_obj->plane[PLANE_PIP].m_rightpad_c;
    topcropc = vpp_obj->plane[PLANE_PIP].m_toppadadjust_c;
    bottomcropc = vpp_obj->plane[PLANE_PIP].m_bottompad_c;

    plane = &vpp_obj->plane[PLANE_PIP];
    if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
        || SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt
        || SRCFMT_YUV420SP_TILED_MODE1 == plane->srcfmt) {
        /* Update the 420_422_cfg registers for getting black pixel padding */
        // Y cropping
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG6;
        stUPS_420_422_CFG6.u32 = vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_Y;
        stUPS_420_422_CFG6.uCFG6_left_crop = leftcropy;
        stUPS_420_422_CFG6.uCFG6_right_crop = rightcropy;
        stUPS_420_422_CFG6.uCFG6_bot_crop = bottomcropy;
        stUPS_420_422_CFG6.uCFG6_top_crop  = topcropy;

        /*write the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG6.u32)))
            return RetVal;

        vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_Y = stUPS_420_422_CFG6.u32;

        //Cr cropping
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG6;
        stUPS_420_422_CFG6_1.u32 = vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_C;
        stUPS_420_422_CFG6_1.uCFG6_left_crop = leftcropc;
        stUPS_420_422_CFG6_1.uCFG6_right_crop = rightcropc;
        stUPS_420_422_CFG6_1.uCFG6_bot_crop = bottomcropc;
        stUPS_420_422_CFG6_1.uCFG6_top_crop  = topcropc;

        /* Write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG6_1.u32)))
             return RetVal;

        vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_C = stUPS_420_422_CFG6_1.u32;
        if(plane->m_SampleCropEnable) {
            /* use_blank & crop_blank programming  for Y & C*/
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG0;
            stUPS_420_422_CFG0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y;
            stUPS_420_422_CFG0.uCFG0_use_blank = 1;
            stUPS_420_422_CFG0.uCFG0_crop_blank = 0;

            /* Write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0.u32)))
                return RetVal;

            vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y = stUPS_420_422_CFG0.u32;

            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG0;
            stUPS_420_422_CFG0_1.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C;
            stUPS_420_422_CFG0_1.uCFG0_use_blank = 1;
            stUPS_420_422_CFG0_1.uCFG0_crop_blank = 0;

            /* Write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0_1.u32)))
                return RetVal;

            vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C = stUPS_420_422_CFG0_1.u32;

            FeTGInWdt = vpp_obj->plane[PLANE_PIP].m_Aligned_Luma_Width;
            FeTGInHgt = vpp_obj->plane[PLANE_PIP].m_Aligned_Luma_Height;

            /* FETG Cropping for compressed inputs will be done in two stages : CROP1 & CROP2*/
            /* CROP1 will program the HB, HB_OUT VB,VB_OUT registers */
            /* CROP2 will program the HB_Y,VB_Y,HB_C,VB_C registers */

            /* ###### CROP 1 ###### */
            /* STARTX = max(leftcropy,leftcropc) ENDX = IHRES-min(rightcropy,rightcropc) */
            Crop1_Start_X = (leftcropy > leftcropc)?leftcropy:leftcropc;
            Crop1_End_X = (rightcropy < rightcropc)?(FeTGInWdt-rightcropy):(FeTGInWdt-rightcropc);

            Crop1_Start_Y = topcropy;
            Crop1_End_Y = FeTGInHgt - bottomcropy;
            if(vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en) {
                /* Calculation of pixel offset for CROP2 */
                /* chroma has more garbage to the left of actual image than luma */
                if(leftcropy < leftcropc) {
                    Chroma_Pix_Off = 0; /* no offset for chroma */
                    Luma_Pix_Off = (leftcropc - leftcropy); /*delay luma read by the difference in the crop*/
                } else { /* luma has equal or more garbage to the left of actual image than chroma */
                    Luma_Pix_Off = 0; /* no offset for luma */
                    Chroma_Pix_Off = (leftcropy - leftcropc);/*delay in chroma read by the difference in crop */
                }

                /* Calculation of Line offset for CROP2 */
                /* chroma has more garbage to the top of actual image than luma */
                if(vpp_obj->plane[PLANE_PIP].m_toppad_y < vpp_obj->plane[PLANE_PIP].m_toppad_c) {
                    Chroma_Line_Off = 0; /* no offset for chroma */
                    Luma_Line_Off = (vpp_obj->plane[PLANE_PIP].m_toppad_c - vpp_obj->plane[PLANE_PIP].m_toppad_y);/* delay Luma read by the difference in the crop */
                } else { /* luma has equal or more garbage to the top of actual image than chroma */
                    Luma_Line_Off = 0;/* no offset for luma */
                    Chroma_Line_Off = (vpp_obj->plane[PLANE_PIP].m_toppad_y - vpp_obj->plane[PLANE_PIP].m_toppad_c);
                }
            }

            /* FETG MAIN SIZE register programming */
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_feTG_EL;
            if (progressive) {
                RegVal = (FeTGInHgt + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((FeTGInWdt + VPP_FE_DUMMY_TG_IHRES_OFF + VPP_FE_DUMMY_TG_IHRES_OFF_PIP + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
            } else {
                RegVal = (FeTGInHgt + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((FeTGInWdt + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
            }
            /* write TG size to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
                return RetVal;

            /* calculate the H blank front edge value */
            RegVal = FeTGInWdt + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);
            /*write the data to BCM buffer*/
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
                return RetVal;

            /* calculate the V blank front edge value */
            RegVal = FeTGInHgt + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);
            /*writer the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
                return RetVal;

            if(vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en) {
                UINT32 Val = 0, Addr = 0;
                vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en = 1;
                Val = vpp_obj->stElPipeCtrl1.u32;
                Addr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;

                /* Programming the crop2 enable register for PIP*/
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, Addr, Val);

                /* Program the HB_Y,VB0_Y,HB_C,VB0_C registers */
                RegVal = (Crop2_End_X + Luma_Pix_Off + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop2_Start_X + Luma_Pix_Off + VPP_FE_DUMMY_TG_HB_BE)<<16);
                /* write the data to BCM buffer */
                if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
                    return RetVal;

                RegVal = (Crop2_End_Y + Luma_Line_Off + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop2_Start_Y + Luma_Line_Off + VPP_FE_DUMMY_TG_VB0_BE)<<16);
                /* write the data to BCM buffer */
                if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
                    return RetVal;

                RegVal = (Crop2_End_X + Chroma_Pix_Off + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop2_Start_X + Chroma_Pix_Off + VPP_FE_DUMMY_TG_HB_BE)<<16);
                /* write the data to BCM buffer */
                if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
                    return RetVal;

                RegVal = (Crop2_End_Y + Chroma_Line_Off + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop2_Start_Y + Chroma_Line_Off + VPP_FE_DUMMY_TG_VB0_BE)<<16);
                /* write the data to BCM buffer */
                if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
                    return RetVal;
            } else {
                UINT32 Val = 0, Addr = 0;
                vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en = 0;
                Val = vpp_obj->stElPipeCtrl1.u32;
                Addr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;
                /* Programming the crop2 enable register for PIP*/
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, Addr, Val);

                /* Program the HB_Y,VB0_Y,HB_C,VB0_C registers */
                RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
                /* write the data to BCM buffer */
                if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
                    return RetVal;

                RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
                /* write the data to BCM buffer */
                if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
                    return RetVal;

                RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
                /* write the data to BCM buffer */
                if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
                    return RetVal;

                RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
                /* write the data to BCM buffer */
                if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
                    return RetVal;

            }

            /* Program the HB_OUT & VB0_OUT registers */
            RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
                return RetVal;

            RegVal = (Crop1_End_Y + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
                return RetVal;
        } else { /* Normal pixel cropping case */
            /* revert use_blank & crop_blank programming which was done in m_SampleCropEnable case*/
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG0;
            stUPS_420_422_CFG0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y;
            stUPS_420_422_CFG0.uCFG0_use_blank = 0;
            stUPS_420_422_CFG0.uCFG0_crop_blank = 1;
            /* Write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0.u32)))
                return RetVal;
            vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y = stUPS_420_422_CFG0.u32;

            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG0;
            stUPS_420_422_CFG0_1.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C;
            stUPS_420_422_CFG0_1.uCFG0_use_blank = 0;
            stUPS_420_422_CFG0_1.uCFG0_crop_blank = 1;
            /* Write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0_1.u32)))
                return RetVal;
            vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C = stUPS_420_422_CFG0_1.u32;

            UINT32 Val = 0, Addr = 0;
            vpp_obj->stElPipeCtrl1.uCTRL1_crop_de2_en = 0;
            Val = vpp_obj->stElPipeCtrl1.u32;
            Addr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;

            /* Programming the crop2 enable register for PIP*/
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, Addr, Val);

            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_feTG_EL;

            if (progressive) {
                RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
            } else {
                RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
            }

            /*write TG size to BCM buffer*/
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
                return RetVal;

            /*calculate the H blank front edge value*/
            RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

            /*write the data to BCM buffer*/
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
                return RetVal;
            RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_FE_DUMMY_TG_HB_FE_OFF + (((pCropRes->leftCrop)+VPP_FE_DUMMY_TG_HB_BE) <<16);
            /*write the data to BCM buffer*/
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
                return RetVal;
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
                return RetVal;
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
                return RetVal;

            /*calculate the V blank front edge value*/
            RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);

            /*writer the data to BCM buffer*/
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
                return RetVal;
            RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((pCropRes->topCrop+VPP_FE_DUMMY_TG_VB0_BE) <<16);
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
                return RetVal;
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
                return RetVal;
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
                return RetVal;
        }

    }

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_pipTG;
    if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
            || SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt
            || SRCFMT_YUV420SP_TILED_MODE1 == plane->srcfmt) {

        backwidth = pTotRes->HRes;
        backheight = pTotRes->VRes;
        pTotRes->HRes = pTotRes->HRes -(pCropRes->leftCrop + pCropRes->rightCrop);
        pTotRes->VRes = pTotRes->VRes -(pCropRes->topCrop + pCropRes->bottomCrop);
        backuplcrop =  pCropRes->leftCrop;
        backuprcrop =  pCropRes->rightCrop;
        backuptcrop =  pCropRes->topCrop;
        backupbcrop =  pCropRes->bottomCrop;
        pCropRes->leftCrop = pCropRes->rightCrop = pCropRes->topCrop = pCropRes->bottomCrop = 0;
    }
    if (progressive) {
        RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
    } else {
        RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
    }

    /*write TG size to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB), RegVal)))
        return RetVal;
    RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_FE_DUMMY_TG_HB_FE_OFF + (((pCropRes->leftCrop)+VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);
    /*writer the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0), RegVal)))
        return RetVal;

    RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((pCropRes->topCrop+VPP_FE_DUMMY_TG_VB0_BE) << 16);
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR), RegVal)))
        return RetVal;

    if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
            || SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt
            || SRCFMT_YUV420SP_TILED_MODE1 == plane->srcfmt) {

        pCropRes->leftCrop = backuplcrop;
        pCropRes->rightCrop = backuprcrop;
        pCropRes->topCrop = backuptcrop;
        pCropRes->bottomCrop = backupbcrop;
        pTotRes->HRes = backwidth;
        pTotRes->VRes = backheight;
    }
#endif //ENABLE_PIP_GFX0_PATH
    return RetVal;
}

/***************************************************************************************
 * FUNCTION: set dummy TG for GFX1 channel.
 * PARAMS: TotRes - input total resolution.
 *
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetDummyTG_gfx1(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes)
{
    UINT32 RegAddr;
    UINT32 RegVal;
    INT RetVal = MV_VPP_OK;

    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_gfx1Tg;

    RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);

    /*write TG size to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB), RegVal)))
        return RetVal;
    RegVal = (pTotRes->HRes) + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);
    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);

    /*writer the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0), RegVal)))
        return RetVal;
    RegVal = (pTotRes->VRes) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR), RegVal)))
        return RetVal;

    return RetVal;
}
/***************************************************************************************
 * FUNCTION: set dummy TG for GFX2 channel.
 * PARAMS: TotRes - input total resolution.
 *
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetDummyTG_gfx2(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes)
{
    INT RetVal = MV_VPP_OK;

#ifdef ENABLE_GFX2_VOP2_PATH
    UINT32 RegAddr;
    UINT32 RegVal;
    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX2 + RA_GFX1_gfx1Tg;

    RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);

    /*write TG size to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB), RegVal)))
        return RetVal;
    RegVal = (pTotRes->HRes) + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);
    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR), RegVal)))
        return RetVal;
    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR2), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);

    /*writer the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0), RegVal)))
        return RetVal;
    RegVal = (pTotRes->VRes) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR), RegVal)))
        return RetVal;

    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR2), RegVal)))
            return RetVal;
#endif //ENABLE_GFX2_VOP2_PATH

    return RetVal;
}
/***************************************************************************************
 * FUNCTION: set dummy TG for  video channel.
 * PARAMS: TotRes - input total resolution.
 *                progressive - 1: prgressive, 0: interlaced.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetDummyTG(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes, int progressive)
{
    INT RetVal = MV_VPP_OK;

    if(ldr_id == VPP_FE_DLR_MAIN) {
        RetVal = FE_DLR_SetDummyTG_main(vpp_obj, ldr_id, pTotRes, pCropRes, progressive);
        //FE_DLR_SetDummyTG_offlineRD(vpp_obj, ldr_id, pTotRes, pCropRes);
    } else if(ldr_id == VPP_FE_DLR_PIP) {
        RetVal = FE_DLR_SetDummyTG_pip(vpp_obj, ldr_id, pTotRes, pCropRes, progressive);
    } else if(ldr_id == VPP_FE_DLR_GFX1) {
        RetVal = FE_DLR_SetDummyTG_gfx1(vpp_obj, ldr_id, pTotRes);
    } else if(ldr_id == VPP_FE_DLR_GFX2) {
        RetVal = FE_DLR_SetDummyTG_gfx2(vpp_obj, ldr_id, pTotRes);
    } else {
        return MV_VPP_EBADPARAM;
    }

    return RetVal;
}

INT FE_DLR_GFX0_UPS8_Config_UVSwap(VPP_OBJ *vpp_obj, UINT8 uiUVswapEn)
{
    T32UPS10_CFG5 stUps10Cfg5;
    UINT32 uiRegAddr = 0;

    if(/*ldr_id == VPP_FE_DLR_PIP*/1)
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS10;
    else
        return MV_VPP_EBADPARAM;

    if(/*ldr_id == VPP_FE_DLR_PIP*/1){
        //GA_REG_WORD32_READ(uiRegAddr, &stUpsSpCfg0.u32);
        stUps10Cfg5.u32 = vpp_obj->plane[PLANE_PIP].ui422Ups10Cfg5;
        stUps10Cfg5.uCFG5_ups_cswap = uiUVswapEn;
        vpp_obj->plane[PLANE_PIP].ui422Ups10Cfg5 = stUps10Cfg5.u32;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS10_CFG5, stUps10Cfg5.u32);
    }

    return  MV_VPP_OK;
}

INT FE_DLR_422_444_UPSConfig_UVSwap(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, UINT8 uiUVswapEn)
{
    UINT32 uiRegAddr = 0;
    T32UPS10_CFG5 stUps10Cfg5;

    stUps10Cfg5.u32 = vpp_obj->plane[uiPlaneId].ui422Ups10Cfg5;
    if(PLANE_MAIN == uiPlaneId) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS422to444_10B + RA_UPS10_CFG5;
    } else
        return  MV_VPP_OK;

    stUps10Cfg5.uCFG5_ups_cswap = uiUVswapEn;
    vpp_obj->plane[uiPlaneId].ui422Ups10Cfg5 = stUps10Cfg5.u32;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, stUps10Cfg5.u32);

    return 0;
}

INT FE_DLR_420_422_UPS_Config_UVSwap(VPP_OBJ *vpp_obj, UINT32 ldr_id, UINT8 uiUVswapEn)
{
    T32UPS_420_422_CFG0 stUpsCfg0;
    T32UPS_420_422_SP_CFG0 stUpsSpCfg0;
    UINT32 uiRegAddr = 0;
    UINT32 uiRegAddr_C = 0;

    if(ldr_id == VPP_FE_DLR_MAIN) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y;
        uiRegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C;
    } else if(ldr_id == VPP_FE_DLR_PIP)
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_SP;
    else
        return MV_VPP_EBADPARAM;

    if(ldr_id == VPP_FE_DLR_MAIN) {
        //VPP_REG_WORD32_READ(uiRegAddr, &stUpsCfg0.u32);
        stUpsCfg0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y;
        stUpsCfg0.uCFG0_cswap_regs = uiUVswapEn;
        vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y = stUpsCfg0.u32;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS_420_422_CFG0, stUpsCfg0.u32);
        stUpsCfg0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C;
        stUpsCfg0.uCFG0_cswap_regs = uiUVswapEn;
        vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C = stUpsCfg0.u32;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_UPS_420_422_CFG0, stUpsCfg0.u32);
    } else if(ldr_id == VPP_FE_DLR_PIP) {
        //VPP_REG_WORD32_READ(uiRegAddr, &stUpsSpCfg0.u32);
        stUpsSpCfg0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0;
        stUpsSpCfg0.uCFG0_cswap_regs = uiUVswapEn;
        vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0 = stUpsSpCfg0.u32;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS_420_422_SP_CFG0, stUpsSpCfg0.u32);
    }

    return  MV_VPP_OK;
}



//FIXME: Need to move the following functions to seperate file
//and arrange properly
INT FE_DLR_420_422_UPS_PixCount(VPP_OBJ *vpp_obj, INT ldr_id, UINT8 uchEnable)
{
    T32UPS_420_422_CFG5 stUpsCfg5;
    T32UPS_420_422_SP_CFG5 stUpsSpCfg5;

    UINT32 uiRegAddr = 0;
    UINT32 uiRegAddr_Y = 0;
    UINT32 uiRegAddr_C = 0;


    if(ldr_id == VPP_FE_DLR_MAIN) {
        uiRegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y;
        uiRegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if(ldr_id == VPP_FE_DLR_PIP) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_SP;
        uiRegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY;
        uiRegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC;
    }
#endif
    else
        return MV_VPP_EBADPARAM;

    if(ldr_id == VPP_FE_DLR_MAIN) {
        VPP_REG_WORD32_READ(uiRegAddr_Y, &stUpsCfg5.u32);
        stUpsCfg5.uCFG5_auto_pixcnt = uchEnable;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_UPS_420_422_CFG5, stUpsCfg5.u32);
        VPP_REG_WORD32_READ(uiRegAddr_C, &stUpsCfg5.u32);
        stUpsCfg5.uCFG5_auto_pixcnt = uchEnable;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_UPS_420_422_CFG5, stUpsCfg5.u32);
    } else if(ldr_id == VPP_FE_DLR_PIP) {
        VPP_REG_WORD32_READ(uiRegAddr, &stUpsSpCfg5.u32);
        stUpsSpCfg5.uCFG5_auto_pixcnt = uchEnable;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS_420_422_SP_CFG5, stUpsSpCfg5.u32);
        VPP_REG_WORD32_READ(uiRegAddr_Y, &stUpsCfg5.u32);
        stUpsCfg5.uCFG5_auto_pixcnt = uchEnable;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_UPS_420_422_CFG5, stUpsCfg5.u32);
        VPP_REG_WORD32_READ(uiRegAddr_C, &stUpsCfg5.u32);
        stUpsCfg5.uCFG5_auto_pixcnt = uchEnable;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_UPS_420_422_CFG5, stUpsCfg5.u32);
    }
    return MV_VPP_OK;
}

INT FE_DLR_422_444_UPS_Coeff(VPP_OBJ *vpp_obj, INT ldr_id, const INT32 *piCoeff, const INT32 *piCoeff_7)
{
    T32UPS10_CFG1 stUpsCfg1;
    T32UPS10_CFG2 stUpsCfg2;
    T32UPS10_CFG3 stUpsCfg3;
    T32UPS10_CFG4 stUpsCfg4;
    T32UPS10_CFG7 stUpsCfg7;
    T32UPS10_CFG8 stUpsCfg8;
    T32UPS10_CFG9 stUpsCfg9;

    UINT32 i = 0;
    UINT32 uiRegAddr = 0;

    if(ldr_id == VPP_FE_DLR_MAIN)
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS422to444_10B;
    else
        return MV_VPP_EBADPARAM;

    i = 0;
    stUpsCfg1.mCFG1_ups_c0 = piCoeff[i];
    i++;
    stUpsCfg1.mCFG1_ups_c1 = piCoeff[i];
    i++;
    stUpsCfg2.mCFG2_ups_c2 = piCoeff[i];
    i++;
    stUpsCfg2.mCFG2_ups_c3 = piCoeff[i];
    i++;
    stUpsCfg3.mCFG3_ups_c4 = piCoeff[i];
    i++;
    stUpsCfg3.mCFG3_ups_c5 = piCoeff[i];
    i++;
    stUpsCfg4.mCFG4_ups_c6 = piCoeff[i];
    i++;

    i = 0;
    stUpsCfg7.mCFG7_ups_7c0 = piCoeff_7[i];
    i++;
    stUpsCfg7.mCFG7_ups_7c1 = piCoeff_7[i];
    i++;
    stUpsCfg8.mCFG8_ups_7c2 = piCoeff_7[i];
    i++;
    stUpsCfg8.mCFG8_ups_7c3 = piCoeff_7[i];
    i++;
    stUpsCfg9.mCFG9_ups_7c4 = piCoeff_7[i];
    i++;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS10_CFG1, stUpsCfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS10_CFG2, stUpsCfg2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS10_CFG3, stUpsCfg3.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS10_CFG4, stUpsCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS10_CFG7, stUpsCfg7.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS10_CFG8, stUpsCfg8.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS10_CFG9, stUpsCfg9.u32);

     return  MV_VPP_OK;
}

const int auiFeClient422UpsCoeff[] = {0x50, 0x0, 0x1130, 0x0, 0x4E0, 0x800, 0x4E0};
const int auiFeClient422Ups7Coeff[] = {0x1024, 0x9, 0x424, 0x7EE, 0x424};

INT FE_DLR_422_444_UPS_Config(VPP_OBJ *vpp_obj, INT ldr_id, INT src_fmt)
{
    INT RetVal = MV_VPP_OK;

    FE_DLR_422_444_UPS_Coeff(vpp_obj, ldr_id, auiFeClient422UpsCoeff, auiFeClient422Ups7Coeff);

    return RetVal;
}

INT FE_DLR_420_422_UPS_Coeff(VPP_OBJ *vpp_obj, INT ldr_id, const INT32 *piCoeff)
{
    T32UPS_420_422_CFG1 stUpsCfg1;
    T32UPS_420_422_CFG2 stUpsCfg2;
    T32UPS_420_422_CFG3 stUpsCfg3;
    T32UPS_420_422_CFG4 stUpsCfg4;

    T32UPS_420_422_SP_CFG1 stUpsSpCfg1;
    T32UPS_420_422_SP_CFG2 stUpsSpCfg2;
    T32UPS_420_422_SP_CFG3 stUpsSpCfg3;
    T32UPS_420_422_SP_CFG4 stUpsSpCfg4;

    UINT32 uiRegAddr = 0;
    UINT32 uiRegAddr_Y = 0;
    UINT32 uiRegAddr_C = 0;
    PLANE *plane = NULL;
    stUpsCfg1.u32 = 0;
    stUpsCfg2.u32 = 0;
    stUpsCfg3.u32 = 0;
    stUpsCfg4.u32 = 0;

    if(ldr_id == VPP_FE_DLR_MAIN) {
        uiRegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y;
        uiRegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C;
    }
#ifdef ENABLE_EL_PIPE_PATH
    else if(ldr_id == VPP_FE_DLR_PIP) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_SP;
        uiRegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY;
        uiRegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC;
    }
#endif
    else
        return MV_VPP_EBADPARAM;

    //Program 2's compliment for -ve coefficients.
    if(ldr_id == VPP_FE_DLR_MAIN) {
        stUpsCfg1.uCFG1_ups_c0 = (piCoeff[0] < 0)? (~(-piCoeff[0]) + 1): piCoeff[0];
        stUpsCfg1.uCFG1_ups_c1 = (piCoeff[1] < 0)? (~(-piCoeff[1]) + 1): piCoeff[1];

        stUpsCfg2.uCFG2_ups_c2 = (piCoeff[2] < 0)? (~(-piCoeff[2]) + 1): piCoeff[2];
        stUpsCfg2.uCFG2_ups_c3 = (piCoeff[3] < 0)? (~(-piCoeff[3]) + 1): piCoeff[3];

        stUpsCfg3.uCFG3_ups_c4 = (piCoeff[4] < 0)? (~(-piCoeff[4]) + 1): piCoeff[4];
        stUpsCfg3.uCFG3_ups_c5 = (piCoeff[5] < 0)? (~(-piCoeff[5]) + 1): piCoeff[5];
        stUpsCfg4.uCFG4_ups_c6 = (piCoeff[6] < 0)? (~(-piCoeff[6]) + 1): piCoeff[6];

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y +  RA_UPS_420_422_CFG1, stUpsCfg1.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y +  RA_UPS_420_422_CFG2, stUpsCfg2.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y +  RA_UPS_420_422_CFG3, stUpsCfg3.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y +  RA_UPS_420_422_CFG4, stUpsCfg4.u32);

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C +  RA_UPS_420_422_CFG1, stUpsCfg1.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C +  RA_UPS_420_422_CFG2, stUpsCfg2.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C +  RA_UPS_420_422_CFG3, stUpsCfg3.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C +  RA_UPS_420_422_CFG4, stUpsCfg4.u32);
    } else if(ldr_id == VPP_FE_DLR_PIP) {
        stUpsSpCfg1.uCFG1_ups_c0 = (piCoeff[0] < 0)? (~(-piCoeff[0]) + 1): piCoeff[0];
        stUpsSpCfg1.uCFG1_ups_c1 = (piCoeff[1] < 0)? (~(-piCoeff[1]) + 1): piCoeff[1];

        stUpsSpCfg2.uCFG2_ups_c2 = (piCoeff[2] < 0)? (~(-piCoeff[2]) + 1): piCoeff[2];
        stUpsSpCfg2.uCFG2_ups_c3 = (piCoeff[3] < 0)? (~(-piCoeff[3]) + 1): piCoeff[3];

        stUpsSpCfg3.uCFG3_ups_c4 = (piCoeff[4] < 0)? (~(-piCoeff[4]) + 1): piCoeff[4];
        stUpsSpCfg3.uCFG3_ups_c5 = (piCoeff[5] < 0)? (~(-piCoeff[5]) + 1): piCoeff[5];
        stUpsSpCfg4.uCFG4_ups_c6 = (piCoeff[6] < 0)? (~(-piCoeff[6]) + 1): piCoeff[6];

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS_420_422_SP_CFG1, stUpsSpCfg1.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr +  RA_UPS_420_422_SP_CFG2, stUpsSpCfg2.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr +  RA_UPS_420_422_SP_CFG3, stUpsSpCfg3.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr +  RA_UPS_420_422_SP_CFG4, stUpsSpCfg4.u32);

        plane = &vpp_obj->plane[PLANE_PIP];
        if(SRCFMT_YUV420SP_TILED_V4H8 == plane->srcfmt
                || SRCFMT_YUV420SP_TILED_MODE0 == plane->srcfmt
                || SRCFMT_YUV420SP_TILED_MODE1 == plane->srcfmt) {

            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y +  RA_UPS_420_422_CFG1, stUpsCfg1.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y +  RA_UPS_420_422_CFG2, stUpsCfg2.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y +  RA_UPS_420_422_CFG3, stUpsCfg3.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y +  RA_UPS_420_422_CFG4, stUpsCfg4.u32);

            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C +  RA_UPS_420_422_CFG1, stUpsCfg1.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C +  RA_UPS_420_422_CFG2, stUpsCfg2.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C +  RA_UPS_420_422_CFG3, stUpsCfg3.u32);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C +  RA_UPS_420_422_CFG4, stUpsCfg4.u32);
        }
    }

    return  MV_VPP_OK;
}

const int auiFeClient420SpUpsCoeff[] = {0, 0, 0, 0, 2048, 2048, 0 /*80, 0, -304,0, 1248, 2048, 1248*/ /*-7,47,-25,-341,1056,2636,1056*/ };
INT FE_DLR_420_422_UPS_Config(VPP_OBJ *vpp_obj, INT ldr_id, INT src_fmt, INT bits_per_pixel)
{
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr;
    T32UPS_420_422_CFG0 ups_420_422_cfg0;
#ifdef ENABLE_PIP_GFX0_PATH
    T32UPS_420_422_SP_CFG0 ups_420_422_sp_cfg;
    T32GFX0_TILE_CTRL stGfx0TileCtrl;
    stGfx0TileCtrl.u32 = 0;
#endif

    FE_DLR_420_422_UPS_Coeff(vpp_obj, ldr_id, auiFeClient420SpUpsCoeff);
    FE_DLR_420_422_UPS_PixCount(vpp_obj, ldr_id, 1);

    if(ldr_id == VPP_FE_DLR_MAIN) {
        /* Luma programming */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG0;
        ups_420_422_cfg0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y;
        if(src_fmt == SRCFMT_YUV422 || src_fmt == SRCFMT_YUV422SP_DWA || src_fmt == SRCFMT_YUV444_DWA) {
            ups_420_422_cfg0.uCFG0_enable = 0;
        } else if(VPP_IS_420SP_FMT(src_fmt)) {
            if(bits_per_pixel != 16) {
                ups_420_422_cfg0.uCFG0_enable = 1;
                ups_420_422_cfg0.uCFG0_sp_en = 1;
                ups_420_422_cfg0.uCFG0_cswap_regs = 0;
            } else
                ups_420_422_cfg0.uCFG0_enable = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_cfg0.u32);
        vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_Y = ups_420_422_cfg0.u32;

        /*Croma programming */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG0;
        ups_420_422_cfg0.u32 = vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C;
        if(src_fmt == SRCFMT_YUV422 || src_fmt == SRCFMT_RGB888 || \
            src_fmt == SRCFMT_YUV422SP_DWA || src_fmt == SRCFMT_YUV444_DWA) {
            ups_420_422_cfg0.uCFG0_enable = 0;
        } else if(VPP_IS_420SP_FMT(src_fmt)) {
            if(bits_per_pixel != 16) {
                ups_420_422_cfg0.uCFG0_enable = 1;
                ups_420_422_cfg0.uCFG0_sp_en = 1;
                ups_420_422_cfg0.uCFG0_cswap_regs = 0;
            } else
                ups_420_422_cfg0.uCFG0_enable = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_cfg0.u32);
        vpp_obj->plane[PLANE_MAIN].ui420UpsCfg0_C = ups_420_422_cfg0.u32;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if(ldr_id == VPP_FE_DLR_PIP) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_SP + RA_UPS_420_422_SP_CFG0;
        ups_420_422_sp_cfg.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0;
        if(src_fmt == SRCFMT_YUV422 || src_fmt == SRCFMT_YUV422SP_DWA || src_fmt == SRCFMT_YUV444_DWA) {
            ups_420_422_sp_cfg.uCFG0_enable = 0;
        } else if(VPP_IS_420SP_FMT(src_fmt)) {
            ups_420_422_sp_cfg.uCFG0_enable = 1;
            ups_420_422_sp_cfg.uCFG0_cblank_regs = 128;
            ups_420_422_sp_cfg.uCFG0_cswap_regs = 0;
        }

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_sp_cfg.u32);
        vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0 = ups_420_422_sp_cfg.u32;

        /* Tile prgm start */
        if(SRCFMT_YUV420SP_TILED_V4H8 == src_fmt
                || SRCFMT_YUV420SP_TILED_MODE0 == src_fmt
                || SRCFMT_YUV420SP_TILED_MODE1 == src_fmt) {
            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_TILE_CTRL;
            stGfx0TileCtrl.uTILE_CTRL_detile_inp_en = 1;
            stGfx0TileCtrl.uTILE_CTRL_infifo_ctrl   = 1;
            stGfx0TileCtrl.uTILE_CTRL_pipInfifo_ctrl= 1;
            stGfx0TileCtrl.uTILE_CTRL_pip_tile_sel  = 0;
            stGfx0TileCtrl.uTILE_CTRL_tile_inp_sel  = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stGfx0TileCtrl.u32);

            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;
            if (bits_per_pixel == 10)
                vpp_obj->stElPipeCtrl1.uCTRL1_idatctrl        = 3;
            else
                vpp_obj->stElPipeCtrl1.uCTRL1_idatctrl        = 2;
            vpp_obj->stElPipeCtrl1.uCTRL1_pip_rd_sel      = 1;
            vpp_obj->stElPipeCtrl1.uCTRL1_clken_ctrl2     = 0;
            vpp_obj->stElPipeCtrl1.uCTRL1_clken_ctrl7     = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->stElPipeCtrl1.u32);

            /* Luma programming */
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG0;
            ups_420_422_cfg0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y;
            ups_420_422_cfg0.uCFG0_enable = 1;
            ups_420_422_cfg0.uCFG0_sp_en = 1;
            ups_420_422_cfg0.uCFG0_cswap_regs = 0;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y = ups_420_422_cfg0.u32;

            /*Croma programming */
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG0;
            ups_420_422_cfg0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C;
            ups_420_422_cfg0.uCFG0_enable = 1;
            ups_420_422_cfg0.uCFG0_sp_en = 1;
            ups_420_422_cfg0.uCFG0_cswap_regs = 0;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_cfg0.u32);
            vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C = ups_420_422_cfg0.u32;
        } else {
            /* Disable detileing if input is non-tile */
            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_TILE_CTRL;
            stGfx0TileCtrl.uTILE_CTRL_detile_inp_en = 0;
            stGfx0TileCtrl.uTILE_CTRL_infifo_ctrl   = 0;
            stGfx0TileCtrl.uTILE_CTRL_pipInfifo_ctrl= 0;
            stGfx0TileCtrl.uTILE_CTRL_pip_tile_sel  = 0;
            stGfx0TileCtrl.uTILE_CTRL_tile_inp_sel  = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stGfx0TileCtrl.u32);

            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1;
            vpp_obj->stElPipeCtrl1.uCTRL1_pip_rd_sel      = 0;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->stElPipeCtrl1.u32);
        }
        /* Tile prgm end */
    }
#endif
    return RetVal;
}

int FE_DLR_SetVideoChannelCSC(VPP_OBJ *vpp_obj, INT ldr_id, INT src_fmt)
{
    int cpcbID_main, cpcbID_pip;
    int main_csc_mode = VPP_CSC_BYPASS_MODE;
    int pip_csc_mode = VPP_CSC_BYPASS_MODE;

    if(ldr_id == VPP_FE_DLR_MAIN) {
        /* get the output resolution info of CPCB which main plane connect to */
        cpcbID_main = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN);
        /*Main plane can be connected to CPCB_1 only*/
        if (cpcbID_main != CPCB_1) {
            return (MV_VPP_OK);
        }
        if (VPP_IS_420SP_FMT(src_fmt) || src_fmt == SRCFMT_ARGB32 ||
                src_fmt == SRCFMT_RGB888 || src_fmt == SRCFMT_YUV422 ||
                src_fmt == SRCFMT_YUV422SP_DWA || src_fmt == SRCFMT_YUV444_DWA ||
                src_fmt == SRCFMT_YUV444 ) {
            /* get ICSC mode for main channel according to input and output resolution */
            if(vpp_obj->m_HdrMode != VPP_HDR_MODE_ON) {
                if(vpp_obj->CscInp_colorprim == VPP_COLOR_PRIM_BT2020) {
                    main_csc_mode = VPP_CSC_YUV_709_TO_sRGB; //HDR2SDR conversion
                    if((vpp_obj->m_HdrMode == VPP_HDR_MODE_AUTO) && \
                            (vpp_obj->m_outformat != VPP_OUTFORMAT_SDR)) {
                        main_csc_mode = VPP_CSC_YUV_2020_TO_sRGB; //HDR passthrough
                    }
                } else {
                    main_csc_mode = COLOR_PRIM_TO_CSC_MODE[vpp_obj->CscInp_colorprim];
                }
            } else {
                //Either VDCM already converted SDR2HDR or input itself is HDR
                main_csc_mode = VPP_CSC_YUV_2020_TO_sRGB;
            }
        } else {
            main_csc_mode = VPP_CSC_BYPASS_MODE;
        }
        if(src_fmt == SRCFMT_ARGB32 || src_fmt == SRCFMT_RGB888) {
            VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj, VPP_CSC_VPTOP, VPP_CSC_RGB_TO_YUV_709);
        }
        /* set main channel ICSC */
        VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj, VPP_CSC_MAIN, main_csc_mode);
    } else if(ldr_id == VPP_FE_DLR_PIP) {
        /* get the output resolution info of CPCB which PIP plane connect to */
        cpcbID_pip = vpp_obj->chan[CHAN_PIP].dvID;

        /* set PIP channel ICSC */
        if(!IS_GFX_SRC_FORMAT(src_fmt)) {
            if((vpp_obj->m_HdrMode == VPP_HDR_MODE_ON) && (cpcbID_pip == CPCB_1)){
                //Either DCM already converted SDR2HDR or input itself is HDR
                pip_csc_mode = VPP_CSC_YUV_2020_TO_sRGB;//SDR2HDR output case
            } else {
                pip_csc_mode = COLOR_PRIM_TO_CSC_MODE[vpp_obj->CscInp_colorprim];
            }
        } else {
            pip_csc_mode = VPP_CSC_BYPASS_MODE;
        }
        VPP_CSC_ICSC_SetColorSpaceConversion(vpp_obj, VPP_CSC_PIP, pip_csc_mode);
    }
    return (MV_VPP_OK);
}

//Following function needs to be moved to vpp_fe.c
INT FE_DLR_Config(VPP_OBJ *vpp_obj, INT ldr_id, INT src_fmt, INT order, INT bits_per_pixel, INT is_prog)
{
    UINT32 RegAddr;
#ifdef ENABLE_PIP_GFX0_PATH
    T32UPS10_CFG0 ups10_cfg0;
#endif
    //T32MAINPL_CTRL0 mainpl_ctrl0;
    T32UPS_420_422_CFG10 ups_420_422_cfg10;

    if(ldr_id == VPP_FE_DLR_MAIN) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL;
        /* No need to Read the register, as we kept all changes in shadow */
        if(VPP_IS_420SP_FMT(src_fmt)) {
            vpp_obj->mainpl_ctrl4.uCTRL4_UPS420_4K_bypass = 0;
            vpp_obj->mainpl_ctrl4.uCTRL4_UPS422_444_10B_bypass = 0;
            vpp_obj->vptop_ctrl_cfg0.uCFG0_vptop_csc_bypass =  1;
        } else if(src_fmt == SRCFMT_ARGB32 ||src_fmt == SRCFMT_RGB888) {
            vpp_obj->mainpl_ctrl4.uCTRL4_UPS420_4K_bypass = 1;
            vpp_obj->mainpl_ctrl4.uCTRL4_UPS422_444_10B_bypass = 1;
            vpp_obj->mainpl_ctrl4.uCTRL4_UPS422_444_12B_bypass = 1;
            vpp_obj->vptop_ctrl_cfg0.uCFG0_vptop_csc_bypass = 0;
            vpp_obj->vptop_ctrl_cfg0.uCFG0_vptop_dns444_422_bypass = 1;
        } else {
            //SHIFT in 422 unless this is done.
            vpp_obj->mainpl_ctrl4.uCTRL4_UPS420_4K_bypass = 1;
            if (src_fmt == SRCFMT_YUV444_DWA || src_fmt == SRCFMT_YUV444)
                vpp_obj->mainpl_ctrl4.uCTRL4_UPS422_444_10B_bypass = 1;
            else
                vpp_obj->mainpl_ctrl4.uCTRL4_UPS422_444_10B_bypass = 0;
            vpp_obj->vptop_ctrl_cfg0.uCFG0_vptop_csc_bypass =  1;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_CFG0, vpp_obj->vptop_ctrl_cfg0.u32);
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_MAINPL_CTRL4, vpp_obj->mainpl_ctrl4.u32);
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINPL_UPS_420_422,
                vpp_obj->mainpl_ctrl4.uCTRL4_UPS420_4K_bypass?VPP_SRAM_PWRGATE_ENABLE:VPP_SRAM_PWRGATE_DISABLE);

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_CTRL0;
        VPP_REG_WORD32_READ(RegAddr, &vpp_obj->clientif_ctrl0.u32);

        vpp_obj->clientif_ctrl0.uCTRL0_ups420_idat_ctrl = INTERNAL_FE_DLR_GetUPS420DataCtrl(src_fmt, bits_per_pixel);
        if(VPP_IS_420SP_FMT(src_fmt)) {
            vpp_obj->clientif_ctrl0.uCTRL0_CLKEN_Main0 = 1;
            vpp_obj->clientif_ctrl0.uCTRL0_CLKEN_Main1 = 1;

            vpp_obj->clientif_ctrl0.uCTRL0_ups420_idataSelM = 1;
            if((src_fmt == SRCFMT_YUV420SP)||(src_fmt == SRCFMT_YUV420SP_DWA))
                vpp_obj->clientif_ctrl0.uCTRL0_read_sel_420SP = 1;
            else
                vpp_obj->clientif_ctrl0.uCTRL0_read_sel_420SP = 0;

            //FIXME: Need to move to defaults
            vpp_obj->clientif_ctrl0.uCTRL0_rdmain_initval0 = 0;
            vpp_obj->clientif_ctrl0.uCTRL0_rdmain_initval1 = 1;
            vpp_obj->clientif_ctrl0.uCTRL0_rdm_mask_sftrst = 0;
        } else /*if(src_fmt == SRCFMT_YUV422)*/ {
            vpp_obj->clientif_ctrl0.uCTRL0_CLKEN_Main0 = 1;
            vpp_obj->clientif_ctrl0.uCTRL0_CLKEN_Main1 = 0;
            vpp_obj->clientif_ctrl0.uCTRL0_ups420_idataSelM = 0;
            vpp_obj->clientif_ctrl0.uCTRL0_read_sel_420SP = 0;

            //FIXME: Need to move to defaults
            vpp_obj->clientif_ctrl0.uCTRL0_rdmain_initval0 = 0;
            vpp_obj->clientif_ctrl0.uCTRL0_rdmain_initval1 = 1;
            vpp_obj->clientif_ctrl0.uCTRL0_rdm_mask_sftrst = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->clientif_ctrl0.u32);

        /* Luma Programming */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG10;
        VPP_REG_WORD32_READ(RegAddr, &ups_420_422_cfg10.u32);
        if(VPP_IS_420SP_TILED_FMT(src_fmt)) {
            ups_420_422_cfg10.uCFG10_tile_en = 1;
            if(INPUT_BIT_DEPTH_8BIT == INTERNAL_FE_DLR_GetBitDepth(src_fmt,bits_per_pixel)) {
                if(src_fmt == SRCFMT_YUV420SP_TILED_MODE0) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  8;
                } else if(src_fmt == SRCFMT_YUV420SP_TILED_MODE1) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  4;
                }
            }
            if(INPUT_BIT_DEPTH_10BIT == INTERNAL_FE_DLR_GetBitDepth(src_fmt,bits_per_pixel)){
                if(src_fmt == SRCFMT_YUV420SP_TILED_MODE0) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  6;
                } else if(src_fmt == SRCFMT_YUV420SP_TILED_MODE1) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  4;
                }
            }

#ifdef CONFIG_AMP_VPP_ROTATION
            else if(src_fmt == SRCFMT_YUV420SP_TILED_V4H8) {
                if(8 == bits_per_pixel) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  8;
                } else if (10 == bits_per_pixel) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  6;
                }
            }
#endif
        } else {
            ups_420_422_cfg10.uCFG10_tile_en = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_cfg10.u32);
        vpp_obj->plane[PLANE_MAIN].m_uiTileCfg10_Y = ups_420_422_cfg10.u32;

        /*Croma Programming */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG10;
        VPP_REG_WORD32_READ(RegAddr, &ups_420_422_cfg10.u32);
        if(VPP_IS_420SP_TILED_FMT(src_fmt)) {
            ups_420_422_cfg10.uCFG10_tile_en = 1;
            if(INPUT_BIT_DEPTH_8BIT == INTERNAL_FE_DLR_GetBitDepth(src_fmt,bits_per_pixel)){
                if(src_fmt == SRCFMT_YUV420SP_TILED_MODE0) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  8;
                } else if(src_fmt == SRCFMT_YUV420SP_TILED_MODE1) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  4;
                }
            }
            if(INPUT_BIT_DEPTH_10BIT == INTERNAL_FE_DLR_GetBitDepth(src_fmt,bits_per_pixel)){
                if(src_fmt == SRCFMT_YUV420SP_TILED_MODE0) {
                    ups_420_422_cfg10.uCFG10_tile_depth = 4;
                    ups_420_422_cfg10.uCFG10_tile_width = 6;
                } else if(src_fmt == SRCFMT_YUV420SP_TILED_MODE1) {
                    ups_420_422_cfg10.uCFG10_tile_depth = 4;
                    ups_420_422_cfg10.uCFG10_tile_width = 4;
                }
            }

#ifdef CONFIG_AMP_VPP_ROTATION
            else if(src_fmt == SRCFMT_YUV420SP_TILED_V4H8) {
                if(8 == bits_per_pixel) {
                    ups_420_422_cfg10.uCFG10_tile_depth = 4;
                    ups_420_422_cfg10.uCFG10_tile_width = 8;
                } else if (10 == bits_per_pixel) {
                    ups_420_422_cfg10.uCFG10_tile_depth =  4;
                    ups_420_422_cfg10.uCFG10_tile_width =  6;
                }
            }
#endif
        } else {
            ups_420_422_cfg10.uCFG10_tile_en = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_cfg10.u32);
        vpp_obj->plane[PLANE_MAIN].m_uiTileCfg10_C = ups_420_422_cfg10.u32;

        if((src_fmt == SRCFMT_YUV420SP_DWA) || (src_fmt == SRCFMT_YUV422SP_DWA)) {
            vpp_obj->stMainPlCtrl7.uCTRL7_sp_dwa_10b_normal_mode = 1;
        } else {
            vpp_obj->stMainPlCtrl7.uCTRL7_sp_dwa_10b_normal_mode = 0;
        }
        if(src_fmt == SRCFMT_YUV422SP_DWA) {
#ifdef BA_MAINPL_CTRL7_yuv422_sp_en
            vpp_obj->stMainPlCtrl7.uCTRL7_yuv422_sp_en = 1;
#else
            vpp_obj->stMainPlCtrl7.uCTRL7_yuv422_sp_dwa_10b_en = 1;
#endif
        } else {
#ifdef BA_MAINPL_CTRL7_yuv422_sp_en
            vpp_obj->stMainPlCtrl7.uCTRL7_yuv422_sp_en = 0;
#else
            vpp_obj->stMainPlCtrl7.uCTRL7_yuv422_sp_dwa_10b_en = 0;
#endif
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL7 , vpp_obj->stMainPlCtrl7.u32);

    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if(ldr_id == VPP_FE_DLR_PIP){
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL;
        if(VPP_IS_420SP_FMT(src_fmt)) {
            vpp_obj->gfx0_ctrl.uCTRL_clken_ctrl = 1;
            vpp_obj->gfx0_ctrl.uCTRL_clken_ctrl_sp = 1;
        } else /*if(src_fmt == SRCFMT_YUV422)*/ {
            vpp_obj->gfx0_ctrl.uCTRL_clken_ctrl = 1;
            vpp_obj->gfx0_ctrl.uCTRL_clken_ctrl_sp = 0;
        }
        if(isTiledFormat(src_fmt) && (vpp_obj->chan[CHAN_PIP].dvID == CPCB_2))
            vpp_obj->vpp_Ctrl.uCTRL_gfx0_tile_autoFlush_sel = 1;
        else
            vpp_obj->vpp_Ctrl.uCTRL_gfx0_tile_autoFlush_sel = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_CTRL, vpp_obj->vpp_Ctrl.u32);

        if(src_fmt == SRCFMT_ARGB32 || src_fmt == SRCFMT_RGB888 || src_fmt == SRCFMT_ARGB32_PM || src_fmt == SRCFMT_ARBG2101010 || src_fmt == SRCFMT_ARGB4444 || src_fmt == SRCFMT_RGB565 || src_fmt == SRCFMT_ARGB1555 || SRCFMT_ARGB8332 == src_fmt || src_fmt == SRCFMT_XRGB32) {
            vpp_obj->gfx0_ctrl.uCTRL_pip_gfx_en = 0;
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_VPP_ENABLE_PLANE_CLKGATE)
            if(IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE)) {
                vpp_obj->gfx0_ctrl.uCTRL_ups8_bypass = 1;
                vpp_obj->gfx0_ctrl.uCTRL_csc_bypass = 1;
            }
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */
        } else if(src_fmt == SRCFMT_YUV444_DWA) {
            vpp_obj->gfx0_ctrl.uCTRL_pip_gfx_en = 1;
            vpp_obj->gfx0_ctrl.uCTRL_ups8_bypass = 1;
            vpp_obj->gfx0_ctrl.uCTRL_csc_bypass = 0;
        } else if(src_fmt == SRCFMT_RGB888 ) {
            vpp_obj->gfx0_ctrl.uCTRL_pip_gfx_en = 1;
            vpp_obj->gfx0_ctrl.uCTRL_ups8_bypass = 1;
            vpp_obj->gfx0_ctrl.uCTRL_csc_bypass = 1;
        } else if(src_fmt == SRCFMT_YUV444 ) {
            vpp_obj->gfx0_ctrl.uCTRL_pip_gfx_en = 1;
            vpp_obj->gfx0_ctrl.uCTRL_ups8_bypass = 1;
            vpp_obj->gfx0_ctrl.uCTRL_csc_bypass = 0;
        } else {
            vpp_obj->gfx0_ctrl.uCTRL_pip_gfx_en = 1;
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_VPP_ENABLE_PLANE_CLKGATE)
            if(IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE)) {
                vpp_obj->gfx0_ctrl.uCTRL_ups8_bypass = 0;
                vpp_obj->gfx0_ctrl.uCTRL_csc_bypass = 0;
            }
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ctrl.u32);

        /*
         * %unsigned 3  gfxpl_idata_ctrl          0x0
         *    dolphin Z1:  [101]: Input data is ARGB32 8bpc
         *    dolphin A0:  [101]: Input data is YUV422 12bpc
         *               [110]: Input data is ARGB32 8bpc
         */
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL0;
        if(IS_GFX_SRC_FORMAT(src_fmt)) {
            vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 6;
        } else if((src_fmt == SRCFMT_YUV422) && (bits_per_pixel == 12)) {
            vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 5;
        } else if(src_fmt == SRCFMT_ARBG2101010) {
            vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 4;
        } else if(src_fmt == SRCFMT_YUV444_DWA) {
            vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 3;
        } else if(src_fmt == SRCFMT_RGB888) {
            vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 2;
        } else if(src_fmt == SRCFMT_YUV444) {
            if(bits_per_pixel == 10)
                vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 3;
            else
                vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 2;
        } else {
            if((bits_per_pixel==10) || (bits_per_pixel==20))
                vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 1;
            else
                vpp_obj->gfx0_ctrl0.uCTRL0_gfxpl_idata_ctrl = 0;
        }

        if((src_fmt == SRCFMT_YUV420SP_DWA) || (src_fmt == SRCFMT_YUV422SP_DWA)) {
            vpp_obj->gfx0_ctrl0.uCTRL0_sp_dwa_10b_normal_mode = 1;
        } else {
            vpp_obj->gfx0_ctrl0.uCTRL0_sp_dwa_10b_normal_mode = 0;
        }
        if(src_fmt == SRCFMT_YUV422SP_DWA) {
            vpp_obj->gfx0_ctrl0.uCTRL0_yuv422_sp_dwa_10b_en = 1;
            vpp_obj->gfx0_ctrl0.uCTRL0_ups422_idata_ctrl = 1;
        } else {
            vpp_obj->gfx0_ctrl0.uCTRL0_yuv422_sp_dwa_10b_en = 0;
            vpp_obj->gfx0_ctrl0.uCTRL0_ups422_idata_ctrl = 0;
        }

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ctrl0.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_PIP_PIPE + RA_PIP_PIPE_CTRL1;
        if((src_fmt == SRCFMT_YUV422)||(src_fmt == SRCFMT_YUV444_DWA)||IS_GFX_SRC_FORMAT(src_fmt)) {
            vpp_obj->gfx0_pip_pipe_ctrl1.uCTRL1_clken_ctrl0 = 1;
            vpp_obj->gfx0_pip_pipe_ctrl1.uCTRL1_clken_ctrl1 = 0;
            vpp_obj->gfx0_pip_pipe_ctrl1.uCTRL1_clken_ctrl2 = 1;

            vpp_obj->gfx0_pip_pipe_ctrl1.uCTRL1_detile_rdsel_420SP = 0;
        } else {
            vpp_obj->gfx0_pip_pipe_ctrl1.uCTRL1_clken_ctrl0 = 1;
            vpp_obj->gfx0_pip_pipe_ctrl1.uCTRL1_clken_ctrl1 = 1;
            vpp_obj->gfx0_pip_pipe_ctrl1.uCTRL1_clken_ctrl2 = 1;

            vpp_obj->gfx0_pip_pipe_ctrl1.uCTRL1_detile_rdsel_420SP = 1;
        }

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_pip_pipe_ctrl1.u32);

        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_CTRL;
        if(VPP_IS_420SP_FMT(src_fmt)) {
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_ups420_bypass = 0;
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_ups422_data_ctrl = 1;

            //FIXME: Need to move to defaults
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_rd_mask_initVal0 = 0;
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_rd_mask_initVal1 = 1;
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_rd_mask_sftrst = 0;
        } else /*if(src_fmt == SRCFMT_YUV422)*/ {
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_ups420_bypass = 1;
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_ups422_data_ctrl = 0;

            //FIXME: Need to move to defaults
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_rd_mask_initVal0 = 0;
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_rd_mask_initVal1 = 1;
            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_rd_mask_sftrst = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ups420_ctrl.u32);
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_GFX0_UPS420_SP,
                            vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_ups420_bypass?VPP_SRAM_PWRGATE_ENABLE:VPP_SRAM_PWRGATE_DISABLE);

        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS10 + RA_UPS10_CFG0;
        VPP_REG_WORD32_READ(RegAddr, &ups10_cfg0.u32);

        ups10_cfg0.uCFG0_ups_en = VPP_IS_UPS_422_to_444_NEEDED(src_fmt);

        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups10_cfg0.u32);

        /* ---- Tile mode programming ---- */
        if(SRCFMT_YUV420SP_TILED_MODE0 == src_fmt || SRCFMT_YUV420SP_TILED_MODE1 == src_fmt || SRCFMT_YUV420SP_TILED_V4H8  == src_fmt) {
            /* Luma Programming */
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG10;
            VPP_REG_WORD32_READ(RegAddr, &ups_420_422_cfg10.u32);
            if(VPP_IS_420SP_TILED_FMT(src_fmt)) {
                ups_420_422_cfg10.uCFG10_tile_en = 1;
                if(INPUT_BIT_DEPTH_8BIT == INTERNAL_FE_DLR_GetBitDepth(src_fmt,bits_per_pixel)){
                    if(src_fmt == SRCFMT_YUV420SP_TILED_MODE0) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  8;
                    } else if(src_fmt == SRCFMT_YUV420SP_TILED_MODE1) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  4;
                    }
                }
                if(INPUT_BIT_DEPTH_10BIT == INTERNAL_FE_DLR_GetBitDepth(src_fmt,bits_per_pixel)){
                    if(src_fmt == SRCFMT_YUV420SP_TILED_MODE0) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  6;
                    } else if(src_fmt == SRCFMT_YUV420SP_TILED_MODE1) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  4;
                    }
                }

#ifdef CONFIG_AMP_VPP_ROTATION
                else if(src_fmt == SRCFMT_YUV420SP_TILED_V4H8) {
                    if(8 == bits_per_pixel) {
                        ups_420_422_cfg10.uCFG10_tile_depth = 4;
                        ups_420_422_cfg10.uCFG10_tile_width = 8;
                    } else if (10 == bits_per_pixel) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  6;
                    }
                }
#endif
            } else {
                ups_420_422_cfg10.uCFG10_tile_en = 0;
            }
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_cfg10.u32);
            vpp_obj->plane[PLANE_PIP].m_uiTileCfg10_Y = ups_420_422_cfg10.u32;

            /*Croma Programming */
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG10;
            VPP_REG_WORD32_READ(RegAddr, &ups_420_422_cfg10.u32);
            if(VPP_IS_420SP_TILED_FMT(src_fmt)) {
                ups_420_422_cfg10.uCFG10_tile_en = 1;
                if(INPUT_BIT_DEPTH_8BIT == INTERNAL_FE_DLR_GetBitDepth(src_fmt,bits_per_pixel)) {
                    if(src_fmt == SRCFMT_YUV420SP_TILED_MODE0) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  8;
                    } else if(src_fmt == SRCFMT_YUV420SP_TILED_MODE1) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  4;
                    }
                }
                if(INPUT_BIT_DEPTH_10BIT == INTERNAL_FE_DLR_GetBitDepth(src_fmt,bits_per_pixel)){
                    if(src_fmt == SRCFMT_YUV420SP_TILED_MODE0) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  6;
                    } else if(src_fmt == SRCFMT_YUV420SP_TILED_MODE1) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  4;
                    }
                }

#ifdef CONFIG_AMP_VPP_ROTATION
                else if(src_fmt == SRCFMT_YUV420SP_TILED_V4H8) {
                    if(8 == bits_per_pixel) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  8;
                    } else if (10 == bits_per_pixel) {
                        ups_420_422_cfg10.uCFG10_tile_depth =  4;
                        ups_420_422_cfg10.uCFG10_tile_width =  6;
                    }
                }
#endif
            } else {
                ups_420_422_cfg10.uCFG10_tile_en = 0;
            }
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, ups_420_422_cfg10.u32);
            vpp_obj->plane[PLANE_PIP].m_uiTileCfg10_C = ups_420_422_cfg10.u32;

            //enable UPS420_2K used by pip detile
            vpp_obj->mainpl_ctrl4.uCTRL4_UPS420_2K_bypass = 0;
            RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->mainpl_ctrl4.u32);
        }
        /* ---- Tile mode programming ---- */
        /*PIP dolby path configuration*/
        VPP_DV_FE_Config(vpp_obj, PLANE_PIP, src_fmt, bits_per_pixel);
    }
#endif //ENABLE_PIP_GFX0_PATH
    else if(ldr_id == VPP_FE_DLR_GFX1) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL1;
        vpp_obj->gfx1_ctrl1.uCTRL1_idata_ctrl = 0;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx1_ctrl1.u32);
    }
#ifndef CONFIG_AMP_VPP_INVERSE_SCAN
    FE_DLR_SetBitmap(vpp_obj, ldr_id, src_fmt, order);
#endif
    FE_DLR_420_422_UPS_Config(vpp_obj, ldr_id, src_fmt, bits_per_pixel);
    FE_DLR_422_444_UPS_Config(vpp_obj, ldr_id, src_fmt);
    FE_DLR_SetVideoChannelCSC(vpp_obj, ldr_id, src_fmt);
    //FIXME: need to call the above api during reference window setting as well.

#ifdef CONFIG_AMP_VPP_ROTATION
    UINT32 uiDifBlkEn = 0;
    if(src_fmt == SRCFMT_YUV420SP_TILED_V4H8 || ((src_fmt == SRCFMT_YUV420SP_TILED_MODE0) && (10 == bits_per_pixel))) {
        uiDifBlkEn = 1;
    } else {
        uiDifBlkEn = 0;
    }
    switch(ldr_id) {
        case VPP_FE_DLR_MAIN:
            vpp_obj->mainplCtrl0.uCTRL0_dif_blk_en = uiDifBlkEn;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
                    vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0, vpp_obj->mainplCtrl0.u32);
            break;
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FE_DLR_PIP:
            if((OVPS_PARAMS->ovp_win_updated) && (OVPS_PARAMS->ovp_source == 5))
                vpp_obj->stElPipeCtrl1.uCTRL1_dif2k_blk_en = 1;
            else
                vpp_obj->stElPipeCtrl1.uCTRL1_dif2k_blk_en = uiDifBlkEn;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,
            vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1, vpp_obj->stElPipeCtrl1.u32);
            break;
#endif
        default:
            /*Do nothing */
            break;
    }
#endif
    return MV_VPP_OK;
}

INT FE_DLR_SetNonStdResPixLineTotal(VPP_OBJ *vpp_obj, INT Channel, PVPP_FE_DLR_INPUT_RES pReadSize) {
    INT RetVal = MV_VPP_OK;
    T32ReadClient_NonStdRes regval;
    UINT32 RegAddr;

    if((Channel < VPP_FE_DLR_CHANNEL_MIN) || (Channel > VPP_FE_DLR_CHANNEL_MAX))
        return MV_VPP_EBADPARAM;

    if(Channel == VPP_FE_DLR_CHANNEL_VMX_0) {
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_ClientIFm + RA_ClientIF_RdClientVmxVm + RA_ReadClient_NonStdRes;
        VPP_REG_WORD32_READ(RegAddr, &regval.u32);
        if(pReadSize->HRes%128 == 0)
            regval.uNonStdRes_enable = 0;
        else {
            regval.uNonStdRes_enable = 1;
            regval.uNonStdRes_pixlineTot = pReadSize->HRes;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, regval.u32);
    }
    return RetVal;

}
INT FE_DLR_ConfigTilePackSel(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, INT iTileW, INT iTileH)
{
    T32UPS_420_422_CFG10 ups_420_422_cfg10;
    UINT32 RegAddr_Y = 0, RegAddr_C = 0;

    if(PLANE_MAIN == uiPlaneId) {
        RegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_Y + RA_UPS_420_422_CFG10;
        RegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_UPS420_422Main_C + RA_UPS_420_422_CFG10;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if (PLANE_PIP == uiPlaneId) {
        RegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG10;
        RegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG10;
    }
#endif
    ups_420_422_cfg10.u32 = vpp_obj->plane[uiPlaneId].m_uiTileCfg10_Y;
    ups_420_422_cfg10.uCFG10_tile_depth =  iTileH;
    ups_420_422_cfg10.uCFG10_tile_width =  iTileW;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr_Y, ups_420_422_cfg10.u32);
    vpp_obj->plane[uiPlaneId].m_uiTileCfg10_Y =  ups_420_422_cfg10.u32;

    ups_420_422_cfg10.u32 = vpp_obj->plane[uiPlaneId].m_uiTileCfg10_C;
    ups_420_422_cfg10.uCFG10_tile_depth =  iTileH;
    ups_420_422_cfg10.uCFG10_tile_width =  iTileW;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr_C, ups_420_422_cfg10.u32);
    vpp_obj->plane[uiPlaneId].m_uiTileCfg10_C =  ups_420_422_cfg10.u32;

    return 0;
}

INT FE_DLR_ConfigCrLineCount(VPP_OBJ *vpp_obj, UINT uiPlaneId)
{
#ifdef CONFIG_AMP_VPP_ROTATION
    PLANE *plane = NULL;
    INT crLineCnt = 0;
    UINT32 uiRegAddr = 0;
    UINT32 uiRegVal = 0;

    plane = &(vpp_obj->plane[uiPlaneId]);

    if((VPP_UNIQUE_FORMAT_ID_10BIT_420_SP_TILED_MODE0 == plane->uchClientMode)
            && (ROTATION_90D == plane->uchRotationAngle || ROTATION_270D == plane->uchRotationAngle)) {
        crLineCnt = 2;
    } else {
        crLineCnt = 3;
    }
    switch(uiPlaneId) {
        case VPP_FE_DLR_MAIN:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0;
            vpp_obj->mainplCtrl0.uCTRL0_cr_line_cnt = crLineCnt;
            uiRegVal = vpp_obj->mainplCtrl0.u32;
            break;
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FE_DLR_PIP:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL2;
            vpp_obj->stElPipeCtrl2.uCTRL2_cr_line_cnt = crLineCnt;
            uiRegVal = vpp_obj->stElPipeCtrl2.u32;
            break;
#endif
        default:
            return -1;
    }
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, uiRegVal);
#endif
    return 0;
}

INT FE_DLR_ConfigDifBlock(VPP_OBJ *vpp_obj, INT ldr_id, INT iSrcFmt, INT iOrder, INT iBitsPerPixel, INT iRotation)
{
#ifdef CONFIG_AMP_VPP_ROTATION
    UINT32 uiRegAddr_Y = 0;
    UINT32 uiRegAddr_C = 0;
    UINT32 uiRotationSrcFmt = 0, uiRotationBitDepth = 0, uiRotationAngle = 0;
    T32DIF_CFG0 stDifCfg0;
    UINT32 j;

    memset(&stDifCfg0, 0, sizeof(T32DIF_CFG0));

    if(VPP_FE_DLR_MAIN == ldr_id) {
        uiRegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_DIF_Y;
        uiRegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_DIF_C;
    }
#ifdef ENABLE_PIP_GFX0_PATH
    else if(VPP_FE_DLR_PIP == ldr_id) {
        uiRegAddr_Y = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DIF2K_Y;
        uiRegAddr_C = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DIF2K_C;
    }
#endif
    switch(iSrcFmt) {
        case SRCFMT_YUV420SP_TILED_V4H8:
            {
                uiRotationSrcFmt = VPP_FE_DLR_ROTATION_SRCFMT_V4H8;

                switch(iBitsPerPixel) {
                    //V4H8
                    case 8:
                        {
                            uiRotationBitDepth = VPP_FE_DLR_BIT_DEPTH_8;
                            uiRotationAngle = iRotation;

                            stDifCfg0.uCFG0_sft_rst     = 1;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);

                            stDifCfg0.uCFG0_sft_rst     = 0;
                            stDifCfg0.uCFG0_tile_size   = 0x20;
                            stDifCfg0.uCFG0_PDWN0       = 0;
                            stDifCfg0.uCFG0_PDLVMC0     = 0;
                            stDifCfg0.uCFG0_PDFVSSM0    = 0;
                            stDifCfg0.uCFG0_PDWN1       = 0;
                            stDifCfg0.uCFG0_PDLVMC1     = 0;
                            stDifCfg0.uCFG0_PDFVSSM1    = 0;
                            stDifCfg0.uCFG0_PDWN2       = 0;
                            stDifCfg0.uCFG0_PDLVMC2     = 0;
                            stDifCfg0.uCFG0_PDFVSSM2    = 0;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);
                        }
                        break;
                    default:
                        break;
                }
            }

        case SRCFMT_YUV420SP_TILED_MODE0:
            {
                uiRotationSrcFmt = VPP_FE_DLR_ROTATION_SRCFMT_V4H8;
                switch(iBitsPerPixel) {
                    case 10:
                        {
                            uiRotationBitDepth = VPP_FE_DLR_BIT_DEPTH_10;
                            uiRotationAngle = iRotation;

                            stDifCfg0.uCFG0_sft_rst     = 1;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);

                            stDifCfg0.uCFG0_sft_rst     = 0;
                            stDifCfg0.uCFG0_tile_size   = 0x18;
                            stDifCfg0.uCFG0_PDWN0       = 0;
                            stDifCfg0.uCFG0_PDWN0       = 0;
                            stDifCfg0.uCFG0_PDLVMC0     = 0;
                            stDifCfg0.uCFG0_PDFVSSM0    = 0;
                            stDifCfg0.uCFG0_PDWN1       = 0;
                            stDifCfg0.uCFG0_PDLVMC1     = 0;
                            stDifCfg0.uCFG0_PDFVSSM1    = 0;
                            stDifCfg0.uCFG0_PDWN2       = 0;
                            stDifCfg0.uCFG0_PDLVMC2     = 0;
                            stDifCfg0.uCFG0_PDFVSSM2    = 0;

                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
                            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);

                            FE_DLR_ConfigCrLineCount(vpp_obj, ldr_id);
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

        for(j=0; j<=31; j++) {
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_DIFLUT + RA_DIF_LUT_DIFLUT0 + j*4, gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][j]);
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_DIFLUT + RA_DIF_LUT_DIFLUT0 + j*4, gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][j]);
        }
#ifdef BG6_commented//FIXME:VPP_SW_BRINGUP need to cleanup?
    //Config Y - LUT

    stDifCfg0.uCFG0_P0  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][0];
    stDifCfg0.uCFG0_P1  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][1];
    stDifCfg0.uCFG0_P2  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][2];
    stDifCfg0.uCFG0_P3  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][3];
    stDifCfg0.uCFG0_P4  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][4];
    stDifCfg0.uCFG0_P5  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][5];
    stDifCfg1.uCFG1_P6  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][6];
    stDifCfg1.uCFG1_P7  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][7];
    stDifCfg1.uCFG1_P8  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][8];
    stDifCfg1.uCFG1_P9  = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][9];
    stDifCfg1.uCFG1_P10 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][10];
    stDifCfg1.uCFG1_P11 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][11];
    stDifCfg2.uCFG2_P12 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][12];
    stDifCfg2.uCFG2_P13 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][13];
    stDifCfg2.uCFG2_P14 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][14];
    stDifCfg2.uCFG2_P15 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][15];
    stDifCfg2.uCFG2_P16 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][16];
    stDifCfg2.uCFG2_P17 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][17];
    stDifCfg3.uCFG3_P18 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][18];
    stDifCfg3.uCFG3_P19 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][19];
    stDifCfg3.uCFG3_P20 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][20];
    stDifCfg3.uCFG3_P21 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][21];
    stDifCfg3.uCFG3_P22 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][22];
    stDifCfg3.uCFG3_P23 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][23];
    stDifCfg4.uCFG4_P24 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][24];
    stDifCfg4.uCFG4_P25 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][25];
    stDifCfg4.uCFG4_P26 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][26];
    stDifCfg4.uCFG4_P27 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][27];
    stDifCfg4.uCFG4_P28 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][28];
    stDifCfg4.uCFG4_P29 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][29];
    stDifCfg5.uCFG5_P30 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][30];
    stDifCfg5.uCFG5_P31 = gVppFeRotationLUT_Y[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][31];

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG0, stDifCfg0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG1, stDifCfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG2, stDifCfg2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG3, stDifCfg3.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG4, stDifCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_Y + RA_DIF_CFG5, stDifCfg5.u32);

    //Configure UV - LUT
    stDifCfg0.uCFG0_P0  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][0];
    stDifCfg0.uCFG0_P1  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][1];
    stDifCfg0.uCFG0_P2  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][2];
    stDifCfg0.uCFG0_P3  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][3];
    stDifCfg0.uCFG0_P4  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][4];
    stDifCfg0.uCFG0_P5  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][5];
    stDifCfg1.uCFG1_P6  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][6];
    stDifCfg1.uCFG1_P7  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][7];
    stDifCfg1.uCFG1_P8  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][8];
    stDifCfg1.uCFG1_P9  = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][9];
    stDifCfg1.uCFG1_P10 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][10];
    stDifCfg1.uCFG1_P11 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][11];
    stDifCfg2.uCFG2_P12 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][12];
    stDifCfg2.uCFG2_P13 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][13];
    stDifCfg2.uCFG2_P14 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][14];
    stDifCfg2.uCFG2_P15 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][15];
    stDifCfg2.uCFG2_P16 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][16];
    stDifCfg2.uCFG2_P17 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][17];
    stDifCfg3.uCFG3_P18 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][18];
    stDifCfg3.uCFG3_P19 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][19];
    stDifCfg3.uCFG3_P20 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][20];
    stDifCfg3.uCFG3_P21 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][21];
    stDifCfg3.uCFG3_P22 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][22];
    stDifCfg3.uCFG3_P23 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][23];
    stDifCfg4.uCFG4_P24 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][24];
    stDifCfg4.uCFG4_P25 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][25];
    stDifCfg4.uCFG4_P26 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][26];
    stDifCfg4.uCFG4_P27 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][27];
    stDifCfg4.uCFG4_P28 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][28];
    stDifCfg4.uCFG4_P29 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][29];
    stDifCfg5.uCFG5_P30 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][30];
    stDifCfg5.uCFG5_P31 = gVppFeRotationLUT_UV[uiRotationSrcFmt][uiRotationBitDepth][uiRotationAngle][31];

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG0, stDifCfg0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG1, stDifCfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG2, stDifCfg2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG3, stDifCfg3.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG4, stDifCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr_C + RA_DIF_CFG5, stDifCfg5.u32);
#endif
#endif
    return 0;
}
INT FE_DLR_ConfigElPipe(VPP_OBJ *vpp_obj, INT iSrcFmt, INT iOrder, INT iBitDepth, INT iEnable)
{
#ifdef ENABLE_EL_PIPE_PATH
     T32EL_PIPE_CTRL1 stElPipeCtrl1;
     T32GFX0_TILE_CTRL stGfx0TileCtrl;
     T32UPS_420_422_CFG0 stUps420_422Cfg0_Y, stUps420_422Cfg0_UV;
     T32UPS_420_422_CFG5 stUps420_422Cfg5_Y, stUps420_422Cfg5_UV;
     T32UPS_420_422_CFG10 stUps420_422Cfg10;
     UINT32 uiRotationAngle = 0;

#if defined(RA_VPTOP_CTRL_RdClientdiR0) || defined(RA_VPTOP_CTRL_RdClientdiR1)
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
     VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1, stElPipeCtrl1.u32);
     stUps420_422Cfg5_Y.uCFG5_auto_pixcnt = stUps420_422Cfg5_UV.uCFG5_auto_pixcnt = 1;

    switch(iSrcFmt) {
            case SRCFMT_YUV422:
            case SRCFMT_YUV420SP:
            {
                stUps420_422Cfg0_Y.uCFG0_enable = 0;
                stUps420_422Cfg0_UV.uCFG0_enable = 0;
                stUps420_422Cfg0_Y.uCFG0_out422 = 0;
                stUps420_422Cfg0_UV.uCFG0_out422 = 0;

                stUps420_422Cfg10.uCFG10_tile_en = 0;
                stElPipeCtrl1.uCTRL1_dif2k_blk_en = 0;
#if defined(RA_VPTOP_CTRL_RdClientdiR0) || defined(RA_VPTOP_CTRL_RdClientdiR1)
                if (8 == iBitDepth) {
                    uiElPackSel = 1;
                }
                else if (10 == iBitDepth) {
                    uiElPackSel = 2;
                }
#endif
            }
            break;

        case SRCFMT_YUV420SP_TILED_MODE0:
            {
                 if(8 == iBitDepth) {
                     stUps420_422Cfg10.uCFG10_tile_depth =  4;
                     stUps420_422Cfg10.uCFG10_tile_width =  8;
#if defined(RA_VPTOP_CTRL_RdClientdiR0) || defined(RA_VPTOP_CTRL_RdClientdiR1)
                     uiElPackSel = 1;
#endif
                 } else if (10 == iBitDepth) {
                     stUps420_422Cfg10.uCFG10_tile_depth =  4;
                     stUps420_422Cfg10.uCFG10_tile_width =  6;
#if defined(RA_VPTOP_CTRL_RdClientdiR0) || defined(RA_VPTOP_CTRL_RdClientdiR1)
                     uiElPackSel = 3;
#endif
                 }

                FE_DLR_ConfigDifBlock(vpp_obj, PLANE_PIP, iSrcFmt, iOrder, iBitDepth, uiRotationAngle);
            }
            break;

        case SRCFMT_YUV420SP_TILED_MODE1:
            {
                stUps420_422Cfg10.uCFG10_tile_depth =  4;
                stUps420_422Cfg10.uCFG10_tile_width =  4;
#if defined(RA_VPTOP_CTRL_RdClientdiR0) || defined(RA_VPTOP_CTRL_RdClientdiR1)
                uiElPackSel = 1;
#endif
                FE_DLR_ConfigDifBlock(vpp_obj, PLANE_PIP, iSrcFmt, iOrder, iBitDepth, uiRotationAngle);
            }
            break;

        case SRCFMT_YUV420SP_TILED_V4H8:
            {
                stUps420_422Cfg10.uCFG10_tile_depth =  4;
                stUps420_422Cfg10.uCFG10_tile_width =  8;
#if defined(RA_VPTOP_CTRL_RdClientdiR0) || defined(RA_VPTOP_CTRL_RdClientdiR1)
                uiElPackSel = 1;
#endif
                FE_DLR_ConfigDifBlock(vpp_obj, PLANE_PIP, iSrcFmt, iOrder, iBitDepth, uiRotationAngle);
            }
            break;
        default:
            break;
    }
    if(VPP_IS_420SP_TILED_FMT(iSrcFmt)) {
        stUps420_422Cfg0_Y.uCFG0_enable = 1;
        stUps420_422Cfg0_UV.uCFG0_enable = 1;
        stUps420_422Cfg0_Y.uCFG0_out422 = 0;
        stUps420_422Cfg0_UV.uCFG0_out422 = 0;

        stUps420_422Cfg10.uCFG10_tile_en = 1;
    }

    if(!OVPS_PARAMS->readTiledfromOVPclient) {
        if(8 == iBitDepth) {
            stElPipeCtrl1.uCTRL1_idatctrl = 0;
        } else if(10 == iBitDepth) {
            stElPipeCtrl1.uCTRL1_idatctrl = 1;
        }
    } else {
          if(8 == iBitDepth) {
              stElPipeCtrl1.uCTRL1_idatctrl = 4;
          } else if(10 == iBitDepth) {
              stElPipeCtrl1.uCTRL1_idatctrl = 5;
          }
    }
    if(!OVPS_PARAMS->readTiledfromOVPclient) {
        if(iEnable) {
            //EL pipe control settings
            stElPipeCtrl1.uCTRL1_pip_rd_sel = 0;
            stElPipeCtrl1.uCTRL1_dint_rd_sel = 1;
        } else {
            stUps420_422Cfg5_Y.uCFG5_auto_pixcnt = stUps420_422Cfg5_UV.uCFG5_auto_pixcnt = 0;
            //EL pipe control settings
            stElPipeCtrl1.uCTRL1_pip_rd_sel = 0;
            stElPipeCtrl1.uCTRL1_dint_rd_sel = 0;
        }
            stElPipeCtrl1.uCTRL1_read_en = iEnable;
            stElPipeCtrl1.uCTRL1_el_rdsel_420SP = iEnable;
            stGfx0TileCtrl.uTILE_CTRL_detile_inp_en = 0;
            stGfx0TileCtrl.uTILE_CTRL_infifo_ctrl   = 0;
            stGfx0TileCtrl.uTILE_CTRL_pipInfifo_ctrl= 0;
            stGfx0TileCtrl.uTILE_CTRL_pip_tile_sel  = 0;
            stGfx0TileCtrl.uTILE_CTRL_tile_inp_sel  = 1;

    }
    //PackSel
#ifdef RA_VPTOP_CTRL_RdClientdiR0
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR0 + RA_ReadClient_pack, uiElPackSel);
#endif
#ifdef RA_VPTOP_CTRL_RdClientdiR1
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_RdClientdiR1 + RA_ReadClient_pack, uiElPackSel);
#endif


    vpp_obj->stElPipeCtrl1.u32 = stElPipeCtrl1.u32;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_CTRL1, stElPipeCtrl1.u32);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_TILE_CTRL, stGfx0TileCtrl.u32);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG5,stUps420_422Cfg5_Y.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG5,stUps420_422Cfg5_UV.u32);

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG10, stUps420_422Cfg10.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG10, stUps420_422Cfg10.u32);
    vpp_obj->plane[PLANE_PIP].m_uiTileCfg10_C = stUps420_422Cfg10.u32;
    vpp_obj->plane[PLANE_PIP].m_uiTileCfg10_Y = stUps420_422Cfg10.u32;

    vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y = stUps420_422Cfg0_Y.u32;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG0, stUps420_422Cfg0_Y.u32);
    vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C = stUps420_422Cfg0_UV.u32;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG0, stUps420_422Cfg0_UV.u32);

#endif //ENABLE_EL_PIPE_PATH
    return 0;
}

INT FE_DLR_ConfigElPipeTg(VPP_OBJ *vpp_obj, UINT32 uiPlaneId, PVPP_FE_DLR_INPUT_RES pTotRes, PVPP_FE_DLR_INPUT_CROP pCropRes)
{
    INT RetVal = MV_VPP_OK;
#ifdef ENABLE_EL_PIPE_PATH
    UINT32 RegAddr = 0, RegVal = 0;
    INT Crop1_Start_X = 0, Crop1_End_X = 0, Crop1_Start_Y_Luma = 0, Crop1_End_Y_Luma = 0, Crop1_Start_Y_Chroma = 0, Crop1_End_Y_Chroma = 0;
    INT Luma_Crop2_Start_X = 0, Luma_Crop2_End_X = 0, Luma_Crop2_Start_Y = 0, Luma_Crop2_End_Y = 0;
    INT Chroma_Crop2_Start_X = 0, Chroma_Crop2_End_X = 0, Chroma_Crop2_Start_Y = 0, Chroma_Crop2_End_Y = 0;
    INT Chroma_Pix_Off = -1, Luma_Pix_Off = -1, Luma_Line_Off = -1, Chroma_Line_Off = -1;

    T32UPS_420_422_CFG6 stUPS_420_422_CFG6;
    T32UPS_420_422_CFG6 stUPS_420_422_CFG6_1;
    T32UPS_420_422_CFG0 stUPS_420_422_CFG0;

    UINT32 leftcropy = 0,rightcropy = 0,topcropy = 0,bottomcropy = 0;
    UINT32 leftcropc = 0,rightcropc = 0,topcropc = 0,bottomcropc = 0;

    leftcropy = vpp_obj->plane[PLANE_MAIN].m_leftpadadjust_y;
    rightcropy = vpp_obj->plane[PLANE_MAIN].m_rightpad_y;
    topcropy = vpp_obj->plane[PLANE_MAIN].m_toppadadjust_y;
    bottomcropy = vpp_obj->plane[PLANE_MAIN].m_bottompad_y;

    leftcropc = vpp_obj->plane[PLANE_MAIN].m_leftpadadjust_c;
    rightcropc = vpp_obj->plane[PLANE_MAIN].m_rightpad_c;
    topcropc = vpp_obj->plane[PLANE_MAIN].m_toppadadjust_c;
    bottomcropc = vpp_obj->plane[PLANE_MAIN].m_bottompad_c;


    /* Update the 420_422_cfg registers for getting black pixel padding */
    // Y cropping
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG6;
    stUPS_420_422_CFG6.u32 = vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_Y;
    stUPS_420_422_CFG6.uCFG6_left_crop = leftcropy;
    stUPS_420_422_CFG6.uCFG6_right_crop = rightcropy;
    stUPS_420_422_CFG6.uCFG6_bot_crop = bottomcropy;
    stUPS_420_422_CFG6.uCFG6_top_crop  = topcropy;

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG6.u32)))
        return RetVal;
    vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_Y = stUPS_420_422_CFG6.u32;

    //Cr cropping
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG6;
    stUPS_420_422_CFG6_1.u32 = vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_C;
    stUPS_420_422_CFG6_1.uCFG6_left_crop = leftcropc;
    stUPS_420_422_CFG6_1.uCFG6_right_crop = rightcropc;
    stUPS_420_422_CFG6_1.uCFG6_bot_crop = bottomcropc;
    stUPS_420_422_CFG6_1.uCFG6_top_crop  = topcropc;

    /* Write the data to BCM buffer */
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG6_1.u32)))
        return RetVal;
    vpp_obj->plane[PLANE_PIP].m_uiTileCfg6_C = stUPS_420_422_CFG6_1.u32;

    if(vpp_obj->plane[PLANE_MAIN].m_SampleCropEnable_el) {

        /* use_blank & crop_blank programming  for Y & C*/
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KY + RA_UPS_420_422_CFG0;
        stUPS_420_422_CFG0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y;
        stUPS_420_422_CFG0.uCFG0_use_blank = 1;
        stUPS_420_422_CFG0.uCFG0_crop_blank = 0;

        /* Write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0.u32)))
            return RetVal;
        vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_Y = stUPS_420_422_CFG0.u32;

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_DTILE_UPS420_2KC + RA_UPS_420_422_CFG0;
        stUPS_420_422_CFG0.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C;
        stUPS_420_422_CFG0.uCFG0_use_blank = 1;
        stUPS_420_422_CFG0.uCFG0_crop_blank = 0;

        /* Write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, stUPS_420_422_CFG0.u32)))
            return RetVal;
        vpp_obj->plane[PLANE_PIP].ui420UpsCfg0_C = stUPS_420_422_CFG0.u32;

        /* FETG Cropping for compressed inputs will be done in two stages : CROP1 & CROP2*/
        /* CROP1 will program the HB, HB_OUT VB,VB_OUT registers */
        /* CROP2 will program the HB_Y,VB_Y,HB_C,VB_C registers */

        /* ###### CROP 1 ###### */
        /* STARTX = max(leftcropy,leftcropc) ENDX = IHRES-min(rightcropy,rightcropc) */
        Crop1_Start_X = (leftcropy > leftcropc)?leftcropy:leftcropc;
        Crop1_End_X = (rightcropy < rightcropc)?(pTotRes->HRes-rightcropy):(pTotRes->HRes-rightcropc);

        Crop1_Start_Y_Luma = topcropy;
        Crop1_End_Y_Luma = pTotRes->VRes - bottomcropy;

        Crop1_Start_Y_Chroma = topcropc;
        Crop1_End_Y_Chroma = vpp_obj->plane[PLANE_MAIN].m_El_Aligned_Chroma_Height - bottomcropc;

        if(vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en) {
            /* ###### CROP2 ###### */
            Luma_Crop2_Start_X = 0;
            Luma_Crop2_End_X = vpp_obj->plane[PLANE_MAIN].m_El_Aligned_Luma_Width;
            Luma_Crop2_Start_Y = 0;
            Luma_Crop2_End_Y = vpp_obj->plane[PLANE_MAIN].m_El_Aligned_Luma_Height;

            /* ###### CROP2 ###### */
            Chroma_Crop2_Start_X = 0;
            Chroma_Crop2_End_X = vpp_obj->plane[PLANE_MAIN].m_El_Aligned_Chroma_Width;;
            Chroma_Crop2_Start_Y = 0;
            Chroma_Crop2_End_Y = vpp_obj->plane[PLANE_MAIN].m_El_Aligned_Luma_Height;

            /* Calculation of pixel offset for CROP2 */
            /* chroma has more garbage to the left of actual image than luma */
            if(leftcropy < leftcropc) {
                Chroma_Pix_Off = 0; /* no offset for chroma */
                Luma_Pix_Off = (leftcropc - leftcropy); /*delay luma read by the difference in the crop*/
            } else { /* luma has equal or more garbage to the left of actual image than chroma */
                Luma_Pix_Off = 0; /* no offset for luma */
                Chroma_Pix_Off = (leftcropy - leftcropc);/*delay in chroma read by the difference in crop */
            }

            /* Calculation of Line offset for CROP2 */
            /* chroma has more garbage to the top of actual image than luma */
            if(topcropy < topcropc) {
                Chroma_Line_Off = 0; /* no offset for chroma */
                Luma_Line_Off = (topcropc - topcropy);/* delay Luma read by the difference in the crop */
            } else { /* luma has equal or more garbage to the top of actual image than chroma */
                Luma_Line_Off = 0;/* no offset for luma */
                Chroma_Line_Off = (topcropy - topcropc);
            }
        }


        /* FETG MAIN SIZE register programming */
        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_feTG_EL;
        RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_IHRES_OFF + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);

        /* write TG size to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
            return RetVal;

        /* calculate the H blank front edge value */
        RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);
        /*write the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
            return RetVal;

        /* calculate the V blank front edge value */
        RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);
        /*writer the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
            return RetVal;

        if(vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en) {
            /* Program the HB_Y,VB0_Y,HB_C,VB0_C registers */
            RegVal = (Luma_Crop2_End_X + Luma_Pix_Off + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Luma_Crop2_Start_X + Luma_Pix_Off + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
                return RetVal;

            RegVal = (Luma_Crop2_End_Y + Luma_Line_Off + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Luma_Crop2_Start_Y + Luma_Line_Off + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
                return RetVal;

            RegVal = (Chroma_Crop2_End_X + Chroma_Pix_Off + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Chroma_Crop2_Start_X + Chroma_Pix_Off + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
                return RetVal;

            RegVal = (Chroma_Crop2_End_Y + Chroma_Line_Off + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Chroma_Crop2_Start_Y + Chroma_Line_Off + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
                return RetVal;
        } else {

            /* Program the HB_Y,VB0_Y,HB_C,VB0_C registers */
            RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
                return RetVal;

            RegVal = (Crop1_End_Y_Luma + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y_Luma + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
                return RetVal;

            RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
                return RetVal;

            RegVal = (Crop1_End_Y_Chroma + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y_Chroma + VPP_FE_DUMMY_TG_VB0_BE)<<16);
            /* write the data to BCM buffer */
            if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
                return RetVal;
        }

        /* Program the HB_OUT & VB0_OUT registers */
        RegVal = (Crop1_End_X + VPP_FE_DUMMY_TG_HB_FE_OFF) + ((Crop1_Start_X + VPP_FE_DUMMY_TG_HB_BE)<<16);
        /* write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
            return RetVal;

        RegVal = (Crop1_End_Y_Luma + VPP_FE_DUMMY_TG_VB0_FE_OFF) + ((Crop1_Start_Y_Luma + VPP_FE_DUMMY_TG_VB0_BE)<<16);
        /* write the data to BCM buffer */
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
            return RetVal;

    } else {
        UINT32 Val = 0, Addr = 0;
        vpp_obj->mainplCtrl0.uCTRL0_crop_de2_en = 0;
        Val = vpp_obj->mainplCtrl0.u32;
        Addr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0;
        /* Programming the crop2 enable register */
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, Addr, Val);

        RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_feTG_EL;

        RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
        /*write TG size to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
            return RetVal;

        /*calculate the H blank front edge value*/
        RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);
        /*write the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
            return RetVal;


        RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_FE_DUMMY_TG_HB_FE_OFF + (((pCropRes->leftCrop)+VPP_FE_DUMMY_TG_HB_BE) <<16);
        /*write the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
            return RetVal;
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
            return RetVal;
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
            return RetVal;

        /*calculate the V blank front edge value*/
        RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);
        /*writer the data to BCM buffer*/
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
            return RetVal;

        RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((pCropRes->topCrop+VPP_FE_DUMMY_TG_VB0_BE) <<16);
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
            return RetVal;
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
            return RetVal;
        if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
            return RetVal;
    }
#endif //ENABLE_EL_PIPE_PATH
    return RetVal;
}
INT FE_DLR_ConfigOfflineRD(VPP_OBJ *vpp_obj, int width, int height)
{
    VPP_FE_DLR_INPUT_RES totres;
    VPP_FE_DLR_INPUT_CROP inpcrop;

    totres.HRes = width;
    totres.VRes = height;

    inpcrop.leftCrop = 0;
    inpcrop.rightCrop = 0;
    inpcrop.topCrop = 0;
    inpcrop.bottomCrop = 0;
    FE_DLR_SetDummyTG_offlineRD(vpp_obj, 0, &totres, &inpcrop);
    return 0;
}

/***************************************************************************************
 * FUNCTION: set dummy TG for main video channel.
 * PARAMS: TotRes - input total resolution.
 *                progressive - 1: prgressive, 0: interlaced.
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_DLR_SetDummyTG_RasterScan(VPP_OBJ *vpp_obj, INT ldr_id,
        PVPP_FE_DLR_INPUT_RES pTotRes,
        PVPP_FE_DLR_INPUT_CROP pCropRes,
        int progressive)
{
    INT RetVal = MV_VPP_OK;
#ifdef ENABLE_PIP_GFX0_PATH
    UINT32 RegAddr;
    UINT32 RegVal;

    T32RASTERSCAN_RD_CTRL1 stRdCtrl1;

    stRdCtrl1.u32 = vpp_obj->uiRasterScanRdCtrl1;
    stRdCtrl1.uCTRL1_crop_de_en = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_CTRL1, stRdCtrl1.u32);
    vpp_obj->uiRasterScanRdCtrl1 = stRdCtrl1.u32;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_RASTERSCAN_RD + RA_RASTERSCAN_RD_rsFeTG;


    if (progressive) {
        RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_P) << 16);
    } else {
        RegVal = (pTotRes->VRes + VPP_FE_DUMMY_TG_SIZE_V_OFF_I) + ((pTotRes->HRes + VPP_FE_DUMMY_TG_SIZE_H_OFF_I) << 16);
    }

    /*write TG size to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pTotRes->HRes + VPP_FE_DUMMY_TG_HB_FE_OFF + ((VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pTotRes->VRes + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((VPP_FE_DUMMY_TG_VB0_BE) << 16);

    /*writer the data to BCM buffer*/
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0), RegVal)))
        return RetVal;

    RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_FE_DUMMY_TG_HB_FE_OFF + (((pCropRes->leftCrop)+VPP_FE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/

    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_Y), RegVal)))
        return RetVal;
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_C), RegVal)))
        return RetVal;

    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_HB_OUT), RegVal)))
        return RetVal;

    RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_FE_DUMMY_TG_VB0_FE_OFF + ((pCropRes->topCrop+VPP_FE_DUMMY_TG_VB0_BE) << 16);

    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_Y), RegVal)))
        return RetVal;
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_C), RegVal)))
        return RetVal;

    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_MAIN_VB0_OUT), RegVal)))
        return RetVal;
#endif //ENABLE_PIP_GFX0_PATH
    return RetVal;
}

INT FE_DLR_ConfigRasterScanLoader(VPP_OBJ *vpp_obj, INT iLdrSel, INT iOutSel, INT iBps, INT iEnable)
{
    INT RetVal = MV_VPP_OK;
#ifdef ENABLE_RASTERSCAN
    UINT32 RegAddr;
    T32RASTERSCAN_RD_CTRL1 stRdCtrl1;
    T32EL_PIPE_RASTERSCAN stElPipeRasterScan;
    T32GFX0_RASTERSCAN stGfx0RasterScan;
    T32UPS_420_422_SP_CFG0 stGfx0Ups;
    T32GFX0_CTRL0 stGfx0Ctrl0;

    stRdCtrl1.u32 = vpp_obj->uiRasterScanRdCtrl1;
    stElPipeRasterScan.u32 = 0;
    stGfx0RasterScan.u32 = 0;
    stGfx0Ups.u32 = vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0;
    stGfx0Ctrl0.u32  =   vpp_obj->gfx0_ctrl0.u32;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_RASTERSCAN_RD;
    stRdCtrl1.uCTRL1_fifo_sts_ctrl = !(iOutSel);
    stRdCtrl1.uCTRL1_fifo_full_ctrlEn = iOutSel;
    stRdCtrl1.uCTRL1_fifo_full_ctrl1 = iOutSel;
    stRdCtrl1.uCTRL1_fifo_full_ctrl2 = iOutSel;
    stRdCtrl1.uCTRL1_clken_ctrl0 = 0;
    stRdCtrl1.uCTRL1_clken_ctrl1 = 0;

    switch(iLdrSel) {
#ifdef ENABLE_EL_PIPE_PATH
        case VPP_FE_DLR_RASTERSCAN_EL:
            {
                stRdCtrl1.uCTRL1_read_sel = 1;
            }
            break;
#endif
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FE_DLR_RASTERSCAN_PIP:
            {
                stRdCtrl1.uCTRL1_read_sel = 2;
                if(VPP_FE_DLR_RASTERSCAN_OUT_DVCOMP == iOutSel) {
                    VPPAPI_DBG_INFO("Enabling Reading of EL through PIP client using RasterScan Module\n");
                    stElPipeRasterScan.uRASTERSCAN_el_composer_420sp = 1;
                    stGfx0RasterScan.uRASTERSCAN_gfx0_rd_sel_420sp_en = 1;
                    stGfx0Ups.uCFG0_enable = 0;
                    stGfx0Ctrl0.uCTRL0_ups422_idata_ctrl = 0;
                }
            }
            break;
#endif
        case VPP_FE_DLR_RASTERSCAN_BL:
        default:
            {
                stRdCtrl1.uCTRL1_read_sel = 0;
                stRdCtrl1.uCTRL1_fifo_sts_ctrl = 0;
            }
            break;
    }

    if(8 == iBps) {
        stRdCtrl1.uCTRL1_ups420_idat_sel = 0;
    } else if(10 == iBps) {
        stRdCtrl1.uCTRL1_ups420_idat_sel = 1;
    }


    if(0 == iEnable) {
        VPPAPI_DBG_INFO("Disbling Reading of EL through PIP client using RasterScan Module. EL will be read from EL client\n");
        stElPipeRasterScan.uRASTERSCAN_el_composer_420sp = 0;
        stGfx0RasterScan.uRASTERSCAN_gfx0_rd_sel_420sp_en = 0;
        stRdCtrl1.uCTRL1_read_sel = 0;
        stRdCtrl1.uCTRL1_fifo_sts_ctrl = 0;
    }

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_RASTERSCAN_RD_CTRL1, stRdCtrl1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_EL_PIPE + RA_EL_PIPE_RASTERSCAN, stElPipeRasterScan.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL0, stGfx0Ctrl0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_RASTERSCAN, stGfx0RasterScan.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_SP + RA_UPS_420_422_SP_CFG0, stGfx0Ups.u32);

    vpp_obj->uiRasterScanRdCtrl1 = stRdCtrl1.u32;
    vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0 = stGfx0Ups.u32;
    vpp_obj->gfx0_ctrl0.u32 = stGfx0Ctrl0.u32;
    vpp_obj->plane[PLANE_PIP].ui420UpsSPCfg0 = stGfx0Ups.u32;
#endif //ENABLE_RASTERSCAN
    return RetVal;
}

INT FE_DLR_EnableDeAlpha(VPP_OBJ *vpp_obj, UINT uiPlaneId, BOOL Enable)
{
    UINT32 RegAddr, RegAddr1;
    T32GFX1_CTRL *pgfx_ctrl = NULL;
    T32GFX1_CTRL1 *pgfx_ctrl1 = NULL;
    T32ALPHADIV_CFG0 alphadiv_cfg0;

    if((uiPlaneId != PLANE_GFX1)
#ifdef ENABLE_PIP_GFX0_PATH
            && (uiPlaneId != PLANE_PIP)
#endif
#ifdef ENABLE_GFX2_VOP2_PATH
            && (uiPlaneId != PLANE_GFX2)
#endif
        ) {
        return (MV_VPP_EBADPARAM);
    }
    switch(uiPlaneId) {
#ifdef ENABLE_PIP_GFX0_PATH
        case PLANE_PIP:
            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL0;
            vpp_obj->gfx0_ctrl0.uCTRL0_pixel_crop_en = Enable;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ctrl0.u32);

            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL1;
            vpp_obj->gfx0_ctrl1.uCTRL1_alphadiv_bypass = Enable?0:1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ctrl1.u32);

            RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_ALPHADIV + RA_ALPHADIV_CFG0;
            break;
#endif
        case PLANE_GFX1:
            pgfx_ctrl = &vpp_obj->gfx1_ctrl;
            pgfx_ctrl1 = &vpp_obj->gfx1_ctrl1;
            RegAddr1 = vpp_obj->base_addr + RA_Vpp_GFX1;
#ifdef ENABLE_GFX2_VOP2_PATH
        case PLANE_GFX2:
            if (uiPlaneId == PLANE_GFX2) {
                pgfx_ctrl = &vpp_obj->gfx2_ctrl;
                pgfx_ctrl1 = &vpp_obj->gfx2_ctrl1;
                RegAddr1 = vpp_obj->base_addr + RA_Vpp_GFX2;
            }
#endif
            RegAddr = RegAddr1 + RA_GFX1_CTRL;
            pgfx_ctrl->uCTRL_pixel_crop_en = Enable;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, pgfx_ctrl->u32);

            RegAddr = RegAddr1 + RA_GFX1_CTRL1;
            pgfx_ctrl1->uCTRL1_alphadiv_bypass = Enable?0:1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, pgfx_ctrl1->u32);

            RegAddr = RegAddr1 + RA_GFX1_ALPHADIV + RA_ALPHADIV_CFG0;
            break;
        default:
            return (MV_VPP_EBADPARAM);
    }
    GA_REG_WORD32_READ(RegAddr, &alphadiv_cfg0.u32);
    alphadiv_cfg0.uCFG0_div_enb = Enable;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, alphadiv_cfg0.u32);
    return MV_VPP_OK;
}
