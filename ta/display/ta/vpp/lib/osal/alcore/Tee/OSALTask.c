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

#include <tee_internal_api.h>

#include "OSAL_api.h"

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
	TEE_Wait(TimeOutMS);
	return S_OK;
}


