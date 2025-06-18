/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_stub.h
//! \brief      Stub/dummy DV IP interfaces
//!                 in vpp.h Firmware header files
//! \author     Neena Mathew
//-----------------------------------------------------------------------------

#ifndef __VPP_STUB_H__
#define __VPP_STUB_H__ (){}

#include "ctypes.h"

#pragma pack(1)
#ifdef __cplusplus
  extern "C" {
#endif

#ifndef _DOCC_H_BITOPS_
#define _DOCC_H_BITOPS_ (){}

    #define _bSETMASK_(b)                                      ((b)<32 ? (1<<((b)&31)) : 0)
    #define _NSETMASK_(msb,lsb)                                (_bSETMASK_((msb)+1)-_bSETMASK_(lsb))
    #define _bCLRMASK_(b)                                      (~_bSETMASK_(b))
    #define _NCLRMASK_(msb,lsb)                                (~_NSETMASK_(msb,lsb))
    #define _BFGET_(r,msb,lsb)                                 (_NSETMASK_((msb)-(lsb),0)&((r)>>(lsb)))
    #define _BFSET_(r,msb,lsb,v)                               do{ (r)&=_NCLRMASK_(msb,lsb); (r)|=_NSETMASK_(msb,lsb)&((v)<<(lsb)); }while(0)

#endif

//////
///
/// $INTERFACE DummyOVPReg                                (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (R-)
///               %unsigned 32 0x0
///                                    ###
///                                    * dummyOVP registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:       4B, bits:      32b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_DummyOVPReg
#define h_DummyOVPReg (){}

    #define     BA_DummyOVPReg_0x0                               0x0000
    #define     B16DummyOVPReg_0x0                               0x0000
    #define   LSb32DummyOVPReg_0x0                                  0
    #define   LSb16DummyOVPReg_0x0                                  0
    #define       bDummyOVPReg_0x0                               32
    #define   MSK32DummyOVPReg_0x0                                  0xFFFFFFFF
    ///////////////////////////////////////////////////////////

    typedef struct SIE_DummyOVPReg {
    ///////////////////////////////////////////////////////////
    #define   GET32DummyOVPReg_0x0(r32)                          _BFGET_(r32,31, 0)
    #define   SET32DummyOVPReg_0x0(r32,v)                        _BFSET_(r32,31, 0,v)

            UNSG32 u_0x0                                       : 32;
    ///////////////////////////////////////////////////////////
    } SIE_DummyOVPReg;

    ///////////////////////////////////////////////////////////
     SIGN32 DummyOVPReg_drvrd(SIE_DummyOVPReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DummyOVPReg_drvwr(SIE_DummyOVPReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DummyOVPReg_reset(SIE_DummyOVPReg *p);
     SIGN32 DummyOVPReg_cmp  (SIE_DummyOVPReg *p, SIE_DummyOVPReg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DummyOVPReg_check(p,pie,pfx,hLOG) DummyOVPReg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DummyOVPReg_print(p,    pfx,hLOG) DummyOVPReg_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: DummyOVPReg
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_REG0             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 DummyOVP
///               $DummyOVPReg           DummyOVP            REG    [37]
///                                    ###
///                                    * dummyOVP registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     116B, bits:     714b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_REG0
#define h_VPP_REG0 (){}

    #define     RA_VPP_REG0_DummyOVP                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_REG0 {
    ///////////////////////////////////////////////////////////
              SIE_DummyOVPReg                                    ie_DummyOVP[37];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_REG0;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_REG0_drvrd(SIE_VPP_REG0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_REG0_drvwr(SIE_VPP_REG0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_REG0_reset(SIE_VPP_REG0 *p);
     SIGN32 VPP_REG0_cmp  (SIE_VPP_REG0 *p, SIE_VPP_REG0 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_REG0_check(p,pie,pfx,hLOG) VPP_REG0_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_REG0_print(p,    pfx,hLOG) VPP_REG0_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_REG1
////////////////////////////////////////////////////////////
//////
///
/// $INTERFACE VPP_REG1             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 DummyOVP
///               $DummyOVPReg           DummyOVP            REG    [2]
///                                    ###
///                                    * dummyOVP registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     116B, bits:     714b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_REG1
#define h_VPP_REG1 (){}

    #define     RA_VPP_REG1_DummyOVP                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_REG1 {
    ///////////////////////////////////////////////////////////
              SIE_DummyOVPReg                                    ie_DummyOVP[2];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_REG1;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_REG1_drvrd(SIE_VPP_REG0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_REG1_drvwr(SIE_VPP_REG0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_REG1_reset(SIE_VPP_REG0 *p);
     SIGN32 VPP_REG1_cmp  (SIE_VPP_REG0 *p, SIE_VPP_REG0 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_REG1_check(p,pie,pfx,hLOG) VPP_REG0_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_REG1_print(p,    pfx,hLOG) VPP_REG0_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_REG1
////////////////////////////////////////////////////////////

#ifdef __cplusplus
  }
#endif
#pragma  pack()

#endif //__VPP_STUB_H__
//////
/// ENDOFFILE: vpp_stub.h
////////////////////////////////////////////////////////////

