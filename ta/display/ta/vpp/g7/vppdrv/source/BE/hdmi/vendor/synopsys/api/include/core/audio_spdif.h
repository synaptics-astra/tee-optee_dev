// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALAUDIOSPDIF_H_
#define HALAUDIOSPDIF_H_

#include "../hdmitx_dev.h"
#include "core/audio.h"
#include "util/types.h"

void audio_spdif_config(hdmi_tx_dev_t *dev, audioParams_t *audio);

void audio_spdif_interrupt_mask(hdmi_tx_dev_t *dev, u8 value);

#endif	/* HALAUDIOSPDIF_H_ */
