/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Synaptics Incorporated */

#include "ovp_module.h"
#ifdef CONFIG_OPTEE
#include "ovp_optee_missing.h"
#endif
#include "ovp_common.h"
#include "tee_ta_buffer.h"

#define	MAX_VPP_BUFF_HANDLE_REG		64
VPP_BUFF_HANDLE registered_vpp_buff_handle[MAX_VPP_BUFF_HANDLE_REG] = {0};

static bool VPP_BUFF_Check_Already_Registered(VPP_BUFF_HANDLE *pBufHandle)
{
	int count;

	for(count = 0; count < MAX_VPP_BUFF_HANDLE_REG; count++) {
		if(registered_vpp_buff_handle[count].phy_addr == pBufHandle->phy_addr) {
			memcpy(pBufHandle, &registered_vpp_buff_handle[count], sizeof(VPP_BUFF_HANDLE));
			return true;
		}
	}

	return false;
}

static bool VPP_BUFF_Add_Mapping(VPP_BUFF_HANDLE *pBufHandle)
{
	int count;

	for(count = 0; count < MAX_VPP_BUFF_HANDLE_REG; count++) {
		if(registered_vpp_buff_handle[count].phy_addr == 0) {
			memcpy(&registered_vpp_buff_handle[count], pBufHandle, sizeof(VPP_BUFF_HANDLE));
			return true;
		}
	}

	OVP_DBG_INFO("%s: No empty slots to add mapping for phy_addr: 0x%x\n", __func__, pBufHandle->phy_addr);
	return false;
}

static TEE_Result MV_VPP_MapMemID(UINT32 mem_id, void *pMem, size_t *pSize)
{
	TEE_Result res;

	res = TEE_Map_Memory(mem_id, pMem, pSize);
	if (TEE_SUCCESS != res)
		OVP_DBG_INFO("Failed to map mem-id(%x) : res = %lx!\n", mem_id, res);

	return res;
}

static TEE_Result MV_VPP_UnMapMemID(UINT32 mem_id)
{
	TEE_Result res;

	res = TEE_Unmap_Memory(mem_id);
	if (TEE_SUCCESS != res)
		OVP_DBG_INFO("Failed to Unmap mem-id(%x) : res = %lx!\n", mem_id, res);

	return res;
}

static TEE_Result VPP_BUFF_MapGen(VPP_BUFF_HANDLE *pBufHandle, BOOL bIsMapToAllInstances) {
	TEE_Result res;

	if (bIsMapToAllInstances)
		res = TEE_Map_Memory_ToAllInstances(pBufHandle->mem_id,
				&pBufHandle->virt_addr, &pBufHandle->size);
	else
		res = TEE_Map_Memory(pBufHandle->mem_id, &pBufHandle->virt_addr, &pBufHandle->size);

	return res;
}

TEE_Result VPP_BUFF_GetHandleByAddress(VPP_BUFF_HANDLE *pBufHandle, BOOL bIsMapToAllInstances) {
	TEE_Result res;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Check if this buf phys address already registered & mapped */
	if(VPP_BUFF_Check_Already_Registered(pBufHandle))
		return TEE_SUCCESS;

	/* Assume phy_addr & size members are already initialized */
	res = VPP_BUFF_Register(pBufHandle);

	if (TEE_SUCCESS == res) {
		res = VPP_BUFF_MapGen(pBufHandle, bIsMapToAllInstances);
		/* Add the new registered mapping to ensure we dont re-map same address */
		VPP_BUFF_Add_Mapping(pBufHandle);
	}

	return res;
}

TEE_Result VPP_BUFF_GetHandleByMemId(VPP_BUFF_HANDLE *pBufHandle,
		BOOL bIsMapToAllInstances, BOOL bIsPhyAddrNeeded) {
	TEE_Result res;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Assume mem_id members is already initialized */
	/* Initialise phy_addr to null */
	pBufHandle->phy_addr = NULL;

	res = VPP_BUFF_MapGen(pBufHandle, bIsMapToAllInstances);

	if (res != TEE_SUCCESS)
		OVP_DBG_INFO("Failed to retrieve virt_addr from memid (res=%lx) : memid=%lx, %x = %p\n",
				res, pBufHandle->mem_id, pBufHandle->size, pBufHandle->virt_addr);
	else if (bIsPhyAddrNeeded)
		res = VPP_BUFF_GetPhyAddr(pBufHandle);

	return res;
}

