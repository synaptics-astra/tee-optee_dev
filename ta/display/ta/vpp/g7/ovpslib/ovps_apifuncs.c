/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#define _OVPS_APIFUNCS_C_
#include "vpp_module.h"
#include "vpp.h"
#include "ovps_api.h"

INT OVPS_NGPTV_EnableBypass(VPP_OBJ* vpp_obj, UINT32 uiBypassOn)
{
    int retStatus = MV_VPP_OK;
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_OVPSCLWRAP + RA_OVPSCLWRAP_CTRL;
    if(uiBypassOn) {
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ngptv_bypass = 1;
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ngptv_scl_sel = 0;
    } else {
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ngptv_bypass = 0;
        OVPS_PARAMS->ovpsclwrap_ctrl.uCTRL_ngptv_scl_sel = 1;
    }
    GA_REG_WORD32_WRITE(RegAddr, OVPS_PARAMS->ovpsclwrap_ctrl.u32);

    return retStatus;
}

int OVPS_NotifyEvent(VPP_OVP_OBJ *ovp_obj, UINT32 event, UINT32 infoID)
{
    if (ovp_obj == NULL)
        return (MV_VPP_EBADPARAM);

    //events used in case of Q12 mode only
    if((ovp_obj->m_BCM_Sched == BCM_SCHED_Q12) && (ovp_obj->callback != NULL)) {
            VPP_CB_FUNC(ovp_obj->callback,(void *)ovp_obj, event, (void *)&infoID, NULL);
    }
    return (MV_VPP_OK);
}
