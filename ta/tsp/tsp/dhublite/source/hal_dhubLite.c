/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

/******************************************************************************
*                 2018 Synaptics Incorporated. All Rights Reserved            *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF Synaptics.                   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT *
* OF Synaptics OR ANY THIRD PARTY. Synaptics RESERVES THE RIGHT AT ITS SOLE   *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO Synaptics.  *
* THIS CODE IS PROVIDED "AS IS". Synaptics MAKES NO WARRANTIES, EXPRESSED,    *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.  *
*                                                                             *
*                   WARNING FOR USING THIS DIAG CODE                          *
*   1. Please take your own responsibility if you refer DIAG source code for  *
*      any type software development.                                         *
*   2. DIAG code is mainly for internal validation purpose, and not intend to *
*      be used in production SW.                                              *
*   3. DIAG code is subject to change without notice.                         *
*******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
//! \file hal_dhub.c
//!
//! \brief DHUB External source file
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    January 14 2014,        Wenjie Zhou
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
//#define    __CUSTOMIZED_IO__
#include	"api_dhubLite.h"
#include	"dHubLite.h"
//#include    "cbase.h"
//#include    "drv_io.h"
//#include    "drv_tsp_io.h"
//#include    "drv_figo_api.h"
#include "tsp_mem_map.h"

#define DRV_IO_RD321(a_)                 ((*((volatile UNSG32 *)tspmem_phy_to_virt(a_))))
#define DRV_IO_WR321(a_,d_)              (*((volatile UNSG32 *)tspmem_phy_to_virt(a_)) = (UNSG32)(d_))

#define IO32WR(d, a)        DRV_IO_WR321((a),(d))
#define IO32RD(d, a)        do { (d) = DRV_IO_RD321((a)); } while(0)
#define ModInc(x, i, mod)           do{ (x) += (i); while((x) >= (mod)) (x) -= (mod); }while(0)

#define IO32CFG(cfgQ, i, a, d)  do{ if(cfgQ) { (cfgQ)[i][0] = (d); (cfgQ)[i][1] = (a); } \
                                            else IO32WR(d, a); \
                                            (i) ++; \
                                            }while(0)
#define	bTST(x, b)		(((x) >> (b)) & 1)

//#include  "drv_io.h"
// comment out the following line to disable debugging output
//#define DHUB_DBG

#ifdef DHUB_DBG
#define xdbg printf
#else
#define xdbg 1?0:
#endif

/**	SECTION - API definitions for $SemaHub
 */
    /******************************************************************************************************************
	*	Function: semaphore_lite_hdl
	*	Description: Initialize HDL_semaphore with a $SemaHub BIU instance.
	******************************************************************************************************************/
