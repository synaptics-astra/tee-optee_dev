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
//! \file m2m_ta_fw.c
//!
//! \brief TSP firmware driver for mem2mem crypto module.
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    Aug 4 2015,              Long Xiao
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "m2m_ta_mgr.h"
#include "m2m_ta_drv.h"
#include "m2m_ta_fw.h"

#include "tsp_filter_allinone_figo.h"
#include "tsp.h"

#if defined(CONFIG_GENX_ENABLE)
#include "bcm_mailbox.h"
#include "bcm_status.h"
#endif

// NOTE: drm headers just copy to local modules.
#ifdef CONFIG_ENABLE_DRM
#include "drm_se_ta.h"
#include "drm_contentkey.h"
#endif

#define DDR2BIU_ALIGN4(p)   (((UINT32)(p))&~3)

#ifdef TA_COMPILE

#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif

static TEE_TASessionHandle TSP_TA_session[2] = {NULL,NULL};
#ifdef CONFIG_ENABLE_DRM
static TEE_TASessionHandle DRM_MGR_TA_session = NULL;
static void *bUUIDHdlVa = NULL;
static uint32_t ShmIdUUID;
#endif

static TEE_Result TspTA_session_open(UINT32 uFigoId)
{
      TEE_Result res = TEE_SUCCESS;
      const TEE_UUID uuid = TA_TSP_UUID;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};;
      uint32_t        returnOrigin;

      /*Open Session to FIGO TA*/
      if(!TSP_TA_session[uFigoId]) {
           core_params[0].value.a = uFigoId;
           core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                     TEE_PARAM_TYPE_NONE,
                                     TEE_PARAM_TYPE_NONE,
                                     TEE_PARAM_TYPE_NONE);
           res = TEE_OpenTASession(&uuid,
                                 cancellationTimeout,
                                 core_paramTypes,
                                 core_params,
                                 &TSP_TA_session[uFigoId],
                                 &returnOrigin);
          TA_LOGH("open session. result=0x%x, origin=0x%x, session=%p\n",
              res, returnOrigin, TSP_TA_session[uFigoId]);
          if (res != TEE_SUCCESS) {
              TA_LOGF("fail to open session. result=0x%08x, origin=%d, session=%p\n",
                                       res, returnOrigin, TSP_TA_session[uFigoId]);
              return TEE_ERROR_NOT_SUPPORTED;
          }
      }
      return res;
}

static TEE_Result TspTA_Dtcm_Alloc(UINT16 *mem_start, UINT16 mem_size, UINT8 align_size, UINT8 figoid, DTCM_MEM_TYPE type)
{
      TEE_Result res = TEE_SUCCESS;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};
      uint32_t        returnOrigin;

      core_paramTypes = TEE_PARAM_TYPES( TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_VALUE_OUTPUT,
                                         TEE_PARAM_TYPE_NONE);

      core_params[0].value.a = figoid;
      core_params[0].value.b = type;

      core_params[1].value.a = mem_size;
      core_params[1].value.b = align_size;

      res = TEE_InvokeTACommand(TSP_TA_session[figoid], cancellationTimeout, TSP_DTCM_MEM_ALLOC,
			core_paramTypes, core_params,&returnOrigin);
      if (res != TEE_SUCCESS) {
              TA_LOGF("fail to setscheme. result=0x%08x, origin=%d, session=%p\n",
                                       res, returnOrigin, TSP_TA_session[figoid]);
              return TEE_ERROR_NOT_SUPPORTED;
      }
      *mem_start = core_params[2].value.a;
      return res;
}

static TEE_Result TspTA_Dtcm_Free(UINT16 mem_start, UINT8 figoid, DTCM_MEM_TYPE type)
{
      TEE_Result res = TEE_SUCCESS;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};
      uint32_t        returnOrigin;

      core_paramTypes = TEE_PARAM_TYPES( TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_NONE,
                                         TEE_PARAM_TYPE_NONE);

      core_params[0].value.a = figoid;
      core_params[0].value.b = type;

      core_params[1].value.a = mem_start;

      res = TEE_InvokeTACommand(TSP_TA_session[figoid], cancellationTimeout, TSP_DTCM_MEM_FREE,
            core_paramTypes, core_params,&returnOrigin);
      if (res != TEE_SUCCESS) {
              TA_LOGF("fail to setscheme. result=0x%08x, origin=%d, session=%p\n",
                                       res, returnOrigin, TSP_TA_session[figoid]);
              return TEE_ERROR_NOT_SUPPORTED;
      }
      return res;
}

static TEE_Result TspTA_Task_Alloc(UINT8 figoid, UINT16 task_addr, INT *task_id)
{
      TEE_Result res = TEE_SUCCESS;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};
      uint32_t        returnOrigin;

      core_paramTypes = TEE_PARAM_TYPES( TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_VALUE_OUTPUT,
                                         TEE_PARAM_TYPE_NONE,
                                         TEE_PARAM_TYPE_NONE);

      core_params[0].value.a = task_addr;

      res = TEE_InvokeTACommand(TSP_TA_session[figoid], cancellationTimeout, TSP_ALLOC_TASK,
            core_paramTypes, core_params,&returnOrigin);
      if (res != TEE_SUCCESS) {
          TA_LOGF("fail to alloc task. result=0x%08x, origin=%d, session=%p\n",
                                   res, returnOrigin, TSP_TA_session[figoid]);
      }else {
          *task_id = core_params[1].value.a;
      }
      return res;
}

static TEE_Result TspTA_Task_Free(UINT8 figoid, INT task_id)
{
      TEE_Result res = TEE_SUCCESS;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};
      uint32_t        returnOrigin;

      core_paramTypes = TEE_PARAM_TYPES( TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_NONE,
                                         TEE_PARAM_TYPE_NONE,
                                         TEE_PARAM_TYPE_NONE);

      core_params[0].value.a = task_id;

      res = TEE_InvokeTACommand(TSP_TA_session[figoid], cancellationTimeout, TSP_FREE_TASK,
            core_paramTypes, core_params,&returnOrigin);
      if (res != TEE_SUCCESS) {
          TA_LOGF("fail to free task. result=0x%08x, origin=%d, session=%p\n",
                                   res, returnOrigin, TSP_TA_session[figoid]);
      }
      return res;
}

static TEE_Result TspTA_Sched_Figo(UINT32 *pFigoId, UINT32 *pFWIdx, UINT32 *pNeedReload)
{
      TEE_Result res = TEE_SUCCESS;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};
      uint32_t        returnOrigin;

      res = TspTA_session_open(0);
      if (res != TEE_SUCCESS) {
        TA_LOGF("fail to TspTA_session_open. result=0x%08x\n", res);
        return 0xFFFF;
      }

      core_paramTypes = TEE_PARAM_TYPES( TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_VALUE_OUTPUT,
                                         TEE_PARAM_TYPE_VALUE_OUTPUT);

      core_params[0].value.a = 0x4; //TSP_INPUT_TYPE_M2M
      core_params[0].value.b = 0;  //ingore tsi port for m2m
      core_params[1].value.a = 0;  //ingore scheme for m2m
      core_params[1].value.b = 0;  //ingore cryptelevel for m2m

      res = TEE_InvokeTACommand(TSP_TA_session[0], cancellationTimeout, TSP_FIGO_SCHED,
                        core_paramTypes, core_params,&returnOrigin);

      *pFigoId = core_params[2].value.a;
      *pFWIdx = core_params[2].value.b;
      *pNeedReload = core_params[3].value.a;
      if (res != TEE_SUCCESS) {
          TA_LOGF("fail to sched figo, res=0x%x\n",res);
      } else if (*pFigoId >= TSP_FIGO_NR) { //not must, just for riscure review
          res = TEE_ERROR_NOT_SUPPORTED;
      }
      TEE_CloseTASession(TSP_TA_session[0]);
      TSP_TA_session[0] = NULL;
      return res;
}

