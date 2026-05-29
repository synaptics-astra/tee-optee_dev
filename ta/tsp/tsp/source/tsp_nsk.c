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

// LOG_TAG shall be put before ta_log.h
// so that ta_log.h will use this tag for debug info
#define LOG_TAG "TSP_TA_NSK"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "tsp_ta_internal.h"
#include "tee_internal_api.h"
#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif
#include "tsp_cmd.h"
#include "tsp_config_log.h"
#include "ta_log.h"
#include "tsp_nsk.h"
#include "tsp_fw_spec.h"
#include "tsp.h"
#include "tsp_nsk_alg.h"
#include "tsp_figo.h"

#define NSK_CMCHANNEL_NUM_WHOLE_TSP 64

#define VALIDATE_NSK_ALGO(uAlgoIdx) \
    do{ \
      if(uAlgoIdx < DVB_CSA2 || \
         (uAlgoIdx > IDSA_AES && uAlgoIdx < NSA_DSS_MDI_MSC4)|| \
         uAlgoIdx > DIRECTV_DSS_DES){ \
           TA_LOGE( "Invaild algo index 0x%x\n",uAlgoIdx); \
           return  TEE_ERROR_BAD_PARAMETERS; \
      } \
    }while(0)

#define VALIDATE_NSK_SCB(scb) \
    do{ \
      if(scb != SCB_OUT_EVEN && \
         scb != SCB_OUT_ODD && \
         scb != SCB_OUT_CLEAR){ \
           TA_LOGE( "Invaild scb 0x%x\n",scb); \
           return  TEE_ERROR_BAD_PARAMETERS; \
      } \
    }while(0)

#define IS_VALID_NSK_KEY(keyIdx)  \
    (keyIdx < (TspKeyTblAdr_Nsk2M2mKeyTblBase - TspKeyTblAdr_Nsk2TsKeyTblBase))
#define VALIDATE_NSK_KEY(keyIdx) \
    do{ \
      if(!IS_VALID_NSK_KEY(keyIdx)){ \
           TA_LOGE( "Invaild keyIdx 0x%x\n",keyIdx); \
           return  TEE_ERROR_BAD_PARAMETERS; \
      } \
    }while(0)

static NSK_CmChanInfo NskCmChannelArray[NSK_CMCHANNEL_NUM_WHOLE_TSP] = {0};

static void dump_OneCrypto(NSKOneCrypto *pCrypto) {
    TA_LOGH("Crypto Valid Mask: 0x%x\n", pCrypto->validMask);
    if (pCrypto->validMask & ALGO_MASK) {
        TA_LOGH("Algo: %d\n", pCrypto->algoId);
    }
}

static void dump_OneParity(NSKOneParity *pParity) {
    TA_LOGH("Parity Valid Mask: 0x%x\n", pParity->validMask);
    if (pParity->validMask & KEY_INDEX_MASK) {
        TA_LOGH("Key Index: 0x%x\n", pParity->keyIdx);
    }
    if (pParity->validMask & SCB_OUT_MASK) {
        TA_LOGH("SCB Out: 0x%x\n", pParity->scbOut);
    }
    if (pParity->validMask & LOCAL_D_MASK) {
        TA_LOGH("LocalD:\n");
        dump_OneCrypto(&pParity->localD);
    }
    if (pParity->validMask & ESA_MASK) {
        TA_LOGH("ESA:\n");
        dump_OneCrypto(&pParity->ESA);
    }
    if (pParity->validMask & LOCAL_S_MASK) {
        TA_LOGH("LocalS:\n");
        dump_OneCrypto(&pParity->localS);
    }
}

static void dump_CmChannel(NSKCmChannel *pChannel) {
    TA_LOGH("CmChannel Valid Mask: 0x%x\n", pChannel->validMask);
    if (pChannel->validMask & EVEN_PARITY_MASK) {
        TA_LOGH("Even Parity:\n");
        dump_OneParity(&pChannel->even);
    }
    if (pChannel->validMask & ODD_PARITY_MASK) {
        TA_LOGH("Odd Parity:\n");
        dump_OneParity(&pChannel->odd);
    }
    if (pChannel->validMask & CLEAR_PARITY_MASK) {
        TA_LOGH("Clear Parity:\n");
        dump_OneParity(&pChannel->clear);
    }
}

static TEE_Result tsp_nsk_validateParity(NSKOneParity* pParity) {
    TEE_Result ret = TEE_SUCCESS;

    if(pParity->validMask & KEY_INDEX_MASK) {
        //currently nsk ta will set -1, so bypass this check, add this check when load to dtcm
        //VALIDATE_NSK_KEY(pParity->keyIdx);
    }
    if(pParity->validMask & SCB_OUT_MASK) {
        VALIDATE_NSK_SCB(pParity->scbOut);
    }
    if(pParity->validMask & LOCAL_D_MASK) {
        if(pParity->localD.validMask & ALGO_MASK) {
            VALIDATE_NSK_ALGO(pParity->localD.algoId);
        }
    }
    if(pParity->validMask & ESA_MASK) {
        if(pParity->ESA.validMask & ALGO_MASK) {
            VALIDATE_NSK_ALGO(pParity->ESA.algoId);
        }
    }
    if(pParity->validMask & LOCAL_S_MASK) {
        if(pParity->localS.validMask & ALGO_MASK) {
            VALIDATE_NSK_ALGO(pParity->localS.algoId);
        }
    }

    return ret;
}

