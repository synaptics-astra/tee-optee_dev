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
//! \file m2m_ta_comm.h
//!
//! \brief This file is used to defnine some common data type for tsp crypto lib.
//!
//!
//!     Version    Date                Author
//!     V 1.00,    Aug 4 2015,        Long Xiao
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __M2M_TA_COMM_H__
#define __M2M_TA_COMM_H__

#include <stdlib.h>
#include "m2m_crypto_api.h"
#include "m2m_types.h"
#include "tsp_fw_spec.h"
#include "tsp_local_data.h"
#include "tsp_cmd.h"
#include "tsp_shared.h"

#ifndef TA_COMPILE

#include "dmx_util.h"
#include "dmx_dbg.h"
#include "std_list.h"
#include "drv_figo_api.h"
#include "tsp_drv_misc.h"
#include "m2m_shared.h"

typedef std_list            DMX_LIST;
typedef std_list_node       DMX_LIST_NODE;
#define DMX_LIST_CREATE     std_list_ctor
#define DMX_LIST_DESTROY    std_list_dtor
#define DMX_LIST_NEXT       std_list_iter
#define DMX_LIST_FIRST      std_list_first
#define DMX_LIST_FRONT      std_list_front
#define DMX_LIST_PUSH_FRONT std_list_push_front
#define DMX_LIST_PUSH_BACK  std_list_push_back
#define DMX_LIST_POP_FRONT  std_list_pop_front
#define DMX_LIST_SEARCH     std_list_search
#define DMX_LIST_REMOVE     std_list_remove
#define DMX_LIST_SIZE       std_list_size

typedef struct tagFigoSubsytemDriverContextData MV_FIGODRV_CTX;
typedef MV_FIGODRV_CTX* FIGODRV_HANDLE;

#define TA_LOGF(...)  AMPLOG(MODULE_DMX, AMP_LOG_FATAL, __VA_ARGS__)
#define TA_LOGE(...)  AMPLOG(MODULE_DMX, AMP_LOG_ERROR, __VA_ARGS__)
#define TA_LOGH(...)  AMPLOG(MODULE_DMX, AMP_LOG_HIGH, __VA_ARGS__)
#define TA_LOGM(...)  AMPLOG(MODULE_DMX, AMP_LOG_MID, __VA_ARGS__)
#define TA_LOGL(...)  AMPLOG(MODULE_DMX, AMP_LOG_LOW, __VA_ARGS__)
#define TA_LOGI(...)  AMPLOG(MODULE_DMX, AMP_LOG_INFO, __VA_ARGS__)
#define TEE_Result uint32_t

#define TA_EXPORT

typedef union TEE_Param
{
	struct
	{
		uint32_t	buffer;
		uint32_t	size;
	} memref;
	struct
	{
		uint32_t	a;
		uint32_t	b;
	} value;
} TEE_Param;

