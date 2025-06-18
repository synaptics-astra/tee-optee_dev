/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef MIPI_OSAL_H_
#define MIPI_OSAL_H_

#include "ctypes.h"
#include "mipi_debug.h"
#include "mipi_ctypes.h"
#include "vpp_api.h"
#include "vpp_module.h"

#define MIPI_RET(ret) MV_VPP_##ret

enum
{
    MIPI_DSIH = 0,
    MIPI_DPHYTX = 1
};

unsigned int module_base(int module);
void mdelay(unsigned int ms);
void iowrite32(VPP_OBJ *vpp_obj, unsigned int addr, unsigned int data);
unsigned int ioread32(VPP_OBJ *vpp_obj, unsigned int addr);
void udelay(unsigned int us);
#endif    /* MIPI_CTYPES_H_ */
