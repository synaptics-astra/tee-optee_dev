/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _VPP_BE_HDCP_C_
#define _VPP_BE_HDCP_C_

#include "vpp_api.h"
#include "vpp_be.h"
#include "vpp_module.h"
#include "vpp_regs.h"
#include "vpp_com.h"
#include "vpp_bcmbuf.h"

#ifdef GPIO_I2C
#endif
// Can be included?
#ifndef CONFIG_AMP_TEE_ENABLE
#include "GaloisTime.h"
#endif
#include "galois_io.h"
#include "vpp_be_hdcp.h"
#include "vpp_be_hdcp_prv.h"
#include "vpp_be_prv.h"
#include "vpp_be_hdmitx.h"
#include "hdtx.h"
#include "hdmi_eddc.h"
#ifndef CONFIG_OPTEE
#ifdef CONFIG_AMP_TEE_ENABLE
#include "tee_internal_time_api.h"
#endif
#endif


#include "com_type.h"


/*-----------------------------------------------------------------------------
 * Description  : This function starts HDCP authentication protocol - it does
 *              : not start encryption though. That is handled only when the
 *              : receiver is authenticated.
 * Arguments    : None
 * Returns      : HDCP_ERR_OK, on success
 *              : Error code, otherwise
 * Notes        : The same function is to be used to re-authenticate also
 *-----------------------------------------------------------------------------
 */
HDCP_RETURNS BE_HDCP_StartHdcp(VPP_OBJ *vpp_obj, UINT32 *pAuthState, BOOL curOutHdmiMode)
{
    /* TODO: map to SNPS */
    return HDCP_ERR_OK;
}

/*-----------------------------------------------------------------------------
 * Description  : This function stops HDCP encryption and brings the HDCP
 *              : state machine to idle state. Interrupt handling would not
 *              : be supported in this state.
 * Arguments    : None
 * Returns      : HDCP_ERR_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
HDCP_RETURNS BE_HDCP_StopHdcp(VPP_OBJ *vpp_obj, UINT32 *pAuthState)
{
    /* TODO: map to SNPS */
    return HDCP_ERR_OK;
}

/*-----------------------------------------------------------------------------
 * Description  : This function handles the HDCP interrupts which are
 *              : enabled
 * Arguments    : None
 * Returns      : HDCP_ERR_OK, on success
 *              : Error code, otherwise
 * Notes        : Clearing the interrupts is also handled after the interrupt
 *              : is serviced
 *-----------------------------------------------------------------------------
 */
HDCP_RETURNS BE_HDCP_HandleInterrupt(VPP_OBJ *vpp_obj,UINT32 *pAuthState)
{
    /* TODO: map to SNPS if required */
    return HDCP_ERR_OK;
}


/*-----------------------------------------------------------------------------
 * Description  : This function controls frame encryption
 * Arguments    : Enable - Enable/Disable encryption
 * Returns      : HDCP_ERR_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
HDCP_RETURNS BE_HDCP_EnableEncryption(VPP_OBJ *vpp_obj,BOOL Enable,BOOL bufWrite)
{
    /* TODO: map to SNPS */
    return HDCP_ERR_OK;
}

/*-----------------------------------------------------------------------------
 * Description  : This function returns frame encryption enable status
 * Arguments    : *pEnableSts - Pointer to return enable status
 * Returns      : HDCP_ERR_OK, on success
 *              : Error code, otherwise
 * Notes        : None
 *-----------------------------------------------------------------------------
 */
HDCP_RETURNS BE_HDCP_ConfigureSRM(VPP_OBJ *vpp_obj,UINT8 *pSrmBuf, int srmBufLen,
                                BOOL forceUpdate, BOOL *pSrmUpdated, UINT32 *pErrorInfo)
{
    HDCP_RETURNS            retCode = HDCP_ERR_OK;
    return (retCode);
}

#endif // _VPP_BE_HDCP_C_
