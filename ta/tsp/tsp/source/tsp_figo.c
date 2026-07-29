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
#define LOG_TAG "TSP_TA_FIGO"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "tsp_ta_internal.h"
#include "tee_internal_api.h"
#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif
#include "mem_region_userdata.h"
#include "tsp_config_log.h"
#include "ta_log.h"
#include "tsp_fw_spec.h"
#include "tsp.h"
#include "tsp_figo.h"
#include "tsp_nsk_alg.h"
#if defined(CONFIG_GENX_ENABLE)
#include "bcm_mailbox.h"
#include "bcm_status.h"
#endif

#define USE_DHUB_LITE_AND_ONE_ITCM
#include "api_dhubLite.h"

#define DSFIFO_BUF_SIZE     128
//! Combine the FIFO ID and HBO ID into a single HBO-FIFO ID
#define TSP_HBOFIFOID(_hboID, _fifoID)  ((((_hboID) & 0x1) << 5) | ((_fifoID) & 31))
#define ADDR_ALIGN(uiAddr, uiAlign) (uiAddr + uiAlign - 1) & ~(uiAlign - 1);

static TEE_FigoContext gFigoCtx[TSP_MAX_FIGO_NUM] = {0};
static void *g_dtcmHeap[TSP_MAX_FIGO_NUM*DTCM_MEM_TYPE_MAX] = {NULL};

extern TSPTA_HBOCfg TSPTA_HBOConfig[TSP_MAX_FIGO_NUM][TSP_HBO_FIFO_ID_MAX];

static inline void write_fw_32(void *addr, uint32_t value)
{
    *((volatile uint32_t *)addr) = value;
}

static inline uint32_t read_fw_32(void *addr)
{
    return *((volatile uint32_t *)addr);
}

static inline void write_fw_ctx(void *addr, void *p, uint32_t size)
{
    uint32_t i;

    for(i = 0; i < size; i += sizeof(uint32_t)) {
        write_fw_32((uint8_t *)addr + i, *((uint32_t *)(uintptr_t)((uint8_t *)p +i)));
    }
}

static inline void read_fw_ctx(void *addr, void *p, uint32_t size)
{
    uint32_t i;

    for(i = 0; i < size; i += sizeof(uint32_t)) {
        *((uint32_t *)(uintptr_t)((uint8_t *)p +i)) = read_fw_32((uint8_t *)addr + i);
    }
}

static inline SIE_LocalArea_Ext* get_local_area(TEE_FigoContext *pCtx)
{
    return (SIE_LocalArea_Ext *)(intptr_t)(pCtx->base + RA_TspDtcmGlobal_LA);
}

static inline SIE_DTCMFifo* get_hbo_fifo(TEE_FigoContext *pCtx,
                                     uint32_t uFifoIdx)
{
    SIE_LocalArea_Ext *pLA = get_local_area(pCtx);

    return &pLA->ie_hboFifoList[uFifoIdx];
}

static inline void dtcm_fifo_config(volatile SIE_DTCMFifo* pDTCMFifo,
                                        uint32_t uDtcmBase,
                                        uint32_t uSize)
{
    pDTCMFifo->u_nMaxSize = uSize;
    pDTCMFifo->u_pbBase   = uDtcmBase;
    pDTCMFifo->u_nRdOff   = pDTCMFifo->u_nWrOff = 0;
}

#if !defined(CONFIG_GENX_ENABLE)
//! uResetVal    1: to reset the Engine  0: normal operation
static void internal_tsp_crypto_resetCtrl(TEE_FigoContext *pCtx,
                                           uint32_t uResetVal)
{
    T32TspCryReg_global reg_global;
    uint32_t CryptoEngineRegReset = RA_TspReg_CryReg + RA_TspCryReg_global;

    //! get the current reset status
    reg_global.u32 = read_fw_32(pCtx->base + CryptoEngineRegReset);
    //! warn and return error if the new state is the same as the old state.
    if(reg_global.uglobal_reset == uResetVal) {
        TA_LOGH("Crypto Engine uResetVal equals to current value [%d]\n",
            reg_global.uglobal_reset);
        return;
    }

    //! uResetVal:
    //! 1: reset state
    //! 0: normal operation
    write_fw_32(pCtx->base + CryptoEngineRegReset, uResetVal);
}

static void internal_tsp_crypto_setReturnAddress(TEE_FigoContext *pCtx,
                                    uint32_t uFifoCmdIdx, uint32_t uReturnAddress)
{
    uint32_t Off = 0;
    uint32_t NthCryptoEngineFifoCmdReg = 0;

    Off = (pCtx->uFigoId == 0) ? (RA_TspCryReg_return_address) :
        (RA_TspCryReg_return_address_3);
    NthCryptoEngineFifoCmdReg = RA_TspReg_CryReg + Off +
        uFifoCmdIdx * (sizeof(T32TspCryReg_return_address) +
        sizeof(T32TspCryReg_command_count));

    //init return_address
    write_fw_32(pCtx->base + NthCryptoEngineFifoCmdReg, uReturnAddress);
}

#endif

