/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


////////////////////////////////////////////////////////////////////////////////
//! \file OSALTimer.c
//! \brief
//!
//! Purpose: Galois Linux platform
//!
//!
//! Version, Date and Author :
//!     V 1.00, 7/19/2007,  Fang Bao
//!     V 1.01, 9/12/2007,  Fang Bao
//!             Clean up code and add the new debug trace function
//!     V 1.02, 7/25/2008,  Fang Bao
//!             Bugfixed - when call MV_Timer_SetState() to start timer, the first time of callback will be called at once.
//!                         set the pTimer->m_TimerTime.m_value the next time value in MV_Timer_SetState()
//!     V 1.03, 2/11/2009,  Fang Bao
//!             Bugfixed -  Disable MV_Timer_Destroy() and MV_Timer_Reset() when Timer is started
//!             Bugfixed -  Disable MV_Timer_Start() and MV_Timer_Stop() when API is called repeated { MV_Timer_SetState() }
//!             Bugfixed -  missing return code in MV_Timer_Reset_Failure_1 of MV_Timer_Reset()
//!     V 1.04, 2/23/2009,  Fang Bao
//!             Bugfixed -  MV_OSAL_Mutex_Unlock() missing in MV_Timer_Destroy()
//!             Bugfixed -  MV_Timer_TimerTask need more time to do cancellation, so add MV_OSAL_Task_Sleep() in MV_Timer_Destroy()
//!     V 1.05, 8/07/2009,  Fang Bao
//!             Add MV_TimerState_EXITED. Now, timer task exited to TASK_JOIN not TASK_DESTROY
//!             Add APIs - MV_Timer_Pause() and MV_Timer_Resume() which are available on Linux and eCos OS
//!     V 1.06, 8/10/2009,  Fang Bao
//!             Add APIs - MV_Timer_Reset_Period()
//!     V 1.07, 8/11/2009,  Fang Bao
//!             Bugfixed - build error in MV_Timer_Resume(), MV_Timer_Pause(), MV_Timer_Stop() when debug_osal is enable.
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "OSAL_api.h"
#include <linux/hrtimer.h>

typedef struct _MV_Timer
{
    struct hrtimer htimer;
    ktime_t ktperiod;

    MV_OSAL_HANDLE_MUTEX_t m_hMutex;

    PVOID UserData;
    VOID (*CallbackFunc)(VOID *);

} MV_Timer_t, *pMV_Timer_t;


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_TimerTask
//!
//! Description: the task of timer to call user function
//!
//! \param arg        (IN): The argument of the task
//!
//! \return Return:     S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
enum hrtimer_restart  MV_Timer_TimerFunc(struct hrtimer * timer)
{
    HRESULT res;
    pMV_Timer_t pTimer = container_of(timer,struct _MV_Timer,htimer);

    /* PARAM Check */
    if (pTimer == NULL)
        MV_OSAL_DBG_Error(E_INVALIDARG, "MV_Timer_TimerTask", NULL);

    MV_OSAL_DBG_Info("(CPU-%d task)-(MV_Timer_TimerTask) start...\n", MV_OSAL_CPU_LOCAL);

    res = MV_OSAL_Mutex_Lock(pTimer->m_hMutex);
    if (res != S_OK)
    {
        MV_OSAL_DBG_Warning(res, "MV_Timer_TimerTask MV_OSAL_Mutex_Lock", NULL);
        goto MV_Timer_TimerTask_Failure_0;
    }

    pTimer->CallbackFunc(pTimer->UserData);

    res = MV_OSAL_Mutex_Unlock(pTimer->m_hMutex);
    if (res != S_OK)
    {
        MV_OSAL_DBG_Warning(res, "MV_Timer_TimerTask MV_OSAL_Mutex_Unlock", NULL);
    }

