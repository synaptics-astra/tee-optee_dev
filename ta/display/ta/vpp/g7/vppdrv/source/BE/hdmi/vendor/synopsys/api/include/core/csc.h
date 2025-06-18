// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALCOLORSPACECONVERTER_H_
#define HALCOLORSPACECONVERTER_H_

#include "../hdmitx_dev.h"
#include "core/video_params.h"
#include "util/types.h"

void csc_config(hdmi_tx_dev_t *dev, videoParams_t * params,
		unsigned interpolation, unsigned decimation, unsigned color_depth);

#endif	/* HALCOLORSPACECONVERTER_H_ */
