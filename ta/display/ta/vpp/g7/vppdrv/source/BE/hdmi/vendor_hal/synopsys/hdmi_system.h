// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef INCLUDE_APP_SYSTEM_H_
#define INCLUDE_APP_SYSTEM_H_

#define LOG_TRACE()
#define LOG_TRACE1(a)
#define LOG_TRACE2(a,b)
#define LOG_TRACE3(a,b,c)

#define LOGGER(level, format, ...)  \
	do { if (1) hdmitx_logger(level, __func__, __LINE__, format, ##__VA_ARGS__); } while (0)

/**
 * From system.h we need:
 *  - A logger function
 *  - A print function
 *  - A sleep function
 */
int  hdmitx_logger(int level, const char *func, int line, const char *format, ...);
void hdmitx_sleep(int ms);

#endif /* INCLUDE_APP_SYSTEM_H_ */
