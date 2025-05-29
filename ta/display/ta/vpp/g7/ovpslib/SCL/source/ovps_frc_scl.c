/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#define _OVP_FRC_SCL_C_

#include "vpp_module.h"
#include "vpp_api.h"
#include "vpp_cfg.h"
#include "vpp_frc_scl.h"
#include "ovps_bcmbuf.h"
#include "ovps_frc_scl_prv.h"
#include "vpp.h"
#include "ovps_coeff_hsoft.h"
#include "ovps_coeff_adp_scl_soft.h"
#include "vpp_clkGate.h"
#include "ovps_api.h"

static UINT16 default_index_coeff_table[COEFF_MAX][VPP_FRC_SCL_NUM_OF_PHASES][VPP_FRC_SCL_NUM_OF_COEFF];

/***************************************************************************************
 * FUNCTION: INTernal fuction to get the start address of an FRC SCL instance by unit number
 * PARAMS: UnitNum - unit number
 * RETURN:  the start address
 ***************************************************************************************/
static UINT32 INTERNAL_OVPS_FRC_SCL_GetRegStartAddr(VPP_OBJ *vpp_obj, VPP_FRC_SCL_NUM UnitNum)
{
    UINT32 uiRegAddr = 0;

    /*get register start address */
    uiRegAddr = vpp_obj->base_addr;
    switch (UnitNum) {
        case VPP_FRC_SCL_OVP_Y:
            /* As VIP base addresss is different, prgm uiRegAddr freshly */
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC;
            break;
        case VPP_FRC_SCL_OVP_UV:
            /* As VIP base addresss is different, prgm uiRegAddr freshly */
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC;
            break;
        default:
            return 0;
            break;
    }

    return uiRegAddr;

}

#define OVPS_BCMBUF_Write_DBG(XX,YY,ZZ) OVPS_BCMBUF_Write(XX,YY,ZZ)

/***************************************************************************************
 * FUNCTION: load the default register values to FRC SCL the unit
 * PARAMS: UnitNum - unit number
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_LoadDefaultVal(VPP_OBJ *vpp_obj, INT UnitNum)
{
    INT uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    T32GFX0_SCL_BYPASS gfx0_scl_bypass;
    T32GFX1_SCL_BYPASS gfx1_scl_bypass;

    uiRegAddr = INTERNAL_OVPS_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
    if (0 == uiRegAddr) {
        return MV_VPP_EBADPARAM;
    }
    vpp_obj->uiCpcb0Ctrl8 = 0x00064064;
    vpp_obj->uiCpcb0Ctrl9 = 0x00038064;
    vpp_obj->uiSclCfg15[UnitNum] = 0;
    vpp_obj->uiSclCfg0[UnitNum] = 0;

    vpp_obj->hscl_coeff[UnitNum] = -1; // invalid
    vpp_obj->vscl_coeff[UnitNum] = -1; // invalid

    /* For customized SCL coeffs */
    vpp_obj->hscl_coeff_update[UnitNum] = 0; // No customized
    vpp_obj->vscl_coeff_update[UnitNum] = 0; // No customized

    vpp_obj->scl_coeff_update[UnitNum] = 0;
    vpp_obj->scl_force_sel[UnitNum] = MODE_COEFF_SEL_AUTO;

    /*Cropping related initilization*/
    vpp_obj->scl_ctrl[UnitNum].leftCrop     = 0;
    vpp_obj->scl_ctrl[UnitNum].rightCrop    = 0;
    vpp_obj->scl_ctrl[UnitNum].uncrop_IHRes = 0;
    vpp_obj->scl_ctrl[UnitNum].uiUVswapEn   = 0;

    /* Crop after scaling related initialization */
    vpp_obj->scl_ctrl[UnitNum].cropAfterSclLeft    = 0;
    vpp_obj->scl_ctrl[UnitNum].cropAfterSclRight   = 0;
    vpp_obj->scl_ctrl[UnitNum].cropAfterSclTop     = 0;
    vpp_obj->scl_ctrl[UnitNum].cropAfterSclBottom  = 0;

    //for GFX1 & GFX0 - Disable Bypass by default
    switch (UnitNum) {
        case VPP_FRC_SCL_PIP:
            GA_REG_WORD32_READ(uiRegAddr + RA_GFX0_SCL_BYPASS, &gfx0_scl_bypass.u32);
            gfx0_scl_bypass.uSCL_BYPASS_en = 0;
            GA_REG_WORD32_WRITE(uiRegAddr + RA_GFX0_SCL_BYPASS, gfx0_scl_bypass.u32);
            break;
        case VPP_FRC_SCL_OSD:
            GA_REG_WORD32_READ(uiRegAddr + RA_GFX1_SCL_BYPASS, &gfx1_scl_bypass.u32);
            gfx1_scl_bypass.uSCL_BYPASS_en = 0;
            GA_REG_WORD32_WRITE(uiRegAddr + RA_GFX1_SCL_BYPASS, gfx1_scl_bypass.u32);
            break;
    }
    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: enable/disable alpha divider of GFX0 or PG scaler
 * PARAMS: UnitNum - unit number(2, 3)
 *         Enable - 1: enable, 0: disable
 * RETURN:  1 - succeed
 *                others - failed to load register to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetAlphaDivider(VPP_OBJ *vpp_obj, INT UnitNum, INT Enable)
{
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: set BE FIFO for loopback
 * PARAMS: pCpcb0Tot - CPCB0 output total resolution
 *               pCpcb0Res - CPCB0 active resolution
 * RETURN:  1 - succeed
 *                others - failed to load CPCB BE FIFO values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetBeFifo(VPP_OBJ *vpp_obj, PVPP_FRC_RES pCpcb0Tot, PVPP_FRC_RES pCpcb0Res)
{
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: Load the scalar UPS coefficients
 * PARAMS:
 *             vpp_obj - vpp object
 *            piCoeff - UPS coefficients
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_ConfigUpsCoeff(VPP_OBJ *vpp_obj, INT *piCoeff, VPP_FRC_SCL_NUM UnitNum)
{
    return MV_VPP_OK;
}


/***************************************************************************************
 * FUNCTION: Load the scalar DNS coefficients
 * PARAMS:
 *             vpp_obj - vpp object
 *             piCoeff -  DNS coefficients
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_ConfigDnsCoeff(VPP_OBJ *vpp_obj, INT *piCoeff, VPP_FRC_SCL_NUM UnitNum)
{
    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: Read the  Main  scalar coefficients
 * PARAMS:
 *             vpp_obj - vpp object
 *             UnitNum - Unit num to identify the plain
 *             HTapN   - Number of H Taps
 *             VTapN   - Number of V Taps
 *             hlut    - Horizontal coefficients
 *             vlut    - Vertical Coefficients
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer or invalide input
 ***************************************************************************************/
INT OVPSCL_MainCoeffRead(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8])
{
    UINT32 uiRegAddr = 0;
    INT16 i;
    INT uiRetVal = MV_VPP_OK;

    T32ADPSCL_COEFF_CFG0 stSclCoeffCfg0;
    T32ADPSCL_COEFF_CFG1 stSclCoeffCfg1;
    T32ADPSCL_COEFF_CFG2 stSclCoeffCfg2;
    T32ADPSCL_COEFF_CFG3 stSclCoeffCfg3;
    T32ADPSCL_COEFF_CFG4 stSclCoeffCfg4;

    switch (UnitNum) {
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
        default:
            return MV_VPP_EBADPARAM;
    }
    GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG0, &stSclCoeffCfg0.u32);
    stSclCoeffCfg0.uCFG0_coeffload   = 0;

    /*Horizontal Coeff table loading*/
    stSclCoeffCfg0.uCFG0_coeff_hvsel = 1;
    for (i=0; i<=16; i++) {
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        GA_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);

        stSclCoeffCfg0.uCFG0_coeffread   = 1;
        GA_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);

        GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG1, &stSclCoeffCfg1.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG2, &stSclCoeffCfg2.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG3, &stSclCoeffCfg3.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG4, &stSclCoeffCfg4.u32);

        hlut[i][0] = stSclCoeffCfg1.uCFG1_coeff0;
        hlut[i][1] = stSclCoeffCfg1.uCFG1_coeff1;
        hlut[i][2] = stSclCoeffCfg2.uCFG2_coeff2;
        hlut[i][3] = stSclCoeffCfg2.uCFG2_coeff3;
        hlut[i][4] = stSclCoeffCfg3.uCFG3_coeff4;
        hlut[i][5] = stSclCoeffCfg3.uCFG3_coeff5;
        hlut[i][6] = stSclCoeffCfg4.uCFG4_coeff6;
        hlut[i][7] = stSclCoeffCfg4.uCFG4_coeff7;
        stSclCoeffCfg0.uCFG0_coeffread   = 0;
        GA_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);
    }

    /*Vertical Coeff table loading*/
    stSclCoeffCfg0.uCFG0_coeff_hvsel = 0;
    for (i=0; i<=16; i++) {
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        GA_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);

        stSclCoeffCfg0.uCFG0_coeffread   = 1;
        GA_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);

        GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG1, &stSclCoeffCfg1.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG2, &stSclCoeffCfg2.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG3, &stSclCoeffCfg3.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG4, &stSclCoeffCfg4.u32);

        vlut[i][0] = stSclCoeffCfg1.uCFG1_coeff0;
        vlut[i][1] = stSclCoeffCfg1.uCFG1_coeff1;
        vlut[i][2] = stSclCoeffCfg2.uCFG2_coeff2;
        vlut[i][3] = stSclCoeffCfg2.uCFG2_coeff3;
        vlut[i][4] = stSclCoeffCfg3.uCFG3_coeff4;
        vlut[i][5] = stSclCoeffCfg3.uCFG3_coeff5;
        vlut[i][6] = stSclCoeffCfg4.uCFG4_coeff6;
        vlut[i][7] = stSclCoeffCfg4.uCFG4_coeff7;
        stSclCoeffCfg0.uCFG0_coeffread   = 0;
        GA_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);
    }

    return uiRetVal;

}

/***************************************************************************************
 * FUNCTION: Read the  GFX  scalar coefficients
 * PARAMS:
 *             vpp_obj - vpp object
 *             UnitNum - Unit num to identify the plain
 *             HTapN   - Number of H Taps
 *             VTapN   - Number of V Taps
 *             hlut    - Horizontal coefficients
 *             vlut    - Vertical Coefficients
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer or invalide input
 ***************************************************************************************/
