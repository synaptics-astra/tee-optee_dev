/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_BE_VOP_PRV_H_
#define _VPP_BE_VOP_PRV_H_

#include "com_type.h"

/**********************************************************************
*macro definitions for backend VOP                                                                       *
***********************************************************************/
/*register number in a CSC block, include coeffs and offsets*/
#define VPP_VOP_CSC_REG_NUM          32

/*CSC coeff index*/
#define VPP_VOP_CSC_MAX_COEFF_INDEX  10

/*CSC offset index*/
#define VPP_VOP_CSC_MAX_OFFSET_INDEX 3

  /**********************************************************************
*VOP Coefficients and Offsets for AHD/ASD/HDMI/TTL24/TTL30 Channel CSC           *
***********************************************************************/
const UINT16 VppVopCscCoeff[VPP_VOP_CSC_MAX_MODES][VPP_VOP_CSC_MAX_COEFF_INDEX] = {
    /*  C0       C1      C2      C3      C4      C5      C6      C7      C8   */
    {   0x200,  0,      0,      0,      0x200,  0,      0,      0,      0x200, 0 }, //Bypass Mode
    {   0x20A,  0,      0x3B,   0x203B, 0x200,  0x206A, 0x26,   0,      0x20D, 0 }, //601->709
    {   0x1FB,  0,      0x2039, 0x33,   0x200,  0x62,   0x2025, 0,      0x1F7, 0 }, //709->601
    {   0x106,  0x20CA, 0x203C, 0x25,   0x16E,  0x6D,   0x2018, 0x20EE, 0x106, 0 }, //RGB->YUV(709)
    {   0x3A2,  0x200,  0,      0x205E, 0x200,  0x20EB, 0,      0x200,  0x314, 0 }, //YUV(709)->RGB
    {   0x106,  0x20AE, 0x2058, 0x3A,   0x12D,  0x99,   0x202A, 0x20DB, 0x106, 0 }, //RGB->YUV(601)
    {   0x377,  0x200,  0,      0x20AC, 0x200,  0x2165, 0,      0x200,  0x2BE, 0 },  //YUV(601)->RGB
    {   0x0E1,  0x20AD, 0x2034, 0x20,   0x13A,  0x5D,   0x2015, 0x20CC, 0x0E1, 0 }, //sRGB->YUV(709)
    {   0x439,  0x254,  0,      0x206D, 0x254,  0x2111, 0,      0x254,  0x396, 0 },  //YUV(709)->sRGB
    {   0x0E1,  0x2095, 0x204C, 0x32,   0x102,  0x84,   0x2025, 0x20BC, 0x0E1, 0 }, //sRGB->YUV(601)
    {   0x408,  0x254,  0,      0x20C9, 0x254,  0x21A0, 0,      0x254,  0x331, 0 },  //YUV(601)->sRGB
    {   0x106,  0x20BD, 0x2049, 0x1E,   0x15B,  0x87,   0x2015, 0x20F1, 0x106 }, //RGB->YUV(2020)
    {   0x3AE,  0x200,  0,      0x2052, 0x200,  0x211E, 0,      0x200,  0x2E2 }, //YUV(2020)->RGB
    {   0xE1,   0x20A2, 0x203F, 0x1A,   0x12A,  0x74,   0x2012, 0x20CF, 0xE1  }, //sRGB->YUV(2020)
    {   0x449,  0x254,  0,      0x2060, 0x254,  0x214D, 0,      0x254,  0x35B }, //YUV(2020)->sRGB
    {   0x205,  0,      0x1F,   0x2040, 0x200,  0x203A, 0x2C,   0,      0x20F }, //601->2020
    {   0x1FE,  0,      0x201E, 0x3B,   0x200,  0x35,   0x202B, 0,      0x1F4 }, //2020->601
    {   0x1FE,  0,      0x201A, 0x2009, 0x200,  0x30,   0x6,    0,      0x201 }, //709->2020
    {   0x202,  0,      0x1A,   0x9,    0x200,  0x2030, 0x2006, 0,      0x1FE }, //2020->709
    {   0,  0,      0x200,      0,      0x200,  0,      0x200,      0,      0, 0 }, // U <-> V swap mode
};

