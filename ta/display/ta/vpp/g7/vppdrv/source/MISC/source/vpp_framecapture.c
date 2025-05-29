/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifdef VPP_ENABLE_FRAME_CAPTURE
#include "vpp_module.h"
#include "vpp.h"
#include "vpp_framecapture.h"
#include "vpp_env_ur.h"

#if (CONFIG_AMP_TEE_ENABLE)
#include "vpp_cmd.h"
#include "tee_internal_api.h"
#endif

/* dump the frame in the format same as source format */
#ifndef CONFIG_CAPTURE_YUV422_MODE
static void  VPP_CaptureFrameNotifyUser(CAPTURE_VID_FRAME *pCaptureVidFrm,
                                                  VBUF_INFO *pFrmDescr, UINT8 *pIntBuf, INT planeID)
{
    void * src_addr;
    void * uvsrc_addr=NULL;
    UINT32 buf_size, final_size, copy_size, i, j, k;
    UINT32 bytes_of_one_line;
    UINT32 bytes_of_one_cap_line;
    UINT8 bytes_per_pixel, capture_mode;
    UINT8 tile_x_size, tile_y_size;
    UINT8 is_progressive_frame;
    UINT8 * src0;
    UINT8 * src1;
    UINT8 * src2;
    UINT8 * src3;
    UINT8 * des;
    UINT8 * inter_buf;

#if (CONFIG_AMP_TEE_ENABLE)
    UINT32 cb_cmdID = VPP_CB_NOTIFYCAPVIDFRAM;
    UINT32 cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                    TEE_PARAM_TYPE_VALUE_INPUT,
                    TEE_PARAM_TYPE_VALUE_INPUT,
                    TEE_PARAM_TYPE_VALUE_INPUT);
    TEE_Param cb_params[4];
