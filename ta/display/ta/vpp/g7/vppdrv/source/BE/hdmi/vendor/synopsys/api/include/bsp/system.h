// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

/* 
 * 	@note: this file should be re-written to match the environment the 
 * 	API is running on
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "util/types.h"
#ifdef __cplusplus
extern "C" {
#endif

extern struct system_functions * snps_functions;


struct system_functions {
	char name[30];

	int  (*logger) (int level, const char *format, ...);
	void (*sleep)  (int ms);
};

typedef enum {
	RX_INT = 1, TX_WAKEUP_INT, TX_INT
} interrupt_id_t;


void register_system_functions(struct system_functions * functions);

void snps_sleep(unsigned ms);


#ifdef __cplusplus
}
#endif
#endif				/* SYSTEM_H_ */