static TEE_Result TspTA_Figo_Unsched(UINT32 uFigoId)
{
      TEE_Result res = TEE_SUCCESS;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};
      uint32_t        returnOrigin;

      res = TspTA_session_open(0);
      if (res != TEE_SUCCESS) {
        TA_LOGF("fail to TspTA_session_open. result=0x%08x\n", res);
        return 0xFFFF;
      }

      core_paramTypes = TEE_PARAM_TYPES( TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_VALUE_INPUT,
                                         TEE_PARAM_TYPE_NONE,
                                         TEE_PARAM_TYPE_NONE);

      core_params[0].value.a = TSP_INPUT_TYPE_M2M;
      core_params[0].value.b = 0;  //ingore tsi port for m2m
      core_params[1].value.a = uFigoId;

      res = TEE_InvokeTACommand(TSP_TA_session[0], cancellationTimeout, TSP_FIGO_UNSCHED,
                        core_paramTypes, core_params,&returnOrigin);

      if (res != TEE_SUCCESS) {
          TA_LOGF("fail to release figo, res=0x%x\n",res);
      }
      TEE_CloseTASession(TSP_TA_session[0]);
      TSP_TA_session[0] = NULL;
      return res;
}

#ifdef CONFIG_ENABLE_DRM
TEE_Result Drm_KeyMgr_TA_session_open(void)
{
      TEE_Result res = TEE_SUCCESS;
      const TEE_UUID uuid = DRM_SE_UUID;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};
      uint32_t        returnOrigin;

      /*Open Session to FIGO TA*/
      if(!DRM_MGR_TA_session) {
           core_params[0].value.a = 0;
           core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                     TEE_PARAM_TYPE_NONE,
                                     TEE_PARAM_TYPE_NONE,
                                     TEE_PARAM_TYPE_NONE);
           res = TEE_OpenTASession(&uuid,
                                 cancellationTimeout,
                                 core_paramTypes,
                                 core_params,
                                 &DRM_MGR_TA_session,
                                 &returnOrigin);
          TA_LOGH("open session. result=0x%x, origin=0x%x, session=0x%p\n",
              res, returnOrigin, DRM_MGR_TA_session);
          if (res != TEE_SUCCESS) {
              TA_LOGF("fail to open session. result=0x%08x, origin=%d, session=%p\n",
                                       res, returnOrigin, DRM_MGR_TA_session);
              goto ErrorExit;
          }
#ifndef CONFIG_OPTEE
          res = TEE_AllocateSharedMemory2(sizeof(TEE_UUID), &ShmIdUUID, &bUUIDHdlVa);
          if (res != TEE_SUCCESS)
          {
              TA_LOGF( "ret=0x%08x\n", res);
              goto ErrorExit;
          }
          res = TEE_AddSharedMemoryAccess(ShmIdUUID, uuid, TEE_SHM_PEER_READWRITE | TEE_SHM_PEER_GRANT);
          if (res != TEE_SUCCESS) {
              TA_LOGF( "TEE_AddSharedMemoryAccess fail, ret = 0x%x\n", res);
              goto ErrorExit;
          }
#else
          bUUIDHdlVa = TEE_Malloc(sizeof(TEE_UUID), 0);
          if (bUUIDHdlVa == NULL) {
              TA_LOGF("TEE_Malloc fail, ret = 0x%x\n", TEE_ERROR_OUT_OF_MEMORY);
              res = TEE_ERROR_OUT_OF_MEMORY;
              goto ErrorExit;
          }
#endif
      }
      return TEE_SUCCESS;
ErrorExit:
#ifndef CONFIG_OPTEE
      if(bUUIDHdlVa != NULL) {
          TEE_UnmapSharedMemory(ShmIdUUID);
          bUUIDHdlVa = NULL;
      }
#else
      if(bUUIDHdlVa != NULL) {
          TEE_Free(bUUIDHdlVa);
          bUUIDHdlVa = NULL;
      }
#endif
      if(DRM_MGR_TA_session != NULL) {
          TEE_CloseTASession(DRM_MGR_TA_session);
          DRM_MGR_TA_session = NULL;
      }
      return res;
}

void Drm_KeyMgr_TA_session_close(void)
{
    if (DRM_MGR_TA_session != NULL) {
        TEE_CloseTASession(DRM_MGR_TA_session);
        DRM_MGR_TA_session = NULL;

#ifndef CONFIG_OPTEE
        if(bUUIDHdlVa != NULL) {
            TEE_UnmapSharedMemory(ShmIdUUID);
            bUUIDHdlVa = NULL;
        }
#else
        if(bUUIDHdlVa != NULL) {
            TEE_Free(bUUIDHdlVa);
            bUUIDHdlVa = NULL;
        }
#endif
    }
}

TEE_Result Drm_SE_GetContentKeyInfo(M2M_DRM_CRYPTO_ST   *pCryptInfo,
                                           DMX_KEY_CTX *pKeyContext)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t        cancellationTimeout = 0;
    uint32_t        core_paramTypes;
    TEE_Param       core_params[4] = {{.value.a = 0, .value.b = 0}};;
    uint32_t        returnOrigin;
    const TEE_UUID uuid = DRM_SE_UUID;

    void *bKeyIDHdlVa = NULL;
    void *contentKeyUnitAttrHdlVa = NULL;
    void *contentKeyIVHdlVa = NULL;
    uint32_t ShmIdKeyID;
    uint32_t ShmIdContentKeyUnitAttr;
    uint32_t ShmIdContentKeyIV;

    MV_DRMContentKeyUnitAttribute KeyUnitAttr;

    if(pCryptInfo->uiKeyIdLen) {
#ifndef CONFIG_OPTEE
        res = TEE_AllocateSharedMemory2(pCryptInfo->uiKeyIdLen, &ShmIdKeyID, &bKeyIDHdlVa);
        if (res != TEE_SUCCESS)
        {
            TA_LOGF( "ret=0x%08x\n", res);
            goto fun_ret;
        }
        res = TEE_AddSharedMemoryAccess(ShmIdKeyID, uuid, TEE_SHM_PEER_READWRITE | TEE_SHM_PEER_GRANT);
        if (res != TEE_SUCCESS) {
            TA_LOGF( "TEE_AddSharedMemoryAccess fail, ret = 0x%x\n", res);
            goto fun_ret;
        }
#else
        bKeyIDHdlVa = TEE_Malloc(pCryptInfo->uiKeyIdLen, 0);
        if (bKeyIDHdlVa == NULL) {
            TA_LOGF("TEE_Malloc fail, ret = 0x%x\n", TEE_ERROR_OUT_OF_MEMORY);
            res = TEE_ERROR_OUT_OF_MEMORY;
            goto fun_ret;
        }
#endif
        memcpy(bKeyIDHdlVa, pCryptInfo->uiKeyID, pCryptInfo->uiKeyIdLen);
    }