INT OVPSCL_GfxCoeffRead(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8])
{
    UINT32 uiRegAddr = 0;
    INT16 i;
    INT uiRetVal = MV_VPP_OK;

    T32COEFF_CFG0 stSclCoeffCfg0;
    T32COEFF_CFG1 stSclCoeffCfg1;
    T32COEFF_CFG2 stSclCoeffCfg2;
    T32COEFF_CFG3 stSclCoeffCfg3;
    T32COEFF_CFG4 stSclCoeffCfg4;

    /*get register start address */
    switch (UnitNum) {
        case VPP_FRC_SCL_PIP:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_COEFF_GFX0;
            break;
        case VPP_FRC_SCL_OSD:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_COEFF_GFX1;
            break;
        default:
            return 0;
            break;
    }
    GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG0, &stSclCoeffCfg0.u32);
    stSclCoeffCfg0.uCFG0_coeffload   = 0;

    /*Horizontal Coeff table loading*/
    stSclCoeffCfg0.uCFG0_coeff_hvsel = 1;
    for (i=0; i<=16; i++) {
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        GA_REG_WORD32_WRITE(uiRegAddr + RA_COEFF_CFG0, stSclCoeffCfg0.u32);

        GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG1, &stSclCoeffCfg1.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG2, &stSclCoeffCfg2.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG3, &stSclCoeffCfg3.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG4, &stSclCoeffCfg4.u32);

        hlut[i][0] = stSclCoeffCfg1.uCFG1_coeff0;
        hlut[i][1] = stSclCoeffCfg1.uCFG1_coeff1;
        hlut[i][2] = stSclCoeffCfg2.uCFG2_coeff2;
        hlut[i][3] = stSclCoeffCfg2.uCFG2_coeff3;
        hlut[i][4] = stSclCoeffCfg3.uCFG3_coeff4;
        hlut[i][5] = stSclCoeffCfg3.uCFG3_coeff5;
        hlut[i][6] = stSclCoeffCfg4.uCFG4_coeff6;
        hlut[i][7] = stSclCoeffCfg4.uCFG4_coeff7;

    }

    /*Vertical Coeff table loading*/
    stSclCoeffCfg0.uCFG0_coeff_hvsel = 0;
    for (i=0; i<=16; i++) {
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        GA_REG_WORD32_WRITE(uiRegAddr + RA_COEFF_CFG0, stSclCoeffCfg0.u32);

        GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG1, &stSclCoeffCfg1.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG2, &stSclCoeffCfg2.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG3, &stSclCoeffCfg3.u32);
        GA_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG4, &stSclCoeffCfg4.u32);

        vlut[i][0] = stSclCoeffCfg1.uCFG1_coeff0;
        vlut[i][1] = stSclCoeffCfg1.uCFG1_coeff1;
        vlut[i][2] = stSclCoeffCfg2.uCFG2_coeff2;
        vlut[i][3] = stSclCoeffCfg2.uCFG2_coeff3;
        vlut[i][4] = stSclCoeffCfg3.uCFG3_coeff4;
        vlut[i][5] = stSclCoeffCfg3.uCFG3_coeff5;
        vlut[i][6] = stSclCoeffCfg4.uCFG4_coeff6;
        vlut[i][7] = stSclCoeffCfg4.uCFG4_coeff7;
    }

    return uiRetVal;

}

/***************************************************************************************
 * FUNCTION: Calulate the non linear scaling parameters
 * PARAMS:
 *             vpp_obj - vpp object
 *             pscl_init_ratio - Intial DDA increment
 *             pscl_inc_ratio  - Increment to DDA increment
 *             pscl_nlcres     - Central undistorted width of the output image
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer or invalide input
 ***************************************************************************************/
static INT OVPSCL_CalcNLSclParams(PVPP_SCL_RES pIORes, INT32 *pscl_init_ratio, INT32 *pscl_inc_ratio, INT32 *pscl_nlcres)
{
    UINT32 ires1, ores1, ires2, ores2;
    UINT32 tail_ores;
    UINT32 c_scale = 0;
    UINT32 scl_nlcres = 0;
    UINT32 c_ratio = 0, c_ires = 0,tail_ires = 0;
    UINT32 nl_inc_ratio = 0, nl_init_ratio = 0;

    ires1 = pIORes->IHRes;
    ores1 = pIORes->OHRes;
    ires2 = pIORes->IVRes;
    ores2 = pIORes->OVRes;

    c_scale = 255; /* TODO Get the init val */
    //check 1: minimum tail length limitation
    tail_ores = (256-c_scale)*ores1/512; // o/p tail zone
    if (tail_ores < 5) {
        tail_ores = 5;
        c_scale = 256 - tail_ores*512/ores1;
    }
    ////////////////////////////////////////////////////////////////
    scl_nlcres = ores1 - 2*tail_ores; //central undistorted o/p res
    c_ratio = ires2*(1<<20)/ores2; //central preserved aspect ratio
    c_ires = (c_ratio * scl_nlcres + (1<<19)) >> 20; //central undistorted i/p res
    tail_ires = (ires1 - c_ires)>>1; //input tail zone

    nl_inc_ratio = 2*(c_ratio - (1<<20)*tail_ires/tail_ores) / (tail_ores+1);
    nl_init_ratio = c_ratio - (nl_inc_ratio) * tail_ores;
    ////////////////////////////////////////////////////////////////
    //check 2: c_ratio and nl_init_ratio should not be negative, but nl_inc_ratio can.
    if (nl_init_ratio < 0) {
        nl_init_ratio = 0;
        tail_ores = (((ires2*ores1/ores2)-ires1)<<20)/((c_ratio-nl_init_ratio)==0?1:(c_ratio-nl_init_ratio));
        if (tail_ores < 5) {
            tail_ores = 5;
        } else if (tail_ores>=(ores1>>1)) {
            tail_ores = ((ores1-20+1)>>1);
        }
        c_ratio = ((ires1<<20)-nl_init_ratio*tail_ores)/(ores1-tail_ores);
        scl_nlcres = ores1 - (tail_ores<<1);
        c_ires = (c_ratio*scl_nlcres + (1<<19))>>20;
        c_scale = 256 - (tail_ores<<9)/ores1;
        nl_inc_ratio = ((c_ratio-nl_init_ratio)+(tail_ores>>1))/tail_ores;
    }
    // Check 3: limited by max downscaling ratio 255
    else if (nl_init_ratio >= (INT)(1<<20)*255) {
        nl_init_ratio = (INT)(1<<20)*255;
        tail_ores = (((ires2*ores1/ores2)-ires1)<<20)/((c_ratio-nl_init_ratio)==0?1:(c_ratio-nl_init_ratio));
        if (tail_ores < 5) {
            tail_ores = 5;
        } else if (tail_ores>=(ores2>>1)) {
            tail_ores = ((ores2-20+1)>>1);
        }
        c_ratio = ((ires1<<20)-nl_init_ratio*tail_ores)/(ores1-tail_ores);
        scl_nlcres = ores1 - (tail_ores<<1);
        c_ires = ((c_ratio*scl_nlcres) + (1<<19))>>20;
        c_scale = 256 - (tail_ores<<9)/ores1;
        nl_inc_ratio = ((c_ratio-nl_init_ratio)+(tail_ores>>1))/tail_ores;
    }
    *pscl_inc_ratio = nl_inc_ratio; //increment to hratio
    *pscl_init_ratio = nl_init_ratio; //initial hratio
    *pscl_nlcres = scl_nlcres;

    return 0;
}

static INT OVPSCL_MainSclAdp1dType(VPP_OBJ *vpp_obj, PVPP_SCL_RES pIORes, INT8 *sel_1D_adp)
{
    *sel_1D_adp = 1;/* 1D Scalar */

    return MV_VPP_OK;
}
/*--------------------------
  -calculate htap and vtap numbers-
 ---------------------------*/
static INT OVPSCL_MainSclCalTaps(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, INT8 *phTap, INT8 *pvTap)
{
    INT8 sel_1D_adp, hTaps, vTaps;
    UINT32 max_pixels_in_scl_mem = 1920;
    if (VPP_FRC_SCL_OVP_UV == UnitNum) {
        /*For Tap calculation should be same for both Luma and Croma.
          So multiplying croma height by 2 to make both luma and croma having same height */
        pIORes->OHRes = pIORes->OHRes * 2;
    }
    OVPSCL_MainSclAdp1dType(vpp_obj, pIORes, &sel_1D_adp);
    if (0 == sel_1D_adp) {
        hTaps = 4;
        vTaps = 4;
    } else {
        if ((pIORes->OHRes > pIORes->IHRes) && (pIORes->IHRes > 1920))/* Upscale */
            hTaps = 5;
        else
            hTaps = 8;

        max_pixels_in_scl_mem = 960;

        if ((pIORes->IVRes == pIORes->OVRes) && (pIORes->OHRes == pIORes->IHRes))
            vTaps = 1;//3;//6;//1;/*horizontalOnly scale modes including bypass modes where scaler.s line buffer SRAM is used as rat control FIFO*/
        else if (pIORes->IVRes == pIORes->OVRes)
            vTaps = 1;//3;//6;//1;/*horizontalOnly scale modes including bypass modes where scaler.s line buffer SRAM is used as rat control FIFO*/
        else if ((pIORes->OHRes > pIORes->IHRes) && (pIORes->IHRes > max_pixels_in_scl_mem))/* Upscale */
            vTaps = 3;
        else if ((pIORes->OHRes <= pIORes->IHRes)) {/* DownScale */
            /*OVP and VIP SCL has memsize max ((575+1)*10) pixles. So if OHRes less than 1152 only can hold 5 lines*/
            if (pIORes->OHRes < 1152)
                vTaps = 5;
            else
                vTaps = 3;
        }
        else if ((pIORes->OVRes > pIORes->IVRes) && (pIORes->OVRes > 1440))/* Fixing the faint line issue for up scaling o/p Vres more than 1440 */
            vTaps = 5;
        else
            vTaps = 6;

    }
    if (VPP_FRC_SCL_OVP_UV == UnitNum) {
        /*REVERTING :: For VIP path Tap calculation should be same for both Luma and Croma.
          So multiplying croma hight by 2 to make both luma and croma having same height */
        pIORes->OHRes = pIORes->OHRes / 2;
    }
    *phTap = hTaps;
    *pvTap = vTaps;
    return MV_VPP_OK;
}

/*--------------------------
  -Configure Scl for special cases in Deint-
 ---------------------------*/
static INT OVPSCL_SclConfigForDeint(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, INT8 *enDeintSmallUpScale)
{
    *enDeintSmallUpScale = 0;
    return (MV_VPP_OK);
}

