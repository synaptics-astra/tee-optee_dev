// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Synaptics Incorporated

#include <stdio.h>
#include "config_log.h"
#include "tee_internal_api.h"
#ifdef CONFIG_OPTEE
#include "tee_internal_api_extensions.h"
#include "tee_api_syna.h"
#endif
#include "tee_ta_pvr_cmd.h"
#include "tee_ta_pvr.h"
#include "string.h"
#ifndef CONFIG_OPTEE
#define LOG_TAG "TA_CALC"
#include "tee_log.h"
#endif
#include "pvr_header.h"

#include "img_defs.h"
#include "img_types.h"
#include "pvrsrv_error.h"
#include "rgx_bvnc_defs_km.h"
#include "rgx_mips.h"
#include "rgx_meta.h"
#include "rgxfwimageutils.h"
#include "rgxstartstop.h"

#include "rgx_fw_info.h"
#define SUPPORT_TRUSTED_DEVICE		1

static uint32_t totalTAGpuSessionNum    = 0;
static uint32_t totalTAGpuSessionHandle = 0;
#define GET_B(x)            (((x) & (B_POSTION_MASK)) >> (B_POSITION))

void              *hPrivate                =  NULL;
void              *code_base               =  NULL;
void              *data_base               =  NULL;
IMG_DEVMEM_SIZE_T codeSize                 = -1;
IMG_DEVMEM_SIZE_T dataSize                 = -1;
IMG_UINT64        sPCAddr                  =  0;
IMG_UINT64        sGPURegAddr              =  0;
IMG_UINT64        sBootRemapAddr           =  0;
IMG_UINT64        sCodeRemapAddr           =  0;
IMG_UINT64        sDataRemapAddr           =  0;
IMG_UINT64        sTrampolineRemapAddr     =  0;
void              *pvRegsVirAddr           =  NULL;
unsigned int      *gfx_core_clock          =  NULL;

