/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define _VBIBUF_C_

#include "com_type.h"
#include "OSAL_api.h"
#include "vbibuf.h"

/*********************************************************
 * FUNCTION: reset a vbi data buffer to be empty state
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 *
 ********************************************************/
void vbi_buf_reset(VBIBUF *vbi_buf)
{
    /* reset read/write pointer of vbi data buffer */
    /* the vbi data buffer is empty when read pointer equals write pointer */
    vbi_buf->reader = (unsigned int)vbi_buf->addr;
    vbi_buf->writer = vbi_buf->shadow_writer = (unsigned int)vbi_buf->addr;

    /* reset vbi buffer */
    if (vbi_buf->length != 0)
        GaloisMemSet(vbi_buf->addr, 0, vbi_buf->length);

    return;
}

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
int vbi_buf_write(VBIBUF *vbi_buf, unsigned char *vbi_data, int data_len)
{
    int diff;
    int count;

    diff = vbi_buf->reader - vbi_buf->shadow_writer;

    /*first check whether vbi buffer has enough space*/
    if (diff <= 0){
        if ((vbi_buf->length + diff) <= data_len)
            return 0;
    }else{
        if (diff <= data_len)
            return 0;
    }

    /*write data to vbi buffer*/
    for (count=0; count<data_len; count++){
        *((unsigned char *)vbi_buf->shadow_writer) = *(vbi_data + count);
        vbi_buf->shadow_writer ++;
        if (vbi_buf->shadow_writer == (unsigned int)vbi_buf->addr + vbi_buf->length)
            vbi_buf->shadow_writer = (unsigned int)vbi_buf->addr;
    }

    return 1;
}

/******************************************************************
 * FUNCTION: read vbi data from a vbi data buffer
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 *         *vbi_data - pointer to a buffer to get vbi data
 *         *data_len - vbi data length to read
 * RETURN: 1 - succeed
 *         0 - vbi data buffer is empty, no vbi data is available
 *****************************************************************/
int vbi_buf_read(VBIBUF *vbi_buf, unsigned char *vbi_data, int data_len)
{
    int count;

    /* to make it simple in ISR, if not empty it should have enough data */
    if (vbi_buf->writer == vbi_buf->reader)
        return 0;

    /*read data from vbi buffer*/
    for (count=0; count<data_len; count++){
        *(vbi_data + count) = *((unsigned char *)vbi_buf->reader);
        vbi_buf->reader ++;
        if (vbi_buf->reader == (unsigned int)vbi_buf->addr + vbi_buf->length)
            vbi_buf->reader = (unsigned int)vbi_buf->addr;
    }

    return 1;
}

/******************************************************************
 * FUNCTION: read a block of data from a vbi data buffer, if data is not enough, read all
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 *         *vbi_data - pointer to a buffer to get vbi data
 *         *block_size - vbi data block size
 * RETURN:
 *         data_len - vbi data length has been read
 *****************************************************************/
int vbi_buf_read_block(VBIBUF *vbi_buf, unsigned char *vbi_data, int block_size)
{
    int count;
    int data_len = block_size;

    /* get data length to be read */
    if ((vbi_buf->writer >= vbi_buf->reader) && (vbi_buf->writer - vbi_buf->reader) < block_size)
        data_len = vbi_buf->writer - vbi_buf->reader;

    if ((vbi_buf->writer < vbi_buf->reader) && (vbi_buf->length + vbi_buf->writer - vbi_buf->reader) < block_size)
        data_len = vbi_buf->length + vbi_buf->writer - vbi_buf->reader;

    /*read data from vbi buffer*/
    for (count=0; count<data_len; count++){
        *(vbi_data + count) = *((unsigned char *)vbi_buf->reader);
        vbi_buf->reader ++;
        if (vbi_buf->reader == (unsigned int)vbi_buf->addr + vbi_buf->length)
            vbi_buf->reader = (unsigned int)vbi_buf->addr;
    }

    return data_len;
}


/*********************************************************************
 * FUNCTION: commit previous vbi data written into a vbi buffer
 * PARAMS: *cmdq - pointer to a vbi data buffer
 *
 ********************************************************************/
void vbi_buf_commit(VBIBUF *vbi_buf)
{
    /* update write pointer with shadow write pointer */
    vbi_buf->writer = vbi_buf->shadow_writer;

    return;
}

/*********************************************************************
 * FUNCTION: remove previous vbi data written into a vbi buffer
 *           without commit.
 * PARAMS: *cmdq - pointer to a vbi data buffer
 *
 ********************************************************************/
void vbi_buf_uncommit(VBIBUF *vbi_buf)
{
    /* update shadow write pointer with write pointer */
    vbi_buf->shadow_writer = vbi_buf->writer;

    return;
}

/******************************************************************
 * FUNCTION: check whether VBI buffer is empty
 * PARAMS: *vbi_buf - pointer to a vbi data buffer
 * RETURN: 1 - vbi data buffer is empty
 *         0  - other wise
 *****************************************************************/
int vbi_buf_check_empty(VBIBUF *vbi_buf)
{
    if (vbi_buf->writer == vbi_buf->reader)
        return 1;
    else
        return 0;
}

