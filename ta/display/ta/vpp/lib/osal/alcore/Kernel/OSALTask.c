/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


////////////////////////////////////////////////////////////////////////////////
//! \file OSALTask.c
//! \brief This is the Android-specific implementation of the OSAL Task.
//!
//! Purpose:
//!
//!
//!	Version, Date and Author :
//!		V 1.00,	4/03/2007,	Fang Bao
//!		V 1.10,	6/07/2007,	Fang Bao
//!				Add Inter-Task Message Queue for OSAL Task
//!				API - MV_OSAL_Task_CreateWithMsgQ
//!				API - MV_OSAL_Task_MsgQITC_Post
//!				API - MV_OSAL_Task_MsgQITC_Get
//!				API - MV_OSAL_Task_MsgQITC_GetTry
//!		V 1.11,	6/11/2007,	Fang Bao
//!				Add OSAL Task List Manage
//!				API - MV_OSAL_Task_Self can return the taskself handle(OSAL)
//!				API - MV_OSAL_TaskInit must run first of all task created.
//!				API - MV_OSAL_TaskExit must run last of all task destroyed.
//!		V 1.12,	7/03/2007,	Fang Bao
//!				Bugfix - *self = (MV_OSAL_HANDLE_TASK_t *)pTask; Now is fixed by *self = (MV_OSAL_HANDLE_TASK_t )pTask;
//!				Add API - MV_OSAL_Task_Sleep
//!				Add API - MV_OSAL_Task_Yield
//!		V 1.13,	7/25/2007,	Fang Bao
//!				Add API - MV_OSAL_Task_Create_Advanced for creating a task with message and priority
//!		V 1.14,	9/12/2007,	Fang Bao
//!				Clean up code and add the new debug trace function
//!		V 1.15,	12/15/2007,	Fang Bao
//!				Bug fix:  Add (pNode == NULL) in MV_OSAL_TaskList_SearchXXX for exception
//!		V 1.23,	5/06/2008,  Fang Bao
//!				Add macro - MV_OSAL_Task_NMsgsMin
//!				Modify the NMsgsMax of the MV_OSAL_Task_Constructor
//!		V 1.24,	9/23/2008,  Fang Bao
//!				Replace old task list by std_list APIs
//!				Remove APIs - MV_OSAL_TaskList_XXX()
//!		V 1.25,	12/12/2008,	Fang Bao
//!				Add parameter check in MV_OSAL_Task_Self(), MV_OSAL_Task_MsgQ_Post(), MV_OSAL_Task_MsgQ_Get(), MV_OSAL_Task_MsgQ_GetTry(), MV_OSAL_Task_Create_Advanced()
//!		V 2.00,	12/29/2010,	Fang Bao
//!				Modify MV_OSAL_Task_Create_Advanced() for Android
//!             		move MV_OSAL_Task_Destroy() which is not supported by Android
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include <linux/kthread.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include "OSAL_api.h"

#include "std_list.h"

////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_TASK_t 	| Object data structure
//!								pMV_OSAL_TASK_t | Pointer to the Object data structure
////////////////////////////////////////////////////////////////////////////////
typedef struct _MV_OSAL_Task {

	//! pthread for Task
	struct task_struct *m_Task;

} MV_OSAL_TASK_t, *pMV_OSAL_TASK_t;

std_list *pGList_Task = NULL;
MV_OSAL_HANDLE_MUTEX_t hMutex_pGList_Task = NULL;


////////////////////////////////////////////////////////////////////////////////
//! \brief OSAL Data Structure: MV_OSAL_TASK_Attr_t 	| Task Attribute Object data structure
//!								pMV_OSAL_TASK_Attr_t	| Pointer to the Task Attribute Object data structure
////////////////////////////////////////////////////////////////////////////////

static size_t tasks_affinity = 1;