#endif

    if((!pCaptureVidFrm->gCapFrameSetting.pFrmBuf)
        || (!pFrmDescr)
        || (pFrmDescr->m_bits_per_pixel != 8)
        || (pCaptureVidFrm->gCapFrameSetting.CapWin.width < 4)
        || (pCaptureVidFrm->gCapFrameSetting.CapWin.height < 4)){
        goto capture_frm_err_exit;
    }

    if((pFrmDescr->m_is_frame_seq)
        ||(IS_GFX_SRC_FORMAT(pFrmDescr->m_srcfmt))){
        is_progressive_frame = 1; //progressive
    }
    else{
        is_progressive_frame = 0; //interlaced
    }
    //capture window need be EVEN value
    if(pFrmDescr->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0){
        tile_x_size = 8;
        tile_y_size = 4;
        pCaptureVidFrm->gCapFrameSetting.CapWin.x /= tile_x_size;
        pCaptureVidFrm->gCapFrameSetting.CapWin.x *= tile_x_size;
        pCaptureVidFrm->gCapFrameSetting.CapWin.width /= tile_x_size;
        pCaptureVidFrm->gCapFrameSetting.CapWin.width *= tile_x_size;
        pCaptureVidFrm->gCapFrameSetting.CapWin.y /= (tile_y_size*2);
        pCaptureVidFrm->gCapFrameSetting.CapWin.y *= (tile_y_size*2);
        pCaptureVidFrm->gCapFrameSetting.CapWin.height /= (tile_y_size*2);
        pCaptureVidFrm->gCapFrameSetting.CapWin.height *= (tile_y_size*2);
    }
    else{
        pCaptureVidFrm->gCapFrameSetting.CapWin.x &= 0xFFFE;
        pCaptureVidFrm->gCapFrameSetting.CapWin.width &= 0xFFFE;
        pCaptureVidFrm->gCapFrameSetting.CapWin.y &= 0xFFFE;
        pCaptureVidFrm->gCapFrameSetting.CapWin.height &= 0xFFFE;
    }

    buf_size = pCaptureVidFrm->gCapFrameSetting.CapWin.width*pCaptureVidFrm->gCapFrameSetting.CapWin.height;

    if(buf_size == 0){
        goto capture_frm_err_exit;
    }

    switch(pFrmDescr->m_srcfmt)
    {
        case SRCFMT_YUV420SP_TILED_MODE0:
            buf_size = buf_size*3/2;
            bytes_per_pixel = 1; //Y
            final_size = buf_size/3*4; // Y and 2*UV
            capture_mode = VPP_CAPTURE_YUV420SP_TILE_HD_MODE;
            pCaptureVidFrm->gCapFrameInfo.ColorFormat = SRCFMT_YUV420SP_TILED_MODE0;
            pCaptureVidFrm->gCapFrameInfo.ColorOrder = pFrmDescr->m_order;
            break;

        case SRCFMT_YUV420SP:
            buf_size = buf_size*3/2;
            bytes_per_pixel = 1; //Y
            final_size = buf_size/3*4; // Y and 2*UV
            capture_mode = VPP_CAPTURE_YUV420SP_HD_MODE;
            pCaptureVidFrm->gCapFrameInfo.ColorFormat = SRCFMT_YUV420SP;
            pCaptureVidFrm->gCapFrameInfo.ColorOrder = pFrmDescr->m_order;
            break;

        case SRCFMT_YUV422:
            buf_size *= 2;
            bytes_per_pixel = 2;
            capture_mode = VPP_CAPTURE_YUV422_HD_MODE;
            final_size = buf_size;
            pCaptureVidFrm->gCapFrameInfo.ColorFormat = SRCFMT_YUV422;
            pCaptureVidFrm->gCapFrameInfo.ColorOrder = pFrmDescr->m_order;
            break;

        case SRCFMT_ARGB32:
        case SRCFMT_ARGB32_PM:
        case SRCFMT_XRGB32:
        case SRCFMT_IPT444:
            buf_size *= 4;
            bytes_per_pixel = 4;
            capture_mode = VPP_CAPTURE_ARGB_MODE;
            final_size = buf_size/4;
            pCaptureVidFrm->gCapFrameInfo.ColorFormat = pFrmDescr->m_srcfmt;
            pCaptureVidFrm->gCapFrameInfo.ColorOrder = pFrmDescr->m_order;
            break;

        default:
            goto capture_frm_err_exit;
    }

    if((final_size > pCaptureVidFrm->gCapFrameSetting.FrmBufSize)
        || (buf_size > CAPTURE_BUFFER_SIZE)
        ||(TEE_CheckMemoryOutputRights((void *)pCaptureVidFrm->gCapFrameSetting.pFrmBuf,
            final_size, TEE_NONSECURE, TEE_MT_COPY)!=TEE_SUCCESS)){
        goto capture_frm_err_exit;
    }

    inter_buf = pIntBuf;
    if(!inter_buf){
        goto capture_frm_err_exit;
    }

    /* Check secure playback status VSSDK-3756 */
    if(!VPP_ENV_SA_isSecurePlayback(0, planeID)) {
        //copy video frame to internal buffer
        if(pFrmDescr->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0){
            bytes_of_one_line = pFrmDescr->m_buf_stride*tile_y_size;
            bytes_of_one_cap_line = (pCaptureVidFrm->gCapFrameSetting.CapWin.width*tile_x_size/2)*bytes_per_pixel
                                    *tile_y_size;
            src_addr = (void *)pFrmDescr->m_pbuf_start
                       + pCaptureVidFrm->gCapFrameSetting.CapWin.y/tile_y_size*bytes_of_one_line;
            src_addr += (UINT32)(pCaptureVidFrm->gCapFrameSetting.CapWin.x*bytes_per_pixel
                                *tile_y_size);
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/tile_y_size; i++)
            {
                memcpy((void *)inter_buf, src_addr, bytes_of_one_cap_line);
                inter_buf += bytes_of_one_cap_line;
                //only progressive
                src_addr += bytes_of_one_line;
            }
        }
        else{
            bytes_of_one_line = pFrmDescr->m_buf_stride;
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width*bytes_per_pixel;
            if(is_progressive_frame){
                //progressive
                src_addr = pFrmDescr->m_pbuf_start
                       + pCaptureVidFrm->gCapFrameSetting.CapWin.y*bytes_of_one_line;
            }
            else{
                src_addr = pFrmDescr->m_pbuf_start
                       + pCaptureVidFrm->gCapFrameSetting.CapWin.y*bytes_of_one_line*2;
            }
            src_addr += (UINT32)(pCaptureVidFrm->gCapFrameSetting.CapWin.x*bytes_per_pixel);
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height; i++)
            {
                memcpy((void *)inter_buf, src_addr, bytes_of_one_cap_line);
                inter_buf += bytes_of_one_cap_line;
                if(is_progressive_frame)//progressive
                    src_addr += bytes_of_one_line;
                else
                    src_addr += bytes_of_one_line*2;
            }
        }

        //copy UV for YUV420SP
        if(pFrmDescr->m_srcfmt == SRCFMT_YUV420SP){
            uvsrc_addr = inter_buf; //save start of UV
            bytes_of_one_line = pFrmDescr->m_buf_stride_UV;
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
            src_addr = (void *)pFrmDescr->m_buf_pbuf_start_UV;
            src_addr += pCaptureVidFrm->gCapFrameSetting.CapWin.y/2*bytes_of_one_line;
            src_addr += pCaptureVidFrm->gCapFrameSetting.CapWin.x;
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/2; i++){
                memcpy((void *)inter_buf, src_addr, bytes_of_one_cap_line);
                inter_buf += bytes_of_one_cap_line;
                if(is_progressive_frame)//progressive
                    src_addr += bytes_of_one_line;
                else
                    src_addr += bytes_of_one_line*2;
            }
        }
        if(pFrmDescr->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0){
            uvsrc_addr = inter_buf; //save start of UV
            bytes_of_one_line = pFrmDescr->m_buf_stride_UV*tile_y_size;
            bytes_of_one_cap_line = (pCaptureVidFrm->gCapFrameSetting.CapWin.width*tile_x_size/2)*tile_y_size;
            src_addr = (void *)pFrmDescr->m_buf_pbuf_start_UV;
            src_addr += pCaptureVidFrm->gCapFrameSetting.CapWin.y/2/tile_y_size*bytes_of_one_line;
            src_addr += pCaptureVidFrm->gCapFrameSetting.CapWin.x*tile_y_size;
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/2/tile_y_size; i++){
                memcpy((void *)inter_buf, src_addr, bytes_of_one_cap_line);
                inter_buf += bytes_of_one_cap_line;
                src_addr += bytes_of_one_line;
            }
        }
    }
    copy_size = 0;
    //goto start of internal buffer
    src_addr = pIntBuf;
    if(!src_addr){
        goto capture_frm_err_exit;
    }

    bytes_of_one_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width*bytes_per_pixel;
    des = (UINT8 *)pCaptureVidFrm->gCapFrameSetting.pFrmBuf;

    switch(capture_mode){

        case VPP_CAPTURE_YUV420SP_TILE_HD_MODE:
            bytes_of_one_cap_line = (pCaptureVidFrm->gCapFrameSetting.CapWin.width*tile_x_size)*bytes_per_pixel*tile_y_size;

            /* Copy Y */
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/tile_y_size; i++)
            {
                memcpy((void *)des, src_addr, bytes_of_one_cap_line);
                des += bytes_of_one_cap_line;
                src_addr += bytes_of_one_cap_line;
                copy_size += bytes_of_one_cap_line;
            }
            /* Copy UV */
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/tile_y_size/2; i++)
            {
                memcpy((void *)des, uvsrc_addr, bytes_of_one_cap_line);
                des += bytes_of_one_cap_line;
                uvsrc_addr += bytes_of_one_cap_line;
                copy_size += bytes_of_one_cap_line;
            }

            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height;
            break;

        case VPP_CAPTURE_YUV420SP_HD_MODE:
            /* Copy Y */
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width*bytes_per_pixel;
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height; i++)
            {
                memcpy((void *)des, src_addr, bytes_of_one_cap_line);
                des += bytes_of_one_cap_line;
                src_addr += bytes_of_one_cap_line;
                copy_size += bytes_of_one_cap_line;
            }
            /* Copy UV */
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/2; i++)
            {
                memcpy((void *)des, uvsrc_addr, bytes_of_one_cap_line);
                des += bytes_of_one_cap_line;
                uvsrc_addr += bytes_of_one_cap_line;
                copy_size += bytes_of_one_cap_line;
            }

            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width/CONFIG_VPP_YUV_CAPTURE_RATIO;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height/CONFIG_VPP_YUV_CAPTURE_RATIO;
            break;

        case VPP_CAPTURE_YUV422_HD_MODE:
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width * bytes_per_pixel;
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height; i++){
                memcpy((void *)des, src_addr, bytes_of_one_cap_line);
                des += bytes_of_one_cap_line;
                src_addr += bytes_of_one_cap_line;
                copy_size += bytes_of_one_cap_line;
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height;
            break;

        case VPP_CAPTURE_ARGB_MODE:
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/CONFIG_VPP_ARGB_CAPTURE_RATIO; i++){
                for(j=0; j<pCaptureVidFrm->gCapFrameSetting.CapWin.width/CONFIG_VPP_ARGB_CAPTURE_RATIO; j++){
                    src0 = src_addr+j*bytes_per_pixel*CONFIG_VPP_ARGB_CAPTURE_RATIO;
                    src1 = src0+bytes_per_pixel;
                    src2 = src0+bytes_of_one_line;
                    src3 = src2+bytes_per_pixel;
                    for(k=0; k<bytes_per_pixel; k++){
                        *des = (*src0+*src1+*src2+*src3)/4;
                        src0++;
                        src1++;
                        src2++;
                        src3++;
                        des++;
                    }
                    copy_size += bytes_per_pixel;
                }
                src_addr += bytes_of_one_line*CONFIG_VPP_ARGB_CAPTURE_RATIO;
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width/CONFIG_VPP_ARGB_CAPTURE_RATIO;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height/CONFIG_VPP_ARGB_CAPTURE_RATIO;
            break;

        default:
            goto capture_frm_err_exit;
    }

    if(copy_size > 0){
        pCaptureVidFrm->gCapFrameInfo.uCapSize = copy_size;
        pCaptureVidFrm->gCapFrameInfo.ColorPrimary = pFrmDescr->m_colorprimaries;
        pCaptureVidFrm->gCapFrameInfo.ColorDepth = SRC_DEPTH_8BIT;
        pCaptureVidFrm->gCapFrameInfo.Video3DFormat = pFrmDescr->m_3D_source_fmt;
        pCaptureVidFrm->gCapFrameInfo.uSuccess = MV_VPP_OK;
    }else{
        pCaptureVidFrm->gCapFrameInfo.uCapSize = 0;
        pCaptureVidFrm->gCapFrameInfo.uSuccess = MV_VPP_EUNSUPPORT;
   }

    if(pCaptureVidFrm->appCbFn){
#if !(CONFIG_AMP_TEE_ENABLE)
        pCaptureVidFrm->appCbFn(&pCaptureVidFrm->gCapFrameInfo);
#else

        cb_params[0].value.a = pCaptureVidFrm->gCapFrameInfo.uSuccess;
        cb_params[0].value.b = pCaptureVidFrm->gCapFrameInfo.uCapSize;
        cb_params[1].value.a = pCaptureVidFrm->gCapFrameInfo.uiWidth
                               | (pCaptureVidFrm->gCapFrameInfo.uiHeight<<16);
        cb_params[1].value.b = pCaptureVidFrm->gCapFrameInfo.ColorFormat
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorPrimary<<8)
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorOrder<<16)
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorDepth<<24);
        cb_params[2].value.a = pCaptureVidFrm->gCapFrameInfo.Video3DFormat;
        cb_params[3].value.a = (int)pCaptureVidFrm->appCbFn;

        VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
