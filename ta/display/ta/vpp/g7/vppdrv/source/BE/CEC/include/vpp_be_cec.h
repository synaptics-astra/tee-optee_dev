/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __VPP_BE_CEC_H__
#define __VPP_BE_CEC_H__

#include "com_type.h"

/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */
/* CEC Interrupts */

#define BE_CEC_INTR_NONE                        0x0000
#define BE_CEC_INTR_TX_UNICAST_NOACK            0x0001
#define BE_CEC_INTR_TX_BRDCAST_NOACK            0x0002
#define BE_CEC_INTR_TX_COLLISION                0x0004
#define BE_CEC_INTR_TX_SFT_FAIL                 0x2008
#define BE_CEC_INTR_TX_FAIL                     0x200F
#define BE_CEC_INTR_TX_COMPLETE                 0x0010
#define BE_CEC_INTR_RX_COMPLETE                 0x0020
#define BE_CEC_INTR_RX_LOW_DT_ERROR             0x0040
#define BE_CEC_INTR_RX_HIGH_DT_ERROR            0x0080
#define BE_CEC_INTR_RX_FAIL                     0x00C0
#define BE_CEC_INTR_TX_FIFO_FULL                0x0100
#define BE_CEC_INTR_TX_FIFO_EMPTY               0x0200
#define BE_CEC_INTR_RX_FIFO_FULL                0x0400
#define BE_CEC_INTR_RX_FIFO_EMPTY               0x0800
#define BE_CEC_INTR_WAKEUP                      0x1000
#define BE_CEC_INTR_ALL                         (BE_CEC_INTR_TX_FAIL       |\
                                                 BE_CEC_INTR_TX_COMPLETE   |\
                                                 BE_CEC_INTR_RX_COMPLETE   |\
                                                 BE_CEC_INTR_RX_FAIL       |\
                                                 BE_CEC_INTR_TX_FIFO_FULL  |\
                                                 BE_CEC_INTR_TX_FIFO_EMPTY |\
                                                 BE_CEC_INTR_RX_FIFO_FULL  |\
                                                 BE_CEC_INTR_RX_FIFO_EMPTY |\
                                                 BE_CEC_INTR_WAKEUP)
