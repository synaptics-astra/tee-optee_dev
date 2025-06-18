/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "vpp_debug.h"
#include "ta_log.h"
#include "vpp_module.h"

#ifdef ANDROID
static android_LogPriority level2prio[VPP_LOG_LEVEL_MAX] = {
    ANDROID_LOG_FATAL,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_WARN,
    ANDROID_LOG_INFO,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_VERBOSE
};
#endif

#ifdef CONFIG_AMP_TEE_ENABLE
static char g_VPP_LogModeName[VPP_LOG_MODULE_MAX][128] = {
    "API",
    "HDMI",
    "DSI",
};
#endif

/*******************************************************************
 * FUNCTION: VPP log path
 * INPUT: module - vpp log sub module id
 *        level - vpp log globle level
 * RETURN: VPP_LOG_PATH_MAX - Don't output log
 *         any other - output log to specified destination
 * Note: check/return the logger output path
 *******************************************************************/
static VPP_LOG_PATH VPP_log_getPath(VPP_LOG_MODULE module, VPP_LOG_LEVEL level)
{
    VPP_LOG_PATH outpath = VPP_LOG_PATH_MAX;

    if (g_vpp == NULL) {
        //Use VPP_BOOTUP_LOGLEVEL until VPP is initialized
        if (level <= VPP_BOOTUP_LOGLEVEL)
            outpath = VPP_LOG_CONSOLE;
    } else {
        VPP_LOGGER *p_vpp_debug = &(g_vpp->VPP_Debug);
        if (p_vpp_debug->enable) {
            //Once VPP is initialized, then check module and log level
            if ((module < VPP_LOG_MODULE_MAX) && (level <= p_vpp_debug->log_lev[module])) {
                outpath = p_vpp_debug->outpath;
            }
        } else if (level <= VPP_BOOTUP_LOGLEVEL) {
            //Use VPP_BOOTUP_LOGLEVEL until VPP is initialized
            outpath = VPP_LOG_CONSOLE;
        }
    }

    return outpath;
}

/*******************************************************************
 * FUNCTION: VPP  log init
 * INPUT: level - vpp log globle level
 *           path - vpp log output path
 * Note: this function has to be called before calling any vpp log api functions
 *******************************************************************/
void VPP_log_init(VPP_LOG_LEVEL level, VPP_LOG_PATH path)
{
    if (g_vpp != NULL) {
        VPP_LOGGER *p_vpp_debug = &(g_vpp->VPP_Debug);

        if (level < VPP_LOG_LEVEL_MAX) {
            for (int i = 0; i < VPP_LOG_MODULE_MAX; i++)
                p_vpp_debug->log_lev[i] = level;
        }
        if (path < VPP_LOG_PATH_MAX)
            p_vpp_debug->outpath = path;
        p_vpp_debug->enable = 1;
    } else {
        VPPAPI_DBG_FIELD("VPP logger not initialized - g_vpp = NULL\n");
    }
}
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
INT VPP_log_head(VPP_LOG_MODULE module, VPP_LOG_LEVEL level, char *head, int size, const char *pfunction, int line)
{
    if (VPP_log_getPath(module, level) != VPP_LOG_PATH_MAX) {
#ifdef CONFIG_AMP_TEE_ENABLE
        snprintf(head, size - 1, "VPP%d@%s@[%s():%d]:", level, g_VPP_LogModeName[module], pfunction, line);
#else
        snprintf(head, size - 1, "VPP%d@@[%s():%d]:", level, pfunction, line);
#endif
        return 1;
    }

    return 0;
}
/*******************************************************************
 * FUNCTION: VPP  log function
 * INPUT: module - vpp log sub module id
 *           level - vpp log level
 *           phead - the log head
 *            msg - the msg that need to be output
 *            ...    - Parameter list for formating the message
 * Note: this function has to be called after the vpp log function initialized.
 *******************************************************************/
void VPP_log(VPP_LOG_MODULE module, VPP_LOG_LEVEL level, const char *phead, CHAR *msg, ...)
{
    VPP_LOG_PATH outpath = VPP_log_getPath(module, level);

    if (outpath != VPP_LOG_PATH_MAX) {
        char buffer[VPP_LOG_HSZ];
        va_list args;

        va_start(args, msg);
        vsnprintf(buffer, VPP_LOG_HSZ - 1, msg, args);
        va_end(args);

        switch (outpath) {
            case VPP_LOG_CONSOLE:
                {
#ifdef CONFIG_AMP_TEE_ENABLE
                    TA_LOGI("%s%s", phead, buffer);
#else
                    ALOGD("%s%s", phead, buffer);
#endif //CONFIG_AMP_TEE_ENABLE
                }
                break;
            case VPP_LOG_ANDROID:
#ifdef ANDROID
                {
                    /*
                       level = MIN(level, VPP_LOG_LEVEL_MAX - 1);
                       __android_log_write(level2prio[level], phead, buffer);
                     */
                }
#endif /* ANDROID */
                break;
            case VPP_LOG_FILE:
                break;
            default:
                break;
        }
    }
}

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
INT VPP_log_set(VPP_LOG_CMD cmd, UINT parm, VPP_LOG_MODULE module, VPP_LOG_LEVEL level)
{
    int i;
    UINT ret = MV_VPP_EBADPARAM;
    VPP_LOGGER *p_vpp_debug = &(g_vpp->VPP_Debug);

    switch(cmd){
        case VPP_LOG_ENABLE:
            p_vpp_debug->enable = parm;
            if(level < VPP_LOG_LEVEL_MAX){
                if(module < VPP_LOG_MODULE_MAX){
                    p_vpp_debug->log_lev[module] = level;
                }else if(module == VPP_LOG_MODULE_ALL){
                    for(i = 0; i < VPP_LOG_MODULE_MAX; i++)
                        p_vpp_debug->log_lev[i] = level;
                }
            }
            ret = MV_VPP_OK;
            break;

        case VPP_LOG_SET_OUTPATH:
            if(parm < VPP_LOG_PATH_MAX){
                p_vpp_debug->outpath = parm;
                if(level < VPP_LOG_LEVEL_MAX){
                    if(module < VPP_LOG_MODULE_MAX){
                        p_vpp_debug->log_lev[module] = level;
                    }else if(module == VPP_LOG_MODULE_ALL){
                        for(i = 0; i < VPP_LOG_MODULE_MAX; i++)
                            p_vpp_debug->log_lev[i] = level;
                    }
                }
                ret = MV_VPP_OK;
            }
            break;
        default:
            break;
    }

    return ret;
}
