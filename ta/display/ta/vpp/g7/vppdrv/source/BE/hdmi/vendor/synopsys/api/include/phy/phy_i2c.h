// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALSOURCEPHY_H_
#define HALSOURCEPHY_H_

#include "../hdmitx_dev.h"
#include "util/types.h"

void phy_i2c_fast_mode(hdmi_tx_dev_t *dev, u8 bit);

void phy_i2c_master_reset(hdmi_tx_dev_t *dev);

void phy_i2c_mask_interrupts(hdmi_tx_dev_t *dev, int mask);

void phy_i2c_slave_address(hdmi_tx_dev_t *dev, u8 value);

int phy_i2c_write(hdmi_tx_dev_t *dev, u8 addr, u16 data);

int phy_i2c_read(hdmi_tx_dev_t *dev, u8 addr, u16 * value);

#endif	/* HALSOURCEPHY_H_ */
