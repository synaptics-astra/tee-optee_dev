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
//! \file m2m_wrap.c
//!
//! \brief  This file includes the common APIs for both TZ/Non-TZ version
//!
//! Author:         wangzl
//! Modified by:
//! Version:        1.0
//! Date:           Mar 13, 2017
//!
///////////////////////////////////////////////////////////////////////////////
#include "tsp_comm.h"
#include "tsp_shared.h"
#include "tsp_fw_spec.h"

#ifdef TA_COMPILE
#include "config.h"
#include "tsp_ta_internal.h"
#include "tsp_config_log.h"
#include "ta_log.h"

typedef unsigned char           BOOL;
#define  FALSE 0
#define  TRUE  1

#endif

#define TSP_MAX_TS_SRC  arr_TspDtcmGlobal_TsDmxFilterTable
#define TSP_MAX_ES_SRC  arr_TspDtcmGlobal_WmFilter
#define TSP_MAX_TSI_PORT_NUM    (9)

typedef struct __figo_rsc_ctx__ {
    BOOL    bUsed;
    uint32_t  uUsedTsSrcNum;
    uint32_t  uUsedTsiNum;
    uint32_t  uUsedEsSrcNum;
    uint32_t  uUsedM2mSessNum;
    uint32_t  uFwPageIdx;
    uint32_t  uTsiUsers[TSP_MAX_TSI_PORT_NUM];
} FIGO_RSC_CTX;

/*
Four sched rules for different cases: just as TS input for example ,
when tsSrcNum0(run on figo 0)<tsSrcNum1(run on figo 1)<TSP_MAX_TS_SRC
1. TSP_FIGO_SCHED_RULE_MAX_PERF,  sched FIGO 0 for max performance
2. TSP_FIGO_SCHED_RULE_SAVE_SOURCE, sched FIGO 1 to save source
3. TSP_FIGO_SCHED_RULE_BALANCE,  sched FIGO 0 when tsSrcNum0!=0, otherwise sched FIGO 1
4. For TSP_FIGO_SCHED_RULE_FIGO_ISOLATION, always use the assigned initial figo to schedule,
    when it is full, then turn to next figo
5. TSP_FIGO_SCHED_RULE_FIGO_UNIQUE if for TSI input, one figo has one unique tuner input

Common Policy is:
    TS/ES/RAW starts from figo 0
    M2M starts from figo (TSP_MAX_FIGO_NUM-1)
*/
typedef enum __tsp_figo_sched_rule__ {
    TSP_FIGO_SCHED_RULE_MAX_PERF          = 0x0,
    TSP_FIGO_SCHED_RULE_SAVE_SOURCE       = 0x1,
    TSP_FIGO_SCHED_RULE_BALANCE           = 0x2,
    TSP_FIGO_SCHED_RULE_FIGO_ISOLATION    = 0x3,
    TSP_FIGO_SCHED_RULE_FIGO_UNIQUE       = 0x4,
    TSP_FIGO_SCHED_RULE_MAX
}TSP_FIGO_SCHED_RULE;

#define  TSP_INVAILD_FIGOID  0xFFFF
#define  TSP_INVAILD_FWIDX   0xFFFF

static FIGO_RSC_CTX g_figoRscCtx[TSP_MAX_FIGO_NUM];

#define FIGO_LOADING(figoId)   (g_figoRscCtx[figoId].uUsedTsSrcNum + g_figoRscCtx[figoId].uUsedEsSrcNum \
                                + g_figoRscCtx[figoId].uUsedM2mSessNum)

static BOOL internal_filter_Figo(uint32_t uSrcType, uint32_t uFigoId)
{
    BOOL bPass = TRUE;

    if(((uSrcType == TSP_INPUT_TYPE_TS_MEM || uSrcType == TSP_INPUT_TYPE_TS_FE) && \
        g_figoRscCtx[uFigoId].uUsedTsSrcNum == TSP_MAX_TS_SRC) || \
        (uSrcType == TSP_INPUT_TYPE_ES && g_figoRscCtx[uFigoId].uUsedEsSrcNum == TSP_MAX_ES_SRC)) {
        bPass = FALSE;
    }
   return bPass;

}

