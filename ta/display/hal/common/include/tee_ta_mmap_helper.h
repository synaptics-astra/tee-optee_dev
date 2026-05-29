/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file tee_ta_mmap_helper.h
//!
//! \brief Implementation of VPP memory mapping helper functions
//!
//! Purpose: This is used for mapping VPP register Physical to virual address
//!          and vice versa
//!
//!     Version    Date                     Author
//!     V 1.00,    Apr 1 2024,             Shanmugam Ramachandran
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef _TEE_TA_MMAP_HELPER_H_
#define _TEE_TA_MMAP_HELPER_H_

#include "tee_internal_api.h"

#include "tee_ta_mmap_priv.h"
#include "tee_ta_mmap_types.h"

#ifdef CONFIG_OPTEE
#include "tee_api_types.h"
#include "tee_api_syna.h"
#endif

#include "com_type.h"

TEE_Result TEE_TA_Register_RegRegion_Local(void *phy_addr, UINT32 size, UINT32 opt1, UINT32 opt2, void **va, TEE_TA_REG_REGION *regRegion);
void* TEE_TA_RegPhyToVirt_Local(UINT32 phy_addr, TEE_TA_REG_REGION *regRegion);


#endif //_TEE_TA_MMAP_HELPER_H_
