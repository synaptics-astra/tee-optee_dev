/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
/*
 * vpp_be_vop2.h
 *
 *  Created on: Jan 28, 2019
 *      Author: jjose
 */

#ifndef _VPP_BE_VOP2_H_
#define _VPP_BE_VOP2_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_gfx_vop.h"
#include "vpp_cpcb_common.h"

/**********************************************************************
*Data structure definitions for VOP2 TG block driver                                                *
***********************************************************************/

/****************************************************************************************
*macro for DETTE dummy TG, only set offset plus input res dynamically, others are set in loading defult values*
*****************************************************************************************/
/*the offset plus with input resolution to set dummy TG size*/

#define VPP_VOP2_DUMMY_TG_SIZE_H_OFF_P 20
#define VPP_VOP2_DUMMY_TG_SIZE_V_OFF_P 6
#define VPP_VOP2_DUMMY_TG_SIZE_H_OFF_I 26
#define VPP_VOP2_DUMMY_TG_SIZE_V_OFF_I 10

/* IHRES_OFF : Needed for the max hcrop of 63 pixels*/
#define VPP_VOP2_DUMMY_TG_IHRES_OFF 46
/* IHERES OFF : For PIP only */
#define VPP_VOP2_DUMMY_TG_IHRES_OFF_PIP 27


/*dummy TG HSync*/
#define VPP_VOP2_DUMMY_TG_HS_VOP2 1
#define VPP_VOP2_DUMMY_TG_HS_BE 3

/*dummy TG HBlank*/
#define VPP_VOP2_DUMMY_TG_HB_VOP2_OFF 8 //VPP_VOP2_DUMMY_TG_HB_BE + 1
#define VPP_VOP2_DUMMY_TG_HB_BE     7 //VPP_VOP2_DUMMY_TG_HS_BE + 4

/*dummy TG VSync0*/
#define VPP_VOP2_DUMMY_TG_VS0_VOP2 1
#define VPP_VOP2_DUMMY_TG_VS0_BE 2

/*dummy TG VBlank*/
#define VPP_VOP2_DUMMY_TG_VB0_VOP2_OFF 3//VPP_VOP2_DUMMY_TG_VS0_BE + 1
#define VPP_VOP2_DUMMY_TG_VB0_BE     2 //VPP_VOP2_DUMMY_TG_VS0_BE

#define VPP_VOP2_DLR_OSD_CLUT_NUM_MAX 256

#endif /* _VPP_BE_VOP2_H_ */