#ifndef CONFIG_OPTEE
    res = TEE_AllocateSharedMemory2(sizeof(MV_DRMContentKeyUnitAttribute), &ShmIdContentKeyUnitAttr,
                                    &contentKeyUnitAttrHdlVa);
    if (res != TEE_SUCCESS)
    {
        TA_LOGF( "ret=0x%08x\n", res);
        goto fun_ret;
    }
    res = TEE_AddSharedMemoryAccess(ShmIdContentKeyUnitAttr, uuid, TEE_SHM_PEER_READWRITE | TEE_SHM_PEER_GRANT);
    if (res != TEE_SUCCESS) {
        TA_LOGF( "TEE_AddSharedMemoryAccess fail, ret = 0x%08x\n", res);
        goto fun_ret;
    }

    res = TEE_AllocateSharedMemory2(48, &ShmIdContentKeyIV,
                                    &contentKeyIVHdlVa);
    if (res != TEE_SUCCESS)
    {
        TA_LOGF( "ret=0x%08x\n", res);
        goto fun_ret;
    }
    res = TEE_AddSharedMemoryAccess(ShmIdContentKeyIV, uuid, TEE_SHM_PEER_READWRITE | TEE_SHM_PEER_GRANT);
    if (res != TEE_SUCCESS) {
        TA_LOGF( "TEE retrieve SHM ID fail, ret = 0x%x\n", res);
        goto fun_ret;
    }

    core_paramTypes = TEE_PARAM_TYPES( TEE_PARAM_TYPE_VALUE_INPUT,
                                       TEE_PARAM_TYPE_VALUE_INPUT,
                                       TEE_PARAM_TYPE_VALUE_INPUT,
                                       TEE_PARAM_TYPE_VALUE_INPUT);

    core_params[0].value.a = pCryptInfo->uiSchemeType;
    core_params[0].value.b = pCryptInfo->uiSessionID;
    core_params[1].value.a = (uint32_t) ShmIdKeyID;
    core_params[1].value.b = pCryptInfo->uiKeyIdLen;
    core_params[2].value.a = (uint32_t) ShmIdContentKeyUnitAttr;
    core_params[2].value.b = sizeof(MV_DRMContentKeyUnitAttribute);
    core_params[3].value.a = (uint32_t) ShmIdContentKeyIV;
    core_params[3].value.b = 48;
#else
    contentKeyUnitAttrHdlVa = TEE_Malloc(sizeof(MV_DRMContentKeyUnitAttribute), 0);
    if (contentKeyUnitAttrHdlVa == NULL) {
        TA_LOGF("TEE_Malloc fail, ret = 0x%x\n", TEE_ERROR_OUT_OF_MEMORY);
        res = TEE_ERROR_OUT_OF_MEMORY;
        goto fun_ret;
    }
    contentKeyIVHdlVa = TEE_Malloc(48, 0);
    if (contentKeyIVHdlVa == NULL) {
        TA_LOGF("TEE_Malloc fail, ret = 0x%x\n", TEE_ERROR_OUT_OF_MEMORY);
        res = TEE_ERROR_OUT_OF_MEMORY;
        goto fun_ret;
    }
    core_paramTypes = TEE_PARAM_TYPES( TEE_PARAM_TYPE_VALUE_INPUT,
                                       TEE_PARAM_TYPE_MEMREF_INPUT,
                                       TEE_PARAM_TYPE_MEMREF_INOUT,
                                       TEE_PARAM_TYPE_MEMREF_INOUT);
    core_params[0].value.a = pCryptInfo->uiSchemeType;
    core_params[0].value.b = pCryptInfo->uiSessionID;
    core_params[1].memref.buffer = bKeyIDHdlVa;
    core_params[1].memref.size = pCryptInfo->uiKeyIdLen;
    core_params[2].memref.buffer = contentKeyUnitAttrHdlVa;
    core_params[2].memref.size = sizeof(MV_DRMContentKeyUnitAttribute);
    core_params[3].memref.buffer = contentKeyIVHdlVa;
    core_params[3].memref.size = 48;
#endif

    res = TEE_InvokeTACommand(DRM_MGR_TA_session, cancellationTimeout, DRM_SE_KEYMGR_CONTENTKEYUNIT_GETINFO,
          core_paramTypes, core_params,&returnOrigin);
    if (res != TEE_SUCCESS) {
            TA_LOGF("fail to setscheme. result=0x%08x, origin=%d, session=%p\n",
                                     res, returnOrigin, DRM_MGR_TA_session);
            res = TEE_ERROR_NOT_SUPPORTED;
            goto fun_ret;
    }
    memcpy(&KeyUnitAttr, contentKeyUnitAttrHdlVa, sizeof(MV_DRMContentKeyUnitAttribute));


    pKeyContext->KeyLength   = KeyUnitAttr.uKeyDataLen;
    pKeyContext->IVKeyLen    = KeyUnitAttr.uIVLen;

    if(KeyUnitAttr.uHWKeyIndex != DRM_HWKEYINDEX_INVALID) {
        pKeyContext->KeyOption = DMX_KEY_TBLIDX;
        pKeyContext->KeyTblIndex = KeyUnitAttr.uHWKeyIndex;

        switch(KeyUnitAttr.uHWKeyArea)
        {
            case HW_KEYAREA_NAGRA:
            case HW_KEYAREA_NSK2:
            case HW_KEYAREA_IFCP_BRD:
            case HW_KEYAREA_IFCP_OTT:
            case HW_KEYAREA_IFCP_PVR:
            case HW_KEYAREA_GKL:
                pKeyContext->HWKeyArea = KeyUnitAttr.uHWKeyArea;
                break;
            default:
                pKeyContext->HWKeyArea = HW_KEYAREA_GENERAL;
                break;
        }
    } else {
        pKeyContext->KeyOption = DMX_KEY_CONTENT;
        memcpy(pKeyContext->KeyContent,contentKeyIVHdlVa, KeyUnitAttr.uKeyDataLen);
    }

    if(KeyUnitAttr.uHWIVIndex != DRM_HWKEYINDEX_INVALID) {
        pKeyContext->IVOption = DMX_KEY_TBLIDX;
        pKeyContext->IVTblIndex = KeyUnitAttr.uHWIVIndex;
    } else {
        pKeyContext->IVOption = DMX_KEY_CONTENT;
        memcpy(pKeyContext->IVData, (char*)contentKeyIVHdlVa+KeyUnitAttr.uKeyDataLen, KeyUnitAttr.uIVLen);
    }

fun_ret:
#ifndef CONFIG_OPTEE
    if(bKeyIDHdlVa != NULL) {
        TEE_UnmapSharedMemory(ShmIdKeyID);
    }
    if(contentKeyUnitAttrHdlVa != NULL) {
        TEE_UnmapSharedMemory(ShmIdContentKeyUnitAttr);
    }
    if(contentKeyIVHdlVa != NULL) {
        TEE_UnmapSharedMemory(ShmIdContentKeyIV);
    }
#else
    if(bKeyIDHdlVa != NULL) {
        TEE_Free(bKeyIDHdlVa);
        bKeyIDHdlVa = NULL;
    }
    if(contentKeyUnitAttrHdlVa != NULL) {
        TEE_Free(contentKeyUnitAttrHdlVa);
        contentKeyUnitAttrHdlVa = NULL;
    }
    if(contentKeyIVHdlVa != NULL) {
        TEE_Free(contentKeyIVHdlVa);
        contentKeyIVHdlVa = NULL;
    }
#endif
    return res;
}

TEE_Result Drm_SE_KeyMgr_GetContentKeyInfo(M2M_DRM_CRYPTO_ST   *pCryptInfo,
                                           DMX_KEY_CTX *pKeyContext)
{
    TEE_Result res = TEE_SUCCESS;
    res = Drm_KeyMgr_TA_session_open();
    if (res != TEE_SUCCESS) {
       TA_LOGF("fail to DRM_MGR_TA_session_open. result=0x%08x\n", res);
    }
    res = Drm_SE_GetContentKeyInfo(pCryptInfo, pKeyContext);
    if (res != TEE_SUCCESS) {
       TA_LOGF("fail to DRM_SE_GetContentKeyInfo. result=0x%08x\n", res);
    }
    //TEE_CloseTASession(DRM_MGR_TA_session);
    //DRM_MGR_TA_session = NULL;
    return res;
}

