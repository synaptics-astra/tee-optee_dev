/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_FRC_SCL_C_

#include "vpp_module.h"
#include "vpp_api.h"
#include "vpp_bcmbuf.h"
#include "vpp_cfg.h"
#include "vpp_frc_scl.h"
#include "vpp_frc_scl_prv.h"

#include "vpp.h"

#include "coeff_hsoft.h"
#include "coeff_adp_scl_soft.h"
#include "vpp_clkGate.h"

#define VPP_FRC_SCL_MIN_WIN_WIDTH   200
#define VPP_FRC_SCL_MIN_WIN_HEIGHT  200
#define IS_VPP_FRC_SCL_MINIMUM_WINDOW(IH,OH,IV,OV)  \
    (((IH)<VPP_FRC_SCL_MIN_WIN_WIDTH) && ((OH)<VPP_FRC_SCL_MIN_WIN_WIDTH) && \
     ((IV)<VPP_FRC_SCL_MIN_WIN_HEIGHT) && ((OV)<VPP_FRC_SCL_MIN_WIN_HEIGHT))

#define VPP_BASE_ADDRESS_VIPSCL (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE)
#define VPP_FRC_COEFFTAB_NUM_OF_ENTRIES (VPP_FRC_SCL_NUM_OF_PHASES*VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS)

static UINT16 default_index_coeff_table[COEFF_MAX][VPP_FRC_SCL_NUM_OF_PHASES][VPP_FRC_SCL_NUM_OF_COEFF];

/***************************************************************************************
 * FUNCTION: INTernal fuction to get the start address of an FRC SCL instance by unit number
 * PARAMS: UnitNum - unit number
 * RETURN:  the start address
 ***************************************************************************************/
UINT32 INTERNAL_FRC_SCL_GetRegStartAddr(VPP_OBJ *vpp_obj, VPP_FRC_SCL_NUM UnitNum)
{
    UINT32 uiRegAddr = 0;

    /*get register start address */
    uiRegAddr = vpp_obj->base_addr;
    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_MAINSCL;
            break;
#ifdef ENABLE_DETEE
        case VPP_FRC_SCL_DET:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_DETSCL;
            break;
#endif
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FRC_SCL_PIP:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_SCL_GFX0;
            break;
#endif
        case VPP_FRC_SCL_OSD:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_SCL_GFX1;
            break;
#ifdef ENABLE_GFX2_VOP2_PATH
        case VPP_FRC_SCL_GFX2:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_SCL_GFX2;
            break;
#endif
        case VPP_FRC_SCL_VIP_Y:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC;
            break;
        case VPP_FRC_SCL_VIP_UV:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC;
            break;
#ifdef ENABLE_OVPSCL
        case VPP_FRC_SCL_OVP_Y:
            /* As VIP base addresss is different, prgm uiRegAddr freshly */
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC;
            break;
        case VPP_FRC_SCL_OVP_UV:
            /* As VIP base addresss is different, prgm uiRegAddr freshly */
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC;
            break;
#endif
        default:
            return 0;
            break;
    }

    return uiRegAddr;

}


/***************************************************************************************
   * FUNCTION: load the default register values to FRC SCL the unit
    * PARAMS: UnitNum - unit number
     * RETURN:  1 - succeed
      *                others - failed to load CPCB EE default values to BCM buffer
       ***************************************************************************************/
INT FRC_SCL_LoadDefaultVal(VPP_OBJ *vpp_obj, INT UnitNum)
{
    INT uiRetVal = MV_VPP_OK;

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

    return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: enable/disable alpha divider of GFX0 or PG scaler
 * PARAMS: UnitNum - unit number(2, 3)
 *         Enable - 1: enable, 0: disable
 * RETURN:  1 - succeed
 *                others - failed to load register to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetAlphaDivider(VPP_OBJ *vpp_obj, INT UnitNum, INT Enable)
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
INT FRC_SCL_SetBeFifo(VPP_OBJ *vpp_obj, PVPP_FRC_RES pCpcb0Tot, PVPP_FRC_RES pCpcb0Res)
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
INT VPP_SCL_ConfigUpsCoeff(VPP_OBJ *vpp_obj, INT *piCoeff, VPP_FRC_SCL_NUM UnitNum)
{

    INT uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    T32ADPSCL_UPS_CFG0 stSclUpsCfg0;
    T32ADPSCL_UPS_CFG1 stSclUpsCfg1;
    T32ADPSCL_UPS_CFG2 stSclUpsCfg2;
    T32ADPSCL_UPS_CFG3 stSclUpsCfg3;

    if(NULL == piCoeff
            || VPP_FRC_SCL_MAIN == UnitNum
      || VPP_FRC_SCL_DET == UnitNum
      )
      {
            return MV_VPP_EBADPARAM;
      }

    /* Make MSB as 1 for -ve values (signed magnitude) */
    stSclUpsCfg0.uCFG0_c0 = (piCoeff[0] < 0)? (0x1000 | -piCoeff[0] ): piCoeff[0];
    stSclUpsCfg0.uCFG0_c1 = (piCoeff[1] < 0)? (0x1000 | -piCoeff[1] ): piCoeff[1];
    stSclUpsCfg1.uCFG1_c2 = (piCoeff[2] < 0)? (0x1000 | -piCoeff[2] ): piCoeff[2];
    stSclUpsCfg1.uCFG1_c3 = (piCoeff[3] < 0)? (0x1000 | -piCoeff[3] ): piCoeff[3];
    stSclUpsCfg2.uCFG2_c4 = (piCoeff[4] < 0)? (0x1000 | -piCoeff[4] ): piCoeff[4];
    stSclUpsCfg2.uCFG2_c5 = (piCoeff[5] < 0)? (0x1000 | -piCoeff[5] ): piCoeff[5];
    stSclUpsCfg3.uCFG3_c6 = (piCoeff[6] < 0)? (0x1000 | -piCoeff[6] ): piCoeff[6];

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_UPS_CFG0, stSclUpsCfg0.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_UPS_CFG1, stSclUpsCfg1.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_UPS_CFG2, stSclUpsCfg2.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_UPS_CFG3, stSclUpsCfg3.u32));

    return uiRetVal;
}


