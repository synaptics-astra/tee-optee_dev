/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */


#ifndef __VPP_DEINT_H__
#define __VPP_DEINT_H__

#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"

/*-----------------------------------------------------------------------------
 * Macros
 *-----------------------------------------------------------------------------
 */

#define VPP_DEINTMAX_SIZE_OF_SBF_LIST          (16)


/*-----------------------------------------------------------------------------
 * Enums, Structures and Unions
 *-----------------------------------------------------------------------------
 */
typedef struct tagVPP_DEINTCAPS
{
    UCHAR   NumUnits;
    UINT32  Flags;

} VPP_DEINTCAPS, *PVPP_DEINTCAPS;

typedef enum tagVPP_DEINTCADENCE_MODE_TYPE
{
    //Film Cadence Selection flags
    VPP_DEINTDISABLE_FLM_CAD = 0x000,
    VPP_DEINT32_FLM_CAD      = 0x001,
    VPP_DEINT22_FLM_CAD      = 0x002,
    VPP_DEINT2224_FLM_CAD    = 0x004,
    VPP_DEINT2332_FLM_CAD    = 0x008,
    VPP_DEINT32322_FLM_CAD   = 0x010,
    VPP_DEINT55_FLM_CAD      = 0x020,
    VPP_DEINT64_FLM_CAD      = 0x040,
    VPP_DEINT87_FLM_CAD      = 0x080,
    VPP_DEINTPROG_FLM_CAD1   = 0x100,
    VPP_DEINTPROG_FLM_CAD2   = 0x200,
    VPP_DEINTSELECT_ALL_CAD  = 0x3FF

}VPP_DEINTCADENCES;

typedef enum tagVPP_DEINTPRESET_MODE
{
    VPP_DEINTPRESET_2D_LINEAR = 0,
    VPP_DEINTPRESET_2D_VECTOR,
    VPP_DEINTPRESET_3D_LINEAR,
    VPP_DEINTPRESET_3D_VECTOR,
    VPP_DEINTPRESET_3D_MA_LINEAR,
    VPP_DEINTPRESET_3D_MA_VECTOR,
    VPP_DEINTPRESET_2D_LINEAR_USE_CURR_FIELD,
    VPP_DEINTPRESET_2D_VECTOR_USE_CURR_FIELD,
    VPP_DEINTPRESET_BYPASS_AND_FLD_BUFF_DISABLED,
    VPP_DEINTPRESET_BYPASS_AND_FLD_BUFF_ENABLED, //Support NR functionality
    VPP_DEINTPRESET_NONE, //Id not used currently in Kyoto
    VPP_DEINTPRESET_MAX_PRESETS,

} VPP_DEINTPRESET_MODE, *PVPP_DEINTPRESET_MODE;

typedef enum tagVPP_DEINTFLD_BUFF_MODE
{
    VPP_DEINTFLD_BUFF_MODE_20BIT = 0,
    VPP_DEINTFLD_BUFF_MODE_26BIT

}VPP_DEINTFLD_BUFF_MODE;

typedef enum tagVPP_DEINT_INPUT_TYPE
{
    VPP_DEINT_INPUT_HD = 0,
    VPP_DEINT_INPUT_SD_NTSC,
    VPP_DEINT_INPUT_SD_PAL,
    VPP_DEINT_INPUT_MAX
}VPP_DEINT_INPUT_TYPE;

typedef struct tagVPP_DEINTSTATUS
{
    UINT32  FilmCadenceDetFlag;

} VPP_DEINTSTATUS, *PVPP_DEINTSTATUS;

