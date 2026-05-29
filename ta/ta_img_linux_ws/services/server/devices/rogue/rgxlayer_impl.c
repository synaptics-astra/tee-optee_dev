/*************************************************************************/ /*!
@File
@Title          DDK implementation of the Services abstraction layer
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Copyright      Copyright 2025 Synaptics Incorporated
@Description    DDK implementation of the Services abstraction layer
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
#include "pvr_header.h"
#include "rgxlayer.h"
#ifdef CONFIG_OPTEE
#include "tee_api.h"
#endif
#include <stdarg.h>

#include <string.h>
#include <stdio.h>
#ifndef CONFIG_OPTEE
#include <tee_internal_time_api.h>
#endif
/* includes:
 * rgx_cr_defs_km.h,
 * RGX_BVNC_CORE_KM_HEADER (rgxcore_km_B.V.N.C.h),
 * RGX_BNC_CONFIG_KM_HEADER (rgxconfig_km_B.V.N.C.h)
 */


extern IMG_UINT64 ui64DevErnsBrns;
extern IMG_UINT64 ui64DevFeatures;

extern void       *hPrivate;
extern IMG_UINT32 codeSize;
extern IMG_UINT32 dataSize;
extern void       *code_base;
extern void       *data_base;
extern IMG_UINT64 sGPURegAddr;
extern IMG_UINT64 sBootRemapAddr;
extern IMG_UINT64 sCodeRemapAddr;
extern IMG_UINT64 sDataRemapAddr;
extern IMG_UINT64 sTrampolineRemapAddr;
extern void       *pvRegsVirAddr;

extern int bDevicePA0IsValid;
extern IMG_UINT32 ui32FeaturesValues[];

#define MAX_HW_TIME_US				(1000)
#define WAIT_TRY_COUNT				(1000)

#define RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, Feature) \
			( ui32FeaturesValues[RGX_FEATURE_##Feature##_IDX] < RGX_FEATURE_VALUE_DISABLED )

#define RGX_GET_FEATURE_VALUE(psDevInfo, Feature) \
			( ui32FeaturesValues[RGX_FEATURE_##Feature##_IDX] )

void RGXMemCopy(const void *hPrivate,
		void *pvDst,
		void *pvSrc,
		size_t uiSize)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);
	memcpy(pvDst, pvSrc, uiSize);
}

void RGXMemSet(const void *hPrivate,
		void *pvDst,
		IMG_UINT8 ui8Value,
		size_t uiSize)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);
	memset(pvDst, ui8Value, uiSize);
}


void RGXCommentLog(const void *hPrivate,
                   const IMG_CHAR *pszString,
                   ...)
{
	(void)hPrivate;
	IMG_CHAR szBuffer[512];
	va_list argList;

	va_start(argList, pszString);
	vsnprintf(szBuffer, sizeof(szBuffer), pszString, argList);
	va_end(argList);

#ifdef DEBUG
	printf("<<[GPU TA]>> %s\n", szBuffer);
#endif
}

void RGXErrorLog(const void *hPrivate,
		const IMG_CHAR *pszString,
		...)
{
	IMG_CHAR szBuffer[512];
	va_list argList;

	(void)(hPrivate);

	va_start(argList, pszString);
	vsnprintf(szBuffer, sizeof(szBuffer), pszString, argList);
	va_end(argList);

	printf("%s", szBuffer);
}

IMG_UINT32 RGXGetOSPageSize(const void *hPrivate)
{
	(void)(hPrivate);
	return 4096;
}

IMG_UINT32 RGXGetFWCorememSize(const void *hPrivate)
{
	IMG_UINT32 ui32CorememSize = 0;

	//Meta API, should not be here
	assert(0);

	return ui32CorememSize;
}

void RGXWriteReg32(const void *hPrivate, IMG_UINT32 ui32RegAddr, IMG_UINT32 ui32RegValue)
{
	(void)hPrivate;
	//#define OSWriteHWReg32(addr, off, val) writel((IMG_UINT32)(val), (IMG_BYTE __iomem *)(addr) + (off))
	*((IMG_UINT32*)((IMG_BYTE __iomem *)pvRegsVirAddr + ui32RegAddr)) = ui32RegValue;
}

