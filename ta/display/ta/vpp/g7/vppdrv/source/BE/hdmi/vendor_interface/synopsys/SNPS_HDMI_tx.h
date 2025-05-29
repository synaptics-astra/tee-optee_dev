// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */
/*
 * hdmi_tx_app.h
 *
 *  Created on: Dec 10, 2014
 *      Author: arcxplorer
 */

#ifndef HDMI_TX_APP_H_
#define HDMI_TX_APP_H_

#include "bsp/board.h"
#include "bsp/access.h"
#include "bsp/system.h"
#include "edid/hdmivsdb.h"
#include "edid/hdmiforumvsdb.h"
#include "core/product.h"
#include "core/audio_params.h"
#include "core/video_params.h"

#include "tee_ta_osal.h"
#include "hdcp22_wrap.h"

#define STRING_ARRAY_SIZE 150

#define IS_EQUAL(buffer, text) \
    ((strcmp(buffer, text) == 0) ? 1 : 0)

#define GET(buf, argc) \
    strtoul(buf[argc], NULL, 0)

#define SIZE_ARRAY(array, type) \
    (sizeof(array) / sizeof(type))

#define HPI_REGISTER_BASE                  (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_BASE + RA_HDCP22_WRAP_MEMMAP_HDCP22_HPI)
#define TRNG_REGISTER_BASE                 (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_SNPS_TRNG_REG_BASE)
#define SNPS_HDMITX_BASE_ADDR              (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_SNPS_HDMITX_REG_BASE)
#define HDMI_TX_WRAPPER_BASE_ADDR          (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_BASE)
#define HDCP22_TX_WRAP_BASE_ADDR           (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_BASE)

struct hdmi_mode {
    videoParams_t       pVideo;
    audioParams_t       pAudio;
    hdcpParams_t        pHdcp;
    productParams_t     pProduct;
    productParams_t     mProduct;
    hdmivsdb_t          vsdb;
    hdmiforumvsdb_t     forumvsdb;

    uint8_t         ksv_list_buffer[670];
    uint8_t         ksv_devices;
    uint8_t         dpk_aksv[7];
    uint8_t         sw_enc_key[2];
    uint8_t         dpk_keys[560];

    int             hpd;
    int             edid_done;

    struct edid     * edid;
    u8              * edid_ext;
    //sink_edid_t   * sink_cap; //Lv

};

/**
 * Main structure
 */
typedef struct {
    /** Verbose */
    int                         verbose;

    /** PHY version */
    int                         hdmi_tx_phy;

    /** HDMI TX Driver */
    char                        hdmi_tx_name[STRING_ARRAY_SIZE];

    uint32_t                    snps_tx_base_address;
    uint32_t                    hdmitx_wrap_base_address;
    uint32_t                    hdcp22_tx_wrap_base_address;

    /* Reserved for API internal use only */
    /** HDMI TX API Internals */
    struct device_access        dev_access;
    struct system_functions     sys_functions;
    struct board_wrappers       board_wrp;
    hdmi_tx_dev_t               hdmi_tx;

    /** Application mode configurations */
    struct hdmi_mode            mode;

    uint32_t                    RiErrCnt;
}SNPS_HDMI_TX;

#endif /* HDMI_TX_APP_H_ */