static TEE_Result tsp_nsk_validateCmChan(NSKCmChannel* cmChanSrc) {
    TEE_Result ret = TEE_SUCCESS;

    //even Parity
    if(cmChanSrc->validMask & EVEN_PARITY_MASK) {
        ret = tsp_nsk_validateParity(&cmChanSrc->even);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("validate Even Parity failed\n");
            goto FAIL_END;
        }
    }

    //odd Parity
    if(cmChanSrc->validMask & ODD_PARITY_MASK) {
        ret = tsp_nsk_validateParity(&cmChanSrc->odd);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("validate odd Parity failed\n");
            goto FAIL_END;
        }
    }

    //clear Parity
    if(cmChanSrc->validMask & CLEAR_PARITY_MASK) {
        ret = tsp_nsk_validateParity(&cmChanSrc->clear);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("validate clear Parity failed\n");
            goto FAIL_END;
        }
    }

FAIL_END:
    return ret;
}

static uint32_t tsp_nsk_getCmchanKeyUpdateMask(NSKCmChannel *pCmChanNew,
                                                             NSKCmChannel *pCmChanOld) {
    uint32_t keyUpdateMask = 0;

    //even key slot
    if(pCmChanNew->validMask & EVEN_PARITY_MASK) {
        if((pCmChanNew->even.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChanNew->even.keyIdx)) {
            if(pCmChanOld && (pCmChanOld->validMask & EVEN_PARITY_MASK) &&
                (pCmChanOld->even.validMask & KEY_INDEX_MASK) &&
                (pCmChanNew->even.keyIdx == pCmChanOld->even.keyIdx)){
            }else
                keyUpdateMask |= EVEN_PARITY_MASK;
        }
    }

    //odd key slot
    if(pCmChanNew->validMask & ODD_PARITY_MASK) {
        if((pCmChanNew->odd.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChanNew->odd.keyIdx)) {
            if(pCmChanOld && (pCmChanOld->validMask & ODD_PARITY_MASK) &&
                (pCmChanOld->odd.validMask & KEY_INDEX_MASK) &&
                (pCmChanNew->odd.keyIdx == pCmChanOld->odd.keyIdx)){
            }else
                keyUpdateMask |= ODD_PARITY_MASK;
        }
    }

    //clear key slot
    if(pCmChanNew->validMask & CLEAR_PARITY_MASK) {
        if((pCmChanNew->clear.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChanNew->clear.keyIdx)) {
            if(pCmChanOld && (pCmChanOld->validMask & CLEAR_PARITY_MASK) &&
                (pCmChanOld->clear.validMask & KEY_INDEX_MASK) &&
                (pCmChanNew->clear.keyIdx == pCmChanOld->clear.keyIdx)){
            }else
                keyUpdateMask |= CLEAR_PARITY_MASK;
        }
    }

    return keyUpdateMask;
}

static TEE_Result tsp_nsk_checkCmchanCheckerCtx(SIE_CmchannelCheckerCtx *pCtx) {
    uint32_t  uThresholdMs = 100, uCurrentMs = 0;

    while(pCtx->u_bNeedUpdate) {
        TEE_Wait(1);
        if((++uCurrentMs) == uThresholdMs) {
           TA_LOGE("CmchannelCheckerCtx(cmchannelIdx=0x%x) timeout(0x%x ms)\n",
                    pCtx->u_cmchannelIdx, uThresholdMs);
           return TEE_ERROR_TIMEOUT;
        }
    }

    return TEE_SUCCESS;
}

/*
 * tsp_nsk_updateCmchanProp will trigger FW to get the key property
 * this Api will be called in below two cases
 * 1.get cmchannel usage and LowerDest before TSPTA_ConnectNSKCmChannel which write cmchannel context to dtcm
 * 2. call TSPTA_UpdateNSKCmChannel  after TSPTA_ConnectNSKCmChannel
 * case 1 is sync mode, case 2 is async mode which uses tsp interrupt to notify dmx
 */
static TEE_Result tsp_nsk_updateCmchanProp(uint32_t uCmChIndex, uint32_t keyUpdateMask,
                                                      uint32_t figo_id, bool bSync, bool bTriggerFWInt) {
    TEE_Result ret = TEE_SUCCESS;
    NSKCmChannel* pCmChan = &NskCmChannelArray[uCmChIndex].cmChan;
    SIE_SecureCtx *pSecureCtx =  (SIE_SecureCtx*)tspmem_phy_to_virt(TSP_FIGO_REG_BASE +
                                  (TSP_FIGO_REG_SIZE*figo_id) + Secure_Define_Dtcm_Base);
    SIE_CmchannelCheckerCtx *pCmchanCheckerCtx = &pSecureCtx->ie_CmchannelCheckerCtx;

    TA_LOGH("update cmchan(%d) property(keyUpdateMask: 0x%x) to figo %d, syncMode: %d, bSendInt: %d\n",
                uCmChIndex, keyUpdateMask, figo_id, bSync, bTriggerFWInt);
    ret = tsp_nsk_checkCmchanCheckerCtx(pCmchanCheckerCtx);
    if(ret != TEE_SUCCESS) {
        TA_LOGE("tsp_nsk_updateCmchanProp last state error, ret=0x%x\n", ret);
        goto FAIL_END;
    }

    if(keyUpdateMask) {
        TEE_MemFill(pCmchanCheckerCtx, 0, sizeof(SIE_CmchannelCheckerCtx));
        if(keyUpdateMask & EVEN_PARITY_MASK) {
            pCmchanCheckerCtx->u_evenKeySlotIdx = pCmChan->even.keyIdx | TspKeyTblAdr_Nsk2TsKeyTblBase;
            pCmchanCheckerCtx->u_bUpdateEvenKey = 1;
        }
        if(keyUpdateMask & ODD_PARITY_MASK) {
            pCmchanCheckerCtx->u_oddKeySlotIdx = pCmChan->odd.keyIdx | TspKeyTblAdr_Nsk2TsKeyTblBase;
            pCmchanCheckerCtx->u_bUpdateOddKey = 1;
        }
        if(keyUpdateMask & CLEAR_PARITY_MASK) {
            pCmchanCheckerCtx->u_clearKeySlotIdx = pCmChan->clear.keyIdx | TspKeyTblAdr_Nsk2TsKeyTblBase;
            pCmchanCheckerCtx->u_bUpdateClearKey = 1;
        }
        pCmchanCheckerCtx->u_cmchannelIdx = uCmChIndex;
        pCmchanCheckerCtx->u_bNeedCallback = bTriggerFWInt? 1:0;
        pCmchanCheckerCtx->u_bNeedUpdate = 1;

        if(bSync) {
            ret = tsp_nsk_checkCmchanCheckerCtx(pCmchanCheckerCtx);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("tsp_nsk_updateCmchanProp new state error, ret=0x%x\n", ret);
                goto FAIL_END;
            }
        }
        NskCmChannelArray[uCmChIndex].bGetLowerDest[figo_id] = true;
    }

