/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

#include <stdlib.h>
#include "com_type.h"
#include "vpp_api.h"
#include "vpp_module.h"
#include "vpp_cfg.h"
#include "vpp_be_hdmitx.h"
#include "hdmi_eddc.h"

#include "bsp/i2cm.h"

#include "SNPS_HDMI_tx.h"

#define DDC_EDID_ADDR 0x50
#define DDC_HDCP_ADDR 0x3A
#define DDC_SCDC_ADDR 0x54

void EDDC_InitializeMaster(BE_HDMITX_DATA  *pHdmiTx, int mode)
{
    /* mode 0 - standard mode : 1 - fast mode */
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return;
    }

    i2cddc_clk_config(&snps_hdmi->hdmi_tx, 2500, I2C_MIN_SS_SCL_LOW_TIME, I2C_MIN_SS_SCL_HIGH_TIME, I2C_MIN_FS_SCL_LOW_TIME, I2C_MIN_FS_SCL_HIGH_TIME);
    i2cddc_fast_mode(&snps_hdmi->hdmi_tx, mode);
}

int EDDC_Read(BE_HDMITX_DATA  *pHdmiTx, UINT8 mode, UINT16 offset, INT16 count, UINT8 *msg) {

    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    switch(mode) {
        case EDDC_SLAVE_HDCP:
            return ddc_read(&snps_hdmi->hdmi_tx, DDC_HDCP_ADDR, 0, 0 , offset, count, msg);
        break;
        case EDDC_SLAVE_SCDC:
            return ddc_read(&snps_hdmi->hdmi_tx, DDC_SCDC_ADDR, 0, 0 , offset, count, msg);
        break;
    }
    return 0;
}

int EDDC_Write(BE_HDMITX_DATA  *pHdmiTx,UINT8 mode, UINT16 offset, UINT16 count, UINT8 *msg)
{
    SNPS_HDMI_TX *snps_hdmi = pHdmiTx->vendorHdmiTx;

    if(NULL == snps_hdmi) {
        VPPHDMI_DBG_ERROR("SNPS_HDMI_TX not initialized, exiting..\n");
        return MV_VPP_EUNCONFIG;
    }
    switch(mode) {
        case EDDC_SLAVE_HDCP:
            return ddc_write(&snps_hdmi->hdmi_tx, DDC_HDCP_ADDR, offset, count, msg);
        break;
        case EDDC_SLAVE_SCDC:
            return ddc_write(&snps_hdmi->hdmi_tx, DDC_SCDC_ADDR, offset, count, msg);
        break;
    }
    return 0;
}

void Destroy_EDDC(VPP_OBJ *vpp_obj)
{
}
