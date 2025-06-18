/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _BDR_ICSC_CFG_
#define _BDR_ICSC_CFG_

#include "vpp_csc_icsc.h"

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */

#define VPP_CSC_MAX_ICSC_MODES 21
#define VPP_CSC_MAX_ICSC_COEFF_ROWS 3
#define VPP_CSC_MAX_ICSC_COEFF_COLMS 3
#define VPP_CSC_MAX_ICSC_OFF 3

/*-----------------------------------------------------------------------------
 * External Variables
 *-----------------------------------------------------------------------------
 */
extern UINT32 gICscWindowCoeff[VPP_CSC_MAX_ICSC_MODES][VPP_CSC_MAX_ICSC_COEFF_ROWS]
    [VPP_CSC_MAX_ICSC_COEFF_COLMS];
extern UINT32 gICscWindowOffset_12bits[VPP_CSC_MAX_ICSC_MODES][VPP_CSC_MAX_ICSC_OFF];
extern UINT32 gICscWindowOffset_10bits[VPP_CSC_MAX_ICSC_MODES][VPP_CSC_MAX_ICSC_OFF];
extern UINT32 gICscWindowOffset_8bits[VPP_CSC_MAX_ICSC_MODES][VPP_CSC_MAX_ICSC_OFF];

#endif //_BDR_ICSC_CFG_
