// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALFRAMECOMPOSERGAMUT_H_
#define HALFRAMECOMPOSERGAMUT_H_

#include "../hdmitx_dev.h"
#include "util/types.h"

void fc_gamut_enable_tx(hdmi_tx_dev_t *dev, u8 enable);

void fc_gamut_config(hdmi_tx_dev_t *dev);

void fc_gamut_packet_config(hdmi_tx_dev_t *dev, const u8 * gbdContent, u8 length);

void fc_gamut_Profile(hdmi_tx_dev_t *dev, u8 profile);
void fc_gamut_AffectedSeqNo(hdmi_tx_dev_t *dev, u8 no);
void fc_gamut_Content(hdmi_tx_dev_t *dev, const u8 * content, u8 length);
void fc_gamut_UpdatePacket(hdmi_tx_dev_t *dev);

#endif	/* HALFRAMECOMPOSERGAMUT_H_ */
