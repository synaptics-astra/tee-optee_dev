// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALFRAMECOMPOSERVSD_H_
#define HALFRAMECOMPOSERVSD_H_

#include "../hdmitx_dev.h"
#include "util/types.h"

/*
 * Configure vsd packet size
 * @param baseAddr Block base address
 * @param size size of vsd packet
 */
void fc_vsd_set_size(hdmi_tx_dev_t *dev, u32 size);

/*
 * Reads vsd packet size
 * @param baseAddr Block base address
 * @return returns vsd packet size
 */
int fc_vsd_get_size(hdmi_tx_dev_t *dev);

/*
 * Configure the 24 bit IEEE Registration Identifier
 * @param baseAddr Block base address
 * @param id vendor unique identifier
 */
void fc_vsd_vendor_OUI(hdmi_tx_dev_t *dev, u32 id);

/*
 * Reads 24 bit IEEE Registration Identifier
 * @param baseAddr Block base address
 * @param id Pointer to vendor unique identifier
 */
void fc_vsd_get_vendor_OUI(hdmi_tx_dev_t *dev, u8 *id);

/*
 * Configure the Vendor Payload to be carried by the InfoFrame
 * @param info array
 * @param length of the array
 * @return 0 when successful and 1 on error
 */
u8 fc_vsd_vendor_payload(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length);

/*
 * Reads Vendor Payload
 * @param pointer to info array
 * @param length of the array
 * @return 0 when successful and 1 on error
 */
u8 fc_vsd_get_vendor_payload(hdmi_tx_dev_t *dev, u8 * data, unsigned short length);

#endif				/* HALFRAMECOMPOSERVSD_H_ */
