/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef _OVP_COMMON_H_
#define _OVP_COMMON_H_
#include "com_type.h"
#include "Galois_memmap.h"
#include "ovp.h"
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#ifndef CONFIG_AMP_TEE_ENABLE
#include "isl/amp_logger.h"
#endif
#include "tee_ta_mmap.h"
#include "tee_internal_api.h"
#ifdef CONFIG_OPTEE
#include "tee_api_types.h"
#include "tee_api_syna.h"
#endif

#define OVP_REG(x)	(MEMMAP_OVP_REG_BASE + (x))
#define MADI_REG(x)	(MEMMAP_OVP_REG_BASE + RA_OvpReg_MadiReg + (x))
#define FMD_REG(x)	(MEMMAP_OVP_REG_BASE + RA_OvpReg_FmdReg + (x))

#define OVP_LOG_HSZ 512

#define OVP_DEBUG

#if defined(OVP_DEBUG)
#ifdef CONFIG_AMP_TEE_ENABLE
#define CONFIG_TEE_LOG_ENABLE
#define CONFIG_TA_LOG_ENABLE
#define CONFIG_ANDROID_LOG
#define CONFIG_LOG_ERROR
#define CONFIG_LOG_INFO
#endif
#endif

#ifdef __MEMIO_SOCKET
#include <assert.h>
#include "trans_h.h"
#endif

#ifdef	__MEMIO_PCIE
#include <asm/io.h>
#include <asm/byteorder.h>
#endif
#include "galois_common.h"
#include "galois_cfg.h"

#if defined(CONFIG_AMP_TEE_ENABLE)
#ifndef CONFIG_OPTEE
    #define devmem_phy_to_virt(pPhyAddr)    TEE_PhysToVirt((void *)pPhyAddr)
#else
    #define devmem_phy_to_virt(pPhyAddr)    TEE_TA_RegPhyToVirt(pPhyAddr)
#endif
#endif

typedef enum {
	OVP_LOG_FATAL = 0,	    /**< For errors that system can not continue. */
	OVP_LOG_ERROR,		    /**< For errors that system can still work. */
	OVP_LOG_HIGH,		    /**< High level debug info. */
	OVP_LOG_MID,		    /**< Middle level debug info. */
	OVP_LOG_LOW,		    /**< Low level debug info. */
	OVP_LOG_INFO,		    /**< Periodical and trivial debug info. */
	OVP_LOG_USER1,		    /**< User specific debug info. */
	OVP_LOG_USER2,		    /**< User specific debug info. */
	OVP_LOG_LEVEL_MAX = 8,	    /**< MAX level should not exceed 7. */
} OVP_LOG_LEVEL;

/*definition of ovp log path*/
typedef enum {
	OVP_LOG_CONSOLE,	    /**< Debug log output to console. */
	OVP_LOG_ANDROID,	    /**<Debug log output to android*/
	OVP_LOG_FILE,		    /**< Debug log output to file.*/
	OVP_LOG_PATH_MAX = 3,	 /**< MAX path should not execeed 2. */
} OVP_LOG_PATH;

void OVP_log_init(OVP_LOG_LEVEL level, OVP_LOG_PATH path);
INT OVP_log_head( OVP_LOG_LEVEL level, char *head, int size, const char *pfunction, int line);
void OVP_log(OVP_LOG_LEVEL level, const char *phead, const char *msg, ...);

#define OVP_logd(level, ...)	       \
	do{		\
		char head[OVP_LOG_HSZ];		\
		if(OVP_log_head(level, head, OVP_LOG_HSZ, __FUNCTION__, __LINE__))	   \
		{OVP_log(level, head, __VA_ARGS__);}	   \
	}while(0)

#if defined(OVP_DEBUG)
#ifndef CONFIG_AMP_TEE_ENABLE
#define OVP_DBG_FATAL(...)	\
	AMP_Log(MODULE_DISPSRV, AMP_LOG_FATAL, __VA_ARGS__)
#define OVP_DBG_ERROR(...)	\
	AMP_Log(MODULE_DISPSRV, AMP_LOG_ERROR, __VA_ARGS__)
#define OVP_DBG_HIGH(...)	\
	AMP_Log(MODULE_DISPSRV, AMP_LOG_HIGH, __VA_ARGS__)
#define OVP_DBG_MID(...)	\
	AMP_Log(MODULE_DISPSRV, AMP_LOG_MID, __VA_ARGS__)
#define OVP_DBG_LOW(...)	\
	AMP_Log(MODULE_DISPSRV, AMP_LOG_LOW, __VA_ARGS__)
#define OVP_DBG_INFO(...)	\
	AMP_Log(MODULE_DISPSRV, AMP_LOG_INFO, __VA_ARGS__)
#else
#define OVP_DBG_FATAL(...)	\
	OVP_logd(OVP_LOG_FATAL, __VA_ARGS__)
#define OVP_DBG_ERROR(...)	\
	OVP_logd(OVP_LOG_ERROR, __VA_ARGS__)
#define OVP_DBG_HIGH(...)	\
	OVP_logd(OVP_LOG_HIGH, __VA_ARGS__)
#define OVP_DBG_MID(...)	\
	OVP_logd(OVP_LOG_MID, __VA_ARGS__)
#define OVP_DBG_LOW(...)	\
	OVP_logd(OVP_LOG_LOW, __VA_ARGS__)
#define OVP_DBG_INFO(...)	\
	OVP_logd(OVP_LOG_INFO, __VA_ARGS__)
#endif
#else

#define OVP_DBG_FATAL	printf
#define OVP_DBG_ERROR	printf
#define OVP_DBG_HIGH	printf
#define OVP_DBG_MID	printf
#define OVP_DBG_LOW	printf
#define OVP_DBG_INFO	printf

#endif
#endif
