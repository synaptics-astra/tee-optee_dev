/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef _VPP_REGS_H_
#define _VPP_REGS_H_

/***************************************************************
* DESCR: get the one byte of a unsigned int
* PARAMS: uint32 - an unsigned int
****************************************************************/
#define GET_LOWEST_BYTE(uint32) ((uint32) & 0xFF)

#define GET_SECOND_BYTE(uint32) (((uint32) & 0xFF00) >> 8)

#define GET_THIRD_BYTE(uint32) (((uint32) & 0xFF0000) >> 16)

/***************************************************************
* DESCR: load register default value by name
* PARAMS: RegName - register name in maddr.h
*               DefValues - register default value
****************************************************************/
#define WRITE_REG_DEFAULT_VAL(RegName, DefValues)\
do{\
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + (RegName << 2), DefValues.VPP_##RegName);\
}while(0)

#define WRITE_REG_DEFAULT_VAL_32BITS(RegName, DefValues)\
do{\
    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + RegName, DefValues.VPP_##RegName);\
}while(0)


/*Enum for maintaining CPCB 0,1 and 2 at Backend*/
 typedef enum VPP_CPCB_UNIT_NUM/////////////////////////////////temp put here
 {
     VPP_CPCB0	= 0,
     VPP_CPCB1  = 1,
     VPP_VOP_TG = 2,
     VPP_VOP2_TG = 3,
     VPP_CPCB_MAX
 }VPP_CPCB_UNIT_NUM, *PVPP_CPCB_UNIT_NUM;

#endif
