/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _VPP_BE_CEC_MSG_C_
#define _VPP_BE_CEC_MSG_C_

#include "com_type.h"
#include "OSAL_api.h"
#include "vpp_api_types.h"
#include "vpp_be_cec_msg.h"
#include "vpp_be_cec_msg_prv.h"
#include "galois_cfg.h"
#include "vpp_debug.h"

/******************************************************************************
 * FUNCTION : Initializes message table
 * PARAMS   : featureSupportCfg - Configuration setting to indicate which
 *          : feature has to be supported
 *          : pCecMsgTable - Pointer to message table
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_InitMessageTable (UINT32 featureSupportCfg, PBE_CEC_MSG_DATA pCecMsgTable)
{
    GaloisMemClear (pCecMsgTable, BE_CEC_MAX_MSGS * sizeof (BE_CEC_MSG_DATA));

    // General protocol messages
    CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_FEATURE_ABORT,
                    CEC_HandleMsgFeatureAbort, BE_CEC_MSG_MODE_UNICAST, 2);

    CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_ABORT,
                    NULL, BE_CEC_MSG_MODE_UNICAST, 0);

    // One touch play feature
    if (featureSupportCfg & VPP_CEC_FEATURE_ONE_TOUCH_PLAY)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_ACTIVE_SOURCE,
                        CEC_HandleOpPhyAddr, BE_CEC_MSG_MODE_BROADCAST, 2);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_IMAGE_VIEW_ON,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_TEXT_VIEW_ON,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
    }

    // System information
    if (featureSupportCfg & VPP_CEC_FEATURE_SYSTEM_INFO)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GET_MENU_LANG,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GIVE_PHY_ADDR,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REPORT_PHY_ADDR,
                        CEC_HandleMsgReportPhyAddr, BE_CEC_MSG_MODE_BROADCAST, 3);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_MENU_LANG,
                        CEC_HandleMsgSetMenuLang, BE_CEC_MSG_MODE_BROADCAST, 3);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_CEC_VERSION,
                        CEC_HandleMsgCecVersion, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GET_CEC_VERSION,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
    }

    // Device Menu Control
    if (featureSupportCfg & VPP_CEC_FEATURE_DEVICE_MENU_CONTROL)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_MENU_REQUEST,
                        CEC_HandleMsgMenuControl, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_MENU_STATUS,
                        CEC_HandleMsgMenuStatus, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_USER_CONTROL_PRESSED,
                        CEC_HandleMsgUserCtrlPressed, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_USER_CONTROL_RELEASED,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
    }

    // System standby
    if (featureSupportCfg & VPP_CEC_FEATURE_SYSTEM_STANDBY)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_STANDBY,
                        NULL, (BE_CEC_MSG_MODE_UNICAST | BE_CEC_MSG_MODE_BROADCAST), 0);
    }

    // One touch record
    if (featureSupportCfg & VPP_CEC_FEATURE_ONE_TOUCH_RECORD)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_RECORD_OFF,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        // Parameter length specifies minimum valid length
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_RECORD_ON,
                        CEC_HandleMsgRecordOn, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_RECORD_STATUS,
                        CEC_HandleMsgRecordStatus, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_RECORD_TV_SCREEN,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
    }

    // Deck control
    if (featureSupportCfg & VPP_CEC_FEATURE_DECK_CONTROL)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_DECK_STATUS,
                        CEC_HandleMsgDeckStatus, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GIVE_DECK_STATUS,
                        CEC_HandleMsgGiveDeckStatus, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_DECK_CONTROL,
                        CEC_HandleMsgDeckControl, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_PLAY,
                        CEC_HandleMsgPlay, BE_CEC_MSG_MODE_UNICAST, 1);
    }

    // OSD Status Display
    if (featureSupportCfg & VPP_CEC_FEATURE_OSD_STATUS_DISPLAY)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_OSD_STRING,
                        CEC_HandleMsgSetOSDString, BE_CEC_MSG_MODE_UNICAST, 2);
    }

    // OSD name transfer
    if (featureSupportCfg & VPP_CEC_FEATURE_DEVICE_OSD_NAME_TX)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GIVE_OSD_NAME,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_OSD_NAME,
                        CEC_HandleMsgSetOSDName, BE_CEC_MSG_MODE_UNICAST, 1);
    }

    // Remote control pass-through
    if (featureSupportCfg & VPP_CEC_FEATURE_REMOTE_CONTROL_PASS_THRU)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_USER_CONTROL_PRESSED,
                        CEC_HandleMsgUserCtrlPressed, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_USER_CONTROL_RELEASED,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
    }

    // Device power status
    if (featureSupportCfg & VPP_CEC_FEATURE_DEVICE_POWER_STATUS)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GIVE_DEVICE_POWER_STATUS,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REPORT_POWER_STATUS,
                        CEC_HandleMsgReportPowerSts, BE_CEC_MSG_MODE_UNICAST, 1);
    }

    // Vendor specific commands
    if (featureSupportCfg & VPP_CEC_FEATURE_VENDOR_SPEC_CMDS)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_DEVICE_VENDOR_ID,
                        CEC_HandleMsgDevVendorID, BE_CEC_MSG_MODE_BROADCAST, 3);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GIVE_DEVICE_VENDOR_ID,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        // Minimum length
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_VENDOR_COMMAND,
                        CEC_HandleMsgVendorCmd, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_VENDOR_COMMAND_WITH_ID,
                        CEC_HandleMsgVendorCmdWithID,
                        (BE_CEC_MSG_MODE_UNICAST | BE_CEC_MSG_MODE_BROADCAST), 3);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_VENDOR_REMOTE_BTN_DOWN,
                        CEC_HandleMsgVendorRCDown,
                        (BE_CEC_MSG_MODE_UNICAST | BE_CEC_MSG_MODE_BROADCAST), 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_VENDOR_REMOTE_BTN_UP,
                        NULL,
                        (BE_CEC_MSG_MODE_UNICAST | BE_CEC_MSG_MODE_BROADCAST), 0);
    }

    // Routing Control
    if (featureSupportCfg & VPP_CEC_FEATURE_ROUTING_CONTROL)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_ACTIVE_SOURCE,
                        CEC_HandleOpPhyAddr, BE_CEC_MSG_MODE_BROADCAST, 2);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REQUEST_ACTIVE_SOURCE,
                        NULL, BE_CEC_MSG_MODE_BROADCAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_STREAM_PATH,
                        CEC_HandleOpPhyAddr, BE_CEC_MSG_MODE_BROADCAST, 2);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_ROUTING_CHANGE,
                        CEC_HandleMsgRoutingChange, BE_CEC_MSG_MODE_BROADCAST, 4);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_ROUTING_INFO,
                        CEC_HandleOpPhyAddr, BE_CEC_MSG_MODE_BROADCAST, 2);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_INACTIVE_SOURCE,
                        CEC_HandleOpPhyAddr, BE_CEC_MSG_MODE_UNICAST, 2);
    }

    // Tuner Control
    if (featureSupportCfg & VPP_CEC_FEATURE_TUNER_CONTROL)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GIVE_TUNER_DEVICE_STATUS,
                        CEC_HandleMsgGiveTunerDeviceSts, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SELECT_ANALOGUE_SERVICE,
                        CEC_HandleMsgSelAnaService, BE_CEC_MSG_MODE_UNICAST, 4);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SELECT_DIGITAL_SERVICE,
                        CEC_HandleMsgSelDigService, BE_CEC_MSG_MODE_UNICAST, 7);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_TUNER_DEVICE_STATUS,
                        CEC_HandleMsgTunerDeviceSts, BE_CEC_MSG_MODE_UNICAST, 8);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_TUNER_STEP_DECREMENT,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_TUNER_STEP_INCREMENT,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
    }

    // System Audio Control
    if (featureSupportCfg & VPP_CEC_FEATURE_SYSTEM_AUDIO_CONTROL)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GIVE_AUDIO_STATUS,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_GIVE_SYS_AUDIO_MODE_STATUS,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REPORT_AUDIO_STATUS,
                        CEC_HandleMsgReportAudioSts, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_SYS_AUDIO_MODE,
                        CEC_HandleMsgSetSysAudioMode,
                        (BE_CEC_MSG_MODE_UNICAST | BE_CEC_MSG_MODE_BROADCAST), 1);

        // TO DO - Handle system audio mode request with no physical address
        // for termination of this feature
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SYS_AUDIO_MODE_REQUEST,
                        CEC_HandleMsgSysAudioModeRequest, BE_CEC_MSG_MODE_UNICAST, 0);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SYS_AUDIO_MODE_STATUS,
                        CEC_HandleMsgSysAudioModeSts, BE_CEC_MSG_MODE_UNICAST, 1);
// new addded for ver 1.4a
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REPORT_SHORT_AUDIO_DESC,
                        CEC_HandleMsgReportShortAudioDesc, BE_CEC_MSG_MODE_UNICAST, 3);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REQUEST_SHORT_AUDIO_DESC,
                        CEC_HandleMsgRequestShortAudioDesc, BE_CEC_MSG_MODE_UNICAST, 1);
    }

    // Audio Rate Control
    if (featureSupportCfg & VPP_CEC_FEATURE_AUDIO_RATE_CONTROL)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_AUDIO_RATE,
                        CEC_HandleMsgSetAudioRate, BE_CEC_MSG_MODE_UNICAST, 1);
    }

    // Timer Programming
    if (featureSupportCfg & VPP_CEC_FEATURE_TIMER_PROGRAMMING)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_CLR_ANALOGUE_TIMER,
                        CEC_HandleAnalogueTimerInfo, BE_CEC_MSG_MODE_UNICAST, 11);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_ANALOGUE_TIMER,
                        CEC_HandleAnalogueTimerInfo, BE_CEC_MSG_MODE_UNICAST, 11);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_CLR_DIGITAL_TIMER,
                        CEC_HandleDigitalTimerInfo, BE_CEC_MSG_MODE_UNICAST, 14);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_DIGITAL_TIMER,
                        CEC_HandleDigitalTimerInfo, BE_CEC_MSG_MODE_UNICAST, 14);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_CLR_EXTERNAL_TIMER,
                        CEC_HandleExternalTimerInfo, BE_CEC_MSG_MODE_UNICAST, 9);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_EXTERNAL_TIMER,
                        CEC_HandleExternalTimerInfo, BE_CEC_MSG_MODE_UNICAST, 9);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_SET_TIMER_PGM_TITLE,
                        CEC_HandleMsgSetTimerPgmTitle, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_TIMER_CLEARED_STATUS,
                        CEC_HandleMsgTimerClearedStatus, BE_CEC_MSG_MODE_UNICAST, 1);

        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_TIMER_STATUS,
                        CEC_HandleMsgTimerStatus, BE_CEC_MSG_MODE_UNICAST, 1);
    }
    if (featureSupportCfg & VPP_CEC_FEATURE_AUDIO_RETURN_CHANNEL)
    {
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_INITIATE_ARC,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REPORT_ARC_INITIATED,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REPORT_ARC_TERMINATED,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REQUEST_ARC_INITIATION,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_REQUEST_ARC_TERMINATION,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
        CEC_SetMessage (pCecMsgTable, VPP_CEC_MSG_OPCODE_TERMINATE_ARC,
                        NULL, BE_CEC_MSG_MODE_UNICAST, 0);
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Returns CEC message mode based on follower's address
 * PARAMS   : follAddr - Logical address of the follower
 * RETURN   : Message mode (Unicast/Broadcast)
 *          : Mode undefined, in case of any error
 *****************************************************************************/
