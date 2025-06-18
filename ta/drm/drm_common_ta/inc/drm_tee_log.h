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

#ifndef __DRM_COMMON_LOG_H__
#define __DRM_COMMON_LOG_H__

#include <stdio.h>
#include "drm_tee_types.h"

//#define DRM_LOG_CONFIG_ENBABLE
//#define DRM_LOG_DEBUG_ENBABLE


#ifdef DRM_LOG_CONFIG_ENBABLE
#ifdef DRM_LOG_DEBUG_ENBABLE
#define DEBUG
#define DRM_LOG printf
#else
#define DRM_LOG(...)
#endif
#define DRM_ERR printf

void DRM_HEXDUMP(unsigned char * pname, unsigned char *pbuf, UINT32 len);

#define DRM_ASSERT(x)                                  \
    do {                                               \
        if (!(x)) {                                    \
            printf("assert fail at "#x " %d/%s()\n",   \
                     __LINE__, __FUNCTION__);          \
            while(1);                                  \
        }                                              \
    }while(0)

#else
#define DRM_LOG(...)
#define DRM_ERR(...)
#define DRM_ASSERT(...)
#define DRM_HEXDUMP(...)
#endif

#define API_TRACE_DEBUG
#ifdef API_TRACE_DEBUG
#define API_ENTER_TRACE()       DRM_LOG("[DRM_API][Enter] %s(), %d\n", __FUNCTION__, __LINE__)
#define API_EXIT_TRACE(ret)     do { \
                                    if((ret != TEE_SUCCESS) && (ret != (-29))) { \
                                        DRM_ERR("[DRM_API][Exit] %s, %d (ret=%d)\n", __FUNCTION__, __LINE__, (int)ret); \
                                    } \
                                    else { \
                                        DRM_LOG("[DRM_API][Exit] %s, %d (ret=%d)\n", __FUNCTION__, __LINE__, (int)ret); \
                                    } \
                                } while(0)
#else
#define API_ENTER_TRACE()
#define API_EXIT_TRACE(ret)
#endif

#endif  // #ifndef __DRM_COMMON_LOG_H__
