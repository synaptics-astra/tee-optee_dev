/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _BCMBUF_H_
#define _BCMBUF_H_

#if !defined(VPP_IN_TRUST_ZONE)
#include "OSAL_api.h"
#include "isl/amp_shm.h"
#else
#include "tee_ta_osal.h"

#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DHUB_CFGQ_T {
    AMP_SHM_HANDLE handle;
    INT32 shm_offset;
    INT *addr;
    INT len;
    UINT32 *phy_addr;
} DHUB_CFGQ;

/* structure of a buffer descriptor */
/* a buffer descriptor contains the pointers of the entire buffer and sub-buffers */
typedef struct BCMBUF_T {
    /*Note: Ensure the following order matches the order of hal_dhub.c:VIP_BCMBUF
     * 'writer' is updated inside hal_dhub.c:clear functions */
    unsigned int *head;          // head of total BCM buffer
    unsigned int *tail;          // tail of the buffer, used for checking wrap around
    unsigned int *writer;        // write pointer of queue, update with shadow_tail with commit
    int size;                    // size of total BCM buffer
    AMP_SHM_HANDLE handle;
    INT32 shm_offset;
    unsigned int *dv1_head;      // head of BCM sub-buffer used for CPCB0
    unsigned int *dv2_head;      // head of BCM sub-buffer used for CPCB1
    unsigned int *dv3_head;      // head of BCM sub-buffer used for CPCB2
    int subID;                   // sub-buffer ID currently in use
    UINT32 *phy_addr;
} BCMBUF;

/******* register programming buffer APIs **************/
/***************************************************************
 * FUNCTION: allocate register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 *       : size - size of the buffer to allocate
 *       :      - (should be a multiple of 4)
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int VPP_BCMBUF_Create(BCMBUF *pbcmbuf,int size);

/***************************************************************
 * FUNCTION: free register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int VPP_BCMBUF_Destroy(BCMBUF *pbcmbuf);

/***************************************************************
 * FUNCTION: reset a register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int VPP_BCMBUF_Reset(BCMBUF *pbcmbuf);

/*********************************************************
 * FUNCTION: selest BCM sub-buffer to use
 * PARAMS: *buf - pointer to the buffer descriptor
 *         subID - DV_1, DV_2, DV_3
 ********************************************************/
void VPP_BCMBUF_Select(BCMBUF *pbcmbuf, int subID);

/*********************************************************
 * FUNCTION: write register address (4bytes) and value (4bytes) to the buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               address - address of the register to be set
 *               value - the value to be written into the register
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
int VPP_BCMBUF_Write(BCMBUF *pbcmbuf, unsigned int address, unsigned int value);

/*********************************************************
 * FUNCTION: write a block of data to BCM buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               *pdata - pointer to the data
 *               length - the length of the data to be written to BCM buffer
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
int VPP_BCMBUF_WriteBlock(BCMBUF *pbcmbuf, unsigned int *pdata, unsigned int length);

/*********************************************************************
 * FUNCTION: do the hardware transmission
 * PARAMS: block - 0: return without waiting for transaction finishing
 *                 1: return after waiting for transaction finishing
 ********************************************************************/
void VPP_BCMBUF_HardwareTrans(BCMBUF *pbcmbuf, int block);

/*********************************************************************
 * FUNCTION: do the hardware transaction
 * PARAMS: pdata - pointer to the data block
 *               length - data length for transaction
 ********************************************************************/
void VPP_BCMBUF_HardwareTransBlock(const unsigned int *pdata, unsigned int length, int block);

/*******************************************************************************
 * FUNCTION: commit cfgQ which contains BCM DHUB programming info to interrupt service routine
 * PARAMS: *cfgQ - cfgQ
 *         cpcbID - cpcb ID which this cmdQ belongs to
 *         intrType - interrupt type which this cmdQ belongs to: 0 - VBI, 1 - VDE
 * NOTE: this API is only called from VBI/VDE ISR.
 *******************************************************************************/
int VPP_BCMDHUB_CFGQ_Commit(DHUB_CFGQ *cfgQ, int cpcbID, int intrType);

/*******************************************************************************
 * FUNCTION: commit cfgQ which contains DMA dhub programming info to VBI interrupt service routine
 * PARAMS: *cfgQ - cfgQ
 *         cpcbID - cpcb ID which this cmdQ belongs to
 * NOTE: this API is only called from VBI/VDE ISR.
 *******************************************************************************/
int VPP_DMADHUB_CFGQ_Commit(DHUB_CFGQ *cfgQ, int cpcbID);

/*********************************************************************
 * FUNCTION: send a BCM BUF info info to a BCM cfgQ
 * PARAMS: *pbcmbuf - pointer to the BCMBUF
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int VPP_BCMBUF_To_CFGQ(BCMBUF *pbcmbuf, DHUB_CFGQ *cfgQ);

/*********************************************************************
 * FUNCTION: send a raw BCM BUF info to a BCM cfgQ
 * PARAMS: pdata - pointer to the data block
 *         length - data length for transaction
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
void VPP_BCMBUF_Raw_To_CFGQ(const UINT32 *pdata, UINT32 length, DHUB_CFGQ *cfgQ);

/*********************************************************************
 * FUNCTION: send a BCM cfgQ info to a BCM cfgQ
 * PARAMS: src_cfgQ - pointer to the source BCM cfgQ
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
void VPP_CFGQ_To_CFGQ(DHUB_CFGQ *src_cfgQ, DHUB_CFGQ *cfgQ);

int VPP_BCMDHUB_AutoPush(int cpcbID, int intrType, int enable);

#ifdef __cplusplus
}
#endif

#endif

