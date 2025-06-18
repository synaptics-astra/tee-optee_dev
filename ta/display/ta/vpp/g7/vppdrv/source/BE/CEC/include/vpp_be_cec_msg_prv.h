/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __VPP_BE_CEC_MSG_PRV_H__
#define __VPP_BE_CEC_MSG_PRV_H__

#include "com_type.h"

/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */
#define CEC_MSG_OPERAND_POS         2

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Sets message definition in the table
 * PARAMS   : pCecMsgTable - Pointer to messsage table
 *          : opcode       - Message opcode
 *          : procFn       - Pointer to process function
 *          : mode         - Message mode
 *          : operandLen   - Message operand length
 * RETURN   : TRUE, if copied; FALSE, otherwise
 *****************************************************************************/
static void CEC_SetMessage (PBE_CEC_MSG_DATA pCecMsgTable, UINT8 opcode,
                            BE_CEC_MSG_PROC_FN procFn, BE_CEC_MSG_MODE mode,
                            UINT8 operandLen);

/******************************************************************************
 * FUNCTION : Copies from source to destination in the same/reverse order
 * PARAMS   : pDest        - Pointer to destination
 *          : pSrc         - Pointer to source
 *          : pDest        - Length to be copied
 * RETURN   : TRUE, if copied; FALSE, otherwise
 *****************************************************************************/
static BOOL CEC_SwapBytes (UINT8 *pDest, UINT8 *pSrc, int len);

/******************************************************************************
 * FUNCTION : Convert decimal number to BCD
 * PARAMS   : *Num - Pointer to decimal number (Input)
 *          : *BCD - Pointer to BCD equivalent (Output)
 * RETURN   : TRUE, if converted; FALSE, otherwise
 *****************************************************************************/
static BOOL CEC_NumToBCD(UINT8 *Num, UINT8 *BCD);

/******************************************************************************
 * FUNCTION : Convert BCD to decimal number
 * PARAMS   : *BCD - Pointer to BCD number (Input)
 *          : *Num - Pointer to decimal equivalent (Output)
 * RETURN   : TRUE, if converted; FALSE, otherwise
 *****************************************************************************/
static BOOL CEC_BCDToNum(UINT8 *BCD, UINT8 *Num);

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
                                      UINT8 *pFrame, UINT8* pLen);

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
                                       UINT8 *pFrame, UINT8* pLen);

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
                                     UINT8 *pFrame, UINT8* pLen);

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
                                    UINT8 *pFrame, UINT8* pLen);

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
                                     UINT8 *pFrame, UINT8* pLen);

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
                                    UINT8 *pFrame, UINT8* pLen);

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
                                         UINT8 *pFrame, UINT8* pLen);

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
                                       UINT8 *pFrame, UINT8* pLen);

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
                                  UINT8 *pFrame, UINT8* pLen);

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
                                      UINT8 *pFrame, UINT8* pLen);

/*-----------------------------------------------------------------------------
 * DECK CONTROL MESSAGES
 *-----------------------------------------------------------------------------
 */
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
                                    UINT8 *pFrame, UINT8* pLen);

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
                                        UINT8 *pFrame, UINT8* pLen);

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
                                     UINT8 *pFrame, UINT8* pLen);

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
                              UINT8 *pFrame, UINT8* pLen);

/*-----------------------------------------------------------------------------
 * OSD STATUS DISPLAY MESSAGES
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Handles "Set OSD string" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgSetOSDString (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                      UINT8 *pFrame, UINT8* pLen);


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
                                    UINT8 *pFrame, UINT8* pLen);

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
                                        UINT8 *pFrame, UINT8* pLen);


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
                                     UINT8 *pFrame, UINT8* pLen);

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
                                   UINT8 *pFrame, UINT8* pLen);

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
                                         UINT8 *pFrame, UINT8* pLen);

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
                                      UINT8 *pFrame, UINT8* pLen);

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
                                            UINT8 *pFrame, UINT8* pLen);

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
                                       UINT8 *pFrame, UINT8* pLen);

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
static INT CEC_HandleMsgSelDigService (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                       UINT8 *pFrame, UINT8* pLen);

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
                                        UINT8 *pFrame, UINT8* pLen);

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
                                        UINT8 *pFrame, UINT8* pLen);

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
                                         UINT8 *pFrame, UINT8* pLen);

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
                                             UINT8 *pFrame, UINT8* pLen);

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
                                         UINT8 *pFrame, UINT8* pLen);

/******************************************************************************
 * FUNCTION : Handles "report short audio Descriptor" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgReportShortAudioDesc (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen);

/******************************************************************************
 * FUNCTION : Handles "request  short audio descritor" message operands
 * PARAMS   : MsgToFrm  - Indicates if message has to be converted to frame
 *          :           - or viceversa
 *          : pMsg      - Pointer to CEC message (Input)
 *          : pFrame    - Pointer to frame (Input)
 *          : pFrameLen - Pointer to frame length (I/O), depending on MsgToFrm
 * RETURN   : BE_CEC_ERR_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_HandleMsgRequestShortAudioDesc (BOOL MsgToFrm, PVPP_CEC_MSG pMsg,
                                        UINT8 *pFrame, UINT8* pLen);
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
                                      UINT8 *pFrame, UINT8* pLen);

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
                                        UINT8 *pFrame, UINT8* pLen);

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
                                       UINT8 *pFrame, UINT8* pLen);

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
                                        UINT8 *pFrame, UINT8* pLen);

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
                                          UINT8 *pFrame, UINT8* pLen);

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
                                            UINT8 *pFrame, UINT8* pLen);

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
                                     UINT8 *pFrame, UINT8* pLen);

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
                                UINT8 *pFrame, UINT8* pLen);

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
                                 UINT8 *pFrame, UINT8 *pLen, UINT8 pos);

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
                                 UINT8 *pFrame, UINT8 *pLen, UINT8 pos);

#endif // __VPP_BE_CEC_MSG_PRV_H__
