/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2017 Synaptics Incorporated. All rights reserved.
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
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
 ////////////////////////////////////////////////////////////////////////////////
 //! \file test_m2m.cpp
 //!
 //! \brief This file tests basic functionalities of dmx m2m
 //!
 //!              Version   Date                                 Author
 //!              V 1.00,   Aug 8 2016                        wangzl
 //!
 //! Note:
 ////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <linux/types.h>

#include "gtest/gtest.h"
#include <time.h>

#include <tee_client_api.h>
#include <tsp_cmd.h>
#include "m2m_crypto_types.h"
#include "m2m_crypto_api.h"

#define BASIC_LOOP 1

struct dma_heap_allocation_data {
        __u64 len;
        __u32 fd;
        __u32 fd_flags;
        __u64 heap_flags;
};

#define DMA_HEAP_IOCTL_ALLOC	_IOWR('H', 0x0, struct dma_heap_allocation_data)

struct bm_pt_param {
    __u64 phy_addr;
    __u64 len;
    __u64 mem_id;
};

struct bm_uv_align_param {
    __u32 y_size;
};

/* extra parameter for frame buffer */
struct bm_fb_param {
    __u32 fb_type;
    struct bm_uv_align_param uva_param;
};

struct bm_fb_data {
    __u32 fd;
    __u32 flags;
    struct bm_fb_param fb_param;
    const struct bm_pt_param pt_param;
};

#define BM_IOC_ALLOC_PT		_IOWR('B', 1, struct bm_fb_data)

static const TEEC_UUID M2M_TA_UUID = TA_M2M_UUID;
static const TEEC_UUID TSP_TA_UUID = TA_TSP_UUID;
typedef struct M2M{
    unsigned char  Keylength;
    const char  *Name;
} M2M_Struct;

static M2M_Struct M2M_File[] =
{
   {16,"aes128ecb"},
   {16,"aes128cbc"},
   {16,"aes128ctr"},
   {24,"tdes128ecb"},
   {24,"tdes128cbc"},
   {24,"tdes128ctr"},
   {16,"dvbcsa30"},
   {16,"asa"},
};

// Path of test resources
extern const char *gPath;
// Path of output result and log
extern const char *gOutPath;

static const char *g_dmaheap_path = "/dev/dma_heap";
static int g_bm_dev_fd = -1;
static TEEC_Context g_m2m_ctx = {0};
static TEEC_Session g_m2m_session_default = {0};

#define TSP_FIGO_NUM 2

static int dmaheap_open(const char *heap_name)
{
    char path[128] = {0};
    int dev_fd = -1;
    snprintf(path, sizeof(path), "%s/%s", g_dmaheap_path, heap_name);
    printf("Opening %s\n", path);

    if (heap_name == NULL) {
        printf("Invalid heap name\n");
        return -1;
    }
    if (dev_fd < 0) {
        dev_fd = open(path, O_RDWR, 0);
        if (dev_fd < 0) {
            printf("Failed to open %s\n", path);
            return -1;
        }
    }
    return dev_fd;
}

static void dmaheap_close(int dev_fd)
{
    if (dev_fd >= 0) {
        close(dev_fd);
    }
    return;
}

static int allocSHMBuffer(int dev_fd, uint32_t size, uint8_t **buffer, int *shm_fd)
{
    struct dma_heap_allocation_data alloc_data;
    int ret = 0;

    if (dev_fd < 0) {
        printf("Invalid device fd\n");
        return -1;
    }

    alloc_data.fd = 0;
    alloc_data.len = size;
    alloc_data.fd_flags = O_RDWR | O_CLOEXEC;
    alloc_data.heap_flags = 0;

    ret = ioctl(dev_fd, DMA_HEAP_IOCTL_ALLOC, &alloc_data);
    if (ret < 0) {
        printf("Failed to allocate buffer errno=%d(%s)\n", errno, strerror(errno));
        return -1;
    }

    if (buffer) {
        *buffer = (uint8_t*)mmap(NULL, alloc_data.len, PROT_READ | PROT_WRITE, MAP_SHARED, alloc_data.fd, 0);
        if (*buffer == MAP_FAILED) {
            printf("Failed to mmap buffer\n");
            close(alloc_data.fd);
            return -1;
        }
    }

    *shm_fd = alloc_data.fd;
    return 0;
}

static void freeSHMBuffer(uint8_t *buffer, int shm_fd)
{
    if (buffer) {
        munmap(buffer, sizeof(buffer));
    }
    if (shm_fd >= 0) {
        close(shm_fd);
    }
}

static int getShmMemid(int shm_fd)
{
    struct bm_fb_data fd_data  = {0};
    int ret = 0;

    if (g_bm_dev_fd < 0) {
        g_bm_dev_fd = open("/dev/berlin-bm", O_RDWR, 0);
        if (g_bm_dev_fd < 0) {
            printf("Failed to open bm device\n");
            return -1;
        }
    }

    fd_data.fd = shm_fd;
    fd_data.flags = 0; // for now it's non-secure
    fd_data.fb_param.fb_type = 1;
    fd_data.fb_param.uva_param.y_size = 0;

    ret = ioctl(g_bm_dev_fd, BM_IOC_ALLOC_PT, &fd_data);
    if (ret < 0) {
        printf("Failed to get shm mem id\n");
        return -1;
    }

    return fd_data.pt_param.mem_id;
}

TEEC_Result m2m_session_open_with_ops(TEEC_Context *ctx, M2MCRYPTO_SESSION_TYPE eType, uint32_t *uFigoSysId,
    void **phSession, uint32_t *uSessionId)
{
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;

    operation.paramTypes = TEEC_PARAM_TYPES(
                TEEC_VALUE_INPUT,
                TEEC_VALUE_OUTPUT,
                TEEC_NONE,
                TEEC_NONE);
    operation.params[0].value.a = eType;
    operation.params[0].value.b = 0;

    *phSession = (TEEC_Session*)malloc(sizeof(TEEC_Session));
    /* Open a Session with the TEE application. */
    result = TEEC_OpenSession(ctx, (TEEC_Session*)*phSession, &M2M_TA_UUID,
                                TEEC_LOGIN_USER,
                                NULL,
                                &operation,
                                NULL);

    *uSessionId = operation.params[1].value.a;
    *uFigoSysId = operation.params[1].value.b;

    if(result != TEEC_SUCCESS) {
        printf("tz_session_open fail with 0x%x\n",result);
        free(*phSession);
        *phSession = NULL;
    }
    return result;
}

TEEC_Result m2m_session_open_default()
{
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;
    int bDummySess = 1;

    operation.paramTypes = TEEC_PARAM_TYPES(
                TEEC_VALUE_INPUT,
                TEEC_VALUE_OUTPUT,
                TEEC_NONE,
                TEEC_NONE);
    operation.params[0].value.a = 0;
    operation.params[0].value.b = bDummySess;

    /* Open a Session with the TEE application. */
    result = TEEC_OpenSession(&g_m2m_ctx, &g_m2m_session_default, &M2M_TA_UUID,
                                TEEC_LOGIN_USER,
                                NULL,
                                &operation,
                                NULL);
    if(result != TEEC_SUCCESS) {
        printf("m2m_session_open_default fail with 0x%x\n",result);
    }
    return result;
}

TEEC_Result m2m_hw_crypto_init(uint32_t shm_memid, uint32_t uFigoSysId)
{
      TEEC_Result result = TEEC_SUCCESS;
      TEEC_Operation operation;

      operation.paramTypes = TEEC_PARAM_TYPES(
              TEEC_VALUE_INPUT,
              TEEC_NONE,
              TEEC_NONE,
              TEEC_NONE);

      operation.params[0].value.a = uFigoSysId;
      operation.params[0].value.b = shm_memid;
      operation.started = 1;

      result = TEEC_InvokeCommand(
              &g_m2m_session_default,
              M2M_CryptoInit,
              &operation,
              NULL);

      if(TEEC_SUCCESS != result){
         printf("ret=0x%08x\n", result);
      }
      return result;
}

TEEC_Result m2m_hw_crypto_deinit(uint32_t uFigoSysId)
{
      TEEC_Result result = TEEC_SUCCESS;
      TEEC_Operation operation;

      operation.paramTypes = TEEC_PARAM_TYPES(
        TEEC_VALUE_INPUT,
        TEEC_NONE,
        TEEC_NONE,
        TEEC_NONE);
      //use the value to represent the shm
      //In ta env, will use params[0].memref.buffer to access
      operation.params[0].value.a = uFigoSysId;
      operation.started = 1;
      result = TEEC_InvokeCommand(
                &g_m2m_session_default,
                M2M_CryptoDeinit,
                &operation,
                NULL);
      if(result != TEEC_SUCCESS) {
          printf("tz_session_crypto_deinit fail with 0x%x\n",result);
          goto fun_ret;
      }
  fun_ret:
      return result;
}

TEEC_Result m2m_session_setscheme(void *phSession, M2MCRYPTO_TYPE eScheme)
{
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_NONE,
            TEEC_NONE,
            TEEC_NONE);
    operation.params[0].value.a = eScheme;
    operation.params[0].value.b = 0;

    result = TEEC_InvokeCommand(
          (TEEC_Session*)phSession,
          M2M_Setscheme,
          &operation,
          NULL);

    if(result != TEEC_SUCCESS) {
        printf("tz_session_setscheme fail with 0x%x\n",result);
    }
    return result;
}

TEEC_Result m2m_session_routine(void *hSess, uint32_t *uCmdFinishNum)
{
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;
    operation.paramTypes = TEEC_PARAM_TYPES(
           TEEC_NONE,
           TEEC_NONE,
           TEEC_NONE,
           TEEC_NONE);

    result = TEEC_InvokeCommand(
          (TEEC_Session*)hSess,
          M2M_FeedCmd,
          &operation,
          NULL);
    if (result != TEEC_SUCCESS) {
        printf("M2M_FeedCmd fail with 0x%x\n",result);
        return result;
    }

    operation.paramTypes = TEEC_PARAM_TYPES(
           TEEC_VALUE_OUTPUT,
           TEEC_NONE,
           TEEC_NONE,
           TEEC_NONE);
    result = TEEC_InvokeCommand(
          (TEEC_Session*)hSess,
          M2M_Postprocess,
          &operation,
          NULL);
    *uCmdFinishNum = operation.params[0].value.a;

    if(result != TEEC_SUCCESS) {
        printf("M2M_Postprocess fail with 0x%x\n",result);
    }
    return result;
}

