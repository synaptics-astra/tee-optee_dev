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

///////////////////////////////////////////////////////////////////////////////
//! \file m2m_ta_mgr.c
//!
//! \brief  This file is used to implement the ta session mgr for m2m ca
//!
//! Author:         Long Xiao
//! Modified by:
//! Version:        1.0
//! Date:           Aug 4 2015
//!
///////////////////////////////////////////////////////////////////////////////
#ifdef TA_COMPILE

#include <string.h>
#include "m2m_ta_mgr.h"
#include "m2m_ta_core.h"
#include "m2m_ta_drv.h"
#include "m2m_ta_fw.h"
#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif

TA_M2M_CRYPTO_T gCryptoSrv;
#define PAGE_SIZE   0x1000

#define VALIDATE_NONSECURE_DTCM(uAddr,uSize) \
   do{ \
      if(uAddr < RA_TspDtcmGlobal_DATA || \
         uAddr >= Secure_Define_Dtcm_Base || \
         uSize > (Secure_Define_Dtcm_Base - uAddr)){ \
           TA_LOGE( "Not nonsecure dtcm mem start=0x%x size=0x%x\n",uAddr, uSize); \
           return  TEE_ERROR_BAD_PARAMETERS; \
      } \
   }while(0)

#define VALIDATE_SECURE_DTCM(uAddr,uSize) \
    do{ \
      if(uAddr < (Secure_Define_Dtcm_Base + sizeof(SIE_SecureCtx)) || \
         uAddr > Secure_Define_Dtcm_End || \
         uSize > (Secure_Define_Dtcm_End + 1 - uAddr)){ \
           TA_LOGE( "Not secure dtcm mem, start=0x%x size=0x%x\n",uAddr, uSize); \
           return  TEE_ERROR_BAD_PARAMETERS; \
      } \
    }while(0)

#define VALIDATE_PRIVATE_DTCM(uAddr,uSize) \
    do{ \
      if(uAddr < (Private_Define_Dtcm_Base + sizeof(SIE_PrivateCtx)) || \
         uAddr > Private_Define_Dtcm_End || \
         uSize > (Private_Define_Dtcm_End + 1 - uAddr)){ \
           TA_LOGE( "Not private dtcm mem, start=0x%x size=0x%x\n",uAddr, uSize); \
           return  TEE_ERROR_BAD_PARAMETERS; \
      } \
    }while(0)

static int  DMXTA_CHECKFIGOID(uint32_t id)
{
    if(id >= TSP_FIGO_NR){
        TA_LOGF( "Invalid figo id:0x%x\n", id);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    return TEE_SUCCESS;
}

/* Should do nothing in TA create/destroy entry point */
TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    TEE_Result res = TEE_SUCCESS;
    memset(&gCryptoSrv, 0, sizeof(gCryptoSrv));
    gCryptoSrv.uSessRefCnt = 0;

    res = tspmem_create_iomap();
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_STORAGE_NOT_AVAILABLE;

    return res;
}


void TA_EXPORT TA_DestroyEntryPoint(void)
{
    Drm_KeyMgr_TA_session_close();
    return;
}


