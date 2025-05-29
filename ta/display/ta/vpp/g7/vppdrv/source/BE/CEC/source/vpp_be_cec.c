/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _VPP_BE_CEC_C_
#define _VPP_BE_CEC_C_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_bcmbuf.h"
#include "galois_io.h"

#include "hdmi_cmn.h"
#include "vpp_be_cec.h"
#include "vpp_be_cec_prv.h"
#include "vpp_debug.h"

#include "galois_hw_init.h"
#include "galois_speed.h"
#include "vpp_apifuncs.h"
#ifndef CONFIG_AMP_TEE_ENABLE
#include <sys/ioctl.h>
#endif

extern int berlin_pe_dev;
extern int berlin_cec_dev;
/******************************************************************************
 * FUNCTION : Loads default values of CEC block to registers
 * PARAMS   : *vpp_obj - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_LoadDefaultVal(VPP_OBJ *vpp_obj)
{
    UINT32  reg = 0;
    BOOL    bufWrite = FALSE;

    int     i;
    // Write signal free time count
    CEC_CalcTimerCount (CEC_SIGNAL_FREE_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_SIGNAL_FREE_TIME_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write start bit low duration
    CEC_CalcTimerCount (CEC_START_BIT_0_LOW_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_START_BIT_LO_THRESH_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write start bit total duration
    CEC_CalcTimerCount ((CEC_START_BIT_0_LOW_TIME + CEC_START_BIT_0_HI_TIME), &reg);
    CEC_RegWrite (vpp_obj, CEC_START_BIT_HI_THRESH_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write data bit 0 low duration
    CEC_CalcTimerCount (CEC_DATA_BIT_0_LOW_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_DATA_BIT_0_LO_THRESH_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write data bit 1 low duration
    CEC_CalcTimerCount (CEC_DATA_BIT_1_LOW_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_DATA_BIT_1_LO_THRESH_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write data bit 0 total duration
    CEC_CalcTimerCount ((CEC_DATA_BIT_0_LOW_TIME + CEC_DATA_BIT_0_HI_TIME), &reg);
    CEC_RegWrite (vpp_obj, CEC_DATA_BIT_0_HI_THRESH_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write data bit 1 total duration
    CEC_CalcTimerCount ((CEC_DATA_BIT_1_LOW_TIME + CEC_DATA_BIT_1_HI_TIME), &reg);
    CEC_RegWrite (vpp_obj, CEC_DATA_BIT_1_HI_THRESH_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write safe sampling time for ACK
    CEC_CalcTimerCount (CEC_SSP_ACK_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_SSP_ACK_TIME_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write nominal sampling time for data bit
    CEC_CalcTimerCount (CEC_NOMINAL_SAMPLE_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_NOMINAL_SAMPLE_TIME_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write hysterisis time count
    CEC_CalcTimerCount (CEC_FOLL_HYST_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_HYST_TIME_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write follower ack time count
    CEC_CalcTimerCount (CEC_FOLL_ACK_ASSRT_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_FOLLOWER_ACK_TIME_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write collision window time count
    CEC_CalcTimerCount (CEC_COLL_WINDOW_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_COLL_WINDOW_TIME_REG_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Enable collision detection
    reg = 0x01;
    CEC_RegWrite (vpp_obj, CEC_COLL_CTRL_REG_ADDR, (UINT8*)(&reg), 1, bufWrite);

    // Write start bit jitter count
    CEC_CalcTimerCount (CEC_STARTBIT_JITTER_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_JITTER_CNT_SB_0, (UINT8*)(&reg), 4, bufWrite);

    // Write data bit jitter count
    CEC_CalcTimerCount (CEC_DATABIT_JITTER_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_JITTER_CNT_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Write error notification time
    CEC_CalcTimerCount (CEC_ERR_GEN_TIME, &reg);
    CEC_RegWrite (vpp_obj, CEC_ERR_NOTIF_TIME_0, (UINT8*)(&reg), 4, bufWrite);

    /* Changed to avoid bit period long error. JIRA:CSMLGUKAO-569/CSMUBQ-124 */
    reg = 0x0F;
    CEC_RegWrite (vpp_obj, CEC_GLITCH_FILT_W_L, (UINT8*)(&reg),1,bufWrite);

    // Disable all logical addresses
    for (i = 0; i < 5; i++)
        BE_CEC_EnableLogAddr(vpp_obj,FALSE,i,0);

    // Disable PMode
    reg = 0x00;
    CEC_RegWrite (vpp_obj, CEC_PMODE_ADDR, (UINT8*)(&reg),1,bufWrite);

    // Clear all interrupts and enable required ones
    CEC_EnableInterrupt (vpp_obj, BE_CEC_INTR_ALL, FALSE, bufWrite);
    CEC_EnableInterrupt (vpp_obj, (BE_CEC_INTR_TX_FAIL | BE_CEC_INTR_TX_COMPLETE | BE_CEC_INTR_RX_COMPLETE | BE_CEC_INTR_RX_FAIL), TRUE, bufWrite);

    // Disable transmit mode and Enable receive mode
    BE_CEC_SetMode (vpp_obj, BE_CEC_MODE_TX, FALSE, bufWrite);
    BE_CEC_SetMode (vpp_obj, BE_CEC_MODE_RX, TRUE, bufWrite);
    return (MV_VPP_OK);
}

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
                    BOOL enable, BOOL bufWrite)
{
    UINT8 reg;

    if (cecMode > BE_CEC_MODE_MAX)
    {
        return (MV_VPP_EBADPARAM);
    }

    if (cecMode == BE_CEC_MODE_TX)
    {
        if (enable)
        {
            // Reset Tx FIFO
            reg = 0x00;
            CEC_RegWrite (vpp_obj, CEC_TX_FIFO_RESET_ADDR, &reg, 1, bufWrite);

            reg = 0x01;
            CEC_RegWrite (vpp_obj, CEC_TX_FIFO_RESET_ADDR, &reg, 1, bufWrite);

            // Enable required interrupts
            CEC_EnableInterrupt (vpp_obj, (BE_CEC_INTR_TX_FAIL | BE_CEC_INTR_TX_COMPLETE), FALSE, bufWrite);
            CEC_EnableInterrupt (vpp_obj, (BE_CEC_INTR_TX_FAIL | BE_CEC_INTR_TX_COMPLETE), TRUE, bufWrite);
        }
        else
        {
            reg = 0x00;

            // Disable CEC_RDY
            CEC_RegWrite (vpp_obj, CEC_RDY_ADDR, &reg, 1, bufWrite);

        }
    }
    else if (cecMode == BE_CEC_MODE_RX)
    {
        if (enable)
        {
            // Reset Rx FIFO
            reg = 0x00;
            CEC_RegWrite (vpp_obj, CEC_RX_FIFO_RESET_ADDR, &reg, 1, bufWrite);

            reg = 0x01;
            CEC_RegWrite (vpp_obj, CEC_RX_FIFO_RESET_ADDR, &reg, 1, bufWrite);

            // Enable Rx Ready
            reg = 0x01;
            CEC_RegWrite (vpp_obj, CEC_RX_RDY_ADDR, &reg, 1, bufWrite);

            // Enable required interrupts
            CEC_EnableInterrupt (vpp_obj, (BE_CEC_INTR_RX_FAIL | BE_CEC_INTR_RX_COMPLETE), FALSE, bufWrite);
            CEC_EnableInterrupt (vpp_obj, (BE_CEC_INTR_RX_FAIL | BE_CEC_INTR_RX_COMPLETE), TRUE, bufWrite);
        }
        else
        {
            // Disable Rx Ready
            reg = 0x00;
            CEC_RegWrite (vpp_obj, CEC_RX_RDY_ADDR, &reg, 1, bufWrite);
        }
    }
    return (MV_VPP_OK);
}