// --  CEC
#define CEC_BASE                             0x0
#define CEC_TOGGLE_FOR_WRITE_REG_ADDR        CEC_BASE + 0x0000
#define CEC_TOGGLE_FOR_READ_REG_ADDR         CEC_BASE + 0x0004
#define CEC_RDY_ADDR                         CEC_BASE + 0x0008 //01
#define CEC_RX_RDY_ADDR                      CEC_BASE + 0x000c //01
#define CEC_TX_FIFO_RESET_ADDR               CEC_BASE + 0x0010 //03
#define CEC_RX_FIFO_RESET_ADDR               CEC_BASE + 0x0014 //03
#define CEC_PMODE_ADDR                       CEC_BASE + 0x0018 //01
#define CEC_TX_TYPE_ADDR                     CEC_BASE + 0x0020 //01
#define CEC_SIGNAL_FREE_TIME_0_ADDR          CEC_BASE + 0x0028 //FF
#define CEC_SIGNAL_FREE_TIME_1_ADDR          CEC_BASE + 0x0029 //FF
#define CEC_SIGNAL_FREE_TIME_2_ADDR          CEC_BASE + 0x002a //FF
#define CEC_SIGNAL_FREE_TIME_3_ADDR          CEC_BASE + 0x002b //FF
#define CEC_START_BIT_LO_THRESH_0_ADDR       CEC_BASE + 0x002c //FF
#define CEC_START_BIT_LO_THRESH_1_ADDR       CEC_BASE + 0x002d //FF
#define CEC_START_BIT_LO_THRESH_2_ADDR       CEC_BASE + 0x002e //FF
#define CEC_START_BIT_LO_THRESH_3_ADDR       CEC_BASE + 0x002f //FF
#define CEC_START_BIT_HI_THRESH_0_ADDR       CEC_BASE + 0x0030 //FF
#define CEC_START_BIT_HI_THRESH_1_ADDR       CEC_BASE + 0x0031 //FF
#define CEC_START_BIT_HI_THRESH_2_ADDR       CEC_BASE + 0x0032 //FF
#define CEC_START_BIT_HI_THRESH_3_ADDR       CEC_BASE + 0x0033 //FF
#define CEC_DATA_BIT_0_LO_THRESH_0_ADDR      CEC_BASE + 0x0034 //FF
#define CEC_DATA_BIT_0_LO_THRESH_1_ADDR      CEC_BASE + 0x0035 //FF
#define CEC_DATA_BIT_0_LO_THRESH_2_ADDR      CEC_BASE + 0x0036 //FF
#define CEC_DATA_BIT_0_LO_THRESH_3_ADDR      CEC_BASE + 0x0037 //FF
#define CEC_DATA_BIT_1_LO_THRESH_0_ADDR      CEC_BASE + 0x0038 //FF
#define CEC_DATA_BIT_1_LO_THRESH_1_ADDR      CEC_BASE + 0x0039 //FF
#define CEC_DATA_BIT_1_LO_THRESH_2_ADDR      CEC_BASE + 0x003a //FF
#define CEC_DATA_BIT_1_LO_THRESH_3_ADDR      CEC_BASE + 0x003b //FF
#define CEC_DATA_BIT_0_HI_THRESH_0_ADDR      CEC_BASE + 0x003c //FF
#define CEC_DATA_BIT_0_HI_THRESH_1_ADDR      CEC_BASE + 0x003d //FF
#define CEC_DATA_BIT_0_HI_THRESH_2_ADDR      CEC_BASE + 0x003e //FF
#define CEC_DATA_BIT_0_HI_THRESH_3_ADDR      CEC_BASE + 0x003f //FF
#define CEC_DATA_BIT_1_HI_THRESH_0_ADDR      CEC_BASE + 0x0040 //FF
#define CEC_DATA_BIT_1_HI_THRESH_1_ADDR      CEC_BASE + 0x0041 //FF
#define CEC_DATA_BIT_1_HI_THRESH_2_ADDR      CEC_BASE + 0x0042 //FF
#define CEC_DATA_BIT_1_HI_THRESH_3_ADDR      CEC_BASE + 0x0043 //FF
#define CEC_SSP_ACK_TIME_0_ADDR              CEC_BASE + 0x0044 //FF
#define CEC_SSP_ACK_TIME_1_ADDR              CEC_BASE + 0x0045 //FF
#define CEC_SSP_ACK_TIME_2_ADDR              CEC_BASE + 0x0046 //FF
#define CEC_SSP_ACK_TIME_3_ADDR              CEC_BASE + 0x0047 //FF
#define CEC_INTR_ENABLE0_REG_ADDR            CEC_BASE + 0x0048 //FF
#define CEC_INTR_ENABLE1_REG_ADDR            CEC_BASE + 0x0049 //FF
#define CEC_DATA_REG_ADDR                    CEC_BASE + 0x004c //FF
#define CEC_EOM_REG_ADDR                     CEC_BASE + 0x0050 //01
#define CEC_INTR_STATUS0_REG_ADDR            CEC_BASE + 0x0058
#define CEC_INTR_STATUS1_REG_ADDR            CEC_BASE + 0x0059
#define CEC_NOMINAL_SAMPLE_TIME_0_ADDR       CEC_BASE + 0x005c //FF
#define CEC_NOMINAL_SAMPLE_TIME_1_ADDR       CEC_BASE + 0x005d //FF
#define CEC_NOMINAL_SAMPLE_TIME_2_ADDR       CEC_BASE + 0x005e //FF
#define CEC_NOMINAL_SAMPLE_TIME_3_ADDR       CEC_BASE + 0x005f //FF
#define CEC_HYST_TIME_0_ADDR                 CEC_BASE + 0x0060 //FF
#define CEC_HYST_TIME_1_ADDR                 CEC_BASE + 0x0061 //FF
#define CEC_HYST_TIME_2_ADDR                 CEC_BASE + 0x0062 //FF
#define CEC_HYST_TIME_3_ADDR                 CEC_BASE + 0x0063 //FF
#define CEC_FOLLOWER_ACK_TIME_0_ADDR         CEC_BASE + 0x0064 //FF
#define CEC_FOLLOWER_ACK_TIME_1_ADDR         CEC_BASE + 0x0065 //FF
#define CEC_FOLLOWER_ACK_TIME_2_ADDR         CEC_BASE + 0x0066 //FF
#define CEC_FOLLOWER_ACK_TIME_3_ADDR         CEC_BASE + 0x0067 //FF
#define CEC_RX_BUF_READ_REG_ADDR             CEC_BASE + 0x0068
#define CEC_RX_EOM_READ_REG_ADDR             CEC_BASE + 0x0069
#define CEC_LOGICAL_ADDR0_REG_ADDR           CEC_BASE + 0x006a //1F
#define CEC_LOGICAL_ADDR1_REG_ADDR           CEC_BASE + 0x006b //1F
#define CEC_LOGICAL_ADDR2_REG_ADDR           CEC_BASE + 0x006c //1F
#define CEC_LOGICAL_ADDR3_REG_ADDR           CEC_BASE + 0x006d //1F
#define CEC_LOGICAL_ADDR4_REG_ADDR           CEC_BASE + 0x006e //1F
#define CEC_JITTER_CNT_0_ADDR                CEC_BASE + 0x0070 //FF
#define CEC_JITTER_CNT_1_ADDR                CEC_BASE + 0x0071 //FF
#define CEC_JITTER_CNT_2_ADDR                CEC_BASE + 0x0072 //FF
#define CEC_JITTER_CNT_3_ADDR                CEC_BASE + 0x0073 //FF
#define CEC_LINE_STATUS_REG_ADDR             CEC_BASE + 0x0074
#define CEC_TX_PRESENT_STATE_REG_ADDR        CEC_BASE + 0x0078
#define CEC_RX_PRESENT_STATE_REG_ADDR        CEC_BASE + 0x0079
#define CEC_COLL_CTRL_REG_ADDR               CEC_BASE + 0x007a //03
#define CEC_COLL_WINDOW_TIME_REG_0_ADDR      CEC_BASE + 0x007c //FF
#define CEC_COLL_WINDOW_TIME_REG_1_ADDR      CEC_BASE + 0x007d //FF
#define CEC_COLL_WINDOW_TIME_REG_2_ADDR      CEC_BASE + 0x007e //FF
#define CEC_COLL_WINDOW_TIME_REG_3_ADDR      CEC_BASE + 0x007f //FF
#define CEC_TX_FIFO_FULL_THRESH              CEC_BASE + 0x0080 //0F
#define CEC_TX_FIFO_WPTR                     CEC_BASE + 0x0081
#define CEC_TX_FIFO_RPTR                     CEC_BASE + 0x0082
#define CEC_TX_FIFO_DPTR                     CEC_BASE + 0x0083
#define CEC_RX_FIFO_FULL_THRESH              CEC_BASE + 0x0084 //0F
#define CEC_RX_FIFO_WPTR                     CEC_BASE + 0x0085
#define CEC_RX_FIFO_RPTR                     CEC_BASE + 0x0086
#define CEC_RX_FIFO_DPTR                     CEC_BASE + 0x0087
#define CEC_JITTER_CNT_SB_0                  CEC_BASE + 0x0088 //FF
#define CEC_JITTER_CNT_SB_1                  CEC_BASE + 0x0089 //FF
#define CEC_JITTER_CNT_SB_2                  CEC_BASE + 0x008a //FF
#define CEC_JITTER_CNT_SB_3                  CEC_BASE + 0x008b //FF
#define CEC_ERR_NOTIF_TIME_0                 CEC_BASE + 0x008c //FF
#define CEC_ERR_NOTIF_TIME_1                 CEC_BASE + 0x008d //FF
#define CEC_ERR_NOTIF_TIME_2                 CEC_BASE + 0x008e //FF
#define CEC_ERR_NOTIF_TIME_3                 CEC_BASE + 0x008f //FF
#define CEC_GLITCH_FILT_W_L                  CEC_BASE + 0x0090 //FF
#define CEC_GLITCH_FILT_W_H                  CEC_BASE + 0x0091 //FF

