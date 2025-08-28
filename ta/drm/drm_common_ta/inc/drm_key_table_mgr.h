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

#ifndef __DRM_TAB_MGR_H__
#define __DRM_TAB_MGR_H__

#include "drm_tee_types.h"

/** drm_key_mgr_alloc
 * @param size  key size.
 * @retval index         on success
 * @retval -1            on failed
 */
INT32 drm_key_mgr_alloc(UINT32 size);


void drm_key_mgr_free(int start);

#endif
