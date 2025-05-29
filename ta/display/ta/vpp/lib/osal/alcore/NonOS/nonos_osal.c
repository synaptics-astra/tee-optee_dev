/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file nonos_osal.c
//!
//! \brief Implementation of dummy OSAL function implementation
//!
//! Purpose: This is used for building TA for bootloader or non OS environment
//!
//!     Version    Date                     Author
//!     V 1.00,    Jun 23 2017,             Shanmugam Ramachandran
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////


#include "tee_internal_api.h"

#include "OSAL_api.h"

#include "vpp_osal.h"


#define MV_OSAL_TIMER_ENABLE
#define MV_OSAL_TASK_ENABLE
#define MV_OSAL_MUTEX_ENABLE
#define MV_OSAL_SEM_ENABLE

#ifdef MV_OSAL_TIMER_ENABLE
HRESULT  MV_Timer_Create(MV_HANDLE_Timer_t  *pHandle,
        UINT               Period,
        PVOID              CallbackFunc,
        PVOID              UserData)
{

    HRESULT retVal = S_OK;
    TEE_Param cb_params[4];
    cb_params[0].value.a = Period;
    cb_params[1].value.a = (UINT32)CallbackFunc;
    cb_params[2].value.a = (UINT32)UserData;

#ifdef TZ_3_0
    VPP_OSAL_SHM_Dynamic_Malloc(AMP_SHM_SECURE_VIDEO_FB, sizeof(MV_HANDLE_Timer_t)*2, 32, (VPP_SHM_HANDLE*)pHandle);
#else
    *pHandle = VPP_Malloc(sizeof(MV_HANDLE_Timer_t)*2);
#endif
    memcpy(*pHandle, &cb_params[0].value.a, sizeof(MV_HANDLE_Timer_t));
    memcpy(((*pHandle)+sizeof(MV_HANDLE_Timer_t)), &cb_params[1].value.a, sizeof(MV_HANDLE_Timer_t));

    return retVal;
}

HRESULT  MV_Timer_Destroy(MV_HANDLE_Timer_t *pHandle)
{
    HRESULT retVal = S_OK;
    *pHandle = NULL;

    return retVal;
}

HRESULT  MV_Timer_Reset(MV_HANDLE_Timer_t   *pHandle,
        UINT               Period,
        PVOID              CallbackFunc,
        PVOID              UserData)
{
    HRESULT retVal = S_OK;
    return retVal;
}


HRESULT   MV_Timer_Start(MV_HANDLE_Timer_t  *pHandle)
{
    HRESULT retVal = S_OK;
    return retVal;
}

HRESULT   MV_Timer_Stop(MV_HANDLE_Timer_t   *pHandle)
{
    HRESULT retVal = S_OK;
    return retVal;
}

HRESULT   MV_Timer_Resume(MV_HANDLE_Timer_t   *pHandle)
{
    HRESULT retVal = S_OK;
    return retVal;
}

HRESULT   MV_Timer_Pause(MV_HANDLE_Timer_t   *pHandle)
{
    HRESULT retVal = S_OK;
    return retVal;
}

HRESULT  MV_Timer_Reset_Period(MV_HANDLE_Timer_t    *pHandle, UINT  Period)
{
    HRESULT retVal = S_OK;
    return retVal;
}
#endif //MV_OSAL_TIMER_ENABLE

#ifdef MV_OSAL_MUTEX_ENABLE
HRESULT  MV_OSAL_Mutex_Create(MV_OSAL_HANDLE_MUTEX_t    *pHandle)
{
   return 0;
}

HRESULT  MV_OSAL_Mutex_Destroy(MV_OSAL_HANDLE_MUTEX_t   *pHandle)
{
   return 0;
}

HRESULT  MV_OSAL_Mutex_Lock(MV_OSAL_HANDLE_MUTEX_t  Handle)
{
   return 0;
}

HRESULT  MV_OSAL_Mutex_Unlock(MV_OSAL_HANDLE_MUTEX_t  Handle)
{
   return 0;
}
#endif //MV_OSAL_MUTEX_ENABLE

#ifdef MV_OSAL_SEM_ENABLE
HRESULT  MV_OSAL_Sem_Create(MV_OSAL_HANDLE_SEM_t    *pHandle, UINT  Value)
{
   return 0;
}

HRESULT  MV_OSAL_Sem_Destroy(MV_OSAL_HANDLE_SEM_t   *pHandle)
{
   return 0;
}

HRESULT  MV_OSAL_Sem_Wait(MV_OSAL_HANDLE_SEM_t  Handle)
{
   return 0;
}

HRESULT  MV_OSAL_Sem_Post(MV_OSAL_HANDLE_SEM_t  Handle)
{
   return 0;
}
HRESULT  MV_OSAL_Sem_TryWait(MV_OSAL_HANDLE_SEM_t        Handle)
{
   return 0;
}
#endif //MV_OSAL_SEM_ENABLE


#ifdef MV_OSAL_TASK_ENABLE

void MV_OSAL_TEE_Wait(unsigned long int DelayTime)
{

	//hack...
	int i;
	volatile int j=0;
	for(i=0; i<DelayTime*1000; i++)
	{
		j++;
		j++;
		j++;
		j++;
		j++;
		j++;
	}
	return;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Sleep
//!
//! Description: Sleep until time is out. (the time which is more than 10ms is better)
//!
//! \param TimeOutMS(IN): -- the millisecond of timeout to sleep
//!
//! \return Return:     S_OK
//!                     E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Sleep(UINT32 TimeOutMS)
{
    TEE_Wait(TimeOutMS);
    return S_OK;
}
#endif //MV_OSAL_TASK_ENABLE

void VPP_Enable_INT(UINT32 DhubSemMap, UINT32 Enable)
{
}

