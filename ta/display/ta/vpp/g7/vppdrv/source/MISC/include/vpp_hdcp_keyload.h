/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __VPP_HDCP_KEYLOAD_H__
#define __VPP_HDCP_KEYLOAD_H__

#include "vpp_api_types.h"

#ifdef ENABLE_HDCP_KEYLOAD_SUPPORT
typedef struct VPP_HDCP_KEYSTORE_t
{
    VPP_HDCP_KEYSTORE_DATA keyData[VPP_HDCP_KEYSTORE_MAX];
    UINT8 keyDataSHM[HDCP_KEY_SIZE];
}VPP_HDCP_KEYSTORE;
#endif

int VPP_HDCP_KEYLOAD_Init(VPP_OBJ *vpp_obj);
void VPP_HDCP_KEYLOAD_DeInit(VPP_OBJ *vpp_obj);
int VPP_HDCP_KEYLOAD_LoadKey(VPP_OBJ *vpp_obj, VPP_HDCP_KEYSTORE_TYPE keystoreType);

#endif  //__VPP_HDCP_KEYLOAD_H__
