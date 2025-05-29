/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

/*
 * vpp_gdetee.c
 *
 *  Created on: Jan 24, 2019
 *      Author: jjose
 */

#include "vpp_gdetee.h"
#include "vpp_cmu_tg.h"

static INT DETEE_SetDummyTG(VPP_OBJ *vpp_obj, INT ldr_id,
                PVPP_DETEE_INPUT_RES pTotRes,
                PVPP_DETEE_INPUT_CROP pCropRes);

/*FE Guide detail register default values*/
static const VPP_CPCB_GDET_CTRL gCpcbGDetCtrlDefVal = {
    .gdet_switch = 0,
    .gdet_en     = 0,
    .gdet_add    = 0,
    .ups_en      = 1,

};

static const VPP_CPCB_GDET_PARAMS gCpcbGDetParamsDefVal = {
    /* Control values */
    .win_r             = 0x2,
    .eps               = 0x28E4,
    .scale             = 0x00,
    .mid               = 0xC8,
    .sigma_s1          = 0xA,
    .sigma_s2          = 0xA,

    /* Noise params */
    .noise_num      = 0x200,
    .noise_offset   = 0x0,
    .noise_cap      = 0x14,
    .noise_adj_s    = 0x8,
    .noise_act_cap  = 0x64,

    /* Skin prams */
    .scale_skin         = 0x00,
    .mid_skin           = 0xC8,
    .sigma_s1_skin      = 0xA,
    .sigma_s2_skin      = 0xA,
    .noise_num_skin     = 0x200,
    .noise_offset_skin  = 0x0,
    .noise_cap_skin     = 0x14,
    .skin_cos2_h_low    = 0x1A7,
    .skin_cos2_h_high   = 0x400,
    .skin_s_low         = 0xCD,
    .skin_s_high        = 0x2B8,
    .skin_v_low         = 0x59,
    .skin_v_high        = 0xFF,
    .skin_y             = 0x2,
    .skin_rmv_th        = 0x2,
    .skin_set_th        = 0x3,

};
/*FE Guide detail EE register default values*/
static const  VPP_FE_GDET gFeGDetDefValues[9] = {
    { /* default */
        .VPP_GDET_SWITCH            = 0x0,
        .VPP_GDET_GDET_EN           = 0x1,
        .VPP_GDET_GDET_ADD          = 0x1,
        .VPP_GDET_UPS_EN            = 0x1,
        .VPP_GDET_WIN_R             = 0x2,
        .VPP_GDET_EPS               = 0x28E4,
        .VPP_GDET_SCALE             = 0x30,
        .VPP_GDET_MID               = 0xC8,
        .VPP_GDET_SIGMA_S1          = 0xA,
        .VPP_GDET_SIGMA_S2          = 0xA,
        .VPP_GDET_SCALE_SKIN        = 0x30,
        .VPP_GDET_MID_SKIN          = 0xC8,
        .VPP_GDET_SIGMA_S1_SKIN     = 0xA,
        .VPP_GDET_SIGMA_S2_SKIN     = 0xA,
        .VPP_GDET_NOISE_NUM         = 0x200,
        .VPP_GDET_NOISE_OFFSET      = 0x0,
        .VPP_GDET_NOISE_CAP         = 0x14,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x200,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0x0,
        .VPP_GDET_NOISE_CAP_SKIN    = 0x14,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x1A7,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x400,
        .VPP_GDET_SKIN_S_LOW        = 0xCD,
        .VPP_GDET_SKIN_S_HIGH       = 0x2B8,
        .VPP_GDET_SKIN_V_LOW        = 0x59,
        .VPP_GDET_SKIN_V_HIGH       = 0xFF,
        .VPP_GDET_SKIN_Y            = 0x2,
        .VPP_GDET_SKIN_RMV_TH       = 0x2,
        .VPP_GDET_SKIN_SET_TH       = 0x3,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0xFF,
        .VPP_GDET_SKIN_DEBUG_C      = 0xFF,
        .VPP_GDET_NOISE_ADJ_S       = 0x8,
        .VPP_GDET_NOISE_ACT_CAP     = 0x64,
    },
    { /* DV*/
        .VPP_GDET_SWITCH            = 0x0,
        .VPP_GDET_GDET_EN           = 0x1,
        .VPP_GDET_GDET_ADD          = 0x1,
        .VPP_GDET_UPS_EN            = 0x0,
        .VPP_GDET_WIN_R             = 0x2,
        .VPP_GDET_EPS               = 0xffffff,
        .VPP_GDET_SCALE             = 0xfff,
        .VPP_GDET_MID               = 0x3ff,
        .VPP_GDET_SIGMA_S1          = 0xf,
        .VPP_GDET_SIGMA_S2          = 0xf,
        .VPP_GDET_SCALE_SKIN        = 0xfff,
        .VPP_GDET_MID_SKIN          = 0x3ff,
        .VPP_GDET_SIGMA_S1_SKIN     = 0xf,
        .VPP_GDET_SIGMA_S2_SKIN     = 0xf,
        .VPP_GDET_NOISE_NUM         = 0x7fff,
        .VPP_GDET_NOISE_OFFSET      = 0xff,
        .VPP_GDET_NOISE_CAP         = 0xff,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x7fff,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0xff,
        .VPP_GDET_NOISE_CAP_SKIN    = 0xff,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x3ff,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x400,
        .VPP_GDET_SKIN_S_LOW        = 0x3ff,
        .VPP_GDET_SKIN_S_HIGH       = 0x3ff,
        .VPP_GDET_SKIN_V_LOW        = 0x3ff,
        .VPP_GDET_SKIN_V_HIGH       = 0x3ff,
        .VPP_GDET_SKIN_Y            = 0x3ff,
        .VPP_GDET_SKIN_RMV_TH       = 0xf,
        .VPP_GDET_SKIN_SET_TH       = 0xf,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0xfff,
        .VPP_GDET_SKIN_DEBUG_C      = 0xfff,
        .VPP_GDET_NOISE_ADJ_S       = 0xe,
        .VPP_GDET_NOISE_ACT_CAP     = 0xff,
    },
    {/* hard bypass */
        .VPP_GDET_SWITCH            = 0x1,
        .VPP_GDET_GDET_EN           = 0x0,
        .VPP_GDET_GDET_ADD          = 0x0,
        .VPP_GDET_UPS_EN            = 0x1,
        .VPP_GDET_WIN_R             = 0x1,
        .VPP_GDET_EPS               = 0x0,
        .VPP_GDET_SCALE             = 0x0,
        .VPP_GDET_MID               = 0x0,
        .VPP_GDET_SIGMA_S1          = 0x0,
        .VPP_GDET_SIGMA_S2          = 0x0,
        .VPP_GDET_SCALE_SKIN        = 0x0,
        .VPP_GDET_MID_SKIN          = 0x0,
        .VPP_GDET_SIGMA_S1_SKIN     = 0x0,
        .VPP_GDET_SIGMA_S2_SKIN     = 0x0,
        .VPP_GDET_NOISE_NUM         = 0x0,
        .VPP_GDET_NOISE_OFFSET      = 0x0,
        .VPP_GDET_NOISE_CAP         = 0x0,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x0,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0x0,
        .VPP_GDET_NOISE_CAP_SKIN    = 0x14,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x0,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x0,
        .VPP_GDET_SKIN_S_LOW        = 0x0,
        .VPP_GDET_SKIN_S_HIGH       = 0x0,
        .VPP_GDET_SKIN_V_LOW        = 0x0,
        .VPP_GDET_SKIN_V_HIGH       = 0x0,
        .VPP_GDET_SKIN_Y            = 0x0,
        .VPP_GDET_SKIN_RMV_TH       = 0x0,
        .VPP_GDET_SKIN_SET_TH       = 0x0,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0x0,
        .VPP_GDET_SKIN_DEBUG_C      = 0x0,
        .VPP_GDET_NOISE_ADJ_S       = 0x0,
        .VPP_GDET_NOISE_ACT_CAP     = 0x0,
    },
    {/* Yout=Yin, DYdet=128 (softbypass) */
        .VPP_GDET_SWITCH            = 0x0,
        .VPP_GDET_GDET_EN           = 0x0,
        .VPP_GDET_GDET_ADD          = 0x0,
        .VPP_GDET_UPS_EN            = 0x1,
        .VPP_GDET_WIN_R             = 0x1,
        .VPP_GDET_EPS               = 0x1,
        .VPP_GDET_SCALE             = 0x0,
        .VPP_GDET_MID               = 0x0,
        .VPP_GDET_SIGMA_S1          = 0x0,
        .VPP_GDET_SIGMA_S2          = 0x0,
        .VPP_GDET_SCALE_SKIN        = 0x0,
        .VPP_GDET_MID_SKIN          = 0x0,
        .VPP_GDET_SIGMA_S1_SKIN     = 0x0,
        .VPP_GDET_SIGMA_S2_SKIN     = 0x0,
        .VPP_GDET_NOISE_NUM         = 0x0,
        .VPP_GDET_NOISE_OFFSET      = 0x0,
        .VPP_GDET_NOISE_CAP         = 0x0,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x0,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0x0,
        .VPP_GDET_NOISE_CAP_SKIN    = 0x14,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x0,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x0,
        .VPP_GDET_SKIN_S_LOW        = 0x0,
        .VPP_GDET_SKIN_S_HIGH       = 0x0,
        .VPP_GDET_SKIN_V_LOW        = 0x0,
        .VPP_GDET_SKIN_V_HIGH       = 0x0,
        .VPP_GDET_SKIN_Y            = 0x0,
        .VPP_GDET_SKIN_RMV_TH       = 0x0,
        .VPP_GDET_SKIN_SET_TH       = 0x0,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0x0,
        .VPP_GDET_SKIN_DEBUG_C      = 0x0,
        .VPP_GDET_NOISE_ADJ_S       = 0x0,
        .VPP_GDET_NOISE_ACT_CAP     = 0x0,
    },
    {/* Gdet enabled, but EE(LTI/CTI) disabled. Yout = Yin + Dydet. No skin enh */
        .VPP_GDET_SWITCH            = 0x0,
        .VPP_GDET_GDET_EN           = 0x0,
        .VPP_GDET_GDET_ADD          = 0x1,
        .VPP_GDET_UPS_EN            = 0x1,
        .VPP_GDET_WIN_R             = 0x2,
        .VPP_GDET_EPS               = 0x28E4,
        .VPP_GDET_SCALE             = 0x30,
        .VPP_GDET_MID               = 0xC8,
        .VPP_GDET_SIGMA_S1          = 0x8,
        .VPP_GDET_SIGMA_S2          = 0x8,
        .VPP_GDET_SCALE_SKIN        = 0x0,
        .VPP_GDET_MID_SKIN          = 0x0,
        .VPP_GDET_SIGMA_S1_SKIN     = 0x0,
        .VPP_GDET_SIGMA_S2_SKIN     = 0x0,
        .VPP_GDET_NOISE_NUM         = 0x200,
        .VPP_GDET_NOISE_OFFSET      = 0x0,
        .VPP_GDET_NOISE_CAP         = 0x5,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x0,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0x0,
        .VPP_GDET_NOISE_CAP_SKIN    = 0x14,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x1A7,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x400,
        .VPP_GDET_SKIN_S_LOW        = 0xB4,
        .VPP_GDET_SKIN_S_HIGH       = 0x2B8,
        .VPP_GDET_SKIN_V_LOW        = 0x59,
        .VPP_GDET_SKIN_V_HIGH       = 0xFF,
        .VPP_GDET_SKIN_Y            = 0x2,
        .VPP_GDET_SKIN_RMV_TH       = 0x2,
        .VPP_GDET_SKIN_SET_TH       = 0x3,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0xFF,
        .VPP_GDET_SKIN_DEBUG_C      = 0xFF,
        .VPP_GDET_NOISE_ADJ_S       = 0x8,
        .VPP_GDET_NOISE_ACT_CAP     = 0x40,
    },
    {/* Gdet enabled, but EE(LTI/CTI) disabled. Yout = Yin + Dydet.  */
        .VPP_GDET_SWITCH            = 0x0,
        .VPP_GDET_GDET_EN           = 0x0,
        .VPP_GDET_GDET_ADD          = 0x1,
        .VPP_GDET_UPS_EN            = 0x1,
        .VPP_GDET_WIN_R             = 0x2,
        .VPP_GDET_EPS               = 0x28E4,
        .VPP_GDET_SCALE             = 0x30,
        .VPP_GDET_MID               = 0xC8,
        .VPP_GDET_SIGMA_S1          = 0x8,
        .VPP_GDET_SIGMA_S2          = 0x8,
        .VPP_GDET_SCALE_SKIN        = 0x30,
        .VPP_GDET_MID_SKIN          = 0x32,
        .VPP_GDET_SIGMA_S1_SKIN     = 0x8,
        .VPP_GDET_SIGMA_S2_SKIN     = 0x8,
        .VPP_GDET_NOISE_NUM         = 0x200,
        .VPP_GDET_NOISE_OFFSET      = 0x0,
        .VPP_GDET_NOISE_CAP         = 0x5,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x200,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0x0,
        .VPP_GDET_NOISE_CAP_SKIN    = 0x14,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x1A7,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x400,
        .VPP_GDET_SKIN_S_LOW        = 0xB4,
        .VPP_GDET_SKIN_S_HIGH       = 0x2B8,
        .VPP_GDET_SKIN_V_LOW        = 0x59,
        .VPP_GDET_SKIN_V_HIGH       = 0xFF,
        .VPP_GDET_SKIN_Y            = 0x2,
        .VPP_GDET_SKIN_RMV_TH       = 0x2,
        .VPP_GDET_SKIN_SET_TH       = 0x3,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0xFF,
        .VPP_GDET_SKIN_DEBUG_C      = 0xFF,
        .VPP_GDET_NOISE_ADJ_S       = 0x8,
        .VPP_GDET_NOISE_ACT_CAP     = 0x40,
    },
    {/* Gdet enabled, EE(LTI/CTI) enabled. Yout = Yin. No skin enh */
        .VPP_GDET_SWITCH            = 0x0,
        .VPP_GDET_GDET_EN           = 0x1,
        .VPP_GDET_GDET_ADD          = 0x0,
        .VPP_GDET_UPS_EN            = 0x1,
        .VPP_GDET_WIN_R             = 0x2,
        .VPP_GDET_EPS               = 0x28E4,
        .VPP_GDET_SCALE             = 0x30,
        .VPP_GDET_MID               = 0xC8,
        .VPP_GDET_SIGMA_S1          = 0x8,
        .VPP_GDET_SIGMA_S2          = 0x8,
        .VPP_GDET_SCALE_SKIN        = 0x0,
        .VPP_GDET_MID_SKIN          = 0x0,
        .VPP_GDET_SIGMA_S1_SKIN     = 0x0,
        .VPP_GDET_SIGMA_S2_SKIN     = 0x0,
        .VPP_GDET_NOISE_NUM         = 0x200,
        .VPP_GDET_NOISE_OFFSET      = 0x0,
        .VPP_GDET_NOISE_CAP         = 0x5,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x0,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0x0,
        .VPP_GDET_NOISE_CAP_SKIN    = 0x14,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x1A7,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x400,
        .VPP_GDET_SKIN_S_LOW        = 0xB4,
        .VPP_GDET_SKIN_S_HIGH       = 0x2B8,
        .VPP_GDET_SKIN_V_LOW        = 0x59,
        .VPP_GDET_SKIN_V_HIGH       = 0xFF,
        .VPP_GDET_SKIN_Y            = 0x2,
        .VPP_GDET_SKIN_RMV_TH       = 0x2,
        .VPP_GDET_SKIN_SET_TH       = 0x3,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0xFF,
        .VPP_GDET_SKIN_DEBUG_C      = 0xFF,
        .VPP_GDET_NOISE_ADJ_S       = 0x8,
        .VPP_GDET_NOISE_ACT_CAP     = 0x40,
    },
    {/* Gdet enabled, EE(LTI/CTI) enabled. Yout = Yin. */
        .VPP_GDET_SWITCH            = 0x0,
        .VPP_GDET_GDET_EN           = 0x1,
        .VPP_GDET_GDET_ADD          = 0x0,
        .VPP_GDET_UPS_EN            = 0x1,
        .VPP_GDET_WIN_R             = 0x2,
        .VPP_GDET_EPS               = 0x28E4,
        .VPP_GDET_SCALE             = 0x30,
        .VPP_GDET_MID               = 0xC8,
        .VPP_GDET_SIGMA_S1          = 0x8,
        .VPP_GDET_SIGMA_S2          = 0x8,
        .VPP_GDET_SCALE_SKIN        = 0x30,
        .VPP_GDET_MID_SKIN          = 0x32,
        .VPP_GDET_SIGMA_S1_SKIN     = 0x8,
        .VPP_GDET_SIGMA_S2_SKIN     = 0x8,
        .VPP_GDET_NOISE_NUM         = 0x200,
        .VPP_GDET_NOISE_OFFSET      = 0x0,
        .VPP_GDET_NOISE_CAP         = 0x5,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x200,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0x0,
        .VPP_GDET_NOISE_CAP_SKIN    = 0x14,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x1A7,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x400,
        .VPP_GDET_SKIN_S_LOW        = 0xB4,
        .VPP_GDET_SKIN_S_HIGH       = 0x2B8,
        .VPP_GDET_SKIN_V_LOW        = 0x59,
        .VPP_GDET_SKIN_V_HIGH       = 0xFF,
        .VPP_GDET_SKIN_Y            = 0x2,
        .VPP_GDET_SKIN_RMV_TH       = 0x2,
        .VPP_GDET_SKIN_SET_TH       = 0x3,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0xFF,
        .VPP_GDET_SKIN_DEBUG_C      = 0xFF,
        .VPP_GDET_NOISE_ADJ_S       = 0x8,
        .VPP_GDET_NOISE_ACT_CAP     = 0x40,
    },
    {/* From C-algo developer */
        .VPP_GDET_SWITCH            = 0x0,
        .VPP_GDET_GDET_EN           = 0x1,
        .VPP_GDET_GDET_ADD          = 0x1,
        .VPP_GDET_UPS_EN            = 0x1,
        .VPP_GDET_WIN_R             = 0x1,
        .VPP_GDET_EPS               = 0x28E4,
        .VPP_GDET_SCALE             = 0x30,
        .VPP_GDET_MID               = 0xC8,
        .VPP_GDET_SIGMA_S1          = 0x8,
        .VPP_GDET_SIGMA_S2          = 0x8,
        .VPP_GDET_SCALE_SKIN        = 0x30,
        .VPP_GDET_MID_SKIN          = 0x32,
        .VPP_GDET_SIGMA_S1_SKIN     = 0x8,
        .VPP_GDET_SIGMA_S2_SKIN     = 0x8,
        .VPP_GDET_NOISE_NUM         = 0x200,
        .VPP_GDET_NOISE_OFFSET      = 0x0,
        .VPP_GDET_NOISE_CAP         = 0x5,
        .VPP_GDET_NOISE_NUM_SKIN    = 0x200,
        .VPP_GDET_NOISE_OFFSET_SKIN = 0x0,
        .VPP_GDET_NOISE_CAP_SKIN    = 0x14,
        .VPP_GDET_SKIN_COS2_H_LOW   = 0x1A7,
        .VPP_GDET_SKIN_COS2_H_HIGH  = 0x400,
        .VPP_GDET_SKIN_S_LOW        = 0xB4,
        .VPP_GDET_SKIN_S_HIGH       = 0x2B8,
        .VPP_GDET_SKIN_V_LOW        = 0x59,
        .VPP_GDET_SKIN_V_HIGH       = 0xFF,
        .VPP_GDET_SKIN_Y            = 0x2,
        .VPP_GDET_SKIN_RMV_TH       = 0x2,
        .VPP_GDET_SKIN_SET_TH       = 0x3,
        .VPP_GDET_SKIN_DEBUG        = 0x0,
        .VPP_GDET_SKIN_DEBUG_Y      = 0xFF,
        .VPP_GDET_SKIN_DEBUG_C      = 0xFF,
        .VPP_GDET_NOISE_ADJ_S       = 0x8,
        .VPP_GDET_NOISE_ACT_CAP     = 0x40,
    },
};