/***************************************************************************************
 * FUNCTION: Load the Main scalar parameters
 * PARAMS:
 *           vpp_obj - vpp object
 *           UnitNum - Unit num to identify the plain
 *           pIORes - Input/Output resolution
 *           pSclCtrl - Scalar Control paramters
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
static INT OVPSCL_SetMainSclparams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    INT8 sel_1D_adp = -1;
    INT8 up_down = -1;
    UINT32 HRatio, VRatio;
    UINT32 minHRes, hRes;
    INT8 hTaps = -1, vTaps = -1;
    INT8 enDeintSmallUpScale = 0;
    INT32 scl_init_ratio, scl_inc_ratio,  scl_nlcres;
    INT16 wls = 0, pls = 0, fcp = 0;
#ifdef BG4CT_A0
    INT32 memsize = 0;
#endif
    INT16 maxMemSize_1D = 0;

    VPP_SCL_CTRL0 stSclCtrl0;
    VPP_SCL_CTRL1 stSclCtrl1;
    INT uiRetVal = MV_VPP_OK;

    T32ADPSCL_CFG0 stSclCfg0;
    T32ADPSCL_CFG1 stSclCfg1;
    T32ADPSCL_CFG2 stSclCfg2;
    T32ADPSCL_CFG3 stSclCfg3 = {0};
    T32ADPSCL_CFG4 stSclCfg4 = {0};
    T32ADPSCL_CFG5 stSclCfg5;
    T32ADPSCL_CFG6 stSclCfg6;
    T32ADPSCL_CFG7 stSclCfg7 = {0};
    T32ADPSCL_CFG8 stSclCfg8;
    T32ADPSCL_CFG9 stSclCfg9;
    T32ADPSCL_CFG10 stSclCfg10;
    T32ADPSCL_CFG11 stSclCfg11;
    T32ADPSCL_CFG12 stSclCfg12;
    T32ADPSCL_CFG15 stSclCfg15;
    T32ADPSCL_CFG16 stSclCfg16;
    T32ADPSCL_CFG17 stSclCfg17;
    T32ADPSCL_CFG18 stSclCfg18;
    T32ADPSCL_CFG19 stSclCfg19;
    T32SCL1D_YC_CFG20 stScl1dYcCfg20;

    UINT32 uiRegAddr = 0;
    UINT32 uncrop_IHRes = pSclCtrl->uncrop_IHRes;

    if ((VPP_FRC_SCL_OVP_Y != UnitNum) && (VPP_FRC_SCL_OVP_UV != UnitNum))
        return MV_VPP_EBADPARAM;

    /*-------------------------
      - select the scalar -
      --------------------------*/
    OVPSCL_SclConfigForDeint(vpp_obj, UnitNum, pIORes, &enDeintSmallUpScale);
    OVPSCL_MainSclAdp1dType(vpp_obj, pIORes, &sel_1D_adp);

    /*-------------------------
      - select the up/down scale mode -
      --------------------------*/

    if ((pIORes->OHRes < pIORes->IHRes) || ((pIORes->OHRes == pIORes->IHRes) && (pIORes->OVRes <  pIORes->IVRes))) {
        /* Horizontal down scaling or veritcal only down scaling */
        up_down = 1;/* down scale */
    } else
        up_down = 0;/* up scale/1:1(bypass) */

    uiRegAddr = INTERNAL_OVPS_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);/* Following this are SCL programming */
    if (0 == uiRegAddr) {
        return MV_VPP_EBADPARAM;
    }
    /*--------------------------
      -calculate htap and vtap numbers-
      ---------------------------*/
    OVPSCL_MainSclCalTaps(vpp_obj, UnitNum, pIORes, &hTaps, &vTaps);
    /*-------------------------
      - set initial phase and ctl0 -
      --------------------------*/
    if (0 == sel_1D_adp) {
        stSclCfg0.uCFG0_even = 0x80;
        stSclCfg0.uCFG0_odd = 0x80;
        stSclCfg0.uCFG0_hinitph = 0x80;
    } else {
        stSclCfg0.uCFG0_even = (vTaps % 2)<<7;
        stSclCfg0.uCFG0_odd = (vTaps % 2)<<7;
        stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
    }

    stSclCtrl0.uchAvg4FiltSel = 0;
    stSclCtrl0.uchCscEn = 0;
    if (1 == pSclCtrl->NLEn) {
        stSclCtrl0.uchHnlEn = 1;
    } else {
        stSclCtrl0.uchHnlEn = 0;
    }

    if (1 == sel_1D_adp && 1 == up_down) {
        /* 1D down scaling */
        stSclCtrl0.uchUpsEn = 1;
        stSclCtrl0.uchDnsEn = 1;
    } else {
        stSclCtrl0.uchUpsEn = 0;
        stSclCtrl0.uchDnsEn = 0;
    }
    stSclCtrl0.uchDgnlEn = 1;
    stSclCtrl0.uchDemoModeEn = 0;

#ifdef BG4CT_A0
    //This is not required if clock gating is done after SetSclParams()
    stSclCfg15.u32 = vpp_obj->uiSclCfg15[UnitNum];
    if (stSclCfg15.uCFG15_bypass) {
        //Enable UPS
        stSclCtrl0.uchUpsEn = 1;
    }
#endif

    stSclCfg0.uCFG0_ctrl0 = stSclCtrl0.uch8;
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG0, stSclCfg0.u32));
    vpp_obj->uiSclCfg0[UnitNum] = stSclCfg0.u32;
    /*-------------------------
      - set input and output resolutions -
      --------------------------*/

    stSclCfg1.uCFG1_ivres = pIORes->IVRes;
    stSclCfg1.uCFG1_ovres = pIORes->OVRes;
    stSclCfg2.uCFG2_ihres = uncrop_IHRes;          //Read the complete uncropped resolution
    stSclCfg2.uCFG2_ohres = pIORes->OHRes;

    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG1, stSclCfg1.u32));
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG2, stSclCfg2.u32));

    /*-------------------------------------
      -calculate horizontal and vertical scaling ratio -
      -------------------------------------*/

    HRatio = ((UINT32) pIORes->IHRes << 20) / pIORes->OHRes;
    VRatio = ((UINT32) pIORes->IVRes << 20) / pIORes->OVRes;

    stSclCfg3.u32 = 0;
    stSclCfg4.u32 = 0;

    stSclCfg3.uCFG3_vratio = VRatio;
    stSclCfg4.uCFG4_hratio = HRatio;

    if (pIORes->OVRes < pIORes->IVRes) {
        /* 1: the most bottom pixel of input image is used to interpolate the most bottom pixel of output image */
        stSclCfg3.uCFG3_mbp_align = 1;
    } else {
        /* Must be 0 if Vertically '1:1 or upscale' */
        stSclCfg3.uCFG3_mbp_align = 0;
    }

    if (pIORes->OHRes < pIORes->IHRes) {
        /* 1: the most right pixel of input image is used to interpolate the most right pixel of output image */
        stSclCfg4.uCFG4_mrp_align = 1;
    } else {
        /* Must be 0 if Horizontally '1:1 or upscale' */
        stSclCfg4.uCFG4_mrp_align = 0;
    }

    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG3 , stSclCfg3.u32));
    vpp_obj->uiSclCfg3[UnitNum] = stSclCfg3.u32;
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG4 , stSclCfg4.u32));

    stSclCfg7.uCFG7_ups_cblank = 0x200;

    stSclCfg7.uCFG7_htap_offset = 0;
    stSclCfg7.uCFG7_vtap_offset = 0;
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG7, stSclCfg7.u32));
    vpp_obj->uiSclCfg7[UnitNum] = stSclCfg7.u32;

    /*-------------------
      -set SCL control register-
      --------------------*/
    /*if non linear scaling, H scaler is after V scaler*/
    if (0 == pSclCtrl->NLEn) {
        stSclCfg8.uCFG8_init_ratio = 0x100000;
        stSclCfg9.uCFG9_inc_ratio  = 0;
        stSclCfg10.uCFG10_nlcres   = 0;
    } else {
        OVPSCL_CalcNLSclParams(pIORes, &scl_init_ratio, &scl_inc_ratio, &scl_nlcres);
        stSclCfg8.uCFG8_init_ratio = scl_init_ratio;
        stSclCfg9.uCFG9_inc_ratio  = scl_inc_ratio;
        stSclCfg10.uCFG10_nlcres   = scl_nlcres;
    }
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG8, stSclCfg8.u32));
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG9, stSclCfg9.u32));
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG10, stSclCfg10.u32));

    stSclCfg11.uCFG11_avg4_coeff0 = 0x840;
    stSclCfg11.uCFG11_avg4_coeff1 = 0x240;
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG11 , stSclCfg11.u32));

    stSclCfg12.uCFG12_avg4_coeff2 = 0x240;
    stSclCfg12.uCFG12_avg4_coeff3 = 0x840;
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG12 , stSclCfg12.u32));
    stSclCtrl1.uch8 = 0;
    stSclCtrl1.uchSel_1D_adp = sel_1D_adp;
    stSclCtrl1.uchUp_down = up_down;
    if (0 == sel_1D_adp) {
        stSclCtrl1.uchVSclInUpsEn    = 1;
        stSclCtrl1.uchVSclOutUpsEn   = 0;
        stSclCtrl1.uchVSclInUpsCoeff = 0;
    } else {
        stSclCtrl1.uchVSclInUpsEn    = 0;
        stSclCtrl1.uchVSclInUpsCoeff = 0;
        stSclCtrl1.uchVSclOutUpsEn   = 0;
    }

    stSclCfg15.u32 = vpp_obj->uiSclCfg15[UnitNum];
    stSclCfg15.uCFG15_ctrl1 = stSclCtrl1.uch8;
    stSclCfg15.uCFG15_frndsel = 1;
    stSclCfg15.uCFG15_hctrl = 0x1A;
    if ((1 == vTaps || 2 == vTaps)
#ifdef BG4CT_A0
            && (0 == stSclCfg15.uCFG15_vscl11)
#endif
      )
        stSclCfg15.uCFG15_vctrl = 0x0B;
    else
        stSclCfg15.uCFG15_vctrl = 0x1A;

    //LSB 2 bit for to crop at begining of line, MSB 2 bit to crop at end of line
    stSclCfg15.uCFG15_crop = ((pSclCtrl->leftCrop&0x3)) | ((pSclCtrl->rightCrop&0x03)<<2);
    VDB_print("Scalar crop control : 0x%x\n", stSclCfg15.uCFG15_crop);
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG15 , stSclCfg15.u32));
    vpp_obj->uiSclCfg15[UnitNum] = stSclCfg15.u32;

    hRes = uncrop_IHRes;   //lsize calculation should be based on complete/uncropped resolution
    if (0 == sel_1D_adp) {
        stSclCfg17.uCFG17_lsize   = (hRes/ 10) + ((hRes% 10)? 1:0);
        stSclCfg17.uCFG17_lsize_A = (hRes/ 10) + ((hRes% 10)? 1:0);
    } else {
        if (hRes < pIORes->OHRes)
            minHRes = hRes;
        else
            minHRes = pIORes->OHRes;
        /* If Scalar chop is enabled, then choose hres as follows
         * Upscale, -> hres = uncropped ihres
         * downscale, -> hres = ohres
         * 1:1, -> hres = uncropped ihres */
        if ((pSclCtrl->leftCrop+pSclCtrl->rightCrop) != 0) {
            //Use cropped Hres for comparision
            if (pIORes->IHRes <= pIORes->OHRes)
                minHRes = hRes;
            else
                minHRes = pIORes->OHRes;
        }
        stSclCfg17.uCFG17_lsize   = (minHRes/ 10) + ((minHRes% 10)? 1:0);
        stSclCfg17.uCFG17_lsize_A = (minHRes/ 10) + ((minHRes% 10)? 1:0);
    }
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG17 , stSclCfg17.u32));
    stSclCfg18.u32=0;
    maxMemSize_1D = 575;
#ifdef BG4CT_A0
    /* Prefetch mode Settings */
    /* Prefetch only enable in 4K small downscaling with 4K canvas.
       If enabled for bigger PIP style cases needs specific tunning of udf/ovf margins, which is not required */
    /* Making the min resolution to RES_MIN_4Kx2K instead of 4K30 to avoid line issue when small down scaling while playing Dolby L5 cert streams */
    stSclCfg18.uCFG18_pftch = 0;

    if (1 == stSclCfg18.uCFG18_pftch) {
        memsize = (stSclCfg17.uCFG17_lsize * vTaps) - 1;
        if ((1 == sel_1D_adp) && (memsize > maxMemSize_1D)) {
            stSclCfg5.uCFG5_memsize = maxMemSize_1D;
        } else if ((0 == sel_1D_adp) && (memsize > 1727))
            stSclCfg5.uCFG5_memsize = 1727;
        else
            stSclCfg5.uCFG5_memsize = memsize;
    }
    else