FAIL_END:
    return ret;
}

static uint32_t tsp_nsk_getCmchanLowerDest(TEE_TspContext* pTspCtx,
                                                       uint32_t uCmChIndex,
                                                       uint32_t keyIdx) {
    TEE_Result ret = TEE_SUCCESS;
    uint32_t bLowerDest = 0;
    SIE_SecureCtx* pSecureCtx = pTspCtx->pSecureCtx;
    SIE_CmchannelCheckerCtx *pCmchanCheckerCtx = &pSecureCtx->ie_CmchannelCheckerCtx;

    if(pCmchanCheckerCtx->u_bNeedUpdate && (pCmchanCheckerCtx->u_cmchannelIdx == uCmChIndex)) {
        ret = tsp_nsk_checkCmchanCheckerCtx(pCmchanCheckerCtx);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("uCmChIndex(0x%x) was not finished updated, ret=0x%x\n",uCmChIndex, ret);
            return 0;
        }
    }

    keyIdx = (keyIdx>>6); //keyIdx /=0x40;
    uintptr_t pLowerDestTmp = (uintptr_t)(&pSecureCtx->ie_NSKKeyLowerDestTable[keyIdx>>5]);
    bLowerDest = (*(unsigned int *)pLowerDestTmp >> (keyIdx & 0x1f))&0x1;
    return bLowerDest;
}

static int32_t tsp_nsk_allocateAndCopyIV(uint32_t figo_id,
        unsigned char* pIVSrc, uint32_t* ivAddrDest) {
    TEE_Result ret = TEE_SUCCESS;
    void *pHeap = NULL;
    int32_t allocateIV = -1;
    void * ivAddrVA = NULL;

    if(*ivAddrDest == 0) {
        pHeap = tsp_get_dtcmHeap(figo_id, DTCM_MEM_TYPE_NONSECURE);
        if(pHeap == NULL){
            ret = TEE_ERROR_ITEM_NOT_FOUND;
            goto error;
        }
        allocateIV = dtcm_mem_alloc(pHeap, IV_LENGTH, 8);
        if(allocateIV == -1) {
            TA_LOGH("allocate IV in figo %d dtcm failed\n", figo_id);
            ret= TEE_ERROR_OUT_OF_MEMORY;
            goto error;
        }
        *ivAddrDest = allocateIV;
    }

    TA_LOGH("%s: allocate IV 0x%x in figo %d dtcm, pIVSrc %p\n",
        __FUNCTION__, (unsigned int)*ivAddrDest, figo_id, pIVSrc);
    ivAddrVA = (void*)tspmem_phy_to_virt(TSP_FIGO_REG_BASE + (TSP_FIGO_REG_SIZE*figo_id) + *ivAddrDest);
    if(pIVSrc != NULL) {
        memcpy(ivAddrVA, pIVSrc, IV_LENGTH);
    } else {
        // pIVSrc == NULL means fixed zero IV
        TEE_MemFill(ivAddrVA, 0, IV_LENGTH);
    }

error:
    return ret;
}


#define NSK_FREE_IV(pHeap, a)          \
    do {                               \
        if(a) dtcm_mem_free(pHeap, a); \
    } while(0)

static void tsp_nsk_ClearKeySlotInDTCM(SIE_NSKKeySlotCtx* pKeySlot, void *pHeap) {

    NSK_FREE_IV(pHeap, pKeySlot->u_localDIV1);
    NSK_FREE_IV(pHeap, pKeySlot->u_localDIV2);

    NSK_FREE_IV(pHeap, pKeySlot->u_ESAIV1);
    NSK_FREE_IV(pHeap, pKeySlot->u_ESAIV2);

    NSK_FREE_IV(pHeap, pKeySlot->u_localSIV1);
    NSK_FREE_IV(pHeap, pKeySlot->u_localSIV2);
}