TEE_Result TA_OpenSessionEntryPoint(uint32_t        paramTypes,
                                    TEE_Param       params[4],
                                    void**          sessionContext)
{
    TEE_Result res = TEE_SUCCESS;
    TA_M2M_SESS_T* pM2MCtx = NULL;

    int32_t eType = 0;
    UINT32 uFigosysId = 0;

    TA_LOGH( "TA_OpenSessionEntryPoint in\n");

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_OUTPUT) {
          return TEE_ERROR_BAD_PARAMETERS;
    }

    eType = params[0].value.a;

    res = M2M_TA_Core_Session_Open(&gCryptoSrv, eType, &pM2MCtx);
    if(pM2MCtx) {
        params[1].value.a = pM2MCtx->uSessId;
        *sessionContext = pM2MCtx;
        pM2MCtx->bDummy = (params[0].value.b == 1) ? true : false;

        if(!pM2MCtx->bDummy){
            res =M2M_TA_Core_StartFw(&uFigosysId);
            if(res != TEE_SUCCESS){
                TA_LOGF( "Faild to Load FW\n");
                goto error;
            }else {
                if(gCryptoSrv.uFigoSessRefCnt[uFigosysId] == 0){
                    res = M2M_TA_Core_AllocFigoTask(uFigosysId);
                    if(res != TEE_SUCCESS){
                        TA_LOGF( "Faild to Alloc figo task\n");
                        M2M_TA_Core_StopFw(uFigosysId);
                        goto error;
                    }
                }
            }
            params[1].value.b = uFigosysId;
            gCryptoSrv.uFigoSessRefCnt[uFigosysId]++;
        }
        pM2MCtx->uFigoSysId = uFigosysId;
        gCryptoSrv.uSessRefCnt++;
    }else{
        TA_LOGF( "Faild to malloc mem for TA CTX\n");
        res = TEE_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    TA_LOGH( "TA_OpenSessionEntryPoint out\n");

    return TEE_SUCCESS;
error:
    if(pM2MCtx) {
        M2M_TA_Core_Session_Close(pM2MCtx);
    }
    return res;
}

void TA_CloseSessionEntryPoint(void*  sessionContext)
{
    TEE_Result res = TEE_SUCCESS;
    TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;
    if(pM2MCtx) {
        res = M2M_TA_Core_Session_Close(pM2MCtx);
        gCryptoSrv.uSessRefCnt--;

        if(!pM2MCtx->bDummy){
            gCryptoSrv.uFigoSessRefCnt[pM2MCtx->uFigoSysId]--;
            if(gCryptoSrv.uFigoSessRefCnt[pM2MCtx->uFigoSysId] == 0){
                M2M_TA_Core_FreeFigoTask(pM2MCtx->uFigoSysId);
            }
            res = M2M_TA_Core_StopFw(pM2MCtx->uFigoSysId);
        }

        if (res != TEE_SUCCESS) {
            TA_LOGF( "TA_CloseSessionEntryPoint error: %x\n", res);
        }
    } else {
        TA_LOGF( "TA_CloseSessionEntryPoint Null Pointer\n");
    }
}

