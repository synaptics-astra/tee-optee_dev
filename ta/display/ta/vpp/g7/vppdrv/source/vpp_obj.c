/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Synaptics Incorporated */

//-----------------------------------------------------------------------------
//! \file       vpp_obj.c
//! \brief      store/retrieve the VPP_OBJ to/from TZK globaldata
//! \author     Shanmugam Ramachandran
//-----------------------------------------------------------------------------

#include "vpp_obj.h"
#ifndef CONFIG_OPTEE
#include "tee_internal_globaldata_api.h"
#else
#include "vpp_optee_missing.h"
#endif

#define VPP_PERSISTENCE_OBJ_KEY     0x11223344
#define VPP_PERSISTENCE_SHM_KEY     0xAABBCCDD

#define VPP_PERSISTENCE_SHM_MEMID_KEY           0xAABBCC00
#define VPP_PERSISTENCE_SHM_OFF_VPP_KEY         0xAABBCC01
#define VPP_PERSISTENCE_SHM_OFF_MEM_KEY         0xAABBCC02

#define VPP_PERSISTENCE_OBJ_CNT_KEY     0xAABBCC10
#define VPP_PERSISTENCE_OBJ_MAP_CNT_KEY 0xAABBCC11

#ifdef VPP_ENABLE_MEMID_BY_VIRT_ADDR
//If virtual address is used, then map is must required for all instance
#define VPP_ENABLE_FORCE_MAP_FOR_VIRT_ADDR
#endif //VPP_ENABLE_MEMID_BY_VIRT_ADDR

static uint32_t inst_num;

static TEE_Result MV_VPP_SetGlobalData(uint32_t key, uint32_t value)
{
    TEE_Result res = TEE_SUCCESS;

    res = set_globaldata(key, value);
    if (res != TEE_SUCCESS) {
        VPPAPI_DBG_FIELD("Failed to store vpp_obj in Global data (res=%lx) : %08x = %08x", res, key, value);
        return (MV_VPP_ENOMEM);
    }

    return res;
}

static TEE_Result MV_VPP_GetGlobalData(uint32_t key, uint32_t *pValue)
{
    TEE_Result res = TEE_SUCCESS;

    res = get_globaldata(key, pValue);
    if (res != TEE_SUCCESS) {
        VPPAPI_DBG_FIELD("Failed to store shm_handle in Global data (res=%ld / %lx) : %08x = %p", res, res,
                key, *pValue);
        return (MV_VPP_ENOMEM);
    }

    return res;
}

static uint32_t VPP_TA_GetInstanceNumber(void) {
    return inst_num;
}

static uint32_t VPP_TA_GetInstanceCount(void) {
    uint32_t count;

    if (MV_VPP_GetGlobalData(VPP_PERSISTENCE_OBJ_CNT_KEY, &count) != TEE_SUCCESS) {
        count = 0;
    }
    return count;
}

static uint32_t VPP_TA_GetInstanceMapCount(void) {
    uint32_t count;

    if (MV_VPP_GetGlobalData(VPP_PERSISTENCE_OBJ_MAP_CNT_KEY, &count) != TEE_SUCCESS) {
        count = 0;
    }
    return count;
}

static void VPP_TA_SetMapInstanceCount(void) {
    uint32_t count;

    //Initialize the mapped instance count to current total instance count
    if (MV_VPP_GetGlobalData(VPP_PERSISTENCE_OBJ_CNT_KEY, &count) == TEE_SUCCESS) {
        //VPP_PERSISTENCE_OBJ_MAP_CNT_KEY created only if VPP_PERSISTENCE_OBJ_CNT_KEY exist
        MV_VPP_SetGlobalData(VPP_PERSISTENCE_OBJ_MAP_CNT_KEY, count);
        VPPOBJ_LOGI("MappedCount initialized to - %d \n", count);
    }
}

static void VPP_TA_IncMapInstanceCount(void) {
    uint32_t count;

    //Increment the mapped instance count
    if (MV_VPP_GetGlobalData(VPP_PERSISTENCE_OBJ_MAP_CNT_KEY, &count) == TEE_SUCCESS) {
        //VPP_PERSISTENCE_OBJ_MAP_CNT_KEY increament only if it already exist
        count++;
        MV_VPP_SetGlobalData(VPP_PERSISTENCE_OBJ_MAP_CNT_KEY, count);
        VPPOBJ_LOGI("MappedCount incremented : %d \n", count);
    }
}

