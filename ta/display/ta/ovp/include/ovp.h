/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef ovp_h
#define ovp_h (){}

#include "ctypes.h"

#pragma pack(1)
#ifdef __cplusplus
extern "C" {
#endif

#ifndef h_DeIntParam
#define h_DeIntParam (){}

typedef struct SIE_DeIntParam {
	UNSG32 u_glamd					:  2;
	UNSG32 u_gosd					:  2;
	UNSG32 u_gmdv					:  2;
	UNSG32 u_grpd					:  2;
	UNSG32 u_ChromaUpSampleError			:  1;
	UNSG32 u_rhmv					:  4;
	UNSG32 u_ghmv					:  1;
	UNSG32 u_fmd_lcnst				:  2;
	UNSG32 u_fmd_still				:  2;
	UNSG32 u_fmd_ph					:  2;
	UNSG32 u_fmd_lock				:  2;
	UNSG32 u_Reserved3				:  2;
	UNSG32 u_NLE					:  8;
	UNSG32 u_YMdvThr_0i				:  8;
	UNSG32 u_YMdvThr_1i				:  8;
	UNSG32 u_YMdvThr_2i				:  8;
	UNSG32 u_YMdvThr_3i				:  8;
	UNSG32 u_UVMdvThr_0i				:  8;
	UNSG32 u_UVMdvThr_1i				:  8;
	UNSG32 u_UVMdvThr_2i				:  8;
	UNSG32 u_UVMdvThr_3i				:  8;
	UNSG32 u_SdiGain_0i				:  8;
	UNSG32 u_SdiGain_1i				:  8;
	UNSG32 u_SdiGain_2i				:  8;
	UNSG32 u_SdiGain_3i				:  8;
	UNSG32 u_SdiGain_4i				:  8;
	UNSG32 u_SdiGain_5i				:  8;
	UNSG32 u_SdiGain_6i				:  8;
	UNSG32 u_SdiOff_0i				:  8;
	UNSG32 u_SdiOff_1i				:  8;
	UNSG32 u_SdiOff_2i				:  8;
	UNSG32 u_SdiOff_3i				:  8;
	UNSG32 u_SdiOff_4i				:  8;
	UNSG32 u_SdiOff_5i				:  8;
	UNSG32 u_SdiOff_6i				:  8;
	UNSG32 u_AngHsgrmThr_0i				:  5;
	UNSG32 u_AngHsgrmThr_1i				:  5;
	UNSG32 u_Reserved4				:  6;
	UNSG32 u_IntValueThr_0i				:  8;
	UNSG32 u_IntValueThr_1i				:  8;
	UNSG32 u_IntValueThr_2i				:  8;
	UNSG32 u_Reserved5				:  8;
	UNSG32 u_AFDF3VPosDelay_0i			: 16;
	UNSG32 u_AFDF3VPosDelay_1i			: 16;
	UNSG32 u_AFDF3VPosDelay_2i			: 16;
	UNSG32 u_AFDF3VPosDelay_3i			: 16;
	UNSG32 u_AFDF2VPosDelay_0i			: 16;
	UNSG32 u_AFDF2VPosDelay_1i			: 16;
	UNSG32 u_AFDF2VPosDelay_2i			: 16;
	UNSG32 u_AFDF2VPosDelay_3i			: 16;
	UNSG32 u_AFDF1VPosDelay_0i			: 16;
	UNSG32 u_AFDF1VPosDelay_1i			: 16;
	UNSG32 u_AFDF1VPosDelay_2i			: 16;
	UNSG32 u_AFDF1VPosDelay_3i			: 16;
} SIE_DeIntParam;

SIGN32 DeIntParam_drvrd(SIE_DeIntParam *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 DeIntParam_drvwr(SIE_DeIntParam *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void DeIntParam_reset(SIE_DeIntParam *p);
SIGN32 DeIntParam_cmp  (SIE_DeIntParam *p, SIE_DeIntParam *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define DeIntParam_check(p,pie,pfx,hLOG) DeIntParam_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define DeIntParam_print(p,    pfx,hLOG) DeIntParam_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif

#ifndef h_DeIntStat
#define h_DeIntStat (){}
typedef struct SIE_DeIntStat {
	UNSG32 u_gmdv_acc				: 16;
	UNSG32 u_grpd_acc				: 16;
	UNSG32 u_ghf_acc				: 16;
	UNSG32 u_glamd_acc				: 16;
	UNSG32 u_gosd_acc				: 32;
} SIE_DeIntStat;

SIGN32 DeIntStat_drvrd(SIE_DeIntStat *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 DeIntStat_drvwr(SIE_DeIntStat *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void DeIntStat_reset(SIE_DeIntStat *p);
SIGN32 DeIntStat_cmp  (SIE_DeIntStat *p, SIE_DeIntStat *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define DeIntStat_check(p,pie,pfx,hLOG) DeIntStat_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define DeIntStat_print(p,    pfx,hLOG) DeIntStat_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif

#ifndef h_FmdStat
#define h_FmdStat (){}
typedef struct SIE_FmdStat {
	UNSG32 u_fmd_lgmv_acc				: 32;
	UNSG32 u_fmd_f0f1_macc_0			: 32;
	UNSG32 u_fmd_f0f1_macc_1			: 32;
	UNSG32 u_fmd_f0f1_macc_2			: 32;
	UNSG32 u_fmd_f0f1_macc_3			: 32;
	UNSG32 u_fmd_f0f1_macc_4			: 32;
	UNSG32 u_fmd_f0f1_macc_5			: 32;
	UNSG32 u_fmd_f0f1_macc_6			: 32;
	UNSG32 u_fmd_f0f1_macc_7			: 32;
	UNSG32 u_fmd_f0f1_sacc_0			: 32;
	UNSG32 u_fmd_f0f1_sacc_1			: 32;
	UNSG32 u_fmd_f0f1_sacc_2			: 32;
	UNSG32 u_fmd_f0f1_sacc_3			: 32;
	UNSG32 u_fmd_f0f1_sacc_4			: 32;
	UNSG32 u_fmd_f0f1_sacc_5			: 32;
	UNSG32 u_fmd_f0f1_sacc_6			: 32;
	UNSG32 u_fmd_f0f1_sacc_7			: 32;
	UNSG32 u_fmd_f2f1_macc_0			: 32;
	UNSG32 u_fmd_f2f1_macc_1			: 32;
	UNSG32 u_fmd_f2f1_macc_2			: 32;
	UNSG32 u_fmd_f2f1_macc_3			: 32;
	UNSG32 u_fmd_f2f1_macc_4			: 32;
	UNSG32 u_fmd_f2f1_macc_5			: 32;
	UNSG32 u_fmd_f2f1_macc_6			: 32;
	UNSG32 u_fmd_f2f1_macc_7			: 32;
	UNSG32 u_fmd_f2f1_sacc_0			: 32;
	UNSG32 u_fmd_f2f1_sacc_1			: 32;
	UNSG32 u_fmd_f2f1_sacc_2			: 32;
	UNSG32 u_fmd_f2f1_sacc_3			: 32;
	UNSG32 u_fmd_f2f1_sacc_4			: 32;
	UNSG32 u_fmd_f2f1_sacc_5			: 32;
	UNSG32 u_fmd_f2f1_sacc_6			: 32;
	UNSG32 u_fmd_f2f1_sacc_7			: 32;
	UNSG32 u_fmd_AFDF0VPosDelay_0			: 16;
	UNSG32 u_fmd_AFDF0VPosDelay_1			: 16;
	UNSG32 u_fmd_AFDF0VPosDelay_2			: 16;
	UNSG32 u_fmd_AFDF0VPosDelay_3			: 16;
} SIE_FmdStat;

SIGN32 FmdStat_drvrd(SIE_FmdStat *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 FmdStat_drvwr(SIE_FmdStat *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void FmdStat_reset(SIE_FmdStat *p);
SIGN32 FmdStat_cmp  (SIE_FmdStat *p, SIE_FmdStat *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define FmdStat_check(p,pie,pfx,hLOG) FmdStat_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define FmdStat_print(p,    pfx,hLOG) FmdStat_cmp(p,0,	pfx,(void*)(hLOG),0,0)
#endif

#ifndef h_OvpCmd
#define h_OvpCmd (){}
#define     RA_OvpCmd_Control				   0x0000
#define     RA_OvpCmd_VideoFormat			   0x0004
#define     RA_OvpCmd_InputBuffer			   0x0008
#define     RA_OvpCmd_OutputBuffer			   0x0030
#define     RA_OvpCmd_DeIntParam			   0x0048
#define     RA_OvpCmd_ScaleParam			   0x0080
#define     RA_OvpCmd_FmdStat				   0x0100
#define     RA_OvpCmd_DeIntStat				   0x0190
#define     RA_OvpCmd_Return				   0x01F0

typedef struct SIE_OvpCmd {
	#define     w32OvpCmd_Control				{\
		UNSG32 uControl_IOC				:  1;\
		UNSG32 uControl_OutputWholeFrame		:  1;\
		UNSG32 uControl_OutputOneField			:  1;\
		UNSG32 uControl_Reserved0			: 29;\
	}
	union {
		UNSG32 u32OvpCmd_Control;
		struct w32OvpCmd_Control;
	};

	#define     w32OvpCmd_VideoFormat			{\
		UNSG32 uVideoFormat_InputUVSample		:  2;\
		UNSG32 uVideoFormat_InputPlanes			:  2;\
		UNSG32 uVideoFormat_InputBitDepth		:  2;\
		UNSG32 uVideoFormat_InputTile			:  2;\
		UNSG32 uVideoFormat_InputInterlaced		:  1;\
		UNSG32 uVideoFormat_Reserved1			:  7;\
		UNSG32 uVideoFormat_OutputUVSample		:  2;\
		UNSG32 uVideoFormat_OutputPlanes		:  2;\
		UNSG32 uVideoFormat_OutputBitDepth		:  2;\
		UNSG32 uVideoFormat_Reserved2			: 10;\
	}
	union {
		UNSG32 u32OvpCmd_VideoFormat;
		struct w32OvpCmd_VideoFormat;
	};

	#define     w32OvpCmd_InputBuffer			{\
		UNSG32 uInputBuffer_InputHSize			: 16;\
		UNSG32 uInputBuffer_InputVSize			: 16;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer;
		struct w32OvpCmd_InputBuffer;
	};

	#define     w32OvpCmd_InputBuffer1			{\
		UNSG32 uInputBuffer_InputYStride		: 16;\
		UNSG32 uInputBuffer_InputUVStride		: 16;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer1;
		struct w32OvpCmd_InputBuffer1;
	};

	#define     w32OvpCmd_InputBuffer2			{\
		UNSG32 uInputBuffer_InputBuf0YAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer2;
		struct w32OvpCmd_InputBuffer2;
	};

	#define     w32OvpCmd_InputBuffer3			{\
		UNSG32 uInputBuffer_InputBuf0UVAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer3;
		struct w32OvpCmd_InputBuffer3;
	};

	#define     w32OvpCmd_InputBuffer4			{\
		UNSG32 uInputBuffer_InputBuf1YAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer4;
		struct w32OvpCmd_InputBuffer4;
	};

	#define     w32OvpCmd_InputBuffer5			{\
		UNSG32 uInputBuffer_InputBuf1UVAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer5;
		struct w32OvpCmd_InputBuffer5;
	};

	#define     w32OvpCmd_InputBuffer6			{\
		UNSG32 uInputBuffer_InputBuf2YAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer6;
		struct w32OvpCmd_InputBuffer6;
	};

	#define     w32OvpCmd_InputBuffer7			{\
		UNSG32 uInputBuffer_InputBuf2UVAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer7;
		struct w32OvpCmd_InputBuffer7;
	};

	#define     w32OvpCmd_InputBuffer8			{\
		UNSG32 uInputBuffer_InputBuf3YAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer8;
		struct w32OvpCmd_InputBuffer8;
	};

	#define     w32OvpCmd_InputBuffer9			{\
		UNSG32 uInputBuffer_InputBuf3UVAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_InputBuffer9;
		struct w32OvpCmd_InputBuffer9;
	};

	#define     w32OvpCmd_OutputBuffer			{\
		UNSG32 uOutputBuffer_OutputHSize		: 16;\
		UNSG32 uOutputBuffer_OutputVSize		: 16;\
	}
	union {
		UNSG32 u32OvpCmd_OutputBuffer;
		struct w32OvpCmd_OutputBuffer;
	};

	#define     w32OvpCmd_OutputBuffer1			{\
		UNSG32 uOutputBuffer_OutputYStride		: 16;\
		UNSG32 uOutputBuffer_OutputUVStride		: 16;\
	}
	union {
		UNSG32 u32OvpCmd_OutputBuffer1;
		struct w32OvpCmd_OutputBuffer1;
	};

	#define     w32OvpCmd_OutputBuffer2			{\
		UNSG32 uOutputBuffer_OutputBuf0YAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_OutputBuffer2;
		struct w32OvpCmd_OutputBuffer2;
	};

	#define     w32OvpCmd_OutputBuffer3			{\
		UNSG32 uOutputBuffer_OutputBuf0UVAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_OutputBuffer3;
		struct w32OvpCmd_OutputBuffer3;
	};

	#define     w32OvpCmd_OutputBuffer4			{\
		UNSG32 uOutputBuffer_OutputBuf1YAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_OutputBuffer4;
		struct w32OvpCmd_OutputBuffer4;
	};

	#define     w32OvpCmd_OutputBuffer5			{\
		UNSG32 uOutputBuffer_OutputBuf1UVAddr		: 32;\
	}
	union {
		UNSG32 u32OvpCmd_OutputBuffer5;
		struct w32OvpCmd_OutputBuffer5;
	};

	SIE_DeIntParam					 ie_DeIntParam;

	#define     w32OvpCmd_ScaleParam			{\
		UNSG32 uScaleParam_HScaleRatio			:  8;\
		UNSG32 uScaleParam_HInitialPhase		:  8;\
		UNSG32 uScaleParam_VScaleRatio			:  8;\
		UNSG32 uScaleParam_VInitialPhase		:  8;\
	}
	union {
		UNSG32 u32OvpCmd_ScaleParam;
		struct w32OvpCmd_ScaleParam;
	};

	UNSG8 RSVDx84						[124];
	SIE_FmdStat						ie_FmdStat;
	UNSG8 RSVDx18C						[4];
	SIE_DeIntStat						ie_DeIntStat;
	UNSG8 RSVDx19C						[84];

	#define     w32OvpCmd_Return				{\
		UNSG32 uReturn_Done				:  1;\
		UNSG32 uReturn_ErrorCode			:  7;\
		UNSG32 RSVDx1F0_b8				: 24;\
	}
	union {
		UNSG32 u32OvpCmd_Return;
		struct w32OvpCmd_Return;
	};
} SIE_OvpCmd;

