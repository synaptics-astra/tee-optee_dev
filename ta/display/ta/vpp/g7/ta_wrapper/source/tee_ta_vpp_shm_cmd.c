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
#include "vpp_env_ur.h"
#include "ovps_api.h"
#include "vpp_ptm.h"
#include "tee_ta_vpp_porting.h"
#include "vpp_debug_util.h"
#include "outctrl_cmd.h"
#include "vpp_obj.h"

#include "vpp_be_hdr.h"
#include "vpp_dv_api.h"
#include "vpp_ngptv_api.h"
#include "vpp_vmx_ta_wrap.h"
#include "vpp_isr.h"
#include "tee_ta_cmd_check.h"

static int MV_VPPOBJ_BindSHM_SingleBuffer_getInfo(int shmId, unsigned int *pBufSize, VPP_SHM_BUFFER_TYPE *pBufType)
{
    TEE_Result res = TEE_SUCCESS;
    unsigned int Size = *pBufSize;
    VPP_SHM_BUFFER_TYPE bufType;

    switch(shmId) {
    case WRITE_REG_GROUP:
        Size = sizeof(VPP_REG_GROUP);
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case READ_REG_GROUP:
        Size = sizeof(VPP_REG_GROUP);
        bufType = VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT;
        break;

    case LOAD_LVDS_GAMMALUT:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

    case READ_LVDS_GAMMALUT:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

    case VPP_LVTXSETLANEMAPP:
#ifdef VPP_CONFIG_LVTX
        Size = sizeof(VBO_OP_LANE_MAPPING_TABLE);
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif

     case LOAD_LVDS_ICSC_CUSTCOEFFS:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

     case SET_LVDS_MAPPING:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
     break;

     case PREPARE_SCL_CUSTCOEFFS:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

     case SET_GFX_LUT:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;
     case SET_SHIFT3D_PARAM:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

     case GET_SHIFT3D_PARAM:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

     case SET_HDMI_AUDIOCFG_PARAM:
        Size = (sizeof(int)*1) + (sizeof(VPP_HDMI_AUDIO_CFG));
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

     case SET_HDMI_AUDIOVUCCFG_PARAM:
        Size = (sizeof(int)*1) + (sizeof(VPP_HDMI_AUDIO_VUC_CFG));
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

     case SET_HDMI_AUDIO_INFO:
        Size = (sizeof(int)*2) + (sizeof(VPP_HDMI_SPKR_ALLOC));
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

     case GET_HDMI_SINKCAPS:
     case READ_HDMI_SINKCAPS:
        Size = sizeof(VPP_HDMI_SINK_CAPS);
        bufType = VPP_SHM_BUFFER_TYPE_OUTPUT;
        break;

     case GET_HDMI_SINKFEATUREMAP:
        Size = sizeof(VPP_HDMI_SINKCAP_BITMASK);
        bufType = VPP_SHM_BUFFER_TYPE_OUTPUT;
        break;

     case READ_HDMI_RAWEDID:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT;
        break;

     case READ_HDMI_HDCP_AKSV:
        //Size = same as *pBufSize
        if (Size != sizeof(VPP_HDMI_HDCP_AKSV)) {
            res = TEE_ERROR_BAD_PARAMETERS;
            break;
        }
        bufType = VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT;
        break;

     case SEND_HDMI_AUXPKT:
        Size = (sizeof(int)*2) + (sizeof(VPP_HDMI_PKT));
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

     case SET_ZORDER:
        Size = sizeof(VPP_ZORDER_CTRL);
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

     case GET_ZORDER:
        Size = sizeof(VPP_ZORDER_CTRL);
        bufType = VPP_SHM_BUFFER_TYPE_OUTPUT;
        break;

    case VPP_SETCLKSIGNATURE:
        Size = sizeof(UINT32) * (VPP_CLKGATE_SIG_MAX_WORDS+1);
        bufType = VPP_SHM_BUFFER_TYPE_OUTPUT;
        break;

     case SET_WSSDATA:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

     case SET_CGMSDATA:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

     case CEC_SEND_MSG:
        Size = sizeof(VPP_CEC_MSG);
        bufType = VPP_SHM_BUFFER_TYPE_INPUT | VPP_SHM_BUFFER_TYPE_SECURE_INPUT;
        break;

    case CEC_READ_EVT_INFO:
        Size = sizeof(VPP_HDMI_CEC_EVT_INFO);
        bufType = VPP_SHM_BUFFER_TYPE_OUTPUT;
        break;

#ifdef EARC_RX_ENABLED
    case EARC_READ_EVT_INFO:
        Size = sizeof(EARC_EVT_INFO);
        bufType = VPP_SHM_BUFFER_TYPE_OUTPUT;
        break;
    case EARC_SET_CAPS:
        /* Validate minimum size to check whether length has been received
         * or not*/
        Size = (sizeof(int)*2);
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;
    case VPP_EARC_SETCONFIG:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;
#endif /* EARC_RX_ENABLED */

    case VPP_GET_RES_DESCRIPTION:
        //Size = same as *pBufSize
        if (Size != sizeof(VPP_RESOLUTION_DESCRIPTION)) {
            res = TEE_ERROR_BAD_PARAMETERS;
            break;
        }
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case GET_HDCP_KSV:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT;
        break;

    case GET_HDMI_AUXPKT:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT;
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
        break;

    case GET_PLANE_VIDEO_INFO:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT;
        if (Size != sizeof(VPP_PLANE_VIDEO_INFO)) {
           res = TEE_ERROR_BAD_PARAMETERS;
        }
        break;

    case VPP_SETHDMIPHY:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

#ifdef CONFIG_AMP_DOLBY_DUMP
    case VPP_DVINITFRAMEDUMP:
        Size = VPP_DVINITFRAMEDUMP_SIZE;
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;
    case VPP_DVGETFRAMEDUMPINFO:
        Size = VPP_DVINITFRAMEDUMP_SIZE;
        bufType = VPP_SHM_BUFFER_TYPE_OUTPUT;
        break;
    case VPP_DVLOADVSDB:
        Size = 32;
        bufType = VPP_SHM_BUFFER_TYPE_INPUT | VPP_SHM_BUFFER_TYPE_SECURE_INPUT;
        break;
#endif //CONFIG_AMP_DOLBY_DUMP

    case VPP_SETHDRCONFIG:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case VPP_SETHDMICONFIG:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case VPP_OBJCONFIG:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case VPP_SETFORMAT:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case LOAD_HDCP_KEYS:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case VPP_MIPI_CONFIG:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case VPP_GET_DISP_VOUTPARAMS:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INOUT;
        break;

    case VPP_SET_DISP_VOUTPARAMS:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case VPP_CAPTUREVIDFRAME:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    case VPP_HDMI_SPECIAL_SINKID:
        //Size = same as *pBufSize
        bufType = VPP_SHM_BUFFER_TYPE_INPUT;
        break;

    default:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
    }

    if (TEE_SUCCESS == res) {
        *pBufSize = Size;
        *pBufType = bufType;
    }

    return res;
}

