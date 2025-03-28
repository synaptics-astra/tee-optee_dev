/* SPDX-License-Identifier: Proprietary */
/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2023 Synaptics Incorporated.
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

#ifndef __VPU_MACROS_H__
#define __VPU_MACROS_H__

#include <sys/types.h>

#ifndef ALIGN
#define IS_POWER2(x)    (((x) & ((x) - 1)) == 0)
#define ALIGN_MASK(x, mask)  (((x) + (mask)) & ~(mask))

/* align x to a, a must be power of 2 */
#define ALIGN2(x, a)    ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ALIGN(x, a)     ALIGN2(x, a)
#endif


#ifndef MAX
# define MAX(a, b)      (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
# define MIN(a, b)      (((a) < (b)) ? (a) : (b))
#endif

#ifndef UNUSED
# define UNUSED(x)  ((void)(x))
#endif

#ifndef N_ELEMENTS
# define N_ELEMENTS(x)      (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE     N_ELEMENTS
#endif

#endif