TEE_Result VPP_BUFF_GetHandle(VPP_BUFF_HANDLE *pBufHandle, uint32_t addr_param,
		int addr_size, BOOL bIsMapToAllInstances, BOOL bIsPhyAddrNeeded) {
	TEE_Result res;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

#ifdef VPP_ENABLE_PASSSHM_BY_MEMID
	/* memid is passed in addr_param */
	pBufHandle->mem_id = addr_param;
	/*Initialize mem_id, and expect mem-id would be auto-filled
	 *    after mapping the mem-id, if required also retreive phy_addr */
	res = VPP_BUFF_GetHandleByMemId(pBufHandle, bIsMapToAllInstances, bIsPhyAddrNeeded);
#else //VPP_ENABLE_PASSSHM_BY_MEMID
	//phy-addr is passed in addr_param
	pBufHandle->phy_addr = (void*)OVP_SHM_ALIGN_ROUNDUP((UINT32)addr_param);
	pBufHandle->size = addr_size;
	/*Initialize phy_addr, size and expect mem-id would be auto-filled
	 *    after register & mapping the address */
	res = VPP_BUFF_GetHandleByAddress(pBufHandle, bIsMapToAllInstances);
#endif /* VPP_ENABLE_PASSSHM_BY_MEMID */

	return res;
}

TEE_Result VPP_BUFF_PutHandle(VPP_BUFF_HANDLE *pBufHandle) {
	TEE_Result res = TEE_SUCCESS;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

#ifdef VPP_BUFF_ENABLE_UNMAP_UNREGISTER
	res = VPP_BUFF_UnMap(pBufHandle);
	if (TEE_SUCCESS == res)
		res = VPP_BUFF_UnRegister(pBufHandle);
#endif /* VPP_BUFF_ENABLE_UNMAP_UNREGISTER */

	return res;
}

TEE_Result VPP_BUFF_Map(VPP_BUFF_HANDLE *pBufHandle) {
	TEE_Result res = TEE_SUCCESS;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

	if (VPP_BUFF_MAP_UNMAP_ALLOWED())
		/* Error is logged in MV_VPP_MapMemID(), so logging not required here */
		res = MV_VPP_MapMemID(pBufHandle->mem_id, (void*)&pBufHandle->virt_addr, &pBufHandle->size);

	return res;
}

TEE_Result VPP_BUFF_UnMap(VPP_BUFF_HANDLE *pBufHandle) {
	TEE_Result res = TEE_SUCCESS;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

	if (VPP_BUFF_MAP_UNMAP_ALLOWED()) {
		/* Error is logged in MV_VPP_UnMapMemID(), so logging not required here */
		res = MV_VPP_UnMapMemID(pBufHandle->mem_id);
	}

	return res;
}

TEE_Result VPP_BUFF_UnRegister(VPP_BUFF_HANDLE *pBufHandle) {
	TEE_Result res;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

	res = TEE_Unregister_Memory(pBufHandle->mem_id);
	if (TEE_SUCCESS != res)
		OVP_DBG_INFO("res = %lu Unknown error while un-register memory for : %x!\n",
				res, pBufHandle->mem_id);

	return res;
}

TEE_Result VPP_BUFF_Register(VPP_BUFF_HANDLE *pBufHandle) {
	TEE_Result res;
	UINT32 phy_size;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

	phy_size = OVP_SHM_ALIGN_ROUNDUP(pBufHandle->size);
	res = TEE_Register_Memory(pBufHandle->phy_addr, phy_size, &pBufHandle->mem_id);
	if (TEE_SUCCESS != res)
		OVP_DBG_INFO("res = %lu Unknown error while register memory for : %x!\n", res,
				pBufHandle->phy_addr);

	return res;
}

TEE_Result VPP_BUFF_GetPhyAddr(VPP_BUFF_HANDLE *pBufHandle) {
	TEE_Result res;

	if (!pBufHandle)
		return TEE_ERROR_BAD_PARAMETERS;

	res = TEE_GetPhys_By_Memid(pBufHandle->mem_id, &pBufHandle->phy_addr);
	if (res != TEE_SUCCESS)
		OVP_DBG_INFO("Failed to retrieve phy_addr from memid (res=%lx) : memid=%lx, %x = %p\n",
				res, pBufHandle->mem_id, pBufHandle->size, pBufHandle->phy_addr);

	return res;
}