typedef union  T32OvpCmd_Control
{
	UNSG32 u32;
	struct w32OvpCmd_Control;
} T32OvpCmd_Control;

typedef union  T32OvpCmd_VideoFormat
{
	UNSG32 u32;
	struct w32OvpCmd_VideoFormat;
} T32OvpCmd_VideoFormat;

typedef union  T32OvpCmd_InputBuffer
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer;
} T32OvpCmd_InputBuffer;

typedef union  T32OvpCmd_InputBuffer1
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer1;
} T32OvpCmd_InputBuffer1;

typedef union  T32OvpCmd_InputBuffer2
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer2;
} T32OvpCmd_InputBuffer2;

typedef union  T32OvpCmd_InputBuffer3
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer3;
} T32OvpCmd_InputBuffer3;

typedef union  T32OvpCmd_InputBuffer4
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer4;
} T32OvpCmd_InputBuffer4;

typedef union  T32OvpCmd_InputBuffer5
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer5;
} T32OvpCmd_InputBuffer5;

typedef union  T32OvpCmd_InputBuffer6
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer6;
} T32OvpCmd_InputBuffer6;

typedef union  T32OvpCmd_InputBuffer7
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer7;
} T32OvpCmd_InputBuffer7;

typedef union  T32OvpCmd_InputBuffer8
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer8;
} T32OvpCmd_InputBuffer8;

typedef union  T32OvpCmd_InputBuffer9
{
	UNSG32 u32;
	struct w32OvpCmd_InputBuffer9;
} T32OvpCmd_InputBuffer9;

typedef union  T32OvpCmd_OutputBuffer
{
	UNSG32 u32;
	struct w32OvpCmd_OutputBuffer;
} T32OvpCmd_OutputBuffer;

typedef union  T32OvpCmd_OutputBuffer1
{
	UNSG32 u32;
	struct w32OvpCmd_OutputBuffer1;
} T32OvpCmd_OutputBuffer1;

typedef union  T32OvpCmd_OutputBuffer2
{
	UNSG32 u32;
	struct w32OvpCmd_OutputBuffer2;
} T32OvpCmd_OutputBuffer2;

typedef union  T32OvpCmd_OutputBuffer3
{
	UNSG32 u32;
	struct w32OvpCmd_OutputBuffer3;
} T32OvpCmd_OutputBuffer3;

typedef union  T32OvpCmd_OutputBuffer4
{
	UNSG32 u32;
	struct w32OvpCmd_OutputBuffer4;
} T32OvpCmd_OutputBuffer4;

typedef union  T32OvpCmd_OutputBuffer5
{
	UNSG32 u32;
	struct w32OvpCmd_OutputBuffer5;
} T32OvpCmd_OutputBuffer5;

typedef union  T32OvpCmd_ScaleParam
{
	UNSG32 u32;
	struct w32OvpCmd_ScaleParam;
} T32OvpCmd_ScaleParam;

typedef union  T32OvpCmd_Return
{
	UNSG32 u32;
	struct w32OvpCmd_Return;
} T32OvpCmd_Return;

typedef union  TOvpCmd_Control
{
	UNSG32 u32[1];
	struct {
		struct w32OvpCmd_Control;
	};
} TOvpCmd_Control;

typedef union  TOvpCmd_VideoFormat
{
	UNSG32 u32[1];
	struct {
		struct w32OvpCmd_VideoFormat;
	};
} TOvpCmd_VideoFormat;

typedef union  TOvpCmd_InputBuffer
{
	UNSG32 u32[10];
	struct {
		struct w32OvpCmd_InputBuffer;
		struct w32OvpCmd_InputBuffer1;
		struct w32OvpCmd_InputBuffer2;
		struct w32OvpCmd_InputBuffer3;
		struct w32OvpCmd_InputBuffer4;
		struct w32OvpCmd_InputBuffer5;
		struct w32OvpCmd_InputBuffer6;
		struct w32OvpCmd_InputBuffer7;
		struct w32OvpCmd_InputBuffer8;
		struct w32OvpCmd_InputBuffer9;
	};
} TOvpCmd_InputBuffer;

typedef union  TOvpCmd_OutputBuffer
{
	UNSG32 u32[6];
	struct {
		struct w32OvpCmd_OutputBuffer;
		struct w32OvpCmd_OutputBuffer1;
		struct w32OvpCmd_OutputBuffer2;
		struct w32OvpCmd_OutputBuffer3;
		struct w32OvpCmd_OutputBuffer4;
		struct w32OvpCmd_OutputBuffer5;
	};
} TOvpCmd_OutputBuffer;

typedef union  TOvpCmd_ScaleParam
{
	UNSG32 u32[1];
	struct {
		struct w32OvpCmd_ScaleParam;
	};
} TOvpCmd_ScaleParam;

typedef union  TOvpCmd_Return
{
	UNSG32 u32[1];
	struct {
		struct w32OvpCmd_Return;
	};
} TOvpCmd_Return;

