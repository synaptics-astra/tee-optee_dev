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

/*******************************************************************************
*! \author    Haifeng Tang
*! \date      Sep. 2014
*******************************************************************************/



#ifndef __DRM_BCM_COM_H__
#define __DRM_BCM_COM_H__

#ifdef CONFIG_OPTEE
#include "tee_internal_api.h"
#include "tee_internal_api_extensions.h"
#include "tee_api_syna.h"
#else // CONFIG_TZK
#include "tee_common.h"
#include "tee_internal_common.h"
#endif

#include "drm_tee_types.h"
#include "bcm_mailbox.h"


#define DRM_BCM_EXEC_CMD_OUT_ARG_INVALID -1

/*
 * Move below definition from pr25/inc/bcm_hal_common.h since they will be
 * reused by PR32
 */
/*Constant Size definition*/
#define ECC256_PUB_KEY_SIZE_IN_BYTE                64
#define ECC256_PRV_KEY_SIZE_IN_BYTE                32
#define ECC256_ENCRYPTED_SIZE_IN_BYTE              128
#define ECC256_PLAINTEXT_SIZE_IN_BYTE              32
#define AES128_KEY_SIZE_IN_BYTE                    16
#define OMAC_SIGNATURE_SIZE_IN_BYTE                16
#define MAX_SECURE_STORE_SIZE_IN_BYTE              256
#define MAX_DEVICE_KEY_SIZE_IN_BYTE                128   /*Group Key and Fallback Key are packed together*/
#define MAX_MODEL_SIGN_MSG_SIZE_IN_BYTE            512
#define ECC256_SIGNATURE_SIZE_IN_BYTE              64
#define PR25_CONTENT_KEY_ID_SIZE_IN_BYTE           16
#define BCM_AES_CK_CHECKSUM_LENGTH                 8
#define MAX_OMAC_SIGN_DATA_SIZE_IN_BYTE            0x600  /*enlarge it from 1024 to PR32 case*/
#define MAX_ECC_VERIFY_SIGN_DATA_SIZE_IN_BYTE      1024

#define BCM_CMD_PARAM_BUF_SIZE_IN_BYTE             2048


typedef enum _AES_MODE {
    AES_ECB,
    AES_CBC,
    AES_CTR,
    AES_XTS,
    AES_CAPTIVE_KEYWRAP,
    AES_CBC_NIST_CS2,
    AES_BCC,
    AES_CTR64,
    AES_CTR128,
    AES_MODES_LAST_ONE
} AES_MODE;

/** Execute bcm command. Do not use this function directly.
 *  Must call bcm_clear_command before calling this function.
 *  For complex case call drm_bcm_exec_cmd_base();
 *
 * @param pshm  secure cache share memory pointer using to pack parameter.
 * @param command   bcm command primtive ID.
 * @param outarg    select out argument number, at most one out argument support.
 *                   e.g. outarg = 0 if arg0.
 *                        outarg = DRM_BCM_EXEC_CMD_OUT_ARG_INVALID if no out argument.
 * @param argx     Copy pargx and set cmd.arg pointer to new buffer,
 *                 if (argx == NULL) cmd.argx = lenx;
 * @param lenx    argx buffer length or immedate number.
 * @param args  Argument array from 6 to 15
 * @param lens  Length array to args.
 * @retval S_OK         on success
 */
HRESULT drm_bcm_exec_cmd(UINT8* pshm, UINT32 command, UINT32 outarg,
                        UINT8* parg0, UINT32 len0,
                        UINT8* parg1, UINT32 len1,
                        UINT8* parg2, UINT32 len2,
                        UINT8* parg3, UINT32 len3,
                        UINT8* parg4, UINT32 len4,
                        UINT8* parg5, UINT32 len5,
                        UINT8* args[10], UINT32 lens[10]);


/** Execute bcm command directly.
 * @param c bcm command primtive ID.
 */
#define DRM_BCM_EXEC_CMD_DIRECT(c)  \
                        drm_bcm_exec_cmd_base(0, c, 0, 0, 0, 0)

/** Execute bcm command, packing by callee.
 */
#define DRM_BCM_EXEC_CMD_0(s, c, o, a0, l0)  \
                        drm_bcm_exec_cmd(s, c, o, a0, l0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,NULL,NULL)

#define DRM_BCM_EXEC_CMD_1(s, c, o, a0, l0, a1, l1)  \
                        drm_bcm_exec_cmd(s, c, o, a0, l0, a1, l1, 0, 0, 0, 0, 0, 0, 0, 0,NULL,NULL)

#define DRM_BCM_EXEC_CMD_2(s, c, o, a0, l0, a1, l1, a2, l2)  \
                        drm_bcm_exec_cmd(s, c, o, a0, l0, a1, l1, a2, l2, 0, 0, 0, 0, 0, 0, \
                                         NULL,NULL)

#define DRM_BCM_EXEC_CMD_3(s, c, o, a0, l0, a1, l1, a2, l2, a3, l3)  \
                        drm_bcm_exec_cmd(s, c, o, a0, l0, a1, l1, a2, l2, a3, l3, 0, 0, 0, 0,\
                                        NULL,NULL)

#define DRM_BCM_EXEC_CMD_4(s, c, o, a0, l0, a1, l1, a2, l2, a3, l3, a4, l4)  \
                        drm_bcm_exec_cmd(s, c, o, a0, l0, a1, l1, a2, l2, a3, l3, a4, l4, 0, 0, \
                                        NULL,NULL)

#define DRM_BCM_EXEC_CMD_5(s, c, o, a0, l0, a1, l1, a2, l2, a3, l3, a4, l4, a5, l5)  \
                        drm_bcm_exec_cmd(s, c, o, a0, l0, a1, l1, a2, l2, a3, l3, a4, l4, a5, l5,\
                                        NULL,NULL)

#define DRM_BCM_EXEC_CMD_ALL  drm_bcm_exec_cmd


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
                             UINT32 outoffset, UINT32 outlen);

/** Execute bcm command.
 *
 * @param pcmd  packed bcm command.
 * @retval S_OK         on success
 */
void execute_bcm_command(BCM_MAILBOX_COMMAND *pcmd,
                         BCM_MAILBOX_STATUS *status);

void execute_bcm_command_Ex(BCM_MAILBOX_COMMAND *pcmd,
                         BCM_MAILBOX_STATUS *status, UINT32 delay);
#endif  //__DRM_BCM_COM_H__
