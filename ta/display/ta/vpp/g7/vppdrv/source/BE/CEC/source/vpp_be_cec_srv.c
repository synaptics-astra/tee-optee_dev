/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#define _VPP_BE_CEC_SRV_C_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "galois_io.h"
#include "apb_perf_base.h"
#include "gpio.h"
#include "vpp_be_cec.h"
#include "vpp_be_cec_msg.h"
#include "vpp_be_cec_srv.h"
#include "vpp_be_cec_srv_prv.h"

#if !defined(VPP_DIAGS)
#ifndef CONFIG_AMP_TEE_ENABLE
#include "OSAL_api.h"
#else
#include "tee_internal_api.h"
#include "tee_ta_osal.h"
#include "tee_ta_vpp_be.h"
#endif
#endif

#if !defined(TZ_3_0)
#define VPP_TA_CEC_ENABLE_INTR
#endif

#define CEC_SEM_POST(sem)       MV_OSAL_Sem_Post (sem)
#define CEC_SEM_WAIT(sem)       MV_OSAL_Sem_Wait (sem)
#define CEC_LOCK_MUTEX(mutex)   MV_OSAL_Mutex_Lock(mutex)
#define CEC_UNLOCK_MUTEX(mutex) MV_OSAL_Mutex_Unlock(mutex)

CEC_CMD         g_cecCmd; // to store ccCmd for Timer

/******** static function declaration *************/
static int CEC_Init (VPP_OBJ *vpp_obj);
static int CEC_Term (VPP_OBJ *vpp_obj);
static int CEC_SetPhyAddr (VPP_OBJ *vpp_obj, BOOL setAddr, int phyAddr);
static int CEC_AllocLogAddr (VPP_OBJ *vpp_obj,VPP_CEC_DEVICE_TYPE deviceType,
                            int startIndex, int curIndex);
static int CEC_SetLogAddr (VPP_OBJ *vpp_obj, BOOL setAddr,
                           VPP_CEC_DEVICE_TYPE deviceType, int logAddr);
static int CEC_NotifyEvent (VPP_OBJ *vpp_obj, VPP_HDMI_CEC_EVT_INFO *pEvtInfo);
static VPP_CEC_DEVICE_TYPE CEC_GetDeviceTypeFromAddr (int logAddr);
static int CEC_SetState (VPP_OBJ *vpp_obj,
                         VPP_CEC_DEVICE_TYPE deviceType, CEC_STATE cecState);
static int CEC_GetState (VPP_OBJ *vpp_obj,
                         VPP_CEC_DEVICE_TYPE deviceType, CEC_STATE *pCecState);

static unsigned int GetSystemTimeMS(void)
{
    unsigned int time_in_ms = 0;
    TEE_Time start_time;

    TEE_GetSystemTime(&start_time);
    time_in_ms = start_time.seconds * 1000 + start_time.millis;
    return time_in_ms;
}

