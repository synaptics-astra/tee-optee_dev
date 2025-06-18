/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_FE_OFFLINERD_C_

#include "com_type.h"
#include "vpp_module.h"
#include "vpp_fe_dlr.h"
#include "vpp_offlineScl.h"

#define VPP_OFFLINE_RD_WORD_SIZE (16) /* Bytes in 128bit words */

/***************************************************************************************
 * FUNCTION: load default values of data loader to BCM buffer
 * PARAMS:  Channel - indicate the channel
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBADPARAM - bad input parameters.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_OfflineRd_LoadDefaultVal(VPP_OBJ *vpp_obj, INT Channel)
{
    UINT32 RegAddr;
    INT RetVal = MV_VPP_OK;

    if((Channel < VPP_FE_DLR_CHANNEL_MIN) || (Channel > VPP_FE_DLR_CHANNEL_MAX))
        return MV_VPP_EBADPARAM;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN + RA_OfflineRd_CTRL3;
    GA_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.u32);

    return RetVal;
}

INT FE_OfflineRd_422_444_UPS_Coeff(VPP_OBJ *vpp_obj, INT ldr_id, const INT32 *piCoeff, const INT32 *piCoeff_7)
{
    T32UPS12_CFG1 stUpsCfg1;
    T32UPS12_CFG2 stUpsCfg2;
    T32UPS12_CFG3 stUpsCfg3;
    T32UPS12_CFG4 stUpsCfg4;
    T32UPS12_CFG7 stUpsCfg7;
    T32UPS12_CFG8 stUpsCfg8;
    T32UPS12_CFG9 stUpsCfg9;

    UINT32 i = 0;
    UINT32 uiRegAddr = 0;

    if(ldr_id == VPP_FE_DLR_CHANNEL_OFFLINE_RD_0)
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN + RA_OfflineRd_UPS422to444_12B;
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

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS12_CFG1, stUpsCfg1.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS12_CFG2, stUpsCfg2.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS12_CFG3, stUpsCfg3.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS12_CFG4, stUpsCfg4.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS12_CFG7, stUpsCfg7.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS12_CFG8, stUpsCfg8.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_UPS12_CFG9, stUpsCfg9.u32);

    return  MV_VPP_OK;
}

const int auiOfflineRd422UpsCoeff[] = {0x50, 0x0, 0x1130, 0x0, 0x4E0, 0x800, 0x4E0};
const int auiOfflineRd422Ups7Coeff[] = {0x1024, 0x9, 0x424, 0x7EE, 0x424};
INT FE_OfflineRd_422_444_UPS_Config(VPP_OBJ *vpp_obj, INT ldr_id, INT src_fmt)
{
    INT RetVal = MV_VPP_OK;

    FE_OfflineRd_422_444_UPS_Coeff(vpp_obj, ldr_id, auiOfflineRd422UpsCoeff, auiOfflineRd422Ups7Coeff);


    return RetVal;
}


/***************************************************************************************
 * FUNCTION: set dummy TG for  video channel.
 * PARAMS:
 *
 * RETURN:  MV_VPP_OK - succeed.
 *                MV_VPP_EBCMBUFFULL -BCM buffer is full.
 ***************************************************************************************/
INT FE_OfflineRd_Config(VPP_OBJ *vpp_obj, INT ldr_id)
{
    INT RetVal = MV_VPP_OK;
    UINT32 RegAddr = 0;
    OFFLINE_BUFF_DESCR *pOfflineBuffDescr = NULL;

    T32OfflineRd_CTRL1 stOfflineRdCtrl1;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OFFLINERD_MAIN;

    stOfflineRdCtrl1.u32 = 0;
    pOfflineBuffDescr = &(vpp_obj->plane[PLANE_MAIN].offline_buff_descrs[vpp_obj->plane[PLANE_MAIN].rb_buff_index]);
    stOfflineRdCtrl1.uCTRL1_tot = (pOfflineBuffDescr->stride * pOfflineBuffDescr->content_win.height)/VPP_OFFLINE_RD_WORD_SIZE;
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_OfflineRd_CTRL1), stOfflineRdCtrl1.u32)))
        return RetVal;
#ifndef VPP_OFFLINE_NONSTDRESWR_DISABLE
    if((pOfflineBuffDescr->content_win.width * pOfflineBuffDescr->uiBitsPerPixel / VPP_OFFLINESCL_BITS_PER_BYTES) &  (VPP_OFFLINE_DHUB_SIZE_MULTI - 1))
    {
        vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_nonStdResEn = 1;
    }
    else
    {
        vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_nonStdResEn = 0;
    }
#endif
    vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_pixlineTot = pOfflineBuffDescr->content_win.width;
    switch(pOfflineBuffDescr->uiBitsPerPixel)
    {
        case 16:
            vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_packSel      = 0;
            vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_idata_ctrl   = 0;
            break;
        case 20:
            vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_packSel      = 1;
            vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_idata_ctrl   = 1;
            break;
        case 24:
            vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_packSel      = 2;
            vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_idata_ctrl   = 2;
            break;
        default:
            vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_packSel      = 0;
            vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.uCTRL3_idata_ctrl   = 0;
            break;
    }
    if(MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_OfflineRd_CTRL3), vpp_obj->plane[PLANE_MAIN].stOfflineRdCtrl3.u32)))
        return RetVal;
    FE_OfflineRd_422_444_UPS_Config(vpp_obj, VPP_FE_DLR_CHANNEL_OFFLINE_RD_0, SRCFMT_YUV422);

    return RetVal;
}

