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
#ifndef __VMETA_CONFIG_H__
#define __VMETA_CONFIG_H__

#define CONFIG_TA_LOG_ENABLE

#ifndef CONFIG_OPTEE
#define CONFIG_ANDROID_LOG
#endif

#define CONFIG_LOG_LOW

/* debug and profile log control */
//#define DYNAMIC_LOG_LEVEL
//#define ENABLE_VPU_PROFILING
//#define ENABLE_VPU_DETAIL_PROFILING

//#define ENABLE_VPU_DEBUG

#define ENABLE_TA_LOG

#endif /* __VMETA_CONFIG_H__ */