#endif
    }

    return;
capture_frm_err_exit:
    pCaptureVidFrm->gCapFrameInfo.uCapSize = 0;
    pCaptureVidFrm->gCapFrameInfo.uSuccess = MV_VPP_EBADPARAM;

    if(pCaptureVidFrm->appCbFn){
#if !(CONFIG_AMP_TEE_ENABLE)
        pCaptureVidFrm->appCbFn(&pCaptureVidFrm->gCapFrameInfo);
#else
        cb_params[0].value.a = pCaptureVidFrm->gCapFrameInfo.uSuccess;
        cb_params[0].value.b = pCaptureVidFrm->gCapFrameInfo.uCapSize;
        cb_params[1].value.a = pCaptureVidFrm->gCapFrameInfo.uiWidth
                               | (pCaptureVidFrm->gCapFrameInfo.uiHeight<<16);
        cb_params[1].value.b = pCaptureVidFrm->gCapFrameInfo.ColorFormat
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorPrimary<<8)
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorOrder<<16)
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorDepth<<24);
        cb_params[2].value.a = pCaptureVidFrm->gCapFrameInfo.Video3DFormat;
        cb_params[3].value.a = (int)pCaptureVidFrm->appCbFn;

        VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
#endif
    }
}
#endif


/* dump the frame in YUV422 */
#ifdef CONFIG_CAPTURE_YUV422_MODE
static void  VPP_CaptureFrameNotifyUser(CAPTURE_VID_FRAME *pCaptureVidFrm,
                                                  VBUF_INFO *pFrmDescr, UINT8 *pIntBuf, INT planeID)
{
    void * src_addr;
    void * uvsrc_addr = NULL;
    UINT32 buf_size, final_size, copy_size, i, j, k;
    UINT32 bytes_of_one_line, bytes_of_one_line_uv;
    UINT32 bytes_of_one_cap_line, bytes_of_one_output_line;
    UINT8 bytes_per_pixel, bytes_per_pixel_uv, capture_mode;
    UINT8 tile_x_size, tile_y_size, tile_half_x_size;
    UINT8 is_progressive_frame;
    UINT8 * src0;
    UINT8 * src1;
    UINT8 * src2;
    UINT8 * src3;
    UINT8 * src4;
    UINT8 * src5;
    UINT8 * des;
    UINT8 * des_tile;
    UINT8 * inter_buf;
    UINT16 temp;

#if (CONFIG_AMP_TEE_ENABLE)
    UINT32 cb_cmdID = VPP_CB_NOTIFYCAPVIDFRAM;
    UINT32 cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                    TEE_PARAM_TYPE_VALUE_INPUT,
                    TEE_PARAM_TYPE_VALUE_INPUT,
                    TEE_PARAM_TYPE_VALUE_INPUT);
    TEE_Param cb_params[4];
