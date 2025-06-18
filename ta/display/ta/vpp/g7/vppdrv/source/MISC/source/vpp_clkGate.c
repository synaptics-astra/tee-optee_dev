/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_clkGate.c
//! \brief      The clock gate & SRAM power gate framework source file
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#define _VPP_CLKGATE_C_
#include "Galois_memmap.h"
#include "vpp_module.h"
#include "vpp_clkGate.h"
#include "vpp.h"
#include "avioGbl.h"
#include "vpp_api.h"
#include "vpp_clkGate_regmap.h"
#include "vpp_frc_scl.h"
#include "vpp_frc_scl_prv.h"
#include "vpp_vmx_apifuncs.h"
#include "vpp_dv_api.h"

#ifdef CONFIG_VPP_VIP_HDMIRX_CLOCKGATE
#undef CONFIG_VPP_VIP_HDMIRX_CLOCKGATE
#endif //CONFIG_VPP_VIP_HDMIRX_CLOCKGATE

#ifdef VPP_DIAGS
#define VPP_REG_WORD32_WRITE(addr, val) GA_REG_WORD32_WRITE(addr, val)
#define VPP_REG_WORD32_READ(addr, val)  GA_REG_WORD32_READ(addr, val)
#endif

//Enable following macro for CLKGATE debug prints, Default=Disable
#define VPP_CLKGATE_DEBUG_ENABLE

#ifdef VPP_CLKGATE_DEBUG_ENABLE
#define VPP_CLKGATE_DEBUG_PRINT(...)            VPP_CLKGATE_PRINT(__VA_ARGS__)
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_DEINT_ENABLE)
#ifdef VPP_FE_DEINT_POWERGATE_ENABLE
#define VPP_CLKGATE_DEBUG_PRINT_DYNAMIC(...)    { if(dynamicDebugPrintEnable) VPP_CLKGATE_PRINT(__VA_ARGS__); }
#endif
#endif
#else
#define VPP_CLKGATE_DEBUG_PRINT(...)
#define VPP_CLKGATE_DEBUG_PRINT_DYNAMIC(...)
#endif

/* Enable, if direct register write required for power gating,
 *  default: disable */
//#define VPP_CLKGATE_ENABLE_DIRECT_WRITE

/* Enable, if all power down done though setBCM and powerup through clearBCM
 *  default: disable, writing to a powerdown RAM only fails & may not crash system
 *         : i.e use only clearBCM buffer for both power down and power up
 */
//#define VPP_CLKGATE_ENABLE_SEPARATE_BCM_WRITE

#ifndef VPP_CLKGATE_ENABLE_SEPARATE_BCM_WRITE
//Enable Direct write along with BCM write to allow immediate effect of power up
//#define VPP_CLKGATE_ENABLE_BCM_AND_DIRECT_WRITE
#endif

//Enable all the possible clock gating at the initial time, Enable bu default
#define CONFIG_VPP_ENABLE_ALL_CLKGATE_AT_INITTIME

//Enable detailed logging whenever clk/pwr gate bit is enabled/disabled
//#define CONFIG_VPP_ENABLE_DEBUG_OPT_AT_INITTIME

//Enable listing the clk/pwr register,value details along with signature dump
#define VPP_CLKGATE_CLKPWR_REG_DUMP_OPTION_ENABLE   0x01
#define VPP_CLKGATE_CLKPWR_REG_DUMP_OPTION_DISABLE  0x00
#define VPP_CLKGATE_CLKPWR_REG_DUMP_OPTION          VPP_CLKGATE_CLKPWR_REG_DUMP_OPTION_ENABLE

//dummy  variable : TBR
UINT32 g_clkgate_dummyvar;

//Enable delayed processing, by default
#define CONFIG_VPP_CLKGATE_DELAYED_PROCESSING

#ifdef CONFIG_VPP_CLKGATE_DELAYED_PROCESSING
static VPP_CLKGATE_OPT delayedOptions = {0};
static VPP_CLKGATE_OPT delayedOptionsBackup = {0};
#endif //CONFIG_VPP_CLKGATE_DELAYED_PROCESSING

//Default value for dynamic debug print is disable(0)
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_DEINT_ENABLE)
#ifdef VPP_FE_DEINT_POWERGATE_ENABLE
static UINT32 dynamicDebugPrintEnable = 1;
#endif
#endif

#define SRAM_POWER_CONTROL(RegAdrr, State)  ( { \
    T32SRAMPWR_ctrl stSramPwrCtrl; \
        GA_REG_WORD32_READ(RegAdrr, &stSramPwrCtrl.u32); \
        stSramPwrCtrl.uctrl_SD = stSramPwrCtrl.uctrl_DSLP = stSramPwrCtrl.uctrl_SLP = State; \
        GA_REG_WORD32_WRITE(RegAdrr, stSramPwrCtrl.u32); \
})

#define SRAMPWR_CTRTL_REG(MODULE_REG_BASE, REG_OFFSET) \
        MEMMAP_AVIO_REG_BASE + MODULE_REG_BASE + REG_OFFSET + RA_SRAMPWR_ctrl

//Enable/Disable delayed SCL PWR gateing
//#define VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE

#ifdef VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE

//The delay specified in number of ISR count
#define VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE_DELAY    5
static UINT8 vppSclPwrGateDelayCount = 0;
static UINT8 vppSclPwrGateEnabledDelayedOperation = 0;
static UINT8 vppSclPwrGateFlagChanged[VPP_FRC_SCL_MAX];
static ENUM_VPP_SRAM_PWRGATE vppSclPwrGateFlag[VPP_FRC_SCL_MAX];
static void VPP_ClkGate_Register_DelayOperation_SclPwrGate(VPP_FRC_SCL_NUM UnitNum, ENUM_VPP_SRAM_PWRGATE powerGateFlag)
{
#ifdef VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE
    if(!vppSclPwrGateEnabledDelayedOperation) {
        //Record the delayed operation only if it is not delayed operation itself
        vppSclPwrGateFlag[UnitNum] = powerGateFlag;
        vppSclPwrGateFlagChanged[UnitNum] = 1;
        vppSclPwrGateDelayCount = VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE_DELAY;
    }
#endif //VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE
}

#endif //VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE

static void VPP_ClkGate_GetRegDump(VPP_OBJ *vpp_obj, int debugOption);
/**
 * VPP_ClkGate_VbiIsr_Callback() - Triggered/Callback from VBI ISR
 *    This function is an interface where any periodic operation required by CLKGATE
 *      module can be performed.
 **/
static void VPP_ClkGate_VbiIsr_Callback(VPP_OBJ *vpp_obj)
{
#ifdef VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE
    if(vppSclPwrGateDelayCount) {
        vppSclPwrGateDelayCount--;
        if(0 == vppSclPwrGateDelayCount) {
            VPP_FRC_SCL_NUM UnitNum = 0;
            //Enaled delayed operation
            vppSclPwrGateEnabledDelayedOperation = 1;
            for(;UnitNum<VPP_FRC_SCL_MAX;UnitNum++) {
                if(vppSclPwrGateFlagChanged[UnitNum]) {
                    //Apply the SCL power gating as recoreded earlier
                    VPP_SRAM_PowerGate_SCL(vpp_obj, UnitNum, vppSclPwrGateFlag[UnitNum]);
                    //Reset the change status
                    vppSclPwrGateFlagChanged[UnitNum] = 0;
                }
            }
            //Disable delayed operation
            vppSclPwrGateEnabledDelayedOperation = 0;
        }
    }
#endif //VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE
}

//Find the entry in the vppClkGateDetails for the given signature bit position
static int VPP_ClkGate_getClkGateNdx(VPP_OBJ *vpp_obj, int sigBitPos)
{
    const int vppClkGateCount = sizeof(vppClkGateDetails) / sizeof(VPP_CLKGATE_IP_DETAILS);
    int i;

    for(i = 0; i < vppClkGateCount; i++) {
        if(vppClkGateDetails[i].uiSignatureBitPos == sigBitPos) {
            break;
        }
    }

    return i;
}

void VPP_ClkGate_Enable(VPP_OBJ *vpp_obj, int option)
{
    vpp_obj->enClkGate = option;

#ifdef CONFIG_VPP_ENABLE_DEBUG_OPT_AT_INITTIME
    vpp_obj->enClkGate |= VPP_CLKGATE_ENABLE_OPT_DBG_CLKGATE;
    vpp_obj->enClkGate |= VPP_CLKGATE_ENABLE_OPT_DBG_PWRGATE;
#endif //CONFIG_VPP_ENABLE_DEBUG_OPT_AT_INITTIME
}

void VPP_PwrGate_InitialDisable(VPP_OBJ *vpp_obj, ENUM_VPP_SRAM_PWRGATE_BLK pwrGateBlk, ENUM_VPP_SRAM_PWRGATE isPowerDown)
{
#ifdef EN_VPP_SRAM_CLKGATE
    const int vppPwrGateCount = sizeof(vppPwrGateDetails) / sizeof(VPP_POWERGATE_DETAILS);
    int i,j;
    if(!IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, PWRGATE)) {
        return;
    }

    for(i=0;i<vppPwrGateCount;i++) {
        if(vppPwrGateDetails[i].pwrBlkId == pwrGateBlk) {
            int clkGateNdx = vppPwrGateDetails[i].signatureBitPos;
            int powerGateIter;
            int tmpClkGateNdx;


            if(vppPwrGateDetails[i].bitMaskIter & VPP_POWERGATE_BITMASK_USEBIT) {
                //If using bitmask then all powergate bits are in sequence, so power gate value can be determined in single iteration
                powerGateIter = 2;  //Check the number of bits enabled in VPP_POWERGATE_BITMASK
            } else {
                //If not using bitmask then each powergate bits need to be iterated to get power gate value in multiple iteration
                powerGateIter = vppPwrGateDetails[i].bitMaskIter;
            }
            for(j = i, tmpClkGateNdx=clkGateNdx; j<i+powerGateIter; j++, tmpClkGateNdx++) {
                VPP_ClkGate_EnableGating(vpp_obj, tmpClkGateNdx, isPowerDown?VPP_CLK_GATE_ENABLE:VPP_CLK_GATE_DISABLE, 0 );
            }
        }
    }
#endif
}

static int VPP_ClkGate_IsClkGateExcluded(int clkGateNdx) {
    int excludedFlag = 0;

    if((clkGateNdx>=17) && (clkGateNdx<21)) {
        excludedFlag = 1;
    }
#ifndef CONFIG_VPP_ENABLE_SCL_CLKGATE
{
        if((clkGateNdx>=0) && (clkGateNdx<=4)) {
            excludedFlag = 1;
        }
        if((clkGateNdx>=8) && (clkGateNdx<=13)) {
            excludedFlag = 1;
        }
        if((clkGateNdx>=14) && (clkGateNdx<=16)) {
            excludedFlag = 1;
        }
    }
#endif
    if((clkGateNdx>=125) && (clkGateNdx<=126)) {
        excludedFlag = 1;
    }
    if((clkGateNdx == 148)||(clkGateNdx == 150) || (clkGateNdx == 151)) {
        excludedFlag = 1;
    }
#ifndef CONFIG_VPP_VIP_CLOCKGATE
    if(clkGateNdx == 152){
        excludedFlag = 1;
    }
#endif
    return excludedFlag;
}
void VPP_ClkGate_Dolby(VPP_OBJ *vpp_obj) {
    int i = 0;
    const int dolby_start_index = 118, dolby_end_index = 128;
    //Dolby Clk gating
    for(i = dolby_start_index; i < dolby_end_index; i++) {
        if(!VPP_ClkGate_IsClkGateExcluded(i)) {
        VPP_ClkGate_EnableGating(vpp_obj, i, VPP_CLK_GATE_ENABLE, 0);
        }
    }
}