void semaphore_lite_hdl(UNSG32 ra,   /*! Base address of a BIU instance of $SemaHub ! */
                   void *hdl    /*! Handle to HDL_semaphore ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;

    sem->ra = ra;
    /**	ENDOFFUNCTION: semaphore_lite_hdl **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_cfg
	*	Description: Configurate a semaphore's depth & reset pointers.
	*	Return:			UNSG32						-	Number of (adr,pair) added to cfgQ
	******************************************************************************************************************/
UNSG32 semaphore_lite_cfg(void *hdl, /*! Handle to HDL_semaphore ! */
                     SIGN32 id, /*! Semaphore ID in $SemaHub ! */
                     SIGN32 depth,  /*! Semaphore (virtual FIFO) depth ! */
                     T64b cfgQ[]    /*! Pass NULL to directly init SemaHub, or
                                       Pass non-zero to receive programming sequence
                                       in (adr,data) pairs
                                       ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;

    T32SemaphoreLite_CFG cfg;
    UNSG32 i = 0, a;
    a = sem->ra + RA_SemaHubLite_ARR + id * sizeof (SIE_SemaphoreLite);

    cfg.u32 = 0;
    cfg.uCFG_DEPTH = sem->depth[id] = depth;
    IO32CFG(cfgQ, i, a + RA_SemaphoreLite_CFG, cfg.u32);

    return i;
    /**	ENDOFFUNCTION: semaphore_lite_cfg **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_intr_enable
	*	Description: Configurate interrupt enable bits of a semaphore.
	******************************************************************************************************************/
void semaphore_lite_intr_enable(void *hdl,   /*! Handle to HDL_semaphore ! */
                           SIGN32 id,   /*! Semaphore ID in $SemaHub ! */
                           SIGN32 empty,    /*! Interrupt enable for CPU at condition 'empty' ! */
                           SIGN32 full, /*! Interrupt enable for CPU at condition 'full' ! */
                           SIGN32 almostEmpty,  /*! Interrupt enable for CPU at condition 'almostEmpty' ! */
                           SIGN32 almostFull,   /*! Interrupt enable for CPU at condition 'almostFull' ! */
                           SIGN32 cpu   /*! CPU ID (0/1/2) ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;
    T32SemaINTRLite_mask mask;
    UNSG32 a;
    a = sem->ra + RA_SemaHubLite_ARR + id * sizeof (SIE_SemaphoreLite);

    mask.u32 = 0;
    mask.umask_empty = empty;
    mask.umask_full = full;
    mask.umask_almostEmpty = almostEmpty;
    mask.umask_almostFull = almostFull;

    IO32WR(mask.u32, a + RA_SemaphoreLite_INTR + cpu * sizeof (SIE_SemaINTRLite));
    /**	ENDOFFUNCTION: semaphore_lite_intr_enable **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_query
	*	Description: Query current status (counter & pointer) of a semaphore.
	*	Return:			UNSG32						-	Current available unit level
	******************************************************************************************************************/
UNSG32 semaphore_lite_query(void *hdl,   /*! Handle to HDL_semaphore ! */
                       SIGN32 id,   /*! Semaphore ID in $SemaHub ! */
                       SIGN32 master,   /*! 0/1 as procuder/consumer query ! */
                       UNSG32 * ptr /*! Non-zero to receive semaphore r/w pointer ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;
    T32SemaQueryLite_RESP resp;
    T32SemaQueryMapLite_ADDR map;

    map.u32 = 0;
    map.uADDR_ID = id;
    map.uADDR_master = master;
    IO32RD(resp.u32, sem->ra + RA_SemaHubLite_Query + map.u32);

    if (ptr)
        *ptr = resp.uRESP_PTR;
    return resp.uRESP_CNT;
    /**	ENDOFFUNCTION: semaphore_lite_query **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_push
	*	Description: Producer semaphore push.
	******************************************************************************************************************/
void semaphore_lite_push(void *hdl,  /*! Handle to HDL_semaphore ! */
                    SIGN32 id,  /*! Semaphore ID in $SemaHub ! */
                    SIGN32 delta    /*! Delta to push as a producer ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;
    T32SemaHubLite_PUSH push;

    push.u32 = 0;
    push.uPUSH_ID = id;
    push.uPUSH_delta = delta;
    IO32WR(push.u32, sem->ra + RA_SemaHubLite_PUSH);
    /**	ENDOFFUNCTION: semaphore_lite_push **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_push
	*	Description: Consumer semaphore pop.
	******************************************************************************************************************/
void semaphore_lite_pop(void *hdl,   /*! Handle to HDL_semaphore ! */
                   SIGN32 id,   /*! Semaphore ID in $SemaHub ! */
                   SIGN32 delta /*! Delta to pop as a consumer ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;
    T32SemaHubLite_POP pop;

    pop.u32 = 0;
    pop.uPOP_ID = id;
    pop.uPOP_delta = delta;
    IO32WR(pop.u32, sem->ra + RA_SemaHubLite_POP);
    /**	ENDOFFUNCTION: semaphore_lite_pop **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_chk_empty
	*	Description: Check 'empty' status of a semaphore (or all semaphores).
	*	Return:			UNSG32						-	status bit of given semaphore, or
	*													status bits of all semaphores if id==-1
	******************************************************************************************************************/
UNSG32 semaphore_lite_chk_empty(void *hdl,   /*! Handle to HDL_semaphore ! */
                           SIGN32 id    /*! Semaphore ID in $SemaHub
                                           -1 to return all 32b of the interrupt status
                                           ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;
    UNSG32 d;

    IO32RD(d, sem->ra + RA_SemaHubLite_empty);
    return (id < 0) ? d : bTST(d, id);
    /**	ENDOFFUNCTION: semaphore_lite_chk_empty **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_chk_full
	*	Description: Check 'full' status of a semaphore (or all semaphores).
	*	Return:			UNSG32						-	status bit of given semaphore, or
	*													status bits of all semaphores if id==-1
	******************************************************************************************************************/
UNSG32 semaphore_lite_chk_full(void *hdl,    /*! Handle to HDL_semaphore ! */
                          SIGN32 id /*! Semaphore ID in $SemaHub
                                       -1 to return all 32b of the interrupt status
                                       ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;
    UNSG32 d;

    IO32RD(d, sem->ra + RA_SemaHubLite_full);
    return (id < 0) ? d : bTST(d, id);
    /**	ENDOFFUNCTION: semaphore_lite_chk_full **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_chk_almostEmpty
	*	Description: Check 'almostEmpty' status of a semaphore (or all semaphores).
	*	Return:			UNSG32						-	status bit of given semaphore, or
	*													status bits of all semaphores if id==-1
	******************************************************************************************************************/
UNSG32 semaphore_lite_chk_almostEmpty(void *hdl, /*! Handle to HDL_semaphore ! */
                                 SIGN32 id  /*! Semaphore ID in $SemaHub
                                               -1 to return all 32b of the interrupt status
                                               ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;
    UNSG32 d;

    IO32RD(d, sem->ra + RA_SemaHubLite_almostEmpty);
    return (id < 0) ? d : bTST(d, id);
    /**	ENDOFFUNCTION: semaphore_lite_chk_almostEmpty **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_chk_almostFull
	*	Description: Check 'almostFull' status of a semaphore (or all semaphores).
	*	Return:			UNSG32						-	status bit of given semaphore, or
	*													status bits of all semaphores if id==-1
	******************************************************************************************************************/
UNSG32 semaphore_lite_chk_almostFull(void *hdl,  /*! Handle to HDL_semaphore ! */
                                SIGN32 id   /*! Semaphore ID in $SemaHub
                                               -1 to return all 32b of the interrupt status
                                               ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;
    UNSG32 d;

    IO32RD(d, sem->ra + RA_SemaHubLite_almostFull);
    return (id < 0) ? d : bTST(d, id);
    /**	ENDOFFUNCTION: semaphore_lite_chk_almostFull **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_clr_empty
	*	Description: Clear 'empty' status of a semaphore.
	******************************************************************************************************************/
void semaphore_lite_clr_empty(void *hdl, /*! Handle to HDL_semaphore ! */
                         SIGN32 id  /*! Semaphore ID in $SemaHub ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;

    IO32WR(1 << id, sem->ra + RA_SemaHubLite_empty);
    /**	ENDOFFUNCTION: semaphore_lite_clr_empty **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_clr_full
	*	Description: Clear 'full' status of a semaphore.
	******************************************************************************************************************/
void semaphore_lite_clr_full(void *hdl,  /*! Handle to HDL_semaphore ! */
                        SIGN32 id   /*! Semaphore ID in $SemaHub ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;

    IO32WR(1 << id, sem->ra + RA_SemaHubLite_full);
    /**	ENDOFFUNCTION: semaphore_lite_clr_full **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_clr_almostEmpty
	*	Description: Clear 'almostEmpty' status of a semaphore.
	******************************************************************************************************************/
void semaphore_lite_clr_almostEmpty(void *hdl,   /*! Handle to HDL_semaphore ! */
                               SIGN32 id    /*! Semaphore ID in $SemaHub ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;

    IO32WR(1 << id, sem->ra + RA_SemaHubLite_almostEmpty);
    /**	ENDOFFUNCTION: semaphore_lite_clr_almostEmpty **/
}

    /******************************************************************************************************************
	*	Function: semaphore_lite_clr_almostFull
	*	Description: Clear 'almostFull' status of a semaphore.
	******************************************************************************************************************/
void semaphore_lite_clr_almostFull(void *hdl,    /*! Handle to HDL_semaphore ! */
                              SIGN32 id /*! Semaphore ID in $SemaHub ! */
    )
{
    HDL_semaphore *sem = (HDL_semaphore *) hdl;

    IO32WR(1 << id, sem->ra + RA_SemaHubLite_almostFull);
    /**	ENDOFFUNCTION: semaphore_lite_clr_almostFull **/
}

/**	ENDOFSECTION
 */

/**	SECTION - API definitions for $HBO
 */
    /******************************************************************************************************************
	*	Function: hbo_lite_hdl
	*	Description: Initialize HDL_hbo with a $HBO BIU instance.
	******************************************************************************************************************/
void hbo_lite_hdl(UNSG32 mem,        /*! Base address of HBO SRAM ! */
             UNSG32 ra,         /*! Base address of a BIU instance of $HBO ! */
             void *hdl          /*! Handle to HDL_hbo ! */
    )
{
    HDL_hbo *hbo = (HDL_hbo *) hdl;
    HDL_semaphore *fifoCtl = &(hbo->fifoCtl);

    hbo->mem = mem;
    hbo->ra = ra;
    semaphore_lite_hdl(ra + RA_HBOLite_FiFoCtl, fifoCtl);
    /**	ENDOFFUNCTION: hbo_lite_hdl **/
}

    /******************************************************************************************************************
	*	Function: hbo_lite_fifoCtl
	*	Description: Get HDL_semaphore pointer from a HBO instance.
	*	Return:			void*						-	Handle for HBO.FiFoCtl
	******************************************************************************************************************/
void *hbo_lite_fifoCtl(void *hdl     /*! Handle to HDL_hbo ! */
    )
{
    HDL_hbo *hbo = (HDL_hbo *) hdl;
    HDL_semaphore *fifoCtl = &(hbo->fifoCtl);

    return fifoCtl;
    /**	ENDOFFUNCTION: hbo_lite_fifoCtl **/
}

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_cfg
	*	Description: Configurate a FIFO's base, depth & reset pointers.
	*	Return:			UNSG32						-	Number of (adr,pair) added to cfgQ
	******************************************************************************************************************/
UNSG32 hbo_lite_queue_cfg(void *hdl, /*! Handle to HDL_hbo ! */
                     SIGN32 id, /*! Queue ID in $HBO ! */
                     UNSG32 base,   /*! Channel FIFO base address (byte address) ! */
                     SIGN32 depth,  /*! Channel FIFO depth, in 64b word ! */
                     SIGN32 enable, /*! 0 to disable, 1 to enable ! */
                     T64b cfgQ[]    /*! Pass NULL to directly init HBO, or
                                       Pass non-zero to receive programming sequence
                                       in (adr,data) pairs
                                       ! */
    )
{
    HDL_hbo *hbo = (HDL_hbo *) hdl;
    HDL_semaphore *fifoCtl = &(hbo->fifoCtl);
    T32FiFoLite_CFG cfg;
    UNSG32 i = 0, a;
    a = hbo->ra + RA_HBOLite_ARR + id * sizeof (SIE_FiFoLite);
    IO32CFG(cfgQ, i, a + RA_FiFoLite_START, 0);

    cfg.u32 = 0;
    cfg.uCFG_BASE = hbo->base[id] = base;
    IO32CFG(cfgQ, i, a + RA_FiFoLite_CFG, cfg.u32);

    i += semaphore_lite_cfg(fifoCtl, id, depth, cfgQ ? (cfgQ + i) : NULL);
    IO32CFG(cfgQ, i, a + RA_FiFoLite_START, enable);

    return i;
    /**	ENDOFFUNCTION: hbo_lite_queue_cfg **/
}

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_enable
	*	Description: HBO FIFO enable/disable.
	*	Return:			UNSG32						-	Number of (adr,pair) added to cfgQ
	******************************************************************************************************************/
UNSG32 hbo_lite_queue_enable(void *hdl,  /*! Handle to HDL_hbo ! */
                        SIGN32 id,  /*! Queue ID in $HBO ! */
                        SIGN32 enable,  /*! 0 to disable, 1 to enable ! */
                        T64b cfgQ[] /*! Pass NULL to directly init HBO, or
                                       Pass non-zero to receive programming sequence
                                       in (adr,data) pairs
                                       ! */
    )
{
    HDL_hbo *hbo = (HDL_hbo *) hdl;
    UNSG32 i = 0, a;
    a = hbo->ra + RA_HBOLite_ARR + id * sizeof (SIE_FiFoLite);

    IO32CFG(cfgQ, i, a + RA_FiFoLite_START, enable);
    return i;
    /**	ENDOFFUNCTION: hbo_lite_queue_enable **/
}

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_clear
	*	Description: Issue HBO FIFO clear (will NOT wait for finish).
	******************************************************************************************************************/
void hbo_lite_queue_clear(void *hdl, /*! Handle to HDL_hbo ! */
                     SIGN32 id  /*! Queue ID in $HBO ! */
    )
{
    HDL_hbo *hbo = (HDL_hbo *) hdl;
    UNSG32 a;
    a = hbo->ra + RA_HBOLite_ARR + id * sizeof (SIE_FiFoLite);

    IO32WR(1, a + RA_FiFoLite_CLEAR);
    /**	ENDOFFUNCTION: hbo_lite_queue_enable **/
}

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_busy
	*	Description: Read HBO 'BUSY' status for all channel FIFOs.
	*	Return:			UNSG32						-	'BUSY' status bits of all channels
	******************************************************************************************************************/
UNSG32 hbo_lite_queue_busy(void *hdl /*! Handle to HDL_hbo ! */
    )
{
    HDL_hbo *hbo = (HDL_hbo *) hdl;
    UNSG32 d;

    IO32RD(d, hbo->ra + RA_HBOLite_BUSY);

    return d;
    /**	ENDOFFUNCTION: hbo_lite_queue_busy **/
}

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_clear_done
	*	Description: Wait for a given channel or all channels to be cleared.
	******************************************************************************************************************/
void hbo_lite_queue_clear_done(void *hdl,    /*! Handle to HDL_hbo ! */
                          SIGN32 id /*! Queue ID in $HBO
                                       -1 to wait for all channel clear done
                                       ! */
    )
{
    UNSG32 d;

    do {
        d = hbo_lite_queue_busy(hdl);
        if (id >= 0)
            d = bTST(d, id);
    } while (d);

    /**	ENDOFFUNCTION: hbo_lite_queue_clear_done **/
}

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_read
	*	Description: Read a number of 64b data & pop FIFO from HBO SRAM.
	*	Return:			UNSG32						-	Number of 64b data being read (=n), or (when cfgQ==NULL)
	*													0 if there're not sufficient data in FIFO
	******************************************************************************************************************/
UNSG32 hbo_lite_queue_read(void *hdl,    /*! Handle to HDL_hbo ! */
                      SIGN32 id,    /*! Queue ID in $HBO ! */
                      SIGN32 n, /*! Number 64b entries to read ! */
                      T64b data[],  /*! To receive read data ! */
                      UNSG32 * ptr  /*! Pass in current FIFO pointer (in 64b word),
                                       & receive updated new pointer,
                                       Pass NULL to read from HW
                                       ! */
    )
{
    HDL_hbo *hbo = (HDL_hbo *) hdl;
    SIGN32 i;
    UNSG32 p, base, depth;

    base = hbo->mem + hbo->base[id];
    depth = hbo->fifoCtl.depth[id];
    i = hbo_lite_queue_query(hdl, id, SemaQueryMapLite_ADDR_master_consumer, &p);
    if (i < n)
        return 0;

    if (ptr)
        p = *ptr;
    for (i = 0; i < n; i++) {
        IO32RD(data[i][0], base + p * 8);
        IO32RD(data[i][1], base + p * 8 + 4);
        ModInc(p, 1, depth);
    }
    hbo_lite_queue_pop(hdl, id, n);
    if (ptr)
        *ptr = p;
    return n;
    /**	ENDOFFUNCTION: hbo_lite_queue_read **/
}

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_write
	*	Description: Write a number of 64b data & push FIFO to HBO SRAM.
	*	Return:			UNSG32						-	Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
	*													0 if there're not sufficient space in FIFO
	******************************************************************************************************************/
UNSG32 hbo_lite_queue_write(void *hdl,   /*! Handle to HDL_hbo ! */
                       SIGN32 id,   /*! Queue ID in $HBO ! */
                       SIGN32 n,    /*! Number 64b entries to write ! */
                       T64b data[], /*! Write data ! */
                       T64b cfgQ[], /*! Pass NULL to directly update HBO, or
                                       Pass non-zero to receive programming sequence
                                       in (adr,data) pairs
                                       ! */
                       UNSG32 * ptr /*! Pass in current FIFO pointer (in 64b word),
                                       & receive updated new pointer,
                                       Pass NULL to read from HW
                                       ! */
    )
{
    HDL_hbo *hbo = (HDL_hbo *) hdl;
    SIGN32 i, depth;
    UNSG32 base, j = 0, p;

    base = hbo->mem + hbo->base[id];
    depth = hbo->fifoCtl.depth[id];
    if (!ptr) {
        i = hbo_lite_queue_query(hdl, id, SemaQueryMapLite_ADDR_master_consumer, &p);
        if (i > depth - n)
            return 0;
    } else {
        p = *ptr;
    }

    for (i = 0; i < n; i++) {
        IO32CFG(cfgQ, j, base + p * 8, data[i][0]);
        IO32CFG(cfgQ, j, base + p * 8 + 4, data[i][1]);
        ModInc(p, 1, (UNSG32)depth);
    }
    if (!cfgQ)
        hbo_lite_queue_push(hdl, id, n);
    else {
        T32SemaHubLite_PUSH push;
        push.u32 = 0;
        push.uPUSH_ID = id;
        push.uPUSH_delta = n;
        IO32CFG(cfgQ, j, hbo->fifoCtl.ra + RA_SemaHubLite_PUSH, push.u32);
    }
    if (ptr)
        *ptr = p;
    return j;
    /**	ENDOFFUNCTION: hbo_lite_queue_write **/
}

/**	ENDOFSECTION
 */

///******************************************************************************************************************
//* Function: io32wr
//* Parameters : 32 bit addr and data
//* Description: This write is only for SYSTEM VERILOG TB AHB Write
//******************************************************************************************************************/
//void io32wr(UNSG32 d, UNSG32 a)
//{
//  xdbg ( " The data is %0d" , d ) ;
//  xdbg ( " The addr is %0d" , a ) ;
//  DRV_IO_WR32 ( a, d );
//}
///******************************************************************************************************************
//* Function: io32wr
//* Parameters : 32 bit addr and read data
//* Description: This write is only for SYSTEM VERILOG TB AHB Write
//******************************************************************************************************************/
//void io32rd(UNSG32 *d, UNSG32 a)
//{
//  xdbg ( " The data is %0d" , d ) ;
//  xdbg ( " The addr is %0d" , a ) ;
//  DRV_IO_RD32 (a, d);
//}

/**	ENDOFSECTION
 */

/**	ENDOFFILE: hal_dhub.c *********************************************************************************************
 */