SIGN32 OvpCmd_drvrd(SIE_OvpCmd *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 OvpCmd_drvwr(SIE_OvpCmd *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void OvpCmd_reset(SIE_OvpCmd *p);
SIGN32 OvpCmd_cmp  (SIE_OvpCmd *p, SIE_OvpCmd *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define OvpCmd_check(p,pie,pfx,hLOG) OvpCmd_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define OvpCmd_print(p,    pfx,hLOG) OvpCmd_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif

#ifndef h_MadiReg
#define h_MadiReg (){}
#define     RA_MadiReg_Edge0				   0x0000
#define     RA_MadiReg_Edge1				   0x0004
#define     RA_MadiReg_Cnst				   0x0008
#define     RA_MadiReg_MdvCnst				   0x000C
#define     RA_MadiReg_LamdNoise			   0x0010
#define     RA_MadiReg_LasdNoise			   0x0014
#define     RA_MadiReg_LAMD				   0x0018
#define     RA_MadiReg_LASD				   0x001C
#define     RA_MadiReg_SOSD0				   0x0020
#define     RA_MadiReg_SOSD1				   0x0024
#define     RA_MadiReg_LAMDGain				   0x0028
#define     RA_MadiReg_LASDGain				   0x002C
#define     RA_MadiReg_Hf				   0x0030
#define     RA_MadiReg_Rpd				   0x0034
#define     RA_MadiReg_SdiSad				   0x0038
#define     RA_MadiReg_DiagByps				   0x003C
#define     RA_MadiReg_SdiNoise				   0x0040
#define     RA_MadiReg_Sdi0				   0x0044
#define     RA_MadiReg_Sdi1				   0x0048
#define     RA_MadiReg_Sdi2				   0x004C
#define     RA_MadiReg_ShiftBit				   0x0050
#define     RA_MadiReg_Sdi3				   0x0054
#define     RA_MadiReg_Sdi4				   0x0058
#define     RA_MadiReg_Sdi5				   0x005C
#define     RA_MadiReg_Sdi6				   0x0060
#define     RA_MadiReg_Angle				   0x0064
#define     RA_MadiReg_MotDet				   0x0068
#define     RA_MadiReg_HVLine				   0x006C
#define     RA_MadiReg_MdvYiir				   0x0070
#define     RA_MadiReg_C420A				   0x0074
#define     RA_MadiReg_C420B				   0x0078
#define     RA_MadiReg_MdvFilterSel			   0x007C
#define     RA_MadiReg_LcnstGain			   0x0080
#define     RA_MadiReg_YUVBlendGain			   0x0084
#define     RA_MadiReg_YUVMdvBlendGain			   0x0088
#define     RA_MadiReg_YMdvGain				   0x008C
#define     RA_MadiReg_YMdvGain1			   0x0090
#define     RA_MadiReg_YMdvOffset			   0x0094
#define     RA_MadiReg_UVMdvGain			   0x0098
#define     RA_MadiReg_Impulse				   0x009C
#define     RA_MadiReg_Nr				   0x00A0
#define     RA_MadiReg_MdvLcnst0			   0x00A4
#define     RA_MadiReg_MdvLcnst1			   0x00A8
#define     RA_MadiReg_MdvLcnst2			   0x00AC
#define     RA_MadiReg_MdvLcnst3			   0x00B0
#define     RA_MadiReg_VTFCoef0				   0x00B4
#define     RA_MadiReg_VTFCoef1				   0x00B8
#define     RA_MadiReg_VTFCoef2				   0x00BC
#define     RA_MadiReg_C420VTFCoef0			   0x00C0
#define     RA_MadiReg_C420VTFCoef1			   0x00C4
#define     RA_MadiReg_C420VTFCoef2			   0x00C8
#define     RA_MadiReg_C420Mode				   0x00CC
#define     RA_MadiReg_IntOutOnly			   0x00D0
#define     RA_MadiReg_ChromaMdvUseOnly			   0x00D4
#define     RA_MadiReg_MdvMin				   0x00D8
#define     RA_MadiReg_GAccShiftBits			   0x00DC
#define     RA_MadiReg_YUV8BitIn			   0x00E0

typedef struct SIE_MadiReg {
	#define     w32MadiReg_Edge0				{\
		UNSG32 uEdge0_EdgeGain_0			:  6;\
		UNSG32 uEdge0_EdgeGain_1			:  6;\
		UNSG32 RSVDx0_b12				: 20;\
	}
	union {
		UNSG32 u32MadiReg_Edge0;
		struct w32MadiReg_Edge0;
	};

	#define     w32MadiReg_Edge1				{\
		UNSG32 uEdge1_EdgeThr_0				:  8;\
		UNSG32 uEdge1_EdgeThr_1				:  8;\
		UNSG32 uEdge1_EdgeThr_2				:  8;\
		UNSG32 uEdge1_EdgeThr_3				:  8;\
	}
	union {
	UNSG32 u32MadiReg_Edge1;
		struct w32MadiReg_Edge1;
	};

	#define     w32MadiReg_Cnst				{\
		UNSG32 uCnst_CnstThr_0				:  8;\
		UNSG32 uCnst_CnstThr_1				:  8;\
		UNSG32 uCnst_CnstThr_2				:  8;\
		UNSG32 RSVDx8_b24				:  8;\
	}
	union {
		UNSG32 u32MadiReg_Cnst;
		struct w32MadiReg_Cnst;
	};

	#define     w32MadiReg_MdvCnst				{\
		UNSG32 uMdvCnst_MdvCnstThr			:  6;\
		UNSG32 RSVDxC_b6				: 26;\
	}
	union {
		UNSG32 u32MadiReg_MdvCnst;
		struct w32MadiReg_MdvCnst;
	};

	#define     w32MadiReg_LamdNoise			{\
		UNSG32 uLamdNoise_LamdNoiseThr_0		:  5;\
		UNSG32 uLamdNoise_LamdNoiseThr_1		:  5;\
		UNSG32 RSVDx10_b10				: 22;\
	}
	union {
		UNSG32 u32MadiReg_LamdNoise;
		struct w32MadiReg_LamdNoise;
	};

	#define     w32MadiReg_LasdNoise			{\
		UNSG32 uLasdNoise_LasdNoiseThr_0		:  5;\
		UNSG32 uLasdNoise_LasdNoiseThr_1		:  5;\
		UNSG32 RSVDx14_b10				: 22;\
	}
	union {
		UNSG32 u32MadiReg_LasdNoise;
		struct w32MadiReg_LasdNoise;
	};

	#define     w32MadiReg_LAMD				{\
		UNSG32 uLAMD_LAMDThr_0				:  5;\
		UNSG32 uLAMD_LAMDThr_1				:  5;\
		UNSG32 uLAMD_LAMDThr_2				:  5;\
		UNSG32 uLAMD_LAMDEnhThr_0			:  2;\
		UNSG32 uLAMD_LAMDEnhThr_1			:  2;\
		UNSG32 RSVDx18_b19				: 13;\
	}
	union {
		UNSG32 u32MadiReg_LAMD;
		struct w32MadiReg_LAMD;
	};

	#define     w32MadiReg_LASD				{\
		UNSG32 uLASD_LASDThr_0				:  3;\
		UNSG32 RSVDx1C_b3				: 29;\
	}
	union {
		UNSG32 u32MadiReg_LASD;
		struct w32MadiReg_LASD;
	};

	#define     w32MadiReg_SOSD0				{\
		UNSG32 uSOSD0_SOSDThr_0				:  8;\
		UNSG32 uSOSD0_SOSDThr_1				:  8;\
		UNSG32 uSOSD0_SOSDThr_2				:  8;\
		UNSG32 uSOSD0_SOSDThr_3				:  8;\
	}
	union {
		UNSG32 u32MadiReg_SOSD0;
		struct w32MadiReg_SOSD0;
	};

	#define     w32MadiReg_SOSD1				{\
		UNSG32 uSOSD1_SOSDThr_4				:  8;\
		UNSG32 uSOSD1_SOSDThr_5				:  8;\
		UNSG32 RSVDx24_b16				: 16;\
	}
	union {
		UNSG32 u32MadiReg_SOSD1;
		struct w32MadiReg_SOSD1;
	};

	#define     w32MadiReg_LAMDGain				{\
		UNSG32 uLAMDGain_LAMDGain_0			:  8;\
		UNSG32 uLAMDGain_LAMDGain_1			:  8;\
		UNSG32 uLAMDGain_LAMDGain_2			:  8;\
		UNSG32 uLAMDGain_LAMDGain_3			:  8;\
	}
	union {
		UNSG32 u32MadiReg_LAMDGain;
		struct w32MadiReg_LAMDGain;
	};

	#define     w32MadiReg_LASDGain				{\
		UNSG32 uLASDGain_LASDGain_0			:  8;\
		UNSG32 uLASDGain_LASDGain_1			:  8;\
		UNSG32 RSVDx2C_b16				: 16;\
	}
	union {
		UNSG32 u32MadiReg_LASDGain;
		struct w32MadiReg_LASDGain;
	};

	#define     w32MadiReg_Hf				{\
		UNSG32 uHf_HfThr_0				:  6;\
		UNSG32 uHf_HfThr_1				:  6;\
		UNSG32 RSVDx30_b12				: 20;\
	}
	union {
		UNSG32 u32MadiReg_Hf;
		struct w32MadiReg_Hf;
	};

	#define     w32MadiReg_Rpd				{\
		UNSG32 uRpd_RpdThr_0				:  5;\
		UNSG32 uRpd_RpdThr_1				:  5;\
		UNSG32 RSVDx34_b10				: 22;\
	}
	union {
		UNSG32 u32MadiReg_Rpd;
		struct w32MadiReg_Rpd;
	};

	#define     w32MadiReg_SdiSad				{\
		UNSG32 uSdiSad_SdiSadThr_0			:  6;\
		UNSG32 uSdiSad_SdiSadThr_1			:  6;\
		UNSG32 uSdiSad_SdiSadThr_2			:  6;\
		UNSG32 uSdiSad_SdiSadThr_3			:  8;\
		UNSG32 RSVDx38_b26				:  6;\
	}
	union {
		UNSG32 u32MadiReg_SdiSad;
		struct w32MadiReg_SdiSad;
	};

	#define     w32MadiReg_DiagByps				{\
		UNSG32 uDiagByps_YDiagByps			:  1;\
		UNSG32 uDiagByps_UVDiagByps			:  1;\
		UNSG32 RSVDx3C_b2				: 30;\
	}
	union {
		UNSG32 u32MadiReg_DiagByps;
		struct w32MadiReg_DiagByps;
	};

	#define     w32MadiReg_SdiNoise				{\
		UNSG32 uSdiNoise_SdiNoiseThr			:  6;\
		UNSG32 RSVDx40_b6				: 26;\
	}
	union {
		UNSG32 u32MadiReg_SdiNoise;
		struct w32MadiReg_SdiNoise;
	};

	#define     w32MadiReg_Sdi0				{\
		UNSG32 uSdi0_SdiAbsAngFilterThr			:  3;\
		UNSG32 uSdi0_SdiIntClamp			:  4;\
		UNSG32 uSdi0_SdiAbsAngConfThr			:  2;\
		UNSG32 uSdi0_SdiAbsAngThr			:  4;\
		UNSG32 uSdi0_SdiAngStretchThr			:  5;\
		UNSG32 uSdi0_SdiHighAngConfValue_0		:  3;\
		UNSG32 uSdi0_SdiHighAngConfValue_1		:  3;\
		UNSG32 RSVDx44_b24				:  8;\
	}
	union {
		UNSG32 u32MadiReg_Sdi0;
		struct w32MadiReg_Sdi0;
	};

	#define     w32MadiReg_Sdi1				{\
		UNSG32 uSdi1_SdiAngConfBlendGain_0		:  5;\
		UNSG32 uSdi1_SdiAngConfBlendGain_1		:  5;\
		UNSG32 uSdi1_SdiAngConfBlendGain_2		:  5;\
		UNSG32 uSdi1_SdiAngConfBlendGain_3		:  5;\
		UNSG32 RSVDx48_b20				: 12;\
	}
	union {
		UNSG32 u32MadiReg_Sdi1;
		struct w32MadiReg_Sdi1;
	};

	#define     w32MadiReg_Sdi2				{\
		UNSG32 uSdi2_SdiAngConfBlendGain_4		:  5;\
		UNSG32 uSdi2_SdiAngConfBlendGain_5		:  5;\
		UNSG32 uSdi2_SdiAngConfBlendGain_6		:  5;\
		UNSG32 RSVDx4C_b15				: 17;\
	}
	union {
		UNSG32 u32MadiReg_Sdi2;
		struct w32MadiReg_Sdi2;
	};

	#define     w32MadiReg_ShiftBit				{\
		UNSG32 uShiftBit_BlamdShiftBit			:  3;\
		UNSG32 uShiftBit_BlasdShiftBit			:  3;\
		UNSG32 uShiftBit_BsosdShiftBit			:  3;\
		UNSG32 uShiftBit_BlmhfShiftBit			:  3;\
		UNSG32 RSVDx50_b12				: 20;\
	}
	union {
		UNSG32 u32MadiReg_ShiftBit;
		struct w32MadiReg_ShiftBit;
	};

	#define     w32MadiReg_Sdi3				{\
		UNSG32 uSdi3_SdiScaleFactor_0			:  4;\
		UNSG32 uSdi3_SdiScaleFactor_1			:  4;\
		UNSG32 uSdi3_SdiScaleFactor_2			:  4;\
		UNSG32 uSdi3_SdiScaleFactor_3			:  4;\
		UNSG32 RSVDx54_b16				: 16;\
	}
	union {
		UNSG32 u32MadiReg_Sdi3;
		struct w32MadiReg_Sdi3;
	};

	#define     w32MadiReg_Sdi4				{\
		UNSG32 uSdi4_SdiIntDifThr_0			: 10;\
		UNSG32 uSdi4_SdiIntDifThr_1			: 10;\
		UNSG32 uSdi4_SdiIntDifThr_2			: 10;\
		UNSG32 RSVDx58_b30				:  2;\
	}
	union {
		UNSG32 u32MadiReg_Sdi4;
		struct w32MadiReg_Sdi4;
	};

	#define     w32MadiReg_Sdi5				{\
		UNSG32 uSdi5_SdiIntDifThr_3			: 10;\
		UNSG32 uSdi5_SdiIntDifThr_4			: 10;\
		UNSG32 uSdi5_SdiIntDifThr_5			: 10;\
		UNSG32 RSVDx5C_b30				:  2;\
	}
	union {
		UNSG32 u32MadiReg_Sdi5;
		struct w32MadiReg_Sdi5;
	};

	#define     w32MadiReg_Sdi6				{\
		UNSG32 uSdi6_SdiMdvThr_0			:  6;\
		UNSG32 uSdi6_SdiMdvThr_1			:  6;\
		UNSG32 uSdi6_SdiLowSadThr			:  6;\
		UNSG32 RSVDx60_b18				: 14;\
	}
	union {
		UNSG32 u32MadiReg_Sdi6;
		struct w32MadiReg_Sdi6;
	};

	#define     w32MadiReg_Angle				{\
		UNSG32 uAngle_AngConfEn				:  1;\
		UNSG32 uAngle_AngConfUser			:  3;\
		UNSG32 uAngle_AngCnstEn				:  1;\
		UNSG32 uAngle_AngCnstUser			:  2;\
		UNSG32 uAngle_AngEdgeEn				:  1;\
		UNSG32 uAngle_AngEdgeUser			:  3;\
		UNSG32 uAngle_AngleEn				:  1;\
		UNSG32 uAngle_AngleUser				:  4;\
		UNSG32 RSVDx64_b16				: 16;\
	}
	union {
		UNSG32 u32MadiReg_Angle;
		struct w32MadiReg_Angle;
	};

	#define     w32MadiReg_MotDet				{\
		UNSG32 uMotDet_LAMDEn				:  1;\
		UNSG32 uMotDet_LAMDUser				:  2;\
		UNSG32 uMotDet_LASDEn				:  1;\
		UNSG32 uMotDet_LASDUser				:  1;\
		UNSG32 uMotDet_LASDSel				:  1;\
		UNSG32 uMotDet_SOSDEn				:  1;\
		UNSG32 uMotDet_SOSDUser				:  1;\
		UNSG32 uMotDet_SOSDSel				:  1;\
		UNSG32 uMotDet_MDVEn				:  1;\
		UNSG32 uMotDet_MDVUser_0			:  3;\
		UNSG32 uMotDet_MDVUser_1			:  3;\
		UNSG32 uMotDet_HVLineEn				:  1;\
		UNSG32 uMotDet_HVLineUser			:  2;\
		UNSG32 RSVDx68_b19				: 13;\
	}
	union {
		UNSG32 u32MadiReg_MotDet;
		struct w32MadiReg_MotDet;
	};

	#define     w32MadiReg_HVLine				{\
		UNSG32 uHVLine_HVLineGain_0			:  8;\
		UNSG32 uHVLine_HVLineGain_1			:  8;\
		UNSG32 uHVLine_HVLineThr_0			:  8;\
		UNSG32 uHVLine_HVLineThr_1			:  8;\
	}
	union {
		UNSG32 u32MadiReg_HVLine;
		struct w32MadiReg_HVLine;
	};

	#define     w32MadiReg_MdvYiir				{\
		UNSG32 uMdvYiir_MdvYiirGain_0			:  5;\
		UNSG32 uMdvYiir_MdvYiirGain_1			:  5;\
		UNSG32 uMdvYiir_MdvYiirClamp			:  8;\
		UNSG32 uMdvYiir_MdvYiirEn			:  1;\
		UNSG32 RSVDx70_b19				: 13;\
	}
	union {
		UNSG32 u32MadiReg_MdvYiir;
		struct w32MadiReg_MdvYiir;
	};

	#define     w32MadiReg_C420A				{\
		UNSG32 uC420A_EThr				:  8;\
		UNSG32 uC420A_C420StNum				:  4;\
		UNSG32 uC420A_C420AccShiftBits			:  4;\
		UNSG32 RSVDx74_b16				: 16;\
	}
	union {
		UNSG32 u32MadiReg_C420A;
		struct w32MadiReg_C420A;
	};

	#define     w32MadiReg_C420B				{\
		UNSG32 uC420B_MThr_0				:  5;\
		UNSG32 uC420B_MThr_1				:  5;\
		UNSG32 uC420B_MThr_2				:  5;\
		UNSG32 uC420B_MdvThr				:  5;\
		UNSG32 RSVDx78_b20				: 12;\
	}
	union {
		UNSG32 u32MadiReg_C420B;
		struct w32MadiReg_C420B;
	};

	#define     w32MadiReg_MdvFilterSel			{\
		UNSG32 uMdvFilterSel_MdvFilterSel_0		:  1;\
		UNSG32 uMdvFilterSel_MdvFilterSel_1		:  1;\
		UNSG32 RSVDx7C_b2				: 30;\
	}
	union {
		UNSG32 u32MadiReg_MdvFilterSel;
		struct w32MadiReg_MdvFilterSel;
	};

	#define     w32MadiReg_LcnstGain			{\
		UNSG32 uLcnstGain_LcnstGain_0			:  8;\
		UNSG32 uLcnstGain_LcnstGain_1			:  8;\
		UNSG32 RSVDx80_b16				: 16;\
	}
	union {
		UNSG32 u32MadiReg_LcnstGain;
		struct w32MadiReg_LcnstGain;
	};

	#define     w32MadiReg_YUVBlendGain			{\
		UNSG32 uYUVBlendGain_YUVBlendGain_0		:  8;\
		UNSG32 uYUVBlendGain_YUVBlendGain_1		:  8;\
		UNSG32 RSVDx84_b16				: 16;\
	}
	union {
		UNSG32 u32MadiReg_YUVBlendGain;
		struct w32MadiReg_YUVBlendGain;
	};

	#define     w32MadiReg_YUVMdvBlendGain			{\
		UNSG32 uYUVMdvBlendGain_YUVMdvBlendGain_0	:  9;\
		UNSG32 uYUVMdvBlendGain_YUVMdvBlendGain_1	:  9;\
		UNSG32 RSVDx88_b18				: 14;\
	}
	union {
		UNSG32 u32MadiReg_YUVMdvBlendGain;
		struct w32MadiReg_YUVMdvBlendGain;
	};

	#define     w32MadiReg_YMdvGain				{\
		UNSG32 uYMdvGain_YMdvGain_0			:  8;\
		UNSG32 uYMdvGain_YMdvGain_1			:  8;\
		UNSG32 uYMdvGain_YMdvGain_2			:  8;\
		UNSG32 uYMdvGain_YMdvGain_3			:  8;\
	}
	union {
		UNSG32 u32MadiReg_YMdvGain;
		struct w32MadiReg_YMdvGain;
	};

	#define     w32MadiReg_YMdvGain1			{\
		UNSG32 uYMdvGain1_YMdvGain_4			:  8;\
		UNSG32 RSVDx90_b8				: 24;\
	}
	union {
		UNSG32 u32MadiReg_YMdvGain1;
		struct w32MadiReg_YMdvGain1;
	};

	#define     w32MadiReg_YMdvOffset			{\
		UNSG32 uYMdvOffset_YMdvOffset_0			:  6;\
		UNSG32 RSVDx94_b6				: 26;\
	}
	union {
		UNSG32 u32MadiReg_YMdvOffset;
		struct w32MadiReg_YMdvOffset;
	};

	#define     w32MadiReg_UVMdvGain			{\
		UNSG32 uUVMdvGain_UVMdvGain			:  8;\
		UNSG32 RSVDx98_b8				: 24;\
	}
	union {
		UNSG32 u32MadiReg_UVMdvGain;
		struct w32MadiReg_UVMdvGain;
	};

	#define     w32MadiReg_Impulse				{\
		UNSG32 uImpulse_ImpulseEn			:  1;\
		UNSG32 uImpulse_ImpulseThr			:  8;\
		UNSG32 RSVDx9C_b9				: 23;\
	}
	union {
		UNSG32 u32MadiReg_Impulse;
		struct w32MadiReg_Impulse;
	};

	#define     w32MadiReg_Nr				{\
		UNSG32 uNr_YNr					:  1;\
		UNSG32 uNr_UVNr					:  1;\
		UNSG32 RSVDxA0_b2				: 30;\
	}
	union {
		UNSG32 u32MadiReg_Nr;
		struct w32MadiReg_Nr;
	};

	#define     w32MadiReg_MdvLcnst0			{\
		UNSG32 uMdvLcnst0_MdvLcnstGain_0		:  9;\
		UNSG32 uMdvLcnst0_MdvLcnstGain_1		:  9;\
		UNSG32 RSVDxA4_b18				: 14;\
	}
	union {
		UNSG32 u32MadiReg_MdvLcnst0;
		struct w32MadiReg_MdvLcnst0;
	};

	#define     w32MadiReg_MdvLcnst1			{\
		UNSG32 uMdvLcnst1_MdvLcnstGain_2		:  9;\
		UNSG32 uMdvLcnst1_MdvLcnstGain_3		:  9;\
		UNSG32 RSVDxA8_b18				: 14;\
	}
	union {
		UNSG32 u32MadiReg_MdvLcnst1;
		struct w32MadiReg_MdvLcnst1;
	};

	#define     w32MadiReg_MdvLcnst2			{\
		UNSG32 uMdvLcnst2_MdvLcnstValue_0		:  5;\
		UNSG32 uMdvLcnst2_MdvLcnstValue_1		:  5;\
		UNSG32 uMdvLcnst2_MdvLcnstValue_2		:  5;\
		UNSG32 uMdvLcnst2_MdvLcnstValue_3		:  5;\
		UNSG32 RSVDxAC_b20				: 12;\
	}
	union {
		UNSG32 u32MadiReg_MdvLcnst2;
		struct w32MadiReg_MdvLcnst2;
	};

	#define     w32MadiReg_MdvLcnst3			{\
		UNSG32 uMdvLcnst3_MdvLcnstEnhSel_0		:  1;\
		UNSG32 uMdvLcnst3_MdvLcnstEnhSel_1		:  1;\
		UNSG32 uMdvLcnst3_MdvLcnstEnhSel_2		:  1;\
		UNSG32 RSVDxB0_b3				: 29;\
	}
	union {
		UNSG32 u32MadiReg_MdvLcnst3;
		struct w32MadiReg_MdvLcnst3;
	};

	#define     w32MadiReg_VTFCoef0				{\
		UNSG32 sVTFCoef0_VTFCoef_0			:  9;\
		UNSG32 sVTFCoef0_VTFCoef_1			:  9;\
		UNSG32 sVTFCoef0_VTFCoef_2			:  9;\
		UNSG32 RSVDxB4_b27				:  5;\
	}
	union {
		UNSG32 u32MadiReg_VTFCoef0;
		struct w32MadiReg_VTFCoef0;
	};

	#define     w32MadiReg_VTFCoef1				{\
		UNSG32 sVTFCoef1_VTFCoef_3			:  9;\
		UNSG32 sVTFCoef1_VTFCoef_4			:  9;\
		UNSG32 sVTFCoef1_VTFCoef_5			:  9;\
		UNSG32 RSVDxB8_b27				:  5;\
	}
	union {
		UNSG32 u32MadiReg_VTFCoef1;
		struct w32MadiReg_VTFCoef1;
	};

	#define     w32MadiReg_VTFCoef2				{\
		UNSG32 sVTFCoef2_VTFCoef_6			:  9;\
		UNSG32 RSVDxBC_b9				: 23;\
	}
	union {
		UNSG32 u32MadiReg_VTFCoef2;
		struct w32MadiReg_VTFCoef2;
	};

	#define     w32MadiReg_C420VTFCoef0			{\
		UNSG32 sC420VTFCoef0_C420VTFCoef_0		:  9;\
		UNSG32 sC420VTFCoef0_C420VTFCoef_1		:  9;\
		UNSG32 sC420VTFCoef0_C420VTFCoef_2		:  9;\
		UNSG32 RSVDxC0_b27				:  5;\
	}
	union {
		UNSG32 u32MadiReg_C420VTFCoef0;
		struct w32MadiReg_C420VTFCoef0;
	};

	#define     w32MadiReg_C420VTFCoef1			{\
		UNSG32 sC420VTFCoef1_C420VTFCoef_3		:  9;\
		UNSG32 sC420VTFCoef1_C420VTFCoef_4		:  9;\
		UNSG32 sC420VTFCoef1_C420VTFCoef_5		:  9;\
		UNSG32 RSVDxC4_b27				:  5;\
	}
	union {
		UNSG32 u32MadiReg_C420VTFCoef1;
		struct w32MadiReg_C420VTFCoef1;
	};

	#define     w32MadiReg_C420VTFCoef2			{\
		UNSG32 sC420VTFCoef2_C420VTFCoef_6		:  9;\
		UNSG32 RSVDxC8_b9				: 23;\
	}
	union {
		UNSG32 u32MadiReg_C420VTFCoef2;
		struct w32MadiReg_C420VTFCoef2;
	};

	#define     w32MadiReg_C420Mode				{\
		UNSG32 uC420Mode_C420InMode			:  1;\
		UNSG32 uC420Mode_C420OutMode			:  1;\
		UNSG32 RSVDxCC_b2				: 30;\
	}
	union {
		UNSG32 u32MadiReg_C420Mode;
		struct w32MadiReg_C420Mode;
	};

	#define     w32MadiReg_IntOutOnly			{\
		UNSG32 uIntOutOnly_IntOutOnly			:  1;\
		UNSG32 RSVDxD0_b1				: 31;\
	}
	union {
		UNSG32 u32MadiReg_IntOutOnly;
		struct w32MadiReg_IntOutOnly;
	};

	#define     w32MadiReg_ChromaMdvUseOnly			{\
		UNSG32 uChromaMdvUseOnly_ChromaMdvUseOnly	:  1;\
		UNSG32 RSVDxD4_b1				: 31;\
	}
	union {
		UNSG32 u32MadiReg_ChromaMdvUseOnly;
		struct w32MadiReg_ChromaMdvUseOnly;
	};

	#define     w32MadiReg_MdvMin				{\
		UNSG32 uMdvMin_MdvMinSt_0			:  8;\
		UNSG32 uMdvMin_MdvMinSt_1			:  8;\
		UNSG32 uMdvMin_MdvMinThr_0			:  8;\
		UNSG32 uMdvMin_MdvMinThr_1			:  8;\
	}
	union {
		UNSG32 u32MadiReg_MdvMin;
		struct w32MadiReg_MdvMin;
	};

	#define     w32MadiReg_GAccShiftBits			{\
		UNSG32 uGAccShiftBits_GhfAccShiftBits		:  2;\
		UNSG32 uGAccShiftBits_GosdAccShiftBits		:  2;\
		UNSG32 uGAccShiftBits_GrpdAccShiftBits		:  2;\
		UNSG32 uGAccShiftBits_GdiAccShiftBits		:  2;\
		UNSG32 RSVDxDC_b8				: 24;\
	}
	union {
		UNSG32 u32MadiReg_GAccShiftBits;
		struct w32MadiReg_GAccShiftBits;
	};

	#define     w32MadiReg_YUV8BitIn			{\
		UNSG32 uYUV8BitIn_YUV8BitIn			:  2;\
		UNSG32 RSVDxE0_b2				: 30;\
	}
	union {
		UNSG32 u32MadiReg_YUV8BitIn;
		struct w32MadiReg_YUV8BitIn;
	};
} SIE_MadiReg;

