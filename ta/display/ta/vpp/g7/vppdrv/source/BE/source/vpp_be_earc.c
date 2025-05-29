/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#include "vpp_api.h"
#include "vpp_api_types.h"
#include "vpp_module.h"
#include "vpp_cfg.h"
#include "vpp_be.h"
#include "vpp_be_prv.h"


#ifdef EARC_RX_ENABLED
#include "vpp_be_earc_vendor.h"

#define VPP_BE_EARC_VALIDATE_PARAM(pHdmiTx)        \
    do { if (pHdmiTx == NULL)                      \
        return (MV_VPP_EBADPARAM);                 \
    if (pHdmiTx->earcContext.initialized == false) \
        return (MV_VPP_EUNCONFIG); } while (0)


/********************************************************************************
 * FUNCTION: Register for eARC event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcRegisterCB(VPP_OBJ *vpp_obj, UINT32 eventCode,
                          UINT32 cbFnAddr, UINT32 contextParam)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    pHdmiTx->earcContext.eventCode = eventCode;
    pHdmiTx->earcContext.cb_func = (MV_VPP_EVENT_CALLBACK) cbFnAddr;
    pHdmiTx->earcContext.contextParam = (void*)contextParam;

    return BE_EARCRX_SetEventFlag(pHdmiTx, true);
}

/********************************************************************************
 * FUNCTION: Unregister for eARC event notifications
 * INPUT: vpp_obj
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcUnregisterCB(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    if (pHdmiTx == NULL)
        return (MV_VPP_EBADPARAM);

    pHdmiTx->earcContext.eventCode = 0;
    pHdmiTx->earcContext.cb_func = NULL;
    pHdmiTx->earcContext.contextParam = NULL;

    return BE_EARCRX_SetEventFlag(pHdmiTx, false);
}

/********************************************************************************
 * FUNCTION: eARC Set mode (eARC/ARC/None)
 * INPUT: hdmiMode - 0:None, 1:ARC, 2:eARC
 * RETURN: MV_VPP_OK - SUCCEED
 * MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eARCSetMode(VPP_OBJ *vpp_obj, EARC_MODE arcMode)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPP_BE_EARC_VALIDATE_PARAM(pHdmiTx);

    pHdmiTx->earcContext.userMode = arcMode;
    return BE_EARCRX_SetMode(pHdmiTx, arcMode);
}

/********************************************************************************
 * FUNCTION: eARC Reset mode
 * INPUT: None
 * RETURN: MV_VPP_OK - SUCCEED
 * MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eARCResetMode(VPP_OBJ *vpp_obj)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPP_BE_EARC_VALIDATE_PARAM(pHdmiTx);

    return BE_EARCRX_ResetMode(pHdmiTx);
}

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
                       UINT8 *earc_caps)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPP_BE_EARC_VALIDATE_PARAM(pHdmiTx);

    return BE_EARCRX_SetCapabilityStruct(pHdmiTx, offset, length, earc_caps);
}

/********************************************************************************
 * FUNCTION: Set eARC Config
 * INPUT: vpp_obj
 *        config  - pointer to structure of type EARC_CONFIG
 *        bufSize - Size of config
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcSetConfig(VPP_OBJ *vpp_obj, EARC_CONFIG *earcConfig)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPP_BE_EARC_VALIDATE_PARAM(pHdmiTx);

    BE_EARCRX_SetCapabilityStruct(pHdmiTx, 0, EARC_CAPABILITIES_LENGTH,
                                  earcConfig->earcCapabilities);
    BE_EARCRX_SetMode(pHdmiTx, earcConfig->bootupEarcMode);
    pHdmiTx->earcContext.userMode = earcConfig->bootupEarcMode;
    BE_EARCRX_ConfigureLatency(pHdmiTx, earcConfig->latency);
    BE_EARCRX_SetExtractionMode(pHdmiTx, earcConfig->extrAudio);

    return (MV_VPP_OK);
}

/********************************************************************************
 * FUNCTION: Update current eARC audio latency
 * INPUT: vpp_obj
 *        latency - Current audio latency value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 ********************************************************************************/
int VPP_BE_eArcAudioLatency(VPP_OBJ *vpp_obj, UINT8 audio_latency)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPP_BE_EARC_VALIDATE_PARAM(pHdmiTx);

    return BE_EARCRX_ConfigureLatency(pHdmiTx, audio_latency);
}

/*******************************************************************
 * FUNCTION: Notifies eARC events to application
 * PARAMS:   *vpp_obj  - pointer to VPP object
 *           event     - eventCode
 *           *pEvtInfo - EARC event info to send
 * RETURN:   MV_VPP_OK
 ******************************************************************/
int EarcNotifyEvent(VPP_OBJ *vpp_obj, UINT32 event, EARC_EVT_INFO *pEvtInfo)
{
    BE_HDMITX_DATA *pHdmiTx = (BE_HDMITX_DATA*)(vpp_obj->be.hdmiTx);

    VPP_BE_EARC_VALIDATE_PARAM(pHdmiTx);

    if (pHdmiTx->earcContext.cb_func != NULL) {
        VPP_CB_FUNC(pHdmiTx->earcContext.cb_func,(void *)vpp_obj,event,
                    (void *) pEvtInfo, pHdmiTx->earcContext.contextParam);
    }

    return (MV_VPP_OK);
}

#endif /* EARC_RX_ENABLED */