TEEC_Result m2m_session_patternmode(void *hSess,
                            M2MCRYPTO_PATTERN_MODE_CFG *pCfg)
{
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_NONE,
            TEEC_NONE,
            TEEC_NONE);
    operation.params[0].value.a = pCfg->pattern_enc;
    operation.params[0].value.b = pCfg->pattern_clr;;

    result = TEEC_InvokeCommand(
          (TEEC_Session*)hSess,
          M2M_SetPatternMode,
          &operation,
          NULL);

    if(result != TEEC_SUCCESS) {
        printf("tz_session_setpatternmode fail with 0x%x\n",result);
    }
    return result;
}

TEEC_Result m2m_session_set_residuemode(void *hSess,
                            M2MCRYPTO_RESIDUE_TYPE eType)
{
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT,
            TEEC_NONE,
            TEEC_NONE,
            TEEC_NONE);
    operation.params[0].value.a = eType;

    result = TEEC_InvokeCommand(
          (TEEC_Session*)hSess,
          M2M_SetResidueMode,
          &operation,
          NULL);

    if(result != TEEC_SUCCESS) {
        printf("m2m_session_set_residuemode fail with 0x%x\n",result);
    }
    return result;
}

TEEC_Result m2m_session_config(void   *hSess,
                            M2MCRYPTO_KEY_OPTION  KeyOption,
                            uint32_t          KeyTblIndex,
                            uint8_t           *pKeyData,
                            uint32_t          KeyLength,
                            M2MCRYPTO_KEY_OPTION  IVOption,
                            uint32_t          IVTblIndex,
                            uint8_t           *pIVData,
                            uint32_t          IVLength,
                            M2M_DRM_CRYPTO_ST   *pCryptInfo)
{
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;

    TEEC_SharedMemory KeyHdlSM;
    TEEC_SharedMemory IVHdlSM;
    TEEC_SharedMemory CryptoHdlSM;
    TEEC_SharedMemory KeyInfoHdlSM;

    M2M_KEY_INFO_ST KeyInfo;
    uint32_t KeyDataParam = TEEC_NONE, IVDataParam = TEEC_NONE;


    KeyHdlSM.size  = 0;//KeyLength;
    KeyHdlSM.buffer = NULL;
    KeyHdlSM.flags = TEEC_MEM_INPUT;

    IVHdlSM.size  = 0;//IVLength;
    IVHdlSM.buffer = NULL;
    IVHdlSM.flags = TEEC_MEM_INPUT;

    CryptoHdlSM.size  = sizeof(M2M_DRM_CRYPTO_ST);
    CryptoHdlSM.flags = TEEC_MEM_INPUT;

    KeyInfoHdlSM.size  = sizeof(M2M_KEY_INFO_ST);
    KeyInfoHdlSM.flags = TEEC_MEM_INPUT;


    /* Use TEE Client API to register the underlying memory buffer. */
    if(KeyOption == M2MCRYPTO_KEY_CONTENT) {
       KeyHdlSM.size = KeyLength;
       KeyDataParam = TEEC_MEMREF_PARTIAL_INPUT;
       result = TEEC_AllocateSharedMemory(
                     &g_m2m_ctx,
                     &KeyHdlSM);
       if (result != TEEC_SUCCESS)
       {
           fprintf(stderr, "ret=0x%08x\n", result);
           goto fun_ret;
       }
       memcpy(KeyHdlSM.buffer, pKeyData, KeyLength);
    }

    if(IVOption == M2MCRYPTO_KEY_CONTENT) {
       IVHdlSM.size = IVLength;
       IVDataParam = TEEC_MEMREF_PARTIAL_INPUT;
       result = TEEC_AllocateSharedMemory(
                              &g_m2m_ctx,
                              &IVHdlSM);
       if (result != TEEC_SUCCESS)
       {
           fprintf(stderr, "ret=0x%08x\n", result);
           goto fun_ret;
       }
       memcpy(IVHdlSM.buffer, pIVData, IVLength);
    }
    result = TEEC_AllocateSharedMemory(
                        &g_m2m_ctx,
                        &CryptoHdlSM);
    if (result != TEEC_SUCCESS)
    {
      fprintf(stderr, "ret=0x%08x\n", result);
      goto fun_ret;
    }

    if(pCryptInfo) {
       memcpy(CryptoHdlSM.buffer, pCryptInfo, sizeof(M2M_DRM_CRYPTO_ST));
    }else {
       memset(CryptoHdlSM.buffer, 0, sizeof(M2M_DRM_CRYPTO_ST));
    }

    result = TEEC_AllocateSharedMemory(
                      &g_m2m_ctx,
                      &KeyInfoHdlSM);
    if (result != TEEC_SUCCESS)
    {
      fprintf(stderr, "ret=0x%08x\n", result);
      goto fun_ret;
    }

    KeyInfo.uiKeyOption = KeyOption;
    KeyInfo.uiKeyLength = KeyLength;
    KeyInfo.uiKeyTblIndex = KeyTblIndex;
    KeyInfo.uiIVOption = IVOption;
    KeyInfo.uiIVLength = IVLength;
    KeyInfo.uiIVTblIndex = IVTblIndex;

    memcpy(KeyInfoHdlSM.buffer, &KeyInfo, sizeof(M2M_KEY_INFO_ST));

    operation.paramTypes = TEEC_PARAM_TYPES(
          KeyDataParam,
          IVDataParam,
          TEEC_MEMREF_PARTIAL_INPUT,
          TEEC_MEMREF_PARTIAL_INPUT);
    //use the value to represent the shm
    //In ta env, will use params[0].memref.buffer to access
    operation.params[0].memref.parent = &KeyHdlSM;
    operation.params[0].memref.size = KeyHdlSM.size;
    operation.params[0].memref.offset = 0;

    operation.params[1].memref.parent = &IVHdlSM;
    operation.params[1].memref.size = IVHdlSM.size;
    operation.params[1].memref.offset = 0;

    operation.params[2].memref.parent = &CryptoHdlSM;
    operation.params[2].memref.size = CryptoHdlSM.size;
    operation.params[2].memref.offset = 0;

    operation.params[3].memref.parent = &KeyInfoHdlSM;
    operation.params[3].memref.size = KeyInfoHdlSM.size;
    operation.params[3].memref.offset = 0;

    result = TEEC_InvokeCommand(
          (TEEC_Session*)hSess,
          M2M_Setkey,
          &operation,
          NULL);

    if(KeyOption == M2MCRYPTO_KEY_CONTENT) {
       TEEC_ReleaseSharedMemory(&KeyHdlSM);
    }
    if(IVOption == M2MCRYPTO_KEY_CONTENT) {
       TEEC_ReleaseSharedMemory(&IVHdlSM);
    }
    TEEC_ReleaseSharedMemory(&CryptoHdlSM);
    TEEC_ReleaseSharedMemory(&KeyInfoHdlSM);

    if(result != TEEC_SUCCESS) {
       fprintf(stderr, "tz_session_config fail with 0x%x\n",result);
    }
fun_ret:
    return result;
}

TEEC_Result m2m_session_update(void *hSess,  M2MCRYPTO_MEM_H hInput,
                              M2MCRYPTO_MEM_H hOutput)
{
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_Operation operation;
    //params[0] for input shm memid and size
    //params[1] for output shm memid and size
    //params[2] for inout offset
    //params[3] for memory type
    operation.paramTypes = TEEC_PARAM_TYPES(
           TEEC_VALUE_INPUT,
           TEEC_VALUE_INPUT,
           TEEC_VALUE_INPUT,
           TEEC_VALUE_INPUT);

    operation.params[0].value.a = (uint32_t)hInput->hShm_memid;
    operation.params[0].value.b = hInput->uSize;

    operation.params[1].value.a = (uint32_t)hOutput->hShm_memid;
    operation.params[1].value.b = hOutput->uSize;

    operation.params[2].value.a = hInput->uOffset;
    operation.params[2].value.b = hOutput->uOffset;

    operation.params[3].value.a = TSP_MEM_PARAM_SHM_ID;

    result = TEEC_InvokeCommand(
        (TEEC_Session*)hSess,
        M2M_Update,
        &operation,
        NULL);
    if(result != TEEC_SUCCESS) {
        fprintf(stderr, "tz_session_update fail with 0x%x\n",result);
    }
    return result;
}

static uint8_t get_m2m_file_item(uint32_t uScheme)
{
   if(uScheme < M2MCRYPTO_TYPE_MAX) {
       return (uScheme-1);
   }
   else
       return 0;
}

