/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VPP_SCL_C_

#include "vpp_module.h"
#include "vpp_frc_scl.h"
#include "vpp.h"
#include "vpp_offlineScl.h"

/*******************************************************************
 * FUNCTION: Reset all VPP SCL blocks
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 ******************************************************************/
int VPP_SCL_Reset(VPP_OBJ *vpp_obj)
{
#ifdef FIXME_IN_BG4CT
    /* reset FRC-SCLs */
    VPP_REG_WRITE32(RA_Vpp_SCL_CLKEN_CTRL, 0x3F); // disable data enable signal checking for SCL fifo reading
#endif
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_MAIN);
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_DET);
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_PIP);
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_OSD);
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_GFX2);
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_VIP_Y);
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_VIP_UV);
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_OVP_Y);
    FRC_SCL_LoadDefaultVal(vpp_obj, VPP_FRC_SCL_OVP_UV);

    VPP_OfflineScl_LoadDefaultVal(vpp_obj);

    return (MV_VPP_OK);
}


/**********************************************************************
 * FUNCTION: initialize all VPP SCL blocks according to configuration
 * PARAMS:   *vpp_obj - pointer to VPP object
 * RETURN: MV_VPP_OK
 *********************************************************************/
int VPP_SCL_Config(VPP_OBJ *vpp_obj)
{
	FRC_SCL_GenerateCoeffTables(vpp_obj);

	/* config FRC-SCLs */
	if (vpp_obj->chan[CHAN_MAIN].dvID != CPCB_INVALID){
		FRC_SCL_SetDeLrstDelay(vpp_obj, VPP_FRC_SCL_MAIN, 80);
		FRC_SCL_SetDeLrstDelay(vpp_obj, VPP_FRC_SCL_DET, 80);
	}

	if (vpp_obj->chan[CHAN_PIP].dvID != CPCB_INVALID){
		FRC_SCL_SetDeLrstDelay(vpp_obj, VPP_FRC_SCL_PIP, 80);
	}

	if (vpp_obj->chan[CHAN_GFX1].dvID != CPCB_INVALID){
		FRC_SCL_SetDeLrstDelay(vpp_obj, VPP_FRC_SCL_OSD, 80);
	}
	if (vpp_obj->chan[CHAN_GFX2].dvID != CPCB_INVALID){
		FRC_SCL_SetDeLrstDelay(vpp_obj, VPP_FRC_SCL_GFX2, 80);
	}
	INT upsCoeff[] = {80, 0, -304, 0, 1248, 2048, 1248};
	//    INT upsCoeff[] = {-7,47,-25,-341,1056,2636,1056};
	INT dnsCoeff[] ={80, 0, -304, 0, 1248, 2048};
	VPP_SCL_ConfigUpsCoeff(vpp_obj, upsCoeff, VPP_FRC_SCL_MAIN);
	VPP_SCL_ConfigDnsCoeff(vpp_obj, dnsCoeff, VPP_FRC_SCL_MAIN);

    return (MV_VPP_OK);
}

/***************************************************************************************
 * FUNCTION: set BE FIFO for loopback
 * PARAMS: pCpcb0Tot - CPCB0 output total resolution
 *               pCpcb0Res - CPCB0 active resolution
 * RETURN:  1 - succeed
 *                others - failed to load CPCB BE FIFO values to BCM buffer
 ***************************************************************************************/
int VPP_SCL_SetBeFifo(VPP_OBJ *vpp_obj, PVPP_FRC_RES pCpcb0Tot, PVPP_FRC_RES pCpcb0Res)
{
    FRC_SCL_SetBeFifo(vpp_obj, pCpcb0Tot, pCpcb0Res);

    return (MV_VPP_OK);
}
