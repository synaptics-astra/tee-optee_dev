/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

/************* Header file include ********************************************/
#define _OVP_DEINT_API_C_

#include "ovp_module.h"
#include "ovp_common.h"
#include "galois_io.h"
/*******************************************************************
 * FUNCTION: Set BitDepth of MADI to 8/10 bit.
 * INPUT: BitDepth  -	8/10 bit input depth
 * RETURN:  None
 *******************************************************************/

void OVP_DEINT_MadiBitDepthIn(UINT32 BitDepth)
{
	T32MadiReg_YUV8BitIn	stMadiRegYuv8BitIn;
	memset(&stMadiRegYuv8BitIn, 0, sizeof(T32MadiReg_YUV8BitIn));

	if(8 == BitDepth)
		stMadiRegYuv8BitIn.uYUV8BitIn_YUV8BitIn = 1;
	else if(10 == BitDepth)
		stMadiRegYuv8BitIn.uYUV8BitIn_YUV8BitIn = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_YUV8BitIn), stMadiRegYuv8BitIn.u32);
}

/*******************************************************************
 * FUNCTION: Enable NR control in MADI.
 * INPUT: uiEnable    -   Enable/Disable
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_MadiNrCtrl(UINT32 uiEnable)
{
	T32MadiReg_Nr		stMadiRegNr;
	memset(&stMadiRegNr, 0, sizeof(T32MadiReg_Nr));

	stMadiRegNr.uNr_YNr = uiEnable;
	stMadiRegNr.uNr_UVNr = uiEnable;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Nr), stMadiRegNr.u32);
}

/*******************************************************************
 * FUNCTION: Configure DI output as full DI or interlaced data.
 * INPUT: uiEnable    -   Enable/Disable
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_MadiDiOutCtrl(UINT32 uiEnable)
{
	T32MadiReg_IntOutOnly	stMadiRegInoutOnly;
	memset(&stMadiRegInoutOnly, 0, sizeof(T32MadiReg_IntOutOnly));

	stMadiRegInoutOnly.uIntOutOnly_IntOutOnly = uiEnable;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_IntOutOnly), stMadiRegInoutOnly.u32);
}

/*******************************************************************
 * FUNCTION: Configure Diagonal bypass for Y and UV in MADI.
 * INPUT: uiBypass    -   Enable/Disable Diagonal bypass
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_MadiDiagonalBypass(UINT32 uiBypass)
{
	T32MadiReg_DiagByps	stMadiRegDiagBypass;
	memset(&stMadiRegDiagBypass, 0, sizeof(T32MadiReg_DiagByps));

	stMadiRegDiagBypass.uDiagByps_YDiagByps = uiBypass;
	stMadiRegDiagBypass.uDiagByps_UVDiagByps = uiBypass;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_DiagByps), stMadiRegDiagBypass.u32);
}

/*******************************************************************
 * FUNCTION: Configure MADI Engine.
 * INPUT:   None
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_ConfigMadi(void)
{
	T32MadiReg_Edge0	stMadiRegEdge0;
	T32MadiReg_Edge1	stMadiRegEdge1;
	T32MadiReg_Cnst		stMadiRegCnst;
	T32MadiReg_MdvCnst	stMadiRegMdvCnst;
	T32MadiReg_LamdNoise	stMadiRegLamdNoise;
	T32MadiReg_LasdNoise	stMadiRegLasdNoise;
	T32MadiReg_LAMD		stMadiRegLAMD;
	T32MadiReg_LASD		stMadiRegLASD;
	T32MadiReg_SOSD0	stMadiRegSOSD0;
	T32MadiReg_SOSD1	stMadiRegSOSD1;
	T32MadiReg_LAMDGain	stMadiRegLamdGain;
	T32MadiReg_LASDGain	stMadiRegLasdGain;
	T32MadiReg_Hf		stMadiRegHf;
	T32MadiReg_Rpd		stMadiRegRpd;
	T32MadiReg_SdiSad	stMadiRegSdiSad;
	T32MadiReg_SdiNoise	stMadiRegSdiNoise;
	T32MadiReg_Sdi0		stMadiRegSdi0;
	T32MadiReg_Sdi1		stMadiRegSdi1;
	T32MadiReg_Sdi2		stMadiRegSdi2;
	T32MadiReg_Sdi3		stMadiRegSdi3;
	T32MadiReg_Sdi4		stMadiRegSdi4;
	T32MadiReg_Sdi5		stMadiRegSdi5;
	T32MadiReg_Sdi6		stMadiRegSdi6;
	T32MadiReg_ShiftBit	stMadiRegShiftBit;
	T32MadiReg_Angle	stMadiRegAngle;
	T32MadiReg_MotDet	stMadiRegMotDet;
	T32MadiReg_HVLine	stMadiRegHvLine;
	T32MadiReg_MdvYiir	stMadiRegMdvYiir;
	T32MadiReg_C420A	stMadiRegC420A;
	T32MadiReg_C420B	stMadiRegC420B;
	T32MadiReg_MdvFilterSel stMadiRegMdvFilterSel;
	T32MadiReg_LcnstGain	stMadiRegLcnstGain;
	T32MadiReg_YUVBlendGain stMadiRegYuvBlendGain;
	T32MadiReg_YUVMdvBlendGain  stMadiRegYuvMdvBlendGain;
	T32MadiReg_YMdvGain	stMadiRegYMdvGain;
	T32MadiReg_YMdvGain1	stMadiRegYMdvGain1;
	T32MadiReg_YMdvOffset	stMadiRegYMdvOffset;
	T32MadiReg_UVMdvGain	stMadiRegMdvGain;
	T32MadiReg_Impulse	stMadiRegImpulse;
	T32MadiReg_MdvLcnst0	stMadiRegMdvLcnst0;
	T32MadiReg_MdvLcnst1	stMadiRegMdvLcnst1;
	T32MadiReg_MdvLcnst2	stMadiRegMdvLcnst2;
	T32MadiReg_MdvLcnst3	stMadiRegMdvLcnst3;
	T32MadiReg_VTFCoef0	stMadiRegVtfCoef0;
	T32MadiReg_VTFCoef1	stMadiRegVtfCoef1;
	T32MadiReg_VTFCoef2	stMadiRegVtfCoef2;
	T32MadiReg_C420VTFCoef0 stMadiRegC420VtfCoef0;
	T32MadiReg_C420VTFCoef1 stMadiRegC420VtfCoef1;
	T32MadiReg_C420VTFCoef2 stMadiRegC420VtfCoef2;
	T32MadiReg_C420Mode	stMadiRegC420Mode;
	T32MadiReg_ChromaMdvUseOnly stMadiRegChromaMdvUseOnly;
	T32MadiReg_MdvMin	stMadiRegMdvMin;
	T32MadiReg_GAccShiftBits    stMadiRegGaccShiftBits;

	memset(&stMadiRegEdge0, 0, sizeof(T32MadiReg_Edge0));
	memset(&stMadiRegEdge1, 0, sizeof(T32MadiReg_Edge1));
	memset(&stMadiRegCnst, 0, sizeof(T32MadiReg_Cnst));
	memset(&stMadiRegMdvCnst, 0, sizeof(T32MadiReg_MdvCnst));
	memset(&stMadiRegLamdNoise, 0, sizeof(T32MadiReg_LamdNoise));
	memset(&stMadiRegLasdNoise, 0, sizeof(T32MadiReg_LasdNoise));
	memset(&stMadiRegLAMD, 0, sizeof(T32MadiReg_LAMD));
	memset(&stMadiRegLASD, 0, sizeof(T32MadiReg_LASD));
	memset(&stMadiRegSOSD0, 0, sizeof(T32MadiReg_SOSD0));
	memset(&stMadiRegSOSD1, 0, sizeof(T32MadiReg_SOSD1));
	memset(&stMadiRegLamdGain, 0, sizeof(T32MadiReg_LAMDGain));
	memset(&stMadiRegLasdGain, 0, sizeof(T32MadiReg_LASDGain));
	memset(&stMadiRegHf, 0, sizeof(T32MadiReg_Hf));
	memset(&stMadiRegRpd, 0, sizeof(T32MadiReg_Rpd));
	memset(&stMadiRegSdiSad, 0, sizeof(T32MadiReg_SdiSad));
	memset(&stMadiRegSdiNoise, 0, sizeof(T32MadiReg_SdiNoise));
	memset(&stMadiRegSdi0, 0, sizeof(T32MadiReg_Sdi0));
	memset(&stMadiRegSdi1, 0, sizeof(T32MadiReg_Sdi1));
	memset(&stMadiRegSdi2, 0, sizeof(T32MadiReg_Sdi2));
	memset(&stMadiRegSdi3, 0, sizeof(T32MadiReg_Sdi3));
	memset(&stMadiRegSdi4, 0, sizeof(T32MadiReg_Sdi4));
	memset(&stMadiRegSdi5, 0, sizeof(T32MadiReg_Sdi5));
	memset(&stMadiRegSdi6, 0, sizeof(T32MadiReg_Sdi6));
	memset(&stMadiRegShiftBit, 0, sizeof(T32MadiReg_ShiftBit));
	memset(&stMadiRegAngle, 0, sizeof(T32MadiReg_Angle));
	memset(&stMadiRegMotDet, 0, sizeof(T32MadiReg_MotDet));
	memset(&stMadiRegHvLine, 0, sizeof(T32MadiReg_HVLine));
	memset(&stMadiRegMdvYiir, 0, sizeof(T32MadiReg_MdvYiir));
	memset(&stMadiRegC420A, 0, sizeof(T32MadiReg_C420A));
	memset(&stMadiRegC420B, 0, sizeof(T32MadiReg_C420B));
	memset(&stMadiRegMdvFilterSel, 0, sizeof(T32MadiReg_MdvFilterSel));
	memset(&stMadiRegLcnstGain, 0, sizeof(T32MadiReg_LcnstGain));
	memset(&stMadiRegYuvBlendGain, 0, sizeof(T32MadiReg_YUVBlendGain));
	memset(&stMadiRegYuvMdvBlendGain, 0, sizeof(T32MadiReg_YUVMdvBlendGain));
	memset(&stMadiRegYMdvGain, 0, sizeof(T32MadiReg_YMdvGain));
	memset(&stMadiRegYMdvGain1, 0, sizeof(T32MadiReg_YMdvGain1));
	memset(&stMadiRegYMdvOffset, 0, sizeof(T32MadiReg_YMdvOffset));
	memset(&stMadiRegMdvGain, 0, sizeof(T32MadiReg_UVMdvGain));
	memset(&stMadiRegImpulse, 0, sizeof(T32MadiReg_Impulse));
	memset(&stMadiRegMdvLcnst0, 0, sizeof(T32MadiReg_MdvLcnst0));
	memset(&stMadiRegMdvLcnst1, 0, sizeof(T32MadiReg_MdvLcnst1));
	memset(&stMadiRegMdvLcnst2, 0, sizeof(T32MadiReg_MdvLcnst2));
	memset(&stMadiRegMdvLcnst3, 0, sizeof(T32MadiReg_MdvLcnst3));
	memset(&stMadiRegVtfCoef0, 0, sizeof(T32MadiReg_VTFCoef0));
	memset(&stMadiRegVtfCoef1, 0, sizeof(T32MadiReg_VTFCoef1));
	memset(&stMadiRegVtfCoef2, 0, sizeof(T32MadiReg_VTFCoef2));
	memset(&stMadiRegC420VtfCoef0, 0, sizeof(T32MadiReg_C420VTFCoef0));
	memset(&stMadiRegC420VtfCoef1, 0, sizeof(T32MadiReg_C420VTFCoef1));
	memset(&stMadiRegC420VtfCoef2, 0, sizeof(T32MadiReg_C420VTFCoef2));
	memset(&stMadiRegC420Mode, 0, sizeof(T32MadiReg_C420Mode));
	memset(&stMadiRegChromaMdvUseOnly, 0, sizeof(T32MadiReg_ChromaMdvUseOnly));
	memset(&stMadiRegMdvMin, 0, sizeof(T32MadiReg_MdvMin));
	memset(&stMadiRegGaccShiftBits, 0, sizeof(T32MadiReg_GAccShiftBits));

	stMadiRegEdge0.uEdge0_EdgeGain_0 = 0x9;
	stMadiRegEdge0.uEdge0_EdgeGain_1 = 0x10;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Edge0), stMadiRegEdge0.u32);

	stMadiRegEdge1.uEdge1_EdgeThr_0 = 0x18;
	stMadiRegEdge1.uEdge1_EdgeThr_1 = 0x46;
	stMadiRegEdge1.uEdge1_EdgeThr_2 = 0x2;
	stMadiRegEdge1.uEdge1_EdgeThr_3 = 0x5;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Edge1), stMadiRegEdge1.u32);

	stMadiRegCnst.uCnst_CnstThr_0 = 0x1E;
	stMadiRegCnst.uCnst_CnstThr_1 = 0x8C;
	stMadiRegCnst.uCnst_CnstThr_2 = 0x8;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Cnst), stMadiRegCnst.u32);

	stMadiRegMdvCnst.uMdvCnst_MdvCnstThr = 0x28;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MdvCnst), stMadiRegMdvCnst.u32);

	stMadiRegLamdNoise.uLamdNoise_LamdNoiseThr_0 = 0x2;
	stMadiRegLamdNoise.uLamdNoise_LamdNoiseThr_1 = 0x6;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_LamdNoise), stMadiRegLamdNoise.u32);

	stMadiRegLasdNoise.uLasdNoise_LasdNoiseThr_0 = 0x4;
	stMadiRegLasdNoise.uLasdNoise_LasdNoiseThr_1 = 0xC;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_LasdNoise), stMadiRegLasdNoise.u32);

	stMadiRegLAMD.uLAMD_LAMDThr_0 = 0xC;
	stMadiRegLAMD.uLAMD_LAMDThr_1 = 0x6;
	stMadiRegLAMD.uLAMD_LAMDThr_2 = 0x15;
	stMadiRegLAMD.uLAMD_LAMDEnhThr_0 = 0;
	stMadiRegLAMD.uLAMD_LAMDEnhThr_1 = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_LAMD), stMadiRegLAMD.u32);

	stMadiRegLASD.uLASD_LASDThr_0 = 0x2;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_LASD), stMadiRegLASD.u32);
	stMadiRegSOSD0.uSOSD0_SOSDThr_0 = 0x96;
	stMadiRegSOSD0.uSOSD0_SOSDThr_1 = 0x8;
	stMadiRegSOSD0.uSOSD0_SOSDThr_2 = 0x6;
	stMadiRegSOSD0.uSOSD0_SOSDThr_3 = 0x78;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_SOSD0), stMadiRegSOSD0.u32);

	stMadiRegSOSD1.uSOSD1_SOSDThr_4 = 0x40;
	stMadiRegSOSD1.uSOSD1_SOSDThr_5 = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_SOSD1), stMadiRegSOSD1.u32);

	stMadiRegLamdGain.uLAMDGain_LAMDGain_0 = 0x4C;
	stMadiRegLamdGain.uLAMDGain_LAMDGain_1 = 0x30;
	stMadiRegLamdGain.uLAMDGain_LAMDGain_2 = 0xFF;
	stMadiRegLamdGain.uLAMDGain_LAMDGain_3 = 0x0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_LAMDGain), stMadiRegLamdGain.u32);

	stMadiRegLasdGain.uLASDGain_LASDGain_0 = 0x28;
	stMadiRegLasdGain.uLASDGain_LASDGain_1 = 0x14;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_LASDGain), stMadiRegLasdGain.u32);

	stMadiRegHf.uHf_HfThr_0 = 0x3;
	stMadiRegHf.uHf_HfThr_1 = 0x12;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Hf), stMadiRegHf.u32);

	stMadiRegRpd.uRpd_RpdThr_0 = 0x2;
	stMadiRegRpd.uRpd_RpdThr_1 = 0xF;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Rpd), stMadiRegRpd.u32);

	stMadiRegSdiSad.uSdiSad_SdiSadThr_0 = 0x26;
	stMadiRegSdiSad.uSdiSad_SdiSadThr_1 = 0x20;
	stMadiRegSdiSad.uSdiSad_SdiSadThr_2 = 0x22;
	stMadiRegSdiSad.uSdiSad_SdiSadThr_3 = 0xAA;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_SdiSad), stMadiRegSdiSad.u32);


	stMadiRegSdiNoise.uSdiNoise_SdiNoiseThr = 0x10;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_SdiNoise), stMadiRegSdiNoise.u32);

	stMadiRegSdi0.uSdi0_SdiAbsAngFilterThr = 0x6;
	stMadiRegSdi0.uSdi0_SdiIntClamp = 0xc;
	stMadiRegSdi0.uSdi0_SdiAbsAngConfThr = 0x1;
	stMadiRegSdi0.uSdi0_SdiAbsAngThr = 0x2;
	stMadiRegSdi0.uSdi0_SdiAngStretchThr = 0;
	stMadiRegSdi0.uSdi0_SdiHighAngConfValue_0 = 0x2;
	stMadiRegSdi0.uSdi0_SdiHighAngConfValue_1 = 0x3;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Sdi0), stMadiRegSdi0.u32);

	stMadiRegSdi1.uSdi1_SdiAngConfBlendGain_0 = 0x10;
	stMadiRegSdi1.uSdi1_SdiAngConfBlendGain_1 = 0xE;
	stMadiRegSdi1.uSdi1_SdiAngConfBlendGain_2 = 0xC;
	stMadiRegSdi1.uSdi1_SdiAngConfBlendGain_3 = 0x9;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Sdi1), stMadiRegSdi1.u32);

	stMadiRegSdi2.uSdi2_SdiAngConfBlendGain_4 = 0x7;
	stMadiRegSdi2.uSdi2_SdiAngConfBlendGain_5 = 0x3;
	stMadiRegSdi2.uSdi2_SdiAngConfBlendGain_6 = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Sdi2), stMadiRegSdi2.u32);

	stMadiRegShiftBit.uShiftBit_BlamdShiftBit = 0x2;
	stMadiRegShiftBit.uShiftBit_BlasdShiftBit = 0x2;
	stMadiRegShiftBit.uShiftBit_BsosdShiftBit = 0x2;
	stMadiRegShiftBit.uShiftBit_BlmhfShiftBit = 0x2;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_ShiftBit), stMadiRegShiftBit.u32);

	stMadiRegSdi3.uSdi3_SdiScaleFactor_0 = 0x8;
	stMadiRegSdi3.uSdi3_SdiScaleFactor_1 = 0xA;
	stMadiRegSdi3.uSdi3_SdiScaleFactor_2 = 0xC;
	stMadiRegSdi3.uSdi3_SdiScaleFactor_3 = 0xE;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Sdi3), stMadiRegSdi3.u32);

	stMadiRegSdi4.uSdi4_SdiIntDifThr_0 = 0x136;
	stMadiRegSdi4.uSdi4_SdiIntDifThr_1 = 0x190;
	stMadiRegSdi4.uSdi4_SdiIntDifThr_2 = 0x136;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Sdi4), stMadiRegSdi4.u32);

	stMadiRegSdi5.uSdi5_SdiIntDifThr_3 = 0x168;
	stMadiRegSdi5.uSdi5_SdiIntDifThr_4 = 0x136;
	stMadiRegSdi5.uSdi5_SdiIntDifThr_5 = 0x190;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Sdi5), stMadiRegSdi5.u32);

	stMadiRegSdi6.uSdi6_SdiMdvThr_0 = 0x3;
	stMadiRegSdi6.uSdi6_SdiMdvThr_1 = 0x6;
	stMadiRegSdi6.uSdi6_SdiLowSadThr = 0x20;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Sdi6), stMadiRegSdi6.u32);

	stMadiRegAngle.uAngle_AngConfEn = 0x1;
	stMadiRegAngle.uAngle_AngConfUser = 0x4;
	stMadiRegAngle.uAngle_AngCnstEn = 0x1;
	stMadiRegAngle.uAngle_AngCnstUser = 0x2;
	stMadiRegAngle.uAngle_AngEdgeEn = 0x1;
	stMadiRegAngle.uAngle_AngEdgeUser = 0x1;
	stMadiRegAngle.uAngle_AngleEn = 0x1;
	stMadiRegAngle.uAngle_AngleUser = 0x6;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Angle), stMadiRegAngle.u32);

	stMadiRegMotDet.uMotDet_LAMDEn = 0x1;
	stMadiRegMotDet.uMotDet_LAMDUser = 0x3;
	stMadiRegMotDet.uMotDet_LASDEn = 0x1;
	stMadiRegMotDet.uMotDet_LASDUser = 0x0;
	stMadiRegMotDet.uMotDet_LASDSel = 0;
	stMadiRegMotDet.uMotDet_SOSDEn = 0x1;
	stMadiRegMotDet.uMotDet_SOSDUser = 0;
	stMadiRegMotDet.uMotDet_SOSDSel = 0;
	stMadiRegMotDet.uMotDet_MDVEn = 0x1;
	stMadiRegMotDet.uMotDet_MDVUser_0 = 0x4;
	stMadiRegMotDet.uMotDet_MDVUser_1 = 0x4;
	stMadiRegMotDet.uMotDet_HVLineEn = 0x1;
	stMadiRegMotDet.uMotDet_HVLineUser= 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MotDet), stMadiRegMotDet.u32);

	stMadiRegHvLine.uHVLine_HVLineGain_0 = 0x24;
	stMadiRegHvLine.uHVLine_HVLineGain_1 = 0x18;
	stMadiRegHvLine.uHVLine_HVLineThr_0 = 0xE;
	stMadiRegHvLine.uHVLine_HVLineThr_1 = 0xE;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_HVLine), stMadiRegHvLine.u32);

	stMadiRegMdvYiir.uMdvYiir_MdvYiirGain_0 = 0x4;
	stMadiRegMdvYiir.uMdvYiir_MdvYiirGain_1 = 0xF;
	stMadiRegMdvYiir.uMdvYiir_MdvYiirClamp = 0xF;
	stMadiRegMdvYiir.uMdvYiir_MdvYiirEn = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MdvYiir), stMadiRegMdvYiir.u32);

	stMadiRegC420A.uC420A_EThr = 0x60;
	stMadiRegC420A.uC420A_C420StNum = 0x3;
	stMadiRegC420A.uC420A_C420AccShiftBits = 0x9;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_C420A), stMadiRegC420A.u32);

	stMadiRegC420B.uC420B_MThr_0 = 0xF;
	stMadiRegC420B.uC420B_MThr_1 = 0x10;
	stMadiRegC420B.uC420B_MThr_2 = 0x3;
	stMadiRegC420B.uC420B_MdvThr = 0x4;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_C420B), stMadiRegC420B.u32);

	stMadiRegMdvFilterSel.uMdvFilterSel_MdvFilterSel_0 = 1;
	stMadiRegMdvFilterSel.uMdvFilterSel_MdvFilterSel_1 = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MdvFilterSel), stMadiRegMdvFilterSel.u32);

	stMadiRegLcnstGain.uLcnstGain_LcnstGain_0 = 0x40;
	stMadiRegLcnstGain.uLcnstGain_LcnstGain_1 = 0xC;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_LcnstGain), stMadiRegLcnstGain.u32);

	stMadiRegYuvBlendGain.uYUVBlendGain_YUVBlendGain_0 = 0x80;
	stMadiRegYuvBlendGain.uYUVBlendGain_YUVBlendGain_1 = 0x80;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_YUVBlendGain), stMadiRegYuvBlendGain.u32);

	stMadiRegYuvMdvBlendGain.uYUVMdvBlendGain_YUVMdvBlendGain_0 = 0x100;
	stMadiRegYuvMdvBlendGain.uYUVMdvBlendGain_YUVMdvBlendGain_1 = 0x100;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_YUVMdvBlendGain),  stMadiRegYuvMdvBlendGain.u32);

	stMadiRegYMdvGain.uYMdvGain_YMdvGain_0 = 0x40;
	stMadiRegYMdvGain.uYMdvGain_YMdvGain_1 = 0x10;
	stMadiRegYMdvGain.uYMdvGain_YMdvGain_2 = 0x1C;
	stMadiRegYMdvGain.uYMdvGain_YMdvGain_3 = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_YMdvGain), stMadiRegYMdvGain.u32);

	stMadiRegYMdvGain1.uYMdvGain1_YMdvGain_4 = 0xD;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_YMdvGain1), stMadiRegYMdvGain1.u32);

	stMadiRegYMdvOffset.uYMdvOffset_YMdvOffset_0 = 0x32;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_YMdvOffset), stMadiRegYMdvOffset.u32);

	stMadiRegMdvGain.uUVMdvGain_UVMdvGain = 0xF;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_UVMdvGain), stMadiRegMdvGain.u32);

	stMadiRegImpulse.uImpulse_ImpulseEn = 0x1;
	stMadiRegImpulse.uImpulse_ImpulseThr = 0x80;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Impulse), stMadiRegImpulse.u32);

	stMadiRegMdvLcnst0.uMdvLcnst0_MdvLcnstGain_0 = 0x12C;
	stMadiRegMdvLcnst0.uMdvLcnst0_MdvLcnstGain_1 = 0x50;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MdvLcnst0), stMadiRegMdvLcnst0.u32);

	stMadiRegMdvLcnst1.uMdvLcnst1_MdvLcnstGain_2 = 0x100;
	stMadiRegMdvLcnst1.uMdvLcnst1_MdvLcnstGain_3 = 0x40;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MdvLcnst1), stMadiRegMdvLcnst1.u32);

	stMadiRegMdvLcnst2.uMdvLcnst2_MdvLcnstValue_0 = 0x3;
	stMadiRegMdvLcnst2.uMdvLcnst2_MdvLcnstValue_1 = 0xA;
	stMadiRegMdvLcnst2.uMdvLcnst2_MdvLcnstValue_2 = 0x3;
	stMadiRegMdvLcnst2.uMdvLcnst2_MdvLcnstValue_3 = 0x3;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MdvLcnst2), stMadiRegMdvLcnst2.u32);

	stMadiRegMdvLcnst3.uMdvLcnst3_MdvLcnstEnhSel_0 = 0;
	stMadiRegMdvLcnst3.uMdvLcnst3_MdvLcnstEnhSel_1 = 0;
	stMadiRegMdvLcnst3.uMdvLcnst3_MdvLcnstEnhSel_2 = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MdvLcnst3), stMadiRegMdvLcnst3.u32);

	stMadiRegVtfCoef0.sVTFCoef0_VTFCoef_0 = 0x10;
	stMadiRegVtfCoef0.sVTFCoef0_VTFCoef_1 = 0x1B0;    //Write 2's complement for -ve numbers.
	stMadiRegVtfCoef0.sVTFCoef0_VTFCoef_2 = 0x70;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_VTFCoef0), stMadiRegVtfCoef0.u32);

	stMadiRegVtfCoef1.sVTFCoef1_VTFCoef_3 = 0xA0;
	stMadiRegVtfCoef1.sVTFCoef1_VTFCoef_4 = 0x70;
	stMadiRegVtfCoef1.sVTFCoef1_VTFCoef_5 = 0x1B0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_VTFCoef1), stMadiRegVtfCoef1.u32);

	stMadiRegVtfCoef2.sVTFCoef2_VTFCoef_6 = 16;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_VTFCoef2), stMadiRegVtfCoef2.u32);

	stMadiRegC420VtfCoef0.sC420VTFCoef0_C420VTFCoef_0 = 0;
	stMadiRegC420VtfCoef0.sC420VTFCoef0_C420VTFCoef_1 = 0;
	stMadiRegC420VtfCoef0.sC420VTFCoef0_C420VTFCoef_2 = 64;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_C420VTFCoef0), stMadiRegC420VtfCoef0.u32);

	stMadiRegC420VtfCoef1.sC420VTFCoef1_C420VTFCoef_3 = 128;
	stMadiRegC420VtfCoef1.sC420VTFCoef1_C420VTFCoef_4 = 64;
	stMadiRegC420VtfCoef1.sC420VTFCoef1_C420VTFCoef_5 = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_C420VTFCoef1), stMadiRegC420VtfCoef1.u32);

	stMadiRegC420VtfCoef2.sC420VTFCoef2_C420VTFCoef_6 = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_C420VTFCoef2), stMadiRegC420VtfCoef2.u32);

	stMadiRegC420Mode.uC420Mode_C420InMode = 1;
	stMadiRegC420Mode.uC420Mode_C420OutMode = 1;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_C420Mode), stMadiRegC420Mode.u32);

	stMadiRegChromaMdvUseOnly.uChromaMdvUseOnly_ChromaMdvUseOnly = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_ChromaMdvUseOnly), stMadiRegChromaMdvUseOnly.u32);

	stMadiRegMdvMin.uMdvMin_MdvMinSt_0 = 0x20;
	stMadiRegMdvMin.uMdvMin_MdvMinSt_1 = 0x10;
	stMadiRegMdvMin.uMdvMin_MdvMinThr_0 = 0xF;
	stMadiRegMdvMin.uMdvMin_MdvMinThr_1 = 0x80;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_MdvMin), stMadiRegMdvMin.u32);

	stMadiRegGaccShiftBits.uGAccShiftBits_GhfAccShiftBits = 1;
	stMadiRegGaccShiftBits.uGAccShiftBits_GosdAccShiftBits = 0;
	stMadiRegGaccShiftBits.uGAccShiftBits_GrpdAccShiftBits = 0;
	stMadiRegGaccShiftBits.uGAccShiftBits_GdiAccShiftBits = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_GAccShiftBits), stMadiRegGaccShiftBits.u32);

	//Fix Me: Satya.....
	OVP_DEINT_MadiDiagonalBypass(0);
	OVP_DEINT_MadiDiOutCtrl(0);
	OVP_DEINT_MadiNrCtrl(0);
	OVP_DEINT_MadiBitDepthIn(8);
}

/*******************************************************************
 * FUNCTION: Configure FilmMode Enable in Hardware
 * INPUT: uiEnable    -   Enable/Disable FilmMode detection
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_FmdHwEnable(UINT32 uiEnable)
{
	T32FmdReg_FmdHWEn	stFmdHWEn;

	stFmdHWEn.uFmdHWEn_FmdHWEn = uiEnable;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdHWEn), stFmdHWEn.u32);
}

/*******************************************************************
 * FUNCTION: Configure FMP Enable For chroma
 * INPUT: uiEnable    -   Enable/Disable FMP enable
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_FmdFmpEnableChroma(UINT32 uiEnable)
{
	T32FmdReg_ChromaUseMadiOnly stFmdChromaUseMadiOnly;

	stFmdChromaUseMadiOnly.uChromaUseMadiOnly_ChromaUseMadiOnly = uiEnable;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_ChromaUseMadiOnly), stFmdChromaUseMadiOnly.u32);
}

/*******************************************************************
 * FUNCTION: Configure Film Mode Detection Engine.
 * INPUT:   None
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_ConfigFmd(void)
{
	T32FmdReg_FieldDifThr stFmdRegFieldDifThr;
	T32FmdReg_FmdHmeGain0	stFmdHmeGain0;
	T32FmdReg_FmdHmeGain1	stFmdHmeGain1;
	T32FmdReg_FmdHmeOff0	stFmdHmeOff0;
	T32FmdReg_FmdHmeOff1	stFmdHmeOff1;
	T32FmdReg_FmdHmeThr	stFmdHmeThr;
	//T32FmdReg_FmdGmdvShiftBit   stFmdGmdvShiftBit;
	T32FmdReg_FmdScrTextFixLineDly0 stFmdScrTextFixLineDly0;
	T32FmdReg_FmdScrTextFixLineDly1 stFmdScrTextFixLineDly1;
	T32FmdReg_Fmdhmvoff	stFmdhmvoff;
	T32FmdReg_FmdLmvThr	stFmdLmvThr;
	T32FmdReg_Fmdhmvshiftbits   stFmdhmvshiftbits;
	T32FmdReg_FmdhmeThr0	stFmdhmeThr0;
	T32FmdReg_FmdhmeThr1	stFmdhmeThr1;
	T32FmdReg_FmdhmeThr2	stFmdhmeThr2;
	T32FmdReg_FmdmdvThr0	stFmdmdvThr0;
	T32FmdReg_FmdmdvThr1	stFmdmdvThr1;

	memset(&stFmdRegFieldDifThr, 0, sizeof(T32FmdReg_FieldDifThr));
	memset(&stFmdHmeGain0, 0, sizeof(T32FmdReg_FmdHmeGain0));
	memset(&stFmdHmeGain1, 0, sizeof(T32FmdReg_FmdHmeGain1));
	memset(&stFmdHmeOff0, 0, sizeof(T32FmdReg_FmdHmeOff0));
	memset(&stFmdHmeOff1, 0, sizeof(T32FmdReg_FmdHmeOff1));
	memset(&stFmdHmeThr, 0, sizeof(T32FmdReg_FmdHmeThr));
	memset(&stFmdScrTextFixLineDly0, 0, sizeof(T32FmdReg_FmdScrTextFixLineDly0));
	memset(&stFmdScrTextFixLineDly1, 0, sizeof(T32FmdReg_FmdScrTextFixLineDly1));
	memset(&stFmdhmvoff, 0, sizeof(T32FmdReg_Fmdhmvoff));
	memset(&stFmdLmvThr, 0, sizeof(T32FmdReg_FmdLmvThr));
	memset(&stFmdhmvshiftbits, 0, sizeof(T32FmdReg_Fmdhmvshiftbits));
	memset(&stFmdhmeThr0, 0, sizeof(T32FmdReg_FmdhmeThr0));
	memset(&stFmdhmeThr1, 0, sizeof(T32FmdReg_FmdhmeThr1));
	memset(&stFmdhmeThr2, 0, sizeof(T32FmdReg_FmdhmeThr2));
	memset(&stFmdmdvThr0, 0, sizeof(T32FmdReg_FmdmdvThr0));
	memset(&stFmdmdvThr1, 0, sizeof(T32FmdReg_FmdmdvThr1));

	stFmdRegFieldDifThr.uFieldDifThr_FieldDifThr_0 = 0x2D;
	stFmdRegFieldDifThr.uFieldDifThr_FieldDifThr_1 = 0x3C;
	stFmdRegFieldDifThr.uFieldDifThr_FieldDifThr_2 = 0x2D;
	stFmdRegFieldDifThr.uFieldDifThr_FieldDifThr_3 = 0x3C;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FieldDifThr), stFmdRegFieldDifThr.u32);

	stFmdHmeGain0.uFmdHmeGain0_FmdHmeGain_0 = 0x46;
	stFmdHmeGain0.uFmdHmeGain0_FmdHmeGain_1 = 0x45;
	stFmdHmeGain0.uFmdHmeGain0_FmdHmeGain_2 = 0x44;
	stFmdHmeGain0.uFmdHmeGain0_FmdHmeGain_3 = 0x43;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdHmeGain0), stFmdHmeGain0.u32);

	stFmdHmeGain1.uFmdHmeGain1_FmdHmeGain_4 = 0x42;
	stFmdHmeGain1.uFmdHmeGain1_FmdHmeGain_5 = 0x41;
	stFmdHmeGain1.uFmdHmeGain1_FmdHmeGain_6 = 0x3D;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdHmeGain1), stFmdHmeGain1.u32);

	stFmdHmeOff0.uFmdHmeOff0_FmdHmeOff_0 = 0;
	stFmdHmeOff0.uFmdHmeOff0_FmdHmeOff_1 = 0;
	stFmdHmeOff0.uFmdHmeOff0_FmdHmeOff_2 = 0;
	stFmdHmeOff0.uFmdHmeOff0_FmdHmeOff_3 = 0;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdHmeOff0), stFmdHmeOff0.u32);

	stFmdHmeOff1.uFmdHmeOff1_FmdHmeOff_4 = 0;
	stFmdHmeOff1.uFmdHmeOff1_FmdHmeOff_5 = 0;
	stFmdHmeOff1.uFmdHmeOff1_FmdHmeOff_6 = 0;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdHmeOff1), stFmdHmeOff1.u32);

	stFmdHmeThr.uFmdHmeThr_FmdHmeNoiseThr = 0x8;
	stFmdHmeThr.uFmdHmeThr_FmdHmeDifThr = 0x2;
	stFmdHmeThr.uFmdHmeThr_FmdHmeLowCnstThr = 0x11;
	stFmdHmeThr.uFmdHmeThr_FmdStillThr = 0xD;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdHmeThr), stFmdHmeThr.u32);

	stFmdScrTextFixLineDly0.uFmdScrTextFixLineDly0_FmdScrTextFixLineDly_0 = 0x6;
	stFmdScrTextFixLineDly0.uFmdScrTextFixLineDly0_FmdScrTextFixLineDly_1 = 0x172;
	stFmdScrTextFixLineDly0.uFmdScrTextFixLineDly0_FmdScrTextFixLineDly_2 = 0x17C;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdScrTextFixLineDly0), stFmdScrTextFixLineDly0.u32);

	stFmdScrTextFixLineDly1.uFmdScrTextFixLineDly1_FmdScrTextFixLineDly_3 = 0x320;
	stFmdScrTextFixLineDly1.uFmdScrTextFixLineDly1_FmdScrTextFixLineDly_4 = 0x46;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdScrTextFixLineDly1), stFmdScrTextFixLineDly1.u32);

	stFmdhmvoff.uFmdhmvoff_Fmdhmvoff = 0x32;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_Fmdhmvoff), stFmdhmvoff.u32);

	stFmdLmvThr.uFmdLmvThr_FmdLmvThr = 0x5;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdLmvThr), stFmdLmvThr.u32);

	stFmdhmvshiftbits.uFmdhmvshiftbits_Fmdhmvshiftbits_0 = 0x6;
	stFmdhmvshiftbits.uFmdhmvshiftbits_Fmdhmvshiftbits_1 = 0x4;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_Fmdhmvshiftbits), stFmdhmvshiftbits.u32);

	stFmdhmeThr0.uFmdhmeThr0_FmdhmvThr_0 = 0x14;
	stFmdhmeThr0.uFmdhmeThr0_FmdhmvThr_1 = 0x1E;
	stFmdhmeThr0.uFmdhmeThr0_FmdhmvThr_2 = 0xA;
	stFmdhmeThr0.uFmdhmeThr0_FmdhmvThr_3 = 0;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdhmeThr0), stFmdhmeThr0.u32);

	stFmdhmeThr1.uFmdhmeThr1_FmdhmvThr_4 = 0x8;
	stFmdhmeThr1.uFmdhmeThr1_FmdhmvThr_5 = 0xA;
	stFmdhmeThr1.uFmdhmeThr1_FmdhmvThr_6 = 0x16;
	stFmdhmeThr1.uFmdhmeThr1_FmdhmvThr_7 = 0x16;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdhmeThr1), stFmdhmeThr1.u32);

	stFmdhmeThr2.uFmdhmeThr2_FmdhmvThr_8 = 0xC;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdhmeThr2), stFmdhmeThr2.u32);

	stFmdmdvThr0.uFmdmdvThr0_FmdmdvThr_0 = 0x14;
	stFmdmdvThr0.uFmdmdvThr0_FmdmdvThr_1 = 0x19;
	stFmdmdvThr0.uFmdmdvThr0_FmdmdvThr_2 = 0x28;
	stFmdmdvThr0.uFmdmdvThr0_FmdmdvThr_3 = 0x28;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdmdvThr0), stFmdmdvThr0.u32);

	stFmdmdvThr1.uFmdmdvThr1_FmdmdvThr_4 = 0x28;
	stFmdmdvThr1.uFmdmdvThr1_FmdmdvThr_5 = 0x28;
	stFmdmdvThr1.uFmdmdvThr1_FmdmdvThr_6 = 0x14;
	stFmdmdvThr1.uFmdmdvThr1_FmdmdvThr_7 = 0x19;

	GA_REG_WORD32_WRITE(FMD_REG(RA_FmdReg_FmdmdvThr1), stFmdmdvThr1.u32);

	OVP_DEINT_FmdHwEnable(1);
	OVP_DEINT_FmdFmpEnableChroma(0);    //0-On, 1 - Off
}

/*******************************************************************
 * FUNCTION: Configure Deint Core.
 * INPUT:   None
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_ConfigDeintCore(void)
{
	OVP_DEINT_ConfigMadi();
	OVP_DEINT_ConfigFmd();
}

/*******************************************************************************
Function:    diFwReset
Arguments:   None
Returns:     None
Description: When RST signal is received put FW in KNOWN State
 ********************************************************************************/