static int test_m2m_basic_encdec(uint32_t uInputScheme, char *strmPath)
{
    uint32_t i=0, j=0, k=0;
    int ret = 0;
    uint8_t *RefData = NULL;
    uint8_t *SrcData = NULL;
    FILE    *Datafp = NULL;
    FILE    *Keyfp = NULL;
    FILE    *IVfp = NULL;
    FILE    *Reffp = NULL;
    char Comm_name[128];
    char Data_name[128];
    char Key_name[128];
    char IV_name[128];
    char Ref_name[128];
    uint32_t uiBuffSize = 0, uDataSize = 0, uReadSize = 0, File_size = 0, uSteps = 0;
    uint8_t uItem = 0;

    int cma_cust_dev_fd = -1;
    int hDumpShm_fd = -1;
    uint8_t *pDumpShmVirtAddr = NULL;

    int hEncInBuff_fd = -1, hEncOutBuff_fd = -1;
    uint8_t *pEncInVirtAddr = NULL, *pEncOutVirtAddr = NULL;

    int hDecInBuff_fd = -1, hDecOutBuff_fd = -1;
    uint8_t *pDecInVirtAddr = NULL, *pDecOutVirtAddr = NULL;


    uint8_t EncKeyData[24] = {0};
    uint8_t EncIVData[16] = {0};

    uint32_t uSessionID_1 = 0, uSessionID_2 = 0;
    uint32_t uFigoSysId_1 = 0, uFigoSysId_2 = 0;
    int      iFigoShmFd_1 = 0, iFigoShmFd_2 = 0;
    uint8_t *pFigoShm1InVirtAddr = NULL, *pFigoShm2VirtAddr = NULL;

    TEEC_Session * pM2MSession1 = NULL;
    TEEC_Session * pM2MSession2 = NULL;
    M2M_DRM_CRYPTO_ST CryptInfo;
    M2MCRYPTO_MEMOBJ InputMem, OutputMem;
    TEEC_Result res = TEEC_SUCCESS;


    if(uInputScheme>M2MCRYPTO_TYPE_ASA || uInputScheme<M2MCRYPTO_TYPE_AES_128_ECB) {
        printf("Input scheme %d not supported\n",uInputScheme);
        return -1;
    }

    res = TEEC_InitializeContext(NULL, &g_m2m_ctx);
    if (res != TEEC_SUCCESS) {
        fprintf(stderr, "TEEC_InitializeContext failed with code 0x%x\n", res);
        return res;
    }

    uItem = get_m2m_file_item(uInputScheme);
    printf("\n<%s> test case begin:\n",M2M_File[uItem].Name);

    memset(Comm_name,0,sizeof(Comm_name));

    memset(Data_name,0,sizeof(Data_name));
    memset(Key_name,0,sizeof(Key_name));
    memset(IV_name,0,sizeof(IV_name));
    memset(Ref_name,0,sizeof(Ref_name));

    assert(gPath);
    strcat(Comm_name,gPath);
    strcat(Comm_name,strmPath);
    strcat(Comm_name,M2M_File[uItem].Name);
    memcpy(Data_name,Comm_name, sizeof(Comm_name));
    memcpy(Key_name,Comm_name, sizeof(Comm_name));
    memcpy(IV_name,Comm_name, sizeof(Comm_name));
    memcpy(Ref_name,Comm_name, sizeof(Comm_name));

    strcat(Data_name,".dat");
    strcat(Key_name,".key");
    strcat(IV_name,".iv");
    strcat(Ref_name,".ref");

    printf("Data_name = %s\n",Data_name);
    printf("Key_name = %s\n",Key_name);
    printf("IV_name = %s\n",IV_name);
    printf("Ref_name = %s\n",Ref_name);

    Datafp = fopen(Data_name, "rb");
    Keyfp  = fopen(Key_name, "rb");
    IVfp   = fopen(IV_name, "rb");
    Reffp  = fopen(Ref_name, "rb");

    if(Datafp == NULL || Keyfp == NULL || IVfp == NULL || Reffp == NULL) {
       printf("Open file Data:%p, Key:%p, IV:%p, Ref:%p fail\n", Datafp, Keyfp, IVfp, Reffp);
       ret = TEEC_ERROR_CANCEL;
       goto error;
    }

    fseek(Datafp , 0 , SEEK_END);
    File_size = ftell(Datafp);
    rewind(Datafp);

    uDataSize = File_size-File_size%16;
    if(0 == uDataSize){
        printf("uDataSize = 0x%x\n",uDataSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    uiBuffSize = BASIC_LOOP*uDataSize;

    cma_cust_dev_fd = dmaheap_open("CMA-CUST-reserved");
    if (cma_cust_dev_fd < 0) {
        printf("Failed to open dmaheap\n");
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uDataSize, &pDumpShmVirtAddr, &hDumpShm_fd);
    if (ret < 0 || pDumpShmVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pDumpShmVirtAddr=%p\n", __LINE__, ret, pDumpShmVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    uSteps = uiBuffSize/uDataSize;

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncInVirtAddr, &hEncInBuff_fd);
    if (ret < 0 || pEncInVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncInVirtAddr=%p\n", __LINE__, ret, pEncInVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncOutVirtAddr, &hEncOutBuff_fd);
    if (ret < 0 || pEncOutVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncOutVirtAddr=%p\n", __LINE__, ret, pEncOutVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pDecInVirtAddr, &hDecInBuff_fd);
    if (ret < 0 || pDecInVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pDecInVirtAddr=%p\n", __LINE__, ret, pDecInVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pDecOutVirtAddr, &hDecOutBuff_fd);
    if (ret < 0 || pDecOutVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pDecOutVirtAddr=%p\n", __LINE__, ret, pDecOutVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }


    uReadSize = fread(EncKeyData, 1, uDataSize, Keyfp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x\n",uReadSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }
    uReadSize = fread(EncIVData, 1, uDataSize, IVfp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x\n",uReadSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }
    uReadSize = fread(pDumpShmVirtAddr, 1, uDataSize, Datafp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x\n",uReadSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

#if 1
    i = 0;
    printf("Key: ");
    while(i<16) {
         printf("0x%02x, ",EncKeyData[i]);
         i++;
    }
    printf("\n");

    i = 0;
    printf("IV: ");
    while(i<16) {
        printf("0x%02x, ",EncIVData[i]);
        i++;
    }
    printf("\n");

    i = 0;
    printf("Src data: ");
    while(i<uDataSize) {
        printf("0x%02x, ",pDumpShmVirtAddr[i]);
        i++;
    }
    printf("\n");
    #endif

    /*Prepare input SHM*/
    for(j=0; j<uSteps; j++) {
        memcpy(pEncInVirtAddr+j*uDataSize, pDumpShmVirtAddr, uDataSize);
    }

    res = m2m_session_open_default();
    if (res != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_open_default failed with code 0x%x\n", res);
        goto error;
    }

    /*Open session#1*/
    res =  m2m_session_open_with_ops(&g_m2m_ctx, M2MCRYPTO_SESSION_TYPE_ENC, &uFigoSysId_1,
                                            (void**)&pM2MSession1, &uSessionID_1);
    if (res != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_open_with_ops failed with code 0x%x\n", res);
        goto error;
    }

    res = m2m_session_setscheme(pM2MSession1, (M2MCRYPTO_TYPE)uInputScheme);
    if (res != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_setscheme failed with code 0x%x\n", res);
        goto error;
    }

    res = m2m_session_config(pM2MSession1,
                            M2MCRYPTO_KEY_CONTENT,
                            0,
                            &EncKeyData[0],
                            M2M_File[uItem].Keylength,
                            M2MCRYPTO_KEY_CONTENT,
                            0,
                            &EncIVData[0],
                            16,
                            &CryptInfo);
    if (res != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_config failed with code 0x%x\n", res);
        goto error;
    }

    /*Open session#2*/
    res =  m2m_session_open_with_ops(&g_m2m_ctx, M2MCRYPTO_SESSION_TYPE_DEC, &uFigoSysId_2,
                                            (void**)&pM2MSession2, &uSessionID_2);
    if (res != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_open_with_ops failed with code 0x%x\n", res);
        goto error;
    }


    res = m2m_session_setscheme(pM2MSession2, (M2MCRYPTO_TYPE)uInputScheme);
    if (res != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_setscheme failed with code 0x%x\n", res);
        goto error;
    }

    res = m2m_session_config(pM2MSession2,
                            M2MCRYPTO_KEY_CONTENT,
                            0,
                            &EncKeyData[0],
                            M2M_File[uItem].Keylength,
                            M2MCRYPTO_KEY_CONTENT,
                            0,
                            &EncIVData[0],
                            16,
                            &CryptInfo);
    if (res != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_config failed with code 0x%x\n", res);
        goto error;
    }

    for(j=0; j<uSteps; j++) {
        /*Clear secure shm as 0*/
        /*Enc uReadSize block data*/
        InputMem.hShm_memid = getShmMemid(hEncInBuff_fd);
        InputMem.uOffset = j*uDataSize;
        InputMem.uSize = uDataSize;
        OutputMem.hShm_memid = getShmMemid(hEncOutBuff_fd);
        OutputMem.uOffset = j*uDataSize;
        OutputMem.uSize = uDataSize;
        res = m2m_session_update(pM2MSession1, &InputMem, &OutputMem);
        if (res != TEEC_SUCCESS) {
            fprintf(stderr, "m2m_session_update failed with code 0x%x\n", res);
            goto error;
        }

        while(1) {
            uint32_t isDone = 0;
            res = m2m_session_routine(pM2MSession1, &isDone);
            if (res != TEEC_SUCCESS) {
                fprintf(stderr, "m2m_session_routine failed with code 0x%x\n", res);
                goto error;
            }
            if (isDone != 0) {
                break;
            }
        }

        /*Dec uReadSize block data*/
        memcpy(pDecInVirtAddr + j*uDataSize, pEncOutVirtAddr + j*uDataSize, uDataSize);

        InputMem.hShm_memid = getShmMemid(hDecInBuff_fd);
        InputMem.uOffset = j*uDataSize;
        InputMem.uSize = uDataSize;
        OutputMem.hShm_memid = getShmMemid(hDecOutBuff_fd);
        OutputMem.uOffset = j*uDataSize;
        OutputMem.uSize = uDataSize;
        res = m2m_session_update(pM2MSession2, &InputMem, &OutputMem);
        if (res != TEEC_SUCCESS) {
            fprintf(stderr, "m2m_session_update failed with code 0x%x\n", res);
            goto error;
        }

        while(1) {
            uint32_t isDone = 0;
            res = m2m_session_routine(pM2MSession2, &isDone);
            if (res != TEEC_SUCCESS) {
                fprintf(stderr, "m2m_session_routine failed with code 0x%x\n", res);
                goto error;
            }
            if (isDone != 0) {
                break;
            }
        }
    }

    /*Check result*/
    /*Prepare input SHM*/
    RefData = (uint8_t *)malloc(uDataSize);
    assert(RefData != NULL);
    uReadSize = fread(RefData, 1, uDataSize, Reffp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x\n",uReadSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    for(j=0; j<uSteps; j++) {
        memcpy(pDumpShmVirtAddr, pEncOutVirtAddr + j*uDataSize, uDataSize);
        for(k=0; k<uDataSize; k++) {
            if(pDumpShmVirtAddr[k] != RefData[k]) {
                printf("ENC fail:Out[%d][%d]=0x%02x,0x%02x\n",
                    j, k, pDumpShmVirtAddr[k],RefData[k]);
                ret = TEEC_ERROR_CANCEL;
                goto error;
            }
        }
    }
    if((j==uSteps) && (k==uDataSize)) {
        printf("/**************************************/\n");
        printf("/*            ENC Success!                */\n");
        printf("/**************************************/\n");
    }

    i = 0;
    printf("Encdata: ");
    while(i<uDataSize) {
        printf("0x%02x, ",pDumpShmVirtAddr[i]);
        i++;
    }
    printf("\n");


    SrcData = (uint8_t *)malloc(uDataSize);
    assert(SrcData != NULL);
    fseek(Datafp, (0L - uDataSize), SEEK_CUR);

    uReadSize = fread(SrcData, 1, uDataSize, Datafp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x, %p\n",uReadSize,Datafp);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    for(j=0; j<uSteps; j++) {
        memcpy(pDumpShmVirtAddr, pDecOutVirtAddr + j*uDataSize, uDataSize);
       for(k=0; k<uDataSize; k++) {
           if(pDumpShmVirtAddr[k] != SrcData[k]) {
               printf("DEC fail:Out[%d][%d]=0x%02x,0x%02x\n",
                   j, k, pDumpShmVirtAddr[k],SrcData[k]);
               ret = TEEC_ERROR_CANCEL;
               goto error;
           }
       }
    }
    if((j==uSteps) && (k==uDataSize)) {
       printf("/**************************************/\n");
       printf("/*            DEC Success!                */\n");
       printf("/**************************************/\n");
    }

    freeSHMBuffer(pEncInVirtAddr, hEncInBuff_fd);
    freeSHMBuffer(pEncOutVirtAddr, hEncOutBuff_fd);
    freeSHMBuffer(pDecInVirtAddr, hDecInBuff_fd);
    freeSHMBuffer(pDecOutVirtAddr, hDecOutBuff_fd);
    freeSHMBuffer(pDumpShmVirtAddr, hDumpShm_fd);

    if (uSessionID_1 != 0) {
        TEEC_CloseSession(pM2MSession1);
        free(pM2MSession1);
        pM2MSession1 = NULL;
    }

    if (uSessionID_2 != 0) {
        TEEC_CloseSession(pM2MSession2);
        free(pM2MSession2);
        pM2MSession2 = NULL;
    }

    TEEC_CloseSession(&g_m2m_session_default);
    TEEC_FinalizeContext(&g_m2m_ctx);

    if(RefData != NULL) {
      free(RefData);
    }
    if(SrcData != NULL) {
      free(SrcData);
    }
    if(Datafp != NULL) {
      fclose(Datafp);
    }
    if(Keyfp != NULL) {
      fclose(Keyfp);
    }
    if(IVfp != NULL) {
      fclose(IVfp);
    }
    if(Reffp != NULL) {
      fclose(Reffp);
    }


    if(cma_cust_dev_fd >= 0) {
        dmaheap_close(cma_cust_dev_fd);
    }

    if (g_bm_dev_fd >= 0) {
        close(g_bm_dev_fd);
        g_bm_dev_fd = -1;
    }

    return 0;
error:
    if (uSessionID_1 != 0) {
        TEEC_CloseSession(pM2MSession1);
        free(pM2MSession1);
        pM2MSession1 = NULL;
    }

    if (uSessionID_2 != 0) {
        TEEC_CloseSession(pM2MSession2);
        free(pM2MSession2);
        pM2MSession2 = NULL;
    }

    if (g_m2m_session_default.imp.session_id != 0) {
        TEEC_CloseSession(&g_m2m_session_default);
    }

    if (g_m2m_ctx.imp.fd > 0) {
        TEEC_FinalizeContext(&g_m2m_ctx);
    }

    if(hEncInBuff_fd){
        freeSHMBuffer(pEncInVirtAddr, hEncInBuff_fd);
    }
    if(hEncOutBuff_fd){
        freeSHMBuffer(pEncOutVirtAddr, hEncOutBuff_fd);
    }
    if(hDecInBuff_fd){
        freeSHMBuffer(pDecInVirtAddr, hDecInBuff_fd);
    }
    if(hDecOutBuff_fd){
        freeSHMBuffer(pDecOutVirtAddr, hDecOutBuff_fd);
    }
    if(hDumpShm_fd){
        freeSHMBuffer(pDumpShmVirtAddr, hDumpShm_fd);
    }

    if(iFigoShmFd_1){
        freeSHMBuffer(pFigoShm1InVirtAddr, iFigoShmFd_1);
    }
    if(iFigoShmFd_2){
        freeSHMBuffer(pFigoShm2VirtAddr, iFigoShmFd_2);
    }


    if(cma_cust_dev_fd >= 0) {
        dmaheap_close(cma_cust_dev_fd);
    }

    if (g_bm_dev_fd >= 0) {
        close(g_bm_dev_fd);
        g_bm_dev_fd = -1;
    }

    if(RefData != NULL) {
      free(RefData);
    }
    if(SrcData != NULL) {
      free(SrcData);
    }
    if(Datafp != NULL) {
      fclose(Datafp);
    }
    if(Keyfp != NULL) {
      fclose(Keyfp);
    }
    if(IVfp != NULL) {
      fclose(IVfp);
    }
    if(Reffp != NULL) {
      fclose(Reffp);
    }
    return ret;
}

#include "uapi/m2m.h"

static int test_m2m_basic_encdec_by_kernel(uint32_t uInputScheme, char *strmPath)
{
    uint32_t i=0, j=0, k=0;
    int ret = 0;
    uint8_t *RefData = NULL;
    uint8_t *SrcData = NULL;
    FILE    *Datafp = NULL;
    FILE    *Keyfp = NULL;
    FILE    *IVfp = NULL;
    FILE    *Reffp = NULL;
    char Comm_name[128];
    char Data_name[128];
    char Key_name[128];
    char IV_name[128];
    char Ref_name[128];
    uint32_t uiBuffSize = 0, uDataSize = 0, uReadSize = 0, File_size = 0, uSteps = 0;
    uint8_t uItem = 0;

    int cma_cust_dev_fd = -1;
    int hDumpShm_fd = -1;
    uint8_t *pDumpShmVirtAddr = NULL;

    int hEncInBuff_fd = -1, hEncOutBuff_fd = -1;
    uint8_t *pEncInVirtAddr = NULL, *pEncOutVirtAddr = NULL;

    int hDecInBuff_fd = -1, hDecOutBuff_fd = -1;
    uint8_t *pDecInVirtAddr = NULL, *pDecOutVirtAddr = NULL;
    int tmp_memid_dec_in = 0;
    int tmp_memid_dec_out = 0;
    int tmp_memid_enc_in = 0;
    int tmp_memid_enc_out = 0;

    uint8_t EncKeyData[24] = {0};
    uint8_t EncIVData[16] = {0};

    M2M_DRM_CRYPTO_ST CryptInfo;

    int m2m_enc_fd = -1;
    int m2m_dec_fd = -1;
    struct m2m_key  m2m_key_info;
    struct m2m_mem m2m_mem_info;

    if(uInputScheme>M2MCRYPTO_TYPE_ASA || uInputScheme<M2MCRYPTO_TYPE_AES_128_ECB) {
        printf("Input scheme %d not supported\n",uInputScheme);
        return -1;
    }

    m2m_enc_fd = open("/dev/m2m", O_RDWR, 0);
    if (m2m_enc_fd < 0) {
        fprintf(stderr, "Failed to open /dev/m2m\n");
        return -1;
    }

    m2m_dec_fd = open("/dev/m2m", O_RDWR, 0);
    if (m2m_dec_fd < 0) {
        fprintf(stderr, "Failed to open /dev/m2m\n");
        return -1;
    }

    uItem = get_m2m_file_item(uInputScheme);
    printf("\n<%s> test case begin:\n",M2M_File[uItem].Name);

    memset(Comm_name,0,sizeof(Comm_name));

    memset(Data_name,0,sizeof(Data_name));
    memset(Key_name,0,sizeof(Key_name));
    memset(IV_name,0,sizeof(IV_name));
    memset(Ref_name,0,sizeof(Ref_name));

    assert(gPath);
    strcat(Comm_name,gPath);
    strcat(Comm_name,strmPath);
    strcat(Comm_name,M2M_File[uItem].Name);
    memcpy(Data_name,Comm_name, sizeof(Comm_name));
    memcpy(Key_name,Comm_name, sizeof(Comm_name));
    memcpy(IV_name,Comm_name, sizeof(Comm_name));
    memcpy(Ref_name,Comm_name, sizeof(Comm_name));

    strcat(Data_name,".dat");
    strcat(Key_name,".key");
    strcat(IV_name,".iv");
    strcat(Ref_name,".ref");

    printf("Data_name = %s\n",Data_name);
    printf("Key_name = %s\n",Key_name);
    printf("IV_name = %s\n",IV_name);
    printf("Ref_name = %s\n",Ref_name);

    Datafp = fopen(Data_name, "rb");
    Keyfp  = fopen(Key_name, "rb");
    IVfp   = fopen(IV_name, "rb");
    Reffp  = fopen(Ref_name, "rb");

    if(Datafp == NULL || Keyfp == NULL || IVfp == NULL || Reffp == NULL) {
       printf("Open file Data:%p, Key:%p, IV:%p, Ref:%p fail\n", Datafp, Keyfp, IVfp, Reffp);
       ret = TEEC_ERROR_CANCEL;
       goto error;
    }

    fseek(Datafp , 0 , SEEK_END);
    File_size = ftell(Datafp);
    rewind(Datafp);

    uDataSize = File_size-File_size%16;
    if(0 == uDataSize){
        printf("uDataSize = 0x%x\n",uDataSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    uiBuffSize = BASIC_LOOP*uDataSize;

    cma_cust_dev_fd = dmaheap_open("CMA-CUST-reserved");
    if (cma_cust_dev_fd < 0) {
        printf("Failed to open dmaheap\n");
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uDataSize, &pDumpShmVirtAddr, &hDumpShm_fd);
    if (ret < 0 || pDumpShmVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pDumpShmVirtAddr=%p\n", __LINE__, ret, pDumpShmVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    uSteps = uiBuffSize/uDataSize;

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncInVirtAddr, &hEncInBuff_fd);
    if (ret < 0 || pEncInVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncInVirtAddr=%p\n", __LINE__, ret, pEncInVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncOutVirtAddr, &hEncOutBuff_fd);
    if (ret < 0 || pEncOutVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncOutVirtAddr=%p\n", __LINE__, ret, pEncOutVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pDecInVirtAddr, &hDecInBuff_fd);
    if (ret < 0 || pDecInVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pDecInVirtAddr=%p\n", __LINE__, ret, pDecInVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pDecOutVirtAddr, &hDecOutBuff_fd);
    if (ret < 0 || pDecOutVirtAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pDecOutVirtAddr=%p\n", __LINE__, ret, pDecOutVirtAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    uReadSize = fread(EncKeyData, 1, uDataSize, Keyfp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x\n",uReadSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }
    uReadSize = fread(EncIVData, 1, uDataSize, IVfp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x\n",uReadSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }
    uReadSize = fread(pDumpShmVirtAddr, 1, uDataSize, Datafp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x\n",uReadSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

#if 1
    i = 0;
    printf("Key: ");
    while(i<16) {
         printf("0x%02x, ",EncKeyData[i]);
         i++;
    }
    printf("\n");

    i = 0;
    printf("IV: ");
    while(i<16) {
        printf("0x%02x, ",EncIVData[i]);
        i++;
    }
    printf("\n");

    i = 0;
    printf("Src data: ");
    while(i<uDataSize) {
        printf("0x%02x, ",pDumpShmVirtAddr[i]);
        i++;
    }
    printf("\n");
    #endif

    /*Prepare input SHM*/
    for(j=0; j<uSteps; j++) {
        memcpy(pEncInVirtAddr+j*uDataSize, pDumpShmVirtAddr, uDataSize);
    }


    ret = ioctl(m2m_enc_fd, M2M_IOC_SET_MODE, M2M_CRYPTO_MODE_ENC);
    if (ret < 0) {
        fprintf(stderr, "Failed to set M2M mode to ENC\n");
        goto error;
    }

    ret = ioctl(m2m_enc_fd, M2M_IOC_SET_SCHEME, uInputScheme);
    if (ret < 0) {
        fprintf(stderr, "Failed to get Figo SysId 1\n");
        goto error;
    }

    m2m_key_info.key_option = M2M_KEY_CONTENT;
    m2m_key_info.key_index = 0;
    m2m_key_info.key_length = M2M_File[uItem].Keylength;
    memcpy(m2m_key_info.key_data, EncKeyData, M2M_File[uItem].Keylength);

    m2m_key_info.iv_option = M2M_KEY_CONTENT;
    m2m_key_info.iv_index = 0;
    m2m_key_info.iv_length = 16; // IV length is always 16 bytes for AES
    memset(m2m_key_info.iv_data, 0, sizeof(m2m_key_info.iv_data));
    memcpy(m2m_key_info.iv_data, EncIVData, 16);
    ret = ioctl(m2m_enc_fd, M2M_IOC_SET_KEY, &m2m_key_info);
    if (ret < 0) {
        fprintf(stderr, "Failed to set key and IV\n");
        goto error;
    }

    ret = ioctl(m2m_dec_fd, M2M_IOC_SET_MODE, M2M_CRYPTO_MODE_DEC);
    if (ret < 0) {
        fprintf(stderr, "Failed to set M2M mode to ENC\n");
        goto error;
    }

    ret = ioctl(m2m_dec_fd, M2M_IOC_SET_SCHEME, uInputScheme);
    if (ret < 0) {
        fprintf(stderr, "Failed to get Figo SysId 1\n");
        goto error;
    }
    memset(&m2m_key_info, 0, sizeof(m2m_key_info));
    m2m_key_info.key_option = M2M_KEY_CONTENT;
    m2m_key_info.key_index = 0;
    m2m_key_info.key_length = M2M_File[uItem].Keylength;
    memcpy(m2m_key_info.key_data, EncKeyData, M2M_File[uItem].Keylength);
    m2m_key_info.iv_option = M2M_KEY_CONTENT;
    m2m_key_info.iv_index = 0;
    m2m_key_info.iv_length = 16; // IV length is always 16 bytes for AES
    memset(m2m_key_info.iv_data, 0, sizeof(m2m_key_info.iv_data));
    memcpy(m2m_key_info.iv_data, EncIVData, 16);
    ret = ioctl(m2m_dec_fd, M2M_IOC_SET_KEY, &m2m_key_info);
    if (ret < 0) {
        fprintf(stderr, "Failed to set key and IV\n");
        goto error;
    }

    //need to alloc memid to shm buffer in order to call M2M_IOC_UPDATE
    tmp_memid_dec_in = getShmMemid(hDecInBuff_fd);
    tmp_memid_dec_out = getShmMemid(hDecOutBuff_fd);
    tmp_memid_enc_in = getShmMemid(hEncInBuff_fd);
    tmp_memid_enc_out = getShmMemid(hEncOutBuff_fd);

    for(j=0; j<uSteps; j++) {
        /*Enc uReadSize block data*/
        m2m_mem_info.inbuf.fd = hEncInBuff_fd;
        m2m_mem_info.inbuf.offset = j*uDataSize;
        m2m_mem_info.inbuf.size = uDataSize;
        m2m_mem_info.outbuf.fd = hEncOutBuff_fd;
        m2m_mem_info.outbuf.offset = j*uDataSize;
        m2m_mem_info.outbuf.size = uDataSize;
        ret = ioctl(m2m_enc_fd, M2M_IOC_UPDATE, &m2m_mem_info);
        if (ret < 0) {
            fprintf(stderr, "m2m_session_update failed with code 0x%x\n", ret);
            goto error;
        }

        /*Dec uReadSize block data*/
        memcpy(pDecInVirtAddr + j*uDataSize, pEncOutVirtAddr + j*uDataSize, uDataSize);
        m2m_mem_info.inbuf.fd = hDecInBuff_fd;
        m2m_mem_info.inbuf.offset = j*uDataSize;
        m2m_mem_info.inbuf.size = uDataSize;
        m2m_mem_info.outbuf.fd = hDecOutBuff_fd;
        m2m_mem_info.outbuf.offset = j*uDataSize;
        m2m_mem_info.outbuf.size = uDataSize;
        ret = ioctl(m2m_dec_fd, M2M_IOC_UPDATE, &m2m_mem_info);
        if (ret < 0) {
            fprintf(stderr, "m2m_session_update failed with code 0x%x\n", ret);
            goto error;
        }
    }

    /*Check result*/
    /*Prepare input SHM*/
    RefData = (uint8_t *)malloc(uDataSize);
    assert(RefData != NULL);
    uReadSize = fread(RefData, 1, uDataSize, Reffp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x\n",uReadSize);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    for(j=0; j<uSteps; j++) {
        memcpy(pDumpShmVirtAddr, pEncOutVirtAddr + j*uDataSize, uDataSize);
        for(k=0; k<uDataSize; k++) {
            if(pDumpShmVirtAddr[k] != RefData[k]) {
                printf("ENC fail:Out[%d][%d]=0x%02x,0x%02x\n",
                    j, k, pDumpShmVirtAddr[k],RefData[k]);
                ret = TEEC_ERROR_CANCEL;
                goto error;
            }
        }
    }
    if((j==uSteps) && (k==uDataSize)) {
        printf("/**************************************/\n");
        printf("/*            ENC Success!                */\n");
        printf("/**************************************/\n");
    }

    i = 0;
    printf("Encdata: ");
    while(i<uDataSize) {
        printf("0x%02x, ",pDumpShmVirtAddr[i]);
        i++;
    }
    printf("\n");


    SrcData = (uint8_t *)malloc(uDataSize);
    assert(SrcData != NULL);
    fseek(Datafp, (0L - uDataSize), SEEK_CUR);

    uReadSize = fread(SrcData, 1, uDataSize, Datafp);
    if(uReadSize < uDataSize) {
        printf("uReadSize = 0x%x, %p\n",uReadSize,Datafp);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    for(j=0; j<uSteps; j++) {
        memcpy(pDumpShmVirtAddr, pDecOutVirtAddr + j*uDataSize, uDataSize);
       for(k=0; k<uDataSize; k++) {
           if(pDumpShmVirtAddr[k] != SrcData[k]) {
               printf("DEC fail:Out[%d][%d]=0x%02x,0x%02x\n",
                   j, k, pDumpShmVirtAddr[k],SrcData[k]);
               ret = TEEC_ERROR_CANCEL;
               goto error;
           }
       }
    }
    if((j==uSteps) && (k==uDataSize)) {
       printf("/**************************************/\n");
       printf("/*            DEC Success!                */\n");
       printf("/**************************************/\n");
    }

    freeSHMBuffer(pEncInVirtAddr, hEncInBuff_fd);
    freeSHMBuffer(pEncOutVirtAddr, hEncOutBuff_fd);
    freeSHMBuffer(pDecInVirtAddr, hDecInBuff_fd);
    freeSHMBuffer(pDecOutVirtAddr, hDecOutBuff_fd);
    freeSHMBuffer(pDumpShmVirtAddr, hDumpShm_fd);

    if(RefData != NULL) {
      free(RefData);
    }
    if(SrcData != NULL) {
      free(SrcData);
    }
    if(Datafp != NULL) {
      fclose(Datafp);
    }
    if(Keyfp != NULL) {
      fclose(Keyfp);
    }
    if(IVfp != NULL) {
      fclose(IVfp);
    }
    if(Reffp != NULL) {
      fclose(Reffp);
    }


    if(cma_cust_dev_fd >= 0) {
        dmaheap_close(cma_cust_dev_fd);
    }

    if (g_bm_dev_fd >= 0) {
        close(g_bm_dev_fd);
        g_bm_dev_fd = -1;
    }

    if (m2m_enc_fd >= 0) {
        close(m2m_enc_fd);
    }
    if (m2m_dec_fd >= 0) {
        close(m2m_dec_fd);
    }

    return 0;
error:
    if(hEncInBuff_fd){
        freeSHMBuffer(pEncInVirtAddr, hEncInBuff_fd);
    }
    if(hEncOutBuff_fd){
        freeSHMBuffer(pEncOutVirtAddr, hEncOutBuff_fd);
    }
    if(hDecInBuff_fd){
        freeSHMBuffer(pDecInVirtAddr, hDecInBuff_fd);
    }
    if(hDecOutBuff_fd){
        freeSHMBuffer(pDecOutVirtAddr, hDecOutBuff_fd);
    }
    if(hDumpShm_fd){
        freeSHMBuffer(pDumpShmVirtAddr, hDumpShm_fd);
    }

    if(cma_cust_dev_fd >= 0) {
        dmaheap_close(cma_cust_dev_fd);
    }

    if (g_bm_dev_fd >= 0) {
        close(g_bm_dev_fd);
        g_bm_dev_fd = -1;
    }

    if(RefData != NULL) {
      free(RefData);
    }
    if(SrcData != NULL) {
      free(SrcData);
    }
    if(Datafp != NULL) {
      fclose(Datafp);
    }
    if(Keyfp != NULL) {
      fclose(Keyfp);
    }
    if(IVfp != NULL) {
      fclose(IVfp);
    }
    if(Reffp != NULL) {
      fclose(Reffp);
    }

    if (m2m_enc_fd >= 0) {
        close(m2m_enc_fd);
    }
    if (m2m_dec_fd >= 0) {
        close(m2m_dec_fd);
    }
    return ret;
}


static int test_m2m_patternMode(uint32_t uScheme,uint8_t enc, uint8_t clr, uint32_t uDataSize)
{
    uint32_t i=0, j=0, uSteps=0;
    uint32_t uEncDataSize = 0,uBlockSize=0,uResidueSize=0, uResidueEncSize = 0;
    int hEncInBuff_fd, hEncOutRefBuff_fd, hEncOutBuff_fd, hDecOutBuff_fd;
    uint8_t *pEncInVirAddr = NULL, *pEncOutVirAddr = NULL;
    uint8_t *pEncOutRefVirAddr = NULL, *pDecOutVirAddr = NULL;
    static uint8_t uKey[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    static uint8_t uIv[16] = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
    uint32_t uiBuffSize = 0;

    int cma_cust_dev_fd = -1;
    uint32_t uSessionID_1 = 0, uSessionID_2 = 0;

    TEEC_Session * pM2MSession1 = NULL;
    TEEC_Session * pM2MSession2 = NULL;
    uint32_t uFigoSysId_1 = 0, uFigoSysId_2 = 0;
    M2M_DRM_CRYPTO_ST CryptInfo;
    M2MCRYPTO_MEMOBJ InputMem, OutputMem;
    M2MCRYPTO_PATTERN_MODE_CFG patternModeCfg;
    TEEC_Result ret = TEEC_SUCCESS;


    if(uScheme>M2MCRYPTO_TYPE_ASA || uScheme<M2MCRYPTO_TYPE_AES_128_ECB) {
        printf("Input scheme %d not supported\n",uScheme);
        return -1;
    }

    ret = TEEC_InitializeContext(NULL, &g_m2m_ctx);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "TEEC_InitializeContext failed with code 0x%x\n", ret);
        return ret;
    }

    cma_cust_dev_fd = dmaheap_open("CMA-CUST-reserved");
    if (cma_cust_dev_fd < 0) {
        printf("Failed to open dmaheap\n");
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    uiBuffSize =  (uDataSize&~(32-1))+32;

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncInVirAddr, &hEncInBuff_fd);
    if (ret < 0 || pEncInVirAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncInVirAddr=%p\n", __LINE__, ret, pEncInVirAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncOutVirAddr, &hEncOutBuff_fd);
    if (ret < 0 || pEncOutVirAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncOutVirAddr=%p\n", __LINE__, ret, pEncOutVirAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncOutRefVirAddr, &hEncOutRefBuff_fd);
    if (ret < 0 || pEncOutRefVirAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncOutRefVirAddr=%p\n", __LINE__, ret, pEncOutRefVirAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pDecOutVirAddr, &hDecOutBuff_fd);
    if (ret < 0 || pDecOutVirAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pDecOutVirAddr=%p\n", __LINE__, ret, pDecOutVirAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }


    memset(pEncInVirAddr, 0x47, uDataSize);
    memset(pEncOutVirAddr, 0, uDataSize);
    memset(pEncOutRefVirAddr, 0x47, uDataSize);

    uBlockSize = 16*(enc+clr);
    uSteps = uDataSize/uBlockSize;
    uResidueSize = uDataSize%uBlockSize;
    if(uResidueSize)
        uSteps += 1;
    else
        uResidueSize = uBlockSize;

    uEncDataSize = enc*16;

    for(j=0;j<uSteps;j++){
        uint32_t uSize;
        if(j != uSteps-1){
            uSize = uEncDataSize;
        }else{
            uSize = (uResidueSize <= uEncDataSize) ? uResidueSize : uEncDataSize;
            uResidueEncSize = uSize;
        }
        for(i=0;i<uSize;i++){
            pEncInVirAddr[i+j*uBlockSize] = i;
        }
    }


    /*CryptInfo structure need to be set when key is from drm*/
    memset(&CryptInfo, 0, sizeof(M2M_DRM_CRYPTO_ST));
    CryptInfo.uiSchemeType = M2M_DRMSCHEME_INVALID;
    CryptInfo.uiSessionID = 0x0;
    patternModeCfg.pattern_enc = enc;
    patternModeCfg.pattern_clr = clr;

    ret = m2m_session_open_default();
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_open_default failed with code 0x%x\n", ret);
        goto error;
    }

    /*Open session#1*/
    ret =  m2m_session_open_with_ops(&g_m2m_ctx, M2MCRYPTO_SESSION_TYPE_ENC, &uFigoSysId_1,
                                            (void**)&pM2MSession1, &uSessionID_1);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_open_with_ops failed with code 0x%x\n", ret);
        goto error;
    }

    ret = m2m_session_setscheme(pM2MSession1, (M2MCRYPTO_TYPE)uScheme);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_setscheme failed with code 0x%x\n", ret);
        goto error;
    }

    ret = m2m_session_patternmode(pM2MSession1, &patternModeCfg);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_patternmode failed with code 0x%x\n", ret);
        goto error;
    }


    ret = m2m_session_config(pM2MSession1,
                            M2MCRYPTO_KEY_CONTENT,
                            0,
                            uKey,
                            16,
                            M2MCRYPTO_KEY_CONTENT,
                            0,
                            uIv,
                            16,
                            &CryptInfo);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_config failed with code 0x%x\n", ret);
        goto error;
    }

    /*Open session#2*/
    ret =  m2m_session_open_with_ops(&g_m2m_ctx, M2MCRYPTO_SESSION_TYPE_DEC, &uFigoSysId_2,
                                            (void**)&pM2MSession2, &uSessionID_2);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_open_with_ops failed with code 0x%x\n", ret);
        goto error;
    }

    ret = m2m_session_setscheme(pM2MSession2, (M2MCRYPTO_TYPE)uScheme);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_setscheme failed with code 0x%x\n", ret);
        goto error;
    }

    ret = m2m_session_patternmode(pM2MSession2, &patternModeCfg);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_patternmode failed with code 0x%x\n", ret);
        goto error;
    }

    ret = m2m_session_config(pM2MSession2,
                            M2MCRYPTO_KEY_CONTENT,
                            0,
                            uKey,
                            16,
                            M2MCRYPTO_KEY_CONTENT,
                            0,
                            uIv,
                            16,
                            &CryptInfo);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_config failed with code 0x%x\n", ret);
        goto error;
    }

    /*Enc uReadSize block data*/
    InputMem.hShm_memid = getShmMemid(hEncInBuff_fd);
    InputMem.uOffset = 0;
    InputMem.uSize = uDataSize;
    OutputMem.hShm_memid = getShmMemid(hEncOutBuff_fd);
    OutputMem.uOffset = 0;
    OutputMem.uSize = uDataSize;
    ret = m2m_session_update(pM2MSession1, &InputMem, &OutputMem);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_update failed with code 0x%x\n", ret);
        goto error;
    }

    while(1) {
        uint32_t isDone = 0;
        ret = m2m_session_routine(pM2MSession1, &isDone);
        if (ret != TEEC_SUCCESS) {
            fprintf(stderr, "m2m_session_routine failed with code 0x%x\n", ret);
            goto error;
        }
        if (isDone != 0) {
            break;
        }
    }

    InputMem.hShm_memid = getShmMemid(hEncOutBuff_fd);
    InputMem.uOffset = 0;
    InputMem.uSize = uDataSize;
    OutputMem.hShm_memid = getShmMemid(hDecOutBuff_fd);
    OutputMem.uOffset = 0;
    OutputMem.uSize = uDataSize;
    ret = m2m_session_update(pM2MSession2, &InputMem, &OutputMem);
    if (ret != TEEC_SUCCESS) {
        fprintf(stderr, "m2m_session_update failed with code 0x%x\n", ret);
        goto error;
    }

    while(1) {
        uint32_t isDone = 0;
        ret = m2m_session_routine(pM2MSession2, &isDone);
        if (ret != TEEC_SUCCESS) {
            fprintf(stderr, "m2m_session_routine failed with code 0x%x\n", ret);
            goto error;
        }
        if (isDone != 0) {
            break;
        }
    }

    if(clr) {
        ret = m2m_session_config(pM2MSession1,
                                M2MCRYPTO_KEY_CONTENT,
                                0,
                                uKey,
                                16,
                                M2MCRYPTO_KEY_CONTENT,
                                0,
                                uIv,
                                16,
                                &CryptInfo);
        if (ret != TEEC_SUCCESS) {
            fprintf(stderr, "m2m_session_config failed with code 0x%x\n", ret);
            goto error;
        }

        // not need, but it is better to set to normal mode
        patternModeCfg.pattern_enc = 1; //default value for normal mode
        patternModeCfg.pattern_clr = 0;

        ret = m2m_session_patternmode(pM2MSession1, &patternModeCfg);
        if (ret != TEEC_SUCCESS) {
            fprintf(stderr, "m2m_session_patternmode failed with code 0x%x\n", ret);
            goto error;
        }

        ret = m2m_session_set_residuemode(pM2MSession1,  M2MCRYPTO_RESIDUE_TYPE_DEFAULT);
        if (ret != TEEC_SUCCESS) {
            fprintf(stderr, "m2m_session_set_residuemode failed with code 0x%x\n", ret);
            goto error;
        }

        for(j=0; j<uSteps; j++){
            uint32_t uInputEncSize = uEncDataSize;
            if(j == uSteps-1 )
                uInputEncSize = uResidueEncSize;
            /*Normal Enc*/
            InputMem.hShm_memid = getShmMemid(hEncInBuff_fd);
            InputMem.uOffset = j*uBlockSize;
            InputMem.uSize = uInputEncSize;
            OutputMem.hShm_memid = getShmMemid(hEncOutRefBuff_fd);
            OutputMem.uOffset = j*uBlockSize;
            OutputMem.uSize = uInputEncSize;

            ret = m2m_session_update(pM2MSession1, &InputMem, &OutputMem);
            if (ret != TEEC_SUCCESS) {
                fprintf(stderr, "m2m_session_update failed with code 0x%x\n", ret);
                goto error;
            }
            while(1) {
                uint32_t isDone = 0;
                ret = m2m_session_routine(pM2MSession1, &isDone);
                if (ret != TEEC_SUCCESS) {
                    fprintf(stderr, "m2m_session_routine failed with code 0x%x\n", ret);
                    goto error;
                }
                if (isDone != 0) {
                    break;
                }
            }
        }

        for(i=0; i<uDataSize; i++) {
            if(pEncOutVirAddr[i] != pEncOutRefVirAddr[i]) {
                printf("verify encrypted data failed at position %d [0x%2x 0x%2x]\n",i,pEncOutVirAddr[i],pEncOutRefVirAddr[i]);

                for(j=i; j<16+i; j++){
                    printf("0x%2x ",pEncOutVirAddr[j]);
                }
                printf("\n");
                for(j=i; j<16+i; j++){
                    printf("0x%2x ",pEncOutRefVirAddr[j]);
                }
                printf("\n");
                goto error;
            }
        }
        if(i==uDataSize) {
            printf("Verify encrypted data Success!\n");
        }
    } else { //allow this case to check normal Mode
        printf("not patternMode, skip verifing encrypted data\n");
    }

    for(i=0; i<uDataSize; i++) {
        if(pDecOutVirAddr[i] != pEncInVirAddr[i]) {
            printf("verify decrypted data failed at position %d\n",i);
            goto error;
        }
    }
    if(i==uDataSize) {
        printf("Verify decrypted data Success!\n\n");
    }


    freeSHMBuffer(pEncInVirAddr, hEncInBuff_fd);
    freeSHMBuffer(pEncOutVirAddr, hEncOutBuff_fd);
    freeSHMBuffer(pDecOutVirAddr, hDecOutBuff_fd);
    freeSHMBuffer(pEncOutRefVirAddr, hEncOutRefBuff_fd);


    if (uSessionID_1 != 0) {
        TEEC_CloseSession(pM2MSession1);
        free(pM2MSession1);
        pM2MSession1 = NULL;
    }

    if (uSessionID_2 != 0) {
        TEEC_CloseSession(pM2MSession2);
        free(pM2MSession2);
        pM2MSession2 = NULL;
    }

    TEEC_CloseSession(&g_m2m_session_default);
    TEEC_FinalizeContext(&g_m2m_ctx);

    if(cma_cust_dev_fd >= 0) {
        dmaheap_close(cma_cust_dev_fd);
    }

    if (g_bm_dev_fd >= 0) {
        close(g_bm_dev_fd);
        g_bm_dev_fd = -1;
    }

    return 0;
error:
    if (uSessionID_1 != 0) {
        TEEC_CloseSession(pM2MSession1);
        free(pM2MSession1);
        pM2MSession1 = NULL;
    }

    if (uSessionID_2 != 0) {
        TEEC_CloseSession(pM2MSession2);
        free(pM2MSession2);
        pM2MSession2 = NULL;
    }

    if (g_m2m_session_default.imp.session_id != 0) {
        TEEC_CloseSession(&g_m2m_session_default);
    }

    if (g_m2m_ctx.imp.fd > 0) {
        TEEC_FinalizeContext(&g_m2m_ctx);
    }

    if(hEncInBuff_fd){
        freeSHMBuffer(pEncInVirAddr, hEncInBuff_fd);
    }
    if(hEncOutBuff_fd){
        freeSHMBuffer(pEncOutVirAddr, hEncOutBuff_fd);
    }
    if(hDecOutBuff_fd){
        freeSHMBuffer(pDecOutVirAddr, hDecOutBuff_fd);
    }
    if(hEncOutRefBuff_fd){
        freeSHMBuffer(pEncOutRefVirAddr, hEncOutRefBuff_fd);
    }
    if(cma_cust_dev_fd >= 0) {
        dmaheap_close(cma_cust_dev_fd);
    }

    if (g_bm_dev_fd >= 0) {
        close(g_bm_dev_fd);
        g_bm_dev_fd = -1;
    }

    return ret;
}

static int test_m2m_patternMode_by_kernel(uint32_t uScheme,uint8_t enc, uint8_t clr, uint32_t uDataSize)
{
    TEEC_Result ret = TEEC_SUCCESS;
    uint32_t i=0, j=0, uSteps=0;
    uint32_t uEncDataSize = 0,uBlockSize=0,uResidueSize=0, uResidueEncSize = 0;
    int hEncInBuff_fd, hEncOutRefBuff_fd, hEncOutBuff_fd, hDecOutBuff_fd;
    uint8_t *pEncInVirAddr = NULL, *pEncOutVirAddr = NULL;
    uint8_t *pEncOutRefVirAddr = NULL, *pDecOutVirAddr = NULL;
    static uint8_t uKey[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    static uint8_t uIv[16] = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
    uint32_t uiBuffSize = 0;

    int cma_cust_dev_fd = -1;
    int m2m_enc_fd = -1;
    int m2m_dec_fd = -1;
    int tmp_memid_dec_out = 0;
    int tmp_memid_enc_in = 0;
    int tmp_memid_enc_out = 0;
    int tmp_memid_enc_ref = 0;

    struct m2m_key  m2m_key_info;
    struct m2m_mem m2m_mem_info;
    struct m2m_pattern_mode s_mem_pattern_mode = {.pattern_enc = enc, .pattern_clr = clr};

    if(uScheme>M2MCRYPTO_TYPE_ASA || uScheme<M2MCRYPTO_TYPE_AES_128_ECB) {
        printf("Input scheme %d not supported\n",uScheme);
        return -1;
    }

    m2m_enc_fd = open("/dev/m2m", O_RDWR, 0);
    if (m2m_enc_fd < 0) {
        fprintf(stderr, "Failed to open /dev/m2m\n");
        return -1;
    }

    m2m_dec_fd = open("/dev/m2m", O_RDWR, 0);
    if (m2m_dec_fd < 0) {
        fprintf(stderr, "Failed to open /dev/m2m\n");
        return -1;
    }

    cma_cust_dev_fd = dmaheap_open("CMA-CUST-reserved");
    if (cma_cust_dev_fd < 0) {
        printf("Failed to open dmaheap\n");
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    uiBuffSize =  (uDataSize&~(32-1))+32;

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncInVirAddr, &hEncInBuff_fd);
    if (ret < 0 || pEncInVirAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncInVirAddr=%p\n", __LINE__, ret, pEncInVirAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncOutVirAddr, &hEncOutBuff_fd);
    if (ret < 0 || pEncOutVirAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncOutVirAddr=%p\n", __LINE__, ret, pEncOutVirAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pEncOutRefVirAddr, &hEncOutRefBuff_fd);
    if (ret < 0 || pEncOutRefVirAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pEncOutRefVirAddr=%p\n", __LINE__, ret, pEncOutRefVirAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }

    ret = allocSHMBuffer(cma_cust_dev_fd, uiBuffSize, &pDecOutVirAddr, &hDecOutBuff_fd);
    if (ret < 0 || pDecOutVirAddr == NULL) {
        printf("%d: Failed to allocate buffer ret=%d pDecOutVirAddr=%p\n", __LINE__, ret, pDecOutVirAddr);
        ret = TEEC_ERROR_CANCEL;
        goto error;
    }


    memset(pEncInVirAddr, 0x47, uDataSize);
    memset(pEncOutVirAddr, 0, uDataSize);
    memset(pEncOutRefVirAddr, 0x47, uDataSize);

    uBlockSize = 16*(enc+clr);
    uSteps = uDataSize/uBlockSize;
    uResidueSize = uDataSize%uBlockSize;
    if(uResidueSize)
        uSteps += 1;
    else
        uResidueSize = uBlockSize;

    uEncDataSize = enc*16;

    for(j=0;j<uSteps;j++){
        uint32_t uSize;
        if(j != uSteps-1){
            uSize = uEncDataSize;
        }else{
            uSize = (uResidueSize <= uEncDataSize) ? uResidueSize : uEncDataSize;
            uResidueEncSize = uSize;
        }
        for(i=0;i<uSize;i++){
            pEncInVirAddr[i+j*uBlockSize] = i;
        }
    }


    ret = ioctl(m2m_enc_fd, M2M_IOC_SET_MODE, M2M_CRYPTO_MODE_ENC);
    if (ret < 0) {
        fprintf(stderr, "Failed to set M2M mode to ENC\n");
        goto error;
    }

    ret = ioctl(m2m_enc_fd, M2M_IOC_SET_SCHEME, uScheme);
    if (ret < 0) {
        fprintf(stderr, "Failed to set scheme for ENC\n");
        goto error;
    }

    ret = ioctl(m2m_enc_fd, M2M_IOC_SET_PATTERN_MODE, &s_mem_pattern_mode);
    if (ret < 0) {
        fprintf(stderr, "Failed to set pattern mode for ENC\n");
        goto error;
    }

    m2m_key_info.key_option = M2M_KEY_CONTENT;
    m2m_key_info.key_index = 0;
    m2m_key_info.key_length = 16;
    memcpy(m2m_key_info.key_data, uKey, 16);

    m2m_key_info.iv_option = M2M_KEY_CONTENT;
    m2m_key_info.iv_index = 0;
    m2m_key_info.iv_length = 16; // IV length is always 16 bytes for AES
    memset(m2m_key_info.iv_data, 0, sizeof(m2m_key_info.iv_data));
    memcpy(m2m_key_info.iv_data, uIv, 16);
    ret = ioctl(m2m_enc_fd, M2M_IOC_SET_KEY, &m2m_key_info);
    if (ret < 0) {
        fprintf(stderr, "Failed to set key and IV\n");
        goto error;
    }


    ret = ioctl(m2m_dec_fd, M2M_IOC_SET_MODE, M2M_CRYPTO_MODE_DEC);
    if (ret < 0) {
        fprintf(stderr, "Failed to set M2M mode to DEC\n");
        goto error;
    }
    ret = ioctl(m2m_dec_fd, M2M_IOC_SET_SCHEME, uScheme);
    if (ret < 0) {
        fprintf(stderr, "Failed to set scheme for DEC\n");
        goto error;
    }
    ret = ioctl(m2m_dec_fd, M2M_IOC_SET_PATTERN_MODE, &s_mem_pattern_mode);
    if (ret < 0) {
        fprintf(stderr, "Failed to set pattern mode for DEC\n");
        goto error;
    }

    ret = ioctl(m2m_dec_fd, M2M_IOC_SET_KEY, &m2m_key_info);
    if (ret < 0) {
        fprintf(stderr, "Failed to set key and IV for DEC\n");
        goto error;
    }

    //need to alloc memid to shm buffer in order to call M2M_IOC_UPDATE
    tmp_memid_dec_out = getShmMemid(hDecOutBuff_fd);
    tmp_memid_enc_in = getShmMemid(hEncInBuff_fd);
    tmp_memid_enc_out = getShmMemid(hEncOutBuff_fd);
    tmp_memid_enc_ref = getShmMemid(hEncOutRefBuff_fd);

    /*Enc uReadSize block data*/
    m2m_mem_info.inbuf.fd = hEncInBuff_fd;
    m2m_mem_info.inbuf.offset = 0;
    m2m_mem_info.inbuf.size = uDataSize;
    m2m_mem_info.outbuf.fd = hEncOutBuff_fd;
    m2m_mem_info.outbuf.offset = 0;
    m2m_mem_info.outbuf.size = uDataSize;
    ret = ioctl(m2m_enc_fd, M2M_IOC_UPDATE, &m2m_mem_info);
    if (ret < 0) {
        fprintf(stderr, "m2m_session_update failed with code 0x%x\n", ret);
        goto error;
    }

    m2m_mem_info.inbuf.fd = hEncOutBuff_fd;
    m2m_mem_info.inbuf.offset = 0;
    m2m_mem_info.inbuf.size = uDataSize;
    m2m_mem_info.outbuf.fd = hDecOutBuff_fd;
    m2m_mem_info.outbuf.offset = 0;
    m2m_mem_info.outbuf.size = uDataSize;
    ret = ioctl(m2m_dec_fd, M2M_IOC_UPDATE, &m2m_mem_info);
    if (ret < 0) {
        fprintf(stderr, "m2m_session_update failed with code 0x%x\n", ret);
        goto error;
    }

    if(clr) {
        ret = ioctl(m2m_enc_fd, M2M_IOC_SET_KEY, &m2m_key_info);
        if (ret < 0) {
            fprintf(stderr, "m2m_session_config failed with code 0x%x\n", ret);
            goto error;
        }

        s_mem_pattern_mode.pattern_enc = 1; //default value for normal mode
        s_mem_pattern_mode.pattern_clr = 0; // not need, but it is better
        ret = ioctl(m2m_enc_fd, M2M_IOC_SET_PATTERN_MODE, &s_mem_pattern_mode);
        if (ret < 0) {
            fprintf(stderr, "m2m_session_patternmode failed with code 0x%x\n", ret);
            goto error;
        }

        ret = ioctl(m2m_enc_fd, M2M_IOC_SET_RESIDUE_MODE, M2M_RESIDUE_MODE_DEFAULT);
        if (ret < 0) {
            fprintf(stderr, "m2m_session_set_residuemode failed with code 0x%x\n", ret);
            goto error;
        }

        for(j=0; j<uSteps; j++){
            uint32_t uInputEncSize = uEncDataSize;
            if(j == uSteps-1 )
                uInputEncSize = uResidueEncSize;

            m2m_mem_info.inbuf.fd = hEncInBuff_fd;
            m2m_mem_info.inbuf.offset = j*uBlockSize;
            m2m_mem_info.inbuf.size = uInputEncSize;
            m2m_mem_info.outbuf.fd = hEncOutRefBuff_fd;
            m2m_mem_info.outbuf.offset = j*uBlockSize;
            m2m_mem_info.outbuf.size = uInputEncSize;

            ret = ioctl(m2m_enc_fd, M2M_IOC_UPDATE, &m2m_mem_info);
            if (ret < 0) {
                fprintf(stderr, "m2m_session_update[%d] failed with code 0x%x\n", __LINE__, ret);
                goto error;
            }
        }

        for(i=0; i<uDataSize; i++) {
            if(pEncOutVirAddr[i] != pEncOutRefVirAddr[i]) {
                printf("verify encrypted data failed at position %d [0x%2x 0x%2x]\n",i,pEncOutVirAddr[i],pEncOutRefVirAddr[i]);

                for(j=i; j<16+i; j++){
                    printf("0x%2x ",pEncOutVirAddr[j]);
                }
                printf("\n");
                for(j=i; j<16+i; j++){
                    printf("0x%2x ",pEncOutRefVirAddr[j]);
                }
                printf("\n");
                goto error;
            }
        }
        if(i==uDataSize) {
            printf("Verify encrypted data Success!\n");
        }
    } else { //allow this case to check normal Mode
        printf("not patternMode, skip verifing encrypted data\n");
    }

    for(i=0; i<uDataSize; i++) {
        if(pDecOutVirAddr[i] != pEncInVirAddr[i]) {
            printf("verify decrypted data failed at position %d\n",i);
            goto error;
        }
    }
    if(i==uDataSize) {
        printf("Verify decrypted data Success!\n\n");
    }


    freeSHMBuffer(pEncInVirAddr, hEncInBuff_fd);
    freeSHMBuffer(pEncOutVirAddr, hEncOutBuff_fd);
    freeSHMBuffer(pDecOutVirAddr, hDecOutBuff_fd);
    freeSHMBuffer(pEncOutRefVirAddr, hEncOutRefBuff_fd);

    if(cma_cust_dev_fd >= 0) {
        dmaheap_close(cma_cust_dev_fd);
    }

    if (g_bm_dev_fd >= 0) {
        close(g_bm_dev_fd);
        g_bm_dev_fd = -1;
    }

    if (m2m_enc_fd >= 0) {
        close(m2m_enc_fd);
        m2m_enc_fd = -1;
    }
    if (m2m_dec_fd >= 0) {
        close(m2m_dec_fd);
        m2m_dec_fd = -1;
    }
    return 0;
error:
    if(hEncInBuff_fd){
        freeSHMBuffer(pEncInVirAddr, hEncInBuff_fd);
    }
    if(hEncOutBuff_fd){
        freeSHMBuffer(pEncOutVirAddr, hEncOutBuff_fd);
    }
    if(hDecOutBuff_fd){
        freeSHMBuffer(pDecOutVirAddr, hDecOutBuff_fd);
    }
    if(hEncOutRefBuff_fd){
        freeSHMBuffer(pEncOutRefVirAddr, hEncOutRefBuff_fd);
    }
    if(cma_cust_dev_fd >= 0) {
        dmaheap_close(cma_cust_dev_fd);
    }

    if (g_bm_dev_fd >= 0) {
        close(g_bm_dev_fd);
        g_bm_dev_fd = -1;
    }

    if (m2m_enc_fd >= 0) {
        close(m2m_enc_fd);
        m2m_enc_fd = -1;
    }
    if (m2m_dec_fd >= 0) {
        close(m2m_dec_fd);
        m2m_dec_fd = -1;
    }
    return ret;
}

TEST(TestM2M, basicByKernel) {
    char* strmPath = (char *)"/M2M/";

    int ret = 0;
    uint32_t scheme;

    for(scheme=M2MCRYPTO_TYPE_AES_128_ECB;scheme<=M2MCRYPTO_TYPE_TDES_128_CTR;scheme++)
    {
        ret = test_m2m_basic_encdec_by_kernel(scheme, strmPath);
        ASSERT_EQ(0, ret);
    }

    printf("%s,%s: End!\n", __FILE__, __FUNCTION__);
    printf("End!\n");
}


TEST(TestM2M, basic) {
    char* strmPath = (char *)"/M2M/";

    int ret = 0;
    uint32_t scheme;

    for(scheme=M2MCRYPTO_TYPE_AES_128_ECB;scheme<=M2MCRYPTO_TYPE_TDES_128_CTR;scheme++)
    {
        ret = test_m2m_basic_encdec(scheme, strmPath);
        ASSERT_EQ(0, ret);
    }

    printf("%s,%s: End!\n", __FILE__, __FUNCTION__);
    printf("End!\n");
}


TEST(TestM2M, pattern) {

    int ret = 0;
    uint32_t uScheme = 0;
    uint8_t  i = 1;
    uint32_t uDataSize = 0;

    printf("\n------------ Test CTR PatternMode ----------------\n");
    uScheme = M2MCRYPTO_TYPE_AES_128_CTR;
    for(uDataSize=10; uDataSize<320;){
        for(i=1; i<10; i++){
            ret = test_m2m_patternMode(uScheme, i, 10-i, uDataSize);
            ASSERT_EQ(0, ret);
        }
        uDataSize +=80;
    }

    printf("------------ Test CBC PatternMode ----------------\n");
    uScheme = M2MCRYPTO_TYPE_AES_128_CBC;
    for(uDataSize=10; uDataSize<320;){
        for(i=1; i<10; i++){
            ret = test_m2m_patternMode(uScheme, i, 10-i, uDataSize);
            ASSERT_EQ(0, ret);
        }
        uDataSize +=80;
    }
}


TEST(TestM2M, patternByKernel) {

    int ret = 0;
    uint32_t uScheme = 0;
    uint8_t  i = 1;
    uint32_t uDataSize = 0;

    printf("\n------------ Test CTR PatternMode ----------------\n");
    uScheme = M2MCRYPTO_TYPE_AES_128_CTR;
    for(uDataSize=10; uDataSize<320;){
        for(i=1; i<10; i++){
            ret = test_m2m_patternMode_by_kernel(uScheme, i, 10-i, uDataSize);
            ASSERT_EQ(0, ret);
        }
        uDataSize +=80;
    }

    printf("------------ Test CBC PatternMode ----------------\n");
    uScheme = M2MCRYPTO_TYPE_AES_128_CBC;
    for(uDataSize=10; uDataSize<320;){
        for(i=1; i<10; i++){
            ret = test_m2m_patternMode_by_kernel(uScheme, i, 10-i, uDataSize);
            ASSERT_EQ(0, ret);
        }
        uDataSize +=80;
    }
}
