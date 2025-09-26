/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

////////////////////////////////////////////////////////////////////////////////
//! \file vpp_ta_vpp_shm_cmd.c
//!
//! \brief Implementation of VPP SHM commands for TrustZone.
//!
//! Purpose: This is used for controlling access to VPP SHM command API in TZ.
//!
//!     Version    Date                     Author
//!     V 1.00,    Oct 21 2022,            Shanmugam Ramachanran
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#include "tee_ta_vpp.h"
#include "tee_ta_vpp_be.h"
#include "vpp_osal.h"

VBUF_INFO* VPP_FrameInfoPool_AllocFrameInfo(int planeId, VBUF_INFO *pVbufInfo) {

    VPP_FrameInfo *pFrameInfo = NULL;
    if (!frmq_pop(&g_vpp->VPP_fl.frame_info_pool, (void**)&pFrameInfo)) {
        //No more free frame_info, so allocate one
        g_vpp->VPP_fl.frame_info_pool_count++;
        VPP_SHM32_MALLOC(VPP_SHM_SECURE, sizeof(VPP_FrameInfo), (VPP_SHM_HANDLE*)&pFrameInfo);
    } else {
        //free frame_info poped, then remove it from free pool
        frmq_pop_commit(&g_vpp->VPP_fl.frame_info_pool);
    }

    if (pFrameInfo) {
        //copy pointer and content into the free fame_info
        pFrameInfo->frame_info_input = pVbufInfo;
        memcpy(&pFrameInfo->frame_info_local, pFrameInfo->frame_info_input, sizeof(VBUF_INFO));
        pVbufInfo = &pFrameInfo->frame_info_local;
    } else {
        VPPAPI_DBG_INFO("%s:%d: pVbufInfo:%p --- Local VBUF_INFO alloation failed!!!!!\n",
                        __func__, __LINE__, planeId, pVbufInfo);
        pVbufInfo = NULL;
    }

    return pVbufInfo;
}

VBUF_INFO* VPP_FrameInfoPool_FreeFrameInfo(VBUF_INFO *pVbufInfo) {
    //Note frame_info_local is the first member of the struct VPP_FrameInfo
    VPP_FrameInfo *pFrameInfo;

    pFrameInfo = (VPP_FrameInfo*) pVbufInfo;
    if (pFrameInfo) {
        pVbufInfo = pFrameInfo->frame_info_input;

        //return VPP_FrameInfo back to free pool
        frmq_push(&g_vpp->VPP_fl.frame_info_pool, pFrameInfo);
    } else {
        pVbufInfo = NULL;
    }

    return pVbufInfo;
}

