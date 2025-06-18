/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef TEE_CA_DHUB_H
#define TEE_CA_DHUB_H

#include    "ctypes.h"
#include    "dhub_cmd.h"

#ifdef  __cplusplus
extern  "C" {
#endif

int  Dhub_TA_Initialize(void);
void Dhub_TA_Finalize(void);
int  DhubRegisterShm(unsigned int *VirtAddr, unsigned int Size);
int  DhubRegisterShm(unsigned int *VirtAddr, unsigned int Size);

void tz_DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, int *pdhubHandle, int *dhub_config,SIGN32 numOfChans);
void tz_DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);

UNSG32 tz_dhub_channel_write_cmd(
        void        *hdl,       /*! Handle to HDL_dhub !*/
        SIGN32      id,     /*! Channel ID in $dHubReg !*/
        UNSG32      addr,       /*! CMD: buffer address !*/
        SIGN32      size,       /*! CMD: number of bytes to transfer !*/
        SIGN32      semOnMTU,   /*! CMD: semaphore operation at CMD/MTU (0/1) !*/
        SIGN32      chkSemId,   /*! CMD: non-zero to check semaphore !*/
        SIGN32      updSemId,   /*! CMD: non-zero to update semaphore !*/
        SIGN32      interrupt,  /*! CMD: raise interrupt at CMD finish !*/
        T64b        cfgQ[],     /*! Pass NULL to directly update dHub, or
                            Pass non-zero to receive programming sequence
                            in (adr,data) pairs
                            !*/
        UNSG32      *ptr        /*! Pass in current cmdQ pointer (in 64b word),
                            & receive updated new pointer,
                            Pass NULL to read from HW
                            !*/
        );

void tz_dhub_channel_generate_cmd(
        void        *hdl,       /*! Handle to HDL_dhub !*/
        SIGN32      id,     /*! Channel ID in $dHubReg !*/
        UNSG32      addr,       /*! CMD: buffer address !*/
        SIGN32      size,       /*! CMD: number of bytes to transfer !*/
        SIGN32      semOnMTU,   /*! CMD: semaphore operation at CMD/MTU (0/1) !*/
        SIGN32      chkSemId,   /*! CMD: non-zero to check semaphore !*/
        SIGN32      updSemId,   /*! CMD: non-zero to update semaphore !*/
        SIGN32      interrupt,  /*! CMD: raise interrupt at CMD finish !*/
        SIGN32      *pData
        );
void tz_semaphore_pop(
        void    *hdl,       /*  Handle to HDL_semaphore */
        SIGN32  id,     /*  Semaphore ID in $SemaHub */
        SIGN32  delta       /*  Delta to pop as a consumer */
        );

void tz_semaphore_clr_full(
        void    *hdl,       /*  Handle to HDL_semaphore */
        SIGN32  id      /*  Semaphore ID in $SemaHub */
        );

UNSG32 tz_semaphore_chk_full(
        void    *hdl,       /*Handle to HDL_semaphore */
        SIGN32  id      /*Semaphore ID in $SemaHub
                    -1 to return all 32b of the interrupt status
                    */
        );

void    tz_semaphore_intr_enable(
        void        *hdl,               /*! Handle to HDL_semaphore !*/
        SIGN32      id,                 /*! Semaphore ID in $SemaHub !*/
        SIGN32      empty,              /*! Interrupt enable for CPU at condition 'empty' !*/
        SIGN32      full,               /*! Interrupt enable for CPU at condition 'full' !*/
        SIGN32      almostEmpty,        /*! Interrupt enable for CPU at condition 'almostEmpty' !*/
        SIGN32      almostFull,         /*! Interrupt enable for CPU at condition 'almostFull' !*/
        SIGN32      cpu                 /*! CPU ID (0/1/2) !*/
        );

UNSG32 tz_semaphore_query(void *hdl, /*! Handle to HDL_semaphore ! */
            SIGN32 id, /*  Semaphore ID in $SemaHub */
            SIGN32  master, /*  0/1 as procuder/consumer query */
            UNSG32  *ptr    /*  Non-zero to receive semaphore r/w pointer */
       );

void tz_dhub2d_channel_start_seq(void *hdl, SIGN32 id);
void tz_dhub2d_channel_clear_seq(void *hdl, SIGN32 id);
void tz_dhub2nd_channel_start_seq(void *hdl, SIGN32 id);
void tz_dhub2nd_channel_clear_seq(void *hdl, SIGN32 id);
int tz_BCM_SCHED_PushCmd(UNSG32 QID, UNSG32 *pCmd, UNSG32 *cfgQ);
void tz_BCM_SCHED_GetEmptySts(UNSG32 QID, UNSG32 *EmptySts);
void tz_BCM_SCHED_SetMux(UNSG32 QID, UNSG32 TrigEvent);
int tz_BCM_SCHED_AutoPushCmd(UNSG32 QID, UNSG8 uchEnable);

#ifdef  __cplusplus
}
#endif

#endif
