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

////////////////////////////////////////////////////////////////////////////////
//! \file tsp_parse_gop_hdr.c
//!
//! \brief Gop Header parse Implementation
//!
//!
//!     Version     Date                    Author
//!     V 1.00,     May 25 2020,            wangzl
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifdef TA_COMPILE
#define LOG_TAG "TSP_TA"
#include "config.h"
#include "tsp_config_log.h"
#include "ta_log.h"

typedef  unsigned char  BOOL;
#define  FALSE 0
#define  TRUE  1
#endif

#include "tsp_parse_gop_hdr.h"

static BOOL internal_check_gop_hdr_mpeg2(uint8_t *pData, uint32_t uSize)
{
    BOOL ret = FALSE;
    //BOOL bIFrame FALSE;
    // make sure it's valid header
    uint32_t data_cnt = 0, stream_id = 0, pic_type = 0;

    if(pData == NULL) {
        TA_LOGE("Invalid parameter\n");
        return ret;
    }

    //skip zeros
    while (pData[data_cnt] == 0 && data_cnt<uSize ) {
        data_cnt ++;
    }

    if(data_cnt < 2 || pData[data_cnt] != 1) {

        TA_LOGE("Invalid data\n");
        return ret;
    }
    data_cnt ++;
    stream_id = pData[data_cnt];

    //bIFrame = FALSE;
    switch (stream_id) {
        case MPEG2_SEQUENCE_HEADER_CODE: {
                ret = TRUE;
            }
            break;
        case MPEG2_PICTURE_START_CODE:
            pic_type = (pData[data_cnt + 2] >> 3) & 0x7;
            if(pic_type == MPEG2_PIC_CODING_TYPE_I) {
                //*bIFrame = TRUE; //may previous unit has sequence header, will refine it later
                ret = TRUE;
            }
            break;
        default:
            break;
    }
    return ret;
}


static BOOL internal_check_gop_hdr_h265(uint8_t *pData, uint32_t uSize)
{
    // make sure it's valid header
    uint32_t data_cnt = 0;
    NALU_T_H265 nalu;
    uint32_t v;


    if(pData == NULL || uSize < 5) {
        TA_LOGE("Invalid parameter\n");
        return FALSE;
    }

    while(1) {
        while(1){
            if(pData[data_cnt++] == 0) {
                if(pData[data_cnt] == 0) {
                    if(pData[data_cnt+1] == 1){
                        data_cnt++;
                        break;
                    }
                }else {
                    data_cnt++;
                }
            }
            if(data_cnt > (uSize-5))
                return FALSE;
        };

        data_cnt ++;
        v = pData[data_cnt] & 0xff;
        data_cnt++;
        v = (v << 8) | (pData[data_cnt] & 0xff);
        nalu.nal_unit_type = (v >> 9) & 0x3f;

        switch(nalu.nal_unit_type)
        {
            case H265_NALU_SPS_NUT:
                return TRUE;
            case H265_NALU_TRAIL_N:
            case H265_NALU_TRAIL_R:
            case H265_NALU_TSA_N:
            case H265_NALU_TSA_R:
            case H265_NALU_STSA_N:
            case H265_NALU_STSA_R:
            case H265_NALU_RADL_N:
            case H265_NALU_RADL_R:
            case H265_NALU_RASL_N:
            case H265_NALU_RASL_R:
            case H265_NALU_BLA_N_LP:
            case H265_NALU_BLA_W_LP:
            case H265_NALU_BLA_W_RADL:
            case H265_NALU_IDR_N_LP:
            case H265_NALU_IDR_W_RADL:
            case H265_NALU_CRA_NUT:
                return FALSE;
            default:
                break;
        }
        if(data_cnt > (uSize-5))
            return FALSE;
    }
    return FALSE;
}

static BOOL internal_check_gop_hdr_h264(uint8_t *pData, uint32_t uSize)
{
    // make sure it's valid header
    uint32_t data_cnt = 0;
    NALU_T_H264 nalu;

    if(pData == NULL || uSize < 4) {
        TA_LOGE("Invalid parameter\n");
        return FALSE;
    }

    TA_LOGL("\n------ <h264> New Unit to check gop head size= 0x%x-----\n",uSize);
    while(1) {
        while(1){
            if(pData[data_cnt++] == 0) {
                if(pData[data_cnt] == 0) {
                    if(pData[data_cnt+1] == 1){
                        data_cnt++;
                        break;
                    }
                }else {
                    data_cnt++;
                }
            }
            if(data_cnt > (uSize-4))
                return FALSE;
        };

        data_cnt ++;
        //now data_cnt points to the byte next to 00 00 01

        nalu.nal_unit_type     = (pData[data_cnt] & 0x1f);

        TA_LOGL("Found nalu type0x%x pos=0x%x\n",
            nalu.nal_unit_type, data_cnt);
        switch (nalu.nal_unit_type) {
            case H264_NALU_SPS:
                return TRUE;
            case H264_NALU_SLICE_NONIDR:
            case H264_NALU_SLICE_PART_A:
            case H264_NALU_SLICE_IDR:
            case H264_NALU_SLICE_AUX:
            case H264_NALU_SLICE_PART_B:
            case H264_NALU_SLICE_PART_C:
                return FALSE;
            default:
                break;
        }
        if(data_cnt > (uSize-4))
            return FALSE;
    }
    return FALSE;
}

int tsp_check_gop_hdr(uint8_t *pData, uint32_t uSize, uint32_t uCodec)
{
    int bFoundGopHdr = FALSE;

    switch(uCodec) {
        case CODEC_MEPG2:
            bFoundGopHdr = internal_check_gop_hdr_mpeg2(pData, uSize);
            break;
        case CODEC_H264:
            bFoundGopHdr = internal_check_gop_hdr_h264(pData, uSize);
            break;
        case CODEC_H265:
            bFoundGopHdr = internal_check_gop_hdr_h265(pData, uSize);
            break;
        default:
            TA_LOGH("invalid video codec 0x%x\n", uCodec);
            break;
    }
    return bFoundGopHdr;

}
