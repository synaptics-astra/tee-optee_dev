// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef HALAUDIOMULTISTREAM_H_
#define HALAUDIOMULTISTREAM_H_

#include "../hdmitx_dev.h"


void halAudioMultistream_MetaDataPacket_Descriptor_X(hdmi_tx_dev_t *dev, u8 descrNr, audioMetaDataDescriptor_t *mAudioMetaDataDescriptor);

void halAudioMultistream_MetaDataPacket_Header(hdmi_tx_dev_t *dev, audioMetaDataPacket_t *mAudioMetaDataPckt);

void halAudioMultistream_MetaDataPacketBody(hdmi_tx_dev_t *dev, audioMetaDataPacket_t *mAudioMetaDataPacket);

#endif	/* HALAUDIOMULTISTREAM_H_ */