/***************************************************************************************
 * FUNCTION: Load the scalar DNS coefficients
 * PARAMS:
 *             vpp_obj - vpp object
 *             piCoeff -  DNS coefficients
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
INT VPP_SCL_ConfigDnsCoeff(VPP_OBJ *vpp_obj, INT *piCoeff, VPP_FRC_SCL_NUM UnitNum)
{
    INT uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    T32ADPSCL_DNS_CFG0 stSclDnsCfg0;
    T32ADPSCL_DNS_CFG1 stSclDnsCfg1;
    T32ADPSCL_DNS_CFG2 stSclDnsCfg2;

    if(NULL == piCoeff
            || VPP_FRC_SCL_MAIN == UnitNum
      || VPP_FRC_SCL_DET == UnitNum
      )
      {
          return MV_VPP_EBADPARAM;
      }

    stSclDnsCfg0.uCFG0_c0 = (piCoeff[0] < 0)? (0x1000 | -piCoeff[0] ): piCoeff[0];
    stSclDnsCfg0.uCFG0_c1 = (piCoeff[1] < 0)? (0x1000 | -piCoeff[1] ): piCoeff[1];
    stSclDnsCfg1.uCFG1_c2 = (piCoeff[2] < 0)? (0x1000 | -piCoeff[2] ): piCoeff[2];
    stSclDnsCfg1.uCFG1_c3 = (piCoeff[3] < 0)? (0x1000 | -piCoeff[3] ): piCoeff[3];
    stSclDnsCfg2.uCFG2_c4 = (piCoeff[4] < 0)? (0x1000 | -piCoeff[4] ): piCoeff[4];
    stSclDnsCfg2.uCFG2_c5 = (piCoeff[5] < 0)? (0x1000 | -piCoeff[5] ): piCoeff[5];

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_DNS_CFG0, stSclDnsCfg0.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_DNS_CFG1, stSclDnsCfg1.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_DNS_CFG2, stSclDnsCfg2.u32));

    return uiRetVal;
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
INT VPP_SCL_MainCoeffRead(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8])
{
    UINT32 uiRegAddr = 0;
    INT16 i;
    INT uiRetVal = MV_VPP_OK;

    T32ADPSCL_COEFF_CFG0 stSclCoeffCfg0;
    T32ADPSCL_COEFF_CFG1 stSclCoeffCfg1;
    T32ADPSCL_COEFF_CFG2 stSclCoeffCfg2;
    T32ADPSCL_COEFF_CFG3 stSclCoeffCfg3;
    T32ADPSCL_COEFF_CFG4 stSclCoeffCfg4;

    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINSCL_COEFF;
            break;
#ifdef ENABLE_DETEE
        case VPP_FRC_SCL_DET:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_DETSCL_COEFF;
            break;
#endif
        case VPP_FRC_SCL_VIP_Y:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
        case VPP_FRC_SCL_VIP_UV:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
#ifdef ENABLE_OVPSCL
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
#endif
        default:
            return MV_VPP_EBADPARAM;
    }
    VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG0, &stSclCoeffCfg0.u32);
    stSclCoeffCfg0.uCFG0_coeffload   = 0;

    /*Horizontal Coeff table loading*/
    stSclCoeffCfg0.uCFG0_coeff_hvsel = 1;
    for(i=0; i<=16; i++)
    {
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        VPP_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);

        stSclCoeffCfg0.uCFG0_coeffread   = 1;
        VPP_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);

        VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG1, &stSclCoeffCfg1.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG2, &stSclCoeffCfg2.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG3, &stSclCoeffCfg3.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG4, &stSclCoeffCfg4.u32);

        hlut[i][0] = stSclCoeffCfg1.uCFG1_coeff0;
        hlut[i][1] = stSclCoeffCfg1.uCFG1_coeff1;
        hlut[i][2] = stSclCoeffCfg2.uCFG2_coeff2;
        hlut[i][3] = stSclCoeffCfg2.uCFG2_coeff3;
        hlut[i][4] = stSclCoeffCfg3.uCFG3_coeff4;
        hlut[i][5] = stSclCoeffCfg3.uCFG3_coeff5;
        hlut[i][6] = stSclCoeffCfg4.uCFG4_coeff6;
        hlut[i][7] = stSclCoeffCfg4.uCFG4_coeff7;
        stSclCoeffCfg0.uCFG0_coeffread   = 0;
        VPP_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);
    }

    /*Vertical Coeff table loading*/
    stSclCoeffCfg0.uCFG0_coeff_hvsel = 0;
    for(i=0; i<=16; i++)
    {
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        VPP_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);

        stSclCoeffCfg0.uCFG0_coeffread   = 1;
        VPP_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);

        VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG1, &stSclCoeffCfg1.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG2, &stSclCoeffCfg2.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG3, &stSclCoeffCfg3.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_ADPSCL_COEFF_CFG4, &stSclCoeffCfg4.u32);

        vlut[i][0] = stSclCoeffCfg1.uCFG1_coeff0;
        vlut[i][1] = stSclCoeffCfg1.uCFG1_coeff1;
        vlut[i][2] = stSclCoeffCfg2.uCFG2_coeff2;
        vlut[i][3] = stSclCoeffCfg2.uCFG2_coeff3;
        vlut[i][4] = stSclCoeffCfg3.uCFG3_coeff4;
        vlut[i][5] = stSclCoeffCfg3.uCFG3_coeff5;
        vlut[i][6] = stSclCoeffCfg4.uCFG4_coeff6;
        vlut[i][7] = stSclCoeffCfg4.uCFG4_coeff7;
        stSclCoeffCfg0.uCFG0_coeffread   = 0;
        VPP_REG_WORD32_WRITE(uiRegAddr + RA_ADPSCL_COEFF_CFG0, stSclCoeffCfg0.u32);
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
INT VPP_SCL_GfxCoeffRead(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8])
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
//    uiRegAddr = vpp_obj->base_addr;
    switch(UnitNum)
    {
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FRC_SCL_PIP:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_COEFF_GFX0;
            break;
#endif
        case VPP_FRC_SCL_OSD:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_COEFF_GFX1;
            break;
#ifdef ENABLE_GFX2_VOP2_PATH
        case VPP_FRC_SCL_GFX2:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_COEFF_GFX2;
            break;
#endif
        default:
            return 0;
            break;
    }
    VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG0, &stSclCoeffCfg0.u32);
    stSclCoeffCfg0.uCFG0_coeffload   = 0;

    /*Horizontal Coeff table loading*/
    stSclCoeffCfg0.uCFG0_coeff_hvsel = 1;
    for(i=0; i<=16; i++)
    {
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        VPP_REG_WORD32_WRITE(uiRegAddr + RA_COEFF_CFG0, stSclCoeffCfg0.u32);


        VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG1, &stSclCoeffCfg1.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG2, &stSclCoeffCfg2.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG3, &stSclCoeffCfg3.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG4, &stSclCoeffCfg4.u32);

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
    for(i=0; i<=16; i++)
    {
        stSclCoeffCfg0.uCFG0_coeff_index = i;
        VPP_REG_WORD32_WRITE(uiRegAddr + RA_COEFF_CFG0, stSclCoeffCfg0.u32);

        VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG1, &stSclCoeffCfg1.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG2, &stSclCoeffCfg2.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG3, &stSclCoeffCfg3.u32);
        VPP_REG_WORD32_READ(uiRegAddr + RA_COEFF_CFG4, &stSclCoeffCfg4.u32);

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
static INT VPP_SCL_CalcNLSclParams(PVPP_SCL_RES pIORes, INT32 *pscl_init_ratio, INT32 *pscl_inc_ratio, INT32 *pscl_nlcres)
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
    if(tail_ores < 5){
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
    if (nl_init_ratio < 0)
    {
        nl_init_ratio = 0;
        tail_ores = (((ires2*ores1/ores2)-ires1)<<20)/((c_ratio-nl_init_ratio)==0?1:(c_ratio-nl_init_ratio));
        if(tail_ores < 5){
            tail_ores = 5;
                  }
        else if (tail_ores>=(ores1>>1)) {
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
        if(tail_ores < 5){
            tail_ores = 5;
          }
        else if (tail_ores>=(ores2>>1)) {
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

static INT VPP_SCL_MainSclAdp1dType(VPP_OBJ *vpp_obj, PVPP_SCL_RES pIORes, INT8 *sel_1D_adp)
{

    *sel_1D_adp = 1;/* 1D Scalar */

    return MV_VPP_OK;
}
/*--------------------------
  -calculate htap and vtap numbers-
 ---------------------------*/
static INT VPP_SCL_MainSclCalTaps(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, INT8 *phTap, INT8 *pvTap)
{
    INT8 sel_1D_adp, hTaps, vTaps;
    UINT32 max_pixels_in_scl_mem = 1920;

    if(VPP_FRC_SCL_VIP_UV == UnitNum)
    {
        /*For VIP path Tap calculation should be same for both Luma and Croma.
          So multiplying croma hight by 2 to make both luma and croma having same height */
        pIORes->OHRes = pIORes->OHRes * 2;
    }

    VPP_SCL_MainSclAdp1dType(vpp_obj, pIORes, &sel_1D_adp);
    if(0 == sel_1D_adp)
    {
        hTaps = 4;
        vTaps = 4;
    }
    else
    {
        if((0 == vpp_obj->plane[PLANE_MAIN].frame_type) && (pIORes->OHRes > pIORes->IHRes) && (pIORes->OHRes > 1920))
            hTaps = 5;
        /* With hTaps=5 vertical lines are seen in NTS HDR test. #VSSDK-1525 */
        //else if((pIORes->OHRes > pIORes->IHRes) && (pIORes->IHRes > 1920))/* Upscale */
        //    hTaps = 5;
        else
            hTaps = 8;

    if(VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_DET == UnitNum)
            max_pixels_in_scl_mem = 1920;
        else
            max_pixels_in_scl_mem = 960;

        //else if((pIORes->OHRes <= pIORes->IHRes) || (pIORes->IHRes <= 1920))
        if((pIORes->IVRes == pIORes->OVRes) && (pIORes->OHRes == pIORes->IHRes))
            vTaps = 1;//3;//6;//1;/*horizontalOnly scale modes including bypass modes where scaler.s line buffer SRAM is used as rat control FIFO*/
        else if(pIORes->IVRes == pIORes->OVRes)
            vTaps = 1;//3;//6;//1;/*horizontalOnly scale modes including bypass modes where scaler.s line buffer SRAM is used as rat control FIFO*/
        else if((pIORes->OHRes > pIORes->IHRes) && (0 == vpp_obj->plane[PLANE_MAIN].frame_type))/* Upscale interlaced */
            vTaps = 3;
        else if((pIORes->OHRes > pIORes->IHRes) && (pIORes->IHRes > max_pixels_in_scl_mem))/* Upscale *///else if((pIORes->OHRes > pIORes->IHRes) && (pIORes->IHRes >= 1920))/* Upscale */
            vTaps = 3;
        else if((vpp_obj->uiResID[CPCB_1] >= RES_MIN_4Kx2K) && (pIORes->OHRes <= pIORes->IHRes) &&
                (pIORes->IHRes > 1920) && (pIORes->OHRes > 1920)) /* DownScale 4K display*/
        {
            if((vpp_obj->uiResID[CPCB_1] >= RES_MIN_4Kx2K) && (vpp_obj->uiResID[CPCB_1] <= RES_4Kx2K30))
                vTaps = 2;
            else
                vTaps = 3;
        }
        else if((pIORes->OHRes <= pIORes->IHRes) && (pIORes->OVRes <= 540) && (vpp_obj->HDR_capable == TRUE))
            vTaps = 3; // to avoid reddish horizontal lines in HDR test cases(netflix)

        else if((pIORes->OHRes <= pIORes->IHRes))/* DownScale */
        {
            if(VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_DET == UnitNum)
                vTaps = 5;//6; /* Faint Line issue fix Bug# 42575 */
            else
            {
                /*OVP and VIP SCL has memsize max ((575+1)*10) pixles. So if OHRes less than 1152 only can hold 5 lines*/
                if(pIORes->OHRes < 1152)
                    vTaps = 5;
                else
                    vTaps = 3;
            }
        }
      /* Fixing the faint horizontal line on NTS HDR upscale; Bug# 1525. vTaps = 3 */
      /* Fixing the faint line issue for up scaling o/p Vres more than 1440
         removed validation (pIORes->OVRes > 1440) as we are seeing issue for OVRes <1440 in DOLBY/HDR cases. vTaps = 5 */
        else if(pIORes->OVRes > pIORes->IVRes)
            vTaps = (vpp_obj->bIsHDROutput == TRUE)? 3 : 5;
        else
            vTaps = 6;

    }
    if(VPP_FRC_SCL_VIP_UV == UnitNum)
    {
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
static INT VPP_SCL_SclConfigForDeint(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, INT8 *enDeintSmallUpScale)
{

    if((VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_DET == UnitNum) && (0 == vpp_obj->plane[PLANE_MAIN].frame_type) && (vpp_obj->uiResID[CPCB_1] >= RES_MIN_4Kx2K)
            && (pIORes->IHRes < pIORes->OHRes) && (pIORes->IVRes < pIORes->OVRes)
            && (pIORes->OVRes > 1080) /*&& (pIORes->OVRes < 1680)*/)
    {
        *enDeintSmallUpScale = 1;
    }
    else
    {
        *enDeintSmallUpScale = 0;
    }
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
static INT VPP_SCL_SetMainSclparams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    INT8 sel_1D_adp = -1;
    INT8 up_down = -1;
    UINT32 HRatio, VRatio;
    UINT32 minHRes, hRes,maxHRes;
    INT8 hTaps = -1, vTaps = -1;
    INT8 enDeintSmallUpScale = 0;
    INT32 scl_init_ratio, scl_inc_ratio,  scl_nlcres;
    INT16 wls = 0, pls = 0, fcp = 0;
    INT32 memsize = 0;
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

    T32CPCB0_CTRL9 stCpcb0Ctrl9;

    UINT32 uiRegAddr = 0;
    UINT32 uncrop_IHRes = pSclCtrl->uncrop_IHRes;
    RESOLUTION_INFO  *pResinfo;

    if(VPP_FRC_SCL_MAIN != UnitNum && VPP_FRC_SCL_DET != UnitNum && VPP_FRC_SCL_VIP_Y != UnitNum && VPP_FRC_SCL_VIP_UV != UnitNum && VPP_FRC_SCL_OVP_Y != UnitNum && VPP_FRC_SCL_OVP_UV != UnitNum)
        return MV_VPP_EBADPARAM;

    /*-------------------------
      - select the scalar -
      --------------------------*/
    VPP_SCL_SclConfigForDeint(vpp_obj, UnitNum, pIORes, &enDeintSmallUpScale);
    VPP_SCL_MainSclAdp1dType(vpp_obj, pIORes, &sel_1D_adp);

    /*-------------------------
      - select the up/down scale mode -
      --------------------------*/
    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[CPCB_1]);
    if((pIORes->OHRes < pIORes->IHRes) || ((pIORes->OHRes == pIORes->IHRes) && (pIORes->OVRes <  pIORes->IVRes)))
    {
        /* Horizontal down scaling or veritcal only down scaling */
        up_down = 1;/* down scale */
    }
    else
        up_down = 0;/* up scale/1:1(bypass) */
    if(VPP_FRC_SCL_MAIN == UnitNum)
    {
        /* Only required for Main Pipe. VIP SCL this is not required */
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL9;
        stCpcb0Ctrl9.u32 = vpp_obj->uiCpcb0Ctrl9;

        if(pIORes->OHRes < pResinfo->active_width)
        {/* Horzontal up and vertical down scaling and other horizontal PIP style cases.
            PIP style Bigger down scaling.
            Closer cases along with Prefetch causes streaking artifacts */
            stCpcb0Ctrl9.uCTRL9_pixLineTot_adpscl = pIORes->OHRes;
            stCpcb0Ctrl9.uCTRL9_lineDn_ADPSCL_En = 1;
        }
        else
        {
            stCpcb0Ctrl9.uCTRL9_pixLineTot_adpscl = 0x64;
            stCpcb0Ctrl9.uCTRL9_lineDn_ADPSCL_En = 0;
        }
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, stCpcb0Ctrl9.u32);
        vpp_obj->uiCpcb0Ctrl9 = stCpcb0Ctrl9.u32;

        if(1 == enDeintSmallUpScale)
        {
            T32vidpl_SCL_CLKEN_CTRL sVidplSclClkenCtrl;

            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_SCL_CLKEN_CTRL;
            GA_REG_WORD32_READ(uiRegAddr, &sVidplSclClkenCtrl.u32);
            sVidplSclClkenCtrl.uSCL_CLKEN_CTRL_Main = 1;
            VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, sVidplSclClkenCtrl.u32);
        }
    }

    uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);/* Following this are SCL programming */
    if(!uiRegAddr)
    {
        return MV_VPP_EBADPARAM;
    }

    /*--------------------------
      -calculate htap and vtap numbers-
      ---------------------------*/
    VPP_SCL_MainSclCalTaps(vpp_obj, UnitNum, pIORes, &hTaps, &vTaps);
    /*-------------------------
      - set initial phase and ctl0 -
      --------------------------*/
    if(0 == sel_1D_adp)
    {
        stSclCfg0.uCFG0_even = 0x80;
        stSclCfg0.uCFG0_odd = 0x80;
        stSclCfg0.uCFG0_hinitph = 0x80;
    }
    else
    {
        /*Fix for Vertical and horizontal black bar in center for down scaling in Main path*/
        if (UnitNum == VPP_FRC_SCL_VIP_Y || UnitNum == VPP_FRC_SCL_VIP_UV)
        {
            stSclCfg0.uCFG0_even = 0;
            stSclCfg0.uCFG0_odd = 0;
            stSclCfg0.uCFG0_hinitph = (1)<<7;
        }
        else
        {
            stSclCfg0.uCFG0_even = (vTaps % 2)<<7;
            stSclCfg0.uCFG0_odd = (vTaps % 2)<<7;
            stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
        }
    }

    stSclCtrl0.uchAvg4FiltSel = 0;
    stSclCtrl0.uchCscEn = 0;
    if(1 == pSclCtrl->NLEn)
    {
        stSclCtrl0.uchHnlEn = 1;
    }
    else
    {
        stSclCtrl0.uchHnlEn = 0;
    }

    if(1 == sel_1D_adp && 1 == up_down)
    {
        /* 1D down scaling */
        stSclCtrl0.uchUpsEn = 1;
        stSclCtrl0.uchDnsEn = 1;
    }
    else
    {
        stSclCtrl0.uchUpsEn = 0;
        switch(UnitNum)
        {
            case VPP_FRC_SCL_MAIN:
            case VPP_FRC_SCL_DET:
                stSclCtrl0.uchDnsEn = 0;
                break;
            case VPP_FRC_SCL_VIP_Y:
            case VPP_FRC_SCL_VIP_UV:
                stSclCtrl0.uchDnsEn = 1;
                break;
            case VPP_FRC_SCL_OVP_Y:
            case VPP_FRC_SCL_OVP_UV:
                stSclCtrl0.uchDnsEn = 0;
                break;
            default:
                return MV_VPP_EBADPARAM;
        }
    }
    stSclCtrl0.uchDgnlEn = 1;
    stSclCtrl0.uchDemoModeEn = 0;

    //This is not required if clock gating is done after SetSclParams()
    //But multiple write to CFG15 in same VBI interval seems to be not working.
    stSclCfg15.u32 = vpp_obj->uiSclCfg15[UnitNum];
    if(stSclCfg15.uCFG15_bypass) {
        //Enable UPS
        stSclCtrl0.uchUpsEn = 1;
    }

    stSclCfg0.uCFG0_ctrl0 = stSclCtrl0.uch8;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG0, stSclCfg0.u32));
    vpp_obj->uiSclCfg0[UnitNum] = stSclCfg0.u32;
    /*-------------------------
      - set input and output resolutions -
      --------------------------*/

    stSclCfg1.uCFG1_ivres = pIORes->IVRes;
    stSclCfg1.uCFG1_ovres = pIORes->OVRes;
    //stSclCfg2.uCFG2_ihres = pIORes->IHRes;
    stSclCfg2.uCFG2_ihres = uncrop_IHRes;          //Read the complete uncropped resolution
    stSclCfg2.uCFG2_ohres = pIORes->OHRes;

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG1, stSclCfg1.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG2, stSclCfg2.u32));

    /*-------------------------------------
      -calculate horizontal and vertical scaling ratio -
      -------------------------------------*/

    HRatio = ((UINT32) pIORes->IHRes << 20) / pIORes->OHRes;
    VRatio = ((UINT32) pIORes->IVRes << 20) / pIORes->OVRes;

    stSclCfg3.u32 = 0;
    stSclCfg4.u32 = 0;
    stSclCfg3.uCFG3_vratio = VRatio;
    stSclCfg4.uCFG4_hratio = HRatio;

    if(pIORes->OVRes < pIORes->IVRes)
    {
        /* 1: the most bottom pixel of input image is used to interpolate the most bottom pixel of output image */
        stSclCfg3.uCFG3_mbp_align = 1;
    }
    else
    {
        /* Must be 0 if Vertically '1:1 or upscale' */
        stSclCfg3.uCFG3_mbp_align = 0;
    }

    if(pIORes->OHRes < pIORes->IHRes)
    {
        /* 1: the most right pixel of input image is used to interpolate the most right pixel of output image */
        stSclCfg4.uCFG4_mrp_align = 1;
    }
    else
    {
        /* Must be 0 if Horizontally '1:1 or upscale' */
        stSclCfg4.uCFG4_mrp_align = 0;
    }


    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG3 , stSclCfg3.u32));
    vpp_obj->uiSclCfg3[UnitNum] = stSclCfg3.u32;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG4 , stSclCfg4.u32));

    stSclCfg7.uCFG7_ups_cblank = 0x200;

    stSclCfg7.uCFG7_htap_offset = 0;
    stSclCfg7.uCFG7_vtap_offset = 0;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG7, stSclCfg7.u32));
    vpp_obj->uiSclCfg7[UnitNum] = stSclCfg7.u32;

    /*-------------------
      -set SCL control register-
      --------------------*/
    /*if non linear scaling, H scaler is after V scaler*/
    if(0 == pSclCtrl->NLEn)
    {
        stSclCfg8.uCFG8_init_ratio = 0x100000;
        stSclCfg9.uCFG9_inc_ratio  = 0;
        stSclCfg10.uCFG10_nlcres   = 0;
    }
    else
    {
        VPP_SCL_CalcNLSclParams(pIORes, &scl_init_ratio, &scl_inc_ratio, &scl_nlcres);
        stSclCfg8.uCFG8_init_ratio = scl_init_ratio;
        stSclCfg9.uCFG9_inc_ratio  = scl_inc_ratio;
        stSclCfg10.uCFG10_nlcres   = scl_nlcres;
    }
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG8, stSclCfg8.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG9, stSclCfg9.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG10, stSclCfg10.u32));

    stSclCfg11.uCFG11_avg4_coeff0 = 0x840;
    stSclCfg11.uCFG11_avg4_coeff1 = 0x240;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG11 , stSclCfg11.u32));

    stSclCfg12.uCFG12_avg4_coeff2 = 0x240;
    stSclCfg12.uCFG12_avg4_coeff3 = 0x840;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG12 , stSclCfg12.u32));

    stSclCtrl1.uch8 = 0;
    stSclCtrl1.uchSel_1D_adp = sel_1D_adp;
    stSclCtrl1.uchUp_down = up_down;
    if(0 == sel_1D_adp)
    {
        stSclCtrl1.uchVSclInUpsEn    = 1;
        stSclCtrl1.uchVSclOutUpsEn   = 0;
        stSclCtrl1.uchVSclInUpsCoeff = 0;
    }
    else
    {
        stSclCtrl1.uchVSclInUpsEn    = 0;
        stSclCtrl1.uchVSclInUpsCoeff = 0;
        switch(UnitNum)
        {
            case VPP_FRC_SCL_MAIN:
            case VPP_FRC_SCL_DET:
                stSclCtrl1.uchVSclOutUpsEn   = 1;
                break;
            case VPP_FRC_SCL_VIP_Y:
            case VPP_FRC_SCL_VIP_UV:
                if(1 == up_down)
                {
                    stSclCtrl1.uchVSclOutUpsEn   = 0;
                }
                else
                {
                    stSclCtrl1.uchVSclOutUpsEn   = 1;
                }
                break;
            case VPP_FRC_SCL_OVP_Y:
            case VPP_FRC_SCL_OVP_UV:
                stSclCtrl1.uchVSclOutUpsEn   = 0;
                break;
            default:
                return MV_VPP_EBADPARAM;
        }
    }

    stSclCfg15.u32 = vpp_obj->uiSclCfg15[UnitNum];
    stSclCfg15.uCFG15_ctrl1 = stSclCtrl1.uch8;
    stSclCfg15.uCFG15_frndsel = 1;
    stSclCfg15.uCFG15_hctrl = 0x1A;
    if((1 == vTaps || 2 == vTaps)
            && (0 == stSclCfg15.uCFG15_vscl11)
      )
        stSclCfg15.uCFG15_vctrl = 0x0B;
    else
        stSclCfg15.uCFG15_vctrl = 0x1A;


    //LSB 2 bit for to crop at begining of line, MSB 2 bit to crop at end of line
    stSclCfg15.uCFG15_crop = ((pSclCtrl->leftCrop&0x3)) | ((pSclCtrl->rightCrop&0x03)<<2);
    VDB_print("Scalar crop control : 0x%x\n", stSclCfg15.uCFG15_crop);


    if(vpp_obj->scl_ctrl[UnitNum].hvswap_en)
    {
         stSclCfg15.uCFG15_mask_hvswap_en = 0x2;
    }
    else
    {
         stSclCfg15.uCFG15_mask_hvswap_en = 0x0;
    }

    if(!(UnitNum==VPP_FRC_SCL_MAIN || VPP_FRC_SCL_DET == UnitNum))
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG15 , stSclCfg15.u32));
    else
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG15 , stSclCfg15.u32));

    vpp_obj->uiSclCfg15[UnitNum] = stSclCfg15.u32;


    hRes = uncrop_IHRes;   //lsize calculation should be based on complete/uncropped resolution

    if(0 == sel_1D_adp)
    {
        stSclCfg17.uCFG17_lsize   = (hRes/ 10) + ((hRes% 10)? 1:0);
        stSclCfg17.uCFG17_lsize_A = (hRes/ 10) + ((hRes% 10)? 1:0);
    }
    else
    {
        if(vpp_obj->scl_ctrl[UnitNum].hvswap_en == 0)
        {
            if(hRes < pIORes->OHRes)
                minHRes = hRes;
            else
                minHRes = pIORes->OHRes;
            /* If Scalar chop is enabled, then choose hres as follows
             * Upscale, -> hres = uncropped ihres
             * downscale, -> hres = ohres
             * 1:1, -> hres = uncropped ihres */
            if( (pSclCtrl->leftCrop+pSclCtrl->rightCrop) != 0) {
                //Use cropped Hres for comparision
                if(pIORes->IHRes <= pIORes->OHRes)
                    minHRes = hRes;
                else
                    minHRes = pIORes->OHRes;
            }
            stSclCfg17.uCFG17_lsize   = (minHRes/ 10) + ((minHRes% 10)? 1:0);
            stSclCfg17.uCFG17_lsize_A = (minHRes/ 10) + ((minHRes% 10)? 1:0);
        }
        else
        {

            if(hRes > pIORes->OHRes)
                maxHRes = hRes;
            else
                maxHRes = pIORes->OHRes;
            /* If Scalar chop is enabled, then choose hres as follows
             * Upscale, -> hres = uncropped ihres
             * downscale, -> hres = ohres
             * 1:1, -> hres = uncropped ihres */
            if( (pSclCtrl->leftCrop+pSclCtrl->rightCrop) != 0) {
                //Use cropped Hres for comparision
                if(pIORes->IHRes >= pIORes->OHRes)
                    maxHRes = hRes;
                else
                    maxHRes = pIORes->OHRes;
            }
            stSclCfg17.uCFG17_lsize   = (maxHRes/ 10) + ((maxHRes% 10)? 1:0);
            stSclCfg17.uCFG17_lsize_A = (maxHRes/ 10) + ((maxHRes% 10)? 1:0);

        }
    }
    if(!(UnitNum==VPP_FRC_SCL_MAIN || VPP_FRC_SCL_DET == UnitNum))
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG17 , stSclCfg17.u32));
    else
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG17 , stSclCfg17.u32));

    stSclCfg18.u32=0;
    if(VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_DET == UnitNum)
        maxMemSize_1D =  1151;
    else if(VPP_FRC_SCL_VIP_UV == UnitNum)
        maxMemSize_1D = 287;
    else
        maxMemSize_1D = 575;

    /* Prefetch mode Settings */
    /* Prefetch only enable in 4K small downscaling with 4K canvas.
       If enabled for bigger PIP style cases needs specific tunning of udf/ovf margins, which is not required */
    /* Making the min resolution to RES_MIN_4Kx2K instead of 4K30 to avoid line issue when small down scaling while playing Dolby L5 cert streams */
    if((pIORes->IVRes > pIORes->OVRes) &&
            (RES_MIN_4Kx2K <= vpp_obj->uiResID[CPCB_1]) &&
            ((pResinfo->active_height*(100-13)/100) <= pIORes->OVRes))/* Tune the limit if required */
        stSclCfg18.uCFG18_pftch = 1;
    else if((pIORes->IVRes/2 > pIORes->OVRes)/* Interlaced ouput case scaler used to downscale by 2x. So divide by 2 in formulas */
            && (SCAN_PROGRESSIVE != pResinfo->scan)
            && ((pResinfo->active_height*(100-13)/100/2) <= pIORes->OVRes))
    {/* Interlaced ouput small downscaling cases */
        stSclCfg18.uCFG18_pftch = 1;
    }
    else
        stSclCfg18.uCFG18_pftch = 0;

    if(1 == stSclCfg18.uCFG18_pftch)
    {
        memsize = (stSclCfg17.uCFG17_lsize * vTaps) - 1;
        if((1 == sel_1D_adp) && (memsize > maxMemSize_1D))
        {
            stSclCfg5.uCFG5_memsize = maxMemSize_1D;
        }
        else if((0 == sel_1D_adp) && (memsize > 1727))
            stSclCfg5.uCFG5_memsize = 1727;
        else
            stSclCfg5.uCFG5_memsize = memsize;
    }
    else
    {
        if(0 == sel_1D_adp)
            stSclCfg5.uCFG5_memsize = 1727;
        else
        {
            stSclCfg5.uCFG5_memsize = maxMemSize_1D;
        }
    }
    stSclCfg5.uCFG5_vwrap   = 0;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG5 , stSclCfg5.u32));

    stSclCfg6.uCFG6_ups_cswap = pSclCtrl->uiUVswapEn;
    stSclCfg6.uCFG6_ups_cshift = 0;
    stSclCfg6.uCFG6_ups_yshift = 0;
    stSclCfg6.uCFG6_ups_yblank = 0;

    if((1920 < pIORes->IHRes &&
                (((pIORes->IHRes == pIORes->OHRes) && (pIORes->IVRes == pIORes->OVRes))/* 1:1 case */ ||
                 (pIORes->IVRes > pIORes->OVRes && ((((pIORes->IVRes - pIORes->OVRes)* 100)/ (pIORes->IVRes)) <= 13 )))) /* vertical only or both vertical and horizontal down scaling till 13% */
       || ((RES_4Kx2K60 == vpp_obj->uiResID[CPCB_1]) && (0 == vpp_obj->plane[PLANE_MAIN].frame_type) && (pIORes->IHRes <= pIORes->OHRes) && (pIORes->IHRes > 1280) && (0 == enDeintSmallUpScale)) /*Interlaced input */
       || (1 == stSclCfg18.uCFG18_pftch)
      )
    {
        stSclCfg6.uCFG6_fstall = 0x0;
    }
    else
    {
        stSclCfg6.uCFG6_fstall = 0x80;
    }
    stSclCfg6.uCFG6_bstall = 0x80;

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG6, stSclCfg6.u32));
    vpp_obj->uiSclCfg6[UnitNum] = stSclCfg6.u32;

    if(((pIORes->IVRes == pIORes->OVRes) && (1 == vTaps))
            && (0 == stSclCfg15.uCFG15_vscl11)
      )
        stSclCfg18.uCFG18_fifo_mode  = 1;
    else
        stSclCfg18.uCFG18_fifo_mode  = 0;

    if((1 == stSclCfg18.uCFG18_fifo_mode)
            || (1 == stSclCfg18.uCFG18_pftch)
      )
    {
        if(pIORes->IHRes < pIORes->OHRes)
            minHRes = pIORes->IHRes;
        else
            minHRes = pIORes->OHRes;
        wls = (stSclCfg5.uCFG5_memsize + 1) / stSclCfg17.uCFG17_lsize;
        pls = ((stSclCfg5.uCFG5_memsize + 1) - (stSclCfg17.uCFG17_lsize * wls)) * 10;
        fcp = wls * minHRes + pls;


        stSclCfg18.uCFG18_fifo_depth = (stSclCfg18.uCFG18_pftch) ? fcp : (fcp - 100);
        if(0 == stSclCfg18.uCFG18_fifo_mode)
        {
            stSclCfg18.uCFG18_fifo_dfst  = 0;
        }
        else
        {
            stSclCfg18.uCFG18_fifo_dfst  = minHRes;
        }
    }
    else
    {
        stSclCfg18.uCFG18_fifo_mode  = 0;
        stSclCfg18.uCFG18_fifo_depth = 0;
        stSclCfg18.uCFG18_fifo_dfst  = 0;
    }
    if(!(UnitNum==VPP_FRC_SCL_MAIN || VPP_FRC_SCL_DET == UnitNum))
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG18 , stSclCfg18.u32));
    else
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG18 , stSclCfg18.u32));


    stSclCfg16.u32=0;

    if((0 == up_down)
            || (1 == stSclCfg18.uCFG18_pftch)
      )
    {
        if (pIORes->OHRes > pIORes->IHRes &&
                pIORes->OVRes < pIORes->IVRes)
            stSclCfg16.uCFG16_en_n          = 0;
        else
            stSclCfg16.uCFG16_en_n          = 1;
    }
    else
    {
        stSclCfg16.uCFG16_en_n          = 0;
    }
    if((pIORes->OHRes > pIORes->IHRes &&  pIORes->OVRes < pIORes->IVRes) ||
            (pIORes->OHRes < pIORes->IHRes &&  pIORes->OVRes > pIORes->IVRes))
        stSclCfg16.uCFG16_xbstall_en    = 0;
    else if(1 == enDeintSmallUpScale)
        stSclCfg16.uCFG16_xbstall_en    = 0;
    else if((1 == stSclCfg16.uCFG16_en_n && 0 == up_down) || (1 == stSclCfg18.uCFG18_fifo_mode))
        stSclCfg16.uCFG16_xbstall_en    = 1;
    else
        stSclCfg16.uCFG16_xbstall_en    = 0;

    if((pIORes->OHRes - pIORes->IHRes) >= 30)
        stSclCfg16.uCFG16_xbstall_dly   = 30;
    else
        stSclCfg16.uCFG16_xbstall_dly   = 0;

    if(1 == stSclCfg18.uCFG18_pftch)
    {
        stSclCfg16.uCFG16_ovf_margin = 0xA;
        stSclCfg16.uCFG16_udf_margin = 0x14;
    }
    else
    {
        stSclCfg16.uCFG16_ovf_margin = 0;
        stSclCfg16.uCFG16_udf_margin = 0;
    }
    if(!(UnitNum==VPP_FRC_SCL_MAIN || VPP_FRC_SCL_DET == UnitNum))
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG16 , stSclCfg16.u32));
    else
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG16 , stSclCfg16.u32));

    stSclCfg19.u32 = 0;
    stSclCfg19.uCFG19_scl_after_crop_h = ((pSclCtrl->cropAfterSclLeft << 4) | (pSclCtrl->cropAfterSclRight));
    stSclCfg19.uCFG19_scl_after_crop_v = ((pSclCtrl->cropAfterSclTop << 4) | (pSclCtrl->cropAfterSclBottom));
    if(!(UnitNum==VPP_FRC_SCL_MAIN || VPP_FRC_SCL_DET == UnitNum))
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG19 , stSclCfg19.u32));
    else
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG19 , stSclCfg19.u32));

    stScl1dYcCfg20.u32 = 0;
    stScl1dYcCfg20.uCFG20_dpwr_regs = 3;
    stScl1dYcCfg20.uCFG20_syp_yc420_regs = 0;
    if(!(UnitNum==VPP_FRC_SCL_MAIN || VPP_FRC_SCL_DET == UnitNum))
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG20 , stScl1dYcCfg20.u32));

    return uiRetVal;
}

