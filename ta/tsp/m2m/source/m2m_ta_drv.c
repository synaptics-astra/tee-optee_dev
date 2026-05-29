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
//! \file m2m_ta_drv.c
//!
//! \brief TSP firmware driver for mem2mem crypto module.
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    November 14 2012,        Weizhao Jiang
//!     V 2.00,    Aug 4 2015,              Long Xiao
//!
//! Note: Moved to TZ in Version 2.0
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "m2m_ta_mgr.h"
#include "m2m_ta_drv.h"
#include "m2m_ta_fw.h"
#include "tsp_nsk_alg.h"

#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif


static TSP_CRYPTO_LIB  gTspCryptoLib;

static NSK_ALG_M2M WRAP_M2M_INPUT_TYPE_TO_NSK(
                    M2MCRYPTO_TYPE M2M_InputType)
{
    NSK_ALG_M2M nskAlg = NSK_ALG_M2M_START;
    switch(M2M_InputType)
    {
        case M2MCRYPTO_TYPE_DES_56_ECB:
            nskAlg = DES_ECB_CLR_CLR;
            break;
        case M2MCRYPTO_TYPE_TDES_128_ECB:
            nskAlg = TDES_ECB_CLR_CLR;
            break;
        case M2MCRYPTO_TYPE_TDES_128_CBC:
            nskAlg = TDES_CBC_CLR_CLR;
            break;
        case M2MCRYPTO_TYPE_AES_128_ECB:
            nskAlg = AES_ECB_CLR_CLR;
            break;
        case M2MCRYPTO_TYPE_AES_128_CBC:
            nskAlg = AES_CBC_CLR_CLR;
            break;
        case M2MCRYPTO_TYPE_AES_128_CTR:
            nskAlg = AES_CTR;
            break;
        case M2MCRYPTO_TYPE_AES_128_CTR64:
            nskAlg = AES_CTR64;
            break;
        default:
            TA_LOGH("no nsk alg to match the input type 0x%x\n", M2M_InputType);
            break;
    }
    return nskAlg;
}

