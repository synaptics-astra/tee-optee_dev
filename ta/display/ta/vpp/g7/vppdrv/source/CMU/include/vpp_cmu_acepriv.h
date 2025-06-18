/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#ifndef __BDRACEPRIV__
#define __BDRACEPRIV__

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */
#define CMU_ACE_NUM_UNITS 2

#define CMU_ACE_REG_NUM 21

#define ACE_BASE_OFFSET 0x0
#define ACE_WF1_OFFSET  0x4
#define ACE_WF2_OFFSET  0x5
#define ACE_WF3_OFFSET  0x6

#define ACE_MS1_OFFSET  0x8
#define ACE_MS2_OFFSET  0x9
#define ACE_MS3_OFFSET  0xa

#define ACE_TH0_OFFSET  0xc
#define ACE_TH1_OFFSET  0xd
#define ACE_TH2_OFFSET  0xe
#define ACE_TH3_OFFSET  0xf
#define ACE_TH4_OFFSET  0x10
#define ACE_TH5_OFFSET  0x11

#define ACE_OP0_OFFSET  0x12
#define ACE_OP5_OFFSET  0x14

#define ACE_GB2_OFFSET  0x16
#define ACE_GB3_OFFSET  0x17

/*
 * BCM Buffer write (for Main and Pip)
 */
#define ACE_REG_ADDR_WRITE(UnitID,Offset,Val,RetVal) \
    if (UnitID == VPP_MAIN) { \
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,((vpp_obj->base_addr) + \
                    ((CMU_ACE_M_EN + Offset) << 2)), Val);\
        if (RetVal != MV_VPP_OK) return RetVal;\
    } else if (UnitID == VPP_PIP) {\
        RetVal = VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf,((vpp_obj->base_addr) + \
                    ((CMU_ACE_P_EN + Offset) << 2)), Val);\
        if (RetVal != MV_VPP_OK) return RetVal;\
    }

/*-----------------------------------------------------------------------------
 * Static
 *-----------------------------------------------------------------------------
 */
static VPP_CMU_INPUT_SIGNAL_TYPE CmuAceInputType[CMU_ACE_NUM_UNITS];

/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */

/* Ace Enable Register */
struct tagCMU_ACE_EN_REGISTER_BITS
{
    REG_WIDTH en           :  1;    // Enable/Disable Ace
    REG_WIDTH cc           :  3;    // color compensation level value
    REG_WIDTH crush        :  1;
} PACKED_STRUCT;

typedef volatile struct tagCMU_ACE_EN_REGISTER_BITS EnableAce;
typedef volatile struct tagCMU_ACE_EN_REGISTER_BITS *pointEnableAce;

union tagAVC_EN_REG
{
    EnableAce EnAce;
    REG_WIDTH Value;
};

typedef volatile union tagAVC_EN_REG AceEnVal;
static AceEnVal InstEnableAce;

/*-----------------------------------------------------------------------------
 * Get/Set Functions
 *-----------------------------------------------------------------------------
 */

/*
 * ACE Enable
 */
#define HwiCmuAceSetEnable(UnitID,Val,RetVal) \
{ \
    InstEnableAce.EnAce.en = Val; \
    ACE_REG_ADDR_WRITE(UnitID,ACE_BASE_OFFSET,(InstEnableAce.Value),RetVal); \
}

/*
 * To program level of color compensation
 */
#define HwiCmuAceSetColorCompLevel(UnitID,Val,RetVal) \
{ \
    InstEnableAce.EnAce.cc = (Val); \
    ACE_REG_ADDR_WRITE(UnitID,ACE_BASE_OFFSET,(InstEnableAce.Value),RetVal); \
}

#define HwiCmuAceCrush(UnitID,Val) \
{\
    InstEnableAce.EnAce.crush = (Val); \
    ACE_REG_ADDR_WRITE(UnitID,ACE_BASE_OFFSET,(InstEnableAce.Value),RetVal); \
}

/*
 * Weighting factor
 */
#define HwiCmuAceSetWf1(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_WF1_OFFSET,Val,RetVal); \
}

#define HwiCmuAceSetWf2(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_WF2_OFFSET,Val,RetVal); \
}

#define HwiCmuAceSetWf3(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_WF3_OFFSET,Val,RetVal); \
}

/*
 * ACE threshold 0,1,2,3,4,5
 */
#define HwiCmuAceSetTh0(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_TH0_OFFSET,Val,RetVal); \
}

#define HwiCmuAceSetTh1(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_TH1_OFFSET,Val,RetVal); \
}

#define HwiCmuAceSetTh2(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_TH2_OFFSET,Val,RetVal); \
}

#define HwiCmuAceSetTh3(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_TH3_OFFSET,Val,RetVal); \
}

#define HwiCmuAceSetTh4(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_TH4_OFFSET,Val,RetVal); \
}

#define HwiCmuAceSetTh5(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_TH5_OFFSET,Val,RetVal); \
}

/*
 * ACE output luma value for pixels below threshold 0
 */
#define HwiCmuAceSetOp0(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID, ACE_OP0_OFFSET,(Val & 0xFF),RetVal); \
    ACE_REG_ADDR_WRITE(UnitID, ACE_OP0_OFFSET + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * ACE output luma value for pixels above threshold 5
 */
#define HwiCmuAceSetOp5(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_OP5_OFFSET,(Val & 0xFF),RetVal); \
    ACE_REG_ADDR_WRITE(UnitID,ACE_OP5_OFFSET + 1,((Val >> 8) & 0xFF),RetVal); \
}

/*
 * This register is used to program the guard band region
 * between dark and mid regions in terms of luma range
 */
#define HwiCmuAceSetGb2(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_GB2_OFFSET,Val,RetVal); \
}

#define HwiCmuAceSetGb3(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_GB3_OFFSET,Val,RetVal); \
}

/*
*To program the maximum slope of the luma output curve in the dark region of the  Channel.
*/
#define HwiCmuAceSetMs1(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_MS1_OFFSET,Val,RetVal); \
}

/*
*To program the maximum slope of the luma output curve in the mid region of the Channel.
*/
#define HwiCmuAceSetMs2(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_MS2_OFFSET,Val,RetVal); \
}

/*
*To program the maximum slope of the luma output curve in the Bright region of the Channel.
*/
#define HwiCmuAceSetMs3(UnitID,Val,RetVal) \
{ \
    ACE_REG_ADDR_WRITE(UnitID,ACE_MS3_OFFSET,Val,RetVal); \
}

#endif  /* __BDRACEPRIV__ */

