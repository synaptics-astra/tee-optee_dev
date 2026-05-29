/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
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
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

////////////////////////////////////////////////////////////////////////////////
//! \file dmx_cmd.h
//!
//! \brief Header file of commands for TrustZone.
//!
//! Purpose: This is used for controlling access to DMX security sensitive
//!          resource in TrustZone.
//!
//!     Version    Date                     Author
//!     V 1.00,    Nov 18 2013,             Wenjie Zhou
//!     V 2.00,   July 30 2015,             Long Xiao

//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __AMP_TSP_TEE_CMD_H__
#define __AMP_TSP_TEE_CMD_H__

#define TA_TSP_UUID {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xe8}}
#define TA_M2M_UUID {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xe9}}

// BCM command to set/get FIGO state, shared by TSP TA and M2M TA
#define BCM_PI_SET_TSP_STATE 0x0030
#define BCM_PI_GET_TSP_STATE 0x0031
#define BCM_PI_SET_TSP_CLK   0x0035
#define BCM_PI_SET_TSP_REF_CLK 0x0036
#define BCM_KERNEL_PI_VERIFY_TSP_FW 0x10
// FIGO state
#define FIGO_RELEASE 1
#define FIGO_RESET   0

enum {
    TSP_CLK_SET_DISABLE = 0,
    TSP_CLK_SET_DIV12 = 1,
    TSP_CLK_SET_DIV4 = 2,
    TSP_CLK_SET_FULL = 3,
    TSP_CLK_SET_MAX
};

enum {
    TSP_REF_CLK_SET_DISABLE = 0,
    TSP_REF_CLK_SET_MAX
};

enum {
    TSP_INVALID,
    TSP_DEBUG,
    TSP_HBO_QUEUE_ENABLE,
    TSP_HBO_QUEUE_QUERY,
    TSP_HBO_QUEUE_CLEAR,
    TSP_HBO_QUEUE_CLEAR_DONE,
    TSP_HBO_QUEUE_CFG,
    TSP_DTCM_MEM_ALLOC,
    TSP_DTCM_MEM_FREE,
    TSP_FW_LOAD,
    TSP_FIGO_SCHED,
    TSP_FIGO_UNSCHED,
    TSP_ALLOC_TASK,
    TSP_FREE_TASK,
    TSP_UPDATE_KEY_PROTECT_FLAG,
    TSP_REMOVE_KEY_PROTECT_FLAG,
    TSP_UPDATE_COPY_PROTECTION_FLAG,
    TSP_GET_PRIVATE_AREAS,
    TSP_MEM_WRAP_MOVE,
    TSP_GET_OUTBUF_SECURE_PROP,
    TSP_SECTION_GET_SYNTAX_INDICATOR,
    TSP_SECTION_GET_DATA_INVALID_MASK,
    TSP_CHECK_GOP_HDR,