void OVP_DEINT_FwInit(OVP_OBJ *pstOvpObj)
{
	int j, i;

	//FW Reg
	pstOvpObj->m_stDiFwIData.ghf =0;
	pstOvpObj->m_stDiFwIData.gmdv =1;
	pstOvpObj->m_stDiFwIData.NLE =0;

	for(i=0; i<2; i++)
		pstOvpObj->m_stDiFwIData.AngHsgrmThr[i] = 0;

	for(i=0; i<3; i++)
		pstOvpObj->m_stDiFwIData.IntValueThr[i] = 0;

	for(i=0; i<4; i++) {
		pstOvpObj->m_stDiFwIData.YMdvThr[i] = 0;
		pstOvpObj->m_stDiFwIData.UVMdvThr[i] = 0;
	}

	for (j=0; j<16; j++)
		for (i=0; i<16; i++) {
			pstOvpObj->m_stDiFwIData.blamd[j][i] = 0;
			pstOvpObj->m_stDiFwIData.blasd[j][i] = 0;
			pstOvpObj->m_stDiFwIData.bsosd[j][i] = 0;
			pstOvpObj->m_stDiFwIData.blmhf[j][i] = 0;
		}
}

/*******************************************************************************
Function:    diFwPowerOnReset
Arguments:   None
Returns:     None
Description: Initialize all parameters to default values.
 ********************************************************************************/
