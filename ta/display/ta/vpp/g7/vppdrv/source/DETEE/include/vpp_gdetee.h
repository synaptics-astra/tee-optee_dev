/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

/*
 * vpp_gdetee.h
 *
 *  Created on: Jan 24, 2019
 *      Author: jjose
 */

#ifndef _VPP_DETEE_H_
#define _VPP_DETEE_H_

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"

/*FE detail EE register structure*/
typedef struct VPP_FE_DET_T
{
    UINT32 VPP_DET_CTRL;
    UINT32 VPP_DET_NOISE1;
    UINT32 VPP_DET_NOISE2;
    UINT32 VPP_DET_GAIN1;
    UINT32 VPP_DET_GAIN2;
    UINT32 VPP_DET_MODE;
    UINT32 VPP_DET_THBASE;
    UINT32 VPP_DET_THSTEP;
    UINT32 VPP_DET_LUT0;
    UINT32 VPP_DET_LUT1;
    UINT32 VPP_DET_LUT2;
    UINT32 VPP_DET_LUT3;
    UINT32 VPP_DET_LUT4;
    UINT32 VPP_DET_LUT5;
    UINT32 VPP_DET_LUT6;
    UINT32 VPP_DET_LUT7;
    UINT32 VPP_DET_LUT8;
    UINT32 VPP_DET_LUT9;
    UINT32 VPP_DET_LUTA;
    UINT32 VPP_DET_LUTB;
    UINT32 VPP_DET_LUTC;
    UINT32 VPP_DET_LUTD;
    UINT32 VPP_DET_LUTE;
    UINT32 VPP_DET_LUTF;
    UINT32 VPP_DET_LPFCTRL;
    UINT32 VPP_DET_LPFHS;
    UINT32 VPP_DET_LPFVS;
} VPP_FE_DET, *PVPP_FE_DET;

/*FE Guide detail EE register structure*/
typedef struct VPP_FE_GDET_T
{
    UINT32 VPP_GDET_SWITCH;
    UINT32 VPP_GDET_GDET_EN;
    UINT32 VPP_GDET_GDET_ADD;
    UINT32 VPP_GDET_UPS_EN;
    UINT32 VPP_GDET_WIN_R;
    UINT32 VPP_GDET_EPS;
    UINT32 VPP_GDET_SCALE;
    UINT32 VPP_GDET_MID;
    UINT32 VPP_GDET_SIGMA_S1;
    UINT32 VPP_GDET_SIGMA_S2;
    UINT32 VPP_GDET_SCALE_SKIN;
    UINT32 VPP_GDET_MID_SKIN;
    UINT32 VPP_GDET_SIGMA_S1_SKIN;
    UINT32 VPP_GDET_SIGMA_S2_SKIN;
    UINT32 VPP_GDET_NOISE_NUM;
    UINT32 VPP_GDET_NOISE_OFFSET;
    UINT32 VPP_GDET_NOISE_CAP;
    UINT32 VPP_GDET_NOISE_NUM_SKIN;
    UINT32 VPP_GDET_NOISE_OFFSET_SKIN;
    UINT32 VPP_GDET_NOISE_CAP_SKIN;
    UINT32 VPP_GDET_SKIN_COS2_H_LOW;
    UINT32 VPP_GDET_SKIN_COS2_H_HIGH;
    UINT32 VPP_GDET_SKIN_S_LOW;
    UINT32 VPP_GDET_SKIN_S_HIGH;
    UINT32 VPP_GDET_SKIN_V_LOW;
    UINT32 VPP_GDET_SKIN_V_HIGH;
    UINT32 VPP_GDET_SKIN_Y;
    UINT32 VPP_GDET_SKIN_RMV_TH;
    UINT32 VPP_GDET_SKIN_SET_TH;
    UINT32 VPP_GDET_SKIN_DEBUG;
    UINT32 VPP_GDET_SKIN_DEBUG_Y;
    UINT32 VPP_GDET_SKIN_DEBUG_C;
    UINT32 VPP_GDET_NOISE_ADJ_S;
    UINT32 VPP_GDET_NOISE_ACT_CAP;
} VPP_FE_GDET, *PVPP_FE_GDET;

