// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALFRAMECOMPOSERDRM_H_
#define HALFRAMECOMPOSERDRM_H_

#include "../hdmitx_dev.h"
#include "util/types.h"

void fc_drm_packet_update(hdmi_tx_dev_t *dev, u8 enable);

void fc_drm_packet_header_version(hdmi_tx_dev_t *dev, u8 data);

void fc_drm_packet_header_length(hdmi_tx_dev_t *dev, u8 data);

u8 fc_drm_packet_body(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length);

u8 fc_drm_packet_body_read(hdmi_tx_dev_t *dev, u8 * data, unsigned short length);

#endif  /* HALFRAMECOMPOSERDRM_H_ */