typedef union  T32MadiReg_Edge0
{
	UNSG32 u32;
	struct w32MadiReg_Edge0;
} T32MadiReg_Edge0;
typedef union  T32MadiReg_Edge1
{
	UNSG32 u32;
	struct w32MadiReg_Edge1;
} T32MadiReg_Edge1;
typedef union  T32MadiReg_Cnst
{
	UNSG32 u32;
	struct w32MadiReg_Cnst;
} T32MadiReg_Cnst;
typedef union  T32MadiReg_MdvCnst
{
	UNSG32 u32;
	struct w32MadiReg_MdvCnst;
} T32MadiReg_MdvCnst;
typedef union  T32MadiReg_LamdNoise
{
	UNSG32 u32;
	struct w32MadiReg_LamdNoise;
} T32MadiReg_LamdNoise;
typedef union  T32MadiReg_LasdNoise
{
	UNSG32 u32;
	struct w32MadiReg_LasdNoise;
} T32MadiReg_LasdNoise;
typedef union  T32MadiReg_LAMD
{
	UNSG32 u32;
	struct w32MadiReg_LAMD;
} T32MadiReg_LAMD;
typedef union  T32MadiReg_LASD
{
	UNSG32 u32;
	struct w32MadiReg_LASD;
} T32MadiReg_LASD;
typedef union  T32MadiReg_SOSD0
{
	UNSG32 u32;
	struct w32MadiReg_SOSD0;
} T32MadiReg_SOSD0;
typedef union  T32MadiReg_SOSD1
{
	UNSG32 u32;
	struct w32MadiReg_SOSD1;
} T32MadiReg_SOSD1;
typedef union  T32MadiReg_LAMDGain
{
	UNSG32 u32;
	struct w32MadiReg_LAMDGain;
} T32MadiReg_LAMDGain;
typedef union  T32MadiReg_LASDGain
{
	UNSG32 u32;
	struct w32MadiReg_LASDGain;
} T32MadiReg_LASDGain;
typedef union  T32MadiReg_Hf
{
	UNSG32 u32;
	struct w32MadiReg_Hf;
} T32MadiReg_Hf;
typedef union  T32MadiReg_Rpd
{
	UNSG32 u32;
	struct w32MadiReg_Rpd;
} T32MadiReg_Rpd;
typedef union  T32MadiReg_SdiSad
{
	UNSG32 u32;
	struct w32MadiReg_SdiSad;
} T32MadiReg_SdiSad;
typedef union  T32MadiReg_DiagByps
{
	UNSG32 u32;
	struct w32MadiReg_DiagByps;
} T32MadiReg_DiagByps;
typedef union  T32MadiReg_SdiNoise
{
	UNSG32 u32;
	struct w32MadiReg_SdiNoise;
} T32MadiReg_SdiNoise;
typedef union  T32MadiReg_Sdi0
{
	UNSG32 u32;
	struct w32MadiReg_Sdi0;
} T32MadiReg_Sdi0;
typedef union  T32MadiReg_Sdi1
{
	UNSG32 u32;
	struct w32MadiReg_Sdi1;
} T32MadiReg_Sdi1;
typedef union  T32MadiReg_Sdi2
{
	UNSG32 u32;
	struct w32MadiReg_Sdi2;
} T32MadiReg_Sdi2;
typedef union  T32MadiReg_ShiftBit
{
	UNSG32 u32;
	struct w32MadiReg_ShiftBit;
} T32MadiReg_ShiftBit;
typedef union  T32MadiReg_Sdi3
{
	UNSG32 u32;
	struct w32MadiReg_Sdi3;
} T32MadiReg_Sdi3;
typedef union  T32MadiReg_Sdi4
{
	UNSG32 u32;
	struct w32MadiReg_Sdi4;
} T32MadiReg_Sdi4;
typedef union  T32MadiReg_Sdi5
{
	UNSG32 u32;
	struct w32MadiReg_Sdi5;
} T32MadiReg_Sdi5;
typedef union  T32MadiReg_Sdi6
{
	UNSG32 u32;
	struct w32MadiReg_Sdi6;
} T32MadiReg_Sdi6;
typedef union  T32MadiReg_Angle
{
	UNSG32 u32;
	struct w32MadiReg_Angle;
} T32MadiReg_Angle;
typedef union  T32MadiReg_MotDet
{
	UNSG32 u32;
	struct w32MadiReg_MotDet;
} T32MadiReg_MotDet;
typedef union  T32MadiReg_HVLine
{
	UNSG32 u32;
	struct w32MadiReg_HVLine;
} T32MadiReg_HVLine;
typedef union  T32MadiReg_MdvYiir
{
	UNSG32 u32;
	struct w32MadiReg_MdvYiir;
} T32MadiReg_MdvYiir;
typedef union  T32MadiReg_C420A
{
	UNSG32 u32;
	struct w32MadiReg_C420A;
} T32MadiReg_C420A;
typedef union  T32MadiReg_C420B
{
	UNSG32 u32;
	struct w32MadiReg_C420B;
} T32MadiReg_C420B;
typedef union  T32MadiReg_MdvFilterSel
{
	UNSG32 u32;
	struct w32MadiReg_MdvFilterSel;
} T32MadiReg_MdvFilterSel;
typedef union  T32MadiReg_LcnstGain
{
	UNSG32 u32;
	struct w32MadiReg_LcnstGain;
} T32MadiReg_LcnstGain;
typedef union  T32MadiReg_YUVBlendGain
{
	UNSG32 u32;
	struct w32MadiReg_YUVBlendGain;
} T32MadiReg_YUVBlendGain;
typedef union  T32MadiReg_YUVMdvBlendGain
{
	UNSG32 u32;
	struct w32MadiReg_YUVMdvBlendGain;
} T32MadiReg_YUVMdvBlendGain;
typedef union  T32MadiReg_YMdvGain
{
	UNSG32 u32;
	struct w32MadiReg_YMdvGain;
} T32MadiReg_YMdvGain;
typedef union  T32MadiReg_YMdvGain1
{
	UNSG32 u32;
	struct w32MadiReg_YMdvGain1;
} T32MadiReg_YMdvGain1;
typedef union  T32MadiReg_YMdvOffset
{
	UNSG32 u32;
	struct w32MadiReg_YMdvOffset;
} T32MadiReg_YMdvOffset;
typedef union  T32MadiReg_UVMdvGain
{
	UNSG32 u32;
	struct w32MadiReg_UVMdvGain;
} T32MadiReg_UVMdvGain;
typedef union  T32MadiReg_Impulse
{
	UNSG32 u32;
	struct w32MadiReg_Impulse;
} T32MadiReg_Impulse;
typedef union  T32MadiReg_Nr
{
	UNSG32 u32;
	struct w32MadiReg_Nr;
} T32MadiReg_Nr;
typedef union  T32MadiReg_MdvLcnst0
{
	UNSG32 u32;
	struct w32MadiReg_MdvLcnst0;
} T32MadiReg_MdvLcnst0;
typedef union  T32MadiReg_MdvLcnst1
{
	UNSG32 u32;
	struct w32MadiReg_MdvLcnst1;
} T32MadiReg_MdvLcnst1;
typedef union  T32MadiReg_MdvLcnst2
{
	UNSG32 u32;
	struct w32MadiReg_MdvLcnst2;
} T32MadiReg_MdvLcnst2;
typedef union  T32MadiReg_MdvLcnst3
{
	UNSG32 u32;
	struct w32MadiReg_MdvLcnst3;
} T32MadiReg_MdvLcnst3;
typedef union  T32MadiReg_VTFCoef0
{
	UNSG32 u32;
	struct w32MadiReg_VTFCoef0;
} T32MadiReg_VTFCoef0;
typedef union  T32MadiReg_VTFCoef1
{
	UNSG32 u32;
	struct w32MadiReg_VTFCoef1;
} T32MadiReg_VTFCoef1;
typedef union  T32MadiReg_VTFCoef2
{
	UNSG32 u32;
	struct w32MadiReg_VTFCoef2;
} T32MadiReg_VTFCoef2;
typedef union  T32MadiReg_C420VTFCoef0
{
	UNSG32 u32;
	struct w32MadiReg_C420VTFCoef0;
} T32MadiReg_C420VTFCoef0;
typedef union  T32MadiReg_C420VTFCoef1
{
	UNSG32 u32;
	struct w32MadiReg_C420VTFCoef1;
} T32MadiReg_C420VTFCoef1;
typedef union  T32MadiReg_C420VTFCoef2
{
	UNSG32 u32;
	struct w32MadiReg_C420VTFCoef2;
} T32MadiReg_C420VTFCoef2;
typedef union  T32MadiReg_C420Mode
{
	UNSG32 u32;
	struct w32MadiReg_C420Mode;
} T32MadiReg_C420Mode;
typedef union  T32MadiReg_IntOutOnly
{
	UNSG32 u32;
	struct w32MadiReg_IntOutOnly;
} T32MadiReg_IntOutOnly;
typedef union  T32MadiReg_ChromaMdvUseOnly
{
	UNSG32 u32;
	struct w32MadiReg_ChromaMdvUseOnly;
} T32MadiReg_ChromaMdvUseOnly;
typedef union  T32MadiReg_MdvMin
{
	UNSG32 u32;
	struct w32MadiReg_MdvMin;
} T32MadiReg_MdvMin;
typedef union  T32MadiReg_GAccShiftBits
{
	UNSG32 u32;
	struct w32MadiReg_GAccShiftBits;
} T32MadiReg_GAccShiftBits;
typedef union  T32MadiReg_YUV8BitIn
{
	UNSG32 u32;
	struct w32MadiReg_YUV8BitIn;
} T32MadiReg_YUV8BitIn;