HRESULT MV_OSAL_Task_Data_Destructor(PVOID pData)
{
	pMV_OSAL_TASK_t pTask = (pMV_OSAL_TASK_t)pData;

	/* Parameter Check */
	if (pData == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Task_Data_Destructor", NULL);

	MV_OSAL_Free(pTask);

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Set_CPUAffinity
//!
//! Description: Set tasks' CPU affinity
//!
//! \param :			affinity mask.
//!
//! \return Return:		S_OK
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Set_CPUAffinity(UINT affinity)
{
    tasks_affinity = affinity;

    return S_OK;
}


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
HRESULT  MV_OSAL_Init_Task()
{
	HRESULT res;

	pGList_Task = std_list_ctor(NULL, &MV_OSAL_Task_Data_Destructor);
	if (pGList_Task == NULL)
	{
		res = E_FAIL;
		MV_OSAL_DBG_Error(res, "MV_OSAL_Init_Task pGList_Task", NULL);
	}

	res = MV_OSAL_Mutex_Create(&hMutex_pGList_Task);
	if (res != S_OK)
		MV_OSAL_DBG_Error(res, "MV_OSAL_Init_Task MV_OSAL_Mutex_Create", NULL);

	return res;
}

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
HRESULT  MV_OSAL_Exit_Task()
{
	HRESULT res;

	res = MV_OSAL_Mutex_Destroy(&hMutex_pGList_Task);
	if (res != S_OK)
		MV_OSAL_DBG_Error(res, "MV_OSAL_Exit_Task MV_OSAL_Mutex_Destroy", NULL);

	if (pGList_Task != NULL)
		std_list_dtor(pGList_Task);

	return res;
}



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Constructor
//!
//! Description: Task Handle Self Constructor
//!
//! \param self 		(OUT): -- the pointer to the task handle
//! \param NMsgsMax		(IN): -- the max number of messages in the Message Queue
//!
//! \return Return:		S_OK
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Constructor(MV_OSAL_HANDLE_TASK_t 	*self,
								  UINT32 const NMsgsMax)
{
	HRESULT res;
	pMV_OSAL_TASK_t pTask;

	pTask = (pMV_OSAL_TASK_t)MV_OSAL_Malloc(sizeof(MV_OSAL_TASK_t));
	if (pTask == NULL)
		MV_OSAL_DBG_Error(E_OUTOFMEMORY, "MV_OSAL_Task_Constructor MV_OSAL_Malloc", NULL);

	// Add a task to pGList_Task
	MV_OSAL_Mutex_Lock(hMutex_pGList_Task);

	res = std_list_push_back(pGList_Task, pTask);
	if (res != S_OK)
		MV_OSAL_DBG_Warning(res, "MV_OSAL_Task_Constructor std_list_push_back", NULL);

	MV_OSAL_Mutex_Unlock(hMutex_pGList_Task);

	*self = (MV_OSAL_HANDLE_TASK_t)pTask;

	return res;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Destructor
//!
//! Description: Task Handle Self Destructor
//!
//! \param self 	(IN/OUT): -- the pointer to the task handle
//!
//! \return Return:		S_OK
//!						E_INVALIDARG
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Destructor(MV_OSAL_HANDLE_TASK_t 	*self)
{
	HRESULT res;
	pMV_OSAL_TASK_t pTask;
	std_list_node* pNode;

	if (self == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Task_Destructor", NULL);

	pTask = (pMV_OSAL_TASK_t)(*self);

	/* Parameter Check */
	if (pTask == NULL)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Task_Destructor", NULL);

	// Remove a task from pGList_Task
	MV_OSAL_Mutex_Lock(hMutex_pGList_Task);

	pNode = std_list_search(pGList_Task, *self);
	if (pNode == NULL)
	{
		res = E_NOTEXIST;
		MV_OSAL_DBG_Warning(res, "MV_OSAL_Task_Destructor hLogBuf", NULL);
	}

	res = std_list_remove(pGList_Task, pNode);
	if (res != S_OK)
		MV_OSAL_DBG_Warning(res, "MV_OSAL_Task_Destructor std_list_remove", NULL);

	MV_OSAL_Mutex_Unlock(hMutex_pGList_Task);

	*self = NULL;

	return res;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_Create_Advanced
//!
//! Description: Create a OSAL advanced task (New Handle, Auto Constructor) with a message queue
//!
//! \param pHandle 	   (OUT): -- the pointer to the task handle (New handle)
//! \param TaskFunc		(IN): -- the pointer of the task function
//! \param Arg			(IN): -- the argument to the TaskFunction()
//! \param Priority		(IN): -- the task priority
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
                           				UINT32 const			NMsgsMax)
{
	HRESULT res;
	pMV_OSAL_TASK_t pTask = NULL;

	/* Parameter Check */
	if ( (TaskFunc == NULL) ||
		( Priority < MV_OSAL_TASK_PRIORITY_MIN ) ||
		( Priority > MV_OSAL_TASK_PRIORITY_MAX ) ||
		( pHandle == NULL ))
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Task_Create_Advanced", NULL);

	res = MV_OSAL_Task_Constructor(pHandle, NMsgsMax);
	if (res != S_OK)
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Task_Create_Advanced MV_OSAL_Task_Constructor", NULL);

	pTask = (pMV_OSAL_TASK_t)(*pHandle);

	pTask->m_Task = kthread_run(TaskFunc,(void *)Arg,"taskname");

	return S_OK;
}

HRESULT  MV_OSAL_Task_Create_Detailed(MV_OSAL_HANDLE_TASK_t 	*pHandle,
										VOID					*TaskFunc,
                           				VOID 					*Arg,
                           				MV_OSAL_PRIORITY_t const Priority,
                           				CHAR					*pName,
                           				VOID					*pStackBase,
                           				UINT32					StackSize,
                           				UINT32 const			NMsgsMax)
{
	return (MV_OSAL_Task_Create_Advanced(pHandle, TaskFunc, Arg, Priority, NMsgsMax));
}



////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_CreateWithMsgQ
//!
//! Description: Create a OSAL task (New Handle, Auto Constructor) with a message queue
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
                           			UINT32 const			NMsgsMax)
{
	return (MV_OSAL_Task_Create_Advanced(pHandle, TaskFunc, Arg, MV_OSAL_TASK_PRIORITY_DEFAULT, NMsgsMax));
}



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
//!						E_TASKCREATE
//!						E_INVALIDARG
//!						E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_Create(MV_OSAL_HANDLE_TASK_t 	*pHandle,
									VOID					*TaskFunc,
                           			VOID 					*Arg)
{
	return (MV_OSAL_Task_Create_Advanced(pHandle, TaskFunc, Arg, MV_OSAL_TASK_PRIORITY_DEFAULT, 0));
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_OSAL_Task_SetName(CHAR *pName)
//!
//!
//! \return Return:     S_OK
//!                     E_INVALIDARG
//!                     E_OUTOFMEMORY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_OSAL_Task_SetName(CHAR *pName)
{

#if defined(BERLIN_SINGLE_CPU) && !defined(__LINUX_KERNEL__)
	if (pName != NULL)
		prctl(PR_SET_NAME, pName, 0, 0, 0);
#endif
	return S_OK;

}


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
#ifdef BERLIN_SINGLE_CPU
#ifndef __BIONIC__

HRESULT  MV_OSAL_Task_Destroy(MV_OSAL_HANDLE_TASK_t 	*pHandle)
{
	HRESULT res;
	pMV_OSAL_TASK_t pTask;

	/* Parameter Check */
	if ((pHandle == NULL) || ((*pHandle) == NULL))
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Task_Destroy", NULL);

	res = S_OK;

	pTask = (pMV_OSAL_TASK_t)(*pHandle);

	if ((kthread_stop(pTask->m_Task)) != 0)
	{
		res = E_TASKDESTROY;
		MV_OSAL_DBG_Warning(res, "MV_OSAL_Task_Destroy pthread_cancel", NULL);
	}

	MV_OSAL_DBG_Info("MV_OSAL_Task_Destroy Task's Handle[0x%08x]\n", pTask);

	res = MV_OSAL_Task_Destructor(pHandle);

	return res;
}
#endif
#endif


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
HRESULT  MV_OSAL_Task_Join(MV_OSAL_HANDLE_TASK_t 	*pHandle)
{
	HRESULT res;
	pMV_OSAL_TASK_t pTask;

	/* Parameter Check */
	if ((pHandle == NULL) || ((*pHandle) == NULL))
		MV_OSAL_DBG_Error(E_INVALIDARG, "MV_OSAL_Task_Join", NULL);

	res = S_OK;

	pTask = (pMV_OSAL_TASK_t)(*pHandle);

	if ((kthread_stop(pTask->m_Task)) != 0)
	{
		res = E_TASKFAIL;
		MV_OSAL_DBG_Warning(res, "MV_OSAL_Task_Join pthread_join", NULL);
	}

	MV_OSAL_DBG_Info("MV_OSAL_Task_Join Task's Handle[0x%08x]\n", pTask);

	res = MV_OSAL_Task_Destructor(pHandle);

	return res;
}


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
HRESULT  MV_OSAL_Task_Sleep(UINT32 TimeOutMS)
{
	msleep(TimeOutMS);
	return S_OK;
}


