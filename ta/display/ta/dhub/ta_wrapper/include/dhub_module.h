/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _DHUB_MODULE_H_
#define _DHUB_MODULE_H_

#include "tee_ta_mmap_types.h"

typedef struct tagDHUB_OBJ {
    TEE_TA_REG_REGION dhubRegRegion;
}DHUB_OBJ;

#ifdef _TEE_TA_MMAP_DHUB_C_
DHUB_OBJ dhub_obj;
#else
extern DHUB_OBJ dhub_obj;
#endif

#endif // _DHUB_MODULE_H_