static TEE_Result M2MTA_CryptoInit(void* sessionContext,
                                uint32_t  paramTypes,
                                TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    int32_t uFigoId = 0;
    UINT16   uCmdQueueBase;
    UINT16   uCmdQueueSize;
    UINT16   uCmdCntBase;
    UINT16   uM2MCryptoCmdBase;
    UINT16   uLocalDataIn, uLocalDataOut;
    FIGODRV_HANDLE  hFigoDrv = NULL;
    TA_M2M_BUFF_T *pCmdBaseBuf, *pBIUImageBuf;
    uint32_t uCmdBufferSize = TSP_CRYPTO_MAX_CMD_NUM * sizeof(SIE_M2MCryptoCmd);
    uintptr_t pShadowBufAddr = 0;
    uint64_t  pShadowBufferLen = 0;
    scatter_list_t *sg;

    TA_LOGH( "M2MTA_CryptoInit\n");

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
            return TEE_ERROR_BAD_PARAMETERS;
    }

    uFigoId = params[0].value.a;
    res = DMXTA_CHECKFIGOID(uFigoId);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    pCmdBaseBuf = &gCryptoSrv.CmdBaseBuf[uFigoId];
    pBIUImageBuf = &gCryptoSrv.BIUImageBuf[uFigoId];

    if(gCryptoSrv.ufigohdl[uFigoId]) {
        TA_LOGH("M2M crypto context of figo[%d] has already been initialized!\n", uFigoId);
        return TEE_ERROR_NOT_IMPLEMENTED;
    }

    gCryptoSrv.ufigohdl[uFigoId] = dmx_malloc(sizeof(MV_FIGODRV_CTX));
    TA_LOGH( "M2MTA_CryptoInit, %p\n", gCryptoSrv.ufigohdl[uFigoId]);
    if(gCryptoSrv.ufigohdl[uFigoId] == NULL) {
        TA_LOGF("Figo[%d] malloc NULL\n", uFigoId);
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    tspbuf_create_alloc(pCmdBaseBuf, uCmdBufferSize);
    M2M_CHECK_RETCODE(res);

    hFigoDrv = gCryptoSrv.ufigohdl[uFigoId];
    if (uFigoId == 0) {
        hFigoDrv->uSubSysRegBase = TSP_FIGO_REG_BASE;
    }else {
        hFigoDrv->uSubSysRegBase = TSP_FIGO_REG_BASE + TSP_FIGO_REG_SIZE;
    }
    hFigoDrv->uSubSysRegSize = TSP_FIGO_REG_SIZE;
    hFigoDrv->uBIUSize = TSP_FIGO_REG_SIZE;
    hFigoDrv->uDTCMRegBase = RA_TspReg_FigoDtcm;
    hFigoDrv->uFigoSysStart = RA_TspReg_FigoReg;

    if (TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_NONE &&
        TEE_PARAM_TYPE_GET(paramTypes, 1) == TEE_PARAM_TYPE_MEMREF_INPUT) {
        pShadowBufAddr = *((uintptr_t*)params[1].memref.buffer);
        pShadowBufferLen = *((uint64_t*)params[1].memref.buffer + 1);

        sg = TEE_Malloc(SIZE_OF_SCATTERLIST(1), TEE_MALLOC_ZEROS);

        sg->num = 1;
        sg->s_item[0].addr = pShadowBufAddr;
        sg->s_item[0].npage =pShadowBufferLen / PAGE_SIZE;

        hFigoDrv->pbBIUReservation = TEE_CreateReservation(sg, 0);
        if (res != TEE_SUCCESS) {
            TEE_Free(sg);
            TA_LOGF("TEE_CreateReservation failed with 0x%x\n", res);
            return res;
        }

        res = TEE_ReserveSecurePages(hFigoDrv->pbBIUReservation);
        if (res != TEE_SUCCESS) {
            TEE_DestroyReservation(hFigoDrv->pbBIUReservation);
            hFigoDrv->pbBIUReservation = NULL;
            TEE_Free(sg);
            TA_LOGF("TEE_ReserveSecurePages failed with 0x%x\n", res);
            return res;
        }
        TEE_Free(sg);
    }

// for OPTEE enable, there is no SHM driver, user space application cannot get physical address any more
#ifndef CONFIG_OPTEE
    res = tspbuf_create_with_pa(pBIUImageBuf, params[0].value.b,
            TSP_FIGO_REG_SIZE, true);
#else
    if (hFigoDrv->pbBIUReservation) {
        res = tspbuf_create_with_memid(pBIUImageBuf, hFigoDrv->pbBIUReservation->mem_id,
            0, TSP_FIGO_REG_SIZE);
    } else {
        res = tspbuf_create_with_memid(pBIUImageBuf, params[0].value.b,
            0, TSP_FIGO_REG_SIZE);
    }
#endif
    M2M_CHECK_RETCODE(res);
    hFigoDrv->pbBIUImage = pBIUImageBuf->virAddr;
    hFigoDrv->pbBIUImagePhy = pBIUImageBuf->phyAddr;

    hFigoDrv->uFigoSysId = uFigoId;
    hFigoDrv->uFigoSysType = 1;
    hFigoDrv->pFigoCfgData.uLocalAreaAddrInDTCM = RA_TspDtcmGlobal_LA;

    uCmdCntBase = M2M_TA_Dtcm_Mem_Alloc(HBOFifoDepth*2, 8, uFigoId, DTCM_MEM_TYPE_NONSECURE);
    uM2MCryptoCmdBase = M2M_TA_Dtcm_Mem_Alloc(sizeof(SIE_M2MCryptoCmd), 8, uFigoId, DTCM_MEM_TYPE_SECURE);
    uCmdQueueSize = TSP_CRYPTO_CMDQ_SIZE * sizeof(SIE_M2MCryptoCmd);
    uCmdQueueBase = M2M_TA_Dtcm_Mem_Alloc(uCmdQueueSize, 8, uFigoId, DTCM_MEM_TYPE_SECURE);
    uLocalDataIn = M2M_TA_Dtcm_Mem_Alloc(M2M_LOCAL_DATA_SIZE, 8, uFigoId, DTCM_MEM_TYPE_NONSECURE);
    uLocalDataOut = ADDR_ALIGN(Private_Define_Dtcm_Base + sizeof(SIE_PrivateCtx), 8);

    TA_LOGF( "Physical address %p\n", gCryptoSrv.ufigohdl[uFigoId]->pbBIUImagePhy);
    TA_LOGF( "Virtual address %p\n", gCryptoSrv.ufigohdl[uFigoId]->pbBIUImage);
    TA_LOGF( "uSubSysRegBase = 0x%x\n", gCryptoSrv.ufigohdl[uFigoId]->uSubSysRegBase);
    TA_LOGF( "uFigoSysId = 0x%x\n", gCryptoSrv.ufigohdl[uFigoId]->uFigoSysId);
    TA_LOGF( "uFigoSysStart = 0x%x\n", gCryptoSrv.ufigohdl[uFigoId]->uFigoSysStart);
    TA_LOGF( "sizeof(MV_FIGODRV_CTX) = 0x%lx\n", sizeof(MV_FIGODRV_CTX));
    TA_LOGF( "pCmdBasePhyAddr %p\n", pCmdBaseBuf->phyAddr);

    TSP_CryptoInit((void* )internal_sess_callback, uFigoId, pCmdBaseBuf, uCmdQueueBase,  \
                   uCmdQueueSize, uCmdCntBase, uLocalDataIn, uLocalDataOut, uM2MCryptoCmdBase);

    TA_LOGH( "M2MTA_CryptoInit exit with 0x%x\n", res);
    return TEE_SUCCESS;

error:
    tspbuf_release(pCmdBaseBuf);
    tspbuf_release(pBIUImageBuf);
    if(gCryptoSrv.ufigohdl[uFigoId]){
        dmx_free(gCryptoSrv.ufigohdl[uFigoId]);
        gCryptoSrv.ufigohdl[uFigoId] = NULL;
    }
    return res;

}

