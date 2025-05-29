/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CPCB_INT_PRV_H_
#define _VPP_CPCB_INT_PRV_H_

#if defined(BERLIN)

#include "com_type.h"

/**********************************************************************
*macro definitions for interlacer                                                                            *
***********************************************************************/

/*register number in an INT unit*/
#define CPCB_INT_REG_NUM 26

/*get local buffer offset by register name*/
#define GET_CPCB_INT_OFF(RegName) (OFF_##RegName)

/**********************************************************************
*bit field definition for CPCB INT                                                                            *
***********************************************************************/

/*INT Control Register Bit Fields */
typedef union CPCB_INT_CTRL_T
{
    struct {
        UINT32 Enable             : 1;
        UINT32 Mode               : 2;
        UINT32 FieldPol           : 1;
        UINT32 BitMode            : 1;
        UINT32 Reserved           :27;
    } field;

    UINT32 data;
} CPCB_INT_CTRL;

/*INT Memory Size Register Bit Fields */
typedef union CPCB_INT_MEMSIZE_L_T
{
    struct {
        UINT32 MemSizeL          : 8;
        UINT32 Reserved          :24;
    } field;

    UINT32 data;
} CPCB_INT_MEMSIZE_L;

typedef union CPCB_INT_MEMSIZE_H_T
{
    struct {
        UINT32 MemSizeH          : 4;
        UINT32 Reserved          :28;
    } field;

    UINT32 data;
} CPCB_INT_MEMSIZE_H;

/*INT Weighting Factor Register Bit Fields */
typedef union CPCB_INT_WF_L_T
{
    struct {
        UINT32 WFactorL           : 8;
        UINT32 Reserved           :24;
    } field;

    UINT32 data;
} CPCB_INT_WF_L;

typedef union CPCB_INT_WF_H_T
{
    struct {
        UINT32 WFactorH          : 1;
        UINT32 Reserved          :31;
    } field;

    UINT32 data;
} CPCB_INT_WF_H;

/*INT 12-bit Register Bit Fields */
typedef union CPCB_INT_12BITS_L_T
{
    struct {
        UINT32 LowByte            : 8;
        UINT32 Reserved           :24;
    } field;

    UINT32 data;
} CPCB_INT_12BITS_L;

typedef union CPCB_INT_12BITS_H_T
{
    struct {
        UINT32 High4Bits          : 4;
        UINT32 Reserved           :28;
    } field;

    UINT32 data;
} CPCB_INT_12BITS_H;

/**********************************************************************
*register offset definitions in local block buffer                                                        *
***********************************************************************/
#define  OFF_CPCB_INT_CTRL       0x00
#define  OFF_CPCB_INT_MEMSIZE_L  0x08
#define  OFF_CPCB_INT_WF_L       0x10
#define  OFF_CPCB_INT_FX_L       0x18

/**********************************************************************
*default register values of VPP CPCB INT block                                                      *
***********************************************************************/
/*CPCB INT register structure*/
typedef struct VPP_CPCB0_INT_T
{
    UINT32 VPP_CPCB0_INT_CTRL;
    UINT32 VPP_CPCB0_INT_MEMSIZE_L;
    UINT32 VPP_CPCB0_INT_MEMSIZE_H;
    UINT32 VPP_CPCB0_INT_WF_L;
    UINT32 VPP_CPCB0_INT_WF_H;
    UINT32 VPP_CPCB0_INT_FX_L;
    UINT32 VPP_CPCB0_INT_FX_H;
    UINT32 VPP_CPCB0_INT_FY_L;
    UINT32 VPP_CPCB0_INT_FY_H;
    UINT32 VPP_CPCB0_INT_V_S_L;
    UINT32 VPP_CPCB0_INT_V_S_H;
    UINT32 VPP_CPCB0_INT_V_E_L;
    UINT32 VPP_CPCB0_INT_V_E_H;
    UINT32 VPP_CPCB0_INT_H_S_L;
    UINT32 VPP_CPCB0_INT_H_S_H;
    UINT32 VPP_CPCB0_INT_H_E_L;
    UINT32 VPP_CPCB0_INT_H_E_H;
    UINT32 VPP_CPCB0_INT_VX_E_L;
    UINT32 VPP_CPCB0_INT_VX_E_H;
    UINT32 VPP_CPCB0_INT_VX_O_L;
    UINT32 VPP_CPCB0_INT_VX_O_H;
    UINT32 VPP_CPCB0_INT_YBL_L;
    UINT32 VPP_CPCB0_INT_YBL_H;
    UINT32 VPP_CPCB0_INT_CBL_L;
    UINT32 VPP_CPCB0_INT_CBL_H;
    UINT32 VPP_RA_Vpp_CPCB0_FLD;
} VPP_CPCB0_INT, *PVPP_CPCB0_INT;

typedef struct VPP_CPCB2_INT_T
{
    UINT32 VPP_CPCB2_INT_CTRL;
    UINT32 VPP_CPCB2_INT_MEMSIZE_L;
    UINT32 VPP_CPCB2_INT_MEMSIZE_H;
    UINT32 VPP_CPCB2_INT_WF_L;
    UINT32 VPP_CPCB2_INT_WF_H;
    UINT32 VPP_CPCB2_INT_FX_L;
    UINT32 VPP_CPCB2_INT_FX_H;
    UINT32 VPP_CPCB2_INT_FY_L;
    UINT32 VPP_CPCB2_INT_FY_H;
    UINT32 VPP_CPCB2_INT_V_S_L;
    UINT32 VPP_CPCB2_INT_V_S_H;
    UINT32 VPP_CPCB2_INT_V_E_L;
    UINT32 VPP_CPCB2_INT_V_E_H;
    UINT32 VPP_CPCB2_INT_H_S_L;
    UINT32 VPP_CPCB2_INT_H_S_H;
    UINT32 VPP_CPCB2_INT_H_E_L;
    UINT32 VPP_CPCB2_INT_H_E_H;
    UINT32 VPP_CPCB2_INT_VX_E_L;
    UINT32 VPP_CPCB2_INT_VX_E_H;
    UINT32 VPP_CPCB2_INT_VX_O_L;
    UINT32 VPP_CPCB2_INT_VX_O_H;
    UINT32 VPP_CPCB2_INT_YBL_L;
    UINT32 VPP_CPCB2_INT_YBL_H;
    UINT32 VPP_CPCB2_INT_CBL_L;
    UINT32 VPP_CPCB2_INT_CBL_H;
    UINT32 VPP_RA_Vpp_CPCB2_FLD;
} VPP_CPCB2_INT, *PVPP_CPCB2_INT;

#endif
#endif

