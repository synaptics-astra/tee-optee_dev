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



#include <linux/semaphore.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/jiffies.h>

#include "OSAL_api.h"


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_SEM_t 	| Object data structure
//!								pMV_OSAL_SEM_t 	| Pointer to the Object data structure
////////////////////////////////////////////////////////////////////////////////
typedef struct semaphore		MV_OSAL_SEM_t;
typedef struct semaphore 		*pMV_OSAL_SEM_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_Time_t 	| Object data structure
//!								pMV_OSAL_Time_t | Pointer to the Object data structure
////////////////////////////////////////////////////////////////////////////////
typedef struct timespec	MV_OSAL_Time_t;
typedef struct timespec *pMV_OSAL_Time_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_Constructor
//!
//! Description: Sem Handle Self Constructor
//!
//! \param self 		(OUT): -- the pointer to the sem handle
//!
//! \return Return:		S_OK
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_Constructor(MV_OSAL_HANDLE_SEM_t 	*self)
{
	pMV_OSAL_SEM_t pSem;

	pSem = (pMV_OSAL_SEM_t)MV_OSAL_Malloc(sizeof(MV_OSAL_SEM_t));
	if (pSem == NULL)
		MV_OSAL_DBG_Error(E_OUTOFMEMORY, "MV_OSAL_Sem_Constructor MV_OSAL_Malloc", NULL);

	*self = (MV_OSAL_HANDLE_SEM_t *)pSem;

	return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_Destructor
//!
//! Description: Sem Handle Self Destructor
//!
//! \param self 	(IN/OUT): -- the pointer to the sem handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_Destructor(MV_OSAL_HANDLE_SEM_t 	*self)
{
	/* Parameter Check */
	if ((*self) == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Sem_Destructor", NULL);

	MV_OSAL_Free(*self);

	return S_OK;
}


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
	HRESULT res;
	pMV_OSAL_SEM_t pSem = NULL;

	/* Parameter Check */
	if (pHandle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Sem_Create", NULL);

	if (Value > MV_OSAL_SEM_VALUE_MAX)
		MV_OSAL_DBG_Error(E_OUTOFRANGE, "MV_OSAL_Sem_Create", NULL);

	res = MV_OSAL_Sem_Constructor(pHandle);
	if (res != S_OK)
		MV_OSAL_DBG_Error(res, "MV_OSAL_Sem_Create MV_OSAL_Sem_Constructor", NULL);

	pSem = (pMV_OSAL_SEM_t)(*pHandle);

	sema_init(pSem, Value);
	if (pSem == NULL)
	{
		res = E_FAIL;
		MV_OSAL_DBG_Warning(res, "MV_OSAL_Sem_Create sem_init", NULL);
		goto MV_OSAL_Sem_Create_Failure;
	}

	MV_OSAL_DBG_Info("MV_OSAL_Sem_Create Handle[0x%08x] of Sem[%d]\n", (UINT)pSem, Value);

	return S_OK;

MV_OSAL_Sem_Create_Failure:

	res = MV_OSAL_Sem_Destructor(pHandle);

	MV_OSAL_DBG_Warning(res, "MV_OSAL_Sem_Create Failure", NULL);

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
	HRESULT res;
	pMV_OSAL_SEM_t pSem;

	/* Parameter Check */
	if ((pHandle == NULL) || ((*pHandle) == NULL))
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Sem_Destroy", NULL);

	res = S_OK;

	pSem = (pMV_OSAL_SEM_t)(*pHandle);

        // to walk around android semaphore bug: when sem->count == 0, sem_destroy will failed with EBUSY.
	MV_OSAL_DBG_Info("MV_OSAL_Sem_Destroy Sem's Handle[0x%08x]\n", (UINT)pSem);

	res = MV_OSAL_Sem_Destructor(pHandle);

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
	HRESULT res;
	pMV_OSAL_SEM_t pSem;

	/* Parameter Check */
	if (Handle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Sem_Wait", NULL);

	res = S_OK;

	pSem = (pMV_OSAL_SEM_t)(Handle);
	down(pSem);

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
	HRESULT res;
	pMV_OSAL_SEM_t pSem;

	/* Parameter Check */
	if (Handle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Sem_TryWait", NULL);

	res = S_OK;

	pSem = (pMV_OSAL_SEM_t)(Handle);

	while ((down_trylock(pSem)) != 0)
	{
		res = S_FALSE;
		MV_OSAL_DBG_Warning(res, "MV_OSAL_Sem_TryWait sem_trywait errno ERROR");
		break;
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
	HRESULT res;
	pMV_OSAL_SEM_t pSem;

	/* Parameter Check */
	if (Handle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Sem_TimedWait", NULL);

	res = S_OK;

	pSem = (pMV_OSAL_SEM_t)(Handle);

	while ((down_timeout(pSem, msecs_to_jiffies(NanoSec/(1000*1000)))) != 0)
	{
		res = S_FALSE;
		MV_OSAL_DBG_Warning(res, "MV_OSAL_Sem_TimedWait sem_timedwait errno ERROR");
		break;
	}

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
	HRESULT res;
	pMV_OSAL_SEM_t pSem;

	/* Parameter Check */
	if (Handle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Sem_Post", NULL);

	res = S_OK;

	pSem = (pMV_OSAL_SEM_t)(Handle);
	up(pSem);

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
HRESULT  MV_OSAL_Sem_GetValue(MV_OSAL_HANDLE_SEM_t 	Handle,
									 INT					*pValue)
{
	HRESULT res;
	pMV_OSAL_SEM_t pSem;

	/* Parameter Check */
	if ((Handle == NULL) || (pValue == NULL))
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Sem_GetValue", NULL);

	res = S_OK;

	pSem = (pMV_OSAL_SEM_t)(Handle);

	*pValue = pSem->count;

	return res;
}


