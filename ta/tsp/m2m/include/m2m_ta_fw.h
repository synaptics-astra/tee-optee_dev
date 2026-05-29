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
//! \file m2m_ta_fw.h
//!
//! \brief This file is used to defnine some common data type for tsp crypto FW.
//!
//!
//!     Version    Date                     Author
//!     V 1.00,    Aug 24 2015,             Long Xiao
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////
#ifndef __M2M_TA_FW_H__
#define __M2M_TA_FW_H__

#include "m2m_ta_comm.h"

/*It is for debug...*/
//#define TSP_FW_CRYPTO_STUB

/* M2M TA HAL Related code */

#ifdef TA_COMPILE

typedef enum __tsp_resource_type_max__ {
    Connector_type_DSStreamIn,
    Connector_type_DSStreamOut,
    Connector_type_DSSpecialOut,
    Connector_type_DTCMStream,
    Connector_type_HboIn,
    Connector_type_Max              = Connector_type_HboIn,

    Filter_type_Min,
    Filter_type_188TS_DMX           = Filter_type_Min,
    Filter_type_WM,
    Filter_type_Max                 = Filter_type_WM,

    PidFilterCtx_type,
    TSRecCtx_type,
    PesFiterCtx_type,
    SecFilterCtx_type,
    VesHdrCtx_type,
    Crypto_type_Task,
    TSP_ResourceType_MAX
}TSP_ResourceType_t;

//! Align a 32 bits integer to 8 bytes aligned value
#define ALIGN8(_Size)						((((UNSG32)(_Size)) + 0x07)  & (~0x07))

//! Align a 32 bits integer to 32 bytes aligned value
#define ALIGN32(_Size)						((((UNSG32)(_Size)) + 0x1f) & (~0x1f))

//! Align a 16 bits integer to 32 bytes aligned value
#define ALIGN16(_Size)						((((UNSG32)(_Size)) + 0xf) & (~0xf))

//! Check whether a address or size is aligned to 32 bytes
#define IS_ALIGNED32(_size)					((((uintptr_t)(_size)) & 0x1f) == 0)

//! Check whether a address or size is aligned to 8 bytes
#define IS_ALIGNED8(_size)					((((uintptr_t)(_size)) & 0x07) == 0)

//! Check whether a address or size is aligned to 8 bytes
#define IS_ALIGNED16(_size)					((((uintptr_t)(_size)) & 0x0F) == 0)

//! Check whether a address or size is aligned to 4 bytes
#define IS_ALIGNED4(_size)					((((uintptr_t)(_size)) & 0x03) == 0)


//#define MV_FIGODRV_ASSERT(x)					    assert(x)
#define MV_FIGODRV_ASSERT(x)

//! Validate the figo driver handle
#define MV_FIGODRV_IS_VALID_HANDLE(_hFigoDrv)			((_hFigoDrv) != NULL)

#define MV_FIGODRV_IO_WR32(_a,_d)                  	\
{													\
	(*((volatile UNSG32 *)tspmem_phy_to_virt(_a)) = (UNSG32)(_d));	\
}
#define MV_FIGODRV_IO_RD32(_a)                ((*((volatile UNSG32 *)tspmem_phy_to_virt(_a))))

//! Update the value at the given address in the DDR shadow image with the corresponding one in the DTCM
#define MV_FIGOLIB_MAP2BIUADDR(_hFigoDrv, _uPtr) (((_uPtr) != NULL) ? ((uintptr_t)(_uPtr) - (uintptr_t)(_hFigoDrv)->pbBIUImage) : FIGOSYSLIB_INVALID_BIU_PTR)

//! Update the value at the given address in the DDR shadow image with the corresponding one in the DTCM
#define MV_FIGOLIB_MAP2DRAMPTR(_hFigoDrv, _uAddr) (((_uAddr) != FIGOSYSLIB_INVALID_BIU_PTR) ? ((uintptr_t)((_hFigoDrv)->pbBIUImage) + (uintptr_t)(_uAddr)) : 0)