static uint32_t internal_sched_Figo(uint32_t uSrcType, uint32_t uTsiPort)
{
    uint32_t iter = TSP_INVAILD_FIGOID, i, uCurFigo;
    BOOL bFoundIter = FALSE;
    TSP_FIGO_SCHED_RULE uRule;

    switch(uSrcType) {
    case TSP_INPUT_TYPE_TS_MEM:
    case TSP_INPUT_TYPE_ES:
        uRule = TSP_FIGO_SCHED_RULE_FIGO_ISOLATION;
        break;
    case TSP_INPUT_TYPE_TS_FE:
    case TSP_INPUT_TYPE_M2M:
        uRule = TSP_FIGO_SCHED_RULE_MAX_PERF;
        break;
    default:
        TA_LOGF( "Invalid uSrcType:0x%x\n", uSrcType);
        goto exit;
    }

    //to support multiple instances to share one TSI
    if(uSrcType == TSP_INPUT_TYPE_TS_FE) {
        for(i = 0;i < TSP_MAX_FIGO_NUM; i++) {
            if(g_figoRscCtx[i].uTsiUsers[uTsiPort]) {
                if(g_figoRscCtx[i].uUsedTsSrcNum == TSP_MAX_TS_SRC) {
                    TA_LOGE("the dmx instances reach full on figo[%d]\n", i);
                } else {
                    iter = i;
                }
                goto exit;
            }
        }
    }

    for(i = 0;i < TSP_MAX_FIGO_NUM; i++) {

        uCurFigo = (uSrcType == TSP_INPUT_TYPE_M2M ? TSP_MAX_FIGO_NUM-1-i : i);//M2M sched from the end
        if(!internal_filter_Figo(uSrcType, uCurFigo))
            continue;

        if(bFoundIter == FALSE){
            iter = uCurFigo;
            bFoundIter = TRUE;
            continue;
        }

        if(uRule == TSP_FIGO_SCHED_RULE_MAX_PERF){
            if(uSrcType == TSP_INPUT_TYPE_TS_FE) {
                if(g_figoRscCtx[iter].uUsedTsiNum > g_figoRscCtx[uCurFigo].uUsedTsiNum)
                    iter = uCurFigo;
            }
            else if(FIGO_LOADING(iter) > FIGO_LOADING(uCurFigo)){
                iter = uCurFigo;
            }
        }else if(uRule == TSP_FIGO_SCHED_RULE_SAVE_SOURCE){
            if(FIGO_LOADING(iter) < FIGO_LOADING(uCurFigo)){
                iter = uCurFigo;
            }
        }else if(uRule == TSP_FIGO_SCHED_RULE_BALANCE){
            if((FIGO_LOADING(iter) > FIGO_LOADING(uCurFigo) && g_figoRscCtx[uCurFigo].bUsed) || \
               (FIGO_LOADING(iter) < FIGO_LOADING(uCurFigo) && !g_figoRscCtx[iter].bUsed)){
                iter = uCurFigo;
            }
        }
    }
exit:
    return iter;
}

