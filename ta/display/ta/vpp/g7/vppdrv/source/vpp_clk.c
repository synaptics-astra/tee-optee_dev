/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
////! \file vpp_ta.c
////!
////! \brief Implementation of VPP_Test_APP functionalities for VPP_SW_BRINGUP.
////!
////! Purpose: Some of the VPP_Test_APP functionality requires register access from
////! user space. These functionality are sequenced through existing TA calls
////!
////!     Version    Date                     Author
////!     V 1.00,    Dec 11 2019,             Vishakh M
////!
////! Note:
//////////////////////////////////////////////////////////////////////////////////

#include "avio_memmap.h"
#include "avioGbl.h"
#include "Galois_memmap.h"
#include "vpp_api.h"
#include "vpp.h"
#include "vpp_clk.h"
#include "galois_io.h"
#include "OSAL_api.h"
#include "global.h"

void MV_VPP_Clk_Ctrl()
{
    GA_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_VPLL_WRAP0+RA_VPLL_WRAP_VPLL_CTRL, 0x820);
#ifdef RA_avioGbl_VPLL_WRAP1
    GA_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_VPLL_WRAP1+RA_VPLL_WRAP_VPLL_CTRL, 0x820);
#endif
    GA_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_AVPLLA_CLK_EN, 0x4D);
    GA_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+RA_avioGbl_AVPLL_CTRL0, 0x0);
}

void MV_VPP_HDMIPhyClkDiv(int div)
{
#ifdef BA_VPLL_WRAP_VPLL_CTRL_HDMIPHYPCLK_DIV2
    T32VPLL_WRAP_VPLL_CTRL vpllctrl;
    unsigned int addr = MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE+
                            RA_avioGbl_VPLL_WRAP0+RA_VPLL_WRAP_VPLL_CTRL;

    GA_REG_WORD32_READ(addr, &vpllctrl.u32);
    vpllctrl.uVPLL_CTRL_HDMIPHYPCLK_DIV2 = div;
    GA_REG_WORD32_WRITE(addr, vpllctrl.u32);
#endif
}

static void pll_save_context(UINT32 *regs_val, UINT32 pll_base)
{
    GA_REG_WORD32_READ(pll_base + RA_abipll_ctrlA, regs_val + CTRLA_BK);
    /* save ssc reg */
    GA_REG_WORD32_READ(pll_base + RA_abipll_ctrlB, regs_val + CTRLB_BK);
    /* save vco related regs */
    GA_REG_WORD32_READ(pll_base + RA_abipll_ctrlC, regs_val + CTRLC_BK);
    GA_REG_WORD32_READ(pll_base + RA_abipll_ctrlD, regs_val + CTRLD_BK);
    GA_REG_WORD32_READ(pll_base + RA_abipll_ctrlE, regs_val + CTRLE_BK);
    /* save dp regs */
    GA_REG_WORD32_READ(pll_base + RA_abipll_ctrlF, regs_val + CTRLF_BK);
    GA_REG_WORD32_READ(pll_base + RA_abipll_ctrlG, regs_val + CTRLG_BK);
}

static void pll_restore(UINT32 *regs_val, UINT32 pll_base)
{
    T32abipll_ctrlA ctrlA;

    ctrlA.u32 = regs_val[CTRLA_BK];
    /* Enable Reset and Bypass */
    ctrlA.uctrlA_BYPASS = 1;
    ctrlA.uctrlA_RESET = 1;
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlA, ctrlA.u32);
    /* restore vco related regs */
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlC, regs_val[CTRLC_BK]);
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlD, regs_val[CTRLD_BK]);
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlE, regs_val[CTRLE_BK]);
    /* restore ssc reg */
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlB, regs_val[CTRLB_BK]);
    /* restore dp regs */
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlF, regs_val[CTRLF_BK]);
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlG, regs_val[CTRLG_BK]);
    /* release reset */
    ctrlA.uctrlA_RESET = 0;
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlA, ctrlA.u32);
    /* release bypass */
    ctrlA.uctrlA_BYPASS = 0;
    GA_REG_WORD32_WRITE(pll_base + RA_abipll_ctrlA, ctrlA.u32);
    /* Wait for pll to be stable. */
    MV_OSAL_Task_Sleep(1);
}

/***************************************************************************************
 * FUNCTION: This function saves the a/v pll clocks
 * PARAMS: None
 *
 * RETURN: None
 ***************************************************************************************/
void MV_VPP_AVPLL_Save(UINT32 regs_bk[PLLBK_MAX][CTRLBK_MAX])
{
    pll_save_context(regs_bk[APLL0_IDX], APLLWRAP0APLL);
    pll_save_context(regs_bk[APLL1_IDX], APLLWRAP1APLL);
    pll_save_context(regs_bk[VPLL0_IDX], VPLLWRAP0VPLL);
#ifdef RA_avioGbl_VPLL_WRAP1
    pll_save_context(regs_bk[VPLL1_IDX], VPLLWRAP1VPLL);
#endif
}

/***************************************************************************************
 * FUNCTION: This function restores the a/v pll clocks
 * PARAMS: None
 *
 * RETURN: None
 ***************************************************************************************/
void MV_VPP_AVPLL_Restore(UINT32 regs_bk[PLLBK_MAX][CTRLBK_MAX])
{
    pll_restore(regs_bk[APLL0_IDX], APLLWRAP0APLL);
    pll_restore(regs_bk[APLL1_IDX], APLLWRAP1APLL);
    pll_restore(regs_bk[VPLL0_IDX], VPLLWRAP0VPLL);
#ifdef RA_avioGbl_VPLL_WRAP1
    pll_restore(regs_bk[VPLL1_IDX], VPLLWRAP1VPLL);
#endif
}