static TEE_Result M2MTA_CryptoDeinit(void* sessionContext,
                                         uint32_t  paramTypes,
                                         TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    int32_t uFigoId = 0;

    TA_LOGH( "M2MTA_CryptoDeinit, Para:0x%x\n", paramTypes);

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
          return TEE_ERROR_BAD_PARAMETERS;
    }
    uFigoId = params[0].value.a;
    res = DMXTA_CHECKFIGOID(uFigoId);
    if(TEE_SUCCESS != res)
         return  TEE_ERROR_BAD_PARAMETERS;

    if(gCryptoSrv.ufigohdl[uFigoId]) {
        TSP_CryptoDeinit(uFigoId);
        tspbuf_release(&gCryptoSrv.CmdBaseBuf[uFigoId]);
        tspbuf_release(&gCryptoSrv.BIUImageBuf[uFigoId]);
        dmx_free(gCryptoSrv.ufigohdl[uFigoId]);
        gCryptoSrv.ufigohdl[uFigoId] = NULL;
    } else {
        TA_LOGH("Figo[%d] handle DeInitialized\n", uFigoId);
        return TEE_SUCCESS;
    }

    if(gCryptoSrv.MutexLock) {
        #if 0
        #ifndef TA_COMPILE
        res = MV_OSAL_Mutex_Destroy(&gCryptoSrv.MutexLock);
        #else
        res = TEE_MutexDestroy(gCryptoSrv.MutexLock);
        #endif
        #endif
        gCryptoSrv.MutexLock = NULL;
        if(res != TEE_SUCCESS) {
            TA_LOGF( "MV_OSAL_Mutex_Destroy fail 0x%x\n", res);
        }
    } else {
        TA_LOGH("gCryptoSrv MutexLock Deinitialized\n");
    }

    TA_LOGH( "M2MTA_CryptoDeinit exit with 0x%x\n", res);
    return TEE_SUCCESS;
}