static int MV_VPP_SetVppObjAddr(uint32_t mem_id, UINT32 virt_addr, UINT32 shmSize)
{
    TEE_Result res = TEE_SUCCESS;
    UINT32 offset, offset1;

#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
    //If continue to use physical address, then
    //Force change g_vpp & gMemhandle virtual address to physical address
    offset = ((uint32_t)g_vpp) - virt_addr;
    g_vpp = (void*) (gMemhandle->start_addr_phy + offset);
    offset1 = ((uint32_t)gMemhandle) - virt_addr;
    gMemhandle = (void*) (gMemhandle->start_addr_phy + offset1);
#endif //!VPP_ENABLE_MEMID_BY_VIRT_ADDR

    //If continue to use virtual address, then let g_vpp/gMemhandle are
    //overwritten by following offset calculation -- Ideally we can store/retrieve g_vpp/gMemhandle
    res = MV_VPP_SetGlobalData(VPP_PERSISTENCE_OBJ_KEY, (uint32_t)g_vpp);

#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    if (res == TEE_SUCCESS) {
        res = MV_VPP_SetGlobalData(VPP_PERSISTENCE_SHM_KEY, (uint32_t)gMemhandle);
    }

    //Store memid/phy-addr
    if (res == TEE_SUCCESS) {
        res = set_globaldata(VPP_PERSISTENCE_SHM_MEMID_KEY, mem_id);
    }

    //Store offset of g_vpp/gMemhandle
    if (res == TEE_SUCCESS) {
        offset = ((uint32_t)g_vpp) - virt_addr;
        res = set_globaldata(VPP_PERSISTENCE_SHM_OFF_VPP_KEY, offset);
    }
    if (res == TEE_SUCCESS) {
        offset1 = ((uint32_t)gMemhandle) - virt_addr;
        res = set_globaldata(VPP_PERSISTENCE_SHM_OFF_MEM_KEY, offset1);
    }
#endif //VPP_ENABLE_INTERNAL_MEM_MGR

    return res;
}

void VPP_TA_IncCreateInstanceCount(void) {
    uint32_t count;

    /*Increment total instance count
     *Initialize to 1 for 1st instance as Count=0 returned */
    count = VPP_TA_GetInstanceCount() + 1;

    ///Initialize/Assign instance number to total instance count
    inst_num = count;

    MV_VPP_SetGlobalData(VPP_PERSISTENCE_OBJ_CNT_KEY, count);
    VPPOBJ_LOGI("update Global data - key(%08x = %08x)\n", VPP_PERSISTENCE_OBJ_CNT_KEY, count);
}

int MV_VPP_GetVppObjAddr(void)
{
    TEE_Result res = TEE_SUCCESS;
    uint32_t keyValue;

    if (g_vpp) {
        //g_vpp is already initialized/mapped, then return immediatly
        return (MV_VPP_ESWSTATEWRONG);
    }

    //Initilaize and map g_vpp
    res = MV_VPP_GetGlobalData(VPP_PERSISTENCE_OBJ_KEY, &keyValue);
    if (res != TEE_SUCCESS) {
        VPPAPI_DBG_FIELD("Failed to retrieve vpp_obj from Global data (res=%lx) : %08x = %08x", res, VPP_PERSISTENCE_OBJ_KEY, keyValue);
        return (MV_VPP_ENOMEM);
    }
    g_vpp = (VPP *)keyValue;

#ifdef VPP_ENABLE_INTERNAL_MEM_MGR
    uint32_t mem_id;
    uint32_t force_memmap = 0;
    uint32_t instance_memmap = 0;

#ifdef VPP_ENABLE_FORCE_MAP_FOR_VIRT_ADDR
    force_memmap = 1;
    g_vpp = NULL;
#endif //VPP_ENABLE_FORCE_MAP_FOR_VIRT_ADDR
    instance_memmap = (VPP_TA_GetInstanceMapCount() < VPP_TA_GetInstanceNumber());

    //Initilaize and map gMemhandle
    if (res == TEE_SUCCESS) {
        res = MV_VPP_GetGlobalData(VPP_PERSISTENCE_SHM_KEY, &keyValue);
    }
    gMemhandle = (bitmap_mem *)keyValue;

    //Map only to the instances created after TEE_Map_Memory_ToAllInstances
    if(res == TEE_SUCCESS && (force_memmap || instance_memmap)) {
        //Retrieve memid/phy-addr
        res = MV_VPP_GetGlobalData(VPP_PERSISTENCE_SHM_MEMID_KEY, &mem_id);

        //map memid/phy-addr
        if (res == TEE_SUCCESS) {

            VPP_BUFF_HANDLE bufHandle;
            bufHandle.mem_id = mem_id;
            res = VPP_BUFF_Map(&bufHandle);
#ifdef VPP_ENABLE_MEMID_BY_VIRT_ADDR
            if (res == TEE_SUCCESS) {
                UINT32 offset, offset1;
                void *virt_addr;

                virt_addr = bufHandle.virt_addr;

                //Retrieve offset of g_vpp/gMemhandle
                if (res == TEE_SUCCESS) {
                    res = MV_VPP_GetGlobalData(VPP_PERSISTENCE_SHM_OFF_VPP_KEY, (uint32_t *)&offset);
                    g_vpp = (void*) (virt_addr+ offset);
                }

                if (res == TEE_SUCCESS) {
                    res = MV_VPP_GetGlobalData(VPP_PERSISTENCE_SHM_OFF_MEM_KEY, (uint32_t *)&offset1);
                    gMemhandle = (void*) (virt_addr + offset1);
                }
            }
#endif //VPP_ENABLE_MEMID_BY_VIRT_ADDR
        }
        if (instance_memmap) {
            //Increment the mapped instance count
            VPP_TA_IncMapInstanceCount();
        }
    }

#endif //VPP_ENABLE_INTERNAL_MEM_MGR

    return (MV_VPP_OK);
}

