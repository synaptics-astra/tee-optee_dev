/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __API_AVIO_DHUB_H__
#define __API_AVIO_DHUB_H__
#include "dHub.h"
#include "avioDhub.h"
#include "api_dhub.h"
#include "avio_dhub_cfg.h"

typedef struct DHUB_channel_config {
    SIGN32 chanId;
    UNSG32 chanCmdBase;
    UNSG32 chanDataBase;
    SIGN32 chanCmdSize;
    SIGN32 chanDataSize;
    SIGN32 chanMtuSize;
    SIGN32 chanQos;
    SIGN32 chanSelfLoop;
    SIGN32 chanEnable;
    UNSG32 chanAxQosLO;
    UNSG32 chanAxQosHI;
} DHUB_channel_config;

typedef struct VIP_BCMBUF_T {
    unsigned int *head;          // head of total BCM buffer
    unsigned int *tail;          // tail of the buffer, used for checking wrap around
    unsigned int *writer;        // write pointer of queue
    int size;                    // size of total BCM buffer
    unsigned int handler;     // amp share memory handler
} VIP_BCMBUF;

extern HDL_dhub2d AG_dhubHandle;
extern HDL_dhub2d VPP_dhubHandle;
extern HDL_dhub2d VIP_dhubHandle;
extern DHUB_channel_config  AG_config[];
extern DHUB_channel_config  VPP_config[];
extern DHUB_channel_config  VIP_config[];

/******************************************************************************************************************
 *    Function: DhubInitialization
 *    Description: Initialize DHUB .
 *    Parameter : cpuId ------------- cpu ID
 *             dHubBaseAddr -------------  dHub Base address.
 *             hboSramAddr ----- Sram Address for HBO.
 *             pdhubHandle ----- pointer to 2D dhubHandle
 *             dhub_config ----- configuration of AG
 *             numOfChans     ----- number of channels
 *    Return:        void
******************************************************************************************************************/
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config,SIGN32 numOfChans, UNSG32 channelInitMask);

void DhubEnableAutoPush(UNSG32 enable);

void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);

int getDhubChannelInfo(HDL_dhub2d *pdhubHandle, SIGN32 IChannel, T32dHubChannel_CFG *cfg);

void DhubInitialization_128BytesAXIChannel(HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config, UNSG8 uchNumChannels, UNSG32 uiBcmBuf);

void DhubInitialization_128BytesAXI(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr, HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config, UNSG8 numOfChans);

void dhub2nd_channel_clear_seq(void *hdl, SIGN32 id);

void dhub2nd_channel_start_seq(void *hdl, SIGN32 id);

void dhub2nd_channel_clear_seq_bcm(void *hdl, SIGN32 id, VIP_BCMBUF *pbcmbuf);

void dhub2d_channel_clear_seq_bcm(void *hdl, SIGN32 id, VIP_BCMBUF *pbcmbuf);

#endif //__API_AVIO_DHUB_H__
