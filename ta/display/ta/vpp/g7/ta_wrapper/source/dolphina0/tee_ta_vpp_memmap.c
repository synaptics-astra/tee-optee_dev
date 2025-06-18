/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_vpp_memmap.c
//!
//! \brief Implementation of VPP memory mapping
//!
//! Purpose: This is used for mapping VPP register address space to Truzt Zone Kernel
//!
//!     Version    Date                     Author
//!     V 1.00,    Nov 12 2013,             Jiankang Yu
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "tee_ta_vpp.h"
#include "Galois_memmap.h"
#include "avio_memmap.h"
#include "tee_ta_mmap_helper.h"

//Consolidate adjacent memory space to single memmap
#define VPP_ENABLE_OPTIMIZED_MEMMAP

#define        SM_MEMMAP_SM_CEC_REG_BASE                   0x61000
#define        SM_MEMMAP_SM_CEC_REG_SIZE                   0x1000

#define        MEMMAP_CHIP_CTRL_REDUCED_REG_SIZE           0x1000

/* ***************************************************
 * Create a new instance for the trusted application *
 * ***************************************************/
TEE_Result VPP_TA_AddMemMap(void)
{
    void *va;
    TEE_Result result =TEE_SUCCESS;
    int Memmap_count=0;
    unsigned int temp_size;

    /*VPP_DHUB*/
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_VPP128B_DHUB_REG_BASE), AVIO_MEMMAP_VPP128B_DHUB_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /*AIO_DHUB */
    result=TEE_TA_Register_RegRegion((void*)( MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AIO64B_DHUB_REG_BASE), AVIO_MEMMAP_AIO64B_DHUB_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

#ifndef VPP_ENABLE_OPTIMIZED_MEMMAP

    /*HDMI TX and EDDC*/
    result=TEE_TA_Register_RegRegion((void*)( MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_SNPS_HDMITX_REG_BASE), AVIO_MEMMAP_AVIO_SNPS_HDMITX_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /*VPP registers*/
    result=TEE_TA_Register_RegRegion((void*)( MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_VPP_REG_BASE), AVIO_MEMMAP_VPP_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /* VPP_MP */
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_VPP_MP_REG_BASE), AVIO_MEMMAP_VPP_MP_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /* VPP_GFX */
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE), AVIO_MEMMAP_VPP_GFX_VOP_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

#else //VPP_ENABLE_OPTIMIZED_MEMMAP

    /* HDMI TX and EDDC */
    /* VPP registers*/
    /* VPP_MP */
    /* VPP_GFX */
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_SNPS_HDMITX_REG_BASE), AVIO_MEMMAP_AVIO_SNPS_HDMITX_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    temp_size = AVIO_MEMMAP_VPP_REG_SIZE + AVIO_MEMMAP_VPP_MP_REG_SIZE + AVIO_MEMMAP_VPP_GFX_VOP_REG_SIZE;
    temp_size = VPP_CIEL_TO_4K_ALIGN(temp_size);
    result=TEE_TA_Register_RegRegion((void*)( MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_VPP_MP_REG_BASE), temp_size, 0, 0, &va);

#endif //VPP_ENABLE_OPTIMIZED_MEMMAP

    /*AVIO_GBL, AVIO_HDMIRX, AVIO_EARC */
    temp_size = AVIO_MEMMAP_AVIO_GBL_SIZE + AVIO_MEMMAP_AVIO_HDMIRX_REG_SIZE + AVIO_MEMMAP_AVIO_EARC_RX_REG_SIZE;
    result=TEE_TA_Register_RegRegion((void*)( MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_GBL_BASE), temp_size, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

#ifndef VPP_ENABLE_OPTIMIZED_MEMMAP

    /* AVIO_I2S */
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_I2S_REG_BASE),AVIO_MEMMAP_AVIO_I2S_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /* HDMIRX_PIPE */
    result=TEE_TA_Register_RegRegion((void*)(AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE),AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /* HDMITX_WRAP */
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_BASE),AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /* AVIO_BCM */
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_BCM_REG_BASE),AVIO_MEMMAP_AVIO_BCM_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

#else //VPP_ENABLE_OPTIMIZED_MEMMAP
    /* I2S */
    /* HDMI RX_PIPE */
    /* HDMI TX_WRAP */
    /* BCM */
    temp_size = AVIO_MEMMAP_AVIO_I2S_REG_SIZE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_SIZE + AVIO_MEMMAP_AVIO_HDMITX_WRAP_REG_SIZE +
                    AVIO_MEMMAP_AVIO_BCM_REG_SIZE;
    temp_size = VPP_CIEL_TO_4K_ALIGN(temp_size);
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_I2S_REG_BASE), temp_size, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

#endif //VPP_ENABLE_OPTIMIZED_MEMMAP
    /* HDMI HDCP22 */
    temp_size = AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_SIZE;
    temp_size = VPP_CIEL_TO_4K_ALIGN(temp_size);/* Covers HDMI_HDCP22, SNPS_TRNG, HDMIRX_WRAP */
    result=TEE_TA_Register_RegRegion((void*)(MEMMAP_AVIO_REG_BASE+AVIO_MEMMAP_AVIO_SNPS_HDCP22_REG_BASE), temp_size, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /*SM_CEC*/
    result = TEE_TA_Register_RegRegion((void*)(MEMMAP_SM_REG_BASE + SM_MEMMAP_SM_CEC_REG_BASE), SM_MEMMAP_SM_CEC_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    /*GLOBAL CTRL*/
    result = TEE_TA_Register_RegRegion((void*)MEMMAP_CHIP_CTRL_REG_BASE, MEMMAP_CHIP_CTRL_REDUCED_REG_SIZE, 0, 0, &va);
    if(result != TEE_SUCCESS)
        goto failure;
    else
        Memmap_count++;

    return TEE_SUCCESS;
failure :
    VPPAPI_DBG_FIELD("%s : TEE_TA_Register_RegRegion  failed result [0x%x] at Memmap_count [%d] \n", __FUNCTION__,result,Memmap_count);
    return result;
}
