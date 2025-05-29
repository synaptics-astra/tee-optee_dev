/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPP_BE_CEC_SRV_H__
#define __VPP_BE_CEC_SRV_H__

#include "com_type.h"
#include "vpp_api_types.h"
#include "hdmi_cmn.h"
#include "vpp_be_cec.h"
#include "vpp_be_cec_msg.h"

/* CEC internal states */
typedef enum CEC_STATE_T
{
    CEC_STATE_IDLE = 0x00,
    CEC_STATE_UNALLOC,
    CEC_STATE_ACTIVE,
    CEC_STATE_MAX,
}CEC_STATE;

/* CEC service data store */
typedef struct CEC_SRV_DATA_T
{
    MV_OSAL_HANDLE_TASK_t   tskHandle;
    MV_OSAL_HANDLE_SEM_t    fifoSem;
    MV_OSAL_HANDLE_MUTEX_t  fifoMutex;
    MV_HANDLE_Timer_t       Rx_LinePollhTimer;
    MV_HANDLE_Timer_t       Rx_LinePollhTimer2;
    HDMI_FIFO               cmdFifo;
    HDMI_FIFO               isrFifo;

    // Address
    UINT16                  phyAddr;
    UINT8                   logAddr[VPP_CEC_DEVICE_MAX];

    // State (device corresponds to logical device here)
    CEC_STATE               curState[VPP_CEC_DEVICE_MAX];

    // Application callback settings
    MV_VPP_EVENT_CALLBACK   appCbFn;
    UINT32                  eventCode;
    void                    *contextParam;

    // Message table
    BE_CEC_MSG_DATA         msgTable[BE_CEC_MAX_MSGS];

    // Local transmit buffer for driver
    BE_CEC_TX_DATA          txData;
    UINT32                  txTime;
}CEC_SRV_DATA, *PCEC_SRV_DATA;

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * FUNCTION : HDMI Cec Task
 * PARAMS   : *arg        - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int CEC_Task (void *arg);

/******************************************************************************
 * FUNCTION : Initialize CEC
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 * x        : Error code, otherwise
 ******************************************************************************/
int VPP_BE_CEC_Init (VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION : Terminate CEC
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 * x        : Error code, otherwise
 ******************************************************************************/
int VPP_BE_CEC_Term (VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION : Enable/Disable CEC
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : enable       - Enable/Disable CEC
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_Enable (VPP_OBJ *vpp_obj, BOOL enable);

/******************************************************************************
 * FUNCTION : Register callback to receive CEC event notifications
 * PARAMS   : *vpp_obj      - Pointer to VPP object
 *          : eventCode     - Event code for which notifications has to be raised
 *          : cbFnAddr      - Address of callback function
 *          : contextParam  - Context parameter for the registering application
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_RegisterCB (VPP_OBJ *vpp_obj, UINT32 eventCode,
                           UINT32 cbFnAddr, UINT32 contextParam);

/******************************************************************************
 * FUNCTION : Unregister for CEC event notifications
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_UnregisterCB (VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION : Interrupt service routine for CEC interrupt
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
void VPP_BE_CEC_ISRService(VPP_OBJ *vpp_obj,UINT16 reg);
/******************************************************************************
 * FUNCTION : Deferred service routine for CEC interrupt to signal the waiting
 *          : task on the interrupt
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
void VPP_BE_CEC_DSR_Service(VPP_OBJ *vpp_obj);

/******************************************************************************
 * FUNCTION : Sets physical address - based on the given physical address, cec
 *          : command is generated and queued
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : phyAddr      - Physical address to be configured
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_SetPhysicalAddr (VPP_OBJ *vpp_obj, int phyAddr);

/******************************************************************************
 * FUNCTION : Allocate/Unallocate logical address for the given device type
 * PARAMS   : *vpp_obj          - Pointer to VPP object
 *          : alloc             - Allocate/Unallocate logical address
 *          : deviceType        - Device type
 *          : initialPollAddr   - Initial polling address
 *          :                   - -1, if default needs to be used
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_AllocLogicalAddr (VPP_OBJ *vpp_obj, BOOL alloc,
                                 VPP_CEC_DEVICE_TYPE deviceType,
                                 int initialPollAddr);

/******************************************************************************
 * FUNCTION : Initiates transmission of given message on CEC line
 * PARAMS   : *vpp_obj  - Pointer to VPP object
 *          : pCecMsg   - Pointer to CEC message (Input)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_SendMessage (VPP_OBJ *vpp_obj, PVPP_CEC_MSG pCecMsg);

#endif // __VPP_BE_CEC_SRV_H__
