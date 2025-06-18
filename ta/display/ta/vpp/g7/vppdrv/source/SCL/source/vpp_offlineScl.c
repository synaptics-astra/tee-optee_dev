/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_OFFLINE_SCL_C_

#include "vpp_module.h"
#include "vpp.h"
#include "vpp_fe_dlr.h"
#include "vpp_bcmbuf.h"
#include "api_avio_dhub.h"
#include "avioDhub.h"
#include "vpp_frc_scl.h"
#include "vpp_offlineScl.h"
#include "vpp_cfg.h"
#include "vpp_isr.h"
#include "ctypes.h"

//Enable macro to disable offline scaler for BG5CTp/Vs680
//#define CONFIG_VPP_DISABLE_OFFLINE_SCALER
//Delay enabling offline scaler for BG5CTp
#define CONFIG_VPP_DELAY_OFFLINE_SCALER
//Valid only if CONFIG_VPP_DELAY_OFFLINE_SCALER is enabled
#define VPP_DELAY_OFFLINE_SCALER_IN_FRAMES  1

/* As VIP pipe is shared between MAIN_offline_downscale and HDMI_Rx; linedrop is the option,
 when HDMI_Rx is pushing buffer to VPP*/
#define GET_RESOLUTION_INFO()                 RESOLUTION_INFO  *pResinfo;\
                                            pResinfo = VPP_GetResInfo(vpp_obj->uiResID[cpcbID]);

#define VPP_OFFLINESCL_IS_FORCED_LINEDROP()  ((SCAN_INTERLACED == pResinfo->scan)\
            || (pResinfo->freq < 148352) \
            || (VBUF_SRC_HDMIRX == plane->uiBufSrc)\
            || VPP_OFFLINESCL_IS_LINEDROP_ONLY(pVppSclRes->IVRes, pVppSclRes->OVRes))

extern UNSG32 vppDhubCfg2NDDMA(VPP_OBJ *vpp_obj, VPP_DHUB_CFG_2NDDMA_PARAM *pstCfg2NDDMAParam);

