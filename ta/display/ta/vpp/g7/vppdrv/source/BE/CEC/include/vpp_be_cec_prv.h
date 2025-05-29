/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef __VPP_BE_CEC_PRV_H__
#define __VPP_BE_CEC_PRV_H__

#include "com_type.h"

/*-----------------------------------------------------------------------------
 * Macros and Constants
 *-----------------------------------------------------------------------------
 */
#define CEC_MAX_MSG_LEN     16
#define SOC_SM_CEC_BASE            0xF7FE1000
// Time in microseconds
#define CEC_RESPONSE_TIME                   200000
#define CEC_SIGNAL_FREE_TIME                12000
#define CEC_START_BIT_0_LOW_TIME            3700
#define CEC_START_BIT_0_HI_TIME             800
#define CEC_DATA_BIT_0_LOW_TIME             1500
#define CEC_DATA_BIT_0_HI_TIME              900
#define CEC_DATA_BIT_1_LOW_TIME             600
#define CEC_DATA_BIT_1_HI_TIME              1800
#define CEC_SSP_ACK_TIME                    1050
#define CEC_NOMINAL_SAMPLE_TIME             1050
#define CEC_FOLL_ACK_ASSRT_TIME             1500

#define CEC_FOLL_HYST_TIME                  800
#define CEC_STARTBIT_JITTER_TIME            205
#define CEC_DATABIT_JITTER_TIME             357
#define CEC_ERR_GEN_TIME                    3600 // 1.5 times data bit time

#define CEC_COLL_WINDOW_TIME                250
#define CEC_START_BIT_TIME                  (CEC_START_BIT_0_LOW_TIME + CEC_START_BIT_0_HI_TIME)
#define CEC_DATA_BIT_TIME                   (CEC_DATA_BIT_0_LOW_TIME  + CEC_DATA_BIT_0_HI_TIME)

// CEC FIFO Status
#define CEC_TX_FIFO_FULL                    0
#define CEC_TX_FIFO_EMPTY                   1
#define CEC_RX_FIFO_FULL                    2
#define CEC_RX_FIFO_EMPTY                   3


/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : Calculates timer count based on block clock (in MHz)
 * PARAMS   : timeVal    - Time value in microseconds
 *          : timerCount - Pointer to return timer count (Output)
 * RETURN   : MV_VPP_OK
 *****************************************************************************/
static INT CEC_CalcTimerCount (UINT32 timeVal, UINT32 *pTimerCount);

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
                                BOOL enable, BOOL bufWrite);

/******************************************************************************
 * FUNCTION : Enables/Disables the given interrupt(s)
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : failMode - Failure mode (Tx or Rx mode)
 *          : *pStsInfo- Pointer to return status information
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_GetFailStatus (VPP_OBJ *vpp_obj,
                              BE_CEC_MODE failMode, BE_CEC_STS_INFO *pStsInfo, UINT16 intr_status);

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
                         UINT32 length, BOOL bufWrite);

/******************************************************************************
 * FUNCTION : Reads block of data starting at specified register address
 * PARAMS   : *vpp_obj - Pointer to VPP object
 *          : pDataBuf - Pointer to data buffer (Output)
 *          : length   - Length of data to be read
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static INT CEC_RegRead (VPP_OBJ *vpp_obj,
                         UINT32 startAddr, UINT8 *pDataBuf, UINT32 length);
#endif // __VPP_BE_CEC_PRV_H__