void OVP_DEINT_FwPowerOnReset(OVP_OBJ *pstOvpObj)
{
	int j, i;

	pstOvpObj->m_stDiFwReg.Reg1 = 0;

#ifndef FRAMEBASESEL
	pstOvpObj->m_stDiFwIData.FrameReset = 2;
#else
	pstOvpObj->m_stDiFwIData.FrameReset = 2;
#endif
	pstOvpObj->m_stDiFwIData.fmd = 0;
	pstOvpObj->m_stDiFwIData.ghf = 0;
	pstOvpObj->m_stDiFwIData.gdif = 0;
	pstOvpObj->m_stDiFwIData.glmv = 1;
	pstOvpObj->m_stDiFwIData.ghmv = 0;
	pstOvpObj->m_stDiFwIData.gmdv = 1;
	pstOvpObj->m_stDiFwIData.gmin = 0;
	pstOvpObj->m_stDiFwIData.grpd = 0;
	pstOvpObj->m_stDiFwIData.gosd = 0;
	pstOvpObj->m_stDiFwIData.glamd = 0;
	pstOvpObj->m_stDiFwIData.Fmd_still = 0;
	pstOvpObj->m_stDiFwIData.ChromaUpSampleError = 0;
	pstOvpObj->m_stDiFwReg.C420FixIirFilterEn = 0;
	pstOvpObj->m_stDiFwReg.CUEid = 0;
	pstOvpObj->m_stDiFwReg.C420FixEn = 0;
	pstOvpObj->m_stDiFwIData.ghf_acc = 0;
	pstOvpObj->m_stDiFwIData.glamd_acc = 0;
	pstOvpObj->m_stDiFwIData.grpd_acc = 0;
	pstOvpObj->m_stDiFwIData.gosd_acc = 0;
	pstOvpObj->m_stDiFwIData.gmdv_acc = 49*256;
	pstOvpObj->m_stDiFwIData.gdif_acc = 0;
	pstOvpObj->m_stDiFwIData.Fmd_lgmv_acc = 0;
	pstOvpObj->m_stDiFwIData.gmdv_max = 49;
	pstOvpObj->m_stDiFwIData.gmdv_min = 0;
	pstOvpObj->m_stDiFwIData.C420Fix_acc = 0;
	pstOvpObj->m_stDiFwIData.C420Fix_yacc = 0;
	pstOvpObj->m_stDiFwReg.FmdScrTxtSel = 0;

	for(i=0; i<2; i++) {
		pstOvpObj->m_stDiFwReg.FmdGlmvThr[i] = 0;
		pstOvpObj->m_stDiFwReg.GosdThr[i] = 0;
		pstOvpObj->m_stDiFwReg.GhfThr[i] = 0;
		pstOvpObj->m_stDiFwIData.AngHsgrmThr[i] = 0;
		pstOvpObj->m_stDiFwReg.GmdvThr[i] = 0;
		pstOvpObj->m_stDiFwReg.FmdGrpdThr[i] = 0;
	}

	for(i=0; i<3; i++) {
		pstOvpObj->m_stDiFwReg.GhmvThr[i] = 0;
		pstOvpObj->m_stDiFwReg.NoiseThr[i] = 0;
		pstOvpObj->m_stDiFwReg.GmdvMinThr[i] = 0;
		pstOvpObj->m_stDiFwReg.GrpdThr[i] = 0;
		pstOvpObj->m_stDiFwReg.GlamdThr[i] = 0;
		pstOvpObj->m_stDiFwReg.C420AccThr[i] = 0;
		pstOvpObj->m_stDiFwIData.IntValueThr[i] =0;
	}

	for(i=0; i<4; i++) {
		pstOvpObj->m_stDiFwReg.FmdGain[i] = 0;
		pstOvpObj->m_stDiFwReg.AngHsgrmThr[i] = 0;
		pstOvpObj->m_stDiFwIData.YMdvThr[i] = 0;
		pstOvpObj->m_stDiFwIData.UVMdvThr[i] = 0;
		pstOvpObj->m_stDiFwReg.YMdvThr[i] = 0;
		pstOvpObj->m_stDiFwReg.UVMdvThr[i] = 0;
		pstOvpObj->m_stDiFwReg.YMdvGhfThr[i] = 0;
		pstOvpObj->m_stDiFwReg.YMdvGlmvThr[i] = 0;
		pstOvpObj->m_stDiFwReg.UVMdvGhfThr[i] = 0;
		pstOvpObj->m_stDiFwReg.YMdvGvhfThr[i] = 0;
		pstOvpObj->m_stDiFwReg.UVMdvGvhfThr[i] = 0;
		pstOvpObj->m_stDiFwIData.AFDF0VPosDelay[i] =0;

	}

	for (i=0; i<5; i++)
		pstOvpObj->m_stDiFwReg.FmdCountThr[i] = 0;

	for (i=0; i<6; i++)
		pstOvpObj->m_stDiFwReg.IntValueThr[i] = 0;

	for (i=0; i<7; i++) {
		pstOvpObj->m_stDiFwReg.SdiOff0[i] = 0;
		pstOvpObj->m_stDiFwReg.SdiGain0[i] = 0;
		pstOvpObj->m_stDiFwReg.SdiOff1[i] = 0;
		pstOvpObj->m_stDiFwReg.SdiGain1[i] = 0;
		pstOvpObj->m_stDiFwReg.SdiOff2[i] = 0;
		pstOvpObj->m_stDiFwReg.SdiGain2[i] = 0;
	}
	for (i=0; i<8; i++) {
		pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[i] =0;
		pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[i] =0;
		pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[i] =0;
		pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[i] =0;
	}


	for (i=0; i<13; i++) {
		pstOvpObj->m_stDiFwReg.FmdOff[i] = 0;
		pstOvpObj->m_stDiFwIData.ghmv_acc[i] = 0;
		pstOvpObj->m_stDiFwIData.rhmv[i] = 0;
		pstOvpObj->m_stDiFwReg.FmdThr[i] = 0;
	}

	for (j=0; j<16; j++)
		for (i=0; i<16; i++) {
			pstOvpObj->m_stDiFwIData.bmdv[j][i] = 0;
			pstOvpObj->m_stDiFwIData.bstc[j][i] = 0;
			pstOvpObj->m_stDiFwIData.bmhf[j][i] = 0;
			pstOvpObj->m_stDiFwIData.bosd[j][i] = 0;
			pstOvpObj->m_stDiFwIData.blamd[j][i] = 0;
			pstOvpObj->m_stDiFwIData.blasd[j][i] = 0;
			pstOvpObj->m_stDiFwIData.bsosd[j][i] = 0;
			pstOvpObj->m_stDiFwIData.blmhf[j][i] = 0;
		}

	pstOvpObj->m_stDiFwReg.GhmvEn = 0;
	pstOvpObj->m_stDiFwReg.GhmvUser = 0;
	pstOvpObj->m_stDiFwReg.FmdFWEn = 0;
	pstOvpObj->m_stDiFwReg.FmdUser = 0;
	pstOvpObj->m_stDiFwReg.GhfEn = 0;
	pstOvpObj->m_stDiFwReg.GhfUser = 0;
	pstOvpObj->m_stDiFwReg.GdifEn = 0;
	pstOvpObj->m_stDiFwReg.GdifUser = 0;
	pstOvpObj->m_stDiFwReg.FmdMcdiBypsThr = 0;
	pstOvpObj->m_stDiFwReg.GdifThr = 0;
	pstOvpObj->m_stDiFwReg.GrpdEn = 0;
	pstOvpObj->m_stDiFwReg.GrpdUser = 0;
	pstOvpObj->m_stDiFwReg.GosdEn = 0;
	pstOvpObj->m_stDiFwReg.GosdUser = 0;
	pstOvpObj->m_stDiFwReg.GmdvEn = 0;
	pstOvpObj->m_stDiFwReg.GmdvUser = 0;
	pstOvpObj->m_stDiFwReg.BLAMDEn = 0;
	pstOvpObj->m_stDiFwReg.BLAMDUser = 0;
	pstOvpObj->m_stDiFwReg.BLASDEn = 0;
	pstOvpObj->m_stDiFwReg.BLASDUser = 0;
	pstOvpObj->m_stDiFwReg.BSOSDEn = 0;
	pstOvpObj->m_stDiFwReg.BSOSDUser = 0;
	pstOvpObj->m_stDiFwReg.BLMHFEn = 0;
	pstOvpObj->m_stDiFwReg.BLMHFUser = 0;
	pstOvpObj->m_stDiFwReg.NLEEn = 0;
	pstOvpObj->m_stDiFwReg.NLEUser = 0;
	pstOvpObj->m_stDiFwReg.BSOSDEn = 0;
	pstOvpObj->m_stDiFwReg.BSOSDUser = 0;
	pstOvpObj->m_stDiFwReg.SOSDCountThr = 0;
	pstOvpObj->m_stDiFwReg.LMHFCountThr = 0;
	pstOvpObj->m_stDiFwReg.LASDCountThr = 0;
	pstOvpObj->m_stDiFwReg.LAMDCountGain = 0;
}

