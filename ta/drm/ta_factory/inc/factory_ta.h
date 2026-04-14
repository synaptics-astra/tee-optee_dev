/*******************************************************************************
 *                 2017 Synaptics Incorporated. All Rights Reserved            *
 * THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF Synaptics.                   *
 * NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT *
 * OF Synaptics OR ANY THIRD PARTY. Synaptics RESERVES THE RIGHT AT ITS SOLE   *
 * DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO Synaptics.  *
 * THIS CODE IS PROVIDED "AS IS". Synaptics MAKES NO WARRANTIES, EXPRESSED,    *
 * IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.  *
 *                                                                             *
 *******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
//! \file    factory_ta.h
//! \brief   Factory TA header file
//! \author  Frank Ho
//! \date    Dec 2017
////////////////////////////////////////////////////////////////////////////////

#ifndef __FACTORY_TA_H__
#define __FACTORY_TA_H__

#ifndef CONFIG_OPTEE
#include "tee_log.h"
#endif

#ifdef TZ_3_0
#include "mem_region_userdata.h"
#endif

/* uuid defined for drm secure engine wrapper ta */
#define FACTORY_UUID {0x1316a183, 0x894d, 0x43fe, \
    {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x20}}


//Command Masks
#define COMMAND_MASK                                0xF000
#define OTP_MASK                                    0xE000
#define CRYPTO_MASK                                 0xD000
#define RPMB_MASK                                   0xC000

//Command
#define DRM_SE_GENX_OEM_COMMAND                     0xF001
#define DRM_SE_OEM_ACCESS_SECURE_SRAM               0xF002
#define DRM_SE_OEM_AES_GCM_ENCRY_DECRY              0xF003

//OTP
#define DRM_SE_OTP_BURNLAYOUT                       0xE001
#define DRM_SE_OTP_READ_RKEKID                      0xE002
#define DRM_SE_OTP_READ_CAS1_JTAG                   0xE003
#define DRM_SE_OTP_READ_CAS2_JTAG                   0xE004
#define DRM_SE_OTP_READ_CAS4_JTAG                   0xE005
#define DRM_SE_OTP_READ_CAS4_NDSID                  0xE006
#define DRM_SE_OTP_READ_AUTO_JTAG                   0xE007
#define DRM_SE_OTP_READ_PUBLIC_ITEMS                0xE008
#define DRM_SE_OTP_READ_SOC_UID                     0xE009
#define DRM_SE_OTP_READ_CHIP_REV                    0xE00A
#define DRM_SE_OTP_WRITE_RKEKID                     0xE00B
#define DRM_SE_OTP_READ_OTP                         0xE00C
#define DRM_SE_OTP_WRITE_OTP                        0xE00D

//CRYPTO
#define DRM_SE_RSA_SIGN_VERIFY                      0xD001
#define DRM_SE_GEN_SECURE_KEY                       0xD002

//RPMB
#define DRM_SE_RPMB_PAIR_KEY                        0xC001

#endif
