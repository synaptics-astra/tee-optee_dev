/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_dhub.c
//!
//! \brief Implementation of dhub commands for TrustZone.
//!
//! Purpose: This is used for controlling access to dhub API in TrustZone.
//!
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "tee_internal_api.h"
#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#include "tee_api_types.h"
#include "tee_api_syna.h"
#endif

#include "dhub_cmd.h"
#include "api_dhub.h"
#include "api_avio_dhub.h"
#include "tee_dhub_memmap.h"

#define LOG_TAG "TA_DHUB"
#include "ta_log.h"

#define VPP_CIEL_TO_4K_ALIGN(val)   ((val+0xFFF)&(~0xFFF))

static TEE_Result DHUB_ValidateAddress(void *addr, size_t size)
{
    TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
    TEE_MemRegion mr;

    if(!addr)
        return res;
    if(TEE_SUCCESS != TEE_FindMemRegion(&mr, addr, size))
        return res;
    return TEE_SUCCESS;
}

/* ***************************************************
 * Create a new instance for the trusted application *
 * ***************************************************/
TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    TEE_Result result = TEE_SUCCESS;
#ifdef TZ_3_0
    result = Dhub_TA_AddMemMap();
#endif
    return result;
}

/* *************************************************
 * Destory the instance of the trusted application *
 * *************************************************/
void TA_EXPORT TA_DestroyEntryPoint(void)
{
    TA_LOGH("%s \n", __FUNCTION__);
    return;
}

/* ********************************************************************
 * open a session with the trusted application when a client requests *
 * ********************************************************************/
TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(uint32_t paramTypes,
                                        TEE_Param params[4],
                                        void **sessionContext)
{
    TA_LOGH("%s \n", __FUNCTION__);
    return TEE_SUCCESS;
}

/* *****************************************************
 * Close a client session with the trusted application *
 * *****************************************************/
void TA_EXPORT TA_CloseSessionEntryPoint(void *sessionContext)
{
    TA_LOGH("%s \n", __FUNCTION__);
    return;
}