int MV_VPPOBJ_BindSHM(TEE_Param params[4])
{
    VBUF_INFO *pinfo;
    int shmId, planeId, resID;
    UINT8 *shmAddr;
    VPP_SHM_BUFFER_DESC bufDesc;

    shmId = params[0].value.a;
    TEE_Result res = TEE_SUCCESS;
    unsigned int Size;
    switch(shmId) {
    case DISPLAY_FRAME:
        shmAddr = (UINT8 *)params[1].memref.buffer;
        pinfo = (VBUF_INFO *)shmAddr;
        planeId = params[3].value.a;
        pinfo = VPP_FrameInfoPool_AllocFrameInfo(planeId, pinfo);
        if (!pinfo) {
            return TEE_ERROR_OUT_OF_MEMORY;
        }
        pinfo->m_dv_el_descriptor=(ARCH_PTR_TYPE)NULL;
        MV_VPPOBJ_DisplayFrame(0, planeId, pinfo);
        break;

    case SET_STILL_PICTURE:
        shmAddr = (UINT8 *)params[1].memref.buffer;
        pinfo = (VBUF_INFO *)shmAddr;
        planeId = params[3].value.a;
        pinfo = VPP_FrameInfoPool_AllocFrameInfo(planeId, pinfo);
        if (!pinfo) {
            return TEE_ERROR_OUT_OF_MEMORY;
        }
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        if ((pinfo->m_srcfmt == SRCFMT_LUT8)
            && pinfo->m_clut_ptr
            && ((planeId >= PLANE_GFX0) && (planeId <= PLANE_PG)))
        {
            pinfo->m_clut_ptr = params[2].memref.buffer;
        }
#endif
        MV_VPPOBJ_SetStillPicture(0, planeId, pinfo, NULL);
        break;

    case VPP_GET_RES_DESCRIPTION:
        {
            shmAddr = (UINT8 *)params[1].memref.buffer;
            Size = params[1].memref.size;
            resID = params[2].value.a;

            if(!shmAddr || Size != sizeof(VPP_RESOLUTION_DESCRIPTION))
            {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res == TEE_SUCCESS){
                params[2].value.a = MV_VPPOBJ_GetResolutionDescription(0, resID,
                                    (VPP_RESOLUTION_DESCRIPTION*)shmAddr);
            }
            break;
        }

    case GET_HDMI_SINKCAPS:
        {
            shmAddr = (UINT8 *)params[1].memref.buffer;
            Size = params[1].memref.size;
            if(!shmAddr) {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_OUTPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res == TEE_SUCCESS)
                params[2].value.a = MV_VPPOBJ_GetHDMISinkCaps(0, (VPP_HDMI_SINK_CAPS *)(shmAddr));
        }
    break;

    case GET_HDMI_SINKFEATUREMAP:
        {
            /* full sink caps structure need not be exposed for ampless case,
               for current use case, setting bitmask for specific feature will suffice
             */
            shmAddr = (UINT8 *)params[1].memref.buffer;
            Size = params[1].memref.size;
            if(!shmAddr) {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_OUTPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res == TEE_SUCCESS)
                params[2].value.a = MV_VPPOBJ_GetHDMISinkFeatureMap(0, (VPP_HDMI_SINKCAP_BITMASK *)(shmAddr));
        }
    break;

    case VPP_SETHDMIPHY:
       {
            shmAddr = (UINT8 *)params[1].memref.buffer;
            Size = params[1].memref.size;

            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res == TEE_SUCCESS) {
                params[2].value.a = MV_VPPOBJ_SetHdmiPhy(0, (unsigned int *)(shmAddr), Size);
            }
       }
       break;

    case VPP_SETHDMICONFIG:
       {
            shmAddr = (UINT8 *)params[1].memref.buffer;
            Size = params[1].memref.size;
            if(!shmAddr || Size != sizeof(HDMI_CONFIG)) {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res == TEE_SUCCESS) {
                params[2].value.a = MV_VPPOBJ_SetHdmiConfig(0, (unsigned int *)(shmAddr), Size);
            }
       }
       break;

    case VPP_OBJCONFIG:
       {
           shmAddr = (UINT8 *)params[1].memref.buffer;
           Size = params[1].memref.size;
           INT8 *config;
           int plane_size;
           int vout_size;
#ifdef CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
           VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
           res = VPP_ValidateInput(&bufDesc, 0);
           if(res != TEE_SUCCESS)
               break;
#endif //CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
           config = (INT8 *)shmAddr;
           plane_size = (sizeof(INT32)*(MAX_NUM_PLANES));
           vout_size = sizeof(INT32)* (MAX_NUM_VOUTS);
           params[2].value.a = MV_VPPOBJ_Config(0, (const int *) &config[0], (const int *) &config[(plane_size)],
                           (const int *) &config[(plane_size*2)], (const int *) &config[(plane_size*3)],
                           (const int *) &config[(plane_size*3) + vout_size]);
       }
    break;

    case VPP_SETFORMAT:
    {
        int cpcbId;
        shmAddr = (UINT8 *)params[1].memref.buffer;
        Size = params[1].memref.size;

        VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
        res = VPP_ValidateInput(&bufDesc, 0);
        if(res != TEE_SUCCESS)
            break;
        cpcbId = *((int*)shmAddr);
        params[2].value.a = MV_VPPOBJ_SetFormat(0, cpcbId, (void *)(shmAddr + sizeof(int)));
    }
    break;

    case VPP_MIPI_CONFIG:
    {
        VPP_MIPI_LOAD_CONFIG *pConfigParams;

        shmAddr = (UINT8 *)params[1].memref.buffer;
        Size = params[1].memref.size;

        VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
        res = VPP_ValidateInput(&bufDesc, 0);
        if(res != TEE_SUCCESS)
            break;

        pConfigParams = (VPP_MIPI_LOAD_CONFIG*) shmAddr;
        params[2].value.a = MV_VPPOBJ_LoadMipiConfig(0, pConfigParams);
    }
    break;

    case VPP_GET_DISP_VOUTPARAMS:
    {
        shmAddr = (UINT8 *)params[1].memref.buffer;
        Size = params[1].memref.size;

        VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_OUTPUT);
        res = VPP_ValidateInput(&bufDesc, 0);
        if(res != TEE_SUCCESS)
            break;

        params[2].value.a = MV_VPPOBJ_GetDispOutParams(0, (VPP_DISP_OUT_PARAMS*) shmAddr, Size);
    }
    break;

    case VPP_SET_DISP_VOUTPARAMS:
    {
        int cpcbId;

        shmAddr = (UINT8 *)params[1].memref.buffer;
        Size = params[1].memref.size;

        VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
        res = VPP_ValidateInput(&bufDesc, 0);
        if(res != TEE_SUCCESS)
            break;

        cpcbId =  *((int*)shmAddr);
        params[2].value.a = MV_VPPOBJ_SetDispOutParams(0, (VPP_DISP_OUT_PARAMS*) (shmAddr + sizeof(int)), cpcbId);
    }
    break;

     case SET_ZORDER:
     {
         int cpcbID;
         shmAddr = (UINT8 *)params[1].memref.buffer;
         Size= sizeof(VPP_ZORDER_CTRL);
         VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
         res = VPP_ValidateInput(&bufDesc, 0);
         if(res != TEE_SUCCESS) {
             break;
         }
         cpcbID = *((int*)shmAddr);
         params[2].value.a = MV_VPPOBJ_ChangeZOrder(0, cpcbID, (VPP_ZORDER_CTRL*)(shmAddr + sizeof(int)));
     }
     break;

     case GET_ZORDER:
     {
         int cpcbID;
         shmAddr = (UINT8 *)params[1].memref.buffer;
         Size= sizeof(VPP_ZORDER_CTRL);
         VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
         res = VPP_ValidateInput(&bufDesc, 0);
         if(res != TEE_SUCCESS) {
             break;
         }
         cpcbID = *((int*)shmAddr);
         params[2].value.a = MV_VPPOBJ_GetZOrder(0, cpcbID, (VPP_ZORDER_CTRL*)(shmAddr + sizeof(int)));
     }
     break;
     case SET_HDMI_AUDIOCFG_PARAM:
     {
         int enable;

         shmAddr = (UINT8 *)params[1].memref.buffer;
         enable = *((int*)shmAddr);
         params[2].value.a = MV_VPPOBJ_SetHdmiAudioFmt(0, enable,(VPP_HDMI_AUDIO_CFG *)(shmAddr + sizeof(int)));

     }
     break;

     case SET_HDMI_AUDIO_INFO:
     {
         int lvlShiftVal;
         int downMixInhFlag;

         shmAddr = (UINT8 *)params[1].memref.buffer;
         lvlShiftVal = *((int*)shmAddr);
         downMixInhFlag = *((int*)shmAddr + 1);

         params[2].value.a = MV_VPPOBJ_SetHdmiAudioInfo(0, (VPP_HDMI_SPKR_ALLOC *)(shmAddr + 2*sizeof(int)), lvlShiftVal, downMixInhFlag);
     }
     break;

     case READ_HDMI_RAWEDID:
     {
         shmAddr = (UINT8 *)params[1].memref.buffer;
         params[2].value.a = MV_VPPOBJ_GetHDMIRawEdid(0, (VPP_HDMI_RAW_EDID *)(shmAddr));
     }
     break;

     default:
     res = TEE_ERROR_NOT_SUPPORTED;
     break;
    }
    return res;
}

