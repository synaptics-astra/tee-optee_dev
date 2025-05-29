/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __VPP_BE_EDDC_H__
#define __VPP_BE_EDDC_H__

int EDDC_Read(BE_HDMITX_DATA  *pHdmiTx,  UINT8 mode, UINT16 offset, INT16 count, UINT8 *msg);
int EDDC_Write(BE_HDMITX_DATA  *pHdmiTx, UINT8 mode, UINT16 offset, UINT16 count, UINT8 *msg);
void EDDC_InitializeMaster(BE_HDMITX_DATA  *pHdmiTx, int index);
void Destroy_EDDC(VPP_OBJ *vpp_obj);

#endif