/******************************************************************************
 * FUNCTION : Initialize CEC
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_Init (VPP_OBJ *vpp_obj)
{
    int             retVal;

    if (vpp_obj->be.hdmiCec == 0)
    {
        // Init HDMI CEC - first time init
        retVal = CEC_Init (vpp_obj);
        if (retVal != MV_VPP_OK)
        {
            return retVal;
        }
    }

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Terminate CEC
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_Term (VPP_OBJ *vpp_obj)
{

    CEC_Term(vpp_obj);

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Enable/Disable CEC
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : enable       - Enable/Disable CEC
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_Enable (VPP_OBJ *vpp_obj, BOOL enable)
{
    CEC_SRV_DATA    *pCecSrv;
    int             retVal;

    if (enable)
    {
        if (vpp_obj->be.hdmiCec == 0)
        {
            // Init HDMI CEC - first time init
            retVal = CEC_Init (vpp_obj);
            if (retVal != MV_VPP_OK)
            {
                return retVal;
            }
        }
        /* Reset data store */
        pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
        HdmiFifoReset (&pCecSrv->isrFifo);
        HdmiFifoReset (&pCecSrv->cmdFifo);

        /* Reset Hdmi Cec   */
        BE_CEC_LoadDefaultVal(vpp_obj);

        /* Init message table */
        BE_CEC_InitMessageTable (vpp_obj->be.cecFeatureSupportCfg, pCecSrv->msgTable);
        vpp_obj->be.enableCec = TRUE;
    }
    else
    {
        if (vpp_obj->be.hdmiCec != 0)
        {
            pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);

            // Disable tx and rx mode
            BE_CEC_SetMode (vpp_obj, BE_CEC_MODE_TX, FALSE, FALSE);
            BE_CEC_SetMode (vpp_obj, BE_CEC_MODE_RX, FALSE, FALSE);

            // Invalidate physical address
            VPP_BE_CEC_SetPhysicalAddr (vpp_obj, VPP_CEC_INVALID_PHY_ADDR);
        }
        vpp_obj->be.enableCec = FALSE;
    }
    return (MV_VPP_OK);
}

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
                           UINT32 cbFnAddr, UINT32 contextParam)
{
    CEC_SRV_DATA    *pCecSrv;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

    if ( pCecSrv->appCbFn != NULL )
    {
        // Callback already registered - cannot register another
        return (MV_VPP_EBADCALL);
    }
    pCecSrv->eventCode = eventCode;
    pCecSrv->appCbFn = (MV_VPP_EVENT_CALLBACK)cbFnAddr;
    pCecSrv->contextParam = (void*)contextParam;
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Unregister for CEC event notifications
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_UnregisterCB (VPP_OBJ *vpp_obj)
{
    CEC_SRV_DATA    *pCecSrv;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

    if ( pCecSrv->appCbFn == NULL )
    {
        // Nothing registered
        return (MV_VPP_OK);
    }
    pCecSrv->eventCode = 0;
    pCecSrv->appCbFn = NULL;
    pCecSrv->contextParam = NULL;
    return (MV_VPP_OK);
}

static void VPP_BE_CEC_TransmitTimerCallback(PVOID pvpp_obj)
{
    static int counter2=0;
    int ret=MV_VPP_OK;
#ifdef TZ_3_0
    g_vpp = (VPP*) pvpp_obj;
    VPP_OBJ *vpp_obj=g_vpp->vpp_objs[0];
#else
    VPP_OBJ *vpp_obj=(VPP_OBJ *)pvpp_obj;
#endif
    UINT8 value;
    VPP_HDMI_CEC_EVT_INFO notifEvt;
    CEC_SRV_DATA    *pCecSrv;
    int txIntervalMs = 0;
    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    counter2++;
    BE_CEC_RxLineStatus(vpp_obj,&value);
    if((value&0x01)==0x01)
    {
        counter2=0;
        // stop timer
        MV_Timer_Stop(&pCecSrv->Rx_LinePollhTimer2);
        txIntervalMs = GetSystemTimeMS() - pCecSrv->txData.txTime;
        if(txIntervalMs <= CEC_MSG_TX_TIMEOUT)
        {
            ret=BE_CEC_TransmitData(vpp_obj, &pCecSrv->txData, FALSE);
        }
        if((txIntervalMs > CEC_MSG_TX_TIMEOUT) || (ret!=MV_VPP_OK))
        {
            // clear S/W Buffer
                        notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_TX_STS;
            if(pCecSrv->txData.txDataLen>=2)
                            notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode = pCecSrv->txData.txDataBuf[1];
            else
                            notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode = 0x00;
                        // Transmit is not complete - clear buffer
                        GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
                        notifEvt.evtInfo.txStatusEvtInfo.txStatus = VPP_CEC_STATUS_TX_ERR_COMM_FAIL;
                        CEC_NotifyEvent (vpp_obj, &notifEvt);
        }
    }
    else
    {
        if(counter2>30)
        {
            counter2=0;
            MV_Timer_Stop(&pCecSrv->Rx_LinePollhTimer2);
            // clear S/W Buffer
                        notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_TX_STS;
            if(pCecSrv->txData.txDataLen>=2)
                            notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode = pCecSrv->txData.txDataBuf[1];
            else
                            notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode = 0x00;
                        // Transmit is not complete - clear buffer
                        GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
                        notifEvt.evtInfo.txStatusEvtInfo.txStatus = VPP_CEC_STATUS_TX_ERR_COMM_FAIL;
                        CEC_NotifyEvent (vpp_obj, &notifEvt);
        }
    }
}

static void VPP_BE_CEC_TimerCallback(PVOID pvpp_obj)
{
    static int counter=0;
#ifdef TZ_3_0
    g_vpp = (VPP*)pvpp_obj;
    VPP_OBJ *vpp_obj=  g_vpp->vpp_objs[0];
#else
    VPP_OBJ *vpp_obj=(VPP_OBJ *)pvpp_obj;
#endif
    UINT8 value;
    CEC_SRV_DATA    *pCecSrv;
    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    counter++;
    BE_CEC_RxLineStatus(vpp_obj,&value);
    if((value&0x01)==0x01)
    {
        counter=0;
        // stop timer
        MV_Timer_Stop(&pCecSrv->Rx_LinePollhTimer);
        // push cmd
        CEC_LOCK_MUTEX(pCecSrv->fifoMutex);
         if(!HdmiFifoPush (&pCecSrv->cmdFifo, (UINT8*)&g_cecCmd, FALSE))
            VPPHDMI_DBG_ERROR("CEC Driver: fifo push failed\n");
        CEC_UNLOCK_MUTEX(pCecSrv->fifoMutex);
        CEC_SEM_POST(pCecSrv->fifoSem);

    }
    else{
        if(counter>=20)
        {
            counter=0;// reset
            // tell application that commm failed
            MV_Timer_Stop(&pCecSrv->Rx_LinePollhTimer);
            VPPHDMI_DBG_ERROR("CEC Driver: RX Line not free in 20 times \n");
            g_cecCmd.cmdData.cmdTxStatus.stsInfo=BE_CEC_STS_TX_FAIL_INFO_NONE; // update aplication about the tx fail
                    // push cmd
                    CEC_LOCK_MUTEX(pCecSrv->fifoMutex);
                     if(!HdmiFifoPush (&pCecSrv->cmdFifo, (UINT8*)&g_cecCmd, FALSE))
                         VPPHDMI_DBG_ERROR("CEC Driver: fifo push failed\n");
                    CEC_UNLOCK_MUTEX(pCecSrv->fifoMutex);
                    CEC_SEM_POST(pCecSrv->fifoSem);
            // if timeout stop timer and post cmd
        }
    }
    return;
}

/******************************************************************************
 * FUNCTION : Interrupt service routine for CEC interrupt
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
void VPP_BE_CEC_ISRService(VPP_OBJ *vpp_obj,UINT16 reg)
{
    CEC_CMD         cecCmd;
    CEC_SRV_DATA    *pCecSrv;
    UINT16           intrField;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    BE_CEC_HandleInterrupt (vpp_obj, &intrField, (UINT8*)&cecCmd.cmdDataLen, cecCmd.cmdData.genData,
                            &pCecSrv->txData,reg);
    if (intrField == BE_CEC_INTR_TX_COMPLETE || intrField == BE_CEC_INTR_TX_FAIL)
    {
        cecCmd.cmdID = CEC_CMD_TX_STS;
        cecCmd.cmdDeviceType = CEC_GetDeviceTypeFromAddr(BE_CEC_GET_INIT_ADDR(cecCmd.cmdData.cmdTxStatus.frmHeader));
        // Queue the command
        HdmiFifoPush (&pCecSrv->isrFifo, (UINT8*)&cecCmd, FALSE);
    }
    else if (intrField == BE_CEC_INTR_RX_COMPLETE)
    {
        cecCmd.cmdID = CEC_CMD_RX_FRAME;
        cecCmd.cmdDeviceType = CEC_GetDeviceTypeFromAddr(BE_CEC_GET_FOLL_ADDR(cecCmd.cmdData.cmdRxFrame.rxFrame[0]));
        // Queue the command
        HdmiFifoPush (&pCecSrv->isrFifo, (UINT8*)&cecCmd, FALSE);
    }
    else if (intrField == BE_CEC_INTR_RX_FAIL)
    {
        cecCmd.cmdID = CEC_CMD_RX_STS;
        cecCmd.cmdDeviceType = VPP_CEC_DEVICE_MAX;
        // Queue the command
        HdmiFifoPush (&pCecSrv->isrFifo, (UINT8*)&cecCmd, FALSE);
    }
    else
    {
        // Unrecognized interrupt
        return;
    }
    return;
}

/******************************************************************************
 * FUNCTION : Deferred service routine for CEC interrupt to signal the waiting
 *          : task on the interrupt
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
void VPP_BE_CEC_DSR_Service(VPP_OBJ *vpp_obj)
{
    CEC_SRV_DATA    *pCecSrv;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    if (vpp_obj->status == STATUS_ACTIVE && pCecSrv != NULL)
        CEC_SEM_POST(pCecSrv->fifoSem);
}

/******************************************************************************
 * FUNCTION : Sets physical address - based on the given physical address, cec
 *          : command is generated and queued
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : phyAddr      - Physical address to be configured
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_SetPhysicalAddr (VPP_OBJ *vpp_obj, int phyAddr)
{
    CEC_SRV_DATA    *pCecSrv;
    CEC_CMD         cecCmd;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

    GaloisMemClear (&cecCmd, sizeof(CEC_CMD));
    if (phyAddr == VPP_CEC_INVALID_PHY_ADDR)
    {
        cecCmd.cmdID         = CEC_CMD_RESET_PHYADDR;
        cecCmd.cmdDeviceType = VPP_CEC_DEVICE_MAX;
        cecCmd.cmdDataLen    = 0;
    }
    else
    {
        cecCmd.cmdID         = CEC_CMD_SET_PHYADDR;
        cecCmd.cmdDeviceType = VPP_CEC_DEVICE_MAX;
        cecCmd.cmdDataLen    = sizeof (cecCmd.cmdData.cmdPhyAddr);
        cecCmd.cmdData.cmdPhyAddr.phyAddr = phyAddr;
    }

    // Add to queue
    CEC_LOCK_MUTEX(pCecSrv->fifoMutex);
    if (!HdmiFifoPush (&pCecSrv->cmdFifo, (UINT8*)&cecCmd, FALSE)){
        CEC_UNLOCK_MUTEX(pCecSrv->fifoMutex);
        return (MV_VPP_ECMDQFULL);
    }
    CEC_UNLOCK_MUTEX(pCecSrv->fifoMutex);
    CEC_SEM_POST (pCecSrv->fifoSem);
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Allocate/Unallocate logical address for the given device type
 * PARAMS   : *vpp_obj          - Pointer to VPP object
 *          : alloc             - Allocate/Unallocate logical address
 *          : deviceType        - Device type
 *          : initialPollAddr   - Initial polling address
 *          :                   - (-1), if default needs to be used
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_AllocLogicalAddr (VPP_OBJ *vpp_obj, BOOL alloc,
                                 VPP_CEC_DEVICE_TYPE deviceType,
                                 int initialPollAddr)
{
    CEC_SRV_DATA    *pCecSrv;
    CEC_CMD         cecCmd;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

    GaloisMemClear (&cecCmd, sizeof(CEC_CMD));
    if (alloc)
    {
        cecCmd.cmdID         = CEC_CMD_ALLOC_LOGADDR;
        cecCmd.cmdDeviceType = deviceType;
        cecCmd.cmdDataLen    = sizeof (cecCmd.cmdData.cmdLogAddr);
        cecCmd.cmdData.cmdLogAddr.initialPollAddr = initialPollAddr;

    }
    else
    {
        cecCmd.cmdID         = CEC_CMD_UNALLOC_LOGADDR;
        cecCmd.cmdDeviceType = deviceType;
        cecCmd.cmdDataLen    = 0;
    }

    // Add to queue
    CEC_LOCK_MUTEX(pCecSrv->fifoMutex);
    if (!HdmiFifoPush (&pCecSrv->cmdFifo, (UINT8*)&cecCmd, FALSE)){
        CEC_UNLOCK_MUTEX(pCecSrv->fifoMutex);
        return (MV_VPP_ECMDQFULL);
    }
    CEC_UNLOCK_MUTEX(pCecSrv->fifoMutex);
    CEC_SEM_POST(pCecSrv->fifoSem);
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Initiates transmission of given message on CEC line
 * PARAMS   : *vpp_obj  - Pointer to VPP object
 *          : pCecMsg   - Pointer to CEC message (Input)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int VPP_BE_CEC_SendMessage (VPP_OBJ *vpp_obj, PVPP_CEC_MSG pCecMsg)
{
    CEC_SRV_DATA            *pCecSrv;
    CEC_CMD                 cecCmd;
    CEC_STATE       cecState = CEC_STATE_IDLE;
    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

    if (pCecMsg->srcAddr > CEC_MAX_LOG_ADDR)
    {
    VPPHDMI_DBG_ERROR("CEC Driver: SEND_FAIL \n");
        return (MV_VPP_EBADPARAM);
    }
    GaloisMemClear (&cecCmd, sizeof(CEC_CMD));

    cecCmd.cmdID         = CEC_CMD_TX_FRAME;
    cecCmd.cmdDeviceType = CEC_GetDeviceTypeFromAddr(pCecMsg->srcAddr);
    cecCmd.cmdDataLen    = sizeof (cecCmd.cmdData.cmdTxFrame);

    CEC_GetState (vpp_obj, cecCmd.cmdDeviceType, &cecState);
    if(cecState != CEC_STATE_ACTIVE)
    {
        VPPHDMI_DBG_ERROR("CEC Driver: SEND_FAIL_2 \n");
        return (MV_VPP_EBADPARAM);
    }

    GaloisMemcpy (&cecCmd.cmdData.cmdTxFrame.cecMsg, pCecMsg, sizeof (VPP_CEC_MSG));
    // Add to queue
    CEC_LOCK_MUTEX(pCecSrv->fifoMutex);
    if (!HdmiFifoPush (&pCecSrv->cmdFifo, (UINT8*)&cecCmd, FALSE)){
        CEC_UNLOCK_MUTEX(pCecSrv->fifoMutex);
        return (MV_VPP_ECMDQFULL);
    }
    CEC_UNLOCK_MUTEX(pCecSrv->fifoMutex);
    CEC_SEM_POST(pCecSrv->fifoSem);
    return (MV_VPP_OK);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////LOCAL STATIC FUNCTIONS//////////////////////
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 * FUNCTION : Init CEC components
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static int CEC_Init (VPP_OBJ *vpp_obj)
{
    CEC_SRV_DATA    *pCecSrv;
#ifdef VPP_TA_CEC_ENABLE_INTR
    UINT32 reg=0;
#endif
    // Allocate memory for data store
    VPP_OSAL_SHM_Dynamic_Malloc(VPP_SHM_SECURE, sizeof(CEC_SRV_DATA), 32, (VPP_SHM_HANDLE*)&pCecSrv);
    if (pCecSrv == NULL) {
        return MV_VPP_ENOMEM;
    }
    GaloisMemClear (pCecSrv, sizeof(CEC_SRV_DATA));

    // Save the address in vpp_obj
    vpp_obj->be.hdmiCec = pCecSrv;
#ifdef VPP_TA_CEC_ENABLE_INTR /* need to move this out  to CA as this memory is not memmaped in TZ3.0 */
 // enable cec interrupts
    VPP_REG_WORD32_READ (SM_APB_ICTL1_BASE,&reg);// didn't find Macro for that ICTL1
    reg |= 0x10000;
    VPP_REG_WORD32_WRITE(SM_APB_ICTL1_BASE,reg);// enable ictl 1
#endif

    // Create task, command fifo
    if (MV_OSAL_Mutex_Create(&pCecSrv->fifoMutex) != MV_VPP_OK)
    {
        CEC_Term (vpp_obj);
        return MV_VPP_ENOMEM;
    }
    if (!HdmiFifoCreate (CEC_MAX_FIFO_SIZE, sizeof(CEC_CMD), &pCecSrv->cmdFifo))
    {
        CEC_Term (vpp_obj);
        return MV_VPP_ENOMEM;
    }

    if (!HdmiFifoCreate (CEC_MAX_FIFO_SIZE, sizeof(CEC_CMD), &pCecSrv->isrFifo))
    {
        CEC_Term (vpp_obj);
        return MV_VPP_ENOMEM;
    }

    if (MV_OSAL_Sem_Create (&pCecSrv->fifoSem, 0) != MV_VPP_OK)
    {
        CEC_Term (vpp_obj);
        return MV_VPP_ENOMEM;
    }

    vpp_obj->be.cecTaskExit = FALSE;
#ifndef CONFIG_AMP_TEE_ENABLE
    if (MV_OSAL_Task_Create (&pCecSrv->tskHandle, CEC_Task, (void*)(vpp_obj)) != MV_VPP_OK)
#else
    if (VPP_CreateCECTask() != MV_VPP_OK)
#endif

    {
        CEC_Term (vpp_obj);
        return MV_VPP_ENOMEM;
    }
    // create timer
#ifdef TZ_3_0
    if(MV_Timer_Create(&pCecSrv->Rx_LinePollhTimer,10, (PVOID)VPP_BE_CEC_TimerCallback, (PVOID)(g_vpp))!=MV_VPP_OK)
#else
    if(MV_Timer_Create(&pCecSrv->Rx_LinePollhTimer,10, (PVOID)VPP_BE_CEC_TimerCallback, (PVOID)(vpp_obj))!=MV_VPP_OK)
#endif
    {
    CEC_Term (vpp_obj);
        return MV_VPP_ENOMEM;
    }
#ifdef TZ_3_0
    if(MV_Timer_Create(&pCecSrv->Rx_LinePollhTimer2,10, (PVOID)VPP_BE_CEC_TransmitTimerCallback, (PVOID)(g_vpp))!=MV_VPP_OK)
#else
    if(MV_Timer_Create(&pCecSrv->Rx_LinePollhTimer2,10, (PVOID)VPP_BE_CEC_TransmitTimerCallback, (PVOID)(vpp_obj))!=MV_VPP_OK)
#endif
    {
    CEC_Term (vpp_obj);
        return MV_VPP_ENOMEM;
    }

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Terminate CEC components
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static int CEC_Term (VPP_OBJ *vpp_obj)
{
    CEC_SRV_DATA  *pCecSrv;

    if (vpp_obj->be.hdmiCec == 0)
    {
        // Not initialized - Do nothing
        return (MV_VPP_EUNCONFIG);
    }

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);

    // Delete Message queue, Task
    if (pCecSrv->fifoMutex != NULL)
    {
        MV_OSAL_Mutex_Destroy(&pCecSrv->fifoMutex);
        pCecSrv->fifoMutex = NULL;
    }

    if (pCecSrv->fifoSem != NULL)
    {
        MV_OSAL_Sem_Destroy (&pCecSrv->fifoSem);
        pCecSrv->fifoSem = NULL;
    }
    if(pCecSrv->Rx_LinePollhTimer!=NULL)
    {
    MV_Timer_Destroy(&pCecSrv->Rx_LinePollhTimer);
    pCecSrv->Rx_LinePollhTimer=NULL;
    }
    if(pCecSrv->Rx_LinePollhTimer2!=NULL)
    {
    MV_Timer_Destroy(&pCecSrv->Rx_LinePollhTimer2);
    pCecSrv->Rx_LinePollhTimer2=NULL;
    }
    HdmiFifoDelete (&pCecSrv->cmdFifo);
    HdmiFifoDelete (&pCecSrv->isrFifo);
#ifndef CONFIG_AMP_TEE_ENABLE
    if (pCecSrv->tskHandle != NULL)
    {
        vpp_obj->be.cecTaskExit = TRUE;
        MV_OSAL_Task_Join(&pCecSrv->tskHandle);
        pCecSrv->tskHandle = NULL;
    }
#else
    VPP_DeleteCECTask();
#endif

    // Release Memory
    GaloisFree (pCecSrv);

    // Clean up in vpp_obj
    vpp_obj->be.hdmiCec = 0;

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : HDMI Cec Task
 * PARAMS   : *arg        - Pointer to VPP object
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
int CEC_Task (void *arg)
{
    CEC_SRV_DATA    *pCecSrv;
    VPP_OBJ         *vpp_obj = (VPP_OBJ*)arg;
    CEC_CMD         cecCmd;
    CEC_STATE       cecState = CEC_STATE_IDLE;
    int             startIndex = 0; // used in logical address allocation
    int             temp;
    VPP_HDMI_CEC_EVT_INFO notifEvt;
    int             retCode;
    UINT8    value;
    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

#ifndef CONFIG_AMP_TEE_ENABLE
    while (vpp_obj->be.cecTaskExit == FALSE)
#endif
    {
        // Read CEC command from queue
        CEC_SEM_WAIT (pCecSrv->fifoSem);

        while (1) {
        if (!HdmiFifoPop (&pCecSrv->cmdFifo, (UINT8*)&cecCmd))
        {
            // No command in command fifo - check isr fifo
            if (!HdmiFifoPop (&pCecSrv->isrFifo, (UINT8*)&cecCmd))
            {
                break;
            }
        }

        // Get current state
        CEC_GetState (vpp_obj, cecCmd.cmdDeviceType, &cecState);
        // Handle Command
        switch (cecState)
        {
            case CEC_STATE_IDLE:
            {
                if (cecCmd.cmdID == CEC_CMD_SET_PHYADDR)
                {
                    CEC_SetPhyAddr (vpp_obj, TRUE, cecCmd.cmdData.cmdPhyAddr.phyAddr);
                }
            }
            break;

            case CEC_STATE_UNALLOC:
            {
                switch (cecCmd.cmdID)
                {
                    case CEC_CMD_RESET_PHYADDR:
                    {
                        CEC_SetPhyAddr (vpp_obj, FALSE, VPP_CEC_INVALID_PHY_ADDR);
                    }
                    break;

                    case CEC_CMD_ALLOC_LOGADDR:
                    {
                        pCecSrv->logAddr[cecCmd.cmdDeviceType] = -1;
                        startIndex = (cecCmd.cmdData.cmdLogAddr.initialPollAddr == -1)?
                                      0:cecCmd.cmdData.cmdLogAddr.initialPollAddr;
                        CEC_AllocLogAddr (vpp_obj, cecCmd.cmdDeviceType,
                                         startIndex, startIndex);
                    }
                    break;

                    case CEC_CMD_TX_STS:
                    {
                        temp = BE_CEC_GET_INIT_ADDR (cecCmd.cmdData.cmdTxStatus.frmHeader);
                        if (cecCmd.cmdData.cmdTxStatus.stsInfo == BE_CEC_STS_TX_ACKED)
                        {
                            // Try the next higher logical address for this device
                            temp = ((temp+1) >= CEC_LOG_ADDR_MAP_SIZE)?0:(temp+1);
                            CEC_AllocLogAddr (vpp_obj, cecCmd.cmdDeviceType,startIndex, temp);
                        }
                        else if (cecCmd.cmdData.cmdTxStatus.stsInfo == BE_CEC_STS_TX_FAIL_NOACK ||
                                 cecCmd.cmdData.cmdTxStatus.stsInfo == BE_CEC_STS_TX_FAIL_INFO_NONE)
                        {
                            // Logical address allocated - register it
                            CEC_SetLogAddr (vpp_obj, TRUE, cecCmd.cmdDeviceType, temp);

                            // On C0, this would be fixed in hardware for multiple addresses
                            // Until then, only one address would be supported
                            if (cecCmd.cmdDeviceType < VPP_CEC_DEVICE_RES)
                                BE_CEC_EnableLogAddr (vpp_obj, TRUE, cecCmd.cmdDeviceType, temp);
                            else
                                BE_CEC_EnableLogAddr (vpp_obj, TRUE, cecCmd.cmdDeviceType-1, temp);

                            // Notify application
                            GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
                            notifEvt.infoID = MV_VPP_EVENT_CEC_LOG_ADDR_STS;
                            notifEvt.evtInfo.logAddrStsEvtInfo.deviceType = cecCmd.cmdDeviceType;
                            notifEvt.evtInfo.logAddrStsEvtInfo.logAddrAllocSts = TRUE;
                            notifEvt.evtInfo.logAddrStsEvtInfo.logAddr = temp;
                            CEC_NotifyEvent (vpp_obj, &notifEvt);
                        }
                        else if ((cecCmd.cmdData.cmdTxStatus.stsInfo == BE_CEC_STS_TX_FAIL_SIG_FREE_TIME) &&
                                 ((GetSystemTimeMS() - pCecSrv->txData.txTime) <= CEC_MSG_TX_TIMEOUT))
                        {
                            CEC_AllocLogAddr (vpp_obj, cecCmd.cmdDeviceType, startIndex, temp);
                        }
                        else
                        {
                            // Any other status - notify application of failure
                            GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
                            notifEvt.infoID = MV_VPP_EVENT_CEC_LOG_ADDR_STS;
                            notifEvt.evtInfo.logAddrStsEvtInfo.deviceType = cecCmd.cmdDeviceType;
                            notifEvt.evtInfo.logAddrStsEvtInfo.logAddrAllocSts = FALSE;
                            CEC_NotifyEvent (vpp_obj, &notifEvt);
                        }
                    }
                    break;

                    default:
                        break;
                }
            }
            break;

            case CEC_STATE_ACTIVE:
            {
                switch (cecCmd.cmdID)
                {
                    case CEC_CMD_RESET_PHYADDR:
                    {
                        CEC_SetPhyAddr (vpp_obj, FALSE, VPP_CEC_INVALID_PHY_ADDR);
                        VPPHDMI_DBG_INFO("CEC Driver: RESET_PHY txlen=%d\n",pCecSrv->txData.txDataLen);
                        if (pCecSrv->txData.txDataLen != 0)
                        {
                                notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_TX_STS;
                                notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode =0;
                                notifEvt.evtInfo.txStatusEvtInfo.txStatus = VPP_CEC_STATUS_TX_ERR_WAIT_FAIL;
                                CEC_NotifyEvent (vpp_obj, &notifEvt);
                        }
                    }
                    break;

                    case CEC_CMD_UNALLOC_LOGADDR:
                    {
                        CEC_SetLogAddr (vpp_obj, FALSE, cecCmd.cmdDeviceType, -1);
                    }
                    break;

                    case CEC_CMD_TX_FRAME:
                    {
                        if (pCecSrv->txData.txDataLen != 0)
                        {
                            VPPHDMI_DBG_INFO("CEC Driver: Tx is already under prcoessing\n");
                            // clear S/W Buffer
                            notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_TX_STS;
                            notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode = cecCmd.cmdData.cmdTxFrame.cecMsg.opcode;
                            notifEvt.evtInfo.txStatusEvtInfo.txStatus = VPP_CEC_STATUS_TX_ERR_WAIT_FAIL;
                            CEC_NotifyEvent (vpp_obj, &notifEvt);
                            // Another message in transmit - If required, add a notification event
                            if ((GetSystemTimeMS() - pCecSrv->txData.txTime) > CEC_MSG_TX_TOTAL_TIMEOUT)
                                GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
                            break;
                        }
                        // Format message to frame
                        retCode = BE_CEC_FormatMessageToFrame (pCecSrv->msgTable, &cecCmd.cmdData.cmdTxFrame.cecMsg,
                                                               pCecSrv->txData.txDataBuf, &pCecSrv->txData.txDataLen);
                        if (retCode != VPP_CEC_STATUS_SUCCESS)
                        {
                            // Error - Send notification event
                            notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_TX_STS;
                            notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode = cecCmd.cmdData.cmdTxFrame.cecMsg.opcode;
                            notifEvt.evtInfo.txStatusEvtInfo.txStatus = (VPP_CEC_STATUS_TYPE)retCode;
                            CEC_NotifyEvent (vpp_obj, &notifEvt);
                        }
                        else
                        {
                            // Initiate frame transfer
                            // If mode is undefined, it would have been caught in the retcode above
                            pCecSrv->txData.txType =
                                (BE_CEC_GetMessageMode (cecCmd.cmdData.cmdTxFrame.cecMsg.destAddr) == BE_CEC_MSG_MODE_BROADCAST)?
                                                        BE_CEC_TX_TYPE_BROADCAST:BE_CEC_TX_TYPE_UNICAST;
                            pCecSrv->txData.retxCondn = CEC_RETX_CONDN;
                            pCecSrv->txData.retxCount = CEC_MAX_RETX_COUNT;
                            pCecSrv->txData.txTime = GetSystemTimeMS ();
                            BE_CEC_RxLineStatus(vpp_obj,&value);
                            if((value&0x01)==0x01) // line free
                            {
                                            retCode=BE_CEC_TransmitData (vpp_obj, &pCecSrv->txData, TRUE);
                            if(retCode!=MV_VPP_OK)
                            {
                                            notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_TX_STS;
                                            notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode = cecCmd.cmdData.cmdTxFrame.cecMsg.opcode;
                                                // Transmit is not complete - clear buffer
                                                GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
                                                notifEvt.evtInfo.txStatusEvtInfo.txStatus = VPP_CEC_STATUS_TX_ERR_COMM_FAIL;
                                                CEC_NotifyEvent (vpp_obj, &notifEvt);

                            }
                            }
                            else
                            MV_Timer_Start(&pCecSrv->Rx_LinePollhTimer2);
                        }
                    }
                    break;

                    case CEC_CMD_RX_FRAME:
                    {
                        // Format frame to message
                        notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_RX_STS;
                        retCode = BE_CEC_FormatFrameToMessage (pCecSrv->msgTable, cecCmd.cmdData.cmdRxFrame.rxFrame,
                                                               &notifEvt.evtInfo.rxStatusEvtInfo.cecMsg,
                                                               cecCmd.cmdDataLen);
                        if (retCode != VPP_CEC_STATUS_ERR_MSG_IGNORED)
                        {
                            notifEvt.evtInfo.rxStatusEvtInfo.rxStatus = (VPP_CEC_STATUS_TYPE)retCode;
                            CEC_NotifyEvent (vpp_obj, &notifEvt);
                        }
                    }
                    break;

                    case CEC_CMD_TX_STS:
                    {
                        notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_TX_STS;
                        notifEvt.evtInfo.txStatusEvtInfo.txMsgOpcode = cecCmd.cmdData.cmdTxStatus.frmOpcode;
                        if (cecCmd.cmdData.cmdTxStatus.stsInfo == BE_CEC_STS_TX_ACKED)
                        {
                            // Transmit is complete - clear buffer
                            GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
                            notifEvt.evtInfo.txStatusEvtInfo.txStatus = VPP_CEC_STATUS_SUCCESS;
                            CEC_NotifyEvent (vpp_obj, &notifEvt);
                        }
                        else
                        {
                            // Transmit failed
                            if ( cecCmd.cmdData.cmdTxStatus.stsInfo == BE_CEC_STS_TX_FAIL_SIG_FREE_TIME )
                            {

                                if ((GetSystemTimeMS() - pCecSrv->txData.txTime) <= CEC_MSG_TX_TIMEOUT)
                                {
                                    BE_CEC_RxLineStatus(vpp_obj,&value);
                                    if((value&0x01)==0x01) // line free
                                    {
                                                        // Retry until time-out
                                                        BE_CEC_TransmitData (vpp_obj, &pCecSrv->txData, FALSE);
                                        break;
                                    }
                                    else
                                    {
                                    MV_Timer_Start(&pCecSrv->Rx_LinePollhTimer2); // start polling timer
                                    break;
                                    }
                                }
                                else {
                                    VPPHDMI_DBG_ERROR("CEC Driver: TX_FAIL_TIMEOUT\n");
                                }
                            }
                            GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
                            notifEvt.evtInfo.txStatusEvtInfo.txStatus = VPP_CEC_STATUS_TX_ERR_COMM_FAIL;
                            CEC_NotifyEvent (vpp_obj, &notifEvt);
                        }
                    }
                    break;

                    case CEC_CMD_RX_STS:
                    {
                        VPPHDMI_DBG_INFO("CEC Driver:RX Faliure code=%x\n",cecCmd.cmdData.cmdRxFrame.rxFrame[0]);
                        // Propagate rx failed to application
                        notifEvt.infoID = MV_VPP_EVENT_CEC_MSG_RX_STS;
                        notifEvt.evtInfo.rxStatusEvtInfo.rxStatus = VPP_CEC_STATUS_RX_ERR_COMM_FAIL;
                        CEC_NotifyEvent (vpp_obj, &notifEvt);
                    }
                    break;

                    default:
                        break;
                }
            }
            break;

            default: break;
        }
    }
    }

#ifndef CONFIG_AMP_TEE_ENABLE
    // Exit Task
    MV_OSAL_Task_Exit(NULL);
#endif

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Configures physical address
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : setAddr      - Set/Reset physical address
 *          : phyAddr      - Physical address
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static int CEC_SetPhyAddr (VPP_OBJ *vpp_obj, BOOL setAddr, int phyAddr)
{
    CEC_SRV_DATA    *pCecSrv;
    int             i;
    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

    if (setAddr)
    {
        pCecSrv->phyAddr = phyAddr;
        CEC_SetState (vpp_obj, VPP_CEC_DEVICE_MAX, CEC_STATE_UNALLOC);
    }
    else
    {
        // Unregister all devices and set state to idle
        for (i = 0; i < VPP_CEC_DEVICE_MAX; i++) {
            CEC_SetLogAddr (vpp_obj,FALSE,i,-1);
        }
        CEC_SetState (vpp_obj, VPP_CEC_DEVICE_MAX, CEC_STATE_IDLE);
    }
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Set/Reset logical address in data store and updates state
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : setAddr      - Set/Reset logical address
 *          : deviceType   - Device type
 *          : logAddr      - Logical address
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static int CEC_SetLogAddr (VPP_OBJ *vpp_obj, BOOL setAddr,
                           VPP_CEC_DEVICE_TYPE deviceType, int logAddr)
{
    CEC_SRV_DATA    *pCecSrv;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);
    if (setAddr)
    {
        pCecSrv->logAddr[deviceType] = logAddr;
        CEC_SetState (vpp_obj, deviceType, CEC_STATE_ACTIVE);
    }
    else
    {
        pCecSrv->logAddr[deviceType] = -1;
        CEC_SetState (vpp_obj, deviceType, CEC_STATE_UNALLOC);
        if (deviceType < VPP_CEC_DEVICE_RES)
            BE_CEC_EnableLogAddr (vpp_obj, FALSE, deviceType, 0);
        else
            BE_CEC_EnableLogAddr (vpp_obj, FALSE, deviceType-1, 0);
    }
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Allocates logical address for the given device type
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : deviceType   - Logical device for which logical address has to
 *          :              - be allocated
 *          : startIndex   - Start index in the map
 *          : curIndex     - Current index in the map
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static int CEC_AllocLogAddr (VPP_OBJ *vpp_obj,VPP_CEC_DEVICE_TYPE deviceType,
                            int startIndex, int curIndex)
{
    BOOL                    pollAddrFound = FALSE;
    VPP_HDMI_CEC_EVT_INFO   notifEvt;
    CEC_SRV_DATA    *pCecSrv;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);
    do
    {
        if (CEC_devLogAddrMap[curIndex] == deviceType)
        {
            // Send a polling message
            pollAddrFound = TRUE;
            break;
        }
        curIndex = ((curIndex+1) >= CEC_LOG_ADDR_MAP_SIZE)?0:(curIndex+1);
    } while (curIndex != startIndex);
    if (!pollAddrFound)
    {
        // Acquire 0xF address
        CEC_SetLogAddr (vpp_obj, TRUE, deviceType, 0x0F);

        // Notify application
        notifEvt.infoID = MV_VPP_EVENT_CEC_LOG_ADDR_STS;
        notifEvt.evtInfo.logAddrStsEvtInfo.deviceType = deviceType;
        notifEvt.evtInfo.logAddrStsEvtInfo.logAddrAllocSts = TRUE;
        notifEvt.evtInfo.logAddrStsEvtInfo.logAddr = 0x0F;
        CEC_NotifyEvent (vpp_obj, &notifEvt);
    }
    else
    {
        // Send polling message for the address found
        GaloisMemClear (&pCecSrv->txData, sizeof(BE_CEC_TX_DATA));
        pCecSrv->txData.txType       = BE_CEC_TX_TYPE_UNICAST;
        pCecSrv->txData.retxCondn    = (BE_CEC_STS_TX_FAIL_RESP_TIMEOUT | BE_CEC_STS_TX_FAIL_COLL_DET);
        pCecSrv->txData.retxCount    = CEC_MAX_RETX_COUNT;
        pCecSrv->txData.txDataLen    = 1;
        pCecSrv->txData.txDataBuf[0] = (curIndex << 4) | curIndex;
        pCecSrv->txData.txTime = GetSystemTimeMS ();
        BE_CEC_TransmitData (vpp_obj, &pCecSrv->txData, TRUE);
    }
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Set internal CEC state
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : deviceType   - Logical device type
 *          : cecState     - Cec state to set
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static int CEC_SetState (VPP_OBJ *vpp_obj,
                         VPP_CEC_DEVICE_TYPE deviceType, CEC_STATE cecState)
{
    CEC_SRV_DATA    *pCecSrv;
    int             i;
    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

    if (deviceType == VPP_CEC_DEVICE_MAX)
    {
        for (i = 0; i < VPP_CEC_DEVICE_MAX; i++)
            pCecSrv->curState[i] = cecState;
    }
    else
    {
        pCecSrv->curState[deviceType] = cecState;
    }
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Get internal CEC state
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : deviceType   - Logical device type
 *          : *pCecState   - Pointer to return internal state (Output)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static int CEC_GetState (VPP_OBJ *vpp_obj,
                         VPP_CEC_DEVICE_TYPE deviceType, CEC_STATE *pCecState)
{
    CEC_SRV_DATA    *pCecSrv;
    int             i;

    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);

    *pCecState = 0;
    if (deviceType == VPP_CEC_DEVICE_MAX)
    {
        for (i = 0; i < VPP_CEC_DEVICE_MAX; i++)
        {
            if (pCecSrv->curState[i] > *pCecState)
                *pCecState = pCecSrv->curState[i];
        }
    }
    else
    {
        *pCecState = pCecSrv->curState[deviceType];
    }
    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Sends events through registered callback, if any
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 *          : *pEvtInfo    - Pointer to event info to be sent (Input)
 * RETURN   : MV_VPP_OK, on success
 *          : Error code, otherwise
 *****************************************************************************/
static int CEC_NotifyEvent (VPP_OBJ *vpp_obj, VPP_HDMI_CEC_EVT_INFO *pEvtInfo)
{
    CEC_SRV_DATA    *pCecSrv;
    pCecSrv = (CEC_SRV_DATA*)(vpp_obj->be.hdmiCec);
    CEC_DATASTORE_CHK (pCecSrv);
    if ((vpp_obj->be.enableCec == TRUE) && (pCecSrv->appCbFn != NULL)) {
        VPP_CB_FUNC(pCecSrv->appCbFn, (void *)vpp_obj, pCecSrv->eventCode, (void*)pEvtInfo, pCecSrv->contextParam);
    }

    return (MV_VPP_OK);
}

/******************************************************************************
 * FUNCTION : Finds device type for the given logical address
 * PARAMS   : logAddr   - Logical address
 * RETURN   : Device type corresponding to the logical address
 *          : VPP_CEC_DEVICE_MAX, in case of error
 *****************************************************************************/
static VPP_CEC_DEVICE_TYPE CEC_GetDeviceTypeFromAddr (int logAddr)
{
    if (logAddr > CEC_MAX_LOG_ADDR)
        return (VPP_CEC_DEVICE_MAX);

    return (CEC_devLogAddrMap[logAddr]);
}

/******************************************************************************
 * FUNCTION : Finds number of active logical devices
 * PARAMS   : *vpp_obj     - Pointer to VPP object
 * RETURN   : Number of active devices
 *****************************************************************************/
int CEC_GetNumberOfActiveDevices (VPP_OBJ *vpp_obj)
{
    int         i;
    int         numActiveDevices = 0;
    CEC_STATE   cecState = CEC_STATE_IDLE;

    for (i = 0; i < VPP_CEC_DEVICE_MAX; i++)
    {
        CEC_GetState (vpp_obj, i, &cecState);
        if (cecState == CEC_STATE_ACTIVE)
            numActiveDevices++;
    }
    return (numActiveDevices);
}

