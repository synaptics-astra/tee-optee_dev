/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __VPP_BE_CEC_SRV_PRV_H__
#define __VPP_BE_CEC_SRV_PRV_H__

#include "com_type.h"

/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */
// Command queue size
#define     CEC_MAX_FIFO_SIZE       32

// Maximum retransmission count in case of transmission failure
// Android also has retries so retry count is reduced here
#define     CEC_MAX_RETX_COUNT      2

// Conditions on which retransmission should be attempted
#define     CEC_RETX_CONDN          (BE_CEC_STS_TX_FAIL_NOACK | \
                                     BE_CEC_STS_TX_FAIL_RESP_TIMEOUT | \
                                     BE_CEC_STS_TX_FAIL_INFO_NONE | \
                                     BE_CEC_STS_TX_FAIL_COLL_DET)

// Maximum time-out for Tx msg interrupt (just for safety in case we miss interrupt or system failure)
#define CEC_MSG_TX_TOTAL_TIMEOUT (36000)
// Maximum time-out for message transmit
#define     CEC_MSG_TX_TIMEOUT      (600)

// Broadcast logical address
#define     CEC_BROADCAST_ADDR      0x0F

// Last logical address
#define     CEC_MAX_LOG_ADDR        0x0F

// Macro to check the data store pointer
#define     CEC_DATASTORE_CHK(dsPtr)    \
                if (dsPtr == NULL) {return (MV_VPP_EBADPARAM);}


/*-----------------------------------------------------------------------------
 * Enums
 *-----------------------------------------------------------------------------
 */
/* CEC queue commands */
typedef enum CEC_CMD_ID_T
{
    CEC_CMD_SET_PHYADDR = 0x00,
    CEC_CMD_RESET_PHYADDR,
    CEC_CMD_ALLOC_LOGADDR,
    CEC_CMD_UNALLOC_LOGADDR,
    CEC_CMD_TX_FRAME,
    CEC_CMD_RX_FRAME,
    CEC_CMD_TX_STS,
    CEC_CMD_RX_STS,
    CEC_CMD_MAX
}CEC_CMD_ID;

/*-----------------------------------------------------------------------------
 * Structures and Unions
 *-----------------------------------------------------------------------------
 */
/* CEC command data */
typedef union CEC_CMD_DATA_T
{
    UINT8   genData[VPP_CEC_MAX_MSG_LEN];

    struct {
        int phyAddr;
    } cmdPhyAddr;

    struct {
        int initialPollAddr;
    } cmdLogAddr;

    struct {
        VPP_CEC_MSG cecMsg;
    } cmdTxFrame;

    struct {
        UINT8   stsInfo;
        UINT8   frmHeader;
        UINT8   frmOpcode;
    } cmdTxStatus;

    struct {
        UINT8 rxFrame[VPP_CEC_MAX_MSG_LEN];
    } cmdRxFrame;

    struct {
        UINT8 stsInfo;
    } cmdRxStatus;
}CEC_CMD_DATA;

/* CEC command data structure */
typedef struct CEC_CMD_T
{
    CEC_CMD_ID      cmdID;
    //If devicetype is not valid for the command, set it to CEC_DEVICE_MAX
    VPP_CEC_DEVICE_TYPE  cmdDeviceType;
    UINT16          cmdDataLen;
    CEC_CMD_DATA    cmdData;
}CEC_CMD, *PCEC_CMD;

/*-----------------------------------------------------------------------------
 * Static variables
 *-----------------------------------------------------------------------------
 */
/* CEC device logical address mapping - based on Spec HDMI 1.3a */
#ifdef _VPP_BE_CEC_SRV_C_
const char CEC_devLogAddrMap[] =
{
    VPP_CEC_DEVICE_TV,          // 0
    VPP_CEC_DEVICE_REC,         // 1
    VPP_CEC_DEVICE_REC,         // 2
    VPP_CEC_DEVICE_TUNER,       // 3
    VPP_CEC_DEVICE_PB,          // 4
    VPP_CEC_DEVICE_AUDIO_SYS,   // 5
    VPP_CEC_DEVICE_TUNER,       // 6
    VPP_CEC_DEVICE_TUNER,       // 7
    VPP_CEC_DEVICE_PB,          // 8
    VPP_CEC_DEVICE_REC,         // 9
    VPP_CEC_DEVICE_TUNER,       // 10 // 1.3a
    VPP_CEC_DEVICE_PB,          // 11 // 1.3a
    VPP_CEC_DEVICE_RES,         // 12
    VPP_CEC_DEVICE_RES,         // 13
    VPP_CEC_DEVICE_TV,          // 14 (since free use addr is also for TV)
    VPP_CEC_DEVICE_MAX
};
#else
extern const char CEC_devLogAddrMap[];
#endif

#define CEC_LOG_ADDR_MAP_SIZE   (sizeof(CEC_devLogAddrMap)/sizeof(char))

#endif // __VPP_BE_CEC_SRV_PRV_H__