typedef union  TMadiReg_Edge0
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Edge0;
	};
} TMadiReg_Edge0;
typedef union  TMadiReg_Edge1
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Edge1;
	};
} TMadiReg_Edge1;
typedef union  TMadiReg_Cnst
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Cnst;
	};
} TMadiReg_Cnst;
typedef union  TMadiReg_MdvCnst
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MdvCnst;
	};
} TMadiReg_MdvCnst;
typedef union  TMadiReg_LamdNoise
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_LamdNoise;
	};
} TMadiReg_LamdNoise;
typedef union  TMadiReg_LasdNoise
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_LasdNoise;
	};
} TMadiReg_LasdNoise;
typedef union  TMadiReg_LAMD
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_LAMD;
	};
} TMadiReg_LAMD;
typedef union  TMadiReg_LASD
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_LASD;
	};
} TMadiReg_LASD;
typedef union  TMadiReg_SOSD0
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_SOSD0;
	};
} TMadiReg_SOSD0;
typedef union  TMadiReg_SOSD1
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_SOSD1;
	};
} TMadiReg_SOSD1;
typedef union  TMadiReg_LAMDGain
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_LAMDGain;
	};
} TMadiReg_LAMDGain;
typedef union  TMadiReg_LASDGain
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_LASDGain;
	};
} TMadiReg_LASDGain;
typedef union  TMadiReg_Hf
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Hf;
	};
} TMadiReg_Hf;
typedef union  TMadiReg_Rpd
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Rpd;
	};
} TMadiReg_Rpd;
typedef union  TMadiReg_SdiSad
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_SdiSad;
	};
} TMadiReg_SdiSad;
typedef union  TMadiReg_DiagByps
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_DiagByps;
	};
} TMadiReg_DiagByps;
typedef union  TMadiReg_SdiNoise
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_SdiNoise;
	};
} TMadiReg_SdiNoise;
typedef union  TMadiReg_Sdi0
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Sdi0;
	};
} TMadiReg_Sdi0;
typedef union  TMadiReg_Sdi1
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Sdi1;
	};
} TMadiReg_Sdi1;
typedef union  TMadiReg_Sdi2
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Sdi2;
	};
} TMadiReg_Sdi2;
typedef union  TMadiReg_ShiftBit
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_ShiftBit;
	};
} TMadiReg_ShiftBit;
typedef union  TMadiReg_Sdi3
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Sdi3;
	};
} TMadiReg_Sdi3;
typedef union  TMadiReg_Sdi4
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Sdi4;
	};
} TMadiReg_Sdi4;
typedef union  TMadiReg_Sdi5
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Sdi5;
	};
} TMadiReg_Sdi5;
typedef union  TMadiReg_Sdi6
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Sdi6;
	};
} TMadiReg_Sdi6;
typedef union  TMadiReg_Angle
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Angle;
	};
} TMadiReg_Angle;
typedef union  TMadiReg_MotDet
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MotDet;
	};
} TMadiReg_MotDet;
typedef union  TMadiReg_HVLine
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_HVLine;
	};
} TMadiReg_HVLine;
typedef union  TMadiReg_MdvYiir
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MdvYiir;
	};
} TMadiReg_MdvYiir;
typedef union  TMadiReg_C420A
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_C420A;
	};
} TMadiReg_C420A;
typedef union  TMadiReg_C420B
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_C420B;
	};
} TMadiReg_C420B;
typedef union  TMadiReg_MdvFilterSel
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MdvFilterSel;
	};
} TMadiReg_MdvFilterSel;
typedef union  TMadiReg_LcnstGain
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_LcnstGain;
	};
} TMadiReg_LcnstGain;
typedef union  TMadiReg_YUVBlendGain
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_YUVBlendGain;
	};
} TMadiReg_YUVBlendGain;
typedef union  TMadiReg_YUVMdvBlendGain
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_YUVMdvBlendGain;
	};
} TMadiReg_YUVMdvBlendGain;
typedef union  TMadiReg_YMdvGain
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_YMdvGain;
	};
} TMadiReg_YMdvGain;
typedef union  TMadiReg_YMdvGain1
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_YMdvGain1;
	};
} TMadiReg_YMdvGain1;
typedef union  TMadiReg_YMdvOffset
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_YMdvOffset;
	};
} TMadiReg_YMdvOffset;
typedef union  TMadiReg_UVMdvGain
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_UVMdvGain;
	};
} TMadiReg_UVMdvGain;
typedef union  TMadiReg_Impulse
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Impulse;
	};
} TMadiReg_Impulse;
typedef union  TMadiReg_Nr
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_Nr;
	};
} TMadiReg_Nr;
typedef union  TMadiReg_MdvLcnst0
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MdvLcnst0;
	};
} TMadiReg_MdvLcnst0;
typedef union  TMadiReg_MdvLcnst1
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MdvLcnst1;
	};
} TMadiReg_MdvLcnst1;
typedef union  TMadiReg_MdvLcnst2
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MdvLcnst2;
	};
} TMadiReg_MdvLcnst2;
typedef union  TMadiReg_MdvLcnst3
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MdvLcnst3;
	};
} TMadiReg_MdvLcnst3;
typedef union  TMadiReg_VTFCoef0
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_VTFCoef0;
	};
} TMadiReg_VTFCoef0;
typedef union  TMadiReg_VTFCoef1
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_VTFCoef1;
	};
} TMadiReg_VTFCoef1;
typedef union  TMadiReg_VTFCoef2
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_VTFCoef2;
	};
} TMadiReg_VTFCoef2;
typedef union  TMadiReg_C420VTFCoef0
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_C420VTFCoef0;
	};
} TMadiReg_C420VTFCoef0;
typedef union  TMadiReg_C420VTFCoef1
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_C420VTFCoef1;
	};
} TMadiReg_C420VTFCoef1;
typedef union  TMadiReg_C420VTFCoef2
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_C420VTFCoef2;
	};
} TMadiReg_C420VTFCoef2;
typedef union  TMadiReg_C420Mode
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_C420Mode;
	};
} TMadiReg_C420Mode;
typedef union  TMadiReg_IntOutOnly
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_IntOutOnly;
	};
} TMadiReg_IntOutOnly;
typedef union  TMadiReg_ChromaMdvUseOnly
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_ChromaMdvUseOnly;
	};
} TMadiReg_ChromaMdvUseOnly;
typedef union  TMadiReg_MdvMin
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_MdvMin;
	};
} TMadiReg_MdvMin;
typedef union  TMadiReg_GAccShiftBits
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_GAccShiftBits;
	};
} TMadiReg_GAccShiftBits;
typedef union  TMadiReg_YUV8BitIn
{
	UNSG32 u32[1];
	struct {
		struct w32MadiReg_YUV8BitIn;
	};
} TMadiReg_YUV8BitIn;

