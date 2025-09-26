/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       tee_ta_vpp.h
//! \brief      data structure and interfaces used & implemented within
//!                    TA wrapper layer.
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef __TEE_TA_VPP_H__
#define __TEE_TA_VPP_H__

#ifndef CONFIG_OPTEE
#include "tee_internal_api.h"
#include "tee_ta.h"
#else
#include "tee_api_syna.h"
#endif
#include "tee_ta_osal.h"
#include "vpp_cmd.h"
#include "vpp_api.h"
#include "vdec_com.h"
#include "vpp_module.h"
#include "assert.h"
#include "tee_ta_mmap_helper.h"

//#define VPP_TA_LOG_CMD_ENTRY

//Enable following macro if the COMMAND enum itself is removed from VPP_G4 CA
#define REMOVE_BG4_NON_SUPPORTED_INTERFACE

#define VPP_CIEL_TO_4K_ALIGN(val)   ((val+0xFFF)&(~0xFFF))

#define VPP_FEATURE_HDMITX            (1<<0)
#define VPP_FEATURE_MIPI_DSI          (1<<1)
#define MAX_NUM_FEATURE_CFG           1

#ifndef CONFIG_OPTEE
extern const TEE_TA TAVPP;
#endif

/***************************************************************
 * FUNCTION: Register/Map the physical address of AVIO register space
 *
 * IN PARAMS : None
 * OUT PARAMS : None
 * RETURN:  TEE_SUCESS - succeed
 *          Otherwise - Some register space cannot be mapped for access
 ****************************************************************/
TEE_Result VPP_TA_AddMemMap(void);

/***************************************************************
 * FUNCTION: Process BindSHM TA command
 *
 * IN PARAMS : TEE_Param passed from CA to TA
 * OUT PARAMS : None
 * RETURN:  TEE_SUCESS - succeed
 *          d$Otherwise - result as returned by respective command handlers
 ****************************************************************/
int MV_VPPOBJ_BindSHM(TEE_Param params[4]);

/***************************************************************
 * FUNCTION: Process BindSHM_PHY TA command
 *
 * IN PARAMS : TEE_Param passed from CA to TA
 * OUT PARAMS : None
 * RETURN:  TEE_SUCESS - succeed
 *          d$Otherwise - result as returned by respective command handlers
 ****************************************************************/
int MV_VPPOBJ_BindSHM_PHY(TEE_Param params[4]);

/***************************************************************
 * FUNCTION: Process BindSHM_PAR TA command
 *
 * IN PARAMS : TEE_Param passed from CA to TA
 * OUT PARAMS : None
 * RETURN:  TEE_SUCESS - succeed
 *          d$Otherwise - result as returned by respective command handlers
 ****************************************************************/
int MV_VPPOBJ_BindSHM_PAR(TEE_Param params[4]);

#endif /* __TEE_TA_VPP_H__ */