#endif

    if((!pCaptureVidFrm->gCapFrameSetting.pFrmBuf)
        || (!pFrmDescr)
        || (pFrmDescr->m_bits_per_pixel != 8)
        || (pCaptureVidFrm->gCapFrameSetting.CapWin.width < 4)
        || (pCaptureVidFrm->gCapFrameSetting.CapWin.height < 4)){
        goto capture_frm_err_exit;
    }

    if((pFrmDescr->m_is_frame_seq)
        ||(IS_GFX_SRC_FORMAT(pFrmDescr->m_srcfmt))){
        is_progressive_frame = 1; //progressive
    }
    else{
        is_progressive_frame = 0; //interlaced
    }
    //capture window need be EVEN value
    if(pFrmDescr->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0){
        tile_x_size = 8;
        tile_y_size = 4;
        pCaptureVidFrm->gCapFrameSetting.CapWin.x /= tile_x_size;
        pCaptureVidFrm->gCapFrameSetting.CapWin.x *= tile_x_size;
        pCaptureVidFrm->gCapFrameSetting.CapWin.width /= tile_x_size;
        pCaptureVidFrm->gCapFrameSetting.CapWin.width *= tile_x_size;
        pCaptureVidFrm->gCapFrameSetting.CapWin.y /= (tile_y_size*2);
        pCaptureVidFrm->gCapFrameSetting.CapWin.y *= (tile_y_size*2);
        pCaptureVidFrm->gCapFrameSetting.CapWin.height /= (tile_y_size*2);
        pCaptureVidFrm->gCapFrameSetting.CapWin.height *= (tile_y_size*2);
    }
    else{
        pCaptureVidFrm->gCapFrameSetting.CapWin.x &= 0xFFFE;
        pCaptureVidFrm->gCapFrameSetting.CapWin.width &= 0xFFFE;
        pCaptureVidFrm->gCapFrameSetting.CapWin.y &= 0xFFFE;
        pCaptureVidFrm->gCapFrameSetting.CapWin.height &= 0xFFFE;
    }

    buf_size = pCaptureVidFrm->gCapFrameSetting.CapWin.width*pCaptureVidFrm->gCapFrameSetting.CapWin.height;

    if(buf_size == 0){
        goto capture_frm_err_exit;
    }

    switch(pFrmDescr->m_srcfmt)
    {
        case SRCFMT_YUV420SP_TILED_MODE0:
            buf_size = buf_size*3/2;
            bytes_per_pixel = 1; //Y
            bytes_per_pixel_uv = 2;
            if((pFrmDescr->m_active_width <= 720)
                && (pFrmDescr->m_active_height <= 576)){
                capture_mode = VPP_CAPTURE_YUV420SP_TILE_SD_MODE;
                final_size = buf_size/3*4; // Y and 2*UV
            }
            else{
                capture_mode = VPP_CAPTURE_YUV420SP_TILE_HD_MODE;
                final_size = buf_size/3; // Y/4 and UV/2
            }
            pCaptureVidFrm->gCapFrameInfo.ColorFormat = SRCFMT_YUV422;
            pCaptureVidFrm->gCapFrameInfo.ColorOrder = ORDER_UYVY;
            break;

        case SRCFMT_YUV420SP:
            buf_size = buf_size*3/2;
            bytes_per_pixel = 1; //Y
            bytes_per_pixel_uv = 2;
            if((pFrmDescr->m_active_width <= 720)
                && (pFrmDescr->m_active_height <= 576)){
                capture_mode = VPP_CAPTURE_YUV420SP_SD_MODE;
                final_size = buf_size/3*4; // Y and 2*UV
            }
            else{
                capture_mode = VPP_CAPTURE_YUV420SP_HD_MODE;
                final_size = buf_size/3; // Y/4 and UV/2
            }
            pCaptureVidFrm->gCapFrameInfo.ColorFormat = SRCFMT_YUV422;
            pCaptureVidFrm->gCapFrameInfo.ColorOrder = ORDER_UYVY;
            break;

        case SRCFMT_YUV422:
            buf_size *= 2;
            bytes_per_pixel = 2;
            if((pFrmDescr->m_active_width <= 720)
                && (pFrmDescr->m_active_height <= 576)){
                capture_mode = VPP_CAPTURE_YUV422_SD_MODE;
                final_size = buf_size;
                pCaptureVidFrm->gCapFrameInfo.ColorFormat = SRCFMT_YUV422;
                if((pFrmDescr->m_order == ORDER_UYVY)
                    ||(pFrmDescr->m_order == ORDER_VYUY)){
                    pCaptureVidFrm->gCapFrameInfo.ColorOrder = pFrmDescr->m_order;
                }
                else if(pFrmDescr->m_order == ORDER_YUYV){
                    pCaptureVidFrm->gCapFrameInfo.ColorOrder = ORDER_YVYU;
                }
                else if(pFrmDescr->m_order == ORDER_YVYU){
                    pCaptureVidFrm->gCapFrameInfo.ColorOrder = ORDER_YUYV;
                }
            }
            else{
                capture_mode = VPP_CAPTURE_YUV422_HD_MODE;
                final_size = buf_size/4;
                pCaptureVidFrm->gCapFrameInfo.ColorFormat = SRCFMT_YUV422;
                pCaptureVidFrm->gCapFrameInfo.ColorOrder = ORDER_UYVY;
            }
            break;

        case SRCFMT_ARGB32:
        case SRCFMT_ARGB32_PM:
        case SRCFMT_XRGB32:
        case SRCFMT_IPT444:
            buf_size *= 4;
            bytes_per_pixel = 4;
            capture_mode = VPP_CAPTURE_ARGB_MODE;
            final_size = buf_size/4;
            pCaptureVidFrm->gCapFrameInfo.ColorFormat = pFrmDescr->m_srcfmt;
            pCaptureVidFrm->gCapFrameInfo.ColorOrder = pFrmDescr->m_order;
            break;

        default:
            goto capture_frm_err_exit;
    }

    if((final_size > pCaptureVidFrm->gCapFrameSetting.FrmBufSize)
        || (buf_size > CAPTURE_BUFFER_SIZE)
        ||(TEE_CheckMemoryOutputRights((void *)pCaptureVidFrm->gCapFrameSetting.pFrmBuf,
            final_size, TEE_NONSECURE, TEE_MT_COPY)!=TEE_SUCCESS)){
        goto capture_frm_err_exit;
    }

    inter_buf = pIntBuf;
    if(!inter_buf){
        goto capture_frm_err_exit;
    }

    /* Check secure playback status VSSDK-3756 */
    if(!VPP_ENV_SA_isSecurePlayback(0, planeID)) {
        //copy video frame to internal buffer
        if(pFrmDescr->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0){
            bytes_of_one_line = pFrmDescr->m_buf_stride*tile_y_size;
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width*bytes_per_pixel
                                    *tile_y_size;
            src_addr = pFrmDescr->m_pbuf_start
                       + pCaptureVidFrm->gCapFrameSetting.CapWin.y/tile_y_size*bytes_of_one_line;
            src_addr += (UINT32)(pCaptureVidFrm->gCapFrameSetting.CapWin.x*bytes_per_pixel
                                *tile_y_size);
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/tile_y_size; i++)
            {
                memcpy((void *)inter_buf, src_addr, bytes_of_one_cap_line);
                inter_buf += bytes_of_one_cap_line;
                //only progressive
                src_addr += bytes_of_one_line;
            }
        }
        else{
            bytes_of_one_line = pFrmDescr->m_buf_stride;
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width*bytes_per_pixel;
            if(is_progressive_frame){
                //progressive
                src_addr = pFrmDescr->m_pbuf_start
                       + pCaptureVidFrm->gCapFrameSetting.CapWin.y*bytes_of_one_line;
            }
            else{
                src_addr = pFrmDescr->m_pbuf_start
                       + pCaptureVidFrm->gCapFrameSetting.CapWin.y*bytes_of_one_line*2;
            }
            src_addr += (UINT32)(pCaptureVidFrm->gCapFrameSetting.CapWin.x*bytes_per_pixel);
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height; i++)
            {
                memcpy((void *)inter_buf, src_addr, bytes_of_one_cap_line);
                inter_buf += bytes_of_one_cap_line;
                if(is_progressive_frame)//progressive
                    src_addr += bytes_of_one_line;
                else
                    src_addr += bytes_of_one_line*2;
            }
        }

        //copy UV for YUV420SP
        if(pFrmDescr->m_srcfmt == SRCFMT_YUV420SP){
            uvsrc_addr = inter_buf; //save start of UV
            bytes_of_one_line = pFrmDescr->m_buf_stride_UV;
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
            src_addr = (void *)pFrmDescr->m_buf_pbuf_start_UV;
            src_addr += pCaptureVidFrm->gCapFrameSetting.CapWin.y/2*bytes_of_one_line;
            src_addr += pCaptureVidFrm->gCapFrameSetting.CapWin.x;
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/2; i++){
                memcpy((void *)inter_buf, src_addr, bytes_of_one_cap_line);
                inter_buf += bytes_of_one_cap_line;
                if(is_progressive_frame)//progressive
                    src_addr += bytes_of_one_line;
                else
                    src_addr += bytes_of_one_line*2;
            }
        }
        if(pFrmDescr->m_srcfmt == SRCFMT_YUV420SP_TILED_MODE0){
            uvsrc_addr = inter_buf; //save start of UV
            bytes_of_one_line = pFrmDescr->m_buf_stride_UV*tile_y_size;
            bytes_of_one_cap_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width*tile_y_size;
            src_addr = (void *)pFrmDescr->m_buf_pbuf_start_UV;
            src_addr += pCaptureVidFrm->gCapFrameSetting.CapWin.y/2/tile_y_size*bytes_of_one_line;
            src_addr += pCaptureVidFrm->gCapFrameSetting.CapWin.x*tile_y_size;
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/2/tile_y_size; i++){
                memcpy((void *)inter_buf, src_addr, bytes_of_one_cap_line);
                inter_buf += bytes_of_one_cap_line;
                src_addr += bytes_of_one_line;
            }
        }
    }
    copy_size = 0;
    //goto start of internal buffer
    src_addr = pIntBuf;
    if(!src_addr){
        goto capture_frm_err_exit;
    }

    bytes_of_one_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width*bytes_per_pixel;
    bytes_of_one_line_uv = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
    des = (UINT8 *)pCaptureVidFrm->gCapFrameSetting.pFrmBuf;

    switch(capture_mode){
        case VPP_CAPTURE_YUV420SP_TILE_SD_MODE:
            bytes_of_one_output_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width*2;//YUV422
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/tile_y_size; i++){
                for(j=0; j<pCaptureVidFrm->gCapFrameSetting.CapWin.width/tile_x_size; j++){
                    for(k=0; k<tile_y_size*tile_x_size; k++){
                        des_tile = des+k/tile_x_size*bytes_of_one_output_line
                                    +(k%tile_x_size)*2;
                        src0 = uvsrc_addr+k/tile_x_size/2*tile_x_size
                                    +(k%tile_x_size)/2*bytes_per_pixel_uv; //UV
                        if(k%2 == 1){
                            src0++; //V
                        }

                        if((k%tile_half_x_size) >= (tile_half_x_size-2)){//right margin
                            src1 = src0-bytes_per_pixel_uv;
                            *des_tile = (*src0+*src1)/2;  //UV
                        }
                        else{
                            src1 = src0+bytes_per_pixel_uv;
                            *des_tile = (*src0+*src1)/2;  //UV
                        }

                        des_tile++;

                        src0 = src_addr+k/tile_x_size*tile_x_size*bytes_per_pixel
                                   +(k%tile_x_size)*bytes_per_pixel;
                        if(k%tile_x_size == tile_x_size-1){
                            src1 = src0-bytes_per_pixel;
                        }
                        else{
                            src1 = src0+bytes_per_pixel;
                        }
                        if(k/tile_x_size == tile_y_size-1){
                            src2 = src0-tile_x_size;
                        }
                        else{
                            src2 = src0+tile_x_size;
                        }
                        temp = (UINT16)*src1+(UINT16)*src2;
                        temp += (UINT16)*src0<<1;
                        *des_tile = (UINT8)(temp>>2); //Y
                        copy_size += 2;
                    }
                    src_addr += tile_x_size*tile_y_size;
                    uvsrc_addr += tile_x_size*tile_y_size;
                    des += tile_x_size*2;
                }
                if(i%2 == 0){
                    uvsrc_addr -= pCaptureVidFrm->gCapFrameSetting.CapWin.width*tile_y_size;
                    uvsrc_addr += tile_x_size*tile_y_size/2;
                }
                else{
                    uvsrc_addr -= tile_x_size*tile_y_size/2;
                }
                des += bytes_of_one_output_line*(tile_y_size-1);
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height;
            break;

        case VPP_CAPTURE_YUV420SP_TILE_HD_MODE:
            bytes_of_one_output_line = pCaptureVidFrm->gCapFrameSetting.CapWin.width;//YUV422
            tile_half_x_size = tile_x_size/2;
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/tile_y_size; i++){
                for(j=0; j<pCaptureVidFrm->gCapFrameSetting.CapWin.width/tile_x_size; j++){
                    for(k=0; k<tile_y_size*tile_x_size/4; k++){
                        des_tile = des+k/tile_half_x_size*bytes_of_one_output_line
                                   +(k%tile_half_x_size)*2;
                        src0 = src_addr+k/tile_half_x_size*tile_x_size*bytes_per_pixel*2
                                   +(k%tile_half_x_size)*bytes_per_pixel*2;
                        src1 = src0+bytes_per_pixel;
                        src2 = src0+tile_x_size*bytes_per_pixel;
                        src3 = src2+bytes_per_pixel;
                        src4 = uvsrc_addr+k*bytes_per_pixel_uv; //UV
                        if(k%2 == 1){
                            src4++;
                        }
                        if((k%tile_half_x_size) == (tile_half_x_size-1)){//right margin
                            src5 = src4-bytes_per_pixel_uv;
                            *des_tile = (*src4+*src5)/2;  //UV
                        }
                        else{
                            src5 = src4+bytes_per_pixel_uv;
                            *des_tile = (*src4+*src5)/2;  //UV
                        }

                        des_tile++;
                        *des_tile = (*src0+*src1+*src2+*src3)/4;   //Y
                        copy_size += 2;
                    }
                    src_addr += tile_x_size*tile_y_size;
                    uvsrc_addr += tile_x_size*tile_y_size;
                    des += tile_x_size;
                }
                if(i%2 == 0){
                    uvsrc_addr -= pCaptureVidFrm->gCapFrameSetting.CapWin.width*tile_y_size;
                    uvsrc_addr += tile_x_size*tile_y_size/2;
                }
                else{
                    uvsrc_addr -= tile_x_size*tile_y_size/2;
                }
                des += bytes_of_one_output_line;
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width/2;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height/2;
            break;

        case VPP_CAPTURE_YUV420SP_SD_MODE:
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height; i++){
                for(j=0; j<pCaptureVidFrm->gCapFrameSetting.CapWin.width; j++){
                    src0 = uvsrc_addr+j/2*bytes_per_pixel_uv; //UV
                    if(j%2 == 1){
                        src0++;
                    }
                    if(j == pCaptureVidFrm->gCapFrameSetting.CapWin.width){//right margin
                        src1 = src0-bytes_per_pixel_uv;
                        *des = (*src0+*src1)/2;  //UV
                    }
                    else{
                        src1 = src0+bytes_per_pixel_uv;
                        *des = (*src0+*src1)/2;  //UV
                    }
                    des++;

                    src0 = src_addr+j*bytes_per_pixel;
                    if(i==0){
                        src0 += bytes_of_one_line;
                    }
                    if(i==pCaptureVidFrm->gCapFrameSetting.CapWin.height){
                        src0 -= bytes_of_one_line;
                    }
                    if(j==0){
                        src0 += bytes_per_pixel;
                    }
                    if(j==pCaptureVidFrm->gCapFrameSetting.CapWin.width){
                        src0 -= bytes_per_pixel;
                    }
                    src1 = src0+bytes_per_pixel;
                    src2 = src0-bytes_per_pixel;
                    src3 = src0+bytes_of_one_line;
                    src4 = src0-bytes_of_one_line;
                    temp = ((UINT16)*src1+(UINT16)*src2+(UINT16)*src3+(UINT16)*src4)<<4;
                    temp += ((UINT16)*src0<<7) + ((UINT16)*src0<<6);
                    *des = (UINT8)(temp>>8); //Y
                    des++;
                    copy_size += 2;
                }
                src_addr += bytes_of_one_line;
                if(i%2){
                    uvsrc_addr += bytes_of_one_line_uv;
                }
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height;
            break;

        case VPP_CAPTURE_YUV420SP_HD_MODE:
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/2; i++){
                for(j=0; j<pCaptureVidFrm->gCapFrameSetting.CapWin.width/2; j++){
                    src0 = src_addr+j*bytes_per_pixel*2;
                    src1 = src0+bytes_per_pixel;
                    src2 = src0+bytes_of_one_line;
                    src3 = src2+bytes_per_pixel;
                    src4 = uvsrc_addr+j*bytes_per_pixel_uv; //UV
                    if(j%2 == 1){
                        src4++;
                    }
                    if(j == pCaptureVidFrm->gCapFrameSetting.CapWin.width/2){//right margin
                        src5 = src4-bytes_per_pixel_uv;
                        *des = (*src4+*src5)/2;  //U
                    }
                    else{
                        src5 = src4+bytes_per_pixel_uv;
                        *des = (*src4+*src5)/2;  //U
                    }

                    des++;
                    *des = (*src0+*src1+*src2+*src3)/4;   //Y
                    des++;
                    copy_size += 2;
                }
                src_addr += bytes_of_one_line*2;
                uvsrc_addr += bytes_of_one_line_uv;
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width/2;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height/2;
            break;

        case VPP_CAPTURE_YUV422_SD_MODE:
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height; i++){
                for(j=0; j<pCaptureVidFrm->gCapFrameSetting.CapWin.width; j++){
                    if((pFrmDescr->m_order == ORDER_UYVY)
                        ||(pFrmDescr->m_order == ORDER_VYUY)){
                        src0 = src_addr+j*bytes_per_pixel;
                        if(i==0){
                            src0 += bytes_of_one_line;
                        }
                        if(i==pCaptureVidFrm->gCapFrameSetting.CapWin.height-1){
                            src0 -= bytes_of_one_line;
                        }
                        src3 = src0+bytes_of_one_line;
                        src4 = src0-bytes_of_one_line;
                        temp = ((UINT16)*src3+(UINT16)*src4)<<5;
                        temp += (((UINT16)*src0)<<7) + (((UINT16)*src0)<<6);
                        *des = (UINT8)(temp>>8); //UV
                        des++;
                        src0++;
                        if(j==0){
                            src0 += bytes_per_pixel;
                        }
                        if(j==pCaptureVidFrm->gCapFrameSetting.CapWin.width-1){
                            src0 -= bytes_per_pixel;
                        }
                        src1 = src0+bytes_per_pixel;
                        src2 = src0-bytes_per_pixel;
                        src3 = src0+bytes_of_one_line;
                        src4 = src0-bytes_of_one_line;
                        temp = ((UINT16)*src1+(UINT16)*src2+(UINT16)*src3+(UINT16)*src4)<<4;
                        temp += (((UINT16)*src0)<<7) + (((UINT16)*src0)<<6);
                        *des = (UINT8)(temp>>8); //Y
                        des++;
                    }
                    else if((pFrmDescr->m_order == ORDER_YUYV)
                        ||(pFrmDescr->m_order == ORDER_YVYU)){
                        src0 = src_addr+j*bytes_per_pixel;
                        if(i==0){
                            src0 += bytes_of_one_line;
                        }
                        if(i==pCaptureVidFrm->gCapFrameSetting.CapWin.height-1){
                            src0 -= bytes_of_one_line;
                        }
                        if(j==0){
                            src0 += bytes_per_pixel;
                        }
                        if(j==pCaptureVidFrm->gCapFrameSetting.CapWin.width-1){
                            src0 -= bytes_per_pixel;
                        }
                        src1 = src0+bytes_per_pixel;
                        src2 = src0-bytes_per_pixel;
                        src3 = src0+bytes_of_one_line;
                        src4 = src0-bytes_of_one_line;
                        temp = ((UINT16)*src1+(UINT16)*src2+(UINT16)*src3+(UINT16)*src4)<<4;
                        temp += ((UINT16)*src0<<7) + ((UINT16)*src0<<6);
                        *des = (UINT8)(temp>>8); //Y
                        des++;
                        src0++;
                        src3++;
                        src4++;
                        temp = ((UINT16)*src3+(UINT16)*src4)<<5;
                        temp += ((UINT16)*src0<<7) + ((UINT16)*src0<<6);
                        *des = (UINT8)(temp>>8); //UV
                        des++;
                    }
                    copy_size += 2;
                }
                src_addr += bytes_of_one_line;
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height;
            break;

        case VPP_CAPTURE_YUV422_HD_MODE:
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/2; i++){
                for(j=0; j<pCaptureVidFrm->gCapFrameSetting.CapWin.width/2; j++){
                    src0 = src_addr+j*bytes_per_pixel*2;
                    src1 = src0+bytes_per_pixel;
                    src2 = src0+bytes_of_one_line;
                    src3 = src2+bytes_per_pixel;
                    if(pFrmDescr->m_order == ORDER_UYVY){
                        if(j%2 == 0){
                            *des = (*src0+*src2)/2;  //U
                        }
                        else{
                            *des = (*src1+*src3)/2;  //V
                        }
                        des++;
                        src0++;
                        src1++;
                        src2++;
                        src3++;
                        *des = (*src0+*src1+*src2+*src3)/4;  //Y
                        des++;
                    }
                    else if(pFrmDescr->m_order == ORDER_VYUY){
                        if(j%2 == 0){
                            *des = (*src1+*src3)/2;  //U
                        }
                        else{
                            *des = (*src0+*src2)/2;  //V
                        }
                        des++;
                        src0++;
                        src1++;
                        src2++;
                        src3++;
                        *des = (*src0+*src1+*src2+*src3)/4;  //Y
                        des++;
                    }
                    else if(pFrmDescr->m_order == ORDER_YUYV){
                        if(j%2 == 0){
                            src0++;
                            src2++;
                            *des = (*src0+*src2)/2;  //U
                            src0--;
                            src2--;
                        }
                        else{
                            src1++;
                            src3++;
                            *des = (*src1+*src3)/2;  //V
                            src1--;
                            src3--;
                        }
                        des++;
                        *des = (*src0+*src1+*src2+*src3)/4;  //Y
                        des++;
                    }
                    else if(pFrmDescr->m_order == ORDER_YVYU){
                        if(j%2 == 0){
                            src1++;
                            src3++;
                            *des = (*src1+*src3)/2;  //U
                            src3--;
                            src3--;
                        }
                        else{
                            src0++;
                            src2++;
                            *des = (*src0+*src2)/2;  //V
                            src0--;
                            src2--;
                        }
                        des++;
                        *des = (*src0+*src1+*src2+*src3)/4;  //Y
                        des++;
                    }
                    copy_size += 2;
                }
                src_addr += bytes_of_one_line*2;
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width/2;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height/2;
            break;

        case VPP_CAPTURE_ARGB_MODE:
            for(i=0; i<pCaptureVidFrm->gCapFrameSetting.CapWin.height/CONFIG_VPP_ARGB_CAPTURE_RATIO; i++){
                for(j=0; j<pCaptureVidFrm->gCapFrameSetting.CapWin.width/CONFIG_VPP_ARGB_CAPTURE_RATIO; j++){
                    src0 = src_addr+j*bytes_per_pixel*CONFIG_VPP_ARGB_CAPTURE_RATIO;
                    src1 = src0+bytes_per_pixel;
                    src2 = src0+bytes_of_one_line;
                    src3 = src2+bytes_per_pixel;
                    for(k=0; k<bytes_per_pixel; k++){
                        *des = (*src0+*src1+*src2+*src3)/4;
                        src0++;
                        src1++;
                        src2++;
                        src3++;
                        des++;
                    }
                    copy_size += bytes_per_pixel;
                }
                src_addr += bytes_of_one_line*CONFIG_VPP_ARGB_CAPTURE_RATIO;
            }
            pCaptureVidFrm->gCapFrameInfo.uiWidth = pCaptureVidFrm->gCapFrameSetting.CapWin.width/CONFIG_VPP_ARGB_CAPTURE_RATIO;
            pCaptureVidFrm->gCapFrameInfo.uiHeight = pCaptureVidFrm->gCapFrameSetting.CapWin.height/CONFIG_VPP_ARGB_CAPTURE_RATIO;
            break;

        default:
            goto capture_frm_err_exit;
    }

    if(copy_size > 0){
        pCaptureVidFrm->gCapFrameInfo.uCapSize = copy_size;
        pCaptureVidFrm->gCapFrameInfo.ColorPrimary = pFrmDescr->m_colorprimaries;
        pCaptureVidFrm->gCapFrameInfo.ColorDepth = SRC_DEPTH_8BIT;
        pCaptureVidFrm->gCapFrameInfo.Video3DFormat = pFrmDescr->m_3D_source_fmt;
        pCaptureVidFrm->gCapFrameInfo.uSuccess = MV_VPP_OK;
    }else{
        pCaptureVidFrm->gCapFrameInfo.uCapSize = 0;
        pCaptureVidFrm->gCapFrameInfo.uSuccess = MV_VPP_EUNSUPPORT;
   }

    if(pCaptureVidFrm->appCbFn){
#if !(CONFIG_AMP_TEE_ENABLE)
        pCaptureVidFrm->appCbFn(&pCaptureVidFrm->gCapFrameInfo);
#else

        cb_params[0].value.a = pCaptureVidFrm->gCapFrameInfo.uSuccess;
        cb_params[0].value.b = pCaptureVidFrm->gCapFrameInfo.uCapSize;
        cb_params[1].value.a = pCaptureVidFrm->gCapFrameInfo.uiWidth
                               | (pCaptureVidFrm->gCapFrameInfo.uiHeight<<16);
        cb_params[1].value.b = pCaptureVidFrm->gCapFrameInfo.ColorFormat
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorPrimary<<8)
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorOrder<<16)
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorDepth<<24);
        cb_params[2].value.a = pCaptureVidFrm->gCapFrameInfo.Video3DFormat;
        cb_params[3].value.a = (int)pCaptureVidFrm->appCbFn;

        VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