/*--------------------------
  -calculate htap and vtap numbers-
 ---------------------------*/
static INT VPP_SCL_GfxSclCalTaps(VPP_OBJ *vpp_obj, PVPP_SCL_RES pIORes, INT8 *phTap, INT8 *pvTap)
{
    INT8 hTaps, vTaps;

    hTaps = 5;//8;

    if(pIORes->IVRes == pIORes->OVRes)
        vTaps = 1;/*horizontalOnly scale modes including bypass modes where scaler.s line buffer SRAM is used as rat control FIFO*/
    /*Bugfix: Fix for vertical only scaling. Considerted vertical resolution also for below 2 calculations */
    else if(((pIORes->OHRes > pIORes->IHRes) || ((pIORes->OHRes == pIORes->IHRes) && (pIORes->OVRes > pIORes->IVRes))) && (pIORes->IHRes > 1440))/* Upscale */
        vTaps = 3;
    else if(((pIORes->OHRes < pIORes->IHRes) || ((pIORes->OHRes == pIORes->IHRes) && (pIORes->OVRes < pIORes->IVRes))) && (pIORes->OHRes > 1440))/* Downscale */
        vTaps = 3;
    else
        vTaps = 4;
#if 1
    //Force VTaps to 1 - 4K downscale for PIP/Gfx path
    if((pIORes->OHRes < pIORes->IHRes) && (pIORes->IHRes > 1920))
    {
        vTaps=1;
    }
#endif

    *phTap = hTaps;
    *pvTap = vTaps;
    return MV_VPP_OK;
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
static INT VPP_SCL_SetGfxSclparams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    UINT32 HRatio, VRatio, c_ratio = 0;
    UINT32 minHRes, hRes = 0, TS = 0;
    INT8 hTaps = -1, vTaps = -1, real_hTaps = 1;
    INT32 scl_init_ratio, scl_inc_ratio,  scl_nlcres;
    INT16 wls = 0, pls = 0, fcp = 0;
    INT32 memsize = 0;

    INT uiRetVal = MV_VPP_OK;

    T32SCL_CFG0 stSclCfg0;
    T32SCL_CFG1 stSclCfg1;
    T32SCL_CFG2 stSclCfg2;
    T32SCL_CFG3 stSclCfg3;
    T32SCL_CFG4 stSclCfg4;
    T32SCL_CFG5 stSclCfg5;
    T32SCL_CFG6 stSclCfg6;
    T32SCL_CFG7 stSclCfg7;
    T32SCL_CFG8 stSclCfg8;
    T32SCL_CFG9 stSclCfg9;
    T32SCL_CFG10 stSclCfg10;
    T32SCL_CFG11 stSclCfg11;
    T32SCL_CFG12 stSclCfg12;
    T32SCL_CFG13 stSclCfg13;
    T32CPCB0_CTRL8 sCpcb0Ctrl8;

    UINT32 uiRegAddr = 0;
    UINT32 uncrop_IHRes = pSclCtrl->uncrop_IHRes;
    int cpcbID, chanID;
    RESOLUTION_INFO  *pResinfo;

    if((VPP_FRC_SCL_PIP != UnitNum) && (VPP_FRC_SCL_OSD != UnitNum) &&(VPP_FRC_SCL_GFX2 != UnitNum))
        return MV_VPP_EBADPARAM;

    if(UnitNum == VPP_FRC_SCL_PIP)
        chanID = CHAN_PIP;
    else if(UnitNum == VPP_FRC_SCL_OSD)
        chanID = CHAN_GFX1;
    else if(UnitNum == VPP_FRC_SCL_GFX2)
        chanID = CHAN_GFX2;
    else
        chanID = CHAN_MAIN; //FIXME:Give appropriate channel ID

    cpcbID = CPCB_OF_CHAN(vpp_obj,chanID);
    /*-------------------------
      - select the scalar -
      --------------------------*/



    /*-------------------------
      - select the up/down scale mode -
      --------------------------*/
    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[cpcbID]);

    uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL8;
    sCpcb0Ctrl8.u32 = vpp_obj->uiCpcb0Ctrl8;
    if(pIORes->OHRes < pResinfo->active_width
            && (pResinfo->active_width >= 1280))
    {/* PIP style down scaling */
        switch(UnitNum)
        {
            case VPP_FRC_SCL_PIP:
                sCpcb0Ctrl8.uCTRL8_pixLineTot_gfx0 = pIORes->OHRes;
                sCpcb0Ctrl8.uCTRL8_lineDn_Gfx0_En = 1;
                break;
            case VPP_FRC_SCL_OSD:
                sCpcb0Ctrl8.uCTRL8_pixLineTot_gfx1 = pIORes->OHRes;
                sCpcb0Ctrl8.uCTRL8_lineDn_Gfx1_En = 1;
                break;
            case VPP_FRC_SCL_GFX2:
                /* Nothing to do here as GFX2 not connected to CPCB OVL */
                break;
            default:
                return MV_VPP_EBADPARAM;
                break;
        }

    }
    else
    {
        switch(UnitNum)
        {
            case VPP_FRC_SCL_PIP:
                sCpcb0Ctrl8.uCTRL8_pixLineTot_gfx0 = 0x64;
                sCpcb0Ctrl8.uCTRL8_lineDn_Gfx0_En = 0;
                break;
            case VPP_FRC_SCL_OSD:
                sCpcb0Ctrl8.uCTRL8_pixLineTot_gfx1 = 0x64;
                sCpcb0Ctrl8.uCTRL8_lineDn_Gfx1_En = 0;
                break;
            case VPP_FRC_SCL_GFX2:
                /* Nothing to do here as GFX2 not connected to CPCB OVL */
                break;
            default:
                return uiRetVal;
                break;
        }

    }
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, sCpcb0Ctrl8.u32);
    vpp_obj->uiCpcb0Ctrl8 = sCpcb0Ctrl8.u32;

    uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
    if(!uiRegAddr)
    {
        return MV_VPP_EBADPARAM;
    }
    /*--------------------------
      -calculate htap and vtap numbers-
      ---------------------------*/
    VPP_SCL_GfxSclCalTaps(vpp_obj, pIORes, &hTaps, &vTaps);

    real_hTaps = hTaps; //TODO

    /*-------------------------
      - set initial phase and ctl0 -
      --------------------------*/
    stSclCfg0.u32 = 0;
    stSclCfg0.uCFG0_vinitph = (vTaps % 2)<<7;
    stSclCfg0.uCFG0_hinitph = (real_hTaps % 2)<<7;//TODO find value for real_hTaps

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG0, stSclCfg0.u32));
    vpp_obj->uiSclCfg0[UnitNum] = stSclCfg0.u32;
    /*-------------------------
      - set input and output resolutions -
      --------------------------*/

    stSclCfg1.uCFG1_ivres = pIORes->IVRes;
    stSclCfg1.uCFG1_ovres = pIORes->OVRes;
    //stSclCfg2.uCFG2_ihres = pIORes->IHRes;
    stSclCfg2.uCFG2_ihres = uncrop_IHRes;          //Read the complete uncropped resolution
    stSclCfg2.uCFG2_ohres = pIORes->OHRes;

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG1, stSclCfg1.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG2, stSclCfg2.u32));

    /*-------------------------------------
      - calculate horizontal and vertical scaling ratio -
      -------------------------------------*/
    if(1 == pSclCtrl->NLEn)
        HRatio = c_ratio;//TODO c_ratio cal
    else
        HRatio = ((UINT32) pIORes->IHRes << 20) / pIORes->OHRes;

    VRatio = ((UINT32) pIORes->IVRes << 20) / pIORes->OVRes;//TODO check any limit with NL SCL

    if(0xFF00000 < VRatio)
        VRatio = 0xFF00000;

    stSclCfg3.uCFG3_vratio = VRatio;
    stSclCfg4.uCFG4_hratio = HRatio;

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG3 , stSclCfg3.u32));
    vpp_obj->uiSclCfg3[UnitNum] = stSclCfg3.u32;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG4 , stSclCfg4.u32));

    stSclCfg6.u32 = 0;
    stSclCfg6.uCFG6_hsign_frac = 0x19;
    stSclCfg6.uCFG6_vsign_frac = stSclCfg6.uCFG6_hsign_frac;

    /* In BG4CT A0,
       Irrespective of Vertical scaling, whenever Horz downscale happens, hpos = 2
       Irrespective of Vertical scaling, whenever Horz upscale happens, hpos = 3
       But, whenever H scaling is 1:1, then Vertical decides hpos */
    if(1 == pSclCtrl->NLEn)
        stSclCfg6.uCFG6_hscalepos = 3;
    else if((pIORes->OHRes > pIORes->IHRes)
            || ((pIORes->OHRes == pIORes->IHRes) && (pIORes->OVRes > pIORes->IVRes))
           )
        stSclCfg6.uCFG6_hscalepos = 3;
    else if(pIORes->OHRes < pIORes->IHRes || ((pSclCtrl->leftCrop+pSclCtrl->rightCrop)!=0) //donot disable scalar, when cropping present in 1:1 case
            || ((pIORes->OHRes == pIORes->IHRes) && (pIORes->OVRes < pIORes->IVRes))
           )
        stSclCfg6.uCFG6_hscalepos = 2;
    else
    {
        stSclCfg6.uCFG6_hscalepos = 0;
    }


    stSclCfg6.uCFG6_hnl_enable = pSclCtrl->NLEn;
    stSclCfg6.uCFG6_rgb_swap   = 0;

    //LSB 2 bit for to crop at begining of line, MSB 2 bit to crop at end of line
    stSclCfg6.uCFG6_crop_ctrl = ((pSclCtrl->leftCrop&0x3)) | ((pSclCtrl->rightCrop&0x03)<<2);
    VDB_print("Scalar crop control : 0x%x\n", stSclCfg6.uCFG6_crop_ctrl);


    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG6, stSclCfg6.u32));
    vpp_obj->uiSclCfg6[UnitNum] = stSclCfg6.u32;

    stSclCfg8.uCFG8_htap_offset = 0;
    stSclCfg8.uCFG8_vtap_offset = 0;//TODO Interlaced output case value needs to be calculated
    stSclCfg8.uCFG8_fix_hlut_rd = 1;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG8, stSclCfg8.u32));
    vpp_obj->uiSclCfg8[UnitNum] = stSclCfg8.u32;

    /*-------------------
      -set SCL control register-
      --------------------*/
    /*if non linear scaling, H scaler is after V scaler*/
    if(0 == pSclCtrl->NLEn)
    {
        stSclCfg9.uCFG9_init_ratio = 0x100000;
        stSclCfg10.uCFG10_inc_ratio  = 0;
        stSclCfg11.uCFG11_nlcres   = 0;
    }
    else
    {
        VPP_SCL_CalcNLSclParams(pIORes, &scl_init_ratio, &scl_inc_ratio, &scl_nlcres);
        stSclCfg9.uCFG9_init_ratio = scl_init_ratio;
        stSclCfg10.uCFG10_inc_ratio  = scl_inc_ratio;
        stSclCfg11.uCFG11_nlcres   = scl_nlcres;
    }
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG9, stSclCfg9.u32));
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG10, stSclCfg10.u32));

    if(2 == stSclCfg6.uCFG6_hscalepos)
        hRes = pIORes->OHRes;
    else {
        //hRes = pIORes->IHRes;
        hRes = uncrop_IHRes;   //lsize calculation should be based on complete/uncropped resolution
    }
    TS = 4;
    stSclCfg11.uCFG11_lsize = (hRes/(TS + 1)) + ((hRes % (TS + 1)) ? 1 : 0);
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG11 , stSclCfg11.u32));


    stSclCfg13.u32=0;
    /* Prefetch mode Settings. Interlaced output prefetch is not working in BG5CT */
    if(SCAN_PROGRESSIVE == pResinfo->scan && (pIORes->IVRes > pIORes->OVRes) &&
            (((pIORes->IVRes - pIORes->OVRes)*100/pIORes->IVRes) <= 20))/* Tune the limit if required */
        stSclCfg13.uCFG13_pftch = 1;
    else
        stSclCfg13.uCFG13_pftch = 0;

    if((pIORes->IVRes == pIORes->OVRes) && (1 == vTaps))
        stSclCfg13.uCFG13_fifo_mode  = 1;
    else
        stSclCfg13.uCFG13_fifo_mode  = 0;

    /* 1:1 case or Small/Min ref/disp WIN with scalar cropping enabled then disable FIFO mode - Jira#16810*/
    if((IS_VPP_FRC_SCL_MINIMUM_WINDOW(pIORes->IHRes,pIORes->OHRes,pIORes->IVRes,pIORes->OVRes)) && ((pSclCtrl->leftCrop+pSclCtrl->rightCrop)!=0)) {
        stSclCfg13.uCFG13_fifo_mode  = 0;
    }


    memsize = stSclCfg11.uCFG11_lsize * vTaps - 1;
    if(memsize > 0x47F || (1 == stSclCfg13.uCFG13_fifo_mode))
        stSclCfg5.uCFG5_memsize = 0x47F;
    else
        stSclCfg5.uCFG5_memsize = memsize;



    stSclCfg5.uCFG5_vwrap   = 0;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG5 , stSclCfg5.u32));

    if((1 == stSclCfg13.uCFG13_fifo_mode)
            || (1 == stSclCfg13.uCFG13_pftch)
      )
    {
        if(pIORes->IHRes < pIORes->OHRes)
            minHRes = pIORes->IHRes;
        else
            minHRes = pIORes->OHRes;

        wls = (stSclCfg5.uCFG5_memsize + 1) / stSclCfg11.uCFG11_lsize;
        pls = ((stSclCfg5.uCFG5_memsize + 1) - (stSclCfg11.uCFG11_lsize * wls)) * 5;
        fcp = wls * minHRes + pls;
        stSclCfg13.uCFG13_fifo_depth = (stSclCfg13.uCFG13_pftch) ? fcp : (fcp - 100);
        if(0 == stSclCfg13.uCFG13_fifo_mode)
        {
            stSclCfg13.uCFG13_fifo_dfst  = 0;
        }
        else
        {
            stSclCfg13.uCFG13_fifo_dfst  = minHRes;
        }

    }
    else
    {
        stSclCfg13.uCFG13_fifo_mode  = 0;
        stSclCfg13.uCFG13_fifo_depth = 0;
        stSclCfg13.uCFG13_fifo_dfst  = 0;
    }
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG13 , stSclCfg13.u32));
    if(1 == stSclCfg13.uCFG13_pftch)
    {
        stSclCfg7.uCFG7_fstall = 0x0;
    }
    else
    {
        stSclCfg7.uCFG7_fstall = 0x80;
    }
    stSclCfg7.uCFG7_bstall = 0x80;

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG7, stSclCfg7.u32));

    stSclCfg12.u32 = 0;
    if((pIORes->OHRes > pIORes->IHRes &&  pIORes->OVRes < pIORes->IVRes) ||
            (pIORes->OHRes < pIORes->IHRes &&  pIORes->OVRes > pIORes->IVRes))
        stSclCfg12.uCFG12_xbstall_en    = 0;
    else if((1 == stSclCfg13.uCFG13_fifo_mode) ||
    (pIORes->OHRes > pIORes->IHRes && pIORes->OVRes > pIORes->IVRes)
           )
        stSclCfg12.uCFG12_xbstall_en    = 1;
    else
        stSclCfg12.uCFG12_xbstall_en    = 0;

    if((pIORes->OHRes - pIORes->IHRes) >= 30)// TODO Check for complition of programing(sign part)
        stSclCfg12.uCFG12_xbstall_dly   = 30;
    else
        stSclCfg12.uCFG12_xbstall_dly   = 0;
    if(1 == stSclCfg13.uCFG13_pftch)
    {
        stSclCfg12.uCFG12_ovf_margin = 0xA;
        stSclCfg12.uCFG12_udf_margin = 0x14;
    }
    else
    {
        stSclCfg12.uCFG12_ovf_margin = 0;
        stSclCfg12.uCFG12_udf_margin = 0;
    }
    if(0 == stSclCfg6.uCFG6_hscalepos)
        stSclCfg12.uCFG12_ovf_margin = 0;
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG12 , stSclCfg12.u32));

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
INT FRC_SCL_ReadSclCoeff(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8])
{
    INT uiRetVal = MV_VPP_OK;

    CHECK_FRC_SCL_NUM(UnitNum);

    VPPAPI_DBG_INFO(">>>> UnitNum = %d \n", UnitNum);
    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
        case VPP_FRC_SCL_DET:
        case VPP_FRC_SCL_VIP_Y:
        case VPP_FRC_SCL_VIP_UV:
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            VPP_SCL_MainCoeffRead(vpp_obj, UnitNum, hlut, vlut);
            break;
        case VPP_FRC_SCL_PIP:
        case VPP_FRC_SCL_OSD:
        case VPP_FRC_SCL_GFX2:
            VPP_SCL_GfxCoeffRead(vpp_obj, UnitNum, hlut, vlut);
            break;
        default:
            return MV_VPP_EBADPARAM;
            break;
    }

    return uiRetVal;
}
/***************************************************************************************
 * FUNCTION: Print scl coeff shadow used for writting
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_PrintSclCoeffShadow(VPP_OBJ *vpp_obj, INT UnitNum, INT16 (* hlut)[8], INT16 (* vlut)[8])
{
    INT uiRetVal = MV_VPP_OK;
    unsigned int *ptr = NULL;
    int sclLayer;
    int coeffMsk = 0;
    int coeffBitWidth = 0;
    int hLutFailCount = 0;
    int vLutFailCount = 0;
    static int hLutFailIndex[VPP_FRC_COEFFTAB_NUM_OF_ENTRIES][2];
    static int vLutFailIndex[VPP_FRC_COEFFTAB_NUM_OF_ENTRIES][2];
    INT i = 0,j = 0;


    CHECK_FRC_SCL_NUM(UnitNum);

    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
            sclLayer = 0;
            coeffMsk = MSK32ADPSCL_COEFF_CFG1_coeff0;
            coeffBitWidth = bADPSCL_COEFF_CFG1_coeff0;
            break;
        case VPP_FRC_SCL_DET:
            sclLayer = 5;
            coeffMsk = MSK32ADPSCL_COEFF_CFG1_coeff0;
            coeffBitWidth = bADPSCL_COEFF_CFG1_coeff0;
            break;
        case VPP_FRC_SCL_PIP:
            sclLayer = 0;
            coeffMsk = MSK32COEFF_CFG1_coeff0;
            coeffBitWidth = bCOEFF_CFG1_coeff0;
            break;
        case VPP_FRC_SCL_OSD:
            sclLayer = 1;
            coeffMsk = MSK32COEFF_CFG1_coeff0;
            coeffBitWidth = bCOEFF_CFG1_coeff0;
            break;
        case VPP_FRC_SCL_GFX2:
            sclLayer = 2;
            coeffMsk = MSK32COEFF_CFG1_coeff0;
            coeffBitWidth = bCOEFF_CFG1_coeff0;
            break;
        case VPP_FRC_SCL_VIP_Y:
            sclLayer = 1;
            coeffMsk = MSK32ADPSCL_COEFF_CFG1_coeff0;
            coeffBitWidth = bADPSCL_COEFF_CFG1_coeff0;
            break;
        case VPP_FRC_SCL_VIP_UV:
            sclLayer = 2;
            coeffMsk = MSK32ADPSCL_COEFF_CFG1_coeff0;
            coeffBitWidth = bADPSCL_COEFF_CFG1_coeff0;
            break;
        case VPP_FRC_SCL_OVP_Y:
            sclLayer = 3;
            coeffMsk = MSK32SCL1D_YC_COEFF_CFG1_coeff0;
            coeffBitWidth = bSCL1D_YC_COEFF_CFG1_coeff0;
            break;
        case VPP_FRC_SCL_OVP_UV:
            sclLayer = 4;
            coeffMsk = MSK32SCL1D_YC_COEFF_CFG1_coeff0;
            coeffBitWidth = bSCL1D_YC_COEFF_CFG1_coeff0;
            break;
        default:
            return MV_VPP_EBADPARAM;
    }


    ptr = vpp_obj->scl_coeffs[vpp_obj->hscl_coeff[UnitNum]];
    ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

    VPPAPI_DBG_INFO("Shadow BCM:Default:Read HLUT\n");
    VPPAPI_DBG_INFO("VPP_FRC_SCL_NUM_OF_PHASES = %d VPP_FRC_SCL_NUM_OF_COEFF =  %d \n", VPP_FRC_SCL_NUM_OF_PHASES, VPP_FRC_SCL_NUM_OF_COEFF);
    for(i=0; i < VPP_FRC_SCL_NUM_OF_PHASES; i++)
    {
        for (j = 0; j < VPP_FRC_SCL_NUM_OF_COEFF; j+=2)
        {
            if(hlut[i][j] != (*ptr & coeffMsk))
            {
                hLutFailIndex[hLutFailCount][0] = i;
                hLutFailIndex[hLutFailCount][1] = j;
                hLutFailCount++;
            }
            if(hlut[i][j+1] != ((*ptr >> coeffBitWidth) & coeffMsk))
            {
                hLutFailIndex[hLutFailCount][0] = i;
                hLutFailIndex[hLutFailCount][1] = j+1;
                hLutFailCount++;
            }
            VPPAPI_DBG_INFO("%4x:%4x %4x:%4x \n",
                *ptr & coeffMsk , hlut[i][j], (*ptr >> coeffBitWidth) & coeffMsk, hlut[i][j+1]);
            ptr = ptr + 2;
        }
        ptr = ptr + 6;
        VPPAPI_DBG_INFO("\n");
    }

    ptr = vpp_obj->scl_coeffs[vpp_obj->vscl_coeff[UnitNum]];
    ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

    VPPAPI_DBG_INFO("Shadow BCM:Default:Read VLUT\n");
    VPPAPI_DBG_INFO("VPP_FRC_SCL_NUM_OF_PHASES = %d VPP_FRC_SCL_NUM_OF_COEFF =  %d \n", VPP_FRC_SCL_NUM_OF_PHASES, VPP_FRC_SCL_NUM_OF_COEFF);
    //TODO:VPP_platypus_BRINGUP: Review VPP_FRC_SCL_NUM_OF_PHASES
    for(i=0; i < VPP_FRC_SCL_NUM_OF_PHASES; i++)
    {
        for (j = 0; j < VPP_FRC_SCL_NUM_OF_COEFF; j+=2)
        {
            if(vlut[i][j] != (*ptr & coeffMsk))
            {
                vLutFailIndex[vLutFailCount][0] = i;
                vLutFailIndex[vLutFailCount][1] = j;
                vLutFailCount++;
            }
            if(vlut[i][j+1] != ((*ptr >> coeffBitWidth) & coeffMsk))
            {
                vLutFailIndex[vLutFailCount][0] = i;
                vLutFailIndex[vLutFailCount][1] = j+1;
                vLutFailCount++;
            }
            VPPAPI_DBG_INFO("%4x:%4x %4x:%4x \n",
                    *ptr & coeffMsk, vlut[i][j], (*ptr >> coeffBitWidth) & coeffMsk, vlut[i][j+1]);
            ptr = ptr + 2;
        }
        ptr = ptr + 6;
        VPPAPI_DBG_INFO("\n");
    }

    VPPAPI_DBG_INFO("\n");
    if((0 != hLutFailCount) || (0 != vLutFailCount))
    {
        VPPAPI_DBG_INFO(" hLutFailCount = %d \n", hLutFailCount);
        for(j=0; j<hLutFailCount; j++)
        {
            VPPAPI_DBG_ERROR("(%d, %d) ",hLutFailIndex[j][0], hLutFailIndex[j][1]);
            if(0 == j%20)
                VPPAPI_DBG_INFO("\n");
        }
        VPPAPI_DBG_ERROR("\n");
        VPPAPI_DBG_ERROR(" vLutFailCount = %d \n", vLutFailCount);
        for(j=0; j<vLutFailCount; j++)
        {
            VPPAPI_DBG_ERROR("(%d, %d) ",vLutFailIndex[j][0], vLutFailIndex[j][1]);
            if(0 == j%20)
                VPPAPI_DBG_ERROR("\n");
        }
        VPPAPI_DBG_ERROR("\n");
        VPPAPI_DBG_ERROR("LUT programming FAILED\n");
    }
    else
    {
        VPPAPI_DBG_INFO("LUT programming PASS\n");
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
INT FRC_SCL_SetSclCoeff(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    int tempHorSclCoeffMode,tempVerSclCoeffMode;
    UINT32 uiRegAddr;
    INT uiRetVal = MV_VPP_OK;
    INT8 hTaps = -1, vTaps = -1;
    INT8 sclLayer = -1;
    T32ADPSCL_COEFF_CFG0 stAdpSclCoeffCfg0;
    T32COEFF_CFG0 stSclCoeffCfg0;
    UINT32 *ptr = NULL;
#ifdef VPP_TIME_PROFILE
    int low, high;
#endif

    CHECK_FRC_SCL_NUM(UnitNum);

    //All scalar calculation should be based on cropped resolution
    pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

    if (UnitNum == VPP_FRC_SCL_MAIN)
    {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINSCL_COEFF;
    }
#ifdef ENABLE_DETEE
    else if (UnitNum == VPP_FRC_SCL_DET)
    {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_DETSCL_COEFF;
    }
#endif
#ifdef ENABLE_PIP_GFX0_PATH
    else if (UnitNum == VPP_FRC_SCL_PIP)
    {
        uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_COEFF_GFX0;
    }
#endif
    else if (UnitNum == VPP_FRC_SCL_OSD)
    {
        uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_COEFF_GFX1;
    }
#ifdef ENABLE_GFX2_VOP2_PATH
    else if (UnitNum == VPP_FRC_SCL_GFX2)
    {
        uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_COEFF_GFX2;
    }
#endif
    else if (UnitNum == VPP_FRC_SCL_VIP_Y)
    {
        uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
    }
    else if(UnitNum == VPP_FRC_SCL_VIP_UV)
    {
        uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
    }
#ifdef ENABLE_OVPSCL
    else if (UnitNum == VPP_FRC_SCL_OVP_Y)
    {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
    }
    else if (UnitNum == VPP_FRC_SCL_OVP_UV)
    {
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
    }/* UnitNum already validated by CHECK_FRC_SCL_NUM(); so no need of else */
