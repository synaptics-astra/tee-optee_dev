// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALFRAMECOMPOSERVIDEO_H_
#define HALFRAMECOMPOSERVIDEO_H_

#include "../hdmitx_dev.h"
#include "core/video_params.h"
#include "util/types.h"

int fc_video_config(hdmi_tx_dev_t *dev, videoParams_t *video);

void fc_video_hdcp_keepout(hdmi_tx_dev_t *dev, u8 bit);

void fc_video_VSyncPolarity(hdmi_tx_dev_t *dev, u8 bit);

void fc_video_HSyncPolarity(hdmi_tx_dev_t *dev, u8 bit);

void fc_video_DataEnablePolarity(hdmi_tx_dev_t *dev, u8 bit);

void fc_video_DviOrHdmi(hdmi_tx_dev_t *dev, u8 bit);

void fc_video_VBlankOsc(hdmi_tx_dev_t *dev, u8 bit);

void fc_video_Interlaced(hdmi_tx_dev_t *dev, u8 bit);

void fc_video_HActive(hdmi_tx_dev_t *dev, u16 value);

void fc_video_HBlank(hdmi_tx_dev_t *dev, u16 value);

void fc_video_VActive(hdmi_tx_dev_t *dev, u16 value);

void fc_video_VBlank(hdmi_tx_dev_t *dev, u16 value);

void fc_video_HSyncEdgeDelay(hdmi_tx_dev_t *dev, u16 value);

void fc_video_HSyncPulseWidth(hdmi_tx_dev_t *dev, u16 value);

void fc_video_VSyncEdgeDelay(hdmi_tx_dev_t *dev, u16 value);

void fc_video_VSyncPulseWidth(hdmi_tx_dev_t *dev, u16 value);

void fc_video_RefreshRate(hdmi_tx_dev_t *dev, u32 value);

void fc_video_ControlPeriodMinDuration(hdmi_tx_dev_t *dev, u8 value);

void fc_video_ExtendedControlPeriodMinDuration(hdmi_tx_dev_t *dev, u8 value);

void fc_video_ExtendedControlPeriodMaxSpacing(hdmi_tx_dev_t *dev, u8 value);

void fc_video_PreambleFilter(hdmi_tx_dev_t *dev, u8 value, unsigned channel);

void fc_video_PixelRepetitionInput(hdmi_tx_dev_t *dev, u8 value);

uint32_t fc_video_GetVSyncPulseWidth(hdmi_tx_dev_t *dev);

#endif	/* HALFRAMECOMPOSERVIDEO_H_ */
