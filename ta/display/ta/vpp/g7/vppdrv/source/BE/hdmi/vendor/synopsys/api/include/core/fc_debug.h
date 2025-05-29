// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALFRAMECOMPOSERDEBUG_H_
#define HALFRAMECOMPOSERDEBUG_H_

#include "../hdmitx_dev.h"
#include "util/types.h"

//void halFrameComposerDebug_ForceAudio(hdmi_tx_dev_t *dev, u8 bit);
//
//void halFrameComposerDebug_ForceVideo(hdmi_tx_dev_t *dev, u8 bit);

void fc_force_output(hdmi_tx_dev_t *dev, int enable);

#endif	/* HALFRAMECOMPOSERDEBUG_H_ */