#endif
    else
    {
        return MV_VPP_EBADPARAM;
    }
    if(NULL != pIORes)
    {
        switch(UnitNum)
        {
            case VPP_FRC_SCL_MAIN:
            case VPP_FRC_SCL_DET:
            case VPP_FRC_SCL_VIP_Y:
            case VPP_FRC_SCL_VIP_UV:
            case VPP_FRC_SCL_OVP_Y:
            case VPP_FRC_SCL_OVP_UV:
                VPP_SCL_MainSclCalTaps(vpp_obj, UnitNum, pIORes, &hTaps, &vTaps);
                break;
            case VPP_FRC_SCL_PIP:
            case VPP_FRC_SCL_OSD:
            case VPP_FRC_SCL_GFX2:
                VPP_SCL_GfxSclCalTaps(vpp_obj, pIORes, &hTaps, &vTaps);
                break;
        }
    }

    //Revert back the modification done to 'pIORes->IHRes'
    pIORes->IHRes = pSclCtrl->uncrop_IHRes;

    if(!(pSclCtrl->ForceSel & MODE_COEFF_SEL_FORCE_H)) {
        if(NULL != pIORes)
        {
            if(VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_VIP_Y == UnitNum || VPP_FRC_SCL_VIP_UV == UnitNum ||
                 VPP_FRC_SCL_DET == UnitNum || VPP_FRC_SCL_OVP_Y == UnitNum || VPP_FRC_SCL_OVP_UV == UnitNum)
            {
                switch(hTaps)
                {
                    case 4:
                        tempHorSclCoeffMode = COEFF_H_MAIN_ADP_HLUT_CS;
                        break;
                    case 5:
                        tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_BLK5;
                        break;
                    case 8:
                        /* Use Adaptive scaler for upscaling of input resolution upto FHD. For other scaling use 1D scalar */
                        if((pIORes->OHRes < pIORes->IHRes) && (vpp_obj->bIsHDROutput == TRUE))
                            tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_PH8_SOFT;
                        else
                            tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_PH8;
                        break;
                    default:
                        tempHorSclCoeffMode = -1;
                        VPPAPI_DBG_ERROR("Unsupported HLut table \n");
                        return MV_VPP_EUNSUPPORT;
                }
            }
            else
            {
                switch(hTaps)
                {
                    case 5:
                        tempHorSclCoeffMode = COEFF_H_GFX_HLUT_BLK5_GFX;
                        break;
                    case 8:
                        tempHorSclCoeffMode = COEFF_H_GFX_HLUT_PV8;
                        break;
                    default:
                        tempHorSclCoeffMode = -1;
                        VPPAPI_DBG_ERROR("Unsupported HLut table \n");
                        return MV_VPP_EUNSUPPORT;
                }
            }
        }
        else
        {
            tempHorSclCoeffMode = -1;
        }
    } else
    {
        // force the user selection.
        tempHorSclCoeffMode = pSclCtrl->HSclMode;
        hTaps = 0;//FIXME
        VPPAPI_DBG_ERROR("Unsupported now. Needs prg updates \n");
        return MV_VPP_EUNSUPPORT;
    }

    if(!(pSclCtrl->ForceSel & MODE_COEFF_SEL_FORCE_V))
    {
        if(NULL != pIORes)
        {
            if(VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_VIP_Y == UnitNum || VPP_FRC_SCL_VIP_UV == UnitNum ||
                VPP_FRC_SCL_DET == UnitNum || VPP_FRC_SCL_OVP_Y == UnitNum || VPP_FRC_SCL_OVP_UV == UnitNum)
            {
                switch(vTaps)
                {
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
                        VPPAPI_DBG_ERROR("Unsupported VLut table \n");
                        return MV_VPP_EUNSUPPORT;
                }
            }
            else
            {
                switch(vTaps)
                {
                    case 1:
                        tempVerSclCoeffMode = COEFF_V_GFX_VLUT_1TAP;
                        break;
                    case 3:
                        tempVerSclCoeffMode = COEFF_V_GFX_VLUT_3TAP_CUBIC;
                        break;
                    case 4:
                        tempVerSclCoeffMode = COEFF_V_GFX_VLUT_CS_4TAP;
                        break;
                    default:
                        tempVerSclCoeffMode = -1;
                        VPPAPI_DBG_ERROR("Unsupported VLut table \n");
                        return MV_VPP_EUNSUPPORT;
                }
            }
        }
        else
        {
            tempVerSclCoeffMode = -1;
        }
    }
    else
    {
        // force the user selection.
        tempVerSclCoeffMode = pSclCtrl->VSclMode;
        vTaps = 0;//FIXME
        VPPAPI_DBG_ERROR("Unsupported now. Needs prg updates \n");
        return MV_VPP_EUNSUPPORT;
    }

    /* Calculating sclLayer of scl_coeffs table */
    if (UnitNum == VPP_FRC_SCL_MAIN || UnitNum == VPP_FRC_SCL_PIP)
        sclLayer = 0;
    else if (UnitNum == VPP_FRC_SCL_OSD || VPP_FRC_SCL_VIP_Y == UnitNum)
        sclLayer = 1;
    else if (VPP_FRC_SCL_VIP_UV == UnitNum || VPP_FRC_SCL_GFX2 == UnitNum)
        sclLayer = 2;
    else if (VPP_FRC_SCL_OVP_Y == UnitNum)
        sclLayer = 3;
    else if (VPP_FRC_SCL_OVP_UV == UnitNum)
        sclLayer = 4;
    else if (VPP_FRC_SCL_DET == UnitNum)
        sclLayer = 5;

    //TODO: VPP_platypus_BRINGUP: Review above sclLayer