/***************************************************************************************
 * FUNCTION: Load Default values for VIP path SCL
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
INT  VPP_OfflineScl_LoadDefaultVal(VPP_OBJ *vpp_obj)
{
    UINT32 uiRegAddr = 0;
    T32OVPSCL_CTRL stOvpSclCtrl;

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCLTOP;
    GA_REG_WORD32_READ(uiRegAddr + RA_VIPSCLTOP_CTRL5, &vpp_obj->stVipSclTopCtrl5.u32);

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y;
    GA_REG_WORD32_READ(uiRegAddr + RA_OVPSCL_CTRL, &stOvpSclCtrl.u32);
    stOvpSclCtrl.uCTRL_dither_bypass = 1;
    GA_REG_WORD32_WRITE(uiRegAddr + RA_OVPSCL_CTRL, stOvpSclCtrl.u32);

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV;
    GA_REG_WORD32_READ(uiRegAddr + RA_OVPSCL_CTRL, &stOvpSclCtrl.u32);
    stOvpSclCtrl.uCTRL_dither_bypass = 1;
    GA_REG_WORD32_WRITE(uiRegAddr + RA_OVPSCL_CTRL, stOvpSclCtrl.u32);

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION:Configure offline read based on new windows.
 *          Needs to call when there is any change in windows.
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 *          plane - Pointer to current PLANE
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static int VPP_offlineScl_offlineReadConfig(VPP_OBJ *vpp_obj, PLANE *plane)
{
    VPP_FE_DLR_INPUT_RES stTotRes;
    VPP_FE_DLR_INPUT_CROP stCropRes;
    /* Configure offlineRd  */
    FE_OfflineRd_Config(vpp_obj, VPP_FE_DLR_CHANNEL_OFFLINE_RD_0);
    /* offlineRd TG programming */
    stTotRes.HRes = plane->offline_buff_descrs[plane->rb_buff_index].content_win.width;
    stTotRes.VRes = plane->offline_buff_descrs[plane->rb_buff_index].content_win.height;
    stCropRes.leftCrop      = 0;
    stCropRes.rightCrop     = 0;
    stCropRes.topCrop       = 0;
    stCropRes.bottomCrop    = 0;
    FE_DLR_SetDummyTG_offlineRD(vpp_obj, VPP_FE_DLR_CHANNEL_OFFLINE_RD_0, &stTotRes, &stCropRes);
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION:Start offline read by setting client and dHub
 *          Needs to call for every interrupt.
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 *          plane - Pointer to current PLANE
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static int VPP_offlineScl_offlineReadStart(VPP_OBJ *vpp_obj, PLANE *plane)
{
    INT rb_i = -1;
    SIGN32      uiOfflineRdStride = 0;
    INT *cfgQ = NULL, *cfgQ_shadow = NULL, *cfgQ_len = NULL;
    INT cpcbID = CPCB_OF_CHAN(vpp_obj, plane->chanID);
    VPP_DHUB_CFG_2NDDMA_PARAM stCfg2NDDMAParam;


    cfgQ_shadow = vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ->addr;
    cfgQ_len = &(vpp_obj->dv[cpcbID].curr_cpcb_vbi_dma_cfgQ->len);

    rb_i = plane->rb_buff_index;
    /* offlineRd TG  & offlineRd client clear & start */
    FE_DLR_ClearChannel(vpp_obj, VPP_FE_DLR_CHANNEL_OFFLINE_RD_0);
    FE_DLR_EnableChannel(vpp_obj, VPP_FE_DLR_CHANNEL_OFFLINE_RD_0, plane->offline_scl_srcfmt, 1);
    /* Programe offlineRd DHub */
    uiOfflineRdStride = plane->offline_buff_descrs[rb_i].stride;
    cfgQ = cfgQ_shadow + *cfgQ_len * 2;
    stCfg2NDDMAParam.dhubId = plane->edr_dmaRdhubID;
    stCfg2NDDMAParam.dmaId  = plane->edr_dmaRID;
    stCfg2NDDMAParam.addr   = (UNSG32)plane->offline_buff_descrs[rb_i].addr;
    stCfg2NDDMAParam.width  = plane->offline_buff_descrs[rb_i].content_win.width *  plane->offline_buff_descrs[rb_i].uiBitsPerPixel / VPP_OFFLINESCL_BITS_PER_BYTES;
    stCfg2NDDMAParam.stride = uiOfflineRdStride;
    stCfg2NDDMAParam.height = plane->offline_buff_descrs[rb_i].content_win.height;
    stCfg2NDDMAParam.cfgQ   = cfgQ;
    stCfg2NDDMAParam.mtrREn = 0;
    stCfg2NDDMAParam.plane  = plane;
    *cfgQ_len += vppDhubCfg2NDDMA(vpp_obj, &stCfg2NDDMAParam);

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION:Program Y and UV TGs inside VIP SCL
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static INT  VPP_OfflineScl_VipSclOTGProg(VPP_OBJ *vpp_obj)
{
    UINT32 uiRegAddr = 0;
    UINT32 uiWidth = 0, uiHeight = 0;
    T32TG_SIZE tg_size;
    T32TG_HB tg_hb;
    T32TG_HB_CR tg_hb_cr;
    T32TG_HB_CR2 tg_hb_cr2;
    T32TG_VB0 tg_vb0;
    T32TG_VB0_CR tg_vb0_cr;
    T32TG_VB0_CR2 tg_vb0_cr2;

    /* Programe Luma scalar output TG */
    uiWidth  = vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OHRes;
    uiHeight = vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OVRes;
    tg_size.uSIZE_Y = uiHeight + 6;
    tg_size.uSIZE_X = uiWidth + 40;
    tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = uiWidth + 8;
    tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
    tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = uiHeight + 1;
    tg_vb0.uVB0_BE = 0;
    tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 2;

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCLTOP + RA_VIPSCLTOP_OVPSCL_OTG_Y;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_SIZE, tg_size.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_HB, tg_hb.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_HB_CR, tg_hb_cr.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_HB_CR2, tg_hb_cr2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_VB0, tg_vb0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_VB0_CR, tg_vb0_cr.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_VB0_CR2, tg_vb0_cr2.u32);

    /* Programe UV scalar output TG */
    uiWidth  = vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].OHRes;
    uiHeight = vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].OVRes;
    tg_size.uSIZE_Y = uiHeight + 6;
    tg_size.uSIZE_X = uiWidth + 40;
    tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = uiWidth + 8;
    tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
    tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = uiHeight + 1;
    tg_vb0.uVB0_BE = 0;
    tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 2;

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCLTOP + RA_VIPSCLTOP_OVPSCL_OTG_UV;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_SIZE, tg_size.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_HB, tg_hb.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_HB_CR, tg_hb_cr.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_HB_CR2, tg_hb_cr2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_VB0, tg_vb0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_VB0_CR, tg_vb0_cr.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_TG_VB0_CR2, tg_vb0_cr2.u32);

    return (MV_VPP_OK);
}

