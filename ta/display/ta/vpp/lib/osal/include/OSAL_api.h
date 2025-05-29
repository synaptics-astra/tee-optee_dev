/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


////////////////////////////////////////////////////////////////////////////////
//! \file OSAL_api.h
//! \brief This is the NoOS-specific implementation of the OSAL APIs.
//!
//! Purpose:
//!
//!
//!	Version, Date and Author :
//!		V 1.00,	1/14/2007,	Fang Bao
//!		V 1.01,	02/02/2007,	Fang Bao,       Add APIs MV_CC_CBuf_GetSpaceSizeAndNoRoll() and MV_CC_CBuf_GetDataSizeAndNoRoll()
//!		V 1.02,	02/05/2007,	Fang Bao,       New feature - Add MV_CC_CBUF_FLAGS_LOCALCPU flag in MV_CC_CBuf_Create() parameter
//!							                Then the buffer will be created in cacheable memory of local CPU.
//!		V 1.03,	03/27/2007,	Fang Bao,       Modify MV_SHM_NON_CACHE_ADDR() and MV_SHM_CACHE_ADDR() to remove warnings
//!		V 1.04,	03/27/2007,	Fang Bao,       Modify MV_OSAL_Task_CleanUp_Push() and MV_OSAL_Task_CleanUp_Pop() to remove warnings
//!		V 1.05,	04/17/2007,	Fang Bao,       New APIs - MV_CC_CBuf_GetRdOffset(), MV_CC_CBuf_GetWrOffset(), MV_CC_CBuf_SetRdOffset(), MV_CC_CBuf_SetWrOffset() for Demux Module(Ailing Du)
//!		V 1.06,	5/06/2008,  Fang Bao,       Add macro - MV_OSAL_Task_NMsgsMin which used by MV_OSAL_Task_Create_Advanced() and MV_OSAL_Task_CreateWithMsgQ()
//!		V 1.07,	5/08/2008,	Fang Bao,       Remove all about MV_Timer_Virtual_Clock_XXX in Windows OS.
//!		V 1.08,	5/28/2008,	Fang Bao,       Remove some warnings on compiling in AF.
//!		V 1.09,	6/02/2008,	Fang Bao,       Modify the MV_TIMER_TASK_PRIORITY_XXX in Linux/eCos/Win32 for [Bug 819]
//!							                Reserve MV_TIMER_TASK_PRIORITY_0 and MV_TIMER_TASK_PRIORITY_31
//!							                Add MV_TIMER_TASK_PRIORITY_HIGHEST and MV_TIMER_TASK_PRIORITY_LOWEST
//!		V 1.09a,6/05/2008,	Fang Bao,       Replace error code E_NOPERMISSION by E_ACCESSDENIED
//!		V 1.09b,7/22/2008,	Fang Bao,       Modify the comment of MV_CC_RPCClnt_Call
//!		V 1.10, 9/03/2008,	Fang Bao,       Add MV_OSAL_Task_Create_Detailed()
//!		V 1.11, 9/09/2008,	Fang Bao,       Add new CC Service APIs for Message Queue Extension Mode (MsgQEx)
//!		V 1.12,	2/04/2009,	Fang Bao,       Modify some comments of APIs
//!		V 1.13,	2/18/2009,	Fang Bao,       Correct some comments of APIs
//!							                use MV_CC_MSGQ_DATA_MSGEXLEN_MAX to replace MV_CC_MSGQ_DATA_BYTESIZE_MAX
//!		V 1.14,	2/24/2009,	Fang Bao,       Remove a build warning in MV_OSAL_Task_CleanUp_Pop()
//!		V 1.15,	7/20/2009,	Fang Bao,       Add MV_CC_CBuf_Reset_WrOffset() - new API to reset buffer, set writing pointer to reading pointer
//!  						                warning!!! this operation is dangerous, maybe conflicted with consumer. m_RdOffset is operated by consumer
//!		V 1.16,	8/07/2009,	Fang Bao,       Add APIs - MV_Timer_Pause() and MV_Timer_Resume() which are available on Linux and eCos OS
//!		V 1.17,	8/10/2009,	Fang Bao,       Add APIs - MV_Timer_Reset_Period()
//!		V 1.17,	9/14/2009,	Fang Bao,       Add APIs - MV_CC_RPC_Call() for Simplified flow of RPC Client, instead of MV_CC_RPCClnt_Create() + MV_CC_RPCClnt_Call() + MV_CC_RPCClnt_Destroy()
//!		V 1.18, 10/23/2009,	Fang Bao,       Add CCDSS, CCDSP support
//!							                Add API - MV_CC_RPCSrv_GetServiceID()
//!		V 1.19, 10/28/2009,	Fang Bao,       Add APIs - MV_CC_MsgQ_GetServiceID(), MV_CC_MsgQEx_GetServiceID()
//!     V 1.20, 01/26/2010, Fang Bao,       remove headfile MV_SHM_def.h which was not used
//!                                         add API - MV_CC_SHM_GetStartAddress() to get the start addres of shared memroy in linux OS.
//!     V 1.21, 02/04/2010, Fang Bao,       add API - MV_CC_SHM_GetDeviceFileID() to get the file id of shared memroy device in linux OS.
//!                                         add API - MV_CC_SHM_GetInfo() to get information of Shared Memory Pool
//!     V 1.22, 04/29/2010, Fang Bao,       add APIs - MV_CC_SHM_GetDeviceFileID_CACHE(), MV_CC_SHM_GetStartAddress_CACHE()
//!     V 1.23, 04/29/2010, Fang Bao,       add APIs - MV_SHM_NON_CACHE_ADDR(), MV_SHM_CACHE_ADDR() to replace macro definition in Linux OS, for multi-process loading
//!                                         add APIs - MV_SHM_PhysAddrToVirtAddr(), MV_SHM_VirtAddrToPhysAddr() to replace macro definition in Linux OS, for multi-process loading
//!                                         remove macro definition MV_SHM_Start_VirtAddr and MV_SHM_Start_VirtAddr_CACHE in Linux OS
//!                                         remove macro definition MV_SHM_PhysAddrToVirtAddr and MV_SHM_VirtAddrToPhysAddr in Linux OS
//!		V 1.24,	08/03/2010,	Fang Bao,       Add APIs - MV_CC_CBuf_Reset_WrRdZero(), to reset buffer Wr and Rd by Zero
//!		V 1.25,	08/10/2010,	Fang Bao,       Remove API - MV_CC_CBuf_Reset_WrRdZero() and modify MV_CC_CBuf_Reset() to replace MV_CC_CBuf_Reset_WrRdZero() with it
//!		V 1.26,	09/15/2010,	Fang Bao,       Add APIs to support cache safe shared memory access - MV_CC_SHM_Malloc_Cache(), MV_CC_SHM_Get_Cache(), MV_CC_SHM_Free_Cache(), MV_CC_SHM_FreeKey_Cache()
//!                                         Modify APIs - MV_CC_SHM_Get() could support get pointer from key created by MV_CC_SHM_Malloc_Cache()
//!                                         Modify APIs - MV_CC_SHM_FreeKey() and MV_CC_SHM_FreeKey_Cache() could support free memory from key created by both MV_CC_SHM_Malloc_Cache() and MV_CC_SHM_Malloc()
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef _OSAL_API_H_
#define _OSAL_API_H_

#include "com_type.h"
#include "OSAL_config.h"
#include "OSAL_type.h"
#include "GaloisTime.h"

#ifndef CONFIG_OPTEE
#include "tee_internal_private_api.h"
#else
#include "tee_api_syna.h"
#endif
#include "tee_ta_osal.h"
#ifdef __cplusplus
extern "C"
{
#endif	// #ifdef __cplusplus

/****************************** OSAL *******************************************/

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Init
//!
//! Description: OSAL system initialize; MV_OSAL_Init must match MV_OSAL_Exit, only first MV_OSAL_Init is available.
//!
//! \param :			None.
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Init(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Exit
//!
//! Description: OSAL system exit. MV_OSAL_Exit must match MV_OSAL_Init, only last MV_OSAL_Exit is available.
//!
//! \param :			None.
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Exit(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_GetSysSrvStatus
//!
//! Description: Get the Service status
//!
//! \param ServiceID 	(IN): -- the ID of the Service
//!
//! \return Return:		S_OK
//!						E_NOTREADY	Service not ready
//!						E_FAIL  	System not ready
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_GetSysSrvStatus(MV_CC_ServiceID_U32_t ServiceID);


/****************************** OSAL SEM*******************************************/




////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_HANDLE_SEM_t | Handle to a semaphore object
//!
////////////////////////////////////////////////////////////////////////////////
//typedef MV_OSAL_HANDLE 	MV_OSAL_HANDLE_SEM_t;
//typedef MV_OSAL_HANDLE 	*pMV_OSAL_HANDLE_SEM_t;


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
							UINT					Value);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_Destroy
//!
//! Description: Destroy a OSAL count Semaphore (Auto Destructor)
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the Semaphore handle

//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_Destroy(MV_OSAL_HANDLE_SEM_t 	*pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_Wait
//!
//! Description: Acquire the semaphore by decrementing the semaphore count.
//! 				if count = 0, blocking wait until it be posted
//! 				if count > 0, count--, no wait and return;
//!
//! \param Handle 		(IN): -- the Semaphore handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_Wait(MV_OSAL_HANDLE_SEM_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_TryWait
//!
//! Description: Acquire the semaphore by decrementing the semaphore count.
//! 				if count = 0, blocking wait until it be posted
//! 				if count > 0, count--, no wait and return;
//!
//! \param Handle 		(IN): -- the Semaphore handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Sem_TryWait(MV_OSAL_HANDLE_SEM_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Sem_TimedWait
//!
//! Description: Acquire the semaphore by decrementing the semaphore count until time out
//! 				if count = 0, blocking wait until it be posted
//! 				if count > 0, count--, no wait and return;
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
									  UINT64				NanoSec);

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
HRESULT  MV_OSAL_Sem_Post(MV_OSAL_HANDLE_SEM_t 	Handle);

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
									 INT					*pValue);



/****************************** OSAL MUTEX*******************************************/



