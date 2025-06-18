/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_dhub_memmap.c
//!
//! \brief Implementation of VPP memory mapping
//!
//! Purpose: This is used for mapping dhub register address space to Truzt Zone Kernel
//!
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "tee_internal_api.h"
#include "Galois_memmap.h"
#include "avio_memmap.h"

#ifdef CONFIG_OPTEE
#include "tee_api_types.h"
#include "tee_api_syna.h"
#include "tee_ta_mmap.h"
#endif

#define LOG_TAG "TA_DHUB"
#include "ta_log.h"

#define VPP_CIEL_TO_4K_ALIGN(val)   ((val+0xFFF)&(~0xFFF))


/* ***************************************************
 * Create a new instance for the trusted application *
 * ***************************************************/
TEE_Result Dhub_TA_AddMemMap(void)
{
    void *va;
    TEE_Result result =TEE_SUCCESS;
    int Memmap_count=0;
    unsigned int temp_size;

#ifdef CONFIG_OPTEE
    result=TEE_TA_Register_RegRegion((void*)MEMMAP_AVIO_REG_BASE, 0xc0000, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;
#else
    result=TEE_Map_Register((void*)MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP128B_DHUB_REG_BASE, AVIO_MEMMAP_VPP128B_DHUB_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
       goto failure;
    else
       Memmap_count++;

    result=TEE_Map_Register((void*)MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AIO64B_DHUB_REG_BASE, AVIO_MEMMAP_AIO64B_DHUB_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
       goto failure;
    else
       Memmap_count++;

    temp_size = AVIO_MEMMAP_AVIO_I2S_REG_SIZE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_SIZE + AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_SIZE +
               AVIO_MEMMAP_AVIO_BCM_REG_SIZE + AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_SIZE + AVIO_MEMMAP_AVIO_SNPS_TRNG_REG_SIZE;
    temp_size = VPP_CIEL_TO_4K_ALIGN(temp_size);
    result=TEE_Map_Register((void*)(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_I2S_REG_BASE), temp_size, 0, 0, &va);
    if(result != TEE_SUCCESS)
       goto failure;
    else
       Memmap_count++;
#endif
    TA_LOGH("%s \n", __FUNCTION__);
    return TEE_SUCCESS;
failure :
    TA_LOGE("%s : TEE_Map_Register  failed result [%d] at Memmap_count [%d] \n", __FUNCTION__,result,Memmap_count);
    return result;
}