/////////////////////////////THRESHOLD PARAMETERS///////////////////////////////////////////////////
typedef struct tagVPP_DEINT_THRES_PARAMS
{
    // Weave error thresholds
    UINT8   InterFldDiffThresWveErrDet;                 // DIN29
    UINT8   IntraFldGainThresForWveErrDet22FlmExit;     // DIN30
    UINT8   IntraFldGainThresForWveErrDet22FlmEntry;    // DIN31

    UINT8   InterFldDiffThresForWveErrDet22FlmSecExit;  // DIN32
    UINT8   IntraFldDiffThresForWveErrDet22FlmSecExit;  // DIN33
    UINT8   IntraFldGainThresForWveErrDet22FlmSecExit;  // DIN34

    // Noise Calibration thresholds
    UINT32   NoiseCalibThresSADRegion0;                 // DIN42
    UINT32   NoiseCalibThresSADRegion1;                 // DIN46
    UINT32   NoiseCalibThresSADRegion2;                 // DIN50

    UINT8    NoiseCalibFramePixMotThresBelowReg0;       // DIN53
    UINT8    NoiseCalibFramePixMotThresBwReg0AndReg1;   // DIN54
    UINT8    NoiseCalibFramePixMotThresBwReg1AndReg2;   // DIN55
    UINT8    NoiseCalibFramePixMotThresAboveReg2;       // DIN56

    UINT8    NoiseCalibFrameGblMotThresBelowReg0;       // DIN57
    UINT8    NoiseCalibFrameGblMotThresBwReg0AndReg1;   // DIN58
    UINT8    NoiseCalibFrameGblMotThresBwReg1AndReg2;   // DIN59
    UINT8    NoiseCalibFrameGblMotThresAboveReg2;       // DIN60

    UINT8    PixelThresFor32Det;                        // DIN61
    UINT16   PostExitFineThresFor32;                    // DIN62
    UINT8    PostExitCoarseThresFor32;                  // DIN64

    // Global thresholds for film detection
    UINT8   NofInHardBldUpPh22Entry;                    // DIN77
    UINT8   NofInSoftBldUpPh22Entry;                    // DIN78
    UINT8   FixedGblFrameMotionThres;                   // DIN94
    UINT8   LowerGblWeaveErrThres;                      // DIN95
    UINT8   UpperGblWeaveErrThres;                      // DIN96

    UINT8   GblWeaveErrThresFor32FilmModeExit;          // DIN97
    UINT8   GblWeaveErrThresFor22FilmModeExit;          // DIN98
    UINT8   GblWeaveErrThresForFilmModeExit;            // DIN99
    UINT16  GblSecWeaveErrThres0ForFilmModeExit;        // DIN100
    UINT16  GblSecWeaveErrThres1ForFilmModeExit;        // DIN102
    UINT16  GblMotThresToCtrlFldMotion;                 // DIN104
}VPP_DEINT_THRES_PARAMS, *PVPP_DEINT_THRES_PARAMS;


typedef union tagVPP_VP_CLIENT_CLK
{
    struct {
        int    ClkEnable       : 1;
        int    WriteEnable     : 1;
        int    Read0Enable     : 1;
        int    Read1Enable     : 1;
        int    Reserved        :28;
    } field;

    int data;
} VPP_VP_CLIENT_CLK, *PVPP_VP_CLIENT_CLK;


typedef struct tagVPP_VP_CLIENT_SIZE
{
    int  ivRes;
    int  ihRes;
    int  IsProgressive;
}VPP_VP_CLIENT_SIZE,*PVPP_VP_CLIENT_SIZE;

typedef enum tagVPP_VP_CLIENT_TYPE
{
    VPP_VP_CLIENT_WRITE = 0,
    VPP_VP_CLIENT_READ0,
    VPP_VP_CLIENT_READ1,
    VPP_VP_CLIENT_MAX
}VPP_VP_CLIENT_TYPE;

typedef struct VPP_VP_CLIENT_RES_T
{
    UINT32 HRes;
    UINT32 VRes;

}VPP_VP_CLIENT_RES, *PVPP_VP_CLIENT_RES;


///////////////////////////////////Advanced Deint Mode config parameters////////////////////////////////



typedef enum tagVPP_DEINTCHROMA_MOT_FLM_MODE
{
    VPP_DEINTCHROMA_MOT_FORCED_2D_DISABLE = 0,
    VPP_DEINTCHROMA_MOT_FORCED_2D_ENABLE

}VPP_DEINTCHROMA_MOT_FLM_MODE;

