/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#define VPP_DEINT_CFG_C

/*-----------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------------
 */

#include "vpp_fe_deint.h"
#include "vpp_fe_deintcfg.h"


/*-----------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------------
 */

//////////////////////////////DEFAULT CFG VALUES///////////////////////////////////////////////////


VPP_DEINTCFG_SPATIO_TEMP_MIXER_CTRL VppDeintCfgSpatioTempMixerCtrl[] =
{
    //VPP_DEINTPRESET_2D_LINEAR
    {
        VPP_DEINTFORCED_SPATIAL,       //LumaCtrlForVideoPix
        VPP_DEINTFORCED_SPATIAL,       //ChromaCtrlForVideoPix
        VPP_DEINTFORCED_SPATIAL,       //LumaCtrlForFilmPix
        VPP_DEINTFORCED_SPATIAL,       //ChromaCtrlForFilmPix
        VPP_DEINTCFG_MAD2_ENG_MODE_LUMA_CHROMA_SEL_0, //LumaChromaMotSelForY
        VPP_DEINTNXT_FLD_AND_PREV_FLD  //TempTapSel
    },
    //VPP_DEINTPRESET_2D_VECTOR
    {
        VPP_DEINTFORCED_SPATIAL,       //LumaCtrlForVideoPix
        VPP_DEINTFORCED_SPATIAL,       //ChromaCtrlForVideoPix
        VPP_DEINTFORCED_SPATIAL,       //LumaCtrlForFilmPix
        VPP_DEINTFORCED_SPATIAL,       //ChromaCtrlForFilmPix
        VPP_DEINTCFG_MAD2_ENG_MODE_LUMA_CHROMA_SEL_0, //LumaChromaMotSelForY
        VPP_DEINTNXT_FLD_AND_PREV_FLD  //TempTapSel
    },
    //VPP_DEINTPRESET_3D_LINEAR
    {
        VPP_DEINTFORCED_TEMPORAL,      //LumaCtrlForVideoPix
        VPP_DEINTFORCED_TEMPORAL,      //ChromaCtrlForVideoPix
        VPP_DEINTFORCED_TEMPORAL,      //LumaCtrlForFilmPix
        VPP_DEINTFORCED_TEMPORAL,      //ChromaCtrlForFilmPix
        VPP_DEINTCFG_MAD2_ENG_MODE_LUMA_CHROMA_SEL_0, //LumaChromaMotSelForY
        VPP_DEINTNXT_FLD_AND_PREV_FLD  //TempTapSel
    },
    //VPP_DEINTPRESET_3D_VECTOR
    {
        VPP_DEINTFORCED_TEMPORAL,      //LumaCtrlForVideoPix
        VPP_DEINTFORCED_TEMPORAL,      //ChromaCtrlForVideoPix
        VPP_DEINTFORCED_TEMPORAL,      //LumaCtrlForFilmPix
        VPP_DEINTFORCED_TEMPORAL,      //ChromaCtrlForFilmPix
        VPP_DEINTCFG_MAD2_ENG_MODE_LUMA_CHROMA_SEL_0, //LumaChromaMotSelForY
        VPP_DEINTNXT_FLD_AND_PREV_FLD  //TempTapSel
    },
    //VPP_DEINTPRESET_3D_MA_LINEAR
    {
        VPP_DEINTMOTION_ADAPTIVE,      //LumaCtrlForVideoPix
        VPP_DEINTMOTION_ADAPTIVE,      //ChromaCtrlForVideoPix
        VPP_DEINTMOTION_ADAPTIVE,      //LumaCtrlForFilmPix
        VPP_DEINTMOTION_ADAPTIVE,      //ChromaCtrlForFilmPix
        VPP_DEINTCFG_MAD2_ENG_MODE_LUMA_CHROMA_SEL_0, //LumaChromaMotSelForY
        VPP_DEINTNXT_FLD_AND_PREV_FLD  //TempTapSel
    },
    //VPP_DEINTPRESET_3D_MA_VECTOR
    {
        VPP_DEINTMOTION_ADAPTIVE,      //LumaCtrlForVideoPix
        VPP_DEINTMOTION_ADAPTIVE,      //ChromaCtrlForVideoPix
        VPP_DEINTMOTION_ADAPTIVE,      //LumaCtrlForFilmPix
        VPP_DEINTMOTION_ADAPTIVE,      //ChromaCtrlForFilmPix
        VPP_DEINTCFG_MAD2_ENG_MODE_LUMA_CHROMA_SEL_0, //LumaChromaMotSelForY
        VPP_DEINTNXT_FLD_AND_PREV_FLD  //TempTapSel
    },
    //VPP_DEINTPRESET_2D_LINEAR_USE_CURR_FIELD
    {
        VPP_DEINTFORCED_SPATIAL,       //LumaCtrlForVideoPix
        VPP_DEINTFORCED_SPATIAL,       //ChromaCtrlForVideoPix
        VPP_DEINTFORCED_SPATIAL,       //LumaCtrlForFilmPix
        VPP_DEINTFORCED_SPATIAL,       //ChromaCtrlForFilmPix
        VPP_DEINTCFG_MAD2_ENG_MODE_LUMA_CHROMA_SEL_0, //LumaChromaMotSelForY
        VPP_DEINTNXT_FLD_AND_PREV_FLD  //TempTapSel
    },
    //VPP_DEINTPRESET_2D_VECTOR_USE_CURR_FIELD
    {
        VPP_DEINTFORCED_SPATIAL,       //LumaCtrlForVideoPix
        VPP_DEINTFORCED_SPATIAL,       //ChromaCtrlForVideoPix
        VPP_DEINTFORCED_SPATIAL,       //LumaCtrlForFilmPix
        VPP_DEINTFORCED_SPATIAL,       //ChromaCtrlForFilmPix
        VPP_DEINTCFG_MAD2_ENG_MODE_LUMA_CHROMA_SEL_0, //LumaChromaMotSelForY
        VPP_DEINTNXT_FLD_AND_PREV_FLD  //TempTapSel
    }
};

