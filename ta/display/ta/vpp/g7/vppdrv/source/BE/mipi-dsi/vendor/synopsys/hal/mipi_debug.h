/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef MIPI_DEBUG_H_
#define MIPI_DEBUG_H_
#include "vpp_debug.h"

enum {
    MIPI_ERROR = 0,
    MIPI_WARNING = 1,
    MIPI_INFO = 2,
    MIPI_DEBUG = 3,
    MIPI_DBG_MAX
};
#define mipi_dbg_print(error, ...) \
{\
        if(error == MIPI_ERROR){ \
            VPPMIPI_DBG_ERROR(__VA_ARGS__);\
        }else{ \
            VPPMIPI_DBG_INFO(__VA_ARGS__);\
        }\
};

#endif	/* MIPI_DSIH_OSAL_H_ */
