// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "edid/monitor_range_limits.h"
#include "util/bit_operation.h"
#include "util/log.h"

void monitor_range_limits_reset(hdmi_tx_dev_t *dev, monitorRangeLimits_t * mrl)
{
	mrl->mMinVerticalRate = 0;
	mrl->mMaxVerticalRate = 0;
	mrl->mMinHorizontalRate = 0;
	mrl->mMaxHorizontalRate = 0;
	mrl->mMaxPixelClock = 0;
	mrl->mValid = FALSE;
}
