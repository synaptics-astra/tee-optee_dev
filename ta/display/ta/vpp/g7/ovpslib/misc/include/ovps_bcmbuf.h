/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _OVPS_BCMBUF_H_
#define _OVPS_BCMBUF_H_

#if !defined(VPP_IN_TRUST_ZONE)
#include "OSAL_api.h"
#include "isl/amp_shm.h"
#else
#include "tee_ta_osal.h"
#endif
#include "vpp_bcmbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/******* register programming buffer APIs **************/
/***************************************************************
 * FUNCTION: allocate register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 *       : size - size of the buffer to allocate
 *       :      - (should be a multiple of 4)
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int OVPS_BCMBUF_Create(BCMBUF *pbcmbuf,int size);

/***************************************************************
 * FUNCTION: free register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int OVPS_BCMBUF_Destroy(BCMBUF *pbcmbuf);

/***************************************************************
 * FUNCTION: reset a register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int OVPS_BCMBUF_Reset(BCMBUF *pbcmbuf);

/*********************************************************
 * FUNCTION: selest BCM sub-buffer to use
 * PARAMS: *buf - pointer to the buffer descriptor
 *         subID - DV_1, DV_2, DV_3
 ********************************************************/
void OVPS_BCMBUF_Select(BCMBUF *pbcmbuf, int subID);

/*********************************************************
 * FUNCTION: write register address (4bytes) and value (4bytes) to the buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               address - address of the register to be set
 *               value - the value to be written into the register
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
int OVPS_BCMBUF_Write(BCMBUF *pbcmbuf, unsigned int address, unsigned int value);

/*********************************************************
 * FUNCTION: write a block of data to BCM buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               *pdata - pointer to the data
 *               length - the length of the data to be written to BCM buffer
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
int OVPS_BCMBUF_WriteBlock(BCMBUF *pbcmbuf, UINT32 *pdata, UINT32 length);
/*******************************************************************************
 * FUNCTION: commit cfgQ which contains BCM DHUB programming info to interrupt service routine
 * PARAMS: *cfgQ - cfgQ
 *         cpcbID - cpcb ID which this cmdQ belongs to
 *         intrType - interrupt type which this cmdQ belongs to: 0 - VBI, 1 - VDE
 * NOTE: this API is only called from VBI/VDE ISR.
 *******************************************************************************/
int OVPS_BCMDHUB_CFGQ_Commit(DHUB_CFGQ *cfgQ, int cpcbID, int intrType);


/*********************************************************************
 * FUNCTION: send a BCM BUF info info to a BCM cfgQ
 * PARAMS: *pbcmbuf - pointer to the BCMBUF
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int OVPS_BCMBUF_To_CFGQ(BCMBUF *pbcmbuf, DHUB_CFGQ *cfgQ);


/*********************************************************************
 * FUNCTION: send a BCM cfgQ info to a BCM cfgQ
 * PARAMS: src_cfgQ - pointer to the source BCM cfgQ
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int OVPS_CFGQ_To_CFGQ(DHUB_CFGQ *src_cfgQ, DHUB_CFGQ *cfgQ);

#ifdef __cplusplus
}
#endif

#endif

