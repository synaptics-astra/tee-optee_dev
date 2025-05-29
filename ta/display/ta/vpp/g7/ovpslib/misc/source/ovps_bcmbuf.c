/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#if defined(VPP_DEBUG)
volatile int dbg_vpp_max_bcmreg_num = 0;
volatile int dbg_cpcb1_max_bcmreg_num = 0;
volatile int dbg_cpcb2_max_bcmreg_num = 0;
volatile int dbg_cpcb3_max_bcmreg_num = 0;
#endif

#include "com_type.h"
#include "vpp_module.h"
#include "Galois_memmap.h"
#include "avio.h"
#include "vpp_bcm_bd.h"
#include "ovps_api.h"

extern int berlin_pe_dev;

//Enable or disable local BCM debug print
#ifdef VPP_BCM_BD_DBG_PRINT_BCM_ENABLE
#define VPP_BCM_BD_DBG_PRINT_LOCAL  VPP_BCM_BD_DBG_PRINT
#else
#define VPP_BCM_BD_DBG_PRINT_LOCAL(...)
#endif

/***************************************************************
 * FUNCTION: allocate register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 *       : size - size of the buffer to allocate
 *       :      - (should be a multiple of 4)
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
INT OVPS_BCMBUF_Create(BCMBUF *pbcmbuf, int size)
{
    if (size <= 0)
        return (MV_VPP_EBADPARAM);

    /* allocate memory for the buffer */
    if (VPP_SHM32_MALLOC(VPP_SHM_NONSECURE, size, &pbcmbuf->handle) != SUCCESS)
        return MV_VPP_ENOMEM;

    pbcmbuf->size = size;
    pbcmbuf->head = (UINT32*)VPP_SHM_GetVirtualAddress(pbcmbuf->handle, 0);
    pbcmbuf->phy_addr = (UINT32*)VPP_SHM_GetPhysicalAddress(pbcmbuf->handle, 0);

    VPP_BCM_BD_DBG_PRINT_LOCAL("VPP_BCMBUF_Create(handle=buff,size): %X = %X,%X\n",pbcmbuf, pbcmbuf->head, pbcmbuf->size);
#if defined(VPP_DIAGS) && PLATFORM!=ASIC
    //Enable BCM dump for FPGA & VELOCE platform only by default
    VPP_BCM_BD_INIT();
#endif
    return MV_VPP_OK;
}

/***************************************************************
 * FUNCTION: free register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
INT OVPS_BCMBUF_Destroy(BCMBUF *pbcmbuf)
{
    /* allocate memory for the buffer */
    if (!pbcmbuf->head)
        return (MV_VPP_EBADCALL);

    VPP_SHM_Free(pbcmbuf->handle);

    pbcmbuf->head = NULL;

    return MV_VPP_OK;
}

/***************************************************************
 * FUNCTION: reset a register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
INT OVPS_BCMBUF_Reset(BCMBUF *pbcmbuf)
{

#ifdef DISABLE_BCM_ENGINE
    return MV_VPP_OK;
#endif

    pbcmbuf->tail = pbcmbuf->head + (pbcmbuf->size/2);

    /*set pointers to the head*/
    pbcmbuf->writer = pbcmbuf->head;
    pbcmbuf->dv1_head = pbcmbuf->head;
    pbcmbuf->dv2_head = pbcmbuf->dv1_head + (pbcmbuf->size/4);
    pbcmbuf->dv3_head = pbcmbuf->dv2_head + (pbcmbuf->size/4);
    pbcmbuf->subID = -1; /* total */

    return MV_VPP_OK;
}

/*********************************************************
 * FUNCTION: Select sub register programming buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *         subID - CPCB_1, CPCB_2, CPCB_3 or total
 ********************************************************/