////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_HANDLE_MUTEX_t | Handle to a mutex object
//!
////////////////////////////////////////////////////////////////////////////////
//typedef MV_OSAL_HANDLE 	MV_OSAL_HANDLE_MUTEX_t;
//typedef MV_OSAL_HANDLE 	*pMV_OSAL_HANDLE_MUTEX_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_Create
//!
//! Description: Create a OSAL Mutex (New Handle, Auto Constructor)
//!
//! \param pHandle 	   (OUT): -- the pointer to the Mutex handle (New handle)
//!
//! \return Return:		S_OK
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Create(MV_OSAL_HANDLE_MUTEX_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Mutex_Destroy
//!
//! Description: Destroy a OSAL Mutex (Auto Destructor)
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the Mutex handle

//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Destroy(MV_OSAL_HANDLE_MUTEX_t 	*pHandle);

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
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Lock(MV_OSAL_HANDLE_MUTEX_t 	Handle);


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
//!						E_FAIL
//!						E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_TryLock(MV_OSAL_HANDLE_MUTEX_t 	Handle);

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
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Mutex_Unlock(MV_OSAL_HANDLE_MUTEX_t 	Handle);


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_HANDLE_COND_t | Handle to a condition object
//!
////////////////////////////////////////////////////////////////////////////////
typedef MV_OSAL_HANDLE 	MV_OSAL_HANDLE_COND_t;
typedef MV_OSAL_HANDLE 	*pMV_OSAL_HANDLE_COND_t;


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Cond_Create
//!
//! Description: Create a OSAL Condition (New Handle, Auto Constructor)
//!
//! \param pHandle 	   (OUT): -- the pointer to the Condition handle (New handle)
//! \param MutexHandle 	(IN): -- the Mutex handle
//!
//! \return Return:		S_OK
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Cond_Create(MV_OSAL_HANDLE_COND_t 		*pHandle,
							 MV_OSAL_HANDLE_MUTEX_t 	MutexHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Cond_Destroy
//!
//! Description: Destroy a OSAL Condition (Auto Destructor)
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the Condition handle

//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Cond_Destroy(MV_OSAL_HANDLE_COND_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Cond_Wait
//!
//! Description: wait on a condition
//!
//! \param CondHandle 		(IN): -- the Condition handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Cond_Wait(MV_OSAL_HANDLE_COND_t 	CondHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Cond_TimedWait
//!
//! Description: wait on a condition until time is out
//!
//! \param CondHandle 		(IN): -- the Condition handle
//! \param MutexHandle 		(IN): -- the Mutex handle
//! \param pTimeOut 		(IN): -- the pointer to the absolute timeout
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_TIMEOUT
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Cond_TimedWait(MV_OSAL_HANDLE_COND_t 	CondHandle,
						   		MV_TimeSpec_t			*pTimeOut);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Cond_Signal
//!
//! Description: signal a condition
//!
//! \param CondHandle 		(IN): -- the Condition handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Cond_Signal(MV_OSAL_HANDLE_COND_t 	CondHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Cond_Broadcast
//!
//! Description: broadcast a condition
//!
//! \param CondHandle 		(IN): -- the Condition handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Cond_Broadcast(MV_OSAL_HANDLE_COND_t 	CondHandle);


/****************************** OSAL RW_LOCK*******************************************/



////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_HANDLE_RWLOCK_t | Handle to a rwlock object
//!
////////////////////////////////////////////////////////////////////////////////
typedef MV_OSAL_HANDLE 	MV_OSAL_HANDLE_RWLOCK_t;
typedef MV_OSAL_HANDLE 	*pMV_OSAL_HANDLE_RWLOCK_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_RWLock_Create
//!
//! Description: Create a OSAL rwlock (New Handle, Auto Constructor)
//!
//! \param pHandle 	   (OUT): -- the pointer to the rw lock handle (New handle)
//!
//! \return Return:		S_OK
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_RWLock_Create(MV_OSAL_HANDLE_RWLOCK_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_RWLock_Destroy
//!
//! Description: Destroy a OSAL rwlock (Auto Destructor)
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the rwlock handle

//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_RWLock_Destroy(MV_OSAL_HANDLE_RWLOCK_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_RWLock_RDLock
//!
//! Description: acquire a lock on the specified rwlock variable.
//!		If the rwlock is already locked by another thread,
//!		this call will block the calling thread until the rwlock is unlocked.
//!
//! \param Handle 		(IN): -- the rwlock handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_RWLock_RDLock(MV_OSAL_HANDLE_RWLOCK_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_RWLock_TryRDLock
//!
//! Description: attempt to lock a rwlock. However, if the rwlock is already locked,
//! 	the routine will return immediately with a "busy" error code.
//! 	This routine may be useful in preventing deadlock conditions,
//! 	as in a priority-inversion situation.
//!
//! \param Handle 		(IN): -- the rwlock handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_RWLock_TryRDLock(MV_OSAL_HANDLE_RWLOCK_t 	Handle, INT64 timeout);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_RWLock_WRLock
//!
//! Description: acquire a lock on the specified rwlock variable.
//!		If the rwlock is already locked by another thread,
//!		this call will block the calling thread until the rwlock is unlocked.
//!
//! \param Handle 		(IN): -- the rwlock handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_RWLock_WRLock(MV_OSAL_HANDLE_RWLOCK_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_RWLock_TryWRLock
//!
//! Description: acquire a lock on the specified rwlock variable.
//!		If the rwlock is already locked by another thread,
//!		this call will block the calling thread until the rwlock is unlocked.
//!
//! \param Handle 		(IN): -- the rwlock handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_RWLock_TryWRLock(MV_OSAL_HANDLE_RWLOCK_t 	Handle, INT64 timeout);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_RWLock_Unlock
//!
//! Description: unlock a rwlock if called by the owning thread.
//!		Calling this routine is required after a thread has completed its use of protected data
//!		if other threads are to acquire the rwlock for their work with the protected data.
//!
//! \param Handle 		(IN): -- the rwlock handle
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_RWLock_Unlock(MV_OSAL_HANDLE_RWLOCK_t 	Handle);


/****************************** OSAL TASK*******************************************/


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_HANDLE_TASK_t | Handle to a task object
//!
////////////////////////////////////////////////////////////////////////////////
//typedef MV_OSAL_HANDLE 	MV_OSAL_HANDLE_TASK_t;
//typedef MV_OSAL_HANDLE 	*pMV_OSAL_HANDLE_TASK_t;

#if defined( __LINUX_KERNEL__ ) || defined(CONFIG_AMP_TEE_ENABLE)

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_Exit | exit a task
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_Exit(x)


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_CleanUp_Push | push the specified cancellation cleanup handler routine onto the calling thread's cancellation cleanup stack.
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_CleanUp_Push(x,y)


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_CleanUp_Pop | remove the routine at the top of the calling thread's cancellation cleanup stack and optionally invoke it (if execute is non-zero).
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_CleanUp_Pop(x)

#elif defined( __LINUX__ )
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_Exit | exit a task
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_Exit(x)


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_CleanUp_Push | push the specified cancellation cleanup handler routine onto the calling thread's cancellation cleanup stack.
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_CleanUp_Push(x,y)	pthread_cleanup_push((x),(y))


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_CleanUp_Pop | remove the routine at the top of the calling thread's cancellation cleanup stack and optionally invoke it (if execute is non-zero).
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_CleanUp_Pop(x)		pthread_cleanup_pop(x)

#elif defined( __ECOS__ )
#include <cyg/kernel/kapi.h>

void  MV_OSAL_Task_ExitForeCos();

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_Exit | exit a task
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_Exit(x)		MV_OSAL_Task_ExitForeCos()

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_CleanUp_Push | push the specified cancellation cleanup handler routine onto the calling thread's cancellation cleanup stack.
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_CleanUp_Push(x,y)		cyg_thread_add_destructor( (cyg_thread_destructor_fn)(x), (cyg_addrword_t)(y) )

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_CleanUp_Pop | remove the routine at the top of the calling thread's cancellation cleanup stack and optionally invoke it (if execute is non-zero).
//!
////////////////////////////////////////////////////////////////////////////////
// !!! the behavior and arguments of cyg_thread_rem_destructor is different from pthread_cleanup_pop, use it carefully
#define MV_OSAL_Task_CleanUp_Pop(x,y)		cyg_thread_rem_destructor( (cyg_thread_destructor_fn)(x), (cyg_addrword_t)(y) )

#elif defined( WIN32 )
#include "windows.h"
#pragma warning ( disable: 4311 ) //warning C4311: 'type cast' : pointer truncation from 'void *' to 'DWORD'
////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_Exit | exit a task
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_Exit(x)		ExitThread((DWORD)(x))

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_CleanUp_Push | push the specified cancellation cleanup handler routine onto the calling thread's cancellation cleanup stack.
//!
////////////////////////////////////////////////////////////////////////////////
#define MV_OSAL_Task_CleanUp_Push(x,y)

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Macro: MV_OSAL_Task_CleanUp_Pop | remove the routine at the top of the calling thread's cancellation cleanup stack and optionally invoke it (if execute is non-zero).
//!
////////////////////////////////////////////////////////////////////////////////
// !!! There is no pthread_cleanup_pop counterpart in Win32, use it carefully
#define MV_OSAL_Task_CleanUp_Pop(x)

#endif

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Set_CPUAffinity
//!
//! Description: Set overall tasks' CPU affinity
//!
//! \param :			None.
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Set_CPUAffinity(UINT affinity);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Init_Task
//!
//! Description: OSAL Task system initialize
//!
//! \param :			None.
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Init_Task(void);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Exit_Task
//!
//! Description: OSAL Task system exit
//!
//! \param :			None.
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Exit_Task(void);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Create_Advanced
//!
//! Description: Create a OSAL advance task (New Handle, Auto Constructor)
//!
//! \param pHandle 	   (OUT): -- the pointer to the task handle (New handle)
//! \param TaskFunc		(IN): -- the pointer of the task function
//! \param Arg	 		(IN): -- the argument to the TaskFunction()
//! \param Priority 	(IN): -- the task priority
//! \param NMsgsMax		(IN): -- the max number of messages in the Message Queue
//!
//! \return Return:		S_OK
//!						E_TASKCREATE
//!						E_INVALIDARG
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Create_Advanced(MV_OSAL_HANDLE_TASK_t 	*pHandle,
										VOID					*TaskFunc,
                           				VOID 					*Arg,
                           				MV_OSAL_PRIORITY_t const Priority,
                           				UINT32 const			NMsgsMax);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Create_Detailed
//!
//! Description: Create a OSAL advance task with all detailed parameter
//!
//! \param pHandle 	   (OUT): -- the pointer to the task handle (New handle)
//! \param TaskFunc		(IN): -- the pointer of the task function
//! \param Arg	 		(IN): -- the argument to the TaskFunction()
//! \param Priority 	(IN): -- the task priority
//! \param pName 		(IN): -- the pointer to the task name string (NULL = Task)(only available in eCos)
//! \param pStackBase 	(IN): -- the pointer to the stack base (NULL = allocate) Now unavailable
//! \param StackSize 	(IN): -- the pointer to the stack size (0 == default size) Now unavailable
//! \param NMsgsMax		(IN): -- the max number of messages in the Message Queue
//!
//! \return Return:		S_OK
//!						E_TASKCREATE
//!						E_INVALIDARG
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Create_Detailed(MV_OSAL_HANDLE_TASK_t 	*pHandle,
										VOID					*TaskFunc,
                           				VOID 					*Arg,
                           				MV_OSAL_PRIORITY_t const Priority,
                           				CHAR					*pName,
                           				VOID					*pStackBase,
                           				UINT32					StackSize,
                           				UINT32 const			NMsgsMax);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_CreateWithMsgQ
//!
//! Description: Create a OSAL task (New Handle, Auto Constructor) with a message queue (ITC MsgQ)
//!
//! \param pHandle 	   (OUT): -- the pointer to the task handle (New handle)
//! \param TaskFunc		(IN): -- the pointer of the task function
//! \param Arg	 		(IN): -- the argument to the TaskFunction()
//! \param NMsgsMax		(IN): -- the max number of messages in the Message Queue
//!
//! \return Return:		S_OK
//!						E_TASKCREATE
//!						E_INVALIDARG
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_CreateWithMsgQ(MV_OSAL_HANDLE_TASK_t 	*pHandle,
									VOID					*TaskFunc,
                           			VOID 					*Arg,
                           			UINT32 const			NMsgsMax);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Create
//!
//! Description: Create a OSAL task (New Handle, Auto Constructor)
//!
//! \param pHandle 	   (OUT): -- the pointer to the task handle (New handle)
//! \param TaskFunc		(IN): -- the pointer of the task function
//! \param Arg	 		(IN): -- the argument to the TaskFunction()
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Create(MV_OSAL_HANDLE_TASK_t 	*pHandle,
									VOID					*TaskFunc,
                           			VOID 					*Arg);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_SetName(CHAR *pName)
//!
//! Description: set the thread name (New Handle, Auto Constructor)
//!
//! \param pName 		(IN): -- the pointer of the task name
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_SetName(CHAR *pName);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Destroy
//!
//! Description: Destroy a OSAL task (Auto Destructor)
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the task handle
//!
//! \return Return:		S_OK
//!						E_TASKDESTROY
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Destroy(MV_OSAL_HANDLE_TASK_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Detach_
//!
//! Description: detach the task after created or in the thread body function, hence system will reclain resource automatically
//! the task with MV_OSAL_Task_Create_Andvanced are joinable, should destroy the task with MV_OSAL_Task_Join.
//! while there are task don't need be joinable, just Detach the task and call MV_OSAL_Destroy to release the resource.
//! Note: this function just detach the task(thread), you still need call MV_OSAL_Destroy to do what you expected.
//! \param pHandle 	(IN/OUT): -- the pointer to the task handle
//!
//! \return Return:		S_OK
//!						E_TASKFAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Detach(MV_OSAL_HANDLE_TASK_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Join
//!
//! Description: Block the calling task until the specified task terminates and
//!					destroy it  (Auto Destructor)
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the task handle
//!
//! \return Return:		S_OK
//!						E_TASKFAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Join(MV_OSAL_HANDLE_TASK_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Yield
//!
//! Description: Yield the processor
//!
//! \param NULL
//!
//! \return Return:		S_OK
//!						E_TASKFAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Yield(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Sleep
//!
//! Description: Sleep until time is out. (the time which is more than 10ms is better)
//!
//! \param TimeOutMS(IN): -- the millisecond of timeout to sleep
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Sleep(UINT32 TimeOutMS);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Self
//!
//! Description: Get the task Handle of calling task
//!
//! \param pHandle 	(IN/OUT): -- the pointer to the task handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Self(MV_OSAL_HANDLE_TASK_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_MsgQ_Post
//!
//! Description: Post the Message into Message Queue of the Task  (ITC MsgQ)
//!
//! \param pHandle 	(IN): -- the pointer to the task handle
//! \param pMSG 	(IN): -- the pointer to the Message Data Which Structure is MV_CC_ITC_MSG_t
//!
//! \return Return:		S_OK
//!						E_NOTEXIST		| the queue dosn't exist
//!						E_FULL			| the queue is full
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_OSAL_Task_MsgQ_Post(MV_OSAL_HANDLE_TASK_t *pHandle, PVOID pMSG);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_MsgQ_Get
//!
//! Description: Get the Message From Message Queue of the Task. If there is no message, wait until new message arrives.  (ITC MsgQ)
//!
//! \param pHandle 	(IN): -- the pointer to the task handle
//! \param pMSG 	(OUT): -- the pointer to the Message Data Which Structure is MV_CC_ITC_MSG_t
//! \param TimeOutMS(IN): -- the millisecond of timeout to wait for getting message
//!
//! \return Return:		S_OK
//!						E_NOTEXIST		| the queue dosn't exist
//!						E_TIMEOUT		| the queue is empty until time is out
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_OSAL_Task_MsgQ_Get(MV_OSAL_HANDLE_TASK_t *pHandle, PVOID pMSG, UINT32 TimeOutMS);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_MsgQ_GetTry
//!
//! Description: Get the Message From Message Queue of the Task. If there is no message, return right now.  (ITC MsgQ)
//!
//! \param pHandle 	(IN): -- the pointer to the task handle
//! \param pMSG 	(OUT): -- the pointer to the Message Data Which Structure is MV_CC_ITC_MSG_t
//!
//! \return Return:		S_OK
//!						E_NOTEXIST		| the queue dosn't exist
//!						E_NOTREADY		| the queue is empty
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_OSAL_Task_MsgQ_GetTry(MV_OSAL_HANDLE_TASK_t *pHandle, PVOID pMSG);


/****************************** OSAL TIMER*******************************************/


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MVHANDLE_Time_t | Handle to a timer object
//!
////////////////////////////////////////////////////////////////////////////////
//typedef MV_OSAL_HANDLE 	MV_HANDLE_Timer_t;
//typedef MV_OSAL_HANDLE 	*pMV_HANDLE_Timer_t;


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Create
//!
//! Description: Create a timer to call the user procedure in the future
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//! \param Period  		  (IN):	The value of the time out for the timer in milliseconds
//! \param CallbackFunc	  (IN):	The pointer to the user procedure
//! \param UserData		  (IN):	The pointer to parameter of the user procedure
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//! \brief Function:    CallbackFunc Templete
//!
//! The CallbackFunc is the same between Linux & eCos and Windows.
//!
//! -	void CallbackFunc(UINT32 UserData);
//!
//!
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Create(MV_HANDLE_Timer_t	*pHandle,
						 UINT				Period,
						 PVOID				CallbackFunc,
						 PVOID				UserData);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Destroy
//!
//! Description: Destroy a timer (before destroying, the timer will be stopped)
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Destroy(MV_HANDLE_Timer_t	*pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Reset
//!
//! Description: Reset a timer (before reseting, the timer will be stopped)
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//! \param Period  		  (IN):	The value of the time out for the timer in milliseconds
//! \param CallbackFunc	  (IN):	The pointer to the user procedure
//! \param UserData		  (IN):	The pointer to parameter of the user procedure
//!
//! \return Return:		S_OK
//!						E_FAIL
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Reset(MV_HANDLE_Timer_t	*pHandle,
						 UINT				Period,
						 PVOID				CallbackFunc,
						 PVOID				UserData);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Start
//!
//! Description: Start a timer
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Start(MV_HANDLE_Timer_t	*pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Stop
//!
//! Description: Stop a timer
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Stop(MV_HANDLE_Timer_t	*pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Resume
//!
//! Description: Resume a timer when timer is on paused status (not available on Win32 OS)
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT   MV_Timer_Resume(MV_HANDLE_Timer_t	*pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Pause
//!
//! Description: Pause a timer when timer is on started status (not available on Win32 OS)
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT   MV_Timer_Pause(MV_HANDLE_Timer_t	*pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Reset_Period
//!
//! Description: Reset a timer for period parameter (before reseting, the timer will be stopped)
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//! \param Period  		  (IN):	The value of the time out for the timer in milliseconds
//!
//! \return Return:		S_OK
//!						E_FAIL
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Reset_Period(MV_HANDLE_Timer_t	*pHandle,
						 UINT				Period);

/****************************** OSAL SHM*******************************************/

#if defined( __LINUX__ )

////////////////////////////////////////////////////////////////////////////////
//! \brief SHM CPU User Space Address for Linux
//!
////////////////////////////////////////////////////////////////////////////////

//#define MV_SHM_PhysAddrToVirtAddr(x)					(PVOID)(MV_SHM_Start_VirtAddr + ((UINT32)(x) - (UINT32)MV_SHM_Start_PhysAddr))
//#define MV_SHM_VirtAddrToPhysAddr(x)					(PVOID)(MV_SHM_Start_PhysAddr + ((UINT32)(x) - (UINT32)MV_SHM_Start_VirtAddr))

//extern PVOID MV_SHM_Start_VirtAddr;
//extern PVOID MV_SHM_Start_VirtAddr_CACHE;

//#define MV_SHM_NON_CACHE_ADDR(x)						((((UINT32)(x) >= (UINT32)MV_SHM_Start_VirtAddr_CACHE) && ((UINT32)(x) < (UINT32)MV_SHM_Start_VirtAddr_CACHE + MV_SHM_FullSize)) ? ((PVOID)(MV_SHM_Start_VirtAddr + ((UINT32)(x) - (UINT32)MV_SHM_Start_VirtAddr_CACHE))): NULL)
//#define MV_SHM_CACHE_ADDR(x)							((((UINT32)(x) >= (UINT32)MV_SHM_Start_VirtAddr) && ((UINT32)(x) < (UINT32)MV_SHM_Start_VirtAddr + MV_SHM_FullSize)) ? ((PVOID)((UINT32)MV_SHM_Start_VirtAddr_CACHE + ((UINT32)(x) - (UINT32)MV_SHM_Start_VirtAddr))): NULL)


#elif defined( __ECOS__ )

#include "galois_cache_flush.h"

#define MV_SHM_NON_CACHE_ADDR(x)						((((UINT32)(x) >= (UINT32)MV_SHM_Start_PhysAddr_CACHE) && ((UINT32)(x) < (UINT32)MV_SHM_Start_PhysAddr_CACHE + MV_SHM_FullSize)) ? ((PVOID)((UINT32)MV_SHM_Start_PhysAddr + ((UINT32)(x) - (UINT32)MV_SHM_Start_PhysAddr_CACHE))): NULL)
#define MV_SHM_CACHE_ADDR(x)							((((UINT32)(x) >= (UINT32)MV_SHM_Start_PhysAddr) && ((UINT32)(x) < (UINT32)MV_SHM_Start_PhysAddr + MV_SHM_FullSize)) ? ((PVOID)((UINT32)MV_SHM_Start_PhysAddr_CACHE + ((UINT32)(x) - (UINT32)MV_SHM_Start_PhysAddr))): NULL)
#define MV_CC_SHM_InvalidateDCacheRegion				InvalidateDCacheRegion
#define MV_CC_SHM_CleanDCacheRegion						CleanDCacheRegion
#define MV_CC_SHM_CleanAndInvalidateDCacheRegion		CleanAndInvalidateDCacheRegion

#endif

#define MV_CC_SHM_Flush(x)	{			\
	volatile UINT i;										\
	i = (UINT)(x);											\
}

////////////////////////////////////////////////////////////////////////////////
//! \brief SHM constant definition: MV_CC_SHM_Key_t
//!
////////////////////////////////////////////////////////////////////////////////
typedef UINT32 	MV_CC_SHM_Key_t;
typedef UINT32 	*pMV_CC_SHM_Key_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_CC_SHM_HANDLE_t | Handle to a task object
//!
////////////////////////////////////////////////////////////////////////////////
typedef MV_OSAL_HANDLE 	MV_CC_SHM_HANDLE_t;
typedef MV_OSAL_HANDLE 	*pMV_CC_SHM_HANDLE_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_Init
//!
//! Description: SHM Manager initialize the private data structure
//!
//!	Warning:	MV_CC_SHM_Init should be called in front of the all CC SHM service
//!
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_SHM_Init(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_Exit
//!
//! Description: SHM Manager destroy the private data structure
//!
//!	Warning:	MV_CC_SHM_Exit should be called after all CC SHM service destroyed
//!
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_SHM_Exit(void);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_Malloc
//!
//! Description: Allocate size bytes Shared Memory Block and returns a pointer
//!				to the allocated memory. The memory is not cleared.
//!				The key should be set for the ID of Shared Memory Block for Galois System SHM Service
//!				If you set key to MV_CC_SHM_KEY_DEFAULT, a random value set by system will be returned after SUCCESS
//!				You should keep this key to inform other on the other CPU.
//!
//! \param pKey (IN/OUT): -- the pointer to the ID of the Shared Memory Block which have been allocated by other app (key != MV_CC_SHM_KEY_DEFAULT)
//! \param Size 	(IN): -- the value to the Shared Memory Block byte size
//!
//! \return Return:		The pointer to the shared Memory Block
//!						NULL
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_CC_SHM_Malloc( pMV_CC_SHM_Key_t pKey, UINT32 Size );

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_Get
//!
//! Description: Get the pointer to Shared Memory Block by key which have been allocated from both MV_CC_SHM_Malloc() and MV_CC_SHM_Malloc_Cache()
//!
//! \param pKey 	(IN): -- the pointer to the ID of the Shared Memory Block which have been allocated by other app (key != MV_CC_SHM_KEY_DEFAULT)
//!
//! \return Return:		The pointer to the shared Memory Block which have been allocated
//!						NULL
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_CC_SHM_Get( pMV_CC_SHM_Key_t pKey );

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_Free
//!
//! Description: Free Shared Memory Block which have been allocated
//!
//! \param Ptr 		(IN): -- the pointer to the Shared Memory Block
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_SHM_Free( PVOID Ptr );

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_FreeKey
//!
//! Description: Free Shared Memory Block by key which have been allocated by key which have been allocated from both MV_CC_SHM_Malloc() and MV_CC_SHM_Malloc_Cache()
//!
//! \param pKey 	(IN): -- the pointer to the ID of the Shared Memory Block which have been allocated by other app (key != MV_CC_SHM_KEY_DEFAULT)
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_SHM_FreeKey( pMV_CC_SHM_Key_t pKey );

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_GetInfo
//!
//! Description: Get information of Shared Memory Pool
//!
//! \param pMV_CC_SHM_PoolInfo_t 	(IN): -- the pointer to shared memory pool information data structure
//!
//! \return Return:		S_OK
//!				E_INVALIDARG
//!				E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_SHM_GetInfo( pMV_CC_SHM_PoolInfo_t pInfo );

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_Malloc_Cache
//!
//! Description: Allocate size bytes Shared Memory Block and returns a pointer
//!				to the allocated memory. The memory is cache safe.
//!				The key should be set for the ID of Shared Memory Block for Galois System SHM Service
//!				If you set key to MV_CC_SHM_KEY_DEFAULT, a random value set by system will be returned after SUCCESS
//!				You should keep this key to inform other on the other CPU.
//!
//! \param pKey (IN/OUT): -- the pointer to the ID of the Shared Memory Block which have been allocated by other app (key != MV_CC_SHM_KEY_DEFAULT)
//! \param Size 	(IN): -- the value to the Shared Memory Block byte size
//!
//! \return Return:		The pointer to the shared Memory Block (Cache accessible)
//!						NULL
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_CC_SHM_Malloc_Cache( pMV_CC_SHM_Key_t pKey, UINT32 Size );

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_Get_Cache
//!
//! Description: Get the pointer (Cache accessible) to Shared Memory Block by key which have been allocated from MV_CC_SHM_Malloc_Cache
//!
//! \param pKey 	(IN): -- the pointer to the ID of the Shared Memory Block which have been allocated by other app (key != MV_CC_SHM_KEY_DEFAULT)
//!
//! \return Return:		The pointer (Cache accessible) to the shared Memory Block which have been allocated from MV_CC_SHM_Malloc_Cache
//!						NULL
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_CC_SHM_Get_Cache( pMV_CC_SHM_Key_t pKey );

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_Free_Cache
//!
//! Description: Free Shared Memory Block which have been allocated from MV_CC_SHM_Malloc_Cache
//!
//! \param Ptr 		(IN): -- the pointer (Cache accessible) to the Shared Memory Block
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_SHM_Free_Cache( PVOID Ptr );

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_FreeKey_Cache
//!
//! Description: Free Shared Memory Block by key which have been allocated by key which have been allocated from both MV_CC_SHM_Malloc() and MV_CC_SHM_Malloc_Cache()
//!
//! \param pKey 	(IN): -- the pointer to the ID of the Shared Memory Block which have been allocated by other app (key != MV_CC_SHM_KEY_DEFAULT)
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_SHM_FreeKey_Cache( pMV_CC_SHM_Key_t pKey );

#if defined( __LINUX__ )

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_InvalidateDCacheRegion
//!
//! Description: Invalidate DCache Region data. The operation need to make sure
//!              that the adress and size and cache line based. Use this one with
//!              cautions.(Typically, it is used on DMA write, CPU read only case)
//!				 All cache lines which are in address rage will be operated. So the addrres
//!				 and size are not requested to be aligned in word or cache line.
//! Allowed by : Cache Memory Address On Galois_Linux
//!
//! \param adr 	(IN): -- contain base adress
//! \param size	(IN): -- contains size of Region in bytes
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_SHM_InvalidateDCacheRegion(void *adr, int size);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_CleanDCacheRegion
//!
//! Description: Clean DCache Region data
//!				 All cache lines which are in address rage will be operated. So the addrres
//!				 and size are not requested to be aligned in word or cache line.
//! Allowed by : Cache Memory Address On Galois_Linux
//!
//! \param adr 	(IN): -- contain base adress
//! \param size	(IN): -- contains size of Region in bytes
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_SHM_CleanDCacheRegion(void *adr, int size);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_CleanAndInvalidateDCacheRegion
//!
//! Description: Clean  and invalidate DCache Region data
//!				 All cache lines which are in address rage will be operated. So the addrres
//!				 and size are not requested to be aligned in word or cache line.
//! Allowed by : Cache Memory Address On Galois_Linux
//!
//! \param adr 	(IN): -- contain base adress
//! \param size	(IN): -- contains size of Region in bytes
//!
//! \return Return:		S_OK
//!						E_FAIL
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_SHM_CleanAndInvalidateDCacheRegion(void *adr, int size);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_GetStartAddress
//!
//! Description: Get start address of shared memory
//!     Warning: It is only available in Linux OS.
//!
//!
//! \return Return:		NULL     |      system not ready
//!                     Not NULL |      system is ok
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_CC_SHM_GetStartAddress();

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_GetStartAddress
//!
//! Description: Get file id of shared memory device
//!     Warning: It is only available in Linux OS.
//!
//!
//! \return Return:		-1     |      system not ready
//!                     Not -1 |      ok
//!
////////////////////////////////////////////////////////////////////////////////
INT MV_CC_SHM_GetDeviceFileID();

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_GetStartAddress_CACHE
//!
//! Description: Get start address of shared memory with CPU cache
//!     Warning: It is only available in Linux OS.
//!
//!
//! \return Return:		NULL     |      system not ready
//!                     Not NULL |      system is ok
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_CC_SHM_GetStartAddress_CACHE();

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_SHM_GetDeviceFileID_CACHE
//!
//! Description: Get file id of shared memory device with CPU cache
//!     Warning: It is only available in Linux OS.
//!
//!
//! \return Return:		-1     |      system not ready
//!                     Not -1 |      ok
//!
////////////////////////////////////////////////////////////////////////////////
INT MV_CC_SHM_GetDeviceFileID_CACHE();

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_SHM_NON_CACHE_ADDR
//!
//! Description: convert cache address to no cache address
//!     Warning: It is only available in Linux OS.
//!
//!
//! \return Return:		NULL     |      system not ready or input is error
//!                     Not NULL |      ok
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_SHM_NON_CACHE_ADDR(PVOID addr_cache);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_SHM_CACHE_ADDR
//!
//! Description: convert no cache address to cache address
//!     Warning: It is only available in Linux OS.
//!
//!
//! \return Return:		NULL     |      system not ready or input is error
//!                     Not NULL |      ok
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_SHM_CACHE_ADDR(PVOID addr_nocache);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_SHM_PhysAddrToVirtAddr
//!
//! Description: convert physical address to virtual address
//!     Warning: It is only available in Linux OS.
//!
//!
//! \return Return:		NULL     |      system not ready or input is error
//!                     Not NULL |      ok
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_SHM_PhysAddrToVirtAddr(PVOID addr_phys);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_SHM_VirtAddrToPhysAddr
//!
//! Description: convert virtual address to physical address
//!     Warning: It is only available in Linux OS.
//!
//!
//! \return Return:		NULL     |      system not ready or input is error
//!                     Not NULL |      ok
//!
////////////////////////////////////////////////////////////////////////////////
PVOID MV_SHM_VirtAddrToPhysAddr(PVOID addr_virt);

#endif


/****************************** OSAL MSGQ*******************************************/

////////////////////////////////////////////////////////////////////////////////
//! \brief CC Data Structure: MV_CC_HANDLE_CBufCtrl_t | Handle to CBuf Control Object
//!
////////////////////////////////////////////////////////////////////////////////
typedef MV_CC_HANDLE 	MV_CC_HANDLE_MsgQ_t;
typedef MV_CC_HANDLE 	*pMV_CC_HANDLE_MsgQ_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_Create
//!
//! Description: Create a MsgQ Object [ICC or ITC]
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the MsgQ Object Handle
//! \param MsgQType		(IN): -- the type of Control Object type
//! \param ServiceID	(IN/OUT): -- MsgQ Service ID (Zero when MsgQType == ITCtype)
//! \param NMsgsMax	 	(IN): -- the max number of messages in the message queue
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQ_Create(MV_CC_HANDLE_MsgQ_t 		*pHandle,
						   MV_CC_MsgQType_t			MsgQType,
						   MV_CC_ServiceID_U32_t 	ServiceID,
						   UINT32 		 			NMsgsMax);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_Destroy
//!
//! Description: Destroy a MsgQ Object [ICC or ITC]
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the MsgQ Object Handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQ_Destroy(MV_CC_HANDLE_MsgQ_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_GetType
//!
//! Description: Get a MsgQ Object Type
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 		(IN): -- the MsgQ Object Handle
//!
//! \return Return:		MsgQ Object Type
//!
////////////////////////////////////////////////////////////////////////////////
MV_CC_MsgQType_t  MV_CC_MsgQ_GetType(MV_CC_HANDLE_MsgQ_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_PostMsg
//!
//! Description: Post the Message into Message Queue
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 		(IN): -- the MsgQ Object Handle
//!
//! \return Return:		S_OK
//! 					E_FAIL
//! 					E_FULL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQ_PostMsg(MV_CC_HANDLE_MsgQ_t 	Handle,
							PVOID 					pMSG);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_PostMsgByID
//!
//! Description: Post the Message into Message Queue which is assign to the Service ID
//! Allowed by : Inter-CPU Message Queue
//!
//! \param ServiceID	(IN): -- the MsgQ Service ID
//!
//! \return Return:		S_OK
//! 					E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQ_PostMsgByID(MV_CC_ServiceID_U32_t 	ServiceID,
								PVOID 					pMSG);

HRESULT MV_CC_MsgQ_PostMsgByID_Open(int *socket_fd);


HRESULT MV_CC_MsgQ_PostMsgByID_Close(int socket_fd);


HRESULT MV_CC_MsgQ_PostMsgByID_Socket(int socket_fd,
			MV_CC_ServiceID_U32_t ServiceID, PVOID pMSG);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_GetMsg
//!
//! Description: Get the Message From Message Queue. If there is no message, wait until new message arrives.
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 	(IN): -- the MsgQ Object Handle
//! \param pMSG 	(OUT): -- the pointer to the Message Data Which Structure is MV_CC_MSG_t
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQ_GetMsg(MV_CC_HANDLE_MsgQ_t 	Handle,
						   PVOID 				pMSG);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_TimedGetMsg
//!
//! Description: Get the Message From Message Queue. If there is no message, wait until new message arrives or timeout.
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 	(IN): -- the MsgQ Object Handle
//! \param pMSG 	(OUT): -- the pointer to the Message Data Which Structure is MV_CC_MSG_t
//! \param TimeOutMS(IN): -- the millisecond of timeout to wait for getting message
//!
//! \return Return:		S_OK
//! 					E_TIMEOUT
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQ_TimedGetMsg(MV_CC_HANDLE_MsgQ_t 	Handle,
								PVOID 					pMSG,
								UINT32 					TimeOutMS);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_MsgQ_TryGetMsg
//!
//! Description: Get the Message From Message Queue. If there is no message, return right now.
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 	(IN): -- the MsgQ Object Handle
//! \param pMSG 	(OUT): -- the pointer to the Message Data Which Structure is MV_CC_MSG_t
//!
//! \return Return:		S_OK
//! 					E_NOTREADY
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_MsgQ_TryGetMsg(MV_CC_HANDLE_MsgQ_t 	Handle,
							 PVOID 					pMSG);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_GetServiceID
//!
//! Description: Get the Service ID of a MsgQ Object
//! Allowed by : Inter-CPU MsgQ
//!
//! \param Handle 		(IN): -- the MsgQ Object Handle
//!
//! \return Return:		Service ID
//! 					Zero when Inter-Task MsgQ
////////////////////////////////////////////////////////////////////////////////
MV_CC_ServiceID_U32_t  MV_CC_MsgQ_GetServiceID(MV_CC_HANDLE_MsgQ_t 	Handle);

/****************************** OSAL MSGQ Ex*******************************************/

////////////////////////////////////////////////////////////////////////////////
//! \brief CC Data Structure: MV_CC_HANDLE_CBufCtrl_t | Handle to CBuf Control Object
//!
////////////////////////////////////////////////////////////////////////////////
typedef MV_CC_HANDLE 	MV_CC_HANDLE_MsgQEx_t;
typedef MV_CC_HANDLE 	*pMV_CC_HANDLE_MsgQEx_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief CC Message Data Structure: MV_CC_MSG_t 	| Object data structure
//!									 pMV_CC_MSG_t 	| Pointer to the Object data structure
////////////////////////////////////////////////////////////////////////////////
typedef struct {
	//! Message ID
	UINT8 m_MsgLen;
	UINT8 m_MsgRsv1;
	UINT8 m_MsgRsv2;
	UINT8 m_MsgRsv3;

	UINT8 m_Data[MV_CC_MSGQ_DATA_MSGEXLEN_MAX];		// The size of the Data could be reduced by user's requirement

} MV_CC_MSGEx_t, *pMV_CC_MSGEx_t;
//!				Warning!!! 	the length of messages includes the m_MsgLen in MV_CC_MSGEx_t
//!							So the length of data in MV_CC_MSGEx_t or structure user difined should be less than MV_CC_MSGQ_DATA_MSGEXLEN_MAX


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQEx_Create
//!
//! Description: Create a MsgQ Object [ICC or ITC] for extension length of message (the length is less than MV_CC_MSGQ_DATA_MSGEXLEN_MAX)
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the MsgQ Object Handle
//! \param MsgQType		(IN): -- the type of Control Object type
//! \param ServiceID	(IN/OUT): -- MsgQ Service ID (Zero when MsgQType == ITCtype)
//! \param NMsgsMax	 	(IN): -- the max number of messages in the message queue
//! \param MsgLenMax	(IN): -- the max length of messages in the Message Queue ( <= MV_CC_MSGQ_DATA_MSGEXLEN_MAX )
//!				Warning!!! 	the length of messages includes the m_MsgLen in MV_CC_MSGEx_t, so the size of MsgQ is MsgLenMax + 1
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQEx_Create(MV_CC_HANDLE_MsgQEx_t 		*pHandle,
						     MV_CC_MsgQType_t			MsgQType,
						     MV_CC_ServiceID_U32_t 		ServiceID,
						     UINT32 		 			NMsgsMax,
						     UINT32						MsgLenMax);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQEx_Destroy
//!
//! Description: Destroy a MsgQ Object [ICC or ITC] for extension length of message
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the MsgQ Object Handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQEx_Destroy(MV_CC_HANDLE_MsgQEx_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQEx_GetType
//!
//! Description: Get a MsgQ Object Type for extension length of message
//! Allowed by : Inter-CPU or Inter-Task Message Queue Extension Mode
//!
//! \param Handle 		(IN): -- the MsgQ Object Handle
//!
//! \return Return:		MsgQ Object Type
//!
////////////////////////////////////////////////////////////////////////////////
MV_CC_MsgQType_t  MV_CC_MsgQEx_GetType(MV_CC_HANDLE_MsgQEx_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQEx_PostMsg
//!
//! Description: Post the Message into Message Queue for extension length of message
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 		(IN): -- the MsgQ Object Handle
//! \param Length		(IN): -- the length of the message
//! \param pMSG			(IN): -- the pointer to the message (MV_CC_MSGEx_t)
//!
//! \return Return:		S_OK
//! 					E_FAIL
//!						E_FULL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQEx_PostMsg(MV_CC_HANDLE_MsgQEx_t 	Handle,
							  PVOID 				pMSG);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQ_PostMsgByID
//!
//! Description: Post the Message into Message Queue which is assign to the Service ID for extension length of message
//! Allowed by : Inter-CPU Message Queue
//!
//! \param ServiceID	(IN): -- the MsgQ Service ID
//! \param pMSG			(IN): -- the pointer to the message
//!
//! \return Return:		S_OK
//! 					E_FAIL
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQEx_PostMsgByID(MV_CC_ServiceID_U32_t 	ServiceID,
								  PVOID 					pMSG);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQEx_GetMsg
//!
//! Description: Get the Message From Message Queue. If there is no message, wait until new message arrives. ( for extension length of message )
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 	(IN): -- the MsgQ Object Handle
//! \param pMSG 	(OUT): -- the pointer to the Message Data (MV_CC_MSGEx_t)
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQEx_GetMsg(MV_CC_HANDLE_MsgQEx_t 	Handle,
						     PVOID 					pMSG);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQEx_TimedGetMsg
//!
//! Description: Get the Message From Message Queue. If there is no message, wait until new message arrives or timeout. ( for extension length of message )
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 	(IN): -- the MsgQ Object Handle
//! \param pMSG 	(OUT): -- the pointer to the Message Data (MV_CC_MSGEx_t)
//! \param TimeOutMS(IN): -- the millisecond of timeout to wait for getting message
//!
//! \return Return:		S_OK
//! 					E_TIMEOUT
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_MsgQEx_TimedGetMsg(MV_CC_HANDLE_MsgQEx_t 	Handle,
								  PVOID 				pMSG,
								  UINT32 				TimeOutMS);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_MsgQEx_TryGetMsg
//!
//! Description: Get the Message From Message Queue. If there is no message, return right now.
//! Allowed by : Inter-CPU or Inter-Task Message Queue
//!
//! \param Handle 	(IN): -- the MsgQ Object Handle
//! \param pMSG 	(OUT): -- the pointer to the Message Data (MV_CC_MSGEx_t)
//!
//! \return Return:		S_OK
//! 					E_NOTREADY
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_MsgQEx_TryGetMsg(MV_CC_HANDLE_MsgQEx_t 	Handle,
							   PVOID				pMSG);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_MsgQEx_GetServiceID
//!
//! Description: Get the Service ID of a MsgQEx Object
//! Allowed by : Inter-CPU MsgQ
//!
//! \param Handle 		(IN): -- the MsgQEx Object Handle
//!
//! \return Return:		Service ID
//! 					Zero when Inter-Task MsgQ
////////////////////////////////////////////////////////////////////////////////
MV_CC_ServiceID_U32_t  MV_CC_MsgQEx_GetServiceID(MV_CC_HANDLE_MsgQEx_t 	Handle);


/****************************** OSAL RPC*******************************************/


////////////////////////////////////////////////////////////////////////////////
//! \brief CC Data Structure:
//!
////////////////////////////////////////////////////////////////////////////////
typedef MV_CC_HANDLE 	MV_CC_HANDLE_RPCClnt_t;
typedef MV_CC_HANDLE 	*pMV_CC_HANDLE_RPCClnt_t;;

typedef MV_CC_HANDLE 	MV_CC_HANDLE_RPCSrv_t;
typedef MV_CC_HANDLE 	*pMV_CC_HANDLE_RPCSrv_t;

////////////////////////////////////////////////////////////////////////////////
//!
//!    Interface  Routines of RPC Service
//!
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPCClnt_Create
//!
//! Description: Create a RPC client for specific RPC CC-Service
//!					Open an UDP port and bind the RPC service on it
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the RPC CC-Service client Object Handle
//! \param ServiceID	(IN): -- the ID of the RPC Global Service
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!	\see MV_CC_UDPPort_Open();
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_RPCClnt_Create(pMV_CC_HANDLE_RPCClnt_t 	pHandle,
						     MV_CC_ServiceID_U32_t 	ServiceID);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPCClnt_Call
//!
//! Description: RPC client calls the specified function on the specified remote host.
//!
//!	Warning:	The size of the parameter buffer must be less than MV_CC_RPC_PARAM_BYTESIZE Bytes Bytes.
//!				The size of the parameter buffer must be large enough to receive the RPC return parameter data.
//!
//! \param Handle 		(IN): -- the RPC CC-Service client Object Handle
//! \param RPC_Cmd	 	(IN): -- the command id of RPC function defined by developer
//! \param pParamBuf(IN/OUT): -- the parameter buffer pointer of RPC function defined by developer
//! \param pParamSize(IN/OUT): -- the Byte size pointer of the parameter buffer (Size <= MV_CC_RPC_PARAM_BYTESIZE Bytes)
//! \param RPC_Rtn	   (OUT): -- the return status of RPC function defined by RPC module
//! \param TimeOut 		(IN): -- the nano seconds for time out (if equal 0, no time limit for waiting response)
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_RPCClnt_Call(MV_CC_HANDLE_RPCClnt_t	Handle,
						   MV_CC_RPC_CmdID_U16_t 	RPC_Cmd,
						   UCHAR 					*pParamBuf,
						   UINT8 					*pParamSize,
						   MV_CC_RPC_RtnSt_U16_t	*RPC_Rtn,
						   UINT64					TimeOut);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPCClnt_Destroy
//!
//! Description: Destroy a RPC client Object of self
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the RPC CC-Service client Object Handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!	\see MV_CC_UDPPort_Close();
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_RPCClnt_Destroy(pMV_CC_HANDLE_RPCClnt_t 	pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPC_Call
//!
//! Description: (Simplified Mode) RPC client calls the specified function on the specified remote host.
//!
//!	Warning:	if the size of the parameter buffer larger than MV_CC_RPC_PARAM_BYTESIZE, the shared memory will be used.
//!				The size of the parameter buffer must be large enough to receive the RPC return parameter data.
//!				It is same as MV_CC_RPCClnt_Call().
//!
//! \param ServiceID	(IN): -- the ID of the RPC Global Service
//! \param RPC_Cmd	 	(IN): -- the command id of RPC function defined by developer
//! \param pParamBuf(IN/OUT): -- the parameter buffer pointer of RPC function defined by developer
//! \param pParamSize(IN/OUT): -- the Byte size pointer of the parameter buffer (Size <= MV_CC_RPC_PARAM_BYTESIZE Bytes)
//! \param RPC_Rtn	   (OUT): -- the return status of RPC function defined by RPC module
//! \param TimeOut 		(IN): -- the nano seconds for time out (if equal 0, no time limit for waiting response)
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_RPC_Call(	MV_CC_ServiceID_U32_t 	ServiceID,
						   	MV_CC_RPC_CmdID_U16_t 		RPC_Cmd,
						   	UCHAR 						*pParamBuf,
						   	UINT8 						*pParamSize,
						   	MV_CC_RPC_RtnSt_U16_t		*RPC_Rtn,
						   	UINT64						TimeOut);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPCSrv_GetServiceID
//!
//! Description: Get Service ID of RPC Server
//!
//!
//! \param Handle 		(IN/OUT): -- the RPC CC-Service Server Object Handle
//!
//! \return Return:		ServiceID == MV_CC_ServiceID_None, Handle is NULL
//!						ServiceID == others, 	OK
//!
////////////////////////////////////////////////////////////////////////////////
MV_CC_ServiceID_U32_t  MV_CC_RPCSrv_GetServiceID(MV_CC_HANDLE_RPCSrv_t 		Handle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPCSrv_Create
//!
//! Description: Create a RPC server for specific RPC CC-Service
//!					Open Server UDP port and bind the RPC service on it
//!					Start the RPC server task
//!
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the RPC CC-Service Server Object Handle
//! \param ServiceID	(IN): -- the ID of the RPC Global Service
//! \param pRegCmdList	(IN): -- the pointer to the RPC Command List of this Service ID
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!	\see MV_CC_UDPPort_Open();
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_RPCSrv_Create(pMV_CC_HANDLE_RPCSrv_t 		pHandle,
							MV_CC_ServiceID_U32_t 		ServiceID,
							pMV_CC_RPC_RegCmdItem_t 		pRegCmdList);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPCSrv_Destroy
//!
//! Description: Destroy a RPC server for specific RPC CC-Service
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the RPC CC-Service Server Object Handle
//!
//! \return Return:		S_OK
//!						Other Error Code see MV_CC_UDPPort_Close()
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_RPCSrv_Destroy(pMV_CC_HANDLE_RPCSrv_t 	pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_NET_RPCSrv_Create
//!
//! Description: Create a RPC server for specific RPC NET-Service
//!					Open Server NET port and bind the RPC service on it
//!					Start the RPC server task
//!
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the RPC NET-Service Server Object Handle
//! \param ServiceID	(IN): -- the ID of the RPC Global Service
//! \param pRegCmdList	(IN): -- the pointer to the RPC Command List of this Service ID
//!
//! \return Return:		S_OK
//!						E_FAIL
//!						E_INVALIDARG
//!	\see MV_NET_UDPPort_Open();
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_NET_RPCSrv_Create(pMV_CC_HANDLE_RPCSrv_t 		pHandle,
							MV_CC_ServiceID_U32_t 		ServiceID,
							pMV_CC_RPC_RegCmdItem_t 		pRegCmdList);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_NET_RPCSrv_Destroy
//!
//! Description: Destroy a RPC server for specific RPC NET-Service
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the RPC NET-Service Server Object Handle
//!
//! \return Return:		S_OK
//!						Other Error Code see MV_NET_UDPPort_Close()
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_NET_RPCSrv_Destroy(pMV_CC_HANDLE_RPCSrv_t 	pHandle);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPCParam_Pack
//!
//! Description: Pack the all parameters of RPC function to the parameter buffer
//!
//! Warning:	The total byte size of all parameters in buffer should be less than MV_CC_RPC_PARAM_BYTESIZE!
//!
//! \param pParamBuf(IN): -- the pointer of the parameter buffer (Size >= MV_CC_RPC_PARAM_BYTESIZE)
//! \param num 		(IN): -- the number of all parameters - N
//! \param 1st		(IN): -- the pointer of 1st parameter
//! \param 2nd		(IN): -- the byte size of 1st parameter
//! \param 3rd		(IN): -- the pointer of 2nd parameter
//! \param 4th		(IN): -- the byte size of 2nd parameter
//! \param (N-1)th	(IN): -- the pointer of (N/2)th parameter
//! \param Nth		(IN): -- the byte size of (N/2)th parameter
//!
//! \return Return:	the total byte size of the buffer parameter packed into
//!
////////////////////////////////////////////////////////////////////////////////
UINT8 MV_CC_RPCParam_Pack(UCHAR *pParamBuf, INT num, ...);


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_RPCParam_Unpack
//!
//! Description: Unpack the all parameters of RPC function from a parameter buffer
//!
//! Warning:	The total byte size of all parameters in buffer should be less than MV_CC_RPC_PARAM_BYTESIZE!
//!
//! \param pParamBuf(IN): -- the pointer of the parameter buffer (Size >= MV_CC_RPC_PARAM_BYTESIZE)
//! \param num 		(IN): -- the number of all parameters - N
//! \param 1st		(OUT): -- the pointer of 1st parameter
//! \param 2nd		(IN): -- the byte size of 1st parameter
//! \param 3rd		(OUT): -- the pointer of 2nd parameter
//! \param 4th		(IN): -- the byte size of 2nd parameter
//! \param (N-1)th	(OUT): -- the pointer of (N/2)th parameter
//! \param Nth		(IN): -- the byte size of (N/2)th parameter
//!
//!
//! \return Return:	the total byte size of the buffer parameter packed into
//!
////////////////////////////////////////////////////////////////////////////////
UINT8 MV_CC_RPCParam_Unpack(UCHAR *pParamBuf, INT num, ...);


/****************************** OSAL CBUF*******************************************/


////////////////////////////////////////////////////////////////////////////////
//! \brief CC Data Structure: MV_CC_HANDLE_CBufCtrl_t | Handle to CBuf Control Object
//!
////////////////////////////////////////////////////////////////////////////////
typedef MV_CC_HANDLE 	MV_CC_HANDLE_CBufCtrl_t;
typedef MV_CC_HANDLE 	*pMV_CC_HANDLE_CBufCtrl_t;

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_Create
//!
//! Description: Create a CBuf Control Object [Producer(client) & Consumer(Server)]
//! Allowed by : Consumer & Producer
//!	Work Flow  :
//!               ----------------------------------------------------------------
//!                  Service Run Mode    |   Object
//!               ----------------------------------------------------------------
//!                CBuf Body Created by  |   Dynamic Service Server
//!               ----------------------------------------------------------------
//!               CBuf Body Destroyed by |   Dynamic Service Server
//!               ----------------------------------------------------------------
//!                    First Start       |   Produer or Consumer
//!               ----------------------------------------------------------------
//!                     Last Quit        |   Produer or Consumer
//!               ----------------------------------------------------------------
//!
//!				It is a RPC to Create a CBuf Control Object in local, and create a CBuf Body in remote server.
//!				If the CBuf service is a new one, the DSS will created a new CBuf Body and return a handle to you.
//!				If the CBuf service is a old one, which has been created by another client, the DSS will return the handle to you and do nothing.
//!				The CBuf service is created on Service ID.
//!
//!               ----------------------------------------------------------------
//!                  Create a new or old Static Service ID   -  StaticStart < Service ID < DynamicStart
//!               ----------------------------------------------------------------
//!                  Create a new Dynamic Service ID  -   Service ID == DynamicStart
//!               ----------------------------------------------------------------
//!                  Create a old Dynamic Service ID  -   Service ID > DynamicStart (Which is passed by creator)
//!               ----------------------------------------------------------------
//!
//!				The Flags is used for the property of CBuf.
//!
//!               ----------------------------------------------------------------
//!                  Flags			           |   meaning
//!               ----------------------------------------------------------------
//!                MV_CC_CBUF_FLAGS_ALIGNMENT  |   memory boundary block should be added to the Circular Buffer Length.
//!               ----------------------------------------------------------------
//!                MV_CC_CBUF_FLAGS_NOTICESTAT |   Allow Procuder to notice Consumer to read data immediatly
//!               ----------------------------------------------------------------
//!                MV_CC_CBUF_FLAGS_LOCALCPU   |   the cbuf used on local cpu, so the buffer is in cache memory for faster access
//!               ----------------------------------------------------------------
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the CBuf Control Object Handle
//! \param pServiceID	(IN/OUT): -- the pointer to the CBuf Service ID
//! \param CBufCtrlType	(IN): -- the type of Control Object type
//! \param BufSize	 	(IN): -- the size of CBuf Body data buffer
//! \param EndBlockSize	(IN): -- the size of CBuf Body End Block !! Limitation: EndBlockSize <= (BufSize / 2 ) - 1 , The min linear space byte size for reading or writing at the end of the buffer , and which is the size of addition block after buffer end for circular operation
//! \param Flags		(IN): -- the flags of CBuf Body initialization
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_Create(MV_CC_HANDLE_CBufCtrl_t 	*pHandle,
						   MV_CC_ServiceID_U32_t 	*pServiceID,
						   MV_CC_CBufCtrlType_t		CBufCtrlType,
						   UINT32					BufSize,
						   UINT32					EndBlockSize,
						   UINT32					Flags);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_Destroy
//!
//! Description: Destroy a CBuf Control Object [Producer(client) & Consumer(Server)] for CBuf Body Object
//! Allowed by : Consumer & Producer
//!	Work Flow  :
//!				It is a RPC to destroy a CBuf Control Object in local, and destroy the CBuf Body in remote server if there is no client to connect to it.
//!
//! \param pHandle 		(IN/OUT): -- the pointer to the CBuf Control Object Handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_Destroy(MV_CC_HANDLE_CBufCtrl_t 	*pHandle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetCtrlType
//!
//! Description: Get a CBuf Control Object Type
//! Allowed by : Consumer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		CBuf Control Object Type
//!
////////////////////////////////////////////////////////////////////////////////
MV_CC_CBufCtrlType_t  MV_CC_CBuf_GetCtrlType(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetServiceID
//!
//! Description: Get the Service ID of a CBuf Control Object
//! Allowed by : Consumer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		Service ID
//!
////////////////////////////////////////////////////////////////////////////////
MV_CC_ServiceID_U32_t  MV_CC_CBuf_GetServiceID(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetBufSize
//!
//! Description: Query about the buffer size of the CBuf Body Object
//! Allowed by : Consumer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		>=0 : the byte size of CBuf
//!						E_INVALIDARG
//!						E_BADVALUE
//!
////////////////////////////////////////////////////////////////////////////////
UINT32  MV_CC_CBuf_GetBufSize(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetEndBlockSize
//!
//! Description: Query about the End Block size of the CBuf Body Object
//! Allowed by : Consumer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		>=0 : the byte size of CBuf
//!						E_INVALIDARG
//!						E_BADVALUE
//!
////////////////////////////////////////////////////////////////////////////////
UINT32  MV_CC_CBuf_GetEndBlockSize(MV_CC_HANDLE_CBufCtrl_t 	Handle);

#define MV_CC_CBuf_GetMaxBlockSize	MV_CC_CBuf_GetEndBlockSize

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetBaseAddr
//!
//! Description: Query about the CBuf Body address of the CBuf Control Object
//! Allowed by : Consumer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		the CBuf Body address of the CBuf Body Object buffer
//!
////////////////////////////////////////////////////////////////////////////////
UINT32  MV_CC_CBuf_GetBodyAddr(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetBaseAddr
//!
//! Description: Query about the base address of the CBuf Body Object buffer
//! Allowed by : Consumer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		the base address of the CBuf Body Object buffer
//!
////////////////////////////////////////////////////////////////////////////////
UINT32  MV_CC_CBuf_GetBaseAddr(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetCBufRdPtr
//!
//! Description: Query about the pointer to read pointer of the CBuf Body Object Buffer
//! Allowed by : Consumer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		the pointer to read pointer of the CBuf Body Object Buffer
//!
////////////////////////////////////////////////////////////////////////////////
UINT32 *MV_CC_CBuf_GetCBufRdPtr(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetCBufWrPtr
//!
//! Description: Query about the pointer to write pointer of the CBuf Body Object Buffer
//! Allowed by : Producer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		the pointer to write pointer of the CBuf Body Object Buffer
//!
////////////////////////////////////////////////////////////////////////////////
UINT32 *MV_CC_CBuf_GetCBufWrPtr(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetRdOffset
//!
//! Description: get the reading offset value of the CBuf Body Object Buffer
//! Allowed by : Producer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		the value of the reading offset
//!
////////////////////////////////////////////////////////////////////////////////
UINT32 MV_CC_CBuf_GetRdOffset(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetWrOffset
//!
//! Description: get the writing offset value of the CBuf Body Object Buffer
//! Allowed by : Producer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		the value of the writing offset
//!
////////////////////////////////////////////////////////////////////////////////
UINT32 MV_CC_CBuf_GetWrOffset(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_SetRdOffset
//!
//! Description: get the reading offset value of the CBuf Body Object Buffer
//! Allowed by : Producer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//! \param Value 		(IN): -- the value of the reading offset
//!
//! \return Return:		S_OK			|
//!						E_INVALIDARG	|  The parameter is invalid

//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_CBuf_SetRdOffset(MV_CC_HANDLE_CBufCtrl_t 	Handle,
							   UINT32					Value);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_SetWrOffset
//!
//! Description: get the writing offset value of the CBuf Body Object Buffer
//! Allowed by : Producer & Producer
//!
//! \param Handle 		(IN): -- the CBuf Control Object Handle
//! \param Value 		(IN): -- the value of the writing offset
//!
//! \return Return:		S_OK			|
//!						E_INVALIDARG	|  The parameter is invalid
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_CBuf_SetWrOffset(MV_CC_HANDLE_CBufCtrl_t 	Handle,
							   UINT32					Value);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_WriteTry
//!
//! Description: Request to get writing pointer by Writing Block Size at a time, and return the real block size when success
//! Allowed by : Producer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param pWrBlockSize (IN/OUT): -- the pointer to Writing Block Size at a time (Maybe not enough)
//! \param pWrPtr	 	(IN/OUT): -- the pointer to CBuf Writing address pointer
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data which could been written;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_OUTOFRANGE	|  WrBlockSize exceeds the Max block size, *pWrBlockSize has return the Max block size
//!						E_ACCESSDENIED	|  Only Producer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_WriteTry(MV_CC_HANDLE_CBufCtrl_t 	Handle,
						 	UINT32		 				*pWrBlockSize,
						 	PVOID			 			*pWrPtr);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_WriteFinish
//!
//! Description: Request to finish writing, and set the block size to CBuf when success
//! Allowed by : Producer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param pWrBlockSize (IN/OUT): -- the pointer to Writing Block Size at last time (Maybe not same as setting)
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data which had been written;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_OUTOFRANGE	|  WrBlockSize exceeds the Max block size, *pWrBlockSize has return the Max block size
//!						E_ACCESSDENIED	|  Only Producer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_WriteFinish(MV_CC_HANDLE_CBufCtrl_t 	Handle,
								UINT32		 				*pWrBlockSize);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_Write
//!
//! Description: Write 1 block data to the CBuf; When no enough space, the part of data will be written.
//! Allowed by : Producer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param pWrBlockSize (IN/OUT): -- the pointer to Writing Block Size at a time (Maybe not enough)
//! \param pSrcBuf	 		(IN): -- the pointer to data which wanted to be written into the CBuf
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data which have been written;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_OUTOFRANGE	|  WrBlockSize exceeds the Max block size, *pWrBlockSize has return the Max block size
//!						E_ACCESSDENIED	|  Only Producer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_Write(MV_CC_HANDLE_CBufCtrl_t 	Handle,
					  	  UINT32		 			*pWrBlockSize,
					  	  UCHAR 					*pSrcBuf);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_PostNotice
//!
//! Description: Post a notice to consumer to start receiving
//! Allowed by : Producer When Flags = MV_CC_CBUF_FLAGS_NOTICESTAT_Yes
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data which have been written;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_ACCESSDENIED	|  Only Producer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_PostNotice(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetSpaceSize
//!
//! Description: Query about the space size of the CBuf
//! Allowed by : Consumer & Producer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		>=0 : the space byte size of CBuf
//!
////////////////////////////////////////////////////////////////////////////////
INT32  MV_CC_CBuf_GetSpaceSize(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetSpaceSize
//!
//! Description: Query about the space size and the linear space size of the CBuf
//! Allowed by : Consumer & Producer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param pLinearSize	(IN/OUT): -- the pointer to the linear size
//!
//! \return Return:		>=0 : the space byte size of CBuf
//!
////////////////////////////////////////////////////////////////////////////////
INT32  MV_CC_CBuf_GetSpaceSizeAndNoRoll(MV_CC_HANDLE_CBufCtrl_t 	Handle,
									    INT32 						*pLinearSize);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_WaitNotice
//!
//! Description: waiting until noticed by producer
//! Allowed by : Consumer When Flags = MV_CC_CBUF_FLAGS_NOTICESTAT_Yes
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data  which would been read;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_ACCESSDENIED	|  Only Consumer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_WaitNotice(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_ReadTry_TimedWaitNotice
//!
//! Description: waiting until noticed by producer or time out
//! Allowed by : Consumer When Flags = MV_CC_CBUF_FLAGS_NOTICESTAT_Yes
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param TimeOutMS 		(IN): -- the millisecond for time out (greater than 0)
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data  which would been read;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_ACCESSDENIED	|  Only Consumer Control Object could call this function
//!						E_TIMEOUT
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_TimedWaitNotice(MV_CC_HANDLE_CBufCtrl_t 	Handle,
									UINT32						TimeOutMS);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_ReadTry
//!
//! Description: Request to get reading pointer by reading Block Size at a time, and return the real block size when success
//! Allowed by : Consumer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param pRdBlockSize (IN/OUT): -- the pointer to reading Block Size at a time (Maybe not enough)
//! \param pRdPtr	 	(IN/OUT): -- the pointer to CBuf reading address pointer
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data  which would been read;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_OUTOFRANGE	|  RdBlockSize exceeds the Max block size, *pRdBlockSize has return the Max block size
//!						E_ACCESSDENIED	|  Only Consumer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_ReadTry(MV_CC_HANDLE_CBufCtrl_t 	Handle,
							UINT32		 				*pRdBlockSize,
							PVOID			 			*pRdPtr);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_ReadFinish
//!
//! Description: Request to finish reading, and set the block size to CBuf when success
//! Allowed by : Consumer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param pRdBlockSize (IN/OUT): -- the pointer to reading Block Size at last time (Maybe not same as setting)
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data  which had been read;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_OUTOFRANGE	|  RdBlockSize exceeds the Max block size, *pRdBlockSize has return the Max block size
//!						E_ACCESSDENIED	|  Only Consumer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_ReadFinish(MV_CC_HANDLE_CBufCtrl_t 	Handle,
						   	   UINT32		 			*pRdBlockSize);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_Read
//!
//! Description: read 1 block data from the CBuf; When no enough data, the part of data will be read.
//! Allowed by : Consumer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param pRdBlockSize (IN/OUT): -- the pointer to reading Block Size at a time (Maybe not enough)
//! \param pDstBuf	 		(IN): -- the pointer to buffer which wanted to write the data read from the CBuf to
//!
//! \return Return:		S_OK			|  if *pWrBlockSize changed, return the size of the part data  which have been read;
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_OUTOFRANGE	|  RdBlockSize exceeds the Max block size, *pRdBlockSize has return the Max block size
//!						E_ACCESSDENIED	|  Only Consumer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_CC_CBuf_Read(MV_CC_HANDLE_CBufCtrl_t 	Handle,
					 	 UINT32		 				*pRdBlockSize,
					 	 UCHAR 						*pDstBuf);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetDataSize
//!
//! Description: Query about the data size of the CBuf
//! Allowed by : Consumer & Producer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		>=0 : the data byte size of CBuf
//!						E_INVALIDARG	|  The parameter is invalid
//!						E_ACCESSDENIED	|  Only Consumer Control Object could call this function
//!
////////////////////////////////////////////////////////////////////////////////
INT32  MV_CC_CBuf_GetDataSize(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:     MV_CC_CBuf_GetDataSize
//!
//! Description: Query about the data size and the linear data size of the CBuf
//! Allowed by : Consumer & Producer
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//! \param pLinearSize	(IN/OUT): -- the pointer to the linear size
//!
//! \return Return:		>=0 : the data byte size of CBuf
//!
////////////////////////////////////////////////////////////////////////////////
INT32  MV_CC_CBuf_GetDataSizeAndNoRoll(MV_CC_HANDLE_CBufCtrl_t 	Handle,
									   INT32 					*pLinearSize);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_CBuf_Reset
//!
//! Description: Reset a CBuf to initialized status
//! Allowed by : warning!!! this operation is dangerous. m_RdOffset is operated by consumer, and m_WrOffset is operated by producer
//!	Work Flow  :
//!				Reset the read pointer and the write pointer to zero for clearring buffer.
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG	|  The parameter is invalid
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_CBuf_Reset(MV_CC_HANDLE_CBufCtrl_t 	Handle);

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_CC_CBuf_Reset_WrOffset
//!
//! Description: reset buffer Wr by Rd
//! Allowed by : warning!!! this operation is dangerous, maybe conflicted with consumer. m_RdOffset is operated by consumer
//!	Work Flow  :
//!				Reset the writing pointer to the reading pointer for clearring buffer.
//!
//! \param Handle 			(IN): -- the CBuf Control Object Handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG	|  The parameter is invalid
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT MV_CC_CBuf_Reset_WrOffset(MV_CC_HANDLE_CBufCtrl_t 	Handle);

void *Galois_shm_align_malloc(UINT32 size, UINT32 align);
void Galois_shm_align_free(void * buf);
#define GALOIS_SHM_1K_MALLOC(x)		Galois_shm_align_malloc((x), 1024)
#define GALOIS_SHM_1K_FREE(x) Galois_shm_align_free(x)
#define GALOIS_SHM_4K_MALLOC(x)		Galois_shm_align_malloc((x), 4096)
#define GALOIS_SHM_4K_FREE(x) Galois_shm_align_free(x)

//The devmem_virt_to_phy macro for various OSAL implemenation
#if defined(VPP_DIAGS)
    #define devmem_phy_to_virt(pPhyAddr)    pPhyAddr
#elif defined(CONFIG_AMP_TEE_ENABLE)
#ifndef CONFIG_OPTEE
    #define devmem_phy_to_virt(pPhyAddr)    TEE_PhysToVirt((void *)pPhyAddr)
#else
    #define devmem_phy_to_virt(pPhyAddr)    TEE_TA_RegPhyToVirt(pPhyAddr)
#endif
#elif defined( __LINUX_KERNEL__ )
    #define devmem_phy_to_virt(pPhyAddr)    virt_to_phys(pPhyAddr)
#else
    //devmem_virt_to_phy: Will be implemented in galois_dev_mem_map.c
    //pVirAddr = devmem_virt_to_phy(pPhyAddr);
#endif


//The devmem_virt_to_phy macro for various OSAL implemenation
#if defined(VPP_DIAGS)
    #define devmem_virt_to_phy(pVirAddr)        pVirAddr
#elif defined(CONFIG_AMP_TEE_ENABLE)
    #define devmem_virt_to_phy(pVirAddr)        TEE_VirtToPhys((void *)pVirAddr)
#elif defined( __LINUX_KERNEL__ )
    #define devmem_virt_to_phy(pVirAddr)        virt_to_phys(pVirAddr)
#else
    //devmem_virt_to_phy: Will be implemented in galois_dev_mem_map.c
    //pPhyAddr = devmem_virt_to_phy(pVirAddr);
#endif

#ifdef __cplusplus
}		// extern "C"
#endif	// #ifdef __cplusplus

#endif
