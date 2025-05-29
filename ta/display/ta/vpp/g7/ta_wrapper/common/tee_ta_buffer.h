/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       tee_ta_buffer.h
//! \brief      data structure and interfaces for buffer passed as parameter
//!                    from CA to TA.
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#ifndef __TEE_TA_BUFFER_H__
#define __TEE_TA_BUFFER_H__

#include "com_type.h"
#ifndef CONFIG_OPTEE
#include "tee_internal_private_api.h"
#else
#include "tee_api_syna.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VPPBUFF_LOGGER
#define VPPBUFF_LOGI(...)     VPPAPI_DBG_INFO(__VA_ARGS__)
#define VPPBUFF_LOGE(...)     VPPAPI_DBG_ERROR(__VA_ARGS__)
#else
#define VPPBUFF_LOGI(...)
#define VPPBUFF_LOGE(...)
#endif

#ifdef VPPBUFF_LOGGER_DETAILED
#define VPPBUFF_LOGL(...)     VPPAPI_DBG_INFO(__VA_ARGS__)
#else
#define VPPBUFF_LOGL(...)
#endif

#define VPPBUFF_LOGF(...)     VPPAPI_DBG_FIELD(__VA_ARGS__)

#ifdef VPP_ENABLE_TA_ISOLATION
/*Enable this macro to use virtual address to access data from TA otherwise,
 *physical address will be used, which is not allowed in TA isloation*/
#define VPP_ENABLE_MEMID_BY_VIRT_ADDR
//Enable this macro to force map/unamp virutal address for phy address usecases
//#define VPP_ENABLE_FORCE_MAP_UNMAP_FOR_PHY_ADDR
#endif //VPP_ENABLE_TA_ISOLATION

/*mapping support is required by default - Retrieve memid/phy-addr
 * expect for physical address passing scenario */
#define VPP_BUFF_MAP_UNMAP_ALLOWED()  TRUE

//By default mapped address is virutal address
#define VPP_BUFF_GET_MAPPED_ADDR(pBuffHandle)    (pBuffHandle)->virt_addr

#ifndef VPP_ENABLE_PASSSHM_BY_MEMID

#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
#ifdef VPP_ENABLE_FORCE_MAP_UNMAP_FOR_PHY_ADDR
//MAP/UNMAP not not needed if physical address is used to access buffer
#undef VPP_BUFF_MAP_UNMAP_ALLOWED
#define VPP_BUFF_MAP_UNMAP_ALLOWED() FALSE
#endif //VPP_ENABLE_FORCE_MAP_UNMAP_FOR_PHY_ADDR

//If continue to use physical address, then force use physical addr
#undef VPP_BUFF_GET_MAPPED_ADDR
#define VPP_BUFF_GET_MAPPED_ADDR(pBuffHandle)    (pBuffHandle)->phy_addr
#endif //VPP_ENABLE_MEMID_BY_VIRT_ADDR

#else //!VPP_ENABLE_PASSSHM_BY_MEMID

/*Enable this macro to force do MEMMAP for every instance.
 * - This is not needed unless TEE_Map_Memory_ToAllInstances is not working
 * - i.e Enable this macro, if TZK is not inheriting MMU table to the
 * - instances created after TEE_Map_Memory_ToAllInstances */
#define VPP_ENABLE_PASSSHM_BY_MEMID_FORCE_MEMID

/*Enable this macro to disable UnMap/UnRegister memid
 * This is required only for MEMID case
 */
//#define VPP_BUFF_ENABLE_UNMAP_UNREGISTER

#endif //VPP_ENABLE_PASSSHM_BY_MEMID

#define VPP_VBUF_INFO_SET_MEMID(VBUF_INFO, MEMID)       do { VBUF_INFO->m_surface[0] = (void*) MEMID; } while(0)
#define VPP_VBUF_INFO_GET_MEMID(VBUF_INFO)              ((UINT32)VBUF_INFO->m_surface[0])
#define VPP_VBUF_INFO_SET_CLUT_MEMID(VBUF_INFO, MEMID)  do { VBUF_INFO->m_surface[1] = (void*) MEMID; } while(0)
#define VPP_VBUF_INFO_GET_CLUT_MEMID(VBUF_INFO)         ((UINT32)VBUF_INFO->m_surface[1])

/* Structure of a buffer recieved from CA to TA
 *  - descriptor contains the mapping/register details of buffer sent from CA
 *  - CA can send the mem-id or phy_addr/size
 *  - TA can populate other member of structure from given CA details using
 *      VPP_BUFF_GetHandleXXXX()
 */
typedef struct VPP_BUFF_HANDLE_T {
    //mem_id for the buffer, allocated from PTM/SMM TA
    uint32_t mem_id;
    //Physical,Virtual address and size of the buffer associated with mem_id
    void *phy_addr;
    void *virt_addr;
    size_t size;
} VPP_BUFF_HANDLE;

