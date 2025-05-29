// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALAUDIOI2S_H_
#define HALAUDIOI2S_H_

#include "../hdmitx_dev.h"
#include "core/audio.h"
#include "util/types.h"

void audio_i2s_configure(hdmi_tx_dev_t * dev, audioParams_t * audio);

void audio_i2s_select(hdmi_tx_dev_t *dev, u8 bit);

void audio_i2s_interrupt_mask(hdmi_tx_dev_t *dev, u8 value);

void audio_i2s_insert_pcuv(hdmi_tx_dev_t *dev, u8 value);

void audio_i2s_hbr_mode(hdmi_tx_dev_t *dev, u8 value);

void audio_i2s_nlpcm_mode(hdmi_tx_dev_t *dev, u8 value);

void audio_i2s_data_enable(hdmi_tx_dev_t *dev, u8 value);

void audio_i2s_reset_fifo(hdmi_tx_dev_t *dev);

#endif	/* HALAUDIOI2S_H_ */