void VPP_ClkGate_NonDolbyNew(VPP_OBJ *vpp_obj)
{
    int i = 0;
    const int start_index = 143, end_index = 156;
    const int start_index1 = 165, end_index1 = 169;
    //New module (non dolby) clk gating
    for(i = start_index; i < end_index; i++) {
        if(!VPP_ClkGate_IsClkGateExcluded(i)) {
            VPP_ClkGate_EnableGating(vpp_obj, i, VPP_CLK_GATE_ENABLE, 0);
        }
    }
    for(i = start_index1; i < end_index1; i++) {
        if(!VPP_ClkGate_IsClkGateExcluded(i)) {
        VPP_ClkGate_EnableGating(vpp_obj, i, VPP_CLK_GATE_ENABLE, 0);
        }
    }
}

void VPP_ClkGate_Clk(VPP_OBJ *vpp_obj)
{
    int i = 0;
    int tmpClkGateNdx;
#ifdef CONFIG_VPP_VIP_HDMIRX_CLOCKGATE
    const int start_index = 179;
#else
    const int start_index = 180;
#endif
    const int end_index = 182;
    for(i = start_index; i <= end_index; i++) {
        if(!VPP_ClkGate_IsClkGateExcluded(i)) {
            tmpClkGateNdx = VPP_ClkGate_getClkGateNdx(vpp_obj, i);
            if(tmpClkGateNdx < MAX_CLK_GATE_SIG_COUNT()) {
                VPP_ClkGate_EnableGating(vpp_obj, tmpClkGateNdx, VPP_CLK_GATE_ENABLE, VPP_CLKGATE_OPTION_NO_SHADOW_REG );
            } else {
                VPP_CLKGATE_PRINT("%s:%d: FAILED: bitmask index = %d, clkgatendx = %d, max : %d\n", __func__,__LINE__,i,tmpClkGateNdx, sizeof(vppClkGateDetails) / sizeof(VPP_CLKGATE_IP_DETAILS));
            }
        }
    }
}

void VPP_ClkGate_InitialDisable(VPP_OBJ *vpp_obj)
{
    //Enable all the clk gate - at the begining
    const int nClkGate = 21;
    int i;
    for(i=0;i<nClkGate;i++) {
        if(!VPP_ClkGate_IsClkGateExcluded(i)) {
            VPP_ClkGate_EnableGating(vpp_obj, i, VPP_CLK_GATE_ENABLE, 0 );
        }
    }

    //Enable all the SRAM clock gate - at the begining
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAIN_DINT, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAIN_DINT_DMX, VPP_SRAM_PWRGATE_ENABLE);

#ifdef CONFIG_VPP_ENABLE_SCL_CLKGATE
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINSCL_LB, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINSCL_LUT, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_GFX0_LB, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_GFX0_LUT, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_GFX1_LB, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_GFX1_LUT, VPP_SRAM_PWRGATE_ENABLE);
#endif
    VPP_ClkGate_Dolby(vpp_obj);
    VPP_ClkGate_NonDolbyNew(vpp_obj);

    //Dolby Power Gate
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_DVCOMP, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_DBVIP_ICSC, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_DBVIP_CVM_MAIN, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_DBVIP_CVM_GFX0, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_DBVIP_CVM_GFX1, VPP_SRAM_PWRGATE_ENABLE);
    //VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_EL_UPS_420_422_SP, VPP_SRAM_PWRGATE_ENABLE);To be checked
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_C0, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_C1, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_Y0, VPP_SRAM_PWRGATE_ENABLE);
    VPP_PwrGate_InitialDisable(vpp_obj, VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_Y1, VPP_SRAM_PWRGATE_ENABLE);
    //VPP_PwrGate_InitialDisable(vpp_obj, , VPP_SRAM_PWRGATE_ENABLE);

    //Various clk disable
    VPP_ClkGate_Clk(vpp_obj);

}

#ifdef EN_VPP_DYNAMIC_CLKGATE
static void VPP_DV_ClkGate_Initialize(VPP_OBJ *vpp_obj)
{
    UINT32* pDVparam[DV_CLKG_LMT];
    VPP_DV_InitClkGate(vpp_obj, pDVparam);
#if defined(EN_VPP_SRAM_CLKGATE) && defined(EN_VPP_DOLBY_CLKGATE)
    vppClkGateDetails[106].pShadowRegister = vppClkGateDetails[107].pShadowRegister = vppClkGateDetails[108].pShadowRegister = pDVparam[0];
    vppClkGateDetails[109].pShadowRegister = vppClkGateDetails[110].pShadowRegister = vppClkGateDetails[111].pShadowRegister = pDVparam[0];
#ifdef ENABLE_PIP_GFX0_PATH
    vppClkGateDetails[112].pShadowRegister = vppClkGateDetails[113].pShadowRegister = vppClkGateDetails[114].pShadowRegister = pDVparam[1];
#endif
    vppClkGateDetails[115].pShadowRegister = vppClkGateDetails[116].pShadowRegister = vppClkGateDetails[117].pShadowRegister = pDVparam[2];
#endif
#ifdef EN_VPP_NEW_NON_DOLBY_CLKGATE
   vppClkGateDetails[119].pShadowRegister = vppClkGateDetails[120].pShadowRegister = pDVparam[0];
#ifdef ENABLE_PIP_GFX0_PATH
   vppClkGateDetails[121].pShadowRegister = vppClkGateDetails[122].pShadowRegister = pDVparam[1];
#endif
   vppClkGateDetails[123].pShadowRegister = vppClkGateDetails[124].pShadowRegister = pDVparam[2];
#endif
}
#endif

void VPP_ClkGate_Initialize(VPP_OBJ *vpp_obj)
{
//Note: Reset()->LoadDefaultVal() invoked before clkgate init is invoked
//i.e. each module is given chance to disable/clkgate its own module
//       and VPP_ClkGate_InitialDisable is final place to do any missed out modules

#ifdef EN_VPP_DYNAMIC_CLKGATE
    UINT32 RegAddr;

    //Initialise the shadow register associated with each clk gate signature bits
    vppClkGateDetails[0].pShadowRegister = vppClkGateDetails[1].pShadowRegister = vppClkGateDetails[2].pShadowRegister = &vpp_obj->mainpl_ctrl4.u32;
    vppClkGateDetails[3].pShadowRegister = &vpp_obj->uiSclCfg15[VPP_FRC_SCL_MAIN];
    vppClkGateDetails[4].pShadowRegister = &vpp_obj->cpcb0_ctrl0.u32;
    vppClkGateDetails[5].pShadowRegister = vppClkGateDetails[6].pShadowRegister = vppClkGateDetails[7].pShadowRegister = &vpp_obj->vptop_cfg0.u32;
    vppClkGateDetails[8].pShadowRegister = &vpp_obj->gfx0_ups420_ctrl.u32;
    vppClkGateDetails[9].pShadowRegister = vppClkGateDetails[10].pShadowRegister = vppClkGateDetails[11].pShadowRegister = &vpp_obj->gfx0_ctrl.u32;
    vppClkGateDetails[12].pShadowRegister = &vpp_obj->gfx0_scl_bypass.u32;
    vppClkGateDetails[13].pShadowRegister = &vpp_obj->vpp_Ctrl.u32;
    vppClkGateDetails[14].pShadowRegister = &vpp_obj->gfx1_ctrl.u32;
    vppClkGateDetails[15].pShadowRegister = &vpp_obj->gfx1_scl_bypass.u32;
    vppClkGateDetails[16].pShadowRegister = &vpp_obj->vpp_Ctrl.u32;
    vppClkGateDetails[17].pShadowRegister = vppClkGateDetails[18].pShadowRegister = vppClkGateDetails[19].pShadowRegister = vppClkGateDetails[20].pShadowRegister = &vpp_obj->hdmi_vop.hdmi_vop_cfg3.u32;
    vppClkGateDetails[21].pShadowRegister = vppClkGateDetails[22].pShadowRegister = vppClkGateDetails[23].pShadowRegister = &vpp_obj->hdrx_phy_pm.u32;
    vppClkGateDetails[24].pShadowRegister = vppClkGateDetails[25].pShadowRegister = &vpp_obj->hdrx_phy_pm.u32;
    vppClkGateDetails[26].pShadowRegister = vppClkGateDetails[27].pShadowRegister = vppClkGateDetails[28].pShadowRegister = &vpp_obj->hdmirx_pipe_ctrl.u32;
    vppClkGateDetails[28].pShadowRegister = vppClkGateDetails[30].pShadowRegister = &vpp_obj->hdmirx_pipe_ctrl.u32;

    vppClkGateDetails[100].pShadowRegister = vppClkGateDetails[101].pShadowRegister = vppClkGateDetails[102].pShadowRegister = &vpp_obj->tgwrap_ctrl1.u32;



    //Read the intial register value into shadow regisgter; List of registers who's shadow is created here/in this module
#ifdef EN_VPP_SRAM_CLKGATE
    RegAddr = vppClkGateDetails[31].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx1_scl_pwrGate.u32);
    vppClkGateDetails[31].pShadowRegister = vppClkGateDetails[32].pShadowRegister = vppClkGateDetails[33].pShadowRegister = &vpp_obj->gfx1_scl_pwrGate.u32;
    vppClkGateDetails[34].pShadowRegister = vppClkGateDetails[35].pShadowRegister = vppClkGateDetails[36].pShadowRegister = &vpp_obj->gfx1_scl_pwrGate.u32;
    RegAddr = vppClkGateDetails[37].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_scl_pwrGate.u32);
    vppClkGateDetails[37].pShadowRegister = vppClkGateDetails[38].pShadowRegister = vppClkGateDetails[39].pShadowRegister = &vpp_obj->gfx0_scl_pwrGate.u32;
    vppClkGateDetails[40].pShadowRegister = vppClkGateDetails[41].pShadowRegister = vppClkGateDetails[42].pShadowRegister = &vpp_obj->gfx0_scl_pwrGate.u32;
    RegAddr = vppClkGateDetails[43].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->hdmirx_scl_cfg20.u32);
    vppClkGateDetails[43].pShadowRegister = vppClkGateDetails[44].pShadowRegister = vppClkGateDetails[45].pShadowRegister = &vpp_obj->hdmirx_scl_cfg20.u32;
    vppClkGateDetails[46].pShadowRegister = vppClkGateDetails[47].pShadowRegister = vppClkGateDetails[48].pShadowRegister = &vpp_obj->hdmirx_scl_cfg20.u32;
    RegAddr = vppClkGateDetails[49].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->vidpl_scl_cfg20.u32);
    vppClkGateDetails[49].pShadowRegister = vppClkGateDetails[50].pShadowRegister = vppClkGateDetails[51].pShadowRegister = &vpp_obj->vidpl_scl_cfg20.u32;
    vppClkGateDetails[52].pShadowRegister = vppClkGateDetails[53].pShadowRegister = vppClkGateDetails[54].pShadowRegister = &vpp_obj->vidpl_scl_cfg20.u32;
    RegAddr = vppClkGateDetails[55].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->vidpl_ups420_422_cfg8.u32);
    vppClkGateDetails[55].pShadowRegister = vppClkGateDetails[56].pShadowRegister = vppClkGateDetails[57].pShadowRegister = &vpp_obj->vidpl_ups420_422_cfg8.u32;
    RegAddr = vppClkGateDetails[58].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->hdmirx_ups420_422_cfg8.u32);
    vppClkGateDetails[58].pShadowRegister = vppClkGateDetails[59].pShadowRegister = vppClkGateDetails[60].pShadowRegister = &vpp_obj->hdmirx_ups420_422_cfg8.u32;
    RegAddr = vppClkGateDetails[61].uiRegAddress;
    vppClkGateDetails[61].pShadowRegister = vppClkGateDetails[62].pShadowRegister = vppClkGateDetails[63].pShadowRegister = VPP_VMXAPI_ClkGate_GetReg_IpCtrl(RegAddr);
    RegAddr = vppClkGateDetails[64].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].y_dif_cfg0.u32);
    vppClkGateDetails[64].pShadowRegister = vppClkGateDetails[65].pShadowRegister = vppClkGateDetails[66].pShadowRegister = &vpp_obj->plane[PLANE_MAIN].y_dif_cfg0.u32;
    vppClkGateDetails[67].pShadowRegister = vppClkGateDetails[68].pShadowRegister = vppClkGateDetails[69].pShadowRegister = &vpp_obj->plane[PLANE_MAIN].y_dif_cfg0.u32;
    RegAddr = vppClkGateDetails[70].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_MAIN].c_dif_cfg0.u32);
    vppClkGateDetails[70].pShadowRegister = vppClkGateDetails[71].pShadowRegister = vppClkGateDetails[72].pShadowRegister = &vpp_obj->plane[PLANE_MAIN].c_dif_cfg0.u32;
    vppClkGateDetails[73].pShadowRegister = vppClkGateDetails[74].pShadowRegister = vppClkGateDetails[75].pShadowRegister = &vpp_obj->plane[PLANE_MAIN].c_dif_cfg0.u32;
    RegAddr = vppClkGateDetails[76].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->avio_gbl_swpdwn_ctrl.u32);
    vppClkGateDetails[76].pShadowRegister = vppClkGateDetails[77].pShadowRegister = vppClkGateDetails[78].pShadowRegister = &vpp_obj->avio_gbl_swpdwn_ctrl.u32;
    vppClkGateDetails[79].pShadowRegister = vppClkGateDetails[80].pShadowRegister = vppClkGateDetails[81].pShadowRegister = &vpp_obj->avio_gbl_swpdwn_ctrl.u32;
    vppClkGateDetails[82].pShadowRegister = vppClkGateDetails[83].pShadowRegister = vppClkGateDetails[84].pShadowRegister = &vpp_obj->avio_gbl_swpdwn_ctrl.u32;
    vppClkGateDetails[85].pShadowRegister = vppClkGateDetails[86].pShadowRegister = vppClkGateDetails[87].pShadowRegister = &vpp_obj->avio_gbl_swpdwn_ctrl.u32;
    RegAddr = vppClkGateDetails[88].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->gfx0_ups420_422sp_cfg8.u32);
    vppClkGateDetails[88].pShadowRegister = vppClkGateDetails[89].pShadowRegister = vppClkGateDetails[90].pShadowRegister = &vpp_obj->gfx0_ups420_422sp_cfg8.u32;
    RegAddr = vppClkGateDetails[91].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->hdmi_vop_dns422_420_cfg1.u32);
    vppClkGateDetails[91].pShadowRegister = vppClkGateDetails[92].pShadowRegister = vppClkGateDetails[93].pShadowRegister = &vpp_obj->hdmi_vop_dns422_420_cfg1.u32;

    RegAddr = vppClkGateDetails[94].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->vptop_dint_cfg43.u32);
    vppClkGateDetails[94].pShadowRegister = vppClkGateDetails[95].pShadowRegister = vppClkGateDetails[96].pShadowRegister = &vpp_obj->vptop_dint_cfg43.u32;

    RegAddr = vppClkGateDetails[97].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->vptop_dint_dmx_cfg4.u32);
    vppClkGateDetails[97].pShadowRegister = vppClkGateDetails[98].pShadowRegister = vppClkGateDetails[99].pShadowRegister = &vpp_obj->vptop_dint_dmx_cfg4.u32;

