/*
 * INTERNAL USE ONLY
 *
 * Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
 * All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 */

#ifndef _TEE_TSP_PARSE_GOP_HDR_H_
#define _TEE_TSP_PARSE_GOP_HDR_H_

#ifdef TA_COMPILE
#include "tsp_ta_internal.h"
#include "tee_internal_api.h"
#else
#include "dmx_util.h"

#define TA_LOGF(...)  AMPLOG(MODULE_DMX, AMP_LOG_FATAL, __VA_ARGS__)
#define TA_LOGE(...)  AMPLOG(MODULE_DMX, AMP_LOG_ERROR, __VA_ARGS__)
#define TA_LOGH(...)  AMPLOG(MODULE_DMX, AMP_LOG_HIGH, __VA_ARGS__)
#define TA_LOGM(...)  AMPLOG(MODULE_DMX, AMP_LOG_MID, __VA_ARGS__)
#define TA_LOGL(...)  AMPLOG(MODULE_DMX, AMP_LOG_LOW, __VA_ARGS__)
#define TA_LOGI(...)  AMPLOG(MODULE_DMX, AMP_LOG_INFO, __VA_ARGS__)
#define TEE_Result uint32_t

#endif

#define H264_NALU_INVALID          (0)
#define H264_NALU_SLICE_NONIDR     (1)
#define H264_NALU_SLICE_PART_A     (2)
#define H264_NALU_SLICE_PART_B     (3)
#define H264_NALU_SLICE_PART_C     (4)
#define H264_NALU_SLICE_IDR        (5)
#define H264_NALU_SEI              (6)
#define H264_NALU_SPS              (7)
#define H264_NALU_PPS              (8)
#define H264_NALU_AUD              (9)
#define H264_NALU_END_SEQ          (10)
#define H264_NALU_END_STREAM       (11)
#define H264_NALU_FILTER_DATA      (12)
#define H264_NALU_SPS_EXT          (13)
#define H264_NALU_SLICE_AUX        (19)

#define H264_SLICE_P               (20)
#define H264_SLICE_B               (21)
#define H264_SLICE_I               (22)
#define H264_SLICE_OTHER           (23)
#define H264_NALU_UNDEFINED        (24)

typedef struct __nalu_t_h264__ {
    uint32_t forbidden_bit;
    uint32_t nal_reference_idc;
    uint32_t nal_unit_type;
} NALU_T_H264;

// MPEG2 start code values.
#define MPEG2_SEQUENCE_HEADER_CODE     (0xB3)
#define MPEG2_GOP_START_CODE           (0xB8)
#define MPEG2_PICTURE_START_CODE       (0x00)
#define MPEG2_PIC_CODING_TYPE_I        (0x01)
#define MPEG2_PIC_CODING_TYPE_P        (0x02)
#define MPEG2_PIC_CODING_TYPE_B        (0x03)

/***********************************/
//Structure for HEVC(H.265)
#define H265_NALU_TRAIL_N              (0)
#define H265_NALU_TRAIL_R              (1)
#define H265_NALU_TSA_N                (2)
#define H265_NALU_TSA_R                (3)
#define H265_NALU_STSA_N               (4)
#define H265_NALU_STSA_R               (5)
#define H265_NALU_RADL_N               (6)
#define H265_NALU_RADL_R               (7)
#define H265_NALU_RASL_N               (8)
#define H265_NALU_RASL_R               (9)
#define H265_NALU_BLA_W_LP             (16)
#define H265_NALU_BLA_W_RADL           (17)
#define H265_NALU_BLA_N_LP             (18)
#define H265_NALU_IDR_W_RADL           (19)
#define H265_NALU_IDR_N_LP             (20)
#define H265_NALU_CRA_NUT              (21)
#define H265_NALU_RSV_IRAP_VCL22       (22)
#define H265_NALU_RSV_IRAP_VCL23       (23)
#define H265_NALU_VPS_NUT              (32)
#define H265_NALU_SPS_NUT              (33)
#define H265_NALU_PPS_NUT              (34)
#define H265_NALU_AUD_NUT              (35)
#define H265_NALU_EOS_NUT              (36)
#define H265_NALU_EOB_NUT              (37)
#define H265_NALU_FD_NUT               (38)
#define H265_NALU_PREFIX_SEI_NUT       (39)
#define H265_NALU_SUFFIX_SEI_NUT       (40)

#define H265_SLICE_P                   (41)
#define H265_SLICE_B                   (42)
#define H265_SLICE_I                   (43)
#define H265_SLICE_OTHER               (44)
#define H265_NALU_UNDEFINED            (45)

typedef struct __nalu_t_h265__ {
    uint32_t forbidden_bit;
    uint32_t nal_unit_type;
    uint32_t nuh_layer_id;
    uint32_t nuh_temporal_id_plus1;
} NALU_T_H265;

#define CODEC_MEPG2 17
#define CODEC_H264  22
#define CODEC_H265  36
#define IS_VALID_CODEC(codec) (codec == CODEC_MEPG2 || codec == CODEC_H264 || codec == CODEC_H265)

#ifdef __cplusplus
extern "C"
{
#endif

int tsp_check_gop_hdr(uint8_t *pData, uint32_t uSize, uint32_t uCodec);

#ifdef __cplusplus
}
#endif

#endif /* _TEE_TSP_PARSE_GOP_HDR_H_ */