static TEE_Result tsp_nsk_ClearCmChanInDTCM(uint32_t figo_id,
        uint16_t cmChanDtcmAddr) {
    TEE_Result ret = TEE_SUCCESS;
    SIE_CmChannelCtx* cmChanDest;
    void *pHeap = NULL;

    pHeap = tsp_get_dtcmHeap(figo_id, DTCM_MEM_TYPE_NONSECURE);
    if(pHeap == NULL){
        ret = TEE_ERROR_ITEM_NOT_FOUND;
        goto FAIL_END;
    }

    cmChanDest = (SIE_CmChannelCtx*)tspmem_phy_to_virt(TSP_FIGO_REG_BASE + (TSP_FIGO_REG_SIZE*figo_id) + cmChanDtcmAddr);
    TA_LOGH("%s to clear figo %d cmChanDest %p\n", __FUNCTION__, figo_id, cmChanDest);

    if(cmChanDest != NULL) {
        tsp_nsk_ClearKeySlotInDTCM(&(cmChanDest->ie_NSKEvenKeySlot),  pHeap);
        tsp_nsk_ClearKeySlotInDTCM(&(cmChanDest->ie_NSKOddKeySlot),   pHeap);
        tsp_nsk_ClearKeySlotInDTCM(&(cmChanDest->ie_NSKClearKeySlot), pHeap);
        TEE_MemFill(cmChanDest, 0, sizeof(SIE_CmChannelCtx));
    }

FAIL_END:
    return ret;
}