#ifdef CONFIG_VPP_FASTLOGO_PUSHPAR_SUPPORT
VBUF_INFO * getCurrVbufInfo(int planeId) {
    int frameNdx = g_vpp->VPP_fl.currFrame[planeId];
    return &(g_vpp->VPP_fl.frame_info[planeId][frameNdx]);
}

void getNextVbufInfo(int planed) {
    g_vpp->VPP_fl.currFrame[planed]++;
    g_vpp->VPP_fl.currFrame[planed] %= VPP_MAX_FRAMES_PER_PLANE;
}

int MV_VPPOBJ_BindSHM_PAR(TEE_Param params[4])
{
    int shmId, planeId;
    VBUF_INFO *pVBufInfo;

    shmId = params[0].value.a;
    TEE_Result res = TEE_SUCCESS;

    switch(shmId&0xFF) {
    case DISPLAY_FRAME:
        planeId = (shmId >> 8);
        pVBufInfo = (VBUF_INFO *)getCurrVbufInfo(planeId);
#ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
        VPPAPI_DBG_INFO("%s:%d: planeId:%d, pVBufInfo:%p\n", __func__, __LINE__, planeId, pVBufInfo);
#endif //CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
        switch(params[0].value.b) {
            case 0:
                pVBufInfo->m_pbuf_start         = (ARCH_PTR_TYPE)params[1].value.a;
                pVBufInfo->m_bytes_per_pixel    = params[1].value.b;
                pVBufInfo->m_bits_per_pixel     = params[2].value.a;
                pVBufInfo->m_srcfmt             = params[2].value.b;
                pVBufInfo->m_order              = params[3].value.a;
            break;
            case 1:
                pVBufInfo->m_content_width      = params[1].value.a;
                pVBufInfo->m_content_height     = params[1].value.b;
                pVBufInfo->m_buf_stride         = params[2].value.a;
                pVBufInfo->m_buf_pbuf_start_UV  = params[2].value.b;
                pVBufInfo->m_buf_size           = params[3].value.a;
            break;
            case 2:
                pVBufInfo->m_allocate_type  = params[1].value.a;
                pVBufInfo->m_buf_type       = params[1].value.b;
                pVBufInfo->m_buf_use_state  = params[2].value.a;
                pVBufInfo->m_flags          = params[2].value.b;
                pVBufInfo->m_is_frame_seq   = params[3].value.a;
            break;
            case 3:
                pVBufInfo->m_frame_rate_num = params[1].value.a;
                pVBufInfo->m_frame_rate_den = params[1].value.b;
                pVBufInfo->m_active_width   = params[2].value.a;
                pVBufInfo->m_active_height  = params[2].value.b;
                pVBufInfo->m_active_left    = params[3].value.a;
            break;
            case 4:
                pVBufInfo->m_active_top         = params[1].value.a;
                pVBufInfo->m_content_offset     = params[1].value.b;
                pVBufInfo->m_is_progressive_pic = params[2].value.a;
                pVBufInfo->m_hDesc              = params[2].value.b;
                pVBufInfo->m_dv_el_descriptor=(ARCH_PTR_TYPE)NULL;
                getNextVbufInfo(planeId);
#ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
                printfVBInfo(pVBufInfo);
#endif //CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
                MV_VPPOBJ_DisplayFrame(0, planeId, pVBufInfo);
            break;
        }
        break;
    default:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
    }
    return res;
}
#endif //CONFIG_VPP_FASTLOGO_PUSHPAR_SUPPORT

