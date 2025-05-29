// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2025 Synaptics Incorporated */

#include "vpp_api.h"
#include "util/types.h"
#include "core/video_params.h"
#include "hdcp/hdcp_params.h"
#include "SNPS_HDMI_tx.h"
#include "param_map.h"

int SYN_mapVppColorFmt(int aColorFmt) {
    switch(aColorFmt) {

    case OUTPUT_COLOR_FMT_RGB888:
        return RGB;

    case OUTPUT_COLOR_FMT_YCBCR444:
    case OUTPUT_COLOR_FMT_IPT444:
    case OUTPUT_COLOR_FMT_YCBCR444_P3D65:
    case OUTPUT_COLOR_FMT_YCBCR444_R2020:
        return YCC444;

    case OUTPUT_COLOR_FMT_YCBCR422:
    case OUTPUT_COLOR_FMT_IPT422:
    case OUTPUT_COLOR_FMT_YCBCR422_P3D65:
    case OUTPUT_COLOR_FMT_YCBCR422_R2020:
        return YCC422;

    case OUTPUT_COLOR_FMT_YCBCR420:
    case OUTPUT_COLOR_FMT_YCBCR420_P3D65:
    case OUTPUT_COLOR_FMT_YCBCR420_R2020:
        return YCC420;

    default:
        return ENC_UNDEFINED;
    }
}

int SYN_mapVppBitDepth(int aBitDepth) {
    switch(aBitDepth) {
    case OUTPUT_BIT_DEPTH_8BIT:
        return COLOR_DEPTH_8;

    case OUTPUT_BIT_DEPTH_10BIT:
        return COLOR_DEPTH_10;

    case OUTPUT_BIT_DEPTH_12BIT:
        return COLOR_DEPTH_12;

    default:
        return COLOR_DEPTH_INVALID;
    }
}