#if defined(DOLPHIN)
char * device_name = "DOLPHIN";
int               bDevicePA0IsValid        =  0;
IMG_UINT64        ui64DevErnsBrns          =  0xa885aLL;
IMG_UINT64        ui64DevFeatures          =  0x2183951feb5LL;
IMG_UINT32        ui32B                    =  29;
IMG_UINT32        ui32V                    =  14;
IMG_UINT32        ui32N                    =  108;
IMG_UINT32        ui32C                    =  208;
IMG_UINT32        ui32FeaturesValues[RGX_FEATURE_WITH_VALUES_MAX_IDX] = {
	 1,         //RGX_FEATURE_CDM_CONTROL_STREAM_FORMAT_IDX
	-1,         //RGX_FEATURE_ECC_RAMS_IDX
	 0,         //RGX_FEATURE_FAULT_DECODE_VERSION_IDX
	 4,         //RGX_FEATURE_FBCDC_IDX
	 4,         //RGX_FEATURE_FBCDC_ALGORITHM_IDX
	 1,         //RGX_FEATURE_FBCDC_ARCHITECTURE_IDX
	 0,         //RGX_FEATURE_FBC_MAX_DEFAULT_DESCRIPTORS_IDX
	 0,         //RGX_FEATURE_FBC_MAX_LARGE_DESCRIPTORS_IDX
	 0,         //RGX_FEATURE_LAYOUT_MARS_IDX
	-1,         //RGX_FEATURE_META_IDX
	-1,         //RGX_FEATURE_META_COREMEM_SIZE_IDX
	 2,         //RGX_FEATURE_NUM_CLUSTERS_IDX
	 8,         //RGX_FEATURE_NUM_ISP_IPP_PIPES_IDX
	-1,         //RGX_FEATURE_NUM_MEMBUS_IDX
	 8,         //RGX_FEATURE_NUM_OSIDS_IDX
	 2,         //RGX_FEATURE_NUM_RASTER_PIPES_IDX
	 36,        //RGX_FEATURE_PHYS_BUS_WIDTH_IDX
	 2,         //RGX_FEATURE_SIMPLE_PARAMETER_FORMAT_VERSION_IDX
	 2,         //RGX_FEATURE_SLC_BANKS_IDX
	 512,       //RGX_FEATURE_SLC_CACHE_LINE_SIZE_BITS_IDX
	 128,       //RGX_FEATURE_SLC_SIZE_IN_KILOBYTES_IDX
	-1,         //RGX_FEATURE_TB_GPU_COUNT_IDX
	-1,         //RGX_FEATURE_TFBC_VERSION_IDX
	 16,        //RGX_FEATURE_TILE_SIZE_X_IDX
	 16,        //RGX_FEATURE_TILE_SIZE_Y_IDX
	 40,        //RGX_FEATURE_VIRTUAL_ADDRESS_SPACE_BITS_IDX
	 1,         //RGX_FEATURE_XE_ARCHITECTURE_IDX
	-1,         //RGX_FEATURE_XPU_MAX_REGBANKS_ADDR_WIDTH_IDX
	-1,         //RGX_FEATURE_XPU_MAX_SLAVES_IDX
	-1,         //RGX_FEATURE_XPU_REGISTER_BROADCAST_IDX
};
#elif defined(VS550)
char * device_name = "VS550";
int               bDevicePA0IsValid        =  0;
IMG_UINT64        ui64DevErnsBrns          =  0xd4072LL;
IMG_UINT64        ui64DevFeatures          =  0x22c24ec05LL;
IMG_UINT32        ui32B                    =  0;
IMG_UINT32        ui32V                    =  0;
IMG_UINT32        ui32N                    =  0;
IMG_UINT32        ui32C                    =  0;
IMG_UINT32        ui32FeaturesValues[RGX_FEATURE_WITH_VALUES_MAX_IDX];
#elif defined(PLATYPUS)
char * device_name = "PLATYPUS";
int               bDevicePA0IsValid        =  0;
IMG_UINT64        ui64DevErnsBrns          =  0xa885aLL;
IMG_UINT64        ui64DevFeatures          =  0x2183951feb5LL;
IMG_UINT32        ui32B                    =  29;
IMG_UINT32        ui32V                    =  19;
IMG_UINT32        ui32N                    =  52;
IMG_UINT32        ui32C                    =  202;
IMG_UINT32        ui32FeaturesValues[RGX_FEATURE_WITH_VALUES_MAX_IDX] = {
	 1,         //RGX_FEATURE_CDM_CONTROL_STREAM_FORMAT_IDX
	-1,         //RGX_FEATURE_ECC_RAMS_IDX
	 0,         //RGX_FEATURE_FAULT_DECODE_VERSION_IDX
	 4,         //RGX_FEATURE_FBCDC_IDX
	 4,         //RGX_FEATURE_FBCDC_ALGORITHM_IDX
	 1,         //RGX_FEATURE_FBCDC_ARCHITECTURE_IDX
	 0,         //RGX_FEATURE_FBC_MAX_DEFAULT_DESCRIPTORS_IDX
	 0,         //RGX_FEATURE_FBC_MAX_LARGE_DESCRIPTORS_IDX
	 0,         //RGX_FEATURE_LAYOUT_MARS_IDX
	-1,         //RGX_FEATURE_META_IDX
	-1,         //RGX_FEATURE_META_COREMEM_SIZE_IDX
	 1,         //RGX_FEATURE_NUM_CLUSTERS_IDX
	 2,         //RGX_FEATURE_NUM_ISP_IPP_PIPES_IDX
	-1,         //RGX_FEATURE_NUM_MEMBUS_IDX
	 8,         //RGX_FEATURE_NUM_OSIDS_IDX
	 1,         //RGX_FEATURE_NUM_RASTER_PIPES_IDX
	 36,        //RGX_FEATURE_PHYS_BUS_WIDTH_IDX
	 2,         //RGX_FEATURE_SIMPLE_PARAMETER_FORMAT_VERSION_IDX
	 1,         //RGX_FEATURE_SLC_BANKS_IDX
	 512,       //RGX_FEATURE_SLC_CACHE_LINE_SIZE_BITS_IDX
	 16,        //RGX_FEATURE_SLC_SIZE_IN_KILOBYTES_IDX
	-1,         //RGX_FEATURE_TB_GPU_COUNT_IDX
	-1,         //RGX_FEATURE_TFBC_VERSION_IDX
	 16,        //RGX_FEATURE_TILE_SIZE_X_IDX
	 16,        //RGX_FEATURE_TILE_SIZE_Y_IDX
	 40,        //RGX_FEATURE_VIRTUAL_ADDRESS_SPACE_BITS_IDX
	 1,         //RGX_FEATURE_XE_ARCHITECTURE_IDX
	-1,         //RGX_FEATURE_XPU_MAX_REGBANKS_ADDR_WIDTH_IDX
	-1,         //RGX_FEATURE_XPU_MAX_SLAVES_IDX
	-1,         //RGX_FEATURE_XPU_REGISTER_BROADCAST_IDX
};
#else
#error "Unspport chip type, please check and add a new one!"
#endif

