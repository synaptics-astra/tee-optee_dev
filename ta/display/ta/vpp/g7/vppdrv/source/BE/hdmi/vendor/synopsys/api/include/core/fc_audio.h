// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALFRAMECOMPOSERAUDIO_H_
#define HALFRAMECOMPOSERAUDIO_H_

#include "../hdmitx_dev.h"
#include "util/types.h"
#include "core/audio_params.h"

void fc_audio_config(hdmi_tx_dev_t *dev, audioParams_t * audio);
void fc_audio_mute(hdmi_tx_dev_t *dev);
void fc_audio_unmute(hdmi_tx_dev_t *dev);

#endif	/* HALFRAMECOMPOSERAUDIO_H_ */
