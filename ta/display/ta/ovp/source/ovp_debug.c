/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#include "ovp_module.h"
#include "ovp_common.h"
#include "ErrorCode.h"
#include "ovp_api.h"
#include "ovp_apifuncs.h"
#include "galois_io.h"
#include "ta_log.h"
#include "vdec_com.h"
#include "ovp_cmd.h"
#include "ovp_debug.h"

#define OVP_DBG_LOGGER	OVP_DBG_HIGH

typedef struct _tag_ovp_reg_dump_table
{
	char *addr_name;
	UINT32 addr;
} OVP_REG_INFO;

const OVP_REG_INFO ovp_reg_table[] = {
	{"FA_OvpReg_Reset				",0x0000},
	{"FA_OvpReg_ClockControl			",0x0004},
	{"FA_OvpReg_CmdAddrPush				",0x0010},
	{"FA_OvpReg_CmdAddrLevel			",0x0014},
	{"FA_OvpReg_CmdAddrRead				",0x0020},
	{"FA_OvpReg_CmdAddrRead1			",0x0024},
	{"FA_OvpReg_CmdAddrRead2			",0x0028},
	{"FA_OvpReg_CmdAddrRead3			",0x002c},
	{"FA_OvpReg_IntStatus				",0x0040},
	{"FA_OvpReg_IntMask				",0x0050},
	{"FA_OvpReg_AxiErrLog				",0x0060},
	{"FA_OvpReg_AxiParam				",0x0080},
	{"FA_OvpReg_MadiReg				",0x0400},
	{"FA_OvpReg_FmdReg				",0x0800},
	{"FA_OvpReg_MadiReg_Edge0			",0x0400},
	{"FA_OvpReg_MadiReg_Edge1			",0x0404},
	{"FA_OvpReg_MadiReg_Cnst			",0x0408},
	{"FA_OvpReg_MadiReg_MdvCnst			",0x040c},
	{"FA_OvpReg_MadiReg_LamdNoise			",0x0410},
	{"FA_OvpReg_MadiReg_LasdNoise			",0x0414},
	{"FA_OvpReg_MadiReg_LAMD			",0x0418},
	{"FA_OvpReg_MadiReg_LASD			",0x041c},
	{"FA_OvpReg_MadiReg_SOSD0			",0x0420},
	{"FA_OvpReg_MadiReg_SOSD1			",0x0424},
	{"FA_OvpReg_MadiReg_LAMDGain			",0x0428},
	{"FA_OvpReg_MadiReg_LASDGain			",0x042c},
	{"FA_OvpReg_MadiReg_Hf				",0x0430},
	{"FA_OvpReg_MadiReg_Rpd				",0x0434},
	{"FA_OvpReg_MadiReg_SdiSad			",0x0438},
	{"FA_OvpReg_MadiReg_DiagByps			",0x043c},
	{"FA_OvpReg_MadiReg_SdiNoise			",0x0440},
	{"FA_OvpReg_MadiReg_Sdi0			",0x0444},
	{"FA_OvpReg_MadiReg_Sdi1			",0x0448},
	{"FA_OvpReg_MadiReg_Sdi2			",0x044c},
	{"FA_OvpReg_MadiReg_ShiftBit			",0x0450},
	{"FA_OvpReg_MadiReg_Sdi3			",0x0454},
	{"FA_OvpReg_MadiReg_Sdi4			",0x0458},
	{"FA_OvpReg_MadiReg_Sdi5			",0x045c},
	{"FA_OvpReg_MadiReg_Sdi6			",0x0460},
	{"FA_OvpReg_MadiReg_Angle			",0x0464},
	{"FA_OvpReg_MadiReg_MotDet			",0x0468},
	{"FA_OvpReg_MadiReg_HVLine			",0x046c},
	{"FA_OvpReg_MadiReg_MdvYiir			",0x0470},
	{"FA_OvpReg_MadiReg_C420A			",0x0474},
	{"FA_OvpReg_MadiReg_C420B			",0x0478},
	{"FA_OvpReg_MadiReg_MdvFilterSel		",0x047c},
	{"FA_OvpReg_MadiReg_LcnstGain			",0x0480},
	{"FA_OvpReg_MadiReg_YUVBlendGain		",0x0484},
	{"FA_OvpReg_MadiReg_YUVMdvBlendGain		",0x0488},
	{"FA_OvpReg_MadiReg_YMdvGain			",0x048c},
	{"FA_OvpReg_MadiReg_YMdvGain1			",0x0490},
	{"FA_OvpReg_MadiReg_YMdvOffset			",0x0494},
	{"FA_OvpReg_MadiReg_UVMdvGain			",0x0498},
	{"FA_OvpReg_MadiReg_Impulse			",0x049c},
	{"FA_OvpReg_MadiReg_Nr				",0x04a0},
	{"FA_OvpReg_MadiReg_MdvLcnst0			",0x04a4},
	{"FA_OvpReg_MadiReg_MdvLcnst1			",0x04a8},
	{"FA_OvpReg_MadiReg_MdvLcnst2			",0x04ac},
	{"FA_OvpReg_MadiReg_MdvLcnst3			",0x04b0},
	{"FA_OvpReg_MadiReg_VTFCoef0			",0x04b4},
	{"FA_OvpReg_MadiReg_VTFCoef1			",0x04b8},
	{"FA_OvpReg_MadiReg_VTFCoef2			",0x04bc},
	{"FA_OvpReg_MadiReg_C420VTFCoef0		",0x04c0},
	{"FA_OvpReg_MadiReg_C420VTFCoef1		",0x04c4},
	{"FA_OvpReg_MadiReg_C420VTFCoef2		",0x04c8},
	{"FA_OvpReg_MadiReg_C420Mode			",0x04cc},
	{"FA_OvpReg_MadiReg_IntOutOnly			",0x04d0},
	{"FA_OvpReg_MadiReg_ChromaMdvUseOnly		",0x04d4},
	{"FA_OvpReg_MadiReg_MdvMin			",0x04d8},
	{"FA_OvpReg_MadiReg_GAccShiftBits		",0x04dc},
	{"FA_OvpReg_MadiReg_YUV8BitIn			",0x04e0},
	{"FA_OvpReg_FmdReg_FieldDifThr			",0x0800},
	{"FA_OvpReg_FmdReg_FmdHmeGain0			",0x0804},
	{"FA_OvpReg_FmdReg_FmdHmeGain1			",0x0808},
	{"FA_OvpReg_FmdReg_FmdHmeOff0			",0x080c},
	{"FA_OvpReg_FmdReg_FmdHmeOff1			",0x0810},
	{"FA_OvpReg_FmdReg_FmdHmeThr			",0x0814},
	{"FA_OvpReg_FmdReg_FmdGhmvShiftBit		",0x0818},
	{"FA_OvpReg_FmdReg_FmdScrTextFixLineDly0	",0x081c},
	{"FA_OvpReg_FmdReg_FmdScrTextFixLineDly1	",0x0820},
	{"FA_OvpReg_FmdReg_FmdHWEn			",0x0824},
	{"FA_OvpReg_FmdReg_ChromaUseMadiOnly		",0x0828},
	{"FA_OvpReg_FmdReg_Fmdhmvoff			",0x082c},
	{"FA_OvpReg_FmdReg_FmdLmvThr			",0x0830},
	{"FA_OvpReg_FmdReg_Fmdhmvshiftbits		",0x0834},
	{"FA_OvpReg_FmdReg_FmdhmeThr0			",0x0838},
	{"FA_OvpReg_FmdReg_FmdhmeThr1			",0x083c},
	{"FA_OvpReg_FmdReg_FmdhmeThr2			",0x0840},
	{"FA_OvpReg_FmdReg_FmdmdvThr0			",0x0844},
	{"FA_OvpReg_FmdReg_FmdmdvThr1			",0x0848},
};

