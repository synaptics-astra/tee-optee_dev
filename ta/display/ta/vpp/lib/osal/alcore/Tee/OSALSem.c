/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


////////////////////////////////////////////////////////////////////////////////
//! \file OSALSem.c
//! \brief This is the Linux-specific implementation of the OSAL Semaphore croess-threads.
//!
//! Purpose:
//!
//!
//!	Version, Date and Author :
//!		V 1.00,	4/03/2007,	Fang Bao
//!		V 1.01,	5/16/2007,	Fang Bao
//!					Add the API MV_OSAL_Sem_TimedWait (need to link /usr/lib/librt.a in Linux)
//!		V 1.02,	12/18/2007,	Fang Bao
//!					Bug fixed :  semphore waiting will be interrupted in GDB of Linux OS.
//!								- MV_OSAL_Sem_Wait()
//!								- MV_OSAL_Sem_TryWait()
//!								- MV_OSAL_Sem_TimedWait()
//!		V 1.03,	02/03/2008,	Fang Bao
//!					Bug fixed :  MV_OSAL_Sem_Destroy() - when destroy failed, return and not MV_OSAL_Sem_Destructor
//!		V 1.04,	04/15/2008,	Fang Bao
//!					Remove some unuseful code
//!		V 1.05,	05/08/2008,	Fang Bao
//!					Bug fixed : MV_OSAL_Sem_Create() misses the return code in MV_OSAL_Sem_Create_Failure tag
//!		V 1.06,	06/24/2008,	Fang Bao
//!					Bug fixed : MV_OSAL_Sem_TimedWait()- replace MV_CC_NSEC_PER_SEC_64 with MV_CC_NSEC_PER_SEC_U64
//!		V 1.07,	2/04/2009,	Fang Bao
//!					add parameter checking in MV_OSAL_Sem_Create() and MV_OSAL_Sem_Destroy()
//!					add MV_OSAL_SEM_VALUE_MAX for parameter checking
//!		V 1.08,	12/29/2010,	Fang Bao,
//!                                     Bug fixed : MV_OSAL_Sem_Destroy(), to walk around android semaphore bug: when sem->count == 0, sem_destroy will failed with EBUSY.
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////



#include "tee_internal_api.h"

#include "OSAL_api.h"


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_Create
//!
//! Description: Create a OSAL count Semaphore (New Handle, Auto Constructor)
//!
//! \param pHandle 	   (OUT): -- the pointer to the Semaphore handle (New handle)
//! \param Value		(IN): -- the initial count for the semaphore object. This value must be greater than or equal to zero and less than or equal to MV_OSAL_SEM_VALUE_MAX
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_OUTOFMEMORY	| system is out of memory
//!						E_OUTOFRANGE	| the value is greater than MV_OSAL_SEM_VALUE_MAX
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_Create(MV_OSAL_HANDLE_SEM_t 	*pHandle,
									UINT					Value)
{
	HRESULT res = S_OK;
        TEE_SemaphoreCreate(pHandle, Value, NULL);
	return res;
}



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_Destroy
//!
//! Description: Destroy a OSAL count Semaphore (Auto Destructor)
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the Semaphore handle

//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_Destroy(MV_OSAL_HANDLE_SEM_t 	*pHandle)
{
	HRESULT res = S_OK;
        TEE_SemaphoreDestroy(*pHandle);
	return res;
}



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_Wait
//!
//! Description: Acquire the semaphore by decrementing the semaphore count.
//! 				if count = 0, no wait and return;
//! 				if count > 0, blocking wait until count = 0
//!
//! \param Handle 		(IN): -- the Semaphore handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_Wait(MV_OSAL_HANDLE_SEM_t 	Handle)
{
	HRESULT res = S_OK;
        TEE_SemaphoreWait(Handle);
	return res;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_TryWait
//!
//! Description: Acquire the semaphore by decrementing the semaphore count.
//! 				if count = 0, no wait and return;
//! 				if count > 0, non blocking wait and return.
//!
//! \param Handle 		(IN): -- the Semaphore handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_TryWait(MV_OSAL_HANDLE_SEM_t 	Handle)
{
	HRESULT res = S_OK;
    TEE_Result tRes;
    tRes = TEE_SemaphoreTryWait(Handle);
    if(tRes != TEE_SUCCESS) {
        res = S_FALSE;
    }
	return res;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_TimedWait
//!
//! Description: Acquire the semaphore by decrementing the semaphore count until time out
//! 				if count = 0, no wait and return;
//! 				if count > 0, blocking wait until time out.
//! warning:	You should set enough NanoSec for timeout, because there is some additional time for this function call
//!
//! \param Handle 		(IN): -- the Semaphore handle
//! \param NanoSec 		(IN): -- the nano seconds for time out (greater than 0)
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_TIMEOUT
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_TimedWait(MV_OSAL_HANDLE_SEM_t 	Handle,
							   UINT64					NanoSec)
{
	HRESULT res = S_OK;
        TEE_SemaphoreTimedWait(Handle, NanoSec);
	return res;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_Post
//!
//! Description: Release the semaphore by incrementing the semaphore count.
//!
//! \param Handle 		(IN): -- the Semaphore handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_Post(MV_OSAL_HANDLE_SEM_t 	Handle)
{
	HRESULT res = S_OK;
        TEE_SemaphorePost(Handle);
	return res;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_GetValue
//!
//! Description: get the value of a semaphore
//!
//! \param Handle 		(IN): -- the Semaphore handle
//! \param pValue 	   (OUT): -- the pointer to the value of the semaphore
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