/***************************************************************************************
 * FUNCTION:Program Y and UV TGs inside VIP SCL
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static INT  VPP_OfflineScl_VipSclPipeProg(VPP_OBJ *vpp_obj)
{
    UINT32 uiRegAddr = 0;

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCLTOP;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_VIPSCLTOP_CTRL2, vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IHRes * vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IVRes);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_VIPSCLTOP_CTRL3, vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].IHRes * vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].IVRes);

    vpp_obj->stVipSclTopCtrl5.uCTRL5_sclOutrdy_sts0_en = 0;
    vpp_obj->stVipSclTopCtrl5.uCTRL5_sclOutrdy_sts1_en = 0;
    vpp_obj->stVipSclTopCtrl5.uCTRL5_sclOfifo_Y_stsCtrl = 1;
    vpp_obj->stVipSclTopCtrl5.uCTRL5_sclOifo_UV_stsCtrl = 1;
    vpp_obj->stVipSclTopCtrl5.uCTRL5_hde_msk_en = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_VIPSCLTOP_CTRL5, vpp_obj->stVipSclTopCtrl5.u32);

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_OVPSCL_scl1d_Inpix, vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IHRes * vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IVRes);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_OVPSCL_scl1d_Outpix, vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OHRes * vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OVRes);

    uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_OVPSCL_scl1d_Inpix, vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].IHRes * vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].IVRes);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_OVPSCL_scl1d_Outpix, vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].OHRes * vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].OVRes);

    VPP_OfflineScl_VipSclOTGProg(vpp_obj);

    return (MV_VPP_OK);
}

/***************************************************************************************
 * FUNCTION:Update the read & Write buffers based on SCL mode and
 *          change SCL configurations if required
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 *          pVppSclRes - pointer to scalar i/p and o/p windows requead by offline SCL
 *          plane - Pointer to current PLANE
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static INT  VPP_OfflineScl_UpdateBuff(VPP_OBJ *vpp_obj, PVPP_SCL_RES pVppSclRes, PLANE *plane)
{
    INT eVppFrcSclNum = -1;
    INT vipOHRes = 0, vipOVRes = 0;
    OFFLINE_BUFF_DESCR *pOfflineBuffDescr = NULL;
    UINT8 orgLeftCrop = 0;
    UINT8 orgRightCrop = 0;
    CHAN *chan = NULL;

    chan = &vpp_obj->chan[plane->chanID];
    if(VPP_SCL_MODE_INLINE != plane->scl_mode)
    {
        plane->wb_buff_index = (plane->wb_buff_index + 1) & OFFLINE_BUFFER_INDEX_MASK;

        pOfflineBuffDescr = &(plane->offline_buff_descrs[plane->wb_buff_index]);
        pOfflineBuffDescr->uiBitsPerPixel = plane->offline_scl_bpp;
        vipOHRes = (pVppSclRes->OHRes > VPP_OFFLINESCL_CONTENT_MAX_H) ? VPP_OFFLINESCL_CONTENT_MAX_H : pVppSclRes->OHRes;
        vipOVRes = (pVppSclRes->OVRes > VPP_OFFLINESCL_CONTENT_MAX_V) ? VPP_OFFLINESCL_CONTENT_MAX_V : pVppSclRes->OVRes;
        if(VPP_OFFLINESCL_IS_4K_IN_LIMIT(plane->scl_mode, pVppSclRes->OHRes, pVppSclRes->OVRes))
        {
            vipOHRes = VPP_OFFLINESCL_4K_IN_CONTENT_MAX_H;
            vipOVRes = VPP_OFFLINESCL_4K_IN_CONTENT_MAX_V;
        }

        /* VIP SCL not supporting odd width from vs6xx SoCs.
         * So below code make sure even width and 1pixel scaling handled in MAIN SCL */
        vipOHRes &=~(0x1);

#ifdef VPP_OFFLINE_NONSTDRESWR_DISABLE
        if(vipOHRes & (VPP_OFFLINE_DHUB_SIZE_MULTI_PIXELS - 1))
        {
            vipOHRes = (SIGN32)((vipOHRes + (VPP_OFFLINE_DHUB_SIZE_MULTI_PIXELS - 1)) & ~(VPP_OFFLINE_DHUB_SIZE_MULTI_PIXELS - 1));
        }
        pOfflineBuffDescr->stride = (vipOHRes * pOfflineBuffDescr->uiBitsPerPixel) / VPP_OFFLINESCL_BITS_PER_BYTES;
#else
        pOfflineBuffDescr->stride = (vipOHRes * pOfflineBuffDescr->uiBitsPerPixel) / VPP_OFFLINESCL_BITS_PER_BYTES;
        if(pOfflineBuffDescr->stride & (VPP_OFFLINE_DHUB_SIZE_MULTI - 1))
        {
            pOfflineBuffDescr->stride = (SIGN32)((pOfflineBuffDescr->stride + (VPP_OFFLINE_DHUB_SIZE_MULTI - 1)) &  ~(VPP_OFFLINE_DHUB_SIZE_MULTI - 1));
        }
#endif
        pOfflineBuffDescr->content_win.width = vipOHRes;
        pOfflineBuffDescr->content_win.height = vipOVRes;
        pOfflineBuffDescr->disp_win.width = pVppSclRes->OHRes;
        pOfflineBuffDescr->disp_win.height = pVppSclRes->OVRes;
        pOfflineBuffDescr->disp_win.x = pVppSclRes->x;
        pOfflineBuffDescr->disp_win.y = pVppSclRes->y;
        /* Back up cpcb crop information along with cpcb disp window deatils for continues interrupt offline-offline switching case */
        GaloisMemcpy(&pOfflineBuffDescr->crop_win_final, &plane->crop_win_final, sizeof(VPP_WIN));
        GaloisMemcpy(&pOfflineBuffDescr->crop_disp_win_final, &chan->crop_disp_win_final, sizeof(VPP_WIN));

