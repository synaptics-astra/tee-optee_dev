/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


////////////////////////////////////////////////////////////////////////////////
//! \file OSALTimer.c
//! \brief
//!
//! Purpose: Galois Linux platform
//!
//!
//!	Version, Date and Author :
//!		V 1.00,	7/19/2007,	Fang Bao
//!		V 1.01,	9/12/2007,	Fang Bao
//!				Clean up code and add the new debug trace function
//!		V 1.02,	7/25/2008,	Fang Bao
//!				Bugfixed - when call MV_Timer_SetState() to start timer, the first time of callback will be called at once.
//!							set the pTimer->m_TimerTime.m_value the next time value in MV_Timer_SetState()
//!		V 1.03,	2/11/2009,	Fang Bao
//!				Bugfixed -  Disable MV_Timer_Destroy() and MV_Timer_Reset() when Timer is started
//!				Bugfixed -  Disable MV_Timer_Start() and MV_Timer_Stop() when API is called repeated { MV_Timer_SetState() }
//!				Bugfixed -  missing return code in MV_Timer_Reset_Failure_1 of MV_Timer_Reset()
//!		V 1.04,	2/23/2009,	Fang Bao
//!				Bugfixed -  MV_OSAL_Mutex_Unlock() missing in MV_Timer_Destroy()
//!				Bugfixed -  MV_Timer_TimerTask need more time to do cancellation, so add MV_OSAL_Task_Sleep() in MV_Timer_Destroy()
//!		V 1.05,	8/07/2009,	Fang Bao
//!				Add MV_TimerState_EXITED. Now, timer task exited to TASK_JOIN not TASK_DESTROY
//!				Add APIs - MV_Timer_Pause() and MV_Timer_Resume() which are available on Linux and eCos OS
//!		V 1.06,	8/10/2009,	Fang Bao
//!				Add APIs - MV_Timer_Reset_Period()
//!		V 1.07,	8/11/2009,	Fang Bao
//!				Bugfixed - build error in MV_Timer_Resume(), MV_Timer_Pause(), MV_Timer_Stop() when debug_osal is enable.
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "OSAL_api.h"
#include "tee_internal_api.h"
#include "vpp_cmd.h"
#include "vpp_osal.h"


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
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Create(MV_HANDLE_Timer_t	*pHandle,
						 UINT				Period,
						 PVOID				CallbackFunc,
						 PVOID				UserData)
{
    HRESULT retVal = S_OK;
    TEE_Result result;
    uint32_t cb_cmdID = CB_TIMER_CREATE;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);
    TEE_Param cb_params[4];
    cb_params[0].value.a = Period;
    cb_params[1].value.a = (UINT32)CallbackFunc;
    cb_params[2].value.a = (UINT32)UserData;

    result = TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
#ifdef TZ_3_0
    VPP_OSAL_SHM_Dynamic_Malloc(AMP_SHM_SECURE_VIDEO_FB, sizeof(MV_HANDLE_Timer_t)*2, 32, (VPP_SHM_HANDLE*)pHandle);
#else
    *pHandle = VPP_Malloc(sizeof(MV_HANDLE_Timer_t)*2);
#endif
    memcpy(*pHandle, &cb_params[0].value.a, sizeof(MV_HANDLE_Timer_t));
    memcpy(((*pHandle)+sizeof(MV_HANDLE_Timer_t)), &cb_params[1].value.a, sizeof(MV_HANDLE_Timer_t));

//    info("timer handler cmdID=%d\n",cb_params[0].value.a);
    return retVal;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Destroy
//!
//! Description: Destroy a timer (before destroying, the timer will be stopped)
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Destroy(MV_HANDLE_Timer_t	*pHandle)
{
    HRESULT retVal = S_OK;
    TEE_Result result;
    uint32_t cb_cmdID = CB_TIMER_DESTROY;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);
    TEE_Param cb_params[4];

    memcpy(&cb_params[0].value.a, *pHandle, sizeof(MV_HANDLE_Timer_t));
    memcpy(&cb_params[1].value.a, ((*pHandle)+sizeof(MV_HANDLE_Timer_t)), sizeof(MV_HANDLE_Timer_t));
    result = TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    TEE_Free(*pHandle);
    *pHandle = NULL;

