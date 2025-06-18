// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "frame_composer/frame_composer_reg.h"
#include "core/fc_vsd.h"
#include "bsp/access.h"
#include "util/log.h"

void fc_vsd_set_size(hdmi_tx_dev_t *dev, u32 size)
{
	LOG_TRACE1(size);
	dev_write_mask(dev, FC_VSDSIZE, FC_VSDSIZE_VSDSIZE_MASK, size);
}

int fc_vsd_get_size(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, FC_VSDSIZE, FC_VSDSIZE_VSDSIZE_MASK);
}

void fc_vsd_vendor_OUI(hdmi_tx_dev_t *dev, u32 id)
{
	LOG_TRACE1(id);
	dev_write(dev, (FC_VSDIEEEID0), id);
	dev_write(dev, (FC_VSDIEEEID1), id >> 8);
	dev_write(dev, (FC_VSDIEEEID2), id >> 16);
}

void fc_vsd_get_vendor_OUI(hdmi_tx_dev_t *dev, u8 *id)
{
	LOG_TRACE();
	id[0] = dev_read(dev, FC_VSDIEEEID0);
	id[1] = dev_read(dev, FC_VSDIEEEID1);
	id[2] = dev_read(dev, FC_VSDIEEEID2);
}

u8 fc_vsd_vendor_payload(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length)
{
	const unsigned short size = 24;
	unsigned i = 0;
	LOG_TRACE();
	if (data == 0) {
		LOGGER(SNPS_WARN,"invalid parameter");
		return 1;
	}
	if (length > size) {
		length = size;
		LOGGER(SNPS_WARN,"vendor payload truncated");
	}
	for (i = 0; i < length; i++) {
		dev_write(dev, (FC_VSDPAYLOAD0 + (i*4)), data[i]);
	}
	return 0;
}

u8 fc_vsd_get_vendor_payload(hdmi_tx_dev_t *dev, u8 * data, unsigned short length)
{
	unsigned i = 0;
	LOG_TRACE();
	if (data == 0) {
		LOGGER(SNPS_WARN,"invalid parameter");
		return 1;
	}

	for (i = 0; i < length; i++) {
		data[i] = dev_read(dev, (FC_VSDPAYLOAD0 + (i*4)));
	}

	return 0;
}