#ifdef VPP_TIME_PROFILE
    ResetCycleCounter();
#endif

    if((-1 != tempHorSclCoeffMode) && (vpp_obj->hscl_coeff[UnitNum]!=tempHorSclCoeffMode ||  vpp_obj->hscl_coeff_update[UnitNum]==1))
    {

        vpp_obj->hscl_coeff[UnitNum] = tempHorSclCoeffMode;
        vpp_obj->hscl_coeff_update[UnitNum] = 0;
        ptr = vpp_obj->scl_coeffs[tempHorSclCoeffMode];
        ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;
        VPP_BCMBUF_WriteBlock(vpp_obj->pVbiBcmBuf, ptr, VPP_FRC_COEFFTAB_BCMBUF_SIZE);
    }

    if((-1 != tempVerSclCoeffMode) && (vpp_obj->vscl_coeff[UnitNum]!=tempVerSclCoeffMode || vpp_obj->vscl_coeff_update[UnitNum]==1))
    {

        vpp_obj->vscl_coeff[UnitNum] = tempVerSclCoeffMode;
        vpp_obj->vscl_coeff_update[UnitNum] = 0;
        ptr = vpp_obj->scl_coeffs[tempVerSclCoeffMode];
        ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;
        VPP_BCMBUF_WriteBlock(vpp_obj->pVbiBcmBuf, ptr, VPP_FRC_COEFFTAB_BCMBUF_SIZE);
    }

    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
        case VPP_FRC_SCL_DET:
        case VPP_FRC_SCL_VIP_Y:
        case VPP_FRC_SCL_VIP_UV:
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            stAdpSclCoeffCfg0.uCFG0_htap        = hTaps;
            stAdpSclCoeffCfg0.uCFG0_vtap        = vTaps;
            stAdpSclCoeffCfg0.uCFG0_coeff_index = 0;
            stAdpSclCoeffCfg0.uCFG0_coeff_hvsel = 0;
            stAdpSclCoeffCfg0.uCFG0_coeffread   = 0;
            stAdpSclCoeffCfg0.uCFG0_coeffload   = 0;
            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_COEFF_CFG0,
                        stAdpSclCoeffCfg0.u32));
            break;
        case VPP_FRC_SCL_PIP:
        case VPP_FRC_SCL_OSD:
        case VPP_FRC_SCL_GFX2:
            stSclCoeffCfg0.uCFG0_htaps       = hTaps;
            stSclCoeffCfg0.uCFG0_vtaps       = vTaps;
            stSclCoeffCfg0.uCFG0_coeff_index = 0;
            stSclCoeffCfg0.uCFG0_coeff_hvsel = 0;
            stSclCoeffCfg0.uCFG0_coeffload   = 0;
            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_COEFF_CFG0, stSclCoeffCfg0.u32));
            break;
    }
