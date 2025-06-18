/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __VPP_BE_CEC_MSG_H__
#define __VPP_BE_CEC_MSG_H__

#include "com_type.h"
#include "vpp_api_types.h"

/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */
#define BE_CEC_BROADCAST_ADDR           0x0F
#define BE_CEC_GET_INIT_ADDR(header)    (((header) >> 4) & 0x0F)
#define BE_CEC_GET_FOLL_ADDR(header)    ((header) & 0x0F)
#define BE_CEC_SET_HEADER(init,foll)    ((foll & 0x0F) | ((init & 0x0F) << 4))

#define BE_CEC_MAX_MSGS                 256

/*-----------------------------------------------------------------------------
 * TypeDefs
 *-----------------------------------------------------------------------------
 */
// Message mode
typedef enum tagBE_CEC_MSG_MODE
{
    BE_CEC_MSG_MODE_UNDEFINED = 0x00,
    BE_CEC_MSG_MODE_UNICAST,
    BE_CEC_MSG_MODE_BROADCAST
}BE_CEC_MSG_MODE;

// Message process function type
typedef int (*BE_CEC_MSG_PROC_FN)(BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                  UINT8 *pFrame, UINT8 *pLen);

// Data store for each message
typedef struct tagBE_CEC_MSG_DATA
{
    BE_CEC_MSG_PROC_FN  procFn;
    BE_CEC_MSG_MODE     mode;
    UINT16              operandLen;
    UINT16              msgEnabled;
} BE_CEC_MSG_DATA, *PBE_CEC_MSG_DATA;

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */

/******************************************************************************
 * FUNCTION : Initializes message table
 * PARAMS   : featureSupportCfg - Configuration setting to indicate which
 *          : feature has to be supported
 *          : pCecMsgTable - Pointer to message table
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_InitMessageTable (UINT32 featureSupportCfg, PBE_CEC_MSG_DATA pCecMsgTable);

/******************************************************************************
 * FUNCTION : Returns CEC message mode based on follower's address
 * PARAMS   : follAddr - Logical address of the follower
 * RETURN   : Message mode (Unicast/Broadcast)
 *          : Mode undefined, in case of any error
 *****************************************************************************/
BE_CEC_MSG_MODE BE_CEC_GetMessageMode (int follAddr);

/******************************************************************************
 * FUNCTION : Formats given message to 16 byte(max) frame
 * PARAMS   : pCecMsgTable - Pointer to message table
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to return frame length (Output)
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_FormatMessageToFrame (PBE_CEC_MSG_DATA pCecMsgTable, PVPP_CEC_MSG pMsg,
                                 UINT8 *pFrame, UINT8 *pFrameLen);

/******************************************************************************
 * FUNCTION : Formats given frame of length 16 byte(max) to CEC message
 * PARAMS   : pCecMsgTable - Pointer to message table
 *          : pFrame    - Pointer to frame (Input)
 *          : pMsg      - Pointer to CEC message (Input)
 *          : frameLen  - Length of the frame in bytes
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_FormatFrameToMessage (PBE_CEC_MSG_DATA pCecMsgTable, UINT8 *pFrame,
                                 PVPP_CEC_MSG pMsg, UINT8 frameLen);

#endif // __VPP_BE_CEC_MSG_H__
