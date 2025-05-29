/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VBIBUF_H_
#define _VBIBUF_H_

#include "vpp_com.h"
#if !defined(VPP_IN_TRUST_ZONE)
#include "isl/amp_shm.h"
#else
#include "tee_ta_osal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VBIBUF_T {
    AMP_SHM_HANDLE handle;                //vbi data buffer handle
    unsigned char *addr;    // vbi data buffer start address
    unsigned int length;     //buffer length
    unsigned int reader;           // read pointer of buffer, update with read
    unsigned int writer;           // write pointer of buffer, update by shadow_writer with commit
    unsigned int shadow_writer;    //shadow write pointer of buffer, update with write
} VBIBUF;

/******* vbi buffer APIs ********/
/*********************************************************
 * FUNCTION: reset a vbi data buffer to be empty state
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 *
 ********************************************************/
void vbi_buf_reset(VBIBUF *vbi_buf);

/*********************************************************
 * FUNCTION: write vbi data to a vbi data buffer
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 *         *vbi_data - pointer to the vbi data
 *         *data_len - vbi data length to write
 * RETURN: 1 - succeed
 *         0 - vbi buffer has not enough space for the new data
 * NOTE: the data written into vbi buffer can not
 *       be read out until a commit is issued.
 ********************************************************/
int vbi_buf_write(VBIBUF *vbi_buf, unsigned char *vbi_data, int data_len);

/******************************************************************
 * FUNCTION: read vbi data from a vbi data buffer
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 *         *vbi_data - pointer to a buffer to get vbi data
 *         *data_len - vbi data length to read
 * RETURN: 1 - succeed
 *         0 - vbi data buffer is empty, no vbi data is available
 *****************************************************************/
int vbi_buf_read(VBIBUF *vbi_buf, unsigned char *vbi_data, int data_len);

/******************************************************************
 * FUNCTION: read a block of data from a vbi data buffer, if data is not enough, read all
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 *         *vbi_data - pointer to a buffer to get vbi data
 *         *block_size - vbi data block size
 * RETURN:
 *         data_len - vbi data length has been read
 *****************************************************************/
int vbi_buf_read_block(VBIBUF *vbi_buf, unsigned char *vbi_data, int block_size);

/*********************************************************************
 * FUNCTION: commit previous vbi data written into a vbi buffer
 * PARAMS: *cmdq - pointer to a vbi data buffer
 *
 ********************************************************************/
void vbi_buf_commit(VBIBUF *vbi_buf);

/*********************************************************************
 * FUNCTION: remove previous vbi data written into a vbi buffer
 *           without commit.
 * PARAMS: *cmdq - pointer to a vbi data buffer
 *
 ********************************************************************/
void vbi_buf_uncommit(VBIBUF *vbi_buf);

/******************************************************************
 * FUNCTION: check whether VBI buffer is empty
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 * RETURN: 1 - vbi data buffer is empty
 *         0  - other wise
 *****************************************************************/
int vbi_buf_check_empty(VBIBUF *vbi_buf);

#ifdef __cplusplus
}
#endif

#endif
