// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef LOG_H_
#define LOG_H_

#include "bsp/system.h"
#include "hdmitx_dev.h"

/**
 * @file
 *	The log  offers a logging utility .
 *	It masks the I/O and logging of the OS, and enables verbose logging in debug mode.
 */
#ifdef __cplusplus
extern "C" {
#endif

#define LOG_TRACE()
#define LOG_TRACE1(a)
#define LOG_TRACE2(a,b)
#define LOG_TRACE3(a,b,c)

#ifndef CONFIG_AMP_TEE_ENABLE

#define LOGGER(level, format, ...)  \
	do { if (snps_functions && snps_functions->logger) snps_functions->logger(level, format, ##__VA_ARGS__); } while (0)

#else

#include "hdmi_system.h"	//LOGGER defined within this

#endif	//CONFIG_AMP_TEE_ENABLE

#ifdef __cplusplus
}
#endif
#endif	/* LOG_H_ */
