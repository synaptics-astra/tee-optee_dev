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
//! \file tsp_ta_internal.h
//!
//! \brief TSP TA Internal header file
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    January 14 2014,        Wenjie Zhou
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef _TEE_TSP_TA_INTERNAL_H_
#define _TEE_TSP_TA_INTERNAL_H_


#include "api_dhubLite.h"

#include "Galois_memmap.h"
#include "tsp_local_data.h"
#include "tsp_nsk.h"
#include "tsp_shared.h"
#include "tsp_cmd.h"
#include "tsp_mem_map.h"
#include "tsp_dtcm_heap.h"

#define SECURE_CHECK_CNT_THRESHOLD 200

#define TSP_CHECK_RETCODE(ret)  if ((ret) != TEE_SUCCESS)     \
                        {TA_LOGF( "Err:0x%x @Line:%d\n", ret, __LINE__);goto end;}

typedef struct TEE_TspContext
{
    uint32_t FigoId;
    SIE_SecureCtx *pSecureCtx;
    HDL_hbo hdl;
} TEE_TspContext;

typedef struct NSK_CmChanInfo {
    NSKCmChannel   cmChan;
    int32_t        connCnt[TSP_MAX_FIGO_NUM];
    uint32_t       dtcmAddr[TSP_MAX_FIGO_NUM];

    //if can get the lowerDest  from the NSKKeyLowerDestTable in secureCtx for each figo
    //will be set ture after triggering the related figo to update the ie_NSKKeyLowerDestTable
    bool           bGetLowerDest[TSP_MAX_FIGO_NUM];
} NSK_CmChanInfo;

#endif /* _TEE_TSP_TA_INTERNAL_H_ */