TEE_Result Drm_SE_GetKeyRight(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,
                                  uint32_t* pbProtected, uint32_t* puKeyUsage)
{
    HRESULT result = TEE_SUCCESS;
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t core_paramTypes;
    TEE_Param core_params[4] = {{.value.a = 0, .value.b = 0}};
    uint32_t returnOrigin;

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                      TEE_PARAM_TYPE_VALUE_OUTPUT,
                                      TEE_PARAM_TYPE_VALUE_OUTPUT,
                                      TEE_PARAM_TYPE_NONE);
    core_params[0].value.a = HWKeyArea;
    core_params[0].value.b = f_keyIndexOffset;
    res = TEE_InvokeTACommand(DRM_MGR_TA_session,
                              cancellationTimeout,
                              DRM_SE_KEYTABLE_ENTRY_ATTRIBUTE_GET,
                              core_paramTypes,
                              core_params,
                              &returnOrigin);
    if (res != TEE_SUCCESS)
    {
        TA_LOGF( "error invoking DRM_SE_KEYRIGHT_CONTENTPROTECT_GET."  \
                "result=0x%08x, origin=%d, session=%p "                        \
                "with HWKeyArea 0x%x, f_keyIndexOffset 0x%x\n",                \
                res, returnOrigin, DRM_MGR_TA_session, HWKeyArea, f_keyIndexOffset);
        result = res;
    }else {
        *pbProtected = core_params[1].value.a;
        switch(core_params[1].value.b)
        {
            case MV_KEYTABLE_KEY_USAGE_DEC:
                *puKeyUsage = M2MCRYPTO_KEY_USAGE_DEC;
                break;
            case MV_KEYTABLE_KEY_USAGE_ENC:
                *puKeyUsage = M2MCRYPTO_KEY_USAGE_ENC;
                break;
            case MV_KEYTABLE_KEY_USAGE_DEC_ENC:
                *puKeyUsage = M2MCRYPTO_KEY_USAGE_DEC | M2MCRYPTO_KEY_USAGE_ENC;
                break;
            default:
                TA_LOGF("abnormal key usage 0x%x\n", core_params[2].value.b);
                *puKeyUsage = 0;
                break;
        }
    }
    return result;
}


TEE_Result Drm_SE_KtMgr_GetKeyRight(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,
                                               uint32_t* pbProtected, uint32_t* puKeyUsage)
{
    HRESULT result = TEE_SUCCESS;

    result = Drm_KeyMgr_TA_session_open();
    if (result == TEE_SUCCESS)
    {
        result = Drm_SE_GetKeyRight(HWKeyArea, f_keyIndexOffset, pbProtected, puKeyUsage);
    }
    //TEE_CloseTASession(DRM_MGR_TA_session);
    //DRM_MGR_TA_session = NULL;
    return result;
}

TEE_Result Drm_SE_Validate_KeyUser(UINT32 HWKeyArea, UINT32 f_TSPKeyIndexOffset,  \
                                   TEE_UUID  *pUUID,  uint32_t* bValidKeyUser)
{
    HRESULT result = TEE_SUCCESS;
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t core_paramTypes;
    TEE_Param core_params[4] = {{.value.a = 0, .value.b = 0}};
    uint32_t returnOrigin;

    if(pUUID) {
        memcpy(bUUIDHdlVa, pUUID, sizeof(TEE_UUID));
    }

    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
#ifndef CONFIG_OPTEE
                                      TEE_PARAM_TYPE_VALUE_INPUT,
#else
                                      TEE_PARAM_TYPE_MEMREF_INPUT,
#endif
                                      TEE_PARAM_TYPE_VALUE_OUTPUT,
                                      TEE_PARAM_TYPE_NONE);

    core_params[0].value.a = HWKeyArea;
    core_params[0].value.b = f_TSPKeyIndexOffset;
#ifndef CONFIG_OPTEE
    core_params[1].value.a = ShmIdUUID;
    core_params[1].value.b = sizeof(TEE_UUID);
#else
    core_params[1].memref.buffer = bUUIDHdlVa;
    core_params[1].memref.size = sizeof(TEE_UUID);
#endif

    res = TEE_InvokeTACommand(DRM_MGR_TA_session,
                              cancellationTimeout,
                              DRM_SE_KEYRIGHT_VALIDATE_KEYUSER,
                              core_paramTypes,
                              core_params,
                              &returnOrigin);
    if (res != TEE_SUCCESS)
    {
        TA_LOGF( "error invoking DRM_SE_KEYRIGHT_VALIDATE_KEYUSER."  \
                "result=0x%08x, origin=%d, session=%p "                        \
                "with HWKeyArea 0x%x, f_keyIndexOffset 0x%x\n",                \
                res, returnOrigin, DRM_MGR_TA_session, HWKeyArea, f_TSPKeyIndexOffset);
        result = res;
    }
    *bValidKeyUser = core_params[2].value.a;

    return result;
}

TEE_Result Drm_SE_KtMgr_Validate_KeyUser(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,   \
                                         TEE_UUID  *pUUID,  uint32_t* bValidKeyUser)
{
    HRESULT result = TEE_SUCCESS;

    result = Drm_KeyMgr_TA_session_open();
    if (result == TEE_SUCCESS)
    {
        result = Drm_SE_Validate_KeyUser(HWKeyArea, f_keyIndexOffset, pUUID, bValidKeyUser);
    }
    //TEE_CloseTASession(DRM_MGR_TA_session);
    //DRM_MGR_TA_session = NULL;
    return result;
}

#else
TEE_Result Drm_KeyMgr_TA_session_open(void)
{
      TEE_Result res = TEE_SUCCESS;
      return res;
}

void Drm_KeyMgr_TA_session_close(void)
{
}

TEE_Result Drm_SE_GetContentKeyInfo(M2M_DRM_CRYPTO_ST   *pCryptInfo,
                                           DMX_KEY_CTX *pKeyContext)
{
    TEE_Result res = TEE_SUCCESS;
    return res;
}


TEE_Result Drm_SE_KeyMgr_GetContentKeyInfo(M2M_DRM_CRYPTO_ST   *pCryptInfo,
                                           DMX_KEY_CTX *pKeyContext)
{
    TEE_Result res = TEE_SUCCESS;
    return res;
}

TEE_Result Drm_SE_GetKeyRight(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,
                                      uint32_t* pbProtected, uint32_t* puKeyUsage)
{
    TEE_Result res = TEE_SUCCESS;
    return res;
}


TEE_Result Drm_SE_KtMgr_GetKeyRight(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,
                                               uint32_t* pbProtected, uint32_t* puKeyUsage)
{
    TEE_Result res = TEE_SUCCESS;
    return res;
}

TEE_Result Drm_SE_Validate_KeyUser(UINT32 HWKeyArea, UINT32 f_TSPKeyIndexOffset,  \
                                   TEE_UUID  *pUUID,  uint32_t* bValidKeyUser)
{
    TEE_Result res = TEE_SUCCESS;
    return res;
}

TEE_Result Drm_SE_KtMgr_Validate_KeyUser(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,   \
                                         TEE_UUID  *pUUID,  uint32_t* bValidKeyUser)
{
    TEE_Result res = TEE_SUCCESS;
    return res;
}


#endif
#endif

HRESULT M2M_TA_Dtcm_Mem_Alloc(UINT16 mem_size, UINT8 align_size, UINT8 figoid, DTCM_MEM_TYPE type)
{
   UINT16 mem_start = 0;

   #ifdef TA_COMPILE
       TEE_Result res = TEE_SUCCESS;

       res = TspTA_session_open(figoid);
       if (res != TEE_SUCCESS) {
          TA_LOGF("fail to TspTA_session_open. result=0x%08x\n", res);
          return 0xFFFF;
       }

       res = TspTA_Dtcm_Alloc(&mem_start, mem_size, align_size, figoid, type);
       if (res != TEE_SUCCESS) {
          TA_LOGF("fail to M2M_TA_Dtcm_Mem_Alloc. result=0x%08x\n", res);
          return 0xFFFF;
       }
   #else
       mem_start = dtcm_mem_alloc(current_fw(figoid)->hDtcmHeap, mem_size, align_size);
       TA_LOGH("NonTZ 0x%x\n", mem_start);
   #endif
   TA_LOGH("M2M_TA_Dtcm_Mem_Alloc successfully 0x%x\n", mem_start);
   return mem_start;
}

