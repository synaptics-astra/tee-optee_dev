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
//! \file tsp_figo.h
//!
//! \brief TSP FIGO header file
//!
//!
//!     Version    Date                     Author
//!     V 0.1,     SEP 23 2022              zhwan
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "tee_internal_api.h"

#ifndef _TEE_TSP_FIGO_H_
#define _TEE_TSP_FIGO_H_

#define TSP_HBO_FIFO_ID_MAX (TSP_HBO_FIFO_ID_TSI12_PKT + 1)
#define TSP_TSI_PORT_MAX    arr_TspDtcmGlobal_TsiBufTable
#define SECURE_RULT_TABLE_SIZE 32
extern const uint16_t SecureRuleTbl[SECURE_RULT_TABLE_SIZE];

typedef struct TEE_FigoContext
{
    uint8_t            uFigoId;
    uint8_t            *base;
    void               *hDtcmHeap;
    void               *hSecureDtcmHeap;
    uint16_t           uSecureRuleTableAddr;
    uint16_t           u_pLocalData_all_filter;
    HDL_hbo            hdl;
    bool               bInit;
} TEE_FigoContext;

typedef struct {
    const uint32_t figo_id;
    const uintptr_t base;    //Physical base address
    const uint32_t size;    //Region size
} TSPTA_RegInfo;

typedef struct {
    bool isCfg;
    uint32_t figo_id;
    uint32_t fifo_id;
    uint32_t base;
    uint32_t depth;
    uint32_t enable;
    HDL_hbo  hdl;
} TSPTA_HBOCfg;

typedef struct {
    uint32_t ResetCtrl;
    uint32_t PortCtrl;
    uint32_t SyncDetect;
    uint32_t PacketFormat;
    uint32_t ErrorDetect;
    uint32_t PidFilter;
    uint32_t StcClkDiv;
    uint32_t FifoSelect;
} TSPTA_TSICfg;

TEE_Result tsp_figo_init(uint32_t uFigoId);
TEE_FigoContext *tsp_figo_get(uint32_t uFigoId);
void * tsp_get_dtcmHeap(uint32_t uFigoId, uint32_t memType);

#endif /* _TEE_TSP_FIGO_H_ */