const UINT32 VppVopCscOffset[VPP_VOP_CSC_MAX_MODES][VPP_VOP_CSC_MAX_OFFSET_INDEX] = {
    /* A0       A1          A2      */
    { 0,           0,         0        }, //Bypass Mode
    { 0x808878,    0x14B40,   0x8066C8 }, //601->709
    { 0x7BB0,      0x812A00,  0x5B30   }, //709->601
    { 0x40000,     0,         0x40000  }, //RGB->YUV(709)
    { 0x874278 ,   0x29140 ,  0x862A10 }, //YUV(709)->RGB
    { 0x40000,     0,         0x40000  }, //RGB->YUV(601)
    { 0x86ED90,    0x423C0,   0x857C50 }, //YUV(601)->RGB
    { 0x40000,     0x8000,    0x40000  }, //sRGB->YUV(709)
    { 0x890F98,    0x269B8,   0x87C6D0 }, //YUV(709)->sRGB
    { 0x40000,     0x8000,    0x40000  }, //sRGB->YUV(601)
    { 0x88AC88,    0x44320,   0x86FD20 }, //YUV(601)->sRGB
    { 0x040000,      0x000000,    0x040000  },  //RGB->YUV(2020)
    { 0x875800,      0x02E000,    0x85C800  },  //YUV(2020)->RGB
    { 0x040000,      0x008000,    0x040000  }, //sRGB->YUV(2020)
    { 0x892800,      0x02C800,    0x875000  }, //YUV(2020)->sRGB
    { 0x804991,      0x00F2DF,    0x80739A  }, //601->2020
    { 0x803BBF,      0x004C01,    0x000D6E  }, //2020->601
    { 0x003486,      0x804F4F,    0x800E34  }, //709->2020
    { 0x80361B,      0x004C01,    0x000D6E  }, //2020->709
    { 0,           0,         0        }, //U <-> V swap Mode
};


/**********************************************************************
*bit field definition for VOP block                                                                            *
***********************************************************************/
typedef union VPP_VOP_CLK_POL_T
{
    struct {
        UINT32 Ttl30ClkPol                 : 1;
        UINT32 Ttl24ClkPol                 : 1;
        UINT32 reserved                    :30;
    } field;

    UINT32 data;
} VPP_VOP_CLK_POL;

/**********************************************************************
*bit field definition for HDMI fomater control                                                                        *
***********************************************************************/
typedef union VPP_VOP_HDMI_FRMT_CTRL0_T
{
    struct {
        UINT32 Enable                   : 1;
        UINT32 Mode                     : 1;
        UINT32 Load                     : 1;
        UINT32 ASInsert                 : 1;
        UINT32 ViewID                   : 2;
        UINT32 PackMode                 : 2;
        UINT32 reserved                 :24;
    } field;

    UINT32 data;
} VPP_VOP_HDMI_FRMT_CTRL0;

