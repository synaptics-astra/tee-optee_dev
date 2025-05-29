/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_TOOL_CHAIN_H_
#define _VPP_TOOL_CHAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

int vprintf(const char *format, va_list arg);
int vfprintf(void *dbghfp, const char *format, va_list arg);

#ifdef __cplusplus
}
#endif

#endif //_VPP_TOOL_CHAIN_H_
