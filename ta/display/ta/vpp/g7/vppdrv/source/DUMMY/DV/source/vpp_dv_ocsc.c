/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_DV_OCSC_C_
#define _VPP_DV_OCSC_C_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_bcmbuf.h"
#include "vpp.h"
#include "vpp_dv_ocsc.h"
#include "vpp_dv_api.h"
#include "vpp_gfx_vop_dv_reg.h"
#include "math.h"
#include "vpp_csc_common.h"
#include "vpp_csc_ocsc.h"
#include "vpp_pq.h"

extern const INT16 VppDVOcsc_Coeff_Matrix[VPP_MAX_OCSC_SUB_MODES_MAX] \
                                         [VPP_MAX_OCSC_COEFF_MAT_ROW][VPP_MAX_OCSC_COEFF_MAT_COL];

extern const INT32 VppDVOcsc_RGB2YUV_Offset[VPP_MAX_OCSC_SUB_MODES_MAX][VPP_MAX_OCSC_OFFSET];

// DENORM Values [Overone]
static const INT32 VppDVOcsc_Denorm[VPP_MAX_OCSC_DENORM_MODES] = {0x10000};

static const UNSG8 OcscMode2SubMode[VPP_DV_OCSC_MODE_MAX] = {0,
                                                        VPP_DV_OCSC_COEFF_RGB_BYPASS,
                                                        VPP_DV_OCSC_COEFF_RGB2YUV_LIMITED,
                                                        VPP_DV_OCSC_COEFF_RGB2YUV_LIMITED,
                                                        VPP_DV_OCSC_COEFF_RGB2YUV_r2020_LIMITED,
                                                        VPP_DV_OCSC_COEFF_RGB2YUV_r2020_LIMITED};

static const UNSG8 ColorFmt2CscMode[MAX_NUM_OUTPUT_COLOR_FMTS] = {
     VPP_DV_OCSC_MODE_BYPASS,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV444,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422_R2020,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV444_R2020,
     VPP_DV_OCSC_MODE_RGB444_TO_YUV422
};

static const UNSG8 OcscMode2OutFmt[VPP_DV_OCSC_MODE_MAX] = {
    VPP_DV_OCSC_OFORMAT_YUV422,
    VPP_DV_OCSC_OFORMAT_RGB444,
    VPP_DV_OCSC_OFORMAT_YUV444,
    VPP_DV_OCSC_OFORMAT_YUV422,
    VPP_DV_OCSC_OFORMAT_YUV444,
    VPP_DV_OCSC_OFORMAT_YUV422
};


static void VPP_BE_OCSC_Coeff_RGB2YUV(VPP_OBJ *vpp_obj, UINT32 SubMode)
{
    UINT32 RegAddr, i, j;
    T32OCSC_R2YOFFSET_0 stRGB2YUV_OFF0; //RGB2YUV Offsets
    T32OCSC_R2YOFFSET_1 stRGB2YUV_OFF1;
    INT16 m33Csc[VPP_MAX_OCSC_COEFF_MAT_ROW][VPP_MAX_OCSC_COEFF_MAT_COL];
    double dm33Csc[VPP_MAX_OCSC_COEFF_MAT_ROW][VPP_MAX_OCSC_COEFF_MAT_COL];
    BOOL bRGBByPassMode = FALSE;
    INT32 offset[VPP_MAX_OCSC_OFFSET];
    T32OCSC_OCSC_SCALE2P ocsc_scale2p;

    VPPAPI_DBG_INFO("CTemp: Rgain[%.7g]Ggain[%.7g]Bgain[%.7g]submode[%d]\n", \
        vpp_obj->clrtemp.Rgain, vpp_obj->clrtemp.Ggain, vpp_obj->clrtemp.Bgain, SubMode);

    //Register Write Operations
    RegAddr = VPP_BASE_ADDR_OCSC + RA_OCSC_OCSC_RGB2YUV_COEF00;
    if (SubMode == VPP_DV_OCSC_COEFF_RGB_BYPASS){
        /*
         * RGB BYPASS mode using RGB2YUV 709_LIMITED coefficient for RGB-YUV conversion
         * then will use YUV-RGB coefficient for getting unity matrix
         */
        SubMode = VPP_DV_OCSC_COEFF_RGB2YUV_LIMITED;
        bRGBByPassMode = TRUE;
    }
    VPP_Memcpy(m33Csc[0], VppDVOcsc_Coeff_Matrix[SubMode][0], sizeof(int)*VPP_MAX_OCSC_COEFF_MAT_COL*VPP_MAX_OCSC_COEFF_MAT_COL);
    for (i = 0; i < VPP_MAX_OCSC_COEFF_MAT_ROW; i++) {
        for (j = 0; j < VPP_MAX_OCSC_COEFF_MAT_COL; j++) {
            m33Csc[i][j] = SIGN16_TO_SHORT_INT_CONVERT(m33Csc[i][j]);
            dm33Csc[i][j] = (double)m33Csc[i][j];
            dm33Csc[i][j] = (double)(dm33Csc[i][j] /(1 << 15));
        }
    }

    //RGB2YUV Offsets value assignment
    offset[0] = VppDVOcsc_RGB2YUV_Offset[SubMode][0];
    offset[1] = VppDVOcsc_RGB2YUV_Offset[SubMode][1];
    offset[2] = VppDVOcsc_RGB2YUV_Offset[SubMode][2];

    VPP_OCSC_BCHS(vpp_obj, dm33Csc, offset);

    if (bRGBByPassMode){
        VPP_DV_OCSC_YUV_2_RGB_ConverSion(dm33Csc, offset);
    }

    for (i = 0; i < VPP_MAX_OCSC_COEFF_MAT_ROW; i++) {
        for (j = 0; j < VPP_MAX_OCSC_COEFF_MAT_COL; j++) {
            m33Csc[i][j] = (INT16)(dm33Csc[i][j] * (1 << VPP_DV_OCSC_SCALE_2P));
        }
    }

    VPP_DM_OCSC_ApplyGainAndSetCSC_Coeff(vpp_obj, RegAddr, m33Csc);

    //RGB2YUV Offsets value assignment
    stRGB2YUV_OFF0.uOCSC_R2YOFF0 = offset[0];
    stRGB2YUV_OFF0.uOCSC_R2YOFF1 = offset[1];
    stRGB2YUV_OFF1.uOCSC_R2YOFF2 = offset[2];

    ocsc_scale2p.uOCSC_SCALE2P_RGB2YUV_SCALE2P = VPP_DV_OCSC_SCALE_2P;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, VPP_BASE_ADDR_OCSC + RA_OCSC_OCSC_SCALE2P, ocsc_scale2p.u32);

    //Register Write Operations
    RegAddr = VPP_BASE_ADDR_OCSC;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr +  RA_OCSC_OCSC_RGB2YUV_OFFSET_0, stRGB2YUV_OFF0.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr +  RA_OCSC_OCSC_RGB2YUV_OFFSET_1 , stRGB2YUV_OFF1.u32);
}

