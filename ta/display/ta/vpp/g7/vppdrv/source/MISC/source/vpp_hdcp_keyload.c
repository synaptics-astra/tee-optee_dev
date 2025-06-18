/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "vpp_module.h"
#include "vpp_hdcp_keyload.h"

#ifdef ENABLE_HDCP_KEYLOAD_SUPPORT

#define __DRM_COMMON_TYPE_H__   /* avoid including drm_types.h
                                   to avoid macro/struct/types conflict
                                   with ErrorCode.h */
#ifndef CONFIG_OPTEE
#include "tee_internal_private_api.h"
#else
#include "tee_api_syna.h"
#endif
#include "bcm_mailbox.h"
#include "drm_bcm_com.h"
#include "bcm_primitive.h"

static int ConvertToDrmPrimitive(VPP_HDCP_KEYSTORE_TYPE keystoreType)
{
    switch(keystoreType) {
        case VPP_HDCP_KEYSTORE_HDCPTX:
            return DRM_HDCP_HDMI_TX_LOADKEYS;
        case VPP_HDCP_KEYSTORE_HDCPRX:
            return DRM_HDCP_HDMI_RX_LOADKEYS;
        case VPP_HDCP_KEYSTORE_HDCP2X:
            return DRM_HDCP2X_HDMI_TX_LOADKEYS;
        default:
            return -1;
    }
}

static int BCMSendMsg(VPP_OBJ *vpp_obj, DRM_PRIMITIVE eBcmCmd,
                                        UINT8* pbData, UINT32 uSize)
{
    TEE_Result ret;
    UINT8 *pbDataSHM;
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    /* call BCM API to generate key store */
    pbDataSHM = ((VPP_HDCP_KEYSTORE *)vpp_obj->mKeystore)->keyDataSHM;
    if (NULL == pbDataSHM) {
        VPPHDMI_DBG_ERROR("SHM alloc error\n");
        return MV_VPP_ENOSHM;
    }

    memcpy(pbDataSHM, pbData, uSize);
    ret = TEE_FlushCache(pbDataSHM, uSize);
    if (TEE_SUCCESS != ret) {
        VPPHDMI_DBG_ERROR("Failed to flush cache\n");
        return MV_VPP_ESWSTATEWRONG;
    }

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.arg0 = (UINT)pbDataSHM;            //secure key
    cmd.arg1 = uSize;                    //size of key

    cmd.command = eBcmCmd;
    execute_bcm_command(&cmd, &status);

    return status.return_status;
}

/******************** Public functions ********************/

int VPP_HDCP_KEYLOAD_LoadKey(VPP_OBJ *vpp_obj, VPP_HDCP_KEYSTORE_TYPE keystoreType)
{
    int retVal = MV_VPP_OK;
    int ret;
    int i, n;
    char *keyStoreStr[VPP_HDCP_KEYSTORE_MAX] = {"TX", "RX", "2X"};
    if ((vpp_obj == NULL) || (vpp_obj->mKeystore == NULL))
        return (MV_VPP_ENODEV);

    VPP_HDCP_KEYSTORE *keyStore = (VPP_HDCP_KEYSTORE *) vpp_obj->mKeystore;

    if (keystoreType <= VPP_HDCP_KEYSTORE_MAX) {
        n = keystoreType;
        i = (keystoreType == VPP_HDCP_KEYSTORE_MAX) ? 0 : n++;
        for (; i < n ; i++) {
            if (keyStore->keyData[i].hdcpKeySize != 0) {
                ret = BCMSendMsg(vpp_obj, ConvertToDrmPrimitive(i),
                                    keyStore->keyData[i].hdcpKey,
                                    keyStore->keyData[i].hdcpKeySize);
                VPPAPI_DBG_INFO("Load HDCP %s keys (%x)\n",
                                keyStoreStr[i], ret);
                if (ret)
                    retVal |= 1 << i;
            }
        }
    } else {
        VPPAPI_DBG_ERROR("Invalid keystore type (%d)\n", keystoreType);
    }
    return retVal;
}

int VPP_HDCP_KEYLOAD_Init(VPP_OBJ *vpp_obj)
{
    VPP_SHM32_MALLOC(VPP_SHM_SECURE, sizeof(VPP_HDCP_KEYSTORE),
                    (VPP_SHM_HANDLE *)&vpp_obj->mKeystore);
    if (vpp_obj->mKeystore == NULL) {
        VPPAPI_DBG_ERROR("Error in allocating memory\n");
        return (MV_VPP_ENOMEM);
    }

    VPP_Memset(vpp_obj->mKeystore, 0, sizeof(VPP_HDCP_KEYSTORE));
    return MV_VPP_OK;
}

void VPP_HDCP_KEYLOAD_DeInit(VPP_OBJ *vpp_obj)
{
    if (vpp_obj->mKeystore)
        VPP_SHM_Free((VPP_SHM_HANDLE) vpp_obj->mKeystore);
}

#else

int VPP_HDCP_KEYLOAD_LoadKey(VPP_OBJ *vpp_obj, VPP_HDCP_KEYSTORE_TYPE keystoreType)
{
    return MV_VPP_OK;
}

int VPP_HDCP_KEYLOAD_Init(VPP_OBJ *vpp_obj)
{
    return MV_VPP_OK;
}

void VPP_HDCP_KEYLOAD_DeInit(VPP_OBJ *vpp_obj)
{
}

#endif  //ENABLE_HDCP_KEYLOAD_SUPPORT
