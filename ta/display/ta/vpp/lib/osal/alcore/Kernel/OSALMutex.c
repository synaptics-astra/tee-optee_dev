/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


////////////////////////////////////////////////////////////////////////////////
//! \file OSALMutex.c
//! \brief This is the Linux-specific implementation of the OSAL Mutex croess-threads.
//!
//! Purpose:
//!
//!
//!	Version, Date and Author :
//!		V 1.00,	4/03/2007,	Fang Bao
//!		V 1.01,	7/26/2007,	Fang Bao
//!					Add the Condition APIs
//!		V 1.10,	11/28/2007,	Fang Bao
//!					Modified Condition APIs to be compatible with eCos Condition APIs
//!		V 1.11,	5/08/2007,	Fang Bao
//!					Bug fixed : MV_OSAL_Mutex_Create() misses the return code in MV_OSAL_Mutex_Create_Failure tag
//!					Add the debug trace in MV_OSAL_Mutex_Create() and MV_OSAL_Mutex_Destroy()
//!		V 1.14,	12/18/2008,	Fang Bao	Add Parameter Check in MV_OSAL_Mutex_Create(), MV_OSAL_Mutex_Destroy()
//!		V 1.15,	12/24/2008,	Fang Bao	Add Parameter Check in MV_OSAL_Cond_Create()
//!		V 1.15,	6/11/2008,	Fang Bao	remove warning in MV_OSAL_Cond_Create()
//!
//! Note:
//!		The recursive mutex is not available in OSAL of Linux.
//!		It is advised that an application should not use a PTHREAD_MUTEX_RECURSIVE mutex with
//!		condition variables because the implicit unlock performed for a pthread_cond_wait() or
//!		pthread_cond_timedwait() may not actually release the mutex (if it had been locked multiple times).
//!		If this happens, no other thread can satisfy the condition of the predicate.
//!
////////////////////////////////////////////////////////////////////////////////

#include <linux/mutex.h>

#include "OSAL_api.h"



////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_MUTEX_t 	| Object data structure
//!								pMV_OSAL_MUTEX_t 	| Pointer to the Object data structure
////////////////////////////////////////////////////////////////////////////////
typedef struct mutex 		MV_OSAL_MUTEX_t;
typedef struct mutex		*pMV_OSAL_MUTEX_t;



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_Constructor
//!
//! Description: Mutex Handle Self Constructor
//!
//! \param self 		(OUT): -- the pointer to the Mutex handle
//!
//! \return Return:		S_OK
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Constructor(MV_OSAL_HANDLE_MUTEX_t 	*self)
{
	pMV_OSAL_MUTEX_t pMutex;

	pMutex = (pMV_OSAL_MUTEX_t)MV_OSAL_Malloc(sizeof(MV_OSAL_MUTEX_t));
	if (pMutex == NULL)
		MV_OSAL_DBG_Error(E_OUTOFMEMORY, "MV_OSAL_Mutex_Constructor MV_OSAL_Malloc", NULL);

	*self = (MV_OSAL_HANDLE_MUTEX_t *)pMutex;

	return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_Destructor
//!
//! Description: Mutex Handle Self Destructor
//!
//! \param self 	(IN/OUT): -- the pointer to the Mutex handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Destructor(MV_OSAL_HANDLE_MUTEX_t 	*self)
{
	/* Parameter Check */
	if ((*self) == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Mutex_Destructor", NULL);

	MV_OSAL_Free(*self);

	return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_Create
//!
//! Description: Create a OSAL Mutex (New Handle, Auto Constructor)
//!
//! \param pHandle 	   (OUT): -- the pointer to the Mutex handle (New handle)
//!
//! \return Return:		S_OK
//!						S_FALSE
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Create(MV_OSAL_HANDLE_MUTEX_t 	*pHandle)
{
	HRESULT res;
	pMV_OSAL_MUTEX_t pMutex = NULL;

	/* Parameter Check */
	if (pHandle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Mutex_Create", NULL);

	res = MV_OSAL_Mutex_Constructor(pHandle);
	if (res != S_OK)
		MV_OSAL_DBG_Error(res, "MV_OSAL_Mutex_Create MV_OSAL_Mutex_Constructor", NULL);

	pMutex = (pMV_OSAL_MUTEX_t)(*pHandle);
	mutex_init(pMutex);

	MV_OSAL_DBG_Info("(+) Mutex=%ph\n", pMutex);

	return S_OK;
}



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_Destroy
//!
//! Description: Destroy a OSAL Mutex (Auto Destructor)
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the Mutex handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Destroy(MV_OSAL_HANDLE_MUTEX_t 	*pHandle)
{
	HRESULT res;
	pMV_OSAL_MUTEX_t pMutex;

	/* Parameter Check */
	if ((pHandle == NULL) || ((*pHandle) == NULL))
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Mutex_Destroy", NULL);

	res = S_OK;

	pMutex = (pMV_OSAL_MUTEX_t)(*pHandle);

	MV_OSAL_DBG_Info("(-) Mutex=%ph\n", pMutex);

	res = MV_OSAL_Mutex_Destructor(pHandle);

	return res;
}



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_Lock
//!
//! Description: acquire a lock on the specified mutex variable.
//!		If the mutex is already locked by another thread,
//!		this call will block the calling thread until the mutex is unlocked.
//!
//! \param Handle 		(IN): -- the Mutex handle
//!
//! \return Return:		S_OK
//!						S_FALSE
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Lock(MV_OSAL_HANDLE_MUTEX_t 	Handle)
{
	HRESULT res;
	pMV_OSAL_MUTEX_t pMutex;

	/* Parameter Check */
	if (Handle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Mutex_Lock", NULL);

	res = S_OK;

	pMutex = (pMV_OSAL_MUTEX_t)(Handle);
	mutex_lock(pMutex);

	return res;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_TryLock
//!
//! Description: attempt to lock a mutex. However, if the mutex is already locked,
//! 	the routine will return immediately with a "busy" error code.
//! 	This routine may be useful in preventing deadlock conditions,
//! 	as in a priority-inversion situation.
//!
//! \param Handle 		(IN): -- the Mutex handle
//!
//! \return Return:		S_OK
//!						S_FALSE
//!						E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_TryLock(MV_OSAL_HANDLE_MUTEX_t 	Handle)
{
	HRESULT res;
	pMV_OSAL_MUTEX_t pMutex;

	/* Parameter Check */
	if (Handle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Mutex_TryLock", NULL);

	res = S_OK;

	pMutex = (pMV_OSAL_MUTEX_t)(Handle);

	if ((res = mutex_trylock(pMutex)) != 0)
	{
		if (res == EBUSY )
			res = E_NOTREADY;
		else
		{
			res = S_FALSE;
			MV_OSAL_DBG_Warning(res, "MV_OSAL_Mutex_TryLock pthread_mutex_trylock", NULL);
		}
	}

	return res;
}



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_Unlock
//!
//! Description: unlock a mutex if called by the owning thread.
//!		Calling this routine is required after a thread has completed its use of protected data
//!		if other threads are to acquire the mutex for their work with the protected data.
//!
//! \param Handle 		(IN): -- the Mutex handle
//!
//! \return Return:		S_OK
//!						S_FALSE
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Unlock(MV_OSAL_HANDLE_MUTEX_t 	Handle)
{
	HRESULT res;
	pMV_OSAL_MUTEX_t pMutex;

	/* Parameter Check */
	if (Handle == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Mutex_Unlock", NULL);

	res = S_OK;

	pMutex = (pMV_OSAL_MUTEX_t)(Handle);
	mutex_unlock(pMutex);

	return res;
}

