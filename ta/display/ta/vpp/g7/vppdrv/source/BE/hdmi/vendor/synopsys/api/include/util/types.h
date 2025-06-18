// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef TYPES_H_
#define TYPES_H_

/*
 * @file: types
 * Define basic type optimized for use in the API so that it can be
 * platform-independent.
 */

#include <stdlib.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define BIT(n)		(1 << n)

typedef void (*handler_t) (void *);

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
	u8 *buffer;
	size_t size;
} buffer_t;

#endif	/* TYPES_H_ */