#ifdef EN_VPP_DOLBY_CLKGATE
    RegAddr = vppClkGateDetails[103].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->stMainPlCtrl7.u32);
    vppClkGateDetails[103].pShadowRegister = vppClkGateDetails[104].pShadowRegister = vppClkGateDetails[105].pShadowRegister = &vpp_obj->stMainPlCtrl7.u32;
#endif
#ifdef EN_VPP_NEW_NON_DOLBY_CLKGATE
    RegAddr = vppClkGateDetails[128].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->pippl_ups420_422_cfg8.u32);
    vppClkGateDetails[128].pShadowRegister = vppClkGateDetails[129].pShadowRegister = vppClkGateDetails[130].pShadowRegister = &vpp_obj->pippl_ups420_422_cfg8.u32;

#ifdef ENABLE_PIP_GFX0_PATH
    RegAddr = vppClkGateDetails[131].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].y_dif_cfg0.u32);
    vppClkGateDetails[131].pShadowRegister = vppClkGateDetails[132].pShadowRegister = vppClkGateDetails[133].pShadowRegister = &vpp_obj->plane[PLANE_PIP].y_dif_cfg0.u32;
    vppClkGateDetails[134].pShadowRegister = vppClkGateDetails[135].pShadowRegister = vppClkGateDetails[136].pShadowRegister = &vpp_obj->plane[PLANE_PIP].y_dif_cfg0.u32;

    RegAddr = vppClkGateDetails[137].uiRegAddress;
    VPP_REG_WORD32_READ(RegAddr, &vpp_obj->plane[PLANE_PIP].c_dif_cfg0.u32);
    vppClkGateDetails[137].pShadowRegister = vppClkGateDetails[138].pShadowRegister = vppClkGateDetails[139].pShadowRegister = &vpp_obj->plane[PLANE_PIP].c_dif_cfg0.u32;
    vppClkGateDetails[140].pShadowRegister = vppClkGateDetails[141].pShadowRegister = vppClkGateDetails[142].pShadowRegister = &vpp_obj->plane[PLANE_PIP].c_dif_cfg0.u32;
#endif //ENABLE_PIP_GFX0_PATH
#endif
#endif //EN_VPP_SRAM_CLKGATE

#ifdef EN_VPP_NEW_NON_DOLBY_CLKGATE
   RegAddr = vppClkGateDetails[118].uiRegAddress;
   vppClkGateDetails[118].pShadowRegister = &vpp_obj->mainpl_ctrl4.u32;

   RegAddr = vppClkGateDetails[125].uiRegAddress;
   //VPP_REG_WORD32_READ(RegAddr, &vpp_obj->hdmi_vop.hdmi_vop_cfg4.u32);
   vppClkGateDetails[125].pShadowRegister = vppClkGateDetails[126].pShadowRegister = vppClkGateDetails[127].pShadowRegister = &vpp_obj->hdmi_vop.hdmi_vop_cfg4.u32;

   RegAddr = vppClkGateDetails[143].uiRegAddress;
   //VPP_REG_WORD32_READ(RegAddr, &vpp_obj->mainplCtrl0.u32);
   vppClkGateDetails[143].pShadowRegister = &vpp_obj->mainplCtrl0.u32;

   RegAddr = vppClkGateDetails[144].uiRegAddress;
   //VPP_REG_WORD32_READ(RegAddr, &vpp_obj->stElPipeCtrl1.u32);
   vppClkGateDetails[144].pShadowRegister = &vpp_obj->stElPipeCtrl1.u32;

   vppClkGateDetails[145].pShadowRegister = vppClkGateDetails[146].pShadowRegister = &vpp_obj->mainpl_ctrl4.u32;

   vppClkGateDetails[147].pShadowRegister = &vpp_obj->vptop_ctrl_cfg0.u32;

   RegAddr = vppClkGateDetails[148].uiRegAddress;
   VPP_REG_WORD32_READ(RegAddr, &vpp_obj->vidpl_ADPSCLctrl1.u32);
   vppClkGateDetails[148].pShadowRegister = &vpp_obj->vidpl_ADPSCLctrl1.u32;

   RegAddr = vppClkGateDetails[149].uiRegAddress;
   VPP_REG_WORD32_READ(RegAddr, &vpp_obj->cpcb0_ctrl0.u32);
   vppClkGateDetails[149].pShadowRegister = &vpp_obj->cpcb0_ctrl0.u32;

   RegAddr = vppClkGateDetails[150].uiRegAddress;
   //VPP_REG_WORD32_READ(RegAddr, &vpp_obj->hdmi_vop.hdmi_vop_cfg4.u32);
   vppClkGateDetails[150].pShadowRegister = &vpp_obj->hdmi_vop.hdmi_vop_cfg4.u32;

   RegAddr = vppClkGateDetails[151].uiRegAddress;
   VPP_REG_WORD32_READ(RegAddr, &vpp_obj->stVop_ctrl.u32);
   vppClkGateDetails[151].pShadowRegister = &vpp_obj->stVop_ctrl.u32;

   vppClkGateDetails[152].pShadowRegister = &g_clkgate_dummyvar;

   RegAddr = vppClkGateDetails[153].uiRegAddress;

   VPP_REG_WORD32_READ(RegAddr, &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.u32);
   vppClkGateDetails[153].pShadowRegister = &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.u32;

   vppClkGateDetails[154].pShadowRegister = &g_clkgate_dummyvar;

   RegAddr = vppClkGateDetails[155].uiRegAddress;

   VPP_REG_WORD32_READ(RegAddr, &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.u32);
   vppClkGateDetails[155].pShadowRegister = &vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.u32;

   vppClkGateDetails[156].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[157].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[158].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[159].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[160].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[161].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[162].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[163].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[164].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[165].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[166].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[167].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[168].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[169].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[170].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[171].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[172].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[173].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[174].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[175].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[176].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[177].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[178].pShadowRegister = &g_clkgate_dummyvar;
#endif
   vppClkGateDetails[179].pShadowRegister = &g_clkgate_dummyvar;
   vppClkGateDetails[180].pShadowRegister = &g_clkgate_dummyvar;

    VPP_DV_ClkGate_Initialize(vpp_obj);

#ifdef CONFIG_VPP_ENABLE_ALL_CLKGATE_AT_INITTIME
    VPP_ClkGate_GetRegDump(vpp_obj, 0);
    VPP_ClkGate_InitialDisable(vpp_obj);
    VPP_ClkGate_GetRegDump(vpp_obj, 0);