static uint32_t internal_FWIdx_schedRule(uint32_t uSrcType, uint32_t uScheme,uint32_t uCryptoLevel)
{
    uint32_t uRetFWIdx = TSP_INVAILD_FWIDX;

    if(uSrcType == TSP_INPUT_TYPE_ES){
        switch(uScheme){
            case TSP_AES_128_CTR_NO_NO:
            case TSP_AES_128_CBC_CTS_CLR:
            case TSP_AES_128_CBC_RP_CLR:
            case TSP_AES_128_CBC_CLR_CLR:
            case TSP_AES_128_CBC_NO_CLR:
            case TSP_AES_128_CBC_NO_NO:
                uRetFWIdx = 1;
                break;
            default:
                TA_LOGF( "unsupport scheme:0x%x\n", uScheme);
        }

    }else if(uSrcType == TSP_INPUT_TYPE_TS_MEM || uSrcType == TSP_INPUT_TYPE_TS_FE){
        if(uScheme == TSP_CRYPTO_SCHEME_TYPE_NONE){
            uRetFWIdx = 0;
        }else {
            if(uCryptoLevel == TSP_SRC_CRYPTO_TS_PAYLOAD){
                if(uScheme == TSP_AES_128_CBC_RBT_CLR || \
                    uScheme ==  TSP_AES_128_ECB_CLR_CLR || \
                    uScheme ==  TSP_AES_128_CBC_CLR_CLR || \
                    uScheme ==  TSP_DES_56_ECB_CLR_CLR || \
                    uScheme == TSP_DVB_CSA_NO_NO || \
                    uScheme == TSP_TDES_128_CBC_EXOR_EXOR || \
                    uScheme == TSP_TDES_128_ECB_CLR_CLR || \
                    uScheme == TSP_DVB_CSA3_NO_NO || \
                    uScheme == TSP_ASA_NO_NO) {
                    uRetFWIdx = 0;
                }else {
                    TA_LOGE("UnSupport SchemeType 0x%x for TS input\n",uScheme);
                }
            }
            else if(uCryptoLevel == TSP_SRC_CRYPTO_WHOLE_PACKET) {
                uRetFWIdx = 2;
            }
            else {
                TA_LOGE("Unknow CyptCfg.Level 0x%x\n",uCryptoLevel);
            }
        }

    }else if(uSrcType == TSP_INPUT_TYPE_RAW){
        if(uScheme == TSP_AES_128_CBC_NO_NO) {
            uRetFWIdx = 1;              /**< page5 for DTCPIP-Enc */
         }
         else if(uScheme == TSP_AES_128_CBC_CLR_CLR) {
            uRetFWIdx = 1;              /**< page3 for DTCPIP-Dec */
         }
         else {
            TA_LOGE("UnSupport SchemeType 0x%x for TSP_INPUT_TYPE_RAW\n",uScheme);
        }
    }else if(uSrcType == TSP_INPUT_TYPE_M2M){
        uRetFWIdx = 0;
    }

    if(uRetFWIdx != TSP_INVAILD_FWIDX){
        uRetFWIdx = 2;
    }

    return uRetFWIdx;
}

static uint32_t internal_sched_FWIdx(uint32_t uSrcType, uint32_t uTsiPort,
                                            uint32_t uScheme, uint32_t uCryptoLevel,
                                            uint32_t uFigoId, uint32_t *pNeedLoad)
{
    uint32_t uRetFWIdx = TSP_INVAILD_FWIDX;

    uRetFWIdx = internal_FWIdx_schedRule(uSrcType, uScheme, uCryptoLevel);
    if(uRetFWIdx == TSP_INVAILD_FWIDX) {
        TA_LOGE("sched faild srcType[0x%x] scheme[0x%x], cryptoLevel[0x%x]\n", \
            uSrcType, uScheme, uCryptoLevel);
        goto end;
    }

    if(uSrcType == TSP_INPUT_TYPE_ES || uSrcType == TSP_INPUT_TYPE_RAW){
        g_figoRscCtx[uFigoId].uUsedEsSrcNum++;
    }else if(uSrcType == TSP_INPUT_TYPE_TS_MEM || uSrcType == TSP_INPUT_TYPE_TS_FE){
        g_figoRscCtx[uFigoId].uUsedTsSrcNum++;
        if(uSrcType == TSP_INPUT_TYPE_TS_FE) {
            g_figoRscCtx[uFigoId].uTsiUsers[uTsiPort]++;
            g_figoRscCtx[uFigoId].uUsedTsiNum++;
        }
    }else if(uSrcType == TSP_INPUT_TYPE_M2M){
        g_figoRscCtx[uFigoId].uUsedM2mSessNum++;
    }

    if(g_figoRscCtx[uFigoId].bUsed){
      // assert(uRetFWIdx == g_figoRscCtx[uFigoId].uFwPageIdx);  //mismatch would not happen, since sched figo check it
        if(uRetFWIdx != g_figoRscCtx[uFigoId].uFwPageIdx){
            TA_LOGE("sched FWId 0x%x did not match current FWId 0x%x in Figo 0x%x\n", \
                uRetFWIdx, g_figoRscCtx[uFigoId].uFwPageIdx, uFigoId);
            uRetFWIdx = TSP_INVAILD_FWIDX;
            goto end;
        }
        *pNeedLoad = 0;
    }else{
        g_figoRscCtx[uFigoId].uFwPageIdx = uRetFWIdx;
        g_figoRscCtx[uFigoId].bUsed = TRUE;
        *pNeedLoad = 1;
    }
end:
    return uRetFWIdx;
}

