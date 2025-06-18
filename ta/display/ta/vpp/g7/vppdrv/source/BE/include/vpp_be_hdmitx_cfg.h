/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPP_BE_HDMITX_CFG_H__
#define __VPP_BE_HDMITX_CFG_H__

#include "com_type.h"

/*-----------------------------------------------------------------------------
 * Externed Variables
 *-----------------------------------------------------------------------------
 */

// Memory size
extern const UINT16 HdmiTxMemSize;

// Deep color fifo read and write pointers
extern const UINT8 HdmiTxReadPtr;
extern const UINT8 HdmiTxWritePtr;

// Swap Control Flags
extern const BOOL HdmiTxEnTmdsBitSwap;
extern const BOOL HdmiTxEnTmdsChnSwap;

// BCH Rotate Setting
extern const BOOL HdmiTxEnBchRot;

// I2S Bus Params
extern VPP_BE_HDMITX_AUDIO_BUS_PARAMS I2SBusParams;
extern const UINT32 HdmiTxTmdsClkCfg;
#endif // __VPP_BE_HDMITX_CFG_H__