#endif //CONFIG_VPP_ENABLE_ALL_CLKGATE_AT_INITTIME

#endif //EN_VPP_DYNAMIC_CLKGATE

    VPP_ClkGate_Plane(vpp_obj, PLANE_MAIN, 1);
    VPP_ClkGate_Plane(vpp_obj, PLANE_PIP, 1);
    VPP_ClkGate_Plane(vpp_obj, PLANE_GFX1, 1);
}

void VPP_ClkGate_triggerDelayOperation(VPP_OBJ *vpp_obj, ENUM_VPP_CLKGATE_OPT options) {

#ifdef CONFIG_VPP_CLKGATE_DELAYED_PROCESSING

    /*If delayed operation is not enabled;
     *If delayed operation is not for given option
     * then return immediatly */
    if(!IS_VPP_CLKGATE_DELAYED_OPTION_ENABLED(delayedOptions) ||
       !(options & delayedOptions.clkGateDelayedOpt)) {
        return;
    }

    //Trigger by setting the initial value for delay
    delayedOptions.clkGateDelayedCount = delayedOptionsBackup.clkGateDelayedCount;
    if(!delayedOptionsBackup.clkGateDelayedCount)
        delayedOptions.clkGateDelayedCount = VPP_CLKGATE_DLY_DEFAULT_COUNT;

#endif //CONFIG_VPP_CLKGATE_DELAYED_PROCESSING
}

void VPP_ClkGate_performDelayOperation(VPP_OBJ *vpp_obj) {

#ifdef CONFIG_VPP_CLKGATE_DELAYED_PROCESSING

    //If delayed operation is enabled, and counter is running then execute the option
    if(IS_VPP_CLKGATE_DELAYED_OPTION_ENABLED(delayedOptions) &&
       IS_VPP_CLKGATE_DELAYED_OPTION_VALUE(delayedOptions)) {
        UINT32 SignatureCount;
        VPP_CLK_GATE_SIGNATURE Signature;

        //If delayed operation is in progress; then decrement the delay count
        DEC_VPP_CLKGATE_DELAYED_OPTION_VALUE(delayedOptions);
        if(!(IS_VPP_CLKGATE_DELAYED_OPTION_VALUE(delayedOptions))) {
            //Disable/Reset the delayed operation, if repeat is not initiated
            if(delayedOptions.clkGateDelayedOpt & VPP_CLKGATE_DLY_OPT_REPEAT) {
                delayedOptions = delayedOptionsBackup;
            } else {
                //SET_VPP_CLKGATE_DELAYED_OPTION_DISABLE(delayedOptions);
                delayedOptions.opt = 0;
            }

            VPP_ClkGate_GetSignature(vpp_obj, &SignatureCount, &Signature, (VPP_CLKGATE_OPT)((UINT32)delayedOptions.opt&0xFF));

            VPP_CLKGATE_PRINT("Signature Count : %d  => %08X, %08X, %08X, %08X\n", SignatureCount,
                Signature.signature0, Signature.signature1, Signature.signature2, Signature.signature3);
            }
    }

#endif //CONFIG_VPP_CLKGATE_DELAYED_PROCESSING
}

void VPP_ClkGate_GetRegDump(VPP_OBJ *vpp_obj, int debugOption) {
#define VPP_MAX_CLKGATE_REGS    256
    int regAddr[VPP_MAX_CLKGATE_REGS];
    const int vppClkGateCount = sizeof(vppClkGateDetails) / sizeof(VPP_CLKGATE_IP_DETAILS);
    int i,nReg=0;
    UINT8 enableDebugPrint = debugOption&0x01;
    UINT32 clkPwrGateBitMask;
    UINT32 clkPwrGateBitValue;

    //memset(&regAddr[0], 0, sizeof(int)*VPP_MAX_CLKGATE_REGS);

    for(i = 0; i < vppClkGateCount; i++) {
        regAddr[i] = 0;
    }

    if(enableDebugPrint) {
        VPP_CLKGATE_PRINT("Register Dump:\n");
        VPP_CLKGATE_PRINT("%6s\t%35s%20s\t\t%20s\t%8s\t\tTotal Clk's : %d\n", "Num", "Name", "Address=Value", "BitMask:BitValue", "Bits", vppClkGateCount);
    }

    for(i = nReg = 0; i < vppClkGateCount; i++) {
        UINT32 regAddress = vppClkGateDetails[i].uiRegAddress;
        UINT32 regValue = GLB_REG_READ32(regAddress);
        UINT32 j,k;
        UINT32 found=0;
        UINT32 nBits;

        //Check if register is already read/displayed
        for(j=0;j<nReg;j++) {
            if(regAddress == regAddr[j]) {
                found=1;
                break;
            }
        }


        if(found) {
            continue;
        } else {
            //Reset bitmask and value for this register
            clkPwrGateBitMask = 0;
            clkPwrGateBitValue = 0;
            nBits = 0;
            for(k = 0; k < vppClkGateCount; k++) {
                if(vppClkGateDetails[k].uiRegAddress == regAddress) {
                    //Update the bitmask & value
                    clkPwrGateBitMask |= (0x1 << vppClkGateDetails[k].uiStartBitPos);
                    clkPwrGateBitValue |= (regValue&(0x1 << vppClkGateDetails[k].uiStartBitPos));
                    nBits++;
                }
            }
            //Store the register
            regAddr[nReg] = regAddress;
            nReg++;
            //Print the detials if required
            if(enableDebugPrint) {
                //Print the detials if all the data regarding the register is collected
                VPP_CLKGATE_PRINT("%6d\t%35s\t0x%08X = 0x%08X\t %08X : %08X (%02d) => ",
                    nReg, vppClkGateDetails[i].pCIpBlockName, regAddress, regValue, clkPwrGateBitMask, clkPwrGateBitValue, nBits);

                for(k=0;clkPwrGateBitMask;k++, clkPwrGateBitMask>>=1, clkPwrGateBitValue>>=1) {
                    if(clkPwrGateBitMask&0x01) {
                        VPP_CLKGATE_PRINT("%02d:%d, ", k, clkPwrGateBitValue&0x01);
        }
                }
                VPP_CLKGATE_PRINT("\n");

            }
        }

        nBits = vppClkGateDetails[i].uiNBits;

    }

    if(debugOption&0x02)
    {
        VPP_CLK_GATE_SIGNATURE signature;
        UINT32 signatureCount;
        VPP_CLKGATE_OPT options;

        memset(&signature, 0, sizeof(VPP_CLK_GATE_SIGNATURE));
        options.opt = 0;
        VPP_ClkGate_GetSignature(vpp_obj, &signatureCount, &signature, options);

        VPP_CLKGATE_PRINT("Signature Dump:\n");
        VPP_CLKGATE_PRINT("Signature Count : %03d  => %08X %08X %08X %08X %08X %08X\n",
            signatureCount, signature.signature0, signature.signature1,
            signature.signature2, signature.signature3, signature.signature4,
            signature.signature5);
    }

}

void VPP_ClkGate_GetSignature(VPP_OBJ *vpp_obj, UINT32 *pSignatureCount,
        VPP_CLK_GATE_SIGNATURE *pSig, VPP_CLKGATE_OPT options) {
    const int vppClkGateCount = sizeof(vppClkGateDetails) / sizeof(VPP_CLKGATE_IP_DETAILS);
    int i;
    UINT32* pSignature = (UINT32 *)pSig;

    UINT8 enableDebugPrint = IS_VPP_CLKGATE_OPTION_ENABLE_PRINT(options.clkGateOpt);
    UINT8 recordRawBits    = IS_VPP_CLKGATE_OPTION_ENABLE_RECORD_RAW(options.clkGateOpt);
    UINT8 enableRegDump    = IS_VPP_CLKGATE_OPTION_ENABLE_REGDUMP(options.clkGateOpt);

    if(options.clkGateOptFuture&0x80) {
        VPP_CLKGATE_PRINT("FUTURE option : 0x%x - enClkGate :  %x\n", options.clkGateOptFuture, vpp_obj->enClkGate);
        if(options.clkGateOptFuture&0x01) {
            VPP_CLKGATE_PRINT("Enabling the debug print while enabling CLK gating \n");
            vpp_obj->enClkGate |= VPP_CLKGATE_ENABLE_OPT_DBG_CLKGATE;
        } else {
            VPP_CLKGATE_PRINT("Disabling the debug print while disabling CLK gating \n");
            vpp_obj->enClkGate &= ~VPP_CLKGATE_ENABLE_OPT_DBG_CLKGATE;
        }
        if(options.clkGateOptFuture&0x02) {
            VPP_CLKGATE_PRINT("Enabling the debug print while enabling PWR gating \n");
            vpp_obj->enClkGate |= VPP_CLKGATE_ENABLE_OPT_DBG_PWRGATE;
        } else {
            VPP_CLKGATE_PRINT("Disabling the debug print while disabling PWR gating \n");
            vpp_obj->enClkGate &= ~VPP_CLKGATE_ENABLE_OPT_DBG_PWRGATE;
        }
        //Reset future option after processing it
        options.clkGateOptFuture = 0;
        VPP_CLKGATE_PRINT("FUTURE option : 0x%x - enClkGate :  %x, final option : %x\n", options.clkGateOptFuture, vpp_obj->enClkGate, options.opt);
    }

#ifdef CONFIG_VPP_CLKGATE_DELAYED_PROCESSING
    if(options.opt&(~0xFF)) {
        delayedOptions = options;
        SET_VPP_CLKGATE_DELAYED_OPTION_ENABLE(delayedOptions);
        delayedOptionsBackup = delayedOptions;
        //Let the count be initialized in trigger function
        SET_VPP_CLKGATE_DELAYED_OPTION_VALUE(delayedOptions, 0x0);
        VPP_CLKGATE_PRINT("DELAYED OPTION SET : 0x%x\n", delayedOptionsBackup.opt);
        return;
    }
#endif //CONFIG_VPP_CLKGATE_DELAYED_PROCESSING

    if(enableDebugPrint)
        VPP_CLKGATE_PRINT("%6s\t%35s\t%20s\t\t%s\t%s\t\tTotal Clk's : %d\n", "BitPos", "Name", "Address=Value", "Description", "BitMask", vppClkGateCount);

    *pSignatureCount = 0;
    memset(pSig, 0 , sizeof(VPP_CLK_GATE_SIGNATURE));

    for(i = 0; i < vppClkGateCount; i++) {
        UINT32 regAddress = vppClkGateDetails[i].uiRegAddress;
        UINT32 regValue = GLB_REG_READ32(regAddress);
        UINT32 regValueMask = (1<<vppClkGateDetails[i].uiNBits) - 1;
        UINT32 strNdx;
        char **strArray;
        UINT32 strArraySize;
        UINT32 regValueBits;
        UINT32 uiSignatureNdx    = i/32;
        UINT32 uiSignatureBitPos = i%32;
        //Store 1 in signature if required bit is bypassed/Disabled
        UINT32 recordedBit = 1;

        regValueBits = regValue >> vppClkGateDetails[i].uiStartBitPos;
        strNdx = regValueBits & regValueMask;

#ifdef VPP_CLGATE_ENABLE_MULTIPLE_STR_TABLE
        switch( vppClkGateDetails[i].uiByPassStringNdx ) {
            case VPP_CLKGATE_STRING_ARRAY_EN_DIS:
            default:
                strArray = pByPassStringName;
                strArraySize = sizeof(strArraySize) / sizeof(char*);
                strArraySize = MAX_BYPASS_STRING_NAME_COUNT;
                //Compare the bit field value against byPass/Enable bit
                strNdx = (regValueBits & regValueMask) == vppClkGateDetails[i].uiByPass;
            break;

        }
#else
        strArray = pByPassStringName;
        strArraySize = MAX_BYPASS_STRING_NAME_COUNT;
        strNdx = (regValueBits & regValueMask) == vppClkGateDetails[i].uiByPass;
#endif
        if(recordRawBits) {
             recordedBit=(regValueBits<<uiSignatureBitPos);
        }
        if((strNdx<strArraySize) && strNdx) {
            pSignature[uiSignatureNdx] |= (recordedBit<<uiSignatureBitPos);
            if(strNdx) {
                //Increment the Signature count
                (*pSignatureCount)++;
            }
        }

        if(enableDebugPrint)
            VPP_CLKGATE_PRINT("%6d\t%35s\t 0x%08X = 0x%08X\t %s\t %02X\n", i, vppClkGateDetails[i].pCIpBlockName, regAddress, regValue, strNdx > strArraySize? "--NA--" : strArray[strNdx], vppClkGateDetails[i].uiStartBitPos);
    }

    if(enableDebugPrint && enableRegDump) {
        VPP_ClkGate_GetRegDump(vpp_obj, VPP_CLKGATE_CLKPWR_REG_DUMP_OPTION);
    }
}