/**********************************************************************
*default register value in local block buffer                                                             *
***********************************************************************/
/*BE VOP register structure*/
typedef struct VPP_BE_VOP_T{
    UINT32 VPP_VOP_HDMI_CBLANK_L;
    UINT32 VPP_VOP_HDMI_CBLANK_H;
    UINT32 VPP_VOP_HDMI_MODE;
    UINT32 VPP_VOP_HDMI_SEL;
    UINT32 VPP_VOP_HDMI_ALIGN;
    UINT32 VPP_VOP_HDMI_RND;
    UINT32 VPP_VOP_AHD_SEL;
    UINT32 VPP_VOP_AHD_ALIGN;
    UINT32 VPP_VOP_AHD_RND;
    UINT32 VPP_VOP_ASD_SEL;
    UINT32 VPP_VOP_ASD_ALIGN;
    UINT32 VPP_VOP_ASD_RND;
    UINT32 VPP_VOP_TTL24_SEL;
    UINT32 VPP_VOP_TTL30_MODE;
    UINT32 VPP_VOP_TTL30_CBLANK_L;
    UINT32 VPP_VOP_TTL30_CBLANK_H;
    UINT32 VPP_VOP_TTL30_SEL;
    UINT32 VPP_VOP_TTL30_ALIGN;
    UINT32 VPP_VOP_TTL30_RND;
    UINT32 VPP_VOP_DITHER;
    UINT32 VPP_VOP_DIT_MIRROR;
    UINT32 VPP_VOP_DIT_INP_RES;
    UINT32 VPP_VOP_DIT_OUT_RES;
    UINT32 VPP_VOP_3D_HDMI_CTRL0;
    UINT32 VPP_VOP_3D_HDMI_CTRL1;
    UINT32 VPP_VOP_3D_HDMI_UDATA_L;
    UINT32 VPP_VOP_3D_HDMI_UDATA_H;
    UINT32 VPP_VOP_3D_HDMI_MDATA_L;
    UINT32 VPP_VOP_3D_HDMI_MDATA_H;
    UINT32 VPP_VOP_3D_HDMI_LDATA_L;
    UINT32 VPP_VOP_3D_HDMI_LDATA_H;
    UINT32 VPP_VOP_AHD_C0_L;
    UINT32 VPP_VOP_AHD_C0_H;
    UINT32 VPP_VOP_AHD_C1_L;
    UINT32 VPP_VOP_AHD_C1_H;
    UINT32 VPP_VOP_AHD_C2_L;
    UINT32 VPP_VOP_AHD_C2_H;
    UINT32 VPP_VOP_AHD_C3_L;
    UINT32 VPP_VOP_AHD_C3_H;
    UINT32 VPP_VOP_AHD_C4_L;
    UINT32 VPP_VOP_AHD_C4_H;
    UINT32 VPP_VOP_AHD_C5_L;
    UINT32 VPP_VOP_AHD_C5_H;
    UINT32 VPP_VOP_AHD_C6_L;
    UINT32 VPP_VOP_AHD_C6_H;
    UINT32 VPP_VOP_AHD_C7_L;
    UINT32 VPP_VOP_AHD_C7_H;
    UINT32 VPP_VOP_AHD_C8_L;
    UINT32 VPP_VOP_AHD_C8_H;
    UINT32 VPP_VOP_AHD_A0_L;
    UINT32 VPP_VOP_AHD_A0_H;
    UINT32 VPP_VOP_AHD_A1_L;
    UINT32 VPP_VOP_AHD_A1_H;
    UINT32 VPP_VOP_AHD_A2_L;
    UINT32 VPP_VOP_AHD_A2_H;

    UINT32 VPP_VOP_ASD_C0_L;
    UINT32 VPP_VOP_ASD_C0_H;
    UINT32 VPP_VOP_ASD_C1_L;
    UINT32 VPP_VOP_ASD_C1_H;
    UINT32 VPP_VOP_ASD_C2_L;
    UINT32 VPP_VOP_ASD_C2_H;
    UINT32 VPP_VOP_ASD_C3_L;
    UINT32 VPP_VOP_ASD_C3_H;
    UINT32 VPP_VOP_ASD_C4_L;
    UINT32 VPP_VOP_ASD_C4_H;
    UINT32 VPP_VOP_ASD_C5_L;
    UINT32 VPP_VOP_ASD_C5_H;
    UINT32 VPP_VOP_ASD_C6_L;
    UINT32 VPP_VOP_ASD_C6_H;
    UINT32 VPP_VOP_ASD_C7_L;
    UINT32 VPP_VOP_ASD_C7_H;
    UINT32 VPP_VOP_ASD_C8_L;
    UINT32 VPP_VOP_ASD_C8_H;
    UINT32 VPP_VOP_ASD_A0_L;
    UINT32 VPP_VOP_ASD_A0_H;
    UINT32 VPP_VOP_ASD_A1_L;
    UINT32 VPP_VOP_ASD_A1_H;
    UINT32 VPP_VOP_ASD_A2_L;
    UINT32 VPP_VOP_ASD_A2_H;

    UINT32 VPP_VOP_HDMI_C0_L;
    UINT32 VPP_VOP_HDMI_C0_H;
    UINT32 VPP_VOP_HDMI_C1_L;
    UINT32 VPP_VOP_HDMI_C1_H;
    UINT32 VPP_VOP_HDMI_C2_L;
    UINT32 VPP_VOP_HDMI_C2_H;
    UINT32 VPP_VOP_HDMI_C3_L;
    UINT32 VPP_VOP_HDMI_C3_H;
    UINT32 VPP_VOP_HDMI_C4_L;
    UINT32 VPP_VOP_HDMI_C4_H;
    UINT32 VPP_VOP_HDMI_C5_L;
    UINT32 VPP_VOP_HDMI_C5_H;
    UINT32 VPP_VOP_HDMI_C6_L;
    UINT32 VPP_VOP_HDMI_C6_H;
    UINT32 VPP_VOP_HDMI_C7_L;
    UINT32 VPP_VOP_HDMI_C7_H;
    UINT32 VPP_VOP_HDMI_C8_L;
    UINT32 VPP_VOP_HDMI_C8_H;
    UINT32 VPP_VOP_HDMI_A0_L;
    UINT32 VPP_VOP_HDMI_A0_H;
    UINT32 VPP_VOP_HDMI_A1_L;
    UINT32 VPP_VOP_HDMI_A1_H;
    UINT32 VPP_VOP_HDMI_A2_L;
    UINT32 VPP_VOP_HDMI_A2_H;

    UINT32 VPP_VOP_TTL24_C0_L;
    UINT32 VPP_VOP_TTL24_C0_H;
    UINT32 VPP_VOP_TTL24_C1_L;
    UINT32 VPP_VOP_TTL24_C1_H;
    UINT32 VPP_VOP_TTL24_C2_L;
    UINT32 VPP_VOP_TTL24_C2_H;
    UINT32 VPP_VOP_TTL24_C3_L;
    UINT32 VPP_VOP_TTL24_C3_H;
    UINT32 VPP_VOP_TTL24_C4_L;
    UINT32 VPP_VOP_TTL24_C4_H;
    UINT32 VPP_VOP_TTL24_C5_L;
    UINT32 VPP_VOP_TTL24_C5_H;
    UINT32 VPP_VOP_TTL24_C6_L;
    UINT32 VPP_VOP_TTL24_C6_H;
    UINT32 VPP_VOP_TTL24_C7_L;
    UINT32 VPP_VOP_TTL24_C7_H;
    UINT32 VPP_VOP_TTL24_C8_L;
    UINT32 VPP_VOP_TTL24_C8_H;
    UINT32 VPP_VOP_TTL24_A0_L;
    UINT32 VPP_VOP_TTL24_A0_H;
    UINT32 VPP_VOP_TTL24_A1_L;
    UINT32 VPP_VOP_TTL24_A1_H;
    UINT32 VPP_VOP_TTL24_A2_L;
    UINT32 VPP_VOP_TTL24_A2_H;

    UINT32 VPP_VOP_TTL30_C0_L;
    UINT32 VPP_VOP_TTL30_C0_H;
    UINT32 VPP_VOP_TTL30_C1_L;
    UINT32 VPP_VOP_TTL30_C1_H;
    UINT32 VPP_VOP_TTL30_C2_L;
    UINT32 VPP_VOP_TTL30_C2_H;
    UINT32 VPP_VOP_TTL30_C3_L;
    UINT32 VPP_VOP_TTL30_C3_H;
    UINT32 VPP_VOP_TTL30_C4_L;
    UINT32 VPP_VOP_TTL30_C4_H;
    UINT32 VPP_VOP_TTL30_C5_L;
    UINT32 VPP_VOP_TTL30_C5_H;
    UINT32 VPP_VOP_TTL30_C6_L;
    UINT32 VPP_VOP_TTL30_C6_H;
    UINT32 VPP_VOP_TTL30_C7_L;
    UINT32 VPP_VOP_TTL30_C7_H;
    UINT32 VPP_VOP_TTL30_C8_L;
    UINT32 VPP_VOP_TTL30_C8_H;
    UINT32 VPP_VOP_TTL30_A0_L;
    UINT32 VPP_VOP_TTL30_A0_H;
    UINT32 VPP_VOP_TTL30_A1_L;
    UINT32 VPP_VOP_TTL30_A1_H;
    UINT32 VPP_VOP_TTL30_A2_L;
    UINT32 VPP_VOP_TTL30_A2_H;
    UINT32 VPP_VOP_C656_SATURATE;
    UINT32 VPP_VOP_TTL24_CBLANK_L;
    UINT32 VPP_VOP_TTL24_CBLANK_H;
    UINT32 VPP_VOP_TTL24_MODE;
    UINT32 VPP_VOP_CLKPOL;
    UINT32 VPP_VOP_TTL30_SATURATE;
    UINT32 VPP_VOP_TTL30_CTRL;
    UINT32 VPP_VOP_TTL24_CTRL;
}VPP_BE_VOP, *PVPP_BE_VOP;