/***************************************************************
 * FUNCTION: Populate other members of VPP_BUFF_HANDLE from mem_id
 *              Map the mem_id to retrieve virt_addr to access buffer
 *
 * IN PARAMS : pBufHandle->mem_id - mem_id associated with the buffer
 *           : bIsMapToAllInstances - map mem_id to this/all TA instance
 *           : bIsPhyAddrNeeded - fill/return phy_addr with physical address of
 *                 the buffer assocted with mem_id. This flag may not be needed
 *                 for parameter buffer used to transfer parameter b/w CA & TA
 * OUT PARAMS : pBufHandle->virt_addr - virtual address of buffer associated
 *                  with mem_id
 *            : pBufHandle->phy_addr - physical address of buffer associated
 *                  with mem_id
 *            : pBufHandle->size - size of buffer associated with mem_id
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_GetHandleByMemId(VPP_BUFF_HANDLE *pBufHandle,
                    BOOL bIsMapToAllInstances, BOOL bIsPhyAddrNeeded);

/***************************************************************
 * FUNCTION: Populate other members of VPP_BUFF_HANDLE from phy_addr & size
 *           Register & Map the phy_addr to retrieve virt_addr to access buffer
 *
 * IN PARAMS : pBufHandle->phy_addr - physical address of buffer associated
 *                  with mem_id
 *           : pBufHandle->size - size of buffer associated with mem_id
 *           : bIsMapToAllInstances - map phy_addr/size to this/all TA instance
 * OUT PARAMS : pBufHandle->mem_id - mem_id associated with the buffer
 *            : pBufHandle->virt_addr - virtual address of buffer associated
 *                  with mem_id
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_GetHandleByAddress(VPP_BUFF_HANDLE *pBufHandle,
                    BOOL bIsMapToAllInstances);

/***************************************************************
 * FUNCTION: Populate other members of VPP_BUFF_HANDLE from mem_id
 *           or phy_addr & size
 *           Register & Map the mem_id/phy_addr to retrieve virt_addr
 *           to access buffer
 *
 * IN PARAMS : pBufHandle->mem_id - mem_id associated with the buffer
 *              : pBufHandle->phy_addr/size  - then populate mem_id
 *           : bIsMapToAllInstances - map mem_id to this/all TA instance
 *           : bIsPhyAddrNeeded - fill/return phy_addr with physical address of
 *           the buffer assocted with mem_id
 * OUT PARAMS : depends on the input parameter as explained above and controlled
 *                  by compile time config macro VPP_ENABLE_PASSSHM_BY_MEMID
 *              If macro is defined, then CA->TA communication is mem_id
 *              otherwise, CA->TA communication is phy_addr & size
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_GetHandle(VPP_BUFF_HANDLE *pBufHandle, uint32_t addr_param,
                int addr_size, BOOL bIsMapToAllInstances, BOOL bIsPhyAddrNeeded);

/***************************************************************
 * FUNCTION:  unMap and unRegister VPP_BUFF_HANDLE/mem_id
 *
 * IN PARAMS : pBufHandle->mem_id - mem_id associated with the buffer
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_PutHandle(VPP_BUFF_HANDLE *pBufHandle);

/***************************************************************
 * FUNCTION: Map & Populate other members of VPP_BUFF_HANDLE from mem_id
 *
 * IN PARAMS : pBufHandle->mem_id - mem_id associated with the buffer
 * OUT PARAMS : pBufHandle->virt_addr - virtual address of buffer associated
 *                  with mem_id
 *            : pBufHandle->phy_addr - physical address of buffer associated
 *                  with mem_id
 *            : pBufHandle->size - size of buffer associated with mem_id
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_Map(VPP_BUFF_HANDLE *pBufHandle);

/***************************************************************
 * FUNCTION: UnMap buffer associated with mem_id member of VPP_BUFF_HANDLE
 *
 * IN PARAMS : pBufHandle->mem_id - mem_id associated with the buffer
 * OUT PARAMS : None
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_UnMap(VPP_BUFF_HANDLE *pBufHandle);

/***************************************************************
 * FUNCTION: Register & Populate other members of VPP_BUFF_HANDLE
 *                from phy_addr & size
 *
 * IN PARAMS : pBufHandle->phy_addr - physical address of buffer
 *              : pBufHandle->size - size of the buffer
 * OUT PARAMS : pBufHandle->mem_id - mem-id associated with buffer
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_Register(VPP_BUFF_HANDLE *pBufHandle);

/***************************************************************
 * FUNCTION: UnRegister buffer associated with mem_id member of VPP_BUFF_HANDLE
 *
 * IN PARAMS : pBufHandle->mem_id - mem_id associated with the buffer
 * OUT PARAMS : None
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_UnRegister(VPP_BUFF_HANDLE *pBufHandle);

/***************************************************************
 * FUNCTION: get physical addr associated with mem_id member of VPP_BUFF_HANDLE
 *
 * IN PARAMS : pBufHandle->mem_id - mem_id associated with the buffer
 * OUT PARAMS : pBufHandle->phy_addr - physical address of buffer
 * RETURN:  TEE_SUCESS - succeed
 *          -ve/+ve value - failed, details in error code returned
 ****************************************************************/
TEE_Result VPP_BUFF_GetPhyAddr(VPP_BUFF_HANDLE *pBufHandle);
#ifdef __cplusplus
}
#endif

#endif

