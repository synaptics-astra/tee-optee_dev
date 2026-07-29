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
//! \file tsp_ta.c
//!
//! \brief TSP TA Implementation
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    January 14 2014,        Wenjie Zhou
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#define LOG_TAG "TSP_TA"
#include "config.h"
#include "string.h"
#include "tee_internal_api.h"
#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif
#include "tsp_cmd.h"
#include "tsp_ta_internal.h"
#include "tsp_local_data.h"
#include "tsp_shared.h"
#include "tsp_fw_spec.h"
#include "tsp_nsk_alg.h"
#include "tsp_nsk.h"

#define USE_DHUB_LITE_AND_ONE_ITCM

#ifdef USE_DHUB_LITE_AND_ONE_ITCM
#include "api_dhubLite.h"
#include "dHubLite.h"

#define _INCLUDE_TSP_FILTER_ALLINONE_FIGO_TABLE_
#include "tsp_filter_allinone_figo.h"

#else

#include "api_dhub.h"
#define _INCLUDE_TSP_FILTER_PAGE0_FIGO_TABLE_
#include "tsp_filter_page0_figo.h"

#define _INCLUDE_TSP_FILTER_PAGE1_FIGO_TABLE_
#include "tsp_filter_page1_figo.h"
#endif

#include "tsp_config_log.h"
#include "ta_log.h"
#include "tsp_comm.h"
#include "tsp_parse_gop_hdr.h"
#include "tsp_figo.h"
#include "bcm_mailbox.h"
#include "bcm_status.h"

#ifdef CONFIG_ENABLE_DRM
#include "drm_se_ta.h"
#include "drmse_ta_wrap.h"
#endif

static  bool bInitTsp = false;

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

#define VALIDATE_BOOL_VALUE(value) \
      do{ \
        if(value !=0 && value !=1){ \
             TA_LOGE( "Not valid bool value 0x%x\n",value); \
             return  TEE_ERROR_BAD_PARAMETERS; \
        } \
      }while(0)

#define FIGO_REG_START_ADDR(id) (id == 0 ? RA_TspReg_FigoReg : RA_TspReg_Figo1Reg)
#define FIGO_REG_RESET (RA_FigoSysBasic_FIGO0 + RA_FigoReg_figoRstn)

#define TSPREG_FIGO0_BASE   RA_TspReg_FigoDtcm
#define TSPREG_FIGO1_BASE   RA_TspReg_Figo1Dtcm

#define IS_TSP_KERNEL_CMD(cmd) ((cmd >= TSP_KERNEL_CMD_MIN) && (cmd < TSP_KERNEL_CMD_MAX))
#define CMD_TO_INDEX(cmd) (cmd - TSP_KERNEL_CMD_MIN)

static uint32_t *TSPTA_SavRes_buf[TSP_MAX_FIGO_NUM] = {NULL, NULL};    /* suspend/resume: save & restore buffer */
static void *TSPTA_SavRes_va[TSP_MAX_FIGO_NUM] = {NULL, NULL};         /* suspend/resume: save & restore virtual address */
static TSPTA_RegInfo TSPTA_Reg_Info[] = {
/* figo id,                 base addr,                  size(byte) */
    {0     ,RA_TspReg_FigoDtcm - TSPREG_FIGO0_BASE      ,Private_Define_Dtcm_Base   },
    {0     ,RA_TspReg_PidTbl                            ,0x800                      },
    {0     ,RA_TspReg_SecTbl                            ,0x800                      },
    {0     ,RA_TspReg_IntReg + RA_TspIntReg_hw_block_int_enable ,0x34               },
    {1     ,RA_TspReg_Figo1Dtcm - TSPREG_FIGO1_BASE     ,Private_Define_Dtcm_Base   },
};

static uint32_t TSPTA_HBO_FIFO_TSI_list[] = {
    TSP_HBO_FIFO_ID_TSI0_PKT,
    TSP_HBO_FIFO_ID_TSI1_PKT,
    TSP_HBO_FIFO_ID_TSI2_PKT,
    TSP_HBO_FIFO_ID_TSI3_PKT,
    TSP_HBO_FIFO_ID_TSI4_PKT,
    TSP_HBO_FIFO_ID_TSI5_PKT,
    TSP_HBO_FIFO_ID_TSI6_PKT,
    TSP_HBO_FIFO_ID_TSI7_PKT,
    TSP_HBO_FIFO_ID_TSI8_PKT,
    TSP_HBO_FIFO_ID_TSI9_PKT,
    TSP_HBO_FIFO_ID_TSI10_PKT,
    TSP_HBO_FIFO_ID_TSI11_PKT,
    TSP_HBO_FIFO_ID_TSI12_PKT
};

TSPTA_HBOCfg TSPTA_HBOConfig[TSP_MAX_FIGO_NUM][TSP_HBO_FIFO_ID_MAX] = {0};
TSPTA_TSICfg TSPTA_TSIConfig[TSP_TSI_PORT_MAX] = {0};

#ifdef CONFIG_ENABLE_DRM

static TEE_TASessionHandle DRM_MGR_TA_session = NULL;
static TEE_Result Drm_KeyMgr_TA_session_open(void)
{
      TEE_Result res = TEE_SUCCESS;
      const TEE_UUID uuid = DRM_SE_UUID;
      uint32_t        cancellationTimeout = 0;
      uint32_t        core_paramTypes;
      TEE_Param       core_params[4];
      uint32_t        returnOrigin;

      /*Open Session to FIGO TA*/
      if(!DRM_MGR_TA_session) {
           memset(&core_params, 0, 4*sizeof(TEE_Param));
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
          TA_LOGH("open session. result=0x%x, origin=0x%x, session=%p\n",
              res, returnOrigin, DRM_MGR_TA_session);
          if (res != TEE_SUCCESS) {
              TA_LOGF("fail to open session. result=0x%08x, origin=%d\n",
                                       res, returnOrigin);
              return TEE_ERROR_NOT_SUPPORTED;
          }
      }
      return res;
}


static TEE_Result Drm_SE_GetKeyRight(uint32_t HWKeyArea, uint32_t f_keyIndexOffset,
                                    uint32_t* pbProtected, uint32_t* pbCasKey,
                                    uint32_t* pbCopyProtection)
{
    TEE_Result result = TEE_SUCCESS;
    TEE_Result res = TEE_SUCCESS;
    uint32_t cancellationTimeout = 0;
    uint32_t core_paramTypes;
    TEE_Param core_params[4];
    uint32_t returnOrigin;

    memset(&core_params, 0, 4*sizeof(TEE_Param));
    core_paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                      TEE_PARAM_TYPE_VALUE_OUTPUT,
                                      TEE_PARAM_TYPE_VALUE_OUTPUT,
                                      TEE_PARAM_TYPE_VALUE_OUTPUT);
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
        TA_LOGF(
                "error in invoking DRM_SE_KEYRIGHT_CONTENTPROTECT_GET. "
                "result=0x%08x, origin=%d, session=%p\n",
                res, returnOrigin, DRM_MGR_TA_session);
        result = res;
    }
    if(pbProtected)
        *pbProtected = core_params[1].value.a;
    if(pbCopyProtection)
        *pbCopyProtection = core_params[2].value.a;
    if(pbCasKey)
        *pbCasKey = core_params[3].value.a;
    return result;
}

static TEE_Result Drm_SE_KtMgr_GetKeyRight(uint32_t HWKeyArea, uint32_t f_keyIndexOffset,
                                            uint32_t* pbProtected, uint32_t* pbCasKey,
                                            uint32_t* pbCopyProtection)
{
    TEE_Result result = TEE_SUCCESS;

    result = Drm_KeyMgr_TA_session_open();
    if (result == TEE_SUCCESS)
    {
        result = Drm_SE_GetKeyRight(HWKeyArea, f_keyIndexOffset, pbProtected,
                        pbCasKey, pbCopyProtection);
    }

    TEE_CloseTASession(DRM_MGR_TA_session);
    DRM_MGR_TA_session = NULL;
    return result;
}

#else

static TEE_Result Drm_SE_KtMgr_GetKeyRight(uint32_t HWKeyArea, uint32_t f_keyIndexOffset,
                                            uint32_t* pbProtected, uint32_t* pbCasKey,
                                            uint32_t* pbCopyProtection)
{
    TEE_Result res = TEE_SUCCESS;

    return res;
}

#endif

TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    TEE_Result res = TEE_SUCCESS;

    if(!bInitTsp) {
        res = tspmem_create_iomap();
        if(TEE_SUCCESS != res)
            return  TEE_ERROR_STORAGE_NOT_AVAILABLE;
        TSP_FIGO_InitSchedCtx();
        bInitTsp = true;

        for(int i = 0; i < TSP_MAX_FIGO_NUM; i++) {
            res = tsp_figo_init(i);
            if(res) {
                TA_LOGE("FIGO[%d] init fail 0x%x\n", i, res);
                bInitTsp = false;
            }
        }
    }

    return res;
}


void TA_EXPORT TA_DestroyEntryPoint(void)
{
    return;
}

static int  DMXTA_CHECKFIGOID(uint32_t id)
{
    if(id >= TSP_MAX_FIGO_NUM){
        TA_LOGF( "Invalid figo id:0x%x\n", id);
        return TEE_ERROR_BAD_PARAMETERS;
    }
    return TEE_SUCCESS;
}

TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(
    uint32_t        paramTypes,
    TEE_Param        params[4],
    void**            sessionContext)
{
    TEE_TspContext* pTspCtx = NULL;
    HDL_semaphore* sem = NULL;
    uint32_t figo_id = 0;
    TEE_Result res = TEE_SUCCESS;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    figo_id = params[0].value.a;
    res = DMXTA_CHECKFIGOID(figo_id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    pTspCtx = (TEE_TspContext*)TEE_Malloc(sizeof(TEE_TspContext),TEE_MALLOC_FLAG_DEFAULT);
    if(pTspCtx){
        memset(pTspCtx, 0, sizeof(TEE_TspContext));
        pTspCtx->FigoId = figo_id;
        pTspCtx->pSecureCtx =  (SIE_SecureCtx*)tspmem_phy_to_virt(TSP_FIGO_REG_BASE + \
                                (TSP_FIGO_REG_SIZE*figo_id) + \
                                Secure_Define_Dtcm_Base);
        pTspCtx->hdl.mem = TSP_FIGO_REG_BASE + (TSP_FIGO_REG_SIZE*figo_id);
        pTspCtx->hdl.ra = pTspCtx->hdl.mem + TSP_FIGO_DTCM_SIZE;
        sem = &(pTspCtx->hdl.fifoCtl);

#ifdef USE_DHUB_LITE_AND_ONE_ITCM
        sem->ra = pTspCtx->hdl.ra + RA_HBOLite_FiFoCtl;
#else
        sem->ra = pTspCtx->hdl.ra + RA_HBO_FiFoCtl;
#endif

        *sessionContext = (void *)pTspCtx;
    }else{
        TA_LOGF( "Faild to malloc mem for TA CTX\n");
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    return res;

}

void TA_EXPORT TA_CloseSessionEntryPoint(void*  sessionContext)
{
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;
    if(pTspCtx) {
        TEE_Free(pTspCtx);
    }
}

static int  DMXTA_CHECKHBOID(int32_t id)
{
    if(!(id >= TSP_HBO_FIFO_ID_DS_CMD && id <= TSP_HBO_FIFO_ID_TSI12_PKT)){
        TA_LOGE( "%s:%d Invalid HBO FIFO ID:0x%x\n", __FUNCTION__, __LINE__,(id));
        return  TEE_ERROR_BAD_PARAMETERS;
    }
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_EnHboQueue(void* sessionContext,
                                         uint32_t  paramTypes,
                                         TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    int32_t id = params[1].value.a;
    int32_t enable = params[1].value.b;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    if (TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKHBOID(id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

#ifdef USE_DHUB_LITE_AND_ONE_ITCM
    hbo_lite_queue_enable(&(pTspCtx->hdl),id,enable,NULL);
#else
    hbo_queue_enable(&(pTspCtx->hdl),id,enable,NULL);
#endif
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_QueryHboQueue(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    int32_t master  = params[2].value.a;
    int32_t id      = params[2].value.b;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    if (TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 3) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKHBOID(id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

#ifdef USE_DHUB_LITE_AND_ONE_ITCM
     params[3].value.a = hbo_lite_queue_query(&(pTspCtx->hdl), id, master, NULL);
#else
     params[3].value.a = hbo_queue_query(&(pTspCtx->hdl), id, master, NULL);
#endif
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_ClrHboQueue(void* sessionContext,
                                         uint32_t  paramTypes,
                                         TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    int32_t id = params[1].value.a;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    if (TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKHBOID(id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

#ifdef USE_DHUB_LITE_AND_ONE_ITCM
    hbo_lite_queue_clear(&(pTspCtx->hdl), id);
#else
    hbo_queue_clear(&(pTspCtx->hdl), id);
#endif
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_ClrDoneHboQueue(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    int32_t id = params[1].value.a;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    if (TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKHBOID(id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

#ifdef USE_DHUB_LITE_AND_ONE_ITCM
     hbo_lite_queue_clear_done(&(pTspCtx->hdl),  id);
#else
     hbo_queue_clear_done(&(pTspCtx->hdl),  id);
#endif
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_ConfigHboQueue(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    int32_t id = params[1].value.a;
    uint32_t base = params[1].value.b;
    uint32_t depth = params[2].value.a;
    int32_t enable = params[2].value.b;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    if (TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKHBOID(id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    if(TSP_HBO_FIFO_ID_DS_CMD == id ||
       TSP_HBO_FIFO_ID_CRYPTO_CMD == id ||
       TSP_HBO_FIFO_ID_CRYPTO_CMD_1 == id ||
       TSP_HBO_FIFO_ID_CRYPTO_CMD_2 == id){
       if(base < (Secure_Define_Dtcm_Base + sizeof(SIE_SecureCtx))||
            (base > Secure_Define_Dtcm_End) ||
            (depth > ((Secure_Define_Dtcm_End + 1 - base)>>3))){
            TA_LOGE("Invalid HBO config: base=0x%x, depth=0x%x\n",base, depth);
            return TEE_ERROR_ACCESS_DENIED;
       }
    }

    TSPTA_HBOConfig[pTspCtx->FigoId][id].isCfg = true;
    TSPTA_HBOConfig[pTspCtx->FigoId][id].base = base;
    TSPTA_HBOConfig[pTspCtx->FigoId][id].depth = depth;
    TSPTA_HBOConfig[pTspCtx->FigoId][id].enable = enable;
    TSPTA_HBOConfig[pTspCtx->FigoId][id].fifo_id = id;
    TSPTA_HBOConfig[pTspCtx->FigoId][id].figo_id = pTspCtx->FigoId;
    memcpy(&TSPTA_HBOConfig[pTspCtx->FigoId][id].hdl, &(pTspCtx->hdl), sizeof(HDL_hbo));

#ifdef USE_DHUB_LITE_AND_ONE_ITCM
     hbo_lite_queue_cfg(&(pTspCtx->hdl),id,base,depth,enable,NULL);
#else
     hbo_queue_cfg(&(pTspCtx->hdl),id,base,depth,enable,NULL);
#endif
    return TEE_SUCCESS;
}

static int  DMXTA_CHECKMEMTYPE(int32_t type)
{
    if(!(type >= DTCM_MEM_TYPE_NONSECURE && type <= DTCM_MEM_TYPE_SECURE)){
        TA_LOGE( "Invalid dtcm mem type:%d\n", type);
        return  TEE_ERROR_BAD_PARAMETERS;
    }
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_AllocDtcmMem(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t figo_id = params[0].value.a;
    uint32_t mem_type = params[0].value.b;
    uint32_t mem_size = params[1].value.a;
    uint32_t align_size = params[1].value.b;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKFIGOID(figo_id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKMEMTYPE(mem_type);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    params[2].value.a = dtcm_mem_alloc(tsp_get_dtcmHeap(figo_id, mem_type),
        mem_size, align_size);

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_FreeDtcmMem(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t figo_id = params[0].value.a;
    uint32_t mem_type = params[0].value.b;
    uint32_t mem_start = params[1].value.a;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKFIGOID(figo_id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKMEMTYPE(mem_type);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    dtcm_mem_free(tsp_get_dtcmHeap(figo_id, mem_type), mem_start);
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_FWLoad(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    uint32_t uInsNum = 0;
    uintptr_t uPCStartPhy = 0;
    uintptr_t uPCStartVir = 0;
    uint32_t i;

// TSP FW in VS640GenX is loaded by BCM, not TSP TA
// Defining DEBUG_LOAD can force loading the FW by TSP TA, for debugging purpose
#ifdef USE_DHUB_LITE_AND_ONE_ITCM
    TEE_Result ret = TEE_SUCCESS;
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status = {0};
    static bool  gItcmIsLoaded = false;

    // params[0].value.b is force load flag, if it's set to 1
    // tsp.fw is located at SRAM, but TA is located at DDR. After suspend/resume,
    // the SRAM content will be lost, so we need to reload the fw by force.
    if (gItcmIsLoaded && (params[0].value.b != 1)) {
        TA_LOGM("FW already loaded, skip loading again\n");
        return TEE_SUCCESS;
    }

    // tsp.fw's physical address and size are passed in by BCM through params[1].value.a and params[1].value.b,
    // if param type is value input.
    if (TEE_PARAM_TYPE_GET(paramTypes, 1) == TEE_PARAM_TYPE_VALUE_INPUT) {
        // GENX has enc fw in rootfs loaded by kernel
        TA_LOGH("Call BCM to load fw [0x%x] size [0x%x]\n", params[1].value.a,
                                                             params[1].value.b);
        cmd.arg0 = params[1].value.a;
        cmd.arg1 = params[1].value.b;
        // tsp kernel driver has do cache flush, so no need to do cache maintenance here.
        ret = TEE_ExecCmd(BCM_KERNEL_PI_VERIFY_TSP_FW, &cmd, sizeof(BCM_MAILBOX_COMMAND),
                        &status, sizeof(BCM_MAILBOX_STATUS));
        if (ret != TEE_SUCCESS) {
            TA_LOGE("Error in sending load firmware command: 0x%x\n", ret);
            return ret;
        }
        gItcmIsLoaded = true;
    // if param type is none, means non encoded tsp.fw is found. So load clear tsp.fw from TA itself.
    } else if (TEE_PARAM_TYPE_GET(paramTypes, 1) == TEE_PARAM_TYPE_NONE) {
        // GEN2 no enc fw in rootfs, need load inside ta
        uint32_t uInsItcm;
        uPCStartPhy = TSP_FIGO_REG_BASE+ TSP_FIGO0_ITCM_BASE;
        uPCStartVir = tspmem_phy_to_virt(uPCStartPhy);
        uInsNum =  sizeof(tsp_filter_allinone_figo)/4;
        uInsItcm = TSP_FIGO_ITCM_SIZE/4;

        TA_LOGH("In %s:%d, 0x%lx, 0x%x\n",__FUNCTION__,__LINE__, uPCStartPhy, uInsNum);

        for (i = 0; i < uInsItcm; i++)
        {
            *((volatile UNSG32 *)(uPCStartVir + (i<<2))) = tsp_filter_allinone_figo[i];
        }
        uInsNum -= uInsItcm;
        uPCStartVir += (TSP_FIGO1_ITCM_BASE - TSP_FIGO0_ITCM_BASE);
        for (i = 0; i < uInsNum; i++)
        {
            *((volatile UNSG32 *)(uPCStartVir + (i<<2))) = tsp_filter_allinone_figo[i+uInsItcm];
        }
        gItcmIsLoaded = true;
    } else {
        TA_LOGE("Invalid param type for fw load: 0x%x\n", TEE_PARAM_TYPE_GET(paramTypes, 1));
        return TEE_ERROR_BAD_PARAMETERS;
    }
#else
    uint32_t uFwIdx = 0;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;
    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    uFwIdx = params[0].value.a;
    TA_LOGH("%s:%d, uFwIdx = 0x%x\n",__FUNCTION__,__LINE__, uFwIdx);
    if(uFwIdx < 0 || uFwIdx > 1) {
       TA_LOGF("%s:%d, Fw Idx %u not supported\n",__FUNCTION__,__LINE__, uFwIdx);
       return TEE_ERROR_BAD_PARAMETERS;
    }

    uPCStartPhy = pTspCtx->hdl.mem + TSP_FIGO_ITCM_SIZE;
    uPCStartVir = tspmem_phy_to_virt(uPCStartPhy);
    if(uFwIdx == 0) {
        uInsNum =  sizeof(tsp_filter_page0_figo)/4;
    } else if(uFwIdx == 1) {
        uInsNum =  sizeof(tsp_filter_page1_figo)/4;
    }

    TA_LOGH("In %s:%d, 0x%x, 0x%x\n",__FUNCTION__,__LINE__, uPCStartPhy, uInsNum);

    for (i = 0; i < uInsNum; i++)
    {
        if(uFwIdx == 0) {
           *((volatile UNSG32 *)(uPCStartVir + (i<<2))) = tsp_filter_page0_figo[i];
        } else if(uFwIdx == 1) {
           *((volatile UNSG32 *)(uPCStartVir + (i<<2))) = tsp_filter_page1_figo[i];
        }
    }
    TA_LOGH("\n %s %d uFwIdx:%d\n", __FUNCTION__, __LINE__, uFwIdx);
#endif
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_FigoSched(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t uInputType, uTsiPort, uScheme, uCryptoLevel;
    uint32_t uFigoId, uFwIdx, uNeedReload;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_OUTPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 3) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    uInputType = params[0].value.a;
    uTsiPort = params[0].value.b;
    uScheme = params[1].value.a;
    uCryptoLevel = params[1].value.b;

    res = TSP_FIGO_Sched(uInputType, uTsiPort, uScheme, uCryptoLevel,
            &uFigoId, &uFwIdx, &uNeedReload);
    if(res == TEE_SUCCESS){
        params[2].value.a = uFigoId;
        params[2].value.b = uFwIdx;
        params[3].value.a = uNeedReload;
    }
    return res;
}

static TEE_Result TSPTA_FigoUnSched(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t uInputType, uTsiPort, uFigoId;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT )
            return TEE_ERROR_BAD_PARAMETERS;

    uInputType = params[0].value.a;
    uTsiPort = params[0].value.b;
    uFigoId = params[1].value.a;

    res = TSP_FIGO_UnSched(uInputType, uTsiPort, uFigoId);
    return res;
}

static TEE_Result TSPTA_AllocTask(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    uint32_t i;
    uint16_t task_addr;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;
    volatile uint16_t * pTaskItem = (uint16_t *)tspmem_phy_to_virt(pTspCtx->hdl.mem + RA_TspDtcmGlobal_SCHED);

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    task_addr = params[0].value.a;

    if(pTaskItem)
    {
        for (i =0;i<(2*arr_TspDtcmGlobal_TaskList); i++){
            TA_LOGH("%s:%d, pTaskItem[%d] addr 0x%x task_addr 0x%x\n",
                    __FUNCTION__,__LINE__, i, pTaskItem[i], task_addr);
            if (pTaskItem[i] == INVALID_DTCM_ADDR) {
                pTaskItem[i] = task_addr;
                params[1].value.a = i;
                return TEE_SUCCESS;
            }
        }
    }
    params[1].value.a = -1;
    return TEE_ERROR_NOT_IMPLEMENTED;
}

static TEE_Result TSPTA_FreeTask(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    uint16_t task_id;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;
    uint16_t * pTaskItem = (uint16_t *)tspmem_phy_to_virt(pTspCtx->hdl.mem + RA_TspDtcmGlobal_SCHED);

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
                return TEE_ERROR_BAD_PARAMETERS;

    task_id = params[0].value.a;
    TA_LOGH("%s:%d, task_id = 0x%x\n",__FUNCTION__,__LINE__, task_id);
    if(pTaskItem)
    {
        if (task_id < (2*arr_TspDtcmGlobal_TaskList))
        {
            pTaskItem[task_id] = INVALID_DTCM_ADDR;
            return TEE_SUCCESS;
        }
    }
    return TEE_ERROR_BAD_PARAMETERS;
}

static TEE_Result TSPTA_SetFigoState(void *sessionContext,
                                             uint32_t paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result ret = TEE_SUCCESS;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    if (params[0].value.a != 0 && params[0].value.a != 1) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

#if defined(CONFIG_GENX_ENABLE)
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status = {0};

    TA_LOGM("Call BCM to set FIGO[%u] to state[%u]\n",
            pTspCtx->FigoId, params[0].value.a);
    cmd.arg0 = pTspCtx->FigoId;
    cmd.arg1 = params[0].value.a;
    ret = TEE_ExecCmd(BCM_PI_SET_TSP_STATE, &cmd, sizeof(BCM_MAILBOX_COMMAND),
                      &status, sizeof(BCM_MAILBOX_STATUS));
    if (ret != TEE_SUCCESS) {
        TA_LOGE("Error in sending BCM_PI_SET_TSP_STATE: 0x%x\n", ret);
    }
#else
    uintptr_t uiPhyAddr = 0, uiVirtAddr = 0;

    uiPhyAddr = TSP_FIGO_REG_BASE + FIGO_REG_START_ADDR(pTspCtx->FigoId)
                + FIGO_REG_RESET;
    uiVirtAddr = tspmem_phy_to_virt(uiPhyAddr);
    *((volatile UNSG32 *)uiVirtAddr) = params[0].value.a;
#endif

    return ret;
}

static TEE_Result TSPTA_GetFigoState(void *sessionContext,
                                             uint32_t paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result ret = TEE_SUCCESS;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_OUTPUT) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

#if defined(CONFIG_GENX_ENABLE)
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status = {0};

    TA_LOGM("Call BCM to get FIGO[%u] state\n", pTspCtx->FigoId);
    cmd.arg0 = pTspCtx->FigoId;
    ret = TEE_ExecCmd(BCM_PI_GET_TSP_STATE, &cmd, sizeof(BCM_MAILBOX_COMMAND),
                      &status, sizeof(BCM_MAILBOX_STATUS));
    if (ret != TEE_SUCCESS) {
        TA_LOGE("Error in sending BCM_PI_GET_TSP_STATE: 0x%x\n", ret);
    } else {
        TA_LOGM("state: %u\n", status.arg0);
        params[0].value.a = status.arg0;
    }
#else
    uintptr_t uiPhyAddr = 0, uiVirtAddr = 0;

    uiPhyAddr = TSP_FIGO_REG_BASE + FIGO_REG_START_ADDR(pTspCtx->FigoId)
                + FIGO_REG_RESET;
    uiVirtAddr = tspmem_phy_to_virt(uiPhyAddr);
    params[0].value.a = *((volatile UNSG32 *)uiVirtAddr);
#endif

    return ret;
}

static TEE_Result TSPTA_SetFigoClk(void *sessionContext,
                                             uint32_t paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result ret = TEE_SUCCESS;

#if defined(CONFIG_GENX_ENABLE)
    uint32_t bRefClk = 0;
    uint32_t bcm_cmd;
    BCM_MAILBOX_COMMAND cmd;
    BCM_MAILBOX_STATUS status = {0};
    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    bRefClk = params[0].value.a;
    if ((bRefClk && params[0].value.b >= TSP_REF_CLK_SET_MAX) ||
        (!bRefClk && params[0].value.b >= TSP_CLK_SET_MAX)) {
        TA_LOGE("Bad Parameters: bRefClk %d, Clock %d\n", bRefClk, params[0].value.b);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    bcm_cmd = bRefClk ? BCM_PI_SET_TSP_REF_CLK : BCM_PI_SET_TSP_CLK;

    TA_LOGH("Call BCM to set TSP bRef(%u) clk to value[%u]\n",
            bRefClk, params[0].value.b);
    cmd.arg0 = params[0].value.b;
    ret = TEE_ExecCmd(bcm_cmd, &cmd, sizeof(BCM_MAILBOX_COMMAND),
                      &status, sizeof(BCM_MAILBOX_STATUS));
    if (ret != TEE_SUCCESS) {
        TA_LOGE("Error in sending BCM CMD %u, ret = %u\n", bcm_cmd, ret);
    }
#else
    TA_LOGE("Can't support clk setting on the chip version: %u\n",
                    (uint32_t)BERLIN_CHIP_VERSION);
    ret = TEE_ERROR_NOT_SUPPORTED;
#endif

    return ret;
}

static int DMXTA_UpdateKeyProtectFlag(SIE_SecureCtx* pSecureCtx,
        uint32_t keyAddr, uint32_t protect,  uint32_t bCopyProtection) {
    int i = 0;
    int pos = -1;
    while(i < arr_SecureCtx_KeyProtectPair) {
        if(keyAddr == pSecureCtx->ie_KeyProtectPair[i].u_KeyAddr) {
            pSecureCtx->ie_KeyProtectPair[i].u_ProtectFlag = protect;
            pSecureCtx->ie_KeyProtectPair[i].u_bForbidPVR = bCopyProtection;
            TA_LOGH( "update keyAddr=0x%x, protect flag=0x%x at %d\n",
                keyAddr, protect, i);
            return TEE_SUCCESS;
        } else if((pos == -1) && (pSecureCtx->ie_KeyProtectPair[i].u_KeyAddr == 0)) {
            pos = i;
        }
        i++;
    }
    if(pos != -1) {
        pSecureCtx->ie_KeyProtectPair[pos].u_KeyAddr = keyAddr;
        pSecureCtx->ie_KeyProtectPair[pos].u_ProtectFlag = protect;
        pSecureCtx->ie_KeyProtectPair[pos].u_bForbidPVR = bCopyProtection;
        TA_LOGH("update keyAddr=0x%x, protect flag=0x%x copyProtection=0x%x at %d\n",
                keyAddr, protect, bCopyProtection, pos);
        return TEE_SUCCESS;
    }
    TA_LOGF("update keyAddr=0x%x, protect flag=0x%x failed\n",
        keyAddr, protect);
    return TEE_ERROR_OVERFLOW;
}

static void DMXTA_RemoveKeyProtectFlag(uint32_t keyAddr) {
    uint32_t i, j;
    SIE_SecureCtx* pSecureCtx = NULL;

    for(i = 0; i < TSP_MAX_FIGO_NUM; i++) {
        pSecureCtx =  (SIE_SecureCtx*)tspmem_phy_to_virt(TSP_FIGO_REG_BASE +
                                  (TSP_FIGO_REG_SIZE * i) + Secure_Define_Dtcm_Base);
        for(j = 0; j < arr_SecureCtx_KeyProtectPair; j++) {
            if(keyAddr == pSecureCtx->ie_KeyProtectPair[j].u_KeyAddr) {
                pSecureCtx->ie_KeyProtectPair[j].u_KeyAddr = 0;
                pSecureCtx->ie_KeyProtectPair[j].u_ProtectFlag = 0;
                pSecureCtx->ie_KeyProtectPair[j].u_bForbidPVR = 0;
                TA_LOGH("Remove keyAddr=0x%x, protect flag at figo %d\n", keyAddr, i);
            }
        }
    }
}

static bool DMXTA_UpdateCopyProtectionFlag(uint32_t keyAddr,
                                           uint32_t bCopyProtection) {
    bool bFound = false;
    SIE_SecureCtx *pSecureCtx;
    uint32_t i, j;

    for(i = 0; i < TSP_MAX_FIGO_NUM; i++) {
        pSecureCtx = (SIE_SecureCtx *)tspmem_phy_to_virt(TSP_FIGO_REG_BASE +
                                  (TSP_FIGO_REG_SIZE * i) + Secure_Define_Dtcm_Base);

        for(j = 0; j < arr_SecureCtx_KeyProtectPair; j++) {
            if(keyAddr == pSecureCtx->ie_KeyProtectPair[j].u_KeyAddr) {
                pSecureCtx->ie_KeyProtectPair[j].u_bForbidPVR = bCopyProtection;
                TA_LOGH("update keyAddr=0x%x, copyProtection flag=0x%x on figo=%d\n",
                    keyAddr, bCopyProtection, i);
                bFound = true;
                break;
            }
        }
    }

    return  bFound;
}

static TEE_Result TSPTA_UpdateKeyProtectFlag(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4]) {
    uint32_t keyAddr, keyIndex;
    uint32_t keyArea;
    uint32_t protect = 0, bCasKey = 1, bCopyProtection = 0;
    uint32_t nRet = TEE_SUCCESS;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;
    SIE_SecureCtx* pSecureCtx = pTspCtx->pSecureCtx;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    keyArea = params[0].value.a;
    keyIndex = params[0].value.b;

    TA_LOGH("%s:%d, keyArea = %d, keyIndex = 0x%x\n",
        __FUNCTION__, __LINE__, keyArea, keyIndex);

    //get the proctect/bCasKey flag from DRM_SE
    nRet = Drm_SE_KtMgr_GetKeyRight(keyArea, keyIndex, &protect, &bCasKey, &bCopyProtection);
    if(nRet != TEE_SUCCESS){
        TA_LOGF( "KeyTblIndex=0x%x, nRet=0x%x\n", keyIndex, nRet);
        return TEE_ERROR_NO_DATA;
    }
    if(bCasKey == 0) {
        TA_LOGF("Error: Key is not for TS stream descrambling/scrambling!\n");
        return TEE_ERROR_SECURITY;
    }

    // for case keyArea == MV_KEYTABLE_AREA_GENERAL, set default value firstly
    keyAddr = keyIndex | TSP_KEY_TABLE_BASE_ADDR_MASK;

#ifdef CONFIG_ENABLE_DRM
    if(keyArea == MV_KEYTABLE_AREA_HWKL) {
         keyAddr = keyIndex | NAGRA_KEY_TABLE_BASE_ADDR_MASK;
         if(DRM_KEY_TABLE_BASE_INVALID == NAGRA_KEY_TABLE_BASE_ADDR_MASK) {
            TA_LOGF("Get Nagra key in Non-nagra product\n");
            return TEE_ERROR_NOT_SUPPORTED;
         }
    }else if(IS_IFCP_KEYAREA(keyArea)){
        keyAddr = keyIndex | IFCP_KEY_TABLE_BASE_ADDR_MASK;
        if(DRM_KEY_TABLE_BASE_INVALID == IFCP_KEY_TABLE_BASE_ADDR_MASK) {
           TA_LOGF("Get Irdeto key in Non-Irdeto product\n");
           return TEE_ERROR_NOT_SUPPORTED;
        }
    }else if(keyArea == HW_KEYAREA_GKL){
        keyAddr = keyIndex | GKL_KEY_TABLE_BASE_ADDR_MASK;
        if(DRM_KEY_TABLE_BASE_INVALID == GKL_KEY_TABLE_BASE_ADDR_MASK) {
           TA_LOGF("Get GKL key in Non-GKL product\n");
           return TEE_ERROR_NOT_SUPPORTED;
        }
    }
#endif

    //update the protect/copyProtection flag to tsp key table
    nRet = DMXTA_UpdateKeyProtectFlag(pSecureCtx, keyAddr, protect, bCopyProtection);
    if(nRet){
        TA_LOGF( "UpdateKeyProtectFlag fail: keyAddr=0x%x, nRet=0x%x\n",
                                keyAddr, nRet);
        return nRet;
    }

    TA_LOGH("%s:%d, update ok, keyAddr = 0x%x, protect = %d, copyProtection = %d\n",
        __FUNCTION__, __LINE__, keyAddr, protect, bCopyProtection);

    return nRet;
}

static TEE_Result TSPTA_RemoveKeyProtectFlag(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4]) {
    TEE_Result ret = TEE_SUCCESS;

 #ifdef CONFIG_ENABLE_DRM
    uint32_t keyAddr, keyIndex;
    uint32_t keyArea;
    TEE_Identity identity;
    const TEE_UUID uuidZero = {0};

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    keyArea = params[0].value.a;
    keyIndex = params[0].value.b;

    TA_LOGH("%s:%d, keyArea = %d, keyIndex = 0x%x\n",
        __FUNCTION__, __LINE__, keyArea, keyIndex);

    ret = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT, "gpd.client.identity", &identity);
    if(ret != TEE_SUCCESS){
       TA_LOGF( "TEE_GetPropertyAsIdentity fail with 0x%x\n", ret);
       return ret;
    }

    if(!memcmp(&identity.uuid, &uuidZero, sizeof(TEE_UUID))) {
       TA_LOGF( "Error: removing key protection flag from CA is not allowed!\n");
       return TEE_ERROR_ACCESS_DENIED;
    }

    if (keyArea == MV_KEYTABLE_AREA_GENERAL ||
       keyArea == MV_KEYTABLE_AREA_HWKL ||
       keyArea == MV_KEYTABLE_AREA_GKL ||
       IS_IFCP_KEYAREA(keyArea)) {
        switch (keyArea) {
        case MV_KEYTABLE_AREA_GENERAL:
            keyAddr = keyIndex | TSP_KEY_TABLE_BASE_ADDR_MASK;
            break;
        case MV_KEYTABLE_AREA_HWKL:
            keyAddr = keyIndex | NAGRA_KEY_TABLE_BASE_ADDR_MASK;
            break;
        case MV_KEYTABLE_AREA_GKL:
            keyAddr = keyIndex | GKL_KEY_TABLE_BASE_ADDR_MASK;
            break;
        case MV_KEYTABLE_AREA_IRDETO_BRD_HWKL:
        case MV_KEYTABLE_AREA_IRDETO_OTT_HWKL:
        case MV_KEYTABLE_AREA_IRDETO_PVR_HWKL:
            keyAddr = keyIndex | IFCP_KEY_TABLE_BASE_ADDR_MASK;
            break;
        default:
            TA_LOGE("Fatal error on keyArea: %d\n", keyArea);
            return TEE_ERROR_BAD_PARAMETERS;
        }
        DMXTA_RemoveKeyProtectFlag(keyAddr);
    }
#endif
    return ret;
}

static TEE_Result TSPTA_UpdateCopyProtectionFlag(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4]) {
    uint32_t keyArea, keyIndex, keyAddr;
    uint32_t bCopyProtection = 0;
    uint32_t ret = TEE_SUCCESS;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    keyArea = params[0].value.a;
    keyIndex = params[0].value.b;

    TA_LOGH("%s:%d, keyArea = %d, keyIndex = 0x%x\n",
        __FUNCTION__, __LINE__, keyArea, keyIndex);

    //get the copy protection flag from DRM_SE
    ret = Drm_SE_KtMgr_GetKeyRight(keyArea, keyIndex, NULL, NULL, &bCopyProtection);
    if(ret != TEE_SUCCESS){
        TA_LOGF("query bCopyProtection failed! keyArea=%d, keyIndex=0x%x, ret=0x%x\n",
            keyArea, keyIndex, ret);
        return TEE_ERROR_NO_DATA;
    }

    // for case keyArea == MV_KEYTABLE_AREA_GENERAL, set default value firstly
    keyAddr = keyIndex | TSP_KEY_TABLE_BASE_ADDR_MASK;

#ifdef CONFIG_ENABLE_DRM
    if(keyArea == MV_KEYTABLE_AREA_HWKL) {
         keyAddr = keyIndex | NAGRA_KEY_TABLE_BASE_ADDR_MASK;
         if(DRM_KEY_TABLE_BASE_INVALID == NAGRA_KEY_TABLE_BASE_ADDR_MASK) {
            TA_LOGF("Get Nagra key in Non-nagra product\n");
            return TEE_ERROR_NOT_SUPPORTED;
         }
    }else if(IS_IFCP_KEYAREA(keyArea)){
        keyAddr = keyIndex | IFCP_KEY_TABLE_BASE_ADDR_MASK;
        if(DRM_KEY_TABLE_BASE_INVALID == IFCP_KEY_TABLE_BASE_ADDR_MASK) {
           TA_LOGF("Get Irdeto key in Non-Irdeto product\n");
           return TEE_ERROR_NOT_SUPPORTED;
        }
    }else if(keyArea == HW_KEYAREA_GKL){
        keyAddr = keyIndex | GKL_KEY_TABLE_BASE_ADDR_MASK;
        if(DRM_KEY_TABLE_BASE_INVALID == GKL_KEY_TABLE_BASE_ADDR_MASK) {
           TA_LOGF("Get GKL key in Non-GKL product\n");
           return TEE_ERROR_NOT_SUPPORTED;
        }
    }
#endif

    //update the Copy Protection flag to tsp key table
    if (!DMXTA_UpdateCopyProtectionFlag(keyAddr, bCopyProtection)) {
        TA_LOGE("UpdateCopyProtectionFlag fail, not find keyAddr=0x%x\n", keyAddr);
        return TEE_ERROR_ITEM_NOT_FOUND;
    }

    TA_LOGH("%s:%d, update ok, keyAddr = 0x%x, bCopyProtection = %d\n",
        __FUNCTION__, __LINE__, keyAddr, bCopyProtection);

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_GetKeyTokenInfo(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4]) {
    TEE_Result ret = TEE_SUCCESS;

#ifdef CONFIG_ENABLE_DRM
    uint32_t schemeType, uSessionId, count, uMaxOutCnt;
    TEE_TASessionHandle drmSession = 0;
    KEY_UNIT_INFO  *pKeyInfo;
    MV_DRMContentKeyUnitAttribute drmKeyAttr;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_MEMREF_OUTPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    schemeType = params[0].value.a;
    uSessionId = params[0].value.b;
    pKeyInfo = (KEY_UNIT_INFO *)params[1].memref.buffer;
    uMaxOutCnt    = params[1].memref.size / sizeof(KEY_UNIT_INFO);

    TA_LOGH("%s:%d, schemeType = %d, uSessionID = 0x%x\n",
        __FUNCTION__, __LINE__, schemeType, uSessionId);

    ret = drmse_openSession(&drmSession);
    if(ret != TEE_SUCCESS){
        TA_LOGF( "drmse_openSession failed, ret=0x%x\n", ret);
        return ret;
    }

    ret = drmse_getContentKeyUnitCount(drmSession, schemeType, uSessionId, &count);
    TSP_CHECK_RETCODE(ret);

    if(count > uMaxOutCnt){
        TA_LOGF( "error: KeyUnit Count(0x%x) is bigger than the max value(0x%x)\n",
            count, uMaxOutCnt);
        ret = TEE_ERROR_EXCESS_DATA;
        goto end;
    }
    params[2].value.a = count;
    for(uint32_t i = 0; i < count; i++) {
        ret = drmse_getContentKeyUnitAttrInfoExt(drmSession, schemeType, uSessionId,
                                i, &drmKeyAttr, pKeyInfo->key, pKeyInfo->iv);
        TSP_CHECK_RETCODE(ret);
        pKeyInfo->keyAttr.e_AlgType = drmKeyAttr.e_AlgType;
        pKeyInfo->keyAttr.e_AlgMode = drmKeyAttr.e_AlgMode;
        pKeyInfo->keyAttr.e_ResidualMode = drmKeyAttr.e_ResidualMode;
        pKeyInfo->keyAttr.e_SolitaryMode = drmKeyAttr.e_SolitaryMode;
        pKeyInfo->keyAttr.e_Polarity = drmKeyAttr.e_Polarity;
        pKeyInfo->keyAttr.e_ValueType = drmKeyAttr.e_ValueType;
        pKeyInfo->keyAttr.e_IVType = drmKeyAttr.e_IVType;
        pKeyInfo->keyAttr.e_KeyUsage = drmKeyAttr.e_KeyUsage;
        pKeyInfo->keyAttr.uKeyIDLen = drmKeyAttr.uKeyIDLen;
        pKeyInfo->keyAttr.uHWKeyArea = drmKeyAttr.uHWKeyArea;
        if(drmKeyAttr.uHWKeyArea == MV_KEYTABLE_AREA_GENERAL) {
            pKeyInfo->keyAttr.uHWKeyIndex = drmKeyAttr.uHWKeyIndex / 2;
            pKeyInfo->keyAttr.uHWIVIndex = drmKeyAttr.uHWIVIndex / 2;
        }else {
            pKeyInfo->keyAttr.uHWKeyIndex = drmKeyAttr.uHWKeyIndex;
            pKeyInfo->keyAttr.uHWIVIndex = drmKeyAttr.uHWIVIndex;
        }
        pKeyInfo->keyAttr.uKeyDataLen = drmKeyAttr.uKeyDataLen;
        pKeyInfo->keyAttr.uIVLen = drmKeyAttr.uIVLen;

        pKeyInfo++;
    }

    TA_LOGH("%s:%d, GetKeyTokenInfo ok, count = 0x%x\n",
        __FUNCTION__, __LINE__, count);
end:
    if(drmSession) {
        drmse_closeSession(drmSession);
    }
#else
    ret = TEE_ERROR_NOT_SUPPORTED;
#endif
    return ret;
}

static TEE_Result TSPTA_UpdateHboinFilters(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    uint32_t i, uInputBuf;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;
    uint16_t firstFilt = 0, *pTaskItem = (uint16_t *)tspmem_phy_to_virt
        (pTspCtx->hdl.mem + RA_TspDtcmGlobal_SCHED);
    uint32_t totalNr = 0, activeNr = 0;
    volatile SIE_TsDmxFilter *pDmxCtxEntry, *activeFilter[arr_TspDtcmGlobal_TsDmxFilterTable];
    SIE_TsDmxFilter tempFilt;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    uInputBuf = params[0].value.a;
    if(uInputBuf < RA_TspDtcmGlobal_HBOIN || uInputBuf >= RA_TspDtcmGlobal_VESHDR) {
        TA_LOGE("invalid hboin address 0x%x\n", uInputBuf);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    TA_LOGM("%s:%d, uInputBufOff = 0x%x\n",__FUNCTION__,__LINE__, uInputBuf);

    for (i = 0; i < (2 * arr_TspDtcmGlobal_TaskList); i++){
        if(pTaskItem[i] >= RA_TspDtcmGlobal_DMX && pTaskItem[i] < RA_TspDtcmGlobal_PARTSEC) {
            pDmxCtxEntry = (volatile SIE_TsDmxFilter *)
                tspmem_phy_to_virt(pTspCtx->hdl.mem + pTaskItem[i]);
            if(pDmxCtxEntry->uINPUT_pInputBuf == uInputBuf) {
                //clear all the flags firstly
                pDmxCtxEntry->u32TsDmxFilter_SharedTSI = 0;
                if(pDmxCtxEntry->ie_TskHdr.u_TaskState == TaskHeader_TaskState_RUNNING) {
                    if(activeNr == 0)
                        firstFilt = pTaskItem[i];
                    activeFilter[activeNr++] = pDmxCtxEntry;
                }
                totalNr++;
            }
        }
    }

    if(totalNr > 1 && activeNr) {
        for(i = 0; i < activeNr; i++){
            tempFilt.u32TsDmxFilter_SharedTSI = 0;
            tempFilt.uSharedTSI_IsShareTsi = 1;
            tempFilt.uSharedTSI_TsiFirstInst = firstFilt;
            if(i == 0)
                tempFilt.uSharedTSI_IsTsiFirstInst = 1;
            if(i == (activeNr - 1))
                tempFilt.uSharedTSI_IsTsiLastInst = 1;
            activeFilter[i]->u32TsDmxFilter_SharedTSI = tempFilt.u32TsDmxFilter_SharedTSI;
        }
    }

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_GetOutBufPropFromSecureRule(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4]) {
    TEE_Result ret = TEE_SUCCESS;
    int32_t bCipher = params[0].value.a;
    int32_t keyArea = params[0].value.b;
    int32_t keyIndex = params[1].value.a;
    uint32_t bProtected = 0;
    int32_t keyOption = params[1].value.b;
    int32_t bInput = params[2].value.a;
    int32_t bOutput = 0;
    int32_t index = 0, iKeyOptionVal = 0;

    TA_LOGH("%s:%d, TSPTA_GetOutBufPropFromSecureRule bCipher %d," \
        "keyArea %d, keyIndex %d, bKeyOption %d, bInput %d\n",
        __FUNCTION__, __LINE__, bCipher, keyArea, keyIndex, keyOption, bInput);

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_INOUT) {
        ret = TEE_ERROR_BAD_PARAMETERS;
        goto end;
    }

    VALIDATE_BOOL_VALUE(bCipher);
    VALIDATE_BOOL_VALUE(bInput);

#ifdef CONFIG_ENABLE_DRM
    if(keyOption == DMX_TSP_KEY_TBLIDX) {
        //when nsk area, the input keyIndex is cmchannelIdx
        if(keyArea == MV_KEYTABLE_AREA_NSK_HWKL)
            ret = TSPTA_GetNSKCmChannelKeyRight(sessionContext, keyIndex, &bProtected);
        else
            ret = Drm_SE_KtMgr_GetKeyRight(keyArea, keyIndex, &bProtected, NULL, NULL);

        if(ret){
            TA_LOGF( "KeyTblIndex=0x%x, nRet=0x%x\n",
                                    keyIndex, ret);
            ret = TEE_ERROR_NO_DATA;
            goto end;
        }
        iKeyOptionVal = 1;
    } else {
        iKeyOptionVal = 0;
    }

    VALIDATE_BOOL_VALUE(bProtected);

    index =  (bCipher << 4) | (bProtected << 3) | (iKeyOptionVal << 2) | (bInput << 1);
    if(SecureRuleTbl[index] != 1) { // case bOutput == 0
        if(SecureRuleTbl[index + 1] == 1) { // case bOutput == 1
            bOutput = 1;
        } else {
            ret = TEE_ERROR_SECURITY;
            goto end;
        }
    }

#endif
    params[2].value.b = bOutput;

    TA_LOGH("%s:%d, TSPTA_GetOutBufPropFromSecureRule ok, %d\n",
        __FUNCTION__, __LINE__, bOutput);
end:
    return ret;
}

static TEE_Result TSPTA_GetPrivateAreas(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4]) {
    TEE_Result res = TEE_SUCCESS;
    int32_t figo_id = params[0].value.a;
    TEE_FigoContext *pFigoCtx;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_OUTPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    res = DMXTA_CHECKFIGOID(figo_id);
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    pFigoCtx = tsp_figo_get(figo_id);
    if(pFigoCtx->bInit) {
        params[1].value.a = pFigoCtx->uSecureRuleTableAddr;
        params[1].value.b = pFigoCtx->u_pLocalData_all_filter;
    }else {
        res = TEE_ERROR_BAD_STATE;
    }

    return res;
}

static TEE_Result TSPTA_MemWrapMove(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_ERROR_GENERIC;
    TEE_MemRegion region;
    TA_TSP_BUFF_T destBuf, srcBuf;

    uint32_t uDest = params[0].value.a;
    uint32_t uSrc  = params[0].value.b;
    uint32_t uSize = params[1].value.a;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT)
            return TEE_ERROR_BAD_PARAMETERS;

    memset(&destBuf, 0, sizeof(TA_TSP_BUFF_T));
    memset(&srcBuf, 0, sizeof(TA_TSP_BUFF_T));

    //currently only ves buffer has bound memid
    res = tspbuf_create_with_pa(&srcBuf, uSrc, uSize, true);
    TSP_CHECK_RETCODE(res);

    //for ves buffer, these two address belog to one same memid,
    //make sure it supports multiple mapping in one ta instance
    res = tspbuf_create_with_pa(&destBuf, uDest, uSize, true);
    TSP_CHECK_RETCODE(res);


    res = TEE_FindMemRegion(&region, destBuf.virAddr, uSize);
    TSP_CHECK_RETCODE(res);

    if(!MR_IS_SECURE(&region)){
        TA_LOGE( "%s non-secure dest mem region\n",__FUNCTION__);
        res = TEE_ERROR_BAD_PARAMETERS;
        goto end;
    }

    res = TEE_FindMemRegion(&region, srcBuf.virAddr, uSize);
    TSP_CHECK_RETCODE(res);

    if(!MR_IS_SECURE(&region)){
        TA_LOGE( "%s non-secure src mem region\n",__FUNCTION__);
        res = TEE_ERROR_BAD_PARAMETERS;
        goto end;
    }
    //8MB is the max es/pes buffer size, only support S-S wrap move,
    //To avoid potential issue, enlarge the max limitation to 16M;
    //256 KB is the min diff of src and dest (uSize=padding), in AES channel(AES/VES/PES),
    //To support SecFlt buf mem move, reduce the min limitation to 2k.
    if(((uSrc + 0x1000000) >= uDest) && ((uSrc + 0x800) < uDest)) {
       memcpy(destBuf.virAddr, srcBuf.virAddr, uSize);
    } else {
      TA_LOGE( "%s:uSrc,uDest,uSize=0x%x,0x%x,0x%x\n",   \
                                       __FUNCTION__,uSrc,uDest,uSize);
      res =   TEE_ERROR_BAD_PARAMETERS;
    }

end:
    tspbuf_release(&destBuf);
    tspbuf_release(&srcBuf);
    return res;
}

static TEE_Result TSPTA_Section_Get_Syntax_Indicator (void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t uSyntaxIndicator;
    uint8_t* data = NULL;
    TA_TSP_BUFF_T inputBuf;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INOUT)
            return TEE_ERROR_BAD_PARAMETERS;

    memset(&inputBuf, 0, sizeof(TA_TSP_BUFF_T));
    res = tspbuf_create_with_pa(&inputBuf, params[0].value.a, 8, true); //read only one byte
    if(TEE_SUCCESS != res)
        return  TEE_ERROR_BAD_PARAMETERS;

    data = (uint8_t*)inputBuf.virAddr;
    uSyntaxIndicator = (data[0] & 0x80) >> 7;
    params[0].value.b = uSyntaxIndicator;
    TA_LOGM("uSyntaxIndicator %d\n", uSyntaxIndicator);
    tspbuf_release(&inputBuf);
    return TEE_SUCCESS;
}

#define AMP_MEMINFO_FLAG_DATAINVALID_MASK  0x00000400 /**< to indicate data is valid or not*/
#define SECTION_PATTERN_COEF_OFFSET 0
#define SECTION_PATTERN_NEG_OFFSET  16
#define SECTION_PATTERN_MASK_OFFSET 32

static TEE_Result TSPTA_Section_Get_Data_Invalid_Mask(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result res = TEE_SUCCESS;
    bool bNegDataValid = false;
    uint8_t* StartPtr = NULL;
    uint8_t* pCNMPtr = NULL;
    uint32_t i = 0, j = 0, BitMask = 0, uNegFlag = 0, uPosFlag = 0, uFlag = 0;
    TA_TSP_BUFF_T dataBuf, CNMBuf;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_OUTPUT) {
            TA_LOGE( "%s bad parameter\n",__FUNCTION__);
            return TEE_ERROR_BAD_PARAMETERS;
    }

    TA_LOGE("PhyAddr 0x%x, PhyCNMAddr 0x%x\n", params[0].value.a, params[0].value.b);
    memset(&dataBuf, 0, sizeof(TA_TSP_BUFF_T));
    memset(&CNMBuf, 0, sizeof(TA_TSP_BUFF_T));

    //the needed size of these two buf are 16 and 48, anyway the real map size is 4k aligned
    res = tspbuf_create_with_pa(&dataBuf, params[0].value.a, 16, true);
    TSP_CHECK_RETCODE(res);
    res = tspbuf_create_with_pa(&CNMBuf, params[0].value.b, 48, true);
    TSP_CHECK_RETCODE(res);

    StartPtr = dataBuf.virAddr;
    pCNMPtr = CNMBuf.virAddr;
    for(i=0; i<16; i++) {
        for(j=0; j<8; j++) {
            BitMask = 1<<j;
            /*check Negtive case*/
            if((BitMask & pCNMPtr[SECTION_PATTERN_MASK_OFFSET+i]) && (BitMask & pCNMPtr[SECTION_PATTERN_NEG_OFFSET+i])) {
                if( false == bNegDataValid ) {
                    if((BitMask & StartPtr[i]) != (BitMask & pCNMPtr[SECTION_PATTERN_COEF_OFFSET+i])) {
                        uNegFlag &= ~AMP_MEMINFO_FLAG_DATAINVALID_MASK;
                        bNegDataValid = true;
                    }
                    else {
                        uNegFlag |= AMP_MEMINFO_FLAG_DATAINVALID_MASK;
                    }
                } else {
                    continue;
                }
            }
            /*check Positive case*/
            if((BitMask & pCNMPtr[SECTION_PATTERN_MASK_OFFSET+i]) && (!(BitMask & pCNMPtr[SECTION_PATTERN_NEG_OFFSET+i]))) {
                if((BitMask & StartPtr[i]) != (BitMask & pCNMPtr[SECTION_PATTERN_COEF_OFFSET+i])) {
                    uPosFlag |= AMP_MEMINFO_FLAG_DATAINVALID_MASK;
                    break;
                }
                else {
                    uPosFlag &= ~AMP_MEMINFO_FLAG_DATAINVALID_MASK;
                }
            }
        }
        TA_LOGL("%2d: data %x, coef %4x, neg %4x, mask %4x\n", i, StartPtr[i],
            pCNMPtr[SECTION_PATTERN_COEF_OFFSET+i],
            pCNMPtr[SECTION_PATTERN_NEG_OFFSET+i],
            pCNMPtr[SECTION_PATTERN_MASK_OFFSET+i]);

        if((uNegFlag & AMP_MEMINFO_FLAG_DATAINVALID_MASK) ||
            (uPosFlag & AMP_MEMINFO_FLAG_DATAINVALID_MASK)) {

            uFlag |= AMP_MEMINFO_FLAG_DATAINVALID_MASK;
            TA_LOGH("Set DATAINVALID_MASK flag, Data[%d]=0x%02x, Coef[%d]=0x%02x," \
                "Neg[%d]=0x%02x, Mask[%d]=0x%02x, uFlag=%d\n",
                i, StartPtr[i], i, pCNMPtr[SECTION_PATTERN_COEF_OFFSET+i],
                i, pCNMPtr[SECTION_PATTERN_NEG_OFFSET+i], i, pCNMPtr[SECTION_PATTERN_MASK_OFFSET+i], uFlag);
            break;
        }
    }

    params[1].value.a = uFlag;

end:
    tspbuf_release(&dataBuf);
    tspbuf_release(&CNMBuf);
    return res;

}

static TEE_Result TSPTA_Check_Gop_Hdr(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    TEE_Result ret = TEE_SUCCESS;
    TEE_MemRegion region;
    uint32_t uCodec = params[1].value.a;
    TA_TSP_BUFF_T dataBuf;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INOUT) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    params[1].value.b = 0;

    if (!IS_VALID_CODEC(uCodec)) {
        TA_LOGE( "invalid video codec 0x%x\n",uCodec);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    memset(&dataBuf, 0, sizeof(TA_TSP_BUFF_T));
    ret = tspbuf_create_with_pa(&dataBuf, params[0].value.a, params[0].value.b, false);
    TSP_CHECK_RETCODE(ret);

    ret = TEE_FindMemRegion(&region, dataBuf.virAddr, dataBuf.uBufSize);
    TSP_CHECK_RETCODE(ret);

    if(MR_IS_SECURE(&region)) {
        /*Invalidate Cache, seems TEE can only invalidate the secure mem*/
        TA_LOGL("%s: secure output mem %p\n", __FUNCTION__, dataBuf.virAddr);
        ret = TEE_InvalidateCache(dataBuf.virAddr, dataBuf.uBufSize);
        TSP_CHECK_RETCODE(ret);
    }

    params[1].value.b = tsp_check_gop_hdr(dataBuf.virAddr, dataBuf.uBufSize, uCodec);

end:
    tspbuf_release(&dataBuf);
    return ret;
}

static TEE_Result TSPTA_SaveCfgTSI(uint32_t figo_id)
{
    for(int uTsiIdx = 0; uTsiIdx < TSP_TSI_PORT_MAX; uTsiIdx++) {
        volatile SIE_TspTsiReg *pReg = (volatile SIE_TspTsiReg *)((uintptr_t)TSPTA_SavRes_va[figo_id] +
            RA_TspReg_Tsi0Reg + uTsiIdx * sizeof(SIE_TspTsiReg));

        TSPTA_TSIConfig[uTsiIdx].ResetCtrl = *(volatile uint32_t *)(&(pReg->u32TspTsiReg_global));
        TSPTA_TSIConfig[uTsiIdx].PortCtrl = *(volatile uint32_t *)(&(pReg->u32TspTsiReg_port_ctrl));
        TSPTA_TSIConfig[uTsiIdx].SyncDetect = *(volatile uint32_t *)(&(pReg->u32TspTsiReg_sync_detect));
        TSPTA_TSIConfig[uTsiIdx].PacketFormat = *(volatile uint32_t *)(&(pReg->u32TspTsiReg_packet_format));
        TSPTA_TSIConfig[uTsiIdx].ErrorDetect = *(volatile uint32_t *)(&(pReg->u32TspTsiReg_error_detect));
        TSPTA_TSIConfig[uTsiIdx].PidFilter = *(volatile uint32_t *)(&(pReg->u32TspTsiReg_pid_filter));
        TSPTA_TSIConfig[uTsiIdx].FifoSelect = *(volatile uint32_t *)(&(pReg->u32TspTsiReg_fifo_select));
        TSPTA_TSIConfig[uTsiIdx].StcClkDiv = *(volatile uint32_t *)(&(pReg->u32TspTsiReg_stc_clock_div));

        //reset
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_global)) = 0x1;
    }

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_SaveCfgSFReg(uint32_t figo_id)
{
    volatile SIE_TspSecReg *pReg_SecReg = (SIE_TspSecReg *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspReg_SecReg);

    //reset section filter
    pReg_SecReg->uglobal_reset = 1;

    //save all rules
    *(TSPTA_SavRes_buf[figo_id] + (RA_TspReg_SecReg + RA_TspSecReg_rule_0  + RA_TspSecRegRule_status) / 4) =
        pReg_SecReg->ie_rule_0.u32TspSecRegRule_status;
    *(TSPTA_SavRes_buf[figo_id] + (RA_TspReg_SecReg + RA_TspSecReg_rule_32 + RA_TspSecRegRule_status) / 4) =
        pReg_SecReg->ie_rule_32.u32TspSecRegRule_status;
    *(TSPTA_SavRes_buf[figo_id] + (RA_TspReg_SecReg + RA_TspSecReg_rule_64 + RA_TspSecRegRule_status) / 4) =
        pReg_SecReg->ie_rule_64.u32TspSecRegRule_status;
    *(TSPTA_SavRes_buf[figo_id] + (RA_TspReg_SecReg + RA_TspSecReg_rule_96 + RA_TspSecRegRule_status) / 4) =
        pReg_SecReg->ie_rule_96.u32TspSecRegRule_status;

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_SaveHWContext(void* sessionContext,
                                             uint32_t  paramTypes,
                                             TEE_Param params[4])
{
    uint32_t i, j;
    uint32_t arr_size;
    uint32_t figo_id;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    figo_id = pTspCtx->FigoId;
    if(figo_id >= TSP_MAX_FIGO_NUM)
        return TEE_ERROR_BAD_PARAMETERS;

    if(TSPTA_SavRes_va[figo_id] == NULL) {
        TSPTA_SavRes_va[figo_id] = (uint32_t *)tspmem_phy_to_virt(TSP_FIGO_REG_BASE + (TSP_FIGO_REG_SIZE * figo_id));
    }

    if(TSPTA_SavRes_buf[figo_id] == NULL) {
        TSPTA_SavRes_buf[figo_id] = (uint32_t *)TEE_Malloc(TSP_FIGO_REG_SIZE, TEE_MALLOC_FLAG_DEFAULT);
    }
    if(!TSPTA_SavRes_buf[figo_id]) {
        TA_LOGE( "Faild to malloc buffer for figo %d mem region\n", figo_id);
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    arr_size = (uint32_t)sizeof(TSPTA_Reg_Info) / (uint32_t)sizeof(TSPTA_RegInfo);
    for(i = 0; i < arr_size; i++) {
        if(TSPTA_Reg_Info[i].figo_id == figo_id) {
            for(j = 0; j < (TSPTA_Reg_Info[i].size / 4); j++) {
                *(TSPTA_SavRes_buf[figo_id] + (TSPTA_Reg_Info[i].base / 4) + j) =
                    *(volatile uint32_t *)((uint32_t *)TSPTA_SavRes_va[figo_id] + (TSPTA_Reg_Info[i].base / 4) + j);
            }
        }
    }

    if(figo_id == 0) {
        TSPTA_SaveCfgTSI(figo_id);
        TSPTA_SaveCfgSFReg(figo_id);
    }

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreHBOFifoCfg(uint32_t figo_id, uint32_t fifo_id)
{
    if(TSPTA_HBOConfig[figo_id][fifo_id].isCfg) {
#ifdef USE_DHUB_LITE_AND_ONE_ITCM
        hbo_lite_queue_cfg(&(TSPTA_HBOConfig[figo_id][fifo_id].hdl),
            TSPTA_HBOConfig[figo_id][fifo_id].fifo_id,
            TSPTA_HBOConfig[figo_id][fifo_id].base,
            TSPTA_HBOConfig[figo_id][fifo_id].depth,
            TSPTA_HBOConfig[figo_id][fifo_id].enable,
            NULL);
#else
        TA_LOGE( "Old chips are not supported\n" );
        return TEE_ERROR_NOT_SUPPORTED;
#endif
    }
    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreCfgDS(uint32_t figo_id)
{
    volatile SIE_LocalArea_Ext *pLa = (SIE_LocalArea_Ext *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspDtcmGlobal_LA);
#if defined(CONFIG_GENX_ENABLE)
    volatile SIE_SecureCtx *pSecureCtx = (SIE_SecureCtx *)((uintptr_t)TSPTA_SavRes_va[figo_id] + Secure_Define_Dtcm_Base);

    pSecureCtx->ie_SuspendResumeCtx.u_bNeedInitDs = 1;
#else
    volatile SIE_DataStreamer *pReg = (SIE_DataStreamer *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspReg_FigoReg + RA_FigoSysBasic_DS);

    pReg->ie_dsCh[0].uretID_addr = RA_TspDtcmGlobal_LA + BA_LocalArea_uDsCmdIdRet;
    pReg->ie_dsCh[0].uinitID_Val = 0;
#endif

    TSPTA_RestoreHBOFifoCfg(figo_id, TSP_HBO_FIFO_ID_DS_CMD);

    pLa->u_uDsCmdIdRet = 0xffff;
    pLa->u_uNextDsCmdId = 0;
    pLa->ie_hboFifoList[TSP_HBO_FIFO_ID_DS_CMD].u_nWrOff = 0;

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreCfgCryptoEngine(uint32_t figo_id)
{
    volatile SIE_LocalArea_Ext *pLa = (SIE_LocalArea_Ext *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspDtcmGlobal_LA);

    TSPTA_RestoreHBOFifoCfg(figo_id, TSP_HBO_FIFO_ID_CRYPTO_CMD);
    TSPTA_RestoreHBOFifoCfg(figo_id, TSP_HBO_FIFO_ID_CRYPTO_CMD_1);
    TSPTA_RestoreHBOFifoCfg(figo_id, TSP_HBO_FIFO_ID_CRYPTO_CMD_2);

    //reset context of command fifo 0 return address
    pLa->ie_hboFifoList[TSP_HBO_FIFO_ID_CRYPTO_CMD].u_nWrOff = 0;
    pLa->ie_CryptoRtn[TSP_HBO_FIFO_ID_CRYPTO_CMD - TSP_HBO_FIFO_ID_CRYPTO_CMD].uLocal_uLocalCmdCnt = 0;
    pLa->ie_CryptoRtn[TSP_HBO_FIFO_ID_CRYPTO_CMD - TSP_HBO_FIFO_ID_CRYPTO_CMD].uRegSyn_uCryptoEngineUpdateCnt = 0;

    //reset context of command fifo 1 return address
    pLa->ie_hboFifoList[TSP_HBO_FIFO_ID_CRYPTO_CMD_1].u_nWrOff = 0;
    pLa->ie_CryptoRtn[TSP_HBO_FIFO_ID_CRYPTO_CMD_1 - TSP_HBO_FIFO_ID_CRYPTO_CMD].uLocal_uLocalCmdCnt = 0;
    pLa->ie_CryptoRtn[TSP_HBO_FIFO_ID_CRYPTO_CMD_1 - TSP_HBO_FIFO_ID_CRYPTO_CMD].uRegSyn_uCryptoEngineUpdateCnt = 0;

    //reset context of command fifo 2 return address
    pLa->ie_hboFifoList[TSP_HBO_FIFO_ID_CRYPTO_CMD_2].u_nWrOff = 0;
    pLa->ie_CryptoRtn[TSP_HBO_FIFO_ID_CRYPTO_CMD_2 - TSP_HBO_FIFO_ID_CRYPTO_CMD].uLocal_uLocalCmdCnt = 0;
    pLa->ie_CryptoRtn[TSP_HBO_FIFO_ID_CRYPTO_CMD_2 - TSP_HBO_FIFO_ID_CRYPTO_CMD].uRegSyn_uCryptoEngineUpdateCnt = 0;

    return TEE_SUCCESS;
}

#if defined(CONFIG_GENX_ENABLE)
static TEE_Result TSPTA_InitCryptoEngineRegAccessLock(uint32_t figo_id)
{
    //Since figo0 and figo1 do not share the same lock.
    //This register is temporarily used as the reset lock.
    //If there is a better method later, you can modify it.
    volatile SIE_TspTsoReg *pReg = (SIE_TspTsoReg *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspReg_Tso1Reg);

    pReg->uport_ctrl_serial_pin_select = 0;

    return TEE_SUCCESS;
}
#endif

static TEE_Result TSPTA_RestoreCfgCryptoEngineReg(uint32_t figo_id)
{
#if defined(CONFIG_GENX_ENABLE)
    TSPTA_InitCryptoEngineRegAccessLock(figo_id);
#else
    volatile SIE_TspCryReg *pReg = (SIE_TspCryReg *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspReg_CryReg);

    //reset crypro engine
    pReg->uglobal_reset = 1;

    //release crypro engine
    pReg->uglobal_reset = 0;
#endif

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreCfgSF(uint32_t figo_id)
{
    volatile SIE_LocalArea_Ext *pLa = (SIE_LocalArea_Ext *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspDtcmGlobal_LA);

    TSPTA_RestoreHBOFifoCfg(figo_id, TSP_HBO_FIFO_ID_SF_INPUT);
    TSPTA_RestoreHBOFifoCfg(figo_id, TSP_HBO_FIFO_ID_SF_OUTPUT);

    pLa->ie_hboFifoList[TSP_HBO_FIFO_ID_SF_INPUT].u_nWrOff = 0;
    pLa->ie_hboFifoList[TSP_HBO_FIFO_ID_SF_OUTPUT].u_nRdOff = 0;

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreCfgSFReg(uint32_t figo_id)
{
    volatile SIE_TspSecReg *pReg_SecReg = (SIE_TspSecReg *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspReg_SecReg);
    uint32_t rule_val = 0;

    //reset section filter
    pReg_SecReg->uglobal_reset = 1;

    //restore rules
    rule_val = *(TSPTA_SavRes_buf[figo_id] + (RA_TspReg_SecReg + RA_TspSecReg_rule_0  + RA_TspSecRegRule_status) / 4);
    pReg_SecReg->ie_rule_0.u32TspSecRegRule_enable = rule_val;
    pReg_SecReg->ie_rule_0.u32TspSecRegRule_disable = ~rule_val;

    rule_val = *(TSPTA_SavRes_buf[figo_id] + (RA_TspReg_SecReg + RA_TspSecReg_rule_32 + RA_TspSecRegRule_status) / 4);
    pReg_SecReg->ie_rule_32.u32TspSecRegRule_enable = rule_val;
    pReg_SecReg->ie_rule_32.u32TspSecRegRule_disable = ~rule_val;

    rule_val = *(TSPTA_SavRes_buf[figo_id] + (RA_TspReg_SecReg + RA_TspSecReg_rule_64 + RA_TspSecRegRule_status) / 4);
    pReg_SecReg->ie_rule_64.u32TspSecRegRule_enable = rule_val;
    pReg_SecReg->ie_rule_64.u32TspSecRegRule_disable = ~rule_val;

    rule_val = *(TSPTA_SavRes_buf[figo_id] + (RA_TspReg_SecReg + RA_TspSecReg_rule_96 + RA_TspSecRegRule_status) / 4);
    pReg_SecReg->ie_rule_96.u32TspSecRegRule_enable = rule_val;
    pReg_SecReg->ie_rule_96.u32TspSecRegRule_disable = ~rule_val;

    //release section filter
    pReg_SecReg->uglobal_reset = 0;

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreCfgSWD(uint32_t figo_id)
{
    TSPTA_RestoreHBOFifoCfg(figo_id, TSP_HBO_FIFO_ID_SWD_CMD);
    TSPTA_RestoreHBOFifoCfg(figo_id, TSP_HBO_FIFO_ID_SWD_RETURN);

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreCfgSWDReg(uint32_t figo_id)
{
    volatile SIE_TspSwdReg *pReg = (SIE_TspSwdReg *)((uintptr_t)TSPTA_SavRes_va[figo_id] + RA_TspReg_SwdReg);

    //reset swd
    pReg->uglobal_reset = 1;

    //release swd
    pReg->uglobal_reset = 0;

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreCfgTSI(uint32_t figo_id)
{
    for(uint32_t uTsiIdx = 0; uTsiIdx < TSP_TSI_PORT_MAX; uTsiIdx++) {
        TSPTA_RestoreHBOFifoCfg(figo_id, TSPTA_HBO_FIFO_TSI_list[uTsiIdx]);
    }

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreCfgTSIReg(uint32_t figo_id)
{
    for(uint32_t uTsiIdx = 0; uTsiIdx < TSP_TSI_PORT_MAX; uTsiIdx++) {
        volatile SIE_TspTsiReg *pReg = (volatile SIE_TspTsiReg *)((uintptr_t)TSPTA_SavRes_va[figo_id] +
            RA_TspReg_Tsi0Reg + uTsiIdx * sizeof(SIE_TspTsiReg));

        //Config below register need to reset
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_global)) = 0x1;

        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_port_ctrl)) = TSPTA_TSIConfig[uTsiIdx].PortCtrl;
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_sync_detect)) = TSPTA_TSIConfig[uTsiIdx].SyncDetect;
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_packet_format)) = TSPTA_TSIConfig[uTsiIdx].PacketFormat;
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_error_detect)) = TSPTA_TSIConfig[uTsiIdx].ErrorDetect;
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_pid_filter)) = TSPTA_TSIConfig[uTsiIdx].PidFilter;
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_fifo_select)) = TSPTA_TSIConfig[uTsiIdx].FifoSelect;
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_stc_clock_div)) = TSPTA_TSIConfig[uTsiIdx].StcClkDiv;

        //Restore the reset resigter before suspend
        *(volatile uint32_t *)(&(pReg->u32TspTsiReg_global)) = TSPTA_TSIConfig[uTsiIdx].ResetCtrl;
    }

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_RestoreHWContext(void* sessionContext,
                                                 uint32_t  paramTypes,
                                                 TEE_Param params[4])
{
    uint32_t i, j;
    uint32_t arr_size;
    uint32_t figo_id;
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    figo_id = pTspCtx->FigoId;
    if(figo_id >= TSP_MAX_FIGO_NUM)
        return TEE_ERROR_BAD_PARAMETERS;

    arr_size = (uint32_t)sizeof(TSPTA_Reg_Info) / (uint32_t)sizeof(TSPTA_RegInfo);
    for(i = 0; i < arr_size; i++) {
        if(TSPTA_Reg_Info[i].figo_id == figo_id) {
            for(j = 0; j < (TSPTA_Reg_Info[i].size / 4); j++) {
                *(volatile uint32_t *)((uint32_t *)TSPTA_SavRes_va[figo_id] + (TSPTA_Reg_Info[i].base / 4) + j) =
                    *(TSPTA_SavRes_buf[figo_id] + (TSPTA_Reg_Info[i].base / 4) + j);
            }
        }
    }

    TSPTA_RestoreCfgDS(figo_id);
    TSPTA_RestoreCfgCryptoEngine(figo_id);
    TSPTA_RestoreCfgSF(figo_id);
    TSPTA_RestoreCfgSWD(figo_id);
    TSPTA_RestoreCfgTSI(figo_id);
    if(figo_id == 0){
        TSPTA_RestoreCfgCryptoEngineReg(figo_id);
        TSPTA_RestoreCfgSFReg(figo_id);
        TSPTA_RestoreCfgSWDReg(figo_id);
        TSPTA_RestoreCfgTSIReg(figo_id);
    }

    if(TSPTA_SavRes_buf[figo_id] != NULL) {
        TEE_Free(TSPTA_SavRes_buf[figo_id]);
        TSPTA_SavRes_buf[figo_id] = NULL;
    }

    return TEE_SUCCESS;
}

static TEE_Result TSPTA_GetSecureChkResult(void *sessionContext,
                                                      uint32_t  paramTypes,
                                                      TEE_Param params[4])
{
    TEE_Result ret = TEE_SUCCESS;
    TEE_Identity identity;
    uint32_t SCID;
    uint32_t PidFilterIdx;
    uint32_t BaseAddr;
    uint32_t OutBufAddr;
    volatile SIE_PidFilterCtx *pPidFilter;
    volatile SIE_PesFilterCtx *pPesFilter;
    volatile SIE_DSStreamOutEx *pDSSOut;
    TEE_UUID uuidZero = {0};
    TEE_TspContext* pTspCtx = (TEE_TspContext*)sessionContext;

    if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_INPUT ||
        TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_OUTPUT) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    ret = TEE_GetPropertyAsIdentity(TEE_PROPSET_CURRENT_CLIENT,
                                    "gpd.client.identity", &identity);
    if (ret != TEE_SUCCESS) {
       TA_LOGE( "TEE_GetPropertyAsIdentity fail with 0x%x\n", ret);
       return ret;
    }
    // only TA can call this command
    if (!memcmp(&identity.uuid, &uuidZero, sizeof(TEE_UUID))) {
        TA_LOGE("This command can only be called by TA\n");
        return TEE_ERROR_ACCESS_DENIED;
    }

    SCID = params[0].value.a;
    PidFilterIdx = params[0].value.b;
    OutBufAddr = params[1].value.a;
    TA_LOGM("PidFilter to be checked. Idx: 0x%x SCID: 0x%x BufPhyAddr: 0x%x\n",
           PidFilterIdx, SCID, OutBufAddr);
    if (PidFilterIdx >= arr_TspDtcmGlobal_PidFilterCtxTable) {
        TA_LOGE("PidFilterIdx is not valid\n");
        return TEE_ERROR_BAD_PARAMETERS;
    }

    BaseAddr = TSP_FIGO_REG_BASE + TSP_FIGO_REG_SIZE * pTspCtx->FigoId;
    pPidFilter =  (SIE_PidFilterCtx*)tspmem_phy_to_virt(BaseAddr + RA_TspDtcmGlobal_PID +
                                    PidFilterIdx * sizeof(SIE_PidFilterCtx));
    TA_LOGM("PidFilter in FW. SCID: 0x%x\n", pPidFilter->u_Pid);
    if (pPidFilter->u_IsPes) {
        pPesFilter = (SIE_PesFilterCtx *)tspmem_phy_to_virt(BaseAddr + pPidFilter->u_pCtx);
        pDSSOut = (SIE_DSStreamOutEx *)tspmem_phy_to_virt(BaseAddr + pPesFilter->u_pEsBuf);
        TA_LOGM("BufPhyAddr of PidFilter in FW: 0x%x\n", pDSSOut->ie_dsFifo.u_pbBase);
        if ((pPidFilter->u_Pid == SCID) &&
            (pDSSOut->ie_dsFifo.u_pbBase == OutBufAddr)) {
            SIE_UINT32 DTCMValue = pTspCtx->pSecureCtx->ie_PidCtxSecureResult[PidFilterIdx >> 5];
            uint32_t value= (DTCMValue.u_uVal1 << 16) | DTCMValue.u_uVal0;
            params[2].value.a = value & (1 << (PidFilterIdx & 0x1f)) ? 1 : 0;
            TA_LOGM("Secure check result on PidFilter 0x%x: %u\n",
                    PidFilterIdx, params[2].value.a);
        } else {
            params[2].value.a = 0;
            TA_LOGM("Mismatched Pid/Buffer on PidFilter 0x%x\n", PidFilterIdx);
        }
    } else {
        TA_LOGE("Mismatched Type on PidFilter 0x%x\n", PidFilterIdx);
        params[2].value.a = 0;
    }

    return ret;
}

typedef struct __tsp_ta_cmd_handler__
{
    uint32_t commandID;
    uint32_t (*cmd_handler)(void* sessionContext, uint32_t  paramTypes, TEE_Param params[4]);
} TSP_TA_CMD_HANDLER;

static TSP_TA_CMD_HANDLER tsp_ta_cmd_handler[TSP_CMD_MAX] =
{
    {TSP_INVALID, NULL},
    {TSP_DEBUG, NULL},
    {TSP_HBO_QUEUE_ENABLE, TSPTA_EnHboQueue},
    {TSP_HBO_QUEUE_QUERY, TSPTA_QueryHboQueue},
    {TSP_HBO_QUEUE_CLEAR, TSPTA_ClrHboQueue},
    {TSP_HBO_QUEUE_CLEAR_DONE, TSPTA_ClrDoneHboQueue},
    {TSP_HBO_QUEUE_CFG, TSPTA_ConfigHboQueue},
    {TSP_DTCM_MEM_ALLOC,TSPTA_AllocDtcmMem},
    {TSP_DTCM_MEM_FREE,TSPTA_FreeDtcmMem},
    {TSP_FW_LOAD, TSPTA_FWLoad},
    {TSP_FIGO_SCHED, TSPTA_FigoSched},
    {TSP_FIGO_UNSCHED, TSPTA_FigoUnSched},
    {TSP_ALLOC_TASK, TSPTA_AllocTask},
    {TSP_FREE_TASK, TSPTA_FreeTask},
    {TSP_UPDATE_KEY_PROTECT_FLAG, TSPTA_UpdateKeyProtectFlag},
    {TSP_REMOVE_KEY_PROTECT_FLAG, TSPTA_RemoveKeyProtectFlag},
    {TSP_UPDATE_COPY_PROTECTION_FLAG, TSPTA_UpdateCopyProtectionFlag},
    {TSP_GET_PRIVATE_AREAS, TSPTA_GetPrivateAreas},
    {TSP_MEM_WRAP_MOVE, TSPTA_MemWrapMove},
    {TSP_GET_OUTBUF_SECURE_PROP, TSPTA_GetOutBufPropFromSecureRule},
    {TSP_SECTION_GET_SYNTAX_INDICATOR, TSPTA_Section_Get_Syntax_Indicator},
    {TSP_SECTION_GET_DATA_INVALID_MASK, TSPTA_Section_Get_Data_Invalid_Mask},
    {TSP_CHECK_GOP_HDR, TSPTA_Check_Gop_Hdr},
    {TSP_NSK_CMCHANNEL_ADD, TSPTA_AddNSKCmChannel},
    {TSP_NSK_CMCHANNEL_REMOVE, TSPTA_RmNSKCmChannel},
    {TSP_NSK_CMCHANNEL_UPDATE, TSPTA_UpdateNSKCmChannel},
    {TSP_NSK_CMCHANNEL_CONNECT, TSPTA_ConnectNSKCmChannel},
    {TSP_NSK_CMCHANNEL_DISCONNECT, TSPTA_DisconnectNSKCmChannel},
    {TSP_NSK_CMCHANNEL_GETPROP, TSPTA_GetNSKCmChannelProp},
    {TSP_GET_SECURE_CHK_RESULT, TSPTA_GetSecureChkResult},
    {TSP_KEYTOKEN_GETINFO, TSPTA_GetKeyTokenInfo},
    {TSP_UPDATE_HBOIN_FILTERS, TSPTA_UpdateHboinFilters}
};

static TSP_TA_CMD_HANDLER tsp_ta_kernel_cmd_handler[TSP_KERNEL_CMD_MAX - TSP_KERNEL_CMD_MIN] =
{
    {CMD_TO_INDEX(TSP_SAVE_HW_CONTEXT), TSPTA_SaveHWContext},
    {CMD_TO_INDEX(TSP_RESTORE_HW_CONTEXT), TSPTA_RestoreHWContext},
    {CMD_TO_INDEX(TSP_SET_FIGO_STATE), TSPTA_SetFigoState},
    {CMD_TO_INDEX(TSP_GET_FIGO_STATE), TSPTA_GetFigoState},
    {CMD_TO_INDEX(TSP_SET_CLK), TSPTA_SetFigoClk}
};

#define VALID_KERNEL_CMD_HANDLER(cmd) (tsp_ta_kernel_cmd_handler[CMD_TO_INDEX(cmd)].cmd_handler)

TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(
    void*            sessionContext,
    uint32_t        commandID,
    uint32_t        paramTypes,
    TEE_Param        params[4])
{
    TEE_Result res = TEE_ERROR_NOT_SUPPORTED;
    //TA_LOGH( "TA_InvokeCommandEntryPoint, ID:0x%x\n", commandID);
    if( (commandID > TSP_INVALID && commandID < TSP_CMD_MAX) &&
        tsp_ta_cmd_handler[commandID].cmd_handler) {
        res = tsp_ta_cmd_handler[commandID].cmd_handler(sessionContext, paramTypes, params);
    } else if (IS_TSP_KERNEL_CMD(commandID) && VALID_KERNEL_CMD_HANDLER(commandID)) {
        uint32_t index = CMD_TO_INDEX(commandID);
        res = tsp_ta_kernel_cmd_handler[index].cmd_handler(sessionContext, paramTypes, params);
    }

    return res;
}

#ifndef CONFIG_OPTEE
TA_DEFINE_BEGIN
    .uuid = TA_TSP_UUID,
    .name = "Tsp",
    .singleInstance = true,
    .multiSession = true,
    .instanceKeepAlive = true,
TA_DEFINE_END
#endif