void OVPS_BCMBUF_Select(BCMBUF *pbcmbuf, INT subID)
{
#ifdef DISABLE_BCM_ENGINE
    return;
#endif
    /* reset read/write pointer of the buffer */
    if (subID == CPCB_1) {
        pbcmbuf->writer = pbcmbuf->dv1_head;
    } else {
        pbcmbuf->writer = pbcmbuf->head;
    }

    pbcmbuf->subID = subID;
    return;
}

/*********************************************************************
 * FUNCTION: send a BCM BUF info to a BCM cfgQ
 * PARAMS: *pbcmbuf - pointer to the BCMBUF
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int OVPS_BCMBUF_To_CFGQ(BCMBUF *pbcmbuf, DHUB_CFGQ *cfgQ)
{
    UINT32 *start, *phy_start;
    INT32 size, shm_offset;
#if defined(VPP_DEBUG)
    INT32 bcmreg_num;
#endif
    unsigned int bcm_sched_cmd[2];

#ifdef DISABLE_BCM_ENGINE
    return MV_VPP_OK;
#endif

    if (pbcmbuf->subID == CPCB_1) {
        start = pbcmbuf->dv1_head;
        shm_offset = pbcmbuf->dv1_head -pbcmbuf->head;

        phy_start = pbcmbuf->phy_addr;
    } else {
        start = pbcmbuf->head;
        shm_offset = 0;
        phy_start = pbcmbuf->phy_addr;
    }

    size = (int)pbcmbuf->writer-(int)start;
    ovps_printf(OVPS_INFO,"BCM SIZE :%d,start = %08X\n",size,start);

    if (size <= 0)
        return MV_VPP_EBADPARAM;

    shm_offset = shm_offset*4;
    VPP_SHM_CleanCache(pbcmbuf->handle, shm_offset,  size);

#if defined(VPP_DEBUG)
    bcmreg_num = size / 8;
    if (pbcmbuf->subID == CPCB_1) {
        if (dbg_cpcb1_max_bcmreg_num < bcmreg_num)
            dbg_cpcb1_max_bcmreg_num = bcmreg_num;
    } else {
        if (dbg_vpp_max_bcmreg_num < bcmreg_num)
            dbg_vpp_max_bcmreg_num = bcmreg_num;
    }
#endif

    VPP_BCM_BD_ADD_CFGQ(start, size);
    VPP_BCM_BD_DBG_PRINT_LOCAL("%s:%d -- addr:%x, size:%x\n",__FUNCTION__,__LINE__, (INT)phy_start, (INT)size);

    dhub_channel_generate_cmd(&(AG_dhubHandle.dhub), avioDhubChMap_aio64b_BCM_R, (INT)phy_start, (INT)size, 0, 0, 0, 1, ( INT * )bcm_sched_cmd);

    while ( !BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, (UINT32*)(cfgQ->addr + cfgQ->len*2)));
    cfgQ->len += 2;

    return MV_VPP_OK;
}

int OVPS_CFGQ_To_CFGQ(DHUB_CFGQ *src_cfgQ, DHUB_CFGQ *cfgQ)
{
    unsigned int bcm_sched_cmd[2];

#ifdef DISABLE_BCM_ENGINE
    return;
#endif

    if (src_cfgQ->len <= 0)
        return MV_VPP_EBADPARAM;

    VPP_SHM_CleanCache(src_cfgQ->handle, 0, src_cfgQ->len*8);

    VPP_BCM_BD_ADD_CFGQ(src_cfgQ->addr, src_cfgQ->len*8);
    VPP_BCM_BD_DBG_PRINT_LOCAL("%s:%d -- addr:%x, size:%x\n",__FUNCTION__,__LINE__, (INT)src_cfgQ->addr, (INT)src_cfgQ->len*8);

    ovps_printf(OVPS_INFO,"CONFIGQ SIZE :%d,start = %08X\n",src_cfgQ->len,src_cfgQ->addr);
    dhub_channel_generate_cmd(&(AG_dhubHandle.dhub), avioDhubChMap_aio64b_BCM_R, (INT)src_cfgQ->phy_addr, (INT)src_cfgQ->len*8, 0, 0, 0, 1, (INT *)bcm_sched_cmd);

    while ( !BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, (UINT32*)(cfgQ->addr + cfgQ->len*2)));
    cfgQ->len += 2;

    return MV_VPP_OK;
}

int OVPS_BCMDHUB_CFGQ_Commit(DHUB_CFGQ *cfgQ, int cpcbID, int intrType)
{
    unsigned int bcm_sched_cmd[2];
    int sched_qid = BCM_SCHED_Q12;

#ifdef DISABLE_BCM_ENGINE
    return;
#endif
    if (!cfgQ)
        return MV_VPP_EBADPARAM;

    if (cfgQ->len <= 0) {
        ovps_printf(OVPS_ERR, "OVP_BCMDHUB Invalid length q \n ");
        return MV_VPP_OK;
    } else {
        ovps_printf(OVPS_INFO,"OVP_BCMDHUB_CFGQ_Commi: 0x%0X:%d\n",cfgQ->addr,cfgQ->len);
    }

    VPP_SHM_CleanCache(cfgQ->handle, 0, cfgQ->len*8);

    VPP_BCM_BD_DBG_PRINT_LOCAL("%s:%d -- addr:%x, size:%x\n",__FUNCTION__,__LINE__,(INT)cfgQ->addr, (INT)cfgQ->len*8);
    VPP_BCM_BD_ADD_CFGQ(cfgQ->addr, cfgQ->len*8);

    dhub_channel_generate_cmd(&(AG_dhubHandle.dhub), avioDhubChMap_aio64b_BCM_R, (INT)cfgQ->phy_addr, (INT)cfgQ->len*8, 0, 0, 0, 1, ( INT * )bcm_sched_cmd);

    while ( !BCM_SCHED_PushCmd(sched_qid, bcm_sched_cmd, NULL));

    return MV_VPP_OK;
}


/*********************************************************
 * FUNCTION: write register address (4 bytes) and value (4 bytes) to the buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               address - address of the register to be set
 *               value - the value to be written into the register
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
INT OVPS_BCMBUF_Write(BCMBUF *pbcmbuf, UINT32 address, UINT32 value)
{
    UINT32 *end;

    VPP_BCM_BD_ADD_BCM(address,value);

#ifdef DISABLE_BCM_ENGINE
    GA_REG_WORD32_WRITE(address, value);
    return MV_VPP_OK;
#endif

    /*if not enough space for storing another 8 bytes, wrap around happens*/
    if (pbcmbuf->subID == CPCB_1)
        end = pbcmbuf->dv2_head;
    else
        end = pbcmbuf->tail;

    if (pbcmbuf->writer == end) {
        /*the buffer is full, no space for wrap around*/
        return MV_VPP_EBCMBUFFULL;
    }

    /*save the data to the buffer*/
    *pbcmbuf->writer = value;
    pbcmbuf->writer ++;
    *pbcmbuf->writer = address;
    pbcmbuf->writer ++;

    return MV_VPP_OK;
}

/*********************************************************
 * FUNCTION: write a block of data to BCM buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               *pdata - pointer to the data
 *               length - the length of the data to be written to BCM buffer
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
INT OVPS_BCMBUF_WriteBlock(BCMBUF *pbcmbuf, UINT32 *pdata, UINT32 length)
{
    UINT32 *end;

#ifdef DISABLE_BCM_ENGINE
    return MV_VPP_OK;
#endif

    if (pbcmbuf->subID == CPCB_1)
        end = pbcmbuf->dv2_head;
    else
        end = pbcmbuf->tail;

    if (pbcmbuf->writer > end-(length >> 2)) {
        /*the buffer is full*/
        return MV_VPP_EBCMBUFFULL;
    }

    /*save the data to BCM buffer*/
    GaloisMemcpy(pbcmbuf->writer, pdata, length);

    pbcmbuf->writer += (length >> 2);

    return MV_VPP_OK;
}
