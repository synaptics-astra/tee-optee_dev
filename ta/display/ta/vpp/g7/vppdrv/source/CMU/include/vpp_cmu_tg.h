/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

/*
 * vpp_cmu_tg.h
 *
 *  Created on: Jan 23, 2019
 *      Author: jjose
 */

#ifndef _VPP_CMU_TG_H_
#define _VPP_CMU_TG_H_

#include "com_type.h"
#include "vpp_module.h"

/*data loader input resolution*/
typedef struct VPP_CMU_INPUT_RES_T
{
    UINT32 HRes;
    UINT32 VRes;

}VPP_CMU_INPUT_RES, *PVPP_CMU_INPUT_RES;

/*data loader crop*/
typedef struct VPP_CMU_INPUT_CROP_T
{
    UINT32 leftCrop;
    UINT32 rightCrop;
    UINT32 topCrop;
    UINT32 bottomCrop;
}VPP_CMU_INPUT_CROP, *PVPP_CMU_INPUT_CROP;

INT CMU_TG_Start(VPP_OBJ *vpp_obj);
INT CMU_Enable(VPP_OBJ *vpp_obj,INT enable);
INT CMU_SetDummyTG(VPP_OBJ *vpp_obj, INT ldr_id,
        PVPP_CMU_INPUT_RES pTotRes,
        PVPP_CMU_INPUT_CROP pCropRes);
INT CMU_ConfigTG(VPP_OBJ *vpp_obj, int width, int height);
INT CMU_TG_Start(VPP_OBJ *vpp_obj);
INT CMU_TG_Clear(VPP_OBJ *vpp_obj);

#endif /* _VPP_CMU_TG_H_ */