#ifdef VPP_TIME_PROFILE
    ReadCycleCounter(&low, &high);
    if (low > gTimeSclBCMWrite)
        gTimeSclBCMWrite = low;
#endif

    return MV_VPP_OK;
}

/***************************************************************************************
 * FUNCTION: Enable/Diable Scl clock gating
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               enable - enable/disable clock gate
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/


            //Don't Clock gate entire scalar- but instead disable all block expect the UPS at the input of scalar
            //Clock gating entire SCL including UPS will not work
            //Enable UPS at the input of scalar and disable rest of the scalar block
            //Please take of this again in SetSclParam



            /*TODO Needs to implement */

    //Do power gate, if SCL is bypassed/disabled
         /* Force coeff reload when SCL is enabled again, by making current index as invalid */

/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -poINTer to I&O resolution
 *               pSclCtrl - poINTer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT FRC_SCL_SetSclCtrlParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
{
    INT uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;

    if(VPP_FRC_SCL_MAIN == UnitNum)
    {
        /* Programing here to handle correct FIFO programming in inline-offline-offline switching and
           vice versa on all input cases */
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL3;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, pIORes->IHRes * pIORes->IVRes);
    }
    if(VPP_FRC_SCL_DET == UnitNum)
    {
        /* Programing here to handle correct FIFO programming in inline-offline-offline switching and
           vice versa on all input cases */
        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL11;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, pIORes->IHRes * pIORes->IVRes);

        uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL12;
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, uiRegAddr, pIORes->IHRes * pIORes->IVRes);
    }

    //Take backup of the uncropped resolution - so that IHRes can be reverted at later function calls also
    pSclCtrl->uncrop_IHRes = pIORes->IHRes;
    //All scalar calculation should be based on cropped resolution
    pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

    CHECK_FRC_SCL_NUM(UnitNum);
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_VPP_ENABLE_SCL_CLKGATE)
    if(IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE))
    {
        //Enable scalar or don't clock gate scalar, when only cropping required in 1:1 cases
        if((pIORes->IVRes == pIORes->OVRes) && (pSclCtrl->uncrop_IHRes == pIORes->OHRes))
        {
            uiRetVal = VPP_ClkGate_SCL(vpp_obj, UnitNum, 1);
     //       if(VPP_FRC_SCL_MAIN != UnitNum)
            {
                return uiRetVal;/* Stop prog as SCL clock gated except for MAIN SCALAR */
            }
        }
        else
        {
            CHECK_VPP_SCL_RETVAL(VPP_ClkGate_SCL(vpp_obj, UnitNum, 0));
        }
    }
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */

    VPPAPI_DBG_FIELD("%s %d  VPPDBG: pIORes =  %d %d %d %d UnitNum = %d vbi_num = %d \n", __func__, __LINE__, pIORes->IHRes, pIORes->IVRes,
            pIORes->OHRes, pIORes->OVRes , UnitNum, vpp_obj->dv[CPCB_1].vbi_num);

    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
        case VPP_FRC_SCL_DET:
        case VPP_FRC_SCL_VIP_Y:
        case VPP_FRC_SCL_VIP_UV:
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            CHECK_VPP_SCL_RETVAL(VPP_SCL_SetMainSclparams(vpp_obj, UnitNum, pIORes, pSclCtrl));
            break;
        case VPP_FRC_SCL_PIP:
        case VPP_FRC_SCL_OSD:
        case VPP_FRC_SCL_GFX2:
            CHECK_VPP_SCL_RETVAL(VPP_SCL_SetGfxSclparams(vpp_obj, UnitNum, pIORes, pSclCtrl));
            break;
        default:
            return MV_VPP_EBADPARAM;
            break;
    }

    //Revert back the modification done to 'pIORes->IHRes'
    pIORes->IHRes = pSclCtrl->uncrop_IHRes;

    /*load HV filter coefficiets*/
    if (pSclCtrl->DynamicLoad)
    {
        FRC_SCL_SetSclCoeff(vpp_obj, UnitNum, pIORes, pSclCtrl);
    }

    return uiRetVal;
}

INT VPP_SCL_SetMainSclScanParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
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

    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[CPCB_1]);
    //All scalar calculation should be based on cropped resolution
    pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);
    stSclCfg0.u32 = vpp_obj->uiSclCfg0[UnitNum];
    stSclCfg3.u32 = vpp_obj->uiSclCfg3[UnitNum];
    stSclCfg7.u32 = vpp_obj->uiSclCfg7[UnitNum];

    uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
    if(!uiRegAddr)
    {
        return MV_VPP_EBADPARAM;
    }
    VPP_SCL_MainSclAdp1dType(vpp_obj, pIORes, &sel_1D_adp);
    VPP_SCL_MainSclCalTaps(vpp_obj, UnitNum, pIORes, &hTaps, &vTaps);

    if(0 == sel_1D_adp) {
        stSclCfg0.uCFG0_even = 0x80;
        stSclCfg0.uCFG0_odd = 0x80;
        stSclCfg0.uCFG0_hinitph = 0x80;
    }else {
        stSclCfg0.uCFG0_even = (vTaps % 2)<<7;
        stSclCfg0.uCFG0_odd = (vTaps % 2)<<7;
        stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
    }
    stSclCfg7.uCFG7_vtap_offset = 0;


    if( SCAN_PROGRESSIVE != pResinfo->scan) { // interlaced output
        vinit_ph_even = (stSclCfg0.uCFG0_even<<12);
        vtap_offset_even = stSclCfg7.uCFG7_vtap_offset;
        vinit_ph_odd = vinit_ph_even + stSclCfg3.uCFG3_vratio/2;
        vtap_offset_odd = vtap_offset_even + ((vinit_ph_odd & 0xff00000) >> 20);

        if (vpp_obj->disptg_intl_count%2 == 0) {
            vinit_ph_cmodel = vinit_ph_even & 0xff000;
            vtap_offset_cmodel = vtap_offset_even % 16;
        }
        else { // odd/bottom_frame
            vinit_ph_cmodel = (vinit_ph_odd % 0x100000) & 0xff000;
            vtap_offset_cmodel = vtap_offset_odd % 16;
        }
        stSclCfg0.uCFG0_even = vinit_ph_cmodel>>12;
        stSclCfg0.uCFG0_odd = vinit_ph_cmodel>>12;
        stSclCfg7.uCFG7_vtap_offset = (vtap_offset_cmodel<<4) | vtap_offset_cmodel;
    }

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG0, stSclCfg0.u32));
    vpp_obj->uiSclCfg0[UnitNum] = stSclCfg0.u32;

    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG7, stSclCfg7.u32));
    vpp_obj->uiSclCfg7[UnitNum] = stSclCfg7.u32;

    //Revert back the modification done to 'pIORes->IHRes'
    pIORes->IHRes = pSclCtrl->uncrop_IHRes;

    return uiRetVal;
}


INT VPP_SCL_SetGfxSclScanParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pIORes, PVPP_SCL_CTRL pSclCtrl)
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

    pResinfo = VPP_GetResInfo(vpp_obj->uiResID[CPCB_1]);
    stSclCfg0.u32 = vpp_obj->uiSclCfg0[UnitNum];
    stSclCfg3.u32 = vpp_obj->uiSclCfg3[UnitNum];
    stSclCfg8.u32 = vpp_obj->uiSclCfg8[UnitNum];

    uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
    if(!uiRegAddr)
    {
        return MV_VPP_EBADPARAM;
    }
    //All scalar calculation should be based on cropped resolution
    pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

    VPP_SCL_GfxSclCalTaps(vpp_obj, pIORes, &hTaps, &vTaps);

    stSclCfg0.uCFG0_vinitph = (vTaps % 2)<<7;
    stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
    stSclCfg8.uCFG8_vtap_offset = 0;


    if( SCAN_PROGRESSIVE != pResinfo->scan) { // interlaced output
            vinit_ph_even = (stSclCfg0.uCFG0_vinitph<<12);
            vtap_offset_even = stSclCfg8.uCFG8_vtap_offset;
            vinit_ph_odd = vinit_ph_even + stSclCfg3.uCFG3_vratio/2;
            vtap_offset_odd = vtap_offset_even + ((vinit_ph_odd & 0xff00000) >> 20);

            if (vpp_obj->disptg_intl_count%2 == 0) {
                /*The odd/even programming is used internally by the scalar
                  to produce output better match with interlaced output.
                  the swap is done here (if compared to main case) is to fix a
                  line swap issue for interlaced output resolutions */
                vinit_ph_cmodel = vinit_ph_even & 0xff000;
                vtap_offset_cmodel = vtap_offset_even % 16;
            }
            else { // odd/bottom_frame
                vinit_ph_cmodel = (vinit_ph_odd % 0x100000) & 0xff000;
                vtap_offset_cmodel = vtap_offset_odd % 16;
            }
            stSclCfg0.uCFG0_vinitph = vinit_ph_cmodel>>12;
            stSclCfg8.uCFG8_vtap_offset = (vtap_offset_cmodel<<4) | vtap_offset_cmodel;
    }
    if(vpp_obj->uiSclCfg0[UnitNum] != stSclCfg0.u32) {
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG0, stSclCfg0.u32));
        vpp_obj->uiSclCfg0[UnitNum] = stSclCfg0.u32;
    }
    if(vpp_obj->uiSclCfg8[UnitNum] != stSclCfg8.u32) {
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG8, stSclCfg8.u32));
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
INT FRC_SCL_SetFrcParams(VPP_OBJ *vpp_obj, INT UnitNum, PVPP_SCL_RES pFrcRes)
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
    int cpcbID = CPCB_1, chanID = CHAN_MAIN;

    VPP_SCL_SclConfigForDeint(vpp_obj, UnitNum, pFrcRes, &enDeintSmallUpScale);
    CHECK_FRC_SCL_NUM(UnitNum);

    stSclFrcCfg1.u32=0;
    /*get register start address of the FRC_SCL unit */
    uiRegAddr = vpp_obj->base_addr;
    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_MAINSCL_FRC;
            chanID = CHAN_MAIN;
            break;
#ifdef ENABLE_DETEE
        case VPP_FRC_SCL_DET:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_DETSCL_FRC;
            break;
#endif
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FRC_SCL_PIP:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX0;
            chanID = CHAN_PIP;
            break;
#endif
        case VPP_FRC_SCL_OSD:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX1;
            chanID = CHAN_GFX1;
            break;
#ifdef ENABLE_GFX2_VOP2_PATH
        case VPP_FRC_SCL_GFX2:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX2;
            chanID = CHAN_GFX2;
            break;
#endif
        case VPP_FRC_SCL_VIP_Y:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC;
            break;
        case VPP_FRC_SCL_VIP_UV:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC;
            break;
#ifdef ENABLE_OVPSCL
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC;
            break;
#endif
        default:
            return MV_VPP_EBADPARAM;
            break;
    }
    cpcbID = CPCB_OF_CHAN(vpp_obj,chanID);
    if(cpcbID == CPCB_INVALID)
        cpcbID = CPCB_1;

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
    if((VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_DET == UnitNum) && (0 == vpp_obj->plane[PLANE_MAIN].frame_type) && (pFrcRes->IHRes <= pFrcRes->OHRes) && (pFrcRes->IHRes > 1280)&& (0 == enDeintSmallUpScale))
    {
        stSclFrcCfg0.uCFG0_sclclk_ctrl1 = 0x60;
    }
    else
    {
        stSclFrcCfg0.uCFG0_sclclk_ctrl1 = 0x80;
    }

    stFrcCtrl1_2.uchTreaLessReadCtrl = 0;
    stFrcCtrl1_2.uchTreaLessWriteCtrl = 0;
    stSclFrcCfg1.uCFG1_ctrl2 = stFrcCtrl1_2.uch8;
    stSclFrcCfg1.uCFG1_dly_frst_de = 0x14;

    if(VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_DET == UnitNum || VPP_FRC_SCL_VIP_Y == UnitNum || VPP_FRC_SCL_VIP_UV == UnitNum)
    {
        if(1 == enDeintSmallUpScale)
        {
            temp_uCFG1_dly_de_lrst = 0x0;
        }
        else if((pFrcRes->IVRes*1.0)/(pFrcRes->OVRes) > (5.5))
        {
            /* For bigger downscaling (Eg. 4K=>442x250,  4K=>449x250) chroma artifcats found in output.
               VIP UV Scaler tunning updated to fix those issue */
            temp_uCFG1_dly_de_lrst = (VPP_FRC_SCL_VIP_UV == UnitNum) ? 0x90 : 0x40;
        }
        /* Fix For exact 2x downscaling for 4K 600MHz */
        else if((pFrcRes->IHRes*1.0)/(pFrcRes->OHRes) > (6))
        {
            /* When down scale increases de_lrst should be increased  */
            temp_uCFG1_dly_de_lrst = 0x100;
        }
        else if((VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_DET == UnitNum)
                && (pFrcRes->IHRes < pFrcRes->OHRes) && (pFrcRes->IVRes > pFrcRes->OVRes))
        {
            /* Fix for H-Up and V-Down sclaing(Eg: 90/270Degree rotation) on BG4CDp A0
               (In A0 vppSysClk reduced from 660MHz to 600MHz) */
            temp_uCFG1_dly_de_lrst = 0x10;
        }
        else
        {
            temp_uCFG1_dly_de_lrst = 0xA0;
        }
    }