#endif
    {
        if (0 == sel_1D_adp)
            stSclCfg5.uCFG5_memsize = 1727;
        else
            stSclCfg5.uCFG5_memsize = maxMemSize_1D;
    }
    stSclCfg5.uCFG5_vwrap   = 0;
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG5 , stSclCfg5.u32));

    stSclCfg6.uCFG6_ups_cswap = pSclCtrl->uiUVswapEn;
    stSclCfg6.uCFG6_ups_cshift = 0;
    stSclCfg6.uCFG6_ups_yshift = 0;
    stSclCfg6.uCFG6_ups_yblank = 0;

    if ((1920 < pIORes->IHRes &&
                (((pIORes->IHRes == pIORes->OHRes) && (pIORes->IVRes == pIORes->OVRes))/* 1:1 case */ ||
                 (pIORes->IVRes > pIORes->OVRes && ((((pIORes->IVRes - pIORes->OVRes)* 100)/ (pIORes->IVRes)) <= 13 )))) /* vertical only or both vertical and horizontal down scaling till 13% */
#ifdef BG4CT_A0
            || (1 == stSclCfg18.uCFG18_pftch)
#endif
    ) {
        stSclCfg6.uCFG6_fstall = 0x0;
    } else {
        stSclCfg6.uCFG6_fstall = 0x80;
    }
    stSclCfg6.uCFG6_bstall = 0x80;

    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG6, stSclCfg6.u32));
    vpp_obj->uiSclCfg6[UnitNum] = stSclCfg6.u32;

    if (((pIORes->IVRes == pIORes->OVRes) && (1 == vTaps))
#ifdef BG4CT_A0
            && (0 == stSclCfg15.uCFG15_vscl11)
#endif
    )
        stSclCfg18.uCFG18_fifo_mode  = 1;
    else
        stSclCfg18.uCFG18_fifo_mode  = 0;

    if ((1 == stSclCfg18.uCFG18_fifo_mode)
#ifdef BG4CT_A0
            || (1 == stSclCfg18.uCFG18_pftch)
#endif
    ) {
        if (pIORes->IHRes < pIORes->OHRes)
            minHRes = pIORes->IHRes;
        else
            minHRes = pIORes->OHRes;
        wls = (stSclCfg5.uCFG5_memsize + 1) / stSclCfg17.uCFG17_lsize;
#ifndef BG4CT_A0
        pls = ((stSclCfg5.uCFG5_memsize + 1) - (stSclCfg17.uCFG17_lsize * wls)) * 5;
#else
        pls = ((stSclCfg5.uCFG5_memsize + 1) - (stSclCfg17.uCFG17_lsize * wls)) * 10;
#endif
        fcp = wls * minHRes + pls;

#if !defined(VPP_DIAGS) && !defined(BG4CT_A0)
        /*Hard coding to disable fifo mode to avoid un-recoverable UF condition when multiple plane is enabled
         *But fifo mode should be 1 for vtaps = 1 ;;;;;Require double check in BG4CT-A0.
         */
        stSclCfg18.uCFG18_fifo_mode  = 0;
#endif
#ifndef BG4CT_A0
        if ((fcp - 100) > 0x1FFF ) {/* Register bit width limitation*/
            stSclCfg18.uCFG18_fifo_depth = (0x1FFF - 100);
        } else {
            stSclCfg18.uCFG18_fifo_depth = (fcp - 100);
        }
#else
        stSclCfg18.uCFG18_fifo_depth = (stSclCfg18.uCFG18_pftch) ? fcp : (fcp - 100);
        if (0 == stSclCfg18.uCFG18_fifo_mode) {
            stSclCfg18.uCFG18_fifo_dfst  = 0;
        } else
#endif
        {
            stSclCfg18.uCFG18_fifo_dfst  = minHRes;
        }
    } else {
        stSclCfg18.uCFG18_fifo_mode  = 0;
        stSclCfg18.uCFG18_fifo_depth = 0;
        stSclCfg18.uCFG18_fifo_dfst  = 0;
    }

    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG18 , stSclCfg18.u32));

    stSclCfg16.u32=0;

    if ((0 == up_down)
#ifdef BG4CT_A0
            || (1 == stSclCfg18.uCFG18_pftch)
#endif
    ) {
        stSclCfg16.uCFG16_en_n          = 1;
    } else {
        stSclCfg16.uCFG16_en_n          = 0;
    }
    if ((pIORes->OHRes > pIORes->IHRes &&  pIORes->OVRes < pIORes->IVRes) ||
            (pIORes->OHRes < pIORes->IHRes &&  pIORes->OVRes > pIORes->IVRes))
        stSclCfg16.uCFG16_xbstall_en    = 0;
    else if (1 == enDeintSmallUpScale)
        stSclCfg16.uCFG16_xbstall_en    = 0;
    else if ((1 == stSclCfg16.uCFG16_en_n && 0 == up_down) || (1 == stSclCfg18.uCFG18_fifo_mode))
        stSclCfg16.uCFG16_xbstall_en    = 1;
    else
        stSclCfg16.uCFG16_xbstall_en    = 0;

    if ((pIORes->OHRes - pIORes->IHRes) >= 30)
        stSclCfg16.uCFG16_xbstall_dly   = 30;
    else
        stSclCfg16.uCFG16_xbstall_dly   = 0;

#ifdef BG4CT_A0
    if (1 == stSclCfg18.uCFG18_pftch) {
        stSclCfg16.uCFG16_ovf_margin = 0xA;
        stSclCfg16.uCFG16_udf_margin = 0x14;
    } else {
        stSclCfg16.uCFG16_ovf_margin = 0;
        stSclCfg16.uCFG16_udf_margin = 0;
    }
#endif
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG16 , stSclCfg16.u32));

    stSclCfg19.u32 = 0;
    stSclCfg19.uCFG19_scl_after_crop_h = ((pSclCtrl->cropAfterSclLeft << 4) | (pSclCtrl->cropAfterSclRight));
    stSclCfg19.uCFG19_scl_after_crop_v = ((pSclCtrl->cropAfterSclTop << 4) | (pSclCtrl->cropAfterSclBottom));
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG19 , stSclCfg19.u32));

    stScl1dYcCfg20.u32 = 0;
    stScl1dYcCfg20.uCFG20_dpwr_regs = 3;
    stScl1dYcCfg20.uCFG20_syp_yc420_regs = 0;
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG20 , stScl1dYcCfg20.u32));

    return uiRetVal;
}

/*--------------------------
  -calculate htap and vtap numbers-
 ---------------------------*/
static void OVPSCL_GfxSclCalTaps(VPP_OBJ *vpp_obj, PVPP_SCL_RES pIORes, INT8 *phTap, INT8 *pvTap)
{
    INT8 hTaps, vTaps;

    hTaps = 5;//8;

    if (pIORes->IVRes == pIORes->OVRes)
        vTaps = 1;/*horizontalOnly scale modes including bypass modes where scaler.s line buffer SRAM is used as rat control FIFO*/
    /*Bugfix: Fix for vertical only scaling. Considerted vertical resolution also for below 2 calculations */
    else if (((pIORes->OHRes > pIORes->IHRes) || (pIORes->OVRes > pIORes->IVRes)) && (pIORes->IHRes > 1440))/* Upscale */
        vTaps = 3;
    else if (((pIORes->OHRes < pIORes->IHRes) || (pIORes->OVRes < pIORes->IVRes)) && (pIORes->OHRes > 1440))/* Downscale */
        vTaps = 3;
    else
        vTaps = 4;

    //Force VTaps to 1 - 4K downscale for PIP/Gfx path
    if ((pIORes->OHRes < pIORes->IHRes) && (pIORes->IHRes > 1920)) {
        vTaps=1;
    }

    *phTap = hTaps;
    *pvTap = vTaps;

}