BE_CEC_MSG_MODE BE_CEC_GetMessageMode (int follAddr)
{
    BE_CEC_MSG_MODE msgMode;

    msgMode = BE_CEC_MSG_MODE_UNDEFINED;
    if (follAddr == BE_CEC_BROADCAST_ADDR)
        msgMode = BE_CEC_MSG_MODE_BROADCAST;
    else if (follAddr < BE_CEC_BROADCAST_ADDR)
        msgMode = BE_CEC_MSG_MODE_UNICAST;
    return (msgMode);
}

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
                                 UINT8 *pFrame, UINT8 *pFrameLen)
{
    VPP_CEC_STATUS_TYPE retCode = VPP_CEC_STATUS_SUCCESS;

    // Special handling if message opcode is undefined (used for polling)
    if ( pMsg->opcode == VPP_CEC_MSG_OPCODE_UNDEFINED )
    {
        *pFrameLen = 0;
        pFrame[(*pFrameLen)++] = BE_CEC_SET_HEADER(pMsg->srcAddr, pMsg->destAddr);
        return (retCode);
    }
    pMsg->opcode &= 0xFF; // In other cases, only 8 bits are valid
    // Check if message is supported
    if ( pCecMsgTable[pMsg->opcode].msgEnabled )
    {
        // Check for mode
        if (!(pCecMsgTable[pMsg->opcode].mode & BE_CEC_GetMessageMode(pMsg->destAddr)))
            return (VPP_CEC_STATUS_ERR_INVALID_MODE);
        if ((pMsg->opcode == VPP_CEC_MSG_OPCODE_ACTIVE_SOURCE) &&
            (pMsg->operand.opPhyAddr == 0xFFFF))
            pMsg->srcAddr = 0xF;

        // Fill header and opcode
        *pFrameLen = 0;
        pFrame[(*pFrameLen)++] = BE_CEC_SET_HEADER(pMsg->srcAddr, pMsg->destAddr);
        pFrame[(*pFrameLen)++] = pMsg->opcode;

        if (pCecMsgTable[pMsg->opcode].procFn != NULL)
        {
            // Fill operands if any
            retCode = pCecMsgTable[pMsg->opcode].procFn (TRUE, pMsg, pFrame, pFrameLen);
        }
    }
    else
    {
        // Message not supported
        retCode = VPP_CEC_STATUS_ERR_MSG_UNSUPPORTED;
    }
    return retCode;
}

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
                                 PVPP_CEC_MSG pMsg, UINT8 frameLen)
{
    VPP_CEC_STATUS_TYPE retCode;

    if ((pFrame == NULL) || (pMsg == NULL) || (frameLen == 0))
        return (VPP_CEC_STATUS_INVALID_PARAM);

    retCode = VPP_CEC_STATUS_ERR_INVALID_FRAME_LEN;
    if (frameLen >= 1)
    {
        // Parse header
        pMsg->srcAddr  = BE_CEC_GET_INIT_ADDR(pFrame[0]);
        pMsg->destAddr = BE_CEC_GET_FOLL_ADDR(pFrame[0]);

        if (frameLen >= 2)
        {
            // Parse opcode
            pMsg->opcode = pFrame[1];
            if (pMsg->srcAddr == 0x0F)
            {
                if (!((pMsg->opcode == VPP_CEC_MSG_OPCODE_REQUEST_ACTIVE_SOURCE) |
                      (pMsg->opcode == VPP_CEC_MSG_OPCODE_SET_STREAM_PATH)       |
                      (pMsg->opcode == VPP_CEC_MSG_OPCODE_ROUTING_CHANGE)        |
                      (pMsg->opcode == VPP_CEC_MSG_OPCODE_ROUTING_INFO)          |
                      (pMsg->opcode == VPP_CEC_MSG_OPCODE_STANDBY)               |
                      (pMsg->opcode == VPP_CEC_MSG_OPCODE_GIVE_PHY_ADDR)         |
                      (pMsg->opcode == VPP_CEC_MSG_OPCODE_GET_MENU_LANG)         |
                      (pMsg->opcode == VPP_CEC_MSG_OPCODE_GIVE_DEVICE_VENDOR_ID)))
                {
                    return (VPP_CEC_STATUS_ERR_MSG_IGNORED);
                }
            }
            if (pCecMsgTable[pMsg->opcode].msgEnabled)
            {
                if (!(pCecMsgTable[pMsg->opcode].mode & BE_CEC_GetMessageMode(pMsg->destAddr)))
                {
                    return (VPP_CEC_STATUS_ERR_MSG_IGNORED);
                }
                // Check operand length
                if ((frameLen-CEC_MSG_OPERAND_POS) < pCecMsgTable[pMsg->opcode].operandLen)
                {
                    // Number of operands is less than the required number
                    retCode = VPP_CEC_STATUS_ERR_MSG_IGNORED;
                }
                else
                {
                    // Operand count is correct
                    if ((frameLen-CEC_MSG_OPERAND_POS) == 0)
                    {
                        /* SYS_AUDIO_MODE_REQUEST - fill phy addr 0xFFFF
                           when no operand present on msg */
                        if (VPP_CEC_MSG_OPCODE_SYS_AUDIO_MODE_REQUEST == pMsg->opcode)
                            retCode = pCecMsgTable[pMsg->opcode].procFn (FALSE, pMsg, pFrame, &frameLen);
                        else
                            // No operands
                            retCode = VPP_CEC_STATUS_SUCCESS;
                    }
                    else
                    {
                        // Parse operands
                        if (pCecMsgTable[pMsg->opcode].procFn != NULL)
                        {
                            retCode = pCecMsgTable[pMsg->opcode].procFn (FALSE, pMsg, pFrame, &frameLen);
                        }
                    }
                }
            }
            else
            {
                // Message not supported
                if (BE_CEC_GetMessageMode(pMsg->destAddr) == BE_CEC_MSG_MODE_BROADCAST)
                {
                    // Ignore message
                    retCode = VPP_CEC_STATUS_ERR_MSG_IGNORED;
                }
                else
                {
                    // Feature abort needs to be sent for unicast message
                    retCode = VPP_CEC_STATUS_ERR_MSG_UNSUPPORTED;
                }
            }
        }
        else
        {
            // No opcode - Polling message
            pMsg->opcode = VPP_CEC_MSG_OPCODE_UNDEFINED;
            retCode = VPP_CEC_STATUS_ERR_MSG_IGNORED;
        }
    }
    return retCode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////LOCAL STATIC FUNCTIONS//////////////////////
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 * FUNCTION : Sets message definition in the table
 * PARAMS   : pCecMsgTable - Pointer to message table
 *          : opcode       - Message opcode
 *          : procFn       - Pointer to process function
 *          : mode         - Message mode
 *          : operandLen   - Message operand length
 * RETURN   : TRUE, if copied; FALSE, otherwise
 *****************************************************************************/
static void CEC_SetMessage (PBE_CEC_MSG_DATA pCecMsgTable, UINT8 opcode,
                            BE_CEC_MSG_PROC_FN procFn, BE_CEC_MSG_MODE mode,
                            UINT8 operandLen)
{
    pCecMsgTable[opcode].procFn = procFn;
    pCecMsgTable[opcode].mode = mode;
    pCecMsgTable[opcode].operandLen = operandLen;
    pCecMsgTable[opcode].msgEnabled = TRUE;
}

/******************************************************************************
 * FUNCTION : Copies from source to destination in the same/reverse order
 * PARAMS   : pDest        - Pointer to destination
 *          : pSrc         - Pointer to source
 *          : pDest        - Length to be copied
 * RETURN   : TRUE, if copied; FALSE, otherwise
 *****************************************************************************/
static BOOL CEC_SwapBytes (UINT8 *pDest, UINT8 *pSrc, int len)
{
#ifdef CONFIG_AMP_TEE_ENABLE
    int diff = (pDest - pSrc);
    if(diff<0)
        diff = -diff;
    if(diff <= len)
#else
    if ((abs)(pDest - pSrc) <= len)
#endif
        return (FALSE);

#if (defined(__LITTLE_ENDIAN))
    pDest += (len-1);
    while (len--)
        *pDest-- = *pSrc++;
#else
    while (len--)
        *pDest++ = *pSrc++;
#endif
    return (TRUE);
}

/******************************************************************************
 * FUNCTION : Convert decimal number to BCD
 * PARAMS   : *Num - Pointer to decimal number (Input)
 *          : *BCD - Pointer to BCD equivalent (Output)
 * RETURN   : TRUE, if converted; FALSE, otherwise
 *****************************************************************************/
static BOOL CEC_NumToBCD(UINT8 *Num, UINT8 *BCD)
{
    if (*Num >= 100)
        return (FALSE);

    *BCD = *Num / 10;
    *BCD <<= 4;
    *BCD |= *Num % 10;
    return (TRUE);
}

/******************************************************************************
 * FUNCTION : Convert BCD to decimal number
 * PARAMS   : *BCD - Pointer to BCD number (Input)
 *          : *Num - Pointer to decimal equivalent (Output)
 * RETURN   : TRUE, if converted; FALSE, otherwise
 *****************************************************************************/
static BOOL CEC_BCDToNum(UINT8 *BCD, UINT8 *Num)
{
    *Num = *BCD & 0x0F;
    *Num += ((*BCD >> 4) & 0x0F) * 10;
    return (TRUE);
}

/*-----------------------------------------------------------------------------
 * GENERAL PROTOCOL MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "feature abort" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgFeatureAbort (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                      UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opFeatureAbort.featureOpcode;
        pFrame[(*pLen)++] = pMsg->operand.opFeatureAbort.abortReason;
    }
    else
    {
        pMsg->operand.opFeatureAbort.featureOpcode = pFrame[pos++];
        pMsg->operand.opFeatureAbort.abortReason = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * SYSTEM INFORMATION MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "report physical address" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgReportPhyAddr (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                       UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opRepPhyAddr.phyAddr, 2);
        *pLen += 2;
        pFrame[*pLen] = pMsg->operand.opRepPhyAddr.deviceType;
        *pLen += 1;
    }
    else
    {
        CEC_SwapBytes ((UINT8*)&pMsg->operand.opRepPhyAddr.phyAddr, &pFrame[pos], 2);
        pos += 2;
        pMsg->operand.opRepPhyAddr.deviceType = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "set menu language" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSetMenuLang (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                     UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;
    int i;
    // Check if these 3 bytes are considered as individual bytes or
    // together - in the latter case, endian conversion has to be
    // done
    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opLanguage[0];
        pFrame[(*pLen)++] = pMsg->operand.opLanguage[1];
        pFrame[(*pLen)++] = pMsg->operand.opLanguage[2];
    }
    else
    {
        pMsg->operand.opLanguage[0] = pFrame[pos++];
        pMsg->operand.opLanguage[1] = pFrame[pos++];
        pMsg->operand.opLanguage[2] = pFrame[pos];
    for(i=0;i<3;i++) // case insensitive langague code as per 1.4a
        {
                if (pMsg->operand.opLanguage[i] >= 65 && pMsg->operand.opLanguage[i] <= 90) /* A-Z */
                {
                        pMsg->operand.opLanguage[i]=pMsg->operand.opLanguage[i]+32;
                }
        }

    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "cec version" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgCecVersion (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                    UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opVersion;
    }
    else
    {
        pMsg->operand.opVersion = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * DEVICE MENU CONTROL MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "menu control" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgMenuControl (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                     UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opMenuRequest;
    }
    else
    {
        pMsg->operand.opMenuRequest = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}


/******************************************************************************
 * FUNCTION : Handles "menu status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgMenuStatus (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                    UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opMenuState;
    }
    else
    {
        pMsg->operand.opMenuState = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "user control pressed" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgUserCtrlPressed (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                         UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opUICommand.userControlCode;
        if (pMsg->operand.opUICommand.fnOperandPresent)
        {
            switch (pMsg->operand.opUICommand.userControlCode)
            {
                case VPP_CEC_UC_PLAY_FN:
                {
                    pFrame[(*pLen)++] = pMsg->operand.opUICommand.fnOperand.playMode;
                }
                break;

                case VPP_CEC_UC_TUNE_FN:
                {
                    pFrame[*pLen] = pMsg->operand.opUICommand.fnOperand.chnIden.chnNumFormat << 2;
                    if (pMsg->operand.opUICommand.fnOperand.chnIden.chnNumFormat == 2) // 2 part channel number
                    {
                        /// 2 most-significant bits of major channel number
                        pFrame[(*pLen)++] |= (pMsg->operand.opUICommand.fnOperand.chnIden.majChnNum >> 8);
                        pFrame[(*pLen)++] = (pMsg->operand.opUICommand.fnOperand.chnIden.majChnNum & 0xFF);
                        pFrame[(*pLen)++] = (pMsg->operand.opUICommand.fnOperand.chnIden.minChnNum >> 8) & 0xFF;
                        pFrame[(*pLen)++] = pMsg->operand.opUICommand.fnOperand.chnIden.minChnNum & 0xFF;
                    }
                    else
                    {
                        (*pLen)++;
                    }
                }
                break;

                case VPP_CEC_UC_SEL_DISK_FN:
                {
                    pFrame[(*pLen)++] = pMsg->operand.opUICommand.fnOperand.uiFnMedia;
                }
                break;

                case VPP_CEC_UC_SEL_AV_INPUT_FN:
                {
                    pFrame[(*pLen)++] = pMsg->operand.opUICommand.fnOperand.uiFnSelAVIn;
                }
                break;

                case VPP_CEC_UC_SEL_AUDIO_INPUT_FN:
                {
                    pFrame[(*pLen)++] = pMsg->operand.opUICommand.fnOperand.uiFnSelAudIn;
                }
                break;

                case VPP_CEC_UC_SEL_BRD_TYPE_FN:
                {
                    pFrame[(*pLen)++] = pMsg->operand.opUICommand.fnOperand.uiFnSelBrdCstIn;
                }
                break;

                case VPP_CEC_UC_SEL_SOUND_PPT_FN:
                {
                    pFrame[(*pLen)++] = pMsg->operand.opUICommand.fnOperand.uiFnSelSoundCtrl;
                }
                break;

                default:
                    break;
            }
        }
    }
    else
    {
        pMsg->operand.opUICommand.userControlCode = pFrame[pos++];
        pMsg->operand.opUICommand.fnOperandPresent = FALSE;
        if (*pLen > 3) // Greater than the base user control code
        {
            pMsg->operand.opUICommand.fnOperandPresent = TRUE;
            switch (pMsg->operand.opUICommand.userControlCode)
            {
                case VPP_CEC_UC_PLAY_FN:
                {
                    pMsg->operand.opUICommand.fnOperand.playMode = pFrame[pos];
                }
                break;

                case VPP_CEC_UC_TUNE_FN:
                {
                    pMsg->operand.opUICommand.fnOperand.chnIden.chnNumFormat = (pFrame[pos] >> 2) & 0x3F;
                    if (pMsg->operand.opUICommand.fnOperand.chnIden.chnNumFormat == 2) // 2 part channel number
                    {
                        pMsg->operand.opUICommand.fnOperand.chnIden.majChnNum = pFrame[pos+1] | ((pFrame[pos] & 0x3) << 8);
                        pos += 2;
                        pMsg->operand.opUICommand.fnOperand.chnIden.minChnNum = pFrame[pos+1]  | pFrame[pos] << 8;
                        pos += 2;
                    }
                }
                break;

                case VPP_CEC_UC_SEL_DISK_FN:
                {
                    pMsg->operand.opUICommand.fnOperand.uiFnMedia = pFrame[pos];
                }
                break;

                case VPP_CEC_UC_SEL_AV_INPUT_FN:
                {
                    pMsg->operand.opUICommand.fnOperand.uiFnSelAVIn = pFrame[pos];
                }
                break;

                case VPP_CEC_UC_SEL_AUDIO_INPUT_FN:
                {
                    pMsg->operand.opUICommand.fnOperand.uiFnSelAudIn = pFrame[pos];
                }
                break;

                case VPP_CEC_UC_SEL_BRD_TYPE_FN:
                {
                    pMsg->operand.opUICommand.fnOperand.uiFnSelBrdCstIn = pFrame[pos];
                }
                break;

                case VPP_CEC_UC_SEL_SOUND_PPT_FN:
                {
                    pMsg->operand.opUICommand.fnOperand.uiFnSelSoundCtrl = pFrame[pos];
                }
                break;

                default:
                    pMsg->operand.opUICommand.fnOperandPresent = FALSE;
                    break;
            }
        }
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * ROUTING CONTROL MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Routing change" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgRoutingChange (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                       UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opRoutingChange.origPhyAddr, 2);
        *pLen += 2;

        CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opRoutingChange.newPhyAddr, 2);
        *pLen += 2;
    }
    else
    {
        CEC_SwapBytes ((UINT8*)&pMsg->operand.opRoutingChange.origPhyAddr, &pFrame[pos], 2);
        pos += 2;

        CEC_SwapBytes ((UINT8*)&pMsg->operand.opRoutingChange.newPhyAddr, &pFrame[pos], 2);
        pos += 2;
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * ONE TOUCH RECORD MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "record on" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgRecordOn (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                  UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opRecSrc.recSrc;
        if (pMsg->operand.opRecSrc.recSrc == VPP_CEC_REC_SRC_DIG_SRV)
        {
            CEC_HandleOpDigSrvId(MsgToFrm, &pMsg->operand.opRecSrc.recSrcIden.digSrvIden, pFrame, pLen, pos);
        }
        else if (pMsg->operand.opRecSrc.recSrc == VPP_CEC_REC_SRC_ANA_SRV)
        {
            CEC_HandleOpAnaSrvId(MsgToFrm, &pMsg->operand.opRecSrc.recSrcIden.anaSrvIden, pFrame, pLen, pos);
        }
        else if (pMsg->operand.opRecSrc.recSrc == VPP_CEC_REC_SRC_EXT_PLUG)
        {
            pFrame[(*pLen)++] = pMsg->operand.opRecSrc.recSrcIden.extPlugNum;
        }
        else if (pMsg->operand.opRecSrc.recSrc == VPP_CEC_REC_SRC_EXT_PHY_ADDR)
        {
            CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opRecSrc.recSrcIden.extPhyAddr, 2);
            *pLen += 2;
        }
    }
    else
    {
        pMsg->operand.opRecSrc.recSrc = pFrame[pos++];
        if (pMsg->operand.opRecSrc.recSrc == VPP_CEC_REC_SRC_DIG_SRV)
        {
            CEC_HandleOpDigSrvId(MsgToFrm, &pMsg->operand.opRecSrc.recSrcIden.digSrvIden, pFrame, pLen, pos);
        }
        else if (pMsg->operand.opRecSrc.recSrc == VPP_CEC_REC_SRC_ANA_SRV)
        {
            CEC_HandleOpAnaSrvId(MsgToFrm, &pMsg->operand.opRecSrc.recSrcIden.anaSrvIden, pFrame, pLen, pos);
        }
        else if (pMsg->operand.opRecSrc.recSrc == VPP_CEC_REC_SRC_EXT_PLUG)
            {
            pMsg->operand.opRecSrc.recSrcIden.extPlugNum = pFrame[pos];
            }
        else if (pMsg->operand.opRecSrc.recSrc == VPP_CEC_REC_SRC_EXT_PHY_ADDR)
        {
            CEC_SwapBytes ((UINT8*)&pMsg->operand.opRecSrc.recSrcIden.extPhyAddr, &pFrame[pos], 2);
        }
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "record status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgRecordStatus (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                      UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opRecStsInfo;
    }
    else
    {
        pMsg->operand.opRecStsInfo = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}


/*-----------------------------------------------------------------------------
 * DECK CONTROL MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Deck Control" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgDeckControl (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                     UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opDeckControlMode;
    }
    else
    {
        pMsg->operand.opDeckControlMode = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "Deck Status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgDeckStatus (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                    UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opDeckInfo;
    }
    else
    {
        pMsg->operand.opDeckInfo = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "Give Deck Status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgGiveDeckStatus (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opStatusRequest;
    }
    else
    {
        pMsg->operand.opStatusRequest = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "Play" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgPlay (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                              UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opPlayMode;
    }
    else
    {
        pMsg->operand.opPlayMode = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * OSD STATUS DISPLAY MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Play" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSetOSDString (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                      UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        if (pMsg->operand.opOsdString.osdStringLen > 13) {
            return (VPP_CEC_STATUS_INVALID_PARAM);
        }
        pFrame[(*pLen)++] = pMsg->operand.opOsdString.dispCtrl;
        GaloisMemcpy (&pFrame[*pLen],&pMsg->operand.opOsdString.osdString,
                      pMsg->operand.opOsdString.osdStringLen);
        *pLen += pMsg->operand.opOsdString.osdStringLen;
    }
    else
    {
        pMsg->operand.opOsdString.dispCtrl = pFrame[pos++];
        pMsg->operand.opOsdString.osdStringLen = (*pLen) - pos; // fix
    GaloisMemClear(pMsg->operand.opOsdString.osdString, 13); // clear garbage
        GaloisMemcpy (pMsg->operand.opOsdString.osdString,&pFrame[pos],
                      pMsg->operand.opOsdString.osdStringLen);
    }
    return (VPP_CEC_STATUS_SUCCESS);
}


/*-----------------------------------------------------------------------------
 * OSD NAME TX MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Set OSD name" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSetOSDName (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                    UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        if (pMsg->operand.opDevOsdName.osdNameLen > 14) {
            return (VPP_CEC_STATUS_INVALID_PARAM);
        }
        GaloisMemcpy (&pFrame[*pLen],pMsg->operand.opDevOsdName.osdName,
                      pMsg->operand.opDevOsdName.osdNameLen);
        *pLen += pMsg->operand.opDevOsdName.osdNameLen;
    }
    else
    {
        pMsg->operand.opDevOsdName.osdNameLen = (*pLen) - pos;
        GaloisMemcpy (pMsg->operand.opDevOsdName.osdName,&pFrame[pos],
                      pMsg->operand.opDevOsdName.osdNameLen);
    }
    return (VPP_CEC_STATUS_SUCCESS);
}


/*-----------------------------------------------------------------------------
 * DEVICE POWER STATUS MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Set OSD name" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgReportPowerSts (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opPwrSts;
    }
    else
    {
        pMsg->operand.opPwrSts = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}


/*-----------------------------------------------------------------------------
 * VENDOR SPECIFIC COMMANDS
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Device Vendor ID" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgDevVendorID (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                     UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opVendorID, 3);
        *pLen += 3;
    }
    else
    {
        CEC_SwapBytes ((UINT8*)&pMsg->operand.opVendorID, &pFrame[pos], 3);
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "Vendor command" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgVendorCmd (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                   UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        if (pMsg->operand.opVendorSpecData.dataLength > 14) {
            return (VPP_CEC_STATUS_INVALID_PARAM);
        }

        GaloisMemcpy (&pFrame[*pLen], (UINT8*)&pMsg->operand.opVendorSpecData.vendorData,
                       pMsg->operand.opVendorSpecData.dataLength);
        *pLen += pMsg->operand.opVendorSpecData.dataLength;
    }
    else
    {
        pMsg->operand.opVendorSpecData.dataLength = (*pLen) - pos;
        GaloisMemcpy ((UINT8*)&pMsg->operand.opVendorSpecData.vendorData, &pFrame[pos],
                       pMsg->operand.opVendorSpecData.dataLength);
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "Vendor command with ID" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgVendorCmdWithID (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                         UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        if (pMsg->operand.opVendorSpecData.dataLength > 11) {
            return (VPP_CEC_STATUS_INVALID_PARAM);
        }

        CEC_SwapBytes (&pFrame[*pLen],(UINT8*)&pMsg->operand.opVendorSpecData.vendorID, 3);
        *pLen += 3;

        GaloisMemcpy (&pFrame[*pLen], (UINT8*)&pMsg->operand.opVendorSpecData.vendorData,
                       pMsg->operand.opVendorSpecData.dataLength);
        *pLen += pMsg->operand.opVendorSpecData.dataLength;
    }
    else
    {
        CEC_SwapBytes ((UINT8*)&pMsg->operand.opVendorSpecData.vendorID, &pFrame[pos],3);
        pos += 3;

        pMsg->operand.opVendorSpecData.dataLength = (*pLen) - pos;
        GaloisMemcpy ((UINT8*)&pMsg->operand.opVendorSpecData.vendorData, &pFrame[pos],
                       pMsg->operand.opVendorSpecData.dataLength);
    }
    return (VPP_CEC_STATUS_SUCCESS);
}


/******************************************************************************
 * FUNCTION : Handles "Vendor remote button down" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgVendorRCDown (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                      UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opVendorRCCode;
    }
    else
    {
        pMsg->operand.opVendorRCCode = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * TUNER CONTROL MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Give tuner device status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgGiveTunerDeviceSts (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                            UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opStatusRequest;
    }
    else
    {
        pMsg->operand.opStatusRequest = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "Select analog service" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSelAnaService (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                       UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    CEC_HandleOpAnaSrvId(MsgToFrm, &pMsg->operand.opAnalogueSrvIden, pFrame, pLen, pos);
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "Select digital service" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSelDigService(BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                       UINT8 *pFrame, UINT8* pLen)
    {
    int pos = CEC_MSG_OPERAND_POS;

    CEC_HandleOpDigSrvId(MsgToFrm, &pMsg->operand.opDigSrvIden, pFrame, pLen, pos);
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "Tuner device status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgTunerDeviceSts (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)] = pMsg->operand.opTunerDeviceInfo.recordingFlag << 7;
        pFrame[(*pLen)++] |= pMsg->operand.opTunerDeviceInfo.tunerDispInfo & 0x7F;
    }
    else
    {
        pMsg->operand.opTunerDeviceInfo.recordingFlag = pFrame[pos] >> 7;
        pMsg->operand.opTunerDeviceInfo.tunerDispInfo = pFrame[pos++] & 0x7F;
    }

    if (pMsg->operand.opTunerDeviceInfo.tunerDispInfo == VPP_CEC_TUNER_DISPLAY_INFO_DIG_TUNER_DISPLAYED)
        CEC_HandleOpDigSrvId(MsgToFrm, &pMsg->operand.opTunerDeviceInfo.srvIden.digSrvIden, pFrame, pLen, pos);
    else if (pMsg->operand.opTunerDeviceInfo.tunerDispInfo == VPP_CEC_TUNER_DISPLAY_INFO_ANA_TUNER_DISPLAYED)
        CEC_HandleOpAnaSrvId(MsgToFrm, &pMsg->operand.opTunerDeviceInfo.srvIden.analogueSrvIden, pFrame, pLen, pos);
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * SYSTEM AUDIO CONTROL
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "report audio status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgReportAudioSts (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)]  = (pMsg->operand.opAudioStatus.audioMuteStatus & 0x01)<<7;
        pFrame[(*pLen)++] |= (pMsg->operand.opAudioStatus.audioVolStatus & 0x7F) ;
    }
    else
    {
        pMsg->operand.opAudioStatus.audioMuteStatus = (pFrame[pos]>>7)&0x01;
        pMsg->operand.opAudioStatus.audioVolStatus = (pFrame[pos] ) & 0x7F;
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "set system audio mode" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSetSysAudioMode (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                         UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opSysAudioStatus & 0x01;
    }
    else
    {
        pMsg->operand.opSysAudioStatus = pFrame[pos] & 0x01;
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "system audio mode request" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSysAudioModeRequest (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                             UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;
    if (MsgToFrm)
    {
    // change for system audio mode off
    if(pMsg->operand.opSysAudioModeRequest.audioMode){
        CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opSysAudioModeRequest.phyAddr, 2);
        *pLen += 2;
    }
    }
    else
    {
        if (*pLen == 2) {
            /* pLen = 2, when no operand, pass invalid phy addr to terminate ARC */
            pMsg->operand.opPhyAddr = VPP_CEC_INVALID_PHY_ADDR;
        } else if (*pLen == 4) {
            /* pLen = 4, when receiving valid phy addr as operand */
            CEC_SwapBytes ((UINT8*)&pMsg->operand.opPhyAddr, &pFrame[pos], 2);
        } else {
            /* ignore message with invalid length */
            return VPP_CEC_STATUS_ERR_MSG_IGNORED;
        }
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "system audio mode status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSysAudioModeSts (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                         UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opSysAudioStatus & 0x01;
    }
    else
    {
        pMsg->operand.opSysAudioStatus = pFrame[pos] & 0x01;
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "report short audio description" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgReportShortAudioDesc (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen)
{
    int i;
    int pos = CEC_MSG_OPERAND_POS;
    if (MsgToFrm)
    {
    if( pMsg->operand.opAudioShortDesc.num>4)
         return (VPP_CEC_STATUS_INVALID_PARAM);
    for(i=0;i<pMsg->operand.opAudioShortDesc.num;i++)
    {
            pFrame[(*pLen)++] = pMsg->operand.opAudioShortDesc.audioShortDesc[i][0];
            pFrame[(*pLen)++] = pMsg->operand.opAudioShortDesc.audioShortDesc[i][1];
            pFrame[(*pLen)++] = pMsg->operand.opAudioShortDesc.audioShortDesc[i][2];
    }
    }
    else
    {
    pMsg->operand.opAudioShortDesc.num=((*pLen) - pos)/3;
    if( pMsg->operand.opAudioShortDesc.num>4)
                 return (VPP_CEC_STATUS_INVALID_PARAM);
    for(i=0;i<pMsg->operand.opAudioShortDesc.num;i++)
        {
            pMsg->operand.opAudioShortDesc.audioShortDesc[i][0] = pFrame[pos++];
            pMsg->operand.opAudioShortDesc.audioShortDesc[i][1] = pFrame[pos++];
            pMsg->operand.opAudioShortDesc.audioShortDesc[i][2] = pFrame[pos++];
    }
    }
    return (VPP_CEC_STATUS_SUCCESS);

}


/******************************************************************************
 * FUNCTION : Handles "request short audio description" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgRequestShortAudioDesc (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen)
{
    int i;
    int pos = CEC_MSG_OPERAND_POS;
    if (MsgToFrm)
    {
        if(pMsg->operand.opAudioFormatWithID.num>4)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        for(i=0;i<pMsg->operand.opAudioFormatWithID.num;i++)
        {
            pFrame[(*pLen)]  = (pMsg->operand.opAudioFormatWithID.audioFmt[i].audioFmtID & 0x03)<<6;
            pFrame[(*pLen)++] |= (pMsg->operand.opAudioFormatWithID.audioFmt[i].audioFmtCode & 0x3F) ;
        }
    }
    else
    {
        pMsg->operand.opAudioFormatWithID.num=(*pLen) - pos;
        if(pMsg->operand.opAudioFormatWithID.num>4)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        for(i=0;i<pMsg->operand.opAudioFormatWithID.num;i++)
        {
            pMsg->operand.opAudioFormatWithID.audioFmt[i].audioFmtID = (pFrame[pos] >>6)&0x03;
            pMsg->operand.opAudioFormatWithID.audioFmt[i].audioFmtCode = (pFrame[pos] ) & 0x3F;
            pos++;
        }
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * AUDIO RATE CONTROL
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Audio rate control" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSetAudioRate (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                      UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opAudioRateControl;
    }
    else
    {
        pMsg->operand.opAudioRateControl = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * TIMER PROGRAMMING
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "analogue timer info" operand for set and clear
 *          : analogue timer info messages
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleAnalogueTimerInfo (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opAnalogueTimerInfo.dayOfMonth;
        pFrame[(*pLen)++] = pMsg->operand.opAnalogueTimerInfo.monthOfYear;

        if (CEC_NumToBCD(&pMsg->operand.opAnalogueTimerInfo.startTimeHour, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opAnalogueTimerInfo.startTimeMin, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opAnalogueTimerInfo.durationHour, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opAnalogueTimerInfo.durationMin, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        pFrame[(*pLen)++] = pMsg->operand.opAnalogueTimerInfo.recSequence;
        pFrame[(*pLen)++] = pMsg->operand.opAnalogueTimerInfo.broadcastType;

        CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opAnalogueTimerInfo.frequency, 2);
        *pLen += 2;

        pFrame[(*pLen)++] = pMsg->operand.opAnalogueTimerInfo.broadcastSystem;
    }
    else
    {
        pMsg->operand.opAnalogueTimerInfo.dayOfMonth   = pFrame[pos++];
        pMsg->operand.opAnalogueTimerInfo.monthOfYear  = pFrame[pos++];

        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opAnalogueTimerInfo.startTimeHour);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opAnalogueTimerInfo.startTimeMin);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opAnalogueTimerInfo.durationHour);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opAnalogueTimerInfo.durationMin);

        pMsg->operand.opAnalogueTimerInfo.recSequence   = pFrame[pos++];
        pMsg->operand.opAnalogueTimerInfo.broadcastType = pFrame[pos++];

        CEC_SwapBytes ((UINT8*)&pMsg->operand.opAnalogueTimerInfo.frequency, &pFrame[pos], 2);
        pos += 2;

        pMsg->operand.opAnalogueTimerInfo.broadcastSystem = pFrame[pos++];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "digital timer info" operand for set and clear
 *          : digital timer info messages
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleDigitalTimerInfo (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                       UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opDigitalTimerInfo.dayOfMonth;
        pFrame[(*pLen)++] = pMsg->operand.opDigitalTimerInfo.monthOfYear;

        if (CEC_NumToBCD(&pMsg->operand.opDigitalTimerInfo.startTimeHour, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opDigitalTimerInfo.startTimeMin, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opDigitalTimerInfo.durationHour, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opDigitalTimerInfo.durationMin, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        pFrame[(*pLen)++] = pMsg->operand.opDigitalTimerInfo.recSequence;
    }
    else
    {
        pMsg->operand.opDigitalTimerInfo.dayOfMonth  = pFrame[pos++];
        pMsg->operand.opDigitalTimerInfo.monthOfYear = pFrame[pos++];

        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opDigitalTimerInfo.startTimeHour);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opDigitalTimerInfo.startTimeMin);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opDigitalTimerInfo.durationHour);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opDigitalTimerInfo.durationMin);

        pMsg->operand.opDigitalTimerInfo.recSequence   = pFrame[pos++];
    }
    CEC_HandleOpDigSrvId(MsgToFrm, &pMsg->operand.opDigitalTimerInfo.digSrvID, pFrame, pLen, pos);
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "external timer info" operand for set and clear
 *          : external timer info messages
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleExternalTimerInfo (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opExtTimerInfo.dayOfMonth;
        pFrame[(*pLen)++] = pMsg->operand.opExtTimerInfo.monthOfYear;

        if (CEC_NumToBCD(&pMsg->operand.opExtTimerInfo.startTimeHour, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opExtTimerInfo.startTimeMin, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opExtTimerInfo.durationHour, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        if (CEC_NumToBCD(&pMsg->operand.opExtTimerInfo.durationMin, &pFrame[*pLen]) != TRUE)
            return (VPP_CEC_STATUS_INVALID_PARAM);
        (*pLen)++;

        pFrame[(*pLen)++] = pMsg->operand.opExtTimerInfo.recSequence;
        pFrame[(*pLen)++] = pMsg->operand.opExtTimerInfo.extSrcSpec;

        if (pMsg->operand.opExtTimerInfo.extSrcSpec == VPP_CEC_EXT_SRC_PLUG)
        {
            pFrame[(*pLen)++] = pMsg->operand.opExtTimerInfo.extPlugNum;
        }
        else // VPP_CEC_EXT_SRC_PHY_ADDR
        {
            CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opExtTimerInfo.extPhyAddr, 2);
            *pLen += 2;
        }
    }
    else
    {
        pMsg->operand.opExtTimerInfo.dayOfMonth  = pFrame[pos++];
        pMsg->operand.opExtTimerInfo.monthOfYear = pFrame[pos++];

        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opExtTimerInfo.startTimeHour);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opExtTimerInfo.startTimeMin);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opExtTimerInfo.durationHour);
        CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opExtTimerInfo.durationMin);

        pMsg->operand.opExtTimerInfo.recSequence   = pFrame[pos++];
        pMsg->operand.opExtTimerInfo.extSrcSpec    = pFrame[pos++];

        if (pMsg->operand.opExtTimerInfo.extSrcSpec == VPP_CEC_EXT_SRC_PLUG)
        {
            pMsg->operand.opExtTimerInfo.extPlugNum    = pFrame[pos++];
        }
        else // VPP_CEC_EXT_SRC_PHY_ADDR
        {
            CEC_SwapBytes ((UINT8*)&pMsg->operand.opExtTimerInfo.extPhyAddr, &pFrame[pos], 2);
            pos += 2;
        }
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "set timer program title" message operand
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSetTimerPgmTitle (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                          UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        if ((pMsg->operand.opPgmTitleString.stringLen < 1) ||
            (pMsg->operand.opPgmTitleString.stringLen > 14))
        {
            return (VPP_CEC_STATUS_INVALID_PARAM);
        }
        GaloisMemcpy (&pFrame[*pLen],&pMsg->operand.opPgmTitleString.stringName,
                      pMsg->operand.opPgmTitleString.stringLen);
        *pLen += pMsg->operand.opPgmTitleString.stringLen;
    }
    else
    {
        pMsg->operand.opPgmTitleString.stringLen = (*pLen) - pos;
        GaloisMemcpy (pMsg->operand.opPgmTitleString.stringName,&pFrame[pos],
                      pMsg->operand.opPgmTitleString.stringLen);
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "timer cleared status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgTimerClearedStatus (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                            UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pMsg->operand.opTimerClearedStsData;
    }
    else
    {
        pMsg->operand.opTimerClearedStsData = pFrame[pos];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles "timer status" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgTimerStatus (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                     UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        pFrame[*pLen] = (pMsg->operand.opTimerStatusData.timerOverLapWarning & 0x01) << 7;
        pFrame[*pLen] |= ((pMsg->operand.opTimerStatusData.mediaInfo & 0x03) << 5);
        pFrame[*pLen] |= ((pMsg->operand.opTimerStatusData.pgmedIndicator & 0x01) << 4);
        if (pMsg->operand.opTimerStatusData.pgmedIndicator == VPP_CEC_TIMER_PGM_IND_PROGRAMMED)
        {
            pFrame[*pLen] |= (pMsg->operand.opTimerStatusData.pgmedInfo & 0x0F);
            *pLen += 1;
        }
        else // VPP_CEC_TIMER_PGM_IND_NOT_PROGRAMMED
        {
            pFrame[*pLen] |= (pMsg->operand.opTimerStatusData.notPgmedErrInfo & 0x0F);
            *pLen += 1;
        }
        if (pMsg->operand.opTimerStatusData.durationAvailable != 0x00)
        {
            if (CEC_NumToBCD(&pMsg->operand.opTimerStatusData.availableDurHour, &pFrame[*pLen]) != TRUE)
                return (VPP_CEC_STATUS_INVALID_PARAM);
            (*pLen)++;

            if (CEC_NumToBCD(&pMsg->operand.opTimerStatusData.availableDurMin, &pFrame[*pLen]) != TRUE)
                return (VPP_CEC_STATUS_INVALID_PARAM);
            (*pLen)++;
        }
    }
    else
    {
        pMsg->operand.opTimerStatusData.timerOverLapWarning = (pFrame[pos] >> 0x07) & 0x01;
        pMsg->operand.opTimerStatusData.mediaInfo           = (pFrame[pos] >> 0x05) & 0x03;
        pMsg->operand.opTimerStatusData.pgmedIndicator      = (pFrame[pos] >> 0x04) & 0x01;
        if (pMsg->operand.opTimerStatusData.pgmedIndicator == VPP_CEC_TIMER_PGM_IND_PROGRAMMED)
        {
            pMsg->operand.opTimerStatusData.pgmedInfo = (pFrame[pos] & 0x0F);
        }
        else // VPP_CEC_TIMER_PGM_IND_NOT_PROGRAMMED
        {
            pMsg->operand.opTimerStatusData.notPgmedErrInfo = (pFrame[pos] & 0x0F);
        }
        pos++;
        if ((*pLen - CEC_MSG_OPERAND_POS) == 0x3) // 3 byte length
        {
            CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opTimerStatusData.availableDurHour);
            CEC_BCDToNum (&pFrame[pos++], &pMsg->operand.opTimerStatusData.availableDurMin);
        }
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * OPERANDS COMMON TO MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles messages with Physical address as operand
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleOpPhyAddr (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                UINT8 *pFrame, UINT8* pLen)
{
    int pos = CEC_MSG_OPERAND_POS;

    if (MsgToFrm)
    {
        CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pMsg->operand.opPhyAddr, 2);
        *pLen += 2;
    }
    else
    {
        CEC_SwapBytes ((UINT8*)&pMsg->operand.opPhyAddr, &pFrame[pos], 2);
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles messages with digital service identification as operand
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pDigSrvIden- Pointer to Digital Service Identifier (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 *          : pos       - Position in frame buffer
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleOpDigSrvId (BOOL MsgToFrm, VPP_CEC_OPERAND_DIG_SRV_ID *pDigSrvIden,
                                 UINT8 *pFrame, UINT8 *pLen, UINT8 pos)
{
    int i;
    if (MsgToFrm)
    {
        pFrame[(*pLen)] = (pDigSrvIden->broadcastSys & 0x7F);
        pFrame[(*pLen)++] |= pDigSrvIden->srvIdenType << 7;

        if (pDigSrvIden->srvIdenType == VPP_CEC_SRV_IDEN_CHN)
        {
            pFrame[*pLen] = pDigSrvIden->srvIden.chnData.chnNumFormat << 2;
            if (pDigSrvIden->srvIden.chnData.chnNumFormat == 2) // 2 part channel number
            {
                // 2 most-significant bits of major channel number
                pFrame[(*pLen)++] |= (pDigSrvIden->srvIden.chnData.majChnNum >> 8);
                // Lower 8 bits of major channel number
                pFrame[(*pLen)++] = (pDigSrvIden->srvIden.chnData.majChnNum & 0xFF);

                pFrame[(*pLen)++] = (pDigSrvIden->srvIden.chnData.minChnNum >> 8) & 0xFF;
                pFrame[(*pLen)++] = pDigSrvIden->srvIden.chnData.minChnNum & 0xFF;
            }
            else
            {
                *pLen += 1;
            }
        }
        else
        {
            if (pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ARIB_GEN ||
                pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ARIB_BS  ||
                pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ARIB_CS  ||
                pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ARIB_T)
            {
                CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pDigSrvIden->srvIden.aribData.txStreamID, 2);
                *pLen += 2;

                CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pDigSrvIden->srvIden.aribData.content_srvID, 2);
                *pLen += 2;

                CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pDigSrvIden->srvIden.aribData.origNetworkID, 2);
                *pLen += 2;
            }
            else if (pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ATSC_GEN ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ATSC_CAB ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ATSC_SAT ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ATSC_TER)
            {
                CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pDigSrvIden->srvIden.atscData.txStreamID, 2);
                *pLen += 2;

                CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pDigSrvIden->srvIden.atscData.content_srvID, 2);
                *pLen += 2;
            }
            else if (pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_GEN ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_C   ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_S   ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_S2  ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_T)
            {
                CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pDigSrvIden->srvIden.dvbData.txStreamID, 2);
                *pLen += 2;

                CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pDigSrvIden->srvIden.dvbData.content_srvID, 2);
                *pLen += 2;

                CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pDigSrvIden->srvIden.dvbData.origNetworkID, 2);
                *pLen += 2;
            }
        }

        // Digital service identification - Fill up rest of the length with zeros since the operand payload
        // has to be 7 bytes (total length of CEC message would then be 9 bytes)
        for (i = (*pLen); i < 9; i++)
            pFrame[(*pLen)++] = 0;
    }
    else
    {
        pDigSrvIden->broadcastSys = pFrame[pos] & 0x7F;
        pDigSrvIden->srvIdenType = pFrame[pos++] >> 7;

        if (pDigSrvIden->srvIdenType == VPP_CEC_SRV_IDEN_CHN)
        {
            pDigSrvIden->srvIden.chnData.chnNumFormat = (pFrame[pos] >> 2) & 0x3F;
            if (pDigSrvIden->srvIden.chnData.chnNumFormat == 2) // 2 part channel number
            {
                pDigSrvIden->srvIden.chnData.majChnNum = pFrame[pos+1] | ((pFrame[pos] & 0x3) << 8);
                pos += 2;
                pDigSrvIden->srvIden.chnData.minChnNum = pFrame[pos+1]  | pFrame[pos] << 8;
                pos += 2;
            }
        }
        else
        {
            if (pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ARIB_GEN ||
                pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ARIB_BS  ||
                pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ARIB_CS  ||
                pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ARIB_T)
            {
                CEC_SwapBytes ((UINT8*)&pDigSrvIden->srvIden.aribData.txStreamID, &pFrame[pos], 2);
                pos += 2;

                CEC_SwapBytes ((UINT8*)&pDigSrvIden->srvIden.aribData.content_srvID, &pFrame[pos], 2);
                pos += 2;

                CEC_SwapBytes ((UINT8*)&pDigSrvIden->srvIden.aribData.origNetworkID, &pFrame[pos], 2);
                pos += 2;
            }
            else if (pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ATSC_GEN ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ATSC_CAB ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ATSC_SAT ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_ATSC_TER)
            {
                CEC_SwapBytes ((UINT8*)&pDigSrvIden->srvIden.atscData.txStreamID, &pFrame[pos], 2);
                pos += 2;

                CEC_SwapBytes ((UINT8*)&pDigSrvIden->srvIden.atscData.content_srvID, &pFrame[pos], 2);
                pos += 2;
            }
            else if (pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_GEN ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_C   ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_S   ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_S2  ||
                     pDigSrvIden->broadcastSys == VPP_CEC_DIG_BROADCAST_SYS_DVB_T)
            {
                CEC_SwapBytes ((UINT8*)&pDigSrvIden->srvIden.dvbData.txStreamID, &pFrame[pos], 2);
                pos += 2;

                CEC_SwapBytes ((UINT8*)&pDigSrvIden->srvIden.dvbData.content_srvID, &pFrame[pos], 2);
                pos += 2;

                CEC_SwapBytes ((UINT8*)&pDigSrvIden->srvIden.dvbData.origNetworkID, &pFrame[pos], 2);
                pos += 2;
            }
        }
    }
    return (VPP_CEC_STATUS_SUCCESS);
}

/******************************************************************************
 * FUNCTION : Handles messages with analog service identification as operand
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pAnaSrvIden- Pointer to Analog Service Identifier (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 *          : pos       - Position in frame buffer
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleOpAnaSrvId (BOOL MsgToFrm, VPP_CEC_OPERAND_ANALOGUE_SRV_ID *pAnaSrvIden,
                                 UINT8 *pFrame, UINT8 *pLen, UINT8 pos)
{
    if (MsgToFrm)
    {
        pFrame[(*pLen)++] = pAnaSrvIden->broadcastType;

        CEC_SwapBytes (&pFrame[*pLen], (UINT8*)&pAnaSrvIden->frequency, 2);
        *pLen += 2;

        pFrame[(*pLen)++] = pAnaSrvIden->broadcastSystem;
    }
    else
    {
        pAnaSrvIden->broadcastType = pFrame[pos++];

        CEC_SwapBytes ((UINT8*)&pAnaSrvIden->frequency, &pFrame[pos], 2);
        pos += 2;

        pAnaSrvIden->broadcastSystem = pFrame[pos++];
    }
    return (VPP_CEC_STATUS_SUCCESS);
}
#endif /* _VPP_BE_CEC_MSG_C_ */
