/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 */

///////////////////////////////////////////////////////////////////////////////
//! \file m2m_ta_core.h
//!
//! \brief  This file is used to implement the ta session wrapper for m2m ca
//!
//! Author:         Long Xiao
//! Modified by:
//! Version:        1.0
//! Date:           Aug 4 2015
//!
///////////////////////////////////////////////////////////////////////////////
#ifndef __M2M_TA_CORE_H__
#define __M2M_TA_CORE_H__

#include "m2m_ta_comm.h"

HRESULT M2M_TA_Core_Session_Open(TA_M2M_CRYPTO_H hCryptoSrv,
                M2MCRYPTO_SESSION_TYPE eType,
                TA_M2M_SESS_HANDLE *phSession);
HRESULT M2M_TA_Core_Session_Close(TA_M2M_SESS_HANDLE hSess);
HRESULT M2M_TA_Core_AllocFigoTask(UINT32 uFigoId);
HRESULT M2M_TA_Core_FreeFigoTask(UINT32 uFigoId);
HRESULT M2M_TA_Core_StartFw(UINT32 *pFigoId);
HRESULT M2M_TA_Core_StopFw(UINT32 uFigoId);
HRESULT M2M_TA_Core_Session_SetScheme(TA_M2M_SESS_HANDLE hSess, M2MCRYPTO_TYPE eScheme);
#ifndef TA_COMPILE
HRESULT M2M_TA_Core_Session_Config(TA_M2M_SESS_HANDLE hSess,
                            M2MCRYPTO_KEY_OPTION  KeyOption,
                            UINT32          KeyTblIndex,
                            UINT8           *pKeyData,
                            UINT32          KeyLength,
                            M2MCRYPTO_KEY_OPTION  IVOption,
                            UINT32          IVTblIndex,
                            UINT8           *pIVData,
                            UINT32          IVLength,
                            DRM_CRYPTO_ST   *pCryptInfo);
#else
HRESULT M2M_TA_Core_Session_Config(TA_M2M_SESS_HANDLE hSess,
                            M2MCRYPTO_KEY_OPTION  KeyOption,
                            UINT32          KeyTblIndex,
                            TA_M2M_BUFF_T  *pKeyBuf,
                            M2MCRYPTO_KEY_OPTION  IVOption,
                            UINT32          IVTblIndex,
                            TA_M2M_BUFF_T  *pIVBuf,
                            TA_M2M_BUFF_T   *pCryptInfoBuf);
#endif
HRESULT M2M_TA_Core_Session_SetPatternMode(TA_M2M_SESS_HANDLE hSess,
                            M2MCRYPTO_PATTERN_MODE_CFG *pCfg);
HRESULT M2M_TA_Core_Session_SetResidueMode(TA_M2M_SESS_HANDLE hSess,
                            M2MCRYPTO_RESIDUE_TYPE eType);
HRESULT M2M_TA_Core_Session_SetIVPolicy(TA_M2M_SESS_HANDLE hSess,
                            M2MCRYPTO_IV_POLICY ePolicy);

#ifndef TA_COMPILE
HRESULT M2M_TA_Core_Session_Update(TA_M2M_SESS_HANDLE hSess,
                                   M2MCRYPTO_MEM_H hInput,
                                   M2MCRYPTO_MEM_H hOutput);
#else
HRESULT M2M_TA_Core_Session_Update(TA_M2M_SESS_HANDLE hSess,
                                   TA_M2M_BUFF_T *pInputBuf,
                                   TA_M2M_BUFF_T *pOutputBuf);
#endif
HRESULT M2M_TA_Core_Session_Feed(TA_M2M_SESS_HANDLE hSess);
HRESULT M2M_TA_Core_Session_Postprocess(TA_M2M_SESS_HANDLE hSess, FIGODRV_HANDLE  hFigoDrv);
HRESULT M2M_TA_Session_Postprocess(TA_M2M_SESS_HANDLE hSess, UINT16 uCmdTag);

#endif

