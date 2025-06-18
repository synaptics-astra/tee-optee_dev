/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

/*
 * vpp_cmu_tg.c
 *
 *  Created on: Jan 23, 2019
 *      Author: jjose
 */

#define _VPP_CMU_TG_
#include "vpp.h"
#include "vpp_mp.h"
#include "vpp_cmu_tg.h"
#include "vpp_cmu_tgpriv.h"
#include "vpp_module.h"

INT VPP_CMU_SetDefault(VPP_OBJ *vpp_obj)
{
    GA_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL10, &vpp_obj->stMainPlCtrl10.u32);

    CMU_Enable(vpp_obj,FALSE);

    return MV_VPP_OK;
}

INT CMU_Enable(VPP_OBJ *vpp_obj,INT enable)
{
    UINT32 RegAddr  = vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CTRL0;

    if (enable == 0)
        vpp_obj->stMainPlCtrl10.uCTRL10_detEE_top_bypass = 1;

    vpp_obj->cmu.cmu_ctl0.uCTRL0_EETop_bypass =  !(enable);
    vpp_obj->cmu.cmu_ctl0.uCTRL0_cmu_bypass =  !(enable);
    vpp_obj->stMainPlCtrl10.uCTRL10_eecmu_top_bypass = !(enable);
    vpp_obj->stMainPlCtrl10.uCTRL10_ee_cmu_sel = enable;
    vpp_obj->stMainPlCtrl10.uCTRL10_eeTop_sel = enable;
    vpp_obj->cmu.cmu_ctl0.uCTRL0_clken_ctrl = 0;
    vpp_obj->cmu.cmu_ctl0.uCTRL0_ififo_sts_ctrl = 0;
    vpp_obj->cmu.cmu_ctl0.uCTRL0_ofifo_sts_ctrl = 0;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->cmu.cmu_ctl0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL10, vpp_obj->stMainPlCtrl10.u32);

    vpp_obj->cmu.cmu_enabled = enable;

    return MV_VPP_OK;
}

INT CMU_ConfigTG(VPP_OBJ *vpp_obj, int width, int height)
{
    VPP_CMU_INPUT_RES totres;
    VPP_CMU_INPUT_CROP inpcrop;

    totres.HRes = width;
    totres.VRes = height;

    inpcrop.leftCrop = 0;
    inpcrop.rightCrop = 0;
    inpcrop.topCrop = 0;
    inpcrop.bottomCrop = 0;
    CMU_SetDummyTG(vpp_obj, 0, &totres, &inpcrop);

    return 0;
}

INT CMU_SetDummyTG(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_CMU_INPUT_RES pTotRes, PVPP_CMU_INPUT_CROP pCropRes)
{
    UINT32 RegAddr;
    UINT32 RegVal;
    INT RetVal = MV_VPP_OK;

    RegAddr = vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_EECMU_TG;

    RegVal = (pTotRes->VRes + VPP_CMU_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_CMU_DUMMY_TG_SIZE_H_OFF_P) << 16);

    /*write TG size to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pTotRes->HRes + VPP_CMU_DUMMY_TG_HB_CMU_OFF + ((VPP_CMU_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB), RegVal)))
        return RetVal;
    RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_CMU_DUMMY_TG_HB_CMU_OFF + (((pCropRes->leftCrop)+VPP_CMU_DUMMY_TG_HB_BE) << 16);
    /*write the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pTotRes->VRes + VPP_CMU_DUMMY_TG_VB0_CMU_OFF + ((VPP_CMU_DUMMY_TG_VB0_BE) << 16);

    /*writer the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0), RegVal)))
        return RetVal;
    RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_CMU_DUMMY_TG_VB0_CMU_OFF + ((pCropRes->topCrop+VPP_CMU_DUMMY_TG_VB0_BE) << 16);
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR), RegVal)))
        return RetVal;

    vpp_obj->stMainPlCtrl13.u32 = pTotRes->HRes*pTotRes->VRes;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL13, vpp_obj->stMainPlCtrl13.u32);

    return RetVal;
}

INT CMU_TG_Start(VPP_OBJ *vpp_obj)
{
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_TG_CTRL;

    vpp_obj->cmu.cmu_tg_ctl.uTG_CTRL_start = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->cmu.cmu_tg_ctl.u32);

    vpp_obj->cmu.cmu_tg_ctl.uTG_CTRL_start = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->cmu.cmu_tg_ctl.u32);

    return MV_VPP_OK;
}

INT CMU_TG_Clear(VPP_OBJ *vpp_obj)
{
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_TG_CTRL;

    vpp_obj->cmu.cmu_tg_ctl.uTG_CTRL_clear  = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr,vpp_obj->cmu.cmu_tg_ctl.u32);

    vpp_obj->cmu.cmu_tg_ctl.uTG_CTRL_clear  = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->cmu.cmu_tg_ctl.u32);

    return MV_VPP_OK;
}
