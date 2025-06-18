// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "OSAL_api.h"
#include "vpp_module.h"
#include "hdmi_system.h"
#include "platform.h"
#include "hdmi_types.h"
#include "SNPS_HDMI_tx.h"

#define Hdmi_logd(mod, level, func, line, ...)  \
    do{     \
        char head[VPP_LOG_HSZ];         \
        if(VPP_log_head(mod, level, head, VPP_LOG_HSZ, func, line))         \
            {VPP_log(mod, level, head, __VA_ARGS__);}       \
    }while(0)

#define HDMI_DBG_INFO(func, line, ...) Hdmi_logd(VPP_LOG_HDMI, VPP_LOG_INFO, func, line, __VA_ARGS__)
#define HDMI_DBG_ERROR(func, line, ...) Hdmi_logd(VPP_LOG_HDMI, VPP_LOG_ERROR, func, line, __VA_ARGS__)

char * error_level_string(log_t level)
{
    switch(level){
        case SNPS_EMERG:  return "SNPS_EMERG";
        case SNPS_ALERT:  return "SNPS_ALERT";
        case SNPS_CRITICAL: return "SNPS_CRITICAL";
        case SNPS_ERROR:  return  "SNPS_ERROR";
        case SNPS_WARN:   return "SNPS_WARN";
        case SNPS_NOTICE: return "SNPS_NOTICE";
        case SNPS_INFO:   return "SNPS_INFO";
        case SNPS_DEBUG:  return "SNPS_DEBUG";
        case SNPS_TRACE:  return "SNPS_TRACE";
        case SNPS_REPORT:  return "";
    }
    return "SNPS_UNKWN";
}


/**
 * System Functions
 */
int hdmitx_logger(int level, const char *func, int line, const char *format, ...)
{
    va_list arg;
    char temp[500];
    int str_end = 0;

    /*
    // remove this as snps_hdmi is not visible in this
    if(snps_hdmi->verbose < level) {
        return 0;
    }
    */

    temp[0] = '\0';
    if(level < SNPS_WARN) {
        strcat(temp, error_level_string((log_t) level));
        strcat(temp, " > ");
        str_end = strlen(temp);
    }

    // Write message
    va_start(arg, format);
    vsnprintf(temp+str_end, 256, format, arg);
    va_end(arg);
    str_end = strlen(temp);
    temp[str_end] = '\n';
    temp[str_end+1] = '\0';

    if(level > SNPS_WARN) {	/* TODO: move this to logging based on log level set */
        HDMI_DBG_INFO(func, line, "%s", temp);
    } else {
        HDMI_DBG_ERROR(func, line, "%s", temp);
    }
    return 0;
}

void hdmitx_sleep(int ms){
    MV_OSAL_Task_Sleep(ms);
}