enum {
	TEE_SUCCESS				= 0x00000000,	/* TEEC_SUCCESS			*/
	TEE_ERROR_CORRUPT_OBJECT		= 0xF0100001,
	TEE_ERROR_CORRUPT_OBJECT_2		= 0xF0100002,
	TEE_ERROR_STORAGE_NOT_AVAILABLE		= 0xF0100003,
	TEE_ERROR_STORAGE_NOT_AVAILABLE_2	= 0xF0100004,
	TEE_ERROR_GENERIC			= 0xFFFF0000,	/* TEEC_ERROR_GENERIC		*/
	TEE_ERROR_ACCESS_DENIED			= 0xFFFF0001,	/* TEEC_ERROR_ACCESS_DENIED	*/
	TEE_ERROR_CANCEL			= 0xFFFF0002,	/* TEEC_ERROR_CANCEL		*/
	TEE_ERROR_ACCESS_CONFLICT		= 0xFFFF0003,	/* TEEC_ERROR_ACCESS_CONFLICT	*/
	TEE_ERROR_EXCESS_DATA			= 0xFFFF0004,	/* TEEC_ERROR_EXCESS_DATA	*/
	TEE_ERROR_BAD_FORMAT			= 0xFFFF0005,	/* TEEC_ERROR_BAD_FORMAT	*/
	TEE_ERROR_BAD_PARAMETERS		= 0xFFFF0006,	/* TEEC_ERROR_BAD_PARAMETERS	*/
	TEE_ERROR_BAD_STATE			= 0xFFFF0007,	/* TEEC_ERROR_BAD_STATE		*/
	TEE_ERROR_ITEM_NOT_FOUND		= 0xFFFF0008,	/* TEEC_ERROR_ITEM_NOT_FOUND	*/
	TEE_ERROR_NOT_IMPLEMENTED		= 0xFFFF0009,	/* TEEC_ERROR_NOT_IMPLEMENTED	*/
	TEE_ERROR_NOT_SUPPORTED			= 0xFFFF000A,	/* TEEC_ERROR_NOT_SUPPORTED	*/
	TEE_ERROR_NO_DATA			= 0xFFFF000B,	/* TEEC_ERROR_NO_DATA		*/
	TEE_ERROR_OUT_OF_MEMORY			= 0xFFFF000C,	/* TEEC_ERROR_OUT_OF_MEMORY	*/
	TEE_ERROR_BUSY				= 0xFFFF000D,	/* TEEC_ERROR_BUSY		*/
	TEE_ERROR_COMMUNICATION			= 0xFFFF000E,	/* TEEC_ERROR_COMMUNICATION	*/
	TEE_ERROR_SECURITY			= 0xFFFF000F,	/* TEEC_ERROR_SECURITY		*/
	TEE_ERROR_SHORT_BUFFER			= 0xFFFF0010,	/* TEEC_ERROR_SHORT_BUFFER	*/
	TEE_ERROR_EXTERNAL_CANCEL		= 0xFFFF0011,	/* TEEC_ERROR_EXTERNAL_CANCEL	*/
	TEE_ERROR_TIMEOUT			= 0xFFFF3001,
	TEE_ERROR_OVERFLOW			= 0xFFFF300F,
	TEE_ERROR_TARGET_DEAD			= 0xFFFF3024,	/* TEEC_ERROR_TARGET_DEAD	*/
	TEE_ERROR_STORAGE_NO_SPACE		= 0xFFFF3041,
	TEE_ERROR_MAC_INVALID			= 0xFFFF3071,
	TEE_ERROR_SIGNATURE_INVALID		= 0xFFFF3072,
	TEE_ERROR_TIME_NOT_SET			= 0xFFFF5000,
	TEE_ERROR_TIME_NEEDS_RESET		= 0xFFFF5001,
};

typedef struct __m2m_buff__ {
    UINT32 hShm;
    void *phyAddr;
    void *virAddr;
    uint32_t uBufSize;
}M2M_BUFF_T;

enum {
	TEE_PARAM_TYPE_NONE		= 0,
	TEE_PARAM_TYPE_VALUE_INPUT	= 1,
	TEE_PARAM_TYPE_VALUE_OUTPUT	= 2,
	TEE_PARAM_TYPE_VALUE_INOUT	= 3,
	TEE_PARAM_TYPE_MEMREF_INPUT	= 5,
	TEE_PARAM_TYPE_MEMREF_OUTPUT	= 6,
	TEE_PARAM_TYPE_MEMREF_INOUT	= 7,
};

#define TEE_PARAM_TYPES(t0, t1, t2, t3)	\
	((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))
#define TEE_PARAM_TYPE_GET(t, i)	(((t) >> (i*4)) & 0xF)

#else // TA_COMPILE

#define LOG_TAG         "[M2M_TA]"

#include "tsp_base_config.h"
#include "tee_internal_api.h"
#include "m2m_config_log.h"
#include "ta_log.h"

#include "m2m_crypto_types.h"
#include "mem_region_userdata.h"

#ifdef CONFIG_OPTEE
#include "tee_api_syna.h"
#endif