INT BE_CEC_RxLineStatus(VPP_OBJ *vpp_obj,UINT8 *state)
{
    UINT8 value;
    CEC_RegRead (vpp_obj, CEC_RX_PRESENT_STATE_REG_ADDR , &value, 1);
    *state=value;
    return 0;
}
/******************************************************************************
 * FUNCTION : Sends data to the transmit FIFO and initiates transfer on the
 *          : CEC line, It also monitors for tx errors and handles
 *          : retransmission if specified
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : pTxData  - Pointer to transmit data structure
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_TransmitData(VPP_OBJ *vpp_obj, PBE_CEC_TX_DATA pTxData, BOOL initTx)
{
    UINT32  reg = 0;
    UINT32  signalFreeTime;
    BOOL    bufWrite = FALSE;
    INT     i;

    if (pTxData == NULL)
        return (MV_VPP_EBADPARAM);

    if (initTx /* First time transfer of a message */)
    {
        if (pTxData->txDataLen > CEC_MAX_MSG_LEN)
            return (MV_VPP_EBADPARAM);

        signalFreeTime = (9 * CEC_DATA_BIT_TIME);
    }
    else
    {
        // Retransmission attempt
        // Time changed for CTS 9.6
        signalFreeTime = (7 * CEC_DATA_BIT_TIME);
    }
    // Set signal free time
    CEC_CalcTimerCount (signalFreeTime, &reg);
    CEC_RegWrite (vpp_obj, CEC_SIGNAL_FREE_TIME_0_ADDR, (UINT8*)(&reg), 4, bufWrite);

    // Set mode to transmit
    BE_CEC_SetMode (vpp_obj, BE_CEC_MODE_TX, TRUE, bufWrite);

    // Set transmit type
    reg = pTxData->txType;
    CEC_RegWrite (vpp_obj, CEC_TX_TYPE_ADDR, (UINT8*)&reg, 1, bufWrite);
    // Write data to FIFO
    for (i = 0; i < pTxData->txDataLen; i++)
    {
        // Write into data register
        CEC_RegWrite (vpp_obj, CEC_DATA_REG_ADDR, &pTxData->txDataBuf[i], 1, bufWrite);

        // If last block, set EOM
        reg = (i == pTxData->txDataLen-1)?0x01:0x00;
        CEC_RegWrite (vpp_obj, CEC_EOM_REG_ADDR, (UINT8*)&reg, 1, bufWrite);

        // Push data to FIFO
        reg = 0x00;
        CEC_RegWrite (vpp_obj, CEC_TOGGLE_FOR_WRITE_REG_ADDR, (UINT8*)&reg, 1, bufWrite);
   }

    // Set initiator ready register
    reg = 0x01;
    CEC_RegWrite (vpp_obj, CEC_RDY_ADDR, (UINT8*)&reg, 1, bufWrite);
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Reads data from the receive FIFO (Data is read until the FIFO
 *          : becomes empty)
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : pDataLen - Pointer to return data length read (Output)
 *          : pDataBuf - Pointer to data buffer (Output)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_ReceiveData(VPP_OBJ *vpp_obj, UINT8 *pDataLen, UINT8 *pDataBuf)
{
#if defined(VPP_DIAGS)
    UINT8   reg,eom_value=0,dptr_len=0;
    UINT8   fifoStatus = 0;
    BOOL    bufWrite = FALSE;
#endif
    VPP_CEC_RX_MSG_BUF rx_buf;
    HRESULT rc = MV_VPP_OK;
    int i;
    if (pDataBuf == NULL)
        return (MV_VPP_EBADPARAM);

    VPP_Memset(&rx_buf, 0, sizeof(VPP_CEC_RX_MSG_BUF));
    *pDataLen = 0;

#ifndef VPP_DIAGS

#ifndef CONFIG_AMP_TEE_ENABLE
    if (berlin_cec_dev >= 0)
        rc=ioctl(berlin_cec_dev, CEC_IOCTL_RX_MSG_BUF, &rx_buf);
    else
        rc = E_FILEREAD;
#else
    rc = VPP_Receive_CECData(&rx_buf);
#endif
    if(rc!=MV_VPP_OK) return (MV_VPP_EBADPARAM);
    *pDataLen=rx_buf.len;
    for(i=0;i<*pDataLen;i++)
    {
        *pDataBuf++=rx_buf.buf[i];
    }
    if (*pDataLen > CEC_MAX_MSG_LEN){
                VPPHDMI_DBG_INFO("RX msg failed recv len exceed\n");
            return (MV_VPP_EIOFAIL);
        }

#else
    // Read FIFO status register
    CEC_RegRead (vpp_obj, CEC_INTR_STATUS1_REG_ADDR, &fifoStatus, 1);

    // Read into dataBuf until the RX FIFO becomes empty
    while (IS_BIT_SET(fifoStatus,CEC_RX_FIFO_EMPTY) == FALSE)
    {
        CEC_RegRead (vpp_obj, CEC_RX_BUF_READ_REG_ADDR, pDataBuf, 1);

        reg = 0x01;
        CEC_RegWrite (vpp_obj, CEC_TOGGLE_FOR_READ_REG_ADDR, &reg, 1, bufWrite);
        CEC_RegRead (vpp_obj, CEC_INTR_STATUS1_REG_ADDR, &fifoStatus, 1);
        pDataBuf++;
        (*pDataLen)++;
        if (*pDataLen > CEC_MAX_MSG_LEN)
            return (MV_VPP_EIOFAIL);
    }
#endif
    return (MV_VPP_OK);
}

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
                           PBE_CEC_TX_DATA pTxData,UINT16 reg)
{
    BE_CEC_STS_INFO     stsInfo;
    BOOL                bufWrite = FALSE;
    INT                 retCode = MV_VPP_OK;
    stsInfo = BE_CEC_STS_INFO_NONE;
    *pIntrField   = BE_CEC_INTR_NONE;
    *pIntrDataLen = 0;

    if (reg & BE_CEC_INTR_TX_FAIL)
    {
        // Read failure code
        CEC_GetFailStatus(vpp_obj, BE_CEC_MODE_TX, &stsInfo, reg);
        if(stsInfo != BE_CEC_STS_TX_FAIL_SIG_FREE_TIME)
        {
            // Check for retransmission
            if ((pTxData->retxCondn & stsInfo) && (pTxData->retxCount > 0))
            {
                retCode = BE_CEC_TransmitData (vpp_obj, pTxData, FALSE);
                pTxData->retxCount --;
            }
            else
            {
                // Fill interrupt status information for service
                *pIntrField = BE_CEC_INTR_TX_FAIL;
                *pIntrDataLen = 3;
                pIntrData[0] = stsInfo;
                pIntrData[1] = pTxData->txDataBuf[0];
                pIntrData[2] = pTxData->txDataBuf[1];
                // Transmission of this message has failed
                // GaloisMemClear (pTxData, sizeof(BE_CEC_TX_DATA));

            }
        }
        else
            {
            // ISR will handle in case of signal free time error
                // Fill interrupt status information for service
                *pIntrField = BE_CEC_INTR_TX_FAIL;
                *pIntrDataLen = 3;
                pIntrData[0] = stsInfo;
                pIntrData[1] = pTxData->txDataBuf[0];
                pIntrData[2] = pTxData->txDataBuf[1];
        }
    }

    else if (reg & BE_CEC_INTR_TX_COMPLETE)
    {
        // Fill interrupt status information for service
        *pIntrField   = BE_CEC_INTR_TX_COMPLETE;
        *pIntrDataLen = 3;
        pIntrData[0] = BE_CEC_STS_TX_ACKED;
        pIntrData[1] = pTxData->txDataBuf[0];
        pIntrData[2] = pTxData->txDataBuf[1];

        // Clear local buffer
        // GaloisMemClear (pTxData, sizeof(BE_CEC_TX_DATA));

        // Disable tx mode
        BE_CEC_SetMode (vpp_obj, BE_CEC_MODE_TX, FALSE, bufWrite);
    }

    else if (reg & BE_CEC_INTR_RX_COMPLETE)
    {
        stsInfo = BE_CEC_STS_RX_FAIL_INFO_NONE;

        // Read Data
        retCode = BE_CEC_ReceiveData (vpp_obj, pIntrDataLen, pIntrData);
        if (retCode != MV_VPP_OK)
            *pIntrField   = BE_CEC_INTR_RX_FAIL;
        else
            *pIntrField   = BE_CEC_INTR_RX_COMPLETE;
    }

    else if (reg & BE_CEC_INTR_RX_FAIL)
    {
        // Read failure code
        CEC_GetFailStatus(vpp_obj, BE_CEC_MODE_RX, &stsInfo, reg);

        // Fill interrupt status information for service
        *pIntrField   = BE_CEC_INTR_RX_FAIL;
        *pIntrDataLen = 1;
        pIntrData[0] = stsInfo;

        // Disable and reenable Rx mode
        BE_CEC_SetMode (vpp_obj, BE_CEC_MODE_RX, FALSE, bufWrite);
        BE_CEC_SetMode (vpp_obj, BE_CEC_MODE_RX, TRUE, bufWrite);
    }

    return (retCode);
}

