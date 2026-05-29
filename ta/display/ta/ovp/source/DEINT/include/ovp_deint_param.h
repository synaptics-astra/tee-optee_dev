/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#ifndef _OVP_DEINT_PARAM_H_
#define _OVP_DEINT_PARAM_H_

#ifdef __cplusplus
extern "C" {
#endif

/************* Header file include ********************************************/
#include "com_type.h"

#define BLKLOGICDISABLE        1
#define HBLKNUM		    16
#define VBLKNUM		    16

typedef struct t_OVP_MADI
{
	unsigned int m_uiMadiRegShiftBit;
	unsigned int m_uiMadiRegLAMD;
	unsigned int m_uiMadiRegMotDet;
	unsigned int m_uiMadiRegSOSD0;
	unsigned int m_uiMadiRegRpd;
} OVP_MADI;

typedef struct DI_FW_IDATA_STRUCT_T
{
	UINT *Firmware_g_hwReg;
	//Add your INTernal global firmware variable here
	INT var1;
	UINT fmd;
	UINT ghf;
	UINT gdif;
	UINT glmv;
	UINT Fmd_lgmv_acc;
	UINT Fmd_lgmv_tacc;
	UINT mcdibyps;
	UINT grpd;
	UINT ghmv;
	UINT rhmv[13];
	UINT gosd;
	UINT gmdv;
	UINT gmin;
	UINT glamd;
	UINT ChromaUpSampleError;
	UINT ghmv_acc[13];
	UINT ghf_acc;
	UINT glamd_acc;
	UINT grpd_acc;
	UINT gosd_acc;
	UINT gmdv_acc;
	UINT gdif_acc;
	UINT gmdv_max;
	UINT gmdv_min;
	UINT C420Fix_acc;
	UINT C420Fix_yacc;
	UINT C420FixState[2];
	UINT GHFState;
	UINT GDIFState;
	UINT GLMVState;
	UINT FMDState[2];
	UINT GRPDState;
	UINT GHMVState[13];
	UINT GOSDState;
	UINT ScrGOSDState;
	UINT GMDVState[2];
	UINT GLAMDState;
	UINT FrameReset;
	UINT DiFWReset;
	UINT bmdv[16][16];
	UINT bmdv_state[16][16];
	UINT blamd[16][16];
	UINT bstc[16][16];
	UINT bmhf[16][16];
	UINT bosd[16][16];
	UINT bstc_state[16][16];
	UINT bmhf_state[16][16];
	UINT bosd_state[16][16];
	UINT blasd[16][16];
	UINT bsosd[16][16];
	UINT blmhf[16][16];
	UINT **Yhsgrm;
	UINT YMdvThr[4];
	UINT UVMdvThr[4];
	UINT AngHsgrmThr[2];
	UINT IntValueThr[3];
	UINT NLE;
	UINT Fmd_f0f1_macc[8];
	UINT Fmd_f0f1_sacc[8];
	UINT Fmd_f2f1_macc[8];
	UINT Fmd_f2f1_sacc[8];
	UINT Fmd_still;
	UINT AFDF0VPosDelay[4];
	UINT AFDF1VPosDelay[4];
	UINT AFDF2VPosDelay[4];
	UINT AFDF3VPosDelay[4];

} DI_FW_IDATA_STRUCT;

typedef struct DI_FW_REG_STRUCT_T
{
	//Add your firmware register variable here
	INT Reg1;
	UINT GhfEn;
	UINT GhfUser;
	UINT GdifEn;
	UINT GdifUser;
	UINT FmdMcdiBypsThr;
	UINT FmdFWEn;
	UINT FmdLmvEn;
	UINT FmdUser;
	UINT FmdLmvUser;
	UINT FmdThr[13];
	UINT GhmvEn;
	UINT GhmvUser;
	UINT GrpdEn;
	UINT GrpdUser;
	UINT GosdEn;
	UINT GosdUser;
	UINT C420FixIirFilterEn;

	UINT C420AccThr[3];
	UINT CUEid;
	UINT C420FixEn;

	UINT GmdvEn;
	UINT GmdvMinEn;
	UINT GlamdEn;
	UINT GmdvUser;
	UINT GmdvMinUser;
	UINT GlamdUser;
	UINT BLAMDEn;
	UINT BLAMDUser;
	UINT BLASDEn;
	UINT BLASDUser;
	UINT BSOSDEn;
	UINT BSOSDUser;
	UINT BLMHFEn;
	UINT BLMHFUser;
	UINT NLEEn;
	UINT NLEUser;
	UINT NoiseThr[3];
	UINT GmdvThr[2];
	UINT GmdvMinThr[3];
	UINT GlamdThr[3];
	UINT GhfThr[2];
	UINT GdifThr;
	UINT FmdGlmvThr[2];
	UINT FmdScrTxtSel;
	UINT YMdvGlmvThr[4];
	UINT GosdThr[2];
	UINT GrpdThr[3];
	UINT FmdGrpdThr[2];
	UINT GhmvThr[3];
	UINT FmdGain[4];
	UINT FmdCountThr[5];
	INT FmdOff[13];

	UINT YMdvThr[4];
	UINT UVMdvThr[4];
	UINT YMdvGhfThr[4];
	UINT UVMdvGhfThr[4];
	UINT YMdvGvhfThr[4];
	UINT UVMdvGvhfThr[4];
	UINT AngHsgrmThr[4];
	UINT IntValueThr[6];
	UINT SOSDCountThr;
	UINT LMHFCountThr;
	UINT LASDCountThr;
	UINT LAMDCountGain;
	UINT SdiOff0[7];
	UINT SdiGain0[7];
	UINT SdiOff1[7];
	UINT SdiGain1[7];
	UINT SdiOff2[7];
	UINT SdiGain2[7];
}DI_FW_REG_STRUCT;

/*******************************************************************
 * FUNCTION: Configure Deint Core.
 * INPUT:   None
 * RETURN:  None
 *******************************************************************/
void ConfigDeintCore(void);

/*******************************************************************
 * FUNCTION: Configure FMP Enable For chroma
 * INPUT: uiEnable    -   Enable/Disable FMP enable
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_FmdFmpEnableChroma(UINT32 uiEnable);

/*******************************************************************
 * FUNCTION: Configure FilmMode Enable in Hardware
 * INPUT: uiEnable    -   Enable/Disable FilmMode detection
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_FmdHwEnable(UINT32 uiEnable);

/*******************************************************************
 * FUNCTION: Set BitDepth of MADI to 8/10 bit.
 * INPUT: BitDepth  -	8/10 bit input depth
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_MadiBitDepthIn(UINT32 BitDepth);

/*******************************************************************
 * FUNCTION: Enable NR control in MADI.
 * INPUT: uiEnable    -   Enable/Disable
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_MadiNrCtrl(UINT32 uiEnable);

/*******************************************************************
 * FUNCTION: Configure DI output as full DI or interlaced data.
 * INPUT: uiEnable    -   Enable/Disable
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_MadiDiOutCtrl(UINT32 uiEnable);

/*******************************************************************
 * FUNCTION: Configure Diagonal bypass for Y and UV in MADI.
 * INPUT: uiBypass    -   Enable/Disable Diagonal bypass
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_MadiDiagonalBypass(UINT32 uiBypass);

#ifdef __cplusplus
}
#endif

#endif	// End of _OVP_DEINT_PARAM_H_
