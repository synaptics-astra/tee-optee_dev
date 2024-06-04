// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021, Synaptics Incorporated.
 */

#ifndef _MEM_TYPE_H_
#define _MEM_TYPE_H_

/*
 * memory type definitions
 *
 * note: kernel space must be secure. so 11xxb are invalid.
 */
#define MT_M_KERN		(0x08)
#define MT_M_NONSECURE		(0x04)
#define MT_M_TYPE		(0x03)

#define MT_USER(type)		((type))
#define MT_KERN(type)		((type) | MT_M_KERN)

#define MT_STRONGORDER		(0x00)	/* access in order */
#define MT_DEVICE		(0x01)	/* access in order */
#define MT_UNCACHED		(0x02)	/* access out-of-order */
#define MT_CACHED		(0x03)	/* access out-of-order */
#define MT_UNKNOWN		(0x0f)	/* unknown attribute */

#define MT_AP_X_RO       (0x10)    /* for excutable but not writeable */

/* memory types for end users */
#define MT_USER_STRONGORDER_S	MT_USER(MT_STRONGORDER)
#define MT_USER_DEVICE_S	MT_USER(MT_DEVICE)
#define MT_USER_UNCACHED_S	MT_USER(MT_UNCACHED)
#define MT_USER_CACHED_S	MT_USER(MT_CACHED)
#define MT_USER_CACHED_X_S	MT_USER(MT_CACHED | MT_AP_X_RO)
#define MT_USER_CACHED_RO_S	(MT_USER(MT_CACHED | MT_AP_X_RO) - 1)
#define MT_USER_STRONGORDER_NS	MT_USER(MT_M_NONSECURE | MT_STRONGORDER)
#define MT_USER_DEVICE_NS	MT_USER(MT_M_NONSECURE | MT_DEVICE)
#define MT_USER_UNCACHED_NS	MT_USER(MT_M_NONSECURE | MT_UNCACHED)
#define MT_USER_CACHED_NS	MT_USER(MT_M_NONSECURE | MT_CACHED)

#define MT_KERN_STRONGORDER	MT_KERN(MT_STRONGORDER)
#define MT_KERN_DEVICE		MT_KERN(MT_DEVICE)
#define MT_KERN_UNCACHED	MT_KERN(MT_UNCACHED)
#define MT_KERN_CACHED		MT_KERN(MT_CACHED)
#define MT_KERN_CACHED_X		MT_KERN(MT_CACHED | MT_AP_X_RO)
//to save space, use MT_AP_X_RO and -1 instead of define MT_AP_RO to 0x20
#define MT_KERN_CACHED_RO		(MT_KERN(MT_CACHED | MT_AP_X_RO) - 1)

/*not enable user mode*/
#define MT_KERN_STRONGORDER_NS	MT_KERN(MT_M_NONSECURE | MT_STRONGORDER)
#define MT_KERN_DEVICE_NS	MT_KERN(MT_M_NONSECURE | MT_DEVICE)
#define MT_KERN_UNCACHED_NS	MT_KERN(MT_M_NONSECURE | MT_UNCACHED)
#define MT_KERN_CACHED_NS	MT_KERN(MT_M_NONSECURE | MT_CACHED)

#endif /* _MEM_TYPE_H_ */
