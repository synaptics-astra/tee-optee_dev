// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "frame_composer/frame_composer_reg.h"
#include "core/fc_debug.h"
#include "bsp/access.h"
#include "util/log.h"
#include "core/video_params.h"

void fc_force_audio(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, FC_DBGFORCE, FC_DBGFORCE_FORCEAUDIO_MASK, bit);
}

void fc_force_video(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);

	/* avoid glitches */
	if (bit != 0) {
		/* enable fix color transmission from hdmi tx.
		   Below values configure black screen for different formats
		 */
		if((dev->snps_hdmi_ctrl.encoding == YCC422) || (dev->snps_hdmi_ctrl.encoding == YCC444)) {
			dev_write(dev, FC_DBGTMDS2, 0x80);	/* Cb */
			dev_write(dev, FC_DBGTMDS1, 0x00);	/* Y  */
			dev_write(dev, FC_DBGTMDS0, 0x80);	/* Cr */
		} else if (dev->snps_hdmi_ctrl.encoding == YCC420) {
			dev_write(dev, FC_DBGTMDS2, 0x00);
			dev_write(dev, FC_DBGTMDS1, 0x00);
			dev_write(dev, FC_DBGTMDS0, 0x80);
		} else {
			dev_write(dev, FC_DBGTMDS2, 0x00);	/* R */
			dev_write(dev, FC_DBGTMDS1, 0x00);	/* G */
			dev_write(dev, FC_DBGTMDS0, 0x00);	/* B */
		}
		dev_write_mask(dev, FC_DBGFORCE, FC_DBGFORCE_FORCEVIDEO_MASK, 1);
	} else {
		dev_write_mask(dev, FC_DBGFORCE, FC_DBGFORCE_FORCEVIDEO_MASK, 0);
		dev_write(dev, FC_DBGTMDS2, 0x00);	/* R */
		dev_write(dev, FC_DBGTMDS1, 0x00);	/* G */
		dev_write(dev, FC_DBGTMDS0, 0x00);	/* B */
	}
}

void fc_force_output(hdmi_tx_dev_t *dev, int enable)
{
	fc_force_audio(dev, 0);
	fc_force_video(dev, (u8)enable);
}
