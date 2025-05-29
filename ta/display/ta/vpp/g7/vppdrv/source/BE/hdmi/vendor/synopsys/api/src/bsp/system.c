// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

/* 
 * 	@note: this file should be re-written to match the environment the 
 * 	API is running on
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "bsp/system.h"
#include "util/log.h"
#include "util/error.h"

#ifndef CONFIG_AMP_TEE_ENABLE
/* in TEE env, fn pointers can't be stored in static struct as below, so call directly for now */

struct system_functions * snps_functions = 0;

void register_system_functions(struct system_functions * functions)
{
	LOGGER(SNPS_INFO,"System functions %s", functions->name);
	snps_functions = functions;
}


void snps_sleep(unsigned ms)
{
	snps_functions->sleep(ms);
}

#else
#include "hdmi_system.h"

void snps_sleep(unsigned ms)
{
	hdmitx_sleep(ms);
}

#endif


#ifdef __cplusplus
}
#endif