#endif
    }

    return;
capture_frm_err_exit:
    pCaptureVidFrm->gCapFrameInfo.uCapSize = 0;
    pCaptureVidFrm->gCapFrameInfo.uSuccess = MV_VPP_EBADPARAM;

    if(pCaptureVidFrm->appCbFn){
#if !(CONFIG_AMP_TEE_ENABLE)
        pCaptureVidFrm->appCbFn(&pCaptureVidFrm->gCapFrameInfo);
#else
        cb_params[0].value.a = pCaptureVidFrm->gCapFrameInfo.uSuccess;
        cb_params[0].value.b = pCaptureVidFrm->gCapFrameInfo.uCapSize;
        cb_params[1].value.a = pCaptureVidFrm->gCapFrameInfo.uiWidth
                               | (pCaptureVidFrm->gCapFrameInfo.uiHeight<<16);
        cb_params[1].value.b = pCaptureVidFrm->gCapFrameInfo.ColorFormat
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorPrimary<<8)
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorOrder<<16)
                               | ((UINT32)pCaptureVidFrm->gCapFrameInfo.ColorDepth<<24);
        cb_params[2].value.a = pCaptureVidFrm->gCapFrameInfo.Video3DFormat;
        cb_params[3].value.a = (int)pCaptureVidFrm->appCbFn;

        VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
