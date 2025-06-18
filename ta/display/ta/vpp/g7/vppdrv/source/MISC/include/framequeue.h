/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _FRAMEQUEUE_H_
#define _FRAMEQUEUE_H_

#include "vpp_com.h"
#ifndef VPP_IN_TRUSTZONE
#include "OSAL_api.h"
#else
#include "OSAL_api.h"
#include "tee_ta_osal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* structure for frame descriptor queue */
typedef struct FRAMEQUEUE_T{
    void *frame_descrs[MAX_NUM_FRAMES];
    int shadow_head;    // shadow read pointer of queue, update with pop
    int head;           // read pointer of queue, update with pop_commit
    int tail;           // write pointer of queue, update with push
    MV_OSAL_HANDLE_MUTEX_t frmq_lock;
} FRAMEQUEUE;


/* frame queue APIs */
/*************************************************
 * FUNCTION: reset frame queue to be empty state
 * PARAMS: *frmq - pionter to a frame queue
 *
 ************************************************/
void frmq_reset(FRAMEQUEUE *frmq);

/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *frmq - pointer to frame queue
 *         *frm_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
int frmq_push(FRAMEQUEUE *frmq, void *frm_descr);

/******************************************************************
 * FUNCTION: pop a frame descriptor out of a frame queue
 *           but actually without update head pointer.
 * PARAMS: *frmq - pointer to a frame queue
 *         **frm_descr - pointer to the frame descriptor
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int frmq_pop(FRAMEQUEUE *frmq, void **frm_descr);

/******************************************************************
 * FUNCTION: commit previous pop operation
 *           by actually update head pointer.
 * PARAMS: *frmq - pointer to a frame queue
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 *****************************************************************/
int frmq_pop_commit(FRAMEQUEUE *frmq);

/******************************************************************
 * FUNCTION: return status of frame queue
 * PARAMS: *frmq - pointer to a frame queue
 * RETURN: 1 - queue is empty
 *         0 - queue is not empty
 *****************************************************************/
int frmq_empty(FRAMEQUEUE *frmq);

/******************************************************************
 * FUNCTION: return number of elements in the queue
 * PARAMS: *frmq - pointer to a frame queue
 * RETURN: 0 - queue is empty
 *         n - number of elements in queue
 *****************************************************************/
int frmq_size(FRAMEQUEUE *frmq);

#ifdef __cplusplus
}
#endif

#endif