/*******************************************************************
 * FUNCTION: Initialize Firmware Reset Sequence.
 * INPUT:   None
 * RETURN:  None
 *******************************************************************/
void OVP_DEINT_FwReset(OVP_OBJ *pstOvpObj)
{
	OVP_DEINT_FwPowerOnReset(pstOvpObj);
	OVP_DEINT_FwInit(pstOvpObj);

	pstOvpObj->m_stDiFwReg.C420FixIirFilterEn = 1;
	pstOvpObj->m_stDiFwReg.C420AccThr[2] = 64;
	pstOvpObj->m_stDiFwReg.C420AccThr[1] = 128;
	pstOvpObj->m_stDiFwReg.C420AccThr[0] = 10;
	pstOvpObj->m_stDiFwReg.C420FixEn = 0;
	pstOvpObj->m_stDiFwReg.CUEid = 0;
	pstOvpObj->m_stDiFwReg.GhfEn = 1;
	pstOvpObj->m_stDiFwReg.GhfUser = 0;
	pstOvpObj->m_stDiFwReg.GrpdEn = 1;
	pstOvpObj->m_stDiFwReg.GrpdUser = 1;
	pstOvpObj->m_stDiFwReg.GosdEn = 1;
	pstOvpObj->m_stDiFwReg.GosdUser = 0;
	pstOvpObj->m_stDiFwReg.GmdvEn = 1;
	pstOvpObj->m_stDiFwReg.GmdvUser = 1;
	pstOvpObj->m_stDiFwReg.GmdvMinEn = 1;
	pstOvpObj->m_stDiFwReg.GmdvMinUser = 0;
	pstOvpObj->m_stDiFwReg.GlamdEn = 1;
	pstOvpObj->m_stDiFwReg.GlamdUser = 2;
	pstOvpObj->m_stDiFwReg.BLAMDEn = 0;
	pstOvpObj->m_stDiFwReg.BLAMDUser = 0;
	pstOvpObj->m_stDiFwReg.BLASDEn = 0;
	pstOvpObj->m_stDiFwReg.BLASDUser = 0;
	pstOvpObj->m_stDiFwReg.BSOSDEn = 0;
	pstOvpObj->m_stDiFwReg.BSOSDUser = 0;
	pstOvpObj->m_stDiFwReg.BLMHFEn = 0;
	pstOvpObj->m_stDiFwReg.BLMHFUser = 0;
	pstOvpObj->m_stDiFwReg.NLEEn = 0;
	pstOvpObj->m_stDiFwReg.NLEUser = 0;

	pstOvpObj->m_stDiFwReg.NoiseThr[2] = 4;
	pstOvpObj->m_stDiFwReg.NoiseThr[1] = 2;
	pstOvpObj->m_stDiFwReg.NoiseThr[0] = 1;

	pstOvpObj->m_stDiFwReg.GlamdThr[2] = 256;
	pstOvpObj->m_stDiFwReg.GlamdThr[1] = 32;
	pstOvpObj->m_stDiFwReg.GlamdThr[0] = 4;

	pstOvpObj->m_stDiFwReg.GmdvThr[1] = 35;
	pstOvpObj->m_stDiFwReg.GmdvThr[0] = 31;

	pstOvpObj->m_stDiFwReg.GmdvMinThr[2] = 32;
	pstOvpObj->m_stDiFwReg.GmdvMinThr[1] = 16;
	pstOvpObj->m_stDiFwReg.GmdvMinThr[0] = 8;

	pstOvpObj->m_stDiFwReg.GhfThr[1] = 42;
	pstOvpObj->m_stDiFwReg.GhfThr[0] = 33;

	pstOvpObj->m_stDiFwReg.GosdThr[1] = 480;
	pstOvpObj->m_stDiFwReg.GosdThr[0] = 17;

	pstOvpObj->m_stDiFwReg.GrpdThr[2] = 118;
	pstOvpObj->m_stDiFwReg.GrpdThr[1] = 130;
	pstOvpObj->m_stDiFwReg.GrpdThr[0] = 3;

	pstOvpObj->m_stDiFwReg.YMdvThr[3] = 64;
	pstOvpObj->m_stDiFwReg.YMdvThr[2] = 60;
	pstOvpObj->m_stDiFwReg.YMdvThr[1] = 6;
	pstOvpObj->m_stDiFwReg.YMdvThr[0] = 2;

	pstOvpObj->m_stDiFwReg.UVMdvThr [3] = 16;
	pstOvpObj->m_stDiFwReg.UVMdvThr [2] = 8;
	pstOvpObj->m_stDiFwReg.UVMdvThr [1] = 4;
	pstOvpObj->m_stDiFwReg.UVMdvThr [0] = 2;

	pstOvpObj->m_stDiFwReg.YMdvGhfThr[3] = 255;
	pstOvpObj->m_stDiFwReg.YMdvGhfThr[2] = 255;
	pstOvpObj->m_stDiFwReg.YMdvGhfThr[1] = 4;
	pstOvpObj->m_stDiFwReg.YMdvGhfThr[0] = 2;

	pstOvpObj->m_stDiFwReg.UVMdvGhfThr[3] = 255;
	pstOvpObj->m_stDiFwReg.UVMdvGhfThr[2] = 255;
	pstOvpObj->m_stDiFwReg.UVMdvGhfThr[1] = 4;
	pstOvpObj->m_stDiFwReg.UVMdvGhfThr[0] = 2;

	pstOvpObj->m_stDiFwReg.YMdvGvhfThr[3] = 64;
	pstOvpObj->m_stDiFwReg.YMdvGvhfThr[2] = 60;
	pstOvpObj->m_stDiFwReg.YMdvGvhfThr[1] = 4;
	pstOvpObj->m_stDiFwReg.YMdvGvhfThr[0] = 2;

	pstOvpObj->m_stDiFwReg.UVMdvGvhfThr[3] = 256;
	pstOvpObj->m_stDiFwReg.UVMdvGvhfThr[2] = 256;
	pstOvpObj->m_stDiFwReg.UVMdvGvhfThr[1] = 4;
	pstOvpObj->m_stDiFwReg.UVMdvGvhfThr[0] = 2;

	pstOvpObj->m_stDiFwReg.AngHsgrmThr[3] = 19;
	pstOvpObj->m_stDiFwReg.AngHsgrmThr[2] = 15;
	pstOvpObj->m_stDiFwReg.AngHsgrmThr[1] = 9;
	pstOvpObj->m_stDiFwReg.AngHsgrmThr[0] = 6;

	pstOvpObj->m_stDiFwReg.IntValueThr[5] = 150;
	pstOvpObj->m_stDiFwReg.IntValueThr[4] = 60;
	pstOvpObj->m_stDiFwReg.IntValueThr[3] = 0;
	pstOvpObj->m_stDiFwReg.IntValueThr[2] = 150;
	pstOvpObj->m_stDiFwReg.IntValueThr[1] = 101;
	pstOvpObj->m_stDiFwReg.IntValueThr[0] = 20;

	pstOvpObj->m_stDiFwReg.SOSDCountThr = 13;
	pstOvpObj->m_stDiFwReg.LMHFCountThr = 140;
	pstOvpObj->m_stDiFwReg.LASDCountThr = 255;
	pstOvpObj->m_stDiFwReg.LAMDCountGain = 13;

	pstOvpObj->m_stDiFwReg.SdiOff0[6] = 0;
	pstOvpObj->m_stDiFwReg.SdiOff0[5] = 0;
	pstOvpObj->m_stDiFwReg.SdiOff0[4] = 0;
	pstOvpObj->m_stDiFwReg.SdiOff0[3] = 0;
	pstOvpObj->m_stDiFwReg.SdiOff0[2] = 0;
	pstOvpObj->m_stDiFwReg.SdiOff0[1] = 2;
	pstOvpObj->m_stDiFwReg.SdiOff0[0] = 16;

	pstOvpObj->m_stDiFwReg.SdiGain0[6] = 0;
	pstOvpObj->m_stDiFwReg.SdiGain0[5] = 0;
	pstOvpObj->m_stDiFwReg.SdiGain0[4] = 0;
	pstOvpObj->m_stDiFwReg.SdiGain0[3] = 0;
	pstOvpObj->m_stDiFwReg.SdiGain0[2] = 0;
	pstOvpObj->m_stDiFwReg.SdiGain0[1] = 8;
	pstOvpObj->m_stDiFwReg.SdiGain0[0] = 11;

	pstOvpObj->m_stDiFwReg.SdiOff1[6] = 0;
	pstOvpObj->m_stDiFwReg.SdiOff1[5] = 1;
	pstOvpObj->m_stDiFwReg.SdiOff1[4] = 2;
	pstOvpObj->m_stDiFwReg.SdiOff1[3] = 4;
	pstOvpObj->m_stDiFwReg.SdiOff1[2] = 12;
	pstOvpObj->m_stDiFwReg.SdiOff1[1] = 28;
	pstOvpObj->m_stDiFwReg.SdiOff1[0] = 255;

	pstOvpObj->m_stDiFwReg.SdiGain1[6] = 0;
	pstOvpObj->m_stDiFwReg.SdiGain1[5] = 2;
	pstOvpObj->m_stDiFwReg.SdiGain1[4] = 10;
	pstOvpObj->m_stDiFwReg.SdiGain1[3] = 15;
	pstOvpObj->m_stDiFwReg.SdiGain1[2] = 18;
	pstOvpObj->m_stDiFwReg.SdiGain1[1] = 23;
	pstOvpObj->m_stDiFwReg.SdiGain1[0] = 23;

	pstOvpObj->m_stDiFwReg.SdiOff2[6] = 0;
	pstOvpObj->m_stDiFwReg.SdiOff2[5] = 1;
	pstOvpObj->m_stDiFwReg.SdiOff2[4] = 8;
	pstOvpObj->m_stDiFwReg.SdiOff2[3] = 16;
	pstOvpObj->m_stDiFwReg.SdiOff2[2] = 32;
	pstOvpObj->m_stDiFwReg.SdiOff2[1] = 64;
	pstOvpObj->m_stDiFwReg.SdiOff2[0] = 255;

	pstOvpObj->m_stDiFwReg.SdiGain2[6] = 0;
	pstOvpObj->m_stDiFwReg.SdiGain2[5] = 20;
	pstOvpObj->m_stDiFwReg.SdiGain2[4] = 20;
	pstOvpObj->m_stDiFwReg.SdiGain2[3] = 20;
	pstOvpObj->m_stDiFwReg.SdiGain2[2] = 31;
	pstOvpObj->m_stDiFwReg.SdiGain2[1] = 31;
	pstOvpObj->m_stDiFwReg.SdiGain2[0] = 31;

	//FW MCDI register controls
	pstOvpObj->m_stDiFwReg.GhmvThr[2] = 2048;
	pstOvpObj->m_stDiFwReg.GhmvThr[1] = 200;
	pstOvpObj->m_stDiFwReg.GhmvThr[0] = 64;

	pstOvpObj->m_stDiFwReg.GhmvEn = 4;
	pstOvpObj->m_stDiFwReg.GhmvUser = 0;
	pstOvpObj->m_stDiFwReg.GdifThr = 28;
	pstOvpObj->m_stDiFwReg.GdifEn = 1;
	pstOvpObj->m_stDiFwReg.GdifUser = 0;
	pstOvpObj->m_stDiFwReg.FmdMcdiBypsThr = 1;

	//FW FMD  register controls
	pstOvpObj->m_stDiFwReg.FmdCountThr[4] = 99;
	pstOvpObj->m_stDiFwReg.FmdCountThr[3] = 99;
	pstOvpObj->m_stDiFwReg.FmdCountThr[2] = 5;
	pstOvpObj->m_stDiFwReg.FmdCountThr[1] = 7;
	pstOvpObj->m_stDiFwReg.FmdCountThr[0] = 8;

	pstOvpObj->m_stDiFwReg.FmdGain[3] = 20;
	pstOvpObj->m_stDiFwReg.FmdGain[2] = 24;
	pstOvpObj->m_stDiFwReg.FmdGain[1] = 9;
	pstOvpObj->m_stDiFwReg.FmdGain[0] = 53;

	pstOvpObj->m_stDiFwReg.FmdOff[12] = 1;
	pstOvpObj->m_stDiFwReg.FmdOff[11] = 1;
	pstOvpObj->m_stDiFwReg.FmdOff[10] = 2;
	pstOvpObj->m_stDiFwReg.FmdOff[9] = 3;
	pstOvpObj->m_stDiFwReg.FmdOff[8] = 1;
	pstOvpObj->m_stDiFwReg.FmdOff[7] = 4;
	pstOvpObj->m_stDiFwReg.FmdOff[6] = 36;
	pstOvpObj->m_stDiFwReg.FmdOff[5] = 87;
	pstOvpObj->m_stDiFwReg.FmdOff[4] = 450;
	pstOvpObj->m_stDiFwReg.FmdOff[3] = 5;
	pstOvpObj->m_stDiFwReg.FmdOff[2] = 3;
	pstOvpObj->m_stDiFwReg.FmdOff[1] = 2;
	pstOvpObj->m_stDiFwReg.FmdOff[0] = 1;

	pstOvpObj->m_stDiFwReg.FmdFWEn = 1;
	pstOvpObj->m_stDiFwReg.FmdUser = 0;
	pstOvpObj->m_stDiFwReg.FmdLmvEn = 1;
	pstOvpObj->m_stDiFwReg.FmdLmvUser = 2;

	pstOvpObj->m_stDiFwReg.FmdThr[12] = 10;
	pstOvpObj->m_stDiFwReg.FmdThr[11] = 2;
	pstOvpObj->m_stDiFwReg.FmdThr[10] = 4;
	pstOvpObj->m_stDiFwReg.FmdThr[9] = 48;
	pstOvpObj->m_stDiFwReg.FmdThr[8] = 18;
	pstOvpObj->m_stDiFwReg.FmdThr[7] = 30;
	pstOvpObj->m_stDiFwReg.FmdThr[6] = 4;
	pstOvpObj->m_stDiFwReg.FmdThr[5] = 4;
	pstOvpObj->m_stDiFwReg.FmdThr[4] = 12;
	pstOvpObj->m_stDiFwReg.FmdThr[3] = 4;
	pstOvpObj->m_stDiFwReg.FmdThr[2] = 16;
	pstOvpObj->m_stDiFwReg.FmdThr[1] = 14;
	pstOvpObj->m_stDiFwReg.FmdThr[0] = 12;

	pstOvpObj->m_stDiFwReg.FmdGrpdThr[1] = 280;
	pstOvpObj->m_stDiFwReg.FmdGrpdThr[0] = 140;

	pstOvpObj->m_stDiFwReg.FmdGlmvThr[1] = 40;
	pstOvpObj->m_stDiFwReg.FmdGlmvThr[0] = 6;

	pstOvpObj->m_stDiFwReg.FmdScrTxtSel = 0;

	pstOvpObj->m_stDiFwReg.YMdvGlmvThr[3] = 60;
	pstOvpObj->m_stDiFwReg.YMdvGlmvThr[2] = 60;
	pstOvpObj->m_stDiFwReg.YMdvGlmvThr[1] = 4;
	pstOvpObj->m_stDiFwReg.YMdvGlmvThr[0] = 1;
}