SIGN32 MadiReg_drvrd(SIE_MadiReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MadiReg_drvwr(SIE_MadiReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MadiReg_reset(SIE_MadiReg *p);
SIGN32 MadiReg_cmp  (SIE_MadiReg *p, SIE_MadiReg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MadiReg_check(p,pie,pfx,hLOG) MadiReg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MadiReg_print(p,    pfx,hLOG) MadiReg_cmp(p,0,	pfx,(void*)(hLOG),0,0)
#endif

#ifndef h_FmdReg
#define h_FmdReg (){}
#define     RA_FmdReg_FieldDifThr			   0x0000
#define     RA_FmdReg_FmdHmeGain0			   0x0004
#define     RA_FmdReg_FmdHmeGain1			   0x0008
#define     RA_FmdReg_FmdHmeOff0			   0x000C
#define     RA_FmdReg_FmdHmeOff1			   0x0010
#define     RA_FmdReg_FmdHmeThr				   0x0014
#define     RA_FmdReg_FmdGhmvShiftBit			   0x0018
#define     RA_FmdReg_FmdScrTextFixLineDly0		   0x001C
#define     RA_FmdReg_FmdScrTextFixLineDly1		   0x0020
#define     RA_FmdReg_FmdHWEn				   0x0024
#define     RA_FmdReg_ChromaUseMadiOnly			   0x0028
#define     RA_FmdReg_Fmdhmvoff				   0x002C
#define     RA_FmdReg_FmdLmvThr				   0x0030
#define     RA_FmdReg_Fmdhmvshiftbits			   0x0034
#define     RA_FmdReg_FmdhmeThr0			   0x0038
#define     RA_FmdReg_FmdhmeThr1			   0x003C
#define     RA_FmdReg_FmdhmeThr2			   0x0040
#define     RA_FmdReg_FmdmdvThr0			   0x0044
#define     RA_FmdReg_FmdmdvThr1			   0x0048

typedef struct SIE_FmdReg {
	#define     w32FmdReg_FieldDifThr			{\
		UNSG32 uFieldDifThr_FieldDifThr_0		:  8;\
		UNSG32 uFieldDifThr_FieldDifThr_1		:  8;\
		UNSG32 uFieldDifThr_FieldDifThr_2		:  8;\
		UNSG32 uFieldDifThr_FieldDifThr_3		:  8;\
	}
	union {
		UNSG32 u32FmdReg_FieldDifThr;
		struct w32FmdReg_FieldDifThr;
	};

	#define     w32FmdReg_FmdHmeGain0			{\
		UNSG32 uFmdHmeGain0_FmdHmeGain_0		:  8;\
		UNSG32 uFmdHmeGain0_FmdHmeGain_1		:  8;\
		UNSG32 uFmdHmeGain0_FmdHmeGain_2		:  8;\
		UNSG32 uFmdHmeGain0_FmdHmeGain_3		:  8;\
	}
	union {
		UNSG32 u32FmdReg_FmdHmeGain0;
		struct w32FmdReg_FmdHmeGain0;
	};

	#define     w32FmdReg_FmdHmeGain1			{\
		UNSG32 uFmdHmeGain1_FmdHmeGain_4		:  8;\
		UNSG32 uFmdHmeGain1_FmdHmeGain_5		:  8;\
		UNSG32 uFmdHmeGain1_FmdHmeGain_6		:  8;\
		UNSG32 RSVDx8_b24				:  8;\
	}
	union {
		UNSG32 u32FmdReg_FmdHmeGain1;
		struct w32FmdReg_FmdHmeGain1;
	};

	#define     w32FmdReg_FmdHmeOff0			{\
		UNSG32 uFmdHmeOff0_FmdHmeOff_0			:  8;\
		UNSG32 uFmdHmeOff0_FmdHmeOff_1			:  8;\
		UNSG32 uFmdHmeOff0_FmdHmeOff_2			:  8;\
		UNSG32 uFmdHmeOff0_FmdHmeOff_3			:  8;\
	}
	union {
		UNSG32 u32FmdReg_FmdHmeOff0;
		struct w32FmdReg_FmdHmeOff0;
	};

	#define     w32FmdReg_FmdHmeOff1			{\
		UNSG32 uFmdHmeOff1_FmdHmeOff_4			:  8;\
		UNSG32 uFmdHmeOff1_FmdHmeOff_5			:  8;\
		UNSG32 uFmdHmeOff1_FmdHmeOff_6			:  8;\
		UNSG32 RSVDx10_b24				:  8;\
	}
	union {
		UNSG32 u32FmdReg_FmdHmeOff1;
		struct w32FmdReg_FmdHmeOff1;
	};

	#define     w32FmdReg_FmdHmeThr				{\
		UNSG32 uFmdHmeThr_FmdHmeNoiseThr		:  4;\
		UNSG32 uFmdHmeThr_FmdHmeDifThr			:  3;\
		UNSG32 uFmdHmeThr_FmdHmeLowCnstThr		:  8;\
		UNSG32 uFmdHmeThr_FmdStillThr			:  8;\
		UNSG32 RSVDx14_b23				:  9;\
	}
	union {
		UNSG32 u32FmdReg_FmdHmeThr;
		struct w32FmdReg_FmdHmeThr;
	};

	#define     w32FmdReg_FmdGhmvShiftBit			{\
		UNSG32 uFmdGhmvShiftBit_FmdGhmvShiftBit		:  4;\
		UNSG32 RSVDx18_b4				: 28;\
	}
	union {
		UNSG32 u32FmdReg_FmdGhmvShiftBit;
		struct w32FmdReg_FmdGhmvShiftBit;
	};

	#define     w32FmdReg_FmdScrTextFixLineDly0		{\
		UNSG32 uFmdScrTextFixLineDly0_FmdScrTextFixLineDly_0: 10;\
		UNSG32 uFmdScrTextFixLineDly0_FmdScrTextFixLineDly_1: 10;\
		UNSG32 uFmdScrTextFixLineDly0_FmdScrTextFixLineDly_2: 10;\
		UNSG32 RSVDx1C_b30				:  2;\
	}
	union {
		UNSG32 u32FmdReg_FmdScrTextFixLineDly0;
		struct w32FmdReg_FmdScrTextFixLineDly0;
	};

	#define     w32FmdReg_FmdScrTextFixLineDly1		{\
		UNSG32 uFmdScrTextFixLineDly1_FmdScrTextFixLineDly_3: 10;\
		UNSG32 uFmdScrTextFixLineDly1_FmdScrTextFixLineDly_4: 10;\
		UNSG32 RSVDx20_b20				: 12;\
	}
	union {
		UNSG32 u32FmdReg_FmdScrTextFixLineDly1;
		struct w32FmdReg_FmdScrTextFixLineDly1;
	};

	#define     w32FmdReg_FmdHWEn				{\
		UNSG32 uFmdHWEn_FmdHWEn				:  1;\
		UNSG32 RSVDx24_b1				: 31;\
	}
	union {
		UNSG32 u32FmdReg_FmdHWEn;
		struct w32FmdReg_FmdHWEn;
	};

	#define     w32FmdReg_ChromaUseMadiOnly			{\
		UNSG32 uChromaUseMadiOnly_ChromaUseMadiOnly	:  1;\
		UNSG32 RSVDx28_b1				: 31;\
	}
	union {
		UNSG32 u32FmdReg_ChromaUseMadiOnly;
		struct w32FmdReg_ChromaUseMadiOnly;
	};

	#define     w32FmdReg_Fmdhmvoff				{\
		UNSG32 uFmdhmvoff_Fmdhmvoff			:  8;\
		UNSG32 RSVDx2C_b8				: 24;\
	}
	union {
		UNSG32 u32FmdReg_Fmdhmvoff;
		struct w32FmdReg_Fmdhmvoff;
	};

	#define     w32FmdReg_FmdLmvThr				{\
		UNSG32 uFmdLmvThr_FmdLmvThr			:  3;\
		UNSG32 RSVDx30_b3				: 29;\
	}
	union {
		UNSG32 u32FmdReg_FmdLmvThr;
		struct w32FmdReg_FmdLmvThr;
	};

	#define     w32FmdReg_Fmdhmvshiftbits			{\
		UNSG32 uFmdhmvshiftbits_Fmdhmvshiftbits_0	:  4;\
		UNSG32 uFmdhmvshiftbits_Fmdhmvshiftbits_1	:  4;\
		UNSG32 RSVDx34_b8				: 24;\
	}
	union {
		UNSG32 u32FmdReg_Fmdhmvshiftbits;
		struct w32FmdReg_Fmdhmvshiftbits;
	};

	#define     w32FmdReg_FmdhmeThr0			{\
		UNSG32 uFmdhmeThr0_FmdhmvThr_0			:  8;\
		UNSG32 uFmdhmeThr0_FmdhmvThr_1			:  8;\
		UNSG32 uFmdhmeThr0_FmdhmvThr_2			:  8;\
		UNSG32 uFmdhmeThr0_FmdhmvThr_3			:  8;\
	}
	union {
		UNSG32 u32FmdReg_FmdhmeThr0;
		struct w32FmdReg_FmdhmeThr0;
	};

	#define     w32FmdReg_FmdhmeThr1			{\
		UNSG32 uFmdhmeThr1_FmdhmvThr_4			:  8;\
		UNSG32 uFmdhmeThr1_FmdhmvThr_5			:  8;\
		UNSG32 uFmdhmeThr1_FmdhmvThr_6			:  8;\
		UNSG32 uFmdhmeThr1_FmdhmvThr_7			:  8;\
	}
	union {
		UNSG32 u32FmdReg_FmdhmeThr1;
		struct w32FmdReg_FmdhmeThr1;
	};

	#define     w32FmdReg_FmdhmeThr2			{\
		UNSG32 uFmdhmeThr2_FmdhmvThr_8			:  8;\
		UNSG32 RSVDx40_b8				: 24;\
	}
	union {
		UNSG32 u32FmdReg_FmdhmeThr2;
		struct w32FmdReg_FmdhmeThr2;
	};

	#define     w32FmdReg_FmdmdvThr0			{\
		UNSG32 uFmdmdvThr0_FmdmdvThr_0			:  6;\
		UNSG32 uFmdmdvThr0_FmdmdvThr_1			:  6;\
		UNSG32 uFmdmdvThr0_FmdmdvThr_2			:  6;\
		UNSG32 uFmdmdvThr0_FmdmdvThr_3			:  6;\
		UNSG32 RSVDx44_b24				:  8;\
	}
	union {
		UNSG32 u32FmdReg_FmdmdvThr0;
		struct w32FmdReg_FmdmdvThr0;
	};

	#define     w32FmdReg_FmdmdvThr1			{\
		UNSG32 uFmdmdvThr1_FmdmdvThr_4			:  6;\
		UNSG32 uFmdmdvThr1_FmdmdvThr_5			:  6;\
		UNSG32 uFmdmdvThr1_FmdmdvThr_6			:  6;\
		UNSG32 uFmdmdvThr1_FmdmdvThr_7			:  6;\
		UNSG32 RSVDx48_b24				:  8;\
	}
	union {
		UNSG32 u32FmdReg_FmdmdvThr1;
		struct w32FmdReg_FmdmdvThr1;
	};
} SIE_FmdReg;