/***************************************************************************************
 * FUNCTION: Set the register values of Guide detail enable/disable control params
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CPCB GDet values to BCM buffer
 ***************************************************************************************/
INT DETEE_SetGDetCtrl(VPP_OBJ *vpp_obj, PVPP_DETEE_GDET_CTRL pGDetCtrl)
{
    INT RetVal = MV_VPP_OK;
    T32GDET_CFG0 stGDetCfg0;

    stGDetCfg0.uCFG0_win_r             = vpp_obj->detee.DETEEGDetParam.win_r;
    stGDetCfg0.uCFG0_eps               = vpp_obj->detee.DETEEGDetParam.eps;

    stGDetCfg0.uCFG0_switch            = pGDetCtrl->gdet_switch;
    stGDetCfg0.uCFG0_gdet_en           = pGDetCtrl->gdet_en;
    stGDetCfg0.uCFG0_gdet_add          = pGDetCtrl->gdet_add;
    stGDetCfg0.uCFG0_ups_en            = pGDetCtrl->ups_en;

    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG0, stGDetCfg0.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    vpp_obj->detee.DETEEGDetCtrl.gdet_switch = stGDetCfg0.uCFG0_switch;
    vpp_obj->detee.DETEEGDetCtrl.gdet_en     = stGDetCfg0.uCFG0_gdet_en;
    vpp_obj->detee.DETEEGDetCtrl.gdet_add    = stGDetCfg0.uCFG0_gdet_add;
    vpp_obj->detee.DETEEGDetCtrl.ups_en      = stGDetCfg0.uCFG0_ups_en;

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: Set the register values of Guide detail parameter
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load DEEE GDet values to BCM buffer
 ***************************************************************************************/
INT DETEE_SetGDetParams(VPP_OBJ *vpp_obj, PVPP_DETEE_GDET_PARAMS pEEGDetParam)
{
    INT RetVal = MV_VPP_OK;
    T32GDET_CFG0 stGDetCfg0;
    T32GDET_CFG1 stGDetCfg1;
    T32GDET_CFG2 stGDetCfg2;
    T32GDET_CFG3 stGDetCfg3;
    T32GDET_CFG4 stGDetCfg4;
    T32GDET_CFG5 stGDetCfg5;
    T32GDET_CFG6 stGDetCfg6;
    T32GDET_CFG7 stGDetCfg7;
    T32GDET_CFG9 stGDetCfg9;

    /* Control values */
    stGDetCfg0.uCFG0_switch            = vpp_obj->detee.DETEEGDetCtrl.gdet_switch;
    stGDetCfg0.uCFG0_gdet_en           = vpp_obj->detee.DETEEGDetCtrl.gdet_en;
    stGDetCfg0.uCFG0_gdet_add          = vpp_obj->detee.DETEEGDetCtrl.gdet_add;
    stGDetCfg0.uCFG0_ups_en            = vpp_obj->detee.DETEEGDetCtrl.ups_en;

    stGDetCfg0.uCFG0_win_r             = pEEGDetParam->win_r;
    stGDetCfg0.uCFG0_eps               = pEEGDetParam->eps;
    stGDetCfg1.uCFG1_scale             = pEEGDetParam->scale;
    stGDetCfg1.uCFG1_mid               = pEEGDetParam->mid;
    stGDetCfg1.uCFG1_sigma_s1          = pEEGDetParam->sigma_s1;
    stGDetCfg1.uCFG1_sigma_s2          = pEEGDetParam->sigma_s2;

    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG0, stGDetCfg0.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    vpp_obj->detee.DETEEGDetParam.win_r   = stGDetCfg0.uCFG0_win_r;
    vpp_obj->detee.DETEEGDetParam.eps     = stGDetCfg0.uCFG0_eps;

    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG1, stGDetCfg1.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    /* Noise params */
    stGDetCfg3.uCFG3_noise_num         = pEEGDetParam->noise_num;
    stGDetCfg3.uCFG3_noise_offset      = pEEGDetParam->noise_offset;
    stGDetCfg3.uCFG3_noise_cap         = pEEGDetParam->noise_cap;
    stGDetCfg9.uCFG9_noise_adj_s       = pEEGDetParam->noise_adj_s;
    stGDetCfg9.uCFG9_noise_act_cap     = pEEGDetParam->noise_act_cap;

    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG3, stGDetCfg3.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG9, stGDetCfg9.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    /* Skin prams */
    stGDetCfg2.uCFG2_scale_skin        = pEEGDetParam->scale_skin;
    stGDetCfg2.uCFG2_mid_skin          = pEEGDetParam->mid_skin;
    stGDetCfg2.uCFG2_sigma_s1_skin     = pEEGDetParam->sigma_s1_skin;
    stGDetCfg2.uCFG2_sigma_s2_skin     = pEEGDetParam->sigma_s2_skin;
    stGDetCfg4.uCFG4_noise_num_skin    = pEEGDetParam->noise_num_skin;
    stGDetCfg4.uCFG4_noise_offset_skin = pEEGDetParam->noise_offset_skin;
    stGDetCfg4.uCFG4_noise_cap_skin    = pEEGDetParam->noise_cap_skin;
    stGDetCfg5.uCFG5_skin_cos2_h_low   = pEEGDetParam->skin_cos2_h_low;
    stGDetCfg5.uCFG5_skin_cos2_h_high  = pEEGDetParam->skin_cos2_h_high;
    stGDetCfg5.uCFG5_skin_s_low        = pEEGDetParam->skin_s_low;
    stGDetCfg6.uCFG6_skin_s_high       = pEEGDetParam->skin_s_high;
    stGDetCfg6.uCFG6_skin_v_low        = pEEGDetParam->skin_v_low;
    stGDetCfg6.uCFG6_skin_v_high       = pEEGDetParam->skin_v_high;
    stGDetCfg7.uCFG7_skin_y            = pEEGDetParam->skin_y;
    stGDetCfg7.uCFG7_skin_rmv_th       = pEEGDetParam->skin_rmv_th;
    stGDetCfg7.uCFG7_skin_set_th       = pEEGDetParam->skin_set_th;

    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG2, stGDetCfg2.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG4, stGDetCfg4.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG5, stGDetCfg5.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG6, stGDetCfg6.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG7, stGDetCfg7.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    return RetVal;
}

/***************************************************************************************
 * FUNCTION: Set the register values of Guide detail based on mode
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CPCB GDet values to BCM buffer
 ***************************************************************************************/
INT DETEE_SetGDetParamsMode(VPP_OBJ *vpp_obj, INT mode)
{
    INT RetVal = MV_VPP_OK;
    T32GDET_CFG0 stGDetCfg0;
    T32GDET_CFG1 stGDetCfg1;
    T32GDET_CFG2 stGDetCfg2;
    T32GDET_CFG3 stGDetCfg3;
    T32GDET_CFG4 stGDetCfg4;
    T32GDET_CFG5 stGDetCfg5;
    T32GDET_CFG6 stGDetCfg6;
    T32GDET_CFG7 stGDetCfg7;
    T32GDET_CFG8 stGDetCfg8;
    T32GDET_CFG9 stGDetCfg9;

    /* Guide detail */
    stGDetCfg0.uCFG0_switch            = gFeGDetDefValues[mode].VPP_GDET_SWITCH;
    stGDetCfg0.uCFG0_gdet_en           = gFeGDetDefValues[mode].VPP_GDET_GDET_EN;
    stGDetCfg0.uCFG0_gdet_add          = gFeGDetDefValues[mode].VPP_GDET_GDET_ADD;
    stGDetCfg0.uCFG0_ups_en            = gFeGDetDefValues[mode].VPP_GDET_UPS_EN;
    stGDetCfg0.uCFG0_win_r             = gFeGDetDefValues[mode].VPP_GDET_WIN_R;
    stGDetCfg0.uCFG0_eps               = gFeGDetDefValues[mode].VPP_GDET_EPS;
    stGDetCfg1.uCFG1_scale             = gFeGDetDefValues[mode].VPP_GDET_SCALE;
    stGDetCfg1.uCFG1_mid               = gFeGDetDefValues[mode].VPP_GDET_MID;
    stGDetCfg1.uCFG1_sigma_s1          = gFeGDetDefValues[mode].VPP_GDET_SIGMA_S1;
    stGDetCfg1.uCFG1_sigma_s2          = gFeGDetDefValues[mode].VPP_GDET_SIGMA_S2;
    stGDetCfg2.uCFG2_scale_skin        = gFeGDetDefValues[mode].VPP_GDET_SCALE_SKIN;
    stGDetCfg2.uCFG2_mid_skin          = gFeGDetDefValues[mode].VPP_GDET_MID_SKIN;
    stGDetCfg2.uCFG2_sigma_s1_skin     = gFeGDetDefValues[mode].VPP_GDET_SIGMA_S1_SKIN;
    stGDetCfg2.uCFG2_sigma_s2_skin     = gFeGDetDefValues[mode].VPP_GDET_SIGMA_S2_SKIN;
    stGDetCfg3.uCFG3_noise_num         = gFeGDetDefValues[mode].VPP_GDET_NOISE_NUM;
    stGDetCfg3.uCFG3_noise_offset      = gFeGDetDefValues[mode].VPP_GDET_NOISE_OFFSET;
    stGDetCfg3.uCFG3_noise_cap         = gFeGDetDefValues[mode].VPP_GDET_NOISE_CAP;
    stGDetCfg4.uCFG4_noise_num_skin    = gFeGDetDefValues[mode].VPP_GDET_NOISE_NUM_SKIN;
    stGDetCfg4.uCFG4_noise_offset_skin = gFeGDetDefValues[mode].VPP_GDET_NOISE_OFFSET_SKIN;
    stGDetCfg4.uCFG4_noise_cap_skin    = gFeGDetDefValues[mode].VPP_GDET_NOISE_CAP_SKIN;
    stGDetCfg5.uCFG5_skin_cos2_h_low   = gFeGDetDefValues[mode].VPP_GDET_SKIN_COS2_H_LOW;
    stGDetCfg5.uCFG5_skin_cos2_h_high  = gFeGDetDefValues[mode].VPP_GDET_SKIN_COS2_H_HIGH;
    stGDetCfg5.uCFG5_skin_s_low        = gFeGDetDefValues[mode].VPP_GDET_SKIN_S_LOW;
    stGDetCfg6.uCFG6_skin_s_high       = gFeGDetDefValues[mode].VPP_GDET_SKIN_S_HIGH;
    stGDetCfg6.uCFG6_skin_v_low        = gFeGDetDefValues[mode].VPP_GDET_SKIN_V_LOW;
    stGDetCfg6.uCFG6_skin_v_high       = gFeGDetDefValues[mode].VPP_GDET_SKIN_V_HIGH;
    stGDetCfg7.uCFG7_skin_y            = gFeGDetDefValues[mode].VPP_GDET_SKIN_Y;
    stGDetCfg7.uCFG7_skin_rmv_th       = gFeGDetDefValues[mode].VPP_GDET_SKIN_RMV_TH;
    stGDetCfg7.uCFG7_skin_set_th       = gFeGDetDefValues[mode].VPP_GDET_SKIN_SET_TH;
    stGDetCfg8.uCFG8_skin_debug        = gFeGDetDefValues[mode].VPP_GDET_SKIN_DEBUG;
    stGDetCfg8.uCFG8_skin_debug_y      = gFeGDetDefValues[mode].VPP_GDET_SKIN_DEBUG_Y;
    stGDetCfg8.uCFG8_skin_debug_c      = gFeGDetDefValues[mode].VPP_GDET_SKIN_DEBUG_C;
    stGDetCfg9.uCFG9_noise_adj_s       = gFeGDetDefValues[mode].VPP_GDET_NOISE_ADJ_S;
    stGDetCfg9.uCFG9_noise_act_cap     = gFeGDetDefValues[mode].VPP_GDET_NOISE_ACT_CAP;

    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG0, stGDetCfg0.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG1, stGDetCfg1.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG2, stGDetCfg2.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG3, stGDetCfg3.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG4, stGDetCfg4.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG5, stGDetCfg5.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG6, stGDetCfg6.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG7, stGDetCfg7.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG8, stGDetCfg8.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_GDET + RA_GDET_CFG9, stGDetCfg9.u32);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    return RetVal;
}


