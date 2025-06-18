/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_FE_PATGEN_C_


#include "com_type.h"
#include "vpp_com.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_regs.h"
#include "vpp_bcmbuf.h"

#include "vpp.h"

#include "vpp_fe_patgen.h"
#include "vpp_fe_patgenprv.h"

/*-----------------------------------------------------------------------------
 * Description  : This function sets the default configuration values to the
 *              : hardware registers
 * Arguments    : None
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT FE_PATGEN_SetDefaults(VPP_OBJ* vpp_obj)
{
#ifdef FIXME_IN_BG4CT
    /* load default value for patgen */
    WRITE_REG_DEFAULT_VAL(PAT_HCNTL, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_VCNTL, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_HPITH, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_VPITH, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_LCOLHP, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_LCOLVP, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_LFSEED, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UCOLHP_0, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UCOLHP_1, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UCOLHP_2, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UCOLVP_0, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UCOLVP_1, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UCOLVP_2, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UFSEED_0, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UFSEED_1, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UFSEED_2, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IHRES_L, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IHRES_H, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IVRES_L, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IVRES_H, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IHTOT_L, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IHTOT_H, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IVTOT_L, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IVTOT_H, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IHSW_L, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IHSW_H, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IVSW_L, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IVSW_H, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IHST_L, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IHST_H, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IVST_L, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_IVST_H, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UFSEED1_0, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UFSEED1_1, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_UFSEED1_2, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_LFSEED1, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_MODE, gPatgenDefValues);
    WRITE_REG_DEFAULT_VAL(PAT_RFF, gPatgenDefValues);