static HRESULT internal_FeedData(FIGODRV_HANDLE  hFigoDrv, M2M_TA_CRYPTO_CMD *pDatChunkCmd)
{
    HRESULT ret = SUCCESS;
    // fixed error: cast increases required alignment of target type, first convert to void*
    // then cast to SIE_TspDtcmGlobal*
	SIE_TspDtcmGlobal* pTspDtcmGlobal = (SIE_TspDtcmGlobal*)(void*)MV_FIGODRV_GET_DTCM_SHADOW_PTR(hFigoDrv);
	SIE_M2MCryptoCtx *pM2MCryptoCtx = &pTspDtcmGlobal->ie_M2MCryptoCtx;
    SIE_M2MCryptoCmd *pCryptoCmdWr = NULL;
    TSP_CRYPTO_CMD_GROUP *pCmdGroup = NULL;
    UINT8 *pPhyInAddr = NULL;
    UINT8 *pPhyOutAddr = NULL;

    TA_LOGI("uFigoId(=%d) CmdSpilt drm_scheme(=0x%x) In(PhyAddr=0x%p, size=0x%x), Out(PhyAddr=0x%p, size=0x%x), CmdBasePhyAddr = 0x%p\n",
        hFigoDrv->uFigoSysId, pDatChunkCmd->eCryptoScheme, pDatChunkCmd->InPhyAddr, pDatChunkCmd->uInSize,
        pDatChunkCmd->OutPhyAddr, pDatChunkCmd->uOutSize, gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].pCmdBasePhyAddr);

    /* Below part needs to be mutex, crypto cmd buffer is shared */
    if((UINT32)TSPFW_Crypto_CheckQSpace(hFigoDrv) >= sizeof(SIE_M2MCryptoCmd)) {
        pCmdGroup = dmx_malloc(sizeof(TSP_CRYPTO_CMD_GROUP));
        if(pCmdGroup == NULL)  {
            ret = ERR_NOMEM;
            goto error;
        }
        memset(pCmdGroup, 0, sizeof(TSP_CRYPTO_CMD_GROUP));

        TA_LOGI("Feed Enter\n");
        pCmdGroup->hSess = pDatChunkCmd->hSess;
        pCmdGroup->uiGroupID = pDatChunkCmd->Tag;
        pCmdGroup->ufigoId = hFigoDrv->uFigoSysId;

        pPhyInAddr = pDatChunkCmd->InPhyAddr;
        pPhyOutAddr = pDatChunkCmd->OutPhyAddr;

        /* M2M TA will use the virtual address */

        // fixed error: cast increases required alignment of target type, first convert to void*
        // then cast to SIE_TspDtcmGlobal*
        pCryptoCmdWr = (SIE_M2MCryptoCmd *)(void*)((UINT8 *)gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].pCmdBaseVirtAddr \
                        + gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].uCmdQWrPos);
        pCmdGroup->uFirstCmdPos = gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].uCmdQWrPos;
        memset(pCryptoCmdWr, 0, sizeof(SIE_M2MCryptoCmd));
        pCryptoCmdWr->u_cmd_id = pCmdGroup->uiGroupID;
        pCryptoCmdWr->u_Scheme = pDatChunkCmd->eCryptoScheme;
        pCryptoCmdWr->u_parameter = pDatChunkCmd->uCryptoParam;
        pCryptoCmdWr->u_PhyInputAddr = (uintptr_t)(pPhyInAddr);
        pCryptoCmdWr->u_PhyOutputAddr= (uintptr_t)(pPhyOutAddr);
        /*calc u_source_address/u_desination_address*/
        pCryptoCmdWr->u_source_address = pM2MCryptoCtx->u_pLocalDataIn;
        MV_FIGOC_ALIGN_PTR(pCryptoCmdWr->u_source_address, pCryptoCmdWr->u_PhyInputAddr);
        pCryptoCmdWr->u_desination_address = pM2MCryptoCtx->u_pLocalDataOut;
        MV_FIGOC_ALIGN_PTR(pCryptoCmdWr->u_desination_address, pCryptoCmdWr->u_PhyOutputAddr);

        pCryptoCmdWr->u_key_address = pDatChunkCmd->uKeyAddr;
        pCryptoCmdWr->u_key_address_2 = 0;
        pCryptoCmdWr->u_iv_address = pDatChunkCmd->uIVAddr;
        pCryptoCmdWr->u_write_back_iv = 1;
        pCryptoCmdWr->u_input_size = pDatChunkCmd->uInSize;
        pCmdGroup->uLastCmdPos = pCmdGroup->uFirstCmdPos+sizeof(SIE_M2MCryptoCmd);

        pCryptoCmdWr->u_pattern_enc = pDatChunkCmd->PatternCfg.pattern_enc;
        pCryptoCmdWr->u_pattern_clr = pDatChunkCmd->PatternCfg.pattern_clr;
        pCryptoCmdWr->u_residue_mode = pDatChunkCmd->eResidueType;

        pCryptoCmdWr->u_crypto_type = (pDatChunkCmd->eInputSessType == M2MCRYPTO_SESSION_TYPE_DEC) ?
                                       M2MCryptoCmd_crypto_type_Dec : M2MCryptoCmd_crypto_type_Enc;
        pCryptoCmdWr->u_algo_index = WRAP_M2M_INPUT_TYPE_TO_NSK(pDatChunkCmd->eInputCryptoType);

        TA_LOGI("Feed Done\n");
        /*clean cache*/
        if(pCmdGroup->uLastCmdPos > pCmdGroup->uFirstCmdPos) {
            #ifndef TA_COMPILE
            AMP_SHM_CleanCache(gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].hCmdMem,
                        pCmdGroup->uFirstCmdPos,
                        sizeof(SIE_M2MCryptoCmd));
            #else
            TEE_FlushCache((void* )((UINT8 *)gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].pCmdBaseVirtAddr+pCmdGroup->uFirstCmdPos),  \
              sizeof(SIE_M2MCryptoCmd));
            #endif
        }
        else {
            TA_LOGE("Shouldn't happen....\n");
        }

        TA_LOGI("Feed Out\n");
        gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].uCmdQWrPos = pCmdGroup->uLastCmdPos;
        if(pCmdGroup->uLastCmdPos == gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].uSize) {
            gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].uCmdQWrPos = 0;
        }
        pCmdGroup->eState = TSP_CMD_GROUP_PUSHED;
        DMX_LIST_PUSH_BACK(gTspCryptoLib.pGroupList, pCmdGroup);

        /*Sync size to Figo*/
        TSPFW_Crypto_SetCmdQWrOffset(hFigoDrv, gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].uCmdQWrPos);
        TA_LOGI("CmdQ Status[uCmdQWrPos=0x%x][uFirstCmdPos=0x%x, uLastCmdPos=0x%x]\n",
                gTspCryptoLib.CmdQ[hFigoDrv->uFigoSysId].uCmdQWrPos, pCmdGroup->uFirstCmdPos,
                pCmdGroup->uLastCmdPos);
    }
    else {
        TA_LOGH("Crypto SHM Queue overflow, it's not a error case!\n");
        ret = ERR_CMDQ_UNDERFLOW;
        goto error;
    }

    return ERR_CMDQ_PUSHED;
