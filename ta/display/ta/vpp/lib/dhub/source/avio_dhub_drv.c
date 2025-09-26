/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "api_avio_dhub.h"

HDL_dhub2d AG_dhubHandle;
HDL_dhub2d VPP_dhubHandle;
HDL_dhub2d VIP_dhubHandle;

static int getDhubCMDDiv(HDL_dhub2d *pdhubHandle)
{
    if (pdhubHandle == &VPP_dhubHandle) {
        return 16;
    }
    if (pdhubHandle == &AG_dhubHandle) {
        return 8;
    }

    return 8;
}

/******************************************************************************************************************
 *      Function: GetChannelInfo
 *      Description: Get the Dhub configuration of requested channel.
 *      Parameter : pdhubHandle ----- pointer to 2D dhubHandle
 *             IChannel        ----- Channel of the dhub
 *             cfg        ----- Configuration need to be updated here.
 *      Return:  0     ----    Success
******************************************************************************************************************/
int getDhubChannelInfo(HDL_dhub2d *pdhubHandle, SIGN32 IChannel, T32dHubChannel_CFG *cfg)
{
       DHUB_channel_config *dhub_config = NULL;
        //Get the Dhub Config array from the received handle
       if(pdhubHandle == &VPP_dhubHandle)
       {
               dhub_config = VPP_config;
       }
       if(pdhubHandle == &AG_dhubHandle)
       {
               dhub_config = AG_config;
       }
       if (dhub_config) {
           //Update the MTU, QOS and self loop paramteres.
           cfg->uCFG_MTU = dhub_config[IChannel].chanMtuSize;
           cfg->uCFG_QoS =  dhub_config[IChannel].chanQos;
           cfg->uCFG_selfLoop = dhub_config[IChannel].chanSelfLoop;
       }

       return 0;
}

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
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr,
            HDL_dhub2d *pdhubHandle, DHUB_channel_config *dhub_config,SIGN32 numOfChans, UNSG32 channelInitMask)
{
    HDL_semaphore *pSemHandle;
    SIGN32 i;
    SIGN32 chanId;
    SIGN32 cmdDiv = 8;
    UNSG32 chanInit;

    cmdDiv = getDhubCMDDiv(pdhubHandle);
    if (!cmdDiv) {
        return;
    }

    //Initialize HDL_dhub with a $dHub BIU instance.
    dhub2d_hdl(	hboSramAddr,			/*!	Base address of dHub.HBO SRAM !*/
            dHubBaseAddr,			/*!	Base address of a BIU instance of $dHub !*/
            pdhubHandle				/*!	Handle to HDL_dhub2d !*/
            );
    //set up semaphore to trigger cmd done interrupt
    //note that this set of semaphores are different from the HBO semaphores
    //the ID must match the dhub ID because they are hardwired.
    pSemHandle = dhub_semaphore(&pdhubHandle->dhub);

    for (i = 0; i< numOfChans; i++) {
        //Configurate a dHub channel
        //note that in this function, it also configured right HBO channels(cmdQ and dataQ) and semaphores
        chanId = dhub_config[i].chanId;
        AVIO_DHUB_INITCHANNELAXQOS(pdhubHandle, dhub_config, chanId, i, 0);
        chanInit = (channelInitMask & (1 << chanId)) ? 1 : 0;
        dhub_channel_cfg(
                &pdhubHandle->dhub,					/*!	Handle to HDL_dhub !*/
                chanId,						/*!	Channel ID in $dHubReg !*/
                dhub_config[i].chanCmdBase,		//UNSG32 baseCmd,	/*!	Channel FIFO base address (byte address) for cmdQ !*/
                dhub_config[i].chanDataBase,		//UNSG32 baseData,	/*!	Channel FIFO base address (byte address) for dataQ !*/
                dhub_config[i].chanCmdSize/cmdDiv,	//SIGN32		depthCmd,			/*!	Channel FIFO depth for cmdQ, in 64b word !*/
                dhub_config[i].chanDataSize/cmdDiv,	//SIGN32		depthData,			/*!	Channel FIFO depth for dataQ, in 64b word !*/
                dhub_config[i].chanMtuSize,						/*!	See 'dHubChannel.CFG.MTU', 0/1/2 for 8/32/128 bytes !*/
                dhub_config[i].chanQos,								/*!	See 'dHubChannel.CFG.QoS' !*/
                dhub_config[i].chanSelfLoop,								/*!	See 'dHubChannel.CFG.selfLoop' !*/
                dhub_config[i].chanEnable,								/*!	0 to disable, 1 to enable !*/
                0,								/*!	Pass NULL to directly init dHub, or
                                                  Pass non-zero to receive programming sequence
                                                  in (adr,data) pairs
                                                  !*/
                chanInit
                );

        //Continue, no further initialization is required
        if (!chanInit)
            continue;

        // setup interrupt for channel chanId
        //configure the semaphore depth to be 1
        semaphore_cfg(pSemHandle, chanId, 1, 0);

        // disable interrupt from this semaphore
        semaphore_intr_enable (
                pSemHandle, // semaphore handler
                chanId,
                0,       // empty
                0,       // full
                0,       // almost_empty
                0,       // almost_full
                cpuId        // 0~2, depending on which CPU the interrupt is enabled for.
                );
    }
}

/******************************************************************************************************************
 *    Function: DhubChannelClear
 *    Description: Clear corresponding DHUB channel.
 *    Parameter:    hdl  ---------- handle to HDL_dhub
 *            id   ---------- channel ID in dHubReg
 *            cfgQ ---------- pass null to directly init dhub, or pass non-zero to receive programming
 *                    sequence in (adr, data) pairs
 *    Return:        void
******************************************************************************************************************/
void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[])
{
    UNSG32    cmdID = dhub_id2hbo_cmdQ(id);
    UNSG32    dataID = dhub_id2hbo_data(id);
    HDL_dhub *dhub = (HDL_dhub *)hdl;
    HDL_hbo *hbo = &(dhub->hbo);

    /* 1.Software stops the command queue in HBO (please refer to HBO.sxw for details) */
    hbo_queue_enable(hbo, cmdID, 0, cfgQ);
    /* 2.Software stops the channel in dHub by writing zero to  dHubChannel.START.EN */
    dhub_channel_enable(dhub, id, 0, cfgQ);
    /* 3.Software clears the channel in dHub by writing one to  dHubChannel.CLEAR.EN */
    dhub_channel_clear(dhub, id);
    /* 4.Software waits for the register bits dHubChannel.PENDING.ST and dHubChannel.BUSY.ST to be 0 */
    dhub_channel_clear_done(dhub, id);
    /* 5.Software stops and clears the command queue */
    hbo_queue_enable(hbo, cmdID, 0, cfgQ);
    hbo_queue_clear(hbo, cmdID);
    /* 6.Software wait for the corresponding busy bit to be 0 */
    hbo_queue_clear_done(hbo, cmdID);
    /* 7.Software stops and clears the data queue */
    hbo_queue_enable(hbo, dataID, 0, cfgQ);
    hbo_queue_clear(hbo, dataID);
    /* 8.Software wait for the corresponding data Q busy bit to be 0 */
    hbo_queue_clear_done(hbo, dataID);
    /* 9.Software enable dHub and HBO */
    dhub_channel_enable(dhub, id, 1, cfgQ);
    hbo_queue_enable(hbo, cmdID, 1, cfgQ);
    hbo_queue_enable(hbo, dataID, 1, cfgQ);
}