/*data loader input resolution*/
typedef struct VPP_DETEE_INPUT_RES_T
{
    UINT32 HRes;
    UINT32 VRes;

} VPP_DETEE_INPUT_RES, *PVPP_DETEE_INPUT_RES;

/*data loader crop*/
typedef struct VPP_DETEE_INPUT_CROP_T
{
    UINT32 leftCrop;
    UINT32 rightCrop;
    UINT32 topCrop;
    UINT32 bottomCrop;
} VPP_DETEE_INPUT_CROP, *PVPP_DETEE_INPUT_CROP;

/****************************************************************************************
*macro for DETTE dummy TG, only set offset plus input res dynamically, others are set in loading defult values*
*****************************************************************************************/
/*the offset plus with input resolution to set dummy TG size*/

#define VPP_DETEE_DUMMY_TG_SIZE_H_OFF_P 60
#define VPP_DETEE_DUMMY_TG_SIZE_V_OFF_P 6
#define VPP_DETEE_DUMMY_TG_SIZE_H_OFF_I 26
#define VPP_DETEE_DUMMY_TG_SIZE_V_OFF_I 10

/* IHRES_OFF : Needed for the max hcrop of 63 pixels*/
#define VPP_DETEE_DUMMY_TG_IHRES_OFF 46
/* IHERES OFF : For PIP only */
#define VPP_DETEE_DUMMY_TG_IHRES_OFF_PIP 27

/*dummy TG HSync*/
#define VPP_DETEE_DUMMY_TG_HS_DETEE 1
#define VPP_DETEE_DUMMY_TG_HS_BE 3

/*dummy TG HBlank*/
#define VPP_DETEE_DUMMY_TG_HB_DETEE_OFF 8 //VPP_DETEE_DUMMY_TG_HB_BE + 1
#define VPP_DETEE_DUMMY_TG_HB_BE     7 //VPP_DETEE_DUMMY_TG_HS_BE + 4

/*dummy TG VSync0*/
#define VPP_DETEE_DUMMY_TG_VS0_DETEE 1
#define VPP_DETEE_DUMMY_TG_VS0_BE 2

/*dummy TG VBlank*/
#define VPP_DETEE_DUMMY_TG_VB0_DETEE_OFF 3//VPP_DETEE_DUMMY_TG_VS0_BE + 1
#define VPP_DETEE_DUMMY_TG_VB0_BE     2 //VPP_DETEE_DUMMY_TG_VS0_BE

#define VPP_DETEE_DLR_OSD_CLUT_NUM_MAX 256

INT VPP_DETEE_ConfigTG(VPP_OBJ *vpp_obj, int width, int height);

INT VPP_DETEE_TG_Start(VPP_OBJ *vpp_obj);

INT VPP_DETEE_TG_Clear(VPP_OBJ *vpp_obj);

INT VPP_DETEE_reset(VPP_OBJ *vpp_obj);

INT VPP_DETEE_SetGDetParamsMode(VPP_OBJ *vpp_obj, INT mode);

INT VPP_DETEE_SetGDetCtrl(VPP_OBJ *vpp_obj, PVPP_DETEE_GDET_CTRL pGDetCtrl);

INT VPP_DETEE_SetGDetParams(VPP_OBJ *vpp_obj, PVPP_DETEE_GDET_PARAMS pEEGDetParam);

INT DTEE_LoadDefaultVal(VPP_OBJ *vpp_obj);

INT VPP_DETEE_Control(VPP_OBJ *vpp_obj,int enable);

INT DETEE_SetGDetCtrl(VPP_OBJ *vpp_obj, PVPP_DETEE_GDET_CTRL pGDetCtrl);

INT DETEE_SetGDetParams(VPP_OBJ *vpp_obj, PVPP_DETEE_GDET_PARAMS pEEGDetParam);

INT DETEE_SetGDetParamsMode(VPP_OBJ *vpp_obj, INT mode);

INT DETEE_Control(VPP_OBJ *vpp_obj,int enable);

INT DETEE_ConfigTG(VPP_OBJ *vpp_obj, int width, int height);

INT DETEE_TG_Start(VPP_OBJ *vpp_obj);

INT DETEE_TG_Clear(VPP_OBJ *vpp_obj);

INT DETEE_Control(VPP_OBJ *vpp_obj,int enable);

#endif /* _VPP_DETEE_H_ */