/*BE VOP register default values*/
const VPP_BE_VOP gBeVopDefValues = {
    .VPP_VOP_HDMI_CBLANK_L     = 0x00000000,
    .VPP_VOP_HDMI_CBLANK_H     = 0x00000000,
    .VPP_VOP_HDMI_MODE         = 0x00000000,
    .VPP_VOP_HDMI_SEL          = 0x00000000,
    .VPP_VOP_HDMI_ALIGN        = 0x00000000,
    .VPP_VOP_HDMI_RND          = 0x00000004,
#ifdef TURN_OFF_CPCB3_AHD_TTL
    .VPP_VOP_AHD_SEL           = 0x00000004,
#else
    .VPP_VOP_AHD_SEL           = 0x00000000,
#endif
    .VPP_VOP_AHD_ALIGN         = 0x00000001,   //always set to MSB aligned
    .VPP_VOP_AHD_RND           = 0x00000004,
    .VPP_VOP_ASD_SEL           = 0x00000004,
    .VPP_VOP_ASD_ALIGN         = 0x0000001,     //always set to MSB aligned
    .VPP_VOP_ASD_RND           = 0x00000004,
#ifdef TURN_OFF_CPCB3_AHD_TTL
    .VPP_VOP_TTL24_SEL         = 0x00000004,
#else
    .VPP_VOP_TTL24_SEL         = 0x00000000,
#endif
    .VPP_VOP_TTL30_MODE        = 0x00000001,//always enable downsampler
    .VPP_VOP_TTL30_CBLANK_L    = 0x00000000,
    .VPP_VOP_TTL30_CBLANK_H    = 0x00000000,
#ifdef TURN_OFF_CPCB3_AHD_TTL
    .VPP_VOP_TTL30_SEL         = 0x00000004,
#else
    .VPP_VOP_TTL30_SEL         = 0x00000000,
#endif
    .VPP_VOP_3D_HDMI_CTRL0     = 0x00000000,
    .VPP_VOP_3D_HDMI_CTRL1     = 0x00000000,
    .VPP_VOP_3D_HDMI_UDATA_L   = 0x00000000,
    .VPP_VOP_3D_HDMI_UDATA_H   = 0x00000000,
    .VPP_VOP_3D_HDMI_MDATA_L   = 0x00000000,
    .VPP_VOP_3D_HDMI_MDATA_H   = 0x00000000,
    .VPP_VOP_3D_HDMI_LDATA_L   = 0x00000000,
    .VPP_VOP_3D_HDMI_LDATA_H   = 0x00000000,
    .VPP_VOP_AHD_C0_L          = 0x00000000,
    .VPP_VOP_AHD_C0_H          = 0x00000002,
    .VPP_VOP_AHD_C1_L          = 0x00000000,
    .VPP_VOP_AHD_C1_H          = 0x00000000,
    .VPP_VOP_AHD_C2_L          = 0x00000000,
    .VPP_VOP_AHD_C2_H          = 0x00000000,
    .VPP_VOP_AHD_C3_L          = 0x00000000,
    .VPP_VOP_AHD_C3_H          = 0x00000000,
    .VPP_VOP_AHD_C4_L          = 0x00000000,
    .VPP_VOP_AHD_C4_H          = 0x00000002,
    .VPP_VOP_AHD_C5_L          = 0x00000000,
    .VPP_VOP_AHD_C5_H          = 0x00000000,
    .VPP_VOP_AHD_C6_L          = 0x00000000,
    .VPP_VOP_AHD_C6_H          = 0x00000000,
    .VPP_VOP_AHD_C7_L          = 0x00000000,
    .VPP_VOP_AHD_C7_H          = 0x00000000,
    .VPP_VOP_AHD_C8_L          = 0x00000000,
    .VPP_VOP_AHD_C8_H          = 0x00000002,
    .VPP_VOP_AHD_A0_L          = 0x00000000,
    .VPP_VOP_AHD_A0_H          = 0x00000000,
    .VPP_VOP_AHD_A1_L          = 0x00000000,
    .VPP_VOP_AHD_A1_H          = 0x00000000,
    .VPP_VOP_AHD_A2_L          = 0x00000000,
    .VPP_VOP_AHD_A2_H          = 0x00000000,

    .VPP_VOP_ASD_C0_L          = 0x00000000,
    .VPP_VOP_ASD_C0_H          = 0x00000002,
    .VPP_VOP_ASD_C1_L          = 0x00000000,
    .VPP_VOP_ASD_C1_H          = 0x00000000,
    .VPP_VOP_ASD_C2_L          = 0x00000000,
    .VPP_VOP_ASD_C2_H          = 0x00000000,
    .VPP_VOP_ASD_C3_L          = 0x00000000,
    .VPP_VOP_ASD_C3_H          = 0x00000000,
    .VPP_VOP_ASD_C4_L          = 0x00000000,
    .VPP_VOP_ASD_C4_H          = 0x00000002,
    .VPP_VOP_ASD_C5_L          = 0x00000000,
    .VPP_VOP_ASD_C5_H          = 0x00000000,
    .VPP_VOP_ASD_C6_L          = 0x00000000,
    .VPP_VOP_ASD_C6_H          = 0x00000000,
    .VPP_VOP_ASD_C7_L          = 0x00000000,
    .VPP_VOP_ASD_C7_H          = 0x00000000,
    .VPP_VOP_ASD_C8_L          = 0x00000000,
    .VPP_VOP_ASD_C8_H          = 0x00000002,
    .VPP_VOP_ASD_A0_L          = 0x00000000,
    .VPP_VOP_ASD_A0_H          = 0x00000000,
    .VPP_VOP_ASD_A1_L          = 0x00000000,
    .VPP_VOP_ASD_A1_H          = 0x00000000,
    .VPP_VOP_ASD_A2_L          = 0x00000000,
    .VPP_VOP_ASD_A2_H          = 0x00000000,

    .VPP_VOP_HDMI_C0_L         = 0x00000000,
    .VPP_VOP_HDMI_C0_H         = 0x00000002,
    .VPP_VOP_HDMI_C1_L         = 0x00000000,
    .VPP_VOP_HDMI_C1_H         = 0x00000000,
    .VPP_VOP_HDMI_C2_L         = 0x00000000,
    .VPP_VOP_HDMI_C2_H         = 0x00000000,
    .VPP_VOP_HDMI_C3_L         = 0x00000000,
    .VPP_VOP_HDMI_C3_H         = 0x00000000,
    .VPP_VOP_HDMI_C4_L         = 0x00000000,
    .VPP_VOP_HDMI_C4_H         = 0x00000002,
    .VPP_VOP_HDMI_C5_L         = 0x00000000,
    .VPP_VOP_HDMI_C5_H         = 0x00000000,
    .VPP_VOP_HDMI_C6_L         = 0x00000000,
    .VPP_VOP_HDMI_C6_H         = 0x00000000,
    .VPP_VOP_HDMI_C7_L         = 0x00000000,
    .VPP_VOP_HDMI_C7_H         = 0x00000000,
    .VPP_VOP_HDMI_C8_L         = 0x00000000,
    .VPP_VOP_HDMI_C8_H         = 0x00000002,
    .VPP_VOP_HDMI_A0_L         = 0x00000000,
    .VPP_VOP_HDMI_A0_H         = 0x00000000,
    .VPP_VOP_HDMI_A1_L         = 0x00000000,
    .VPP_VOP_HDMI_A1_H         = 0x00000000,
    .VPP_VOP_HDMI_A2_L         = 0x00000000,
    .VPP_VOP_HDMI_A2_H         = 0x00000000,

    .VPP_VOP_TTL24_C0_L        = 0x00000000,
    .VPP_VOP_TTL24_C0_H        = 0x00000002,
    .VPP_VOP_TTL24_C1_L        = 0x00000000,
    .VPP_VOP_TTL24_C1_H        = 0x00000000,
    .VPP_VOP_TTL24_C2_L        = 0x00000000,
    .VPP_VOP_TTL24_C2_H        = 0x00000000,
    .VPP_VOP_TTL24_C3_L        = 0x00000000,
    .VPP_VOP_TTL24_C3_H        = 0x00000000,
    .VPP_VOP_TTL24_C4_L        = 0x00000000,
    .VPP_VOP_TTL24_C4_H        = 0x00000002,
    .VPP_VOP_TTL24_C5_L        = 0x00000000,
    .VPP_VOP_TTL24_C5_H        = 0x00000000,
    .VPP_VOP_TTL24_C6_L        = 0x00000000,
    .VPP_VOP_TTL24_C6_H        = 0x00000000,
    .VPP_VOP_TTL24_C7_L        = 0x00000000,
    .VPP_VOP_TTL24_C7_H        = 0x00000000,
    .VPP_VOP_TTL24_C8_L        = 0x00000000,
    .VPP_VOP_TTL24_C8_H        = 0x00000002,
    .VPP_VOP_TTL24_A0_L        = 0x00000000,
    .VPP_VOP_TTL24_A0_H        = 0x00000000,
    .VPP_VOP_TTL24_A1_L        = 0x00000000,
    .VPP_VOP_TTL24_A1_H        = 0x00000000,
    .VPP_VOP_TTL24_A2_L        = 0x00000000,
    .VPP_VOP_TTL24_A2_H        = 0x00000000,

    .VPP_VOP_TTL30_C0_L        = 0x00000000,
    .VPP_VOP_TTL30_C0_H        = 0x00000002,
    .VPP_VOP_TTL30_C1_L        = 0x00000000,
    .VPP_VOP_TTL30_C1_H        = 0x00000000,
    .VPP_VOP_TTL30_C2_L        = 0x00000000,
    .VPP_VOP_TTL30_C2_H        = 0x00000000,
    .VPP_VOP_TTL30_C3_L        = 0x00000000,
    .VPP_VOP_TTL30_C3_H        = 0x00000000,
    .VPP_VOP_TTL30_C4_L        = 0x00000000,
    .VPP_VOP_TTL30_C4_H        = 0x00000002,
    .VPP_VOP_TTL30_C5_L        = 0x00000000,
    .VPP_VOP_TTL30_C5_H        = 0x00000000,
    .VPP_VOP_TTL30_C6_L        = 0x00000000,
    .VPP_VOP_TTL30_C6_H        = 0x00000000,
    .VPP_VOP_TTL30_C7_L        = 0x00000000,
    .VPP_VOP_TTL30_C7_H        = 0x00000000,
    .VPP_VOP_TTL30_C8_L        = 0x00000000,
    .VPP_VOP_TTL30_C8_H        = 0x00000002,
    .VPP_VOP_TTL30_A0_L        = 0x00000000,
    .VPP_VOP_TTL30_A0_H        = 0x00000000,
    .VPP_VOP_TTL30_A1_L        = 0x00000000,
    .VPP_VOP_TTL30_A1_H        = 0x00000000,
    .VPP_VOP_TTL30_A2_L        = 0x00000000,
    .VPP_VOP_TTL30_A2_H        = 0x00000000,
    .VPP_VOP_C656_SATURATE     = 0x00000000,
    .VPP_VOP_TTL24_CBLANK_L    = 0x00000000,
    .VPP_VOP_TTL24_CBLANK_H    = 0x00000000,
    .VPP_VOP_TTL24_MODE        = 0x00000001,  //always enable downsampler
    .VPP_VOP_CLKPOL            = 0x00000000,
    .VPP_VOP_TTL30_SATURATE    = 0x00000000,
    .VPP_VOP_TTL30_CTRL        = 0x00000000,
    .VPP_VOP_TTL24_CTRL        = 0x00000000,
};

#endif