/* *****************************************
 * Invoke a command within a given session *
 * *****************************************/
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(void *sessionContext,
                                          uint32_t commandID,
                                          uint32_t paramTypes,
                                          TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;//TEE_ERROR_NOT_SUPPORTED;
    HDL_semaphore *pSemHandle;
    HDL_dhub *pdHubHandle = NULL;
    HDL_hbo *pHBO = NULL;
    unsigned int channel_init_mask_vpp = 0;

#if 0
    TA_LOGH("%s, cmd:%d, para0a=%x, para0b=%x, para1a=%x, para1b=%x,\n",
        __FUNCTION__, commandID, params[0].value.a, params[0].value.b
        , params[1].value.a, params[1].value.b);
#endif
    switch (commandID)
    {
        case DHUB_INIT:
            /*strictly init VPP dhub; params[0].value.a: CPUINDEX from kernel driver*/
            DhubInitialization(params[0].value.a, VPP_DHUB_BASE, VPP_HBO_SRAM_BASE,
                               &VPP_dhubHandle, VPP_config, VPP_NUM_OF_CHANNELS, channel_init_mask_vpp);
#ifndef CONFIG_OPTEE
            uint8_t *shmAddr = NULL;
            uint32_t size = 0;
            shmAddr = (uint8_t*)params[2].memref.buffer;
            if (shmAddr == NULL || params[2].memref.size == 0) {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }
            size = params[2].memref.size;
            res = DHUB_ValidateAddress(shmAddr, size);
            if (res != TEE_SUCCESS)
                break;
            memcpy(shmAddr, &VPP_dhubHandle, size);
#endif
            break;

        case DHUB_SEM_QUERY:
            pdHubHandle = & VPP_dhubHandle.dhub;
            if(!pdHubHandle){
                res = TEE_ERROR_ACCESS_DENIED;
                break;
            }
            pHBO = &(pdHubHandle->hbo);
            params[1].value.a = semaphore_query(hbo_fifoCtl(pHBO),
                                                    params[0].value.a, params[0].value.b,
                                                    NULL);
            break;

        case DHUB_AUTOPUSH_EN:
            break;

        case DHUB_CHANNEL_WRITECMD:
            /*in amp_driver.c, chanID =avioDhubChMap_vpp_HDMI_R for aout*/
            /*  chanID = avioDhubChMap_vpp_SPDIF_W for aip*/
            /* for HDMI Audio out, the data inside is PCM buffer, zero buffer,taginfo*/
            if(!params[0].value.b){
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }
            params[3].value.b = dhub_channel_write_cmd(&(VPP_dhubHandle.dhub),
                                            params[0].value.a, params[0].value.b,
                                            params[1].value.a, params[1].value.b,
                                            params[2].value.a, params[2].value.b,
                                            params[3].value.a, 0, 0);
           break;

        case DHUB_SEM_POP:
            pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
            if(!pSemHandle){
                res = TEE_ERROR_ACCESS_DENIED;
                break;
            }
            semaphore_pop(pSemHandle, params[0].value.a, params[0].value.b);
            break;

        case DHUB_SEM_CLR_FULL:
            pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
            if(!pSemHandle){
                res = TEE_ERROR_ACCESS_DENIED;
                break;
            }
            semaphore_clr_full(pSemHandle,params[0].value.a);
            break;

        case DHUB_SEM_CHK_FULL:
            pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
            if(!pSemHandle){
                res = TEE_ERROR_ACCESS_DENIED;
                break;
            }
            params[1].value.a = semaphore_chk_full(pSemHandle,params[0].value.a);
            break;

        case DHUB_CHANNEL_GENERATECMD:
            {
                int cmd[2];
                void *data_addr = (void*)params[0].value.b;
                size_t data_size = params[1].value.a;
                if(TEE_ERROR_BAD_PARAMETERS == DHUB_ValidateAddress(data_addr, data_size)) {
                    res = TEE_ERROR_BAD_PARAMETERS;
                    break;
                }
                dhub_channel_generate_cmd(&(VPP_dhubHandle.dhub),
                                    params[0].value.a, params[0].value.b,
                                    params[1].value.a, params[1].value.b,
                                    params[2].value.a, params[2].value.b,
                                    params[3].value.a, &cmd[0]);
                params[0].value.a = cmd[0];
                params[0].value.b = cmd[1];
            }
            break;

        case DHUB_CHANNEL_CLEAR:
            DhubChannelClear(&VPP_dhubHandle.dhub, params[0].value.a, NULL);
            break;

        case DHUB_SEM_INTR_ENABLE:
            //params[2].value.b is passed a parameter to cpu; max 'cpu' is 2
            if(params[2].value.b > 2) {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

            pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
            if(!pSemHandle){
                res = TEE_ERROR_ACCESS_DENIED;
                break;
            }
            semaphore_intr_enable(pSemHandle,
                            params[0].value.a, params[0].value.b,
                            params[1].value.a, params[1].value.b,
                            params[2].value.a, params[2].value.b);
            break;

        case DHUB_CHANNEL_START_SEQ:
            dhub2d_channel_start_seq(&VPP_dhubHandle, params[0].value.a);
            break;
        case DHUB_CHANNEL_CLEAR_SEQ:
            dhub2d_channel_clear_seq(&VPP_dhubHandle, params[0].value.a);
            break;
        case DHUB_2ND_CHANNEL_START_SEQ:
            dhub2nd_channel_start_seq(&VPP_dhubHandle, params[0].value.a);
            break;
        case DHUB_2ND_CHANNEL_CLEAR_SEQ:
            dhub2nd_channel_clear_seq(&VPP_dhubHandle, params[0].value.a);
            break;
        case DHUB_BCM_SCHED_PUSHCMD:
            {
                unsigned int cmd[2];
                int ret = 0;
                cmd[0] = params[0].value.b;
                cmd[1] = params[1].value.a;
                ret = BCM_SCHED_PushCmd(params[0].value.a, cmd, NULL/*params[1].value.b*/);
                params[1].value.b = ret;
            }
            break;
        case DHUB_BCM_SCHED_GETEMPTYSTS:
            {
                unsigned int val = 0;
                BCM_SCHED_GetEmptySts(params[0].value.a, &val);
                params[0].value.b = val;
            }
            break;
        case DHUB_BCM_SCHED_SETMUX:
                BCM_SCHED_SetMux(params[0].value.a, params[0].value.b);
            break;
        case DHUB_BCM_SCHED_AUTOPUSHCMD:
            {
                int ret = 0;
                ret = BCM_SCHED_AutoPushCmd(params[0].value.a, params[0].value.b);
                params[1].value.a = ret;
            }
            break;
        default:
            res = TEE_ERROR_NOT_SUPPORTED;
            break;
    }
    return res;
}

#ifndef CONFIG_OPTEE
TA_DEFINE_BEGIN
    .uuid              = TA_DHUB_UUID,
    .name              = "DHUB",
    .singleInstance    = false,
    .multiSession      = false,
    .instanceKeepAlive = false,
TA_DEFINE_END
#endif