/******************************************************************************
 * FUNCTION : Controls logical address at given index
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : enable       - TRUE/FALSE to enable/disable logical address
 *          : logAddr      - Logical address (if disabled, it is forced to 0)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
INT BE_CEC_EnableLogAddr (VPP_OBJ *vpp_obj,
                          BOOL enable, UINT8 addrIndex, UINT8 logAddr)
{
    UINT32  regAddr;

    switch (addrIndex)
    {
        case 0: regAddr = CEC_LOGICAL_ADDR0_REG_ADDR; break;
        case 1: regAddr = CEC_LOGICAL_ADDR1_REG_ADDR; break;
        case 2: regAddr = CEC_LOGICAL_ADDR2_REG_ADDR; break;
        case 3: regAddr = CEC_LOGICAL_ADDR3_REG_ADDR; break;
        case 4: regAddr = CEC_LOGICAL_ADDR4_REG_ADDR; break;
        default: return (MV_VPP_EBADPARAM);
    }

    if (enable)
    {
        logAddr |= (1 << 4);
        CEC_RegWrite (vpp_obj, regAddr, (UINT8*)(&logAddr),1,FALSE);
    }
    else
    {
        logAddr = 0;
        CEC_RegWrite (vpp_obj, regAddr, (UINT8*)(&logAddr),1,FALSE);
    }
    return (MV_VPP_OK);
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////LOCAL STATIC FUNCTIONS////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 * FUNCTION : Calculates timer count based on block clock (in MHz)
 * PARAMS   : timeVal    - Time value in microseconds
 *          : timerCount - Pointer to return timer count (Output)
 * RETURN   : MV_VPP_OK
 *****************************************************************************/