//! Macro to get the available bytes for pop operations
#define MV_FIGOLIB_GET_FIFO_BYTES(_pFifo)															\
	(int)(((_pFifo)->u_nMaxSize > 0) ?																\
	(((_pFifo)->u_nWrOff + (_pFifo)->u_nMaxSize - (_pFifo)->u_nRdOff) % (_pFifo)->u_nMaxSize) :	\
	0)

//! Get read/write pointer of the FIFO
#define MV_FIGOLIB_GET_FIFO_PTR(_pFifo, _inc)									\
	(UNSG8*)((uintptr_t)((_pFifo)->u_pbBase) + ((_inc) % (_pFifo)->u_nMaxSize))

//! Get read pointer of the FIFO
#define MV_FIGOLIB_GET_FIFO_RDPTR(_pFifo)		MV_FIGOLIB_GET_FIFO_PTR(_pFifo, (_pFifo)->u_nRdOff)

//! Get read pointer of the FIFO
#define MV_FIGOLIB_GET_FIFO_WRPTR(_pFifo)		MV_FIGOLIB_GET_FIFO_PTR(_pFifo, (_pFifo)->u_nWrOff)

//! Macro to get the available spaces in bytes for FIFO PUSH operations
#define MV_FIGOLIB_GET_FIFO_SPACE(_pFifo)										\
	(((_pFifo)->u_nMaxSize > 0) ?												\
	(((_pFifo)->u_nMaxSize - 1) - MV_FIGOLIB_GET_FIFO_BYTES(_pFifo)) :			\
	0)

//! Macro to advance the read pointer by the specific offset
#define MV_FIGOLIB_FIFO_POP(_pFifo, _size)										\
{																				\
	int nNewRdOff = ((_pFifo)->u_nRdOff + (_size)) % (_pFifo)->u_nMaxSize;		\
	(_pFifo)->u_nRdOff = nNewRdOff;												\
}

//! Macro to advance the write pointer by the specific offset
#define MV_FIGOLIB_FIFO_PUSH(_pFifo, _size)										\
{																				\
	int nNewWrOff = ((_pFifo)->u_nWrOff + (_size)) % (_pFifo)->u_nMaxSize;		\
	(_pFifo)->u_nWrOff = nNewWrOff;												\
}


#define MV_FIGOLIB_GET_LAPTR(_hFigoDrv)     \
     ((SIE_LocalArea_Ext*)((uintptr_t)((_hFigoDrv)->pbBIUImage) + (_hFigoDrv)->uDTCMRegBase + (_hFigoDrv)->pFigoCfgData.uLocalAreaAddrInDTCM))

//! Write an unsigned 32 bits value into the BIU address
#define MV_FIGODRV_BIUWR32(_hFigoDrv, _pHboAddr, _uVal)												\
{																						\
	MV_FIGODRV_IO_WR32((uintptr_t)(_pHboAddr) + ((_hFigoDrv)->uSubSysRegBase), (_uVal));					\
}

//! Read an unsigned 32 bits value from the BIU address
#define MV_FIGODRV_BIURD32(_hFigoDrv, _pHboAddr, _uVal)										\
{																							\
	(_uVal) = MV_FIGODRV_IO_RD32((UNSG32)(_pHboAddr) + (_hFigoDrv)->uSubSysRegBase);		\
}

//! Read an unsigned 32 bits value from the DTCM address
#define MV_FIGODRV_DTCMRD32(_hFigoDrv, _pDtcmAddr, _uVal)														\
{																												\
	(_uVal) = MV_FIGODRV_IO_RD32((UNSG32)(_pDtcmAddr) + (_hFigoDrv)->uSubSysRegBase + (_hFigoDrv)->uDTCMRegBase);\
}

//! Retrieve the pointer of the BIU shadow virtual image in DDR
#define MV_FIGODRV_GET_DTCM_SHADOW_PTR(_hFigoDrv)	(((UNSG8*)(_hFigoDrv)->pbBIUImage) + (_hFigoDrv)->uDTCMRegBase)
//! Update the value at the given address in the DDR shadow image with the corresponding one in the DTCM