error:
    TA_LOGE("internal_FeedData() return error(ret=0x%x)\n", ret);
    return ret;
}

#if 0
static HRESULT internal_EnableIntFlag()
{
    return SUCCESS;
}
#endif

HRESULT internal_CmdPostProcess(FIGODRV_HANDLE  hFigoDrv)
{
	HRESULT	ret = SUCCESS;
    DMX_LIST_NODE *pGroupNode = NULL;
    TSP_CRYPTO_CMD_GROUP *pCmdGroup = NULL;
    UINT32 uCmdQRdOffset = 0;

    uCmdQRdOffset = TSPFW_Crypto_GetCmdQRdOffset(hFigoDrv);

    while(((DMX_LIST_NEXT(gTspCryptoLib.pGroupList,
            &pGroupNode)) == SUCCESS) && (pGroupNode != NULL))
    {
        pCmdGroup =(TSP_CRYPTO_CMD_GROUP *)pGroupNode->pData;
        if(pCmdGroup->ufigoId == hFigoDrv->uFigoSysId) {
            if(pCmdGroup->eState == TSP_CMD_GROUP_PUSHED)
            {
                if(pCmdGroup->uLastCmdPos > pCmdGroup->uFirstCmdPos) {
                    if((uCmdQRdOffset>=pCmdGroup->uFirstCmdPos) &&
                       (uCmdQRdOffset<pCmdGroup->uLastCmdPos))
                    {
                        /*The top group cmds is still in processing*/
                        //DMX_INFO("Processing[uCmdQRdOffset=0x%x], [uFirstCmdPos=0x%x, uLastCmdPos=0x%x]\n",
                            //uCmdQRdOffset, pCmdGroup->uFirstCmdPos, pCmdGroup->uLastCmdPos);
                        break;
                    }
                    else {
                        /*All the cmd within this group has been processed*/
                        pCmdGroup->eState = TSP_CMD_GROUP_DONE;
                        TA_LOGI("Group[%d] state[%d]\n", pCmdGroup->uiGroupID, pCmdGroup->eState);
                        gTspCryptoLib.pCallback(pCmdGroup->hSess, pCmdGroup->uiGroupID);
                        DMX_LIST_REMOVE(gTspCryptoLib.pGroupList, pGroupNode);
                        dmx_free(pCmdGroup);
                        pGroupNode = NULL;
                    }
                }
                else if(pCmdGroup->uLastCmdPos < pCmdGroup->uFirstCmdPos) {
                    if((uCmdQRdOffset >= pCmdGroup->uFirstCmdPos) ||
                       (uCmdQRdOffset < pCmdGroup->uLastCmdPos))
                    {
                        /*The top group cmds is still in processing*/
                        //MX_INFO("Processing[uCmdQRdOffset=0x%x], [uFirstCmdPos=0x%x, uLastCmdPos=0x%x]\n",
                            //uCmdQRdOffset, pCmdGroup->uFirstCmdPos, pCmdGroup->uLastCmdPos);
                        break;
                    }
                    else {
                        /*All the cmd within this group has been processed*/
                        pCmdGroup->eState = TSP_CMD_GROUP_DONE;
                        TA_LOGI("Group[%d] state[%d]\n", pCmdGroup->uiGroupID, pCmdGroup->eState);
                        gTspCryptoLib.pCallback(pCmdGroup->hSess, pCmdGroup->uiGroupID);
                        DMX_LIST_REMOVE(gTspCryptoLib.pGroupList, pGroupNode);
                        dmx_free(pCmdGroup);
                        pGroupNode = NULL;
                    }
                }
                else {
                    /*(pCmdGroup->uLastCmdPhyAddr == pCmdGroup->uFirstCmdPhyAddr)*/
                    /* CMDQ is empty */
                }
            }
        }
    }

	return ret;
}

