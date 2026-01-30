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

/** Definition of logging API for TA.
 *
 * Purpose: This file defines a uniform macro for AMP TAs to output log in
 *          the same way as AMP does.
 *
 *     Version    Date                     Author
 *     V 1.01,    Feb 27 2014,             Yongsen Chen
 *        - change the copyright info to Marvell commercial notice.
 *        - use tab rather than space as the indent to make it consitent with
 *          other TA code.
 *        - use hex for bit fields
 *     V 1.00,    Feb 12 2014,             Jun Ma
 */

#ifndef __TA_LOG_H__
#define __TA_LOG_H__

/** Definition of TA debug log level
 */
typedef enum {
	TA_LOG_FATAL = 0,	/**< For errors that system can't continue. */
	TA_LOG_ERROR,		/**< For errors that system can still work. */
	TA_LOG_HIGH,		/**< High level debug info.                 */
	TA_LOG_MED,		/**< Medium level debug info.               */
	TA_LOG_LOW,		/**< Low level debug info.                  */
	TA_LOG_INFO,		/**< Periodical and trivial debug info.     */
	TA_LOG_USER1,		/**< User specific debug info.              */
	TA_LOG_USER2,		/**< User specific debug info.              */
	TA_LOG_LEVEL_MAX	/**< MAX level should not exceed 7.         */
} TA_LOG_LEVEL;


/*
 * Bellow bits are used to control which log to display.
 *
 * it's not to use simple log level is because sometimes the user may need log
 * FATAL and USER2 together.
 */
#ifdef CONFIG_LOG_FATAL
#define LOG_FATAL   0x01
#else
#define LOG_FATAL   0
#endif

#ifdef CONFIG_LOG_ERROR
#define LOG_ERROR   0x02
#else
#define LOG_ERROR   0
#endif

#ifdef CONFIG_LOG_HIGH
#define LOG_HIGH    0x04
#else
#define LOG_HIGH    0
#endif

#ifdef CONFIG_LOG_MED
#define LOG_MED     0x08
#else
#define LOG_MED     0
#endif

#ifdef CONFIG_LOG_LOW
#define LOG_LOW     0x10
#else
#define LOG_LOW     0
#endif

#ifdef CONFIG_LOG_INFO
#define LOG_INFO    0x20
#else
#define LOG_INFO    0
#endif

#ifdef CONFIG_LOG_USER1
#define LOG_USER1   0x40
#else
#define LOG_USER1   0
#endif

#ifdef CONFIG_LOG_USER2
#define LOG_USER2   0x80
#else
#define LOG_USER2   0
#endif

#define TA_LOG_MASK (LOG_FATAL | LOG_ERROR | LOG_HIGH | LOG_MED |	\
		LOG_LOW | LOG_INFO | LOG_USER1 | LOG_USER2)


#ifdef CONFIG_TA_LOG_ENABLE

#ifdef CONFIG_ANDROID_LOG

#ifndef LOG_TAG
#define LOG_TAG "AMP_TA"
#endif /* LOG_TAG */

#ifndef CONFIG_OPTEE
#include "cutils/log.h"

/*
 *  Table for TA log levels to ANDROID log levels conversion.
 */
static const unsigned char _tee_log_lev_map_tbl[] = {
	ANDROID_LOG_FATAL,		/**< TA_LOG_FATAL  */
	ANDROID_LOG_ERROR,		/**< TA_LOG_ERROR  */
	ANDROID_LOG_WARN,		/**< TA_LOG_HIGH   */
	ANDROID_LOG_INFO,		/**< TA_LOG_MED    */
	ANDROID_LOG_DEBUG,		/**< TA_LOG_LOW    */
	ANDROID_LOG_VERBOSE,		/**< TA_LOG_INFO   */
	ANDROID_LOG_VERBOSE,		/**< TA_LOG_USER1  */
	ANDROID_LOG_VERBOSE,		/**< TA_LOG_USER2  */
};

#define TA_LOG(_lvl, ...)						\
	do {								\
		if ((1 << _lvl) & TA_LOG_MASK) {			\
			__android_log_buf_print(LOG_ID_SYSTEM,		\
					_tee_log_lev_map_tbl[_lvl],	\
					LOG_TAG,			\
					__VA_ARGS__);			\
		}							\
	} while (0)
#else
#include <trace.h>
#define TA_LOG(_lvl, ...)						\
	do {								\
		if ((1 << _lvl) & TA_LOG_MASK) {			\
			EMSG(__VA_ARGS__);				\
		}							\
	} while (0)
#endif

#else /* CONFIG_ANDROID_LOG */
#include <stdio.h>

#define TA_LOG(_lvl, ...)						\
	do {								\
		if ((1 << _lvl) & TA_LOG_MASK) {			\
			printf(__VA_ARGS__);				\
		}							\
	} while (0)

#endif /* CONFIG_ANDROID_LOG */

#else /* CONFIG_TA_LOG_ENABLE */

#define TA_LOG(_lvl, ...)   do { } while (0)

#endif

#ifdef CONFIG_MV_AMP_DISABLE_TA_LOG
#define TA_LOGF(...)
#define TA_LOGE(...)
#define TA_LOGH(...)
#define TA_LOGM(...)
#define TA_LOGL(...)
#define TA_LOGI(...)
#define TA_LOGU1(...)
#define TA_LOGU2(...)
#else
#define TA_LOGF(...)        TA_LOG(TA_LOG_FATAL, __VA_ARGS__)
#define TA_LOGE(...)        TA_LOG(TA_LOG_ERROR, __VA_ARGS__)
#define TA_LOGH(...)        TA_LOG(TA_LOG_HIGH, __VA_ARGS__)
#define TA_LOGM(...)        TA_LOG(TA_LOG_MED, __VA_ARGS__)
#define TA_LOGL(...)        TA_LOG(TA_LOG_LOW, __VA_ARGS__)
#define TA_LOGI(...)        TA_LOG(TA_LOG_INFO, __VA_ARGS__)
#define TA_LOGU1(...)       TA_LOG(TA_LOG_USER1, __VA_ARGS__)
#define TA_LOGU2(...)       TA_LOG(TA_LOG_USER2, __VA_ARGS__)
#endif

#endif /* __TA_LOG_H__ */
