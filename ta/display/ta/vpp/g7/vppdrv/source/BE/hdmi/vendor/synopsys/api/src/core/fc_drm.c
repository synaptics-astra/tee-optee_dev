// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

/*
 * @file:fc_drm.c
 *
 */
#include "frame_composer/frame_composer_reg.h"
#include "core/fc_drm.h"
#include "bsp/access.h"
#include "util/log.h"

#define HDMI_DRM_INFOFRAME_SIZE    26

void fc_drm_packet_update(hdmi_tx_dev_t *dev, u8 enable)
{
   LOG_TRACE1(enable);
   dev_write_mask(dev, FC_DRM_UP_ON, (1 << FC_DRM_UP_ON_DRM_PACKET_UPDATE_MASK), (enable ? 1 : 0));
}

void fc_drm_packet_header_version(hdmi_tx_dev_t *dev, u8 data)
{
    LOG_TRACE1(data);
    dev_write(dev, FC_DRM_HB0, data);
}

void fc_drm_packet_header_length(hdmi_tx_dev_t *dev, u8 data)
{
    LOG_TRACE1(data);
    dev_write(dev, FC_DRM_HB1, data);
}

u8 fc_drm_packet_body(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length)
{
    unsigned i = 0;
    LOG_TRACE();
    if (data == 0) {
        LOGGER(SNPS_WARN,"invalid parameter");
        return 1;
    }
    for (i = 0; i < length; i++) {
        dev_write(dev, (FC_DRM_PB0 + (i*4)), data[i]);
    }
    return 0;
}

u8 fc_drm_packet_body_read(hdmi_tx_dev_t *dev, u8 * data, unsigned short length)
{
    unsigned i = 0;
    LOG_TRACE();
    if (data == 0) {
        LOGGER(SNPS_WARN,"invalid parameter");
        return 1;
    }
    for (i = 0; i < length; i++) {
        data[i] = dev_read(dev, (FC_DRM_PB0 + (i*4)));
    }
    return 0;
}
