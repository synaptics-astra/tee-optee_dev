/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_FRC_SCL_PRV_H_
#define _VPP_FRC_SCL_PRV_H_

#include "vpp.h"
#include "com_type.h"

/**********************************************************************
*macro definitions for FRC scaler                                                                           *
***********************************************************************/

/*register number in a FRC_SCL unit, neglect those useless*/
#define FRC_SCL_REG_NUM 73//register number in local buffer

/*maximum address in the line buffer*/
#define VPP_SCL_LINE_BUF_ADDR_MAX 1279


/*scaler tap number restrictions*/
#define VPP_SCL_HTAP_NUM_MIN 1
#define VPP_SCL_HTAP_NUM_MAX 8
#define VPP_SCL_VTAP_NUM_MIN 1
#define VPP_SCL_VTAP_NUM_MAX 6

/*max allowable expension ratio in non linear*/
#define VPP_SCL_MAX_EXP 4
/*max allowable reduction ratio in non linear*/
#define VPP_SCL_MAX_RED 3

/*CSC coefficient and offset number*/
#define VPP_SCL_MAX_COL_COEFF_INDEX   9
#define VPP_SCL_MAX_COL_OFFSET_INDEX  3

/*CPCB FRC_SCL unit number validation*/
#define CHECK_FRC_SCL_NUM(UnitNum)\
do{\
    if((UnitNum < VPP_FRC_SCL_MAIN) ||(UnitNum >= VPP_FRC_SCL_MAX))\
        return MV_VPP_EBADPARAM;\
}while(0)

#define CHECK_VPP_SCL_RETVAL(cmd)\
do{\
    uiRetVal = cmd;\
    if(MV_VPP_OK != uiRetVal)\
    return uiRetVal;\
}while(0)

#define WRITE_FRC_SCL_VAL(UnitNum, RegName, DefValues)\
do{\
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + ((FRC0_OFFSET + UnitNum * 0x100 + RegName) << 2), DefValues);\
}while(0)

/*load register default value by name*/
#define WRITE_FRC_SCL_DEFAULT_VAL(UnitNum, RegName, DefValues)\
do{\
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + ((FRC0_OFFSET + UnitNum * 0x100 + RegName) << 2), DefValues.VPP_##RegName);\
}while(0)

#define WRITE_FRC_SCL_DEFAULT_VAL_32BITS(RegName, DefValues)\
do{\
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RegName, DefValues.VPP_##RegName);\
}while(0)


#define VPP_BCMBUF_Write_DBG(XX,YY,ZZ) VPP_BCMBUF_Write(XX,YY,ZZ)

#endif