static void internal_dump_SchedCtx(void)
{
    uint32_t uFigoId;

    for(uFigoId=0; uFigoId<TSP_MAX_FIGO_NUM; uFigoId++){
        TA_LOGH("----------DUMP FIGO[%d] SCHED CONTEXT-----------\n",uFigoId);
        TA_LOGH("bUsed=[%d], TsiNum=[%d], FwPageIdx=[%d] Instance Num: [TS,ES,M2M]=[%d, %d, %d] \n",
                g_figoRscCtx[uFigoId].bUsed, g_figoRscCtx[uFigoId].uUsedTsiNum,
                g_figoRscCtx[uFigoId].uFwPageIdx, g_figoRscCtx[uFigoId].uUsedTsSrcNum,
                g_figoRscCtx[uFigoId].uUsedEsSrcNum, g_figoRscCtx[uFigoId].uUsedM2mSessNum);
    }
}

TEE_Result TSP_FIGO_InitSchedCtx(void)
{
    memset(g_figoRscCtx, 0, sizeof(FIGO_RSC_CTX)*TSP_MAX_FIGO_NUM);
    return TEE_SUCCESS;
}

TEE_Result TSP_FIGO_FreeSchedCtx(void)
{
    return TEE_SUCCESS;
}

TEE_Result TSP_FIGO_Sched(uint32_t uSrcType, uint32_t uTsiPort,
                                uint32_t uScheme, uint32_t uCryptoLevel,
                                uint32_t *pFigoId, uint32_t *pFwIdx,
                                uint32_t *pNeedReload)
{
    TEE_Result ret = TEE_SUCCESS;
    uint32_t uFwIdx, uFigoId;

    if(uSrcType >= TSP_INPUT_TYPE_MAX || uScheme >= TSP_SCHEME_TYPE_MAX ||
        uCryptoLevel >= TSP_SRC_CRYPTO_LEVEL_MAX || uTsiPort >= TSP_MAX_TSI_PORT_NUM){
        TA_LOGF("Invalid input parmas:0x%x 0x%x 0x%x 0x%x\n",
            uSrcType, uTsiPort, uScheme, uCryptoLevel);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    TA_LOGH("figo sched enter uSrcType=0x%x, uTsiPort=0x%x, uScheme=0x%x\n",
                uSrcType, uTsiPort, uScheme);
    uFigoId = internal_sched_Figo(uSrcType, uTsiPort);
    if(uFigoId == TSP_INVAILD_FIGOID){
        TA_LOGE( "sched Figo fail\n");
        internal_dump_SchedCtx();
        ret =TEE_ERROR_NOT_IMPLEMENTED;
    }else {
        uFwIdx = internal_sched_FWIdx(uSrcType, uTsiPort, uScheme, uCryptoLevel,
                    uFigoId, pNeedReload);
        if(uFwIdx != TSP_INVAILD_FWIDX){
            *pFigoId = uFigoId;
            *pFwIdx = uFwIdx;
            TA_LOGH( "sched FWIdx success FigoId=0x%x, FWIdx=0x%x, IsNeedReload=0x%x,\n",
                *pFigoId, *pFwIdx, *pNeedReload);
        }else {
            TA_LOGE( "sched FWIdx fail\n");
            internal_dump_SchedCtx();
            ret =TEE_ERROR_NOT_IMPLEMENTED;
        }
    }
    return ret;
}

TEE_Result TSP_FIGO_UnSched(uint32_t uSrcType, uint32_t uTsiPort,
                                     uint32_t uFigoId)
{
    TEE_Result ret = TEE_SUCCESS;

    if(uSrcType >= TSP_INPUT_TYPE_MAX || uFigoId >= TSP_MAX_FIGO_NUM ||
        uTsiPort >= TSP_MAX_TSI_PORT_NUM){
        TA_LOGF("Invalid input parmas:0x%x 0x%x 0x%x\n", uSrcType, uTsiPort, uFigoId);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    if(!g_figoRscCtx[uFigoId].bUsed){
        TA_LOGE("Figo id:0x%x not used\n", uFigoId);
        return TEE_ERROR_NOT_IMPLEMENTED;
    }

    TA_LOGH("Figo[0x%x] unSched enter, srcType=0x%x, TsiPort=0x%x\n",
                uFigoId, uSrcType, uTsiPort);

    if(uSrcType == TSP_INPUT_TYPE_ES || uSrcType == TSP_INPUT_TYPE_RAW){
        if(g_figoRscCtx[uFigoId].uUsedEsSrcNum > 0)
            g_figoRscCtx[uFigoId].uUsedEsSrcNum--;
        else {
            TA_LOGE( "No Es Src for figo 0x%x, unSched fail\n", uFigoId);
            ret = TEE_ERROR_NO_DATA;
        }
    }else if(uSrcType == TSP_INPUT_TYPE_TS_MEM || uSrcType == TSP_INPUT_TYPE_TS_FE) {
        if(g_figoRscCtx[uFigoId].uUsedTsSrcNum > 0) {
            if(uSrcType == TSP_INPUT_TYPE_TS_FE) {
                if(!g_figoRscCtx[uFigoId].uTsiUsers[uTsiPort]) {
                   TA_LOGE("No Tsi %d instance on figo 0x%x, unSched fail\n",
                        uTsiPort, uFigoId);
                   return TEE_ERROR_NO_DATA;
                }
                g_figoRscCtx[uFigoId].uUsedTsiNum--;
                g_figoRscCtx[uFigoId].uTsiUsers[uTsiPort]--;
            }
            g_figoRscCtx[uFigoId].uUsedTsSrcNum--;
        }
        else {
            TA_LOGE( "No Ts Src for figo 0x%x, unSched fail\n", uFigoId);
            ret = TEE_ERROR_NO_DATA;
        }
    }else if(uSrcType == TSP_INPUT_TYPE_M2M) {
        if(g_figoRscCtx[uFigoId].uUsedM2mSessNum> 0) {
            g_figoRscCtx[uFigoId].uUsedM2mSessNum--;
        }
        else {
            TA_LOGE( "No M2M session for figo 0x%x, unSched fail\n", uFigoId);
            ret = TEE_ERROR_NO_DATA;
        }
    }
    if(ret == TEE_SUCCESS){
        TA_LOGH( "Figo 0x%x unSched success\n", uFigoId);
        if(g_figoRscCtx[uFigoId].uUsedTsSrcNum == 0 &&
            g_figoRscCtx[uFigoId].uUsedEsSrcNum == 0 &&
              g_figoRscCtx[uFigoId].uUsedM2mSessNum== 0) {
            g_figoRscCtx[uFigoId].bUsed = FALSE;
            TA_LOGH( "Figo 0x%x unSched all src\n", uFigoId);
        }
    }
    return ret;
}

