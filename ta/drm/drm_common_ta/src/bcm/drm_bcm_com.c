/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 */
#include <string.h>

#ifdef CONFIG_OPTEE
#include "tee_api.h"
#include "tee_internal_api.h"
#else // CONFIG_TZK
#include "tee_common.h"
#include "tee_internal_common.h"
#include "tee_internal_private_api.h"
#endif

#include "drm_tee_types.h"
#include "drm_tee_log.h"
#include "drm_bcm_com.h"
#include "bcm_status.h"
#include "bcm_primitive.h"

extern void *memcpy(void *dest, const void *src, size_t n);

void bcm_memclr(void *p, UINT32 numInt)
{
    volatile UINT32 *pStart = (UINT32 *)p;
    volatile UINT32 *pEnd = pStart + numInt;

    while (pStart < pEnd) {
        *pStart++ = 0;
    }

    return;
}

void bcm_clear_status(BCM_MAILBOX_STATUS * pStatus)
{
    bcm_memclr(pStatus, sizeof(BCM_MAILBOX_STATUS)/sizeof(UINT32));
}

void bcm_clear_command(BCM_MAILBOX_COMMAND * pcmd)
{
    bcm_memclr(pcmd, sizeof(BCM_MAILBOX_COMMAND)/sizeof(UINT32));
}

/** Execute bcm command, packing cmd by caller.
 *
 * @param pshm  secure cache share memory pointer using to pack parameter.
 * @param pcmd  packed bcm command.
 * @param inoffset  input offset from pshm
 * @param inlen input length
 * @param outoffset input offset from pshm
 * @param outlen    output length
 * @retval S_OK         on success
 */
HRESULT drm_bcm_exec_cmd_base(UINT8* pshm, BCM_MAILBOX_COMMAND *pcmd,
                         UINT32 inoffset, UINT32 inlen,
                         UINT32 outoffset, UINT32 outlen)
{
    BCM_MAILBOX_STATUS status;
    TEE_Result ret = TEE_SUCCESS;

    bcm_clear_status(&status);

    if (inlen > 0) {
        ret = TEE_FlushCache(pshm + inoffset, inlen);
        if (ret != TEE_SUCCESS) {
            return E_FAIL;
        }
    }

    ret = TEE_ExecCmd(pcmd->command, (void *)pcmd,
                      sizeof(BCM_MAILBOX_COMMAND) - sizeof(UINT32),
                      (void*)&status, sizeof(status));
    if (ret == TEE_SUCCESS) {
        if (outlen > 0) {
            ret = TEE_InvalidateCache(pshm + outoffset, outlen);
            if (ret != TEE_SUCCESS) {
                return E_FAIL;
            }
        }
        if (pcmd->command == DRM_GET_VERSION) {
            DRM_LOG("%s:%d, ver:0x%08x\n", __FUNCTION__, __LINE__, status.arg0);
        }
    }

    return ret;
}

static void s_process_arg(UINT8* pshm, UINT32 boutarg,
                              UINT8* pargin, UINT32 inlen,
                              UINT32 *parg, UINT8 **ppout, UINT32 *pinlen,
                              UINT32 *poutlen, UINT32 *poutos)
{
    DRM_ASSERT(pshm);
    DRM_ASSERT(parg);
    DRM_ASSERT(pinlen);
    DRM_ASSERT(poutlen);

    if(NULL == pargin) {//then len should be immediate number assigned to arg.
        if (inlen != 0) {//NOTE default 0
            *parg = inlen;
        }
        return;
    }

    if (inlen > 0) {
        *parg = (UINT)pshm + *pinlen;
        if (boutarg) {
            *poutlen = inlen;
            *ppout = (UINT8*)pargin;
            *poutos = *pinlen;
        }
        else {
            memcpy(pshm + *pinlen, pargin, inlen);
            *pinlen +=inlen;
        }
    }

    return;
}