const VPP_DEINTCFG_DEINT_PRESET_MODE_PARAMS VppDeintCfgPresetModeParam[] =
{
    //VPP_DEINTPRESET_2D_LINEAR
    {
        VPP_DEINTMODE_SPATIAL_NEXT,                                    //DeintMode
        VPP_DEINTVI_NONE,                                              //ViCtrl
        &VppDeintCfgSpatioTempMixerCtrl[VPP_DEINTPRESET_2D_LINEAR]     //pSpatioTemporalMixerCtrl
    },
    //VPP_DEINTPRESET_2D_VECTOR
    {
        VPP_DEINTMODE_SPATIAL_NEXT,                                    //DeintMode
        VPP_DEINTVI_ALL,                                               // ViCtrl
        &VppDeintCfgSpatioTempMixerCtrl[VPP_DEINTPRESET_2D_VECTOR]     //pSpatioTemporalMixerCtrl
    },
    //VPP_DEINTPRESET_3D_LINEAR
    {
        VPP_DEINTMODE_MA_3D,                          //DeintMode
        VPP_DEINTVI_NONE,                                              // ViCtrl
        &VppDeintCfgSpatioTempMixerCtrl[VPP_DEINTPRESET_3D_LINEAR]            //pSpatioTemporalMixerCtrl
    },
    //VPP_DEINTPRESET_3D_VECTOR
    {
        VPP_DEINTMODE_MA_3D,                          //DeintMode
        VPP_DEINTVI_ALL,                                              // ViCtrl
        &VppDeintCfgSpatioTempMixerCtrl[VPP_DEINTPRESET_3D_VECTOR]            //pSpatioTemporalMixerCtrl
    },
    //VPP_DEINTPRESET_3D_MA_LINEAR
    {
        VPP_DEINTMODE_MA_3D,                                           // DeintMode
        VPP_DEINTVI_NONE,                                              // ViCtrl
        &VppDeintCfgSpatioTempMixerCtrl[VPP_DEINTPRESET_3D_MA_LINEAR]  //pSpatioTemporalMixerCtrl
    },
    //VPP_DEINTPRESET_3D_MA_VECTOR
    {
        VPP_DEINTMODE_MA_3D,                                           // DeintMode
        VPP_DEINTVI_ALL,                                               // ViCtrl
        &VppDeintCfgSpatioTempMixerCtrl[VPP_DEINTPRESET_3D_MA_VECTOR]  //pSpatioTemporalMixerCtrl
    },
    //VPP_DEINTPRESET_2D_LINEAR_USE_CURR_FIELD
    {
        VPP_DEINTMODE_SPATIAL_CURR,                                    //DeintMode
        VPP_DEINTVI_NONE,                                              //ViCtrl
        &VppDeintCfgSpatioTempMixerCtrl[VPP_DEINTPRESET_2D_LINEAR_USE_CURR_FIELD]     //pSpatioTemporalMixerCtrl
    },
    //VPP_DEINTPRESET_2D_VECTOR_USE_CURR_FIELD
    {
        VPP_DEINTMODE_SPATIAL_CURR,                                    //DeintMode
        VPP_DEINTVI_ALL,                                               // ViCtrl
        &VppDeintCfgSpatioTempMixerCtrl[VPP_DEINTPRESET_2D_VECTOR_USE_CURR_FIELD]     //pSpatioTemporalMixerCtrl
    }
};