/***************************************************************************************
 * FUNCTION: load the default register values of FE detail EE and CMU EE block
 * PARAMS: None.
 * RETURN:  1 - succeed
 *                others - failed to load CMU EE default values to BCM buffer
 ***************************************************************************************/
INT DTEE_LoadDefaultVal(VPP_OBJ *vpp_obj)
{
    INT RetVal = MV_VPP_OK;

#ifdef ENABLE_DETEE
    //Read default first time
    GA_REG_WORD32_READ(vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL10,&vpp_obj->stMainPlCtrl10.u32);

    RetVal = DETEE_Control(vpp_obj,FALSE);
    if (MV_VPP_OK != RetVal)
        return RetVal;

    RetVal = DETEE_SetGDetCtrl(vpp_obj, (PVPP_DETEE_GDET_CTRL)&gCpcbGDetCtrlDefVal);
    if (MV_VPP_OK != RetVal)
        return RetVal;
    RetVal = DETEE_SetGDetParams(vpp_obj, (PVPP_DETEE_GDET_PARAMS)&gCpcbGDetParamsDefVal);
    if (MV_VPP_OK != RetVal)
        return RetVal;
#endif

    return RetVal;
}

INT DETEE_Control(VPP_OBJ *vpp_obj,int enable)
{
    INT RetVal = MV_VPP_OK;
    T32DETEE_CTRL0 stDeteeCtl;

    stDeteeCtl.uCTRL0_clken_ctrl =  0;
    stDeteeCtl.uCTRL0_ofifo_sts_ctrl = 0;
    stDeteeCtl.uCTRL0_ififo_sts_ctrl = 0;

    if (enable == 0) {
        stDeteeCtl.uCTRL0_detailEE_bypass = 1;
        stDeteeCtl.uCTRL0_detEE_top_bypass = 1;
        vpp_obj->cmu.cmu_ctl0.uCTRL0_EETop_bypass =  0;
        vpp_obj->stMainPlCtrl10.uCTRL10_detEE_top_bypass = 1;
        vpp_obj->stMainPlCtrl10.uCTRL10_detEE_top_ctrl2 = 0;
        vpp_obj->stMainPlCtrl10.uCTRL10_adpDetScl_bypass = 1;
        if (vpp_obj->cmu.cmu_enabled == 0) {
            vpp_obj->stMainPlCtrl10.uCTRL10_eecmu_top_bypass = 1;
            vpp_obj->cmu.cmu_ctl0.uCTRL0_EETop_bypass = 1;
            vpp_obj->stMainPlCtrl10.uCTRL10_ee_cmu_sel = 0;
        }
    } else {
        stDeteeCtl.uCTRL0_detailEE_bypass = 0;
        stDeteeCtl.uCTRL0_detEE_top_bypass = 0;
        vpp_obj->stMainPlCtrl10.uCTRL10_detEE_top_bypass = 0;
        vpp_obj->stMainPlCtrl10.uCTRL10_detEE_top_ctrl2 = 1;
        vpp_obj->stMainPlCtrl10.uCTRL10_adpDetScl_bypass = 0;
        vpp_obj->stMainPlCtrl10.uCTRL10_eeTop_sel = 1;
        vpp_obj->cmu.cmu_ctl0.uCTRL0_EETop_bypass =  0;
        if (vpp_obj->cmu.cmu_enabled == 0) {
            vpp_obj->cmu.cmu_ctl0.uCTRL0_cmu_bypass = 1;
            vpp_obj->stMainPlCtrl10.uCTRL10_ee_cmu_sel = 0;
            vpp_obj->stMainPlCtrl10.uCTRL10_eecmu_top_bypass = 1;
        }

    }
    RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_CTRL0, stDeteeCtl.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL10, vpp_obj->stMainPlCtrl10.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr_mp + RA_VPP_MP_EECMU + RA_EECMU_CTRL0, vpp_obj->cmu.cmu_ctl0.u32);

    vpp_obj->detee.detee_enabled  = enable;

    return RetVal;
}