void VPP_ClkGate_SetSignature(VPP_OBJ *vpp_obj, VPP_CLK_GATE_SIGNATURE *pSig,
        VPP_CLK_GATE_SIGNATURE *pSigMask, VPP_CLKGATE_OPT options) {
    const int vppClkGateCount = sizeof(vppClkGateDetails) / sizeof(VPP_CLKGATE_IP_DETAILS);
    int i;
    UINT32 *pSignatureValue = (UINT32*)pSig;
    UINT32 *pSignatureMask= (UINT32*)pSigMask;

    UINT8 enableDebugPrint = IS_VPP_CLKGATE_OPTION_ENABLE_PRINT(options.clkGateOpt);
    UINT8 recordRawBits = IS_VPP_CLKGATE_OPTION_ENABLE_RECORD_RAW(options.clkGateOpt);

    int signatureCount = 0;
    if(enableDebugPrint)
        VPP_CLKGATE_PRINT("%35s\t %20s\t %s\n", "Name", "  Address = org_Value => new_Value", "StartBitPos");

    for(i = 0; i < vppClkGateCount; i++) {
        UINT32 regAddress = vppClkGateDetails[i].uiRegAddress;
        UINT32 regValueMask = (1<<vppClkGateDetails[i].uiNBits) - 1;
        UINT32 strNdx;
        UINT32 regValueBits;
        UINT32 uiSignatureNdx    = i/32;
        UINT32 uiSignatureBitPos = i%32;
        UINT32 regValue = (pSignatureValue[uiSignatureNdx] >> uiSignatureBitPos) & regValueMask;
        //Store 1 in signature if required bit is bypassed/Disabled
        UINT32 recordedBit = 1;

        if( !(pSignatureMask[uiSignatureNdx] & (regValueMask<<uiSignatureBitPos)) ) {
                continue;
        }

        regValueBits = regValue;
        strNdx = regValueBits & regValueMask;

        strNdx = (regValueBits & regValueMask) == vppClkGateDetails[i].uiByPass;

        if(recordRawBits) {
             recordedBit=(regValueBits<<vppClkGateDetails[i].uiStartBitPos);
        } else {
             recordedBit=(strNdx<<vppClkGateDetails[i].uiStartBitPos);
        }

        //Copy only the recordedBit to the register without altering other bits
        regValue = (*vppClkGateDetails[i].pShadowRegister) & ~(regValueMask<<vppClkGateDetails[i].uiStartBitPos);
        regValue |= recordedBit;
        //Store recordedBit at regAddress.uiStartBitPos
        GLB_REG_WRITE32(regAddress, regValue);

        //Increment the Signature count
        signatureCount++;

        if(enableDebugPrint)
            VPP_CLKGATE_PRINT("%35s\t 0x%08X = 0x%08X => 0x%08X, %d\n", vppClkGateDetails[i].pCIpBlockName, regAddress,
                            *vppClkGateDetails[i].pShadowRegister, regValue, vppClkGateDetails[i].uiStartBitPos);

        //Update the shadow register with new value
        *vppClkGateDetails[i].pShadowRegister = regValue;
    }
}

void VPP_ClkGate_EnableGating(VPP_OBJ *vpp_obj, int clkGateNdx, ENUM_VPP_CLK_GATE isPowerDown, int option)
{
#ifdef EN_VPP_DYNAMIC_CLKGATE
    UINT32 regValue;
    UINT32 regMask = 1; //one bit at a time
    UINT32 regClkBits=0;
    UINT32 regClkBitsMask=0;

    if(!IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE)) {
        return;
    }

    if(IS_VPP_CLKGATE_DBG_ENABLE(vpp_obj->enClkGate, CLKGATE)) {
        VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("\n\rPower: %s\n\r", isPowerDown?"DOWN":"UP");
        VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("%35s\t %20s\t %s\t %s\n\r", "Name", "  Address = org_Value => new_Value", "StartBitPos", "PowerBits & PowerBitsMask");
    }

    if(!(option & VPP_CLKGATE_OPTION_NO_SHADOW_REG)) {
        //Get the copy of the shadow register value
        regValue = *vppClkGateDetails[clkGateNdx].pShadowRegister;
    } else {
        //No Shadow register, so read from directly from register
        VPP_REG_WORD32_READ(vppClkGateDetails[clkGateNdx].uiRegAddress, &regValue);
    }

    //Get the register value & mask for clk gate bit
    {
        //Bits
        if(isPowerDown) {
            if(vppClkGateDetails[clkGateNdx].uiByPass) {
                //Set Bits - if we need to power down and clkgated by setting 1
                regClkBits |= regMask << vppClkGateDetails[clkGateNdx].uiStartBitPos;
            }
        } else {
            if(!vppClkGateDetails[clkGateNdx].uiByPass) {
                //Set Bits - if we need to power up and clkgate disabled by setting 1
                regClkBits |= regMask << vppClkGateDetails[clkGateNdx].uiStartBitPos;
            }
        }
        //Construct the mask for regPwrBits
        regClkBitsMask |= regMask << vppClkGateDetails[clkGateNdx].uiStartBitPos;
    }

    //Clear Bits
    regValue &= ~regClkBitsMask;

    //Set Bits - if required
    regValue |= regClkBits;

    if(IS_VPP_CLKGATE_DBG_ENABLE(vpp_obj->enClkGate, PWRGATE)) {
        VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("%35s\t 0x%08X = 0x%08X => 0x%08X, \t%d, \t%08X & %08X\n\r",
            vppClkGateDetails[clkGateNdx].pCIpBlockName, vppClkGateDetails[clkGateNdx].uiRegAddress,
            *vppClkGateDetails[clkGateNdx].pShadowRegister, regValue, vppClkGateDetails[clkGateNdx].uiStartBitPos, regClkBits, regClkBitsMask);
    }

    //Program the register with the newly computed register value
    if(!(option & VPP_CLKGATE_OPTION_NO_SHADOW_REG)) {
        option &= ~VPP_CLKGATE_OPTION_NO_SHADOW_REG;

        if(option == 1) {
            //VPP_CLKGATE_ENABLE_SEPARATE_BCM_WRITE
            if(isPowerDown) {
                //Powerdown after all register programming, writting to power-down'ed block might put the block to unrecoverable/undetermined state
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
            } else
            {
                //Let us power up as part of clear i.e enable power before configuring respective block
                VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
            }
        }
        else if (option == 2) {
            //VPP_CLKGATE_ENABLE_SAME_BCM_WRITE
            //Powerdown after all register programming, writting to power-down'ed block might put the block to unrecoverable/undetermined state

            VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);

        }
        else {
            //VPP_CLKGATE_ENABLE_DIRECT_WRITE
            VPP_REG_WORD32_WRITE(vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
        }

        //Update the shadow register with new value
        *vppClkGateDetails[clkGateNdx].pShadowRegister = regValue;
    }
    else {
        VPPAPI_DBG_INFO("%x = %x\n",vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);

        //No Shadow register, so write directly to register
        VPP_REG_WORD32_WRITE(vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
    }

#endif //EN_VPP_DYNAMIC_CLKGATE
}