/***************************************************************************************
 * FUNCTION: Load the GFX scalar parameters
 * PARAMS:
 *           vpp_obj - vpp object
 *           UnitNum - Unit num to identify the plain
 *           pIORes - Input/Output resolution
 *           pSclCtrl - Scalar Control paramters
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_SetGfxSclparams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    INT uiRetVal = MV_VPP_OK;

    return uiRetVal;
}
/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -poINTer to I&O resolution
 *               pSclCtrl - poINTer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_ReadSclCoeff(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8])
{
    INT uiRetVal = MV_VPP_OK;

    CHECK_FRC_SCL_NUM(UnitNum);

    ovps_printf(OVPS_HIGH," %d %s >>>> UnitNum = %d \n", __LINE__, __func__, UnitNum);
    OVPSCL_MainCoeffRead(vpp_obj, UnitNum, hlut, vlut);
    return uiRetVal;
}
/***************************************************************************************
 * FUNCTION: Print scl coeff shadow used for writting
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
int OVPSCL_FRC_SCL_PrintSclCoeffShadow(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8])
{
    INT uiRetVal = MV_VPP_OK;
    unsigned int *ptr = NULL;
    int sclLayer;
    int coeffMsk = 0;
    int coeffBitWidth = 0;
    int hLutFailCount = 0;
    int vLutFailCount = 0;
    static int hLutFailIndex[VPP_FRC_SCL_NUM_OF_PHASES][VPP_FRC_SCL_NUM_OF_COEFF];
    static int vLutFailIndex[VPP_FRC_SCL_NUM_OF_PHASES][VPP_FRC_SCL_NUM_OF_COEFF];
    INT i = 0,j = 0;

    CHECK_FRC_SCL_NUM(UnitNum);

    if (UnitNum == VPP_FRC_SCL_OVP_Y) {
        sclLayer = 3;
        coeffMsk = MSK32SCL1D_YC_COEFF_CFG1_coeff0;
        coeffBitWidth = bSCL1D_YC_COEFF_CFG1_coeff0;
    } else {
        sclLayer = 4;
        coeffMsk = MSK32SCL1D_YC_COEFF_CFG1_coeff0;
        coeffBitWidth = bSCL1D_YC_COEFF_CFG1_coeff0;
    }
    ptr = vpp_obj->scl_coeffs[vpp_obj->hscl_coeff[UnitNum]];
    ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

    ovps_printf(OVPS_HIGH,"Shadow BCM:Default:Read HLUT\n");
    ovps_printf(OVPS_HIGH,"VPP_FRC_SCL_NUM_OF_PHASES = %d VPP_FRC_SCL_NUM_OF_COEFF =  %d \n", VPP_FRC_SCL_NUM_OF_PHASES, VPP_FRC_SCL_NUM_OF_COEFF);
    for (i=0; i < VPP_FRC_SCL_NUM_OF_PHASES; i++) {
        for (j = 0; j < VPP_FRC_SCL_NUM_OF_COEFF; j+=2) {
            if ((hlut[i][j] != (*ptr & coeffMsk)) || (hlut[i][j] != default_index_coeff_table[vpp_obj->hscl_coeff[UnitNum]][i][j])) {
                hLutFailIndex[hLutFailCount][0] = i;
                hLutFailIndex[hLutFailCount][1] = j;
                hLutFailCount++;
            }
            if ((hlut[i][j+1] != ((*ptr >> coeffBitWidth) & coeffMsk)) ||
                (hlut[i][j+1] != default_index_coeff_table[vpp_obj->hscl_coeff[UnitNum]][i][j+1])) {
                hLutFailIndex[hLutFailCount][0] = i;
                hLutFailIndex[hLutFailCount][1] = j+1;
                hLutFailCount++;
            }
            ovps_printf(OVPS_HIGH,"%4x:%4x:%4x %4x:%4x:%4x ",
                    *ptr & coeffMsk , default_index_coeff_table[vpp_obj->hscl_coeff[UnitNum]][i][j], hlut[i][j],
                    (*ptr >> coeffBitWidth) & coeffMsk, default_index_coeff_table[vpp_obj->hscl_coeff[UnitNum]][i][j+1], hlut[i][j+1]);
            ptr = ptr + 2;
        }
        ptr = ptr + 6;
        ovps_printf(OVPS_HIGH,"\n");
    }

    ptr = vpp_obj->scl_coeffs[vpp_obj->vscl_coeff[UnitNum]];
    ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

    ovps_printf(OVPS_HIGH,"Shadow BCM:Default:Read VLUT\n");
    ovps_printf(OVPS_HIGH,"VPP_FRC_SCL_NUM_OF_PHASES = %d VPP_FRC_SCL_NUM_OF_COEFF =  %d \n", VPP_FRC_SCL_NUM_OF_PHASES, VPP_FRC_SCL_NUM_OF_COEFF);
    for (i=0; i < VPP_FRC_SCL_NUM_OF_PHASES; i++) {
        for (j = 0; j < VPP_FRC_SCL_NUM_OF_COEFF; j+=2) {
            if ((vlut[i][j] != (*ptr & coeffMsk)) || (vlut[i][j] != default_index_coeff_table[vpp_obj->vscl_coeff[UnitNum]][i][j])) {
                vLutFailIndex[vLutFailCount][0] = i;
                vLutFailIndex[vLutFailCount][1] = j;
                vLutFailCount++;
            }
            if ((vlut[i][j+1] != ((*ptr >> coeffBitWidth) & coeffMsk)) ||
                (vlut[i][j+1] != default_index_coeff_table[vpp_obj->vscl_coeff[UnitNum]][i][j+1])) {
                vLutFailIndex[vLutFailCount][0] = i;
                vLutFailIndex[vLutFailCount][1] = j+1;
                vLutFailCount++;
            }
            ovps_printf(OVPS_HIGH,"%4x:%4x:%4x %4x:%4x:%4x ",
                    *ptr & coeffMsk, default_index_coeff_table[vpp_obj->vscl_coeff[UnitNum]][i][j], vlut[i][j],
                    (*ptr >> coeffBitWidth) & coeffMsk, default_index_coeff_table[vpp_obj->vscl_coeff[UnitNum]][i][j+1], vlut[i][j+1]);
            ptr = ptr + 2;
        }
        ptr = ptr + 6;
        ovps_printf(OVPS_HIGH,"\n");
    }

    ovps_printf(OVPS_HIGH,"\n");
    if ((0 != hLutFailCount) || (0 != vLutFailCount)) {
        ovps_printf(OVPS_ERR, " hLutFailCount = %d \n", hLutFailCount);
        for (j=0; j<hLutFailCount; j++) {
            ovps_printf(OVPS_ERR, "(%d, %d) ",hLutFailIndex[j][0], hLutFailIndex[j][1]);
            if (0 == j%20) {
                ovps_printf(OVPS_ERR, "\n");
            }
        }
        ovps_printf(OVPS_ERR, "\n");
        ovps_printf(OVPS_ERR, " vLutFailCount = %d \n", vLutFailCount);
        for (j=0; j<vLutFailCount; j++) {
            ovps_printf(OVPS_ERR, "(%d, %d) ",vLutFailIndex[j][0], vLutFailIndex[j][1]);
            if (0 == j%20) {
                ovps_printf(OVPS_ERR, "\n");
            }
        }
        ovps_printf(OVPS_ERR, "\n");
        ovps_printf(OVPS_ERR, "LUT programming FAILED\n");
    } else {
        ovps_printf(OVPS_HIGH, "LUT programming PASS\n");
    }

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Load the required Coeff from pre calculated table
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -poINTer to I&O resolution
 *               pSclCtrl - poINTer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetSclCoeff(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    int tempHorSclCoeffMode,tempVerSclCoeffMode;
    UINT32 uiRegAddr;
    INT uiRetVal = MV_VPP_OK;
    INT8 hTaps = -1, vTaps = -1;
    INT8 sclLayer;
    T32ADPSCL_COEFF_CFG0 stAdpSclCoeffCfg0;
    UINT32 *ptr = NULL;
#ifdef VPP_TIME_PROFILE
    int low, high;
#endif

    CHECK_FRC_SCL_NUM(UnitNum);

    //All scalar calculation should be based on cropped resolution
    if (UnitNum == VPP_FRC_SCL_OVP_Y) {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
    } else {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
    }

    if (NULL != pIORes) {
        OVPSCL_MainSclCalTaps(vpp_obj, UnitNum, pIORes, &hTaps, &vTaps);
    }

    //Revert back the modification done to 'pIORes->IHRes'
    pIORes->IHRes = pSclCtrl->uncrop_IHRes;

    if (!(pSclCtrl->ForceSel & MODE_COEFF_SEL_FORCE_H)) {
        if (NULL != pIORes) {
            switch (hTaps) {
                case 4:
                    tempHorSclCoeffMode = COEFF_H_MAIN_ADP_HLUT_CS;
                    break;
                case 5:
                    tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_BLK5;
                    break;
                case 8:
                    tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_PH8;
                    break;
                default:
                    tempHorSclCoeffMode = -1;
                    ovps_printf(OVPS_ERR, "%d %s Unsupported HLut table \n", __LINE__, __func__);
                    return MV_VPP_EUNSUPPORT;
            }
        } else {
            tempHorSclCoeffMode = -1;
        }
    } else {
        // force the user selection.
        tempHorSclCoeffMode = pSclCtrl->HSclMode;
        hTaps = 0;//FIXME
        ovps_printf(OVPS_ERR, "%d %s Unsupported now. Needs prg updates \n", __LINE__, __func__);
        return MV_VPP_EUNSUPPORT;
    }

    if (!(pSclCtrl->ForceSel & MODE_COEFF_SEL_FORCE_V)) {
        if (NULL != pIORes) {
            switch (vTaps) {
                case 1:
                    tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_DBG1;
                    break;
                case 2:
                    tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_2TAP;
                    break;
                case 3:
                    tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL;
                    break;
                case 4:
                    tempVerSclCoeffMode = COEFF_V_MAIN_ADP_HLUT_BS;
                    break;
                case 5:
                    tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_BLK5;
                    break;
                case 6:
                    tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_6TAP;
                    break;
                default:
                    tempVerSclCoeffMode = -1;
                    ovps_printf(OVPS_ERR, "%d %s Unsupported VLut table \n", __LINE__, __func__);
                    return MV_VPP_EUNSUPPORT;
            }
        } else {
            tempVerSclCoeffMode = -1;
        }
    } else {
        // force the user selection.
        tempVerSclCoeffMode = pSclCtrl->VSclMode;
        vTaps = 0;//FIXME
        ovps_printf(OVPS_ERR, "%d %s Unsupported now. Needs prg updates \n", __LINE__, __func__);
        return MV_VPP_EUNSUPPORT;
    }

    /* Calculating sclLayer of scl_coeffs table */
    if (VPP_FRC_SCL_OVP_Y == UnitNum)
        sclLayer = 3;
    else
        sclLayer = 4;

#ifdef VPP_TIME_PROFILE
    ResetCycleCounter();
#endif

    if ((-1 != tempHorSclCoeffMode) && (vpp_obj->hscl_coeff[UnitNum]!=tempHorSclCoeffMode ||  vpp_obj->hscl_coeff_update[UnitNum]==1)) {

        vpp_obj->hscl_coeff[UnitNum] = tempHorSclCoeffMode;
        vpp_obj->hscl_coeff_update[UnitNum] = 0;
        ptr = vpp_obj->scl_coeffs[tempHorSclCoeffMode];
        ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;
        OVPS_BCMBUF_WriteBlock(vpp_obj->ovp_obj.pOvpSclBcmBuf, ptr, VPP_FRC_COEFFTAB_BCMBUF_SIZE);
    }

    if ((-1 != tempVerSclCoeffMode) && (vpp_obj->vscl_coeff[UnitNum]!=tempVerSclCoeffMode || vpp_obj->vscl_coeff_update[UnitNum]==1)) {

        vpp_obj->vscl_coeff[UnitNum] = tempVerSclCoeffMode;
        vpp_obj->vscl_coeff_update[UnitNum] = 0;
        ptr = vpp_obj->scl_coeffs[tempVerSclCoeffMode];
        ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;
        OVPS_BCMBUF_WriteBlock(vpp_obj->ovp_obj.pOvpSclBcmBuf, ptr, VPP_FRC_COEFFTAB_BCMBUF_SIZE);
    }

    stAdpSclCoeffCfg0.uCFG0_htap        = hTaps;
    stAdpSclCoeffCfg0.uCFG0_vtap        = vTaps;
    stAdpSclCoeffCfg0.uCFG0_coeff_index = 0;
    stAdpSclCoeffCfg0.uCFG0_coeff_hvsel = 0;
    stAdpSclCoeffCfg0.uCFG0_coeffread   = 0;
    stAdpSclCoeffCfg0.uCFG0_coeffload   = 0;
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf,
                uiRegAddr + RA_ADPSCL_COEFF_CFG0,stAdpSclCoeffCfg0.u32));
#ifdef VPP_TIME_PROFILE
    ReadCycleCounter(&low, &high);
    if (low > gTimeSclBCMWrite)
        gTimeSclBCMWrite = low;
#endif

    return MV_VPP_OK;
}

#ifdef EN_VPP_DYNAMIC_CLKGATE
/***************************************************************************************
 * FUNCTION: Enable/Diable Scl clock gating
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               enable - enable/disable clock gate
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_EnClkGate(VPP_OBJ *vpp_obj, INT UnitNum, INT enable)
{
    return MV_VPP_OK;
}
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */
/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -poINTer to I&O resolution
 *               pSclCtrl - poINTer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetSclCtrlParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    INT uiRetVal = MV_VPP_OK;

    //Take backup of the uncropped resolution - so that IHRes can be reverted at later function calls also
    pSclCtrl->uncrop_IHRes = pIORes->IHRes;
    //All scalar calculation should be based on cropped resolution
    pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

    CHECK_FRC_SCL_NUM(UnitNum);
#ifdef EN_VPP_DYNAMIC_CLKGATE
    if (IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE)) {
        //Enable scalar or don't clock gate scalar, when only cropping required in 1:1 cases
        if ((pIORes->IVRes == pIORes->OVRes) && (pSclCtrl->uncrop_IHRes == pIORes->OHRes)) {
            uiRetVal = OVPSCL_FRC_SCL_EnClkGate(vpp_obj, UnitNum, 1);
            if (VPP_FRC_SCL_MAIN != UnitNum) {
                return uiRetVal;/* Stop prog as SCL clock gated except for MAIN SCALAR */
            }
        } else {
            CHECK_VPP_SCL_RETVAL(OVPSCL_FRC_SCL_EnClkGate(vpp_obj, UnitNum, 0));
        }
    }
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */

    switch (UnitNum) {
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            CHECK_VPP_SCL_RETVAL(OVPSCL_SetMainSclparams(vpp_obj, UnitNum, pIORes, pSclCtrl));
            break;
        default:
            return MV_VPP_EBADPARAM;
            break;
    }

    //Revert back the modification done to 'pIORes->IHRes'
    pIORes->IHRes = pSclCtrl->uncrop_IHRes;

    /*load HV filter coefficiets*/
    if (pSclCtrl->DynamicLoad) {
        OVPSCL_FRC_SCL_SetSclCoeff(vpp_obj, UnitNum, pIORes, pSclCtrl);
    }

    return uiRetVal;
}