    /* the following 3 commands can only be called by NSK TA */
    /* CA to TA access, debug only (if params[1].a == TSP_MEMREF_FROM_CA):
     *   params[0]: TEE_PARAM_TYPE_MEMREF_INPUT
     * TA to TA access (if params[1].a == TSP_SHM_FROM_TA):
     *   params[0]: TEE_PARAM_TYPE_VALUE_INPUT
     *     params[0].a: Id of the TEE SharedMemory buffer
     *     params[0].b: size of the TEE SharedMemory buffer
     * The input data format: CmChannel data, pls refer to NSKCmChannel in tsp_nsk.h
     *                            set the corresponding masks to tell TSP TA which fields are valid
     *                            - at least one of the following 3 masks shall be set to NSKCmChanel.validMask
     *                                - EVEN_PARITY_MASK
     *                                - ODD_PARITY_MASK
     *                                - CLEAR_PARITY_MASK
     *                              which means at least one NSKOneParity is valid
     *                            - For one valid NSKOneParity
     *                                - at least one of the following 3 masks shall be set to NSKOneParity.validMask
     *                                    - LOCAL_D_MASK
     *                                    - ESA_MASK
     *                                    - LOCAL_S_MASK
     *                                which means at least one NSKOneCrypto is valid
     *                                - KEY_INDEX_MASK and SCB_OUT_MASK shall be set to NSKOneParity.validMask
     *                            - For one valid NSKOneCrypto
     *                                - ALGO_MASK shall be set to NSKOneCrypto.validMask
     *
     *                            Here is an example:
     *                            if you want to set mandatory fields for even and odd parity
     *                            and additionally set iv1 for LocalD, even parity and iv2 for ESA, odd parity:
     *                                NSKCmChannel.validMask = EVEN_PARITY_MASK | ODD_PARITY_MASK; // clear parity not set
     *
     *                                NSKCmChannel.even.validMask = LOCAL_D_MASK | KEY_INDEX_MASK | SCB_OUT_MASK;
     *                                NSKCmChannel.even.keyIdx = keyIndexEvenLocalD; // mandatory
     *                                NSKCmChannel.even.scbOut = scbOutEvenLocalD; // mandatory
     *                                NSKCmChannel.even.localD.validMask = IV1_MASK | ALGO_MASK;
     *                                NSKCmChannel.even.localD.algoId = algoIdEvenLocalD; // mandatory
     *                                memcpy(NSKCmChannel.even.localD.iv1, iv1, 16); // optional
     *
     *                                NSKCmChannel.odd.validMask = ESA_MASK | KEY_INDEX_MASK | SCB_OUT_MASK;
     *                                NSKCmChannel.odd.keyIdx = keyIndexOddESA; // mandatory
     *                                NSKCmChannel.odd.scbOut = scbOutOddESA; // mandatory
     *                                NSKCmChannel.odd.ESA.validMask = IV2_MASK | ALGO_MASK;
     *                                NSKCmChannel.odd.ESA.algoId = algoIdOddESA; // mandatory
     *                                memcpy(NSKCmChannel.odd.ESA.iv2, iv2, 16); // optional
     *
     * param[1]: TEE_PARAM_TYPE_VALUE_INPUT
     *   param[1].a: type of the input buffer, see comments above
     * params[2]: TEE_PARAM_TYPE_VALUE_OUTPUT
     *   params[2].a: index of the CmChannel created in CmChannelTable in TSP TA
     */
    TSP_NSK_CMCHANNEL_ADD,

    /* params[0]: TEE_PARAM_TYPE_VALUE_INPUT
     *   params[0].a: index of the CmChannel to be removed from CmChannelTable in TSP TA
     */
    TSP_NSK_CMCHANNEL_REMOVE,

    /* CA to TA access, debug only (if params[1].a == TSP_MEMREF_FROM_CA):
     *   params[0]: TEE_PARAM_TYPE_MEMREF_INPUT
     * TA to TA access (if params[1].a == TSP_SHM_FROM_TA):
     *   params[0]: TEE_PARAM_TYPE_VALUE_INPUT
     *     params[0].a: Id of the TEE SharedMemory buffer
     *     params[0].b: size of the TEE SharedMemory buffer
     * The input data format: CmChannel data, pls refer to NSKCmChannel in tsp_nsk.h
     *                            set the corresponding masks to tell TSP TA which fields are to be updated
     *                            for example, if you want to update iv1 for LocalD, even parity and
     *                            iv2 for ESA, odd parity, scbOut for LocalS, clear parity:
     *                                NSKCmChannel.validMask = EVEN_PARITY_MASK | ODD_PARITY_MASK | CLEAR_PARITY_MASK;
     *
     *                                NSKCmChannel.even.validMask = LOCAL_D_MASK;
     *                                NSKCmChannel.even.localD.validMask = IV1_MASK;
     *                                memcpy(NSKCmChannel.even.localD.iv1, iv1, 16);
     *
     *                                NSKCmChannel.odd.validMask = ESA_MASK;
     *                                NSKCmChannel.odd.ESA.validMask = IV2_MASK;
     *                                memcpy(NSKCmChannel.odd.ESA.iv2, iv2, 16);
     *
     *                                NSKCmChannel.clear.validMask = LOCAL_S_MASK;
     *                                NSKCmChannel.clear.localS.validMask = ALGO_MASK;
     *                                NSKCmChannel.clear.localS.scbOut = scbOutClearLocalS;
     *
     * param[1]: TEE_PARAM_TYPE_VALUE_INPUT
     *   param[1].a: type of the input buffer, see comments above
     * params[2]: TEE_PARAM_TYPE_VALUE_INPUT
     *   params[2].a: index of the CmChannel to be updated
     */
    TSP_NSK_CMCHANNEL_UPDATE,

