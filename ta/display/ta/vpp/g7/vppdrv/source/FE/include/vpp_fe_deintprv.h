/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPPDEINTPRV__
#define __VPPDEINTPRV__

#include "com_type.h"
#include "vpp_fe_deintcfg.h"
#include "vpp_fe_deint.h"

//FIXME: need common place
/*the offset plus with input resolution to set dummy TG size*/
#define VPP_FE_VP_TG_SIZE_H_OFF_P 26
#define VPP_FE_VP_TG_SIZE_V_OFF_P 10

#define VPP_FE_VP_TG_SIZE_H_OFF_I 26
#define VPP_FE_VP_TG_SIZE_V_OFF_I 10

/*dummy TG HSync*/
#define VPP_FE_VP_TG_HS_FE 1
#define VPP_FE_VP_TG_HS_BE 3

/*dummy TG HBlank*/
#define VPP_FE_VP_TG_HB_FE_OFF 8 //VPP_FE_VP_TG_HB_BE + 1
#define VPP_FE_VP_TG_HB_BE     7 //VPP_FE_VP_TG_HS_BE + 4

/*dummy TG VSync0*/
#define VPP_FE_VP_TG_VS0_FE 1
#define VPP_FE_VP_TG_VS0_BE 2

/*dummy TG VBlank*/
#define VPP_FE_VP_TG_VB0_FE_OFF 3//VPP_FE_VP_TG_VS0_BE + 1
#define VPP_FE_VP_TG_VB0_BE     2 //VPP_FE_VP_TG_VS0_BE


