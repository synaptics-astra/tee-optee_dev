/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef _VPP_FE_DUMMYTG_H_
#define _VPP_FE_DUMMYTG_H_

#include "vpp.h"
#include "com_type.h"


/****************************************************************************************
*macro for FE dummy TG, only set offset plus input res dynamically, others are set in loading defult values*
*****************************************************************************************/
/*the offset plus with input resolution to set dummy TG size*/
#ifdef CONFIG_TILE_MODE_ROTATION
#define VPP_FE_DUMMY_TG_SIZE_H_OFF_P 60
#else
#define VPP_FE_DUMMY_TG_SIZE_H_OFF_P 20
#endif
#define VPP_FE_DUMMY_TG_SIZE_V_OFF_P 6

#define VPP_FE_DUMMY_TG_SIZE_H_OFF_I 26
#define VPP_FE_DUMMY_TG_SIZE_V_OFF_I 10

/* IHRES_OFF : Needed for the max hcrop of 63 pixels*/
#define VPP_FE_DUMMY_TG_IHRES_OFF 46
/* IHERES OFF : For PIP only */
#define VPP_FE_DUMMY_TG_IHRES_OFF_PIP 27


/*dummy TG HSync*/
#define VPP_FE_DUMMY_TG_HS_FE 1
#define VPP_FE_DUMMY_TG_HS_BE 3

/*dummy TG HBlank*/
#define VPP_FE_DUMMY_TG_HB_FE_OFF 8 //VPP_FE_DUMMY_TG_HB_BE + 1
#define VPP_FE_DUMMY_TG_HB_BE     7 //VPP_FE_DUMMY_TG_HS_BE + 4

/*dummy TG VSync0*/
#define VPP_FE_DUMMY_TG_VS0_FE 1
#define VPP_FE_DUMMY_TG_VS0_BE 2

/*dummy TG VBlank*/
#define VPP_FE_DUMMY_TG_VB0_FE_OFF 3//VPP_FE_DUMMY_TG_VS0_BE + 1
#define VPP_FE_DUMMY_TG_VB0_BE     2 //VPP_FE_DUMMY_TG_VS0_BE
#endif
