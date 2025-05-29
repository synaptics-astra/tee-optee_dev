// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synopsys, Inc. */
/* Copyright (C) 2025 Synaptics Incorporated */

#include "bsp/eddc_reg.h"
#include "edid/edid.h"


#include "../../include/edid/edid_type.h"
#include "util/bit_operation.h"
#include "util/log.h"
#include "util/error.h"
#include "bsp/i2cm.h"
#include <string.h>


#define EDID_I2C_ADDR  		0x50
#define EDID_I2C_SEGMENT_ADDR  	0x30
#define EDID_I2C_CSUM_ADDR      0x7F

int _edid_checksum(u8 * edid)
{
	int i, checksum = 0;

	for(i = 0; i < EDID_LENGTH; i++)
		checksum += edid[i];

	return checksum % 256; //CEA-861 Spec
}

int edid_read(hdmi_tx_dev_t *dev, u8 * edid)
{
	int error = 0;
	const u8 header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	error = ddc_read(dev, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR, 0, 0, 128, (u8 *)edid);
	if(error){
		LOGGER(SNPS_ERROR, "EDID read failed");
		return error;
	}
	error = memcmp((u8 * ) edid, (u8 *) header, sizeof(header));
	if(error){
		LOGGER(SNPS_ERROR, "EDID header check failed");
		return error;
	}

	error = _edid_checksum((u8 *) edid);
	if(error){
		LOGGER(SNPS_ERROR, "EDID checksum failed");
		return error;
	}
	return 0;
}

int edid_cs_read(hdmi_tx_dev_t *dev, int blks, u8 * cs)
{
	int error = 0;

	for(int i = 0;i <= blks;i++){
		u8 start_pointer = i / 2; // pointer to segments of 256 bytes
		u8 start_address = ((i % 2) * 0x80)+ EDID_I2C_CSUM_ADDR; //offset in segment; first block 0-127; second 128-255
		error = ddc_read(dev, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR, start_pointer, start_address, 1, (u8 *)cs);
		cs++;
		if(error){
			LOGGER(SNPS_ERROR, "Checksum read failed");
			return error;
		}
	}
	return error;
}


int edid_extension_read(hdmi_tx_dev_t *dev, int block, u8 * edid_ext)
{
	int error = 0;
	/*to incorporate extensions we have to include the following - see VESA E-DDC spec. P 11 */
	u8 start_pointer = block / 2; // pointer to segments of 256 bytes
	u8 start_address = ((block % 2) * 0x80); //offset in segment; first block 0-127; second 128-255

	error = ddc_read(dev, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR, start_pointer, start_address, 128, edid_ext);
	if(error){
		LOGGER(SNPS_ERROR, "EDID extension read failed");
		return error;
	}

	error = _edid_checksum(edid_ext);
	if(error){
		LOGGER(SNPS_ERROR, "EDID extension checksum failed");
		return error;
	}
	return 0;
}

