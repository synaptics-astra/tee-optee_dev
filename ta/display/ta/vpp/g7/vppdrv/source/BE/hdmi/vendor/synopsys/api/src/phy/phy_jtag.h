// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef SRC_PHY_PHY_JTAG_H_
#define SRC_PHY_PHY_JTAG_H_


#define JTAG_TAP_ADDR_CMD 	0
#define JTAG_TAP_WRITE_CMD 	1
#define JTAG_TAP_READ_CMD 	3

int phy_jtag_init(hdmi_tx_dev_t * dev, u8 jtag_addr);
void phy_jtag_slave_address(hdmi_tx_dev_t * dev, u8 jtag_addr);
int phy_jtag_read(hdmi_tx_dev_t * dev, u16 addr,  u16 * pvalue);
int phy_jtag_write(hdmi_tx_dev_t * dev, u16 addr,  u16 value);
void phy_jtag_reset(hdmi_tx_dev_t * dev);

#endif /* SRC_PHY_PHY_JTAG_H_ */
