/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_DEBUG_H_
#define _VPP_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpp_api_types.h"

typedef struct _tag_vpp_reg_dump_table {
    char *addr_name;
    UINT32 addr;
} VPP_REG_INFO;

#define VPP_DEBUG_REG_DUMP_ENABLE_VPP  1
#define VPP_DEBUG_REG_DUMP_ENABLE_MP   2
#define VPP_DEBUG_REG_DUMP_ENABLE_GFX  4
#define VPP_DEBUG_REG_DUMP_ENABLE_HDRX 8
#define VPP_DEBUG_REG_DUMP_ENABLE_MTR  0x10
#define VPP_DEBUG_REG_DUMP_ENABLE_ALL  0x1F

#ifdef CONFIG_VPP_FASTLOGO_DEBUG_ENABLE
//Eable ta log only if macro is defined in fastlogo.ta
#define CONFIG_TA_LOG_ENABLE
#endif

#ifndef CONFIG_VPP_FASTLOGO_SUPPORT
//Enable ta/android log by default in vpp ta
#define CONFIG_TA_LOG_ENABLE
#define CONFIG_ANDROID_LOG
#endif
#define CONFIG_LOG_ERROR
#define CONFIG_LOG_INFO

/*definition of vpp log level*/
typedef enum {
    VPP_LOG_FIELD,
    VPP_LOG_ERROR,              /**< For errors that system can still work. */
    VPP_LOG_INFO,               /**< Periodical and trivial debug info. */
    VPP_LOG_LEVEL_MAX = 3,      /**< MAX level should not exceed 7. */
} VPP_LOG_LEVEL;

/*definition of vpp log path*/
typedef enum {
    VPP_LOG_CONSOLE,            /**< Debug log output to console. */
    VPP_LOG_ANDROID,            /**<Debug log output to android*/
    VPP_LOG_FILE,               /**< Debug log output to file.*/
    VPP_LOG_PATH_MAX = 3,    /**< MAX path should not execeed 2. */
} VPP_LOG_PATH;

/*definition of vpp log module*/
typedef enum {
    VPP_LOG_API,
    VPP_LOG_HDMI,
    VPP_LOG_MIPI,
    VPP_LOG_DV,
    VPP_LOG_FACTORY,
    VPP_LOG_MODULE_MAX = 5,
    VPP_LOG_MODULE_ALL,
}VPP_LOG_MODULE;

/*definition of vpp log cmd*/
typedef enum{
    VPP_LOG_ENABLE,             /**enbale/disable VPP log system*/
    VPP_LOG_SET_OUTPATH,   /**set VPP log out path*/
    VPP_LOG_CMD_MAX = 2,
}VPP_LOG_CMD;

/* register dump type */
typedef enum{
    VPP_LOG_REG,
    VPP_LOG_MP_REG,
    VPP_LOG_GFX_VOP_REG,
    VPP_LOG_HDRX_PIPE_REG,
    VPP_LOG_MTR_REG,
}VPP_LOG_REG_DUMP_TYPE;

typedef struct __VPP_REG_INFO_GROUP__ {
    const VPP_REG_INFO *pRegInfo;
    UINT32 regBase;
    UINT32 regSize;
    char *prefix;
}VPP_REG_INFO_GROUP;

/*******************************************************************
 * FUNCTION: VPP  log init
 * INPUT: level - vpp log globle level
 *           path - vpp log output path
 * Note: this function has to be called before calling any vpp log api functions
 *******************************************************************/
void VPP_log_init(VPP_LOG_LEVEL level, VPP_LOG_PATH path);

/*******************************************************************
 * FUNCTION: VPP  log check head
 * INPUT: module - vpp log sub module id
 *            level - vpp log level
 *            head - the buffer to store the head
 *            size - max head size
 *            pfunction - the __FUNCTION__
 *            line - the __LINE__
 * RETURN: 1 - OK
 *               0 - FAIL
 * Note: this function has to be called before VPP_log
 *******************************************************************/
INT VPP_log_head(VPP_LOG_MODULE module, VPP_LOG_LEVEL level, char *head, int size, const char *pfunction, int line);

/*******************************************************************
 * FUNCTION: VPP  log function
 * INPUT: module - vpp log sub module id
 *           level - vpp log level
 *           phead - the log head
 *            msg - the msg that need to be output
 *            ...    - Parameter list for formating the message
 * Note: this function has to be called after the vpp log function initialized.
 *******************************************************************/
void VPP_log(VPP_LOG_MODULE module, VPP_LOG_LEVEL level, const char *phead, char *msg, ...);

/*******************************************************************
 * FUNCTION: VPP  log config set
 * INPUT: cmd - vpp log cmd
 *            Parm - command related
 *            module - VPP log module
 *            level - VPP log level
 * RETURN: MV_VPP_OK - SUCCEED
 *               MV_EBADPARAM - invalid parameters
 * Note: this function has to be called after the vpp log function initialized.
 *******************************************************************/
INT VPP_log_set(VPP_LOG_CMD cmd, UINT parm, VPP_LOG_MODULE module, VPP_LOG_LEVEL level);

#define VPP_LOG_HSZ 512

#if defined(VPP_DIAGS)
    do{             \
        printf(__VA_ARGS__);       \
    }while(0)
#else
#define VPP_logd(mod, level, ...)           \
    do{             \
            char head[VPP_LOG_HSZ];         \
            if(VPP_log_head(mod, level, head, VPP_LOG_HSZ, __FUNCTION__, __LINE__))         \
                {VPP_log(mod, level, head, __VA_ARGS__);}       \
    }while(0)
#endif

//The Loglevel to be used until logger is enabled
#define VPP_BOOTUP_LOGLEVEL    VPP_LOG_FIELD

#define VPPAPI_DBG_ERROR(...) VPP_logd(VPP_LOG_API, VPP_LOG_ERROR, __VA_ARGS__)

#define VPPAPI_DBG_INFO(...) VPP_logd(VPP_LOG_API, VPP_LOG_INFO, __VA_ARGS__)

/**perhaps this definition should not here*/
#define VPPHDMI_DBG_ERROR(...) VPP_logd(VPP_LOG_HDMI, VPP_LOG_ERROR, __VA_ARGS__)

#define VPPHDMI_DBG_INFO(...) VPP_logd(VPP_LOG_HDMI, VPP_LOG_INFO, __VA_ARGS__)

/**perhaps this definition should not here*/
#define VPPMIPI_DBG_ERROR(...) VPP_logd(VPP_LOG_MIPI, VPP_LOG_ERROR, __VA_ARGS__)

#define VPPMIPI_DBG_INFO(...) VPP_logd(VPP_LOG_MIPI, VPP_LOG_INFO, __VA_ARGS__)

#define VPPAPI_DBG_FIELD(...) VPP_logd(VPP_LOG_FACTORY, VPP_LOG_FIELD, __VA_ARGS__)

UINT32 VPP_DEBUG_DumpRegs(UINT32 base_addr, UINT32 sel);
void VPP_DEBUG_DumpRegs_Details(VPP_REG_INFO **pRegInfo, UINT32 *pSize);
UINT32 VPP_DEBUG_GetDumpSize(VPP_LOG_REG_DUMP_TYPE dump_type);

#ifdef __cplusplus
}
#endif

#endif

