/* SPDX-License-Identifier: Proprietary */
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

#ifndef __VPU_LOG_H_
#define __VPU_LOG_H_
#include <stdio.h>
#include <stdint.h>

#include "config_log.h"
#include "ta_log.h"

#ifdef ENABLE_VPU_DEBUG
#define D_LogWarn(x) \
    if ((x) != VMETACBLOCK_SUCCESS) { \
        printf("%d: A Call in VCODEC TA failed\n", __LINE__); \
    }
#define D_TEELogWarn(x) \
    if ((x) != TEE_SUCCESS) { \
        printf("%d: A Call in VCODEC TA failed\n", __LINE__); \
    }
#define D_LogRet(x) \
    if ((x) != VMETACBLOCK_SUCCESS) { \
        printf("%d: A Call in VCODEC TA failed\n", __LINE__); \
    }
#else
#define D_LogWarn(x) x
#define D_TEELogWarn(x) x
// define this macro for simple value statement, to avoid "statement with no effect" warning
#define D_LogRet(x)
#endif


/**
 * enable vmeta ta log
*/
#define ENABLE_TA_LOG

#define VMETA_LOGI(format, ...) AMP_TA_Log(AMP_LOG_HIGH, format, ##__VA_ARGS__)
#define VMETA_LOGF(format, ...) AMP_TA_Log(AMP_LOG_FATAL, format, ##__VA_ARGS__)
#define VMETA_LOGE(format, ...) AMP_TA_Log(AMP_LOG_ERROR, format, ##__VA_ARGS__)
#define VMETA_LOGD(format, ...) AMP_TA_Log(AMP_LOG_INFO, format, ##__VA_ARGS__)
#define VMETA_LOGM(format, ...) AMP_TA_Log(AMP_LOG_MID, format, ##__VA_ARGS__)
#define VMETA_LOGL(format, ...) AMP_TA_Log(AMP_LOG_LOW, format, ##__VA_ARGS__)
#define VMETA_LOGU1(format, ...) AMP_TA_Log(AMP_LOG_USER1, format, ##__VA_ARGS__)
#define VMETA_LOGU2(format, ...) AMP_TA_Log(AMP_LOG_USER1, format, ##__VA_ARGS__)

#ifdef ENABLE_TA_LOG
#define AMP_TA_Log(level, format, ...)                              \
    vmeta_ta_log(level, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define AMP_TA_Log(lev, format, ...)
#endif

typedef enum {
    AMP_LOG_FATAL = 0,          /**< For errors that system can not continue. */
    AMP_LOG_ERROR,              /**< For errors that system can still work. */
    AMP_LOG_HIGH,               /**< High level debug info. */
    AMP_LOG_MID,                /**< Middle level debug info. */
    AMP_LOG_LOW,                /**< Low level debug info. */
    AMP_LOG_INFO,               /**< Periodical and trivial debug info. */
    AMP_LOG_USER1,              /**< User specific debug info. */
    AMP_LOG_USER2,              /**< User specific debug info. */
    AMP_LOG_LEVEL_MAX = 8,      /**< MAX level should not exceed 7. */
} AMP_LOG_LEVEL;

#ifdef ENABLE_TA_LOG
void vmeta_ta_log(uint32_t level, const char *func, uint32_t line,
                  const char *format, ...);
#endif

void vMetaFwPrint(const char *format, ...);
int wrap_set_level(uint32_t level);
uint32_t vpu_profiling(void *inst);
void vpu_perf_summary(void);

#endif
