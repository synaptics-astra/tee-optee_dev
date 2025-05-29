// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALFRAMECOMPOSERSPD_H_
#define HALFRAMECOMPOSERSPD_H_

#include "../hdmitx_dev.h"
#include "util/types.h"

typedef struct fc_spd_info {
	const u8 * vName;
	u8 vLength;
	const u8 * pName;
	u8 pLength;
	u8 code;
	u8 autoSend;
}fc_spd_info_t;

int fc_spd_config(hdmi_tx_dev_t *dev, fc_spd_info_t *spd_data);
void fc_spd_VendorName(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length);
void fc_spd_ProductName(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length);
void fc_spd_SourceDeviceInfo(hdmi_tx_dev_t *dev, u8 code);

#endif	/* HALFRAMECOMPOSERSPD_H_ */