const OVP_REG_INFO ovp_cmd_reg_table[] = {
	{"FA_OvpCmd_Control				",0x0000},
	{"FA_OvpCmd_VideoFormat				",0x0004},
	{"FA_OvpCmd_InputBuffer				",0x0008},
	{"FA_OvpCmd_InputBuffer1			",0x000c},
	{"FA_OvpCmd_InputBuffer2			",0x0010},
	{"FA_OvpCmd_InputBuffer3			",0x0014},
	{"FA_OvpCmd_InputBuffer4			",0x0018},
	{"FA_OvpCmd_InputBuffer5			",0x001c},
	{"FA_OvpCmd_InputBuffer6			",0x0020},
	{"FA_OvpCmd_InputBuffer7			",0x0024},
	{"FA_OvpCmd_InputBuffer8			",0x0028},
	{"FA_OvpCmd_InputBuffer9			",0x002c},
	{"FA_OvpCmd_OutputBuffer			",0x0030},
	{"FA_OvpCmd_OutputBuffer1			",0x0034},
	{"FA_OvpCmd_OutputBuffer2			",0x0038},
	{"FA_OvpCmd_OutputBuffer3			",0x003c},
	{"FA_OvpCmd_OutputBuffer4			",0x0040},
	{"FA_OvpCmd_OutputBuffer5			",0x0044},
	{"FA_OvpCmd_DeIntParam				",0x0048},
	{"FA_OvpCmd_ScaleParam				",0x0080},
	{"FA_OvpCmd_FmdStat				",0x0100},
	{"FA_OvpCmd_DeIntStat				",0x0190},
	{"FA_OvpCmd_Return				",0x01f0},
	{"FA_OvpCmd_End					",0x01f4},
};