static TEE_Result M2MTA_Setscheme( void* sessionContext,
                                uint32_t  paramTypes,
                                TEE_Param params[4])
{
     TEE_Result res = TEE_SUCCESS;
     int32_t uScheme = 0;
     TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;

     TA_LOGH( "M2MTA_Setscheme, pM2MCtx:%p,  scheme: 0x%x\n", pM2MCtx, params[0].value.a);
     #if 0
     MV_OSAL_Mutex_Lock(pM2MCtx->SessionMutexLock);
     #endif

     if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
             return TEE_ERROR_BAD_PARAMETERS;
     }
     uScheme = params[0].value.a;
     res = M2M_TA_Core_Session_SetScheme(pM2MCtx, uScheme);

     #if 0
     MV_OSAL_Mutex_Unlock(pM2MCtx->SessionMutexLock);
     #endif
     return res;
}

static TEE_Result M2MTA_Setkey(void* sessionContext,
                            uint32_t  paramTypes,
                            TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;

    M2MCRYPTO_KEY_OPTION  KeyOption;
    UINT32          TSPKeyTblIndex;

    M2MCRYPTO_KEY_OPTION  IVOption;
    UINT32          TSPIVTblIndex;

    M2M_KEY_INFO_ST *KeyInfo = NULL;

    TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;
    TA_M2M_BUFF_T KeyBuf, IVBuf, CryptoInfoBuf, KeyInfoBuf;

    TA_LOGH( "M2MTA_Setkey, pM2MCtx:%p\n", pM2MCtx);
    #if 0
    MV_OSAL_Mutex_Lock(pM2MCtx->SessionMutexLock);
    #endif

    memset(&KeyBuf, 0, sizeof(TA_M2M_BUFF_T));
    memset(&IVBuf, 0, sizeof(TA_M2M_BUFF_T));
    memset(&CryptoInfoBuf, 0, sizeof(TA_M2M_BUFF_T));
    memset(&KeyInfoBuf, 0, sizeof(TA_M2M_BUFF_T));

    // memref
    if (TEE_PARAM_TYPE_GET(paramTypes, 2) == TEE_PARAM_TYPE_MEMREF_INPUT &&
        TEE_PARAM_TYPE_GET(paramTypes, 3) == TEE_PARAM_TYPE_MEMREF_INPUT) {

        if (TEE_PARAM_TYPE_GET(paramTypes, 0) == TEE_PARAM_TYPE_MEMREF_INPUT) {
            res = tspbuf_create_with_va(&KeyBuf, (uintptr_t)params[0].memref.buffer,
                                            params[0].memref.size);
            M2M_CHECK_RETCODE(res);
        }

        if (TEE_PARAM_TYPE_GET(paramTypes, 1) == TEE_PARAM_TYPE_MEMREF_INPUT) {
            res = tspbuf_create_with_va(&IVBuf, (uintptr_t)params[1].memref.buffer,
                                            params[1].memref.size);
            M2M_CHECK_RETCODE(res);
        }

        res = tspbuf_create_with_va(&CryptoInfoBuf, (uintptr_t)params[2].memref.buffer,
                                        params[2].memref.size);
        M2M_CHECK_RETCODE(res);

        if (params[3].memref.size < sizeof(M2M_KEY_INFO_ST)) {
            TA_LOGE("Invalid keyInfoBuf size(%u)\n", params[3].memref.size);
            goto error;
         }


        res = tspbuf_create_with_va(&KeyInfoBuf, (uintptr_t)params[3].memref.buffer,
                                        params[3].memref.size);
         M2M_CHECK_RETCODE(res);
    // memid
    } else if (TEE_PARAM_TYPE_GET(paramTypes, 0) == TEE_PARAM_TYPE_VALUE_INPUT &&
        TEE_PARAM_TYPE_GET(paramTypes, 1) == TEE_PARAM_TYPE_VALUE_INPUT &&
        TEE_PARAM_TYPE_GET(paramTypes, 2) == TEE_PARAM_TYPE_VALUE_INPUT &&
        TEE_PARAM_TYPE_GET(paramTypes, 3) == TEE_PARAM_TYPE_VALUE_INPUT) {

        if (params[0].value.a != 0) {
            res = tspbuf_create_with_memid(&KeyBuf, params[0].value.a, 0, params[0].value.b);
            M2M_CHECK_RETCODE(res);
        }

        if (params[1].value.a != 0) {
            res = tspbuf_create_with_memid(&IVBuf, params[1].value.a, 0, params[1].value.b);
            M2M_CHECK_RETCODE(res);
        }

        res = tspbuf_create_with_memid(&CryptoInfoBuf, params[2].value.a, 0,
                                                            sizeof(M2M_DRM_CRYPTO_ST));
        M2M_CHECK_RETCODE(res);

        res = tspbuf_create_with_memid(&KeyInfoBuf, params[3].value.a, 0,
                                                        sizeof(M2M_KEY_INFO_ST));
        M2M_CHECK_RETCODE(res);
    } else {
        TA_LOGE("Invalid paramTypes: 0x%x\n", paramTypes);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    KeyInfo = (M2M_KEY_INFO_ST *)KeyInfoBuf.virAddr;

    if (KeyInfo->uiKeyOption == M2MCRYPTO_KEY_CONTENT &&
    (!KeyBuf.virAddr || KeyBuf.uBufSize < KeyInfo->uiKeyLength)) {
        TA_LOGE("Invalid KeyBuf: virAddr(%p), size(%u)\n",
        KeyBuf.virAddr, KeyBuf.uBufSize);
        goto error;
    }

    if (KeyInfo->uiIVOption == M2MCRYPTO_KEY_CONTENT &&
    (!IVBuf.virAddr || IVBuf.uBufSize < KeyInfo->uiIVLength)) {
        TA_LOGE("Invalid IVBuf: virAddr(%p), size(%u)\n",
        IVBuf.virAddr, IVBuf.uBufSize);
        goto error;
    }

    KeyOption   = KeyInfo->uiKeyOption;
    KeyBuf.uBufSize   = KeyInfo->uiKeyLength;
    TSPKeyTblIndex = KeyInfo->uiKeyTblIndex;

    IVOption    = KeyInfo->uiIVOption;
    IVBuf.uBufSize    = KeyInfo->uiIVLength;
    TSPIVTblIndex  = KeyInfo->uiIVTblIndex;

    TA_LOGH( "M2MTA_Setkey,pM2MCtx:0x%p,Key:0x%x,0x%x,0x%x;IV:0x%x,0x%x,0x%x\n", pM2MCtx, KeyOption, \
        TSPKeyTblIndex,  KeyInfo->uiKeyLength, IVOption, TSPIVTblIndex, KeyInfo->uiIVLength);

    /* Key should be set to an allocated dtcm address, or else crypto engine will hang meeting an unknown address */
    res = M2M_TA_Core_Session_Config(pM2MCtx, KeyOption, TSPKeyTblIndex, &KeyBuf,  \
                                    IVOption, TSPIVTblIndex, &IVBuf, &CryptoInfoBuf);

    #if 0
    MV_OSAL_Mutex_Unlock(pM2MCtx->SessionMutexLock);
    #endif
error:
    tspbuf_release(&KeyBuf);
    tspbuf_release(&IVBuf);
    tspbuf_release(&KeyInfoBuf);
    tspbuf_release(&CryptoInfoBuf);

    return res;
}

static TEE_Result M2MTA_SetPatternMode( void* sessionContext,
                                uint32_t  paramTypes,
                                TEE_Param params[4])
{
     TEE_Result res = TEE_SUCCESS;
     M2MCRYPTO_PATTERN_MODE_CFG patternCfg;
     TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;

     TA_LOGH( "M2MTA_SetPatternMode, pM2MCtx:%p, enc:clr [0x%x:0x%x]\n", \
           pM2MCtx, params[0].value.a,params[0].value.b);

     if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
             return TEE_ERROR_BAD_PARAMETERS;
     }
     patternCfg.pattern_enc = params[0].value.a;
     patternCfg.pattern_clr = params[0].value.b;

     res = M2M_TA_Core_Session_SetPatternMode(pM2MCtx, &patternCfg);

     return res;
}

