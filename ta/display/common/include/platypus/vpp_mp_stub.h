/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_mp_stub.h
//! \brief      Stub/dummy NGPTV/VMX watermark IP interfaces
//!                 in vpp_mp.h Firmware header files
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef __VPP_MP_STUB_H__
#define __VPP_MP_STUB_H__ (){}

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
/// $INTERFACE Dummy0Reg                                (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (R-)
///               %unsigned 32 0x0
///                                    ###
///                                    * dummy0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:       4B, bits:      32b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_Dummy0Reg
#define h_Dummy0Reg (){}

    #define     BA_Dummy0Reg_0x0                               0x0000
    #define     B16Dummy0Reg_0x0                               0x0000
    #define   LSb32Dummy0Reg_0x0                                  0
    #define   LSb16Dummy0Reg_0x0                                  0
    #define       bDummy0Reg_0x0                               32
    #define   MSK32Dummy0Reg_0x0                                  0xFFFFFFFF
    ///////////////////////////////////////////////////////////

    typedef struct SIE_Dummy0Reg {
    ///////////////////////////////////////////////////////////
    #define   GET32Dummy0Reg_0x0(r32)                          _BFGET_(r32,31, 0)
    #define   SET32Dummy0Reg_0x0(r32,v)                        _BFSET_(r32,31, 0,v)

            UNSG32 u_0x0                                       : 32;
    ///////////////////////////////////////////////////////////
    } SIE_Dummy0Reg;

    ///////////////////////////////////////////////////////////
     SIGN32 Dummy0Reg_drvrd(SIE_Dummy0Reg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 Dummy0Reg_drvwr(SIE_Dummy0Reg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void Dummy0Reg_reset(SIE_Dummy0Reg *p);
     SIGN32 Dummy0Reg_cmp  (SIE_Dummy0Reg *p, SIE_Dummy0Reg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define Dummy0Reg_check(p,pie,pfx,hLOG) Dummy0Reg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define Dummy0Reg_print(p,    pfx,hLOG) Dummy0Reg_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: Dummy0Reg
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_MP_REG0             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 Dummy0
///               $Dummy0Reg           Dummy0            REG    [29]
///                                    ###
///                                    * dummy0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     116B, bits:     714b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_MP_REG0
#define h_VPP_MP_REG0 (){}

    #define     RA_VPP_MP_REG0_Dummy0                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_MP_REG0 {
    ///////////////////////////////////////////////////////////
              SIE_Dummy0Reg                                    ie_Dummy0[29];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_MP_REG0;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_MP_REG0_drvrd(SIE_VPP_MP_REG0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_MP_REG0_drvwr(SIE_VPP_MP_REG0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_MP_REG0_reset(SIE_VPP_MP_REG0 *p);
     SIGN32 VPP_MP_REG0_cmp  (SIE_VPP_MP_REG0 *p, SIE_VPP_MP_REG0 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_MP_REG0_check(p,pie,pfx,hLOG) VPP_MP_REG0_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_MP_REG0_print(p,    pfx,hLOG) VPP_MP_REG0_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_MP_REG0
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_MP_REG1             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 Dummy1
///               $Dummy1Reg           Dummy1            REG    [56]
///                                    ###
///                                    * dummy0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     224B, bits:    1521b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_MP_REG1
#define h_VPP_MP_REG1 (){}

    #define     RA_VPP_MP_REG1_Dummy1                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_MP_REG1 {
    ///////////////////////////////////////////////////////////
              SIE_Dummy0Reg                                    ie_Dummy0[56];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_MP_REG1;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_MP_REG1_drvrd(SIE_VPP_MP_REG1 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_MP_REG1_drvwr(SIE_VPP_MP_REG1 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_MP_REG1_reset(SIE_VPP_MP_REG1 *p);
     SIGN32 VPP_MP_REG1_cmp  (SIE_VPP_MP_REG1 *p, SIE_VPP_MP_REG1 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_MP_REG1_check(p,pie,pfx,hLOG) VPP_MP_REG1_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_MP_REG1_print(p,    pfx,hLOG) VPP_MP_REG1_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_MP_REG1
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_MP_REG2             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 Dummy1
///               $Dummy1Reg           Dummy1            REG    [44]
///                                    ###
///                                    * dummy0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     176B, bits:    1002b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_MP_REG2
#define h_VPP_MP_REG2 (){}

    #define     RA_VPP_MP_REG2_Dummy1                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_MP_REG2 {
    ///////////////////////////////////////////////////////////
              SIE_Dummy0Reg                                    ie_Dummy0[44];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_MP_REG2;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_MP_REG2_drvrd(SIE_VPP_MP_REG2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_MP_REG2_drvwr(SIE_VPP_MP_REG2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_MP_REG2_reset(SIE_VPP_MP_REG2 *p);
     SIGN32 VPP_MP_REG2_cmp  (SIE_VPP_MP_REG2 *p, SIE_VPP_MP_REG2 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_MP_REG2_check(p,pie,pfx,hLOG) VPP_MP_REG2_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_MP_REG2_print(p,    pfx,hLOG) VPP_MP_REG2_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_MP_REG2
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_MP_REG3             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 Dummy1
///               $Dummy1Reg           Dummy1            REG    [6]
///                                    ###
///                                    * dummy0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:      24B, bits:     119b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_MP_REG3
#define h_VPP_MP_REG3 (){}

    #define     RA_VPP_MP_REG3_Dummy1                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_MP_REG3 {
    ///////////////////////////////////////////////////////////
              SIE_Dummy0Reg                                    ie_Dummy0[6];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_MP_REG3;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_MP_REG3_drvrd(SIE_VPP_MP_REG3 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_MP_REG3_drvwr(SIE_VPP_MP_REG3 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_MP_REG3_reset(SIE_VPP_MP_REG3 *p);
     SIGN32 VPP_MP_REG3_cmp  (SIE_VPP_MP_REG3 *p, SIE_VPP_MP_REG3 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_MP_REG3_check(p,pie,pfx,hLOG) VPP_MP_REG3_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_MP_REG3_print(p,    pfx,hLOG) VPP_MP_REG3_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_MP_REG3
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_MP_REG4             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 Dummy1
///               $Dummy1Reg           Dummy1            REG    [169]
///                                    ###
///                                    * dummy0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     676B, bits:    3734b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_MP_REG4
#define h_VPP_MP_REG4 (){}

    #define     RA_VPP_MP_REG4_Dummy1                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_MP_REG4 {
    ///////////////////////////////////////////////////////////
              SIE_Dummy0Reg                                    ie_Dummy0[169];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_MP_REG4;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_MP_REG4_drvrd(SIE_VPP_MP_REG4 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_MP_REG4_drvwr(SIE_VPP_MP_REG4 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_MP_REG4_reset(SIE_VPP_MP_REG4 *p);
     SIGN32 VPP_MP_REG4_cmp  (SIE_VPP_MP_REG4 *p, SIE_VPP_MP_REG4 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_MP_REG4_check(p,pie,pfx,hLOG) VPP_MP_REG4_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_MP_REG4_print(p,    pfx,hLOG) VPP_MP_REG4_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_MP_REG4
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_MP_REG5             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 Dummy1
///               $Dummy1Reg           Dummy1            REG    [11]
///                                    ###
///                                    * dummy0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:      44B, bits:     274b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_MP_REG5
#define h_VPP_MP_REG5 (){}

    #define     RA_VPP_MP_REG5_Dummy1                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_MP_REG5 {
    ///////////////////////////////////////////////////////////
              SIE_Dummy0Reg                                    ie_Dummy0[11];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_MP_REG5;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_MP_REG5_drvrd(SIE_VPP_MP_REG5 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_MP_REG5_drvwr(SIE_VPP_MP_REG5 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_MP_REG5_reset(SIE_VPP_MP_REG5 *p);
     SIGN32 VPP_MP_REG5_cmp  (SIE_VPP_MP_REG5 *p, SIE_VPP_MP_REG5 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_MP_REG5_check(p,pie,pfx,hLOG) VPP_MP_REG5_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_MP_REG5_print(p,    pfx,hLOG) VPP_MP_REG5_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_MP_REG5
////////////////////////////////////////////////////////////


#ifdef __cplusplus
  }
#endif
#pragma  pack()

#endif //__VPP_MP_STUB_H__
//////
/// ENDOFFILE: vpp_mp_stub.h
////////////////////////////////////////////////////////////