static INT CEC_CalcTimerCount (UINT32 timeVal, UINT32 *pTimerCount)
{
    // Clock frequency has to be in MHz
    int clkFreq = 25;//GaloisGetFrequency(SOC_FREQ_CFG);

    if (clkFreq > 100) {
        return (MV_VPP_EBADPARAM);
    }
    *pTimerCount = (timeVal * clkFreq);
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Enables/Disables the given interrupt(s)
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : intr     - Interrupt(s) to be enabled/disabled
 *          : enable   - 1, to enable and 0 to disable
 *          : bufWrite - Flag to indicate if data has to be written to
 *          :          - BCM buffer or to register immediately
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_EnableInterrupt (VPP_OBJ *vpp_obj, UINT16 intr,
                                BOOL enable, BOOL bufWrite)
{
    UINT8 reg;

    CEC_RegRead (vpp_obj, CEC_INTR_ENABLE0_REG_ADDR, &reg, 1);
    if (enable)
    {
        CEC_RegRead (vpp_obj, CEC_INTR_ENABLE0_REG_ADDR, &reg, 1);
        reg |= (intr & 0x00ff);
        CEC_RegWrite(vpp_obj,CEC_INTR_ENABLE0_REG_ADDR,&reg,1,bufWrite);
        CEC_RegRead (vpp_obj, CEC_INTR_ENABLE1_REG_ADDR, &reg, 1);
        reg |= ((intr>>8) & 0x00ff);
        CEC_RegWrite(vpp_obj,CEC_INTR_ENABLE1_REG_ADDR,&reg,1,bufWrite);
    }
    else
    {
        CEC_RegRead (vpp_obj, CEC_INTR_ENABLE0_REG_ADDR, &reg, 1);
        reg &= ~(intr & 0x00ff);
        CEC_RegWrite(vpp_obj,CEC_INTR_ENABLE0_REG_ADDR,&reg,1,bufWrite);
        CEC_RegRead (vpp_obj, CEC_INTR_ENABLE1_REG_ADDR, &reg, 1);
        reg &= ~((intr>>8) & 0x00ff);
        CEC_RegWrite(vpp_obj,CEC_INTR_ENABLE1_REG_ADDR,&reg,1,bufWrite);
    }
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Enables/Disables the given interrupt(s)
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : failMode - Failure mode (Tx or Rx mode)
 *          : *pStsInfo- Pointer to return status information
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_GetFailStatus (VPP_OBJ *vpp_obj,
                              BE_CEC_MODE failMode, BE_CEC_STS_INFO *pStsInfo, UINT16 intr_status)
{
    if (failMode == BE_CEC_MODE_TX)
    {
        if (intr_status & (BE_CEC_INTR_TX_UNICAST_NOACK | BE_CEC_INTR_TX_BRDCAST_NOACK))
        {
            *pStsInfo = BE_CEC_STS_TX_FAIL_NOACK;
        }
        else if (intr_status & BE_CEC_INTR_TX_COLLISION)
        {
            *pStsInfo = BE_CEC_STS_TX_FAIL_COLL_DET;
        }
        else if (intr_status & BE_CEC_INTR_TX_SFT_FAIL)
        {
            *pStsInfo = BE_CEC_STS_TX_FAIL_SIG_FREE_TIME;
        }
        else
            *pStsInfo = BE_CEC_STS_TX_FAIL_INFO_NONE;
    }
    else if(failMode ==  BE_CEC_MODE_RX)
    {
        if (intr_status & BE_CEC_INTR_RX_LOW_DT_ERROR)
        {
            *pStsInfo = BE_CEC_STS_RX_FAIL_LOW_DB_TIME;
        }
        else if (intr_status & BE_CEC_INTR_RX_HIGH_DT_ERROR)
        {
            *pStsInfo = BE_CEC_STS_RX_FAIL_HIGH_DB_TIME;
        }
        else
            *pStsInfo = BE_CEC_STS_RX_FAIL_INFO_NONE;
    }
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Writes block of data starting at specified register address
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : pDataBuf - Pointer to data buffer
 *          : length   - Length of data to be written
 *          : bufWrite - Flag to indicate if data has to be written to
 *          :          - BCM buffer or to register immediately
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_RegWrite (VPP_OBJ *vpp_obj,
                         UINT32 startAddr, UINT8 *pDataBuf,
                         UINT32 length, BOOL bufWrite)
{
    UINT32  i;
    UINT32  wrAddr;
    INT     retCode = MV_VPP_OK;
    wrAddr = SOC_SM_CEC_BASE + (startAddr << 2);
    for (i = 0; i < length; i++)
    {
        if (bufWrite)
        {
            if(MV_VPP_OK != (retCode = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, wrAddr + (4*i), *pDataBuf)))
                return retCode;
        }
        else
        {
            GA_REG_BYTE_WRITE((wrAddr + 4*i),*pDataBuf);
        }
        pDataBuf++;
    }
    return (retCode);
}

/******************************************************************************
 * FUNCTION : Reads block of data starting at specified register address
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : pDataBuf - Pointer to data buffer (Output)
 *          : length   - Length of data to be read
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_RegRead (VPP_OBJ *vpp_obj,
                        UINT32 startAddr, UINT8 *pDataBuf, UINT32 length)
{
    UINT32  i;
    UINT32  rdAddr;
    rdAddr = SOC_SM_CEC_BASE + (startAddr << 2);
    for (i = 0; i < length; i++)
    {
         GA_REG_BYTE_READ (rdAddr+4*i, &pDataBuf[i]);
    }
    return (MV_VPP_OK);
}

#endif /* _VPP_BE_CEC_C_ */
