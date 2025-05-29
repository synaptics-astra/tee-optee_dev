// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synaptics Incorporated */

#include "core/frame_composer/frame_composer_reg.h"
#include "util/log.h"
#include "bsp/access.h"

void BE_HDMITX_PacketTxEnableACR(hdmi_tx_dev_t *dev, u8 enable)
{
    LOG_TRACE1(enable);
    dev_write_mask(dev, FC_PACKET_TX_EN, FC_PACKET_TX_EN_ACR_TX_EN_MASK, enable);
}

void BE_HDMITX_PacketTxEnableGCP(hdmi_tx_dev_t *dev, u8 enable)
{
    LOG_TRACE1(enable);
    dev_write_mask(dev, FC_PACKET_TX_EN, FC_PACKET_TX_EN_GCP_TX_EN_MASK, enable);
}

void BE_HDMITX_PacketTxEnableAVI(hdmi_tx_dev_t *dev, u8 enable)
{
    LOG_TRACE1(enable);
    dev_write_mask(dev, FC_PACKET_TX_EN, FC_PACKET_TX_EN_AVI_TX_EN_MASK, enable);
}

void BE_HDMITX_PacketTxEnableAUDI(hdmi_tx_dev_t *dev, u8 enable)
{
    LOG_TRACE1(enable);
    dev_write_mask(dev, FC_PACKET_TX_EN, FC_PACKET_TX_EN_AUDI_TX_EN_MASK, enable);
}

void BE_HDMITX_PacketTxEnableDRM(hdmi_tx_dev_t *dev, u8 enable)
{
    LOG_TRACE1(enable);
    dev_write_mask(dev, FC_PACKET_TX_EN, FC_PACKET_TX_EN_DRM_TX_EN_MASK, enable);
}

void BE_HDMITX_PacketsTxDisableAll(hdmi_tx_dev_t *dev)
{
    LOG_TRACE();
    u32 value = ~( FC_PACKET_TX_EN_AVI_TX_EN_MASK | FC_PACKET_TX_EN_AUDI_TX_EN_MASK );
    LOG_TRACE();
    dev_write(dev, FC_PACKET_TX_EN, value & dev_read(dev, FC_PACKET_TX_EN));
}

void BE_HDMITX_PacketTxEnableGCPStatus(hdmi_tx_dev_t *dev, u8 *status)
{
    *status = dev_read_mask (dev, FC_PACKET_TX_EN, FC_PACKET_TX_EN_GCP_TX_EN_MASK);
}