/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */
/*load register default value by name*/
#define WRITE_VPDEINT_DEFAULT_VAL(RegName, DefValues) \
        VPP_BCMBUF_Write(vpp_obj->pVbiBcmBuf, vpp_obj->base_addr + (RegName << 2), DefValues.REG_##RegName)

typedef struct VPP_VP_T {
    UINT32 REG_VP_DEINT_MODE;
    UINT32 REG_VP_CTRL;
    UINT32 REG_VP_FB_CTRL;
}VPP_VP, *PVPP_VP;

typedef struct VPP_DEINT_T {
    UINT32 REG_DIN0;
    UINT32 REG_DIN1;
    UINT32 REG_DIN2;
    UINT32 REG_DIN3;
    UINT32 REG_DIN4;
    UINT32 REG_DIN5;
    UINT32 REG_DIN8;
    UINT32 REG_DIN9;
    UINT32 REG_DIN10;
    UINT32 REG_DIN11;
    UINT32 REG_DIN12;
    UINT32 REG_DIN13;
    UINT32 REG_DIN14;
    UINT32 REG_DIN15;
    UINT32 REG_DIN16;
    UINT32 REG_DIN17;
    UINT32 REG_DIN18;
    UINT32 REG_DIN19;
    UINT32 REG_DIN20;
    UINT32 REG_DIN21;
    UINT32 REG_DIN22;
    UINT32 REG_DIN23;
    UINT32 REG_DIN24;
    UINT32 REG_DIN25;
    UINT32 REG_DIN26;
    UINT32 REG_DIN27;
    UINT32 REG_DIN28;
    UINT32 REG_DIN29;
    UINT32 REG_DIN30;
    UINT32 REG_DIN31;
    UINT32 REG_DIN32;
    UINT32 REG_DIN33;
    UINT32 REG_DIN34;
    UINT32 REG_DIN37;
    UINT32 REG_DIN38;
    UINT32 REG_DIN39;
    UINT32 REG_DIN40;
    UINT32 REG_DIN41;
    UINT32 REG_DIN42;
    UINT32 REG_DIN43;
    UINT32 REG_DIN44;
    UINT32 REG_DIN46;
    UINT32 REG_DIN47;
    UINT32 REG_DIN48;
    UINT32 REG_DIN50;
    UINT32 REG_DIN51;
    UINT32 REG_DIN52;
    UINT32 REG_DIN53;
    UINT32 REG_DIN54;
    UINT32 REG_DIN55;
    UINT32 REG_DIN56;
    UINT32 REG_DIN57;
    UINT32 REG_DIN58;
    UINT32 REG_DIN59;
    UINT32 REG_DIN60;
    UINT32 REG_DIN61;
    UINT32 REG_DIN62;
    UINT32 REG_DIN63;
    UINT32 REG_DIN64;
    UINT32 REG_DIN65;
    UINT32 REG_DIN66;
    UINT32 REG_DIN67;
    UINT32 REG_DIN68;
    UINT32 REG_DIN72;
    UINT32 REG_DIN73;
    UINT32 REG_DIN74;
    UINT32 REG_DIN75;
    UINT32 REG_DIN76;
    UINT32 REG_DIN77;
    UINT32 REG_DIN78;
    UINT32 REG_DIN79;
    UINT32 REG_DIN80;
    UINT32 REG_DIN81;
    UINT32 REG_DIN82;
    UINT32 REG_DIN84;
    UINT32 REG_DIN85;
    UINT32 REG_DIN86;
    UINT32 REG_DIN87;
    UINT32 REG_DIN88;
    UINT32 REG_DIN89;
    UINT32 REG_DIN90;
    UINT32 REG_DIN91;
    UINT32 REG_DIN92;
    UINT32 REG_DIN93;
    UINT32 REG_DIN94;
    UINT32 REG_DIN95;
    UINT32 REG_DIN96;
    UINT32 REG_DIN97;
    UINT32 REG_DIN98;
    UINT32 REG_DIN99;
    UINT32 REG_DIN100;
    UINT32 REG_DIN101;
    UINT32 REG_DIN102;
    UINT32 REG_DIN103;
    UINT32 REG_DIN104;
    UINT32 REG_DIN105;
    UINT32 REG_DIN106;
    UINT32 REG_DIN107;
    UINT32 REG_DIN108;
    UINT32 REG_DIN109;
    UINT32 REG_DIN110;
    UINT32 REG_DIN111;
    UINT32 REG_DIN112;
    UINT32 REG_DIN113;
    UINT32 REG_DIN114;
    UINT32 REG_DIN115;
    UINT32 REG_DIN116;
    UINT32 REG_DIN117;
    UINT32 REG_DIN118;
    UINT32 REG_DIN119;
    UINT32 REG_DIN120;
    UINT32 REG_DIN121;
    UINT32 REG_DIN122;
    UINT32 REG_DIN143;
}VPP_DEINT, *PVPP_DEINT;

const VPP_VP gVpDefValues = {
    .REG_VP_DEINT_MODE    = 0x70,
    //.REG_VP_CTRL          = 0x24, /* original 0x20 */
    .REG_VP_CTRL          = 0x64, /* original 0x20 */
    .REG_VP_FB_CTRL       = 0x3e, /* original 0x3f */ //20 bit mode - 3
};

const VPP_DEINT gDeintDefValues = {
    .REG_DIN0    = 0x44,    // DIN_STM_CTRL1
    .REG_DIN1    = 0xC0,    // DIN_STM_CTRL2
    .REG_DIN2    = 0x77,    // DIN_VI_CTRL
    .REG_DIN3    = 0x0f,    // DIN_VI_HPF_UTH
    .REG_DIN4    = 0x0a,    // DIN_VI_HPF_LTH
    .REG_DIN5    = 0x14,    // DIN_VI_CUED_TH
    .REG_DIN8    = 0x00,    // DIN_EXC_B
    .REG_DIN9    = 0x11,    // DIN_EXC_T
    .REG_DIN10   = 0x00,    // DIN_EXC_R
    .REG_DIN11   = 0x00,    // DIN_EXC_L
    .REG_DIN12   = 0x56,    // DIN_MCB_CTRL1
    .REG_DIN13   = 0x7F,    // DIN_MCB_CTRL2
    .REG_DIN14   = 0x04,    // DIN_MCB_CTRL3
    .REG_DIN15   = 0x8b,    // DIN_ME_CTRL1
    .REG_DIN16   = 0xCB,    // DIN_ME_CTRL2
    .REG_DIN17   = 0x01,    // DIN_MAD_VID_THL
    .REG_DIN18   = 0x09,    // DIN_MAD_VID_THH
    .REG_DIN19   = 0x10,    // DIN_MAD_FLM_TH
    .REG_DIN20   = 0x02,    // DIN_MAD2_VID_THL
    .REG_DIN21   = 0x22,    // DIN_MAD2_VID_THH
    .REG_DIN22   = 0x18,    // DIN_MAD2_FLM_TH
    .REG_DIN23   = 0x01,    // DIN_MAX_VID_THL
    .REG_DIN24   = 0x07,    // DIN_MAX_VID_THH
    .REG_DIN25   = 0x06,    // DIN_MAX_STH1
    .REG_DIN26   = 0x03,    // DIN_MAX_STH2
    .REG_DIN27   = 0x02,    // DIN_FLD_THL
    .REG_DIN28   = 0x1e,    // DIN_FLD_THH
    .REG_DIN29   = 0x0a,    // DIN_WVE_TH
    .REG_DIN30   = 0x24,    // DIN_WVE1_GAIN
    .REG_DIN31   = 0x30,    // DIN_WVE2_GAIN
    .REG_DIN32   = 0x0F,    // DIN_WVE3_TH1
    .REG_DIN33   = 0x06,    // DIN_WVE3_TH2
    .REG_DIN34   = 0x48,    // DIN_WVE3_GAIN
    .REG_DIN37   = 0x06,    // DIN_WVE_PE_TH1
    .REG_DIN38   = 0x07,    // DIN_WVE_PE_TH2
    .REG_DIN39   = 0x0f,    // DIN_PE_MASK
    .REG_DIN40   = 0x02,    // DIN_CHR_THL
    .REG_DIN41   = 0x1e,    // DIN_CHR_THH
    .REG_DIN42   = 0x00,    // DIN_NC_REG0_THRESH
    .REG_DIN43   = 0x60,    // DIN_NC_REG0_THRESH
    .REG_DIN44   = 0x00,    // DIN_NC_REG0_THRESH
    .REG_DIN46   = 0x00,    // DIN_NC_REG1_THRESH
    .REG_DIN47   = 0xC0,    // DIN_NC_REG1_THRESH
    .REG_DIN48   = 0x00,    // DIN_NC_REG1_THRESH
    .REG_DIN50   = 0x00,    // DIN_NC_REG2_THRESH
    .REG_DIN51   = 0x80,    // DIN_NC_REG2_THRESH
    .REG_DIN52   = 0x01,    // DIN_NC_REG2_THRESH
    .REG_DIN53   = 0x0D,    // DIN_NC_PIX_MAD_TH0
    .REG_DIN54   = 0x10,    // DIN_NC_PIX_MAD_TH1
    .REG_DIN55   = 0x14,    // DIN_NC_PIX_MAD_TH2
    .REG_DIN56   = 0x18,    // DIN_NC_PIX_MAD_TH3
    .REG_DIN57   = 0x12,    // DIN_NC_GLB_MAD_TH0
    .REG_DIN58   = 0x12,    // DIN_NC_GLB_MAD_TH1
    .REG_DIN59   = 0x12,    // DIN_NC_GLB_MAD_TH2
    .REG_DIN60   = 0x0C,    // DIN_NC_GLB_MAD_TH3
    .REG_DIN61   = 0x01,    // DIN_PF_MADTH
    .REG_DIN62   = 0x20,    // DIN_PEXIT_TH_F
    .REG_DIN63   = 0x12,    // DIN_PEXIT_TH_F
    .REG_DIN64   = 0x00,    // DIN_PEXIT_TH_C
    .REG_DIN65   = 0x7f,    // DIN_FCH_CTRL // 0x6F
    .REG_DIN66   = 0x3f,    // DIN_FCH_EN
    .REG_DIN67   = 0x03,    // DIN_FCH_EN
    .REG_DIN68   = 0x00,    // DIN_FCH_PFMODE
    .REG_DIN72   = 0x7d,    // DIN_FCH_32CTRL0
    .REG_DIN73   = 0xea,    // DIN_FCH_32CTRL1
    .REG_DIN74   = 0x08,    // DIN_FCH_32NOF0
    .REG_DIN75   = 0x02,    // DIN_FCH_32NOF1
    .REG_DIN76   = 0x1D,//0xFF,    // DIN_FCH_22CTRL
    .REG_DIN77   = 0x10,    // DIN_FCH_22NOF0 //Fix
    .REG_DIN78   = 0x00,    // DIN_FCH_22NOF1 //Fix
    .REG_DIN79   = 0x05,    // DIN_FCH_SCNOF0 //Fix
    .REG_DIN80   = 0x00,    // DIN_FCH_SCNOF1 //Fix
    .REG_DIN81   = 0x1f,    // DIN_FCH_SCCTRL0 // Fix
    .REG_DIN82   = 0xea,    // DIN_FCH_SCCTRL1
    .REG_DIN84   = 0x00,    // DIN_FCH_SC_PAT0
    .REG_DIN85   = 0x00,    // DIN_FCH_SC_PAT0
    .REG_DIN86   = 0x00,    // DIN_FCH_SC_PAT0
    .REG_DIN87   = 0x00,    // DIN_FCH_SC_PAT0
    .REG_DIN88   = 0x00,    // DIN_FCH_SC_PAT1
    .REG_DIN89   = 0x00,    // DIN_FCH_SC_PAT1
    .REG_DIN90   = 0x00,    // DIN_FCH_SC_PAT1
    .REG_DIN91   = 0x00,    // DIN_FCH_SC_PAT1
    .REG_DIN92   = 0x00,    // DIN_FCH_SC_LEN
    .REG_DIN93   = 0x07,    // New Added Fix  Doubt
    .REG_DIN94   = 0x18,    // DIN_FRM_FXD_TH
    .REG_DIN95   = 0x2A,    // DIN_FLD_ENT_TH0
    .REG_DIN96   = 0x54,    // DIN_FLD_ENT_TH1
    .REG_DIN97   = 0x48,    // DIN_FLD_EXT1_TH0
    .REG_DIN98   = 0x24,    // DIN_FLD_EXT1_TH0A
    .REG_DIN99   = 0x2A,    // DIN_FLD_EXT1_TH1
    .REG_DIN100  = 0x00,    // DIN_FLD_EXT2_TH0
    .REG_DIN101  = 0x2A,    // DIN_FLD_EXT2_TH0
    .REG_DIN102  = 0x00,    // DIN_FLD_EXT2_TH1
    .REG_DIN103  = 0x2A,    // DIN_FLD_EXT2_TH1
    .REG_DIN104  = 0xCA,    // DIN_GLB_MOT
    .REG_DIN105  = 0x02,    // DIN_GLB_MOT
    .REG_DIN106  = 0x00,    // DIN_FCH_IND
    .REG_DIN107  = 0x00,    // DIN_STM_SBF0
    .REG_DIN108  = 0x01,    // DIN_STM_SBF1
    .REG_DIN109  = 0x02,    // DIN_STM_SBF2
    .REG_DIN110  = 0x03,    // DIN_STM_SBF3
    .REG_DIN111  = 0x04,    // DIN_STM_SBF4
    .REG_DIN112  = 0x05,    // DIN_STM_SBF5
    .REG_DIN113  = 0x06,    // DIN_STM_SBF6
    .REG_DIN114  = 0x07,    // DIN_STM_SBF7
    .REG_DIN115  = 0x08,    // DIN_STM_SBF8
    .REG_DIN116  = 0x09,    // DIN_STM_SBF9
    .REG_DIN117  = 0x0a,    // DIN_STM_SBF10
    .REG_DIN118  = 0x0b,    // DIN_STM_SBF11
    .REG_DIN119  = 0x0e,    // DIN_STM_SBF12
    .REG_DIN120  = 0x0f,    // DIN_STM_SBF13
    .REG_DIN121  = 0x10,    // DIN_STM_SBF14
    .REG_DIN122  = 0x10,    // DIN_STM_SBF15
    .REG_DIN143  = 0x00,
};


/*-----------------------------------------------------------------------------
 * Extern variables
 *-----------------------------------------------------------------------------
 */

//////////////////////////////DEFAULT CFG VALUES/////////////////////////////////////////////

extern VPP_DEINTCFG_SPATIO_TEMP_MIXER_CTRL VppDeintCfgSpatioTempMixerCtrl[];
extern const VPP_DEINTCFG_DEINT_PRESET_MODE_PARAMS VppDeintCfgPresetModeParam[];
extern VPP_DEINT_THRES_PARAMS VppDeintCfgThresParams[];

/*-----------------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 * Structure Definitions for Video Processor
 *-----------------------------------------------------------------------------
 */

typedef struct tagVP_DEINT_MODE_REG {
    REG_WIDTH Mode                           : 3;
    REG_WIDTH reserved                       : 4;
    REG_WIDTH FiPol                          : 1;
}VP_DEINT_MODE_REG;

typedef union tagVP_DEINT_MODE_BITS {
    VP_DEINT_MODE_REG Bits;
    REG_WIDTH Value;
}VP_DEINT_MODE_BITS;

typedef struct tagVP_CTRL_FIFO_CTRL {
    REG_WIDTH Rst                            : 1;
    REG_WIDTH WenMode                        : 1;
    REG_WIDTH Mode                           : 2;
    REG_WIDTH ManualReset                    : 1;
    REG_WIDTH AutoReset                      : 1;
    REG_WIDTH VpEnable                       : 1;
    REG_WIDTH DeintEnable                    : 1;
}VP_CTRL_FIFO_CTRL;

typedef union tagVP_CTRL_FIFO_CTRL_BITS {
    VP_CTRL_FIFO_CTRL Bits;
    REG_WIDTH Value;
}VP_CTRL_FIFO_CTRL_BITS;

/*-----------------------------------------------------------------------------
 * Structure Definitions for Deinterlacer
 *-----------------------------------------------------------------------------
 */

//////////////////////////////////////////////////////////
typedef struct tagDEINT_STM_CTRL1_REG {
    REG_WIDTH LumaVidCtrl                      : 2;
    REG_WIDTH ChromaVidCtrl                    : 2;
    REG_WIDTH LumaFlmCtrl                      : 2;
    REG_WIDTH ChromaFlmCtrl                    : 2;
}DEINT_STM_CTRL1_REG;


typedef struct tagDEINT_STM_CTRL2_REG {
    REG_WIDTH LumaChromaYMotionSel             : 3;
    REG_WIDTH LumaChromaCMotionSel             : 3;
    REG_WIDTH TempTapSelVidMotion              : 2;
}DEINT_STM_CTRL2_REG;

typedef union tagDEINT_STM_CTRL1_REG_BITS {
    DEINT_STM_CTRL1_REG Bits;
    REG_WIDTH Value;
}DEINT_STM_CTRL1_REG_BITS;

typedef union tagDEINT_STM_CTRL2_REG_BITS {
    DEINT_STM_CTRL2_REG Bits;
    REG_WIDTH Value;
}DEINT_STM_CTRL2_REG_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_VI_CTRL_REG {
    REG_WIDTH EnShallowMinRej                : 1;
    REG_WIDTH EnSlope                        : 1;
    REG_WIDTH EnVarMeasure                   : 1;
    REG_WIDTH EnHighPassChar                 : 1;
    REG_WIDTH EnVectInterpol                 : 1;
    REG_WIDTH EnInterpolLumTrans             : 1;
    REG_WIDTH EnShallowMinDistFromVert       : 1;

}DEINT_VI_CTRL_REG;

typedef union tagDEINT_VI_CTRL_REG_BITS {
    DEINT_VI_CTRL_REG Bits;
    REG_WIDTH Value;
}DEINT_VI_CTRL_REG_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_MCB_CTRL_REG1 {
    REG_WIDTH FrmMotionSelCurrFld            : 4;
    REG_WIDTH FrmMotionSelPrevFld            : 2;
    REG_WIDTH CombFrmMotionSelCurrAndPrev    : 2;
}DEINT_MCB_CTRL_REG1;

typedef union tagDEINT_MCB_CTRL_REG1_BITS {
    DEINT_MCB_CTRL_REG1 Bits;
    REG_WIDTH Value;
}DEINT_MCB_CTRL_REG1_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_MCB_CTRL_REG2 {
    REG_WIDTH MotionComb                     : 2;
    REG_WIDTH MotionReCirCtrl                : 2;
    REG_WIDTH WegtAvForMotionRecCirCtrl      : 4;
}DEINT_MCB_CTRL_REG2;

typedef union tagDEINT_MCB_CTRL_REG2_BITS {
    DEINT_MCB_CTRL_REG2 Bits;
    REG_WIDTH Value;
}DEINT_MCB_CTRL_REG2_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_MCB_CTRL_REG3 {
    REG_WIDTH HorizExpMotionCtrl             : 2;
    REG_WIDTH FxdThres32FilmOutput           : 1;
    REG_WIDTH GradThres32FilmOutput          : 1;
}DEINT_MCB_CTRL_REG3;

typedef union tagDEINT_MCB_CTRL_REG3_BITS {
    DEINT_MCB_CTRL_REG3 Bits;
    REG_WIDTH Value;
}DEINT_MCB_CTRL_REG3_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_MOT_ENG_CTRL_REG1 {
    REG_WIDTH MADEngCtrl                     : 2;
    REG_WIDTH MAD2EngCtrl                    : 4;
    REG_WIDTH MAXEngCtrl                     : 2;
}DEINT_MOT_ENG_CTRL_REG1;

typedef union tagDEINT_MOT_ENG_CTRL_REG1_BITS {
    DEINT_MOT_ENG_CTRL_REG1 Bits;
    REG_WIDTH Value;
}DEINT_MOT_ENG_CTRL_REG1_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_MOT_ENG_CTRL_REG2 {
    REG_WIDTH VidFldMotionEngCtrl            : 3;
    REG_WIDTH ChromaMotionEngCtrl            : 2;
}DEINT_MOT_ENG_CTRL_REG2;

typedef union tagDEINT_MOT_ENG_CTRL_REG2_BITS {
    DEINT_MOT_ENG_CTRL_REG2 Bits;
    REG_WIDTH Value;
}DEINT_MOT_ENG_CTRL_REG2_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_FLM_CAD_CTRL_REG {
    REG_WIDTH EnSplitZoneDet32               : 1;
    REG_WIDTH EnSplitZoneDet22               : 1;
    REG_WIDTH EnTopZoneDet32                 : 1;
    REG_WIDTH EnTopZoneDet22                 : 1;
    REG_WIDTH EnPixelExcl32                  : 1;
    REG_WIDTH EnPixelExcl22                  : 1;
    REG_WIDTH EnPixelExclAuxCad              : 1;
}DEINT_FLM_CAD_CTRL_REG;

typedef union tagDEINT_FLM_CAD_CTRL_REG_BITS {
    DEINT_FLM_CAD_CTRL_REG Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_CTRL_REG_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_FLM_CAD_DET_MODE_REG1 {
    REG_WIDTH En2224Det                      : 1;
    REG_WIDTH En2332Det                      : 1;
    REG_WIDTH En32322Det                     : 1;
    REG_WIDTH En55Det                        : 1;
    REG_WIDTH En64Det                        : 1;
    REG_WIDTH En87Det                        : 1;
    REG_WIDTH EnPrgCad1Det                   : 1;
    REG_WIDTH EnPrgCad2Det                   : 1;
}DEINT_FLM_CAD_DET_MODE_REG1;

typedef struct tagDEINT_FLM_CAD_DET_MODE_REG2 {
    REG_WIDTH En32CadDet                     : 1;
    REG_WIDTH En22CadDet                     : 1;
}DEINT_FLM_CAD_DET_MODE_REG2;

typedef union tagDEINT_FLM_CAD_DET_MODE_REG1_BITS {
    DEINT_FLM_CAD_DET_MODE_REG1 Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_DET_MODE_REG1_BITS;

typedef union tagDEINT_FLM_CAD_DET_MODE_REG2_BITS {
    DEINT_FLM_CAD_DET_MODE_REG2 Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_DET_MODE_REG2_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_FLM_CAD_PROG_FLAG_REG {
    REG_WIDTH EnDinPixFrameMADThresh         : 1;
    REG_WIDTH EnFxdGblMotionThresh           : 1;
    REG_WIDTH EnPixelExclusion               : 1;
    REG_WIDTH EnSmallFxdWin                  : 1;
}DEINT_FLM_CAD_PROG_FLAG_REG;

typedef union tagDEINT_FLM_CAD_PROG_FLAG_REG_BITS {
    DEINT_FLM_CAD_PROG_FLAG_REG Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_PROG_FLAG_REG_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_FLM_CAD_32CTRL1_REG {
    REG_WIDTH EnWveErrSig1For32Exit          : 1;
    REG_WIDTH EnWveErrSig2For32Exit          : 1;
    REG_WIDTH EnDynFldExitFor32Exit          : 1;
    REG_WIDTH EnPrevFldExitFor32Exit         : 1;
    REG_WIDTH EnWithinFldExitFor32Exit       : 1;
    REG_WIDTH EnNoiseCalibFrmMotionFor32     : 1;
    REG_WIDTH EnPostExitFor32                : 1;
    REG_WIDTH En32DetForProgInput            : 1;
}DEINT_FLM_CAD_32CTRL1_REG;

typedef struct tagDEINT_FLM_CAD_32CTRL2_REG {
    REG_WIDTH BoolOperToCtrlEntryInto32      : 4;
    REG_WIDTH BoolOperToCtrlExitFrom32       : 4;
}DEINT_FLM_CAD_32CTRL2_REG;

typedef union tagDEINT_FLM_CAD_32CTRL1_REG_BITS {
    DEINT_FLM_CAD_32CTRL1_REG Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_32CTRL1_REG_BITS;

typedef union tagDEINT_FLM_CAD_32CTRL2_REG_BITS {
    DEINT_FLM_CAD_32CTRL2_REG Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_32CTRL2_REG_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_FLM_CAD_22CTRL_REG {
    REG_WIDTH EnWveErrSig1For22Exit          : 1;
    REG_WIDTH EnWveErrSig2For22Exit          : 1;
    REG_WIDTH EnDynFldExitFor22Exit          : 1;
    REG_WIDTH EnPrevFldExitFor22Exit         : 1;
    REG_WIDTH EnWithinFldExitFor22Exit       : 1;
}DEINT_FLM_CAD_22CTRL_REG;

typedef union tagDEINT_FLM_CAD_22CTRL_REG_BITS {
    DEINT_FLM_CAD_22CTRL_REG Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_22CTRL_REG_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_FLM_CAD_SPLCAD_CTRL1_REG {
    REG_WIDTH EnWveErrSig1ForSCExit          : 1;
    REG_WIDTH EnWveErrSig2ForSCExit          : 1;
    REG_WIDTH EnDynFldExitForSCExit          : 1;
    REG_WIDTH EnPrevFldExitForSCExit         : 1;
    REG_WIDTH EnWithinFldExitForSCExit       : 1;
    REG_WIDTH EnGradFrmMotionForSCEntry      : 1;
    REG_WIDTH                                : 2;
}DEINT_FLM_CAD_SPLCAD_CTRL1_REG;

typedef struct tagDEINT_FLM_CAD_SPLCAD_CTRL2_REG {
    REG_WIDTH BoolOperToCtrlEntryIntoSC      : 4;
    REG_WIDTH BoolOperToCtrlExitFromSC       : 4;
}DEINT_FLM_CAD_SPLCAD_CTRL2_REG;

typedef union tagDEINT_FLM_CAD_SPLCAD_CTRL1_REG_BITS {
    DEINT_FLM_CAD_SPLCAD_CTRL1_REG Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_SPLCAD_CTRL1_REG_BITS;

typedef union tagDEINT_FLM_CAD_SPLCAD_CTRL2_REG_BITS {
    DEINT_FLM_CAD_SPLCAD_CTRL2_REG Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_SPLCAD_CTRL2_REG_BITS;

//////////////////////////////////////////////////////////

typedef struct tagDEINT_FLM_CAD_SPL_CAD_LEN_REG {
    REG_WIDTH LenOfProgCadPat0       : 4;
    REG_WIDTH LenOfProgCadPat1       : 4;
}DEINT_FLM_CAD_SPL_CAD_LEN_REG;

typedef union tagDEINT_FLM_CAD_SPL_CAD_LEN_REG_BITS {
    DEINT_FLM_CAD_SPL_CAD_LEN_REG Bits;
    REG_WIDTH Value;
}DEINT_FLM_CAD_SPL_CAD_LEN_REG_BITS;


///////////////////////////////////LOCAL STATIC FUNCTIONS/////////////////////////////////////////
#ifdef CONFIG_DEINT_ENABLE
static void DeintSetThresholds(VPP_OBJ* vpp_obj,PVPP_DEINT_THRES_PARAMS pDeintThParams);
#endif

#endif // __VPPDEINTPRV__