HRESULT M2M_TA_Dtcm_Mem_Free(UINT16 mem_start, UINT8 figoid, DTCM_MEM_TYPE type)
{
     TEE_Result res = TEE_SUCCESS;
   #ifdef TA_COMPILE
     res = TspTA_session_open(figoid);
     if (res != TEE_SUCCESS) {
        TA_LOGF("fail to TspTA_session_open. result=0x%08x\n", res);
        return res;
     }

     res = TspTA_Dtcm_Free(mem_start, figoid, type);
     if (res != TEE_SUCCESS) {
        TA_LOGF("fail to TspTA_Dtcm_Free. result=0x%08x\n", res);
        return res;
     }
     TEE_CloseTASession(TSP_TA_session[figoid]);
     TSP_TA_session[figoid] = NULL;
   #else
    dtcm_mem_free(current_fw(figoid)->hDtcmHeap, mem_start);
    TA_LOGH("NonTZ 0x%x\n", mem_start);
   #endif
    TA_LOGH("M2M_TA_Dtcm_Mem_Free successfully 0x%x\n", mem_start);
    return res;
}


INT M2M_TA_AllocTask(FIGODRV_HANDLE  hFigoDrv, UINT16 task_addr)
{
    #ifdef TA_COMPILE
       TEE_Result res = TEE_SUCCESS;
       INT task_id = -1;

       res = TspTA_session_open(hFigoDrv->uFigoSysId);
       if (res != TEE_SUCCESS) {
          TA_LOGF("fail to TspTA_session_open. result=0x%08x\n", res);
          return 0xFFFF;
       }

       res = TspTA_Task_Alloc(hFigoDrv->uFigoSysId, task_addr, &task_id);
       if (res != TEE_SUCCESS) {
          TA_LOGF("fail to TspTA_Task_Alloc. result=0x%08x\n", res);
       }
       return task_id;
    #else
    INT i;
    UINT16 * pTaskItem = (UINT16 *)MV_FIGOLIB_MAP2DRAMPTR(hFigoDrv, RA_TspDtcmGlobal_SCHED);

    if(pTaskItem)
    {
        for (i =0;i<(2*arr_TspDtcmGlobal_TaskList); i++){
            if (pTaskItem[i] == INVALID_DTCM_ADDR) {
                pTaskItem[i] = task_addr;
                MV_FIGODRV_DDR2BIU32(hFigoDrv,
                                     (UNSG8 *)DDR2BIU_ALIGN4(&pTaskItem[i]));
                return i;
            }
        }
    }
    return -1;
    #endif
}

void M2M_TA_FreeTask(FIGODRV_HANDLE  hFigoDrv, INT task_id)
{
    #ifdef TA_COMPILE
     TEE_Result res = TEE_SUCCESS;
     res = TspTA_session_open(hFigoDrv->uFigoSysId);
     if (res != TEE_SUCCESS) {
        TA_LOGF("fail to TspTA_session_open. result=0x%08x\n", res);
     }

     res = TspTA_Task_Free(hFigoDrv->uFigoSysId, task_id);
     if (res != TEE_SUCCESS) {
        TA_LOGF("fail to TspTA_Task_Free. result=0x%08x\n", res);
     }
     TEE_CloseTASession(TSP_TA_session[hFigoDrv->uFigoSysId]);
     TSP_TA_session[hFigoDrv->uFigoSysId] = NULL;
   #else
    UINT16 * pTaskItem = (UINT16 *)MV_FIGOLIB_MAP2DRAMPTR(hFigoDrv, RA_TspDtcmGlobal_SCHED);

    if(pTaskItem)
    {
        if (task_id < (2*arr_TspDtcmGlobal_TaskList))
        {
            pTaskItem[task_id] = INVALID_DTCM_ADDR;
            MV_FIGODRV_DDR2BIU32(hFigoDrv,
                                    (UNSG8 *)DDR2BIU_ALIGN4(&pTaskItem[task_id]));
        }
    }
    #endif
}

SIGN32 MV_FIGODRV_DDR2BIU(FIGODRV_HANDLE hFigoDrv, UNSG8* pbSrc, UNSG32 uSize)
{
    UNSG32 i = 0;
    if ( !MV_FIGODRV_IS_VALID_HANDLE(hFigoDrv) ||
        !MV_FIGODRV_IS_VALID_BIUPTR(hFigoDrv, pbSrc, uSize))
    {
        TA_LOGF("Invalid argument\n");
        return TEE_ERROR_BAD_PARAMETERS;
    }
    for( i = 0; i < uSize ; i += sizeof(UNSG32))
    {
        MV_FIGODRV_DDR2BIU32(hFigoDrv, &pbSrc[i]);
    }
    return TEE_SUCCESS;
}


SIGN32 MV_FIGODRV_BIU2DDR(FIGODRV_HANDLE hFigoDrv, UNSG8* pbSrc, UNSG32 uSize)
{
    UNSG32 i = 0;
    if ( !MV_FIGODRV_IS_VALID_HANDLE(hFigoDrv) ||
        !MV_FIGODRV_IS_VALID_BIUPTR(hFigoDrv, pbSrc, uSize))
    {
        TA_LOGF("Invalid argument\n");
        return TEE_ERROR_BAD_PARAMETERS;
    }
    for( i = 0; i < uSize ; i += sizeof(UNSG32))
    {
        MV_FIGODRV_BIU2DDR32(hFigoDrv, &pbSrc[i]);
    }
    return TEE_SUCCESS;
}


HRESULT M2M_TA_LoadDataToDTCM(FIGODRV_HANDLE  hFigoDrv, UINT16 pDtcm,
            UINT8 *pData, UINT32 uLen)
{
    HRESULT ret = SUCCESS;
    UINT8 *ptr = NULL;

    ptr = (UINT8 *)MV_FIGOLIB_MAP2DRAMPTR(hFigoDrv, pDtcm);
    if(ptr != NULL) {
      memcpy(ptr, pData, uLen);
      MV_FIGODRV_DDR2BIU(hFigoDrv, ptr, uLen);
    }

    return ret;
}


UINT32 TSPFW_Crypto_CheckQSpace(FIGODRV_HANDLE  hFigoDrv)
{
    // "cast increases required alignment of target type". From UINT8* to (SIE_TspDtcmGlobal*). So first convert to void*
    // and then cast to target type
	SIE_TspDtcmGlobal* pTspDtcmGlobal = (SIE_TspDtcmGlobal*)(void*)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);
	SIE_M2MCryptoCtx *pM2MCryptoCtx = &pTspDtcmGlobal->ie_M2MCryptoCtx;
    SIE_DSFifo32 *pDDRCmdQueue = &pM2MCryptoCtx->ie_DDRCmdQueue;

    MV_FIGODRV_BIU2DDR32(hFigoDrv, (uintptr_t)pDDRCmdQueue + BA_DSFifo32_nRdOff);
    return MV_FIGOLIB_GET_FIFO_SPACE(pDDRCmdQueue);
}

UINT32 TSPFW_Crypto_GetQCmdNum(FIGODRV_HANDLE  hFigoDrv)
{
    // "cast increases required alignment of target type". From UINT8* to (SIE_TspDtcmGlobal*). So first convert to void*
    // and then cast to target type
	SIE_TspDtcmGlobal *pTspDtcmGlobal = (SIE_TspDtcmGlobal *)(void*)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);
	SIE_M2MCryptoCtx *pM2MCryptoCtx = &pTspDtcmGlobal->ie_M2MCryptoCtx;
    SIE_DSFifo32 *pDDRCmdQueue = &pM2MCryptoCtx->ie_DDRCmdQueue;

    MV_FIGODRV_BIU2DDR32(hFigoDrv, (uintptr_t)pDDRCmdQueue + BA_DSFifo32_nRdOff);
    return MV_FIGOLIB_GET_FIFO_BYTES(pDDRCmdQueue)/sizeof(SIE_M2MCryptoCmd);
}

