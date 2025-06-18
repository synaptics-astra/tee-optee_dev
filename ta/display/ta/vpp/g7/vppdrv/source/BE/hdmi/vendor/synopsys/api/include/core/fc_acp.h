// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALFRAMECOMPOSERACP_H_
#define HALFRAMECOMPOSERACP_H_

#include "../hdmitx_dev.h"
#include "util/types.h"

void fc_acp_type(hdmi_tx_dev_t *dev, u8 type);

void fc_acp_type_dependent_fields(hdmi_tx_dev_t *dev, u8 * fields, u8 fieldsLength);

#endif	/* HALFRAMECOMPOSERACP_H_ */