static TEE_Result M2MTA_SetResidueMode( void* sessionContext,
                                uint32_t  paramTypes,
                                TEE_Param params[4])
{
     TEE_Result res = TEE_SUCCESS;
     TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;

     TA_LOGH( "M2MTA_SetResidueMode, pM2MCtx:%p, residueType: 0x%x\n", pM2MCtx, params[0].value.a);

     if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
             return TEE_ERROR_BAD_PARAMETERS;
     }

     res = M2M_TA_Core_Session_SetResidueMode(pM2MCtx, params[0].value.a);

     return res;
}

static TEE_Result M2MTA_SetIVPolicy( void* sessionContext,
                                uint32_t  paramTypes,
                                TEE_Param params[4])
{
     TEE_Result res = TEE_SUCCESS;
     TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;

     TA_LOGH( "M2MTA_IVPolicy, pM2MCtx:%p, IVPolicy: 0x%x\n", pM2MCtx, params[0].value.a);

     if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
             return TEE_ERROR_BAD_PARAMETERS;
     }

     res = M2M_TA_Core_Session_SetIVPolicy(pM2MCtx, params[0].value.a);

     return res;
}

static TEE_Result M2MTA_Update(void* sessionContext,
                            uint32_t  paramTypes,
                            TEE_Param params[4])
{
      TEE_Result res = TEE_SUCCESS;
      uint32_t uInputPhyAddr, uOutputPhyAddr;
      uint32_t uInputSize = 0;
      uint32_t uOutputSize = 0;
      TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;
      TA_M2M_BUFF_T inputBuf, outputBuf;

      //params[0] for input shmid/address size
      //params[1] for output shmid/address size
      //params[3] for memory type
      if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
          TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
          TEE_PARAM_TYPE_GET(paramTypes, 3) != TEE_PARAM_TYPE_VALUE_INPUT) {
          return TEE_ERROR_BAD_PARAMETERS;
      }

      memset(&inputBuf, 0, sizeof(TA_M2M_BUFF_T));
      memset(&outputBuf, 0, sizeof(TA_M2M_BUFF_T));

      if (params[3].value.a == TSP_MEM_PARAM_SHM_ID) {

          //params[2] for input/output offset
          if (TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_INPUT) {
              return TEE_ERROR_BAD_PARAMETERS;
          }

          res = tspbuf_create_with_memid(&inputBuf, params[0].value.a, params[2].value.a,
                    params[0].value.b);
          M2M_CHECK_RETCODE(res);

          if(params[1].value.a != params[0].value.a) {
              res = tspbuf_create_with_memid(&outputBuf, params[1].value.a, params[2].value.b,
                        params[1].value.b);
              M2M_CHECK_RETCODE(res);
          }else {
              memcpy(&outputBuf, &inputBuf, sizeof(TA_M2M_BUFF_T));
              outputBuf.bDup = true;
          }
      } else if(params[3].value.a == TSP_MEM_PARAM_PHY_ADDR){
          uInputPhyAddr  = params[0].value.a;
          uInputSize     = params[0].value.b;

          uOutputPhyAddr = params[1].value.a;
          uOutputSize    = params[1].value.b;

          res = tspbuf_create_twobuf_with_pa(uInputPhyAddr, uInputSize, uOutputPhyAddr,
                        uOutputSize, &inputBuf, &outputBuf);
          M2M_CHECK_RETCODE(res);

          TA_LOGH( "pM2MCtx:%p, 0x%x,0x%x, 0x%x, 0x%x\n",   \
              pM2MCtx, uInputPhyAddr, uInputSize, uOutputPhyAddr, uOutputSize);
      }else {
          TA_LOGE( "pM2MCtx:%p, invalid memory type 0x%x\n", pM2MCtx, params[3].value.a);
          return TEE_ERROR_BAD_PARAMETERS;
      }

      res = M2M_TA_Core_Session_Update(pM2MCtx, &inputBuf, &outputBuf);
      TA_LOGH( "pM2MCtx:%p out with 0x%x\n", pM2MCtx, res);
      M2M_CHECK_RETCODE(res);
      return TEE_SUCCESS;