INT OVPSCL_SetMainSclScanParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    UINT32 uiRegAddr;
    INT8 sel_1D_adp;
    INT8 vTaps, hTaps;
    T32ADPSCL_CFG0 stSclCfg0;
    T32ADPSCL_CFG3 stSclCfg3;
    T32ADPSCL_CFG7 stSclCfg7;
    INT uiRetVal = MV_VPP_OK;
    long vinit_ph_cmodel = 0, vtap_offset_cmodel = 0;
    long vinit_ph_even = 0, vtap_offset_even = 0;
    long vinit_ph_odd = 0, vtap_offset_odd = 0;
    RESOLUTION_INFO  *pResinfo;

    //All scalar calculation should be based on cropped resolution
    pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);
    stSclCfg0.u32 = vpp_obj->uiSclCfg0[UnitNum];
    stSclCfg3.u32 = vpp_obj->uiSclCfg3[UnitNum];
    stSclCfg7.u32 = vpp_obj->uiSclCfg7[UnitNum];

    uiRegAddr = INTERNAL_OVPS_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
    if (0 == uiRegAddr) {
        return MV_VPP_EBADPARAM;
    }
    OVPSCL_MainSclAdp1dType(vpp_obj, pIORes, &sel_1D_adp);
    OVPSCL_MainSclCalTaps(vpp_obj, UnitNum, pIORes, &hTaps, &vTaps);

    if (0 == sel_1D_adp) {
        stSclCfg0.uCFG0_even = 0x80;
        stSclCfg0.uCFG0_odd = 0x80;
        stSclCfg0.uCFG0_hinitph = 0x80;
    } else {
        stSclCfg0.uCFG0_even = (vTaps % 2)<<7;
        stSclCfg0.uCFG0_odd = (vTaps % 2)<<7;
        stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
    }
    stSclCfg7.uCFG7_vtap_offset = 0;

    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[CPCB_1]);

    if ( SCAN_PROGRESSIVE != pResinfo->scan) { // interlaced output
        vinit_ph_even = (stSclCfg0.uCFG0_even<<12);
        vtap_offset_even = stSclCfg7.uCFG7_vtap_offset;
        vinit_ph_odd = vinit_ph_even + stSclCfg3.uCFG3_vratio/2;
        vtap_offset_odd = vtap_offset_even + ((vinit_ph_odd & 0xff00000) >> 20);

        if (vpp_obj->disptg_intl_count%2 == 0) {
            vinit_ph_cmodel = vinit_ph_even & 0xff000;
            vtap_offset_cmodel = vtap_offset_even % 16;
        } else { // odd/bottom_frame
            vinit_ph_cmodel = (vinit_ph_odd % 0x100000) & 0xff000;
            vtap_offset_cmodel = vtap_offset_odd % 16;
        }
        stSclCfg0.uCFG0_even = vinit_ph_cmodel>>12;
        stSclCfg0.uCFG0_odd = vinit_ph_cmodel>>12;
        stSclCfg7.uCFG7_vtap_offset = (vtap_offset_cmodel<<4) | vtap_offset_cmodel;
    }

    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG0, stSclCfg0.u32));
    vpp_obj->uiSclCfg0[UnitNum] = stSclCfg0.u32;

    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG7, stSclCfg7.u32));
    vpp_obj->uiSclCfg7[UnitNum] = stSclCfg7.u32;

    //Revert back the modification done to 'pIORes->IHRes'
    pIORes->IHRes = pSclCtrl->uncrop_IHRes;

    return uiRetVal;
}


INT OVPSCL_SetGfxSclScanParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    UINT32 uiRegAddr;
    INT8 vTaps, hTaps;
    T32SCL_CFG0 stSclCfg0;
    T32SCL_CFG3 stSclCfg3;
    T32SCL_CFG8 stSclCfg8;
    INT uiRetVal = MV_VPP_OK;
    long vinit_ph_cmodel = 0, vtap_offset_cmodel = 0;
    long vinit_ph_even = 0, vtap_offset_even = 0;
    long vinit_ph_odd = 0, vtap_offset_odd = 0;
    RESOLUTION_INFO  *pResinfo;

    stSclCfg0.u32 = vpp_obj->uiSclCfg0[UnitNum];
    stSclCfg3.u32 = vpp_obj->uiSclCfg3[UnitNum];
    stSclCfg8.u32 = vpp_obj->uiSclCfg8[UnitNum];

    uiRegAddr = INTERNAL_OVPS_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
    if (0 == uiRegAddr) {
        return MV_VPP_EBADPARAM;
    }
    //All scalar calculation should be based on cropped resolution
    pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

    OVPSCL_GfxSclCalTaps(vpp_obj, pIORes, &hTaps, &vTaps);

    stSclCfg0.uCFG0_vinitph = (vTaps % 2)<<7;
    stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
    stSclCfg8.uCFG8_vtap_offset = 0;

    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[CPCB_1]);

    if ( SCAN_PROGRESSIVE != pResinfo->scan) { // interlaced output
            vinit_ph_even = (stSclCfg0.uCFG0_vinitph<<12);
            vtap_offset_even = stSclCfg8.uCFG8_vtap_offset;
            vinit_ph_odd = vinit_ph_even + stSclCfg3.uCFG3_vratio/2;
            vtap_offset_odd = vtap_offset_even + ((vinit_ph_odd & 0xff00000) >> 20);

            if (vpp_obj->disptg_intl_count%2 == 0) {
                vinit_ph_cmodel = vinit_ph_even & 0xff000;
                vtap_offset_cmodel = vtap_offset_even % 16;
            } else { // odd/bottom_frame
                vinit_ph_cmodel = (vinit_ph_odd % 0x100000) & 0xff000;
                vtap_offset_cmodel = vtap_offset_odd % 16;
            }
            stSclCfg0.uCFG0_vinitph = vinit_ph_cmodel>>12;
            stSclCfg8.uCFG8_vtap_offset = (vtap_offset_cmodel<<4) | vtap_offset_cmodel;
    }
    if (vpp_obj->uiSclCfg0[UnitNum] != stSclCfg0.u32) {
        CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL_CFG0, stSclCfg0.u32));
        vpp_obj->uiSclCfg0[UnitNum] = stSclCfg0.u32;
    }
    if (vpp_obj->uiSclCfg8[UnitNum] != stSclCfg8.u32) {
        CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL_CFG8, stSclCfg8.u32));
        vpp_obj->uiSclCfg8[UnitNum] = stSclCfg8.u32;
    }

    //Revert back the modification done to 'pIORes->IHRes'
    pIORes->IHRes = pSclCtrl->uncrop_IHRes;

    return uiRetVal;
}



/***************************************************************************************
 * FUNCTION: Load the scalar FRC parameters
 * PARAMS:
 *           vpp_obj - vpp object
 *           UnitNum - Unit num to identify the plain
 *           pFrcRes - FRC input and output resolutions
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetFrcParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pFrcRes)
{
    UINT32 uiRegAddr;
    VPP_FRC_CTRL0_0 stFrcCtrl0_0;
    VPP_FRC_CTRL0_1 stFrcCtrl0_1;
    VPP_FRC_CTRL1_2 stFrcCtrl1_2;
    T32ADPSCL_FRC_CFG0 stSclFrcCfg0;
    T32ADPSCL_FRC_CFG1 stSclFrcCfg1;
    T32ADPSCL_FRC_CFG2 stSclFrcCfg2;
    T32ADPSCL_FRC_CFG3 stSclFrcCfg3;
    INT uiRetVal = MV_VPP_OK;
    UNSG16 temp_uCFG1_dly_de_lrst = 0;
    INT8 enDeintSmallUpScale = 0;

    OVPSCL_SclConfigForDeint(vpp_obj, UnitNum, pFrcRes, &enDeintSmallUpScale);
    CHECK_FRC_SCL_NUM(UnitNum);

    stSclFrcCfg1.u32 = 0;

    /*get register start address of the FRC_SCL unit */
    uiRegAddr = vpp_obj->base_addr;
    if (UnitNum == VPP_FRC_SCL_OVP_Y) {
        uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC;
    } else {
        uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC;
    }

    stFrcCtrl0_0.uchFrcMode = 3;
    stFrcCtrl0_0.uchMosiacMode = 0;
    stFrcCtrl0_0.uchBitMode_16_24 = 1;
    stFrcCtrl0_0.uchSpMode = 0;

    stFrcCtrl0_1.uchLoadRead = 0;
    stFrcCtrl0_1.uchLoadWrite = 0;
    stFrcCtrl0_1.uchReadCtrl = 0;
    stFrcCtrl0_1.uchWriteCtrl = 0;
    stFrcCtrl0_1.uchFreeze = 0;
    stFrcCtrl0_1.uchInvFeFld = 0;
    stFrcCtrl0_1.uchInvBeFld = 0;
    stFrcCtrl0_1.uchBeFrstResetEn = 0;

    stSclFrcCfg0.uCFG0_sclclk_ctrl = 0x80;
    stSclFrcCfg0.uCFG0_ctrl0 = stFrcCtrl0_0.uch8;
    stSclFrcCfg0.uCFG0_ctrl1 = stFrcCtrl0_1.uch8;
    stSclFrcCfg0.uCFG0_sclclk_ctrl1 = 0x80;

    stFrcCtrl1_2.uchTreaLessReadCtrl = 0;
    stFrcCtrl1_2.uchTreaLessWriteCtrl = 0;
    stSclFrcCfg1.uCFG1_ctrl2 = stFrcCtrl1_2.uch8;
    stSclFrcCfg1.uCFG1_dly_frst_de = 0x14;
    /* For downscale >= 5x, add more delay to avoid video roll on right side */
    if(pFrcRes->OHRes < (pFrcRes->IHRes/5)) {
        temp_uCFG1_dly_de_lrst = 0x100;
    } else {
        temp_uCFG1_dly_de_lrst = 0x14;
    }

    if (pFrcRes->OHRes > pFrcRes->IHRes)
        stSclFrcCfg1.uCFG1_dly_de_lrst = temp_uCFG1_dly_de_lrst;
    else if (1 == vpp_obj->scl_ctrl[UnitNum].NLEn)
        stSclFrcCfg1.uCFG1_dly_de_lrst = 0x200;
    else
        stSclFrcCfg1.uCFG1_dly_de_lrst = temp_uCFG1_dly_de_lrst;

    stSclFrcCfg2.uCFG2_bevres = pFrcRes->IVRes;
    stSclFrcCfg2.uCFG2_fevres = pFrcRes->OVRes;
    stSclFrcCfg2.uCFG2_dly_lrst_de = 0x14;//250;

    stSclFrcCfg3.uCFG3_behres = pFrcRes->IHRes;
    stSclFrcCfg3.uCFG3_clnt_ctrl = 3;
    stSclFrcCfg3.uCFG3_ctrl = 0;
    stSclFrcCfg3.uCFG3_rff_ctrl = 0;

    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_FRC_CFG0, stSclFrcCfg0.u32));
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_FRC_CFG1, stSclFrcCfg1.u32));
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_FRC_CFG2, stSclFrcCfg2.u32));
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_FRC_CFG3, stSclFrcCfg3.u32));

    vpp_obj->uiSclFrcCfg1[UnitNum] = stSclFrcCfg1.u32;
    vpp_obj->uiSclFrcCfg0[UnitNum] = stSclFrcCfg0.u32;
    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: set delay between DE  to line reset.
 * PARAMS: UnitNum - unit number
 *               DeLrstDelay -delay value
 * RETURN:  1 - succeed
 *                others - failed to load  values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetDeLrstDelay(VPP_OBJ *vpp_obj, INT UnitNum, UINT32 DeLrstDelay)
{
    UINT32 uiRegAddr;
    UINT32 uiRetVal = MV_VPP_OK;
    T32ADPSCL_FRC_CFG1 stSclFrcCfg1;

    CHECK_FRC_SCL_NUM(UnitNum);

    /*get register start address */
    uiRegAddr = vpp_obj->base_addr;
    switch (UnitNum) {
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG1;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG1;
            break;
        default:
            return MV_VPP_EBADPARAM;
            break;
    }

    stSclFrcCfg1.u32 = vpp_obj->uiSclFrcCfg1[UnitNum];

    stSclFrcCfg1.uCFG1_dly_de_lrst = DeLrstDelay;

    /*set it to BCM buffer*/
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr, stSclFrcCfg1.u32));

    vpp_obj->uiSclFrcCfg1[UnitNum] = stSclFrcCfg1.u32;

    return  uiRetVal;
}