INT DETEE_ConfigTG(VPP_OBJ *vpp_obj, int width, int height)
{
    VPP_DETEE_INPUT_RES totres;
    VPP_DETEE_INPUT_CROP inpcrop;

    totres.HRes = width;
    totres.VRes = height;

    inpcrop.leftCrop = 0;
    inpcrop.rightCrop = 0;
    inpcrop.topCrop = 0;
    inpcrop.bottomCrop = 0;

    DETEE_SetDummyTG(vpp_obj, 0, &totres, &inpcrop);

    return 0;
}

static INT DETEE_SetDummyTG(VPP_OBJ *vpp_obj, INT ldr_id, PVPP_DETEE_INPUT_RES pTotRes, PVPP_DETEE_INPUT_CROP pCropRes)
{
    UINT32 RegAddr;
    UINT32 RegVal;
    INT RetVal = MV_VPP_OK;

    RegAddr = vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_DET_TG;

    RegVal = (pTotRes->VRes + VPP_DETEE_DUMMY_TG_SIZE_V_OFF_P) + ((pTotRes->HRes + VPP_DETEE_DUMMY_TG_SIZE_H_OFF_P) << 16);

    /*write TG size to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_SIZE), RegVal)))
        return RetVal;

    /*calculate the H blank front edge value*/
    RegVal = pTotRes->HRes + VPP_DETEE_DUMMY_TG_HB_DETEE_OFF + ((VPP_DETEE_DUMMY_TG_HB_BE) << 16);

    /*write the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB), RegVal)))
        return RetVal;
    RegVal = (pTotRes->HRes-pCropRes->rightCrop) + VPP_DETEE_DUMMY_TG_HB_DETEE_OFF + (((pCropRes->leftCrop)+VPP_DETEE_DUMMY_TG_HB_BE) << 16);
    /*write the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_HB_CR), RegVal)))
        return RetVal;

    /*calculate the V blank front edge value*/
    RegVal = pTotRes->VRes + VPP_DETEE_DUMMY_TG_VB0_DETEE_OFF + ((VPP_DETEE_DUMMY_TG_VB0_BE) << 16);

    /*writer the data to BCM buffer*/
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0), RegVal)))
        return RetVal;
    RegVal = (pTotRes->VRes-pCropRes->bottomCrop) + VPP_DETEE_DUMMY_TG_VB0_DETEE_OFF + ((pCropRes->topCrop+VPP_DETEE_DUMMY_TG_VB0_BE) << 16);
    if (MV_VPP_OK != (RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, (RegAddr + RA_TG_VB0_CR), RegVal)))
        return RetVal;

    vpp_obj->stMainPlCtrl11.u32 = pTotRes->HRes*pTotRes->VRes;
    vpp_obj->stMainPlCtrl12.u32 = pTotRes->HRes*pTotRes->VRes;
    vpp_obj->stMainPlCtrl14.u32 = pTotRes->HRes*pTotRes->VRes;

    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL14, vpp_obj->stMainPlCtrl14.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL12, vpp_obj->stMainPlCtrl12.u32);
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL11, vpp_obj->stMainPlCtrl11.u32);

    return RetVal;
}

INT DETEE_TG_Start(VPP_OBJ *vpp_obj)
{
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_TG_CTRL;

    vpp_obj->detee.detee_tg_ctl.uTG_CTRL_start = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->detee.detee_tg_ctl.u32);
    vpp_obj->detee.detee_tg_ctl.uTG_CTRL_start = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->detee.detee_tg_ctl.u32);
    if (vpp_obj->cmu.cmu_enabled == 0) {
        CMU_TG_Start(vpp_obj);
    }

    return MV_VPP_OK;
}

INT DETEE_TG_Clear(VPP_OBJ *vpp_obj)
{
    UINT32 RegAddr;

    RegAddr = vpp_obj->base_addr_mp + RA_VPP_MP_DETEE + RA_DETEE_TG_CTRL;

    vpp_obj->detee.detee_tg_ctl.uTG_CTRL_clear = 1;
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->detee.detee_tg_ctl.u32);
    vpp_obj->detee.detee_tg_ctl.uTG_CTRL_clear = 0;
    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, RegAddr, vpp_obj->detee.detee_tg_ctl.u32);
    if (vpp_obj->cmu.cmu_enabled == 0) {
        CMU_TG_Clear(vpp_obj);
    }

    return MV_VPP_OK;
}