#ifdef VPP_OFFLINESCL_USING_VIP
        /* Progame VIP SCL for downscaling */
        eVppFrcSclNum = VPP_FRC_SCL_VIP_Y;
        if(vpp_obj->scl_res[eVppFrcSclNum].IHRes != pVppSclRes->IHRes ||
                vpp_obj->scl_res[eVppFrcSclNum].OHRes != vipOHRes ||
                vpp_obj->scl_res[eVppFrcSclNum].IVRes != pVppSclRes->IVRes ||
                vpp_obj->scl_res[eVppFrcSclNum].OVRes != vipOVRes)
        {
            vpp_obj->scl_res[eVppFrcSclNum].IHRes = pVppSclRes->IHRes;
            vpp_obj->scl_res[eVppFrcSclNum].OHRes = vipOHRes;
            vpp_obj->scl_res[eVppFrcSclNum].IVRes = pVppSclRes->IVRes;
            vpp_obj->scl_res[eVppFrcSclNum].OVRes = vipOVRes;

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
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclLeft    = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclRight   = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclTop     = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclBottom  = 0;

            configVppWriteBackMode(vpp_obj, VPP_WRITEBACK_MODE_OFFLINE_SCL,pOfflineBuffDescr->content_win.width, pOfflineBuffDescr->content_win.height);
            VPP_VIP_TG_SetTGParams(vpp_obj, vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IHRes, vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IVRes);

            FRC_SCL_SetSclCtrlParams(vpp_obj, eVppFrcSclNum, &vpp_obj->scl_res[eVppFrcSclNum], &vpp_obj->scl_ctrl[eVppFrcSclNum]);
            FRC_SCL_SetFrcParams(vpp_obj, eVppFrcSclNum, &vpp_obj->scl_res[eVppFrcSclNum]);

            eVppFrcSclNum = VPP_FRC_SCL_VIP_UV;
            vpp_obj->scl_res[eVppFrcSclNum].IHRes = pVppSclRes->IHRes;
            vpp_obj->scl_res[eVppFrcSclNum].OHRes = vipOHRes/2;
            vpp_obj->scl_res[eVppFrcSclNum].IVRes = pVppSclRes->IVRes;
            vpp_obj->scl_res[eVppFrcSclNum].OVRes = vipOVRes;

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
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclLeft    = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclRight   = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclTop     = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclBottom  = 0;
            FRC_SCL_SetSclCtrlParams(vpp_obj, eVppFrcSclNum, &vpp_obj->scl_res[eVppFrcSclNum], &vpp_obj->scl_ctrl[eVppFrcSclNum]);
            FRC_SCL_SetFrcParams(vpp_obj, eVppFrcSclNum, &vpp_obj->scl_res[eVppFrcSclNum]);

            VPP_OfflineScl_VipSclPipeProg(vpp_obj);

        }
        startVppWriteBackClients(vpp_obj, VPP_WRITEBACK_MODE_OFFLINE_SCL, pOfflineBuffDescr->content_win.width, pOfflineBuffDescr->content_win.height);
#else
        VPP_VIP_TG_SetTGParams(vpp_obj, pOfflineBuffDescr->content_win.width, pOfflineBuffDescr->content_win.height); FIXME will not work
            VPP_VIP_TG_SetTGParams(); Lindo : Needs to fix this programming
#endif
    }

    if((VPP_SCL_MODE_OFFLINE == plane->scl_mode) || (VPP_SCL_MODE_OFFLINE_OFFLINE == plane->scl_mode))
    {
        plane->rb_buff_index = (plane->rb_buff_index + 1) & OFFLINE_BUFFER_INDEX_MASK;
        pOfflineBuffDescr = &(plane->offline_buff_descrs[plane->rb_buff_index]);
#ifdef VPP_OFFLINESCL_USING_VIP
        /* Progame MAIN SCL for downscaling */
        eVppFrcSclNum = VPP_FRC_SCL_MAIN;
        if(vpp_obj->scl_res[eVppFrcSclNum].IHRes != pOfflineBuffDescr->content_win.width ||
                vpp_obj->scl_res[eVppFrcSclNum].OHRes != pOfflineBuffDescr->disp_win.width ||
                vpp_obj->scl_res[eVppFrcSclNum].IVRes != pOfflineBuffDescr->content_win.height ||
                vpp_obj->scl_res[eVppFrcSclNum].OVRes != pOfflineBuffDescr->disp_win.height)
        {
            VPP_offlineScl_offlineReadConfig(vpp_obj, plane);

            vpp_obj->scl_res[eVppFrcSclNum].IHRes = pOfflineBuffDescr->content_win.width;
            vpp_obj->scl_res[eVppFrcSclNum].OHRes = pOfflineBuffDescr->disp_win.width;
            vpp_obj->scl_res[eVppFrcSclNum].IVRes = pOfflineBuffDescr->content_win.height;
            vpp_obj->scl_res[eVppFrcSclNum].OVRes = pOfflineBuffDescr->disp_win.height;
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

            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclLeft    = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclRight   = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclTop     = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].cropAfterSclBottom  = 0;
            /* In offline case VIP SCL will take care of cropping. So no need to do cropping on MAIN SCL */
            orgLeftCrop  = vpp_obj->scl_ctrl[eVppFrcSclNum].leftCrop;
            orgRightCrop = vpp_obj->scl_ctrl[eVppFrcSclNum].rightCrop;
            vpp_obj->scl_ctrl[eVppFrcSclNum].leftCrop  = 0;
            vpp_obj->scl_ctrl[eVppFrcSclNum].rightCrop = 0;

            FRC_SCL_SetSclCtrlParams(vpp_obj, eVppFrcSclNum, &vpp_obj->scl_res[eVppFrcSclNum], &vpp_obj->scl_ctrl[eVppFrcSclNum]);
            FRC_SCL_SetFrcParams(vpp_obj, eVppFrcSclNum, &vpp_obj->scl_res[eVppFrcSclNum]);

            vpp_obj->scl_ctrl[eVppFrcSclNum].leftCrop  = orgLeftCrop;
            vpp_obj->scl_ctrl[eVppFrcSclNum].rightCrop = orgRightCrop;
        }