#ifdef ENABLE_OVPSCL
    else if (VPP_FRC_SCL_OVP_Y == UnitNum || VPP_FRC_SCL_OVP_UV == UnitNum)
    {
        temp_uCFG1_dly_de_lrst = 0x14;
    }
#endif
    else
    {
        if((pFrcRes->IHRes*1.0)/(pFrcRes->OHRes) > (24))
        {
           /* When down scale increases de_lrst should be increased  */
            temp_uCFG1_dly_de_lrst = 0x300;
        }
        else
        {
            temp_uCFG1_dly_de_lrst = 0x40;
        }
    }

    if(pFrcRes->OHRes > pFrcRes->IHRes)
        stSclFrcCfg1.uCFG1_dly_de_lrst = temp_uCFG1_dly_de_lrst;
    else if (1 == vpp_obj->scl_ctrl[UnitNum].NLEn)
        stSclFrcCfg1.uCFG1_dly_de_lrst = 0x200;
    else if((VPP_FRC_SCL_MAIN != UnitNum) && (VPP_FRC_SCL_VIP_Y != UnitNum) && (VPP_FRC_SCL_VIP_UV != UnitNum) && (vpp_obj->uiResID[cpcbID] >= RES_MIN_4Kx2K)&& (pFrcRes->OHRes < pFrcRes->IHRes) &&
            (0x100 == temp_uCFG1_dly_de_lrst) && (VPP_FRC_SCL_OVP_Y != UnitNum)  && (VPP_FRC_SCL_OVP_UV != UnitNum) && (VPP_FRC_SCL_DET != UnitNum))/* change only if temp_uCFG1_dly_de_lrst = 0x100(default) */
        stSclFrcCfg1.uCFG1_dly_de_lrst = 0x2C;
    else
        stSclFrcCfg1.uCFG1_dly_de_lrst = temp_uCFG1_dly_de_lrst;


    stSclFrcCfg2.uCFG2_bevres = pFrcRes->IVRes;
    stSclFrcCfg2.uCFG2_fevres = pFrcRes->OVRes;
    stSclFrcCfg2.uCFG2_dly_lrst_de = 0x14;//250;

    stSclFrcCfg3.uCFG3_behres = pFrcRes->IHRes;
    stSclFrcCfg3.uCFG3_clnt_ctrl = 3;
    stSclFrcCfg3.uCFG3_ctrl = 0;
    stSclFrcCfg3.uCFG3_rff_ctrl = 0;

    if(VPP_FRC_SCL_MAIN == UnitNum || VPP_FRC_SCL_VIP_Y == UnitNum || VPP_FRC_SCL_VIP_UV == UnitNum
        || VPP_FRC_SCL_DET == UnitNum || VPP_FRC_SCL_OVP_Y == UnitNum || VPP_FRC_SCL_OVP_UV == UnitNum)
    {
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_FRC_CFG0, stSclFrcCfg0.u32));
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_FRC_CFG1, stSclFrcCfg1.u32));
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_FRC_CFG2, stSclFrcCfg2.u32));
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_FRC_CFG3, stSclFrcCfg3.u32));
    }
    else
    {
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_FRC_CFG0, stSclFrcCfg0.u32));
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_FRC_CFG1, stSclFrcCfg1.u32));
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_FRC_CFG2, stSclFrcCfg2.u32));
        CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_FRC_CFG3, stSclFrcCfg3.u32));
    }

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
INT FRC_SCL_SetDeLrstDelay(VPP_OBJ *vpp_obj, INT UnitNum, UINT32 DeLrstDelay)
{
    UINT32 uiRegAddr;
    UINT32 uiRetVal = MV_VPP_OK;
    T32ADPSCL_FRC_CFG1 stSclFrcCfg1;

    CHECK_FRC_SCL_NUM(UnitNum);

    /*get register start address */
    uiRegAddr = vpp_obj->base_addr;
    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_MAINSCL_FRC + RA_ADPSCL_FRC_CFG1;
            break;
#ifdef ENABLE_DETEE
        case VPP_FRC_SCL_DET:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_DETSCL_FRC + RA_ADPSCL_FRC_CFG1;
            break;
#endif
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FRC_SCL_PIP:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX0 + RA_FRC_CFG1;
            break;
#endif
        case VPP_FRC_SCL_OSD:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX1 + RA_FRC_CFG1;
            break;
#ifdef ENABLE_GFX2_VOP2_PATH
        case VPP_FRC_SCL_GFX2:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX2 + RA_FRC_CFG1;
            break;
#endif
        case VPP_FRC_SCL_VIP_Y:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC + RA_ADPSCL_FRC_CFG1;
            break;
        case VPP_FRC_SCL_VIP_UV:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC + RA_ADPSCL_FRC_CFG1;
            break;
#ifdef ENABLE_OVPSCL
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG1;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG1;
            break;
#endif
        default:
            return MV_VPP_EBADPARAM;
            break;
    }

    stSclFrcCfg1.u32 = vpp_obj->uiSclFrcCfg1[UnitNum];

    stSclFrcCfg1.uCFG1_dly_de_lrst = DeLrstDelay;

    /*set it to BCM buffer*/
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr, stSclFrcCfg1.u32));

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
INT FRC_SCL_SetSclClk(VPP_OBJ *vpp_obj, INT UnitNum, UINT32 ClkVal)
{
    UINT32 uiRegAddr = 0;
    UINT32 uiRetVal = MV_VPP_OK;
    T32ADPSCL_FRC_CFG0 stSclFrcCfg0;

    CHECK_FRC_SCL_NUM(UnitNum);

    if(ClkVal > 128)
        return MV_VPP_EBADPARAM;

    /*get register start address */
    uiRegAddr = vpp_obj->base_addr;
    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_MAINSCL_FRC + RA_ADPSCL_FRC_CFG0;
            break;
#ifdef ENABLE_DETEE
        case VPP_FRC_SCL_DET:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_DETSCL_FRC + RA_ADPSCL_FRC_CFG0;
            break;
#endif
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FRC_SCL_PIP:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX0 + RA_FRC_CFG0;
            break;
#endif
        case VPP_FRC_SCL_OSD:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX1 + RA_FRC_CFG0;
            break;
#ifdef ENABLE_GFX2_VOP2_PATH
        case VPP_FRC_SCL_GFX2:
            uiRegAddr = vpp_obj->base_addr_gfx_vop + RA_VPP_GFX_VOP_FRC_GFX2 + RA_FRC_CFG0;
            break;
#endif
        case VPP_FRC_SCL_VIP_Y:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC + RA_ADPSCL_FRC_CFG0;
            break;
        case VPP_FRC_SCL_VIP_UV:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC + RA_ADPSCL_FRC_CFG0;
            break;
#ifdef ENABLE_OVPSCL
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG0;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr += RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG0;
            break;
#endif
        default:
            return MV_VPP_EBADPARAM;
            break;
    }

    stSclFrcCfg0.u32 = vpp_obj->uiSclFrcCfg0[UnitNum];

    stSclFrcCfg0.uCFG0_sclclk_ctrl = ClkVal;
    /*set it to BCM buffer*/
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr, stSclFrcCfg0.u32));

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
INT FRC_SCL_SetPhaseDda(VPP_OBJ *vpp_obj, VPP_FRC_SCL_NUM scalerId, INT hv, INT counter )
{
    UINT32 uiRegAddr = 0;
    UINT32 uiRetVal = MV_VPP_OK;
    T32ADPSCL_CFG0 stAdpSclCfg0;
    T32SCL_CFG0 stSclCfg0;

    /*get register start address */
    uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, scalerId);
    if(!uiRegAddr)
    {
        return MV_VPP_EBADPARAM;
    }
    switch(scalerId)
    {
        case VPP_FRC_SCL_MAIN:
        case VPP_FRC_SCL_DET:
        case VPP_FRC_SCL_VIP_Y:
        case VPP_FRC_SCL_VIP_UV:
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            stAdpSclCfg0.u32 = vpp_obj->uiSclCfg0[scalerId];
            if(hv == VPP_FRC_SCL_PHASE_HORIZONTAL)
                stAdpSclCfg0.uCFG0_hinitph = counter;
            else if(hv == VPP_FRC_SCL_PHASE_VERTICAL)
            {
                stAdpSclCfg0.uCFG0_even = counter;
                stAdpSclCfg0.uCFG0_odd  = counter;
            }
            else
                return (MV_VPP_EBADPARAM);

            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG0, stAdpSclCfg0.u32));
            vpp_obj->uiSclCfg0[scalerId] = stAdpSclCfg0.u32;
            break;
        case VPP_FRC_SCL_PIP:
        case VPP_FRC_SCL_OSD:
        case VPP_FRC_SCL_GFX2:
            stSclCfg0.u32 = vpp_obj->uiSclCfg0[scalerId];
            if(hv == VPP_FRC_SCL_PHASE_HORIZONTAL)
                stSclCfg0.uCFG0_hinitph = counter;
            else if(hv == VPP_FRC_SCL_PHASE_VERTICAL)
                stSclCfg0.uCFG0_vinitph = counter;
            else
                return (MV_VPP_EBADPARAM);

            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG0, stSclCfg0.u32));
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
INT FRC_SCL_ChopCtrl(VPP_OBJ *vpp_obj, INT UnitNum, INT Value)
{
    UINT32 uiRegAddr = 0;
    UINT32 uiRetVal = MV_VPP_OK;
    T32ADPSCL_CFG15 stSclCfg15;
    T32SCL_CFG6 stSclCfg6;

    /*get register start address */
    uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
    if(!uiRegAddr)
    {
        return MV_VPP_EBADPARAM;
    }
    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
        case VPP_FRC_SCL_DET:
        case VPP_FRC_SCL_VIP_Y:
        case VPP_FRC_SCL_VIP_UV:
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            stSclCfg15.u32 = vpp_obj->uiSclCfg15[UnitNum];
            stSclCfg15.uCFG15_crop  = Value;
            if(!(UnitNum==VPP_FRC_SCL_MAIN || VPP_FRC_SCL_DET == UnitNum))
                    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL1D_YC_CFG15 , stSclCfg15.u32));
            else
                    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG15 , stSclCfg15.u32));
            vpp_obj->uiSclCfg15[UnitNum] = stSclCfg15.u32;
            break;
        case VPP_FRC_SCL_PIP:
        case VPP_FRC_SCL_OSD:
        case VPP_FRC_SCL_GFX2:
            stSclCfg6.u32 = vpp_obj->uiSclCfg6[UnitNum];
            stSclCfg6.uCFG6_crop_ctrl  = Value;
            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_SCL_CFG6, stSclCfg6.u32));
            vpp_obj->uiSclCfg6[UnitNum] = stSclCfg6.u32;
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
INT FRC_SCL_SetOsdFmt(VPP_OBJ *vpp_obj, INT UnitNum, INT CscMode)
{
    return MV_VPP_OK;
}



static INT Generate_Scl_MainCoefficiena(VPP_OBJ *vpp_obj, int UnitNum, unsigned int **pptr, int hv, UINT16 (* coeffs)[VPP_FRC_SCL_NUM_OF_COEFF])
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

    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINSCL_COEFF;
            break;
#ifdef ENABLE_DETEE
        case VPP_FRC_SCL_DET:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_DETSCL_COEFF;
            break;
#endif
        case VPP_FRC_SCL_VIP_Y:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
        case VPP_FRC_SCL_VIP_UV:
            uiRegAddr = VPP_BASE_ADDRESS_VIPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
#ifdef ENABLE_OVPSCL
        case VPP_FRC_SCL_OVP_Y:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
        case VPP_FRC_SCL_OVP_UV:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
            break;