UINT32 TSPFW_Crypto_GetCmdQRdOffset(FIGODRV_HANDLE  hFigoDrv)
{
    // "cast increases required alignment of target type". From UINT8* to (SIE_TspDtcmGlobal*). So first convert to void*
    // and then cast to target type
	SIE_TspDtcmGlobal *pTspDtcmGlobal = (SIE_TspDtcmGlobal *)(void*)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);
	SIE_M2MCryptoCtx *pM2MCryptoCtx = &pTspDtcmGlobal->ie_M2MCryptoCtx;
    SIE_DSFifo32 *pDDRCmdQueue = &pM2MCryptoCtx->ie_DDRCmdQueue;

    MV_FIGODRV_BIU2DDR32(hFigoDrv, (uintptr_t)pDDRCmdQueue + BA_DSFifo32_nRdOff);
    return pDDRCmdQueue->u_nRdOff;
}

HRESULT TSPFW_Crypto_SetCmdQWrOffset(FIGODRV_HANDLE  hFigoDrv, UINT32 uOffSet)
{
    // "cast increases required alignment of target type". From UINT8* to (SIE_TspDtcmGlobal*). So first convert to void*
    // and then cast to target type
	SIE_TspDtcmGlobal *pTspDtcmGlobal = (SIE_TspDtcmGlobal *)(void*)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);
	SIE_M2MCryptoCtx *pM2MCryptoCtx = &pTspDtcmGlobal->ie_M2MCryptoCtx;
    SIE_DSFifo32 *pDDRCmdQueue = &pM2MCryptoCtx->ie_DDRCmdQueue;

    if(uOffSet >= pDDRCmdQueue->u_nMaxSize) {
        TA_LOGE("UpdateCmdQWr() with error offset(0x%x)\n", uOffSet);
    }
    pDDRCmdQueue->u_nWrOff = uOffSet;
    MV_FIGODRV_DDR2BIU32(hFigoDrv, (uintptr_t)pDDRCmdQueue + BA_DSFifo32_nWrOff);
    return SUCCESS;
}



HRESULT TSPFW_Crypto_SendCmd(FIGODRV_HANDLE  hFigoDrv,
                SIE_M2MCryptoCmd *pCryptoCmd)
{
    HRESULT ret = SUCCESS;
    // "cast increases required alignment of target type". From UINT8* to (SIE_TspDtcmGlobal*). So first convert to void*
    // and then cast to target type
    SIE_TspDtcmGlobal* pTspDtcmGlobal = (SIE_TspDtcmGlobal*)(void*)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);
    SIE_M2MCryptoCtx *pM2MCryptoCtx = &pTspDtcmGlobal->ie_M2MCryptoCtx;
    SIE_DTCMFifo *uCmdQueue = &pM2MCryptoCtx->ie_CmdQueue;
    UINT8 *pData = NULL;
    UINT32 i = 0;
    static UINT32 uIndex = 0;

    /*re-calc source/dest address*/
    pCryptoCmd->u_source_address = pM2MCryptoCtx->u_pLocalDataIn + uIndex*pM2MCryptoCtx->u_MaxBlockSize;
    MV_FIGOC_ALIGN_PTR(pCryptoCmd->u_source_address, pCryptoCmd->u_PhyInputAddr);
    pCryptoCmd->u_desination_address = pM2MCryptoCtx->u_pLocalDataOut + uIndex*pM2MCryptoCtx->u_MaxBlockSize;
    MV_FIGOC_ALIGN_PTR(pCryptoCmd->u_desination_address, pCryptoCmd->u_PhyOutputAddr);

    uIndex++;
    if(uIndex == pM2MCryptoCtx->u_MaxCmdNum) {
        uIndex = 0;
    }
    if(MV_FIGOLIB_GET_FIFO_SPACE(uCmdQueue)/sizeof(SIE_M2MCryptoCmd)>=1) {
        pData = (UINT8 *)pCryptoCmd;
        for (i=0; i<sizeof(SIE_M2MCryptoCmd); i+=4) {
            MV_FIGODRV_BIUWR32(hFigoDrv, MV_FIGOLIB_GET_FIFO_WRPTR(uCmdQueue),
                        *(UINT32*)(void*)&pData[i]);
            MV_FIGOLIB_FIFO_PUSH(uCmdQueue, 4);
         }
    }
    MV_FIGODRV_DDR2BIU32(hFigoDrv, (uintptr_t)uCmdQueue + BA_DTCMFifo_nWrOff);
    TA_LOGI("Pushed CMD[u_cmd_id=0x%x, u_Scheme=0x%x, u_parameter=0x%x, " \
        "u_PhyInputAddr=0x%08x, u_PhyOutputAddr=0x%08x, u_source_address=0x%x, "
        "u_desination_address=0x%x, u_input_size=0x%x, u_key_address=0x%x, "
        "u_key_address_2=0x%x, u_iv_address=0x%x, u_write_back_iv=0x%x], "
        "u_crypto_type[VS6XX only][0x%x]\n",
        pCryptoCmd->u_cmd_id, pCryptoCmd->u_Scheme, pCryptoCmd->u_parameter,
        pCryptoCmd->u_PhyInputAddr, pCryptoCmd->u_PhyOutputAddr, pCryptoCmd->u_source_address,
        pCryptoCmd->u_desination_address, pCryptoCmd->u_input_size, pCryptoCmd->u_key_address,
        pCryptoCmd->u_key_address_2, pCryptoCmd->u_iv_address, pCryptoCmd->u_write_back_iv,
        pCryptoCmd->u_crypto_type
        );

    return ret;
}

SIE_M2MCryptoCmd *TSPFW_Crypto_GetCmd(FIGODRV_HANDLE  hFigoDrv, UINT32 Index)
{
    // "cast increases required alignment of target type". From UINT8* to (SIE_TspDtcmGlobal*). So first convert to void*
    // and then cast to target type
    SIE_TspDtcmGlobal* pTspDtcmGlobal = (SIE_TspDtcmGlobal*)(void*)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);
    SIE_M2MCryptoCtx *pM2MCryptoCtx = &pTspDtcmGlobal->ie_M2MCryptoCtx;
    SIE_DTCMFifo *uCmdQueue = &pM2MCryptoCtx->ie_CmdQueue;
    UINT8 *pRet = NULL;

    if(Index >= TSP_CRYPTO_CMDQ_SIZE) {
        TA_LOGE("Param error Index=0x%x\n", Index);
        return NULL;
    }

    //MV_FIGODRV_BIU2DDR32(hFigoDrv, (UINT32)uCmdQueue + BA_DTCMFifo_nRdOff);
    TA_LOGI("MV_FIGOLIB_GET_FIFO_RDPTR(uCmdQueue) = %p\n", MV_FIGOLIB_GET_FIFO_RDPTR(uCmdQueue));
    pRet = (UINT8 *)(uintptr_t)MV_FIGOLIB_MAP2DRAMPTR(hFigoDrv, (uintptr_t)MV_FIGOLIB_GET_FIFO_RDPTR(uCmdQueue));
    pRet = pRet + sizeof(SIE_M2MCryptoCmd)*Index;
    MV_FIGODRV_BIU2DDR(hFigoDrv, pRet, sizeof(SIE_M2MCryptoCmd));


    TA_LOGI("pRet = %p\n", pRet);
    return (SIE_M2MCryptoCmd *)(void*)pRet;
}

HRESULT TSPFW_Crypto_CheckCmdErrState(FIGODRV_HANDLE  hFigoDrv)
{
    // "cast increases required alignment of target type". From UINT8* to (SIE_TspDtcmGlobal*). So first convert to void*
    // and then cast to target type
    SIE_TspDtcmGlobal *pTspDtcmGlobal = (SIE_TspDtcmGlobal *)(void*)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);
    SIE_M2MCryptoCtx *pM2MCryptoCtx = &pTspDtcmGlobal->ie_M2MCryptoCtx;
    UINT8 uLastErrCnt = pM2MCryptoCtx->u_ErrCryptoCnt;

    MV_FIGODRV_BIU2DDR32(hFigoDrv, (uintptr_t)pM2MCryptoCtx + BA_M2MCryptoCtx_CryptoState);

    if(pM2MCryptoCtx->u_ErrCryptoCnt != uLastErrCnt) {
        TA_LOGE("M2M crypto error cnt: [0x%x -> 0x%x]\n", uLastErrCnt,pM2MCryptoCtx->u_ErrCryptoCnt);
        return ERR_HWFAIL;
    }

    return SUCCESS;
}