typedef union  T32FmdReg_FieldDifThr
{
	UNSG32 u32;
	struct w32FmdReg_FieldDifThr;
} T32FmdReg_FieldDifThr;
typedef union  T32FmdReg_FmdHmeGain0
{
	UNSG32 u32;
	struct w32FmdReg_FmdHmeGain0;
} T32FmdReg_FmdHmeGain0;
typedef union  T32FmdReg_FmdHmeGain1
{
	UNSG32 u32;
	struct w32FmdReg_FmdHmeGain1;
} T32FmdReg_FmdHmeGain1;
typedef union  T32FmdReg_FmdHmeOff0
{
	UNSG32 u32;
	struct w32FmdReg_FmdHmeOff0;
} T32FmdReg_FmdHmeOff0;
typedef union  T32FmdReg_FmdHmeOff1
{
	UNSG32 u32;
	struct w32FmdReg_FmdHmeOff1;
} T32FmdReg_FmdHmeOff1;
typedef union  T32FmdReg_FmdHmeThr
{
	UNSG32 u32;
	struct w32FmdReg_FmdHmeThr;
} T32FmdReg_FmdHmeThr;
typedef union  T32FmdReg_FmdGhmvShiftBit
{
	UNSG32 u32;
	struct w32FmdReg_FmdGhmvShiftBit;
} T32FmdReg_FmdGhmvShiftBit;
typedef union  T32FmdReg_FmdScrTextFixLineDly0
{
	UNSG32 u32;
	struct w32FmdReg_FmdScrTextFixLineDly0;
} T32FmdReg_FmdScrTextFixLineDly0;
typedef union  T32FmdReg_FmdScrTextFixLineDly1
{
	UNSG32 u32;
	struct w32FmdReg_FmdScrTextFixLineDly1;
} T32FmdReg_FmdScrTextFixLineDly1;
typedef union  T32FmdReg_FmdHWEn
{
	UNSG32 u32;
	struct w32FmdReg_FmdHWEn;
} T32FmdReg_FmdHWEn;
typedef union  T32FmdReg_ChromaUseMadiOnly
{
	UNSG32 u32;
	struct w32FmdReg_ChromaUseMadiOnly;
} T32FmdReg_ChromaUseMadiOnly;
typedef union  T32FmdReg_Fmdhmvoff
{
	UNSG32 u32;
	struct w32FmdReg_Fmdhmvoff;
} T32FmdReg_Fmdhmvoff;
typedef union  T32FmdReg_FmdLmvThr
{
	UNSG32 u32;
	struct w32FmdReg_FmdLmvThr;
} T32FmdReg_FmdLmvThr;
typedef union  T32FmdReg_Fmdhmvshiftbits
{
	UNSG32 u32;
	struct w32FmdReg_Fmdhmvshiftbits;
} T32FmdReg_Fmdhmvshiftbits;
typedef union  T32FmdReg_FmdhmeThr0
{
	UNSG32 u32;
	struct w32FmdReg_FmdhmeThr0;
} T32FmdReg_FmdhmeThr0;
typedef union  T32FmdReg_FmdhmeThr1
{
	UNSG32 u32;
	struct w32FmdReg_FmdhmeThr1;
} T32FmdReg_FmdhmeThr1;
typedef union  T32FmdReg_FmdhmeThr2
{
	UNSG32 u32;
	struct w32FmdReg_FmdhmeThr2;
} T32FmdReg_FmdhmeThr2;
typedef union  T32FmdReg_FmdmdvThr0
{
	UNSG32 u32;
	struct w32FmdReg_FmdmdvThr0;
} T32FmdReg_FmdmdvThr0;
typedef union  T32FmdReg_FmdmdvThr1
{
	UNSG32 u32;
	struct w32FmdReg_FmdmdvThr1;
} T32FmdReg_FmdmdvThr1;

typedef union  TFmdReg_FieldDifThr
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FieldDifThr;
	};
} TFmdReg_FieldDifThr;
typedef union  TFmdReg_FmdHmeGain0
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdHmeGain0;
	};
} TFmdReg_FmdHmeGain0;
typedef union  TFmdReg_FmdHmeGain1
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdHmeGain1;
	};
} TFmdReg_FmdHmeGain1;
typedef union  TFmdReg_FmdHmeOff0
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdHmeOff0;
	};
} TFmdReg_FmdHmeOff0;
typedef union  TFmdReg_FmdHmeOff1
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdHmeOff1;
	};
} TFmdReg_FmdHmeOff1;
typedef union  TFmdReg_FmdHmeThr
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdHmeThr;
	};
} TFmdReg_FmdHmeThr;
typedef union  TFmdReg_FmdGhmvShiftBit
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdGhmvShiftBit;
	};
} TFmdReg_FmdGhmvShiftBit;
typedef union  TFmdReg_FmdScrTextFixLineDly0
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdScrTextFixLineDly0;
	};
} TFmdReg_FmdScrTextFixLineDly0;
typedef union  TFmdReg_FmdScrTextFixLineDly1
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdScrTextFixLineDly1;
	};
} TFmdReg_FmdScrTextFixLineDly1;
typedef union  TFmdReg_FmdHWEn
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdHWEn;
	};
} TFmdReg_FmdHWEn;
typedef union  TFmdReg_ChromaUseMadiOnly
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_ChromaUseMadiOnly;
	};
} TFmdReg_ChromaUseMadiOnly;
typedef union  TFmdReg_Fmdhmvoff
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_Fmdhmvoff;
	};
} TFmdReg_Fmdhmvoff;
typedef union  TFmdReg_FmdLmvThr
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdLmvThr;
	};
} TFmdReg_FmdLmvThr;
typedef union  TFmdReg_Fmdhmvshiftbits
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_Fmdhmvshiftbits;
	};
} TFmdReg_Fmdhmvshiftbits;
typedef union  TFmdReg_FmdhmeThr0
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdhmeThr0;
	};
} TFmdReg_FmdhmeThr0;
typedef union  TFmdReg_FmdhmeThr1
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdhmeThr1;
	};
} TFmdReg_FmdhmeThr1;
typedef union  TFmdReg_FmdhmeThr2
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdhmeThr2;
	};
} TFmdReg_FmdhmeThr2;
typedef union  TFmdReg_FmdmdvThr0
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdmdvThr0;
	};
} TFmdReg_FmdmdvThr0;
typedef union  TFmdReg_FmdmdvThr1
{
	UNSG32 u32[1];
	struct {
		struct w32FmdReg_FmdmdvThr1;
	};
} TFmdReg_FmdmdvThr1;