#if 0
static MV_OSAL_HANDLE_TASK_t hTask;

static void internal_task(void )
{
    while(1) {
        TSP_DRV_Lib_Process(1);
        MV_OSAL_Task_Sleep(1);
    }

}
#endif


HRESULT TSP_CryptoInit(void *pCallback, int32_t uFigoId, void *pCmdBaseBuf, UINT16 uCmdQueueBase,
    UINT16  uCmdQueueSize, UINT16 uCmdCntBase, UINT16 uLocalDataIn, UINT16 uLocalDataOut, UINT16 uM2MCryptoCmdBase)
{
    HRESULT ret = SUCCESS;

    TA_LOGH( "Enter TSP_CryptoInit\n");

    if(uFigoId >= TSP_FIGO_NR) {
        ret = ERR_ERRPARAM;
        goto error;
    }

    if(gTspCryptoLib.Libcfg[uFigoId].bInited == FALSE) {
        /*init cmd pool*/
        #ifndef TA_COMPILE
        M2M_BUFF_T *pM2MCmdBuf = pCmdBaseBuf;
        ret = AMP_SHM_GetVirtualAddress(pM2MCmdBuf->hShm, 0, &gTspCryptoLib.CmdQ[uFigoId].pCmdBaseVirtAddr);
        gTspCryptoLib.CmdQ[uFigoId].hCmdMem = pM2MCmdBuf->hShm;
        #else
        //in tz2.0, pCmdBaseVirtAddr=pCmdBasePhyAddr
        TA_M2M_BUFF_T *pM2MCmdBuf = pCmdBaseBuf;
        gTspCryptoLib.CmdQ[uFigoId].pCmdBaseVirtAddr = pM2MCmdBuf->virAddr;
        #endif


        gTspCryptoLib.CmdQ[uFigoId].uCmdQWrPos = 0;
        gTspCryptoLib.CmdQ[uFigoId].pCmdBasePhyAddr = pM2MCmdBuf->phyAddr;
        gTspCryptoLib.CmdQ[uFigoId].uSize = pM2MCmdBuf->uBufSize;

        gTspCryptoLib.Libcfg[uFigoId].uiMaxBlkSize = TSP_CRYPTO_BLOCK_MAXSIZE;
        gTspCryptoLib.Libcfg[uFigoId].uFigoSysId = uFigoId;
        gTspCryptoLib.Libcfg[uFigoId].uTaskId= 0xFFFFFFFF;

        gTspCryptoLib.Libcfg[uFigoId].bInited = TRUE;
    }
    gTspCryptoLib.pCallback = (TSP_CRYPTO_CB)pCallback;
    if(gTspCryptoLib.pGroupList == NULL) {
        gTspCryptoLib.pGroupList = DMX_LIST_CREATE(NULL, NULL);
        if(!gTspCryptoLib.pGroupList) {
            ret = ERR_OSALFAIL;
            goto error;
        }
    }
    TA_LOGH( "Figo(=%d) gTspCryptoLib init 0x%x, pCmdBasePhyAddr = %p\n",       \
            uFigoId, gTspCryptoLib.Libcfg[uFigoId].bInited, gTspCryptoLib.CmdQ[uFigoId].pCmdBasePhyAddr);
    TSPFW_CRYPTO_INIT(&gTspCryptoLib, uFigoId, uCmdQueueBase, uCmdQueueSize, uCmdCntBase, uLocalDataIn, uLocalDataOut, uM2MCryptoCmdBase);
    TA_LOGH("Success to Init\n");

    /*Add below process to handle figo breakpoint*/
    #if 0
    ret = MV_OSAL_Task_Create_Detailed(&hTask,
        internal_task,
        NULL,
        MV_OSAL_TASK_PRIORITY_15,
        "dmx_task",
        NULL, 0, 32);
    #endif

    return SUCCESS;
error:
    if(gTspCryptoLib.pGroupList) {
        DMX_LIST_DESTROY(gTspCryptoLib.pGroupList);
        gTspCryptoLib.pGroupList = NULL;
    }
    return ret;
}