//    info("timer handler cmdID=%d\n",cb_params[0].value.a);
    return retVal;
}


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
//!						E_NOTREADY
//! 					E_INVALIDARG
//!
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT  MV_Timer_Reset(MV_HANDLE_Timer_t	*pHandle,
						 UINT				Period,
						 PVOID				CallbackFunc,
						 PVOID				UserData)
{
    HRESULT retVal = S_OK;
    TEE_Result result;
    uint32_t cb_cmdID = CB_TIMER_RESET;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);
    TEE_Param cb_params[4];

    memcpy(&cb_params[0].value.a, *pHandle, sizeof(MV_HANDLE_Timer_t));
    memcpy(&cb_params[0].value.b, ((*pHandle)+sizeof(MV_HANDLE_Timer_t)), sizeof(MV_HANDLE_Timer_t));
    cb_params[1].value.a = Period;
    cb_params[2].value.a = (UINT32)CallbackFunc;
    cb_params[3].value.a = (UINT32)UserData;

    result = TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    memcpy(*pHandle, &cb_params[0].value.a, sizeof(MV_HANDLE_Timer_t));
    memcpy(((*pHandle)+sizeof(MV_HANDLE_Timer_t)), &cb_params[1].value.a, sizeof(MV_HANDLE_Timer_t));
    return retVal;
}

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
						 UINT				Period)
{
    HRESULT retVal = S_OK;
    TEE_Result result;
    uint32_t cb_cmdID = CB_TIMER_RESET_PERIOD;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);
    TEE_Param cb_params[4];

    memcpy(&cb_params[0].value.a, *pHandle, sizeof(MV_HANDLE_Timer_t));
    cb_params[1].value.a = Period;
    result = TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return retVal;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Start
//!
//! Description: Start a timer
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT   MV_Timer_Start(MV_HANDLE_Timer_t	*pHandle)
{
    HRESULT retVal = S_OK;
    TEE_Result result;
    uint32_t cb_cmdID = CB_TIMER_START;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);
    TEE_Param cb_params[4];

    memcpy(&cb_params[0].value.a, *pHandle, sizeof(MV_HANDLE_Timer_t));
    result = TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return retVal;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Stop
//!
//! Description: Stop a timer
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT   MV_Timer_Stop(MV_HANDLE_Timer_t	*pHandle)
{
    HRESULT retVal = S_OK;
    TEE_Result result;
    uint32_t cb_cmdID = CB_TIMER_STOP;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);
    TEE_Param cb_params[4];

    memcpy(&cb_params[0].value.a, *pHandle, sizeof(MV_HANDLE_Timer_t));
    result = TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return retVal;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Resume
//!
//! Description: Resume a timer when timer is on paused status
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT   MV_Timer_Resume(MV_HANDLE_Timer_t	*pHandle)
{
    HRESULT retVal = S_OK;
    TEE_Result result;
    uint32_t cb_cmdID = CB_TIMER_RESUME;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);
    TEE_Param cb_params[4];

    memcpy(&cb_params[0].value.a, *pHandle, sizeof(MV_HANDLE_Timer_t));
    result = TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return retVal;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Function:    MV_Timer_Pause
//!
//! Description: Pause a timer when timer is on started status
//!
//! \param pHandle		  (IN):	The pointer to the handle of the timer object
//!
//! \return Return:		S_OK
//! 					E_INVALIDARG
//!						E_NOTREADY
//!
////////////////////////////////////////////////////////////////////////////////
HRESULT   MV_Timer_Pause(MV_HANDLE_Timer_t	*pHandle)
{
    HRESULT retVal = S_OK;
    TEE_Result result;
    uint32_t cb_cmdID = CB_TIMER_PAUSE;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_VALUE_INOUT);
    TEE_Param cb_params[4];

    memcpy(&cb_params[0].value.a, *pHandle, sizeof(MV_HANDLE_Timer_t));
    result = TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    return retVal;
}