#define MV_FIGODRV_IS_VALID_BIUPTR(_hFigoDrv, _pbAddr, _size)							                        \
	(((uintptr_t)(_pbAddr) >= (uintptr_t)(_hFigoDrv)->pbBIUImage) &&											    \
	((uintptr_t)(_pbAddr) + (_size) <= ((uintptr_t)(_hFigoDrv)->pbBIUImage + (_hFigoDrv)->uBIUSize)) &&		\
	((_size) != 0) && (IS_ALIGNED4(_size)) && (IS_ALIGNED4(_pbAddr)))											\

//! Update the value at the given address in the DTCM with the corresponding one in the DDR shadow image
#define MV_FIGODRV_DDR2BIU32(_hFigoDrv, _uPtr)                  										\
{																									\
	MV_FIGODRV_ASSERT(MV_FIGODRV_IS_VALID_BIUPTR(_hFigoDrv, _uPtr, sizeof(uintptr_t)));						\
	MV_FIGODRV_BIUWR32(_hFigoDrv, (uintptr_t)_uPtr - (uintptr_t)(_hFigoDrv)->pbBIUImage, *(UNSG32*)(uintptr_t)(_uPtr));	\
}

//! Update the value at the given address in the DDR shadow image with the corresponding one in the DTCM
#define MV_FIGODRV_BIU2DDR32(_hFigoDrv, _uPtr)                  										\
{																									\
	MV_FIGODRV_ASSERT(MV_FIGODRV_IS_VALID_BIUPTR(_hFigoDrv, _uPtr, sizeof(uintptr_t)));						\
	MV_FIGODRV_BIURD32(_hFigoDrv, (uintptr_t)(_uPtr) - (uintptr_t)(_hFigoDrv)->pbBIUImage, *(UNSG32*)(uintptr_t)(_uPtr));			\
}

#else
#include "figosys_lib.h"
#include "drv_figo_api.h"
#include "tsp_drv_cnn.h"
#endif


#ifdef TSP_FW_CRYPTO_STUB
#define TSPFW_CRYPTO_CHECKQSPACE    TSPFW_STUB_Crypto_CheckQSpace
#define TSPFW_CRYPTO_GETQCMDNUM     TSPFW_STUB_Crypto_GetQCmdNum
#define TSPFW_CRYPTO_SENDCMD        TSPFW_STUB_Crypto_SendCmd
#define TSPFW_CRYPTO_GETCMD         TSPFW_STUB_Crypto_GetCmd
#define TSPFW_CRYPTO_INIT           TSPFW_STUB_Crypto_Init
#define TSPFW_CRYPTO_DEINIT         TSPFW_STUB_Crypto_Deinit
#else
#define TSPFW_CRYPTO_CHECKQSPACE    TSPFW_Crypto_CheckQSpace
#define TSPFW_CRYPTO_GETQCMDNUM     TSPFW_Crypto_GetQCmdNum
#define TSPFW_CRYPTO_SENDCMD        TSPFW_Crypto_SendCmd
#define TSPFW_CRYPTO_GETCMD         TSPFW_Crypto_GetCmd
#define TSPFW_CRYPTO_INIT           TSPFW_Crypto_Init
#define TSPFW_CRYPTO_DEINIT         TSPFW_Crypto_Deinit
#endif

#define MV_FIGOC_ALIGN_PTR( _nPtr, _nOff )  { (_nPtr) += ((((_nOff) & 7) + 8 - ((_nPtr) & 7) ) & 7);}
#define ADDR_ALIGN(uiAddr, uiAlign)     (uiAddr + uiAlign - 1) & ~(uiAlign - 1);