#endif
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets patgen timing information
 *              :
 * Arguments    : pTiming - patgen timing information
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT FE_PATGEN_SetTiming(VPP_OBJ* vpp_obj, VPP_PATGEN_TIMING *pTiming)
{
#ifdef FIXME_IN_BG4CT
    /* H total */
    WRITE_PATGEN_REG(PAT_IHTOT_L, pTiming->hTot & 0x0ff);
    WRITE_PATGEN_REG(PAT_IHTOT_H, (pTiming->hTot >> 8) & 0x0f);

    /* V total */
    WRITE_PATGEN_REG(PAT_IVTOT_L, pTiming->vTot & 0x0ff);
    WRITE_PATGEN_REG(PAT_IVTOT_H, (pTiming->vTot >> 8) & 0x0f);

    /* H active resolution */
    WRITE_PATGEN_REG(PAT_IHRES_L, pTiming->hRes & 0x0ff);
    WRITE_PATGEN_REG(PAT_IHRES_H, (pTiming->hRes >> 8) & 0x0f);

    /* V active resolution */
    WRITE_PATGEN_REG(PAT_IVRES_L, pTiming->vRes & 0x0ff);
    WRITE_PATGEN_REG(PAT_IVRES_H, (pTiming->vRes >> 8) & 0x0f);

    /* H sync pulse width */
    WRITE_PATGEN_REG(PAT_IHSW_L, pTiming->hSw & 0x0ff);
    WRITE_PATGEN_REG(PAT_IHSW_H, (pTiming->hSw >> 8) & 0x0f);

    /* V sync pulse width */
    WRITE_PATGEN_REG(PAT_IVSW_L, pTiming->vSw & 0x0ff);
    WRITE_PATGEN_REG(PAT_IVSW_H, (pTiming->vSw >> 8) & 0x0f);

    /* H active start */
    WRITE_PATGEN_REG(PAT_IHST_L, pTiming->hSt & 0x0ff);
    WRITE_PATGEN_REG(PAT_IHST_H, (pTiming->hSt >> 8) & 0x0f);

    /* V active start */
    WRITE_PATGEN_REG(PAT_IVST_L, pTiming->vSt & 0x0ff);
    WRITE_PATGEN_REG(PAT_IVST_H, (pTiming->vSt >> 8) & 0x0f);
#endif
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets patgen horizontal and vertical mode
 *              :
 * Arguments    : hmodeID - horizontal mode ID
 *              : vmodeID - vertical mode ID
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT FE_PATGEN_SetMode(VPP_OBJ* vpp_obj, int hmodeID, int vmodeID)
{
#ifdef FIXME_IN_BG4CT
    /* pattern horizontal mode */
    WRITE_PATGEN_REG(PAT_HCNTL, hmodeID);

    /* pattern vertical mode */
    WRITE_PATGEN_REG(PAT_VCNTL, vmodeID);
#endif
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets patgen data control
 *              :
 * Arguments    : pDataCtrl - data control
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT FE_PATGEN_SetDataCtrl(VPP_OBJ* vpp_obj, VPP_PATGEN_DATA_CTRL *pDataCtrl)
{
#ifdef FIXME_IN_BG4CT
    /* pattern horizontal pitch */
    WRITE_PATGEN_REG(PAT_HPITH, pDataCtrl->hPith);

    /* pattern vertical pitch */
    WRITE_PATGEN_REG(PAT_VPITH, pDataCtrl->vPith);

    /* pattern horizontal color pitch */
    WRITE_PATGEN_REG(PAT_UCOLHP_0, (pDataCtrl->hColPith & 0x0ff)); // R
    WRITE_PATGEN_REG(PAT_UCOLHP_1, ((pDataCtrl->hColPith >> 8) & 0x0ff)); // G
    WRITE_PATGEN_REG(PAT_UCOLHP_2, ((pDataCtrl->hColPith >> 16) & 0x0ff)); // B

    /* pattern vertical color pitch */
    WRITE_PATGEN_REG(PAT_UCOLVP_0, (pDataCtrl->vColPith & 0x0ff));
    WRITE_PATGEN_REG(PAT_UCOLVP_1, ((pDataCtrl->vColPith >> 8) & 0x0ff));
    WRITE_PATGEN_REG(PAT_UCOLVP_2, ((pDataCtrl->vColPith >> 16) & 0x0ff));

    /* pattern frame-1 color seed */
    WRITE_PATGEN_REG(PAT_UFSEED_0, (pDataCtrl->ColSeed1 & 0x0ff));
    WRITE_PATGEN_REG(PAT_UFSEED_1, ((pDataCtrl->ColSeed1 >> 8) & 0x0ff));
    WRITE_PATGEN_REG(PAT_UFSEED_2, ((pDataCtrl->ColSeed1 >> 16) & 0x0ff));

    /* pattern frame-2 color seed */
    WRITE_PATGEN_REG(PAT_UFSEED1_0, (pDataCtrl->ColSeed2 & 0x0ff));
    WRITE_PATGEN_REG(PAT_UFSEED1_1, ((pDataCtrl->ColSeed2 >> 8) & 0x0ff));
    WRITE_PATGEN_REG(PAT_UFSEED1_2, ((pDataCtrl->ColSeed2 >> 16) & 0x0ff));
#endif
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function sets patgen data control
 *              :
 * Arguments    : type - 0: progressive, 1: interlaced
 *              : cadence - 0: normal, 1: 3-2 cadence, 2: 2-2 cadence
 *              : rff - repeat field flag
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT FE_PATGEN_SetFrameType(VPP_OBJ* vpp_obj, int type, int cadence, int rff)
{
#ifdef FIXME_IN_BG4CT
    int val;

    /* pattern input type control */
    val = type & 0x01;
    val |= ((cadence & 0x03) << 1);
    WRITE_PATGEN_REG(PAT_MODE, val);

    /* repeat filed flag in cadence */
    WRITE_PATGEN_REG(PAT_RFF, rff);
#endif
    return (MV_VPP_OK);
}

/*-----------------------------------------------------------------------------
 * Description  : This function enable/disable patgen
 *              :
 * Arguments    : enable - 1: enable, 0: disable
 * Returns      : MV_VPP_OK on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
INT FE_PATGEN_Enable(VPP_OBJ* vpp_obj, int flag)
{
#ifdef FIXME_IN_BG4CT
    if (flag) {
        VPP_REG_WRITE32(RA_Vpp_FE_PAT_SEL, 1);
        VPP_REG_WRITE32(RA_Vpp_PAT_DNS_CTRL, 0x1800);
    } else {
        VPP_REG_WRITE32_RAW(RA_Vpp_LDR + RA_LDR_MAIN + RA_PLANE_CLEAR, 1);
        VPP_REG_WRITE32_RAW(RA_Vpp_LDR + RA_LDR_MAIN + RA_PLANE_CLEAR, 0);
        VPP_REG_WRITE32(RA_Vpp_FE_PAT_SEL, 0);
    }
#endif
    return (MV_VPP_OK);
}