#endif
        default:
            return MV_VPP_EBADPARAM;
    }

    /*Coeff table loading*/
    for(i=0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++)
    {
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
static INT Generate_Scl_GfxCoefficiena(VPP_OBJ *vpp_obj, int UnitNum, unsigned int **pptr, int hv, UINT16 (* coeffs)[VPP_FRC_SCL_NUM_OF_COEFF])
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

    if((VPP_FRC_SCL_PIP != UnitNum) && (VPP_FRC_SCL_OSD != UnitNum) && (VPP_FRC_SCL_GFX2 != UnitNum))
        return uiRetVal;

    /*get register start address */
    uiRegAddr = vpp_obj->base_addr_gfx_vop;
    switch(UnitNum)
    {
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FRC_SCL_PIP:
            uiRegAddr += RA_VPP_GFX_VOP_COEFF_GFX0;
            break;
#endif
        case VPP_FRC_SCL_OSD:
            uiRegAddr += RA_VPP_GFX_VOP_COEFF_GFX1;
            break;
#ifdef ENABLE_GFX2_VOP2_PATH
        case VPP_FRC_SCL_GFX2:
            uiRegAddr += RA_VPP_GFX_VOP_COEFF_GFX2;
#endif
            break;
        default:
            return 0;
            break;
    }

    /*Coeff table loading*/
    for(i=0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++)
    {
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

static INT Generate_Scl_Coefficiena(VPP_OBJ *vpp_obj, int UnitNum, unsigned int **pptr, int hv, UINT16 (* coeffs)[VPP_FRC_SCL_NUM_OF_COEFF])
{
    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
        case VPP_FRC_SCL_DET:
        case VPP_FRC_SCL_VIP_Y:
        case VPP_FRC_SCL_VIP_UV:
        case VPP_FRC_SCL_OVP_Y:
        case VPP_FRC_SCL_OVP_UV:
            Generate_Scl_MainCoefficiena(vpp_obj, UnitNum, pptr, hv, coeffs);
            break;
        case VPP_FRC_SCL_PIP:
        case VPP_FRC_SCL_OSD:
        case VPP_FRC_SCL_GFX2:
            Generate_Scl_GfxCoefficiena(vpp_obj, UnitNum, pptr, hv, coeffs);
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
INT Customize_Coefficients(VPP_OBJ *vpp_obj, int coeffID, int hv, INT UnitNum, BOOL bCust)
{
    UINT16 (* coeffs)[VPP_FRC_SCL_NUM_OF_COEFF];
    unsigned int *ptr;
    int sclLayer = 0;

    /* Calculating sclLayer of scl_coeffs table */
    if (UnitNum == VPP_FRC_SCL_MAIN || UnitNum == VPP_FRC_SCL_PIP)
        sclLayer = 0;
    else if (UnitNum == VPP_FRC_SCL_OSD || UnitNum == VPP_FRC_SCL_VIP_Y)
        sclLayer = 1;
    else if (UnitNum == VPP_FRC_SCL_VIP_UV || VPP_FRC_SCL_GFX2 == UnitNum)
        sclLayer = 2;
    else if (UnitNum == VPP_FRC_SCL_OVP_Y)
        sclLayer = 3;
    else if (UnitNum == VPP_FRC_SCL_OVP_UV)
        sclLayer = 4;
    else if(VPP_FRC_SCL_DET == UnitNum)
        sclLayer = 5;
    else
        return MV_VPP_EBADPARAM;

    if(coeffID < FIRST_SCL_COEFF || coeffID >= COEFF_MAX)
        return MV_VPP_EBADPARAM;

    ptr = vpp_obj->scl_coeffs[coeffID];
    ptr += VPP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

    if(bCust == TRUE)
    {
        VPPAPI_DBG_INFO(" sclLayer=%d, coeffID=%d,addr=0x%08X \n", sclLayer, coeffID,vpp_obj->cust_scl_coeffs[sclLayer][coeffID] );
        if(vpp_obj->cust_sc_bPrepared[sclLayer][coeffID])
        {
            coeffs = (UINT16 (*)[VPP_FRC_SCL_NUM_OF_COEFF]) vpp_obj->cust_scl_coeffs[sclLayer][coeffID];
            vpp_obj->cust_sc_bLoaded[sclLayer][coeffID] = TRUE;
            VPPAPI_DBG_INFO("sclLayer=%d, coeffID=%d, value=%d\n", sclLayer, coeffID, vpp_obj->cust_sc_bLoaded[sclLayer][coeffID]);
        }
        else
            return MV_VPP_EUNCONFIG;
    }
    else
    {
        coeffs=(UINT16 (*)[VPP_FRC_SCL_NUM_OF_COEFF])default_index_coeff_table[coeffID];
        vpp_obj->cust_sc_bLoaded[sclLayer][coeffID] = FALSE;
    }
    Generate_Scl_Coefficiena(vpp_obj, UnitNum, &ptr, hv, coeffs);
    VPP_SHM_CleanCache(vpp_obj->scl_coeffs_handle[coeffID], (VPP_FRC_COEFFTAB_BCMBUF_SIZE * sclLayer), VPP_FRC_COEFFTAB_BCMBUF_SIZE);


    return MV_VPP_OK;
}

static INT FRC_SCL_GenerateMainAdpCoeffTabHV(VPP_OBJ *vpp_obj, INT UnitNum, INT16 coeffID, INT16 hvSel, INT format, const INT16 (* lut)[4])
{
    INT16 cf1[32][4],cf2;
    INT16 i,j;
    INT uiRetVal = MV_VPP_OK;


    /* Convert INTo sign magnitude */
    for(i=0;i<32;i++)
    {
        for(j=0;j<4;j++)
        {
            cf2 = (lut[i][3-j])&0x800;
            cf1[i][j] = (lut[i][3-j])&0x7ff;
            cf1[i][j] = (cf2 ? -cf1[i][j] : cf1[i][j])&0x7ff;
            cf1[i][j] = ((cf2 | cf1[i][j])&0xfff);
        }
    }
    /*Coeff table loading*/
    for(i=0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++)
    {
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

static INT FRC_SCL_GenerateMain1DCoeffTabH(VPP_OBJ *vpp_obj, INT UnitNum, INT16 coeffID, INT16 hvSel, INT format, const INT16 (* hlut)[8])
{
    INT16 hcf1[32][8],hcf2;
    INT16 i,j;
    INT uiRetVal = MV_VPP_OK;

    /* Convert INTo sign magnitude */
    for(i=0; i<32; i++)
    {
        for(j=0; j<8; j++)
        {
            hcf2 = (hlut[i][7-j])&0x800;
            hcf1[i][j] = (hlut[i][7-j])&0x7ff;
            hcf1[i][j] = (hcf2 ? -hcf1[i][j] : hcf1[i][j])&0x7ff;
            hcf1[i][j] = ((hcf2 | hcf1[i][j])&0xfff);
        }
}

    /*Horizontal Coeff table loading*/
    for(i=0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++)
    {
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

static INT FRC_SCL_GenerateMain1DCoeffTabV(VPP_OBJ *vpp_obj, INT UnitNum, INT16 coeffID, INT16 hvSel, INT format, const INT16 (* vlut)[6])
{
    INT16 vcf1[32][8],vcf2;
    INT16 i,j;
    INT uiRetVal = MV_VPP_OK;
    INT vc[6];
    INT16 vTaps;
    /* Convert INTo sign magnitude */
    for(i=0; i<32; i++)
    {
        for(j=0; j<6; j++)
        {
            vcf2 = (vlut[i][5-j])&0x800;
            vcf1[i][j] = (vlut[i][5-j])&0x7ff;
            vcf1[i][j] = (vcf2 ? -vcf1[i][j] : vcf1[i][j])&0x7ff;
            vcf1[i][j] = ((vcf2 | vcf1[i][j])&0xfff);
        }
     }

    switch(coeffID)
    {
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
    for(i=0;i<6;i++)
    {
        vc[i] = 6 +i - vTaps;
        if(vc[i]>=6)
            vc[i]=0;
    }

    /*Vertical Coeff table loading*/
    for(i=0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++)
    {
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

static INT FRC_SCL_GenerateGfxCoeffTabHV(VPP_OBJ *vpp_obj, INT UnitNum, INT16 coeffID, INT16 hvSel, INT format, const INT16 (* lut)[8])
{
    INT16 cf1[17][8],cf2;
    INT16 i,j;
    INT uiRetVal = MV_VPP_OK;

    if((VPP_FRC_SCL_PIP != UnitNum) && (VPP_FRC_SCL_OSD != UnitNum) && (VPP_FRC_SCL_GFX2 != UnitNum))
        return MV_VPP_EBADPARAM;

    /* Convert INTo sign magnitude */
    for(i=0; i<=16; i++)
    {
        for(j=0; j<8; j++)
        {
            cf2 = (lut[i][j])&0x200;
            cf1[i][j] = (lut[i][j])&0x1ff;
            cf1[i][j] = (cf2 ? -cf1[i][j] : cf1[i][j])&0x1ff;
            cf1[i][j] = ((cf2 | cf1[i][j])&0x3ff);
        }
    }

    /*Horizontal Coeff table loading*/
    for(i=0; i<VPP_FRC_SCL_NUM_OF_PHASES; i++)
    {
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


INT FRC_SCL_GenerateCoeffTables(VPP_OBJ *vpp_obj)
{

    FRC_SCL_GenerateMainAdpCoeffTabHV(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_H_MAIN_ADP_HLUT_CS, 1, 0x1A, hlut_cs);
    FRC_SCL_GenerateMainAdpCoeffTabHV(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_ADP_HLUT_BS, 0, 0x1A, hlut_bs);

    FRC_SCL_GenerateMain1DCoeffTabH(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_H_MAIN_1D_HLUT_BLK5, 1, 0x1A, hlut_blk5);
    FRC_SCL_GenerateMain1DCoeffTabH(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_H_MAIN_1D_HLUT_PH8, 1, 0x1A, hlut_ph8);
    FRC_SCL_GenerateMain1DCoeffTabH(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_H_MAIN_1D_HLUT_PH8_SOFT, 1, 0x1A, hlut_ph8_soft);
    FRC_SCL_GenerateMain1DCoeffTabV(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_DBG1, 0, 0x0B, vlut_dbg1);
    FRC_SCL_GenerateMain1DCoeffTabV(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_2TAP, 0, 0x0B, vlut_2tap);
    FRC_SCL_GenerateMain1DCoeffTabV(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 0, 0x1A, vlut_3tap_cubic_1DScl);
    FRC_SCL_GenerateMain1DCoeffTabV(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_BLK5, 0, 0x1A, vlut_blk5);
    FRC_SCL_GenerateMain1DCoeffTabV(vpp_obj, VPP_FRC_SCL_MAIN, COEFF_V_MAIN_1D_VLUT_6TAP, 0, 0x1A, vlut_6tap);

    FRC_SCL_GenerateGfxCoeffTabHV(vpp_obj, VPP_FRC_SCL_PIP, COEFF_H_GFX_HLUT_BLK5_GFX, 1, 0x19, hlut_blk5_Gfx);
    FRC_SCL_GenerateGfxCoeffTabHV(vpp_obj, VPP_FRC_SCL_PIP, COEFF_H_GFX_HLUT_PV8, 1, 0x19, hlut_pv8);
    FRC_SCL_GenerateGfxCoeffTabHV(vpp_obj, VPP_FRC_SCL_PIP, COEFF_V_GFX_VLUT_CS_4TAP, 0, 0x19, vlut_cs_4tap);
    FRC_SCL_GenerateGfxCoeffTabHV(vpp_obj, VPP_FRC_SCL_PIP, COEFF_V_GFX_VLUT_3TAP_CUBIC, 0, 0x19, vlut_3tap_cubic);
    FRC_SCL_GenerateGfxCoeffTabHV(vpp_obj, VPP_FRC_SCL_PIP, COEFF_V_GFX_VLUT_1TAP, 0, 0x19, vlut_1tap);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_ADP_HLUT_CS, 0, VPP_FRC_SCL_MAIN, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_ADP_HLUT_BS, 1, VPP_FRC_SCL_MAIN, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_BLK5, 0, VPP_FRC_SCL_MAIN, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8, 0, VPP_FRC_SCL_MAIN, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8_SOFT, 0, VPP_FRC_SCL_MAIN, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_DBG1, 1, VPP_FRC_SCL_MAIN, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_2TAP, 1, VPP_FRC_SCL_MAIN, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 1, VPP_FRC_SCL_MAIN, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_BLK5, 1, VPP_FRC_SCL_MAIN, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_6TAP, 1, VPP_FRC_SCL_MAIN, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_ADP_HLUT_CS, 0, VPP_FRC_SCL_DET, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_ADP_HLUT_BS, 1, VPP_FRC_SCL_DET, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_BLK5, 0, VPP_FRC_SCL_DET, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8, 0, VPP_FRC_SCL_DET, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_DBG1, 1, VPP_FRC_SCL_DET, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_2TAP, 1, VPP_FRC_SCL_DET, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 1, VPP_FRC_SCL_DET, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_BLK5, 1, VPP_FRC_SCL_DET, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_6TAP, 1, VPP_FRC_SCL_DET, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_ADP_HLUT_CS, 0, VPP_FRC_SCL_VIP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_ADP_HLUT_BS, 1, VPP_FRC_SCL_VIP_Y, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_BLK5, 0, VPP_FRC_SCL_VIP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8, 0, VPP_FRC_SCL_VIP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8_SOFT, 0, VPP_FRC_SCL_VIP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_DBG1, 1, VPP_FRC_SCL_VIP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_2TAP, 1, VPP_FRC_SCL_VIP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 1, VPP_FRC_SCL_VIP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_BLK5, 1, VPP_FRC_SCL_VIP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_6TAP, 1, VPP_FRC_SCL_VIP_Y, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_ADP_HLUT_CS, 0, VPP_FRC_SCL_VIP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_ADP_HLUT_BS, 1, VPP_FRC_SCL_VIP_UV, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_BLK5, 0, VPP_FRC_SCL_VIP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8, 0, VPP_FRC_SCL_VIP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8_SOFT, 0, VPP_FRC_SCL_VIP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_DBG1, 1, VPP_FRC_SCL_VIP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_2TAP, 1, VPP_FRC_SCL_VIP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 1, VPP_FRC_SCL_VIP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_BLK5, 1, VPP_FRC_SCL_VIP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_6TAP, 1, VPP_FRC_SCL_VIP_UV, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_ADP_HLUT_CS, 0, VPP_FRC_SCL_OVP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_ADP_HLUT_BS, 1, VPP_FRC_SCL_OVP_Y, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_BLK5, 0, VPP_FRC_SCL_OVP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8, 0, VPP_FRC_SCL_OVP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_DBG1, 1, VPP_FRC_SCL_OVP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_2TAP, 1, VPP_FRC_SCL_OVP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 1, VPP_FRC_SCL_OVP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_BLK5, 1, VPP_FRC_SCL_OVP_Y, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_6TAP, 1, VPP_FRC_SCL_OVP_Y, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_ADP_HLUT_CS, 0, VPP_FRC_SCL_OVP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_ADP_HLUT_BS, 1, VPP_FRC_SCL_OVP_UV, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_BLK5, 0, VPP_FRC_SCL_OVP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_MAIN_1D_HLUT_PH8, 0, VPP_FRC_SCL_OVP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_DBG1, 1, VPP_FRC_SCL_OVP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_2TAP, 1, VPP_FRC_SCL_OVP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL, 1, VPP_FRC_SCL_OVP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_BLK5, 1, VPP_FRC_SCL_OVP_UV, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_MAIN_1D_VLUT_6TAP, 1, VPP_FRC_SCL_OVP_UV, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_GFX_HLUT_BLK5_GFX, 0, VPP_FRC_SCL_PIP, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_GFX_HLUT_PV8, 0, VPP_FRC_SCL_PIP, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_CS_4TAP, 1, VPP_FRC_SCL_PIP, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_3TAP_CUBIC, 1, VPP_FRC_SCL_PIP, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_1TAP, 1, VPP_FRC_SCL_PIP, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_GFX_HLUT_BLK5_GFX, 0, VPP_FRC_SCL_OSD, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_GFX_HLUT_PV8, 0, VPP_FRC_SCL_OSD, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_CS_4TAP, 1, VPP_FRC_SCL_OSD, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_3TAP_CUBIC, 1, VPP_FRC_SCL_OSD, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_1TAP, 1, VPP_FRC_SCL_OSD, FALSE);

    Customize_Coefficients(vpp_obj, COEFF_H_GFX_HLUT_BLK5_GFX, 0, VPP_FRC_SCL_GFX2, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_H_GFX_HLUT_PV8, 0, VPP_FRC_SCL_GFX2, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_CS_4TAP, 1, VPP_FRC_SCL_GFX2, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_3TAP_CUBIC, 1, VPP_FRC_SCL_GFX2, FALSE);
    Customize_Coefficients(vpp_obj, COEFF_V_GFX_VLUT_1TAP, 1, VPP_FRC_SCL_GFX2, FALSE);

    return MV_VPP_OK;
}

INT FRC_SCL_Set_CSwap(VPP_OBJ *vpp_obj, INT UnitNum, INT iEnable)
{
    UINT32 uiRegAddr = 0;
    UINT32 uiRetVal = 0;
    T32ADPSCL_CFG6 stSclCfg6;
    stSclCfg6.u32 = vpp_obj->uiSclCfg6[UnitNum];
    stSclCfg6.uCFG6_ups_cswap = iEnable;

    uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);/* Following this are SCL programming */
    if(!uiRegAddr)
    {
        return MV_VPP_EBADPARAM;
    }
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG6, stSclCfg6.u32));
    vpp_obj->uiSclCfg6[UnitNum] = stSclCfg6.u32;

    return MV_VPP_OK;
}