HRESULT TSP_CryptoDeinit(int32_t uFigoId)
{
    VOID *pData = NULL;

    if(gTspCryptoLib.pGroupList) {
        while(DMX_LIST_POP_FRONT(gTspCryptoLib.pGroupList, (PVOID*)&pData) == 0) {
            if(pData) {
               dmx_free(pData);
            }
            else {
               break;
            }
        }
        DMX_LIST_DESTROY(gTspCryptoLib.pGroupList);
        gTspCryptoLib.pGroupList = NULL;
    }

    if(gTspCryptoLib.Libcfg[uFigoId].bInited == TRUE) {
        TSPFW_CRYPTO_DEINIT(&gTspCryptoLib, uFigoId);
        gTspCryptoLib.Libcfg[uFigoId].bInited = FALSE;
    }

    return SUCCESS;
}


HRESULT TSP_CryptoAllocFigoTask(int32_t uFigoId)
{
    return TSPFW_Crypto_AllocFigoTask(&gTspCryptoLib, uFigoId);
}

HRESULT TSP_CryptoFreeFigoTask(int32_t uFigoId)
{
    return TSPFW_Crypto_FreeFigoTask(&gTspCryptoLib, uFigoId);
}

HRESULT TSP_CryptoStartFw(int32_t *pFigoId)
{
    return TSPFW_Crypto_StartFW(pFigoId);
}

HRESULT TSP_CryptoStopFw(int32_t uFigoId)
{
    return TSPFW_Crypto_StopFW(uFigoId);
}

HRESULT TSP_CryptoProcess(M2M_TA_CRYPTO_CMD *pDatChunkCmd, int32_t uFigoId)
{
    HRESULT ret = SUCCESS;
    FIGODRV_HANDLE hFigoDrv = NULL;;

    if(uFigoId > TSP_FIGO_NR) {
        return ERR_HWSTATEWRONG;
    }
    hFigoDrv = current_figohandle(uFigoId);
    /*The 1st::split Data Chunk*/

    if(pDatChunkCmd != NULL) {
        ret = internal_FeedData(hFigoDrv, pDatChunkCmd);
    }

    return ret;
}