void RGXWriteReg64(const void *hPrivate, IMG_UINT32 ui32RegAddr, IMG_UINT64 ui64RegValue)
{
	/*	Little endian support only
		#define OSWriteHWReg64(addr, off, val) do \
			{ \
				__typeof__(addr) _addr = addr; \
				__typeof__(off) _off = off; \
				__typeof__(val) _val = val; \
				writel((IMG_UINT32)((_val) & 0xffffffff), (IMG_BYTE __iomem *)(_addr) + (_off));	\
				writel((IMG_UINT32)(((IMG_UINT64)(_val) >> 32) & 0xffffffff), (IMG_BYTE __iomem *)(_addr) + (_off) + 4); \
			} while (0)
	*/

	(void)hPrivate;

	*((IMG_UINT32*)((IMG_BYTE __iomem *)pvRegsVirAddr + ui32RegAddr)) = (IMG_UINT32)(ui64RegValue & 0xffffffff);
	*((IMG_UINT32*)((IMG_BYTE __iomem *)pvRegsVirAddr + ui32RegAddr + 4)) = ((IMG_UINT64)ui64RegValue >> 32) & 0xffffffff;
}

IMG_UINT32 RGXReadReg32(const void *hPrivate, IMG_UINT32 ui32RegAddr)

{	
	/*
		#define OSReadHWReg32(addr, off) ((IMG_UINT32)readl((IMG_BYTE __iomem *)(addr) + (off)))
	*/
	(void)hPrivate;
	IMG_UINT32 ui32RegValue = 0;

	ui32RegValue = *((IMG_UINT32*)((IMG_BYTE __iomem *)pvRegsVirAddr + ui32RegAddr));

	return ui32RegValue;
}

IMG_UINT64 RGXReadReg64(const void *hPrivate, IMG_UINT32 ui32RegAddr)
{
	/* Little endian support only 
		#define OSReadHWReg64(addr, off) \
				({ \
					__typeof__(addr) _addr = addr; \
					__typeof__(off) _off = off; \
					(IMG_UINT64) \
					( \
						( (IMG_UINT64)(readl((IMG_BYTE __iomem *)(_addr) + (_off) + 4)) << 32) \
						| readl((IMG_BYTE __iomem *)(_addr) + (_off)) \
					); \
				})

	*/
	IMG_UINT64 ui64RegValue;

	ui64RegValue =  (((IMG_UINT64)(*((IMG_UINT32*)((IMG_BYTE __iomem*)pvRegsVirAddr + ui32RegAddr + 4)))) << 32)
					| (IMG_UINT64)(*((IMG_UINT32*)((IMG_BYTE __iomem*)pvRegsVirAddr + ui32RegAddr)));

	return ui64RegValue;
}