static TEE_Result VPP_MapAndValidate_Param(void **mappedAddr, TEE_Param *pParams, UINT32 sizeParam, VPP_SHM_BUFFER_TYPE bufType)
{
    TEE_Result res = TEE_ERROR_BAD_PARAMETERS;
#ifdef VPP_ENABLE_PASSSHM_BY_MEMREF
    UINT8 *shmAddr = (UINT8 *)pParams->memref.buffer;
    unsigned int Size = pParams->memref.size;
#else //VPP_ENABLE_PASSSHM_BY_MEMREF
    UINT8 *shmAddr = (UINT8 *)pParams->value.a;
    unsigned int Size = pParams->value.b;
    VPP_BUFF_HANDLE buffHandle;
#endif //VPP_ENABLE_PASSSHM_BY_MEMREF
    VPP_SHM_BUFFER_DESC bufDesc;

    //Validate only valid buffer address/id
    if (shmAddr) {
        //Override size with the given sizeParam
        if (sizeParam) {
            Size = sizeParam;
        }

        //Check the buffer validity based on buffer type
        VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, bufType);
#ifdef VPP_ENABLE_PASSSHM_BY_MEMREF
        res = VPP_Validate_SingleBuffer(&bufDesc);
#else //VPP_ENABLE_PASSSHM_BY_MEMREF
        res = VPP_MapAndValidate_SingleBuffer(&buffHandle, &bufDesc);
#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
        //If continue to use physical address, then
        //Force change base address also from virtual to physical
        shmAddr = buffHandle.phy_addr;
#else //VPP_ENABLE_MEMID_BY_VIRT_ADDR
        shmAddr = buffHandle.virt_addr;
#endif //VPP_ENABLE_MEMID_BY_VIRT_ADDR
#endif //VPP_ENABLE_PASSSHM_BY_MEMREF

        if (res != TEE_SUCCESS) {
            VPPAPI_DBG_ERROR("Buffer from CA fail security check");
        } else {
            //Return back the virt/phy address to access from this TA instance.
            *mappedAddr = shmAddr;
        }
    }

    return res;
}

