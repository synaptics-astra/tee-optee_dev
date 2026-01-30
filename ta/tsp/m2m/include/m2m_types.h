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

////////////////////////////////////////////////////////////////////////////////
//! \file m2m_types.h
//!
//! \brief This file defined common types used in AMP.
//!
//! Purpose:
//!
//!
//! Version, Date and Author :
//!   V 1.00,    Septemper 18 2012,    Justin Wu
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
#ifndef __M2M_TYPES_H__
#define __M2M_TYPES_H__

/** \enum HRESULT_VALUE
 *
 *  \brief The AMP software module code.
 */
enum HRESULT_VALUE {
    SUCCESS         = 0x0000,   /**< Success. */
    ERR_NOTIMPL     = 0x0001,   /**< Function/feature not implementated. */
    ERR_ERRPARAM    = 0x0002,   /**< Function parameter error. */
    ERR_NOMEM       = 0x0003,   /**< Not enough memory. */
    ERR_NOSHM       = 0x0004,   /**< Not enough share memory. */
    ERR_TIMEOUT     = 0x0005,   /**< Operation timeout. */
    ERR_ERRSYSCALL  = 0x0006,   /**< Syscall error. */
    ERR_IOFAIL      = 0x0007,   /**< Perepheral IO fail. */
    ERR_EVENTFULL   = 0x0008,   /**< Event queue full. */
    ERR_HWBUSY      = 0x0009,   /**< Hardware busy. */
    ERR_HWFAIL      = 0x000A,   /**< Hardware fail. */
    ERR_OSALFAIL    = 0x000B,   /**< OSAL fail. */
    ERR_NOSWRSC     = 0x000C,   /**< Not enough software resource. */
    ERR_NOHWRSC     = 0x000D,   /**< Not enough hardware resource. */
    ERR_SWSTATEWRONG= 0x000E,   /**< Don't permit in current software state. */
    ERR_HWSTATEWRONG= 0x000F,   /**< Don't permit in current hardware state. */
    ERR_RCPERROR    = 0x0010,   /**< RPC error. */
    ERR_SWMODEWRONG = 0x0011,   /**< Software mode wrong. */
    ERR_HWMODEWRONG = 0x0012,   /**< Hardware mode wrong. */
    ERR_CONNCLEARED = 0x0013,   /**< Connection has been cleared error. */
    ERR_RANGEPARAM  = 0x0014,   /**< Parameter is out of range. */
    ERR_CANCELLED   = 0x0015,   /**< Operation is cancelled. */
    ERR_BDERROR     = 0x0016,   /**< BD data error. */

    ERR_PRIVATE_BASE= 0x1000,   /**< The module private error value base. */
};

/** \enum HRESULT_PRIVATE_VALUE
 *
 *  \brief The private error code define.
 */
enum HRESULT_PRIVATE_VALUE {
    ERR_NULL_PTR = ERR_PRIVATE_BASE | 0x1,
    ERR_SECURE_RULE = ERR_PRIVATE_BASE | 0x20,
    ERR_CMDQ_PUSHED = ERR_PRIVATE_BASE | 0x22,
    ERR_CMDQ_UNDERFLOW = ERR_PRIVATE_BASE | 0x23,    

};

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif


#endif  /* __M2M_TYPES_H__ */