static TEE_Result tsp_nsk_fillKeySlottoDTCM(uint32_t figo_id,
        NSKOneParity* pParity, SIE_NSKKeySlotCtx* pKeySlot) {
    TEE_Result ret = TEE_SUCCESS;
    uint32_t ivAddr = 0;

    if(pParity->validMask & SCB_OUT_MASK) {
        pKeySlot->u_SCB_enable = 1;
        pKeySlot->u_SCB = pParity->scbOut;
    }
    if(pParity->validMask & LOCAL_D_MASK) {
        if(pParity->localD.validMask & ALGO_MASK) {
            pKeySlot->u_localDesAlgoIndex = pParity->localD.algoId;
            //force to set algo? if not move this enable outside
            pKeySlot->u_localD_enable = 1;
        }
        if(pParity->localD.validMask & IV1_MASK) {
            ivAddr = pKeySlot->u_localDIV1;
            ret = tsp_nsk_allocateAndCopyIV(figo_id, pParity->localD.iv1, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot LocalD IV1 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_localDIV1 = ivAddr;
        //allocate IV for CryptoAlgorithmElement_IV_ClearIV_with_fixed_value
        } else if (TSPTA_shallAllocateFixedZeroIV(pParity->localD.algoId)) {
            ivAddr = pKeySlot->u_localDIV1;
            ret = tsp_nsk_allocateAndCopyIV(figo_id, NULL, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot LocalD Fixed zero IV1 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_localDIV1 = ivAddr;
        }
        if(pParity->localD.validMask & IV2_MASK) {
            ivAddr = pKeySlot->u_localDIV2;
            ret = tsp_nsk_allocateAndCopyIV(figo_id, pParity->localD.iv2, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot LocalD IV2 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_localDIV2 = ivAddr;
        }
    }
    if(pParity->validMask & ESA_MASK) {
        if(pParity->ESA.validMask & ALGO_MASK) {
            pKeySlot->u_ESAAlgoIndex = pParity->ESA.algoId;
            pKeySlot->u_ESA_enable = 1;
        }
        if(pParity->ESA.validMask & IV1_MASK) {
            ivAddr = pKeySlot->u_ESAIV1;
            ret = tsp_nsk_allocateAndCopyIV(figo_id, pParity->ESA.iv1, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot ESA IV1 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_ESAIV1 = ivAddr;

        //allocate IV for CryptoAlgorithmElement_IV_ClearIV_with_fixed_value
        } else if (TSPTA_shallAllocateFixedZeroIV(pParity->ESA.algoId)) {
            ivAddr = pKeySlot->u_ESAIV1;
            TA_LOGI("%s to allocate key slot ESA Fixed zero IV1 0x%x\n", __FUNCTION__, (unsigned int)ivAddr);
            ret = tsp_nsk_allocateAndCopyIV(figo_id, NULL, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot ESA Fixed zero IV1 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_ESAIV1 = ivAddr;
            TA_LOGI("%s allocated key slot ESA Fixed zero IV1 0x%x\n", __FUNCTION__, (unsigned int)ivAddr);
        }
        if(pParity->ESA.validMask & IV2_MASK) {
            ivAddr = pKeySlot->u_ESAIV2;
            ret = tsp_nsk_allocateAndCopyIV(figo_id, pParity->ESA.iv2, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot ESA IV2 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_ESAIV2 = ivAddr;
        }
    }
    if(pParity->validMask & LOCAL_S_MASK) {
        if(pParity->localS.validMask & ALGO_MASK) {
            pKeySlot->u_localScramAlgoIndex = pParity->localS.algoId;
            pKeySlot->u_localS_enable = 1;
        }
        if(pParity->localS.validMask & IV1_MASK) {
            ivAddr = pKeySlot->u_localSIV1;
            ret = tsp_nsk_allocateAndCopyIV(figo_id, pParity->localS.iv1, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot LocalS IV1 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_localSIV1 = ivAddr;
        //allocate IV for CryptoAlgorithmElement_IV_ClearIV_with_fixed_value
        } else if (TSPTA_shallAllocateFixedZeroIV(pParity->localS.algoId)) {
            ivAddr = pKeySlot->u_localSIV1;
            ret = tsp_nsk_allocateAndCopyIV(figo_id, NULL, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot LocalS Fixed zero IV1 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_localSIV1 = ivAddr;
        }
        if(pParity->localS.validMask & IV2_MASK) {
            ivAddr = pKeySlot->u_localSIV2;
            ret = tsp_nsk_allocateAndCopyIV(figo_id, pParity->localS.iv2, &ivAddr);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("allocate key slot LocalS IV2 failed\n");
                goto FAIL_END;
            }
            pKeySlot->u_localSIV2 = ivAddr;
        }
    }

    if(pParity->validMask & KEY_INDEX_MASK) {
        if(pKeySlot->u_localD_enable || pKeySlot->u_ESA_enable ||pKeySlot->u_localS_enable) {
            //VALIDATE_NSK_KEY(pParity->keyIdx); //still will be -1 here, will check with indian team
            pKeySlot->u_pKeySlotIndex = pParity->keyIdx | TspKeyTblAdr_Nsk2TsKeyTblBase;
        }else {
            TA_LOGE("not set any AlgoIndex to match the keyindex 0x%x\n", pParity->keyIdx);
            ret = TEE_ERROR_BAD_PARAMETERS;
            goto FAIL_END;
        }
    }

FAIL_END:
    return ret;
}


static TEE_Result tsp_nsk_fillCmChantoDTCM(uint32_t figo_id,
        NSKCmChannel* cmChanSrc, uint16_t cmChanDtcmAddr) {
    SIE_CmChannelCtx* cmChanDest;
    TEE_Result ret = TEE_SUCCESS;

    cmChanDest = (SIE_CmChannelCtx*)tspmem_phy_to_virt(TSP_FIGO_REG_BASE + (TSP_FIGO_REG_SIZE*figo_id) + cmChanDtcmAddr);

    //even key slot
    if(cmChanSrc->validMask & EVEN_PARITY_MASK) {
        ret = tsp_nsk_fillKeySlottoDTCM(figo_id, &cmChanSrc->even, &cmChanDest->ie_NSKEvenKeySlot);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("allocate Even key slot IVs failed\n");
            goto FAIL_END;
        }
    }

    //odd key slot
    if(cmChanSrc->validMask & ODD_PARITY_MASK) {
        ret = tsp_nsk_fillKeySlottoDTCM(figo_id, &cmChanSrc->odd, &cmChanDest->ie_NSKOddKeySlot);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("allocate Odd key slot IVs failed\n");
            goto FAIL_END;
        }
    }

    //clear key slot
    if(cmChanSrc->validMask & CLEAR_PARITY_MASK) {
        ret = tsp_nsk_fillKeySlottoDTCM(figo_id, &cmChanSrc->clear, &cmChanDest->ie_NSKClearKeySlot);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("allocate Clear key slot IVs failed\n");
            goto FAIL_END;
        }
    }

FAIL_END:
    return ret;
}

static TEE_Result tsp_nsk_getInputCmchannel(uint32_t paramTypes, TEE_Param params[4],
                                            NSKCmChannel **pCmChanSrc, uint32_t *pMemId)
{
    TEE_Result ret;
    uint32_t uInSize = 0;
    size_t size;

    if (TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    if (TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_OUTPUT) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    if (params[1].value.a == TSP_MEMREF_FROM_CA) {
        if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) {
            return TEE_ERROR_BAD_PARAMETERS;
        }
        *pCmChanSrc = (NSKCmChannel*)params[0].memref.buffer;
        uInSize = params[0].memref.size;
        *pMemId = 0;
    } else if (params[1].value.a == TSP_SHM_FROM_TA) {
        if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
            return TEE_ERROR_BAD_PARAMETERS;
        }
#ifdef CONFIG_OPTEE
        ret = TEE_Map_Memory(params[0].value.a, (void **)pCmChanSrc, &size);
#else
        ret = TEE_MapSharedMemory(params[0].value.a, (void **)pCmChanSrc, &size);
#endif
        if (ret != TEE_SUCCESS) {
            TA_LOGF("TEE_MapSharedMemory failed, ret = 0x%x\n", ret);
            return ret;
        }
        *pMemId = params[0].value.a;
        uInSize = params[0].value.b;
    } else {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    if(uInSize != sizeof(NSKCmChannel)) {
        TA_LOGE("Input memref size 0x%x, doesn't align to NSKCmChannel size 0x%lx\n",
                uInSize, sizeof(NSKCmChannel));
        if (*pMemId) {
#ifdef CONFIG_OPTEE
            TEE_Unmap_Memory(*pMemId);
#else
            TEE_UnmapSharedMemory(*pMemId);
#endif
        }
        return TEE_ERROR_BAD_PARAMETERS;
    }

    return TEE_SUCCESS;
}

static void tsp_nsk_unmapShm(uint32_t *pMemId)
{
    if (*pMemId) {
#ifdef CONFIG_OPTEE
        TEE_Unmap_Memory(*pMemId);
#else
        TEE_UnmapSharedMemory(*pMemId);
#endif
    }
}

TEE_Result TSPTA_AddNSKCmChannel(void* sessionContext,
                                        uint32_t  paramTypes,
                                        TEE_Param params[4])
{
    TEE_Result ret = TEE_SUCCESS;
    uint32_t memId = 0;
    NSKCmChannel *cmChanSrc = NULL;
    NSK_CmChanInfo* cmChanDest = NULL;
    uint32_t i = 0, CmChanSize = sizeof(NSKCmChannel);

    ret = tsp_nsk_getInputCmchannel(paramTypes, params, &cmChanSrc, &memId);
    if (ret != TEE_SUCCESS) {
        return ret;
    }

    params[2].value.a = -1;

    ret = tsp_nsk_validateCmChan(cmChanSrc);
    if (ret != TEE_SUCCESS) {
        return ret;
    }

    cmChanDest = &NskCmChannelArray[0];
    for(i = 0; i < NSK_CMCHANNEL_NUM_WHOLE_TSP; i++, cmChanDest++) {
        if(cmChanDest->cmChan.validMask == 0) {
            memcpy(&(cmChanDest->cmChan), cmChanSrc, CmChanSize);
            params[2].value.a = i;
            TA_LOGH("find index %d to store the CmChannel 0x%x(validmask)\n", i, cmChanSrc->validMask);
            dump_CmChannel(cmChanSrc);
            tsp_nsk_unmapShm(&memId);
            return TEE_SUCCESS;
        }
    }

    tsp_nsk_unmapShm(&memId);
    return TEE_ERROR_OVERFLOW;
}


TEE_Result TSPTA_RmNSKCmChannel(void* sessionContext,
                                       uint32_t  paramTypes,
                                       TEE_Param params[4])
{
    uint32_t uCmChIndex = 0, connCnt = 0, i = 0;
    NSK_CmChanInfo* cmChan = NULL;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    uCmChIndex = params[0].value.a;
    if(uCmChIndex >= NSK_CMCHANNEL_NUM_WHOLE_TSP) {
        TA_LOGE("CmChannel index %d is wrong\n", uCmChIndex);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    cmChan = &NskCmChannelArray[uCmChIndex];
    for(; i < TSP_MAX_FIGO_NUM; i++)
        connCnt += cmChan->connCnt[i];

    TA_LOGH("%s remove cmchan %d, connCnt %d\n", __FUNCTION__, uCmChIndex, connCnt);
    if(connCnt != 0) {
        TA_LOGE("CmChannelIndex %d is still connecting with / used by %d Pids\n", uCmChIndex, connCnt);
        return TEE_ERROR_ACCESS_DENIED;
    }

    memset(cmChan, 0, sizeof(NSK_CmChanInfo));
    return TEE_SUCCESS;
}


TEE_Result TSPTA_UpdateNSKCmChannel(void* sessionContext,
                                           uint32_t  paramTypes,
                                           TEE_Param params[4])
{
    TEE_Result ret = TEE_SUCCESS;
    uint32_t memId = 0;
    NSKCmChannel* cmChanSrc = NULL;
    NSK_CmChanInfo* cmChanInfo = NULL;
    uint32_t uCmChIndex = 0, CmChanSize = sizeof(NSKCmChannel);
    uint32_t filledRet[TSP_MAX_FIGO_NUM] = {0};
    uint32_t i = 0;
    uint32_t keyUpdateMask = 0;
    bool bTriggerFWInt = true;

    ret = tsp_nsk_getInputCmchannel(paramTypes, params, &cmChanSrc, &memId);
    if (ret != TEE_SUCCESS) {
        return ret;
    }

    ret = tsp_nsk_validateCmChan(cmChanSrc);
    if (ret != TEE_SUCCESS) {
        return ret;
    }

    uCmChIndex = params[2].value.a;
    if(uCmChIndex >= NSK_CMCHANNEL_NUM_WHOLE_TSP) {
        TA_LOGE("CmChannel index %d is wrong\n", uCmChIndex);
        tsp_nsk_unmapShm(&memId);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    TA_LOGH("%s update cmchan %d\n", __FUNCTION__, uCmChIndex);
    dump_CmChannel(cmChanSrc);
    cmChanInfo = &NskCmChannelArray[uCmChIndex];
    keyUpdateMask = tsp_nsk_getCmchanKeyUpdateMask(cmChanSrc, &(cmChanInfo->cmChan));
    memcpy(&(cmChanInfo->cmChan), cmChanSrc, CmChanSize);

    //check if associated with pid and applied in DTCM;
    for(i = 0; i < TSP_MAX_FIGO_NUM; i++) {
        //check same figo_id to avoid redundant update
        if(cmChanInfo->connCnt[i]) {
            TA_LOGH("fill cmchannel %d to figo %d DTCM addr 0x%x\n",
                uCmChIndex, i, cmChanInfo->dtcmAddr[i]);

            if(keyUpdateMask) {
                ret = tsp_nsk_updateCmchanProp(uCmChIndex, keyUpdateMask, i, false, bTriggerFWInt);
                if(ret != TEE_SUCCESS) {
                    TA_LOGE("tsp_nsk_updateCmchanProp in UpdateNSKCmChannel fail\n");
                }else {
                    bTriggerFWInt = false; //only trigger one figo to callback interrupt
                }
            }

            ret = tsp_nsk_fillCmChantoDTCM(i, &cmChanInfo->cmChan, cmChanInfo->dtcmAddr[i]);
            filledRet[i] = ret;
            if(ret != TEE_SUCCESS) {
                TA_LOGE("fail to fill cmchannel %d to figo %d DTCM addr 0x%x, ret 0x%x\n",
                    uCmChIndex, i, cmChanInfo->dtcmAddr[i], ret);
            }
        }
    }

    for(i = 0; i < TSP_MAX_FIGO_NUM; i++) {
        ret |= filledRet[i];
    }
    tsp_nsk_unmapShm(&memId);
    return ret;
}


/*
 * Connect CmChannel with PIDCtx, copy CmChannel to DTCM Heap, return the addr.
 */
TEE_Result TSPTA_ConnectNSKCmChannel(void* sessionContext,
                                       uint32_t  paramTypes,
                                       TEE_Param params[4])
{
    uint32_t figo_id = params[0].value.b;
    uint32_t uCmChIndex = params[0].value.a;
    TEE_Result ret = TEE_SUCCESS;
    void *pHeap;
    intptr_t cmChanDtcmAddr = 0;
    NSK_CmChanInfo* cmChanInfo = NULL;
    SIE_CmChannelCtx* cmChanDest;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    params[1].value.a = -1;

    if(uCmChIndex >= NSK_CMCHANNEL_NUM_WHOLE_TSP) {
        TA_LOGE("CmChannel index %d is wrong\n", uCmChIndex);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    cmChanInfo = &NskCmChannelArray[uCmChIndex];

    TA_LOGH("%s To connect uCmChIndex %d to pid of figo %d, connCnt %d\n",
        __FUNCTION__, uCmChIndex, figo_id, cmChanInfo->connCnt[figo_id]);
    if(cmChanInfo->connCnt[figo_id] != 0) {
        TA_LOGH("dtcm addr 0x%x for cmchan %d is already allocated in figo %d\n",
            cmChanInfo->dtcmAddr[figo_id], uCmChIndex, figo_id);
    } else {
        //allocate in new figo_id's dtcm
        pHeap = tsp_get_dtcmHeap(figo_id, DTCM_MEM_TYPE_NONSECURE);
        if(pHeap == NULL){
            return TEE_ERROR_ITEM_NOT_FOUND;
        }

        cmChanDtcmAddr = dtcm_mem_alloc(pHeap, sizeof(SIE_CmChannelCtx), 8);
        if(cmChanDtcmAddr == -1) {
            TA_LOGH("allocate dtcm addr 0x%lx failed\n", cmChanDtcmAddr);
            return TEE_ERROR_OUT_OF_MEMORY;
        }
        TA_LOGH("allocate dtcm addr 0x%lx for cmchannel %d\n", cmChanDtcmAddr, uCmChIndex);
        cmChanDest = (SIE_CmChannelCtx*)tspmem_phy_to_virt(TSP_FIGO_REG_BASE + (TSP_FIGO_REG_SIZE*figo_id) + cmChanDtcmAddr);
        TEE_MemFill(cmChanDest, 0, sizeof(SIE_CmChannelCtx));

        ret = tsp_nsk_fillCmChantoDTCM(figo_id, &(cmChanInfo->cmChan), cmChanDtcmAddr);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("fail to fill cmchannel %d to figo %d DTCM, ret 0x%x\n", uCmChIndex, figo_id, ret);
            tsp_nsk_ClearCmChanInDTCM(figo_id, cmChanDtcmAddr);
            dtcm_mem_free(pHeap, cmChanDtcmAddr);
            return ret;
        }
        cmChanInfo->dtcmAddr[figo_id] = cmChanDtcmAddr;
    }
    params[1].value.a = cmChanInfo->dtcmAddr[figo_id];
    cmChanInfo->connCnt[figo_id]++;
    return TEE_SUCCESS;
}


TEE_Result TSPTA_DisconnectNSKCmChannel(void* sessionContext,
                                       uint32_t  paramTypes,
                                       TEE_Param params[4])
{
    uint32_t figo_id = params[0].value.b;
    uint32_t uCmChIndex = params[0].value.a;
    uint32_t uCmChanDtcmAddr = 0;
    void *pHeap;
    NSK_CmChanInfo* cmChanInfo = NULL;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
        return TEE_ERROR_BAD_PARAMETERS;

    if(uCmChIndex >= NSK_CMCHANNEL_NUM_WHOLE_TSP) {
        TA_LOGE("CmChannel index %d is wrong\n", uCmChIndex);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    cmChanInfo = &NskCmChannelArray[uCmChIndex];
    uCmChanDtcmAddr = cmChanInfo->dtcmAddr[figo_id];

    TA_LOGH("%s To disconnect uCmChIndex %d, DTCM addr 0x%x from figo %d, connCnt %d\n",
        __FUNCTION__, uCmChIndex, uCmChanDtcmAddr, figo_id, cmChanInfo->connCnt[figo_id]);

    cmChanInfo->connCnt[figo_id]--;
    if(cmChanInfo->connCnt[figo_id] == 0) {
        //clear in DTCM
        TA_LOGH("clear and free the cmchan 0x%x in figo %d, connCnt %d\n",
            uCmChanDtcmAddr, figo_id, cmChanInfo->connCnt[figo_id]);
        tsp_nsk_ClearCmChanInDTCM(figo_id, uCmChanDtcmAddr);
        pHeap = tsp_get_dtcmHeap(figo_id, DTCM_MEM_TYPE_NONSECURE);
        if(pHeap == NULL){
            return TEE_ERROR_ITEM_NOT_FOUND;
        }
        dtcm_mem_free(pHeap, uCmChanDtcmAddr);
    }
    else if(cmChanInfo->connCnt[figo_id] > 0) {
        TA_LOGH("the cmchan 0x%x is still referred by %d PIDs\n",
            uCmChanDtcmAddr, cmChanInfo->connCnt[figo_id]);
    }
    else if(cmChanInfo->connCnt[figo_id] < 0) {
        TA_LOGE("the cmchan 0x%x is disconnected again even its connCnt is %d\n",
            uCmChanDtcmAddr, cmChanInfo->connCnt[figo_id]+1);
        cmChanInfo->connCnt[figo_id] = 0;
        return TEE_ERROR_ITEM_NOT_FOUND;
    }

    return TEE_SUCCESS;
}

TEE_Result TSPTA_GetNSKCmChannelProp(void* sessionContext,
                                       uint32_t  paramTypes,
                                       TEE_Param params[4])
{
    uint32_t uCmChIndex = params[0].value.a;
    uint32_t uFigoId = params[0].value.b;
    TEE_Result ret = TEE_SUCCESS;
    NSKCmChannel* pCmChan;
    uint32_t keyUsage = 0;
    uint32_t keyMask = 0;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    params[1].value.a = 0;

    if(uCmChIndex >= NSK_CMCHANNEL_NUM_WHOLE_TSP) {
        TA_LOGE("CmChannel index %d is wrong\n", uCmChIndex);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    pCmChan = &NskCmChannelArray[uCmChIndex].cmChan;

    //even key slot
    if(pCmChan->validMask & EVEN_PARITY_MASK) {
        if((pCmChan->even.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChan->even.keyIdx)) {
            if((pCmChan->even.validMask & ESA_MASK) || (pCmChan->even.validMask & LOCAL_D_MASK))
                keyUsage |= HW_KEY_USAGE_DEC;
            if(pCmChan->even.validMask & LOCAL_S_MASK)
                keyUsage |= HW_KEY_USAGE_ENC;
            keyMask |= EVEN_PARITY_MASK;
        }
    }

    //odd key slot
    if(pCmChan->validMask & ODD_PARITY_MASK) {
        if((pCmChan->odd.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChan->odd.keyIdx)) {
            if((pCmChan->odd.validMask & ESA_MASK) || (pCmChan->odd.validMask & LOCAL_D_MASK))
                keyUsage |= HW_KEY_USAGE_DEC;
            if(pCmChan->odd.validMask & LOCAL_S_MASK)
                keyUsage |= HW_KEY_USAGE_ENC;
            keyMask |= ODD_PARITY_MASK;
        }
    }

    //clear key slot
    if(pCmChan->validMask & CLEAR_PARITY_MASK) {
        if((pCmChan->clear.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChan->clear.keyIdx)) {
            if((pCmChan->clear.validMask & ESA_MASK) || (pCmChan->clear.validMask & LOCAL_D_MASK))
                keyUsage |= HW_KEY_USAGE_DEC;
            if(pCmChan->clear.validMask & LOCAL_S_MASK)
                keyUsage |= HW_KEY_USAGE_ENC;
            keyMask |= CLEAR_PARITY_MASK;
        }
    }

    if(keyMask && !NskCmChannelArray[uCmChIndex].bGetLowerDest[uFigoId]) {
        ret = tsp_nsk_updateCmchanProp(uCmChIndex, keyMask, uFigoId, true, false);
        if(ret != TEE_SUCCESS) {
            TA_LOGE("tsp_nsk_updateCmchanProp fail, set keyUsage=0\n");
            keyUsage = 0;
            ret = TEE_SUCCESS; //if timeout happen(fw in error state?), still return success
        }
    }
    params[1].value.a = keyUsage;
    return ret;
}

TEE_Result TSPTA_GetNSKCmChannelKeyRight(void* sessionContext,
                                          uint32_t uCmChIndex,
                                          uint32_t  *pbProtected)
{
    TEE_Result ret = TEE_SUCCESS;
    NSKCmChannel* pCmChan;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;
    uint32_t uFigoId = pTspCtx->FigoId;

    if(uCmChIndex >= NSK_CMCHANNEL_NUM_WHOLE_TSP) {
        TA_LOGE("CmChannel index %d is wrong\n", uCmChIndex);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    *pbProtected = 0;
    pCmChan = &NskCmChannelArray[uCmChIndex].cmChan;

    if(!NskCmChannelArray[uCmChIndex].bGetLowerDest[uFigoId]) {
        uint32_t keyUpdateMask = 0;
        TA_LOGH("need to update CmChannel(0x%x) property firstly\n", uCmChIndex);
        keyUpdateMask = tsp_nsk_getCmchanKeyUpdateMask(pCmChan, NULL);

        if(keyUpdateMask) {
            ret = tsp_nsk_updateCmchanProp(uCmChIndex, keyUpdateMask, uFigoId, true, false);
            if(ret != TEE_SUCCESS) {
                TA_LOGE("tsp_nsk_updateCmchanProp fail\n");
                return ret;
            }
        }else //no valid key
            return TEE_SUCCESS;
    }

    //even key slot
    if(pCmChan->validMask & EVEN_PARITY_MASK) {
        if((pCmChan->even.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChan->even.keyIdx)) {
            *pbProtected |= tsp_nsk_getCmchanLowerDest(pTspCtx, uCmChIndex, pCmChan->even.keyIdx);
        }
    }

    //odd key slot
    if(pCmChan->validMask & ODD_PARITY_MASK) {
        if((pCmChan->odd.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChan->odd.keyIdx)) {
            *pbProtected |= tsp_nsk_getCmchanLowerDest(pTspCtx, uCmChIndex, pCmChan->odd.keyIdx);
        }
    }

    //clear key slot
    if(pCmChan->validMask & CLEAR_PARITY_MASK) {
        if((pCmChan->clear.validMask & KEY_INDEX_MASK) && IS_VALID_NSK_KEY(pCmChan->clear.keyIdx)) {
            *pbProtected |= tsp_nsk_getCmchanLowerDest(pTspCtx, uCmChIndex, pCmChan->clear.keyIdx);
        }
    }
    return ret;
}