        hrtimer_forward_now(timer, pTimer->ktperiod);
MV_Timer_TimerTask_Failure_0:
    return HRTIMER_NORESTART;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Constructor
//!
//! Description: Timer Handle Self Constructor
//!
//! \param self         (OUT): -- the pointer to the timer handle
//!
//! \return Return:     S_OK
//!                     E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Constructor(MV_HANDLE_Timer_t *self)
{
    pMV_Timer_t pTimer;

    pTimer = (pMV_Timer_t)MV_OSAL_Malloc(sizeof(MV_Timer_t));
    if (pTimer == NULL)
        MV_OSAL_DBG_Error(E_OUTOFMEMORY, "MV_Timer_Constructor MV_OSAL_Malloc", NULL);

    *self = (MV_HANDLE_Timer_t)pTimer;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Destructor
//!
//! Description: Timer Handle Self Destructor
//!
//! \param self     (IN/OUT): -- the pointer to the timer handle
//!
//! \return Return:     S_OK
//!                     E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Destructor(MV_HANDLE_Timer_t  *self)
{
    /* Parameter Check */
    if ((self == NULL) || (*self == NULL))
        MV_OSAL_DBG_Error(E_INVALIDARG, "MV_Timer_Destructor", NULL);

    MV_OSAL_Free(*self);

    return S_OK;
}



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Create
//!
//! Description: Create a timer to call the user procedure in the future
//!
//! \param pHandle        (IN): The pointer to the handle of the timer object
//! \param Period         (IN): The value of the time out for the timer in milliseconds
//! \param CallbackFunc   (IN): The pointer to the user procedure
//! \param UserData       (IN): The pointer to parameter of the user procedure
//!
//! \return Return:     S_OK
//!                     E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Create(MV_HANDLE_Timer_t  *pHandle,
                         UINT               Period,
                         PVOID              CallbackFunc,
                         PVOID              UserData)
{
    HRESULT res;
    pMV_Timer_t pTimer;

    /* Parameter Check */
    if ((pHandle == NULL) ||
        (Period == 0) ||
        (CallbackFunc == NULL))
        MV_OSAL_DBG_Error(E_INVALIDARG, "MV_Timer_Create", NULL);

    res = MV_Timer_Constructor(pHandle);
    if (res != S_OK)
    {
        MV_OSAL_DBG_Warning(res, "MV_Timer_Create MV_Timer_Constructor", NULL);
        goto MV_Timer_Create_Failure;
    }

    pTimer = (pMV_Timer_t)(*pHandle);

    res = MV_OSAL_Mutex_Create(&(pTimer->m_hMutex));
    if (res != S_OK)
    {
        MV_OSAL_DBG_Warning(res, "MV_Timer_Create MV_OSAL_Mutex_Create", NULL);
        goto MV_Timer_Create_Failure;
    }

    pTimer->UserData = UserData;
    pTimer->CallbackFunc = CallbackFunc;

    pTimer->ktperiod = ktime_set(0, Period*1000*1000); //seconds,nanoseconds
    hrtimer_init(&pTimer->htimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        pTimer->htimer.function = MV_Timer_TimerFunc;

    return S_OK;

MV_Timer_Create_Failure:

    MV_Timer_Destructor(pHandle);

    return res;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Destroy
//!
//! Description: Destroy a timer (before destroying, the timer will be stopped)
//!
//! \param pHandle        (IN): The pointer to the handle of the timer object
//!
//! \return Return:     S_OK
//!                     E_INVALIDARG
//!                     E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Destroy(MV_HANDLE_Timer_t *pHandle)
{
    HRESULT res;
    pMV_Timer_t pTimer;

    /* Parameter Check */
    if (pHandle == NULL)
        MV_OSAL_DBG_Error(E_INVALIDARG, "MV_Timer_Destroy", NULL);

    pTimer = (pMV_Timer_t)(*pHandle);

    res = MV_OSAL_Mutex_Lock(pTimer->m_hMutex);
    if (res != S_OK)
    {
        MV_OSAL_DBG_Warning(res, "MV_Timer_Destroy MV_OSAL_Mutex_Lock", NULL);
        goto MV_Timer_Destroy_Failure_0;
    }

    hrtimer_cancel(&pTimer->htimer);

    res = MV_OSAL_Mutex_Unlock(pTimer->m_hMutex);
    if (res != S_OK)
    {
        MV_OSAL_DBG_Warning(res, "MV_Timer_Destroy MV_OSAL_Mutex_Unlock", NULL);
    }

    res = MV_OSAL_Mutex_Destroy(&(pTimer->m_hMutex));
    if (res != S_OK)
    {
        MV_OSAL_DBG_Warning(res, "MV_Timer_Destroy MV_OSAL_Mutex_Destroy", NULL);
        goto MV_Timer_Destroy_Failure_0;
    }

    res = MV_Timer_Destructor(pHandle);

    return res;

MV_Timer_Destroy_Failure_0:

    return res;

}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Start
//!
//! Description: Start a timer
//!
//! \param pHandle        (IN): The pointer to the handle of the timer object
//!
//! \return Return:     S_OK
//!                     E_INVALIDARG
//!                     E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT   MV_Timer_Start(MV_HANDLE_Timer_t  *pHandle)
{
    pMV_Timer_t pTimer;

    /* Parameter Check */
    if (pHandle == NULL)
        MV_OSAL_DBG_Error(E_INVALIDARG, "MV_Timer_Start", NULL);

    pTimer = (pMV_Timer_t)(*pHandle);

    hrtimer_start(&pTimer->htimer, pTimer->ktperiod, HRTIMER_MODE_REL);
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Stop
//!
//! Description: Stop a timer
//!
//! \param pHandle        (IN): The pointer to the handle of the timer object
//!
//! \return Return:     S_OK
//!                     E_INVALIDARG
//!                     E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Stop(MV_HANDLE_Timer_t   *pHandle)
{
//  return (MV_Timer_SetState(pHandle, MV_TimerState_STOPPED));

    HRESULT res;
    pMV_Timer_t pTimer;

    /* Parameter Check */
    if (pHandle == NULL)
        MV_OSAL_DBG_Error(E_INVALIDARG, "MV_Timer_Stop", NULL);

    pTimer = (pMV_Timer_t)(*pHandle);

    MV_OSAL_Mutex_Lock(pTimer->m_hMutex);

    hrtimer_cancel(&pTimer->htimer);

    res = MV_OSAL_Mutex_Unlock(pTimer->m_hMutex);
    if (res != S_OK)
        MV_OSAL_DBG_Warning(res, "MV_Timer_Stop MV_OSAL_Cond_Broadcast", NULL);

    return res;
}

