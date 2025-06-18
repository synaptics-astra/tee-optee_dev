// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "scdc/scdc.h"
#include "scdc/scrambling.h"
#include "util/types.h"
#include "util/log.h"
#include "core/fc_video.h"
#include "core/main_controller.h"
#include "bsp/access.h"


void scrambling(hdmi_tx_dev_t *dev, u8 enable){
	u8 data = 0;
	if (enable == 1) {
		scdc_scrambling_enable_flag(dev, 1);
		snps_sleep(5);

		/* Start/stop HDCP keep-out window generation not needed because it's always on */
		/* TMDS software reset request */
		mc_tmds_clock_reset(dev, TRUE);

		/* Enable/Disable Scrambling */
		scrambling_Enable(dev, TRUE);

		/* Write on RX the bit Scrambling_Enable, register 0x20 */
		data = 0x01;
		scdc_write(dev, SCDC_TMDS_CONFIG, 1, &data);
	} else {
		/* Enable/Disable Scrambling */
		scrambling_Enable(dev, FALSE);
		scdc_scrambling_enable_flag(dev, 0);

		/* TMDS software reset request */
		mc_tmds_clock_reset(dev, FALSE);

		/* Write on RX the bit Scrambling_Enable, register 0x20 */
		data = 0x00;
		scdc_write(dev, SCDC_TMDS_CONFIG, 1, &data);
	}
}

void scrambling_Enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(dev, FC_SCRAMBLER_CTRL, FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK, bit);
}