INT DumpOvpCmdRegs(OVP_OBJ *pstOvpObj)
{
	UINT32 size = 0;
	UINT32 idx = 0;
	UINT32 regVal = 0;
	UINT32 *ovpCmdAddr;
	UINT32 regOffset,regIdx;
	UINT32 regAddr;

	if(!pstOvpObj)
		return 0;

	ovpCmdAddr = (UINT32*)&pstOvpObj->pOvp_cmd_obj->stOvpBufInfo;
	size = sizeof(ovp_cmd_reg_table)/sizeof(OVP_REG_INFO);
	OVP_DBG_LOGGER("@@@@@@@ OVP REG DUMP START for client - %d @@@@@@@\n", pstOvpObj->m_uchOvpClientId);

	for(idx=0; idx<size; idx++) {
		regIdx = regOffset = 0;
		while(((idx+1)<size) && ((ovp_cmd_reg_table[idx].addr+regOffset)<ovp_cmd_reg_table[idx+1].addr)) {
			regAddr = (ovp_cmd_reg_table[idx].addr/4)+regIdx;
			regVal = *(ovpCmdAddr+regAddr);
			if(regIdx==0) {
				OVP_DBG_LOGGER("%s 0x%08X \t 0x%X\n\r", ovp_cmd_reg_table[idx].addr_name, ovp_cmd_reg_table[idx].addr, regVal);
			} else {
				OVP_DBG_LOGGER("%02d_%s 0x%08X \t 0x%X\n\r", regIdx, ovp_cmd_reg_table[idx].addr_name, regAddr*4, regVal);
			}
			regOffset+=4;
			regIdx++;
		}
	}

	OVP_DBG_LOGGER("uControl_IOC = %d, OutputWholeFrame = %d, OutputOneField = %d \n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uControl_IOC, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uControl_OutputWholeFrame, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uControl_OutputOneField);
	OVP_DBG_LOGGER("InputUVSample = %d, InputPlanes = %d, InputBitDepth = %d, InputTile = %d\n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputUVSample, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputPlanes, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputBitDepth,	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputTile);
	OVP_DBG_LOGGER("InputInterlaced = %d, OutputUVSample = %d, OutputPlanes = %d, OutputBitDepth = %d\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputInterlaced , pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_OutputUVSample, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_OutputPlanes, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_OutputBitDepth );
	OVP_DBG_LOGGER("InputHSize = %d, InputVSize = %d, InputYStride = %d, InputUVStride = %d\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputYStride, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputUVStride);

	OVP_DBG_LOGGER("InputBuf0YAddr = 0x%x, InputBuf0UVAddr = 0x%x, InputBuf1YAddr = 0x%x, InputBuf1UVAddr = 0x%x \n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf0YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf0UVAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf1YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf1UVAddr);
	OVP_DBG_LOGGER("InputBuf2YAddr = 0x%x, InputBuf2UVAddr = 0x%x, InputBuf3YAddr = 0x%x, InputBuf3UVAddr = 0x%x \n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf2YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf2UVAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf3YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf3UVAddr);

	OVP_DBG_LOGGER("OutputHSize = %d, OutputVSize = %d, OutputYStride = %d, OutputUVStride = %d \n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputHSize, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputVSize , pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputYStride, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputUVStride );

	OVP_DBG_LOGGER("OutputBuf0YAddr = 0x%x, OutputBuf0UVAddr = 0x%x, OutputBuf1YAddr = 0x%x, OutputBuf1UVAddr = 0x%x\n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputBuf0YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputBuf0UVAddr , pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputBuf1YAddr , pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputBuf1UVAddr);

	OVP_DBG_LOGGER("uReturn_Done = %d, uReturn_ErrorCode = %d\n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uReturn_Done, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uReturn_ErrorCode);

	OVP_DBG_LOGGER("@@@@@@@ OVP REG DUMP END for client - %d @@@@@@@\n", pstOvpObj->m_uchOvpClientId);

	return 0;
}

int DumpOvpRegs(OVP_OBJ *pstOvpObj)
{
	UINT32 size = 0;
	UINT32 idx = 0;
	UINT32 regVal = 0;
	UINT32 base_addr = OVP_REG(0);

	size = sizeof(ovp_reg_table)/sizeof(OVP_REG_INFO);
	OVP_DBG_LOGGER("@@@@@@@ OVP REG DUMP START @@@@@@@\n");

	for(idx=0; idx<size; idx++) {
		GA_REG_WORD32_READ(base_addr + ovp_reg_table[idx].addr, &regVal);
		OVP_DBG_LOGGER("%s 0x%X \t 0x%X\n\r", ovp_reg_table[idx].addr_name, base_addr + ovp_reg_table[idx].addr, regVal);
	}

	if(!pstOvpObj) {
		OVP_DBG_LOGGER("@@@@@@@ VPP REG DUMP END @@@@@@@\n");
		return 0;
	}

	OVP_DBG_LOGGER("uControl_IOC = %d, OutputWholeFrame = %d, OutputOneField = %d \n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uControl_IOC, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uControl_OutputWholeFrame, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uControl_OutputOneField);
	OVP_DBG_LOGGER("InputUVSample = %d, InputPlanes = %d, InputBitDepth = %d, InputTile = %d\n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputUVSample, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputPlanes, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputBitDepth,	pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputTile);
	OVP_DBG_LOGGER("InputInterlaced = %d, OutputUVSample = %d, OutputPlanes = %d, OutputBitDepth = %d\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_InputInterlaced , pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_OutputUVSample, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_OutputPlanes, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uVideoFormat_OutputBitDepth );
	OVP_DBG_LOGGER("InputHSize = %d, InputVSize = %d, InputYStride = %d, InputUVStride = %d\n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputHSize, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputVSize, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputYStride, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputUVStride);

	OVP_DBG_LOGGER("InputBuf0YAddr = 0x%x, InputBuf0UVAddr = 0x%x, InputBuf1YAddr = 0x%x, InputBuf1UVAddr = 0x%x \n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf0YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf0UVAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf1YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf1UVAddr);
	OVP_DBG_LOGGER("InputBuf2YAddr = 0x%x, InputBuf2UVAddr = 0x%x, InputBuf3YAddr = 0x%x, InputBuf3UVAddr = 0x%x \n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf2YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf2UVAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf3YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uInputBuffer_InputBuf3UVAddr);

	OVP_DBG_LOGGER("OutputHSize = %d, OutputVSize = %d, OutputYStride = %d, OutputUVStride = %d \n",  pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputHSize, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputVSize , pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputYStride, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputUVStride );

	OVP_DBG_LOGGER("OutputBuf0YAddr = 0x%x, OutputBuf0UVAddr = 0x%x, OutputBuf1YAddr = 0x%x, OutputBuf1UVAddr = 0x%x\n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputBuf0YAddr, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputBuf0UVAddr , pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputBuf1YAddr , pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uOutputBuffer_OutputBuf1UVAddr);

	OVP_DBG_LOGGER("uReturn_Done = %d, uReturn_ErrorCode = %d\n", pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uReturn_Done, pstOvpObj->pOvp_cmd_obj->stOvpBufInfo.uReturn_ErrorCode);

	OVP_DBG_LOGGER("@@@@@@@ VPP REG DUMP END @@@@@@@\n");
	return 0;
}

void OvpFrameUpdateInCnt(OVP_OBJ *pstOvpObj) {
	if(!pstOvpObj)
		return;

	pstOvpObj->frame_inCnt++;
}

void OvpFrameUpdateOutCnt(OVP_OBJ *pstOvpObj) {
	if(!pstOvpObj)
		return;

	pstOvpObj->frame_outCnt++;
}

void OvpFrameUpdateHalfFieldCnt(OVP_OBJ *pstOvpObj, UINT32 u32HalfFieldType) {
	if(!pstOvpObj) {
		return;
	}
	if(u32HalfFieldType) {
		pstOvpObj->frame_halfFldInCnt++;
	} else {
		pstOvpObj->frame_BottomFieldInCnt++;
	}
}

void OvpFrameUpdate(OVP_OBJ *pstOvpObj, UINT32 addr, UINT32 size, UINT32 width, UINT32 height) {
	if(!pstOvpObj)
		return;

	pstOvpObj->last_frame_addr = addr;
	pstOvpObj->last_frame_size = size;
	pstOvpObj->last_frame_width = width;
	pstOvpObj->last_frame_height = height;
	pstOvpObj->last_frame_cnt++;
}

void OvpResetFrameCnt(OVP_OBJ *pstOvpObj) {
	if(!pstOvpObj)
		return;

	pstOvpObj->frame_inCnt = pstOvpObj->frame_outCnt = 0;
	pstOvpObj->frame_halfFldInCnt = 0;
	pstOvpObj->frame_BottomFieldInCnt = 0;
	pstOvpObj->last_frame_addr = pstOvpObj->last_frame_cnt = 0;
	pstOvpObj->last_frame_size = 0;
	pstOvpObj->last_frame_width = pstOvpObj->last_frame_height = 0;
}

void OvpResetAllFrameCnt(void) {
	UINT32 uiLoopCount;
	OVP_OBJ *pstOvpObj;

	for(uiLoopCount = 0; uiLoopCount < ovp->m_uchNoOfObjs; uiLoopCount++) {
		pstOvpObj = ovp->m_pOvp_objs[uiLoopCount];
		OvpResetFrameCnt(pstOvpObj);
	}
}
