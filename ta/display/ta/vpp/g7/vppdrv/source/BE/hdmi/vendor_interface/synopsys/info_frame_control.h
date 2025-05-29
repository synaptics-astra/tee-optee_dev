// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __INFO_FRAME_CONTROL_H__
#define __INFO_FRAME_CONTROL_H__
#include "hdmitx_dev.h"

/* function definitions for packet transmission */

void BE_HDMITX_PacketTxEnableACR(hdmi_tx_dev_t *dev, u8 enable);

void BE_HDMITX_PacketTxEnableGCP(hdmi_tx_dev_t *dev, u8 enable);

void BE_HDMITX_PacketTxEnableAVI(hdmi_tx_dev_t *dev, u8 enable);

void BE_HDMITX_PacketTxEnableAUDI(hdmi_tx_dev_t *dev, u8 enable);

void BE_HDMITX_PacketTxEnableDRM(hdmi_tx_dev_t *dev, u8 enable);

void BE_HDMITX_PacketsTxDisableAll(hdmi_tx_dev_t *dev);

void BE_HDMITX_PacketTxEnableGCPStatus(hdmi_tx_dev_t *dev, u8 *status);

#endif	//__INFO_FRAME_CONTROL_H__