void VPP_SRAM_PowerGate(VPP_OBJ *vpp_obj, ENUM_VPP_SRAM_PWRGATE_BLK pwrGateBlk, ENUM_VPP_SRAM_PWRGATE isPowerDown) {
#ifdef EN_VPP_SRAM_CLKGATE
    const int vppPwrGateCount = sizeof(vppPwrGateDetails) / sizeof(VPP_POWERGATE_DETAILS);
    int i,j;

    if(!IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, PWRGATE)) {
        return;
    }

    if(IS_VPP_CLKGATE_DBG_ENABLE(vpp_obj->enClkGate, PWRGATE)) {
        VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("\n\rPowerBitMask = %d, Power: %s\n\r", VPP_POWERGATE_BITMASK, isPowerDown?"DOWN":"UP");
        VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("%35s\t %20s\t %s\t %s\n\r", "Name", "  Address = org_Value => new_Value", "StartBitPos", "PowerBits & PowerBitsMask");
    }
    for(i=0;i<vppPwrGateCount;i++) {
        if(vppPwrGateDetails[i].pwrBlkId == pwrGateBlk) {
            int clkGateNdx = vppPwrGateDetails[i].signatureBitPos;
            int tmpClkGateNdx;
            UINT32 regValue;
            int powerGateIter;
            UINT32 regMask;
            UINT32 regPwrBits=0;
            UINT32 regPwrBitsMask=0;

            //Get the copy of the shadow register value
            regValue = *vppClkGateDetails[clkGateNdx].pShadowRegister;

            if(vppPwrGateDetails[i].bitMaskIter & VPP_POWERGATE_BITMASK_USEBIT) {
                //If using bitmask then all powergate bits are in sequence, so power gate value can be determined in single iteration
                powerGateIter = 1;
                regMask = VPP_POWERGATE_BITMASK; //all power bit at a time
            } else {
                //If not using bitmask then each powergate bits need to be iterated to get power gate value in multiple iteration
                powerGateIter = vppPwrGateDetails[i].bitMaskIter;
                regMask = 0x01;     //one power bit at a time
            }

            //Get the register value & mask for power gate bits - pdfvssmBitPos, pdlvmcBitPos, pwdnBitPos
            for(j = i, tmpClkGateNdx=clkGateNdx; j<i+powerGateIter; j++, tmpClkGateNdx++) {
                //Construct the bits for regPwrBits
                if(isPowerDown) {
                    if(vppClkGateDetails[tmpClkGateNdx].uiByPass) {
                        //Set Bits - if we need to power down and clkgated by setting 1
                        regPwrBits |= regMask << vppClkGateDetails[tmpClkGateNdx].uiStartBitPos;
                    }
                } else {
                    if(!vppClkGateDetails[clkGateNdx].uiByPass) {
                        //Set Bits - if we need to power up and clkgate disabled by setting 1
                        regPwrBits |= regMask << vppClkGateDetails[tmpClkGateNdx].uiStartBitPos;
                    }
                }

                //Construct the mask for regPwrBits
                regPwrBitsMask |= regMask << vppClkGateDetails[tmpClkGateNdx].uiStartBitPos;
            }

            //Clear Bits
            regValue &= ~regPwrBitsMask;
            //Set Bits
            regValue |= regPwrBits;

            if(IS_VPP_CLKGATE_DBG_ENABLE(vpp_obj->enClkGate, PWRGATE)) {
                VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("%35s\t 0x%08X = 0x%08X => 0x%08X, \t%d, \t%08X & %08X\n\r",
                    vppClkGateDetails[clkGateNdx].pCIpBlockName, vppClkGateDetails[clkGateNdx].uiRegAddress,
                    *vppClkGateDetails[clkGateNdx].pShadowRegister, regValue, vppClkGateDetails[clkGateNdx].uiStartBitPos, regPwrBits, regPwrBitsMask);
            }

            //Program the register with the newly computed register value
            #ifdef VPP_CLKGATE_ENABLE_DIRECT_WRITE
                //VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("%s:%d: reg:%x, val:%x\n", __func__, __LINE__, vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
                VPP_REG_WORD32_WRITE(vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
            #else
                #ifdef VPP_CLKGATE_ENABLE_SEPARATE_BCM_WRITE
                if(isPowerDown) {
                    //Powerdown after all register programming, writting to power-down'ed block might put the block to unrecoverable/undetermined state
                    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
                } else
                #endif //VPP_CLKGATE_ENABLE_SEPARATE_BCM_WRITE
                {
                    //Let us power up as part of clear i.e enable power before configuring respective block
                    VPP_BCMBUF_Write(vpp_obj->pVbiClearBcmBuf, vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
                    #ifdef VPP_CLKGATE_ENABLE_BCM_AND_DIRECT_WRITE
                    //Let us power up immediately so that power up done much before BCM and can give sufficient time for power to stabilise
                    {
                        VPP_REG_WORD32_WRITE(vppClkGateDetails[clkGateNdx].uiRegAddress, regValue);
                    }
                    #endif
                }
            #endif
            //Update the shadow register with new value
            *vppClkGateDetails[clkGateNdx].pShadowRegister = regValue;

            //Terminate the loop; we are done with powergating for the requested block
            break;
        }
    }
#endif //EN_VPP_SRAM_CLKGATE
}

void VPP_ClkGate_HDMIRX(VPP_OBJ *vpp_obj, UINT32 isClkGateEnabled)
{
#ifdef CONFIG_VPP_VIP_HDMIRX_CLOCKGATE
    if(!IS_VPP_CLKGATE_DISABLE(vpp_obj->enClkGate, HDMIRX) &&
        IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE)) {
        UINT32 powerDownFlag = isClkGateEnabled;
        UINT32 uiRegAddr;
#ifdef CONFIG_VPP_VIP_CLOCKGATE
        UINT32 bypass = isClkGateEnabled;
        UINT32 vip_pipe_base_addr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE;
#endif
        UINT32 hdmirx_base_addr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE;

#ifdef CONFIG_VPP_VIP_CLOCKGATE
        //Clock gate the HDMI RX PIPE module
        uiRegAddr = vip_pipe_base_addr+RA_HDMIRX_PIPE_BYPASS_CTRL;
        VPP_REG_WORD32_READ(uiRegAddr, &vpp_obj->hdmirx_pipe_ctrl.u32);
        if(IS_VPP_CLKGATE_DBG_ENABLE(vpp_obj->enClkGate, CLKGATE))
            VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("HDMIRX PIPE clockgate : %x = %x\n", uiRegAddr, vpp_obj->hdmirx_pipe_ctrl.u32);
        vpp_obj->hdmirx_pipe_ctrl.uBYPASS_CTRL_scl1d_bypass = bypass;
        vpp_obj->hdmirx_pipe_ctrl.uBYPASS_CTRL_ups420_bypass = bypass;
        vpp_obj->hdmirx_pipe_ctrl.uBYPASS_CTRL_csc_bypass = bypass;
        vpp_obj->hdmirx_pipe_ctrl.uBYPASS_CTRL_dns444_422_bypass = bypass;
        vpp_obj->hdmirx_pipe_ctrl.uBYPASS_CTRL_dither_bypass = bypass;
        VPP_REG_WORD32_WRITE(uiRegAddr, vpp_obj->hdmirx_pipe_ctrl.u32);
        if(IS_VPP_CLKGATE_DBG_ENABLE(vpp_obj->enClkGate, CLKGATE))
            VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("HDMIRX PIPE clockgate : %x = %x\n", uiRegAddr, vpp_obj->hdmirx_pipe_ctrl.u32);
#endif //CONFIG_VPP_VIP_CLOCKGATE

        //Clock gate the HDMI RX PHY module
        uiRegAddr = hdmirx_base_addr + RA_HDRX_PHY_PM;
        VPP_REG_WORD32_READ(uiRegAddr, &vpp_obj->hdrx_phy_pm.u32);
        if(IS_VPP_CLKGATE_DBG_ENABLE(vpp_obj->enClkGate, CLKGATE))
            VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("HDMIRX PHY clockgate : %x = %x\n", uiRegAddr, vpp_obj->hdrx_phy_pm.u32);
        vpp_obj->hdrx_phy_pm.uPHY_PM_pd_rxck = powerDownFlag;
        //vpp_obj->hdrx_phy_pm.uPHY_PM_syn_ck2rx = powerDownFlag;        //0 is default
        vpp_obj->hdrx_phy_pm.uPHY_PM_pd_comm = powerDownFlag;
        vpp_obj->hdrx_phy_pm.uPHY_PM_pd_pll = powerDownFlag;
        vpp_obj->hdrx_phy_pm.uPHY_PM_pd_cdr = powerDownFlag;
        vpp_obj->hdrx_phy_pm.uPHY_PM_pd_frt_c = powerDownFlag;
        vpp_obj->hdrx_phy_pm.uPHY_PM_pd_frt_d = powerDownFlag;
        //vpp_obj->hdrx_phy_pm.uPHY_PM_reset_rx = powerDownFlag;     //0 is default
        vpp_obj->hdrx_phy_pm.uPHY_PM_reset_pll = powerDownFlag;
        VPP_REG_WORD32_WRITE(uiRegAddr, vpp_obj->hdrx_phy_pm.u32);
        if(IS_VPP_CLKGATE_DBG_ENABLE(vpp_obj->enClkGate, CLKGATE))
            VPP_CLKGATE_DEBUG_PRINT_DYNAMIC("HDMIRX PHY clockgate : %x = %x\n", uiRegAddr, vpp_obj->hdrx_phy_pm.u32);

        //Power gate the repective module - HDMIRX
        VPP_SRAM_PowerGate_HDMIRX(vpp_obj, isClkGateEnabled?VPP_SRAM_PWRGATE_ENABLE:VPP_SRAM_PWRGATE_DISABLE);
    }
#endif
}

void VPP_ClkGate_FE_DEINT(VPP_OBJ *vpp_obj, ENUM_VPP_CLK_GATE enableClkGate)
{
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_DEINT_ENABLE)
    static ENUM_VPP_CLK_GATE enableClkGateBkup = VPP_CLK_GATE_IGNORE;
#endif
    VPP_ClkGate_VbiIsr_Callback(vpp_obj);

#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_DEINT_ENABLE)
    UINT32 temp;

    if(IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE) && (enableClkGateBkup != enableClkGate))
    {

        if(VPP_CLK_GATE_ENABLE == enableClkGate) {
            vpp_obj->vptop_cfg0.uCFG0_clkgate = 0; /* Clk gate both DEINT+DMX and CAR */
            vpp_obj->vptop_cfg0.uCFG0_bypass = 1; /* Bypass DEINT+DMX(vpTop) */
        } else {
            vpp_obj->vptop_cfg0.uCFG0_clkgate = 3; /* Enable both DEINT_DMX and CAR */
            vpp_obj->vptop_cfg0.uCFG0_bypass = 0; /* Through pass DEINT+DMX(vpTop) */
        }

        VPP_BCMBUF_Write(VPP_CLK_GATE_ENABLE == enableClkGate?vpp_obj->pVbiBcmBuf:vpp_obj->pVbiClearBcmBuf,
                vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_VPTOP_CTRL + RA_VPTOP_CTRL_VPTOP + RA_VPTOP_CFG0,
                vpp_obj->vptop_cfg0.u32);

        enableClkGateBkup = enableClkGate;

    #ifdef VPP_FE_DEINT_POWERGATE_ENABLE
        temp = dynamicDebugPrintEnable;
        //DEINT is configure per frame, so avoid continueous print
        dynamicDebugPrintEnable = 0;
        //DEINT can be power gated if MAIN plane is power gated
        VPP_SRAM_PowerGate_DEINT(vpp_obj, enableClkGate);
        dynamicDebugPrintEnable = temp;
    #endif
    }
#endif
}

void VPP_SRAM_PowerGate_HDMIRX(VPP_OBJ *vpp_obj, ENUM_VPP_SRAM_PWRGATE powerGateFlag)
{
    //PowerGate all the IP in HDMIRX
#ifdef CONFIG_VPP_VIP_CLOCKGATE
    VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_HDRX_PIPE_SCL1D_LB, powerGateFlag);
    VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_HDRX_PIPE_SCL1D_LUT, powerGateFlag);
    VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_HDRX_PIPE_VIP_UPS420, powerGateFlag);
#endif //CONFIG_VPP_VIP_CLOCKGATE
    VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_HDMIRX, powerGateFlag);
}