static int MV_VPPOBJ_BindSHM_SingleBuffer(TEE_Param params[4])
{
    TEE_Result res;
    int shmId = params[0].value.a;
#ifdef VPP_ENABLE_PASSSHM_BY_MEMREF
    UINT8 *shmAddr = (UINT8 *)params[1].memref.buffer;
    unsigned int Size = params[1].memref.size;
#else //VPP_ENABLE_PASSSHM_BY_MEMREF
    UINT8 *shmAddr = (UINT8 *)params[1].value.a;
    unsigned int Size = params[1].value.b;
#endif //VPP_ENABLE_PASSSHM_BY_MEMREF
    VPP_SHM_BUFFER_TYPE bufType = VPP_SHM_BUFFER_TYPE_INPUT;

    if (!shmAddr) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    //Get the size and buffer type details - default size is from paramas
    res = MV_VPPOBJ_BindSHM_SingleBuffer_getInfo(shmId, &Size, &bufType);
    if (TEE_SUCCESS != res) {
        //Not a single buffer command, then return immediately
        return res;
    }

    //Validate the buffer in params[1]
    res = VPP_MapAndValidate_Param((void**)&shmAddr, &params[1], Size, bufType);
    if (TEE_SUCCESS != res) {
        VPPAPI_DBG_ERROR("Single I/O Buffer fail security check");
        return res;
    }

    switch(shmId) {

    case WRITE_REG_GROUP:
        params[2].value.a = MV_VPPOBJ_WriteRegGroup(0,(VPP_REG_GROUP *)shmAddr);
        break;

    case READ_REG_GROUP:
        params[2].value.a = MV_VPPOBJ_ReadRegGroup(0,(VPP_REG_GROUP *)shmAddr);
        break;

    case LOAD_LVDS_GAMMALUT:
        {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            UINT8 deGamma_Gamma, optRGB;
            deGamma_Gamma = *shmAddr;
            optRGB = *(shmAddr+4);
            params[2].value.a = MV_VPPOBJ_LoadLvdsGammaLUT(0, deGamma_Gamma, optRGB, (UINT16 *)(shmAddr + sizeof(UINT32)*2));
#else
            res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;

    case READ_LVDS_GAMMALUT:
        {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            UINT8 deGamma_Gamma, optRGB;
            deGamma_Gamma = *shmAddr;
            optRGB = *(shmAddr+4);
            params[2].value.a = MV_VPPOBJ_ReadLvdsGammaLut(0, deGamma_Gamma, optRGB, (UINT16 *)(shmAddr + sizeof(UINT32)*2));
#else
            res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;

    case VPP_LVTXSETLANEMAPP:
        {
#ifdef VPP_CONFIG_LVTX
            params[2].value.a = MV_VPPOBJ_VBO_SetLaneMapping(0, shmAddr);
#else
            res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;

     case LOAD_LVDS_ICSC_CUSTCOEFFS:
        {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            UINT8 deGamma_Gamma;
            deGamma_Gamma = *shmAddr;
            params[2].value.a = MV_VPPOBJ_LoadLvdsIcscCustCoeffs(0, deGamma_Gamma,(UINT32 *)(shmAddr + sizeof(UINT32)));
#else
            res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;

     case SET_LVDS_MAPPING:
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            params[2].value.a = MV_VPPOBJ_SetLvdsMapping(0, shmAddr);
#else
            res = TEE_ERROR_NOT_SUPPORTED;
#endif
     break;

     case PREPARE_SCL_CUSTCOEFFS:
        {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            int sclUnit, coeffID;
            sclUnit = *((int*)shmAddr);
            coeffID = *((int*)shmAddr+1);
            params[2].value.a = MV_VPPOBJ_PrepareSCLCustCoeffs(0, sclUnit, coeffID,(UINT16 *)(shmAddr + sizeof(int)*2));
#else
            res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;

     case SET_GFX_LUT:
        {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            int planeID, idxStart, idxCount;
            planeID = *((int *)shmAddr);
            idxStart = *((int *)shmAddr + 1);
            idxCount = *((int *)shmAddr + 2);
            params[2].value.a = MV_VPPOBJ_SetGfxClut(0, planeID, idxStart, idxCount,(int *)(shmAddr + sizeof(int)*3));
#else
                res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;
     case SET_SHIFT3D_PARAM:
        {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            int shift3dCmd;
            shift3dCmd = *((int*)shmAddr);
            params[2].value.a = MV_VPPOBJ_SetSHIFT3DParam(0, shift3dCmd,(VPP_VDO_SHIFT3D_REG *)(shmAddr + sizeof(int)));
#else
                res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;

     case GET_SHIFT3D_PARAM:
        {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
            int shift3dCmd;
            shift3dCmd = *((int*)shmAddr);
            params[2].value.a = MV_VPPOBJ_GetSHIFT3DParam(0, shift3dCmd,(VPP_VDO_SHIFT3D_REG *)(shmAddr + sizeof(int)));
#else
                res = TEE_ERROR_NOT_SUPPORTED;
#endif
        }
        break;

     case SET_HDMI_AUDIOCFG_PARAM:
        {
            int enable;
            enable = *((int*)shmAddr);
            params[2].value.a = MV_VPPOBJ_SetHdmiAudioFmt(0, enable,(VPP_HDMI_AUDIO_CFG *)(shmAddr + sizeof(int)));
        }
        break;

     case SET_HDMI_AUDIOVUCCFG_PARAM:
        {
            int loadOnAudioCfg;
            loadOnAudioCfg = *((int*)shmAddr);
            params[2].value.a = MV_VPPOBJ_SetHdmiAudioVUCBits(0, (VPP_HDMI_AUDIO_VUC_CFG *)(shmAddr + sizeof(int)), loadOnAudioCfg);
        }
        break;

     case SET_HDMI_AUDIO_INFO:
        {
            int lvlShiftVal;
            int downMixInhFlag;
            lvlShiftVal = *((int*)shmAddr);
            downMixInhFlag = *((int*)shmAddr + 1);
            params[2].value.a = MV_VPPOBJ_SetHdmiAudioInfo(0, (VPP_HDMI_SPKR_ALLOC *)(shmAddr + 2*sizeof(int)), lvlShiftVal, downMixInhFlag);
        }
        break;

     case GET_HDMI_SINKCAPS:
     case READ_HDMI_SINKCAPS:
        params[2].value.a = MV_VPPOBJ_GetHDMISinkCaps(0, (VPP_HDMI_SINK_CAPS *)(shmAddr));
        break;

     case GET_HDMI_SINKFEATUREMAP:
        params[2].value.a = MV_VPPOBJ_GetHDMISinkFeatureMap(0, (VPP_HDMI_SINKCAP_BITMASK *)(shmAddr));
        break;

     case READ_HDMI_RAWEDID:
        params[2].value.a = MV_VPPOBJ_GetHDMIRawEdid(0, (VPP_HDMI_RAW_EDID *)(shmAddr));
        break;

     case READ_HDMI_HDCP_AKSV:
        params[2].value.a = MV_VPPOBJ_GetHDCPAksv(0, (VPP_HDMI_HDCP_AKSV *)(shmAddr));
        break;

     case SEND_HDMI_AUXPKT:
        {
            int pktType;
            int repeatPkt;
            pktType = *((int*)shmAddr);
            repeatPkt = *((int*)shmAddr + 1);
            params[2].value.a = MV_VPPOBJ_SendHdmiAuxPkt(0, (VPP_HDMI_PKT_ID)pktType,
                    (VPP_HDMI_PKT *)(shmAddr + 2*sizeof(int)), (BOOL)repeatPkt);
        }
        break;

     case SET_ZORDER:
        {
            int cpcbID;
            cpcbID = *((int*)shmAddr);
            params[2].value.a = MV_VPPOBJ_ChangeZOrder(0, cpcbID, (VPP_ZORDER_CTRL*)(shmAddr + sizeof(int)));
        }
        break;

     case GET_ZORDER:
        {
            int cpcbID;
            cpcbID = *((int*)shmAddr);
            params[2].value.a = MV_VPPOBJ_GetZOrder(0, cpcbID, (VPP_ZORDER_CTRL*)(shmAddr + sizeof(int)));
        }
        break;

    case VPP_SETCLKSIGNATURE:
        {
            VPP_CLKGATE_OPT options;
            int offset=0;
            options.opt = *((int*)shmAddr);
            offset=sizeof(int);
            params[2].value.a = MV_VPPOBJ_SetClkGateSignature(0, (VPP_CLK_GATE_SIGNATURE*)(shmAddr + offset), (VPP_CLK_GATE_SIGNATURE*)(shmAddr + offset+ (VPP_CLKGATE_SIG_MAX_WORDS*(sizeof(UINT32)))),options);
        }
        break;

     case SET_WSSDATA:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

     case SET_CGMSDATA:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;

     case CEC_SEND_MSG:
        params[2].value.a = MV_VPPOBJ_CecSendMessage(0, (PVPP_CEC_MSG)shmAddr);
        break;

    case CEC_READ_EVT_INFO:
        params[2].value.a = VPP_GetCECEvtInfo(0,(void *)shmAddr);
        break;

#ifdef EARC_RX_ENABLED
    case EARC_READ_EVT_INFO:
        params[2].value.a = VPP_GetEarcEvtInfo(0,(void *)shmAddr);
        break;

    case EARC_SET_CAPS:
        {
            UINT8 offset;
            size_t length;
            VPP_SHM_BUFFER_DESC bufDesc;

            offset = *((int*)shmAddr);
            length = *((int*)shmAddr + 1);

            /* Validate total length */
            Size= (sizeof(int)*2 + length);

            VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmAddr, Size, VPP_SHM_BUFFER_TYPE_INPUT);
            res = VPP_Validate_SingleBuffer(&bufDesc);
            if (res != TEE_SUCCESS) {
                break;
            }

            params[2].value.a = MV_VPPOBJ_eARC_SetCapabilities(0, offset, length, (UINT8 *)((int *)shmAddr + 2));
        }
        break;

    case VPP_EARC_SETCONFIG:
        params[2].value.a = MV_VPPOBJ_eARC_SetConfig(0, (EARC_CONFIG *)(shmAddr));
        break;
#endif /* EARC_RX_ENABLED */

    case VPP_GET_RES_DESCRIPTION:
        {
            int resID = params[2].value.a;
            params[2].value.a = MV_VPPOBJ_GetResolutionDescription(0, resID,
                                (VPP_RESOLUTION_DESCRIPTION*)shmAddr);
            break;
        }

    case GET_HDCP_KSV:
       params[2].value.a = MV_VPPOBJ_GetHDCPKsv(0, (VPP_HDCP_KSV_DATA *)shmAddr);
    break;

    case GET_HDMI_AUXPKT:
       {
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
           VPP_HDMI_PKT_ID pktType;
           pktType = *((VPP_HDMI_PKT_ID*)shmAddr);
           params[2].value.a = MV_VPPOBJ_GetHdmiAuxPacket(0, pktType, (VPP_HDMI_TX_DATA_PACKET *)shmAddr);
#else
        res = TEE_ERROR_NOT_SUPPORTED;
#endif
       }
    break;

    case GET_PLANE_VIDEO_INFO:
       {
           UINT32 PlaneId;
           PlaneId = *((UINT32*)shmAddr);
           params[2].value.a = MV_VPPOBJ_GetVideoInfo(0, PlaneId, (VPP_PLANE_VIDEO_INFO *)shmAddr);
       }
    break;

    case VPP_SETHDMIPHY:
       {
           params[2].value.a = MV_VPPOBJ_SetHdmiPhy(0, (unsigned int *)(shmAddr), Size);
       }
    break;

#ifdef CONFIG_AMP_DOLBY_DUMP
    case VPP_DVINITFRAMEDUMP:
        {
            if(!VPP_ENV_SA_isSecurePlayback(0,PLANE_MAIN))
            {
                INT Path;
                INT Enable;
                VPP_DV_FRAME_DUMP *pFrameDumpConfig;
                int offset=0;
                Path = *((int*)shmAddr);
                offset+=sizeof(int);
                Enable = *((int*)(shmAddr+offset));
                offset+=sizeof(int);
                pFrameDumpConfig = (VPP_DV_FRAME_DUMP*)(shmAddr + offset);
                if(Enable == 1)
                {
                    VPP_SHM_BUFFER_DESC bufDesc;
                    VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)pFrameDumpConfig->phyAddr, pFrameDumpConfig->frameSize,
                                        VPP_SHM_BUFFER_TYPE_NON_SECURE_OUTPUT);
                    res = VPP_Validate_SingleBuffer(&bufDesc);
                    if(res != TEE_SUCCESS){
                        VPPAPI_DBG_ERROR("VPP_DVINITFRAMEDUMP Fail security check\n");
                        break;
                    }
                }
                params[2].value.a = MV_VPPOBJ_DV_InitFrameDump(0, Path, Enable, pFrameDumpConfig);
            }
            else
            {
                VPPAPI_DBG_INFO("VPP_DVINITFRAMEDUMP Not Secure Playback  Failed\n");
                res = TEE_ERROR_ACCESS_DENIED;
            }
        }
        break;

    case VPP_DVGETFRAMEDUMPINFO:
        {
            VPP_DV_FRAME_DUMP *pFrameDumpConfig;
            pFrameDumpConfig = (VPP_DV_FRAME_DUMP*)(shmAddr);
            params[2].value.a = MV_VPPOBJ_DV_GetFrameDumpInfo(0, pFrameDumpConfig);
        }
        break;

    case VPP_DVLOADVSDB:
        params[2].value.a = MV_VPPOBJ_LoadVSDB(0, shmAddr);
        break;
#endif //CONFIG_AMP_DOLBY_DUMP

    case VPP_SETHDRCONFIG:
        params[2].value.a = MV_VPPOBJ_SetHDRConfig(0, (VPP_HDR_CONFIG_PARAM *)(shmAddr));
    break;

    case VPP_SETHDMICONFIG:
        params[2].value.a = MV_VPPOBJ_SetHdmiConfig(0, (unsigned int *)(shmAddr), params[1].memref.size);
    break;

    case VPP_OBJCONFIG:
       {
           INT8 *config;
           int plane_size;
           int vout_size;
           config = (INT8 *)shmAddr;
           plane_size = (sizeof(INT32)*(MAX_NUM_PLANES));
           vout_size = sizeof(INT32)* (MAX_NUM_VOUTS);
           params[2].value.a = MV_VPPOBJ_Config(0,(const int *) &config[0],(const int *) &config[(plane_size)],
                           (const int *) &config[(plane_size*2)],(const int *) &config[(plane_size*3)],
                           (const int *) &config[(plane_size*3) + vout_size]);
       }
    break;

    case VPP_SETFORMAT:
    {
        int cpcbId;
        cpcbId = *((int*)shmAddr);
        params[2].value.a = MV_VPPOBJ_SetFormat(0, cpcbId, (void *)(shmAddr + sizeof(int)));
    }
    break;

    case LOAD_HDCP_KEYS:
        params[2].value.a = MV_VPPOBJ_LoadHdcpKeys(0, (VPP_HDCP_KEYSTORE_DATA *) shmAddr);
        break;

    case VPP_MIPI_CONFIG:
    {
        VPP_MIPI_LOAD_CONFIG *pConfigParams;

        pConfigParams = (VPP_MIPI_LOAD_CONFIG*) shmAddr;
        params[2].value.a = MV_VPPOBJ_LoadMipiConfig(0, pConfigParams);
    }
    break;

    case VPP_GET_DISP_VOUTPARAMS:
    {
        params[2].value.a = MV_VPPOBJ_GetDispOutParams(0, (VPP_DISP_OUT_PARAMS*) shmAddr, Size);
    }
    break;

    case VPP_SET_DISP_VOUTPARAMS:
    {
        int cpcbId;
        cpcbId = *((int*)shmAddr);
        params[2].value.a = MV_VPPOBJ_SetDispOutParams(0, (VPP_DISP_OUT_PARAMS*) (shmAddr + sizeof(int)), cpcbId);
    }
    break;

#ifdef VPP_ENABLE_FRAME_CAPTURE
    case VPP_CAPTUREVIDFRAME:
    {
        VPP_CAPTURE_FRAME_SETTING *pCap_Frm;

        pCap_Frm = (VPP_CAPTURE_FRAME_SETTING*) shmAddr;
        if(!VPP_ENV_SA_isSecurePlayback(0, pCap_Frm->PlaneId)) {
            res = TEE_CheckMemoryOutputRights((void *)pCap_Frm->pFrmBuf, pCap_Frm->FrmBufSize, TEE_NONSECURE, TEE_MT_COPY);
            if(res != TEE_SUCCESS){
                VPPAPI_DBG_ERROR("VPP_CAPTUREVIDFRAME fail security check");
                break;
            }
            params[2].value.a = MV_VPPOBJ_CaptureVideoFrame(0, pCap_Frm);
        } else {
            VPPAPI_DBG_ERROR("VPP_CAPTUREVIDFRAME: Secure Playback");
            res = TEE_ERROR_ACCESS_DENIED;
        }
    }
    break;
#endif

    case VPP_HDMI_SPECIAL_SINKID:
    {
       params[2].value.a = MV_VPPOBJ_SetSpecialSinkIds(0, shmAddr, Size);
    }
    break;
    default:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
    }
    return res;
}

static void* VPP_MapAndValidate_VbufInfo(TEE_Param params[4])
{
    TEE_Result res;
    VBUF_INFO *pinfo = NULL;
    //No need to invoke MV_VPPOBJ_BindSHM_SingleBuffer_getInfo() for bufType & size
    const VPP_SHM_BUFFER_TYPE bufType = VPP_SHM_BUFFER_TYPE_INPUT;
    unsigned int Size = sizeof(VBUF_INFO);

    res = VPP_MapAndValidate_Param((void**)&pinfo, &params[1], Size, bufType);
    if (res != TEE_SUCCESS) {
        VPPAPI_DBG_ERROR("VBUF_INFO Buffer fail security check");
    }
#ifdef ENABLE_BG4_NON_SUPPORTED_INTERFACE
    else if ((pinfo->m_srcfmt == SRCFMT_LUT8) && pinfo->m_clut_ptr &&
             ((planeId >= PLANE_GFX0) && (planeId <= PLANE_PG))) {
#ifdef VPP_ENABLE_PASSSHM_BY_MEMREF
        Size = params[2].memref.size;
#else //VPP_ENABLE_PASSSHM_BY_MEMREF
        Size = params[2].value.b;
#endif //VPP_ENABLE_PASSSHM_BY_MEMREF
        res = VPP_MapAndValidate_Param((void**)&pinfo->m_clut_ptr, &params[2], Size, bufType);
        if (res != TEE_SUCCESS) {
            VPPAPI_DBG_ERROR("VBUF_INFO->m_clut_ptr Buffer fail security check");
        }
    }
#endif

    return pinfo;
}

static int MV_VPPOBJ_BindSHM_VbufInfoBuffer(TEE_Param params[4])
{
    VBUF_INFO *pinfo;
    int shmId, planeId;
    TEE_Result res = TEE_SUCCESS;

    shmId = params[0].value.a;
    planeId = params[3].value.a;

    switch(shmId) {
    case DISPLAY_FRAME:
        pinfo = VPP_MapAndValidate_VbufInfo(params);
        if (!pinfo) {
            break;
        }
        pinfo->m_dv_el_descriptor = (ARCH_PTR_TYPE)NULL;
        MV_VPPOBJ_DisplayFrame(0, planeId, pinfo);
        break;

    case DISPLAY_EARLY_FRAME:
        pinfo = VPP_MapAndValidate_VbufInfo(params);
        if (!pinfo) {
            break;
        }
        MV_VPPOBJ_EarlyDisplayFrame (0, planeId, pinfo);
        break;

    case SET_STILL_PICTURE:
        pinfo = VPP_MapAndValidate_VbufInfo(params);
        if (!pinfo) {
            break;
        }
        MV_VPPOBJ_SetStillPicture(0, planeId, pinfo, NULL);
        break;

    default:
        res = TEE_ERROR_NOT_SUPPORTED;
        break;
    }

    return res;
}

int MV_VPPOBJ_BindSHM(TEE_Param params[4])
{
    TEE_Result res;

    res = MV_VPPOBJ_BindSHM_VbufInfoBuffer(params);

    if (res == TEE_ERROR_NOT_SUPPORTED) {
        res = MV_VPPOBJ_BindSHM_SingleBuffer(params);
    }

    return res;
}

