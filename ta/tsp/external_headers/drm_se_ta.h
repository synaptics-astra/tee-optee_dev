/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 */

////////////////////////////////////////////////////////////////////////////////
//! \file    drm_se_ta.h
//! \brief
//! \author
//! \date
////////////////////////////////////////////////////////////////////////////////

#ifndef __DRM_SE_TA_H__
#define __DRM_SE_TA_H__

#include "drm_log.h"
#ifndef CONFIG_OPTEE
#include "drm_memory.h"
#endif
#include "mem_region_userdata.h"
#include "tee_internal_api.h"
#include "drm_se_cmd.h"


#define SHM_ALIGN_SIZE_4_KB 4096
#define roundup(size)  ((size + SHM_ALIGN_SIZE_4_KB - 1) & (~(SHM_ALIGN_SIZE_4_KB - 1)))

typedef enum MV_KEYTABLE_AREA_TYPE
{
    MV_KEYTABLE_AREA_GENERAL,
    MV_KEYTABLE_AREA_HWKL,
    MV_KEYTABLE_AREA_BCM_SWKL,
    MV_KEYTABLE_AREA_NSK_HWKL,
    MV_KEYTABLE_AREA_NSK_M2M_HWKL,
    MV_KEYTABLE_AREA_GKL,
    MV_KEYTABLE_AREA_IRDETO_BRD_HWKL,
    MV_KEYTABLE_AREA_IRDETO_OTT_HWKL,
    MV_KEYTABLE_AREA_IRDETO_PVR_HWKL,
    MV_KEYTABLE_AREA_MAX,
} MV_KEYTABLE_AREA_TYPE;

typedef enum MV_KEYTABLE_KEY_USAGE
{
    MV_KEYTABLE_KEY_USAGE_INVALID,
    MV_KEYTABLE_KEY_USAGE_DEC,
    MV_KEYTABLE_KEY_USAGE_ENC,
    MV_KEYTABLE_KEY_USAGE_DEC_ENC,  //For timeshift feature
    MV_KEYTABLE_KEY_USAGE_SIGN,
    MV_KEYTABLE_KEY_USAGE_VERIFY,
    MV_KEYTABLE_KEY_USAGE_MAX,
} MV_KEYTABLE_KEY_USAGE;


TEE_Result DRM_SE_CheckMemref(char * pName, void *buf, uint32_t size);
TEE_Result DRM_SE_IsCallFromCA(bool *bIsFromCA);

#endif