PVRSRV_FW_BOOT_PARAMS boot_params;

TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

void TA_EXPORT TA_DestroyEntryPoint(void)
{
}

TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(
	uint32_t		paramTypes,
	TEE_Param		params[4],
	void**			sessionContext)
{
	totalTAGpuSessionNum++;
	totalTAGpuSessionHandle++;
	*sessionContext = (void *)totalTAGpuSessionHandle;
	unsigned char * tmp = 0x0;
	unsigned int clock_value = 0;
	TEE_Result res = TEE_SUCCESS;

#if defined(PLATYPUS)
	res = TEE_Map_Register((void*)(unsigned int)0xf7ea0000, 0x00001000, 0, 0, (void **)&tmp);
	if(res != TEE_SUCCESS) {
		printf("<<[GPU TA]>> Can not map GPU clock, res =%x!\n", res);
		assert(0);
	}
	gfx_core_clock =  (unsigned int*)(tmp+0x760);
	clock_value = *gfx_core_clock;
	*gfx_core_clock = clock_value & 0xFFFFFFFE;
	if(clock_value&0x1) {
		printf("<<[GPU TA]>> GFX core clock default is on, close it\n");
	}
#endif
	printf("<<[GPU TA]>> open session for %s.\n", device_name);
	return TEE_SUCCESS;
}

void TA_EXPORT TA_CloseSessionEntryPoint(void*  sessionContext)
{
	if (totalTAGpuSessionNum > 0) {
		totalTAGpuSessionNum--;
	}
}

static TEE_Result copyFwImage(uint32_t		paramTypes, TEE_Param		params[4])
{
	TEE_Result res = TEE_SUCCESS;
	IMG_DEVMEM_SIZE_T uiFWCorememCodeAllocSize;
	IMG_DEVMEM_SIZE_T uiFWCorememDataAllocSize;
	RGX_FW_INFO_HEADER psFWInfoHeader;

	if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_MEMREF_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_MEMREF_INPUT) {
		printf("<<[GPU TA]>> bad parameters for copyFwImage\n");
		return TEE_ERROR_BAD_PARAMETERS;
	}


	uint8_t *src_in = (uint8_t *)params[0].memref.buffer;
	size_t  src_len = params[0].memref.size;

	uint8_t *dst_in = (uint8_t *)params[1].memref.buffer;
	size_t  dst_len = params[1].memref.size;

	if(dst_len < src_len) {
		printf("<<[GPU TA]>> copyFwImage dstLen(%d) must >= srcLen(%d)\n", dst_len, src_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	RGXGetFWImageAllocSize(NULL,
						src_in,
						src_len,
						&codeSize,
						&dataSize,
						&uiFWCorememCodeAllocSize,
						&uiFWCorememDataAllocSize,
						&psFWInfoHeader);

	codeSize = (codeSize+0xFFFFLL)&0XFFFFFFFFFFFF0000LL;
	dataSize = (dataSize+0xFFFFLL)&0XFFFFFFFFFFFF0000LL;

	uint8_t *dst_code = dst_in;
	uint8_t *dst_data = dst_in + codeSize;
	code_base = dst_code;
	data_base = dst_data;

	RGXProcessFWImage(NULL, src_in, dst_code, dst_data, NULL, NULL, &boot_params);

#ifndef CONFIG_OPTEE
	TEE_FlushCache(dst_in, codeSize+dataSize);
#else
	TEE_CacheFlush(dst_in, codeSize+dataSize);
#endif
	return res;
}

TEE_Result setFwParams(uint32_t		   paramTypes, TEE_Param params[4])
{
	TEE_Result res = TEE_SUCCESS;

	if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 3) != TEE_PARAM_TYPE_VALUE_INPUT) {
		printf("<<[GPU TA]>>  %s Some of the input parameters is not a value\n", __FUNCTION__);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	boot_params.sMips.sGPURegAddr.uiAddr			= ((long long )(params[0].value.a))<<32 | (long long )(params[0].value.b);
	boot_params.sMips.sFWStackAddr.uiAddr			= ((long long )(params[1].value.a))<<32 | (long long )(params[1].value.b);
	boot_params.sMips.ui32FWPageTableLog2PageSize	= params[2].value.a;
	boot_params.sMips.ui32FWPageTableNumPages		= params[2].value.b;

	return res;
}