/*-----------------------------------------------------------------------------
 * Enums
 *-----------------------------------------------------------------------------
 */
/* CEC Transmit type */
typedef enum tagBE_CEC_TX_TYPE
{
    BE_CEC_TX_TYPE_FIRST   = 0,
    BE_CEC_TX_TYPE_UNICAST = 0,
    BE_CEC_TX_TYPE_BROADCAST,
    BE_CEC_TX_TYPE_MAX
} BE_CEC_TX_TYPE, *PBE_CEC_TX_TYPE;

/* CEC Mode */
typedef enum tagBE_CEC_MODE
{
    BE_CEC_MODE_FIRST  = 0,
    BE_CEC_MODE_TX     = 0,
    BE_CEC_MODE_RX,
    BE_CEC_MODE_MAX
} BE_CEC_MODE, *PBE_CEC_MODE;

/* CEC status information */
typedef enum tagBE_CEC_STS_INFO
{
    BE_CEC_STS_INFO_NONE = 0x00,

    BE_CEC_STS_TX_FAIL_INFO_NONE,
    // Follower not giving a ACK bit
    BE_CEC_STS_TX_FAIL_NOACK,
    // Follower acked the message
    BE_CEC_STS_TX_ACKED,
    // Transmission of data did not start within time speciifed by response timer
    BE_CEC_STS_TX_FAIL_RESP_TIMEOUT,
    // Collision detected on CEC line
    BE_CEC_STS_TX_FAIL_COLL_DET = 0x08,
    // Signal free time check failed, lost the arbitration
    BE_CEC_STS_TX_FAIL_SIG_FREE_TIME,

    BE_CEC_STS_RX_FAIL_INFO_NONE,
    // High-to-low transition occurred on CEC line after safe sampling period of
    // data bit duration
    BE_CEC_STS_RX_FAIL_TRANS_AFTER_SSP,
    // No reception after waiting for a high-to-low transition on CEC line for data bit
    BE_CEC_STS_RX_FAIL_NO_TRANS_FOR_DATA_BIT,
    // No reception after waiting for a high-to-low transition on CEC line
    // when ACK bit was due to be placed
    BE_CEC_STS_RX_FAIL_NO_TRANS_FOR_ACK_BIT,
    // Data bit total time interval is less than the time specified by CEC spec
    BE_CEC_STS_RX_FAIL_LOW_DB_TIME,
    // Data bit total time interval is more than the time specified by CEC spec
    BE_CEC_STS_RX_FAIL_HIGH_DB_TIME
} BE_CEC_STS_INFO, *PBE_CEC_STS_INFO;