HRESULT M2M_TA_Dtcm_Mem_Alloc(UINT16 mem_size, UINT8 align_size, UINT8 figoid, DTCM_MEM_TYPE type);
HRESULT M2M_TA_Dtcm_Mem_Free(UINT16 mem_start, UINT8 figoid, DTCM_MEM_TYPE type);
INT M2M_TA_AllocTask(FIGODRV_HANDLE  hFigoDrv, UINT16 task_addr);
void M2M_TA_FreeTask(FIGODRV_HANDLE  hFigoDrv, INT task_id);
SIGN32 MV_FIGODRV_DDR2BIU(FIGODRV_HANDLE hFigoDrv, UNSG8* pbSrc, UNSG32 uSize);
SIGN32 MV_FIGODRV_BIU2DDR(FIGODRV_HANDLE hFigoDrv, UNSG8* pbSrc, UNSG32 uSize);
HRESULT M2M_TA_LoadDataToDTCM(FIGODRV_HANDLE  hFigoDrv, UINT16 pDtcm,
            UINT8 *pData, UINT32 uLen);
UINT32 TSPFW_Crypto_CheckQSpace(FIGODRV_HANDLE  hFigoDrv);
UINT32 TSPFW_Crypto_GetQCmdNum(FIGODRV_HANDLE  hFigoDrv);
UINT32 TSPFW_Crypto_GetCmdQRdOffset(FIGODRV_HANDLE  hFigoDrv);
HRESULT TSPFW_Crypto_SetCmdQWrOffset(FIGODRV_HANDLE  hFigoDrv, UINT32 uOffSet);
HRESULT TSPFW_Crypto_SendCmd(FIGODRV_HANDLE  hFigoDrv,
                SIE_M2MCryptoCmd *pCryptoCmd);
SIE_M2MCryptoCmd *TSPFW_Crypto_GetCmd(FIGODRV_HANDLE  hFigoDrv, UINT32 Index);
HRESULT TSPFW_Crypto_CheckCmdErrState(FIGODRV_HANDLE hFigoDrv);
HRESULT TSPFW_Crypto_Init(TSP_CRYPTO_LIB *pCryptoLib, int32_t uFigoId, UINT16 uCmdQueueBase, UINT16  uCmdQueueSize,
                           UINT16 uCmdCntBase, UINT16 uLocalDataIn, UINT16 uLocalDataOut, UINT16 uM2MCryptoCmdBase);
HRESULT TSPFW_Crypto_Deinit(TSP_CRYPTO_LIB *pCryptoLib, int32_t uFigoId);
HRESULT TSPFW_Crypto_AllocFigoTask(TSP_CRYPTO_LIB *pCryptoLib, int32_t uFigoId);
HRESULT TSPFW_Crypto_FreeFigoTask(TSP_CRYPTO_LIB *pCryptoLib, int32_t uFigoId);
HRESULT TSPFW_Crypto_StartFW(int32_t *pFigoId);
HRESULT TSPFW_Crypto_StopFW(int32_t uFigoId);

#ifdef TA_COMPILE
TEE_Result Drm_KeyMgr_TA_session_open(void);
void Drm_KeyMgr_TA_session_close(void);

TEE_Result Drm_SE_GetContentKeyInfo(M2M_DRM_CRYPTO_ST   *pCryptInfo, DMX_KEY_CTX *pKeyContext);
TEE_Result Drm_SE_KeyMgr_GetContentKeyInfo(M2M_DRM_CRYPTO_ST   *pCryptInfo, DMX_KEY_CTX *pKeyContext);
TEE_Result Drm_SE_GetKeyRight(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,
                                        uint32_t* pbProtected, uint32_t* puKeyUsage);
TEE_Result Drm_SE_KtMgr_GetKeyRight(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,
                                                uint32_t* pbProtected, uint32_t* puKeyUsage);
TEE_Result Drm_SE_Validate_KeyUser(UINT32 HWKeyArea, UINT32 f_TSPKeyIndexOffset,
                                   TEE_UUID  *pUUID,  uint32_t* bValidKeyUser);
TEE_Result Drm_SE_KtMgr_Validate_KeyUser(UINT32 HWKeyArea, UINT32 f_keyIndexOffset,
                                         TEE_UUID  *pUUID,  uint32_t* bValidKeyUser);
#endif

#endif

