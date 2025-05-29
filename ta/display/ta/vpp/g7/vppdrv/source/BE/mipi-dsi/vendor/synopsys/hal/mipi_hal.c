/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef MIPI_HAL_C_
#define MIPI_HAL_C_
#include "vpp_module.h"
#include "mipi_hal.h"

void mdelay(unsigned int ms)
{
    MV_OSAL_Task_Sleep(ms);
}

void udelay(unsigned int us)
{
    unsigned int ms;

    if (us > 1000)
        ms = (us+500)/1000;
    else
        ms = 1;
    MV_OSAL_Task_Sleep(ms);
    return;
}

void copy_from_user(void *dst, void *src, int size)
{
    if (dst != src)
        GaloisMemcpy(dst, src, size);
}

unsigned int ioread32(VPP_OBJ *vpp_obj, unsigned int addr)
{
    unsigned int data;

    GA_REG_WORD32_READ(addr, &data);
    GA_REG_WORD32_READ(addr, &data);
    return data;
}

void iowrite32(VPP_OBJ *vpp_obj, unsigned int addr, unsigned int data)
{
     //VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, addr, data);
     GA_REG_WORD32_WRITE(addr, data);
     return;
}

unsigned int module_base(int module)
{
    unsigned int base = 0;
    switch (module) {
        case MIPI_DSIH:
            base = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_MEMMAP_MIPI;
            break;
        case MIPI_DPHYTX:
            base = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_DPHYTX;
            break;
    }
    return base;
}
#endif    /* MIPI_HAL_C_ */