/*-----------------------------------------------------------------------------
 * Structures
 *-----------------------------------------------------------------------------
 */
typedef struct tagBE_CEC_TX_DATA
{
    BE_CEC_TX_TYPE      txType;
    UINT8               retxCondn;
    UINT8               retxCount;
    UINT8               txDataLen;
    UINT8               txDataBuf[16];
    UINT32              txTime;
} BE_CEC_TX_DATA, *PBE_CEC_TX_DATA;

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Loads default values of CEC block to registers
 * PARAMS   : *vpp_obj - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_LoadDefaultVal(VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION : Sets/Resets transmit/receive mode
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : cecMode  - CEC mode (Tx/Rx)
 *          : enable   - 1, to enable and 0 to disable
 *          : bufWrite - Flag to indicate if data has to be written to
 *          :          - BCM buffer or to register immediately
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_SetMode (VPP_OBJ *vpp_obj, BE_CEC_MODE cecMode,
                    BOOL enable, BOOL bufWrite);

/******************************************************************************
 * FUNCTION : Give Rx Line Status (added for polling in case of Sig free Time Error)
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : *state   - Line State (1 is idle)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_RxLineStatus(VPP_OBJ *vpp_obj, UINT8 *state);
/******************************************************************************
 * FUNCTION : Sends data to the transmit FIFO and initiates transfer on the
 *          : CEC line, It also monitors for tx errors and handles
 *          : retransmission if specified
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : pTxData  - Pointer to transmit data structure
 *          : initTx   - First attempt in transfer of a message or retransmit
 *          : attempt
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_TransmitData(VPP_OBJ *vpp_obj, PBE_CEC_TX_DATA pTxData, BOOL initTx);

/******************************************************************************
 * FUNCTION : Reads data from the receive FIFO (Data is read until the FIFO
 *          : becomes empty)
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : pDataLen - Pointer to return data length read (Output)
 *          : pDataBuf - Pointer to data buffer (Output)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_ReceiveData(VPP_OBJ *vpp_obj, UINT8 *pDataLen, UINT8 *pDataBuf);

/******************************************************************************
 * FUNCTION : Handles CEC interrupt
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : pIntrField   - Pointer to return interrupt field (Output)
 *          : pIntrDataLen - Pointer to return interrupt data length (Output)
 *          : pIntrData    - Pointer to interrupt data (Output)
 *          : pTxData      - Pointer to transmit data in case it is required
 *          : for a retransmission attempt
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_HandleInterrupt(VPP_OBJ *vpp_obj, UINT16 *pIntrField,
                           UINT8 *pIntrDataLen, UINT8 *pIntrData,
                           PBE_CEC_TX_DATA pTxData,UINT16 reg);
/******************************************************************************
 * FUNCTION : Controls logical address at given index
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : enable       - TRUE/FALSE to enable/disable logical address
 *          : logAddr      - Logical address (if disabled, it is forced to 0)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_EnableLogAddr (VPP_OBJ *vpp_obj,
                          BOOL enable, UINT8 addrIndex, UINT8 logAddr);

#endif // __VPP_BE_CEC_H__