HRESULT TSPFW_Crypto_Init(TSP_CRYPTO_LIB *pCryptoLib, int32_t uFigoId, UINT16 uCmdQueueBase, UINT16  uCmdQueueSize,
                           UINT16 uCmdCntBase, UINT16 uLocalDataIn, UINT16 uLocalDataOut, UINT16 uM2MCryptoCmdBase)
{
    HRESULT ret = SUCCESS;
    FIGODRV_HANDLE hFigoDrv = current_figohandle(uFigoId);
    //SIE_TspDtcmGlobal* pTspDtcmGlobal = NULL;
    SIE_M2MCryptoCtx *pM2mCryptoCtx = NULL;

    TspTA_session_open(uFigoId);
    //pTspDtcmGlobal = (SIE_TspDtcmGlobal *)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);

    TA_LOGH("[Crypto FW] Physical addr %p\n", hFigoDrv->pbBIUImagePhy);

    pM2mCryptoCtx =
        (SIE_M2MCryptoCtx *)MV_FIGOLIB_MAP2DRAMPTR(hFigoDrv, RA_TspDtcmGlobal_M2MCRYPTOCTX);
    if (!pM2mCryptoCtx) {
        ret = ERR_NULL_PTR;
        goto error;
    }
    TA_LOGH("[Crypto FW] %p, %p, %p\n", hFigoDrv, MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv), pM2mCryptoCtx);
    TA_LOGH("[Crypto FW] 0x%x, 0x%x, 0x%x, 0x%x\n", uCmdQueueBase, uCmdCntBase, uLocalDataIn, uLocalDataOut);

    /**
     *  prepare filter context
     */
    memset(pM2mCryptoCtx, 0, sizeof(SIE_M2MCryptoCtx));

    pM2mCryptoCtx->ie_TskHdr.u_TaskState = TaskHeader_TaskState_STOPPED/*TaskHeader_TaskState_RUNNING*/;
    pM2mCryptoCtx->ie_TskHdr.u_uSrcId = 0;

    pM2mCryptoCtx->ie_TskHdr.u_pFunc = tsp_filter_allinone_figo_MV_FIGO_TSP_M2M_Task;

    pM2mCryptoCtx->ie_TskHdr.u_TaskType = Crypto_type_Task;
    pM2mCryptoCtx->u_MaxBlockSize = TSP_CRYPTO_BLOCK_MAXSIZE;
    pM2mCryptoCtx->u_MaxCmdNum = TSP_CRYPTO_CMD_MAXNUM;
    /*Reuse filter local data as u_pLocalDataIn/u_pLocalDataOut*/
    pM2mCryptoCtx->u_pLocalDataIn = uLocalDataIn;
    pM2mCryptoCtx->u_pLocalDataOut = uLocalDataOut;
    TA_LOGH("[Crypto FW] u_pLocalDataIn = 0x%x\n", pM2mCryptoCtx->u_pLocalDataIn);
    TA_LOGH("[Crypto FW] u_pLocalDataOut = 0x%x\n", pM2mCryptoCtx->u_pLocalDataOut);

    pM2mCryptoCtx->ie_CmdQueue.u_nMaxSize = uCmdQueueSize;
    pM2mCryptoCtx->ie_CmdQueue.u_pbBase   = uCmdQueueBase;

    pM2mCryptoCtx->ie_CmdQueue.u_pbBase = ADDR_ALIGN(pM2mCryptoCtx->ie_CmdQueue.u_pbBase, 8);
    pM2mCryptoCtx->ie_CmdQueue.u_nWrOff = pM2mCryptoCtx->ie_CmdQueue.u_nRdOff = 0;
    TA_LOGH("[Crypto FW] ie_CmdQueue.u_pbBase = 0x%x\n", pM2mCryptoCtx->ie_CmdQueue.u_pbBase);

    pM2mCryptoCtx->u_uCmdCntBase = uCmdCntBase;
    TA_LOGH("[Crypto FW] u_uCmdCntBase = 0x%x\n", pM2mCryptoCtx->u_uCmdCntBase);

    pM2mCryptoCtx->ie_DDRCmdQueue.u_pbBase = (uintptr_t)pCryptoLib->CmdQ[uFigoId].pCmdBasePhyAddr;
    pM2mCryptoCtx->ie_DDRCmdQueue.u_nMaxSize = pCryptoLib->CmdQ[uFigoId].uSize;
    pM2mCryptoCtx->u_pLocalCryptoCmd = uM2MCryptoCmdBase;

    /**
     *  sync all context in pM2mCryptoCtx from shadow to dtcm
     */
    MV_FIGODRV_DDR2BIU(hFigoDrv, (UINT8*)pM2mCryptoCtx, sizeof(SIE_M2MCryptoCtx));

    TA_LOGH("M2mCryptoCtx init success\n");
error:
    return ret;
}

HRESULT TSPFW_Crypto_Deinit(TSP_CRYPTO_LIB *pCryptoLib, int32_t uFigoId)
{
    SIE_M2MCryptoCtx *pM2mCryptoCtx = NULL;
    FIGODRV_HANDLE hFigoDrv = current_figohandle(uFigoId);

    pM2mCryptoCtx =
        (SIE_M2MCryptoCtx *)MV_FIGOLIB_MAP2DRAMPTR(hFigoDrv, RA_TspDtcmGlobal_M2MCRYPTOCTX);
    if (!pM2mCryptoCtx) {
        return ERR_NULL_PTR;
    }

    M2M_TA_Dtcm_Mem_Free(pM2mCryptoCtx->u_uCmdCntBase, uFigoId, DTCM_MEM_TYPE_NONSECURE);
    M2M_TA_Dtcm_Mem_Free(pM2mCryptoCtx->u_pLocalCryptoCmd, uFigoId, DTCM_MEM_TYPE_SECURE);
    M2M_TA_Dtcm_Mem_Free(pM2mCryptoCtx->ie_CmdQueue.u_pbBase, uFigoId, DTCM_MEM_TYPE_SECURE);
    M2M_TA_Dtcm_Mem_Free(pM2mCryptoCtx->u_pLocalDataIn, uFigoId, DTCM_MEM_TYPE_NONSECURE);

    TA_LOGH("M2mCryptoCtx deinit success\n");
    return SUCCESS;
}


HRESULT TSPFW_Crypto_AllocFigoTask(TSP_CRYPTO_LIB *pCryptoLib, int32_t uFigoId)
{
    HRESULT ret = SUCCESS;
    FIGODRV_HANDLE hFigoDrv = current_figohandle(uFigoId);
    SIE_M2MCryptoCtx *pM2mCryptoCtx = (SIE_M2MCryptoCtx *)MV_FIGOLIB_MAP2DRAMPTR(hFigoDrv, RA_TspDtcmGlobal_M2MCRYPTOCTX);

    TA_LOGH("[Crypto FW] Physical addr %p\n", hFigoDrv->pbBIUImagePhy);
    if (!pM2mCryptoCtx) {
        ret = ERR_NULL_PTR;
        goto error;
    }

    pCryptoLib->Libcfg[uFigoId].uTaskId = M2M_TA_AllocTask(hFigoDrv, MV_FIGOLIB_MAP2BIUADDR(hFigoDrv, pM2mCryptoCtx));
    if (pCryptoLib->Libcfg[uFigoId].uTaskId < 0) {
        ret = ERR_NULL_PTR;
        goto error;
    }

    pM2mCryptoCtx->ie_TskHdr.u_TaskState = TaskHeader_TaskState_RUNNING;
    MV_FIGODRV_DDR2BIU(hFigoDrv, (UINT8*)pM2mCryptoCtx, sizeof(SIE_TaskHeader));

    TA_LOGH("Alloc figo task success\n");
error:
    return ret;
}