TEE_Result setFWPageTableAddr(uint32_t		   paramTypes, TEE_Param params[4])
{
	TEE_Result res = TEE_SUCCESS;

	if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 3) != TEE_PARAM_TYPE_VALUE_INPUT) {
		printf("<<[GPU TA]>>  %s Some of the input parameters is not a value\n", __FUNCTION__);
		return TEE_ERROR_BAD_PARAMETERS;
	}
	boot_params.sMips.asFWPageTableAddr[0].uiAddr	= ((long long )(params[0].value.a))<<32 | (long long )(params[0].value.b);
	boot_params.sMips.asFWPageTableAddr[1].uiAddr	= ((long long )(params[1].value.a))<<32 | (long long )(params[1].value.b);
	boot_params.sMips.asFWPageTableAddr[2].uiAddr	= ((long long )(params[2].value.a))<<32 | (long long )(params[2].value.b);
	boot_params.sMips.asFWPageTableAddr[3].uiAddr	= ((long long )(params[3].value.a))<<32 | (long long )(params[3].value.b);

	return res;
}


TEE_Result setPowerParams(uint32_t 		 paramTypes, TEE_Param params[4])
{
	TEE_Result res = TEE_SUCCESS;

	if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_INPUT ||
		TEE_PARAM_TYPE_GET(paramTypes, 3) != TEE_PARAM_TYPE_VALUE_INPUT) {
		printf("<<[GPU TA]>> %s Bad parameter\n", __FUNCTION__);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	sPCAddr    			    = (uint64_t)params[0].value.a;
	sGPURegAddr 			= (uint64_t)params[0].value.b;
	sBootRemapAddr 			= (uint64_t)params[1].value.a;
	sCodeRemapAddr 			= (uint64_t)params[1].value.b;
	sDataRemapAddr          = (uint64_t)params[2].value.a;

	if(sGPURegAddr>>32 != 0) {
		printf("sGPURegAddr should not large then 32Bits\n");
		assert(0);
	}
	res = TEE_Map_Register((void*)(unsigned int)sGPURegAddr, 0x80000, 0, 0, &pvRegsVirAddr);
	return res;
}

TEE_Result setConfigParams(	uint32_t		commandID, uint32_t	paramTypes, TEE_Param params[4]) {
	TEE_Result res = TEE_SUCCESS;

	switch (commandID) {
		case GPU_CMD_SET_bDevicePA0IsValid: {
			if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
				printf("<<[GPU TA]>> For GPU_CMD_SET_bDevicePA0IsValid paramter must be value\n");
				return TEE_ERROR_BAD_PARAMETERS;
			}

			if(bDevicePA0IsValid != params[0].value.a)
				printf("<<[GPU TA]>> bDevicePA0IsValid=%d, update to %d\n", (int)bDevicePA0IsValid, (int)params[0].value.a);

			bDevicePA0IsValid = params[0].value.a;
			break;
		}
		case GPU_CMD_SET_ui64DevErnsBrns: {
			long long new_value = (long long)params[0].value.a<<32 | (long long)params[0].value.b;
			if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
				printf("<<[GPU TA]>> For GPU_CMD_SET_ui64DevErnsBrns paramter must be value\n");
				return TEE_ERROR_BAD_PARAMETERS;
			}

			if(ui64DevErnsBrns != new_value)
				printf("<<[GPU TA]>> ui64DevErnsBrns=0x%x%x, update to 0x%x%x\n",
							(unsigned int)(ui64DevErnsBrns>>32),
							(unsigned int)(ui64DevErnsBrns&0xFFFFFFFF),
							(unsigned int)params[0].value.a,
							(unsigned int)params[0].value.b);

			ui64DevErnsBrns = new_value;
			break;
		}
		case GPU_CMD_SET_ui64DevFeatures: {
			long long new_value = (long long)params[0].value.a<<32 | (long long)params[0].value.b;
			if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
				printf("<<[GPU TA]>> For GPU_CMD_SET_ui64DevFeatures paramter must be value\n");
				return TEE_ERROR_BAD_PARAMETERS;
			}

			if(ui64DevFeatures != new_value)
				printf("<<[GPU TA]>> ui64DevFeatures=0x%x%x, update to 0x%x%x\n",
							(unsigned int)(ui64DevFeatures>>32),
							(unsigned int)(ui64DevFeatures&0xFFFFFFFF),
							(unsigned int)params[0].value.a,
							(unsigned int)params[0].value.b);

			//ui64DevFeatures = new_value;
			break;
		}
		case GPU_CMD_SET_ui64DevFeaturesValue: {
			unsigned int type = params[0].value.a;
			unsigned int new_value     = params[0].value.b;
			if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
				printf("<<[GPU TA]>> For GPU_CMD_SET_ui64DevFeatures paramter must be value\n");
				return TEE_ERROR_BAD_PARAMETERS;
			}

			if(type >= RGX_FEATURE_WITH_VALUES_MAX_IDX)
				printf("<<[GPU TA]>> Features type(%d) can not larger then RGX_FEATURE_WITH_VALUES_MAX_IDX(%d)\n",
							type, RGX_FEATURE_WITH_VALUES_MAX_IDX);
			if(ui32FeaturesValues[type] != new_value) {
				printf("<<[GPU TA]>> ui32FeaturesValues[%d] update to 0x%u, original is %lu\n",
							type, new_value, ui32FeaturesValues[type]);
			}
			ui32FeaturesValues[type] = new_value;
			break;
		}
	}

	return res;
}

TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(
	void*			sessionContext,
	uint32_t		commandID,
	uint32_t		paramTypes,
	TEE_Param		params[4])
{
	TEE_Result res = TEE_SUCCESS;

	switch (commandID) {
		case GPU_CMD_SEND_IMAGE:{
			res = copyFwImage(paramTypes, params);
			break;
		}
		case GPU_CMD_GET_CODE_SIZE:{
			params[0].value.a = codeSize;
			break;
		}
		case GPU_CMD_GET_DATA_SIZE:{
			params[0].value.a = dataSize;
			break;
		}
		case GPU_CMD_SET_FW_PARAMS:{
			res = setFwParams(paramTypes, params);
			break;
		}
		case GPU_CMD_SET_FW_PageTableAddr:{
			res = setFWPageTableAddr(paramTypes, params);
			break;
		}
		case GPU_CMD_SET_POWER_PARAMS: {
			res = setPowerParams(paramTypes, params);
			break;
		}
		case GPU_CMD_SET_bDevicePA0IsValid:
		case GPU_CMD_SET_ui64DevErnsBrns:
		case GPU_CMD_SET_ui64DevFeatures:
		case GPU_CMD_SET_ui64DevFeaturesValue:{
			setConfigParams(commandID, paramTypes, params);
			break;
		}
		case GPU_CMD_RGX_START:{
			RGXStart(NULL);
			break;
		}
		case GPU_CMD_RGX_STOP:{
			RGXStop(NULL);
			break;
		}
	    case GPU_CMD_RGX_GFX_CORE_CLOCK: {
#if defined(PLATYPUS)
	       if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
	               printf("<<[GPU TA]>> %s Bad parameter for GPU_CMD_RGX_GFX_CORE_CLOCK\n", __FUNCTION__);
	               return TEE_ERROR_BAD_PARAMETERS;
	       }
	       unsigned int value = params[0].value.a;
	       unsigned int clock_value = *gfx_core_clock;
	       if(value == 0) {
	               *gfx_core_clock = clock_value & 0xFFFFFFFE;
	       } else {
	               *gfx_core_clock = clock_value | 0x1;
	       }
	       break;
#else
	       printf("<<[GPU TA]>> DOLPHIN can not touch gfx core clock in TA\n");
	       assert(0);
#endif
		}
		case GPU_CMD_SET_BVNC: {
			if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
				TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT) {
				printf("<<[GPU TA]>> %s Bad parameter for GPU_CMD_SET_BVNC\n", __FUNCTION__);
				return TEE_ERROR_BAD_PARAMETERS;
			}

			if( ui32B != params[0].value.a ||
				ui32V != params[0].value.b ||
				ui32N != params[1].value.a ||
				ui32C != params[1].value.b) {
				printf("<<[GPU TA]>> Kernel BVNC as:%u.%u.%u.%u, TA BNVC as:%u.%u.%u.%u. Do not match\n",
					(unsigned int)ui32B, (unsigned int)ui32V, (unsigned int)ui32N, (unsigned int)ui32C,
					(unsigned int)params[0].value.a, (unsigned int)params[0].value.b, (unsigned int)params[1].value.a, (unsigned int)params[1].value.b);
				assert(0);
			}
		}
	}

	return res;
}

#ifndef CONFIG_OPTEE
TA_DEFINE_BEGIN
	.uuid                     = TAPVR_UUID,
	.name                     = "pvr_fw_load",
	.singleInstance           = false,
	.multiSession             = false,
	.instanceKeepAlive        = false,
TA_DEFINE_END
#endif
