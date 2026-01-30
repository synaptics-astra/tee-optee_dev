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

////////////////////////////////////////////////////////////////////////////////
//! \file api_dhubLite.h
//!
//! \brief DHUB External header file
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    DEC 15 2017,        Zhiliang Wang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef	DHUB_LITE_API
#define	DHUB_LITE_API			"        DHUB_LITE_API >>>    "
/**	DHUB_API
 */

#include	"ctypes.h"
#include	"tsp.h"

#ifndef	DHUB_API

typedef struct HDL_semaphore {
    UNSG32 ra;                  /*! Base address of $SemaHub ! */
    SIGN32 depth[32];           /*! Array of semaphore (virtual FIFO) depth ! */
} HDL_semaphore;

typedef struct HDL_hbo {
    UNSG32 mem;                 /*! Base address of HBO SRAM ! */
    UNSG32 ra;                  /*! Base address of $HBO ! */
    HDL_semaphore fifoCtl;      /*! Handle of HBO.FiFoCtl ! */
    UNSG32 base[32];            /*! Array of HBO queue base address ! */
} HDL_hbo;

typedef struct HDL_dhub {
    UNSG32 ra;                  /*! Base address of $dHub ! */
    HDL_semaphore semaHub;      /*! Handle of dHub.SemaHub ! */
    HDL_hbo hbo;                /*! Handle of dHub.HBO ! */
    SIGN32 MTUb[16];            /*! Array of dHub channel MTU size bits ! */
} HDL_dhub;

typedef struct HDL_dhub2d {
    UNSG32 ra;                  /*! Base address of $dHub2D ! */
    HDL_dhub dhub;              /*! Handle of dHub2D.dHub ! */
} HDL_dhub2d;
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/**	SECTION - handle of local contexts
 */
    extern UNSG32 sizeof_hdl_semaphore; /*! sizeof(HDL_semaphore) ! */
    extern UNSG32 sizeof_hdl_hbo;   /*! sizeof(HDL_hbo) ! */
    extern UNSG32 sizeof_hdl_dhub;  /*! sizeof(HDL_dhub) ! */
    extern UNSG32 sizeof_hdl_dhub2d;    /*! sizeof(HDL_dhub2d) ! */

/**	SECTION - API definitions for $SemaHub
 */
    /******************************************************************************************************************
	*	Function: semaphore_lite_hdl
	*	Description: Initialize HDL_semaphore with a $SemaHub BIU instance.
	******************************************************************************************************************/
    void semaphore_lite_hdl(UNSG32 ra,   /*! Base address of a BIU instance of $SemaHub ! */
                       void *hdl    /*! Handle to HDL_semaphore ! */
        );

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
        );

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
        );

    /******************************************************************************************************************
	*	Function: semaphore_lite_query
	*	Description: Query current status (counter & pointer) of a semaphore.
	*	Return:			UNSG32						-	Current available unit level
	******************************************************************************************************************/
    UNSG32 semaphore_lite_query(void *hdl,   /*! Handle to HDL_semaphore ! */
                           SIGN32 id,   /*! Semaphore ID in $SemaHub ! */
                           SIGN32 master,   /*! 0/1 as procuder/consumer query ! */
                           UNSG32 * ptr /*! Non-zero to receive semaphore r/w pointer ! */
        );

    /******************************************************************************************************************
	*	Function: semaphore_lite_push
	*	Description: Producer semaphore push.
	******************************************************************************************************************/
    void semaphore_lite_push(void *hdl,  /*! Handle to HDL_semaphore ! */
                        SIGN32 id,  /*! Semaphore ID in $SemaHub ! */
                        SIGN32 delta    /*! Delta to push as a producer ! */
        );

    /******************************************************************************************************************
	*	Function: semaphore_lite_push
	*	Description: Consumer semaphore pop.
	******************************************************************************************************************/
    void semaphore_lite_pop(void *hdl,   /*! Handle to HDL_semaphore ! */
                       SIGN32 id,   /*! Semaphore ID in $SemaHub ! */
                       SIGN32 delta /*! Delta to pop as a consumer ! */
        );

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
        );

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
        );

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
        );

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
        );

    /******************************************************************************************************************
	*	Function: semaphore_lite_clr_empty
	*	Description: Clear 'empty' status of a semaphore.
	******************************************************************************************************************/
    void semaphore_lite_clr_empty(void *hdl, /*! Handle to HDL_semaphore ! */
                             SIGN32 id  /*! Semaphore ID in $SemaHub ! */
        );

    /******************************************************************************************************************
	*	Function: semaphore_lite_clr_full
	*	Description: Clear 'full' status of a semaphore.
	******************************************************************************************************************/
    void semaphore_lite_clr_full(void *hdl,  /*! Handle to HDL_semaphore ! */
                            SIGN32 id   /*! Semaphore ID in $SemaHub ! */
        );

    /******************************************************************************************************************
	*	Function: semaphore_lite_clr_almostEmpty
	*	Description: Clear 'almostEmpty' status of a semaphore.
	******************************************************************************************************************/
    void semaphore_lite_clr_almostEmpty(void *hdl,   /*! Handle to HDL_semaphore ! */
                                   SIGN32 id    /*! Semaphore ID in $SemaHub ! */
        );

    /******************************************************************************************************************
	*	Function: semaphore_lite_clr_almostFull
	*	Description: Clear 'almostFull' status of a semaphore.
	******************************************************************************************************************/
    void semaphore_lite_clr_almostFull(void *hdl,    /*! Handle to HDL_semaphore ! */
                                  SIGN32 id /*! Semaphore ID in $SemaHub ! */
        );
/**	ENDOFSECTION
 */