#endif
    }
}
#endif

/***************************************************************************************
 * FUNCTION: capture frame task
 * PARAMS:   None
 *
 * RETURN:  None
 ***************************************************************************************/
void VPP_CaptureFrameTask(VPP_OBJ *vpp_obj)
{
    void *pFrmDescr;
    CAPTURE_VID_FRAME *pCaptureVidFrm;
    PLANE *plane;
    UINT8 i;

    MV_OSAL_Sem_Wait(vpp_obj->vpp_FrameCapture_sem);

    for (i=0; i<MAX_NUM_PLANES; i++){
        plane = &vpp_obj->plane[i];
        pCaptureVidFrm = &plane->CaptureVidFrm;
        if(pCaptureVidFrm->gCapStartFlag == TRUE){
            break;
        }
    }
    if(i == MAX_NUM_PLANES){
        return;
    }

    if (plane->status == STATUS_DISP_VIDEO){
        pFrmDescr = plane->curr_frame;
    }
    else if (plane->status == STATUS_DISP_LOGO){
        pFrmDescr = plane->curr_still_picture;
    }
    else if (plane->status == STATUS_DISP_PATGEN){
        pFrmDescr = plane->dummy_frame;
    }
    else{
        pFrmDescr = NULL;
    }

    if(pFrmDescr != NULL){
        if(vpp_obj->vpp_FrameBuff_addr == NULL){
            HRESULT result = VPP_SHM32_MALLOC(AMP_SHM_DYNAMIC, CAPTURE_BUFFER_SIZE,
                                            &(vpp_obj->vpp_FrameBuff_start));
            if (result != S_OK){
                pCaptureVidFrm->gCapStartFlag = FALSE;
                return;
            }
            vpp_obj->vpp_FrameBuff_addr = (UINT8 *)VPP_SHM_GetPhysicalAddress(vpp_obj->vpp_FrameBuff_start, 0);
        }

        VPP_CaptureFrameNotifyUser(pCaptureVidFrm, (VBUF_INFO *)pFrmDescr, vpp_obj->vpp_FrameBuff_addr, i);
    }
    pCaptureVidFrm->gCapStartFlag = FALSE;
}

