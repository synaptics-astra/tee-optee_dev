/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifdef EARC_RX_ENABLED

#include "Galois_memmap.h"
#include "galois_io.h"
#include "global.h"
#include "vbuf.h"
#include "vpp_module.h"
#include "vpp_apifuncs.h"
#include "vpp_api_types.h"
#include "vpp_be.h"
#include "vpp_be_hdmitx.h"
#include "vpp_be_earc.h"

#if defined(VPP_DIAGS)
#include "pllDiag.h"
#else
#include "vpp_apifuncs_wrapper.h"
#endif //VPP_DIAGS
#include "vpp.h"

#include "vpp_isr.h"
#include "api_avio_dhub.h"

#define EARC_CMD_CHECK(handle)  \
    do { if (g_vpp->magic != VPP_MAGIC){  \
        return (MV_VPP_EUNCONFIG);   \
    }  \
    if (handle<0 || handle>=MAX_NUM_OBJECTS)  \
        return (MV_VPP_EBADPARAM);  \
    vpp_obj = g_vpp->vpp_objs[handle];  \
    if (!vpp_obj)  \
        return (MV_VPP_ENODEV);  \
    if (vpp_obj->status == STATUS_INACTIVE){ \
        return (MV_VPP_EUNCONFIG);  \
    }} while (0)


/********************************************************************************
 * FUNCTION: Set eARC mode
 * INPUT: earcMode - 0:None, 1:ARC, 2:eARC
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_eARC_SetMode(int handle, EARC_MODE arcMode)
{
    VPP_OBJ *vpp_obj;

    EARC_CMD_CHECK(handle);

    VPPAPI_DBG_INFO(" eARC_SetMode [%d] \n", arcMode);
    return VPP_BE_eARCSetMode (vpp_obj, arcMode);
}

/********************************************************************************
 * FUNCTION: Reset eARC mode
 * INPUT: None
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_eARC_ResetMode(int handle)
{
    VPP_OBJ *vpp_obj;

    EARC_CMD_CHECK(handle);

    VPPAPI_DBG_INFO(" eARC_ResetMode\n");
    return VPP_BE_eARCResetMode(vpp_obj);
}

/********************************************************************************
 * FUNCTION: Register for eARC event notifications
 * INPUT: eventCode - Event code for which notifications has to be raised
 *      : cbFnAddr - Address of callback function
 *      : contextParam - Context parameter for the registering application
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_eARC_RegisterCallback (int handle, UINT32 eventCode,
                                     MV_VPP_EVENT_CALLBACK callbackFn,
                                     void *contextParam)
{
    VPP_OBJ *vpp_obj;

    EARC_CMD_CHECK(handle);

    return VPP_BE_eArcRegisterCB(vpp_obj, (int)eventCode, (int)callbackFn,
                                 (int)contextParam);
}

/********************************************************************************
 * FUNCTION: Unregister for eARC event notifications
 * INPUT: handle
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
int MV_VPPOBJ_eARC_UnRegisterCallback (int handle)
{
    VPP_OBJ *vpp_obj;

    EARC_CMD_CHECK(handle);

    return VPP_BE_eArcUnregisterCB (vpp_obj);
}

/********************************************************************************
 * FUNCTION: Set eARC capabilities
 * INPUT:  offset - offset from where to capabilities needs to be updated
 *         length - Length earc_cap array
 *         earc_cap - eARC capabilities buffer
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
INT MV_VPPOBJ_eARC_SetCapabilities(int handle, UINT8 offset, size_t length,
                                   UINT8 *earc_cap)
{
    VPP_OBJ *vpp_obj;

    EARC_CMD_CHECK(handle);

    return VPP_BE_eArcSetCaps(vpp_obj, offset, length, earc_cap);
}

/********************************************************************************
 * FUNCTION: Set Earc Config values
 * INPUT:  EARC config values
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 ********************************************************************************/
int MV_VPPOBJ_eARC_SetConfig(int handle, EARC_CONFIG *config)
{
    VPP_OBJ *vpp_obj;

    EARC_CMD_CHECK(handle);
    return VPP_BE_eArcSetConfig(vpp_obj, config);
}

/********************************************************************************
 * FUNCTION: Update actual eARC audio latency
 * INPUT: latency - Actual system latency value
 * RETURN: MV_VPP_OK - SUCCEED
 *         MV_EBADPARAM - invalid parameters
 *         MV_EUNCONFIG - VPP not configured
 *         MV_VPP_ENODEV - no device
 *         MV_EUNSUPPORT - channel not connected in configuration
 *         MV_VPP_EBADCALL - channel not connected to DV1
 *         MV_ECMDQFULL - command queue is full
 ********************************************************************************/
INT MV_VPPOBJ_eARC_AudioLatency(int handle, UINT8 latency)
{
    VPP_OBJ *vpp_obj;

    EARC_CMD_CHECK(handle);

    return VPP_BE_eArcAudioLatency (vpp_obj, latency);
}

#endif /* EARC_RX_ENABLED */