#if 0
/* Already defined in ctypes.h */
typedef    unsigned char       UNSG8;
typedef    signed char         SIGN8;
typedef    unsigned short      UNSG16;
typedef    signed short        SIGN16;
typedef    unsigned int        UNSG32;
typedef    signed int          SIGN32;
typedef    unsigned long long  UNSG64;
typedef    signed long long    SIGN64;
#endif

#define  FALSE 0
#define  TRUE  1

#ifndef TSP_FIGO_NR
#define TSP_FIGO_NR 2
#endif

// workaround: drm_types.h defines all primary types
// so we don't need to define again or there will be
// compilation warnings
// but it's better to put the common types into a common
// header file to avoid duplication
#include "drm_types.h"

#ifndef CONFIG_ENABLE_DRM
typedef int                     INT;
typedef int                     INT32;
typedef unsigned char           UINT8;
typedef unsigned short          UINT16;
typedef unsigned char           BOOL;
typedef unsigned int            UINT32;
typedef unsigned int            UINT;
typedef void*                   PVOID;
typedef void                    VOID;
typedef signed int              HRESULT;
#endif

#ifndef CONFIG_OPTEE
#include "tee_internal_common.h"
#endif
#include "std_list.h"
#include "m2m_shared.h"
#include "tsp_mem_map.h"

typedef std_list            DMX_LIST;
typedef std_list_node       DMX_LIST_NODE;
#define DMX_LIST_CREATE     std_list_ctor
#define DMX_LIST_DESTROY    std_list_dtor
#define DMX_LIST_NEXT       std_list_iter
#define DMX_LIST_FIRST      std_list_first
#define DMX_LIST_FRONT      std_list_front
#define DMX_LIST_PUSH_FRONT std_list_push_front
#define DMX_LIST_PUSH_BACK  std_list_push_back
#define DMX_LIST_POP_FRONT  std_list_pop_front
#define DMX_LIST_SEARCH     std_list_search
#define DMX_LIST_REMOVE     std_list_remove
#define DMX_LIST_SIZE       std_list_size

typedef enum __key_option__ {
    DMX_KEY_INVALID = 0,
    DMX_KEY_CONTENT = 1,
    DMX_KEY_TBLIDX  = 2,
    DMX_KEY_MAX
} DMX_KEY_OPTION;

typedef struct __dmx_key_ctx__ {
    UINT8   KeyType;   //odd or even for TS descramblers, or both if a stream do not change key
    UINT8   KeyOption;   //Key: clear key data or key table index
    UINT8   IVOption;   //IV: clear IV data or key table index
    UINT8   UpdateIVFlag;   //if update IV or not
    UINT32  KeyLength;      //key length in number of bytes,
    //only used when the key or IV is provided in clear key data form
    UINT8   KeyContent[32];    //only used when KeyOption is set to clear key data
    UINT8   IVData[16];     //only used when IVOption is set to clear IV data
    UINT32  IVKeyLen;       //only used when IVOption is set to clear IV data
    UINT16  KeyTblIndex;    //only used when KeyOption is set to key table index
    UINT16  IVTblIndex;     //only used when IVOption is set to key table index
    BOOL bKeyFromDrm;
    BOOL bIVKeyFromDrm;
    UINT32 HWKeyArea;
}DMX_KEY_CTX;

#define MV_OSAL_Mutex_Lock(lock)    TEE_MutexLock(lock)
#define MV_OSAL_Mutex_Unlock(lock)  TEE_MutexUnlock(lock)

#define MV_OSAL_Mutex_TryLock(lock)  TEE_MutexTryLock(lock)

static inline void* m2m_malloc(const char *func, UINT32 line, UINT32 usize) {
      void *base = 0;
      base = malloc(usize);
      TA_LOGH("[%s, %d]: %s base = %p, usize = 0x%x\n", func, line, __FUNCTION__, base, usize);
      return base;
}

static inline void m2m_free(const char *func, UINT32 line, void *base) {
      TA_LOGH("[%s, %d]: %s base = %p\n", func, line, __FUNCTION__, base);
      free(base);
}

#define dmx_malloc(usize)  m2m_malloc(__FUNCTION__, __LINE__,usize)
#define dmx_free(base)     m2m_free(__FUNCTION__, __LINE__, base)