/***************************************************************************************
 * FUNCTION: set clock puncture value, control the rate of pumping data out from SDRAM
 * PARAMS: UnitNum - unit number
 *               ClkVal -clock value(should be less than 128)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetSclClk(VPP_OBJ *vpp_obj, INT UnitNum, UINT32 ClkVal)
{
    UINT32 uiRegAddr = 0;
    UINT32 uiRetVal = MV_VPP_OK;
    T32ADPSCL_FRC_CFG0 stSclFrcCfg0;

    CHECK_FRC_SCL_NUM(UnitNum);

    if (ClkVal > 128)
        return MV_VPP_EBADPARAM;

    /*get register start address */
    uiRegAddr = vpp_obj->base_addr;
    switch (UnitNum) {
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG0;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG0;
            break;
        default:
            return MV_VPP_EBADPARAM;
            break;
    }

    stSclFrcCfg0.u32 = vpp_obj->uiSclFrcCfg0[UnitNum];

    stSclFrcCfg0.uCFG0_sclclk_ctrl = ClkVal;
    /*set it to BCM buffer*/
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr, stSclFrcCfg0.u32));

    vpp_obj->uiSclFrcCfg0[UnitNum] = stSclFrcCfg0.u32;

    return  uiRetVal;
}

/***************************************************************************************
 * FUNCTION: set phase DDA counter ( sub-pixel shifting)
 * PARAMS: scalerId - scaler index
 *               hv - horizontal or vertical
 *               counter - DDA counter
 * RETURN:  1 - succeed
 *                others - failed to set phase DDA counter
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetPhaseDda(VPP_OBJ *vpp_obj, VPP_FRC_SCL_NUM scalerId, INT hv, INT counter )
{
    UINT32 uiRegAddr = 0;
    UINT32 uiRetVal = MV_VPP_OK;
    T32ADPSCL_CFG0 stAdpSclCfg0;
    T32SCL_CFG0 stSclCfg0;

    /*get register start address */
    uiRegAddr = INTERNAL_OVPS_FRC_SCL_GetRegStartAddr(vpp_obj, scalerId);
    if (0 == uiRegAddr) {
        return MV_VPP_EBADPARAM;
    }
    switch (scalerId) {
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            stAdpSclCfg0.u32 = vpp_obj->uiSclCfg0[scalerId];
            if (hv == VPP_FRC_SCL_PHASE_HORIZONTAL)
                stAdpSclCfg0.uCFG0_hinitph = counter;
            else if (hv == VPP_FRC_SCL_PHASE_VERTICAL) {
                stAdpSclCfg0.uCFG0_even = counter;
                stAdpSclCfg0.uCFG0_odd  = counter;
            } else
                return (MV_VPP_EBADPARAM);

            CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG0, stAdpSclCfg0.u32));
            vpp_obj->uiSclCfg0[scalerId] = stAdpSclCfg0.u32;
            break;
        case VPP_FRC_SCL_PIP:
        case VPP_FRC_SCL_OSD:
            stSclCfg0.u32 = vpp_obj->uiSclCfg0[scalerId];
            if (hv == VPP_FRC_SCL_PHASE_HORIZONTAL)
                stSclCfg0.uCFG0_hinitph = counter;
            else if (hv == VPP_FRC_SCL_PHASE_VERTICAL)
                stSclCfg0.uCFG0_vinitph = counter;
            else
                return (MV_VPP_EBADPARAM);

            CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL_CFG0, stSclCfg0.u32));
            vpp_obj->uiSclCfg0[scalerId] = stSclCfg0.u32;
            break;
        default:
            return MV_VPP_EBADPARAM;
            break;
}

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: control Hscaler chopping
 * PARAMS: UnitNum -
 *         Value -
 * RETURN:  1 - succeed
 *                others - failed to load register to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_ChopCtrl(VPP_OBJ *vpp_obj, INT UnitNum, INT Value)
{
    UINT32 uiRegAddr = 0;
    UINT32 uiRetVal = MV_VPP_OK;
    T32ADPSCL_CFG15 stSclCfg15;

    /*get register start address */
    uiRegAddr = INTERNAL_OVPS_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
    if (0 == uiRegAddr) {
        return MV_VPP_EBADPARAM;
    }
    switch (UnitNum) {
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            stSclCfg15.u32 = vpp_obj->uiSclCfg15[UnitNum];
            stSclCfg15.uCFG15_crop  = Value;
            CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG15 , stSclCfg15.u32));
            vpp_obj->uiSclCfg15[UnitNum] = stSclCfg15.u32;
            break;
        default:
        return MV_VPP_EBADPARAM;
            break;
    }

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: set OSD formatter data format and CSC mode
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               CscMode -CSC mode (passthrough, 601 or 709)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_FRC_SCL_SetOsdFmt(VPP_OBJ *vpp_obj, INT UnitNum, INT CscMode)
{
    return MV_VPP_OK;
}



static int OVPSCL_Generate_Scl_MainCoefficiena(VPP_OBJ *vpp_obj, int UnitNum, unsigned int **pptr, int hv, UINT16 (* coeffs)[VPP_FRC_SCL_NUM_OF_COEFF])
{
    UINT32 uiRegAddr = 0;
    INT16 i;
    INT uiRetVal = MV_VPP_OK;
    unsigned int *ptr = *pptr;

    T32ADPSCL_COEFF_CFG0 stSclCoeffCfg0;
    T32ADPSCL_COEFF_CFG1 stSclCoeffCfg1;
    T32ADPSCL_COEFF_CFG2 stSclCoeffCfg2;
    T32ADPSCL_COEFF_CFG3 stSclCoeffCfg3;
    T32ADPSCL_COEFF_CFG4 stSclCoeffCfg4;

    switch (UnitNum) {
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
        default:
            return MV_VPP_EBADPARAM;
    }

    /*Coeff table loading*/
    for (i=0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++) {
        stSclCoeffCfg1.uCFG1_coeff0 = coeffs[i][0];
        stSclCoeffCfg1.uCFG1_coeff1 = coeffs[i][1];
        stSclCoeffCfg2.uCFG2_coeff2 = coeffs[i][2];
        stSclCoeffCfg2.uCFG2_coeff3 = coeffs[i][3];
        stSclCoeffCfg3.uCFG3_coeff4 = coeffs[i][4];
        stSclCoeffCfg3.uCFG3_coeff5 = coeffs[i][5];
        stSclCoeffCfg4.uCFG4_coeff6 = coeffs[i][6];
        stSclCoeffCfg4.uCFG4_coeff7 = coeffs[i][7];

        *ptr++ = stSclCoeffCfg1.u32;
        *ptr++ = uiRegAddr + RA_ADPSCL_COEFF_CFG1;
        *ptr++ = stSclCoeffCfg2.u32;
        *ptr++ = uiRegAddr + RA_ADPSCL_COEFF_CFG2;
        *ptr++ = stSclCoeffCfg3.u32;
        *ptr++ = uiRegAddr + RA_ADPSCL_COEFF_CFG3;
        *ptr++ = stSclCoeffCfg4.u32;
        *ptr++ = uiRegAddr + RA_ADPSCL_COEFF_CFG4;

        stSclCoeffCfg0.uCFG0_htap        = 0;//TODO
        stSclCoeffCfg0.uCFG0_vtap        = 0;//TODO
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        stSclCoeffCfg0.uCFG0_coeff_hvsel = (hv ^ 1); /* Invert it as the vpp_api.h and vpp.h has different */
        stSclCoeffCfg0.uCFG0_coeffread   = 0;

        stSclCoeffCfg0.uCFG0_coeffload   = 0;
        *ptr++ = stSclCoeffCfg0.u32;
        *ptr++ = uiRegAddr + RA_ADPSCL_COEFF_CFG0;
        stSclCoeffCfg0.uCFG0_coeffload   = 1;
        *ptr++ = stSclCoeffCfg0.u32;
        *ptr++ = uiRegAddr + RA_ADPSCL_COEFF_CFG0;
        stSclCoeffCfg0.uCFG0_coeffload   = 0;
        *ptr++ = stSclCoeffCfg0.u32;
        *ptr++ = uiRegAddr + RA_ADPSCL_COEFF_CFG0;
    }

    *pptr = ptr;

    return uiRetVal;
}
int OVPSCL_Generate_Scl_GfxCoefficiena(VPP_OBJ *vpp_obj, int UnitNum, unsigned int **pptr, int hv, UINT16 (* coeffs)[VPP_FRC_SCL_NUM_OF_COEFF])
{
    UINT32 uiRegAddr;
    INT16 i;
    INT uiRetVal = MV_VPP_OK;
    unsigned int *ptr = *pptr;

    T32COEFF_CFG0 stSclCoeffCfg0;
    T32COEFF_CFG1 stSclCoeffCfg1;
    T32COEFF_CFG2 stSclCoeffCfg2;
    T32COEFF_CFG3 stSclCoeffCfg3;
    T32COEFF_CFG4 stSclCoeffCfg4;

    if ((VPP_FRC_SCL_PIP != UnitNum) && (VPP_FRC_SCL_OSD != UnitNum))
        return MV_VPP_EBADPARAM;

    /*get register start address */
    uiRegAddr = vpp_obj->base_addr_gfx_vop;
    switch (UnitNum) {
        case VPP_FRC_SCL_PIP:
            uiRegAddr += RA_VPP_GFX_VOP_COEFF_GFX0;
            break;
        case VPP_FRC_SCL_OSD:
            uiRegAddr += RA_VPP_GFX_VOP_COEFF_GFX1;
            break;
        default:
            return 0;
            break;
    }

    /*Coeff table loading*/
    for (i=0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++) {
        stSclCoeffCfg1.uCFG1_coeff0 = coeffs[i][0];
        stSclCoeffCfg1.uCFG1_coeff1 = coeffs[i][1];
        stSclCoeffCfg2.uCFG2_coeff2 = coeffs[i][2];
        stSclCoeffCfg2.uCFG2_coeff3 = coeffs[i][3];
        stSclCoeffCfg3.uCFG3_coeff4 = coeffs[i][4];
        stSclCoeffCfg3.uCFG3_coeff5 = coeffs[i][5];
        stSclCoeffCfg4.uCFG4_coeff6 = coeffs[i][6];
        stSclCoeffCfg4.uCFG4_coeff7 = coeffs[i][7];

        *ptr++ = stSclCoeffCfg1.u32;
        *ptr++ = uiRegAddr + RA_COEFF_CFG1;
        *ptr++ = stSclCoeffCfg2.u32;
        *ptr++ = uiRegAddr + RA_COEFF_CFG2;
        *ptr++ = stSclCoeffCfg3.u32;
        *ptr++ = uiRegAddr + RA_COEFF_CFG3;
        *ptr++ = stSclCoeffCfg4.u32;
        *ptr++ = uiRegAddr + RA_COEFF_CFG4;

        stSclCoeffCfg0.uCFG0_htaps        = 0;//TODO
        stSclCoeffCfg0.uCFG0_vtaps        = 0;//TODO
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        stSclCoeffCfg0.uCFG0_coeff_hvsel = (hv ^ 1);

        stSclCoeffCfg0.uCFG0_coeffload   = 0;
        *ptr++ = stSclCoeffCfg0.u32;
        *ptr++ = uiRegAddr + RA_COEFF_CFG0;
        stSclCoeffCfg0.uCFG0_coeffload   = 1;
        *ptr++ = stSclCoeffCfg0.u32;
        *ptr++ = uiRegAddr + RA_COEFF_CFG0;
        stSclCoeffCfg0.uCFG0_coeffload   = 0;
        *ptr++ = stSclCoeffCfg0.u32;
        *ptr++ = uiRegAddr + RA_COEFF_CFG0;
    }

    *pptr = ptr;
    return uiRetVal;
}

