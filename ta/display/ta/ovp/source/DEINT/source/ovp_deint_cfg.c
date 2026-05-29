/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#define _OVP_DEINT_CFG_C_
/************* Header file include ********************************************/
#include "ovp_module.h"
#include "ovp_common.h"
#include "galois_io.h"

UINT16 MaxPix(UINT16 ain[], UINT16 numElements)
{
	UINT16 i, I;
	UINT16 temp;

	temp = 0;
	I = numElements-1;
	for (i = 0; i<numElements; i++) {
		if(ain[i]>= temp) {
			temp = ain[i];
			I = i;
		}
	}
	return (I);
}

/*******************************************************************************
Function:    gdif_det()
Arguments:   None
Returns:     None
Description: global MC difference detection
 ********************************************************************************/
void gdif_det(OVP_OBJ *pstOvpObj)
{
	UINT32 dif_acc, gdifthr;
	int cur_x, pre_x;
	/* Persistent state across calls for IIR filtering */
	static int out_y = 0;

	gdifthr = pstOvpObj->m_stDiFwReg.GdifThr;
	/* global high-frequency detection */
	dif_acc = (pstOvpObj->m_stDiFwIData.gdif_acc<<8)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize + 1)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize + 1);

	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset)||((UINT32) abs(dif_acc-out_y)>(out_y+dif_acc)/2))
		pre_x = 4*gdifthr;
	else
		pre_x = pstOvpObj->m_stDiFwIData.GDIFState;

	/* iir state and output filters */
	cur_x = (3*pre_x + 4*dif_acc + 2)/4;
	pstOvpObj->m_stDiFwIData.GDIFState = cur_x;
	out_y = (cur_x + pre_x)/8;

	if(out_y>(int) gdifthr)
		pstOvpObj->m_stDiFwIData.gdif = 1;
	else
		pstOvpObj->m_stDiFwIData.gdif = 0;

	/* manual mode */
	if((! pstOvpObj->m_stDiFwReg.GdifEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
		pstOvpObj->m_stDiFwIData.gdif = pstOvpObj->m_stDiFwReg.GdifUser;

}

/*******************************************************************************
Function:    grpd_det()
Arguments:   None
Returns:     None
Description: global rpd detection
 ********************************************************************************/
void grpd_det(OVP_OBJ *pstOvpObj)
{
	UINT32 rpd_acc, fmdgrpd;
	int cur_x, pre_x;
	static int out_y = 0;
	UINT32 fmd_lock;

	fmd_lock = (pstOvpObj->m_stDiFwIData.fmd%64)/32;

	/* global repeated pattern detection */
	rpd_acc = (pstOvpObj->m_stDiFwIData.grpd_acc<<12)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize + 1)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize + 1);

	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset))    /* ||((UINT32)abs(rpd_acc-out_y)>(out_y+rpd_acc)/2)) */
		pre_x = 4*rpd_acc;
	else
		pre_x = pstOvpObj->m_stDiFwIData.GRPDState;

	/* iir state and output filters */
	cur_x = (3*pre_x + 4*rpd_acc + 2)/4;
	pstOvpObj->m_stDiFwIData.GRPDState = cur_x;
	out_y = (cur_x + pre_x)/8;

	if(out_y>(int)	pstOvpObj->m_stDiFwReg.GrpdThr[1])
		pstOvpObj->m_stDiFwIData.grpd = 2;
	else if(out_y>(int)  pstOvpObj->m_stDiFwReg.GrpdThr[0])
		pstOvpObj->m_stDiFwIData.grpd = 1;
	else
		pstOvpObj->m_stDiFwIData.grpd = 0;

	if(out_y>(int)	pstOvpObj->m_stDiFwReg.FmdGrpdThr[1])
		fmdgrpd = 2;
	else if(out_y>(int)  pstOvpObj->m_stDiFwReg.FmdGrpdThr[0])
		fmdgrpd = 1;
	else
		fmdgrpd = 0;

	/* new added for MCDI */
	if(out_y>(int)	pstOvpObj->m_stDiFwReg.GrpdThr[2])
		pstOvpObj->m_stDiFwIData.grpd = 4 + pstOvpObj->m_stDiFwIData.grpd;
	else if(pstOvpObj->m_stDiFwIData.gdif&&(!fmd_lock))
		pstOvpObj->m_stDiFwIData.grpd = 4 + pstOvpObj->m_stDiFwIData.grpd;
	else if(pstOvpObj->m_stDiFwIData.mcdibyps)
		pstOvpObj->m_stDiFwIData.grpd = 4 + pstOvpObj->m_stDiFwIData.grpd;

	/* manu mode */
	if((! pstOvpObj->m_stDiFwReg.GrpdEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
		pstOvpObj->m_stDiFwIData.grpd = pstOvpObj->m_stDiFwReg.GrpdUser;

	pstOvpObj->m_stDiFwIData.grpd = pstOvpObj->m_stDiFwIData.grpd*4 + fmdgrpd;
}

/*******************************************************************************
Function:    gosd_det()
Arguments:   None
Returns:     None
Description: global osd detection
 ********************************************************************************/
void gosd_det(OVP_OBJ *pstOvpObj)
{
	UINT32 sosd_acc, scr_osd_acc;
	int cur_x, pre_x;
	static int out_x = 0, out_y = 0;

	/* global high-frequency detection */
	sosd_acc = ((pstOvpObj->m_stDiFwIData.gosd_acc/65536)<<8)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize + 1)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize + 1);
	scr_osd_acc = ((pstOvpObj->m_stDiFwIData.gosd_acc%65536)<<16)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize + 1)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize + 1);

	/* static OSD */
	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset)||((out_y+sosd_acc)/2<(UINT32)abs(sosd_acc - out_x)))
		pre_x = 4*sosd_acc;
	else
		pre_x = pstOvpObj->m_stDiFwIData.GOSDState;

	/* iir state and output filters */
	cur_x = (3*pre_x + 4*sosd_acc + 2)/4;
	pstOvpObj->m_stDiFwIData.GOSDState = cur_x;
	out_y = (cur_x + pre_x)/8;


	/* scrolling OSD */
	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset)||((out_x+scr_osd_acc)/2<(UINT32)abs(scr_osd_acc - out_x)))
		pre_x = 4*scr_osd_acc;
	else
		pre_x = pstOvpObj->m_stDiFwIData.ScrGOSDState;

	/* iir state and output filters */
	cur_x = (3*pre_x + 4*scr_osd_acc + 2)/4;
	pstOvpObj->m_stDiFwIData.ScrGOSDState = cur_x;
	out_x = (cur_x + pre_x)/8;


	if(out_y>(int)	pstOvpObj->m_stDiFwReg.GosdThr[0])
		pstOvpObj->m_stDiFwIData.gosd = 1;
	else if(out_x>(int)  pstOvpObj->m_stDiFwReg.GosdThr[1])
		pstOvpObj->m_stDiFwIData.gosd = 2;
	else if(out_y>0)
		pstOvpObj->m_stDiFwIData.gosd = 3;
	else
		pstOvpObj->m_stDiFwIData.gosd = 0;

	/* manu mode */
	if((! pstOvpObj->m_stDiFwReg.GosdEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
		pstOvpObj->m_stDiFwIData.gosd = pstOvpObj->m_stDiFwReg.GosdUser;
}

/*******************************************************************************
Function:    ghf_det()
Arguments:   None
Returns:     None
Description: global high-frequence detection
 ********************************************************************************/
void ghf_det(OVP_OBJ *pstOvpObj)
{
	UINT32 hf_acc, ghfthr, rpd;
	int cur_x, pre_x;
	static int out_y = 0;

	rpd = (pstOvpObj->m_stDiFwIData.grpd/4)%4;
	if(rpd == 2)
		ghfthr = pstOvpObj->m_stDiFwReg.GhfThr[1];
	else
		ghfthr = pstOvpObj->m_stDiFwReg.GhfThr[0];

	/* global high-frequency detection */
	hf_acc = (pstOvpObj->m_stDiFwIData.ghf_acc<<8)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize + 1)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize + 1);

	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset))    /* ||((UINT32)abs(hf_acc-out_y)>(out_y+hf_acc)/2)) */
		pre_x = 4*hf_acc;
	else
		pre_x = pstOvpObj->m_stDiFwIData.GHFState;

	/* iir state and output filters */
	cur_x = (3*pre_x + 4*hf_acc + 2)/4;
	pstOvpObj->m_stDiFwIData.GHFState = cur_x;
	out_y = (cur_x + pre_x)/8;

	if(out_y>(int) ghfthr)
		pstOvpObj->m_stDiFwIData.ghf = 1;
	else
		pstOvpObj->m_stDiFwIData.ghf = 0;

	/* manu mode */
	if((! pstOvpObj->m_stDiFwReg.GhfEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
		pstOvpObj->m_stDiFwIData.ghf = pstOvpObj->m_stDiFwReg.GhfUser;
}

/*******************************************************************************
Function:    gmdv_det()
Arguments:   None
Returns:     None
Description: global motion detection
 ********************************************************************************/
void gmdv_det(OVP_OBJ *pstOvpObj)
{
	UINT32 gmdv;
	int cur_x, pre_x;
	static int out_y = 0;

	gmdv = pstOvpObj->m_stDiFwIData.gmdv_max;
	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset)||(abs(out_y-gmdv)>(int)(out_y + gmdv)/2))
		pre_x = 4*gmdv*32;
	else
		pre_x = pstOvpObj->m_stDiFwIData.GMDVState[0];

	/* iir state and output filters */
	cur_x = (3*pre_x + 4*gmdv*32)/4;
	pstOvpObj->m_stDiFwIData.GMDVState[0] = cur_x;
	out_y = (cur_x + pre_x)/256;

	if(out_y>(int)	pstOvpObj->m_stDiFwReg.GmdvThr[1])
		pstOvpObj->m_stDiFwIData.gmdv = 2;
	else if(out_y>(int)  pstOvpObj->m_stDiFwReg.GmdvThr[0])
		pstOvpObj->m_stDiFwIData.gmdv = 1;
	else
		pstOvpObj->m_stDiFwIData.gmdv = 0;

	if((! pstOvpObj->m_stDiFwReg.GmdvEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
		pstOvpObj->m_stDiFwIData.gmdv = pstOvpObj->m_stDiFwReg.GmdvUser;

}

/*******************************************************************************
Function:    gmdv_min_det()
Arguments:   None
Returns:     None
Description: global minimum motion detection
 ********************************************************************************/
void gmdv_min_det(OVP_OBJ *pstOvpObj)
{
	UINT32 gmdv;
	int cur_x, pre_x;
	static int out_y = 0;

	gmdv = pstOvpObj->m_stDiFwIData.gmdv_min;
	if(gmdv> pstOvpObj->m_stDiFwReg.GmdvMinThr[2])
		gmdv = pstOvpObj->m_stDiFwReg.GmdvMinThr[2] + 1;

	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset)||((UINT32)abs(gmdv-out_y)>(out_y+gmdv)/2))
		pre_x = 4*gmdv*32;
	else
		pre_x = pstOvpObj->m_stDiFwIData.GMDVState[1];

	/* iir state and output filters */
	cur_x = (3*pre_x + 4*gmdv*32)/4;
	pstOvpObj->m_stDiFwIData.GMDVState[1] = cur_x;
	out_y = (cur_x + pre_x)/256;

	if(out_y>(int)	pstOvpObj->m_stDiFwReg.GmdvMinThr[2])
		pstOvpObj->m_stDiFwIData.gmin = 3;
	else if(out_y>(int)  pstOvpObj->m_stDiFwReg.GmdvMinThr[1])
		pstOvpObj->m_stDiFwIData.gmin = 2;
	else if(out_y>(int)  pstOvpObj->m_stDiFwReg.GmdvMinThr[0])
		pstOvpObj->m_stDiFwIData.gmin = 1;
	else
		pstOvpObj->m_stDiFwIData.gmin = 0;

	if((! pstOvpObj->m_stDiFwReg.GmdvMinEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
		pstOvpObj->m_stDiFwIData.gmin = pstOvpObj->m_stDiFwReg.GmdvMinUser;
}

/*******************************************************************************
Function:    glamd_det()
Arguments:   None
Returns:     None
Description: global large area motion detection
 ********************************************************************************/
void glamd_det(OVP_OBJ *pstOvpObj)
{
	int cur_x, pre_x;
	static int out_y = 0;
	UINT32 gacc;
	T32MadiReg_ShiftBit	m_stMadiRegShiftBit;

	m_stMadiRegShiftBit.u32 = pstOvpObj->m_stMadiReg.m_uiMadiRegShiftBit;

#if(!BLKLOGICDISABLE)
	UINT32 j, i;

	gacc = 0;
	for (j = 0;j <16; j++)
		for (i = 0; i <16; i++)
		{
			gacc = gacc + (pstOvpObj->m_stDiFwIData.bmdv[j][i]<<pDiHwReg->BlamdShiftBit);
		}
#else
	gacc = pstOvpObj->m_stDiFwIData.glamd_acc<<m_stMadiRegShiftBit.uShiftBit_BlamdShiftBit;
#endif

	gacc = (gacc<<8)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize + 1)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize + 1);

	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset)||((UINT32)abs(gacc-out_y)>(gacc+out_y)/2))
		pre_x = 2*gacc;
	else
		pre_x = pstOvpObj->m_stDiFwIData.GLAMDState;

	/* iir state and output filters */
	cur_x = (pre_x + 2*gacc + 1)/2;
	pstOvpObj->m_stDiFwIData.GLAMDState = cur_x;
	out_y = (cur_x + pre_x)/4;

	if(out_y>(int)	pstOvpObj->m_stDiFwReg.GlamdThr[2])
		pstOvpObj->m_stDiFwIData.glamd = 3;
	else if(out_y>(int)  pstOvpObj->m_stDiFwReg.GlamdThr[1])
		pstOvpObj->m_stDiFwIData.glamd = 2;
	else if(out_y>(int)  pstOvpObj->m_stDiFwReg.GlamdThr[0])
		pstOvpObj->m_stDiFwIData.glamd = 1;
	else
		pstOvpObj->m_stDiFwIData.glamd = 0;

	if((! pstOvpObj->m_stDiFwReg.GmdvEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
		pstOvpObj->m_stDiFwIData.glamd = pstOvpObj->m_stDiFwReg.GmdvUser;

}

/*******************************************************************************
Function:    ghmv_det()
Arguments:   None
Returns:     None
Description: global hmv detection
 ********************************************************************************/
void ghmv_det(OVP_OBJ *pstOvpObj)
{
	UINT32 i, j, max, hmv_acc, hmv_count;
	int cur_x, pre_x, ghmv_en;
	static int out_y = 0;
	UINT16 ghmv_acc[13];

	ghmv_en = 0;
	hmv_count = 0;
	/* global motion vector detection */
	for (i= 0; i<13; i++) {
		ghmv_acc[i] = (UINT16) pstOvpObj->m_stDiFwIData.ghmv_acc[i];
		pstOvpObj->m_stDiFwIData.rhmv[i] = 0;
		hmv_acc = (pstOvpObj->m_stDiFwIData.ghmv_acc[i]<<12)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize + 1)/(pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize + 1);
		/* temporal IIR filter */
		if((pstOvpObj->m_stDiFwIData.FrameReset)||((UINT32)abs(hmv_acc-out_y)>(out_y+hmv_acc)/2))
			pre_x = 4*hmv_acc;
		else
			pre_x = pstOvpObj->m_stDiFwIData.GHMVState[i];

		/* iir state and output filters */
		cur_x = (3*pre_x + 4*hmv_acc + 2)/4;
		pstOvpObj->m_stDiFwIData.GHMVState[i] = cur_x;
		out_y = (cur_x + pre_x)/8;

		if((out_y>(int)  pstOvpObj->m_stDiFwReg.GhmvThr[1])&&(i!= 6))
			ghmv_en = 1;
		if(out_y>(int)	pstOvpObj->m_stDiFwReg.GhmvThr[0]) {
			if(i!= 6)
				hmv_count = hmv_count + 1;
			/* manu mode */
			if((! pstOvpObj->m_stDiFwReg.GhmvEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
				pstOvpObj->m_stDiFwIData.ghmv = pstOvpObj->m_stDiFwReg.GhmvUser;
			else
				pstOvpObj->m_stDiFwIData.ghmv = hmv_count;

			if((! pstOvpObj->m_stDiFwReg.GhmvEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
				pstOvpObj->m_stDiFwIData.rhmv[i] = pstOvpObj->m_stDiFwReg.GhmvUser;
			else if(hmv_count>0)
				pstOvpObj->m_stDiFwIData.rhmv[i] = 1;

		}
	}

	ghmv_acc[6] = 0;
	i = MaxPix(ghmv_acc, 13);
	max = pstOvpObj->m_stDiFwIData.ghmv_acc[i];


	if(((i == 0)||(i==12))&&(max> pstOvpObj->m_stDiFwReg.GhmvThr[2])) {
		for (j= 0; j<13; j++)
			pstOvpObj->m_stDiFwIData.rhmv[j] = 0;
		pstOvpObj->m_stDiFwIData.rhmv[i] = 1;
		pstOvpObj->m_stDiFwIData.ghmv = 15;
	} else if((pstOvpObj->m_stDiFwIData.ghmv == 1)&&(ghmv_en==0)) {
		for (i= 0; i<13; i++)
			pstOvpObj->m_stDiFwIData.rhmv[i] = 0;
		pstOvpObj->m_stDiFwIData.ghmv = 0;
	}
}

/*******************************************************************************
Function:    film_mode_det()
Arguments:   None
Returns:     None
Description: film mode detection using fimware
 ********************************************************************************/
void film_mode_det(OVP_OBJ *pstOvpObj)
{
	int macc0[8], sacc0[8];
	int macc1[8], sacc1[8];
	int macc0_temp[8], sacc0_temp[8];
	int macc1_temp[8], sacc1_temp[8];
	UINT32 still01[8], still21[8];
	int i, j, still0, still1;
	static UINT32 fcount = 0, fcount22_0 = 0, fcount22_1 = 0;
	static UINT32 fcount32 = 0, divider_i = 0, sel22 = 0, lock32 = 0, lock32_temp = 0;
	static UINT32 lock = 0, lock22 = 0, local_lock = 0, ph = 0;
	UINT32 max, pre_divider_i, min, sel;
	UINT32 max0, max1, min0, min1, gstill;
	int cur_x, pre_x;
	int fmdmoff, fmdsoff, lcnst;
	static UINT32 lock_count = 0, lock22_count = 0, lock32_count = 0;
	static UINT32 st_rst, st_fcount = 0;
	UINT16 reset22, reset32, reset_lock;
	UINT32 tmacc[2], tsacc[2], image_ratio[2];
	UINT32 rpd, osd, fmdgain, fmdcountthr;
	UINT32 fmdthr_lo, fmdthr_hi, fmdthr_dif;

	if( pstOvpObj->m_stDiFwReg.FmdFWEn) {
		rpd = pstOvpObj->m_stDiFwIData.grpd%4;
		osd = pstOvpObj->m_stDiFwIData.gosd;
		fmdthr_lo = pstOvpObj->m_stDiFwReg.FmdThr[3];
		fmdthr_hi = pstOvpObj->m_stDiFwReg.FmdThr[5];
		fmdthr_dif = pstOvpObj->m_stDiFwReg.FmdThr[10];

		tmacc[0] = 0;
		tmacc[1] = 0;
		tsacc[0] = 0;
		tsacc[1] = 0;
		for(i= 0;i<8;i++)
		{
			tmacc[0] = tmacc[0] + pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[i];
			tmacc[1] = tmacc[1] + pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[i];
			tsacc[0] = tsacc[0] + pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[i];
			tsacc[1] = tsacc[1] + pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[i];
		}
		image_ratio[0] = (1<<20)/(tmacc[0]+tsacc[0]+1);
		image_ratio[1] = (1<<20)/(tmacc[1]+tsacc[1]+1);

		/* new added for lgmv enhancement (7/2/14) */
		if(tmacc[0]<tmacc[1])
			pstOvpObj->m_stDiFwIData.Fmd_lgmv_tacc = tmacc[1];
		else
			pstOvpObj->m_stDiFwIData.Fmd_lgmv_tacc = tmacc[0];


		if (image_ratio[0]>512)
			image_ratio[0] = 512;
		else if (image_ratio[0]<16)
			image_ratio[0] = 16;

		if (image_ratio[1]>512)
			image_ratio[1] = 512;
		else if (image_ratio[1]<16)
			image_ratio[1] = 16;


		for(i= 0;i<4;i++) {
			macc0_temp[i] = (15*pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[i]+1*pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[i+4])*image_ratio[0]/256;
			sacc0_temp[i] = (15*pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[i]+1*pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[i+4])*image_ratio[0]/256;
			macc1_temp[i] = (15*pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[i]+1*pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[i+4])*image_ratio[1]/256;
			sacc1_temp[i] = (15*pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[i]+1*pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[i+4])*image_ratio[1]/256;
		}
		for(i= 4;i<8;i++) {
			macc0_temp[i] = (15*pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[i]+1*pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[i-4])*image_ratio[0]/256;
			sacc0_temp[i] = (15*pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[i]+1*pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[i-4])*image_ratio[0]/256;
			macc1_temp[i] = (15*pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[i]+1*pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[i-4])*image_ratio[1]/256;
			sacc1_temp[i] = (15*pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[i]+1*pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[i-4])*image_ratio[1]/256;
		}

		for(i= 0;i<20;i++) {
			for(j= 0;j<8;j++) {
				macc0[j] = macc0_temp[j]>>i;
				sacc0[j] = sacc0_temp[j]>>i;
				macc1[j] = macc1_temp[j]>>i;
				sacc1[j] = sacc1_temp[j]>>i;
			}

			max0 = 0;
			for(j= 0;j<4;j++)
				if(macc0[j]<(int)  pstOvpObj->m_stDiFwReg.FmdThr[11])
					max0 = max0 + 3;

			max1 = 0;
			for(j= 4;j<8;j++)
				if(macc0[j]<(int)  pstOvpObj->m_stDiFwReg.FmdThr[11])
					max1 = max1 + 2;

			max = (max0 + max1)/2;
			if(max>(int)  pstOvpObj->m_stDiFwReg.FmdThr[12]) break;

			max0 = 0;
			for(j= 0;j<4;j++)
				if(macc1[j]<(int)  pstOvpObj->m_stDiFwReg.FmdThr[11])
					max0 = max0 + 3;

			max1 = 0;
			for(j= 4;j<8;j++)
				if(macc1[j]<(int)  pstOvpObj->m_stDiFwReg.FmdThr[11])
					max1 = max1 + 2;

			max = (max0 + max1)/2;
			if(max>(int)  pstOvpObj->m_stDiFwReg.FmdThr[12]) break;

			max0 = 0;
			for(j= 0;j<4;j++)
				if(sacc0[j]<(int)  pstOvpObj->m_stDiFwReg.FmdThr[11])
					max0 = max0 + 3;

			max1 = 0;
			for(j= 4;j<8;j++)
				if(sacc0[j]<(int)  pstOvpObj->m_stDiFwReg.FmdThr[11])
					max1 = max1 + 2;

			max = (max0 + max1)/2;
			if(max>(int)  pstOvpObj->m_stDiFwReg.FmdThr[12]) break;

			max0 = 0;
			for(j= 0;j<4;j++)
				if(sacc1[j]<(int)  pstOvpObj->m_stDiFwReg.FmdThr[11])
					max0 = max0 + 3;

			max1 = 0;
			for(j= 4;j<8;j++)
				if(sacc1[j]<(int)  pstOvpObj->m_stDiFwReg.FmdThr[11])
					max1 = max1 + 2;

			max = (max0 + max1)/2;
			if(max>(int)  pstOvpObj->m_stDiFwReg.FmdThr[12]) break;

		}

		pre_divider_i = (UINT16) divider_i;

		/* graphic mode patch */
		if(rpd == 2)
			divider_i = (UINT16) (i +  pstOvpObj->m_stDiFwReg.FmdOff[10]);
		else
			divider_i = (UINT16) i;


		/* ***********************this is used for MADI only */
		max0 = 0;
		max1 = 0;
		for(i= 0;i<8;i++) {
			if(macc0[i]>0)
				max0 = max0 + 1;
			if(macc1[i]>0)
				max1 = max1 + 1;
		}

		max = (max0 + max1);
		if(max> pstOvpObj->m_stDiFwReg.FmdThr[9])
			max = pstOvpObj->m_stDiFwReg.FmdThr[9];

		/* temporal IIR filter */
		if((pstOvpObj->m_stDiFwIData.FrameReset)||(divider_i> pstOvpObj->m_stDiFwReg.FmdThr[8]))
			pre_x = 4* pstOvpObj->m_stDiFwReg.FmdThr[9];
		else
			pre_x = pstOvpObj->m_stDiFwIData.FMDState[1];

		/* iir state and output filters (used for MADI) */
		cur_x = (3*pre_x + 4*8*max)/4;
		pstOvpObj->m_stDiFwIData.FMDState[1] = cur_x;
		max = (cur_x + pre_x + 32)/64;
		if(max<(int)  pstOvpObj->m_stDiFwReg.FmdThr[6])
			pstOvpObj->m_stDiFwIData.Fmd_still = 1;
		else
			pstOvpObj->m_stDiFwIData.Fmd_still = 0;
		/* ************************************ */
		min0 = 0;
		min1 = 0;
		for(i= 0;i<8;i++) {
			min0 = min0 + macc0[i];
			min1 = min1 + macc1[i];
		}
		min0 = min0/8;
		min1 = min1/8;

		if(abs(min1-min0)<(int) fmdthr_dif)
			sel = 0;
		else if((min0<fmdthr_lo)&&(min1<fmdthr_lo))
			sel = 0;
		else if(max1<max0)
			sel = 1;
		else
			sel = 0;

		fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[2];

		if((min0>fmdthr_hi)&&(min1>fmdthr_hi))
			gstill = 0;
		else if(abs(min1-min0)<(int) fmdthr_dif)
			gstill = 3;
		else if((min0<fmdthr_lo)&&(min1<fmdthr_lo))
			gstill = 3;
		else if((fmdgain*(max0+1))/4<max1)
			gstill = 1;
		else if(max0>(fmdgain*(max1+1))/4)
			gstill = 2;
		else
			gstill = 4;

		/* temporal IIR filter */
		if((pstOvpObj->m_stDiFwIData.FrameReset)) {
			if((pre_divider_i == 0)&&(divider_i==0))
				pre_x = 4*8* pstOvpObj->m_stDiFwReg.FmdThr[7];
			else if(pre_divider_i>(divider_i+ pstOvpObj->m_stDiFwReg.FmdThr[0]))
				pre_x = 4*8*pre_divider_i;
			else
				pre_x = 4*8*divider_i;
		}
		else
			pre_x = pstOvpObj->m_stDiFwIData.FMDState[0];

		/* iir state and output filters */
		if((divider_i< pstOvpObj->m_stDiFwReg.FmdThr[0])&&(pre_divider_i< pstOvpObj->m_stDiFwReg.FmdThr[0]))
			cur_x = (3*pre_x + 4*8*( pstOvpObj->m_stDiFwReg.FmdThr[0]-1) + 2)/4;
		else if(pre_divider_i>(divider_i+ pstOvpObj->m_stDiFwReg.FmdThr[0]))
			cur_x = (3*pre_x + 4*8*pre_divider_i + 2)/4;
		else
			cur_x = (3*pre_x + 4*8*divider_i + 2)/4;
		min = (UINT16) (cur_x + pre_x + 32)/64;

		if((pre_divider_i>0)||(divider_i>0)||(!lock))
			pstOvpObj->m_stDiFwIData.FMDState[0] = cur_x;

		if((rpd == 2)&&(osd>0))
			local_lock = 2;
		else if(osd == 1)
			local_lock = 2;
		else if(rpd>0)
			local_lock = 1;
		else if(min> pstOvpObj->m_stDiFwReg.FmdThr[4])
			local_lock = 1;
		else
			local_lock = 0;

		if(min> pstOvpObj->m_stDiFwReg.FmdThr[2]) {
			lcnst = 3;
			if(local_lock>0)
			{
				if(lock)
					fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[3];
				else
					fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[2];
				if(local_lock == 1)
					fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[9];
				else
					fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[9]* pstOvpObj->m_stDiFwReg.FmdOff[9]* pstOvpObj->m_stDiFwReg.FmdOff[9];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[8];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[1];
			}
			else if(lock22||lock32)
			{
				fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[4];
				fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[3];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[12];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[3];
			}
			else
			{
				fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[2];
				fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[3];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[7];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[0];
			}
		} else if(min> pstOvpObj->m_stDiFwReg.FmdThr[1]) {
			lcnst = 2;
			if(local_lock>0) {
				if(lock)
					fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[3];
				else
					fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[2];
				if(local_lock == 1)
					fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[9];
				else
					fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[9]* pstOvpObj->m_stDiFwReg.FmdOff[9]* pstOvpObj->m_stDiFwReg.FmdOff[9];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[8];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[1];
			} else if(lock22||lock32) {
				fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[4];
				fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[2];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[12];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[3];
			} else {
				fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[2];
				fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[2];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[6];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[0];
			}
		} else if(min> pstOvpObj->m_stDiFwReg.FmdThr[0]) {
			lcnst = 1;
			if(local_lock>0) {
				if(lock)
					fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[3];
				else
					fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[2];
				if(local_lock == 1)
					fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[9];
				else
					fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[9]* pstOvpObj->m_stDiFwReg.FmdOff[9]* pstOvpObj->m_stDiFwReg.FmdOff[9];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[8];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[1];
			} else if(lock22||lock32) {
				fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[4];
				fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[1];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[12];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[3];
			} else {
				fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[2];
				fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[1];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[5];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[0];
			}
		} else {
			lcnst = 0;
			if(local_lock>0) {
				if(lock)
					fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[3];
				else
					fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[2];
				if(local_lock == 1)
					fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[9];
				else
					fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[9]* pstOvpObj->m_stDiFwReg.FmdOff[9]* pstOvpObj->m_stDiFwReg.FmdOff[9];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[8];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[1];
			} else if(lock22||lock32) {
				fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[4];
				fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[0];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[12];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[3];
			} else {
				fmdcountthr = pstOvpObj->m_stDiFwReg.FmdCountThr[2];
				fmdmoff = pstOvpObj->m_stDiFwReg.FmdOff[0];
				fmdsoff = pstOvpObj->m_stDiFwReg.FmdOff[4];
				fmdgain = pstOvpObj->m_stDiFwReg.FmdGain[0];
			}
		}

		/* global high-frequency adaptation */
		for(i= 0;i<4;i++) {
			macc0[i] = macc0[i] - fmdmoff;
			if(macc0[i]<0)
				macc0[i] = 0;

			macc1[i] = macc1[i] - fmdmoff;
			if(macc1[i]<0)
				macc1[i] = 0;

			sacc0[i] = sacc0[i] - fmdsoff;
			if(sacc0[i]<0)
				sacc0[i] = 0;
			sacc1[i] = sacc1[i]- fmdsoff;
			if(sacc1[i]<0)
				sacc1[i] = 0;

			if((macc0[i]*(int)fmdgain)<sacc0[i])
				still01[i] = 1;
			else
				still01[i] = 0;

			if((macc1[i]*(int)fmdgain)<sacc1[i])
				still21[i] = 1;
			else
				still21[i] = 0;
		}

		min0 = 255;
		min1 = 255;
		for(i= 4;i<8;i++) {
			if(macc0[i]<(int) min0)
				min0 = macc0[i];
			if(macc1[i]<(int) min1)
				min1 = macc1[i];
		}

		if(! pstOvpObj->m_stDiFwReg.FmdScrTxtSel) {
			min0 = 0;
			min1 = 0;
		}

		for(i= 4;i<8;i++) {
			if((!macc0[0])&&(!macc0[1])&&(!macc0[2])&&(!macc0[3]))
				macc0[i] = macc0[i] - min0 - fmdmoff;
			else
				macc0[i] = macc0[i] - fmdmoff;
			if(macc0[i]<0)
				macc0[i] = 0;

			if((!macc1[0])&&(!macc1[1])&&(!macc1[2])&&(!macc1[3]))
				macc1[i] = macc1[i] - min1 - fmdmoff;
			else
				macc1[i] = macc1[i] - fmdmoff;
			if(macc1[i]<0)
				macc1[i] = 0;

			sacc0[i] = sacc0[i] - fmdsoff;
			if(sacc0[i]<0)
				sacc0[i] = 0;
			sacc1[i] = sacc1[i]- fmdsoff;
			if(sacc1[i]<0)
				sacc1[i] = 0;

			if((macc0[i]*(int)fmdgain)<sacc0[i])
				still01[i] = 1;
			else
				still01[i] = 0;

			if((macc1[i]*(int)fmdgain)<sacc1[i])
				still21[i] = 1;
			else
				still21[i] = 0;
		}

		still0 = 0;
		still1 = 0;
		for(i= 4;i<8;i++) {
			still0 = still0 + still01[i];
			still1 = still1 + still21[i];
		}

		if((still0> pstOvpObj->m_stDiFwReg.FmdOff[11])&&still01[0]&&still01[1]&&still01[2]&&still01[3])
			still0 = 1;
		else
			still0 = 0;

		if((still1> pstOvpObj->m_stDiFwReg.FmdOff[11])&&still21[0]&&still21[1]&&still21[2]&&still21[3])
			still1 = 1;
		else
			still1 = 0;

		/* still count detection */
		if(pstOvpObj->m_stDiFwIData.FrameReset) {
			st_rst = 0;
			st_fcount = 0;
		}
		else if((still0)&&still1&&(gstill == 3)) {
			if(st_fcount<fmdcountthr)
			{
				st_fcount = st_fcount + 1;
				st_rst = 0;
			}
			else
			{
				st_rst = 1;
				st_fcount = 0;
			}
		} else {
			st_rst = 0;
			st_fcount = 0;
		}

		/* fmd lock reset logic */
		if((pstOvpObj->m_stDiFwIData.FrameReset)) {
			lock_count = 0;
			reset_lock = 0;
		} else if (lock) {
			if(lock_count<4)
				lock_count = lock_count + 1;
			reset_lock = 0;
		} else if(lock_count == 4) {
			lock_count = 0;
			reset_lock = 1;
		} else {
			lock_count = 0;
			reset_lock = 0;
		}

		/* 22 pulldown detection */
		if(pstOvpObj->m_stDiFwIData.FrameReset||lock32||reset_lock||st_rst) {
			fcount22_0 = 0;
			fcount22_1 = 0;
			sel22 = 0;
			lock22 = 0;
		} else if((!still0)&&(!still1)) {
			/* all motion */
			fcount22_0 = 0;
			fcount22_1 = 0;
			sel22 = 0;
			lock22 = 0;
		} else if((still0)&&(!still1)) {
			fcount22_1 = fcount22_1/2;
			if(fcount22_0< pstOvpObj->m_stDiFwReg.FmdCountThr[1])
			{
				fcount22_0= fcount22_0 + 1;
				lock22 = 0;
			}
			else
			{
				sel22 = 1;
				lock22 = 1;
			}
		} else if((!still0)&&(still1)) {
			fcount22_0 = fcount22_0/2;
			sel22 = 0;
			if(fcount22_1< pstOvpObj->m_stDiFwReg.FmdCountThr[1])
			{
				fcount22_1= fcount22_1 + 1;
				lock22 = 0;
			}
			else
				lock22 = 1;
		} else {
			if((gstill == 1)&&(!lock22)) {
				fcount22_1 = fcount22_1/2;
				if(fcount22_0< pstOvpObj->m_stDiFwReg.FmdCountThr[1])
				{
					fcount22_0= fcount22_0 + 1;
					lock22 = 0;
				}
				else
				{
					sel22 = 1;
					lock22 = 1;
				}
			} else if((gstill == 2)&&(!lock22)) {
				fcount22_0 = fcount22_0/2;
				sel22 = 0;
				if(fcount22_1< pstOvpObj->m_stDiFwReg.FmdCountThr[1])
				{
					fcount22_1= fcount22_1 + 1;
					lock22 = 0;
				}
				else
					lock22 = 1;
			} else if((gstill == 0)&&(!lock22)) {
				fcount22_0 = 0;
				fcount22_1 = 0;
				sel22 = 0;
				lock22 = 0;
			}
		}

		/* 22 reset logic */
		if((pstOvpObj->m_stDiFwIData.FrameReset)||(!lock)) {
			lock22_count = 0;
			reset22 = 0;
		} else if (lock22) {
			if(lock22_count<4)
				lock22_count = lock22_count + 1;
			reset22 = 0;
		} else if(lock22_count == 4) {
			lock22_count = 5;
			reset22 = 0;
		} else if(lock22_count == 5) {
			lock22_count = 6;
			reset22 = 0;
		} else if(lock22_count == 6) {
			lock22_count = 0;
			reset22 = 1;
		} else {
			lock22_count = 0;
			reset22 = 0;
		}

		/* 32 pulldown detection */
		if(pstOvpObj->m_stDiFwIData.FrameReset||lock22||reset_lock||st_rst)
		{
			fcount32 = 0;
			lock32 = 0;
			lock32_temp = 0;
		}
		/* all motion */
		else if((!still0)&&(!still1))
		{
			fcount32 = 0;
			lock32 = 0;
			lock32_temp = 0;
		}
		else if((still0)&&(!still1))
		{
			if(fcount32 == 1)
				fcount32= 2;
			else if(fcount32 == 2)
				fcount32= 3;
			else
			{
				lock32_temp = 0;
				lock32 = 0;
				fcount32= 0;
			}
		}
		else if((!still0)&&(still1))
		{
			if(fcount32 == 3)
				fcount32= 4;
			else if(fcount32 == 4)
			{
				if(lock32_temp == 1)
					lock32 = 1;
				lock32_temp = 1;
				fcount32= 0;
			}
			else
			{
				lock32_temp = 0;
				lock32 = 0;
				fcount32= 0;
			}
		} else {
			if(lock32 == 1) {
				if(fcount32 == 4)
					fcount32 = 0;
				else
					fcount32 = fcount32 + 1;
			} else {
				if((fcount32 == 0)&&(gstill==3))
					fcount32 = 1;
				else if((fcount32 == 1)&&(gstill==1))
					fcount32 = 2;
				else if((fcount32 == 2)&&(gstill==1))
					fcount32 = 3;
				else if((fcount32 == 3)&&(gstill==2))
					fcount32 = 4;
				else if((fcount32 == 4)&&(gstill==2)) {
					if(lock32_temp == 1)
						lock32 = 1;
					lock32_temp = 1;
					fcount32 = 0;
				} else {
					fcount32 = 1;
					lock32_temp = 0;
					lock32 = 0;
				}
			}
		}

		/* 32 reset logic */
		if(pstOvpObj->m_stDiFwIData.FrameReset||(!lock)) {
			lock32_count = 0;
			reset32 = 0;
		} else if (lock32) {
			if(lock32_count<4)
				lock32_count = lock32_count + 1;
			reset32 = 0;
		} else if(lock32_count == 4) {
			lock32_count = 5;
			reset32 = 0;
		} else if(lock32_count == 5) {
			lock32_count = 6;
			reset32 = 0;
		} else if(lock32_count == 6) {
			lock32_count = 0;
			reset32 = 1;
		} else {
			lock32_count = 0;
			reset32 = 0;
		}

		/* film mode lock */
		if(pstOvpObj->m_stDiFwIData.FrameReset||reset22|reset32||st_rst) {
			lock = 0;
			fcount = 0;
		} else if((!still0)&&(!still1)) {
			/* all motion */
			fcount = fcount/2;
			lock = 0;
		} else if((still0)&&(!still1)) {
			if(fcount< pstOvpObj->m_stDiFwReg.FmdCountThr[0]) {
				fcount = fcount + 1;
				lock = 0;
			} else
				lock = 1;
		} else if((!still0)&&(still1)) {
			if(fcount< pstOvpObj->m_stDiFwReg.FmdCountThr[0]) {
				fcount = fcount + 1;
				lock = 0;
				ph = 0;
			} else
				lock = 1;
		} else {
			if(fcount< pstOvpObj->m_stDiFwReg.FmdCountThr[0]) {
				lock = 0;
				if(gstill == 0)
					fcount = 0;
				else if(gstill == 1)
					fcount = fcount + 1;
				else if(gstill == 2)
					fcount = fcount + 1;
			} else
				lock = 1;
		}

		/* reset mode */
		if(pstOvpObj->m_stDiFwIData.FrameReset)
			ph = 0;
		/* all motion */
		else if((!still0)&&(!still1))
			ph = 0;
		else if((sel22 == 1)&&(lock22))
			ph = 2;
		else if((sel22 == 0)&&(lock22))
			ph = 0;
		else if(lock32 == 1) {
			if(fcount32 == 1)
				ph = 2;
			else if(fcount32 == 2)
				ph = 2;
			else if(fcount32 == 3)
				ph = 3;
			else
				ph = 0;
		}
		else if((still0)&&(!still1))
			ph = 1;
		else if((!still0)&&(still1))
			ph = 0;
		else {
			if(sel == 1)
				ph = 0;
			else
				ph = 1;
		}
	} else {
		local_lock = 0;
		fcount = 0;
		pstOvpObj->m_stDiFwIData.Fmd_still = 0;
		lcnst = 0;
		lock = pstOvpObj->m_stDiFwReg.FmdUser%2;
		ph = pstOvpObj->m_stDiFwReg.FmdUser/2;
	}
	pstOvpObj->m_stDiFwIData.fmd = (local_lock/2)*64 + lock*32 + ph*8 + 4*pstOvpObj->m_stDiFwIData.Fmd_still + lcnst;
	if((fcount> pstOvpObj->m_stDiFwReg.FmdMcdiBypsThr)&&(!lock))
		pstOvpObj->m_stDiFwIData.mcdibyps = 1;
	else
		pstOvpObj->m_stDiFwIData.mcdibyps = 0;
}

/*******************************************************************************
Function:    glmv_det()
Arguments:   None
Returns:     None
Description: global large motion vector detection
 ********************************************************************************/
void glmv_det(OVP_OBJ *pstOvpObj)
{
	UINT32 lmv_acc, glmvthr_lo, glmvthr_hi;
	int cur_x, pre_x;
	static int out_y = 0;

	glmvthr_lo = pstOvpObj->m_stDiFwReg.FmdGlmvThr[0];
	glmvthr_hi = pstOvpObj->m_stDiFwReg.FmdGlmvThr[1];

	/* global high-frequency detection */
	if(pstOvpObj->m_stDiFwIData.Fmd_lgmv_tacc<8192)
		lmv_acc = (pstOvpObj->m_stDiFwIData.Fmd_lgmv_acc<<7)/8192;    //(pDiHwOpt->Width+1)/(pDiHwOpt->Height+1);
	else
		lmv_acc = (pstOvpObj->m_stDiFwIData.Fmd_lgmv_acc<<7)/pstOvpObj->m_stDiFwIData.Fmd_lgmv_tacc;   //(pDiHwOpt->Width+1)/(pDiHwOpt->Height+1);


	/* temporal IIR filter */
	if((pstOvpObj->m_stDiFwIData.FrameReset)||((UINT32)abs(lmv_acc-out_y)>(lmv_acc+out_y)/2))
		pre_x = 4*lmv_acc;
	else
		pre_x = pstOvpObj->m_stDiFwIData.GLMVState;

	/* iir state and output filters */
	cur_x = (3*pre_x + 4*lmv_acc + 2)/4;

	pstOvpObj->m_stDiFwIData.GLMVState = cur_x;
	out_y = (cur_x + pre_x)/8;

	if(out_y>(int) glmvthr_hi)
		pstOvpObj->m_stDiFwIData.glmv = 2;
	else if(out_y>(int) glmvthr_lo)
		pstOvpObj->m_stDiFwIData.glmv = 1;
	else
		pstOvpObj->m_stDiFwIData.glmv = 0;

	/* manu mode */
	if((! pstOvpObj->m_stDiFwReg.FmdLmvEn)||(pstOvpObj->m_stDiFwIData.FrameReset))
		pstOvpObj->m_stDiFwIData.glmv = pstOvpObj->m_stDiFwReg.FmdLmvUser;

}

/*******************************************************************************
Function:    blamd_det()
Arguments:   None
Returns:     None
Description: block-base (16x16) motion detection
 ********************************************************************************/
void blamd_det(OVP_OBJ *pstOvpObj)
{
	UINT32 j, i;
	int cur_x, pre_x, out_y;
	UINT32 blamd_thr;

	T32MadiReg_ShiftBit	m_stMadiRegShiftBit;
	m_stMadiRegShiftBit.u32 = pstOvpObj->m_stMadiReg.m_uiMadiRegShiftBit;

	blamd_thr = pstOvpObj->m_stDiFwReg.LAMDCountGain*((pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize / HBLKNUM)* (pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize/VBLKNUM))/256;

	for (j = 0;j <16; j++)
		for (i = 0; i <16; i++) {
			/* temporal IIR filter */
			if(pstOvpObj->m_stDiFwIData.FrameReset)
				pre_x = 0;
			else
				pre_x = pstOvpObj->m_stDiFwIData.bmdv_state[j][i];

			/* iir state and output filters */
			if(pstOvpObj->m_stDiFwIData.FrameReset)
				cur_x = 0;
			else
				cur_x = (pre_x + 2*(pstOvpObj->m_stDiFwIData.bmdv[j][i]<<m_stMadiRegShiftBit.uShiftBit_BlamdShiftBit)+1)/2;

			pstOvpObj->m_stDiFwIData.bmdv_state[j][i] = (UINT16) cur_x;
			out_y = (cur_x + pre_x)/4;

			if(! pstOvpObj->m_stDiFwReg.BLAMDEn)
				pstOvpObj->m_stDiFwIData.blamd[j][i] = pstOvpObj->m_stDiFwReg.BLAMDUser;
			else if(out_y>(int) blamd_thr)
				pstOvpObj->m_stDiFwIData.blamd[j][i] = 1;
			else
				pstOvpObj->m_stDiFwIData.blamd[j][i] = 0;
		}
}

/*******************************************************************************
Function:    blasd_det()
Arguments:   None
Returns:     None
Description: block-base (16x16) static detection
 ********************************************************************************/
void blasd_det(OVP_OBJ *pstOvpObj)
{
	UINT32 j, i;
	int cur_x, pre_x, out_y;
	int blasd_thr, block_pixel;
	int static_pixel;

	T32MadiReg_ShiftBit	m_stMadiRegShiftBit;
	m_stMadiRegShiftBit.u32 = pstOvpObj->m_stMadiReg.m_uiMadiRegShiftBit;

	block_pixel = ((pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize / HBLKNUM)* (pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize/VBLKNUM));
	blasd_thr = pstOvpObj->m_stDiFwReg.LASDCountThr*block_pixel/256;

	for (j = 0;j <16; j++)
		for (i = 0; i <16; i++) {
			static_pixel = block_pixel - (pstOvpObj->m_stDiFwIData.bstc[j][i]<<m_stMadiRegShiftBit.uShiftBit_BlasdShiftBit);
			/* temporal IIR filter */
			if(pstOvpObj->m_stDiFwIData.FrameReset)
				pre_x = 2*block_pixel;
			else
				pre_x = pstOvpObj->m_stDiFwIData.bstc_state[j][i];

			/* iir state and output filters */
			if(pstOvpObj->m_stDiFwIData.FrameReset)
				cur_x = (pre_x + 2*block_pixel+1)/2;
			else
				cur_x = (pre_x + 2*static_pixel+1)/2;

			pstOvpObj->m_stDiFwIData.bstc_state[j][i] = (UINT16) cur_x;
			out_y = (cur_x + pre_x)/4;

			if(! pstOvpObj->m_stDiFwReg.BLASDEn)
				pstOvpObj->m_stDiFwIData.blasd[j][i] = pstOvpObj->m_stDiFwReg.BLASDUser;
			else if(out_y>(int) blasd_thr)
				pstOvpObj->m_stDiFwIData.blasd[j][i] = 1;
			else
				pstOvpObj->m_stDiFwIData.blasd[j][i] = 0;
		}
}

/*******************************************************************************
Function:    bsosd_det()
Arguments:   None
Returns:     None
Description: block-base (16x16) static osd detection
 ********************************************************************************/
void bsosd_det(OVP_OBJ *pstOvpObj)
{
	UINT32 j, i;
	int cur_x, pre_x, out_y;
	UINT32 bsosd_thr;

	T32MadiReg_ShiftBit	m_stMadiRegShiftBit;
	m_stMadiRegShiftBit.u32 = pstOvpObj->m_stMadiReg.m_uiMadiRegShiftBit;

	bsosd_thr = pstOvpObj->m_stDiFwReg.SOSDCountThr*((pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize/HBLKNUM)* (pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize/VBLKNUM))/256;

	for (j = 0;j <16; j++)
		for (i = 0; i <16; i++) {
			/* temporal IIR filter */
			if(pstOvpObj->m_stDiFwIData.FrameReset)
				pre_x = 0;
			else
				pre_x = pstOvpObj->m_stDiFwIData.bosd_state[j][i];

			/* iir state and output filters */
			cur_x = (pre_x + 2*(pstOvpObj->m_stDiFwIData.bosd[j][i]<<m_stMadiRegShiftBit.uShiftBit_BsosdShiftBit)+1)/2;
			pstOvpObj->m_stDiFwIData.bosd_state[j][i] = (UINT16) cur_x;
			out_y = (cur_x + pre_x)/4;
			if(! pstOvpObj->m_stDiFwReg.BSOSDEn)
				pstOvpObj->m_stDiFwIData.bsosd[j][i] = pstOvpObj->m_stDiFwReg.BSOSDUser;
			else if(out_y>(int) bsosd_thr)
				pstOvpObj->m_stDiFwIData.bsosd[j][i] = 1;
			else
				pstOvpObj->m_stDiFwIData.bsosd[j][i] = 0;
		}
}

/*******************************************************************************
Function:    blmhf_det()
Arguments:   None
Returns:     None
Description: block-base (16x16) motion high-frequency detection
 ********************************************************************************/
void blmhf_det(OVP_OBJ *pstOvpObj)
{
	UINT32 j, i;
	int cur_x, pre_x, out_y;
	UINT32 bmhf_thr;

	T32MadiReg_ShiftBit	m_stMadiRegShiftBit;
	m_stMadiRegShiftBit.u32 = pstOvpObj->m_stMadiReg.m_uiMadiRegShiftBit;

	bmhf_thr = pstOvpObj->m_stDiFwReg.LMHFCountThr*((pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize/HBLKNUM)* (pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize/VBLKNUM))/256;

	for (j = 0;j <16; j++)
		for (i = 0; i <16; i++) {
			/* temporal IIR filter */
			if(pstOvpObj->m_stDiFwIData.FrameReset)
				pre_x = 2*pstOvpObj->m_stDiFwIData.bmhf[j][i];
			else
				pre_x = pstOvpObj->m_stDiFwIData.bmhf_state[j][i];

			/* iir state and output filters */
			cur_x = (pre_x + 2*(pstOvpObj->m_stDiFwIData.bmhf[j][i]<<m_stMadiRegShiftBit.uShiftBit_BlmhfShiftBit) + 1)/2;
			pstOvpObj->m_stDiFwIData.bmhf_state[j][i] = (UINT16) cur_x;
			out_y = (cur_x + pre_x)/4;

			if(! pstOvpObj->m_stDiFwReg.BLMHFEn)
				pstOvpObj->m_stDiFwIData.blmhf[j][i] = pstOvpObj->m_stDiFwReg.BLMHFUser;
			else if(out_y>(int) bmhf_thr)
				pstOvpObj->m_stDiFwIData.blmhf[j][i] = 1;
			else
				pstOvpObj->m_stDiFwIData.blmhf[j][i] = 0;
		}
}

/*******************************************************************************
Function:    afd_frame_delay()
Arguments:   None
Returns:     None
Description: delay the input from HW for 4 frames and send back to HW
 ********************************************************************************/
void afd_frame_delay(OVP_OBJ *pstOvpObj)
{
	int i;
	for (i= 0; i<4; i++) {
		pstOvpObj->m_stDiFwIData.AFDF3VPosDelay[i] = pstOvpObj->m_stDiFwIData.AFDF2VPosDelay[i];
		pstOvpObj->m_stDiFwIData.AFDF2VPosDelay[i] = pstOvpObj->m_stDiFwIData.AFDF1VPosDelay[i];
		pstOvpObj->m_stDiFwIData.AFDF1VPosDelay[i] = pstOvpObj->m_stDiFwIData.AFDF0VPosDelay[i];
	}
}

/*******************************************************************************
Function:    diReadDataFromHw
Arguments:   None
Returns:     None
Description: read all HW data from DRAM
 ********************************************************************************/
void diReadDataFromHw(OVP_OBJ *pstOvpObj)
{
	pstOvpObj->m_stDiFwIData.ghf_acc = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_ghf_acc;
	pstOvpObj->m_stDiFwIData.glamd_acc = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_glamd_acc;

	pstOvpObj->m_stDiFwIData.gosd_acc = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_gosd_acc;
	pstOvpObj->m_stDiFwIData.grpd_acc = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_grpd_acc;

	pstOvpObj->m_stDiFwIData.gmdv_acc = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_gmdv_acc;

	pstOvpObj->m_stDiFwIData.gmdv_max = pstOvpObj->m_stDiFwIData.gmdv_acc/256;
	pstOvpObj->m_stDiFwIData.gmdv_min = pstOvpObj->m_stDiFwIData.gmdv_acc%256;


	pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[0] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_0;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[1] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_1;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[2] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_2;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[3] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_3;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[4] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_4;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[5] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_5;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[6] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_6;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_macc[7] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_7;

	pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[0] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_0;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[1] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_1;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[2] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_2;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[3] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_3;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[4] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_4;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[5] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_5;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[6] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_6;
	pstOvpObj->m_stDiFwIData.Fmd_f0f1_sacc[7] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_7;

	pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[0] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_0;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[1] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_1;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[2] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_2;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[3] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_3;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[4] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_4;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[5] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_5;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[6] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_6;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_macc[7] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_7;

	pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[0] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_0;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[1] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_1;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[2] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_2;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[3] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_3;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[4] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_4;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[5] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_5;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[6] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_6;
	pstOvpObj->m_stDiFwIData.Fmd_f2f1_sacc[7] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_7;

	pstOvpObj->m_stDiFwIData.AFDF0VPosDelay[0] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_AFDF0VPosDelay_0;
	pstOvpObj->m_stDiFwIData.AFDF0VPosDelay[1] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_AFDF0VPosDelay_1;
	pstOvpObj->m_stDiFwIData.AFDF0VPosDelay[2] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_AFDF0VPosDelay_2;
	pstOvpObj->m_stDiFwIData.AFDF0VPosDelay[3] = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_AFDF0VPosDelay_3;

	pstOvpObj->m_stDiFwIData.Fmd_lgmv_acc = pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_lgmv_acc;


#ifdef DEBUG_DUMP_OVP_REGS
	OVP_DBG_INFO("\n/************************************************************************/\n");
	OVP_DBG_INFO("\n/*****************\t\t DEINT STATS \t\t****************/\n");
	OVP_DBG_INFO("\n/************************************************************************/\n");
	OVP_DBG_INFO("\tghf_acc\t\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_ghf_acc);
	OVP_DBG_INFO("\tglamd_acc\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_glamd_acc);
	OVP_DBG_INFO("\tgrpd_acc\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_grpd_acc);
	OVP_DBG_INFO("\tgmdv_acc\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_gmdv_acc);
	OVP_DBG_INFO("\tgosd_acc\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntStat.u_gosd_acc);
	OVP_DBG_INFO("\n/************************************************************************/\n");

	OVP_DBG_INFO("\n/************************************************************************/\n");
	OVP_DBG_INFO("\n/*****************\t\t FMD STATS \t\t******************/\n");
	OVP_DBG_INFO("\n/************************************************************************/\n");
	OVP_DBG_INFO("\tfmd_lgmv_acc\t\t= \t0x%x\n",   pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_lgmv_acc);
	OVP_DBG_INFO("\tfmd_f0f1_macc_0\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_0);
	OVP_DBG_INFO("\tfmd_f0f1_macc_1\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_1);
	OVP_DBG_INFO("\tfmd_f0f1_macc_2\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_2);
	OVP_DBG_INFO("\tfmd_f0f1_macc_3\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_3);
	OVP_DBG_INFO("\tfmd_f0f1_macc_4\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_4);
	OVP_DBG_INFO("\tfmd_f0f1_macc_5\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_5);
	OVP_DBG_INFO("\tfmd_f0f1_macc_6\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_6);
	OVP_DBG_INFO("\tfmd_f0f1_macc_7\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_macc_7);

	OVP_DBG_INFO("\tfmd_f0f1_sacc_0\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_0);
	OVP_DBG_INFO("\tfmd_f0f1_sacc_1\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_1);
	OVP_DBG_INFO("\tfmd_f0f1_sacc_2\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_2);
	OVP_DBG_INFO("\tfmd_f0f1_sacc_3\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_3);
	OVP_DBG_INFO("\tfmd_f0f1_sacc_4\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_4);
	OVP_DBG_INFO("\tfmd_f0f1_sacc_5\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_5);
	OVP_DBG_INFO("\tfmd_f0f1_sacc_6\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_6);
	OVP_DBG_INFO("\tfmd_f0f1_sacc_7\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f0f1_sacc_7);

	OVP_DBG_INFO("\tfmd_f2f1_macc_0\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_0);
	OVP_DBG_INFO("\tfmd_f2f1_macc_1\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_1);
	OVP_DBG_INFO("\tfmd_f2f1_macc_2\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_2);
	OVP_DBG_INFO("\tfmd_f2f1_macc_3\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_3);
	OVP_DBG_INFO("\tfmd_f2f1_macc_4\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_4);
	OVP_DBG_INFO("\tfmd_f2f1_macc_5\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_5);
	OVP_DBG_INFO("\tfmd_f2f1_macc_6\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_6);
	OVP_DBG_INFO("\tfmd_f2f1_macc_7\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_macc_7);

	OVP_DBG_INFO("\tfmd_f2f1_sacc_0\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_0);
	OVP_DBG_INFO("\tfmd_f2f1_sacc_1\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_1);
	OVP_DBG_INFO("\tfmd_f2f1_sacc_2\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_2);
	OVP_DBG_INFO("\tfmd_f2f1_sacc_3\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_3);
	OVP_DBG_INFO("\tfmd_f2f1_sacc_4\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_4);
	OVP_DBG_INFO("\tfmd_f2f1_sacc_5\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_5);
	OVP_DBG_INFO("\tfmd_f2f1_sacc_6\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_6);
	OVP_DBG_INFO("\tfmd_f2f1_sacc_7\t\t= \t0x%x\n",	  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_f2f1_sacc_7);

	OVP_DBG_INFO("\tfmd_AFDF0VPosDelay_0\t= \t0x%x\n",    pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_AFDF0VPosDelay_0);
	OVP_DBG_INFO("\tfmd_AFDF0VPosDelay_1\t= \t0x%x\n",    pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_AFDF0VPosDelay_1);
	OVP_DBG_INFO("\tfmd_AFDF0VPosDelay_2\t= \t0x%x\n",    pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_AFDF0VPosDelay_2);
	OVP_DBG_INFO("\tfmd_AFDF0VPosDelay_3\t= \t0x%x\n",    pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_FmdStat.u_fmd_AFDF0VPosDelay_3);

	OVP_DBG_INFO("\n/************************************************************************/\n");
#endif

}

/*******************************************************************************
Function:    diSendDataToHw
Arguments:   None
Returns:     None
Description: send all FW registers to DRAM
 ********************************************************************************/
void diSendDataToHw(OVP_OBJ *pstOvpObj)
{
	int i;
	UINT32 SdiGain[7];
	UINT32 SdiOff[7];
	UINT32 rpd;

	pstOvpObj->m_stDiFwIData.grpd = pstOvpObj->m_stDiFwIData.grpd/4;
	rpd = pstOvpObj->m_stDiFwIData.grpd%4;

	if(pstOvpObj->m_stDiFwIData.ghf == 1){
		for(i= 0; i<4; i++) {
			pstOvpObj->m_stDiFwIData.YMdvThr[i] = pstOvpObj->m_stDiFwReg.YMdvGhfThr[i];
			pstOvpObj->m_stDiFwIData.UVMdvThr[i] = pstOvpObj->m_stDiFwReg.UVMdvGhfThr[i];
		}
	} else {
		for(i= 0; i<4; i++) {
			if(pstOvpObj->m_stDiFwIData.glmv == 2)
				pstOvpObj->m_stDiFwIData.YMdvThr[i] = pstOvpObj->m_stDiFwReg.YMdvGlmvThr[i];
			else if((pstOvpObj->m_stDiFwIData.C420Fix_yacc > pstOvpObj->m_stDiFwReg.C420AccThr[2]) &&
					pstOvpObj->m_stDiFwIData.ChromaUpSampleError)
				pstOvpObj->m_stDiFwIData.YMdvThr[i] = pstOvpObj->m_stDiFwReg.YMdvGvhfThr[i];
			else
				pstOvpObj->m_stDiFwIData.YMdvThr[i] = pstOvpObj->m_stDiFwReg.YMdvThr[i];


			if((pstOvpObj->m_stDiFwIData.C420Fix_acc > pstOvpObj->m_stDiFwReg.C420AccThr[1]) &&
					pstOvpObj->m_stDiFwIData.ChromaUpSampleError)
				pstOvpObj->m_stDiFwIData.UVMdvThr[i] = pstOvpObj->m_stDiFwReg.UVMdvGvhfThr[i];
			else
				pstOvpObj->m_stDiFwIData.UVMdvThr[i] = pstOvpObj->m_stDiFwReg.UVMdvThr[i];
		}
	}


	if(rpd == 0) {
		pstOvpObj->m_stDiFwIData.AngHsgrmThr[0] = pstOvpObj->m_stDiFwReg.AngHsgrmThr[0];
		pstOvpObj->m_stDiFwIData.AngHsgrmThr[1] = pstOvpObj->m_stDiFwReg.AngHsgrmThr[1];
	} else {
		pstOvpObj->m_stDiFwIData.AngHsgrmThr[0] = pstOvpObj->m_stDiFwReg.AngHsgrmThr[2];
		pstOvpObj->m_stDiFwIData.AngHsgrmThr[1] = pstOvpObj->m_stDiFwReg.AngHsgrmThr[3];
	}

	if(rpd == 0) {
		pstOvpObj->m_stDiFwIData.IntValueThr[0] = pstOvpObj->m_stDiFwReg.IntValueThr[0];
		pstOvpObj->m_stDiFwIData.IntValueThr[1] = pstOvpObj->m_stDiFwReg.IntValueThr[1];
		pstOvpObj->m_stDiFwIData.IntValueThr[2] = pstOvpObj->m_stDiFwReg.IntValueThr[2];
	} else if(rpd == 1) {
		pstOvpObj->m_stDiFwIData.IntValueThr[0] = pstOvpObj->m_stDiFwReg.IntValueThr[3];
		pstOvpObj->m_stDiFwIData.IntValueThr[1] = pstOvpObj->m_stDiFwReg.IntValueThr[4];
		pstOvpObj->m_stDiFwIData.IntValueThr[2] = pstOvpObj->m_stDiFwReg.IntValueThr[5];
	} else {
		pstOvpObj->m_stDiFwIData.IntValueThr[0] = 0;
		pstOvpObj->m_stDiFwIData.IntValueThr[1] = 0;
		pstOvpObj->m_stDiFwIData.IntValueThr[2] = 0;
	}

	for(i= 0; i<7; i++) {
		if(rpd == 0) {
			if ((pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize > 600)&&(i<3))  /* in HD mode, half of low-angles are off */
				SdiOff[i] = 255;
			else
				SdiOff[i] = pstOvpObj->m_stDiFwReg.SdiOff0[i];

			SdiGain[i] = pstOvpObj->m_stDiFwReg.SdiGain0[i];
		} else if(rpd == 1) {
			if ((pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize > 600)&&(i<3))  /* in HD mode, half of low-angles are off */
				SdiOff[i] = 255;
			else
				SdiOff[i] = pstOvpObj->m_stDiFwReg.SdiOff1[i];

			SdiGain[i] = pstOvpObj->m_stDiFwReg.SdiGain1[i];
		} else {
			if ((pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize > 600)&&(i<3))  /* in HD mode, half of low-angles are off */
				SdiOff[i] = 255;
			else
				SdiOff[i] = pstOvpObj->m_stDiFwReg.SdiOff2[i];

			SdiGain[i] = pstOvpObj->m_stDiFwReg.SdiGain2[i];
		}
	}

	/*******new added from FMD*********/
	if((pstOvpObj->m_stDiFwIData.Fmd_still == 1)&&(pstOvpObj->m_stDiFwIData.gmdv==1))
		pstOvpObj->m_stDiFwIData.gmdv = 0;
	else if((pstOvpObj->m_stDiFwIData.Fmd_still == 1)&&(pstOvpObj->m_stDiFwIData.gosd==1))
		pstOvpObj->m_stDiFwIData.gmdv = 0;
	else if((pstOvpObj->m_stDiFwIData.glmv == 0)&&(pstOvpObj->m_stDiFwIData.gmdv==1))
		pstOvpObj->m_stDiFwIData.gmdv = 0;
	else if((pstOvpObj->m_stDiFwIData.glmv == 0)&&(pstOvpObj->m_stDiFwIData.gosd==1))
		pstOvpObj->m_stDiFwIData.gmdv = 0;

	if((pstOvpObj->m_stDiFwIData.glamd == 2)&&(pstOvpObj->m_stDiFwIData.glmv==2)&&(pstOvpObj->m_stDiFwIData.gosd!=3))
		pstOvpObj->m_stDiFwIData.glamd = 3;
	/***********************************/

	/* Program to Hardware registers */
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_glamd = pstOvpObj->m_stDiFwIData.glamd;
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_gosd = pstOvpObj->m_stDiFwIData.gosd;
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_gmdv = pstOvpObj->m_stDiFwIData.gmdv;
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_grpd = pstOvpObj->m_stDiFwIData.grpd;

	OVP_DBG_INFO("\tfmd\t\t= \t0x%x\n", pstOvpObj->m_stDiFwIData.fmd);
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_fmd_lcnst = pstOvpObj->m_stDiFwIData.fmd % 4;
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_fmd_still = (pstOvpObj->m_stDiFwIData.fmd % 8)/4;
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_fmd_ph = (pstOvpObj->m_stDiFwIData.fmd % 32)/8;
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_fmd_lock = pstOvpObj->m_stDiFwIData.fmd / 32;
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_NLE = pstOvpObj->m_stDiFwIData.NLE;

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_YMdvThr_0i = pstOvpObj->m_stDiFwIData.YMdvThr[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_YMdvThr_1i = pstOvpObj->m_stDiFwIData.YMdvThr[1];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_YMdvThr_2i = pstOvpObj->m_stDiFwIData.YMdvThr[2];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_YMdvThr_3i = pstOvpObj->m_stDiFwIData.YMdvThr[3];

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_UVMdvThr_0i = pstOvpObj->m_stDiFwIData.UVMdvThr[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_UVMdvThr_1i = pstOvpObj->m_stDiFwIData.UVMdvThr[1];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_UVMdvThr_2i = pstOvpObj->m_stDiFwIData.UVMdvThr[2];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_UVMdvThr_3i = pstOvpObj->m_stDiFwIData.UVMdvThr[3];

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_0i = SdiGain[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_1i = SdiGain[1];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_2i = SdiGain[2];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_3i = SdiGain[3];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_4i = SdiGain[4];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_5i = SdiGain[5];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_6i = SdiGain[6];

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_0i = SdiOff[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_1i = SdiOff[1];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_2i = SdiOff[2];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_3i = SdiOff[3];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_4i = SdiOff[4];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_5i = SdiOff[5];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_6i = SdiOff[6];

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AngHsgrmThr_0i = pstOvpObj->m_stDiFwIData.AngHsgrmThr[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AngHsgrmThr_1i = pstOvpObj->m_stDiFwIData.AngHsgrmThr[1];

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_IntValueThr_0i = pstOvpObj->m_stDiFwIData.IntValueThr[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_IntValueThr_1i = pstOvpObj->m_stDiFwIData.IntValueThr[1];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_IntValueThr_2i = pstOvpObj->m_stDiFwIData.IntValueThr[2];

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF3VPosDelay_0i = pstOvpObj->m_stDiFwIData.AFDF3VPosDelay[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF3VPosDelay_1i = pstOvpObj->m_stDiFwIData.AFDF3VPosDelay[1];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF3VPosDelay_2i = pstOvpObj->m_stDiFwIData.AFDF3VPosDelay[2];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF3VPosDelay_3i = pstOvpObj->m_stDiFwIData.AFDF3VPosDelay[3];

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF2VPosDelay_0i = pstOvpObj->m_stDiFwIData.AFDF2VPosDelay[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF2VPosDelay_1i = pstOvpObj->m_stDiFwIData.AFDF2VPosDelay[1];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF2VPosDelay_2i = pstOvpObj->m_stDiFwIData.AFDF2VPosDelay[2];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF2VPosDelay_3i = pstOvpObj->m_stDiFwIData.AFDF2VPosDelay[3];

	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF1VPosDelay_0i = pstOvpObj->m_stDiFwIData.AFDF1VPosDelay[0];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF1VPosDelay_1i = pstOvpObj->m_stDiFwIData.AFDF1VPosDelay[1];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF1VPosDelay_2i = pstOvpObj->m_stDiFwIData.AFDF1VPosDelay[2];
	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF1VPosDelay_3i = pstOvpObj->m_stDiFwIData.AFDF1VPosDelay[3];

#ifdef DEBUG_DUMP_OVP_REGS
	OVP_DBG_INFO("\n/************************************************************************/\n");
	OVP_DBG_INFO("\n/*****************\t\t DEINT INPUT DATA \t\t****************/\n");
	OVP_DBG_INFO("\n/************************************************************************/\n");
	OVP_DBG_INFO("\tghf\t\t= \t0x%x\n", pstOvpObj->m_stDiFwIData.ghf);
	OVP_DBG_INFO("\tgrpd\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_grpd);
	OVP_DBG_INFO("\tgmdv\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_gmdv);
	OVP_DBG_INFO("\tglamd\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_glamd);
	OVP_DBG_INFO("\tgosd\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_gosd);
	OVP_DBG_INFO("\tglmv\t\t= \t0x%x\n", pstOvpObj->m_stDiFwIData.glmv);
	OVP_DBG_INFO("\tfmd_lcnst\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_fmd_lcnst);
	OVP_DBG_INFO("\tfmd_still\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_fmd_still);
	OVP_DBG_INFO("\tfmd_lock\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_fmd_lock);
	OVP_DBG_INFO("\tfmd_ph\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_fmd_ph);
	OVP_DBG_INFO("\tNLE\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_NLE);

	OVP_DBG_INFO("\tYMdvThr_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_YMdvThr_0i);
	OVP_DBG_INFO("\tYMdvThr_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_YMdvThr_1i);
	OVP_DBG_INFO("\tYMdvThr_2i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_YMdvThr_2i);
	OVP_DBG_INFO("\tYMdvThr_3i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_YMdvThr_3i);

	OVP_DBG_INFO("\tUVMdvThr_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_UVMdvThr_0i);
	OVP_DBG_INFO("\tUVMdvThr_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_UVMdvThr_1i);
	OVP_DBG_INFO("\tUVMdvThr_2i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_UVMdvThr_2i);
	OVP_DBG_INFO("\tUVMdvThr_3i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_UVMdvThr_3i);

	OVP_DBG_INFO("\tSdiGain_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_0i);
	OVP_DBG_INFO("\tSdiGain_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_1i);
	OVP_DBG_INFO("\tSdiGain_2i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_2i);
	OVP_DBG_INFO("\tSdiGain_3i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_3i);
	OVP_DBG_INFO("\tSdiGain_4i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_4i);
	OVP_DBG_INFO("\tSdiGain_5i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_5i);
	OVP_DBG_INFO("\tSdiGain_6i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiGain_6i);

	OVP_DBG_INFO("\tSdiOff_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_0i);
	OVP_DBG_INFO("\tSdiOff_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_1i);
	OVP_DBG_INFO("\tSdiOff_2i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_2i);
	OVP_DBG_INFO("\tSdiOff_3i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_3i);
	OVP_DBG_INFO("\tSdiOff_4i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_4i);
	OVP_DBG_INFO("\tSdiOff_5i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_5i);
	OVP_DBG_INFO("\tSdiOff_6i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_SdiOff_6i);

	OVP_DBG_INFO("\tAngHsgrmThr_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AngHsgrmThr_0i);
	OVP_DBG_INFO("\tAngHsgrmThr_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AngHsgrmThr_1i);

	OVP_DBG_INFO("\tIntValueThr_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_IntValueThr_0i);
	OVP_DBG_INFO("\tIntValueThr_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_IntValueThr_1i);
	OVP_DBG_INFO("\tIntValueThr_2i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_IntValueThr_2i);

	OVP_DBG_INFO("\tAFDF1VPosDelay_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF1VPosDelay_0i);
	OVP_DBG_INFO("\tAFDF1VPosDelay_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF1VPosDelay_1i);
	OVP_DBG_INFO("\tAFDF1VPosDelay_2i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF1VPosDelay_2i);
	OVP_DBG_INFO("\tAFDF1VPosDelay_3i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF1VPosDelay_3i);

	OVP_DBG_INFO("\tAFDF2VPosDelay_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF2VPosDelay_0i);
	OVP_DBG_INFO("\tAFDF2VPosDelay_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF2VPosDelay_1i);
	OVP_DBG_INFO("\tAFDF2VPosDelay_2i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF2VPosDelay_2i);
	OVP_DBG_INFO("\tAFDF2VPosDelay_3i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF2VPosDelay_3i);

	OVP_DBG_INFO("\tAFDF3VPosDelay_0i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF3VPosDelay_0i);
	OVP_DBG_INFO("\tAFDF3VPosDelay_1i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF3VPosDelay_1i);
	OVP_DBG_INFO("\tAFDF3VPosDelay_2i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF3VPosDelay_2i);
	OVP_DBG_INFO("\tAFDF3VPosDelay_3i\t\t= \t0x%x\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.ie_DeIntParam.u_AFDF3VPosDelay_3i);

	OVP_DBG_INFO("\n/************************************************************************/\n");
#endif
}

void diFwProcess(OVP_OBJ *pstOvpObj)
{
	/* FW read HW histogram data from DRAM */
	diReadDataFromHw(pstOvpObj);

	/* global MC error detection */
	gdif_det(pstOvpObj);

	/* global rpd detection */
	grpd_det(pstOvpObj);

	/* global rpd detection */
	gosd_det(pstOvpObj);

	/* global high-frequence detection */
	ghf_det(pstOvpObj);

	/* global motion detection */
	gmdv_det(pstOvpObj);

	/* global minimum motion dtection */
	gmdv_min_det(pstOvpObj);

	/* global large area motion detection */
	glamd_det(pstOvpObj);

	/* global motion vector detection */
	/* ghmv_det();	//Not available in the BG4-CT */

	/* film mode detection and processing by FW */
	film_mode_det(pstOvpObj);

	/* large motioln vector detection and processing by FW */
	glmv_det(pstOvpObj);

	/* global vertical position delay controls by FW */
	afd_frame_delay(pstOvpObj);

#if(!BLKLOGICDISABLE)
	/* This flag is defined. So, it is no required. */
	/* block-base motion detection */
	blamd_det(pstOvpObj);

	/* block-base motion detection */
	blasd_det(pstOvpObj);

	/* block-base static osd detection */
	bsosd_det(pstOvpObj);

	/* block-base large motion HF detection */
	blmhf_det(pstOvpObj);
#endif

	/* noise adaptation */
	if(!pstOvpObj->m_stDiFwReg.NLEEn)
		pstOvpObj->m_stDiFwIData.NLE = pstOvpObj->m_stDiFwReg.NLEUser;
	else if(pstOvpObj->m_stDiFwIData.gmin == 3)
		pstOvpObj->m_stDiFwIData.NLE = pstOvpObj->m_stDiFwReg.NoiseThr[2];
	else if(pstOvpObj->m_stDiFwIData.gmin == 2)
		pstOvpObj->m_stDiFwIData.NLE = pstOvpObj->m_stDiFwReg.NoiseThr[1];
	else if(pstOvpObj->m_stDiFwIData.gmin == 1)
		pstOvpObj->m_stDiFwIData.NLE = pstOvpObj->m_stDiFwReg.NoiseThr[0];
	else
		pstOvpObj->m_stDiFwIData.NLE = 0;

	/* power on frame reset */
	if(pstOvpObj->m_stDiFwIData.FrameReset>0)
		pstOvpObj->m_stDiFwIData.FrameReset = pstOvpObj->m_stDiFwIData.FrameReset-1;
}

void MadiBitDepthIn(unsigned int BitDepth)
{
	T32MadiReg_YUV8BitIn	stMadiRegYuv8BitIn;

	if(8 == BitDepth)
		stMadiRegYuv8BitIn.uYUV8BitIn_YUV8BitIn = 1;
	else if(10 == BitDepth)
		stMadiRegYuv8BitIn.uYUV8BitIn_YUV8BitIn = 0;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_YUV8BitIn), stMadiRegYuv8BitIn.u32);
}

void MadiNrCtrl(unsigned int uiEnable)
{
	T32MadiReg_Nr		stMadiRegNr;

	stMadiRegNr.uNr_YNr = uiEnable;
	stMadiRegNr.uNr_UVNr = uiEnable;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_Nr), stMadiRegNr.u32);
}

void MadiDiOutCtrl(unsigned int uiEnable)
{
	T32MadiReg_IntOutOnly	stMadiRegInoutOnly;

	stMadiRegInoutOnly.uIntOutOnly_IntOutOnly = uiEnable;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_IntOutOnly), stMadiRegInoutOnly.u32);
}

void MadiDiagonalBypass(unsigned int uiBypass)
{
	T32MadiReg_DiagByps	stMadiRegDiagBypass;

	stMadiRegDiagBypass.uDiagByps_YDiagByps = uiBypass;
	stMadiRegDiagBypass.uDiagByps_UVDiagByps = uiBypass;

	GA_REG_WORD32_WRITE(MADI_REG(RA_MadiReg_DiagByps), stMadiRegDiagBypass.u32);
}