static void internal_tsp_resetFigo(TEE_FigoContext *pCtx)
{
#if defined(CONFIG_GENX_ENABLE)
    TEE_Result ret = TEE_SUCCESS;
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status;

    TA_LOGM("Call BCM to set FIGO[%u] to state[%u]\n",
            pCtx->uFigoId, FIGO_RESET);
    cmd.arg0 = pCtx->uFigoId;
    cmd.arg1 = FIGO_RESET;
    ret = TEE_ExecCmd(BCM_PI_SET_TSP_STATE, &cmd, sizeof(BCM_MAILBOX_COMMAND),
                      &status, sizeof(BCM_MAILBOX_STATUS));
    if (ret != TEE_SUCCESS) {
        TA_LOGE("Error in sending BCM_PI_SET_TSP_STATE: 0x%x\n", ret);
    }
#else
    T32FigoReg_figoRstn figoRstn;
    uint32_t uAddr = RA_TspReg_FigoReg + RA_FigoSysBasic_FIGO0;

    figoRstn.u32 = 0;
    figoRstn.ufigoRstn_up = 0;

    write_fw_32(pCtx->base + uAddr + RA_FigoReg_figoRstn, figoRstn.u32);
#endif
}

static void internal_tsp_InitLocalArea(TEE_FigoContext *pCtx,
                uint32_t uIsBreakPointEnable, uint32_t uIsFIGOStall)
{
    volatile SIE_LocalArea_Ext *pLA =
        (volatile SIE_LocalArea_Ext *)get_local_area(pCtx);
    uint32_t uInitCmdRsp = (uIsFIGOStall) ? StallCmd_CMD_cmdTag_STALL :
                StallCmd_CMD_cmdTag_RUN;

    memset((void *)&pLA->ie_regDumpArea, 0, sizeof(pLA->ie_regDumpArea));
    pLA->u_nDSBase = FIGOSYSLIB_INVALID_DS_BASE;
    pLA->uBRKCFG_uDisableBreakPoint = !uIsBreakPointEnable;
    pLA->ie_stallCmd.uRSP_rspTag = uInitCmdRsp;
    pLA->ie_stallCmd.uCMD_cmdTag = uInitCmdRsp;
    //must be set with the actual shadow phy addr in tspdriver or demux kernel driver
    pLA->uIMGBASE_uDDRImgBase = 0;
    pLA->ie_retQ.uHDR_nWrOffset = pLA->ie_retQ.uHDR_nRdOffset = 0;
}

static void internal_tsp_configBreakPoint(TEE_FigoContext *pCtx, bool bEnabled)
{
    SIE_LocalArea_Ext *pLA = get_local_area(pCtx);
    volatile T32LocalArea_BRKCFG* puBrkCfg = NULL;

    puBrkCfg = (volatile T32LocalArea_BRKCFG*)(uintptr_t)((uint8_t *)pLA + RA_LocalArea_BRKCFG);
    puBrkCfg->u32 = 0;
    puBrkCfg->uBRKCFG_uDisableBreakPoint = bEnabled ? 0 : 1;
}

static void internal_tsp_InitBlkQ(volatile SIE_DTCMBlkQ* pBlkQ)
{
    volatile uint16_t *pBlkQList;
    uint32_t i  = 0 ;

    pBlkQList = (volatile uint16_t *)(uintptr_t)((volatile uint8_t *)pBlkQ + RA_DTCMBlkQ_LIST);
    for (i = 0; i < FIGOSYSLIB_MAX_BLK_NUM; i++) {
        pBlkQList[i] = FIGOSYSLIB_INVALID_BIU_PTR;
    }
    pBlkQ->uHDR_nRdOff = pBlkQ->uHDR_nWrOff = 0;
    pBlkQ->uHDR_nTotalSize = 0;
    pBlkQ->uHDR_nMaxSize =  0xFFFF;
}

static inline void internal_tsp_PushBlock(volatile SIE_DTCMBlkQ* pBlkQ, uint16_t uBlkOff)
{
    volatile uint16_t *pBlkQList;

    pBlkQList = (volatile uint16_t *)(uintptr_t)((volatile uint8_t *)pBlkQ + RA_DTCMBlkQ_LIST);
    pBlkQList[pBlkQ->uHDR_nWrOff] = uBlkOff;
    pBlkQ->uHDR_nWrOff = ((pBlkQ->uHDR_nWrOff + 1) % FIGOSYSLIB_MAX_BLK_NUM);
}


#define FIGO_IO_CONFIG_nDatOutBlkNum            (2)
#define FIGO_IO_CONFIG_nDatOutBlkSize           (256)
static TEE_Result internal_tsp_ConfigDSStreamOutModule(TEE_FigoContext *pCtx)
{
    volatile SIE_DSStreamOutModule *pDSStrmOutModule =
        (volatile SIE_DSStreamOutModule *)(uintptr_t)(pCtx->base + RA_TspDtcmGlobal_OUTMODULE);
    volatile SIE_DTCMBlkPool* pBlkPool =
        (volatile SIE_DTCMBlkPool *)(uintptr_t)(pCtx->base + RA_TspDtcmGlobal_OUTPOOL);
    volatile SIE_DTCMBlk* pCurBlk;
    uint32_t i, uDtcmOffset;

    pDSStrmOutModule->uHDR_datBlkPool = RA_TspDtcmGlobal_OUTPOOL;
    pDSStrmOutModule->uHDR_lastDatBlkPtr = FIGOSYSLIB_INVALID_BIU_PTR;
    pDSStrmOutModule->uHDR_lastDSOutOwner = FIGOSYSLIB_INVALID_BIU_PTR;

    /* not finished, need to config the dsFifo for the event*/

    internal_tsp_InitBlkQ(&pBlkPool->ie_blkQ);

    uDtcmOffset = dtcm_mem_alloc(pCtx->hSecureDtcmHeap,
        FIGO_IO_CONFIG_nDatOutBlkSize * FIGO_IO_CONFIG_nDatOutBlkNum, 8);
    if (INVALID_DTCM_ADDR == uDtcmOffset) {
        TA_LOGE("[FIGO%d] no DTCM Memory\n", pCtx->uFigoId);
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < FIGO_IO_CONFIG_nDatOutBlkNum ; i++) {
        pCurBlk = &pBlkPool->ie_blkList[i];
        dtcm_fifo_config(&pCurBlk->ie_dtcmFifo, uDtcmOffset, FIGO_IO_CONFIG_nDatOutBlkSize);
        pCurBlk->uHDR_addrHdr = FIGOSYSLIB_INVALID_BIU_PTR;
        pCurBlk->uHDR_hdrSize = 0;
        pCurBlk->uHDR_isDSFinished = 1;
        uDtcmOffset += FIGO_IO_CONFIG_nDatOutBlkSize;
        internal_tsp_PushBlock(&pBlkPool->ie_blkQ, (uint16_t)((uint8_t *)pCurBlk - pCtx->base));
    }

    return TEE_SUCCESS;
}

