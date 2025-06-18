/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_SCL_DATA_H_
#define _VPP_SCL_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************/
/*****************Macros and Constants************/
/*************************************************/
/* definition of VPP in-line and off-line modes */
 enum {
    VPP_SCL_MODE_INVALID     = -1,
    VPP_SCL_MODE_INLINE     = 0,
    VPP_SCL_MODE_OFFLINE,
    VPP_SCL_MODE_INLINE_OFFLINE,
    VPP_SCL_MODE_OFFLINE_OFFLINE,
};

/*************************************************/
/********************** SCL structure***************/
/*************************************************/
/*CPCB0 output resolution*/
typedef struct VPP_CPCB0_OUTPUT_RES_T
{
    UINT32  HRes;
    UINT32  VRes;
} VPP_CPCB0_OUTPUT_RES, *PVPP_CPCB0_OUTPUT_RES;

/*scaler input and output resolution*/
typedef struct VPP_SCL_RES_T
{
    UINT32 IVRes;
    UINT32 IHRes;
    UINT32 OVRes;
    UINT32 OHRes;
    UINT16 x;
    UINT16 y;
} VPP_SCL_RES, *PVPP_SCL_RES;

/*data structure for scaler control*/
typedef struct VPP_SCL_CTRL_T
{
    UINT8  HScalePos;  //vertical scaler position, usually set to auto
    UINT8  InputClr;   //input color space
    UINT8  OsdInput;   //whether OSD input
    UINT8  NLEn;       //enable non linear scaler
    UINT8  BitMode;    //input and out put bit mode
    UINT8  I565;       //whether 18 bits 656 input

    UINT8  HTapNum;
    UINT8  VTapNum;

    UINT8  DynamicLoad; // 1: dynamically load coefficient table, 0: don't load coefficient table dynamically.
    UINT8  ForceSel; // 0: secect coefficients by scaling ratio.1:force selection scaler H coeff, 2:force selection scaler V coeff, 3:force selection both
    INT  HSclMode; //if set ForceSel to 1, HSclMode should be set
    INT  VSclMode; //if set ForceSel to 1, VSclMode should be set


    UINT8  CscMode;    //used when the input is OSD
    UINT8  CenterFrac; //used when enable NL scaling
    UINT8  CenterRatio;//used when enable NL scaling

    //cropping related variables
    UINT8  leftCrop;        //for odd start x
    UINT8  rightCrop;       //for odd width
    UINT32 uncrop_IHRes;    //complete/uncropped resolution input to SCL - used for lsize calculation
    UINT8  uiUVswapEn;

    /* Crop after Scaling */
    UINT8 cropAfterSclLeft;
    UINT8 cropAfterSclRight;
    UINT8 cropAfterSclTop;
    UINT8 cropAfterSclBottom;

    UINT8 hvswap_en;
    UINT8 hv_swap_updated;

} VPP_SCL_CTRL, *PVPP_SCL_CTRL;

#ifdef __cplusplus
}
#endif

#endif

