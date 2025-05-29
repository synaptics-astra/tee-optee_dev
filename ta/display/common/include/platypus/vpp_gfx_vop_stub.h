/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_gfx_vop_stub.h
//! \brief      Stub/dummy DV IP interfaces
//!                 in vpp_gfx_vop.h Firmware header files
//! \author     Neena Mathew
//-----------------------------------------------------------------------------

#ifndef __VPP_GFX_VOP_STUB_H__
#define __VPP_GFX_VOP_STUB_H__ (){}

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
/// $INTERFACE DummyGfx0Reg                                (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (R-)
///               %unsigned 32 0x0
///                                    ###
///                                    * dummyGfx0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:       4B, bits:      32b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_DummyGfx0Reg
#define h_DummyGfx0Reg (){}

    #define     BA_DummyGfx0Reg_0x0                               0x0000
    #define     B16DummyGfx0Reg_0x0                               0x0000
    #define   LSb32DummyGfx0Reg_0x0                                  0
    #define   LSb16DummyGfx0Reg_0x0                                  0
    #define       bDummyGfx0Reg_0x0                               32
    #define   MSK32DummyGfx0Reg_0x0                                  0xFFFFFFFF
    ///////////////////////////////////////////////////////////

    typedef struct SIE_DummyGfx0Reg {
    ///////////////////////////////////////////////////////////
    #define   GET32DummyGfx0Reg_0x0(r32)                          _BFGET_(r32,31, 0)
    #define   SET32DummyGfx0Reg_0x0(r32,v)                        _BFSET_(r32,31, 0,v)

            UNSG32 u_0x0                                       : 32;
    ///////////////////////////////////////////////////////////
    } SIE_DummyGfx0Reg;

    ///////////////////////////////////////////////////////////
     SIGN32 DummyGfx0Reg_drvrd(SIE_DummyGfx0Reg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DummyGfx0Reg_drvwr(SIE_DummyGfx0Reg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DummyGfx0Reg_reset(SIE_DummyGfx0Reg *p);
     SIGN32 DummyGfx0Reg_cmp  (SIE_DummyGfx0Reg *p, SIE_DummyGfx0Reg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DummyGfx0Reg_check(p,pie,pfx,hLOG) DummyGfx0Reg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DummyGfx0Reg_print(p,    pfx,hLOG) DummyGfx0Reg_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: DummyGfx0Reg
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_GFX_VOP_REG0             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 DummyGfx0
///               $DummyGfx0Reg           DummyGfx0            REG    [37]
///                                    ###
///                                    * dummyGfx0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     176B, bits:    1002b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_GFX_VOP_REG0
#define h_VPP_GFX_VOP_REG0 (){}

    #define     RA_VPP_GFX_VOP_REG0_DummyGfx0                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_GFX_VOP_REG0 {
    ///////////////////////////////////////////////////////////
              SIE_DummyGfx0Reg                                    ie_DummyGfx0[37];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_GFX_VOP_REG0;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_GFX_VOP_REG0_drvrd(SIE_VPP_GFX_VOP_REG0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_GFX_VOP_REG0_drvwr(SIE_VPP_GFX_VOP_REG0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_GFX_VOP_REG0_reset(SIE_VPP_GFX_VOP_REG0 *p);
     SIGN32 VPP_GFX_VOP_REG0_cmp  (SIE_VPP_GFX_VOP_REG0 *p, SIE_VPP_GFX_VOP_REG0 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_GFX_VOP_REG0_check(p,pie,pfx,hLOG) VPP_GFX_VOP_REG0_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_GFX_VOP_REG0_print(p,    pfx,hLOG) VPP_GFX_VOP_REG0_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_GFX_VOP_REG0
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_GFX_VOP_REG1             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 DummyGfx1
///               $DummyGfx0Reg           DummyGfx0            REG    [1]
///                                    ###
///                                    * dummyGfx0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     176B, bits:    1002b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_GFX_VOP_REG1
#define h_VPP_GFX_VOP_REG1 (){}

    #define     RA_VPP_GFX_VOP_REG1_DummyGfx0                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_GFX_VOP_REG1 {
    ///////////////////////////////////////////////////////////
              SIE_DummyGfx0Reg                                    ie_DummyGfx0[1];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_GFX_VOP_REG1;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_GFX_VOP_REG1_drvrd(SIE_VPP_GFX_VOP_REG1 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_GFX_VOP_REG1_drvwr(SIE_VPP_GFX_VOP_REG1 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_GFX_VOP_REG1_reset(SIE_VPP_GFX_VOP_REG1 *p);
     SIGN32 VPP_GFX_VOP_REG1_cmp  (SIE_VPP_GFX_VOP_REG1 *p, SIE_VPP_GFX_VOP_REG1 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_GFX_VOP_REG1_check(p,pie,pfx,hLOG) VPP_GFX_VOP_REG1_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_GFX_VOP_REG1_print(p,    pfx,hLOG) VPP_GFX_VOP_REG1_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_GFX_VOP_REG1
////////////////////////////////////////////////////////////

//////
///
/// $INTERFACE VPP_GFX_VOP_REG2             biu              (4,4)
///     # # ----------------------------------------------------------
///     @ 0x00000                      (P)
///     # 0x00000 DummyGfx2
///               $DummyGfx0Reg           DummyGfx0            REG    [2]
///                                    ###
///                                    * dummyGfx0 registers
///                                    ###
///     # # ----------------------------------------------------------
/// $ENDOFINTERFACE  # size:     176B, bits:    1002b, padding:     0B
////////////////////////////////////////////////////////////
#ifndef h_VPP_GFX_VOP_REG2
#define h_VPP_GFX_VOP_REG2 (){}

    #define     RA_VPP_GFX_VOP_REG2_DummyGfx0                          0x0000
    ///////////////////////////////////////////////////////////

    typedef struct SIE_VPP_GFX_VOP_REG2 {
    ///////////////////////////////////////////////////////////
              SIE_DummyGfx0Reg                                    ie_DummyGfx0[2];
    ///////////////////////////////////////////////////////////
    } SIE_VPP_GFX_VOP_REG2;

    ///////////////////////////////////////////////////////////
     SIGN32 VPP_GFX_VOP_REG2_drvrd(SIE_VPP_GFX_VOP_REG2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPP_GFX_VOP_REG2_drvwr(SIE_VPP_GFX_VOP_REG2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPP_GFX_VOP_REG2_reset(SIE_VPP_GFX_VOP_REG2 *p);
     SIGN32 VPP_GFX_VOP_REG2_cmp  (SIE_VPP_GFX_VOP_REG2 *p, SIE_VPP_GFX_VOP_REG2 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPP_GFX_VOP_REG2_check(p,pie,pfx,hLOG) VPP_GFX_VOP_REG2_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPP_GFX_VOP_REG2_print(p,    pfx,hLOG) VPP_GFX_VOP_REG2_cmp(p,0,  pfx,(void*)(hLOG),0,0)

#endif
//////
/// ENDOFINTERFACE: VPP_GFX_VOP_REG2
////////////////////////////////////////////////////////////

#ifdef __cplusplus
  }
#endif
#pragma  pack()

#endif //__VPP_GFX_VOP_STUB_H__
//////
/// ENDOFFILE: vpp_gfx_vop_stub.h
////////////////////////////////////////////////////////////