typedef struct tagVP_FB_CTRL_REG {
    REG_WIDTH FldBufReset                    : 1;
    REG_WIDTH WrMask                         : 1;
    REG_WIDTH RdMaskFldBufLow                : 1;
    REG_WIDTH RdMaskFldBufHigh               : 1;
    REG_WIDTH CrcCheckFldBufLow              : 1;
    REG_WIDTH CrcCheckFldBufHigh             : 1;
    REG_WIDTH DeintClientMode                : 1;
}VP_FB_CTRL_REG;

typedef union tagVP_FB_CTRL_BITS {
    VP_FB_CTRL_REG Bits;
    REG_WIDTH Value;
}VP_FB_CTRL_BITS;

///////////////////////////////////Advanced Deint Mode config parameters////////////////////////////////

/*-----------------------------------------------------------------------------
 * Function Prototypes
 *-----------------------------------------------------------------------------
 */

INT FE_DEINT_SetDefaults(VPP_OBJ* vpp_obj);

INT FE_DEINT_SetPresetMode(VPP_OBJ* vpp_obj, int DeintPresetMode, PVPP_DINT_PRESET_MODE_PARAMS pMdParams);
INT FE_DEINT_SetInputParams(VPP_OBJ *vpp_obj, VPP_DEINT_INPUT_TYPE DeintInpType,
                            PVPP_DEINT_THRES_PARAMS pThresSettings);



INT FE_DEINT_SetAdvancedModeParams(VPP_OBJ* vpp_obj, PVPP_DEINTADV_MD_PARAMS pAdvMdParams);

INT FE_DEINT_ConfigUserCadence(VPP_OBJ* vpp_obj, int Cadence, PVPP_DEINTUSER_CAD_PARAMS pUserCadence);

INT FE_DEINT_SetExclCtrlParams(VPP_OBJ* vpp_obj, PVPP_DEINTEXCL_CTRL_PARAMS pExclParams);

INT FE_DEINT_SetFilmCadenceCtrl(VPP_OBJ* vpp_obj,UINT16 FilmCadence);

// Client setting API's
INT FE_DEINT_SetVpDeintClientClk(VPP_OBJ* vpp_obj, PVPP_VP_CLIENT_CLK pVpClientClk);
INT FE_DEINT_SetVpDeintClientSize(VPP_OBJ* vpp_obj,PVPP_VP_CLIENT_SIZE pVpClientSize, int training);
INT FE_DEINT_SetVpTG(VPP_OBJ *vpp_obj, PVPP_VP_CLIENT_RES pVpClientRes, int progressive);
INT FE_DEINT_SetVpDMX(VPP_OBJ *vpp_obj, PVPP_VP_CLIENT_RES pVpClientRes, int progressive);
INT FE_DEINT_SetField(VPP_OBJ *vpp_obj, UINT8 is_top);
INT FE_DEINT_VpTG_Enable(VPP_OBJ *vpp_obj, UINT8 enable);
INT FE_DEINT_VpClient_Enable(VPP_OBJ *vpp_obj, VPP_VP_CLIENT_TYPE client_id, UINT8 Enable);
INT FE_DEINT_VpClient_Clear(VPP_OBJ *vpp_obj, VPP_VP_CLIENT_TYPE client_id);
INT FE_DEINT_SetForced2DCtrl(VPP_OBJ* vpp_obj,PVPP_DEINT_FORCED2D_CTRL pForced2DCtrl);
INT FE_DEINT_SetFilmModeEntry(VPP_OBJ* vpp_obj,PVPP_DEINT_FILM_MODE_ENTRY_PARAM pFilmModeEntry);
INT FE_DEINT_SetOSDCtrl(VPP_OBJ* vpp_obj,UINT8 OSDCtrl);
INT FE_DEINT_ChangeMode(VPP_OBJ* vpp_obj, UINT32 uiMode, UINT32 uiScanType);

#endif /*__VPP_DEINT_H__*/



