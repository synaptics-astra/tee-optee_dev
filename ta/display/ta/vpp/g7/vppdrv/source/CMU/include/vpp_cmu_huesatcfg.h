/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRHUESATCFG__
#define __BDRHUESATCFG__

#include "vpp_cmu_huesat.h"

extern const UCHAR  gCmuHueSatInput;
extern const UCHAR  gCmuHueSatIcrMode;

extern const UINT16 gHueSatOff[2][3];

extern const UINT16 gHueSatMaxSat[2][7];

extern const UINT16 gHSMaxSatXYZ[2][3];

extern const UINT16 gHSMaxSatKXYZ[2][3];

extern const UINT16 gHueSatConvCoeff[2][3][3];

extern const INT16  gCmuSaturation;

extern const UCHAR  gCmuModeTable[2][6][3][14];

#endif //__BDRHUESATCFG__
