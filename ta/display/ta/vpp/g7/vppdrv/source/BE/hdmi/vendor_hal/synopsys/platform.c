// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "SNPS_HDMI_tx.h"
#include "platform.h"
//#include "hdmitx_ipk_api.h"
#include <limits.h>
#include "hdmitx_dev.h"


/*******************************
 * HDMI TX Support functions   *
 ******************************/

uint16_t audio_clock(double value, uint16_t over_sampling_factor)
{
/*
        TODO: replace this with PLL related if PLL configuration done inside hdmi
*/
	return TRUE;
}

uint16_t pixel_clock(double freq)
{
/*
        TODO: replace this with PLL related if PLL configuration done inside hdmi
*/
	return TRUE;
}