#endif
        VPP_offlineScl_offlineReadStart(vpp_obj, plane);
    }

    return MV_VPP_OK;
}

static BOOL VPP_IsHdmiRxConfigured(void) {
    BOOL hdmiRxConfigured = 0;
#ifdef BA_HDMIRX_PIPE_CTRL_read_sel
    T32HDMIRX_PIPE_CTRL stHdmiPipeCtrl;
    GA_REG_WORD32_READ(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_CTRL, &stHdmiPipeCtrl.u32);
    hdmiRxConfigured = (0 == stHdmiPipeCtrl.uCTRL_read_sel);
#endif
    return hdmiRxConfigured;
}

/***************************************************************************************
 * FUNCTION:Based on the scaling requirement and previous SclMode,
 *          this function will give the new scaling mode
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 *          pVppSclRes - pointer to scalar i/p and o/p windows requead by offline SCL
 *          plane - Pointer to current PLANE
 *          sclMode - Output variable to store new scaling mode
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static INT  VPP_OfflineScl_GetMode(VPP_OBJ *vpp_obj, PVPP_SCL_RES pVppSclRes, PLANE *plane, INT *sclMode)
{
    UNSG32 vpp_sys_clk = 600000; //Fixme. Needs to read from registers instead of hard coding
    INT mult_fact_vratio = 100;
    INT req_vscale_ratio = 1;
    INT max_vscale_ratio = 1;
    INT cpcbID = CPCB_OF_CHAN(vpp_obj, plane->chanID);

    GET_RESOLUTION_INFO();
#ifdef CONFIG_VPP_DELAY_OFFLINE_SCALER
    if(plane->force_scl_mode_state) {
        if(plane->force_scl_mode_count) {
            *sclMode = VPP_SCL_MODE_INLINE;
            return MV_VPP_OK;
        } else {
            //Remove the force scl mode setting
            plane->force_scl_mode_state = 0;
            VPPAPI_DBG_INFO("SCL Force line-dropping : Plane:%d, frame-delay:%d\n", plane->chanID, plane->force_scl_mode_count);
        }
    }
#endif //CONFIG_VPP_DELAY_OFFLINE_SCALER

    if(1 == plane->sclLineDropEn)
    {
        /* For line dropping cases force VPP_SCL_MODE_INLINE_OFFLINE without doing any more calculation */
#ifdef CONFIG_VPP_DISABLE_OFFLINE_SCALER
        *sclMode = VPP_SCL_MODE_INLINE;
#else //CONFIG_VPP_DISABLE_OFFLINE_SCALER
        if(VPP_OFFLINESCL_IS_FORCED_LINEDROP() || VPP_IsHdmiRxConfigured())
        {
            *sclMode = VPP_SCL_MODE_INLINE;
        }
        else
        {
            *sclMode = VPP_SCL_MODE_INLINE_OFFLINE;
        }