int MV_VPP_InitVppObj(VPP_INIT_PARM *vpp_init_parm)
{
    TEE_Result res = TEE_SUCCESS;
    UINT32 addr;

    if (g_vpp != NULL) {
        res = MV_VPP_EBADCALL;
    } else {
#if defined (VPP_ENABLE_INTERNAL_MEM_MGR)
        UINT32 shmVirtAddr;
        int shmSize;
        VPP_SHM_HANDLE shmHandle;
        bitmap_mem Memhandle;
        void *phy_addr;
        VPP_BUFF_HANDLE bufHandle;
        VPP_SHM_BUFFER_DESC bufDesc;

        //memid/phy-addr is passed in shmHandle
        shmHandle = vpp_init_parm->phShm;
        shmSize = VPP_SHM_ALIGN_ROUNDUP(vpp_init_parm->uiShmSize);

        VPP_INIT_SHM_BUFFER_DESC(bufDesc, (void*)shmHandle, shmSize, VPP_SHM_BUFFER_TYPE_INPUT);
        VPP_MapAndValidate_SingleBuffer(&bufHandle, &bufDesc);
        //Retrieve the value from VPP_BUFF_HANDLE
        shmVirtAddr = (UINT32)bufHandle.virt_addr;
        shmSize = bufHandle.size;
        //ensure shmHandle always stores/hold mem_id in all mechansim
        shmHandle = bufHandle.mem_id;
        phy_addr = bufHandle.phy_addr;

#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
        //If continue to use physical address, then
        //Force change pool base address also from virtual to physical
        shmVirtAddr = (UINT32)phy_addr;
#endif //VPP_ENABLE_MEMID_BY_VIRT_ADDR

        //Initialize internal memory manager with virutal/phy-address base-address
        VPP_OSAL_SHM_Init((UINT32)phy_addr, shmVirtAddr, shmSize, &Memhandle);

        //Allocate, Initialize gMemhandle
        gMemhandle = (bitmap_mem*)bm_malloc(&Memhandle, sizeof(bitmap_mem));
        VPP_Memcpy((void *)gMemhandle, (const void *) &Memhandle, sizeof(bitmap_mem));
#endif //VPP_ENABLE_INTERNAL_MEM_MGR

        /*Record the instance count, so that mapping can be avoided until this point
         * Duplicate/repeated mapping to be avoided*/
        VPP_TA_SetMapInstanceCount();

        //Allocated VPP_OBJ
        VPP_SHM32_MALLOC(VPP_SHM_SECURE, sizeof(VPP), &addr);
        g_vpp = (VPP *)addr;

        //Store in globaldata for sharing with other instances
        res = MV_VPP_SetVppObjAddr((UINT32)shmHandle, shmVirtAddr, shmSize);
        if(res != TEE_SUCCESS)
            return res;

        //Avoid dhub init for this instance from INITVPPS
        vpp_init_parm->g_vpp = VPP_CA_INIT_MAGIC_NUM;

#if defined (VPP_ENABLE_INTERNAL_MEM_MGR)
        vpp_init_parm->gMemhandle = (UINT32)gMemhandle;
#endif //VPP_ENABLE_INTERNAL_MEM_MGR
    }

    return res;
}

void MV_VPP_DeInitVppObj(void) {
    TEE_Result res = TEE_SUCCESS;
    uint32_t mem_id;

    //Retrieve memid/phy-addr
    res = MV_VPP_GetGlobalData(VPP_PERSISTENCE_SHM_MEMID_KEY, &mem_id);
    if (res != TEE_SUCCESS) {
        VPPAPI_DBG_FIELD("Failed to retrieve shm_memid from Global data (res=%ld / %lx) : %08x = %08lx\n",
                res, res, VPP_PERSISTENCE_SHM_MEMID_KEY, mem_id);
    } else {
#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
        char *func_name="_FromAllInstances";
#else
        char *func_name="";
#endif //VPP_ENABLE_MEMID_BY_VIRT_ADDR
        VPPAPI_DBG_INFO("TEE_Unregister_Memory/Unmap_Memory for mem-id : %08lx\n", mem_id);
#ifndef VPP_ENABLE_MEMID_BY_VIRT_ADDR
        res = TEE_Unmap_Memory_FromAllInstances(mem_id);
#else
        res = TEE_Unmap_Memory(mem_id);
#endif //VPP_ENABLE_MEMID_BY_VIRT_ADDR
        if (res != TEE_SUCCESS) {
            VPPAPI_DBG_FIELD("TEE_Unmap_Memory%s failed for mem-id : %08lx, (res=%ld / %lx)\n",
                func_name, mem_id, res, res);
        }

        res = TEE_Unregister_Memory(mem_id);
        if (res != TEE_SUCCESS) {
            VPPAPI_DBG_FIELD("TEE_Unregister_Memory failed for mem-id : %08lx, (res=%ld / %lx)\n",
                mem_id, res, res);
        }
    }
}