error:
      tspbuf_release(&inputBuf);
      tspbuf_release(&outputBuf);
      return res;
}

static TEE_Result M2MTA_Update_Residue(void* sessionContext,
                                    uint32_t  paramTypes,
                                    TEE_Param params[4])
{
    // Not implemented yet

    return TEE_SUCCESS;
}


static TEE_Result M2MTA_FeedCmd(void* sessionContext,
                             uint32_t  paramTypes,
                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;
#if 0
    MV_OSAL_Mutex_Lock(pM2MCtx->SessionMutexLock);
#endif

    res = M2M_TA_Core_Session_Feed(pM2MCtx);
#if 0
    MV_OSAL_Mutex_Unlock(pM2MCtx->SessionMutexLock);
#endif
    return res;
}

static TEE_Result M2MTA_Postprocess(void* sessionContext,
                                 uint32_t  paramTypes,
                                 TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    TA_M2M_SESS_T* pM2MCtx = (TA_M2M_SESS_T*)sessionContext;
    FIGODRV_HANDLE  hFigoDrv = NULL;
#if 0
    MV_OSAL_Mutex_Lock(pM2MCtx->SessionMutexLock);
#endif

    hFigoDrv = gCryptoSrv.ufigohdl[pM2MCtx->uFigoSysId];
    res = M2M_TA_Core_Session_Postprocess(pM2MCtx, hFigoDrv);

    params[0].value.a = pM2MCtx->uCmdFinishNum;
#if 0
    MV_OSAL_Mutex_Unlock(pM2MCtx->SessionMutexLock);
#endif
    return res;
}

