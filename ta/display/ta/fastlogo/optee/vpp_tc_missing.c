/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "vpp_tc_missing.h"

int vprintf(const char *format, va_list arg)
{
    return 0;
}

double fabs(double a)
{
    return a;
}

float sin(float a)
{
#if CONFIG_VPP_ENABLE_HUE_BYPASS == 1
    a = 0.0;
#endif
    return a;
}

float cos(float a)
{
#if CONFIG_VPP_ENABLE_HUE_BYPASS == 1
    a = 1.0;
#endif
    return a;
}

double ceil(double a)
{
    return a;
}