static INT OVPSCL_Generate_Scl_Coefficiena(VPP_OBJ *vpp_obj, int UnitNum, unsigned int **pptr, int hv, UINT16 (* coeffs)[VPP_FRC_SCL_NUM_OF_COEFF])
{
    switch (UnitNum) {
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            OVPSCL_Generate_Scl_MainCoefficiena(vpp_obj, UnitNum, pptr, hv, coeffs);
            break;
        default:
            return (MV_VPP_EBADPARAM);
    }

    return (MV_VPP_OK);
}

/***************************************************************************************
 * FUNCTION: program SCL scaler coeffs with customized coeffs table
 * PARAMS:
 *            coeffID - scalar coeffs mode
 *              hv     - indicating it is for horizontal(0) or vertical(1) scaler
 *              UnitNum    - indicating which plane the coeff is for
 *        coeffs    - customized coeffs talbe's pointer;
 *                      Note: if it is NULL, it means recover default coeffs table
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT OVPSCL_Customize_Coefficients(VPP_OBJ *vpp_obj, int coeffID, int hv, INT UnitNum, BOOL bCust)
{
    UINT16 (* coeffs)[8];
    unsigned int *ptr;
    int sclLayer = -1;

    /* Calculating sclLayer of scl_coeffs table */
    if (UnitNum == VPP_FRC_SCL_OVP_Y)
        sclLayer = 3;
    else if (UnitNum == VPP_FRC_SCL_OVP_UV)
        sclLayer = 4;
    else
        ASSERT(0);

    if (coeffID < FIRST_SCL_COEFF || coeffID >= COEFF_MAX)
        return MV_VPP_EBADPARAM;

    ptr = vpp_obj->scl_coeffs[coeffID];
    ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

    if (bCust == TRUE) {
        VPPAPI_DBG_INFO(" sclLayer=%d, coeffID=%d,addr=0x%08X \n", sclLayer, coeffID,vpp_obj->cust_scl_coeffs[sclLayer][coeffID] );
        if (vpp_obj->cust_sc_bPrepared[sclLayer][coeffID]) {
            coeffs = (UINT16 (*)[VPP_FRC_SCL_NUM_OF_COEFF]) vpp_obj->cust_scl_coeffs[sclLayer][coeffID];
            vpp_obj->cust_sc_bLoaded[sclLayer][coeffID] = TRUE;
            VPPAPI_DBG_INFO("sclLayer=%d, coeffID=%d, value=%d\n", sclLayer, coeffID, vpp_obj->cust_sc_bLoaded[sclLayer][coeffID]);
        } else
            return MV_VPP_EUNCONFIG;
    } else {
        coeffs=(UINT16 (*)[VPP_FRC_SCL_NUM_OF_COEFF])default_index_coeff_table[coeffID];
        vpp_obj->cust_sc_bLoaded[sclLayer][coeffID] = FALSE;
    }
    OVPSCL_Generate_Scl_Coefficiena(vpp_obj, UnitNum, &ptr, hv, coeffs);
    VPP_SHM_CleanCache(vpp_obj->scl_coeffs_handle[coeffID], (VPP_FRC_COEFFTAB_BCMBUF_SIZE * sclLayer), VPP_FRC_COEFFTAB_BCMBUF_SIZE);

    return MV_VPP_OK;
}

INT OVPSCL_FRC_SCL_GenerateMainAdpCoeffTabHV(VPP_OBJ *vpp_obj, INT UnitNum, INT16 coeffID, INT16 hvSel, INT format, UINT16 (* lut)[4])
{
    INT16 cf1[32][4],cf2;
    INT16 i,j;
    INT uiRetVal = MV_VPP_OK;

    /* Convert INTo sign magnitude */
    for (i = 0; i < 32; i++) {
        for(j = 0; j < 4; j++) {
            cf2 = (lut[i][3-j])&0x800;
            cf1[i][j] = (lut[i][3-j])&0x7ff;
            cf1[i][j] = (cf2 ? -cf1[i][j] : cf1[i][j])&0x7ff;
            cf1[i][j] = ((cf2 | cf1[i][j])&0xfff);
        }
    }
    /*Coeff table loading*/
    for (i = 0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++) {
        default_index_coeff_table[coeffID][i][0] = cf1[i][3];
        default_index_coeff_table[coeffID][i][1] = cf1[i][2];
        default_index_coeff_table[coeffID][i][2] = cf1[i][1];
        default_index_coeff_table[coeffID][i][3] = cf1[i][0];
        default_index_coeff_table[coeffID][i][4] = 0;
        default_index_coeff_table[coeffID][i][5] = 0;
        default_index_coeff_table[coeffID][i][6] = 0;
        default_index_coeff_table[coeffID][i][7] = 0;
    }
    return uiRetVal;
}

INT OVPSCL_FRC_SCL_GenerateMain1DCoeffTabH(VPP_OBJ *vpp_obj, INT UnitNum, INT16 coeffID, INT16 hvSel, INT format, UINT16 (* hlut)[8])
{
    INT16 hcf1[32][8],hcf2;
    INT16 i,j;
    INT uiRetVal = MV_VPP_OK;

    /* Convert INTo sign magnitude */
    for (i = 0; i < 32; i++) {
        for (j = 0; j < 8; j++) {
            hcf2 = (hlut[i][7-j])&0x800;
            hcf1[i][j] = (hlut[i][7-j])&0x7ff;
            hcf1[i][j] = (hcf2 ? -hcf1[i][j] : hcf1[i][j])&0x7ff;
            hcf1[i][j] = ((hcf2 | hcf1[i][j])&0xfff);
        }
}

    /*Horizontal Coeff table loading*/
    for(i = 0; i < VPP_FRC_SCL_NUM_OF_PHASES; i++) {
        default_index_coeff_table[coeffID][i][0] = hcf1[i][7];
        default_index_coeff_table[coeffID][i][1] = hcf1[i][6];
        default_index_coeff_table[coeffID][i][2] = hcf1[i][5];
        default_index_coeff_table[coeffID][i][3] = hcf1[i][4];
        default_index_coeff_table[coeffID][i][4] = hcf1[i][3];
        default_index_coeff_table[coeffID][i][5] = hcf1[i][2];
        default_index_coeff_table[coeffID][i][6] = hcf1[i][1];
        default_index_coeff_table[coeffID][i][7] = hcf1[i][0];
    }

    return uiRetVal;

}

INT OVPSCL_FRC_SCL_GenerateMain1DCoeffTabV(VPP_OBJ *vpp_obj, INT UnitNum, INT16 coeffID, INT16 hvSel, INT format, UINT16 (* vlut)[6])
{
    INT16 vcf1[32][8],vcf2;
    INT16 i,j;
    INT uiRetVal = MV_VPP_OK;
    INT vc[6];
    INT16 vTaps;
    /* Convert INTo sign magnitude */
    for (i = 0; i < 32; i++) {
        for (j = 0; j < 6; j++) {
            vcf2 = (vlut[i][5-j])&0x800;
            vcf1[i][j] = (vlut[i][5-j])&0x7ff;
            vcf1[i][j] = (vcf2 ? -vcf1[i][j] : vcf1[i][j])&0x7ff;
            vcf1[i][j] = ((vcf2 | vcf1[i][j])&0xfff);
        }
     }

    switch (coeffID) {
        case COEFF_V_MAIN_1D_VLUT_DBG1:
            vTaps = 1;
            break;
        case COEFF_V_MAIN_1D_VLUT_2TAP:
            vTaps = 2;
            break;
        case COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL:
            vTaps = 3;
            break;
        case COEFF_V_MAIN_1D_VLUT_BLK5:
            vTaps = 5;
            break;
        case COEFF_V_MAIN_1D_VLUT_6TAP:
            vTaps = 6;
            break;
        default:
            vTaps = -1;
            break;
    }

    /* Fitting tap position based on number vtaps */
    for (i = 0; i < 6; i++) {
        vc[i] = 6 +i - vTaps;
        if (vc[i] >= 6)
            vc[i] = 0;
    }

    /*Vertical Coeff table loading*/
    for (i = 0; i < VPP_FRC_SCL_NUM_OF_PHASES; i++) {
        default_index_coeff_table[coeffID][i][0] = vcf1[i][vc[5]];
        default_index_coeff_table[coeffID][i][1] = vcf1[i][vc[4]];
        default_index_coeff_table[coeffID][i][2] = vcf1[i][vc[3]];
        default_index_coeff_table[coeffID][i][3] = vcf1[i][vc[2]];
        default_index_coeff_table[coeffID][i][4] = vcf1[i][vc[1]];
        default_index_coeff_table[coeffID][i][5] = vcf1[i][vc[0]];
        default_index_coeff_table[coeffID][i][6] = 0;
        default_index_coeff_table[coeffID][i][7] = 0;
    }

    return uiRetVal;

}

INT OVPSCL_FRC_SCL_GenerateGfxCoeffTabHV(VPP_OBJ *vpp_obj, INT UnitNum, INT16 coeffID, INT16 hvSel, INT format, UINT16 (* lut)[8])
{
    INT16 cf1[17][8],cf2;
    INT16 i,j;
    INT uiRetVal = MV_VPP_OK;

    if ((VPP_FRC_SCL_PIP != UnitNum) && (VPP_FRC_SCL_OSD != UnitNum))
        return MV_VPP_EBADPARAM;

    /* Convert INTo sign magnitude */
    for (i = 0; i <= 16; i++) {
        for(j = 0; j < 8; j++) {
            cf2 = (lut[i][j])&0x200;
            cf1[i][j] = (lut[i][j])&0x1ff;
            cf1[i][j] = (cf2 ? -cf1[i][j] : cf1[i][j])&0x1ff;
            cf1[i][j] = ((cf2 | cf1[i][j])&0x3ff);
        }
    }

    /*Horizontal Coeff table loading*/
    for (i = 0; i < VPP_FRC_SCL_NUM_OF_PHASES; i++) {
        default_index_coeff_table[coeffID][i][0] = cf1[i][0];
        default_index_coeff_table[coeffID][i][1] = cf1[i][1];
        default_index_coeff_table[coeffID][i][2] = cf1[i][2];
        default_index_coeff_table[coeffID][i][3] = cf1[i][3];
        default_index_coeff_table[coeffID][i][4] = cf1[i][4];
        default_index_coeff_table[coeffID][i][5] = cf1[i][5];
        default_index_coeff_table[coeffID][i][6] = cf1[i][6];
        default_index_coeff_table[coeffID][i][7] = cf1[i][7];
    }

    return uiRetVal;
}

INT OVPSCL_FRC_SCL_Set_CSwap(VPP_OBJ *vpp_obj, INT UnitNum, INT iEnable)
{
    UINT32 uiRegAddr = 0;
    UINT32 uiRetVal = 0;
    T32ADPSCL_CFG6 stSclCfg6;
    stSclCfg6.u32 = vpp_obj->uiSclCfg6[UnitNum];
    stSclCfg6.uCFG6_ups_cswap = iEnable;

    uiRegAddr = INTERNAL_OVPS_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);/* Following this are SCL programming */
    if (0 == uiRegAddr) {
        return MV_VPP_EBADPARAM;
    }
    CHECK_VPP_SCL_RETVAL(OVPS_BCMBUF_Write_DBG(vpp_obj->ovp_obj.pOvpSclBcmBuf, uiRegAddr + RA_ADPSCL_CFG6, stSclCfg6.u32));
    vpp_obj->uiSclCfg6[UnitNum] = stSclCfg6.u32;

    return MV_VPP_OK;
}

