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
//! \file    factory_otp.h
//! \brief   Factory TA OTP header
//! \author  Frank Ho
//! \date    Dec 2017
////////////////////////////////////////////////////////////////////////////////

#ifndef __FACTORY_OTP_H__
#define __FACTORY_OTP_H__

#include "drm_tee_types.h"

#define PUBLIC_OTP_SIZE 256

typedef struct {
   unsigned int   TA_Vendor_SegID[6];
   unsigned char  TA_Vendor_GID[6];
   unsigned char  Security_Indicators;
   unsigned int   BOOT_SegID;
   unsigned char  Lock_Boot_SegID;
   unsigned char  MP_Provision_Done;
   unsigned char  REE_Security_Enable;
   unsigned char  TEE_Segmentation_Version_Enable;
   unsigned char  Boot_Security_Enable;
   unsigned int   TEE_SegID;
   unsigned char  SCS_Total_Area_Size_Sel;
   unsigned char  Background_Check_Enable;
   unsigned char  JtagProg_Disable;
   unsigned int   REE_SegID;
   unsigned int   ConcurrencyControl;
   unsigned char  v_PubOtpMinConfVer;
   unsigned char  v_PubOtpRsaIndex;
   unsigned short v_PubOtpVID;
   unsigned short v_PubOtpOID;
   unsigned char  OtpWrtPwdProt;
   unsigned char  JTAG_Protection;
   unsigned char  JTAG_Protection_dup;
   unsigned char  BSCAN_Protection;
   unsigned char  BSCAN_Protection_dup;
   unsigned char  STEST_Protection;
   unsigned char  STEST_Protection_dup;
   unsigned char  RKEKID[8];
   unsigned int   K0_REE_HASH[8];
   unsigned int   REE_VERSION;
   unsigned int   BOOT_VERSION;
   unsigned int   JTAGProtectionLevel;
} PUBLIC_OTP_ITEMS;

/* OTP related command dispatcher
 *
 * Arguments:
 * - commandID      (IN) - command related with OTP
 * - params[4]      (IN) - parameters from normal world
 *
 * Return:
 * TEE_SUCCESS or one of the TEE error code
 */
TEE_Result MV_DRM_OTP_InvokeCommandEntryPoint(uint32_t commandID,
                                            TEE_Param params[4]);

#endif /* __FACTORY_OTP_H__ */
