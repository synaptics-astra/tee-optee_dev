/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
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
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
////////////////////////////////////////////////////////////////////////////////
//! \file: otp_program_main.h
//!
//! \brief: program OTP tool
//!
//!
//! Version    Date                     Author
//! V 0.01,    Apr 16 2019,             Cheney Yang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////


#ifndef __DRM_OTP_PROGRAM_H__
#define __DRM_OTP_PROGRAM_H__

typedef enum
{
    PROVISION_SUCCESS = 0,
    PROVISION_ERROR_INVALID_PARAM = 1,
    PROVISION_ERROR_AMP = 2,
    PROVISION_ERROR_KMS = 3,
    PROVISION_ERROR_OPENFILE = 4,
    PROVISION_ERROR_MEMORY = 5,
    PROVISION_ERROR_READFILE = 6,
    PROVISION_ERROR_RUN_TWICE = 7,
    PROVISION_ERROR_OTP = 8,
    PROVISION_ERROR_CHECK_IMAGE = 9,
    PROVISION_ERROR_DELETE_KEYS = 10,
    PROVISION_ERROR_WRITE_APP_KEYS = 11,
    PROVISION_ERROR_LOAD_SECURE_BOOT_KEYS = 12,
    PROVISION_ERROR_CHECK_APPKEYS = 13,
    PROVISION_ERROR_READ_OTP = 14,
    PROVISION_ERROR_READ_PROD_BCM = 15,
    PROVISION_ERROR_UPDATE_PREBOOT = 16,
    PROVISION_ERROR_UPDATE_KEYS = 17,
} PROVISION_RETURN_CODE;

#endif