#define dmx_memset   memset


typedef struct tagFigoSubsytemDriverContextData MV_FIGODRV_CTX;

typedef MV_FIGODRV_CTX* FIGODRV_HANDLE;


//! FIGO callback function pointer define
typedef SIGN32 (*SyscallHandler)(FIGODRV_HANDLE hFigoDrv,
                                 UNSG32 uFigoID,
                                 void *pDtcmPtr,
                                 UNSG32 param);
//! FIGO EventHandler function pointer define
typedef SIGN32 (*FigoEventHandler)(FIGODRV_HANDLE hFigoDrv,
                                   UNSG8* pCtrlDat,
                                   UNSG32 uCtrlSize,
                                   UNSG16 uDSFifoDTCMOffset,
                                   UNSG32 uWrOffset);
////////////////////////////////////////////////////////////////////////////////
//! \brief  Release FIGO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_IsFIGORunning)(FIGODRV_HANDLE hFigoDrv, UNSG32 uFigoId, UNSG32* puIsRunning);
////////////////////////////////////////////////////////////////////////////////
//! \brief  Release FIGO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_ReleaseFIGO)(FIGODRV_HANDLE hFigoDrv, UNSG32 uFigoId);
////////////////////////////////////////////////////////////////////////////////
//! \brief  Set the reset bit to 0 to stop the FIGO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_StopFIGO)(FIGODRV_HANDLE hFigoDrv, UNSG32 uFigoId);
////////////////////////////////////////////////////////////////////////////////
//! \brief  Set the reset bit to 1 to release the FIGO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_ResumeFIGO)(FIGODRV_HANDLE hFigoDrv, UNSG32 uFigoId);
////////////////////////////////////////////////////////////////////////////////
//! \brief  Configure data streamer
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_ConfigDS)(    FIGODRV_HANDLE hFigoDrv,
        UNSG32 uFigoId,
        UNSG32 uRetCmdAddr,
        UNSG32 uCmdDepth,
        UNSG32 uInitCmdID);
////////////////////////////////////////////////////////////////////////////////
//! \brief Specify the 28 bits base address of the data streamer command
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_SetDSBase)(    FIGODRV_HANDLE hFigoDrv,
        UNSG32 uFigoId,
        UNSG32 uBaseAddr);
////////////////////////////////////////////////////////////////////////////////
//! \brief  Configure the HBO FIFO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_HBOFifoConfig)(FIGODRV_HANDLE hFigoDrv,
        UNSG32 uHboId,
        UNSG32 uFifoId,
        UNSG32 uBIUBase,
        UNSG32 uDataCnt,
        UNSG32 uIsEnable);
////////////////////////////////////////////////////////////////////////////////
//! \brief  Clear the HBO FIFO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_HBOFifoClear)(FIGODRV_HANDLE hFigoDrv,
        UNSG32 uHboId,
        UNSG32 uFifoId);
////////////////////////////////////////////////////////////////////////////////
//! \brief   Push data into HBO FIFO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_HBOFifoPush)(FIGODRV_HANDLE hFigoDrv,
        UNSG32 uHboId,
        UNSG32 uFifoId,
        UNSG64* pbData,
        UNSG32 uDataCnt);
////////////////////////////////////////////////////////////////////////////////
//! Pop data into a given HBO FIFO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_HBOFifoPop)(FIGODRV_HANDLE hFigoDrv,
        UNSG32 uHboId,
        UNSG32 uFifoId,
        UNSG64* pbData,
        UNSG32 uDataCnt);