#define ION_ATTR_CHECK_MASK       (ION_A_CO | ION_A_CG | ION_A_CV | ION_A_CC)
#define ION_ATTR_CHECK_GFX_ONLY   (ION_A_CG)
static TEE_Result internal_ConfigMemzoneInfo(TEE_FigoContext *pCtx)
{
    uint32_t regIdx = 0;
    uint32_t regionCnt = 0;
    TEE_MemRegion *regionBuf = NULL;
    TEE_MemRegion *ionSecureRegBuf = NULL;
    uint32_t ionSecureRegCnt = 0;
    uint32_t ion_first_pool_base = 0xFFFFFFFF;  /* The first ION pools base address */
    uint32_t ion_last_pool_end = 0;            /* The last ION pools end address */
    uint32_t ion_first_securepool_base = 0xFFFFFFFF;   /* The first ION secure pools base address */
    uint32_t ion_last_securepool_end = 0;            /* The last ION secure pools end address */
    volatile SIE_SecureCtx *pSecureCtx =
        (volatile SIE_SecureCtx*)(uintptr_t)(pCtx->base + Secure_Define_Dtcm_Base);

    TEE_MemFill((void *)pSecureCtx, 0, Secure_Define_Dtcm_End - Secure_Define_Dtcm_Base + 1);

    regionCnt = TEE_GetMemRegionCount(0, 0);
    regionBuf = (TEE_MemRegion *)TEE_Malloc(regionCnt * sizeof(TEE_MemRegion),
        TEE_MALLOC_FLAG_DEFAULT);
    if(!regionBuf) {
        TA_LOGE( "Faild to malloc buffer for mem region:%p\n",regionBuf);
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    ionSecureRegBuf = (TEE_MemRegion *)TEE_Malloc(regionCnt * sizeof(TEE_MemRegion),
        TEE_MALLOC_FLAG_DEFAULT);
    if(!ionSecureRegBuf) {
        TA_LOGE( "Faild to malloc buffer for mem region:%p\n",regionBuf);
        TEE_Free(regionBuf);
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    TEE_GetMemRegionList(regionBuf, regionCnt, 0, 0);

    /*filter secure mem region*/
    while(regionCnt--) {
        TEE_MemRegion *mr = &regionBuf[regionCnt];
        TA_LOGH("%s      0x%x         0x%x         0x%x\n",
                mr->name,
                mr->base,
                mr->size,
                TEE_MR_IS_SECURE(mr));
        if(TEE_MR_USER(mr) == TEE_MR_USER_ION &&
            (TEE_MR_ION_ATTRIB(mr) & ION_ATTR_CHECK_MASK) != ION_ATTR_CHECK_GFX_ONLY){
            if(ion_first_pool_base > mr->base) {
                ion_first_pool_base = mr->base;
            }
            if(ion_last_pool_end < (mr->base + (mr->size - 1))) {
                ion_last_pool_end = mr->base + (mr->size - 1);
            }
        }
        if(TEE_MR_USER(mr) == TEE_MR_USER_ION &&
        (TEE_MR_ION_ATTRIB(mr) & ION_ATTR_CHECK_MASK) != ION_ATTR_CHECK_GFX_ONLY &&
        TEE_MR_ION_FOR_SECURE(mr)) {
            if(ion_first_securepool_base > mr->base) {
                ion_first_securepool_base = mr->base;
            }
            if(ion_last_securepool_end < (mr->base + (mr->size - 1))) {
                ion_last_securepool_end = mr->base + (mr->size - 1);
            }
            ionSecureRegBuf[ionSecureRegCnt++] = *mr;
        }
    }

    /*Check Secure ION pools contiguous*/
    regIdx = 0;
    if(ionSecureRegCnt > 1) {
        while((regIdx + 1) < ionSecureRegCnt) {
            TA_LOGH("%s-0x%x-0x%x, %s-0x%x-0x%x\n",
                    ionSecureRegBuf[regIdx].name,
                    ionSecureRegBuf[regIdx].base,
                    ionSecureRegBuf[regIdx].size,
                    ionSecureRegBuf[regIdx+1].name,
                    ionSecureRegBuf[regIdx+1].base,
                    ionSecureRegBuf[regIdx+1].size);
            if(ionSecureRegBuf[regIdx + 1].base + ionSecureRegBuf[regIdx + 1].size != \
                ionSecureRegBuf[regIdx].base) {
                TA_LOGE( "Secure ION Pool discontiguous\n");
                pSecureCtx->ie_SecMemBound.u_StartAddr = 0;
                pSecureCtx->ie_SecMemBound.u_EndAddr = 0;
                TEE_Free(ionSecureRegBuf);
                TEE_Free(regionBuf);
                return TEE_ERROR_ACCESS_DENIED;
            }
            regIdx++;
        }
    }

    pSecureCtx->ie_SecMemBound.u_StartAddr = ion_first_securepool_base;
    pSecureCtx->ie_SecMemBound.u_EndAddr = ion_last_securepool_end;
    pSecureCtx->ie_DstMemBound.u_StartAddr = ion_first_pool_base;
    pSecureCtx->ie_DstMemBound.u_EndAddr = ion_last_pool_end;
    pSecureCtx->ie_SecDtcmBound.u_StartAddr = Secure_Define_Dtcm_Base + sizeof(SIE_SecureCtx);
    pSecureCtx->ie_SecDtcmBound.u_EndAddr = Secure_Define_Dtcm_End;
    pSecureCtx->ie_SecCheckerCtx.u_CheckerCntThresh = SECURE_CHECK_CNT_THRESHOLD;
    pSecureCtx->ie_SecCheckerCtx.u_MagicNum = Secure_Define_CTX_MagicNum;
    TEE_Free(regionBuf);
    TEE_Free(ionSecureRegBuf);

    //update TZ enabled flag to TSP fw
    pSecureCtx->ie_SecCheckerCtx.u_bTZEnabled = 1;
    TA_LOGH("%s:%d: u_bTZEnabled %d\n", __FUNCTION__, __LINE__,
        pSecureCtx->ie_SecCheckerCtx.u_bTZEnabled);

    TA_LOGH("ion_base:0x%x, ion_end:0x%x | ion_secure_base:0x%x, ion_secure_end:0x%x\n",
            pSecureCtx->ie_DstMemBound.u_StartAddr , pSecureCtx->ie_DstMemBound.u_EndAddr,
            pSecureCtx->ie_SecMemBound.u_StartAddr, pSecureCtx->ie_SecMemBound.u_EndAddr);

    TSPTA_InitNSKAlgTable((void *)pSecureCtx);
    return TEE_SUCCESS;
}

static void internal_tsp_init_tasklist(TEE_FigoContext *pCtx)
{
    int i, task_nr = (arr_TspDtcmGlobal_TaskList * 2);
    volatile uint16_t *pTaskItem = (volatile uint16_t *)(uintptr_t)(pCtx->base +
                                    RA_TspDtcmGlobal_SCHED);
    for(i = 0; i < task_nr; i++) {
        pTaskItem[i] = INVALID_DTCM_ADDR;
    }
}

static void internal_tsp_configHboFifo(TEE_FigoContext *pCtx,
                                            uint32_t uHboId,
                                            uint32_t uFifoId,
                                            uint32_t uIsProducer,
                                            uint32_t uASMFifoIdx,
                                            uint32_t uDtcmBase,
                                            uint32_t uDataCnt,
                                            uint32_t uIsEnable)
{
    volatile SIE_DTCMFifo *pDtcmFifo =
        (volatile SIE_DTCMFifo *)get_hbo_fifo(pCtx, uASMFifoIdx);

    //! \li Then initialize the FIFO structure with base and depth information
    dtcm_fifo_config(pDtcmFifo, uDtcmBase, uDataCnt << 3);

    if(uIsProducer) {
        //! \li If ARM or FIGO will act as a producer, the read offset in the FIFO
        //! strcutre will be used to store the actual hardware HBO and FIFO ID
        pDtcmFifo->u_nRdOff = TSP_HBOFIFOID(uHboId, uFifoId);
    } else {
        //! \li Otherwise, the write offset in the FIFO strcutre will be used to
        //! store the actual hardware HBO and FIFO ID
        pDtcmFifo->u_nWrOff = TSP_HBOFIFOID(uHboId, uFifoId);
    }

#ifdef USE_DHUB_LITE_AND_ONE_ITCM
    hbo_lite_queue_clear(&(pCtx->hdl), uFifoId);
    hbo_lite_queue_clear_done(&(pCtx->hdl), uFifoId);
    hbo_lite_queue_cfg(&(pCtx->hdl), uFifoId, uDtcmBase, uDataCnt, uIsEnable, NULL);
#else
    hbo_queue_clear(&(pCtx->hdl), uFifoId);
    hbo_queue_clear_done(&(pCtx->hdl), uFifoId);
    hbo_queue_cfg(&(pCtx->hdl), uFifoId, uDtcmBase, uDataCnt, uIsEnable, NULL);
#endif

    TSPTA_HBOConfig[pCtx->uFigoId][uFifoId].isCfg = true;
    TSPTA_HBOConfig[pCtx->uFigoId][uFifoId].base = uDtcmBase;
    TSPTA_HBOConfig[pCtx->uFigoId][uFifoId].depth = uDataCnt;
    TSPTA_HBOConfig[pCtx->uFigoId][uFifoId].enable = 1;
    TSPTA_HBOConfig[pCtx->uFigoId][uFifoId].fifo_id = uFifoId;
    TSPTA_HBOConfig[pCtx->uFigoId][uFifoId].figo_id = pCtx->uFigoId;
    memcpy(&TSPTA_HBOConfig[pCtx->uFigoId][uFifoId].hdl, &(pCtx->hdl), sizeof(HDL_hbo));
}

static void internal_tsp_configDS(TEE_FigoContext *pCtx,
                                        uint32_t  uDtcmBase,
                                        uint32_t  nCmdDepth)
{
    volatile SIE_LocalArea_Ext *pLA =
        (volatile SIE_LocalArea_Ext *)get_local_area(pCtx);

    // For VS640GenX, DataStreamer register is not accessible by ACPU
#if !defined(CONFIG_GENX_ENABLE)
    uint32_t uAddr = 0;
    T32dsCh_retID dsRetAddr;
    T32dsCh_initID dsInitID;

    //! Config DS return command ID
    uAddr = RA_TspReg_FigoReg + RA_FigoSysBasic_DS +
            RA_DataStreamer_dsCh + RA_dsCh_retID;
    dsRetAddr.u32 = 0;
    dsRetAddr.uretID_addr = RA_TspDtcmGlobal_LA + BA_LocalArea_uDsCmdIdRet;
    write_fw_32(pCtx->base + uAddr, dsRetAddr.u32);

    uAddr =  RA_TspReg_FigoReg + RA_FigoSysBasic_DS +
            RA_DataStreamer_dsCh + RA_dsCh_initID;
    dsInitID.u32 = 0;
    dsInitID.uinitID_Val = 0;
    write_fw_32(pCtx->base + uAddr, dsInitID.u32);
#endif

    pLA->u_uDsCmdIdRet = 0xffff;
    pLA->u_uNextDsCmdId = 0;

    //! Config HBO FIFO
    internal_tsp_configHboFifo(pCtx,
        0,
        TSP_HBO_FIFO_ID_DS_CMD,
        1,      //uIsProducer
        TSP_HBO_FIFO_ID_DS_CMD,
        uDtcmBase,
        nCmdDepth,
        1);
}

static void internal_tsp_sefEngineReset(TEE_FigoContext *pCtx)
{
    uint32_t i = 0;

    write_fw_32(pCtx->base + RA_TspReg_SecReg + RA_TspSecReg_global, 1);
    for(i = 0; i < 4; i++) {
        write_fw_32(pCtx->base + RA_TspReg_SecReg + RA_TspSecReg_rule_0 +
            i * sizeof(SIE_TspSecRegRule) + RA_TspSecRegRule_disable, 0xffffffff);
    }
    for(i = 0; i < (sizeof(SIE_TspSecTbl) * 2048 / sizeof(uint32_t)); i++) {
        write_fw_32(pCtx->base + RA_TspReg_SecTbl + i * sizeof(uint32_t), 0);
    }
    write_fw_32(pCtx->base + RA_TspReg_SecReg + RA_TspSecReg_global, 0);
}

static void internal_tsp_swdEngineReset(TEE_FigoContext *pCtx,
                                                  uint32_t uResetVal)
{
    T32TspSwdReg_global reg_global;

    uint32_t SwdRegReset =  RA_TspReg_SwdReg + RA_TspSwdReg_global;

    //! get the current reset status
    reg_global.u32 = read_fw_32(pCtx->base + SwdRegReset);
    //! warn and return error if the new state is the same as the old state.
    if(reg_global.uglobal_reset == uResetVal) {
        return;
    }
    //! uResetVal:
    //! 1: reset state
    //! 0: normal operation
    write_fw_32(pCtx->base + SwdRegReset, uResetVal);
}

// Nagra rule
/*
     Crypto operation     P    Source        Destination   result
     secSessionEncrypt   No  unprotected unprotected OK
     secSessionEncrypt   No  unprotected protected    OK
     secSessionEncrypt   No  protected    unprotected Fail
     secSessionEncrypt   No  protected    protected    Fail
     secSessionEncrypt   Yes unprotected unprotected Fail
     secSessionEncrypt   Yes unprotected protected    Fail
     secSessionEncrypt   Yes protected    unprotected OK
     secSessionEncrypt   Yes protected    protected    OK
     secSessionDecrypt   No  unprotected unprotected OK
     secSessionDecrypt   No  unprotected protected    Fail
     secSessionDecrypt   No  protected    unprotected OK
     secSessionDecrypt   No  protected    protected    Fail
     secSessionDecrypt   Yes unprotected unprotected Fail
     secSessionDecrypt   Yes unprotected protected   OK
     secSessionDecrypt   Yes protected    unprotected Fail
     secSessionDecrypt   Yes protected    protected   OK
*/
/* Crypto Protected  Source  Destination result
static uint32_t Nagra_RuleTbl[16] = {1, 1, 0, 0,   \
                                   0, 0, 1, 1,   \
                                   1, 0, 1, 0,   \
                                   0, 1, 0, 1};  */


//MRVL unified rule
//Notes:  Clr key, protected, not exist

/*
Operation   Protected Key Option  In Buffer   Out Buffer  Result

  Encryption  No          Clear       NS          NS          Pass
  Encryption  No          Clear       NS          S           Fail
  Encryption  No          Clear       S           NS          Fail
  Encryption  No          Clear       S           S           Fail

  Encryption  No          DRM         NS          NS          Pass
  Encryption  No          DRM         NS          S           Fail
  Encryption  No          DRM         S           NS          Fail
  Encryption  No          DRM         S           S           Fail

  Encryption  Yes         Clear       NS          NS          Fail
  Encryption  Yes         Clear       NS          S           Fail
  Encryption  Yes         Clear       S           NS          Fail
  Encryption  Yes         Clear       S           S           Fail

  Encryption  Yes         DRM         NS          NS          Fail
  Encryption  Yes         DRM         NS          S           Fail
  Encryption  Yes         DRM         S           NS          Pass
  Encryption  Yes         DRM         S           S           Fail

  Decryption  No          Clear       NS          NS          Pass
  Decryption  No          Clear       NS          S           Fail
  Decryption  No          Clear       S           NS          Fail
  Decryption  No          Clear       S           S           Pass // for Nagra HLS case

  Decryption  No          DRM         NS          NS          Pass
  Decryption  No          DRM         NS          S           Fail
  Decryption  No          DRM         S           NS          Fail
  Decryption  No          DRM         S           S           Fail

  Decryption  Yes         Clear       NS          NS          Fail
  Decryption  Yes         Clear       NS          S           Fail
  Decryption  Yes         Clear       S           NS          Fail
  Decryption  Yes         Clear       S           S           Fail

  Decryption  Yes         DRM         NS          NS          Fail
  Decryption  Yes         DRM         NS          S           Pass
  Decryption  Yes         DRM         S           NS          Fail
  Decryption  Yes         DRM         S           S           Fail

*/
const uint16_t SecureRuleTbl[SECURE_RULT_TABLE_SIZE] =
                                 {1,    0,    0,     0,   \
                                  1,    0,    0,     0,   \
                                  0,    0,    0,     0,   \
                                  0,    0,    1,     0,   \
                                  1,    0,    0,     1,   \
                                  1,    0,    0,     0,   \
                                  0,    0,    0,     0,   \
                                  0,    1,    0,     0};
#define SECURE_RULE_MAGIC 0x98d8

static TEE_Result internal_AllocSecureRuleTable(TEE_FigoContext *pCtx)
{
    uint32_t ruleTableAddr;
    volatile SIE_SecureCtx *pSecureCtx =  (volatile SIE_SecureCtx *)(uintptr_t)(pCtx->base +
        Secure_Define_Dtcm_Base);
    int32_t mem_size = sizeof(SIE_SecureRuleTable);
    int32_t alloc_table_size = mem_size - RA_SecureRuleTable_SecureRule;
    SIE_SecureRuleTable *pRuleTable;
    uint32_t i = 0;

    if((int32_t)sizeof(SecureRuleTbl) > alloc_table_size) {
        return TEE_ERROR_EXCESS_DATA;
    }
    ruleTableAddr = dtcm_mem_alloc(pCtx->hSecureDtcmHeap, mem_size, 8);

    if(INVALID_DTCM_ADDR == ruleTableAddr) {
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    pRuleTable = (SIE_SecureRuleTable *)(uintptr_t)(pCtx->base + ruleTableAddr);
    pRuleTable->u_MagicNum = SECURE_RULE_MAGIC;
    for(i = 0; i < (SECURE_RULT_TABLE_SIZE / 2); i++) {
        write_fw_32(&pRuleTable->ie_SecureRule[i],
            (SecureRuleTbl[2 * i + 1] << 16) | SecureRuleTbl[2 * i]);
    }
    pCtx->uSecureRuleTableAddr = ruleTableAddr;

    TA_LOGH("%s:%d, figo_id[%d] alloc SecureRuleTable 0x%x ok\n",
        __FUNCTION__, __LINE__, pCtx->uFigoId, ruleTableAddr);

    //init nsk key - lowdest table
    for(i = 0; i < arr_SecureCtx_NSKKeyLowerDestTable; i++) {
        pSecureCtx->ie_NSKKeyLowerDestTable[i].u_uVal0 = 0xFFFF;
        pSecureCtx->ie_NSKKeyLowerDestTable[i].u_uVal1 = 0xFFFF;
    }

    //init IFCP key table partitions, startAddr <= keyIndex < EndAddr
    pSecureCtx->ie_IFCPKeyTablePartition.ie_Broadcast.u_StartAddr = \
                            IFCP_KEY_BROADCAST_START + IFCP_KEY_TABLE_BASE_ADDR_MASK;
    pSecureCtx->ie_IFCPKeyTablePartition.ie_Broadcast.u_EndAddr = \
                            IFCP_KEY_OTT_START + IFCP_KEY_TABLE_BASE_ADDR_MASK;
    pSecureCtx->ie_IFCPKeyTablePartition.ie_OTT.u_StartAddr = \
                            IFCP_KEY_OTT_START + IFCP_KEY_TABLE_BASE_ADDR_MASK;
    pSecureCtx->ie_IFCPKeyTablePartition.ie_OTT.u_EndAddr = \
                            IFCP_KEY_PVR_START + IFCP_KEY_TABLE_BASE_ADDR_MASK;
    pSecureCtx->ie_IFCPKeyTablePartition.ie_PVR.u_StartAddr = \
                            IFCP_KEY_PVR_START + IFCP_KEY_TABLE_BASE_ADDR_MASK;
    pSecureCtx->ie_IFCPKeyTablePartition.ie_PVR.u_EndAddr = \
                            IFCP_KEY_IRDETO_PRIVATE_START + IFCP_KEY_TABLE_BASE_ADDR_MASK;
    pSecureCtx->ie_IFCPKeyTablePartition.ie_IrdetoPrivate.u_StartAddr = \
                            IFCP_KEY_IRDETO_PRIVATE_START + IFCP_KEY_TABLE_BASE_ADDR_MASK;
    pSecureCtx->ie_IFCPKeyTablePartition.ie_IrdetoPrivate.u_EndAddr = \
        (IFCP_KEY_IRDETO_PRIVATE_START + IFCP_KEY_IRDETO_PRIVATE_SIZE) + IFCP_KEY_TABLE_BASE_ADDR_MASK;
    return TEE_SUCCESS;
}

TEE_Result tsp_figo_init(uint32_t uFigoId)
{
    TEE_Result ret = TEE_SUCCESS;
    SIE_LocalArea_Ext *pLA = 0;
#if !defined(CONFIG_GENX_ENABLE)
    uint32_t pCprEngRntAddr = 0;
#endif
    uint32_t uDtcmOffset = 0, i;
    void **pHeap = 0;
    TEE_FigoContext *pCtx = &gFigoCtx[uFigoId];

    if(pCtx->bInit) {
        return TEE_SUCCESS;
    }

    pCtx->uFigoId = uFigoId;
    pCtx->hdl.mem = TSP_FIGO_REG_BASE + (TSP_FIGO_REG_SIZE  * uFigoId);
    pCtx->hdl.ra = pCtx->hdl.mem + TSP_FIGO_DTCM_SIZE;
#ifdef USE_DHUB_LITE_AND_ONE_ITCM
        pCtx->hdl.fifoCtl.ra = pCtx->hdl.ra + RA_HBOLite_FiFoCtl;
#else
        pCtx->hdl.fifoCtl.ra = pCtx->hdl.ra + RA_HBO_FiFoCtl;
#endif
    pCtx->base = (uint8_t *)tspmem_phy_to_virt(pCtx->hdl.mem);

    //create none-secure/secure dtcm heap
    pHeap = g_dtcmHeap + (DTCM_MEM_TYPE_MAX * uFigoId + 0);
    *pHeap = dtcm_heap_create((void *)RA_TspDtcmGlobal_DATA,
        Secure_Define_Dtcm_Base-RA_TspDtcmGlobal_DATA);
    pCtx->hDtcmHeap = *pHeap;
    TA_LOGH("[FIGO %d]dtcm_heap created\n", uFigoId);

    pHeap = g_dtcmHeap + (DTCM_MEM_TYPE_MAX * uFigoId + 1);
    *pHeap = dtcm_heap_create(
        (void *)(Secure_Define_Dtcm_Base + sizeof(SIE_SecureCtx)),
        Secure_Define_Dtcm_End + 1 -Secure_Define_Dtcm_Base - sizeof(SIE_SecureCtx));
    pCtx->hSecureDtcmHeap = *pHeap;
    TA_LOGH("[FIGO %d]secure dtcm_heap created\n", uFigoId);

    for(i = 0; i < Secure_Define_Dtcm_Base; i += 4) {
        write_fw_32(pCtx->base + i, 0);
    }

    internal_tsp_resetFigo(pCtx);

    internal_tsp_InitLocalArea(pCtx, 0, 1);

    ret = internal_tsp_ConfigDSStreamOutModule(pCtx);
    if(ret) {
        TA_LOGE("[FIGO%d]Failed to config DSStreamOutModule\n", uFigoId);
        goto error;
    }

    ret = internal_ConfigMemzoneInfo(pCtx);
    if(ret) {
        TA_LOGE("[FIGO%d]Failed to config secure memory zone\n", uFigoId);
        goto error;
    }

    internal_tsp_init_tasklist(pCtx);

    internal_tsp_configBreakPoint(pCtx, 1);

    /**
     *  transform to 8-byte cmd depth
     */
    uDtcmOffset = dtcm_mem_alloc(pCtx->hSecureDtcmHeap, DSFIFO_BUF_SIZE, 8);
    if(INVALID_DTCM_ADDR == uDtcmOffset) {
        TA_LOGE("[FIGO%d] no DTCM Memory\n", uFigoId);
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    internal_tsp_configDS(pCtx, uDtcmOffset, DSFIFO_BUF_SIZE >> 3);

    /**
     *  config the crypto engine
     */
    // For VS640GenX, CryptoEngine register is not accessible by ACPU
#if !defined(CONFIG_GENX_ENABLE)
    internal_tsp_crypto_resetCtrl(pCtx, 1);
#endif

    /**
     *  config Crypto Engine Input HBO FIFOs
     */
    for(i = 0; i < 3; i++) {
        uDtcmOffset = dtcm_mem_alloc(pCtx->hSecureDtcmHeap, FilterFifoTotalSize, 8);
        if(INVALID_DTCM_ADDR == uDtcmOffset) {
            TA_LOGE("[FIGO%d] no DTCM Memory\n", uFigoId);
            ret = TEE_ERROR_OUT_OF_MEMORY;
            goto error;
        }

        internal_tsp_configHboFifo(pCtx,
            0,
            TSP_HBO_FIFO_ID_CRYPTO_CMD + i,     /**< uFifoId */
            1 ,                                 /**< uIsProducer */
            TSP_HBO_FIFO_ID_CRYPTO_CMD + i,     /**< uASMFifoIdx */
            uDtcmOffset ,                       /**< uDtcmBase */
            FilterFifoDepth,                    /**< uDataCnt */
            1 );                                /**< uIsEnable */
    }

    pLA = get_local_area(pCtx);
    memset(&pLA->ie_CryptoRtn, 0, 3 * sizeof(SIE_LACryptoCmdCnt));

    // For VS640GenX, CryptoEngine register is not accessible by ACPU
#if !defined(CONFIG_GENX_ENABLE)
    pCprEngRntAddr = RA_TspDtcmGlobal_LA +
                     RA_LocalArea_Ext_CryEngine + RA_LACryptoCmdCnt_RegSyn;

    internal_tsp_crypto_setReturnAddress(pCtx, 0, pCprEngRntAddr);
    pCprEngRntAddr += sizeof(SIE_LACryptoCmdCnt);

    internal_tsp_crypto_setReturnAddress(pCtx, 1, pCprEngRntAddr);
    pCprEngRntAddr += sizeof(SIE_LACryptoCmdCnt);

    internal_tsp_crypto_setReturnAddress(pCtx, 2, pCprEngRntAddr);
    pCprEngRntAddr += sizeof(SIE_LACryptoCmdCnt);

    internal_tsp_crypto_resetCtrl(pCtx, 0);
#endif

    /**
     *  config SEF Input HBO FIFO
     */
    uDtcmOffset = dtcm_mem_alloc(pCtx->hSecureDtcmHeap, FilterFifoTotalSize, 8);
    if(INVALID_DTCM_ADDR == uDtcmOffset) {
        TA_LOGE("[FIGO%d] no DTCM Memory\n", uFigoId);
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    internal_tsp_configHboFifo(pCtx,
            0,
            TSP_HBO_FIFO_ID_SF_INPUT,       /**< uFifoId */
            1 ,                             /**< uIsProducer */
            TSP_HBO_FIFO_ID_SF_INPUT,       /**< uASMFifoIdx */
            uDtcmOffset,                    /**< uDtcmBase */
            FilterFifoDepth,                /**< uDataCnt */
            1 );                            /**< uIsEnable */

    /**
     *  config SEF Output HBO FIFO
     */
    uDtcmOffset = dtcm_mem_alloc(pCtx->hSecureDtcmHeap, FilterFifoTotalSize, 8);
    if(INVALID_DTCM_ADDR == uDtcmOffset) {
        TA_LOGE("[FIGO%d] no DTCM Memory\n", uFigoId);
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    internal_tsp_configHboFifo(pCtx,
            0,
            TSP_HBO_FIFO_ID_SF_OUTPUT,      /**< uFifoId */
            0 ,                             /**< uIsProducer */
            TSP_HBO_FIFO_ID_SF_OUTPUT,      /**< uASMFifoIdx */
            uDtcmOffset,                    /**< uDtcmBase */
            FilterFifoDepth,                /**< uDataCnt */
            1 );                            /**< uIsEnable */

    if(uFigoId == 0) {
        // Only one SefEngine, so init/reset only once
        internal_tsp_sefEngineReset(pCtx);
        // Only one Swd, so reset once
        internal_tsp_swdEngineReset(pCtx, 1);
    }

    /**
     *  config Swd Input HBO FIFO
     */
    uDtcmOffset = dtcm_mem_alloc(pCtx->hDtcmHeap, FilterFifoTotalSize, 8);
    if(INVALID_DTCM_ADDR == uDtcmOffset) {
        TA_LOGE("[FIGO%d] no DTCM Memory\n", uFigoId);
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    internal_tsp_configHboFifo(pCtx,
            0,
            TSP_HBO_FIFO_ID_SWD_CMD,        /**< uFifoId */
            1 ,                             /**< uIsProducer */
            TSP_HBO_FIFO_ID_SWD_CMD,        /**< uASMFifoIdx */
            uDtcmOffset,                    /**< uDtcmBase */
            FilterFifoDepth,                /**< uDataCnt */
            1 );                            /**< uIsEnable */

    /**
     *  config Swd Output HBO FIFO
     */
    uDtcmOffset = dtcm_mem_alloc(pCtx->hDtcmHeap, FilterFifoTotalSize, 8);
    if(INVALID_DTCM_ADDR == uDtcmOffset) {
        TA_LOGE("[FIGO%d] no DTCM Memory\n", uFigoId);
        ret = TEE_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    internal_tsp_configHboFifo(pCtx,
            0,
            TSP_HBO_FIFO_ID_SWD_RETURN,     /**< uFifoId */
            0 ,                             /**< uIsProducer */
            TSP_HBO_FIFO_ID_SWD_RETURN,     /**< uASMFifoIdx */
            uDtcmOffset,                    /**< uDtcmBase */
            FilterFifoDepth,                /**< uDataCnt  */
            1 );                            /**< uIsEnable */

    // Only one Swd, so release once
    if(uFigoId == 0) {
        internal_tsp_swdEngineReset(pCtx, 0);
    }

    /**
     *  config secure rule table
     */
    ret = internal_AllocSecureRuleTable(pCtx);
    if(ret) {
        TA_LOGE("[FIGO%d]Failed to alloc secure rule table\n", pCtx->uFigoId);
        goto error;
    }

    pCtx->u_pLocalData_all_filter = ADDR_ALIGN(Private_Define_Dtcm_Base +
        sizeof(SIE_PrivateCtx), 8);
    TA_LOGH("[FIGO %d]init success!\n", uFigoId);
    pCtx->bInit = true;
    return TEE_SUCCESS;

error:
    return ret;
}

TEE_FigoContext *tsp_figo_get(uint32_t uFigoId)
{
    return &gFigoCtx[uFigoId];
}

void * tsp_get_dtcmHeap(uint32_t uFigoId, uint32_t memType)
{
    return (memType == DTCM_MEM_TYPE_NONSECURE) ? gFigoCtx[uFigoId].hDtcmHeap :
        gFigoCtx[uFigoId].hSecureDtcmHeap;
}