typedef struct __m2m_ta_cmd_handler__
{
    uint32_t commandID;
    TEE_Result (*cmd_handler)(void* sessionContext, uint32_t  paramTypes, TEE_Param params[4]);
} M2M_TA_CMD_HANDLER;

static M2M_TA_CMD_HANDLER m2m_ta_mgr_cmd_handler[M2M_CMD_MAX + 1] =
{
    {M2M_INVALID, NULL},
    {M2M_CryptoInit, M2MTA_CryptoInit},
    {M2M_Setscheme, M2MTA_Setscheme},
    {M2M_Setkey, M2MTA_Setkey},
    {M2M_Update, M2MTA_Update},
    {M2M_Update_Residue, M2MTA_Update_Residue},
    {M2M_FeedCmd, M2MTA_FeedCmd},
    {M2M_Postprocess, M2MTA_Postprocess},
    {M2M_CryptoDeinit, M2MTA_CryptoDeinit},
    {M2M_SetPatternMode, M2MTA_SetPatternMode},
    {M2M_SetResidueMode, M2MTA_SetResidueMode},
    {M2M_SetIVPolicy, M2MTA_SetIVPolicy},
    {M2M_CMD_MAX, NULL},
};

TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(
    void*            sessionContext,
    uint32_t        commandID,
    uint32_t        paramTypes,
    TEE_Param        params[4])
{
    TEE_Result res = TEE_ERROR_NOT_SUPPORTED;

    //TA_LOGH( "TA_InvokeCommandEntryPoint, ID:0x%x\n", commandID);
    if( (commandID > M2M_INVALID && commandID < M2M_CMD_MAX) &&
        m2m_ta_mgr_cmd_handler[commandID].cmd_handler)
        res = m2m_ta_mgr_cmd_handler[commandID].cmd_handler(sessionContext, paramTypes, params);
    return res;
}

#ifndef CONFIG_OPTEE
TA_DEFINE_BEGIN
    .uuid = TA_M2M_UUID,
    .name = "M2M",
    .singleInstance = true,
    .multiSession = true,
    .instanceKeepAlive = true,
TA_DEFINE_END
#endif

#endif