////////////////////////////////////////////////////////////////////////////////
//! Start a given HBO FIFO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_HBOFifoStart)(FIGODRV_HANDLE hFigoDrv, UNSG32 uHboId, UNSG32 uFifoId);
////////////////////////////////////////////////////////////////////////////////
//! Stop a given HBO FIFO
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_HBOFifoStop)(FIGODRV_HANDLE hFigoDrv, UNSG32 uHboId,  UNSG32 uFifoId);
////////////////////////////////////////////////////////////////////////////////
//! Retrieve the HBO FIFO counter
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_HBOFifoCnt)(FIGODRV_HANDLE hFigoDrv, UNSG32 uHboId, UNSG32 uFifoId);
////////////////////////////////////////////////////////////////////////////////
//! Retrieve the HBO FIFO empty slots
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_HBOFifoSpace)(FIGODRV_HANDLE hFigoDrv,
                                            UNSG32 uHboId,
                                            UNSG32 uFifoId,
                                            UNSG32 uMaxFifoCnt);
////////////////////////////////////////////////////////////////////////////////
//! \brief Genereate the data streamer message
////////////////////////////////////////////////////////////////////////////////
typedef SIGN32 (*FUNC_GenDSMsg)(FIGODRV_HANDLE hFigoDrv,
        UNSG32    uDDRAddr,
        UNSG16    uBIUAddr,
        UNSG16    uSize,
        UNSG16    uDsCmdId,
        UNSG8     uIsOut,
        UNSG8     uIsOutStanding,
        UNSG8     uIsBurst,
        UNSG64*   pDsMsg);

struct tagFigoSubsytemDriverContextData
{
    //! 32-bits FIGO subsystem register base address
    UNSG32            uSubSysRegBase;
    //! 32-bits FIGO subsystem register base address
    UNSG32            uSubSysRegSize;

    //! 16-bits BIU size
    UNSG32            uBIUSize;

    //! 16-bits size of DTCM
    UNSG32            uDTCMRegBase;
    //! 16-bits size of DTCM
    UNSG32            uDTCMRegSize;

    //! 16-bits ITCM start address
    UNSG32            uITCMRegBase;
    //! 16-bits size of ITCM
    UNSG32            uITCMRegSize;

    //!    uFigoSysStart
    UNSG32            uFigoSysStart;

    struct tagHBOConfigData
    {
        UNSG32        uIsEnabled;
        UNSG32        uFIFONum;
    } pHboCfgData[2];

    struct tagFigoConfigData
    {
        UNSG32        uIsEnabled;
        UNSG32        uDSCmdHBOID;
        UNSG32        uDSCmdFifoID;
        UNSG32        uLocalAreaAddrInDTCM;
    } pFigoCfgData;

    UNSG32            uCfgDataEnd;

    //! FIGO syscall handler
    //! Pointer to the BIU shadow image in DDR
    UNSG32*                    pbBIUImage;
    SyscallHandler             pSysCallBack;
    FigoEventHandler           pEvCallBack;
    FUNC_ReleaseFIGO           pReleaseFIGO;
    FUNC_IsFIGORunning         pIsFIGORunning;
    FUNC_StopFIGO              pStopFIGO;
    FUNC_ResumeFIGO            pResumeFIGO;
    FUNC_ConfigDS              pConfigDS;
    FUNC_SetDSBase             pSetDSBase;
    FUNC_HBOFifoConfig         pHBOFifoConfig;
    FUNC_HBOFifoClear          pHBOFifoClear;
    FUNC_HBOFifoPush           pHBOFifoPush;
    FUNC_HBOFifoPop            pHBOFifoPop;
    FUNC_HBOFifoStart          pHBOFifoStart;
    FUNC_HBOFifoStop           pHBOFifoStop;
    FUNC_HBOFifoCnt            pHBOFifoCnt;
    FUNC_HBOFifoSpace          pHBOFifoSpace;
    FUNC_GenDSMsg              pGenDSMsg;
    UNSG32                     pHboHdl[2][128];
    UNSG32                     uDrvVer;
    void*                      pVMemHandle;

    UNSG64                     uPerfCounter;
    UNSG32                     uPerfStartCounter;

    UNSG32                     uFigoSysId;
    UNSG32                     uFigoSysType;        //1: FIGOSYS_BASIC, 0: other wise
    UNSG32                     uEvtBufVirAddr;
    UNSG32                     uEvtBufPhyAddr;
    UNSG32*                    pbBIUImagePhy;
    TEE_Reservation*           pbBIUReservation;
};

#endif

#endif