IMG_UINT32 RGXReadModifyWriteReg64(const void *hPrivate,
		IMG_UINT32 ui32RegAddr,
		IMG_UINT64 uiRegValueNew,
		IMG_UINT64 uiRegKeepMask)
{
	/* only use the new values for bits we update according to the keep mask */
	uiRegValueNew &= ~uiRegKeepMask;

	IMG_UINT64 uiRegValue = RGXReadReg64(hPrivate, ui32RegAddr);
	uiRegValue &= uiRegKeepMask;
	RGXWriteReg64(hPrivate, ui32RegAddr, uiRegValue | uiRegValueNew);

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXPollReg32(const void *hPrivate,
                          IMG_UINT32 ui32RegAddr,
                          IMG_UINT32 ui32RegValue,
                          IMG_UINT32 ui32RegMask)
{
	IMG_UINT32	ui32ActualValue = 0xFFFFFFFFU;

	for(int i=0; i<WAIT_TRY_COUNT; i++)
	{
		ui32ActualValue = RGXReadReg32(NULL, ui32RegAddr) & ui32RegMask;

		if (ui32ActualValue == ui32RegValue)
		{
			return PVRSRV_OK;
		}

		IMG_UINT32 ms = (MAX_HW_TIME_US+999)/1000;
		TEE_Wait(ms);
	}

	printf("PollForValueKM: Timeout. Expected 0x%x but found 0x%x (mask 0x%x).",
			(unsigned int)ui32RegValue, (unsigned int)ui32ActualValue, (unsigned int)ui32RegMask);

	return PVRSRV_ERROR_TIMEOUT;
}

PVRSRV_ERROR RGXPollReg64(const void *hPrivate,
                          IMG_UINT32 ui32RegAddr,
                          IMG_UINT64 ui64RegValue,
                          IMG_UINT64 ui64RegMask)
{
	/* Split lower and upper words */
	int res = PVRSRV_ERROR_TIMEOUT;
	IMG_UINT32 ui32UpperValue = (IMG_UINT32) (ui64RegValue >> 32);
	IMG_UINT32 ui32LowerValue = (IMG_UINT32) (ui64RegValue);
	IMG_UINT32 ui32UpperMask  = (IMG_UINT32) (ui64RegMask >> 32);
	IMG_UINT32 ui32LowerMask  = (IMG_UINT32) (ui64RegMask);

	IMG_UINT32	ui32ActualValue = 0xFFFFFFFFU;

	for(int i=0; i<WAIT_TRY_COUNT; i++)
	{
		ui32ActualValue = RGXReadReg32(NULL, (ui32RegAddr + 4)) & ui32UpperMask;

		if (ui32ActualValue == ui32UpperValue)
		{
			res = PVRSRV_OK;
			break;
		}

		IMG_UINT32 ms = (MAX_HW_TIME_US+999)/1000;
		TEE_Wait(ms);
	}

	if(res != PVRSRV_OK) {
		printf("RGXPollReg64: Poll for upper part of Reg (0x%x) failed, ui32ActualValue=%x ui32UpperValue=%x\n", (unsigned int)ui32RegAddr, ui32ActualValue, ui32UpperValue);
		return PVRSRV_ERROR_TIMEOUT;
	}

	for(int i=0; i<WAIT_TRY_COUNT; i++)
	{
		ui32ActualValue = RGXReadReg32(NULL, ui32RegAddr) & ui32LowerMask;

		if (ui32ActualValue == ui32LowerValue)
		{
			return PVRSRV_OK;
		}

		IMG_UINT32 ms = (MAX_HW_TIME_US+999)/1000;
		TEE_Wait(ms);
	}

	printf("RGXPollReg64: Poll for lower part of Reg (0x%x) failed, ui32ActualValue=%x ui32LowerValue=%x\n", (unsigned int)ui32RegAddr, ui32ActualValue, ui32LowerValue);
	return PVRSRV_ERROR_TIMEOUT;
}

void RGXWaitCycles(const void *hPrivate, IMG_UINT32 ui32Cycles, IMG_UINT32 ui32TimeUs)
{
	(void)hPrivate;
	IMG_UINT32 ms = (ui32TimeUs+999)/1000;
	TEE_Wait(ms);
}

void RGXAcquireKernelMMUPC(const void *hPrivate, IMG_DEV_PHYADDR *psPCAddr) {
	printf("META CPU only API, should not be here\n");
}

void RGXAcquireGPURegsAddr(const void *hPrivate, IMG_DEV_PHYADDR *psGPURegsAddr)
{
	psGPURegsAddr->uiAddr = sGPURegAddr;
}

void RGXAcquireBootRemapAddr(const void *hPrivate, IMG_DEV_PHYADDR *psBootRemapAddr)
{
	psBootRemapAddr->uiAddr = sBootRemapAddr;
}


void RGXAcquireCodeRemapAddr(const void *hPrivate, IMG_DEV_PHYADDR *psCodeRemapAddr)
{
	psCodeRemapAddr->uiAddr = sCodeRemapAddr;
}

void RGXAcquireDataRemapAddr(const void *hPrivate, IMG_DEV_PHYADDR *psDataRemapAddr)
{
	psDataRemapAddr->uiAddr = sDataRemapAddr;
}

void RGXAcquireTrampolineRemapAddr(const void *hPrivate, IMG_DEV_PHYADDR *psTrampolineRemapAddr)
{
	/*TODO*/
	/*This function only use for 32 BUS width, we are 36 bits, ignore it now*/
	printf("32 BUS width device only API, should not be here\n");

	psTrampolineRemapAddr->uiAddr = sTrampolineRemapAddr;
}

#define MAX_NUM_COHERENCY_TESTS  (10)
IMG_BOOL RGXDoFWSlaveBoot(const void *hPrivate)
{
	printf("\n\nMETA CPU device only API, should not be here\n\n");
	assert(0);

	return false;
}

//static PVRSRV_ERROR RGXWriteMetaRegThroughSP(const void *hPrivate, IMG_UINT32 ui32RegAddr, IMG_UINT32 ui32RegValue)
//{
//	printf("\n\nMETA CPU device only API, should not be here\n\n");
//	assert(0);
//
//	return PVRSRV_OK;
//}

/*
 * The fabric coherency test is performed when platform supports fabric coherency
 * either in the form of ACE-lite or Full-ACE. This test is done quite early
 * with the firmware processor quiescent and makes exclusive use of the slave
 * port interface for reading/writing through the device memory hierarchy. The
 * rationale for the test is to ensure that what the CPU writes to its dcache
 * is visible to the GPU via coherency snoop miss/hit and vice-versa without
 * any intervening cache maintenance by the writing agent.
 */
PVRSRV_ERROR RGXFabricCoherencyTest(const void *hPrivate)
{
	printf("META CPU device only API, should not be here\n");
	assert(0);

	return PVRSRV_OK;
}

IMG_INT32 RGXDeviceGetFeatureValue(const void *hPrivate, IMG_UINT64 ui64Feature)
{
	(void)hPrivate;
	if (ui64Feature >= RGX_FEATURE_WITH_VALUES_MAX_IDX)
	{
		return -1;
	}

	if (ui32FeaturesValues[ui64Feature] == RGX_FEATURE_VALUE_DISABLED)
	{
		return -1;
	}

	return ui32FeaturesValues[ui64Feature];
}

IMG_BOOL RGXDeviceHasFeature(const void *hPrivate, IMG_UINT16 ui16FeatureIndex, IMG_UINT64 ui64Feature)
{
	(void)hPrivate;
	(void)ui16FeatureIndex;
	return (ui64DevFeatures & ui64Feature) != 0;
}

IMG_BOOL RGXDeviceHasErnBrn(const void *hPrivate, IMG_UINT64 ui64ErnsBrns)
{
	return (ui64DevErnsBrns & ui64ErnsBrns) != 0;
}

IMG_UINT32 RGXGetDeviceSLCBanks(const void *hPrivate)
{
	if (!RGX_IS_FEATURE_VALUE_SUPPORTED(NULL, SLC_BANKS))
	{
		return 0;
	}
	return RGX_GET_FEATURE_VALUE(NULL, SLC_BANKS);
}

IMG_UINT32 RGXGetDeviceCacheLineSize(const void *hPrivate)
{
	(void)hPrivate;
	if (!RGX_IS_FEATURE_VALUE_SUPPORTED(NULL, SLC_CACHE_LINE_SIZE_BITS))
	{
		return 0;
	}
	return RGX_GET_FEATURE_VALUE(NULL, SLC_CACHE_LINE_SIZE_BITS);
}

IMG_UINT32 RGXGetDevicePhysBusWidth(const void *hPrivate)
{
	(void)hPrivate;
	if (!RGX_IS_FEATURE_VALUE_SUPPORTED(NULL, PHYS_BUS_WIDTH))
	{
		return 0;
	}
	return RGX_GET_FEATURE_VALUE(NULL, PHYS_BUS_WIDTH);
}

IMG_BOOL RGXDevicePA0IsValid(const void *hPrivate)
{
	return bDevicePA0IsValid;
}

void RGXAcquireBootCodeAddr(const void *hPrivate, IMG_DEV_VIRTADDR *psBootCodeAddr)
{
	(void)hPrivate;

	//Should not be here, only RISC-V call this API
	assert(0);

	psBootCodeAddr->uiAddr = 0x0;
}

void RGXAcquireBootDataAddr(const void *hPrivate, IMG_DEV_VIRTADDR *psBootDataAddr)
{
	(void)hPrivate;

	//Should not be here, only RISC-V call this API
	assert(0);

	psBootDataAddr->uiAddr = 0x0;
}
static inline IMG_BOOL RGXAckHwIrq(void *psDevInfo,
								   IMG_UINT32 ui32IRQStatusReg,
								   IMG_UINT32 ui32IRQStatusEventMsk,
								   IMG_UINT32 ui32IRQClearReg,
								   IMG_UINT32 ui32IRQClearMask)
{
	IMG_UINT32 ui32IRQStatus = RGXReadReg32(NULL, ui32IRQStatusReg);

	if (ui32IRQStatus & ui32IRQStatusEventMsk)
	{
		/* acknowledge and clear the interrupt */
		RGXWriteReg32(NULL, ui32IRQClearReg, ui32IRQClearMask);
		return IMG_TRUE;
	}
	else
	{
		/* spurious interrupt */
		return IMG_FALSE;
	}
}

static IMG_BOOL RGXAckIrqMIPS(void *psDevInfo)
{
	return RGXAckHwIrq(psDevInfo,
					   RGX_CR_MIPS_WRAPPER_IRQ_STATUS,
					   RGX_CR_MIPS_WRAPPER_IRQ_STATUS_EVENT_EN,
					   RGX_CR_MIPS_WRAPPER_IRQ_CLEAR,
					   RGX_CR_MIPS_WRAPPER_IRQ_CLEAR_EVENT_EN);
}

void *RGXCalculateHostFWDataAddress(const void *hPrivate, void *pvHostFWDataAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);

	return pvHostFWDataAddr;
}

void RGXSetPoweredState(const void *hPrivate, IMG_BOOL bPowered)
{
	PVR_UNREFERENCED_PARAMETER(hPrivate);
	PVR_UNREFERENCED_PARAMETER(bPowered);
}

IMG_BOOL RGXDeviceAckIrq(const void *hPrivate)
{
	(void)hPrivate;
	return RGXAckIrqMIPS(NULL);
}