void VPP_SRAM_PowerGate_SCL(VPP_OBJ *vpp_obj, VPP_FRC_SCL_NUM UnitNum, ENUM_VPP_SRAM_PWRGATE powerGateFlag)
{
    ENUM_VPP_SRAM_PWRGATE_BLK lbSramBlk;
    ENUM_VPP_SRAM_PWRGATE_BLK lutSramBlk;

    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
            lbSramBlk = VPP_SRAM_PWRGATE_BLK_MAINSCL_LB;
            lutSramBlk = VPP_SRAM_PWRGATE_BLK_MAINSCL_LUT;
            /*We cannot power down UPS 420_422 & DIF block because SCL is bypassed,
             *We need UPS 420_422 for 420SP cases, DIF block for TILED mode
             *Please take this decission at FE_DLR*/
            /*Since scalar shares the UPS420_422 SRAM, we should allow enabling power(disable power gating)
             * when SCL is enabled even though is is disabled in FE_DLR based on the format(e.g.422) */
            if(powerGateFlag==VPP_SRAM_PWRGATE_DISABLE) {
                //VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINPL_UPS_420_422, powerGateFlag);
            }
            break;
#ifdef ENABLE_PIP_GFX0_PATH
        case VPP_FRC_SCL_PIP:
            lbSramBlk = VPP_SRAM_PWRGATE_BLK_GFX0_LB;
            lutSramBlk = VPP_SRAM_PWRGATE_BLK_GFX0_LUT;
            break;
#endif
        case VPP_FRC_SCL_OSD:
            lbSramBlk = VPP_SRAM_PWRGATE_BLK_GFX1_LB;
            lutSramBlk = VPP_SRAM_PWRGATE_BLK_GFX1_LUT;
            break;
        default:
            UnitNum = VPP_FRC_SCL_MAX;
            break;
    }

    if(VPP_FRC_SCL_MAX != UnitNum) {
#ifdef CONFIG_VPP_ENABLE_DEBUG_OPT_AT_INITTIME
        DV *pDV = &(vpp_obj->dv[0]);
        VPPAPI_DBG_INFO("Power gating scalar %d : %d, flag : %d\n",pDV->vbi_num, UnitNum, powerGateFlag);
#endif //CONFIG_VPP_ENABLE_DEBUG_OPT_AT_INITTIME
#ifdef VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE
        VPP_ClkGate_Register_DelayOperation_SclPwrGate(UnitNum, powerGateFlag);
#endif //VPP_CLKGATE_ENABLE_DELAYED_SCL_PWRGATE

        VPP_SRAM_PowerGate(vpp_obj, lbSramBlk, powerGateFlag);
        VPP_SRAM_PowerGate(vpp_obj, lutSramBlk, powerGateFlag);
    }
}

void VPP_SRAM_PowerGate_DEINT(VPP_OBJ *vpp_obj, ENUM_VPP_SRAM_PWRGATE powerGateFlag)
{
    VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAIN_DINT, powerGateFlag);
    VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAIN_DINT_DMX, powerGateFlag);
}

void VPP_SRAM_PowerGate_DIF(VPP_OBJ *vpp_obj, ENUM_PLANE_ID planeID, ENUM_VPP_SRAM_PWRGATE powerGateFlag)
{
    if(PLANE_MAIN == planeID) {
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINPL_DIF_C0, powerGateFlag);
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINPL_DIF_C1, powerGateFlag);
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINPL_DIF_Y0, powerGateFlag);
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINPL_DIF_Y1, powerGateFlag);
    }
    else if(PLANE_PIP == planeID) {
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_C0, powerGateFlag);
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_C1, powerGateFlag);
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_Y0, powerGateFlag);
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_PIPPL_DIF_Y1, powerGateFlag);
    }
}

void VPP_SRAM_PowerGate_Plane(VPP_OBJ *vpp_obj, ENUM_PLANE_ID planeID, ENUM_VPP_SRAM_PWRGATE powerGateFlag)
{
    VPP_FRC_SCL_NUM UnitNum;

    //Covert PlaneID to SCL ID
    switch(planeID)
    {
        case PLANE_MAIN:
            UnitNum = VPP_FRC_SCL_MAIN;
            break;
#ifdef ENABLE_PIP_GFX0_PATH
        case PLANE_PIP:
            UnitNum = VPP_FRC_SCL_PIP;
            break;
#endif
        case PLANE_GFX1:
            UnitNum = VPP_FRC_SCL_OSD;
            break;
        default:
            UnitNum = VPP_FRC_SCL_MAX;
            break;
    }

    if(VPP_FRC_SCL_MAX != UnitNum) {
        //Power gate the Plane & corresponding SCL ; if plane is not enabled
        if(VPP_SRAM_PWRGATE_ENABLE == powerGateFlag) {
            VPP_SRAM_PowerGate_SCL(vpp_obj, UnitNum, powerGateFlag);
        }

        if(PLANE_MAIN == planeID) {
            if(VPP_SRAM_PWRGATE_ENABLE == powerGateFlag) {
                //VMX can be power gated if MAIN plane is clock gated; Enable shall be done by VMX module only
                VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_VMXIP, powerGateFlag);
                //Main plane UPS can be clk/pwr gated here; Enable it in FE_DLR when frame with respecitive format is pushed.
                VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_MAINPL_UPS_420_422, VPP_SRAM_PWRGATE_ENABLE);
            }
        }
        //TODO: Disable in lower level using if(vpp_powergate_block_map[pwrGateBlk].is_valid)
#ifdef ENABLE_PIP_GFX0_PATH
        else if(PLANE_PIP == planeID) {
            //PIP plane UPS can be clk/pwr gated here; Enable it in FE_DLR when frame with respecitive format is pushed.
            VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_GFX0_UPS420_SP, VPP_SRAM_PWRGATE_ENABLE);
        }
#endif

        if(VPP_SRAM_PWRGATE_ENABLE == powerGateFlag) {
            //DIF block can be power gated if respective plane is power gated, Enable shall be donw if respective frame is pushed to plane
            VPP_SRAM_PowerGate_DIF(vpp_obj, planeID, powerGateFlag);
        }
    }

}
int VPP_ClkGate_Plane(VPP_OBJ *vpp_obj, int planeID, int enable)
{
#if defined(EN_VPP_DYNAMIC_CLKGATE) && defined(CONFIG_VPP_ENABLE_PLANE_CLKGATE)
    UINT32 RegAddr;
    if(IS_VPP_CLKGATE_ENABLE(vpp_obj->enClkGate, CLKGATE))
    {
        if(enable)
        {
            VPP_ClkGate_SCL(vpp_obj, planeID, enable);
            switch(planeID)
            {
                case PLANE_MAIN:
                    RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4;
                    vpp_obj->mainpl_ctrl4.uCTRL4_UPS420_4K_bypass = enable;
                    vpp_obj->mainpl_ctrl4.uCTRL4_vmxIP_bypass = enable;
                    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->mainpl_ctrl4.u32);
                    break;
#ifdef ENABLE_PIP_GFX0_PATH
                case PLANE_PIP:
                    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL;
                    vpp_obj->gfx0_ctrl.uCTRL_ups8_bypass = enable;
                    vpp_obj->gfx0_ctrl.uCTRL_csc_bypass = enable;
                    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ctrl.u32);
                    RegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_UPS420_CTRL;
                    vpp_obj->gfx0_ups420_ctrl.u32 = 0;
                    vpp_obj->gfx0_ups420_ctrl.uUPS420_CTRL_ups420_bypass = enable;
                    VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->gfx0_ups420_ctrl.u32);
                    break;
#endif
                case PLANE_GFX1:
                    break;
                default:
                    return (MV_VPP_EBADPARAM);
            }
        }
        switch(planeID)
        {
            case PLANE_MAIN:
                RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4;
                vpp_obj->mainpl_ctrl4.uCTRL4_adpScl_bypass = enable;
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->mainpl_ctrl4.u32);
                RegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_CPCB0 + RA_CPCB0_CTRL0;
                vpp_obj->cpcb0_ctrl0.uCTRL0_CSC_bypass = enable;
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->cpcb0_ctrl0.u32);
                break;
#ifdef ENABLE_PIP_GFX0_PATH
            case PLANE_PIP:
                RegAddr = vpp_obj->base_addr + RA_Vpp_CTRL;
                vpp_obj->vpp_Ctrl.uCTRL_GFX1_PLANE_CG_EN = (enable ^ 1);
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->vpp_Ctrl.u32);
                break;
#endif
            case PLANE_GFX1:
                RegAddr = vpp_obj->base_addr + RA_Vpp_CTRL;
                vpp_obj->vpp_Ctrl.uCTRL_GFX2_PLANE_CG_EN = (enable ^ 1);
                VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, RegAddr, vpp_obj->vpp_Ctrl.u32);
                break;
            default:
                return (MV_VPP_EBADPARAM);
        }
        VPP_SRAM_PowerGate_Plane(vpp_obj, planeID, enable?VPP_SRAM_PWRGATE_ENABLE:VPP_SRAM_PWRGATE_DISABLE);
    }
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */
#if defined(CONFIG_VPP_ENABLE_VMX_PLANE_CLKGATE)
    if(PLANE_MAIN == planeID) {
        VPP_SRAM_PowerGate(vpp_obj, VPP_SRAM_PWRGATE_BLK_VMXIP, vpp_obj->mainpl_ctrl4.uCTRL4_vmxIP_bypass?VPP_SRAM_PWRGATE_ENABLE:VPP_SRAM_PWRGATE_DISABLE);
        VPP_ClkGate_HDMIRX(vpp_obj, VPP_SRAM_PWRGATE_ENABLE);
    }
#endif /* #ifdef CONFIG_VPP_ENABLE_VMX_PLANE_CLKGATE*/
    return (MV_VPP_OK);
}
#ifdef EN_VPP_DYNAMIC_CLKGATE
int VPP_ClkGate_SCL(VPP_OBJ *vpp_obj, int UnitNum, int enable)
{
    int uiRetVal = MV_VPP_OK;
    UINT32 uiRegAddr = 0;
    UINT32 val = 0;
    T32ADPSCL_CFG15 stSclCfg15;
    T32ADPSCL_CFG0 stSclCfg0;
    CHECK_FRC_SCL_NUM(UnitNum);
#if defined(CONFIG_VPP_ENABLE_SCL_CLKGATE)
    switch(UnitNum)
    {
        case VPP_FRC_SCL_MAIN:
#ifdef VPP_FRC_SCL_CLOCK_GATE_MAIN_SCL_COMPLETLY
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_vidpl + RA_vidpl_MAINPL + RA_MAINPL_CTRL4;
            vpp_obj->mainpl_ctrl4.uCTRL4_adpScl_bypass = enable;
            val = vpp_obj->mainpl_ctrl4.u32;
#else
        {
            stSclCfg0.uCFG0_ctrl0 = vpp_obj->uiSclCfg0[UnitNum];
            if(enable)
                stSclCfg0.uCFG0_ctrl0 |= 0x10;
            else
                stSclCfg0.uCFG0_ctrl0 &= ~(0x10);
            uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_ADPSCL_CFG0, stSclCfg0.u32));
            vpp_obj->uiSclCfg0[UnitNum] = stSclCfg0.u32;
            uiRegAddr = INTERNAL_FRC_SCL_GetRegStartAddr(vpp_obj, UnitNum);
            uiRegAddr += RA_ADPSCL_CFG15;
            stSclCfg15.u32 = vpp_obj->uiSclCfg15[UnitNum];
            stSclCfg15.uCFG15_bypass = enable;
            vpp_obj->uiSclCfg15[UnitNum] = stSclCfg15.u32;
            val = vpp_obj->uiSclCfg15[UnitNum];
		}
