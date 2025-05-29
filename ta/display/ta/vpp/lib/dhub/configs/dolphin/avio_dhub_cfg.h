/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */



#ifndef __AVIO_DHUB_CFG_H__
#define __AVIO_DHUB_CFG_H__
#include "dHub.h"
#include "avioDhub.h"
#include "avio_memmap.h"

#define  MAX_DHUB_CHANNEL_NUMBER        avioDhubChMap_vpp128b_OVPSCL_R1

//Enable this macro to allocate more SRAM to MAIN PIP & GFX Plane

#define VPP_DHUB_BASE           (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_dHub0)
#define VPP_HBO_SRAM_BASE       (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_tcm0)
#define VPP_NUM_OF_CHANNELS     (avioDhubChMap_vpp128b_OVPSCL_R1+1)
#define AG_DHUB_BASE            (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_dHub0)
#define AG_HBO_SRAM_BASE        (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_tcm0)
#define AG_NUM_OF_CHANNELS      (avioDhubChMap_aio64b_SPDIF_W+1)

#define VPP_DHUB_BANK0_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK0_START_ADDR
#define VPP_DHUB_BANK1_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK1_START_ADDR
#define VPP_DHUB_BANK2_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK2_START_ADDR
#define VPP_DHUB_BANK3_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK3_START_ADDR
#define AG_DHUB_BANK0_START_ADDR        avioDhubTcmMap_aio64bDhub_BANK0_START_ADDR

#define AVIO_DHUB_INITCHANNELAXQOS(pdhubHandle, dhub_config, chanId, i, cfgQ) \
    dhub_channel_axqos(                 \
            &pdhubHandle->dhub,         \
            chanId,                     \
            dhub_config[i].chanAxQosLO, \
            dhub_config[i].chanAxQosHI, \
            dhub_config[i].chanAxQosLO, \
            dhub_config[i].chanAxQosHI, \
            cfgQ                        \
            );

#define AVIO_DHUB_MTR_CFG_QOS_EN     1
#define AVIO_DHUB_MTR_CFG_QOS        0xF
#define AVIO_DHUB_DESC_OVRDQOS       1
#define AVIO_DHUB_DESC_QOSSEL        1
#define AVIO_DHUB_BURST_BW_OPT       0
#define AVIO_DHUB_MTU_OPT            dHubChannel_CFG_MTU_256byte
#define AVIO_DHUB_MTU_MMU_ONLY       dHubChannel_CFG_MTU_256byte
#define AVIO_DHUB_OPT_EN_ONLY_MTR()  IS_MTR_ENABLED(pstCfg2NDDMAParam->mtrREn)
#define VPP_AVIOMTR_DHUB_STRIDE (262144)

#endif /*__AVIO_DHUB_CFG_H__*/
