/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
//-----------------------------------------------------------------------------
//! \file       vpp_devkey.c
//! \brief      VPP device security key generation source file
//! \author     Dharini
//-----------------------------------------------------------------------------

#ifdef CONFIG_VPP_DEV_KEY_SUPPORT
#ifdef CONFIG_AMP_TEE_ENABLE
#include <string.h>
#include "vpp_cmd.h"
#include "tee_common.h"
#include "tee_internal_common.h"
#include "tee_internal_core_common.h"
#include "tee_internal_api.h"

#ifndef CONFIG_AMP_TEE_ENABLE
#include "isl/amp_shm.h"
#endif

#include "vpp_devkey.h"

#ifdef TZ_3_0
#include "tee_internal_securekey.h"
#define APPID_SIZE_IN_BYTE                          (16)

static unsigned char gApplicationID[APPID_SIZE_IN_BYTE] = {'V', 'P', 'P', '4', '-', 'A', 'p', 'p', 'I', 'D', '0', '0', '0', '0', '0', '0'};
#else
#include "bcm_primitive.h"
#include "bcm_status.h"
#include "bcm_mailbox.h"
#include "drm_bcm_com.h"
#endif

#include "vpp_osal.h"

#ifndef S_OK
#define S_OK 0
#endif

int VPP_DEVKEY_AllocMem (uintptr_t *pShmHandle, unsigned char **ppAddr)
{
    TEE_Result ret = TEE_SUCCESS;
    unsigned char*  phyAddr = NULL;

    if (VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, VPP_DEVKEY_CMD_SIZE, pShmHandle) == SUCCESS) {
        phyAddr = (unsigned char*)VPP_SHM_GetPhysicalAddress((VPP_SHM_HANDLE)pShmHandle, 0);
        if (phyAddr) {
            *ppAddr = (unsigned char*)VPP_SHM_GetVirtualAddress((VPP_SHM_HANDLE)pShmHandle, 0);
        }
    }

    if (!phyAddr) {
        *ppAddr = (unsigned char *)NULL;
        ret = TEE_ERROR_OUT_OF_MEMORY;
    }

    // Current implementation does not return shmHandle. So keep it 0, until implemented
    *pShmHandle = 0;

    return ret;
}

int VPP_DEVKEY_FreeMem (uintptr_t shmHandle)
{
    return VPP_SHM_Free(shmHandle);
}

#ifdef TZ_3_0
int VPP_DEVKEY_Generate(unsigned char *pVirtAddr)
{
    HRESULT result = S_OK;
    UINT32  appVariant = 0;
    unsigned char   *pkey = NULL;
    struct SecureKeyParam sk_param;

    // Clear Key Bytes
    pkey = pVirtAddr;
    VPP_Memset (pkey, 0, 16);

    // Copy the appID
    memcpy(sk_param.appID, gApplicationID, sizeof(sk_param.appID));
    sk_param.appVariant = appVariant;

    result = getSecureKey(&sk_param);
    if (result == TEE_SUCCESS) {
        //TZK already handled cache
        memcpy(pkey, sk_param.secureKey, VPP_DEVKEY_SIZE);
    }

    return ((result == S_OK)?VPP_DEVKEY_SIZE:0);
}
#else
int VPP_DEVKEY_Generate(unsigned char *pVirtAddr)
{
    HRESULT result = S_OK;
    unsigned char   *puuid = NULL;
    unsigned char   *pappID = NULL;
    unsigned char   *pkey = NULL;
    UINT32  appVariant = 0;
    UINT32  offset = 0;

    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS  status;
    TEE_UUID vppID = TAVPP_UUID;

    pkey = pVirtAddr;
    puuid = pVirtAddr + 16;
    pappID = pVirtAddr + 32;

    // Set UUID
    VPP_Memcpy (puuid, (unsigned char*)&vppID.timeLow, 4);
    offset += 4;

    VPP_Memcpy (puuid+offset, (unsigned char*)&vppID.timeMid, 2);
    offset += 2;

    VPP_Memcpy (puuid+offset, (unsigned char*)&vppID.timeHiAndVersion, 2);
    offset += 2;

    VPP_Memcpy (puuid+offset, (unsigned char*)&vppID.clockSeqAndNode, 8);

    // Copy the same to appID
    VPP_Memcpy (pappID, puuid, 16);
    // Clear Key Bytes
    VPP_Memset (pkey, 0, 16);

    bcm_clear_command(&cmd);
    bcm_clear_status(&status);
    cmd.command =  DRM_GENERATE_SECUREKEY;

    cmd.arg0 = (UINT32)TEE_VirtToPhys(puuid);
    cmd.arg1 = (UINT32)TEE_VirtToPhys(pappID);
    cmd.arg2 = appVariant;
    cmd.arg3 = (UINT32)TEE_VirtToPhys(pkey);

    execute_bcm_command(&cmd, &status);
    result = status.return_status;
    return ((result == S_OK)?VPP_DEVKEY_SIZE:0);
}
#endif
#else
int VPP_DEVKEY_AllocMem (uintptr_t *pShmHandle, unsigned char **ppPhyAddr)
{
    return 0;
}

int VPP_DEVKEY_FreeMem (uintptr_t shmHandle);
{
    return 0;
}

int VPP_DEVKEY_Generate(unsigned char *pPhyAddr);
{
    return 0;
}
#endif
#endif //CONFIG_VPP_DEV_KEY_SUPPORT
