/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#include "includes.h"
#include "dsih_core.h"
#include "DPHYTX.h"
#include "mipi_hal.h"

void mipi_dphy_BiuCtrlPHYEn(dphy_t *phy, int en)
{
    T32DPHYTX_DPHY_CTL1 ctrl;
    ctrl.u32 = ioread32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_CTL0);
    SET32DPHYTX_DPHY_CTL0_BiuCtrlPhyEn(ctrl.u32, en);
    iowrite32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
    return;
}

void mipi_dphy_shutdown(dphy_t *phy, int shutdown)
{
    T32DPHYTX_DPHY_CTL1 ctrl;
    ctrl.u32 = ioread32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_CTL1);
    SET32DPHYTX_DPHY_CTL1_shutdownz(ctrl.u32,shutdown);
    iowrite32(phy->pHandle, phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
    return;
}

void mipi_dphy_resetz(dphy_t *phy, int resetz)
{
    T32DPHYTX_DPHY_CTL1 ctrl;
    ctrl.u32 = ioread32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_CTL1);
    SET32DPHYTX_DPHY_CTL1_rstz(ctrl.u32,resetz);
    iowrite32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
    return;
}

void mipi_dphy_enable_lanes(dphy_t *phy, int lanes)
{
    int lane_mask = 0x1;
    T32DPHYTX_DPHY_CTL1 ctrl;

    ctrl.u32 = ioread32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_CTL1);
    if (lanes & lane_mask) {
        SET32DPHYTX_DPHY_CTL1_enable_0(ctrl.u32, 1);
    } else {
        SET32DPHYTX_DPHY_CTL1_enable_0(ctrl.u32, 0);
    }
    lane_mask = (lane_mask << 1);
    if (lanes & lane_mask) {
        SET32DPHYTX_DPHY_CTL1_enable_1(ctrl.u32, 1);
    } else {
        SET32DPHYTX_DPHY_CTL1_enable_1(ctrl.u32, 0);
    }
    lane_mask = (lane_mask << 1);
    if (lanes & lane_mask) {
        SET32DPHYTX_DPHY_CTL1_enable_2(ctrl.u32, 1);
    } else {
        SET32DPHYTX_DPHY_CTL1_enable_2(ctrl.u32, 0);
    }
    lane_mask = (lane_mask << 1);
    if (lanes & lane_mask) {
        SET32DPHYTX_DPHY_CTL1_enable_1(ctrl.u32, 1);
    } else {
        SET32DPHYTX_DPHY_CTL1_enable_1(ctrl.u32, 0);
    }
    iowrite32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
    return;
}

void mipi_dphy_EnableClkBIU(dphy_t *phy, int en)
{
    T32DPHYTX_DPHY_CTL1 ctrl;

    ctrl.u32 = ioread32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_CTL1);
    SET32DPHYTX_DPHY_CTL1_enableclkBIU(ctrl.u32, en);
    iowrite32(phy->pHandle, phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
    return;
}

void mipi_dphy_CfgClkFreqRange(dphy_t *phy, int range)
{
    T32DPHYTX_DPHY_CTL1 ctrl;

    ctrl.u32 = ioread32(phy->pHandle, phy->base + RA_DPHYTX_DPHY_CTL1);
    SET32DPHYTX_DPHY_CTL1_cfgclkfreqrange(ctrl.u32,range);
    iowrite32(phy->pHandle, phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
    return;
}

void mipi_dphy_pll_shadow_control_en(dphy_t *phy, int en)
{
    T32DPHYTX_DPHY_PLL2 ctrl;
    ctrl.u32 = ioread32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_PLL2);
    SET32DPHYTX_DPHY_PLL2_pll_shadow_control(ctrl.u32, en);
    iowrite32(phy->pHandle, phy->base + RA_DPHYTX_DPHY_PLL2, ctrl.u32);
}

void mipi_dphy_pll_clksel(dphy_t *phy, int clksel)
{
    T32DPHYTX_DPHY_PLL2 ctrl;
    ctrl.u32 = ioread32(phy->pHandle,phy->base + RA_DPHYTX_DPHY_PLL2);
    SET32DPHYTX_DPHY_PLL2_clksel(ctrl.u32, clksel);
    iowrite32(phy->pHandle, phy->base + RA_DPHYTX_DPHY_PLL2, ctrl.u32);
}

void mipi_dphy_stopstate_wait(dphy_t *phy, int lanes)
{
    T32DPHYTX_DPHY_RB0 rb0, cond;
    unsigned int wait = 0xF00;

    lanes = (1<<lanes)-1;
    cond.u32 = 0;
    cond.uDPHY_RB0_stopstateclk = 1;
    cond.uDPHY_RB0_stopstatedata_0 = (lanes & 0x1)?1:0;
    cond.uDPHY_RB0_stopstatedata_1 = (lanes & 0x2)?1:0;
    cond.uDPHY_RB0_stopstatedata_2 = (lanes & 0x4)?1:0;
    cond.uDPHY_RB0_stopstatedata_3 = (lanes & 0x8)?1:0;
    do {
        rb0.u32 = ioread32(phy->pHandle, phy->base + RA_DPHYTX_DPHY_RB0);
        rb0.u32 = (rb0.u32 & 0x3D000);
        if (!wait)
            break;
        wait = wait - 1;
    } while (rb0.u32 != cond.u32);
}