    /* params[0]: TEE_PARAM_TYPE_VALUE_INPUT
     *   params[0].a: index of the CmChannel to connect in CmChannelTable in TSP TA
     *   params[0].b: figo id
     * params[1]: TEE_PARAM_TYPE_VALUE_OUTPUT
     *   params[1].a: address of CmChannel in DTCM
     */
    TSP_NSK_CMCHANNEL_CONNECT,

    /* params[0]: TEE_PARAM_TYPE_VALUE_INPUT
     *   params[0].a: index of the CmChannel to disconnect from CmChannelTable in TSP TA
     *   params[0].b: figo id
     */
    TSP_NSK_CMCHANNEL_DISCONNECT,

    /* params[0]: TEE_PARAM_TYPE_VALUE_INPUT
     *   params[0].a: index of the CmChannel to get the property from CmChannelTable in TSP TA
     *   params[1].a: key usage
     */
    TSP_NSK_CMCHANNEL_GETPROP,

    TSP_GET_SECURE_CHK_RESULT,

    /* params[0]: TEE_PARAM_TYPE_VALUE_INPUT
     *   params[0].a: drm schemeType
     *   params[0].b: the session Id
     * params[1]: TEE_PARAM_TYPE_MEMREF_OUTPUT
     *   params[1].a: buffer to store all the Content Key  attributes and key/iv data
     *   params[1].b: buffer size
     * params[2]: TEE_PARAM_TYPE_VALUE_OUTPUT
     *   params[2].a: the count of the keys unit of this keyToken
     */
    TSP_KEYTOKEN_GETINFO,

    /* params[0]: TEE_PARAM_TYPE_VALUE_INPUT
     *   params[0].a: the shared hbo input dtcm buffer among multiple dmx filters
     */
    TSP_UPDATE_HBOIN_FILTERS,

    TSP_CMD_MAX,

    // TSP TA commands called by TSP kernel driver should be defined from here
    TSP_KERNEL_CMD_MIN = 0x10000,
    /*
     * Save the TSP HW context (DTCM/ITCM/Register) into a buffer in TZ.
     * This command is usually called by TSP kernel driver.
     * to save its HW context for the further recovery from system suspend/resume.
     */
    TSP_SAVE_HW_CONTEXT = TSP_KERNEL_CMD_MIN,
    /*
     * Restore the TSP HW context (DTCM/ITCM/Register) from a buffer in TZ.
     * This command is called by TSP kernel driver during system resume
     * to restore the TSP HW back to the state before last suspend.
     */
    TSP_RESTORE_HW_CONTEXT,
    /*
     * This command is called by both DMX driver and TSP kernel driver
     * to release or reset one FIGO
     */
    TSP_SET_FIGO_STATE,
    /*
     * This command is called by both DMX driver and TSP kernel driver
     * to get state of one FIGO: reset or release
     */
    TSP_GET_FIGO_STATE,
    /*
     * This command can be called by both DMX driver and TSP kernel driver
     * to set tsp or tsp ref clk
     */
    TSP_SET_CLK,
    TSP_KERNEL_CMD_MAX
};

enum {
    M2M_INVALID,
    M2M_CryptoInit,
    M2M_Setscheme,
    M2M_Setkey,
    M2M_Update,
    M2M_Update_Residue,
    M2M_FeedCmd,
    M2M_Postprocess,
    M2M_CryptoDeinit,
    M2M_SetPatternMode,
    M2M_SetResidueMode,
    M2M_SetIVPolicy,
    M2M_CMD_MAX
};

typedef enum {
    DTCM_MEM_TYPE_NONSECURE,
    DTCM_MEM_TYPE_SECURE,
    DTCM_MEM_TYPE_MAX
}DTCM_MEM_TYPE;

typedef enum {
    TSP_MEM_PARAM_PHY_ADDR,
    TSP_MEM_PARAM_SHM_ID,
    TSP_MEM_PARAM_MEMREF,
    TSP_MEM_PARAM_HEAP,
} TSP_MEM_PARAM;

typedef enum {
    TSP_MEMREF_FROM_CA,
    TSP_SHM_FROM_TA
} TSP_MEM_TYPE;

#endif /* __AMP_TSP_TEE_CMD_H__ */

