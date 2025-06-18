/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _BDR_FTDC_CFG_
#define _BDR_FTDC_CFG_

#include "vpp_cmu_ftdc.h"

extern const UCHAR  gCmuFtdcInput;
extern const UCHAR  gCmuFtdcMode;

/*
 * Flesh Tone detection Ranges
 */
/*
    CmuFtdc---[0] = SD
    CmuFtdc---[1] = HD */
extern  const UINT16 gCmuFtdcInLow[2];
extern  const UINT16 gCmuFtdcOutLow[2];
extern  const UINT16 gCmuFtdcInHigh[2];
extern  const UINT16 gCmuFtdcOutHigh[2];
extern  const UCHAR  gCmuFtdcYthLow[2];
extern  const UCHAR  gCmuFtdcYthHigh[2];
extern  const UINT16 gCmuFtdcOutChromaLow[2];
extern  const UINT16 gCmuFtdcInChromaLow[2];
extern  const UINT16 gCmuFtdcOutChromaHigh[2];
extern  const UINT16 gCmuFtdcInChromaHigh[2];
extern  const UINT16 gCmuFtdcHueWidth[2];
extern  const UINT16 gCmuFtdcChromaWidth[2];

/*
 * Flesh Tone correction Coefficents and Offsets
 */
extern const UCHAR gCmuFtdcCoeffRegion[4][2][4];
extern const UCHAR gCmuFtdcOffsetRegion[4][2][2];

#endif //_BDR_FTDC_CFG_