HRESULT drm_bcm_exec_cmd(UINT8* pshm, UINT32 command, UINT32 outarg,
                        UINT8* parg0, UINT32 len0,
                        UINT8* parg1, UINT32 len1,
                        UINT8* parg2, UINT32 len2,
                        UINT8* parg3, UINT32 len3,
                        UINT8* parg4, UINT32 len4,
                        UINT8* parg5, UINT32 len5,
                        UINT8* args[10], UINT32 lens[10])
{
    BCM_MAILBOX_COMMAND cmd;
    HRESULT lRes = S_OK;
    UINT32 inlen = 0;
    UINT32 outlen = 0;
    UINT32 outos = 0;   //copy src buf offset
    UINT8* pout = NULL; //copy dst buf.

    bcm_clear_command(&cmd);

    DRM_LOG("%s:%d, pshm:%p\n", __FUNCTION__, __LINE__, pshm);
    cmd.command = command;

    s_process_arg(pshm, outarg == 0, parg0, len0, &cmd.arg0, &pout, &inlen, &outlen,&outos);
    s_process_arg(pshm, outarg == 1, parg1, len1, &cmd.arg1, &pout, &inlen, &outlen,&outos);
    s_process_arg(pshm, outarg == 2, parg2, len2, &cmd.arg2, &pout, &inlen, &outlen,&outos);
    s_process_arg(pshm, outarg == 3, parg3, len3, &cmd.arg3, &pout, &inlen, &outlen,&outos);
    s_process_arg(pshm, outarg == 4, parg4, len4, &cmd.arg4, &pout, &inlen, &outlen,&outos);
    s_process_arg(pshm, outarg == 5, parg5, len5, &cmd.arg5, &pout, &inlen, &outlen,&outos);

    if (NULL != args && NULL != lens) {
        s_process_arg(pshm, outarg==6, args[0], lens[0], &cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==7, args[1], lens[1], &cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==8, args[2], lens[2], &cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==9, args[3], lens[3], &cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==10, args[4], lens[4],&cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==11, args[5], lens[5],&cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==12, args[6], lens[6],&cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==13, args[7], lens[7],&cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==14, args[8], lens[8],&cmd.arg6, &pout, &inlen, &outlen,&outos);
        s_process_arg(pshm, outarg==15, args[9], lens[9],&cmd.arg6, &pout, &inlen, &outlen,&outos);
    }

    lRes = drm_bcm_exec_cmd_base(pshm, &cmd, 0, inlen, outos, outlen);
    if (lRes != S_OK) {
        return lRes;
    }

    if (DRM_BCM_EXEC_CMD_OUT_ARG_INVALID != outarg) {
        memcpy(pout, pshm + outos, outlen);
    }

    return S_OK;
}

/** Execute bcm command.
 *
 * @param pcmd  packed bcm command.
 * @retval S_OK         on success
 */
void execute_bcm_command(BCM_MAILBOX_COMMAND *pcmd,
                         BCM_MAILBOX_STATUS *status)
{
    TEE_Result ret = TEE_SUCCESS;

    ret = TEE_ExecCmd(pcmd->command, (void *)pcmd,
                      sizeof(BCM_MAILBOX_COMMAND) - sizeof(UINT32),
                      (void*)status, sizeof(*status));
    if (ret != TEE_SUCCESS) {
        DRM_LOG("TADEV: execute_bcm_command: TEE_ExecCmd(%x) returned %x\n",
                pcmd->command, status->return_status);
    }
    return;
}

/** Execute bcm command.
 *
 * @param pcmd  packed bcm command.
 * @retval S_OK         on success
 */
void execute_bcm_command_Ex(BCM_MAILBOX_COMMAND *pcmd,
                         BCM_MAILBOX_STATUS *status, UINT32 delay)
{
    TEE_Result ret = TEE_SUCCESS;

    ret = TEE_ExecCmdEx(pcmd->command, (void *)pcmd,
                      sizeof(BCM_MAILBOX_COMMAND) - sizeof(UINT32),
                      (void*)status, sizeof(*status), delay);
    if (ret != TEE_SUCCESS) {
        DRM_LOG("TADEV: execute_bcm_command_Ex: TEE_ExecCmd_Ex(%x) returned %x\n",
                pcmd->command, status->return_status);
    }
    return;
}
