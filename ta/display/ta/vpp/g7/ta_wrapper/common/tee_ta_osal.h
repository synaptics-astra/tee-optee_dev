/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */
#ifndef _TEST_OSAL_TA_H_
#define _TEST_OSAL_TA_H_

#include "com_type.h"
#ifndef CONFIG_OPTEE
#include "tee_internal_private_api.h"
#else
#include "tee_api_syna.h"
#endif
#if defined(CONFIG_VPP_FASTLOGO_SUPPORT)
#include "fastlogo_defs.h"
#include "vpp_api_fastlogo.h"
#endif //CONFIG_VPP_FASTLOGO_SUPPORT
#include "tee_ta_buffer_check.h"
#include "tee_ta_buffer.h"
#include "tee_ta_mmap.h"

typedef void *MV_OSAL_HANDLE;
typedef MV_OSAL_HANDLE       MV_OSAL_HANDLE_SEM_t;
typedef MV_OSAL_HANDLE       *pMV_OSAL_HANDLE_SEM_t;
typedef MV_OSAL_HANDLE  MV_HANDLE_Timer_t;
typedef MV_OSAL_HANDLE  *pMV_HANDLE_Timer_t;
typedef MV_OSAL_HANDLE  MV_OSAL_HANDLE_MUTEX_t;
typedef MV_OSAL_HANDLE  *pMV_OSAL_HANDLE_MUTEX_t;
typedef MV_OSAL_HANDLE 	MV_OSAL_HANDLE_TASK_t;

typedef UINT32                  AMP_SHM_HANDLE;


typedef struct {
    UINT32 m_MsgID;
    UINT32 m_Param1;
    UINT32 m_Param2;
} MV_CC_MSG_t, *pMV_CC_MSG_t;

#define MV_OSAL_Task_Exit(x)
   //The devmem_virt_to_phy macro for various OSAL implemenation
#if defined(VPP_DIAGS)
    #define devmem_phy_to_virt(pPhyAddr)    pPhyAddr
#elif defined(CONFIG_AMP_TEE_ENABLE)
#ifndef CONFIG_OPTEE
    #define devmem_phy_to_virt(pPhyAddr)    TEE_PhysToVirt((void *)pPhyAddr)
#else
    #define devmem_phy_to_virt(pPhyAddr)    TEE_TA_RegPhyToVirt(pPhyAddr)
#endif
#elif defined( __LINUX_KERNEL__ )
    #define devmem_phy_to_virt(pPhyAddr)    virt_to_phys(pPhyAddr)
#else
    //devmem_virt_to_phy: Will be implemented in galois_dev_mem_map.c
    //pVirAddr = devmem_virt_to_phy(pPhyAddr);
#endif


//The devmem_virt_to_phy macro for various OSAL implemenation
#if defined(VPP_DIAGS)
    #define devmem_virt_to_phy(pVirAddr)        pVirAddr
#elif defined(CONFIG_AMP_TEE_ENABLE)
    #define devmem_virt_to_phy(pVirAddr)        TEE_VirtToPhys((void *)pVirAddr)
#elif defined( __LINUX_KERNEL__ )
    #define devmem_virt_to_phy(pVirAddr)        virt_to_phys(pVirAddr)
#else
    //devmem_virt_to_phy: Will be implemented in galois_dev_mem_map.c
    //pPhyAddr = devmem_virt_to_phy(pVirAddr);
#endif


#if defined(CONFIG_VPP_FASTLOGO_SUPPORT)
#define TEE_Wait MV_OSAL_TEE_Wait
#else //CONFIG_VPP_FASTLOGO_SUPPORT
TEE_Result MV_OSAL_Handler(
        void*           sessionContext,
        uint32_t        commandID,
        uint32_t        paramTypes,
        TEE_Param       params[4]);
#endif //CONFIG_VPP_FASTLOGO_SUPPORT

#endif /* _VPP_TA_H_ */