SIGN32 FmdReg_drvrd(SIE_FmdReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 FmdReg_drvwr(SIE_FmdReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void FmdReg_reset(SIE_FmdReg *p);
SIGN32 FmdReg_cmp  (SIE_FmdReg *p, SIE_FmdReg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define FmdReg_check(p,pie,pfx,hLOG) FmdReg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define FmdReg_print(p,    pfx,hLOG) FmdReg_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif

#ifndef h_OvpReg
#define h_OvpReg (){}
#define	RA_OvpReg_Reset				0x0000
#define	RA_OvpReg_ClockControl			0x0004
#define	RA_OvpReg_CmdAddrPush			0x0010
#define	RA_OvpReg_CmdAddrLevel			0x0014
#define	RA_OvpReg_CmdAddrRead			0x0020
#define	RA_OvpReg_CmdAddrRead1			0x0024
#define	RA_OvpReg_CmdAddrRead2			0x0028
#define	RA_OvpReg_CmdAddrRead3			0x002C
#define	RA_OvpReg_IntStatus			0x0040
#define	RA_OvpReg_IntMask			0x0050
#define	RA_OvpReg_AxiErrLog			0x0060
#define	RA_OvpReg_AxiParam			0x0080
#define	RA_OvpReg_MadiReg			0x0400
#define	RA_OvpReg_FmdReg			0x0800

/* Defined values for RA_OvpReg_IntStatus */
#define INT_STATUS_CMD_ADDR_OVERFLOW		0x1
#define INT_STATUS_CMD_ADDR_FULL		0x2
#define INT_STATUS_CMD_ADDR_ALMOST_EMPTY	0x4
#define INT_STATUS_CMD_ADDR_DONE		0x8
#define INT_STATUS_CMD_ADDR_ERROR		0x10
#define INT_STATUS_CMD_ADDR_AXI_ERROR		0x20

typedef struct SIE_OvpReg {

	#define     w32OvpReg_Reset				{\
		UNSG32 uReset_Reset				:  1;\
		UNSG32 RSVDx0_b1				: 31;\
	}
	union {
		UNSG32 u32OvpReg_Reset;
		struct w32OvpReg_Reset;
	};

	#define     w32OvpReg_ClockControl			{\
		UNSG32 uClockControl_ClockGatingEnable		:  1;\
		UNSG32 RSVDx4_b1				: 31;\
	}
	union {
		UNSG32 u32OvpReg_ClockControl;
		struct w32OvpReg_ClockControl;
	};
	UNSG8 RSVDx8					  [8];

	#define     w32OvpReg_CmdAddrPush			{\
		UNSG32 uCmdAddrPush_Address			: 32;\
	}
	union {
		UNSG32 u32OvpReg_CmdAddrPush;
		struct w32OvpReg_CmdAddrPush;
	};

	#define     w32OvpReg_CmdAddrLevel			{\
		UNSG32 uCmdAddrLevel_Level			:  3;\
		UNSG32 RSVDx14_b3				: 29;\
	}
	union {
		UNSG32 u32OvpReg_CmdAddrLevel;
		struct w32OvpReg_CmdAddrLevel;
	};
	UNSG8 RSVDx18					  [8];

	#define     w32OvpReg_CmdAddrRead			{\
		UNSG32 uCmdAddrRead_Address_0i			: 32;\
	}
	union {
		UNSG32 u32OvpReg_CmdAddrRead;
		struct w32OvpReg_CmdAddrRead;
	};

	#define     w32OvpReg_CmdAddrRead1			{\
		UNSG32 uCmdAddrRead_Address_1i			: 32;\
	}
	union {
		UNSG32 u32OvpReg_CmdAddrRead1;
		struct w32OvpReg_CmdAddrRead1;
	};

	#define     w32OvpReg_CmdAddrRead2			{\
		UNSG32 uCmdAddrRead_Address_2i			: 32;\
	}
	union {
		UNSG32 u32OvpReg_CmdAddrRead2;
		struct w32OvpReg_CmdAddrRead2;
	};

	#define     w32OvpReg_CmdAddrRead3			{\
		UNSG32 uCmdAddrRead_Address_3i			: 32;\
	}
	union {
		UNSG32 u32OvpReg_CmdAddrRead3;
		struct w32OvpReg_CmdAddrRead3;
	};
	UNSG8 RSVDx30					  	[16];

	#define     w32OvpReg_IntStatus				{\
		UNSG32 uIntStatus_CmdAddrOverflow		:  1;\
		UNSG32 uIntStatus_CmdAddrFull			:  1;\
		UNSG32 uIntStatus_CmdAddrAlmostEmpty		:  1;\
		UNSG32 uIntStatus_CmdDone			:  1;\
		UNSG32 uIntStatus_CmdError			:  1;\
		UNSG32 uIntStatus_AxiError			:  1;\
		UNSG32 RSVDx40_b6				: 26;\
	}
	union {
		UNSG32 u32OvpReg_IntStatus;
		struct w32OvpReg_IntStatus;
	};
	UNSG8 RSVDx44						[12];

	#define     w32OvpReg_IntMask				{\
		UNSG32 uIntMask_IntMask				:  6;\
		UNSG32 RSVDx50_b6				: 26;\
	}
	union {
		UNSG32 u32OvpReg_IntMask;
		struct w32OvpReg_IntMask;
	};
	UNSG8 RSVDx54						[12];

	#define     w32OvpReg_AxiErrLog				{\
		UNSG32 uAxiErrLog_BID				:  3;\
		UNSG32 uAxiErrLog_BRESP				:  2;\
		UNSG32 uAxiErrLog_RID				:  3;\
		UNSG32 uAxiErrLog_RRESP				:  2;\
		UNSG32 RSVDx60_b10				: 22;\
	}
	union {
		UNSG32 u32OvpReg_AxiErrLog;
		struct w32OvpReg_AxiErrLog;
	};
	UNSG8 RSVDx64						[28];

	#define     w32OvpReg_AxiParam				{\
		UNSG32 uAxiParam_ALEN				:  8;\
		UNSG32 RSVDx80_b8				: 24;\
	}
	union {
		UNSG32 u32OvpReg_AxiParam;
		struct w32OvpReg_AxiParam;
	};

	UNSG8 RSVDx84						[892];
	SIE_MadiReg						ie_MadiReg;
	UNSG8 RSVDx4E4						[796];
	SIE_FmdReg						ie_FmdReg;
} SIE_OvpReg;

typedef union  T32OvpReg_Reset
{
	UNSG32 u32;
	struct w32OvpReg_Reset;
} T32OvpReg_Reset;
typedef union  T32OvpReg_ClockControl
{
	UNSG32 u32;
	struct w32OvpReg_ClockControl;
} T32OvpReg_ClockControl;
typedef union  T32OvpReg_CmdAddrPush
{
	UNSG32 u32;
	struct w32OvpReg_CmdAddrPush;
} T32OvpReg_CmdAddrPush;
typedef union  T32OvpReg_CmdAddrLevel
{
	UNSG32 u32;
	struct w32OvpReg_CmdAddrLevel;
} T32OvpReg_CmdAddrLevel;
typedef union  T32OvpReg_CmdAddrRead
{
	UNSG32 u32;
	struct w32OvpReg_CmdAddrRead;
} T32OvpReg_CmdAddrRead;
typedef union  T32OvpReg_CmdAddrRead1
{
	UNSG32 u32;
	struct w32OvpReg_CmdAddrRead1;
} T32OvpReg_CmdAddrRead1;
typedef union  T32OvpReg_CmdAddrRead2
{
	UNSG32 u32;
	struct w32OvpReg_CmdAddrRead2;
} T32OvpReg_CmdAddrRead2;
typedef union  T32OvpReg_CmdAddrRead3
{
	UNSG32 u32;
	struct w32OvpReg_CmdAddrRead3;
} T32OvpReg_CmdAddrRead3;
typedef union  T32OvpReg_IntStatus
{
	UNSG32 u32;
	struct w32OvpReg_IntStatus;
} T32OvpReg_IntStatus;
typedef union  T32OvpReg_IntMask
{
	UNSG32 u32;
	struct w32OvpReg_IntMask;
} T32OvpReg_IntMask;
typedef union  T32OvpReg_AxiErrLog
{
	UNSG32 u32;
	struct w32OvpReg_AxiErrLog;
} T32OvpReg_AxiErrLog;
typedef union  T32OvpReg_AxiParam
{
	UNSG32 u32;
	struct w32OvpReg_AxiParam;
} T32OvpReg_AxiParam;

typedef union  TOvpReg_Reset
{
	UNSG32 u32[1];
	struct {
		struct w32OvpReg_Reset;
	};
} TOvpReg_Reset;
typedef union  TOvpReg_ClockControl
{
	UNSG32 u32[1];
	struct {
		struct w32OvpReg_ClockControl;
	};
} TOvpReg_ClockControl;
typedef union  TOvpReg_CmdAddrPush
{
	UNSG32 u32[1];
	struct {
		struct w32OvpReg_CmdAddrPush;
	};
} TOvpReg_CmdAddrPush;
typedef union  TOvpReg_CmdAddrLevel
{
	UNSG32 u32[1];
	struct {
		struct w32OvpReg_CmdAddrLevel;
	};
} TOvpReg_CmdAddrLevel;
typedef union  TOvpReg_CmdAddrRead
{
	UNSG32 u32[4];
	struct {
		struct w32OvpReg_CmdAddrRead;
		struct w32OvpReg_CmdAddrRead1;
		struct w32OvpReg_CmdAddrRead2;
		struct w32OvpReg_CmdAddrRead3;
	};
} TOvpReg_CmdAddrRead;
typedef union  TOvpReg_IntStatus
{
	UNSG32 u32[1];
	struct {
		struct w32OvpReg_IntStatus;
	};
} TOvpReg_IntStatus;
typedef union  TOvpReg_IntMask
{
	UNSG32 u32[1];
	struct {
		struct w32OvpReg_IntMask;
	};
} TOvpReg_IntMask;
typedef union  TOvpReg_AxiErrLog
{
	UNSG32 u32[1];
	struct {
		struct w32OvpReg_AxiErrLog;
	};
} TOvpReg_AxiErrLog;
typedef union  TOvpReg_AxiParam
{
	UNSG32 u32[1];
	struct {
		struct w32OvpReg_AxiParam;
	};
} TOvpReg_AxiParam;

SIGN32 OvpReg_drvrd(SIE_OvpReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 OvpReg_drvwr(SIE_OvpReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void OvpReg_reset(SIE_OvpReg *p);
SIGN32 OvpReg_cmp  (SIE_OvpReg *p, SIE_OvpReg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define OvpReg_check(p,pie,pfx,hLOG) OvpReg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define OvpReg_print(p,    pfx,hLOG) OvpReg_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif

#ifdef __cplusplus
}
#endif
#pragma  pack()

#endif