static void VPP_BE_OCSC_Denorm(VPP_OBJ *vpp_obj)
{
    UINT32 RegAddr;
    T32OCSC_DENORM_RANGE_OVERONE stDenorm_range_overone; //DENORM.

    //DENORM value assignment
    stDenorm_range_overone.uOCSC_DENORM_RANGE_OVERONE = VppDVOcsc_Denorm[0];

    //Register Write Operations
    RegAddr = VPP_BASE_ADDR_OCSC;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_OCSC_OCSC_DENORM_RANGE_OVERONE, stDenorm_range_overone.u32);
}

static INT VPP_BE_OCSC_Config(VPP_OBJ *vpp_obj, VPP_DV_OCSC_MODE OcscMode)
{
    VPP_BE_OCSC_Coeff_RGB2YUV(vpp_obj, OcscMode2SubMode[OcscMode]);
    VPP_BE_OCSC_Denorm(vpp_obj);
    return 0;
}

static INT VPP_BE_OCSC_Control(VPP_OBJ *vpp_obj, UINT32 ctrl_obj)
{
    UINT32 RegAddr;
    vpp_obj->be.m_uiDvOcscCtrl = ctrl_obj;
    RegAddr = VPP_BASE_ADDR_OCSC; //Register Write Operations.
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr + RA_OCSC_OCSC_CTRL, ctrl_obj);
    return 0;
}

INT VPP_BE_VOP_LoadCscCoeff(VPP_OBJ *vpp_obj, INT ColorFmt, INT ActWidth)
{
    T32OCSC_CTRL stOcscCtrl;
    UINT32 uiVopCscMode;

    uiVopCscMode = ColorFmt2CscMode[ColorFmt];
    stOcscCtrl.u32 = 0;
    stOcscCtrl.uOCSC_IBT1886 = 0;
    stOcscCtrl.uOCSC_HACTIVE = ActWidth;
    stOcscCtrl.uOCSC_INPSWAPSEL_Cb = 2;
    stOcscCtrl.uOCSC_INPSWAPSEL_Cr = 1;
    stOcscCtrl.uOCSC_INPSWAPSEL_Y = 0;
    stOcscCtrl.uOCSC_OUT_UVSWAP = 0;
    stOcscCtrl.uOCSC_IFORMAT = VPP_DV_OCSC_IFORMAT_RGB444;
    stOcscCtrl.uOCSC_OFORMAT = OcscMode2OutFmt[uiVopCscMode];

    if (VPP_DV_OCSC_MODE_BYPASS == uiVopCscMode) {
        /* To get RGB output from RGB2YUV OCSC module, we need to set output format as YUV444 and
        UV swap need to be set */
        stOcscCtrl.uOCSC_OUT_UVSWAP = 1;
        stOcscCtrl.uOCSC_OFORMAT = VPP_DV_OCSC_OFORMAT_YUV444;
    } else {
        stOcscCtrl.uOCSC_OFORMAT = OcscMode2OutFmt[uiVopCscMode];
    }
    VPP_BE_OCSC_Control(vpp_obj, stOcscCtrl.u32);
    VPP_BE_OCSC_Config(vpp_obj, uiVopCscMode);
    return MV_VPP_OK;
}

#endif