/***************************************************************************************
 * FUNCTION: Creat Frame Capture task
 * PARAMS:   None
 *
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to Creat Frame Capture task
 ***************************************************************************************/
HRESULT VPP_CreateFrameCaptureTask(VPP_OBJ *vpp_obj)
{
    HRESULT retVal = S_OK;

    TEE_Result result;
    uint32_t cb_cmdID = VPP_CB_CREATE_FRAMECAPTURETASK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = 0xdeadbeef;

    result = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    if(result != S_OK){
        return retVal;
    }

    return result;
}

/***************************************************************************************
 * FUNCTION: Destory Frame Capture task
 * PARAMS:   None
 *
 * RETURN:  MV_VPP_OK - succeed
 *          others - failed to Destory Frame Capture task
 ***************************************************************************************/
HRESULT VPP_DestroyFrameCaptureTask(VPP_OBJ *vpp_obj)
{
    HRESULT retVal = S_OK;

    uint32_t cb_cmdID = VPP_CB_DELETE_FRAMECAPTURETASK;
    uint32_t cb_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INOUT,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE,
            TEE_PARAM_TYPE_NONE);

    TEE_Param cb_params[4];
    cb_params[0].value.a = 0xdeadbeef;

    retVal = VPP_OSAL_TEE_Callback(cb_cmdID, cb_paramTypes, cb_params, 0, NULL);
    if(vpp_obj->vpp_FrameBuff_start) {
        VPP_SHM_Free(vpp_obj->vpp_FrameBuff_start);
    }
    vpp_obj->vpp_FrameBuff_addr = NULL;

    return retVal;
}

#endif