#endif //!CONFIG_VPP_DISABLE_OFFLINE_SCALER
        return MV_VPP_OK;
    }

    VPP_GetVppSysClk_KHz(vpp_obj, &vpp_sys_clk,plane->chanID);
    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[CPCB_1]);

    max_vscale_ratio = mult_fact_vratio;
    max_vscale_ratio *= vpp_sys_clk;
    max_vscale_ratio /= pResinfo->freq;
    max_vscale_ratio *= pResinfo->active_width; //active width or htotal???
    max_vscale_ratio /= pVppSclRes->IHRes;

    req_vscale_ratio = mult_fact_vratio;
    req_vscale_ratio *= pVppSclRes->IVRes;
    req_vscale_ratio /= pVppSclRes->OVRes;

    if(req_vscale_ratio > max_vscale_ratio)
    {
        if(VPP_SCL_MODE_INLINE == plane->scl_mode)
            *sclMode = VPP_SCL_MODE_INLINE_OFFLINE;
        else if((VPP_SCL_MODE_OFFLINE == plane->scl_mode) ||
                (VPP_SCL_MODE_OFFLINE_OFFLINE == plane->scl_mode) /* Continues interrupt offline-offline swithing case */ )
            *sclMode = VPP_SCL_MODE_OFFLINE_OFFLINE;
        else
            *sclMode = VPP_SCL_MODE_OFFLINE;
    }
    else
    {
        *sclMode = VPP_SCL_MODE_INLINE;
    }

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION:Set registers and initial values based on SclMode
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 *          plane - Pointer to current PLANE
 *          sclMode - The new scale mode needs to set
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
static INT VPP_OfflineScl_SetMode(VPP_OBJ *vpp_obj, PLANE *plane, INT sclMode)
{
    UINT32 RegAddr = 0;

    switch(plane->scl_mode)
    {
        case VPP_SCL_MODE_INLINE:
            { /* VPP_SCL_MODE_INLINE return fucntion without any modification */

                vpp_obj->stVidplSclClkenCtrl.uSCL_CLKEN_CTRL_fall_bk_en = 0;

                vpp_obj->mainpl_ctrl4.uCTRL4_inline_offline_sel = 0;
                vpp_obj->mainpl_ctrl4.uCTRL4_scl1d_ofl_wr_en    = 0;
                vpp_obj->mainpl_ctrl4.uCTRL4_scl1d_inl_ofl_en   = 0;

                vpp_obj->stMainPlCtrl7.uCTRL7_offline_rdsel_1dscl   = 0;
                vpp_obj->stMainPlCtrl7.uCTRL7_main_video_wrbk       = 0;
                vpp_obj->stMainPlCtrl7.uCTRL7_write_sel_1dscl_dram  = 0;
                plane->rb_buff_index = OFFLINE_BUFFER_INVALID_INDEX;
                plane->wb_buff_index = OFFLINE_BUFFER_INVALID_INDEX;

                vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IHRes = 0;
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OHRes = 0;
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].IVRes = 0;
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_Y].OVRes = 0;
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].IHRes = 0;
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].OHRes = 0;
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].IVRes = 0;
                vpp_obj->scl_res[VPP_FRC_SCL_VIP_UV].OVRes = 0;
                break;
            }
        case VPP_SCL_MODE_INLINE_OFFLINE:
            {
                /* Enable inline mode with offline write */
                vpp_obj->stVidplSclClkenCtrl.uSCL_CLKEN_CTRL_fall_bk_en = 1;

                vpp_obj->mainpl_ctrl4.uCTRL4_inline_offline_sel = 0;
                vpp_obj->mainpl_ctrl4.uCTRL4_scl1d_ofl_wr_en    = 0;
                vpp_obj->mainpl_ctrl4.uCTRL4_scl1d_inl_ofl_en   = 1;

                vpp_obj->stMainPlCtrl7.uCTRL7_offline_rdsel_1dscl   = 0;
                vpp_obj->stMainPlCtrl7.uCTRL7_main_video_wrbk       = 0;
                vpp_obj->stMainPlCtrl7.uCTRL7_write_sel_1dscl_dram  = 0;
                break;
            }
        case VPP_SCL_MODE_OFFLINE:
        case VPP_SCL_MODE_OFFLINE_OFFLINE:
            {/* Enable offline read and write */
                vpp_obj->stVidplSclClkenCtrl.uSCL_CLKEN_CTRL_fall_bk_en = 0;

                vpp_obj->mainpl_ctrl4.uCTRL4_inline_offline_sel = 0;
                vpp_obj->mainpl_ctrl4.uCTRL4_scl1d_ofl_wr_en    = 0;
                vpp_obj->mainpl_ctrl4.uCTRL4_scl1d_inl_ofl_en   = 0;

                vpp_obj->stMainPlCtrl7.uCTRL7_offline_rdsel_1dscl   = 1;
                vpp_obj->stMainPlCtrl7.uCTRL7_main_video_wrbk       = 1;
                vpp_obj->stMainPlCtrl7.uCTRL7_write_sel_1dscl_dram  = 0;
                break;
            }
        default:
            break;
            /* As this all are internal function, default case is not implementing */
    }
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->mainpl_ctrl4.u32);
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL7;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->stMainPlCtrl7.u32);
    if(vpp_obj->stMainPlCtrl7.uCTRL7_main_video_wrbk)
    {
        /*Expecting vpp_obj->mainpl_ctrl4.uCTRL4_tHDR_bypass needs to set while tHDR disabled */
        /*while vITM enables; m_uiThdrEnable also get set in software. In that case we needs to consider tHDR as bypass */
        vpp_obj->mainplCtrl0.uCTRL0_vptop_fifo_rdsel = 1;/* enable if(tHDR_bypass && main_video_wrbk) */
    }
    else
    {
        vpp_obj->mainplCtrl0.uCTRL0_vptop_fifo_rdsel = 0;
    }
    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->mainplCtrl0.u32);

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION:This funcion is used determine whether line drop is required for smooth
 *          inline to offline swithing for refernce window(SCL input) change
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 *          pVppSclRes - pointer to scalar i/p and o/p windows requead by offline SCL
 *          chanID - value of current channel ID
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
INT VPP_OfflineScl_ConfigLineDrop(VPP_OBJ *vpp_obj, PVPP_SCL_RES pVppSclRes, INT chanID)
{
    PLANE *plane = NULL;
    INT planeID, cpcbID;
    INT iSclMode = VPP_SCL_MODE_INVALID;

    planeID = PLANE_OF_CHAN(vpp_obj, chanID);
    plane = &vpp_obj->plane[planeID];

    cpcbID = CPCB_OF_CHAN(vpp_obj, chanID);

    GET_RESOLUTION_INFO();
#ifdef CONFIG_VPP_DELAY_OFFLINE_SCALER
    if(memcmp(&(plane->force_scl_res), pVppSclRes, sizeof(VPP_SCL_RES))) {
        memcpy(&(plane->force_scl_res), pVppSclRes, sizeof(VPP_SCL_RES));
        if(plane->uiDolbyPresentMode != VPP_DOLBY_PRESENT_BL_EL_METADATA) {
             plane->force_scl_mode_state = 1;
             plane->force_scl_mode_count = VPP_DELAY_OFFLINE_SCALER_IN_FRAMES;
             VPPAPI_DBG_INFO("SCL Force line-dropping : Plane:%d, frame-delay:%d\n", plane->chanID, plane->force_scl_mode_count);
        }
    }
    if(plane->force_scl_mode_count) {
        //Force linedrop for N frames
        plane->sclLineDropEn = 1;
        return MV_VPP_OK;
    }
#endif //CONFIG_VPP_DELAY_OFFLINE_SCALER

    plane->sclLineDropEn = 0; /* Reset to disable mode */

#ifdef CONFIG_VPP_DISABLE_OFFLINE_SCALER
    //Use formula/inline for all plane
    if(1)
#else //CONFIG_VPP_DISABLE_OFFLINE_SCALER
    //Don't use formula/inline for Main plane
    if(PLANE_MAIN != planeID
            || VPP_OFFLINESCL_IS_FORCED_LINEDROP()
            || VPP_IsHdmiRxConfigured())
#endif //!CONFIG_VPP_DISABLE_OFFLINE_SCALER
    {
        /* For actual Downscaling cases of PIP/GFX1 plane
         * - Decide the line dropping requirement from formula */
        plane->sclLineDropEn = 1;
        return MV_VPP_EBADPARAM; /* Offline support is only for MAIN */
    }

    if(VPP_SCL_MODE_INLINE != plane->scl_mode)
        return MV_VPP_OK; /* No needs to line drop as it is already in some offline mode */

    //Don't consider the refwin, whenever canvas is changed, i.e force inline line dropping for canvas change.
    if(pVppSclRes->IVRes == vpp_obj->scl_res[VPP_FRC_SCL_MAIN].IVRes && (0 == vpp_obj->resIdUpdated[cpcbID]))
        return MV_VPP_OK; /* No needs as there is no reference window height changed */

    VPP_OfflineScl_GetMode(vpp_obj, pVppSclRes, plane, &iSclMode);
    if(VPP_SCL_MODE_INLINE_OFFLINE == iSclMode)
    {
        plane->sclLineDropEn = 1;
        plane->ref_win_updated = 1; /* Force again, so that next ISR on wards it will do without any line dropping as offline 1st buffer is ready */
        plane->dolbyLineDropEn = 1;
    }

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION:This funcion is used to configure offline scaling based on sclaing requirement
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 *          pVppSclRes - pointer to scalar i/p and o/p windows requead by offline SCL
 *          chanID - value of current channel ID
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
int VPP_OfflineScl_Config(VPP_OBJ *vpp_obj, PVPP_SCL_RES pVppSclRes, INT chanID)
{
    PLANE *plane = NULL;
    INT planeID;
    INT iSclMode = VPP_SCL_MODE_INVALID;
    //INT isSclModeChanged = 0;
    VPP_SCL_RES stVppSclRes;

    planeID = PLANE_OF_CHAN(vpp_obj, chanID);

    if(PLANE_MAIN != planeID)
        return MV_VPP_EBADPARAM; /* Offline support is only for MAIN */

    plane = &vpp_obj->plane[planeID];

    /* Get the scl_mode */
    VPP_OfflineScl_GetMode(vpp_obj, pVppSclRes, plane, &iSclMode);

    if((iSclMode != plane->scl_mode) ||
       (VPP_SCL_MODE_OFFLINE_OFFLINE == iSclMode)/* Continues interrupt offline-offline swithing case, needs to consider as scl mode changed */)
    {
        plane->scl_mode = iSclMode;
        plane->sclModeChanged = 1;
#ifdef EN_VPP_DYNAMIC_CLKGATE

    /* TODO: BG7 Clock gating */
#endif
    }

    if(1 == plane->sclModeChanged)
    {
        /* Set registers and initial values based on new SclMode */
        VPP_OfflineScl_SetMode(vpp_obj, plane, plane->scl_mode);

        if(VPP_SCL_MODE_INLINE != plane->scl_mode)
        {
            vpp_obj->sclResCfg.IVRes = pVppSclRes->IVRes;
            vpp_obj->sclResCfg.IHRes = pVppSclRes->IHRes;
            vpp_obj->sclResCfg.OVRes = pVppSclRes->OVRes;
            vpp_obj->sclResCfg.OHRes = pVppSclRes->OHRes;
            vpp_obj->sclResCfg.x     = pVppSclRes->x;
            vpp_obj->sclResCfg.y     = pVppSclRes->y;
            if(VPP_SCL_MODE_INLINE_OFFLINE == plane->scl_mode)
            {
                /* While inline-offline switch, the output of MAIN SCL is feed to VIP SCL */
                stVppSclRes.IHRes = vpp_obj->scl_res[VPP_FRC_SCL_MAIN].OHRes;
                stVppSclRes.IVRes = vpp_obj->scl_res[VPP_FRC_SCL_MAIN].OVRes;
                stVppSclRes.OHRes = pVppSclRes->OHRes;
                stVppSclRes.OVRes = pVppSclRes->OVRes;
                stVppSclRes.x     = pVppSclRes->x;
                stVppSclRes.y     = pVppSclRes->y;
                VPP_OfflineScl_UpdateBuff(vpp_obj, &stVppSclRes, plane);
            }
            else
            {
            VPP_OfflineScl_UpdateBuff(vpp_obj, &vpp_obj->sclResCfg, plane);
            }
        }
    }

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION:This funcion is used to start client and dHub for Offline Scaling
 * PARAMS:  vpp_obj - pointer to current VPP_OBJ
 *          pVppSclRes - pointer to scalar i/p and o/p windows requead by offline SCL
 *          chanID - value of current channel ID
 * RETURN:  MV_VPP_OK - succeed.
 *          MV_VPP_EBADPARAM - bad input parameters.
 ***************************************************************************************/
int VPP_OfflineScl_ClientStart(VPP_OBJ *vpp_obj, PVPP_SCL_RES pVppSclRes, INT chanID)
{
    PLANE *plane = NULL;
    INT planeID;

    planeID = PLANE_OF_CHAN(vpp_obj, chanID);

    plane = &vpp_obj->plane[planeID];

    if((PLANE_MAIN != planeID) || (VPP_SCL_MODE_INLINE == plane->scl_mode))
        return MV_VPP_OK; /* Offline support is only for MAIN. Inline case no need to do any exta dHub */

    if(VPP_SCL_MODE_OFFLINE_OFFLINE == plane->scl_mode)
    {
        plane->scl_mode = VPP_SCL_MODE_OFFLINE;
//        return MV_VPP_OK;
    }

    if(VPP_SCL_MODE_OFFLINE != plane->scl_mode)
    {
        VPP_OfflineScl_Config(vpp_obj, pVppSclRes, chanID);
    }
    else
    {
        VPP_OfflineScl_UpdateBuff(vpp_obj, pVppSclRes, plane);
    }
    return MV_VPP_OK;
}


/* Return the vppSysClk used in the current resolution */
int VPP_GetVppSysClk_KHz(VPP_OBJ *vpp_obj, UNSG32 *pVppSysClk,INT chanID)
{
    /*Note: Please sync this to the configurtion done in avpll_clock_cfg.c
     * TBD: AVPLL callback to retrive/read the lastest VPPSYSCLK configured by AVPLL HAL layer
     */
    CHAN *chan = &vpp_obj->chan[chanID];
    INT planeID = PLANE_OF_CHAN(vpp_obj,chanID);
    int cpcbID = CPCB_OF_CHAN(vpp_obj, CHAN_MAIN);
    PLANE *plane = &vpp_obj->plane[planeID];
    RESOLUTION_INFO  *pResinfo;

    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[cpcbID]);
    if(pResinfo->freq > 297000)
    {
        /*Reduced vppSysClk around 5% to avoid down scaling
        issue for HDR/Dolby cases in offline switching boundary for main plane*/
        if(planeID == PLANE_MAIN && plane->ref_win.height > chan->disp_win_final.height)
        {
            if(SRCFMT_YUV420SP == plane->srcfmt)
                *pVppSysClk = 510000;
            else
                *pVppSysClk = 565000;

        }
        else
        {
            *pVppSysClk = 600000;
        }
    }
    else
    {
        int resId = VPP_GetResId(cpcbID);
        if(planeID == PLANE_MAIN)
        {
            *pVppSysClk = 594000;
        }
        else
        {
            *pVppSysClk = 300000;
        }
        //Avoid line dropping(go for scaler) for 1080i case, TBD:why it should not be done for all cases
        if((resId==RES_1080I60) || (resId==RES_1080I50) || (resId==RES_1080I5994)) {
            *pVppSysClk = 600000;
        }
    }

    return MV_VPP_OK;
}