/**	SECTION - API definitions for $HBO
 */
    /******************************************************************************************************************
	*	Function: hbo_lite_hdl
	*	Description: Initialize HDL_hbo with a $HBO BIU instance.
	******************************************************************************************************************/
    void hbo_lite_hdl(UNSG32 mem,    /*! Base address of HBO SRAM ! */
                 UNSG32 ra,     /*! Base address of a BIU instance of $HBO ! */
                 void *hdl      /*! Handle to HDL_hbo ! */
        );

    /******************************************************************************************************************
	*	Function: hbo_fifoCtl
	*	Description: Get HDL_semaphore pointer from a HBO instance.
	*	Return:			void*						-	Handle for HBO.FiFoCtl
	******************************************************************************************************************/
    void *hbo_lite_fifoCtl(void *hdl /*! Handle to HDL_hbo ! */
        );

    /******************************************************************************************************************
	*	DEFINITION - convert HBO FIFO control to semaphore control
	******************************************************************************************************************/
#define	hbo_lite_queue_intr_enable(hdl,id,empty,full,almostEmpty,almostFull,cpu)								\
			  semaphore_lite_intr_enable(hbo_lite_fifoCtl(hdl),id,empty,full,almostEmpty,almostFull,cpu)

#define	hbo_lite_queue_query(hdl,id,master,ptr)															\
			  semaphore_lite_query(hbo_lite_fifoCtl(hdl),id,master,ptr)

#define	hbo_lite_queue_push(hdl,id,delta)																\
			  semaphore_lite_push(hbo_lite_fifoCtl(hdl),id,delta)

#define	hbo_lite_queue_pop(hdl,id,delta)																\
			  semaphore_lite_pop(hbo_lite_fifoCtl(hdl),id,delta)

#define	hbo_lite_queue_chk_empty(hdl,id)																	\
			  semaphore_lite_chk_empty(hbo_lite_fifoCtl(hdl),id)

#define	hbo_lite_queue_chk_full(hdl,id)																	\
			  semaphore_lite_chk_full(hbo_lite_fifoCtl(hdl),id)

#define	hbo_lite_queue_chk_almostEmpty(hdl,id)																	\
			  semaphore_lite_chk_almostEmpty(hbo_lite_fifoCtl(hdl),id)

#define	hbo_lite_queue_chk_almostFull(hdl,id)																	\
			  semaphore_lite_chk_almostFull(hbo_lite_fifoCtl(hdl),id)

#define	hbo_lite_queue_clr_empty(hdl,id)																	\
			  semaphore_lite_clr_empty(hbo_lite_fifoCtl(hdl),id)

#define	hbo_lite_queue_clr_full(hdl,id)																	\
			  semaphore_lite_clr_full(hbo_lite_fifoCtl(hdl),id)

#define	hbo_lite_queue_clr_almostEmpty(hdl,id)																	\
			  semaphore_lite_clr_almostEmpty(hbo_lite_fifoCtl(hdl),id)

#define	hbo_lite_queue_clr_almostFull(hdl,id)																	\
			  semaphore_lite_clr_almostFull(hbo_lite_fifoCtl(hdl),id)

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
        );

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
        );

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_clear
	*	Description: Issue HBO FIFO clear (will NOT wait for finish).
	******************************************************************************************************************/
    void hbo_lite_queue_clear(void *hdl, /*! Handle to HDL_hbo ! */
                         SIGN32 id  /*! Queue ID in $HBO ! */
        );

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_busy
	*	Description: Read HBO 'BUSY' status for all channel FIFOs.
	*	Return:			UNSG32						-	'BUSY' status bits of all channels
	******************************************************************************************************************/
    UNSG32 hbo_lite_queue_busy(void *hdl /*! Handle to HDL_hbo ! */
        );

    /******************************************************************************************************************
	*	Function: hbo_lite_queue_clear_done
	*	Description: Wait for a given channel or all channels to be cleared.
	******************************************************************************************************************/
    void hbo_lite_queue_clear_done(void *hdl,    /*! Handle to HDL_hbo ! */
                              SIGN32 id /*! Queue ID in $HBO
                                           -1 to wait for all channel clear done
                                           ! */
        );

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
        );

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
        );
/**	ENDOFSECTION
 */

/**	SECTION - API definitions for $dHubReg
 */
    /******************************************************************************************************************
	*	Function: dhub_lite_hdl
	*	Description: Initialize HDL_dhub with a $dHub BIU instance.
	******************************************************************************************************************/
    void dhub_lite_hdl(UNSG32 mem,   /*! Base address of dHub.HBO SRAM ! */
                  UNSG32 ra,    /*! Base address of a BIU instance of $dHub ! */
                  void *hdl     /*! Handle to HDL_dhub ! */
        );

    /******************************************************************************************************************
	*	Function: dhub_lite_semaphore
	*	Description: Get HDL_semaphore pointer from a dHub instance.
	*	Return:			void*						-	Handle for dHub.SemaHub
	******************************************************************************************************************/
    void *dhub_lite_semaphore(void *hdl  /*! Handle to HDL_dhub ! */
        );

    /******************************************************************************************************************
	*	Function: dhub_lite_hbo
	*	Description: Get HDL_hbo pointer from a dHub instance.
	*	Return:			void*						-	Handle for dHub.HBO
	******************************************************************************************************************/
    void *dhub_lite_hbo(void *hdl    /*! Handle to HDL_dhub ! */
        );

	/******************************************************************************************************************
	*	Function: dhub_lite_hbo_fifoCtl
	*	Description: Get HDL_semaphore pointer from the HBO of a dHub instance.
	*	Return:			void*						-	Handle for dHub.HBO.FiFoCtl
	******************************************************************************************************************/

#endif
/**	ENDOFFILE: api_dhubLite.h *********************************************************************************************
 */