HRESULT TSPFW_Crypto_FreeFigoTask(TSP_CRYPTO_LIB *pCryptoLib, int32_t uFigoId)
{
    FIGODRV_HANDLE hFigoDrv = current_figohandle(uFigoId);
    SIE_M2MCryptoCtx *pM2mCryptoCtx = (SIE_M2MCryptoCtx *)MV_FIGOLIB_MAP2DRAMPTR(hFigoDrv, RA_TspDtcmGlobal_M2MCRYPTOCTX);

    pM2mCryptoCtx->ie_TskHdr.u_TaskState = TaskHeader_TaskState_STOPPING;
    MV_FIGODRV_DDR2BIU(hFigoDrv, (UINT8*)pM2mCryptoCtx, sizeof(SIE_TaskHeader));
    while(1){
        MV_FIGODRV_BIU2DDR(hFigoDrv, (UINT8*)pM2mCryptoCtx, sizeof(SIE_TaskHeader));
        if(pM2mCryptoCtx->ie_TskHdr.u_TaskState == TaskHeader_TaskState_STOPPED)
            break;
    }
    M2M_TA_FreeTask(hFigoDrv, pCryptoLib->Libcfg[uFigoId].uTaskId);
    TA_LOGH("Free figo task success\n");
    return SUCCESS;
}

#ifdef TA_COMPILE

static HRESULT TSPFW_Crypto_ResetFIGO(FIGODRV_HANDLE hFigoDrv, UNSG32 uRstValue)
{
#if defined(CONFIG_GENX_ENABLE)
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    HRESULT ret = SUCCESS;
    TEE_Result teeRet;

    TA_LOGM("Call BCM to reset FIGO[%u]\n", hFigoDrv->uFigoSysId);
    cmd.arg0 = hFigoDrv->uFigoSysId;
    cmd.arg1 = FIGO_RESET;
    teeRet = TEE_ExecCmd(BCM_PI_SET_TSP_STATE, &cmd, sizeof(BCM_MAILBOX_COMMAND),
                      &status, sizeof(BCM_MAILBOX_STATUS));
    if (teeRet != TEE_SUCCESS) {
        TA_LOGE("Error in sending BCM_PI_SET_TSP_STATE: 0x%x\n", ret);
        ret = ERR_ERRSYSCALL;
    }

    return ret;
#else
    T32FigoReg_figoRstn figoRstn;
    UNSG32 uAddr = 0;

    if (hFigoDrv->uFigoSysType)
    {
        uAddr = hFigoDrv->uFigoSysStart + RA_FigoSysBasic_FIGO0;
    }
    else
    {
        uAddr = hFigoDrv->uFigoSysStart + RA_FigoSys_FIGO0;
    }
    figoRstn.u32 = 0;
    figoRstn.ufigoRstn_up = uRstValue;

    MV_FIGODRV_BIUWR32(hFigoDrv, uAddr + RA_FigoReg_figoRstn, figoRstn.u32);

    return SUCCESS;
#endif
}

static HRESULT TSPFW_Crypto_ReleaseFIGO_FromPC(FIGODRV_HANDLE hFigoDrv,  UNSG32 pcStartLoc)
{
#if defined(CONFIG_GENX_ENABLE)
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;
    HRESULT ret = SUCCESS;
    TEE_Result teeRet;

    TA_LOGM("Call BCM to release FIGO[%u]\n", hFigoDrv->uFigoSysId);
    cmd.arg0 = hFigoDrv->uFigoSysId;
    cmd.arg1 = FIGO_RELEASE;
    teeRet = TEE_ExecCmd(BCM_PI_SET_TSP_STATE, &cmd, sizeof(BCM_MAILBOX_COMMAND),
                      &status, sizeof(BCM_MAILBOX_STATUS));
    if (teeRet != TEE_SUCCESS) {
        TA_LOGE("Error in sending BCM_PI_SET_TSP_STATE: 0x%x\n", ret);
        ret = ERR_ERRSYSCALL;
    }

    return ret;
#else
    T32FigoReg_figoCtrl figoCtrl;
    T32FigoReg_figoRstn figoRstn;
    UNSG32 uAddr = 0;

    if (hFigoDrv->uFigoSysType)
    {
        uAddr = hFigoDrv->uFigoSysStart + RA_FigoSysBasic_FIGO0;
    }
    else
    {
        uAddr = hFigoDrv->uFigoSysStart + RA_FigoSys_FIGO0;
    }
    figoCtrl.u32 = 0;
    figoCtrl.ufigoCtrl_pcStartLoc = pcStartLoc;

    MV_FIGODRV_BIUWR32(hFigoDrv, uAddr + RA_FigoReg_figoCtrl, figoCtrl.u32);

    figoRstn.u32 = 0;
    figoRstn.ufigoRstn_up = 1;

    MV_FIGODRV_BIUWR32(hFigoDrv, uAddr + RA_FigoReg_figoRstn, figoRstn.u32);
    return SUCCESS;
#endif
}



static HRESULT TSPFW_Crypto_SendStartStallCmd(FIGODRV_HANDLE hFigoDrv, UNSG32 uCmd)
{
    SIE_LocalArea_Ext* pLA = NULL;
    pLA = MV_FIGOLIB_GET_LAPTR(hFigoDrv);
    MV_FIGODRV_BIU2DDR32(hFigoDrv, ((uintptr_t)&pLA->ie_stallCmd + RA_StallCmd_CMD));
    if ( pLA->ie_stallCmd.uCMD_cmdTag == uCmd )
        return SUCCESS;
    pLA->ie_stallCmd.uCMD_cmdTag = uCmd;
    MV_FIGODRV_DDR2BIU32(hFigoDrv, ((uintptr_t)&pLA->ie_stallCmd + RA_StallCmd_CMD));
    while(1)
    {
        MV_FIGODRV_BIU2DDR32(hFigoDrv, (uintptr_t)&pLA->ie_stallCmd + RA_StallCmd_RSP);
        if ( pLA->ie_stallCmd.uRSP_rspTag == uCmd)
        {
            break;
        }
    }
    return SUCCESS;
}

HRESULT TSPFW_Crypto_StartFW(int32_t *pFigoId)
{
    TEE_Result res = TEE_SUCCESS;
    UNSG32 uPgIdx = 0, uNeedReload = 0;
    FIGODRV_HANDLE hFigoDrv;

    res = TspTA_Sched_Figo((UINT32 *)pFigoId, &uPgIdx, &uNeedReload);
    if(res != TEE_SUCCESS) {
        TA_LOGE("sched figo fail, res=0x%x\n", res);
    }else if(uNeedReload) {
        hFigoDrv = current_figohandle(*pFigoId);
        TSPFW_Crypto_ResetFIGO(hFigoDrv, 0);
        TspTA_session_open(*pFigoId);
        TSPFW_Crypto_ReleaseFIGO_FromPC(hFigoDrv, 0);
        TSPFW_Crypto_SendStartStallCmd(hFigoDrv, StallCmd_CMD_cmdTag_RUN);
        TEE_CloseTASession(TSP_TA_session[*pFigoId]);
        TSP_TA_session[*pFigoId] = NULL;
    }
    return res;
}

HRESULT TSPFW_Crypto_StopFW(int32_t uFigoId)
{
    TEE_Result res = TEE_SUCCESS;

    res = TspTA_Figo_Unsched(uFigoId);
    if(res != TEE_SUCCESS) {
        TA_LOGE("release figo fail, res=0x%x\n", res);
    }
    return res;
}
#else
HRESULT TSPFW_Crypto_StartFW(int32_t *pFigoId)
{
    return TEE_SUCCESS;  //for non_tz version, load fw will call api from startup.c
}

HRESULT TSPFW_Crypto_StopFW(int32_t uFigoId)
{
    return TEE_SUCCESS;  //for non_tz version, load fw will call api from startup.c
}
#endif

