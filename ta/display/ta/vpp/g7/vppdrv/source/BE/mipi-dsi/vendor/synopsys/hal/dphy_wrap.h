/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __DPHY_WRAP_H__
#define __DPHY_WRAP_H__

#include "dsih_core.h"

enum{
    PLL_OUTPUT_CLOCK_STOP = 0,
    PLL_OUTPUT_CLOCK_GENERATE = 1,
    PLL_OUTPUT_CLOCK_EXTERNAL = 2,
    PLL_OUTPUT_CLOCK_FORBIDDEN = 3,
};

void mipi_dphy_pll_clksel(dphy_t *phy, int clksel);
void mipi_dphy_pll_shadow_control_en(dphy_t *phy, int en);
void mipi_dphy_CfgClkFreqRange(dphy_t *phy, int range);
void mipi_dphy_stopstate_wait(dphy_t *phy, int lanes);

#endif