int MV_VPPOBJ_BindSHM_PHY(TEE_Param params[4])
{
    VBUF_INFO *pinfo;
    int shmId, planeId, resID;
    UINT8 *shmAddr;
#ifdef CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
    VPP_SHM_BUFFER_DESC bufDesc;
#endif //CONFIG_VPP_FASTLOGO_VALIDATE_INPUT

    shmId = params[0].value.a;
    TEE_Result res = TEE_SUCCESS;
    unsigned int Size;

    switch(shmId) {
    case DISPLAY_FRAME:
        shmAddr = (UINT8 *)params[0].value.b;
        pinfo = (VBUF_INFO *)shmAddr;
        planeId = params[3].value.a;
        pinfo->m_dv_el_descriptor=(ARCH_PTR_TYPE)NULL;

#ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
        printfVBInfo(pinfo);
#endif //CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO

        pinfo = VPP_FrameInfoPool_AllocFrameInfo(planeId, pinfo);
        if (!pinfo) {
            return TEE_ERROR_OUT_OF_MEMORY;
        }
        MV_VPPOBJ_DisplayFrame(0, planeId, pinfo);
        break;

        case SET_STILL_PICTURE:
        shmAddr = (UINT8 *)params[0].value.b;
        pinfo = (VBUF_INFO *)shmAddr;
        planeId = params[3].value.a;
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
        if ((pinfo->m_srcfmt == SRCFMT_LUT8)
            && pinfo->m_clut_ptr
            && ((planeId >= PLANE_GFX0) && (planeId <= PLANE_PG)))
        {
            pinfo->m_clut_ptr = params[2].memref.buffer;
        }
#endif
#ifdef CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO
        printfVBInfo(pinfo);
#endif //CONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO

        pinfo = VPP_FrameInfoPool_AllocFrameInfo(planeId, pinfo);
        if (!pinfo) {
            return TEE_ERROR_OUT_OF_MEMORY;
        }
        MV_VPPOBJ_SetStillPicture(0, planeId, pinfo, NULL);
        break;

    case VPP_GET_RES_DESCRIPTION:
        {
            shmAddr = (UINT8 *)params[1].value.b;
            resID = params[2].value.a;
            Size = sizeof(VPP_RESOLUTION_DESCRIPTION);
            if(!shmAddr || Size != sizeof(VPP_RESOLUTION_DESCRIPTION))
            {
                res = TEE_ERROR_BAD_PARAMETERS;
                break;
            }

       #ifdef CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
            res = VPP_ValidateInput(&bufDesc, 0);
            if(res == TEE_SUCCESS)
       #endif //CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
            {
                params[2].value.a = MV_VPPOBJ_GetResolutionDescription(0, resID,
                                    (VPP_RESOLUTION_DESCRIPTION*)shmAddr);
            }
            break;
        }

#ifdef CONFIG_FASTLOGO_ENABLE_UNSUPPORTED_INTERFACE
    case VPP_SETHDMICONFIG:
       {
            shmAddr = (UINT8 *)params[1].memref.buffer;
            Size = params[1].memref.size;
       #ifdef CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
            res = VPP_ValidateInput(shmAddr, Size, 0);
            if(res != TEE_SUCCESS)
                break;
       #endif //CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
            params[2].value.a = MV_VPPOBJ_SetHdmiConfig(0, (unsigned int *)(shmAddr), params[1].memref.size);
       }
    break;
#endif

    case VPP_OBJCONFIG:
       {
           shmAddr = (UINT8 *)params[1].memref.buffer;
           Size = params[1].memref.size;
           INT8 *config;
           int plane_size;
           int vout_size;
       #ifdef CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
           res = VPP_ValidateInput(shmAddr, Size, 0);
           if(res != TEE_SUCCESS)
               break;
       #endif //CONFIG_VPP_FASTLOGO_VALIDATE_INPUT
           config = (INT8 *)shmAddr;
           plane_size = (sizeof(INT32)*(MAX_NUM_PLANES));
           vout_size = sizeof(INT32)* (MAX_NUM_VOUTS);
           params[2].value.a = MV_VPPOBJ_Config(0, (const int *) &config[0], (const int *) &config[(plane_size)],
                          (const int *) &config[(plane_size*2)], (const int *) &config[(plane_size*3)],
                          (const int *) &config[(plane_size*3) + vout_size]);
       }
    break;

    default:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
    }
    return res;
}
