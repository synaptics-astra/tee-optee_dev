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
//! \file: DRM_OTP_Agent_API.h
//!
//! \brief: program OTP tool API
//!
//!
//! Version    Date                     Author
//! V 0.01,    Apr 16 2019,             Cheney Yang
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __DRM_OTP_AGENT_API_H__
#define __DRM_OTP_AGENT_API_H__

#ifndef _typedef___UCHAR
#define _typedef___UCHAR
typedef unsigned char       UCHAR;
#endif /* _typedef___UCHAR */

#ifndef _typedef___UINT8
#define _typedef___UINT8
typedef UCHAR               UINT8;
#endif /* _typedef___UINT8 */

#ifndef _typedef___UINT16
#define _typedef___UINT16
typedef unsigned short      UINT16;
#endif /* _typedef___UINT16 */

#ifndef _typedef___UINT32
#define _typedef___UINT32
typedef unsigned int        UINT32;
#endif /* _typedef___UINT32 */

#ifndef _typedef___HRESULT
#define _typedef___HRESULT
typedef signed int HRESULT;
#endif /* _typedef___HRESULT */

#if !defined(S_OK) && !defined(S_FALSE)
#define S_OK 0
#define S_FALSE 1
#endif

#define MV_OTP_BCM_CTRL_FLAG_BYPASS_BLANK_CHECK (0x1 << 0)

#define DRM_OTP_RESP_BITMAP_RKEKID      (0x1 << 0)
#define DRM_OTP_RESP_BITMAP_RETRY_MAP   (0x1 << 1)
#define DRM_OTP_RESP_BITMAP_FAIL_MAP    (0x1 << 2)
#define DRM_OTP_RESP_BITMAP_RELOAD_FAIL (0x1 << 3)

/**
 *
 */
HRESULT MV_OTP_BCM_Layout_Program(UCHAR* otp_pack_file);
HRESULT MV_OTP_Full_Factory_Flow(UCHAR* otp_pack_file);

#endif
