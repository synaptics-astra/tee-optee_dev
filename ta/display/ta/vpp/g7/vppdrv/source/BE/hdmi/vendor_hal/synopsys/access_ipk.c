// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "SNPS_HDMI_tx.h"
#include "galois_io.h"
#include "access_ipk.h"
#include "platform.h"
#include "OSAL_api.h"
#include "vpp_debug.h"
#include "Galois_memmap.h"
#include "avio_memmap.h"
#include "version.h"

/**
 * Access Functions
 */
int hdmitx_initialize(void){
    VPPHDMI_DBG_INFO("Opening SNPS hdmitx driver api version: %s\n", HDMITX_VNDR_API_VERSION);
    VPPHDMI_DBG_INFO("Opening SNPS esm driver version: %s\n", HDMITX_VNDR_ESM_VERSION);
    VPPHDMI_DBG_INFO("HDMI core base address: 0x%x\n", SNPS_HDMITX_BASE_ADDR);
    return TRUE;
}

int hdmitx_disable(void){
    /* nothing to be done here */
    return TRUE;
}
void hdmitx_write(u32 addr, u32 data){
    GA_REG_WORD32_WRITE(SNPS_HDMITX_BASE_ADDR + addr, data);
}

u32 hdmitx_read(u32 addr){
    u32 data;
    GA_REG_WORD32_READ(SNPS_HDMITX_BASE_ADDR + addr, &data);
    return data;
}