VPP_DEINT_THRES_PARAMS VppDeintCfgThresParams[] =
{
    // VPP_DEINT_INPUT_HD
    {
        0x0A,   // DIN_WVE_TH
        0x10,//0x24,   // DIN_WVE1_GAIN
        0x15,//0x30,   // DIN_WVE2_GAIN
        0x04,//0x0F,   // DIN_WVE3_TH1
        0x02,//0x06,   // DIN_WVE3_TH2
        0x30,//0x48,   // DIN_WVE3_GAIN

        0x6000, // DIN_NC_REG0_THRESH
        0xC000, // DIN_NC_REG1_THRESH
        0x18000,// DIN_NC_REG2_THRESH
        0x0D,   // DIN_NC_PIX_MAD_TH0
        0x10,   // DIN_NC_PIX_MAD_TH1
        0x14,   // DIN_NC_PIX_MAD_TH2
        0x18,   // DIN_NC_PIX_MAD_TH3
        0x12,   // DIN_NC_GLB_MAD_TH0
        0x12,   // DIN_NC_GLB_MAD_TH1
        0x12,   // DIN_NC_GLB_MAD_TH2
        0x0C,   // DIN_NC_GLB_MAD_TH3

        0x01,   // DIN_PF_MADTH
        0x1220, // DIN_PEXIT_TH_F
        0x00,   // DIN_PEXIT_TH_C

        0x14,//0x10,   // DIN_FCH_22NOF0
        0x20,//0x0,   // DIN_FCH_22NOF1
        0x18,   // DIN_FRM_FXD_TH
        0x0A,   // DIN_FLD_ENT_TH0
        0x2A,   // DIN_FLD_ENT_TH1
        0x2E,   // DIN_FLD_EXT1_TH0
        0x03,//0x11,   // DIN_FLD_EXT1_TH0A
        0x2A,   // DIN_FLD_EXT1_TH1
        0x1080, // DIN_FLD_EXT2_TH0
        0x0420, // DIN_FLD_EXT2_TH1
        0x02CA, // DIN_GLB_MOT
    },

    // VPP_DEINT_INPUT_SD_NTSC
    {
        0x0A,   // DIN_WVE_TH
        0x10,//0x24,   // DIN_WVE1_GAIN
        0x15,//0x30,   // DIN_WVE2_GAIN
        0x04,//0x04,   // DIN_WVE3_TH1
        0x02,//0x06,   // DIN_WVE3_TH2
        0x30,//0x48,   // DIN_WVE3_GAIN

        0x1000, // DIN_NC_REG0_THRESH
        0x2000, // DIN_NC_REG1_THRESH
        0x4000, // DIN_NC_REG2_THRESH
        0x0D,   // DIN_NC_PIX_MAD_TH0
        0x10,   // DIN_NC_PIX_MAD_TH1
        0x14,   // DIN_NC_PIX_MAD_TH2
        0x18,   // DIN_NC_PIX_MAD_TH3
        0x03,   // DIN_NC_GLB_MAD_TH0
        0x03,   // DIN_NC_GLB_MAD_TH1
        0x03,   // DIN_NC_GLB_MAD_TH2
        0x02,   // DIN_NC_GLB_MAD_TH3

        0x01,   // DIN_PF_MADTH
        0x0320, // DIN_PEXIT_TH_F
        0x00,   // DIN_PEXIT_TH_C

        //0x20,   // DIN_FCH_22NOF0
        //0x30,   // DIN_FCH_22NOF1
        0x14,//0x08,   // DIN_FCH_22NOF0   //BG_FIX
        0x20,//0x00,   // DIN_FCH_22NOF1  //BG_FIX
        0x01,   // DIN_FRM_FXD_TH //Fix
        0x05,   // DIN_FLD_ENT_TH0
        0x0E,   // DIN_FLD_ENT_TH1
        0x0B,   // DIN_FLD_EXT1_TH0 //Fix
        0x03,//0x04,   // DIN_FLD_EXT1_TH0A //0x06 // Fix
        0x07,   // DIN_FLD_EXT1_TH1
       // 0x01A0, // DIN_FLD_EXT2_TH0
       // 0x0010, // DIN_FLD_EXT2_TH1
        0x02C0, // DIN_FLD_EXT2_TH0 //BG_FIX
        0x0100, // DIN_FLD_EXT2_TH1 //BG_FIX // Fix
        0x0077, // DIN_GLB_MOT
    },

    // VPP_DEINT_INPUT_SD_PAL
    {
        0x0A,   // DIN_WVE_TH
        0x10,//0x24,   // DIN_WVE1_GAIN
        0x15,//0x30,   // DIN_WVE2_GAIN
        0x04,//0x04,   // DIN_WVE3_TH1
        0x02,//0x06,   // DIN_WVE3_TH2
        0x30,//0x48,   // DIN_WVE3_GAIN

        0x1000, // DIN_NC_REG0_THRESH
        0x2000, // DIN_NC_REG1_THRESH
        0x4000, // DIN_NC_REG2_THRESH
        0x0D,   // DIN_NC_PIX_MAD_TH0
        0x10,   // DIN_NC_PIX_MAD_TH1
        0x14,   // DIN_NC_PIX_MAD_TH2
        0x18,   // DIN_NC_PIX_MAD_TH3
        0x03,   // DIN_NC_GLB_MAD_TH0
        0x03,   // DIN_NC_GLB_MAD_TH1
        0x03,   // DIN_NC_GLB_MAD_TH2
        0x02,   // DIN_NC_GLB_MAD_TH3

        0x01,   // DIN_PF_MADTH
        0x0320, // DIN_PEXIT_TH_F
        0x00,   // DIN_PEXIT_TH_C

        //0x20,   // DIN_FCH_22NOF0
        //0x30,   // DIN_FCH_22NOF1
        0x14,//0x08,   // DIN_FCH_22NOF0   //BG_FIX
        0x20,//0x00,   // DIN_FCH_22NOF1  //BG_FIX
        0x01,   // DIN_FRM_FXD_TH  // Fix
        0x05,   // DIN_FLD_ENT_TH0
        0x0E,   // DIN_FLD_ENT_TH1
        0x0B,   // DIN_FLD_EXT1_TH0 //Fix
        0x03,//0x04,   // DIN_FLD_EXT1_TH0A //Fix
        0x07,   // DIN_FLD_EXT1_TH1
        // 0x01A0, // DIN_FLD_EXT2_TH0
       // 0x0010, // DIN_FLD_EXT2_TH1
        0x02C0, // DIN_FLD_EXT2_TH0 //BG_FIX
        0x0100, // DIN_FLD_EXT2_TH1 //BG_FIX  //Fix
        0x0077, // DIN_GLB_MOT
    }
};
