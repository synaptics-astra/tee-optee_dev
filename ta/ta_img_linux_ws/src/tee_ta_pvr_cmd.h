// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Synaptics Incorporated

#ifndef _PVR_CMD_H_
#define _PVR_CMD_H_

#ifndef CONFIG_SO
#define TAPVR_UUID	{0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x36}}
#else
#define TAPVR_UUID  {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x36}}
#endif

enum {
	GPU_CMD_SEND_IMAGE               = 0,
	GPU_CMD_GET_CODE_SIZE            = 1,
	GPU_CMD_GET_DATA_SIZE            = 2,
	GPU_CMD_SET_FW_PARAMS            = 3,
	GPU_CMD_SET_FW_PageTableAddr     = 4,
	GPU_CMD_SET_POWER_PARAMS         = 5,
	GPU_CMD_SET_bDevicePA0IsValid    = 6,
	GPU_CMD_SET_ui64DevErnsBrns      = 7,
	GPU_CMD_SET_ui64DevFeatures      = 8,
	GPU_CMD_SET_ui64DevFeaturesValue = 9,
	GPU_CMD_RGX_START                = 10,
	GPU_CMD_RGX_STOP                 = 11,
	GPU_CMD_RGX_GFX_CORE_CLOCK       = 12,
	GPU_CMD_SET_BVNC                 = 13,
};

#endif /* _PVR_CMD_H_ */