#endif
            break;
        case VPP_FRC_SCL_PIP:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_SCL_BYPASS;
            vpp_obj->gfx0_scl_bypass.u32 = 0;
            vpp_obj->gfx0_scl_bypass.uSCL_BYPASS_en = enable;
            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr, vpp_obj->gfx0_scl_bypass.u32));
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_GFX0 + RA_GFX0_CTRL;
            vpp_obj->gfx0_ctrl.uCTRL_gfx_sclEnb = (enable ^ 1);
            val = vpp_obj->gfx0_ctrl.u32;
            break;
        case VPP_FRC_SCL_OSD:
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_SCL_BYPASS;
            vpp_obj->gfx1_scl_bypass.u32 = 0;
            vpp_obj->gfx1_scl_bypass.uSCL_BYPASS_en = enable;
            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr, vpp_obj->gfx1_scl_bypass.u32));
            uiRegAddr = vpp_obj->base_addr + RA_Vpp_GFX1 + RA_GFX1_CTRL;
            vpp_obj->gfx1_ctrl.uCTRL_gfx_sclEnb = (enable ^ 1);
            val = vpp_obj->gfx1_ctrl.u32;
            break;
        case VPP_FRC_SCL_VIP:
            uiRegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE;
            /* path configuration */
            vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.uCTRL_sb_clkg_en = (enable^1);
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_scl1d_bypass = enable;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_dns444_422_bypass = enable;
            vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.uBYPASS_CTRL_dither_bypass = enable;

            //CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_CTRL, vpp_obj->stVppWriteBackCtrl.m_stHdmiPipeCtrl.u32));
            CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr + RA_HDMIRX_PIPE_BYPASS_CTRL, vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.u32));
            uiRegAddr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE + RA_HDMIRX_PIPE_BYPASS_CTRL;
            val = vpp_obj->stVppWriteBackCtrl.m_stBypassCtrl.u32;

            break;
        default:
            return MV_VPP_EBADPARAM;
            break;
    }
    CHECK_VPP_SCL_RETVAL(VPP_BCMBUF_Write_DBG(vpp_obj->pVbiBcmBuf, uiRegAddr, val));
    VPP_SRAM_PowerGate_SCL(vpp_obj, UnitNum, enable?VPP_SRAM_PWRGATE_ENABLE:VPP_SRAM_PWRGATE_DISABLE);
    if(0 == enable)
    {
         vpp_obj->hscl_coeff[UnitNum] = -1; // invalid
         vpp_obj->vscl_coeff[UnitNum] = -1; // invalid
    }
#endif //CONFIG_VPP_ENABLE_SCL_CLKGATE
    return uiRetVal;
}
#endif /* #ifdef EN_VPP_DYNAMIC_CLKGATE */

#if defined(VPP_SUSPEND_RESUME_CLKGATE_ENABLE)
/***************************************************************************************
 * FUNCTION: This function enable/disable the RA Gbl  clock
 * PARAMS: u32ClkRegAddr - Clock Reg Address
 *         bClkState - clkD2_ctrl_ClkEn_disable/clkD2_ctrl_ClkEn_enable
 *
 * RETURN:
 ***************************************************************************************/
void MV_RA_Gbl_ClkD2_Ctrl(UINT32 u32ClkRegAddr, BOOL bClkState)
{
    T32clkD2_ctrl stclkD2_ctrl;

    GA_REG_WORD32_READ(MEMMAP_CHIP_CTRL_REG_BASE + u32ClkRegAddr + RA_clkD2_ctrl, &stclkD2_ctrl.u32);
    stclkD2_ctrl.uctrl_ClkEn = bClkState;
    GA_REG_WORD32_WRITE(MEMMAP_CHIP_CTRL_REG_BASE + u32ClkRegAddr + RA_clkD2_ctrl, stclkD2_ctrl.u32);
}

/***************************************************************************************
 * FUNCTION:[Main Plane]SRAM SHUTDWN/ON for the adaptive scalar of Line Buffer Memories
 * PARAMS: bSramPwrCtrlState - SRAMPWR_ctrl_SD_SHUTDWN/SRAMPWR_ctrl_SD_ON
 *
 * RETURN:
 ***************************************************************************************/
static void MV_SCLLB_SRAMPWR_Ctrl(BOOL bSramPwrCtrlState)
{
#ifdef VPP_SRAM_PWR_CTRL_ENABLE
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG((AVIO_MEMMAP_VPP_REG_BASE + RA_Vpp_vidpl), RA_vidpl_SRAMPWR_MP_1DSCL_LB)\
                                                    ,bSramPwrCtrlState);
#endif
}

/***************************************************************************************
 * FUNCTION:[Main Plane]SRAM SHUTDWN/ON for the adaptive scalar of LUT Memories
 * PARAMS: bSramPwrCtrlState - SRAMPWR_ctrl_SD_SHUTDWN/SRAMPWR_ctrl_SD_ON
 *
 * RETURN:
 ***************************************************************************************/
static void MV_SCLUT_SRAMPWR_Ctrl(BOOL bSramPwrCtrlState)
{
    /* In vs6xx SCL LUT SRAM power down cause green/red/purple screen issue
     * Isssue reported in 7 out of 50 boxes tested
     * So VPP_SCL_LUT_CLKGATE_ENABLE not defined in vs6xx*/
#ifdef VPP_SCL_LUT_CLKGATE_ENABLE
    SRAM_POWER_CONTROL(\
            SRAMPWR_CTRTL_REG((AVIO_MEMMAP_VPP_REG_BASE + RA_Vpp_vidpl), RA_vidpl_SRAMPWR_MP_1DSCL_LUT)\
            ,bSramPwrCtrlState);
#endif
}

/***************************************************************************************
 * FUNCTION:SRAM SHUTDWN/ON for the MTR Memories
 * PARAMS: bSramPwrCtrlState - SRAMPWR_ctrl_SD_SHUTDWN/SRAMPWR_ctrl_SD_ON
 *
 * RETURN:
 ***************************************************************************************/
static void MV_MTR_SRAMPWR_Ctrl(BOOL bSramPwrCtrlState)
{
#ifdef VPP_SRAM_PWR_CTRL_ENABLE
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_AVIO_GBL_BASE, RA_avioGbl_MTR_SRAMPWR)\
                                                    ,bSramPwrCtrlState);
#endif
}

/***************************************************************************************
 * FUNCTION:SRAM SHUTDWN/ON for VOP GFX0/GFX1/GFX2
 * PARAMS: bSramPwrCtrlState - SRAMPWR_ctrl_SD_SHUTDWN/SRAMPWR_ctrl_SD_ON
 *
 * RETURN:
 ***************************************************************************************/
static void MV_VOP_SRAMPWR_Ctrl(BOOL bSramPwrCtrlState)
{
    /* VPP_SRAM_PWR_CTRL_ENABLE  not enabled as it results in blurry UI issue in 5 out
     * of 50 boxes tried */
#ifdef VPP_SRAM_PWR_CTRL_ENABLE
#ifdef ENABLE_PIP_GFX0_PATH
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_SCLLUT_GFX0)\
                                                ,bSramPwrCtrlState);
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_SCLLB_GFX0)\
                                                ,bSramPwrCtrlState);
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_UPS420SP_GFX0)\
                                                ,bSramPwrCtrlState);
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_HDRLUT_GFX0)\
                                                ,bSramPwrCtrlState);

    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_GDCM_GFX0)\
                                                ,bSramPwrCtrlState);
#endif
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_SCLLUT_GFX1)\
                                                ,bSramPwrCtrlState);
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_SCLLB_GFX1)\
                                                ,bSramPwrCtrlState);

    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_HDRLUT_GFX1)\
                                                ,bSramPwrCtrlState);
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_GDCM_GFX1)\
                                                ,bSramPwrCtrlState);
#ifdef ENABLE_GFX2_VOP2_PATH
    SRAM_POWER_CONTROL(\
        SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_SCLLUT_GFX2)\
                                                ,bSramPwrCtrlState);
    SRAM_POWER_CONTROL(\
     SRAMPWR_CTRTL_REG(AVIO_MEMMAP_VPP_GFX_VOP_REG_BASE, RA_VPP_GFX_VOP_SRAMPWR_SCLLB_GFX2)\
                                                ,bSramPwrCtrlState);
#endif
#endif
}

/***************************************************************************************
 * FUNCTION:vPLL0 & vPLL0 Clock Enable/Disable
 * PARAMS: bAvpllClkState
 *
 * RETURN:
 ***************************************************************************************/
static void MV_vPLL_Clock_Disable(BOOL bAvpllClkState)
{
    T32avioGbl_AVPLL_CTRL0 stAvpllClkCtrl;

    GA_REG_WORD32_READ(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_AVPLL_CTRL0, \
                                                                             &stAvpllClkCtrl.u32);
    stAvpllClkCtrl.uAVPLL_CTRL0_VPLL0_PD = bAvpllClkState;
#ifdef ENABLE_GFX2_VOP2_PATH
    stAvpllClkCtrl.uAVPLL_CTRL0_VPLL1_PD = bAvpllClkState;
#endif
    GA_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_AVPLL_CTRL0, \
                                                                               stAvpllClkCtrl.u32);
}
#endif /*  defined(VPP_SUSPEND_RESUME_CLKGATE_ENABLE) */

/***************************************************************************************
 * FUNCTION:MV_VPP_EnableClkGate
 * PARAMS:
 *
 * RETURN:
 ***************************************************************************************/
void MV_VPP_EnableClkGate(void)
{
#if defined(VPP_SUSPEND_RESUME_CLKGATE_ENABLE)
    MV_RA_Gbl_ClkD2_Ctrl(RA_Gbl_vppSysClk, clkD2_ctrl_ClkEn_disable);
    MV_RA_Gbl_ClkD2_Ctrl(RA_Gbl_vipPipeClk, clkD2_ctrl_ClkEn_disable);
    MV_vPLL_Clock_Disable(TRUE);
    MV_VOP_SRAMPWR_Ctrl(SRAMPWR_ctrl_SD_SHUTDWN);
    MV_SCLLB_SRAMPWR_Ctrl(SRAMPWR_ctrl_SD_SHUTDWN);
    MV_SCLUT_SRAMPWR_Ctrl(SRAMPWR_ctrl_SD_SHUTDWN);
    MV_MTR_SRAMPWR_Ctrl(SRAMPWR_ctrl_SD_SHUTDWN);
#endif
}

/***************************************************************************************
 * FUNCTION:MV_VPP_DisableClkGate
 * PARAMS:
 *
 * RETURN:
 ***************************************************************************************/
void MV_VPP_DisableClkGate(void)
{
#if defined(VPP_SUSPEND_RESUME_CLKGATE_ENABLE)
    MV_MTR_SRAMPWR_Ctrl(SRAMPWR_ctrl_SD_ON);
    MV_SCLUT_SRAMPWR_Ctrl(SRAMPWR_ctrl_SD_ON);
    MV_SCLLB_SRAMPWR_Ctrl(SRAMPWR_ctrl_SD_ON);
    MV_VOP_SRAMPWR_Ctrl(SRAMPWR_ctrl_SD_ON);
    MV_vPLL_Clock_Disable(FALSE);
    MV_RA_Gbl_ClkD2_Ctrl(RA_Gbl_vipPipeClk, clkD2_ctrl_ClkEn_enable);
    MV_RA_Gbl_ClkD2_Ctrl(RA_Gbl_vppSysClk, clkD2_ctrl_ClkEn_enable);
#endif
}
