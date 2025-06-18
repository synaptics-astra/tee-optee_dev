// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

/**
 * Access Functions
 */
int hdmitx_initialize(void);

int hdmitx_disable(void);

void hdmitx_write(u32 addr, u32 data);
u32 hdmitx_read(u32 addr);
