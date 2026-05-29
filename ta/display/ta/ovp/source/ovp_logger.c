/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

/************* Header file include ********************************************/
#include <stdio.h>
#include "ovp_common.h"
#include "ovp_module.h"
#ifdef CONFIG_AMP_TEE_ENABLE
#include "ta_log.h"
#endif
#include "ovp_debug.h"

typedef struct tagOVP_LOGGER
{
	UINT	g_enable;
	OVP_LOG_LEVEL	log_lev;
	OVP_LOG_PATH	outpath;
} OVP_LOGGER;

static OVP_LOGGER g_OVP_Debug ={0, 0, OVP_LOG_CONSOLE};

/*******************************************************************
 * FUNCTION: OVP  log init
 * INPUT: level - ovp log globle level
 *	     path - ovp log output path
 * Note: this function has to be called before calling any ovp log api functions
 *******************************************************************/
void OVP_log_init(OVP_LOG_LEVEL level, OVP_LOG_PATH path)
{
	if(level < OVP_LOG_LEVEL_MAX)
		g_OVP_Debug.log_lev = level;

	if(path < OVP_LOG_PATH_MAX)
		g_OVP_Debug.outpath = path;

	g_OVP_Debug.g_enable = 1;
}


/*******************************************************************
 * FUNCTION: OVP  log check head
 * INPUT: module - ovp log sub module id
 *	      level - ovp log level
 *	      head - the buffer to store the head
 *	      size - max head size
 *	      pfunction - the __FUNCTION__
 *	      line - the __LINE__
 * RETURN: 1 - OK
 *		 0 - FAIL
 * Note: this function has to be called before OVP_log
 *******************************************************************/
INT OVP_log_head(OVP_LOG_LEVEL level, char *head, int size, const char *pfunction, int line)
{
	OVP_LOGGER *p_ovp_debug = &g_OVP_Debug;
	if((p_ovp_debug->g_enable) && (level <= p_ovp_debug->log_lev)) {
		snprintf(head, size - 1, "OVPD%d@[%s():%d]:", level, pfunction, line);
		return 1;
	}
	return 0;
}
/*******************************************************************
 * FUNCTION: OVP  log function
 * INPUT: module - ovp log sub module id
 *	     level - ovp log level
 *	     phead - the log head
 *	      msg - the msg that need to be output
 *	      ...    - Parameter list for formating the message
 * Note: this function has to be called after the ovp log function initialized.
 *******************************************************************/

void OVP_log(OVP_LOG_LEVEL level, const char *phead, const char *msg, ...)
{
	char buffer[OVP_LOG_HSZ];
	OVP_LOGGER *p_ovp_debug = &g_OVP_Debug;

	va_list args;
	va_start(args, msg);
	vsnprintf(buffer, OVP_LOG_HSZ - 1, msg, args);
	va_end(args);

	switch(p_ovp_debug->outpath) {
	case OVP_LOG_CONSOLE:
		{
#ifdef CONFIG_AMP_TEE_ENABLE
			TA_LOGI("%s%s", phead, buffer);
#else
			/* printf("%s%s", phead, buffer); */
#endif
		}
		break;
	case OVP_LOG_ANDROID:
		break;
	case OVP_LOG_FILE:
		break;
	default:
		break;
	}
}
