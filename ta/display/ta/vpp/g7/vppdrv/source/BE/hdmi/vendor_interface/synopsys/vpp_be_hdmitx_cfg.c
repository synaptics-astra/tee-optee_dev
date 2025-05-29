/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
/*-----------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------------
 */
#include "vpp_module.h"
#include "vpp_be_hdmitx.h"
#include "vpp_be_hdmitx_cfg.h"

/*-----------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------------
 */

// Memory size
const UINT16 HdmiTxMemSize = 0xFFFF;

// Deep color fifo read and write pointers
const UINT8 HdmiTxReadPtr  = 0x1F;
const UINT8 HdmiTxWritePtr = 0x00;

// Swap Control Flags
const BOOL HdmiTxEnTmdsBitSwap = FALSE;
const BOOL HdmiTxEnTmdsChnSwap = FALSE;

// BCH Rotate Setting
const BOOL HdmiTxEnBchRot = FALSE;

// I2S Bus Params
VPP_BE_HDMITX_AUDIO_BUS_PARAMS I2SBusParams =
{{
   0x0, /* ClkDelay */
   0x0, /* DataShift*/
   0x0, /* DataJustification */
   0x0, /* LeftChnWordSelEdge*/
   0x0, /* DataLatchingEdge  */
   0x2, /* PortCfg - Port 1  */
}};
const UINT32 HdmiTxTmdsClkCfg = 0x0F83E0;
