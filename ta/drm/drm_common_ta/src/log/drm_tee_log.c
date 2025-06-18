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


#include "drm_tee_log.h"

#ifdef DRM_LOG_CONFIG_ENBABLE

void DRM_HEXDUMP(unsigned char* pname, unsigned char* pbuf, UINT32 len)
{
    UINT32 i = 0;

    if (pname) {
        DRM_LOG("%s(%d)", pname, len);
    }

    for (i = 0; i < len; i++) {
        if ((i%16)==0) {
             DRM_LOG("\n");
        }
        DRM_LOG("0x%02x ", pbuf[i]);
    }
    DRM_LOG("\n");
}
#endif
