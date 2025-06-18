/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_CPCB_TG_PRV_H_
#define _VPP_CPCB_TG_PRV_H_

#include "com_type.h"

/**********************************************************************
*macro for CPCB timing generator                                                                         *
***********************************************************************/

/*CPCB unit number validation*/
#define CHECK_CPCB_NUM(UnitNum)\
do{\
    if((UnitNum < VPP_CPCB0) || (UnitNum >= VPP_CPCB_MAX))\
        return MV_VPP_EBADPARAM;\
}while(0)

/**********************************************************************
*bit field definition for CPCB INT                                                                            *
***********************************************************************/

/*CPCB clock control Register Bit Fields */
typedef union VPP_CPCB_CLK_CTRL_T
{
    struct {
        UINT32 ClkSwitch             : 1;
        UINT32 ClkD3Switch           : 1;
        UINT32 ClkEn                 : 1;
        UINT32 ClkSel                : 3;
        UINT32 Reserved              :26;
    } field;

    UINT32 data;
} VPP_CPCB_CLK_CTRL;

#endif
