/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_BE_EARC_H_
#define _VPP_BE_EARC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpp_module.h"

#ifdef EARC_RX_ENABLED
/********************************************************************************
 * FUNCTION: eARC Set mode (eARC/ARC/None)
 * INPUT: arcMode - 0:None, 1:ARC, 2:eARC
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eARCSetMode(VPP_OBJ *vpp_obj, EARC_MODE arcMode);

/********************************************************************************
 * FUNCTION: eARC Reset mode
 * INPUT: None
 * RETURN: MV_VPP_OK - SUCCEED
 * MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eARCResetMode(VPP_OBJ *vpp_obj);

/********************************************************************************
 * FUNCTION: Register for eARC event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcRegisterCB(VPP_OBJ *vpp_obj, UINT32 eventCode,
                          UINT32 cbFnAddr, UINT32 contextParam);

/********************************************************************************
 * FUNCTION: Unregister for eARC event notifications
 * INPUT: vpp_obj
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcUnregisterCB(VPP_OBJ *vpp_obj);

/********************************************************************************
 * FUNCTION: Set eARC Capabilities
 * INPUT: vpp_obj
 *        offset - offset from where the capabilities needs to be updated
 *        length - Maximum 256
 *        earc_caps - UINT8 array of capabilities
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcSetCaps(VPP_OBJ *vpp_obj, UINT8 offset, size_t length,
                       UINT8 *earc_caps);

/********************************************************************************
 * FUNCTION: Set eARC Config
 * INPUT: vpp_obj
 *        config  - pointer to structure of type EARC_CONFIG
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcSetConfig(VPP_OBJ *vpp_obj, EARC_CONFIG *config);

/********************************************************************************
 * FUNCTION: Update current eARC audio latency
 * INPUT: vpp_obj
 *        latency - Current audio latency value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcAudioLatency(VPP_OBJ *vpp_obj, UINT8 audio_latency);

/*******************************************************************
 * FUNCTION: Notifies eARC events to application
 * PARAMS:   *vpp_obj  - pointer to VPP object
 *           event     - eventCode
 *           *pEvtInfo - EARC event info to send
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int EarcNotifyEvent(VPP_OBJ *vpp_obj, UINT32 event,
                    EARC_EVT_INFO *pEvtInfo);

#endif /* EARC_RX_ENABLED */

#endif /* _VPP_BE_EARC_H_ */
